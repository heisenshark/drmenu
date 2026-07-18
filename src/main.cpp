#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QProcess>
#include <QObject>
#include <QScreen>
#include <QDebug>
#include <QTextStream>
#include <QVariantList>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <chrono>

// Wayland Layer Shell support headers
#include <LayerShellQt/Window>

const QString SOCKET_NAME = "drmenu-socket";

struct MenuItem {
    QString label;
    QString icon;
    QString command;
};

static MenuItem parseEntry(const QString &entry) {
    MenuItem item;
    QStringList tabParts = entry.split('\t');
    QString labelSide = tabParts.value(0).trimmed();
    item.command      = tabParts.size() > 1 ? tabParts[1].trimmed() : QString();

    int colonIdx = labelSide.indexOf(':');
    if (colonIdx > 0) {
        item.label = labelSide.left(colonIdx).trimmed();
        item.icon  = labelSide.mid(colonIdx + 1).trimmed();
    } else {
        item.label = labelSide;
        item.icon  = QString();
    }
    return item;
}

static QVariantList buildModel(const QList<MenuItem> &items) {
    QVariantList list;
    for (const MenuItem &item : items) {
        QVariantMap m;
        m["label"]   = item.label;
        m["icon"]    = item.icon;
        m["command"] = item.command;
        list.append(m);
    }
    return list;
}

struct TargetScreenInfo {
    QString monitorName;
    int localX = -1;
    int localY = -1;
};

static TargetScreenInfo getTargetScreenInfo() {
    TargetScreenInfo info;
    if (qgetenv("HYPRLAND_INSTANCE_SIGNATURE").isEmpty())
        return info;

    QProcess cursorProc;
    cursorProc.start("hyprctl", {"cursorpos"});
    cursorProc.waitForFinished(2000);
    if (cursorProc.exitCode() != 0) return info;

    QString cursorOut = QString::fromUtf8(cursorProc.readAllStandardOutput()).trimmed();
    QStringList parts = cursorOut.split(',');
    if (parts.size() < 2) return info;
    bool okX, okY;
    int cx = parts[0].trimmed().toInt(&okX);
    int cy = parts[1].trimmed().toInt(&okY);
    if (!okX || !okY) return info;

    QProcess monitorsProc;
    monitorsProc.start("hyprctl", {"monitors", "-j"});
    monitorsProc.waitForFinished(2000);
    if (monitorsProc.exitCode() != 0) return info;

    QString json = QString::fromUtf8(monitorsProc.readAllStandardOutput());

    auto extractInt = [](const QString &block, const QString &key) -> int {
        QString searchKey = "\"" + key + "\": ";
        int idx = block.indexOf(searchKey);
        if (idx < 0) return -1;
        int start = idx + searchKey.length();
        int end = start;
        while (end < block.size() && (block[end].isDigit() || block[end] == '-')) ++end;
        return block.mid(start, end - start).toInt();
    };
    auto extractStr = [](const QString &block, const QString &key) -> QString {
        QString searchKey = "\"" + key + "\": \"";
        int idx = block.indexOf(searchKey);
        if (idx < 0) return {};
        int start = idx + searchKey.length();
        int end = block.indexOf('"', start);
        return end > start ? block.mid(start, end - start) : QString();
    };

    int pos = 0;
    while (pos < json.size()) {
        int blockStart = json.indexOf('{', pos);
        if (blockStart < 0) break;
        int depth = 0, blockEnd = blockStart;
        for (int i = blockStart; i < json.size(); ++i) {
            if (json[i] == '{') ++depth;
            else if (json[i] == '}') { --depth; if (depth == 0) { blockEnd = i; break; } }
        }
        QString block = json.mid(blockStart, blockEnd - blockStart + 1);
        QString name = extractStr(block, "name");
        int mx = extractInt(block, "x");
        int my = extractInt(block, "y");
        int mw = extractInt(block, "width");
        int mh = extractInt(block, "height");
        if (!name.isEmpty() && mw > 0 && mh > 0) {
            if (cx >= mx && cx < mx + mw && cy >= my && cy < my + mh) {
                info.monitorName = name;
                info.localX = cx - mx;
                info.localY = cy - my;
                return info;
            }
        }
        pos = blockEnd + 1;
    }
    return info;
}

