#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QProcess>
#include <QObject>
#include <QScreen>
#include <QDebug>

// Wayland Layer Shell support headers
#include <LayerShellQt/Window>

// A simple utility class to spawn detached processes from within QML
class ProcessLauncher : public QObject {
    Q_OBJECT
public:
    explicit ProcessLauncher(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void launch(const QString &command) {
        QStringList args = QProcess::splitCommand(command);
        if (!args.isEmpty()) {
            QString program = args.takeFirst();
            QProcess::startDetached(program, args);
        }
    }
};

// Query hyprctl to get the name of the monitor currently under the mouse cursor.
// Uses "hyprctl cursorpos" for cursor coords + "hyprctl monitors -j" for geometry.
// Returns empty string if not on Hyprland or on failure.
static QString hyprlandMonitorUnderCursor() {
    if (qgetenv("HYPRLAND_INSTANCE_SIGNATURE").isEmpty())
        return {};

    // Step 1: get cursor position
    QProcess cursorProc;
    cursorProc.start("hyprctl", {"cursorpos"});
    cursorProc.waitForFinished(2000);
    if (cursorProc.exitCode() != 0)
        return {};

    // Output format: "X, Y"
    QString cursorOut = QString::fromUtf8(cursorProc.readAllStandardOutput()).trimmed();
    QStringList parts = cursorOut.split(',');
    if (parts.size() < 2)
        return {};
    bool okX, okY;
    int cx = parts[0].trimmed().toInt(&okX);
    int cy = parts[1].trimmed().toInt(&okY);
    if (!okX || !okY)
        return {};

    // Step 2: get monitor list and find which one contains the cursor
    QProcess monitorsProc;
    monitorsProc.start("hyprctl", {"monitors", "-j"});
    monitorsProc.waitForFinished(2000);
    if (monitorsProc.exitCode() != 0)
        return {};

    QString json = QString::fromUtf8(monitorsProc.readAllStandardOutput());

    // Parse each monitor's x, y, width, height and name
    // JSON format: [{"name":"DP-1","x":1920,"y":0,"width":1920,"height":1080,...}, ...]
    // Simple line-by-line parse: collect name, x, y, width, height per block
    QString bestName;
    int pos = 0;
    while (pos < json.size()) {
        int blockStart = json.indexOf('{', pos);
        if (blockStart < 0) break;
        // Find the matching closing brace (top-level monitor object)
        int depth = 0, blockEnd = blockStart;
        for (int i = blockStart; i < json.size(); ++i) {
            if (json[i] == '{') ++depth;
            else if (json[i] == '}') { --depth; if (depth == 0) { blockEnd = i; break; } }
        }
        QString block = json.mid(blockStart, blockEnd - blockStart + 1);

        // Extract fields using simple string search
        auto extractInt = [&](const QString &key) -> int {
            QString searchKey = "\"" + key + "\": ";
            int idx = block.indexOf(searchKey);
            if (idx < 0) return -1;
            int start = idx + searchKey.length();
            int end = start;
            while (end < block.size() && (block[end].isDigit() || block[end] == '-')) ++end;
            return block.mid(start, end - start).toInt();
        };
        auto extractStr = [&](const QString &key) -> QString {
            QString searchKey = "\"" + key + "\": \"";
            int idx = block.indexOf(searchKey);
            if (idx < 0) return {};
            int start = idx + searchKey.length();
            int end = block.indexOf('"', start);
            return end > start ? block.mid(start, end - start) : QString();
        };

        QString name = extractStr("name");
        int mx = extractInt("x");
        int my = extractInt("y");
        int mw = extractInt("width");
        int mh = extractInt("height");

        if (!name.isEmpty() && mx >= 0 && mw > 0 && mh > 0) {
            if (cx >= mx && cx < mx + mw && cy >= my && cy < my + mh) {
                bestName = name;
                break;
            }
        }

        pos = blockEnd + 1;
    }

    return bestName;
}

int main(int argc, char *argv[]) {
    // Force Wayland client to support transparent background layers
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setApplicationName("drmenu");
    app.setApplicationDisplayName("drMenu Radial Launcher");
    app.setApplicationVersion("0.1.0");

    // Query hyprctl cursorpos + monitors to find the screen under the mouse.
    // Done here (before event loop) because hyprctl is an external process.
    // Screen matching happens in objectCreated where Qt's Wayland connection is live.
    const QString focusedMonitorName = hyprlandMonitorUnderCursor();
    qDebug() << "[drmenu] Focused monitor from hyprctl:" << focusedMonitorName;

    QQmlApplicationEngine engine;
    ProcessLauncher launcher;
    engine.rootContext()->setContextProperty("launcher", &launcher);

    const QUrl url(QStringLiteral("qrc:/drmenu/src/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url, focusedMonitorName](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);

        QQuickWindow *window = qobject_cast<QQuickWindow*>(obj);
        if (!window) return;

        auto layerWindow = LayerShellQt::Window::get(window);
        if (!layerWindow) return;

        // At this point Qt's Wayland connection is live - screens() is populated.
        // Match the focused monitor name to a QScreen* and assign it via LayerShellQt.
        qDebug() << "[drmenu] Qt screens at objectCreated:";
        for (QScreen *s : QGuiApplication::screens())
            qDebug() << " " << s->name() << s->geometry();

        QScreen *targetScreen = nullptr;
        if (!focusedMonitorName.isEmpty()) {
            for (QScreen *s : QGuiApplication::screens()) {
                if (s->name() == focusedMonitorName) {
                    targetScreen = s;
                    break;
                }
            }
        }
        if (!targetScreen)
            targetScreen = QGuiApplication::primaryScreen();

        qDebug() << "[drmenu] Using screen:" << (targetScreen ? targetScreen->name() : "null");

        // Set the wl_output via LayerShellQt before making window visible
        layerWindow->setScreen(targetScreen);
        layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);
        layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
        layerWindow->setAnchors(LayerShellQt::Window::Anchors(
            LayerShellQt::Window::AnchorTop    |
            LayerShellQt::Window::AnchorBottom |
            LayerShellQt::Window::AnchorLeft   |
            LayerShellQt::Window::AnchorRight));
        layerWindow->setExclusiveZone(-1);

        window->setVisible(true);

    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}

// Needed because ProcessLauncher (a QObject subclass) is defined in this .cpp file.
#include "main.moc"
