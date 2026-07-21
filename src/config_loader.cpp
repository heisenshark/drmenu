#include "config_loader.h"
#include "desktop_entry.h"
#include "cli_parser.h"
#include "theme_manager.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>

QString ConfigLoader::defaultConfigPath() {
    return QDir::homePath() + "/.config/drmenu/config.json";
}

static QJsonObject loadRoot(const QString &configPath) {
    QString path = configPath.isEmpty() ? ConfigLoader::defaultConfigPath() : configPath;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QTextStream(stderr) << "drmenu: cannot open config: " << path << "\n";
        return {};
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        QTextStream(stderr) << "drmenu: JSON parse error in " << path
                            << ": " << err.errorString() << "\n";
        return {};
    }
    return doc.object();
}

static MenuItem itemFromJson(const QJsonObject &obj) {
    MenuItem item;

    if (obj.contains("app")) {
        item = DesktopEntry::resolve(obj["app"].toString().trimmed());
        if (obj.contains("label"))   item.label   = obj["label"].toString().trimmed();
        if (obj.contains("command")) item.command = obj["command"].toString().trimmed();
    } else if (obj.contains("submenu")) {
        // Submenu navigation item
        item.submenuName = obj["submenu"].toString().trimmed();
        item.label       = obj.contains("label") ? obj["label"].toString().trimmed()
                                                  : item.submenuName;
        item.icon        = obj["icon"].toString().trimmed();
        item.iconName    = obj["iconName"].toString().trimmed();
    } else {
        item.label    = obj["label"].toString().trimmed();
        item.icon     = obj["icon"].toString().trimmed();
        item.iconName = obj["iconName"].toString().trimmed();
        item.command  = obj["command"].toString().trimmed();
    }

    if (obj.contains("key")) {
        item.key = obj["key"].toString().trimmed();
    }

    return item;
}

QList<MenuItem> ConfigLoader::loadMenu(const QString &menuName, const QString &configPath) {
    QJsonObject root = loadRoot(configPath);
    if (root.isEmpty()) return {};

    QJsonObject menus = root["menus"].toObject();
    if (!menus.contains(menuName)) {
        QTextStream(stderr) << "drmenu: menu '" << menuName << "' not found.\n"
                            << "Available: " << menus.keys().join(", ") << "\n";
        return {};
    }

    QJsonArray itemsArray = menus[menuName].toObject()["items"].toArray();
    QList<MenuItem> items;
    for (const QJsonValue &v : itemsArray) {
        MenuItem item = itemFromJson(v.toObject());
        if (!item.label.isEmpty())
            items.append(item);
    }
    return items;
}

QVariantMap ConfigLoader::loadStyle(const QString &menuName, const QString &configPath) {
    QJsonObject root = loadRoot(configPath);
    if (root.isEmpty()) return ThemeManager::resolveStyle("blender");

    QString theme = root.contains("theme") ? root["theme"].toString() : "blender";
    QVariantMap styleOverrides = root.contains("style") ? root["style"].toObject().toVariantMap() : QVariantMap{};

    if (!menuName.isEmpty() && root.contains("menus")) {
        QJsonObject menuObj = root["menus"].toObject()[menuName].toObject();
        if (menuObj.contains("theme"))
            theme = menuObj["theme"].toString();
        if (menuObj.contains("style")) {
            QVariantMap menuStyle = menuObj["style"].toObject().toVariantMap();
            for (auto it = menuStyle.cbegin(); it != menuStyle.cend(); ++it)
                styleOverrides[it.key()] = it.value();
        }
    }

    return ThemeManager::resolveStyle(theme, styleOverrides);
}

QVariantMap ConfigLoader::loadAllMenus(const QString &configPath) {
    QJsonObject root = loadRoot(configPath);
    if (root.isEmpty()) return {};

    bool globalSpawnAtMouse    = root.contains("spawnAtMouse")    ? root["spawnAtMouse"].toBool(true)    : true;
    bool globalEscapeClosesAll = root.contains("escapeClosesAll") ? root["escapeClosesAll"].toBool(false) : false;

    QJsonObject menusJson = root["menus"].toObject();
    QVariantMap result;

    for (const QString &menuName : menusJson.keys()) {
        QJsonObject menuObj = menusJson[menuName].toObject();
        QJsonArray itemsArray = menuObj["items"].toArray();

        QVariantList itemList;
        for (const QJsonValue &v : itemsArray) {
            MenuItem item = itemFromJson(v.toObject());
            if (!item.label.isEmpty())
                itemList.append(item.toVariantMap());
        }

        bool spawnAtMouse    = menuObj.contains("spawnAtMouse")    ? menuObj["spawnAtMouse"].toBool(true)    : globalSpawnAtMouse;
        bool escapeClosesAll = menuObj.contains("escapeClosesAll") ? menuObj["escapeClosesAll"].toBool(false) : globalEscapeClosesAll;

        QVariantMap menuStyle = loadStyle(menuName, configPath);

        QVariantMap menuData;
        menuData["items"]           = itemList;
        menuData["spawnAtMouse"]    = spawnAtMouse;
        menuData["escapeClosesAll"] = escapeClosesAll;
        menuData["style"]           = menuStyle;

        result[menuName] = menuData;
    }

    return result;
}

QStringList ConfigLoader::availableMenus(const QString &configPath) {
    QJsonObject root = loadRoot(configPath);
    if (root.isEmpty()) return {};
    return root["menus"].toObject().keys();
}

ConfigLoader::MenuOptions ConfigLoader::loadMenuOptions(const QString &menuName,
                                                         const QString &configPath) {
    MenuOptions opts;
    QJsonObject root = loadRoot(configPath);
    if (root.isEmpty()) return opts;

    // Global defaults
    if (root.contains("spawnAtMouse"))    opts.spawnAtMouse    = root["spawnAtMouse"].toBool(true);
    if (root.contains("escapeClosesAll")) opts.escapeClosesAll = root["escapeClosesAll"].toBool(false);

    // Per-menu overrides
    QJsonObject menuObj = root["menus"].toObject()[menuName].toObject();
    if (menuObj.contains("spawnAtMouse"))    opts.spawnAtMouse    = menuObj["spawnAtMouse"].toBool(true);
    if (menuObj.contains("escapeClosesAll")) opts.escapeClosesAll = menuObj["escapeClosesAll"].toBool(false);

    return opts;
}
