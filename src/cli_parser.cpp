#include "cli_parser.h"
#include <QTextStream>
#include <QProcess>
#include <cstdio>

// ── Core entry parser ─────────────────────────────────────────────────────────
MenuItem CliParser::parseEntry(const QString &entry) {
    MenuItem item;
    QStringList tabParts = entry.split('\t');
    if (tabParts.size() >= 2) {
        item.label   = tabParts[0].trimmed();
        item.command = tabParts[1].trimmed();
        if (tabParts.size() >= 3) item.icon     = tabParts[2].trimmed();
        if (tabParts.size() >= 4) item.iconName = tabParts[3].trimmed();
        if (tabParts.size() >= 5) item.key      = tabParts[4].trimmed();
    } else {
        QString labelSide = tabParts.value(0).trimmed();
        int colonIdx = labelSide.indexOf(':');
        if (colonIdx > 0) {
            item.label = labelSide.left(colonIdx).trimmed();
            item.icon  = labelSide.mid(colonIdx + 1).trimmed();
        } else {
            item.label = labelSide;
        }
    }
    return item;
}

QVariantList CliParser::buildModel(const QList<MenuItem> &items) {
    QVariantList list;
    for (const MenuItem &item : items)
        list.append(item.toVariantMap());
    return list;
}

QList<MenuItem> CliParser::getMediaItems() {
    QList<MenuItem> items;

    QProcess listProc;
    listProc.start("playerctl", {"-l"});
    listProc.waitForFinished(500);
    QString listOut = QString::fromUtf8(listProc.readAllStandardOutput()).trimmed();
    QStringList players = listOut.split('\n', Qt::SkipEmptyParts);

    int count = 1;
    QMap<QString, int> appCounts;

    for (const QString &p : players) {
        QString player = p.trimmed();
        if (player.isEmpty()) continue;

        QProcess statusProc;
        statusProc.start("playerctl", {"-p", player, "status"});
        statusProc.waitForFinished(300);
        QString status = QString::fromUtf8(statusProc.readAllStandardOutput()).trimmed();

        QProcess metaProc;
        metaProc.start("playerctl", {"-p", player, "metadata"});
        metaProc.waitForFinished(300);
        QString meta = QString::fromUtf8(metaProc.readAllStandardOutput()).toLower();

        // Also query busctl for process/binary name to disambiguate flatpak/forks
        QProcess busProc;
        busProc.start("sh", {"-c", "busctl --user list 2>/dev/null | grep '" + player + "'"});
        busProc.waitForFinished(300);
        QString busInfo = QString::fromUtf8(busProc.readAllStandardOutput()).toLower();

        QString combined = player.toLower() + " " + meta + " " + busInfo;

        QString appName = "Media Player";
        QString icon = QString::fromUtf8("🎵");
        QString iconName;

        if (combined.contains("zen")) {
            appName  = "Zen Browser";
            icon     = QString::fromUtf8("🌐");
            iconName = "zen-browser";
        } else if (combined.contains("librewolf")) {
            appName  = "LibreWolf";
            icon     = QString::fromUtf8("🐺");
            iconName = "io.gitlab.librewolf-community";
        } else if (combined.contains("waterfox")) {
            appName  = "Waterfox";
            icon     = QString::fromUtf8("🦊");
            iconName = "waterfox";
        } else if (combined.contains("floorp")) {
            appName  = "Floorp";
            icon     = QString::fromUtf8("🌀");
            iconName = "floorp";
        } else if (combined.contains("firefox")) {
            appName  = "Firefox";
            icon     = QString::fromUtf8("🦊");
            iconName = "firefox";
        } else if (combined.contains("spotify")) {
            appName  = "Spotify";
            icon     = QString::fromUtf8("🟢");
            iconName = "spotify";
        } else if (combined.contains("chrome") || combined.contains("chromium")) {
            appName  = "Chrome";
            icon     = QString::fromUtf8("🌐");
            iconName = "google-chrome";
        } else if (combined.contains("mpv")) {
            appName  = "mpv";
            icon     = QString::fromUtf8("🎬");
            iconName = "mpv";
        } else if (combined.contains("vlc")) {
            appName  = "VLC";
            icon     = QString::fromUtf8("🟧");
            iconName = "vlc";
        } else {
            appName  = player.split('.').first();
            if (!appName.isEmpty())
                appName[0] = appName[0].toUpper();
        }

        appCounts[appName]++;

        QString symbol = (status == "Playing") ? QString::fromUtf8("▶") : QString::fromUtf8("⏸");
        QString label  = appName;
        if (appCounts[appName] > 1) {
            label += " " + QString::number(appCounts[appName]);
        }
        label += " [" + symbol + "]";

        MenuItem item;
        item.label    = label;
        item.icon     = icon;
        item.iconName = iconName;
        item.command  = "playerctl -p '" + player + "' play-pause";
        if (count <= 9) item.key = QString::number(count);
        items.append(item);
        count++;
    }

    MenuItem globalToggle;
    globalToggle.label   = "Global Play/Pause";
    globalToggle.icon    = QString::fromUtf8("⏯");
    globalToggle.command = "playerctl play-pause";
    globalToggle.key     = "p";
    items.append(globalToggle);

    MenuItem globalNext;
    globalNext.label   = "Next Track";
    globalNext.icon    = QString::fromUtf8("⏭");
    globalNext.command = "playerctl next";
    globalNext.key     = "n";
    items.append(globalNext);

    MenuItem globalPrev;
    globalPrev.label   = "Previous Track";
    globalPrev.icon    = QString::fromUtf8("⏮");
    globalPrev.command = "playerctl previous";
    globalPrev.key     = "r";
    items.append(globalPrev);

    return items;
}

// ── Full argument parser ──────────────────────────────────────────────────────
ParsedArgs CliParser::parse(int argc, char *argv[]) {
    ParsedArgs args;

    QStringList positional;
    for (int i = 1; i < argc; ++i) {
        QString a(argv[i]);
        if (a == "--daemon" || a == "-d") {
            args.daemonMode = true;
        } else if (a == "--media") {
            args.mediaMode = true;
        } else if (a == "--ping" || a == "-p" || a == "--test-glass") {
            args.pingMode = true;
        } else if ((a == "--menu" || a == "-m") && i + 1 < argc) {
            args.menuName = QString(argv[++i]);
        } else if ((a == "--config" || a == "-c") && i + 1 < argc) {
            args.configPath = QString(argv[++i]);
        } else if (!a.startsWith('-')) {
            positional.append(a);
        }
    }

    if (args.mediaMode) {
        args.items = getMediaItems();
    } else if (!positional.isEmpty()) {
        for (const QString &a : positional)
            args.items.append(parseEntry(a));
    } else if (!args.daemonMode && !args.pingMode && args.menuName.isEmpty()) {
        // stdin items (dmenu style) only if not daemon mode and no --menu flag
        QTextStream in(stdin);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (!line.isEmpty())
                args.items.append(parseEntry(line));
        }
    }

    return args;
}

// ── Legacy convenience helpers ────────────────────────────────────────────────
bool CliParser::isDaemonMode(int argc, char *argv[]) {
    return parse(argc, argv).daemonMode;
}

QList<MenuItem> CliParser::parseItems(int argc, char *argv[]) {
    return parse(argc, argv).items;
}
