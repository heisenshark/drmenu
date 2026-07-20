#include "client.h"
#include "daemon.h"

#include <QLocalSocket>
#include <QTextStream>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <chrono>

static QMap<QString, QString> buildCommandMap(const QVariantMap &allMenus) {
    QMap<QString, QString> map;
    for (auto it = allMenus.cbegin(); it != allMenus.cend(); ++it) {
        QVariant val = it.value();
        QVariantList itemList = (val.typeId() == QMetaType::QVariantMap)
            ? val.toMap()["items"].toList()
            : val.toList();

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
        QTextStream(stdout) << selected << "\n";

        auto it = commandMap.find(selected);
        if (it != commandMap.end())
            QProcess::startDetached("sh", {"-c", it.value()});

        QTextStream(stderr) << "[drmenu client latency: "
                            << QString::number(ms, 'f', 2) << " ms]\n";
    }
}

// ── Flat item list (inline / stdin mode) ──────────────────────────────────────
bool ClientRunner::tryRun(const QList<MenuItem> &items, const QVariantMap &style) {
    QLocalSocket socket;
    socket.connectToServer(DaemonServer::SOCKET_NAME);
    if (!socket.waitForConnected(50)) return false;

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
    if (!socket.waitForConnected(50)) return false;

    auto t_start = std::chrono::high_resolution_clock::now();

    QJsonObject menusJson;
    for (auto it = allMenus.cbegin(); it != allMenus.cend(); ++it) {
        QVariant val = it.value();
        if (val.typeId() == QMetaType::QVariantMap) {
            menusJson[it.key()] = QJsonObject::fromVariantMap(val.toMap());
        } else {
            menusJson[it.key()] = QJsonArray::fromVariantList(val.toList());
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
