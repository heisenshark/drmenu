#include "standalone_app.h"
#include "cli_parser.h"
#include "screen_detector.h"
#include "output_controller.h"
#include "theme_icon_provider.h"
#include "screen_grabber.h"
#include "hypr_shader.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QScreen>
#include <QTextStream>
#include <QProcess>
#include <QLockFile>
#include <QDir>
#include <csignal>

#include <LayerShellQt/Window>

static int runInternal(int argc, char *argv[], OutputController &output, bool spawnAtMouse) {
    QString lockPath = QDir::tempPath() + "/drmenu-standalone.lock";
    QLockFile lockFile(lockPath);
    lockFile.setStaleLockTime(5000);

    if (!lockFile.tryLock(50)) {
        // Another instance is already open; disable creating a new one
        return 0;
    }

    auto t_start = std::chrono::high_resolution_clock::now();

    if (!qgetenv("HYPRLAND_INSTANCE_SIGNATURE").isEmpty()) {
        bool enableLayerBlur = false;
        QVariantMap s = output.style();
        if (s.contains("layerBlur") || s.contains("layer_blur")) {
            enableLayerBlur = s.value("layerBlur", s.value("layer_blur")).toBool();
        }
        QString blurStr = enableLayerBlur ? "true" : "false";
        QString cmd = QString("hl.layer_rule({ match = { namespace = 'drmenu' }, blur = %1, ignore_alpha = 0.15, no_anim = true })").arg(blurStr);
        QProcess proc;
        proc.setStandardOutputFile(QProcess::nullDevice());
        proc.setStandardErrorFile(QProcess::nullDevice());
        proc.start("hyprctl", {"eval", cmd});
        proc.waitForFinished(80);
    }

    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    static auto signalHandler = [](int sig) {
        HyprlandGlassShader::deactivate();
        std::_Exit(128 + sig);
    };
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGHUP, signalHandler);

    QGuiApplication app(argc, argv);
    app.setApplicationName("drmenu");

    TargetScreenInfo targetInfo = spawnAtMouse
        ? ScreenDetector::getTargetScreenInfo()
        : TargetScreenInfo{};

    QQmlApplicationEngine engine;
    engine.addImportPath("/run/current-system/sw/lib/qt-6/qml");
    engine.addImportPath(QDir::homePath() + "/.nix-profile/lib/qt-6/qml");
    engine.addImportPath(QDir::homePath() + "/.local/state/nix/profile/lib/qt-6/qml");
    engine.addImportPath("/etc/profiles/per-user/" + qgetenv("USER") + "/lib/qt-6/qml");
    const char* nixQml = getenv("NIXPKGS_QML_SEARCH_PATHS");
    if (nixQml && nixQml[0]) {
        for (const QString &p : QString(nixQml).split(':', Qt::SkipEmptyParts)) {
            engine.addImportPath(p);
        }
    }
    const char* envQml = getenv("QML2_IMPORT_PATH");
    if (envQml && envQml[0]) {
        for (const QString &p : QString(envQml).split(':', Qt::SkipEmptyParts)) {
            engine.addImportPath(p);
        }
    }

    ScreenGrabber screenGrabber;
    engine.rootContext()->setContextProperty("output", &output);
    engine.rootContext()->setContextProperty("screenGrabber", &screenGrabber);

    const QUrl url(QStringLiteral("qrc:/drmenu/src/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url, targetInfo, t_start](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) QCoreApplication::exit(-1);

        QQuickWindow *window = qobject_cast<QQuickWindow*>(obj);
        if (!window) return;

        auto layerWindow = LayerShellQt::Window::get(window);
        if (layerWindow) {
            layerWindow->setScope(QStringLiteral("drmenu"));

            QScreen *targetScreen = nullptr;
            if (!targetInfo.monitorName.isEmpty()) {
                for (QScreen *s : QGuiApplication::screens()) {
                    if (s->name() == targetInfo.monitorName) { targetScreen = s; break; }
                }
            }
            if (!targetScreen) targetScreen = QGuiApplication::primaryScreen();
            if (targetScreen) layerWindow->setScreen(targetScreen);

            layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);
            layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
            layerWindow->setAnchors(LayerShellQt::Window::Anchors(
                LayerShellQt::Window::AnchorTop    |
                LayerShellQt::Window::AnchorBottom |
                LayerShellQt::Window::AnchorLeft   |
                LayerShellQt::Window::AnchorRight));
            layerWindow->setExclusiveZone(-1);

            if (targetScreen) {
                int localX = targetInfo.localX < 0 ? targetScreen->geometry().width()  / 2 : targetInfo.localX;
                int localY = targetInfo.localY < 0 ? targetScreen->geometry().height() / 2 : targetInfo.localY;
                window->setProperty("menuX", localX);
                window->setProperty("menuY", localY);
            }
        }
        window->setVisible(true);

        auto t_visible = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t_visible - t_start).count();
        QTextStream(stderr) << "[drmenu standalone: " << QString::number(ms, 'f', 2) << " ms]\n";
    }, Qt::QueuedConnection);

    engine.addImageProvider(QStringLiteral("icon"), new ThemeIconProvider);
    engine.addImageProvider(QStringLiteral("screengrab"), new ScreenGrabProvider(&screenGrabber));
    engine.load(url);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        HyprlandGlassShader::deactivate();
    });

    return app.exec();
}

