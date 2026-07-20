#pragma once

#include <QString>
#include <QVariantMap>

struct MenuItem {
    QString label;
    QString icon;       // emoji / text fallback
    QString iconName;   // XDG icon theme name (e.g. "firefox")
    QString command;    // shell command to run on selection
    QString submenuName; // if set, clicking navigates to this named menu

    QVariantMap toVariantMap() const {
        QVariantMap m;
        m["label"]       = label;
        m["icon"]        = icon;
        m["iconName"]    = iconName;
        m["command"]     = command;
        m["submenuName"] = submenuName;
        return m;
    }
};
