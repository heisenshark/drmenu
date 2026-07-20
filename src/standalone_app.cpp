#include "standalone_app.h"
#include "cli_parser.h"
#include "screen_detector.h"
#include "output_controller.h"
#include "theme_icon_provider.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QScreen>
#include <QTextStream>
#include <QProcess>
#include <chrono>

#include <LayerShellQt/Window>

int StandaloneApp::run(int argc, char *argv[], const QList<MenuItem> &items) {
    auto t_start = std::chrono::high_resolution_clock::now();

    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setApplicationName("drmenu");

    TargetScreenInfo targetInfo = ScreenDetector::getTargetScreenInfo();

    QQmlApplicationEngine engine;
    OutputController output;
    output.onSelectCallback = [items](const QString &label) {
        // Print to stdout (dmenu compatible)
        QTextStream(stdout) << label << "\n";
        // Run associated command if defined
        for (const MenuItem &item : items) {
            if (item.label == label && !item.command.isEmpty()) {
                QProcess::startDetached("sh", {"-c", item.command});
                break;
            }
        }
        QCoreApplication::exit(0);
    };
    output.onCancelCallback = []() {
        QCoreApplication::exit(1);
    };

    engine.rootContext()->setContextProperty("output", &output);
    output.setItems(CliParser::buildModel(items));

    const QUrl url(QStringLiteral("qrc:/drmenu/src/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url, targetInfo, t_start](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);

        QQuickWindow *window = qobject_cast<QQuickWindow*>(obj);
        if (!window) return;

        auto layerWindow = LayerShellQt::Window::get(window);
        if (layerWindow) {
            QScreen *targetScreen = nullptr;
            if (!targetInfo.monitorName.isEmpty()) {
                for (QScreen *s : QGuiApplication::screens()) {
                    if (s->name() == targetInfo.monitorName) {
                        targetScreen = s;
                        break;
                    }
                }
            }
            if (!targetScreen)
                targetScreen = QGuiApplication::primaryScreen();

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
                int localX = targetInfo.localX;
                int localY = targetInfo.localY;
                if (localX < 0 || localY < 0) {
                    localX = targetScreen->geometry().width() / 2;
                    localY = targetScreen->geometry().height() / 2;
                }
                window->setProperty("menuX", localX);
                window->setProperty("menuY", localY);
            }
        }
        window->setVisible(true);

        auto t_visible = std::chrono::high_resolution_clock::now();
        double ms_total = std::chrono::duration<double, std::milli>(t_visible - t_start).count();
        QTextStream err(stderr);
        err << "[drmenu standalone launch latency: " << QString::number(ms_total, 'f', 2) << " ms]\n";

    }, Qt::QueuedConnection);

    engine.addImageProvider(QStringLiteral("icon"), new ThemeIconProvider);
    engine.load(url);
    return app.exec();
}
