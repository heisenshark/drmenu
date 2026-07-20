#pragma once

#include "item.h"
#include <QList>
#include <QVariantMap>
#include <QString>

class StandaloneApp {
public:
    static int run(int argc, char *argv[], const QVariantMap &allMenus,
                   const QString &initialMenu,
                   bool spawnAtMouse = true, bool escapeClosesAll = false);

    static int runItems(int argc, char *argv[], const QList<MenuItem> &items);
};
