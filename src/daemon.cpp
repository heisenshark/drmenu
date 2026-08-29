#include "daemon.h"
#include "cli_parser.h"
#include "config_loader.h"
#include "screen_detector.h"
#include "output_controller.h"
#include "theme_icon_provider.h"
#include "screen_grabber.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QLocalServer>
#include <QLocalSocket>
#include <QScreen>
#include <QProcess>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFile>
#include <QTimer>

#include <LayerShellQt/Window>
#include <chrono>

const QString DaemonServer::SOCKET_NAME = "drmenu-socket";

static QVariantList jsonArrayToVariantList(const QJsonArray &arr) {
    QVariantList list;
    for (const QJsonValue &v : arr) {
        if (v.isObject()) {
            QJsonObject obj = v.toObject();
            QVariantMap itemMap;
            itemMap["label"]       = obj["label"].toString();
            itemMap["icon"]        = obj["icon"].toString();
            itemMap["iconName"]    = obj["iconName"].toString();
            itemMap["command"]     = obj["command"].toString();
            itemMap["submenuName"] = obj["submenuName"].toString();
            itemMap["key"]         = obj["key"].toString();
            if (!itemMap["label"].toString().isEmpty()) {
                list.append(itemMap);
            }
        }
    }
    return list;
}

static QVariantMap jsonObjectToMenuMap(const QJsonObject &mObj) {
    QVariantMap menuMap;
    if (mObj.contains("items"))
        menuMap["items"] = jsonArrayToVariantList(mObj["items"].toArray());
    if (mObj.contains("spawnAtMouse"))
        menuMap["spawnAtMouse"] = mObj["spawnAtMouse"].toBool();
    if (mObj.contains("escapeClosesAll"))
        menuMap["escapeClosesAll"] = mObj["escapeClosesAll"].toBool();
    if (mObj.contains("style"))
        menuMap["style"] = mObj["style"].toObject().toVariantMap();
    return menuMap;
}

