#pragma once

#include "item.h"
#include <QString>
#include <QList>
#include <QVariantMap>

class ConfigLoader {
public:
    struct MenuOptions {
        bool spawnAtMouse    = true;   // spawn at cursor position vs screen center
        bool escapeClosesAll = false;  // Escape closes whole stack vs go back one level
    };

    static QString defaultConfigPath();

    // Load a named menu's items
    static QList<MenuItem> loadMenu(const QString &menuName,
                                    const QString &configPath = {});

    // Load ALL menus as QVariantMap (for nested navigation)
    static QVariantMap loadAllMenus(const QString &configPath = {});

    // Load display/behavior options for a named menu
    static MenuOptions loadMenuOptions(const QString &menuName,
                                       const QString &configPath = {});

    static QStringList availableMenus(const QString &configPath = {});
};
