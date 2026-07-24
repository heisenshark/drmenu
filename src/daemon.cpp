#include "daemon.h"
#include "cli_parser.h"
#include "config_loader.h"
#include "screen_detector.h"
#include "output_controller.h"
#include "theme_icon_provider.h"

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
        proc.start("hyprctl", {"eval", "layerrule = noanim, drmenu"});
        proc.waitForFinished(100);
    }

    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
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
    OutputController output;
    engine.rootContext()->setContextProperty("output", &output);

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
        }
        window->create();
    }, Qt::QueuedConnection);

    engine.addImageProvider(QStringLiteral("icon"), new ThemeIconProvider);
    engine.load(url);

    QLocalSocket *activeClientSocket = nullptr;
    QMap<QString, QString> activeCommandMap;

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

    output.onSelectCallback = [&activeClientSocket, &window, &activeCommandMap](const QString &label) {
        if (activeClientSocket && activeClientSocket->isOpen()) {
            activeClientSocket->write(("SELECTED\t" + label + "\n").toUtf8());
            activeClientSocket->flush();
            activeClientSocket->close();
            activeClientSocket = nullptr;
        } else {
            auto it = activeCommandMap.find(label);
            if (it != activeCommandMap.end())
                QProcess::startDetached("sh", {"-c", it.value()});
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

    QVariantMap cachedAllMenus = ConfigLoader::loadAllMenus("");
    QFileSystemWatcher configWatcher;
    QString userConfigPath = QDir::homePath() + "/.config/drmenu/config.json";
    if (QFile::exists(userConfigPath)) {
        configWatcher.addPath(userConfigPath);
        QObject::connect(&configWatcher, &QFileSystemWatcher::fileChanged, [&cachedAllMenus, userConfigPath, &configWatcher]() {
            cachedAllMenus = ConfigLoader::loadAllMenus("");
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
                          &activeCommandMap, &buildCommandMap, &cachedAllMenus]() {
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

                    if (cachedAllMenus.isEmpty()) {
                        cachedAllMenus = ConfigLoader::loadAllMenus("");
                    }

                    if (cachedAllMenus.contains(initial)) {
                        ConfigLoader::MenuOptions opts = ConfigLoader::loadMenuOptions(initial, "");
                        QVariantMap mStyle = reqStyle.isEmpty() ? ConfigLoader::loadStyle(initial, "") : reqStyle;

                        activeClientSocket = clientSocket;
                        activeCommandMap   = buildCommandMap(cachedAllMenus);
                        output.setMenuData(cachedAllMenus, initial, opts.spawnAtMouse, opts.escapeClosesAll, mStyle);
                        spawnAtMouse = opts.spawnAtMouse;
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

            // Position the window
            if (window) {
                auto layerWindow = LayerShellQt::Window::get(window);
                TargetScreenInfo targetInfo = spawnAtMouse
                    ? ScreenDetector::getTargetScreenInfo()
                    : TargetScreenInfo{};

                QScreen *targetScreen = nullptr;
                if (!targetInfo.monitorName.isEmpty()) {
                    for (QScreen *s : QGuiApplication::screens()) {
                        if (s->name() == targetInfo.monitorName) { targetScreen = s; break; }
                    }
                }
                if (!targetScreen) targetScreen = QGuiApplication::primaryScreen();

                if (layerWindow && targetScreen) layerWindow->setScreen(targetScreen);

                if (targetScreen) {
                    int localX = targetInfo.localX < 0 ? targetScreen->geometry().width()  / 2 : targetInfo.localX;
                    int localY = targetInfo.localY < 0 ? targetScreen->geometry().height() / 2 : targetInfo.localY;
                    window->setProperty("menuX", localX);
                    window->setProperty("menuY", localY);
                }

                window->setVisible(true);
                window->raise();
                window->requestActivate();
            }

            auto t_end = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
            qDebug() << "[drmenu daemon] popup in:" << ms << "ms";
        });
    });

    return app.exec();
}
