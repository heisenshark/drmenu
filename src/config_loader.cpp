#include "config_loader.h"
#include "desktop_entry.h"

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

QList<MenuItem> ConfigLoader::loadMenu(const QString &menuName, const QString &configPath) {
    QJsonObject root = loadRoot(configPath);
    if (root.isEmpty()) return {};

    QJsonObject menus = root["menus"].toObject();
    if (!menus.contains(menuName)) {
        QTextStream(stderr) << "drmenu: menu '" << menuName << "' not found in config.\n"
                            << "Available menus: " << menus.keys().join(", ") << "\n";
        return {};
    }

    QJsonObject menu = menus[menuName].toObject();
    QJsonArray itemsArray = menu["items"].toArray();

    QList<MenuItem> items;
    for (const QJsonValue &v : itemsArray) {
        QJsonObject obj = v.toObject();
        MenuItem item;

        if (obj.contains("app")) {
            // Resolve from XDG .desktop file
            item = DesktopEntry::resolve(obj["app"].toString().trimmed());
            // Allow overriding resolved values
            if (obj.contains("label"))   item.label   = obj["label"].toString().trimmed();
            if (obj.contains("command")) item.command = obj["command"].toString().trimmed();
        } else {
            item.label    = obj["label"].toString().trimmed();
            item.icon     = obj["icon"].toString().trimmed();
            item.iconName = obj["iconName"].toString().trimmed();
            item.command  = obj["command"].toString().trimmed();
        }

        if (!item.label.isEmpty())
            items.append(item);
    }

    if (items.isEmpty())
        QTextStream(stderr) << "drmenu: menu '" << menuName << "' has no items.\n";

    return items;
}

QStringList ConfigLoader::availableMenus(const QString &configPath) {
    QJsonObject root = loadRoot(configPath);
    if (root.isEmpty()) return {};
    return root["menus"].toObject().keys();
}
