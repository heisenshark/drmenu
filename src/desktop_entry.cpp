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

    QStringList extraDirs = {
        "/run/current-system/sw/share/applications",
        QDir::homePath() + "/.nix-profile/share/applications",
        QDir::homePath() + "/.local/state/nix/profile/share/applications",
        "/etc/profiles/per-user/" + qgetenv("USER") + "/share/applications",
        QDir::homePath() + "/.local/share/flatpak/exports/share/applications",
        "/var/lib/flatpak/exports/share/applications",
        "/usr/local/share/applications",
        "/usr/share/applications"
    };
    for (const QString &d : extraDirs) {
        if (!dataDirs.contains(d) && QDir(d).exists()) {
            dataDirs.append(d);
        }
    }

    // Also add subdirectories (e.g. /usr/share/applications/kde/, etc.)
    QStringList searchDirs;
    for (const QString &base : dataDirs) {
        if (!QDir(base).exists()) continue;
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
        item.label = appId;
        item.iconName = appId;
        item.command = appId;
        return item;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        item.label = appId;
        item.iconName = appId;
        item.command = appId;
        return item;
    }

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
        else if (line.startsWith("Icon=")    && item.iconName.isEmpty()) {
            QString ic = line.mid(5).trimmed();
            if ((ic.startsWith('"') && ic.endsWith('"')) || (ic.startsWith('\'') && ic.endsWith('\''))) {
                ic = ic.mid(1, ic.length() - 2).trimmed();
            }
            item.iconName = ic;
        }
        else if (line.startsWith("Exec=")    && item.command.isEmpty())
            item.command = cleanExec(line.mid(5).trimmed());
    }

    // Fallback: use appId as label, iconName, and command if missing
    if (item.label.isEmpty()) item.label = appId;
    if (item.iconName.isEmpty()) item.iconName = appId;
    if (item.command.isEmpty()) item.command = appId;

    return item;
}

QList<QVariantMap> DesktopEntry::listAllApplications() {
    QList<QVariantMap> result;
    QSet<QString> seenApps;

    QStringList dataDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    QStringList extraDirs = {
        "/run/current-system/sw/share/applications",
        QDir::homePath() + "/.nix-profile/share/applications",
        QDir::homePath() + "/.local/state/nix/profile/share/applications",
        "/etc/profiles/per-user/" + qgetenv("USER") + "/share/applications",
        QDir::homePath() + "/.local/share/flatpak/exports/share/applications",
        "/var/lib/flatpak/exports/share/applications",
        "/usr/local/share/applications",
        "/usr/share/applications"
    };
    for (const QString &d : extraDirs) {
        if (!dataDirs.contains(d) && QDir(d).exists()) {
            dataDirs.append(d);
        }
    }

    QStringList searchDirs;
    for (const QString &base : dataDirs) {
        if (!QDir(base).exists()) continue;
        searchDirs << base;
        QDirIterator it(base, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) searchDirs << it.next();
    }

    for (const QString &dir : searchDirs) {
        QDir d(dir);
        QFileInfoList files = d.entryInfoList({"*.desktop"}, QDir::Files | QDir::Readable);
        for (const QFileInfo &fi : files) {
            QString appId = fi.baseName();
            if (seenApps.contains(appId)) continue;

            QFile file(fi.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly)) continue;

            bool inEntry = false;
            bool noDisplay = false;
            QString name, icon, exec, comment, type = "Application";

            QTextStream ts(&file);
            while (!ts.atEnd()) {
                QString line = ts.readLine().trimmed();
                if (line == "[Desktop Entry]") { inEntry = true; continue; }
                if (line.startsWith('[') && inEntry) break;
                if (!inEntry) continue;

                if (line.startsWith("Type=") && type == "Application")
                    type = line.mid(5).trimmed();
                else if (line.startsWith("NoDisplay=true") || line.startsWith("Hidden=true"))
                    noDisplay = true;
                else if (line.startsWith("Name=") && name.isEmpty())
                    name = line.mid(5).trimmed();
                else if (line.startsWith("Icon=") && icon.isEmpty()) {
                    QString ic = line.mid(5).trimmed();
                    if ((ic.startsWith('"') && ic.endsWith('"')) || (ic.startsWith('\'') && ic.endsWith('\''))) {
                        ic = ic.mid(1, ic.length() - 2).trimmed();
                    }
                    icon = ic;
                }
                else if (line.startsWith("Exec=") && exec.isEmpty())
                    exec = cleanExec(line.mid(5).trimmed());
                else if (line.startsWith("Comment=") && comment.isEmpty())
                    comment = line.mid(8).trimmed();
            }

            if (noDisplay || type != "Application" || (name.isEmpty() && exec.isEmpty()))
                continue;

            seenApps.insert(appId);
            QVariantMap appMap;
            appMap["appId"]    = appId;
            appMap["name"]     = name.isEmpty() ? appId : name;
            appMap["iconName"] = icon.isEmpty() ? appId : icon;
            appMap["exec"]     = exec;
            appMap["comment"]  = comment;
            result.append(appMap);
        }
    }

    // Sort alphabetically by name
    std::sort(result.begin(), result.end(), [](const QVariantMap &a, const QVariantMap &b) {
        return a["name"].toString().localeAwareCompare(b["name"].toString()) < 0;
    });

    return result;
}

