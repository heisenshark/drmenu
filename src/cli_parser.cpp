#include "cli_parser.h"
#include <QTextStream>
#include <cstdio>

bool CliParser::isDaemonMode(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        QString arg(argv[i]);
        if (arg == "--daemon" || arg == "-d") {
            return true;
        }
    }
    return false;
}

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

QList<MenuItem> CliParser::parseItems(int argc, char *argv[]) {
    QList<MenuItem> items;
    QStringList posArgs;
    for (int i = 1; i < argc; ++i) {
        QString arg(argv[i]);
        if (arg != "--daemon" && arg != "-d") {
            posArgs.append(arg);
        }
    }

    if (!posArgs.isEmpty()) {
        for (const QString &arg : posArgs) {
            items.append(parseEntry(arg));
        }
    } else {
        QTextStream in(stdin);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (!line.isEmpty()) {
                items.append(parseEntry(line));
            }
        }
    }

    return items;
}

QVariantList CliParser::buildModel(const QList<MenuItem> &items) {
    QVariantList list;
    for (const MenuItem &item : items) {
        list.append(item.toVariantMap());
    }
    return list;
}
