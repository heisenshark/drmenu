#pragma once

#include "item.h"
#include <QList>
#include <QVariantMap>
#include <QString>

class ClientRunner {
public:
    static bool tryRun(const QList<MenuItem> &items);
    static bool tryRunMenus(const QVariantMap &allMenus, const QString &initialMenu,
                            bool spawnAtMouse = true, bool escapeClosesAll = false);
};
