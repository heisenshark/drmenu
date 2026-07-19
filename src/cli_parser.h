#pragma once

#include "item.h"
#include <QStringList>
#include <QList>
#include <QVariantList>

class CliParser {
public:
    static bool isDaemonMode(int argc, char *argv[]);
    static QList<MenuItem> parseItems(int argc, char *argv[]);
    static MenuItem parseEntry(const QString &entry);
    static QVariantList buildModel(const QList<MenuItem> &items);
};
