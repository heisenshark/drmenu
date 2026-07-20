#include "desktop_entry.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QRegularExpression>

// Strip .desktop field modifiers like %u %f %U %F %i %c %k from Exec lines
static QString cleanExec(const QString &exec) {
    QString result = exec;
    result.remove(QRegularExpression(R"(\s*%[uUfFidcCkKvmDnNb])"));
    return result.trimmed();
}

// Search XDG application dirs for a matching .desktop file.
// Tries: exact match "<appId>.desktop", then case-insensitive prefix match.
static QString findDesktopFile(const QString &appId) {
    QStringList dataDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);

    // Also add subdirectories (e.g. /usr/share/applications/kde/, etc.)
    QStringList searchDirs;
    for (const QString &base : dataDirs) {
        searchDirs << base;
        QDirIterator it(base, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) searchDirs << it.next();
    }

    // 1. Exact match: <appId>.desktop
    for (const QString &dir : searchDirs) {
        QString path = dir + "/" + appId + ".desktop";
        if (QFile::exists(path)) return path;
    }

    // 2. Case-insensitive substring match on filename
    QString lowerAppId = appId.toLower();
    for (const QString &dir : searchDirs) {
        for (const QFileInfo &fi : QDir(dir).entryInfoList({"*.desktop"}, QDir::Files)) {
            if (fi.baseName().toLower() == lowerAppId) return fi.absoluteFilePath();
        }
    }

    // 3. Search inside .desktop files for Name= match
    for (const QString &dir : searchDirs) {
        for (const QFileInfo &fi : QDir(dir).entryInfoList({"*.desktop"}, QDir::Files)) {
            QFile f(fi.absoluteFilePath());
            if (!f.open(QIODevice::ReadOnly)) continue;
            QTextStream ts(&f);
            while (!ts.atEnd()) {
                QString line = ts.readLine();
                if (line.startsWith("Name=") && line.mid(5).trimmed().toLower() == lowerAppId)
                    return fi.absoluteFilePath();
            }
        }
    }

    return {};
}

MenuItem DesktopEntry::resolve(const QString &appId) {
    MenuItem item;
    QString path = findDesktopFile(appId);
    if (path.isEmpty()) {
        QTextStream(stderr) << "drmenu: could not find .desktop file for '" << appId << "'\n";
        return item;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return item;

    bool inDesktopEntry = false;
    QTextStream ts(&file);
    while (!ts.atEnd()) {
        QString line = ts.readLine().trimmed();
        if (line == "[Desktop Entry]") { inDesktopEntry = true; continue; }
        if (line.startsWith('[') && inDesktopEntry) break; // left [Desktop Entry] section

        if (!inDesktopEntry) continue;

        // Only read non-localised keys (i.e. skip Name[fr]=...)
        if (line.startsWith("Name=")    && item.label.isEmpty())
            item.label = line.mid(5).trimmed();
        else if (line.startsWith("Icon=")    && item.iconName.isEmpty())
            item.iconName = line.mid(5).trimmed();
        else if (line.startsWith("Exec=")    && item.command.isEmpty())
            item.command = cleanExec(line.mid(5).trimmed());
    }

    // Fallback: use appId as label if Name= was missing
    if (item.label.isEmpty()) item.label = appId;

    return item;
}
