#include "daemon.h"
#include "cli_parser.h"
#include "screen_detector.h"
#include "output_controller.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QLocalServer>
#include <QLocalSocket>
#include <QScreen>
#include <QDebug>

#include <LayerShellQt/Window>
#include <chrono>

const QString DaemonServer::SOCKET_NAME = "drmenu-socket";

int DaemonServer::run(int argc, char *argv[]) {
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setApplicationName("drmenu-daemon");

    QLocalServer::removeServer(SOCKET_NAME);

    QLocalServer server;
    if (!server.listen(SOCKET_NAME)) {
        qWarning() << "Failed to start drmenu daemon server:" << server.errorString();
        return 1;
    }

    qDebug() << "[drmenu daemon] Started successfully on socket:" << SOCKET_NAME;

    QQmlApplicationEngine engine;
    OutputController output;
    engine.rootContext()->setContextProperty("output", &output);

    const QUrl url(QStringLiteral("qrc:/drmenu/src/qml/main.qml"));
    QQuickWindow *window = nullptr;

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [&window, url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);

        window = qobject_cast<QQuickWindow*>(obj);
        if (!window) return;

        auto layerWindow = LayerShellQt::Window::get(window);
        if (layerWindow) {
            layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);
            layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
            layerWindow->setAnchors(LayerShellQt::Window::Anchors(
                LayerShellQt::Window::AnchorTop    |
                LayerShellQt::Window::AnchorBottom |
                LayerShellQt::Window::AnchorLeft   |
                LayerShellQt::Window::AnchorRight));
            layerWindow->setExclusiveZone(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    QLocalSocket *activeClientSocket = nullptr;

    output.onSelectCallback = [&activeClientSocket, &window](const QString &label) {
        if (activeClientSocket && activeClientSocket->isOpen()) {
            activeClientSocket->write(("SELECTED\t" + label + "\n").toUtf8());
            activeClientSocket->flush();
            activeClientSocket->disconnectFromServer();
            activeClientSocket = nullptr;
        }
        if (window) window->setVisible(false);
    };

    output.onCancelCallback = [&activeClientSocket, &window]() {
        if (activeClientSocket && activeClientSocket->isOpen()) {
            activeClientSocket->write("CANCELLED\n");
            activeClientSocket->flush();
            activeClientSocket->disconnectFromServer();
            activeClientSocket = nullptr;
        }
        if (window) window->setVisible(false);
    };

    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *clientSocket = server.nextPendingConnection();
        if (!clientSocket) return;

        QObject::connect(clientSocket, &QLocalSocket::readyRead, [clientSocket, &window, &output, &activeClientSocket]() {
            auto t_req_start = std::chrono::high_resolution_clock::now();

            QByteArray data = clientSocket->readAll();
            QString rawInput = QString::fromUtf8(data).trimmed();

            QList<MenuItem> items;
            for (const QString &line : rawInput.split('\n')) {
                if (!line.trimmed().isEmpty())
                    items.append(CliParser::parseEntry(line));
            }

            if (items.isEmpty()) {
                clientSocket->write("CANCELLED\n");
                clientSocket->disconnectFromServer();
                return;
            }

            activeClientSocket = clientSocket;
            output.setItems(CliParser::buildModel(items));

            TargetScreenInfo targetInfo = ScreenDetector::getTargetScreenInfo();

            if (window) {
                auto layerWindow = LayerShellQt::Window::get(window);
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

                if (layerWindow && targetScreen)
                    layerWindow->setScreen(targetScreen);

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

                window->setVisible(true);
                window->raise();
                window->requestActivate();
            }

            auto t_req_end = std::chrono::high_resolution_clock::now();
            double ms_daemon_pop = std::chrono::duration<double, std::milli>(t_req_end - t_req_start).count();
            qDebug() << "[drmenu daemon] Request handled & popup visible in:" << ms_daemon_pop << "ms";
        });
    });

    return app.exec();
}