int DaemonServer::run(int argc, char *argv[]) {
    if (!qgetenv("HYPRLAND_INSTANCE_SIGNATURE").isEmpty()) {
        QProcess proc;
        proc.setStandardOutputFile(QProcess::nullDevice());
        proc.setStandardErrorFile(QProcess::nullDevice());
        proc.start("hyprctl", {"eval", "hl.layer_rule({ match = { namespace = 'drmenu' }, blur = false, no_anim = true })"});
        proc.waitForFinished(80);
    }

    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setApplicationName("drmenu-daemon");

    QLocalServer::removeServer(SOCKET_NAME);
    QLocalServer server;
    if (!server.listen(SOCKET_NAME)) {
        qWarning() << "Failed to start drmenu daemon:" << server.errorString();
        return 1;
    }
    qDebug() << "[drmenu daemon] Listening on socket:" << SOCKET_NAME;

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

    OutputController output;
    ScreenGrabber screenGrabber;
    engine.rootContext()->setContextProperty("output", &output);
    engine.rootContext()->setContextProperty("screenGrabber", &screenGrabber);

    const QUrl url(QStringLiteral("qrc:/drmenu/src/qml/main.qml"));
    QQuickWindow *window = nullptr;

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [&window, url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) QCoreApplication::exit(-1);

        window = qobject_cast<QQuickWindow*>(obj);
        if (!window) return;

        auto layerWindow = LayerShellQt::Window::get(window);
        if (layerWindow) {
            layerWindow->setScope(QStringLiteral("drmenu"));
            layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);
            layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
            layerWindow->setAnchors(LayerShellQt::Window::Anchors(
                LayerShellQt::Window::AnchorTop    |
                LayerShellQt::Window::AnchorBottom |
                LayerShellQt::Window::AnchorLeft   |
                LayerShellQt::Window::AnchorRight));
            layerWindow->setExclusiveZone(-1);
            QScreen *pri = QGuiApplication::primaryScreen();
            if (pri) layerWindow->setScreen(pri);
        }
        window->create();
    }, Qt::QueuedConnection);

    engine.addImageProvider(QStringLiteral("icon"), new ThemeIconProvider);
    engine.addImageProvider(QStringLiteral("screengrab"), new ScreenGrabProvider(&screenGrabber));
    engine.load(url);
    auto buildCommandMap = [](const QVariantMap &allMenus) {
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
    };

    QVariantMap cachedAllMenus = ConfigLoader::loadAllMenus("");
    QMap<QString, QString> cachedCommandMap = buildCommandMap(cachedAllMenus);

    if (!cachedAllMenus.isEmpty()) {
        QString initialMenu = cachedAllMenus.contains("main") ? "main" : cachedAllMenus.keys().first();
        QVariantMap mData    = cachedAllMenus[initialMenu].toMap();
        bool spawnAtMouse    = mData.contains("spawnAtMouse")    ? mData["spawnAtMouse"].toBool()    : true;
        bool escapeClosesAll = mData.contains("escapeClosesAll") ? mData["escapeClosesAll"].toBool() : false;
        QVariantMap style    = mData["style"].toMap();
        output.setMenuData(cachedAllMenus, initialMenu, spawnAtMouse, escapeClosesAll, style);
    }

    // Force GPU EGL surface & Wayland layer-shell pre-warm on daemon startup
    QTimer::singleShot(150, [&window]() {
        if (window) {
            QObject::connect(window, &QQuickWindow::afterRendering, window, [window]() {
                window->setVisible(false);
                QTextStream(stderr) << "[PRE-WARM] GPU shaders, Wayland surface & EGL context ready!\n";
            }, Qt::SingleShotConnection);
            window->setVisible(true);
        }
    });

    QLocalSocket *activeClientSocket = nullptr;
    QMap<QString, QString> activeCommandMap;

    output.onSelectCallback = [&activeClientSocket, &window, &activeCommandMap, &output](const QString &label) {
        auto it = activeCommandMap.find(label);
        if (it != activeCommandMap.end() && it.value() == "drmenu --media") {
            // In-place dynamic transition: fetch media sources instantly without closing window or dropping Wayland focus!
            QList<MenuItem> mediaItems = CliParser::getMediaItems();
            activeCommandMap.clear();
            for (const MenuItem &item : mediaItems)
                if (!item.command.isEmpty()) activeCommandMap[item.label] = item.command;
            output.setItems(CliParser::buildModel(mediaItems), QVariantMap{});
            return;
        }

        bool hasClient = (activeClientSocket && activeClientSocket->isOpen());
        if (hasClient) {
            activeClientSocket->write(("SELECTED\t" + label + "\n").toUtf8());
            activeClientSocket->flush();
            activeClientSocket->close();
            activeClientSocket = nullptr;
        }

        // Execute in daemon ONLY if no active client socket or if it's a dynamic item (e.g. playerctl)
        if (it != activeCommandMap.end()) {
            bool isDynamicItem = it.value().startsWith("playerctl");
            if (!hasClient || isDynamicItem) {
                QProcess::startDetached("sh", {"-c", it.value()});
            }
        }

        if (window) window->setVisible(false);
        activeCommandMap.clear();
    };

    output.onCancelCallback = [&activeClientSocket, &window, &activeCommandMap]() {
        if (activeClientSocket && activeClientSocket->isOpen()) {
            activeClientSocket->write("CANCELLED\n");
            activeClientSocket->flush();
            activeClientSocket->close();
            activeClientSocket = nullptr;
        }
        if (window) window->setVisible(false);
        activeCommandMap.clear();
    };

    QFileSystemWatcher configWatcher;
    QString userConfigPath = QDir::homePath() + "/.config/drmenu/config.json";
    if (QFile::exists(userConfigPath)) {
        configWatcher.addPath(userConfigPath);
        QObject::connect(&configWatcher, &QFileSystemWatcher::fileChanged, [&cachedAllMenus, &cachedCommandMap, userConfigPath, &configWatcher, &buildCommandMap]() {
            cachedAllMenus   = ConfigLoader::loadAllMenus("");
            cachedCommandMap = buildCommandMap(cachedAllMenus);
            if (QFile::exists(userConfigPath) && !configWatcher.files().contains(userConfigPath)) {
                configWatcher.addPath(userConfigPath);
            }
        });
    }

    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *clientSocket = server.nextPendingConnection();
        if (!clientSocket) return;

        QObject::connect(clientSocket, &QLocalSocket::readyRead,
                         [clientSocket, &window, &output, &activeClientSocket,
                          &activeCommandMap, &buildCommandMap, &cachedAllMenus, &cachedCommandMap]() {
            auto t_start = std::chrono::high_resolution_clock::now();

            // If a menu is already active, cancel the previous client
            if (activeClientSocket && activeClientSocket != clientSocket) {
                if (activeClientSocket->isOpen()) {
                    activeClientSocket->write("CANCELLED\n");
                    activeClientSocket->flush();
                    activeClientSocket->close();
                }
                activeClientSocket = nullptr;
                activeCommandMap.clear();
                if (window) window->setVisible(false);
            }

            QByteArray data = clientSocket->readAll();
            QString rawInput = QString::fromUtf8(data).trimmed();

            QJsonParseError jsonErr;
            QJsonDocument doc = QJsonDocument::fromJson(data, &jsonErr);
            auto t_json = std::chrono::high_resolution_clock::now();

            bool spawnAtMouse = true;
            QVariantMap reqStyle;

            if (jsonErr.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject payload = doc.object();
                QString type = payload["type"].toString();
                spawnAtMouse = payload["spawnAtMouse"].toBool(true);
                bool escapeClosesAll = payload["escapeClosesAll"].toBool(false);

                if (payload.contains("style"))
                    reqStyle = payload["style"].toObject().toVariantMap();

                if (type == "show") {
                    QString initial = payload["menu"].toString();
                    if (initial.isEmpty()) initial = payload["initialMenu"].toString();

                    cachedAllMenus   = ConfigLoader::loadAllMenus("");
                    cachedCommandMap = buildCommandMap(cachedAllMenus);

                    if (cachedAllMenus.contains(initial)) {
                        QVariantMap mData    = cachedAllMenus[initial].toMap();
                        spawnAtMouse         = mData.contains("spawnAtMouse")    ? mData["spawnAtMouse"].toBool()    : true;
                        bool escapeClosesAll = mData.contains("escapeClosesAll") ? mData["escapeClosesAll"].toBool() : false;
                        QVariantMap mStyle   = reqStyle.isEmpty() ? mData["style"].toMap() : reqStyle;

                        activeClientSocket = clientSocket;
                        activeCommandMap   = cachedCommandMap;
                        output.setMenuData(cachedAllMenus, initial, spawnAtMouse, escapeClosesAll, mStyle);
                    } else {
                        clientSocket->write("CANCELLED\n");
                        clientSocket->close();
                        return;
                    }
                } else if (type == "menus") {
                    QVariantMap allMenus;
                    QJsonObject menusObj = payload["menus"].toObject();
                    for (const QString &key : menusObj.keys()) {
                        QJsonValue val = menusObj[key];
                        if (val.isObject()) {
                            allMenus[key] = jsonObjectToMenuMap(val.toObject());
                        } else if (val.isArray()) {
                            QVariantMap m;
                            m["items"] = jsonArrayToVariantList(val.toArray());
                            allMenus[key] = m;
                        }
                    }

                    QString initial = payload["initialMenu"].toString();
                    if (allMenus.isEmpty() || initial.isEmpty()) {
                        clientSocket->write("CANCELLED\n");
                        clientSocket->close();
                        return;
                    }

                    activeClientSocket = clientSocket;
                    activeCommandMap   = buildCommandMap(allMenus);
                    output.setMenuData(allMenus, initial, spawnAtMouse, escapeClosesAll, reqStyle);

                } else {
                    // Inline item list
                    QList<MenuItem> items;
                    for (const QJsonValue &v : payload["items"].toArray()) {
                        QJsonObject obj = v.toObject();
                        MenuItem it;
                        it.label       = obj["label"].toString();
                        it.icon        = obj["icon"].toString();
                        it.iconName    = obj["iconName"].toString();
                        it.command     = obj["command"].toString();
                        it.submenuName = obj["submenuName"].toString();
                        it.key         = obj["key"].toString();
                        if (!it.label.isEmpty()) items.append(it);
                    }
                    if (items.isEmpty()) {
                        clientSocket->write("CANCELLED\n");
                        clientSocket->close();
                        return;
                    }
                    activeClientSocket = clientSocket;
                    for (const MenuItem &item : items)
                        if (!item.command.isEmpty()) activeCommandMap[item.label] = item.command;
                    output.setItems(CliParser::buildModel(items), reqStyle);
                }

            } else {
                // Plain text fallback
                QList<MenuItem> items;
                for (const QString &line : rawInput.split('\n'))
                    if (!line.trimmed().isEmpty())
                        items.append(CliParser::parseEntry(line));
                if (items.isEmpty()) {
                    clientSocket->write("CANCELLED\n");
                    clientSocket->close();
                    return;
                }
                activeClientSocket = clientSocket;
                for (const MenuItem &item : items)
                    if (!item.command.isEmpty()) activeCommandMap[item.label] = item.command;
                output.setItems(CliParser::buildModel(items), reqStyle);
            }

            auto t_model = std::chrono::high_resolution_clock::now();

            // Position the window
            if (window) {
                auto layerWindow = LayerShellQt::Window::get(window);
                TargetScreenInfo targetInfo = spawnAtMouse
                    ? ScreenDetector::getTargetScreenInfo()
                    : TargetScreenInfo{};

                auto t_screen = std::chrono::high_resolution_clock::now();

                QScreen *targetScreen = nullptr;
                if (!targetInfo.monitorName.isEmpty()) {
                    for (QScreen *s : QGuiApplication::screens()) {
                        if (s->name() == targetInfo.monitorName) { targetScreen = s; break; }
                    }
                }
                if (!targetScreen) targetScreen = QGuiApplication::primaryScreen();

                if (layerWindow && targetScreen && layerWindow->screen() != targetScreen) {
                    layerWindow->setScreen(targetScreen);
                }

                if (targetScreen) {
                    int localX = targetInfo.localX < 0 ? targetScreen->geometry().width()  / 2 : targetInfo.localX;
                    int localY = targetInfo.localY < 0 ? targetScreen->geometry().height() / 2 : targetInfo.localY;
                    window->setProperty("menuX", localX);
                    window->setProperty("menuY", localY);
                }

                window->setVisible(true);
                window->raise();
                window->requestActivate();

                auto t_visible = std::chrono::high_resolution_clock::now();

                double ms_json   = std::chrono::duration<double, std::milli>(t_json - t_start).count();
                double ms_model  = std::chrono::duration<double, std::milli>(t_model - t_json).count();
                double ms_screen = std::chrono::duration<double, std::milli>(t_screen - t_model).count();
                double ms_vis    = std::chrono::duration<double, std::milli>(t_visible - t_screen).count();
                double ms_total  = std::chrono::duration<double, std::milli>(t_visible - t_start).count();

                QTextStream(stderr) << QString("[TIMING] JSON Parse: %1 ms | QML Model Update: %2 ms | ScreenDetect: %3 ms | SetVisible: %4 ms | Total C++: %5 ms\n")
                    .arg(ms_json, 0, 'f', 3)
                    .arg(ms_model, 0, 'f', 3)
                    .arg(ms_screen, 0, 'f', 3)
                    .arg(ms_vis, 0, 'f', 3)
                    .arg(ms_total, 0, 'f', 3);

                QObject::connect(window, &QQuickWindow::afterRendering, window, [t_start]() {
                    auto t_render = std::chrono::high_resolution_clock::now();
                    double ms_render = std::chrono::duration<double, std::milli>(t_render - t_start).count();
                    QTextStream(stderr) << QString("[TIMING] First Frame Rendered on Screen: %1 ms\n")
                        .arg(ms_render, 0, 'f', 3);
                }, Qt::SingleShotConnection);
            }
        });
    });

    return app.exec();
}
