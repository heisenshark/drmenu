#pragma once

#include <QString>
#include <QVariantMap>

struct MenuItem {
    QString label;
    QString icon;       // emoji / text fallback
    QString iconName;   // XDG icon theme name (e.g. "firefox", "utilities-terminal")
    QString command;

    QVariantMap toVariantMap() const {
        QVariantMap m;
        m["label"]    = label;
        m["icon"]     = icon;
        m["iconName"] = iconName;
        m["command"]  = command;
        return m;
    }
};
