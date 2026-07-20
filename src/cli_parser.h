#pragma once

#include "item.h"
#include <QString>
#include <QStringList>
#include <QList>
#include <QVariantList>

struct ParsedArgs {
    bool        daemonMode  = false;
    QString     menuName;       // --menu <name>
    QString     configPath;     // --config <path>
    QList<MenuItem> items;      // positional args or stdin
};

class CliParser {
public:
    static ParsedArgs    parse(int argc, char *argv[]);

    // Convenience helpers (used internally and from main.cpp)
    static bool          isDaemonMode(int argc, char *argv[]);
    static QList<MenuItem> parseItems(int argc, char *argv[]);
    static MenuItem      parseEntry(const QString &entry);
    static QVariantList  buildModel(const QList<MenuItem> &items);
};
