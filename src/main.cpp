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

// Wayland Layer Shell support headers
#include <LayerShellQt/Window>

// ─── CLI Usage ────────────────────────────────────────────────────────────────
// Items are read from stdin (one per line, like dmenu) OR positional arguments.
//
// Line / argument format:
//   label               → shown as-is, output = label
//   label\tcommand      → tab-separated: label shown, command executed + label printed
//   label:icon          → colon-separated: label shown with emoji icon
//   label:icon\tcommand → both icon and command
//
// On selection → selected label is written to stdout, exit 0
// On Escape    → nothing written,                    exit 1
// ─────────────────────────────────────────────────────────────────────────────

struct MenuItem {
    QString label;
    QString icon;
    QString command; // if empty, just print label to stdout
};

// Parse a single entry string into a MenuItem.
// Supported formats: "label", "label\tcommand", "label:icon", "label:icon\tcommand"
static MenuItem parseEntry(const QString &entry) {
    MenuItem item;
    // Split on tab first to separate label-side from command
    QStringList tabParts = entry.split('\t');
    QString labelSide = tabParts.value(0).trimmed();
    item.command      = tabParts.size() > 1 ? tabParts[1].trimmed() : QString();

    // Split label-side on first ':' to get optional icon
    int colonIdx = labelSide.indexOf(':');
    if (colonIdx > 0) {
        item.label = labelSide.left(colonIdx).trimmed();
        item.icon  = labelSide.mid(colonIdx + 1).trimmed();
    } else {
        item.label = labelSide;
        item.icon  = QString(); // QML will use a default
    }
    return item;
}

// Build QVariantList for QML from parsed MenuItems.
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

// ─── Hyprland screen detection ────────────────────────────────────────────────
static QString hyprlandMonitorUnderCursor() {
    if (qgetenv("HYPRLAND_INSTANCE_SIGNATURE").isEmpty())
        return {};

    QProcess cursorProc;
    cursorProc.start("hyprctl", {"cursorpos"});
    cursorProc.waitForFinished(2000);
    if (cursorProc.exitCode() != 0) return {};

    QString cursorOut = QString::fromUtf8(cursorProc.readAllStandardOutput()).trimmed();
    QStringList parts = cursorOut.split(',');
    if (parts.size() < 2) return {};
    bool okX, okY;
    int cx = parts[0].trimmed().toInt(&okX);
    int cy = parts[1].trimmed().toInt(&okY);
    if (!okX || !okY) return {};

    QProcess monitorsProc;
    monitorsProc.start("hyprctl", {"monitors", "-j"});
    monitorsProc.waitForFinished(2000);
    if (monitorsProc.exitCode() != 0) return {};

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
            if (cx >= mx && cx < mx + mw && cy >= my && cy < my + mh)
                return name;
        }
        pos = blockEnd + 1;
    }
    return {};
}

// ─── Selection output helper ──────────────────────────────────────────────────
// Exposed to QML to print the selected label and exit.
class Output : public QObject {
    Q_OBJECT
public:
    explicit Output(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void select(const QString &label) {
        QTextStream out(stdout);
        out << label << "\n";
        out.flush();
        QCoreApplication::exit(0);
    }

    Q_INVOKABLE void cancel() {
        QCoreApplication::exit(1);
    }
};

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char *argv[]) {
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setApplicationName("drmenu");
    app.setApplicationVersion("0.2.0");

    // ── Parse items ──────────────────────────────────────────────────────────
    QList<MenuItem> items;
    QStringList posArgs = app.arguments().mid(1); // skip argv[0]

    if (!posArgs.isEmpty()) {
        // Items from command-line arguments
        for (const QString &arg : posArgs)
            items.append(parseEntry(arg));
    } else {
        // Items from stdin (dmenu style: one per line)
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
            << "  echo -e 'Item1\\nItem2' | drmenu\n"
            << "  drmenu 'Item1' 'Item2'\n"
            << "  drmenu 'Label:icon\\tcommand' ...\n";
        return 1;
    }

    // ── Detect screen under cursor (before event loop) ────────────────────────
    const QString focusedMonitorName = hyprlandMonitorUnderCursor();

    // ── Set up QML engine ─────────────────────────────────────────────────────
    QQmlApplicationEngine engine;
    Output output;
    engine.rootContext()->setContextProperty("output", &output);
    engine.rootContext()->setContextProperty("menuItems", buildModel(items));

    const QUrl url(QStringLiteral("qrc:/drmenu/src/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url, focusedMonitorName](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);

        QQuickWindow *window = qobject_cast<QQuickWindow*>(obj);
        if (!window) return;

        auto layerWindow = LayerShellQt::Window::get(window);
        if (!layerWindow) return;

        // Match focused monitor name to QScreen* (screens() is populated here)
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

#include "main.moc"
