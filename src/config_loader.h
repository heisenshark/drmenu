#pragma once

#include "item.h"
#include <QString>
#include <QList>

class ConfigLoader {
public:
    // Default config path: ~/.config/drmenu/config.json
    static QString defaultConfigPath();

    // Load a named menu from the config file.
    // Returns an empty list if the file or menu name is not found.
    static QList<MenuItem> loadMenu(const QString &menuName,
                                    const QString &configPath = {});

    // List all named menus available in the config file.
    static QStringList availableMenus(const QString &configPath = {});
};
