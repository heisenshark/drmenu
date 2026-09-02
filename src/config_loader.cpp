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
    QString jsoncPath = QDir::homePath() + "/.config/drmenu/config.jsonc";
    if (QFile::exists(jsoncPath))
        return jsoncPath;
    return QDir::homePath() + "/.config/drmenu/config.json";
}

static QByteArray cleanJsonc(const QByteArray &raw) {
    // Pass 1: Strip single-line (//) and multi-line (/* */) comments outside strings
    QByteArray noComments;
    noComments.reserve(raw.size());
    bool inString = false;
    bool inLineComment = false;
    bool inBlockComment = false;
    bool escaped = false;

    for (int i = 0; i < raw.size(); ++i) {
        char c = raw[i];
        char next = (i + 1 < raw.size()) ? raw[i + 1] : '\0';

        if (inLineComment) {
            if (c == '\n') {
                inLineComment = false;
                noComments.append('\n');
            } else {
                noComments.append(' ');
            }
            continue;
        }

        if (inBlockComment) {
            if (c == '*' && next == '/') {
                inBlockComment = false;
                noComments.append("  ");
                ++i;
            } else if (c == '\n') {
                noComments.append('\n');
            } else {
                noComments.append(' ');
            }
            continue;
        }

        if (inString) {
            noComments.append(c);
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
            escaped = false;
            noComments.append(c);
        } else if (c == '/' && next == '/') {
            inLineComment = true;
            noComments.append("  ");
            ++i;
        } else if (c == '/' && next == '*') {
            inBlockComment = true;
            noComments.append("  ");
            ++i;
        } else {
            noComments.append(c);
        }
    }

    // Pass 2: Clean up leading, trailing, and redundant commas outside strings
    QByteArray clean;
    clean.reserve(noComments.size());
    inString = false;
    escaped = false;
    int len = noComments.size();

    auto lastNonWhitespace = [&clean]() -> char {
        for (int j = clean.size() - 1; j >= 0; --j) {
            char ch = clean[j];
            if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')
                return ch;
        }
        return '\0';
    };

    auto nextNonWhitespace = [&noComments, len](int start) -> char {
        for (int j = start; j < len; ++j) {
            char ch = noComments[j];
            if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')
                return ch;
        }
        return '\0';
    };

    for (int i = 0; i < len; ++i) {
        char c = noComments[i];

        if (inString) {
            clean.append(c);
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
            escaped = false;
            clean.append(c);
            continue;
        }

        if (c == ',') {
            char prevChar = lastNonWhitespace();
            char nextChar = nextNonWhitespace(i + 1);

            // Leading comma right after '{' or '[' or duplicate comma
            if (prevChar == '{' || prevChar == '[' || prevChar == ',') {
                clean.append(' ');
                continue;
            }
            // Trailing comma right before '}' or ']'
            if (nextChar == '}' || nextChar == ']' || nextChar == ',') {
                clean.append(' ');
                continue;
            }
        }

        clean.append(c);
    }

    return clean;
}

static QJsonObject loadRoot(const QString &configPath) {
    QString path = configPath;
    if (path.isEmpty()) {
        path = ConfigLoader::defaultConfigPath();
    } else {
        if (!QFile::exists(path) && path.endsWith(".json") && QFile::exists(path + "c")) {
            path += "c";
        }
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QTextStream(stderr) << "drmenu: cannot open config: " << path << "\n";
        return {};
    }
    QJsonParseError err;
    QByteArray cleaned = cleanJsonc(file.readAll());
    QJsonDocument doc = QJsonDocument::fromJson(cleaned, &err);
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
        if (obj.contains("label"))    item.label    = obj["label"].toString().trimmed();
        if (obj.contains("command"))  item.command  = obj["command"].toString().trimmed();
        if (obj.contains("iconName")) item.iconName = obj["iconName"].toString().trimmed();
        if (obj.contains("icon"))     item.icon     = obj["icon"].toString().trimmed();
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

    // Support user-defined custom themes in config.json under "themes"
    QJsonObject customThemes = root.contains("themes") ? root["themes"].toObject() : QJsonObject{};

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

    if (customThemes.contains(theme)) {
        QJsonObject customThemeObj = customThemes[theme].toObject();
        QString baseTheme = customThemeObj.contains("theme") ? customThemeObj["theme"].toString() : (customThemeObj.contains("base") ? customThemeObj["base"].toString() : "liquid-glass");
        QVariantMap customThemeStyle = customThemeObj.toVariantMap();
        for (auto it = customThemeStyle.cbegin(); it != customThemeStyle.cend(); ++it) {
            if (it.key() != "theme" && it.key() != "base") {
                if (!styleOverrides.contains(it.key())) {
                    styleOverrides[it.key()] = it.value();
                }
            }
        }
        theme = baseTheme;
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
