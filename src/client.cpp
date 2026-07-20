#include "client.h"
#include "daemon.h"

#include <QLocalSocket>
#include <QTextStream>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <chrono>

static QJsonArray variantListToJsonArray(const QVariantList &list) {
    QJsonArray arr;
    for (const QVariant &v : list) {
        if (v.typeId() == QMetaType::QVariantMap)
            arr.append(QJsonObject::fromVariantMap(v.toMap()));
    }
    return arr;
}

static QJsonObject menuMapToJsonObject(const QVariantMap &menuMap) {
    QJsonObject obj;
    if (menuMap.contains("items"))
        obj["items"] = variantListToJsonArray(menuMap["items"].toList());
    if (menuMap.contains("spawnAtMouse"))
        obj["spawnAtMouse"] = menuMap["spawnAtMouse"].toBool();
    if (menuMap.contains("escapeClosesAll"))
        obj["escapeClosesAll"] = menuMap["escapeClosesAll"].toBool();
    if (menuMap.contains("style"))
        obj["style"] = QJsonObject::fromVariantMap(menuMap["style"].toMap());
    return obj;
}

static QMap<QString, QString> buildCommandMap(const QVariantMap &allMenus) {
    QMap<QString, QString> map;
    for (auto it = allMenus.cbegin(); it != allMenus.cend(); ++it) {
        QVariant val = it.value();
        QVariantList itemList;
        if (val.typeId() == QMetaType::QVariantMap) {
            itemList = val.toMap()["items"].toList();
        } else {
            itemList = val.toList();
        }

        for (const QVariant &v : itemList) {
            QVariantMap item = v.toMap();
            QString label   = item["label"].toString();
            QString command = item["command"].toString();
            if (!label.isEmpty() && !command.isEmpty())
                map[label] = command;
        }
    }
    return map;
}

static void waitForResponse(QLocalSocket &socket,
                             const QMap<QString, QString> &commandMap,
                             std::chrono::high_resolution_clock::time_point t_start) {
    if (!socket.waitForReadyRead(30000)) return;

    QByteArray response = socket.readAll().trimmed();
    QString respStr = QString::fromUtf8(response);

    auto t_end = std::chrono::high_resolution_clock::now();
    double ms  = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    if (respStr.startsWith("SELECTED\t")) {
        QString selected = respStr.mid(9);
        QTextStream out(stdout);
        out << selected << Qt::endl;

        auto it = commandMap.find(selected);
        if (it != commandMap.end())
            QProcess::startDetached("sh", {"-c", it.value()});

        QTextStream err(stderr);
        err << "[drmenu client latency: "
            << QString::number(ms, 'f', 2) << " ms]" << Qt::endl;
    }
}

// ── Flat item list (inline / stdin mode) ──────────────────────────────────────
bool ClientRunner::tryRun(const QList<MenuItem> &items, const QVariantMap &style) {
    QLocalSocket socket;
    socket.connectToServer(DaemonServer::SOCKET_NAME);
    if (!socket.waitForConnected(200)) return false;

    auto t_start = std::chrono::high_resolution_clock::now();

    QJsonArray arr;
    for (const MenuItem &item : items)
        arr.append(QJsonObject::fromVariantMap(item.toVariantMap()));

    QJsonObject payload;
    payload["type"]  = "items";
    payload["items"] = arr;
    if (!style.isEmpty())
        payload["style"] = QJsonObject::fromVariantMap(style);

    socket.write(QJsonDocument(payload).toJson(QJsonDocument::Compact) + "\n");
    socket.flush();

    QMap<QString, QString> cmdMap;
    for (const MenuItem &item : items)
        if (!item.label.isEmpty() && !item.command.isEmpty())
            cmdMap[item.label] = item.command;

    waitForResponse(socket, cmdMap, t_start);
    return true;
}

// ── Full menu map (config / nested mode) ──────────────────────────────────────
bool ClientRunner::tryRunMenus(const QVariantMap &allMenus, const QString &initialMenu,
                                bool spawnAtMouse, bool escapeClosesAll,
                                const QVariantMap &style) {
    QLocalSocket socket;
    socket.connectToServer(DaemonServer::SOCKET_NAME);
    if (!socket.waitForConnected(200)) return false;

    auto t_start = std::chrono::high_resolution_clock::now();

    QJsonObject menusJson;
    for (auto it = allMenus.cbegin(); it != allMenus.cend(); ++it) {
        QVariant val = it.value();
        if (val.typeId() == QMetaType::QVariantMap) {
            menusJson[it.key()] = menuMapToJsonObject(val.toMap());
        } else {
            menusJson[it.key()] = variantListToJsonArray(val.toList());
        }
    }

    QJsonObject payload;
    payload["type"]            = "menus";
    payload["menus"]           = menusJson;
    payload["initialMenu"]     = initialMenu;
    payload["spawnAtMouse"]    = spawnAtMouse;
    payload["escapeClosesAll"]  = escapeClosesAll;
    if (!style.isEmpty())
        payload["style"] = QJsonObject::fromVariantMap(style);

    socket.write(QJsonDocument(payload).toJson(QJsonDocument::Compact) + "\n");
    socket.flush();

    waitForResponse(socket, buildCommandMap(allMenus), t_start);
    return true;
}
