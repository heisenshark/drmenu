#include "cli_parser.h"
#include <QTextStream>
#include <cstdio>

// ── Core entry parser ─────────────────────────────────────────────────────────
MenuItem CliParser::parseEntry(const QString &entry) {
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

QVariantList CliParser::buildModel(const QList<MenuItem> &items) {
    QVariantList list;
    for (const MenuItem &item : items)
        list.append(item.toVariantMap());
    return list;
}

// ── Full argument parser ──────────────────────────────────────────────────────
ParsedArgs CliParser::parse(int argc, char *argv[]) {
    ParsedArgs args;

    QStringList positional;
    for (int i = 1; i < argc; ++i) {
        QString a(argv[i]);
        if (a == "--daemon" || a == "-d") {
            args.daemonMode = true;
        } else if ((a == "--menu" || a == "-m") && i + 1 < argc) {
            args.menuName = QString(argv[++i]);
        } else if ((a == "--config" || a == "-c") && i + 1 < argc) {
            args.configPath = QString(argv[++i]);
        } else if (!a.startsWith('-')) {
            positional.append(a);
        }
    }

    if (!positional.isEmpty()) {
        for (const QString &a : positional)
            args.items.append(parseEntry(a));
    } else if (args.menuName.isEmpty()) {
        // stdin items (dmenu style) only if no --menu flag
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
