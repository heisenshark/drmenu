#include "screen_detector.h"
#include <QProcess>
#include <QByteArray>
#include <QStringList>

TargetScreenInfo ScreenDetector::getTargetScreenInfo() {
    TargetScreenInfo info;
    if (qgetenv("HYPRLAND_INSTANCE_SIGNATURE").isEmpty())
        return info;

    QProcess cursorProc;
    cursorProc.start("hyprctl", {"cursorpos"});
    cursorProc.waitForFinished(2000);
    if (cursorProc.exitCode() != 0) return info;

    QString cursorOut = QString::fromUtf8(cursorProc.readAllStandardOutput()).trimmed();
    QStringList parts = cursorOut.split(',');
    if (parts.size() < 2) return info;
    bool okX, okY;
    int cx = parts[0].trimmed().toInt(&okX);
    int cy = parts[1].trimmed().toInt(&okY);
    if (!okX || !okY) return info;

    QProcess monitorsProc;
    monitorsProc.start("hyprctl", {"monitors", "-j"});
    monitorsProc.waitForFinished(2000);
    if (monitorsProc.exitCode() != 0) return info;

    QString json = QString::fromUtf8(monitorsProc.readAllStandardOutput());

    auto extractInt = [](const QString &block, const QString &key) -> int {
        QString searchKey = "\"" + key + "\": ";
        int idx = block.indexOf(searchKey);
        if (idx < 0) return -1;
        int start = idx + searchKey.length();
        int end = start;
        while (end < block.size() && (block[end].isDigit() || block[end] == '-')) ++end;
        return block.mid(start, end - start).toInt();
    };
    auto extractStr = [](const QString &block, const QString &key) -> QString {
        QString searchKey = "\"" + key + "\": \"";
        int idx = block.indexOf(searchKey);
        if (idx < 0) return {};
        int start = idx + searchKey.length();
        int end = block.indexOf('"', start);
        return end > start ? block.mid(start, end - start) : QString();
    };

    int pos = 0;
    while (pos < json.size()) {
        int blockStart = json.indexOf('{', pos);
        if (blockStart < 0) break;
        int depth = 0, blockEnd = blockStart;
        for (int i = blockStart; i < json.size(); ++i) {
            if (json[i] == '{') ++depth;
            else if (json[i] == '}') { --depth; if (depth == 0) { blockEnd = i; break; } }
        }
        QString block = json.mid(blockStart, blockEnd - blockStart + 1);
        QString name = extractStr(block, "name");
        int mx = extractInt(block, "x");
        int my = extractInt(block, "y");
        int mw = extractInt(block, "width");
        int mh = extractInt(block, "height");
        if (!name.isEmpty() && mw > 0 && mh > 0) {
            if (cx >= mx && cx < mx + mw && cy >= my && cy < my + mh) {
                info.monitorName = name;
                info.localX = cx - mx;
                info.localY = cy - my;
                return info;
            }
        }
        pos = blockEnd + 1;
    }
    return info;
}
