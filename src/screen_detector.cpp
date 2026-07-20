#include "screen_detector.h"
#include <QProcess>
#include <QByteArray>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>

TargetScreenInfo ScreenDetector::getTargetScreenInfo() {
    TargetScreenInfo info;

    int cx = -1, cy = -1;

    // 1. Try hyprctl cursorpos
    QProcess cursorProc;
    cursorProc.start("hyprctl", {"cursorpos"});
    if (cursorProc.waitForFinished(1000) && cursorProc.exitCode() == 0) {
        QString cursorOut = QString::fromUtf8(cursorProc.readAllStandardOutput()).trimmed();
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

    // 2. Try hyprctl monitors -j
    QProcess monitorsProc;
    monitorsProc.start("hyprctl", {"monitors", "-j"});
    if (monitorsProc.waitForFinished(1000) && monitorsProc.exitCode() == 0) {
        QByteArray jsonBytes = monitorsProc.readAllStandardOutput();
        QJsonParseError jsonErr;
        QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &jsonErr);
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
                        return info;
                    }
                }
            }

            // If cursor wasn't inside explicit bounds, pick the first monitor
            if (!monitorList.isEmpty() && monitorList[0].isObject()) {
                QJsonObject mon = monitorList[0].toObject();
                info.monitorName = mon["name"].toString();
                int mx = mon["x"].toInt();
                int my = mon["y"].toInt();
                info.localX = cx - mx;
                info.localY = cy - my;
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
    }

    return info;
}