static QMap<QString, QString> buildCommandMap(const QVariantMap &allMenus) {
    QMap<QString, QString> map;
    for (auto it = allMenus.cbegin(); it != allMenus.cend(); ++it) {
        QVariant val = it.value();
        QVariantList itemList = (val.typeId() == QMetaType::QVariantMap)
            ? val.toMap()["items"].toList()
            : val.toList();

        for (const QVariant &v : itemList) {
            QVariantMap item = v.toMap();
            QString label   = item["label"].toString();
            QString command = item["command"].toString();
            if (!label.isEmpty() && !command.isEmpty())
                map[label] = command;
        }
    }
    return map;
}

// ── Config / nested menus mode ────────────────────────────────────────────────
int StandaloneApp::run(int argc, char *argv[], const QVariantMap &allMenus,
                       const QString &initialMenu, bool spawnAtMouse, bool escapeClosesAll,
                       const QVariantMap &style) {
    OutputController output;
    QMap<QString, QString> cmdMap = buildCommandMap(allMenus);

    output.onSelectCallback = [cmdMap](const QString &label) {
        HyprlandGlassShader::deactivate();
        QTextStream out(stdout);
        out << label << Qt::endl;
        auto it = cmdMap.find(label);
        if (it != cmdMap.end())
            QProcess::startDetached("sh", {"-c", it.value()});
        QCoreApplication::exit(0);
    };
    output.onCancelCallback = []() {
        HyprlandGlassShader::deactivate();
        QCoreApplication::exit(1);
    };
    output.setMenuData(allMenus, initialMenu, spawnAtMouse, escapeClosesAll, style);
    return runInternal(argc, argv, output, spawnAtMouse);
}

// ── Inline / stdin mode ───────────────────────────────────────────────────────
int StandaloneApp::runItems(int argc, char *argv[], const QList<MenuItem> &items,
                            const QVariantMap &style) {
    OutputController output;
    output.onSelectCallback = [items](const QString &label) {
        HyprlandGlassShader::deactivate();
        QTextStream out(stdout);
        out << label << Qt::endl;
        for (const MenuItem &item : items) {
            if (item.label == label && !item.command.isEmpty()) {
                QProcess::startDetached("sh", {"-c", item.command});
                break;
            }
        }
        QCoreApplication::exit(0);
    };
    output.onCancelCallback = []() {
        HyprlandGlassShader::deactivate();
        QCoreApplication::exit(1);
    };
    output.setItems(CliParser::buildModel(items), style);
    return runInternal(argc, argv, output, true);
}
