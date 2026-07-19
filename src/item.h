#pragma once

#include <QString>
#include <QVariantMap>

struct MenuItem {
    QString label;
    QString icon;
    QString command;

    QVariantMap toVariantMap() const {
        QVariantMap m;
        m["label"]   = label;
        m["icon"]    = icon;
        m["command"] = command;
        return m;
    }
};
