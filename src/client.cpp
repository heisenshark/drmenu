#include "client.h"
#include "daemon.h"

#include <QLocalSocket>
#include <QTextStream>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <chrono>

bool ClientRunner::tryRun(const QList<MenuItem> &items) {
    QLocalSocket socket;
    socket.connectToServer(DaemonServer::SOCKET_NAME);
    if (!socket.waitForConnected(50))
        return false;

    auto t_client_start = std::chrono::high_resolution_clock::now();

    // Serialize items as a JSON array, terminated by newline
    QJsonArray arr;
    for (const MenuItem &item : items)
        arr.append(QJsonObject::fromVariantMap(item.toVariantMap()));

    socket.write(QJsonDocument(arr).toJson(QJsonDocument::Compact) + "\n");
    socket.flush();

    if (socket.waitForReadyRead(30000)) {
        QByteArray response = socket.readAll().trimmed();
        QString respStr = QString::fromUtf8(response);

        auto t_client_end = std::chrono::high_resolution_clock::now();
        double ms_client = std::chrono::duration<double, std::milli>(t_client_end - t_client_start).count();

        if (respStr.startsWith("SELECTED\t")) {
            QString selected = respStr.mid(9);

            // Print to stdout (dmenu compatible)
            QTextStream(stdout) << selected << "\n";

            // Run the associated command if defined
            for (const MenuItem &item : items) {
                if (item.label == selected && !item.command.isEmpty()) {
                    QProcess::startDetached("sh", {"-c", item.command});
                    break;
                }
            }

            QTextStream(stderr) << "[drmenu daemon client latency: "
                                << QString::number(ms_client, 'f', 2) << " ms]\n";
        }
    }

    return true;
}