// ── Output controller exposed to QML ──
class Output : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
public:
    explicit Output(QObject *parent = nullptr) : QObject(parent) {}

    QVariantList items() const { return m_items; }

    void setItems(const QVariantList &items) {
        m_items = items;
        emit itemsChanged();
    }

    std::function<void(const QString &)> onSelectCallback;
    std::function<void()> onCancelCallback;

    Q_INVOKABLE void select(const QString &label) {
        if (onSelectCallback) onSelectCallback(label);
    }

    Q_INVOKABLE void cancel() {
        if (onCancelCallback) onCancelCallback();
    }

Q_SIGNALS:
    void itemsChanged();

private:
    QVariantList m_items;
};

// ── Run Daemon Mode ───────────────────────────────────────────────────────────
static int runDaemon(int argc, char *argv[]) {
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setApplicationName("drmenu-daemon");

    // Remove any stale socket
    QLocalServer::removeServer(SOCKET_NAME);

    QLocalServer server;
    if (!server.listen(SOCKET_NAME)) {
        qWarning() << "Failed to start drmenu daemon server:" << server.errorString();
        return 1;
    }

    qDebug() << "[drmenu daemon] Started successfully on socket:" << SOCKET_NAME;

    QQmlApplicationEngine engine;
    Output output;
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

    // Active client socket currently serving a request
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
                    items.append(parseEntry(line));
            }

            if (items.isEmpty()) {
                clientSocket->write("CANCELLED\n");
                clientSocket->disconnectFromServer();
                return;
            }

            activeClientSocket = clientSocket;

            // Update QML model reactively via Output property
            output.setItems(buildModel(items));

            // Query cursor position
            TargetScreenInfo targetInfo = getTargetScreenInfo();

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

// ── Run Client / Standalone Mode ──────────────────────────────────────────────
int main(int argc, char *argv[]) {
    // Check if --daemon or -d passed
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "--daemon" || QString(argv[i]) == "-d") {
            return runDaemon(argc, argv);
        }
    }

    // Read input items from args or stdin
    QList<MenuItem> items;
    QStringList posArgs;
    for (int i = 1; i < argc; ++i)
        posArgs.append(argv[i]);

    if (!posArgs.isEmpty()) {
        for (const QString &arg : posArgs)
            items.append(parseEntry(arg));
    } else {
        QTextStream in(stdin);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (!line.isEmpty())
                items.append(parseEntry(line));
        }
    }

    if (items.isEmpty()) {
        QTextStream err(stderr);
        err << "drmenu: no items provided.\n"
            << "Usage:\n"
            << "  drmenu --daemon              (start background daemon)\n"
            << "  echo -e 'Item1\\nItem2' | drmenu\n"
            << "  drmenu 'Item1' 'Item2'\n";
        return 1;
    }

    // Try connecting to running daemon socket
    QLocalSocket socket;
    socket.connectToServer(SOCKET_NAME);
    if (socket.waitForConnected(50)) {
        // Daemon is running! Fast client path (< 5ms)
        auto t_client_start = std::chrono::high_resolution_clock::now();

        QString payload;
        for (const MenuItem &item : items) {
            payload += item.label;
            if (!item.icon.isEmpty()) payload += ":" + item.icon;
            if (!item.command.isEmpty()) payload += "\t" + item.command;
            payload += "\n";
        }
        socket.write(payload.toUtf8());
        socket.flush();

        if (socket.waitForReadyRead(30000)) {
            QByteArray response = socket.readAll().trimmed();
            QString respStr = QString::fromUtf8(response);

            auto t_client_end = std::chrono::high_resolution_clock::now();
            double ms_client = std::chrono::duration<double, std::milli>(t_client_end - t_client_start).count();

            if (respStr.startsWith("SELECTED\t")) {
                QString selected = respStr.mid(9);
                QTextStream out(stdout);
                out << selected << "\n";
                out.flush();

                QTextStream err(stderr);
                err << "[drmenu daemon client latency: " << QString::number(ms_client, 'f', 2) << " ms]\n";
                return 0;
            }
        }
        return 1; // Cancelled
    }

    // Fallback: Run Standalone inline if daemon is not active
    auto t_start = std::chrono::high_resolution_clock::now();

    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setApplicationName("drmenu");

    TargetScreenInfo targetInfo = getTargetScreenInfo();

    QQmlApplicationEngine engine;
    Output output;
    output.onSelectCallback = [](const QString &label) {
        QTextStream out(stdout);
        out << label << "\n";
        out.flush();
        QCoreApplication::exit(0);
    };
    output.onCancelCallback = []() {
        QCoreApplication::exit(1);
    };

    engine.rootContext()->setContextProperty("output", &output);
    output.setItems(buildModel(items));

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

    engine.load(url);
    return app.exec();
}

#include "main.moc"
