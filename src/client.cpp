#include "client.h"
#include "daemon.h"

#include <QLocalSocket>
#include <QTextStream>
#include <chrono>

bool ClientRunner::tryRun(const QList<MenuItem> &items) {
    QLocalSocket socket;
    socket.connectToServer(DaemonServer::SOCKET_NAME);
    if (!socket.waitForConnected(50)) {
        return false;
    }

    auto t_client_start = std::chrono::high_resolution_clock::now();

    QString payload;
    for (const MenuItem &item : items) {
        payload += item.label;
        if (!item.icon.isEmpty()) payload += ":" + item.icon;
        if (!item.command.isEmpty()) payload += "\t" + item.command;
        payload += "\n";
    }
    socket.write(payload.toUtf8());
    socket.flush();

    if (socket.waitForReadyRead(30000)) {
        QByteArray response = socket.readAll().trimmed();
        QString respStr = QString::fromUtf8(response);

        auto t_client_end = std::chrono::high_resolution_clock::now();
        double ms_client = std::chrono::duration<double, std::milli>(t_client_end - t_client_start).count();

        if (respStr.startsWith("SELECTED\t")) {
            QString selected = respStr.mid(9);
            QTextStream out(stdout);
            out << selected << "\n";
            out.flush();

            QTextStream err(stderr);
            err << "[drmenu daemon client latency: " << QString::number(ms_client, 'f', 2) << " ms]\n";
        }
    }

    return true;
}
