#include "screen_detector.h"
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <unistd.h>

static QString queryHyprlandSocket(const QString &cmd) {
    QString sig = qgetenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (sig.isEmpty()) return {};

    QString xdgRuntime = qgetenv("XDG_RUNTIME_DIR");
    if (xdgRuntime.isEmpty()) xdgRuntime = "/run/user/" + QString::number(getuid());

    QString socketPath = xdgRuntime + "/hypr/" + sig + "/.socket.sock";
    QLocalSocket socket;
    socket.connectToServer(socketPath);
    if (!socket.waitForConnected(50)) return {};

    socket.write(cmd.toUtf8());
    socket.flush();

    if (!socket.waitForReadyRead(50)) return {};

    return QString::fromUtf8(socket.readAll()).trimmed();
}

TargetScreenInfo ScreenDetector::getTargetScreenInfo() {
    TargetScreenInfo info;

    int cx = -1, cy = -1;

    // 1. Try Hyprland IPC socket (direct socket read, <0.2ms, no QProcess fork)
    QString cursorOut = queryHyprlandSocket("cursorpos");
    if (!cursorOut.isEmpty()) {
        QStringList parts = cursorOut.split(',');
        if (parts.size() >= 2) {
            bool okX = false, okY = false;
            int x = parts[0].trimmed().toInt(&okX);
            int y = parts[1].trimmed().toInt(&okY);
            if (okX && okY) {
                cx = x;
                cy = y;
            }
        }
    }

    // Fallback: QCursor::pos()
    if (cx < 0 || cy < 0) {
        QPoint gpos = QCursor::pos();
        cx = gpos.x();
        cy = gpos.y();
    }

    // 2. Try Hyprland IPC monitors
    QString monitorsOut = queryHyprlandSocket("j/monitors");
    if (!monitorsOut.isEmpty()) {
        QJsonParseError jsonErr;
        QJsonDocument doc = QJsonDocument::fromJson(monitorsOut.toUtf8(), &jsonErr);
        if (jsonErr.error == QJsonParseError::NoError && doc.isArray()) {
            QJsonArray monitorList = doc.array();
            for (const QJsonValue &val : monitorList) {
                if (!val.isObject()) continue;
                QJsonObject mon = val.toObject();

                QString name = mon["name"].toString();
                int mx = mon["x"].toInt();
                int my = mon["y"].toInt();
                int mw = mon["width"].toInt();
                int mh = mon["height"].toInt();

                if (!name.isEmpty() && mw > 0 && mh > 0) {
                    if (cx >= mx && cx < mx + mw && cy >= my && cy < my + mh) {
                        info.monitorName = name;
                        info.localX = cx - mx;
                        info.localY = cy - my;
                        info.monitorX = mx;
                        info.monitorY = my;
                        info.monitorWidth = mw;
                        info.monitorHeight = mh;
                        return info;
                    }
                }
            }

            // Fallback to first monitor if cursor not within bounds
            if (!monitorList.isEmpty() && monitorList[0].isObject()) {
                QJsonObject mon = monitorList[0].toObject();
                info.monitorName = mon["name"].toString();
                int mx = mon["x"].toInt();
                int my = mon["y"].toInt();
                info.localX = cx - mx;
                info.localY = cy - my;
                info.monitorX = mx;
                info.monitorY = my;
                info.monitorWidth = mon["width"].toInt();
                info.monitorHeight = mon["height"].toInt();
                return info;
            }
        }
    }

    // 3. Fallback using QGuiApplication screens
    QPoint gpos(cx, cy);
    QScreen *targetScreen = QGuiApplication::screenAt(gpos);
    if (!targetScreen) targetScreen = QGuiApplication::primaryScreen();
    if (targetScreen) {
        info.monitorName = targetScreen->name();
        info.localX = cx - targetScreen->geometry().x();
        info.localY = cy - targetScreen->geometry().y();
        info.monitorX = targetScreen->geometry().x();
        info.monitorY = targetScreen->geometry().y();
        info.monitorWidth = targetScreen->geometry().width();
        info.monitorHeight = targetScreen->geometry().height();
    }

    return info;
}
