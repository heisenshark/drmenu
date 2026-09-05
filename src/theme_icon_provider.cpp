#include "theme_icon_provider.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QIcon>
#include <QMutex>
#include <QPixmap>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <algorithm>

namespace {

QMutex s_cacheMutex;
QHash<QString, QString> s_resolvedPathCache;
QHash<QString, QString> s_resolvedThemeNameCache;
QSet<QString> s_notFoundCache;
bool s_themeInitialized = false;

QString detectSystemIconTheme() {
    QStringList candidates = {
        QDir::homePath() + "/.config/gtk-3.0/settings.ini",
        QDir::homePath() + "/.config/gtk-4.0/settings.ini",
        QDir::homePath() + "/.config/kdeglobals"
    };

    for (const QString &path : candidates) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("gtk-icon-theme-name=", Qt::CaseInsensitive)) {
                QString theme = line.mid(20).trimmed();
                if (!theme.isEmpty()) return theme;
            } else if (line.startsWith("Theme=", Qt::CaseInsensitive)) {
                // In kdeglobals under [Icons]
                QString theme = line.mid(6).trimmed();
                if (!theme.isEmpty() && !theme.contains('/') && !theme.contains('\\')) return theme;
            }
        }
    }
    return {};
}

QStringList getPixmapsDirs() {
    static QStringList dirs;
    if (!dirs.isEmpty()) return dirs;

    QString username = QDir::home().dirName();
    QStringList candidates = {
        "/run/current-system/sw/share/pixmaps",
        QDir::homePath() + "/.nix-profile/share/pixmaps",
        "/etc/profiles/per-user/" + username + "/share/pixmaps",
        QDir::homePath() + "/.local/share/pixmaps",
        "/usr/local/share/pixmaps",
        "/usr/share/pixmaps"
    };
    for (const QString &genericData : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
        candidates << (genericData + "/pixmaps");
    }

    for (const QString &p : candidates) {
        if (QDir(p).exists() && !dirs.contains(p)) {
            dirs.append(p);
        }
    }
    return dirs;
}

QStringList getIconSearchDirs() {
    static QStringList dirs;
    if (!dirs.isEmpty()) return dirs;

    QString username = QDir::home().dirName();
    QStringList candidates = {
        QDir::homePath() + "/.local/share/icons",
        QDir::homePath() + "/.nix-profile/share/icons",
        "/etc/profiles/per-user/" + username + "/share/icons",
        "/run/current-system/sw/share/icons",
        "/usr/local/share/icons",
        "/usr/share/icons"
    };
    for (const QString &genericData : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
        candidates << (genericData + "/icons");
    }

    for (const QString &p : candidates) {
        if (QDir(p).exists() && !dirs.contains(p)) {
            dirs.append(p);
        }
    }
    return dirs;
}

} // namespace

void ThemeIconProvider::initTheme() {
    QMutexLocker locker(&s_cacheMutex);
    if (s_themeInitialized) return;
    s_themeInitialized = true;

    // Detect user-configured theme (e.g. breeze-dark) if Qt is unconfigured or defaulting to hicolor
    if (QIcon::themeName().isEmpty() || QIcon::themeName() == "hicolor") {
        QString theme = detectSystemIconTheme();
        if (!theme.isEmpty()) {
            QIcon::setThemeName(theme);
        }
    }

    // Ensure icon search paths include NixOS and local user paths
    QStringList searchPaths = QIcon::themeSearchPaths();
    QStringList extraDirs = getIconSearchDirs();
    for (const QString &d : extraDirs) {
        if (!searchPaths.contains(d)) {
            searchPaths.prepend(d);
        }
    }
    QIcon::setThemeSearchPaths(searchPaths);
}

ThemeIconProvider::ThemeIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap) {
    initTheme();
}

QIcon ThemeIconProvider::findIcon(const QString &id) {
    initTheme();

    // 1. Clean input string
    QString cleanId = QUrl::fromPercentEncoding(id.toUtf8()).trimmed();
    int qIdx = cleanId.indexOf('?');
    if (qIdx != -1) cleanId = cleanId.left(qIdx).trimmed();

    if ((cleanId.startsWith('"') && cleanId.endsWith('"')) ||
        (cleanId.startsWith('\'') && cleanId.endsWith('\''))) {
        cleanId = cleanId.mid(1, cleanId.length() - 2).trimmed();
    }

    if (cleanId.isEmpty()) {
        return QIcon::fromTheme("application-x-executable");
    }

    // Expand ~
    if (cleanId.startsWith("~/")) {
        cleanId = QDir::homePath() + cleanId.mid(1);
    } else if (cleanId.startsWith("file://")) {
        cleanId = QUrl(cleanId).toLocalFile();
    }

    // 2. Direct file path check
    if (cleanId.startsWith("/") && QFile::exists(cleanId) && QFileInfo(cleanId).isFile()) {
        return QIcon(cleanId);
    }
    // Check if QML URL stripped a leading slash (e.g. "run/current-system/..." or "nix/store/...")
    if (!cleanId.startsWith("/") && QFile::exists("/" + cleanId) && QFileInfo("/" + cleanId).isFile()) {
        return QIcon("/" + cleanId);
    }
    if (QFile::exists(cleanId) && QFileInfo(cleanId).isFile()) {
        return QIcon(cleanId);
    }

    // 3. Thread-safe cache check
    {
        QMutexLocker locker(&s_cacheMutex);
        if (s_resolvedPathCache.contains(cleanId)) {
            return QIcon(s_resolvedPathCache[cleanId]);
        }
        if (s_resolvedThemeNameCache.contains(cleanId)) {
            return QIcon::fromTheme(s_resolvedThemeNameCache[cleanId]);
        }
        if (s_notFoundCache.contains(cleanId)) {
            QIcon generic = QIcon::fromTheme("application-x-executable");
            if (generic.isNull()) generic = QIcon::fromTheme("application-x-desktop");
            return generic;
        }
    }

    // 4. Base name without extension
    QString baseName = cleanId;
    static const QStringList imageExts = {".png", ".svg", ".svgz", ".xpm", ".jpg", ".jpeg", ".ico"};
    for (const QString &ext : imageExts) {
        if (cleanId.endsWith(ext, Qt::CaseInsensitive)) {
            baseName = cleanId.left(cleanId.length() - ext.length());
            break;
        }
    }

    // 5. Standard theme lookup
    if (QIcon::hasThemeIcon(cleanId)) {
        QIcon ico = QIcon::fromTheme(cleanId);
        if (!ico.isNull()) {
            QMutexLocker locker(&s_cacheMutex);
            s_resolvedThemeNameCache[cleanId] = cleanId;
            return ico;
        }
    }
    if (baseName != cleanId && QIcon::hasThemeIcon(baseName)) {
        QIcon ico = QIcon::fromTheme(baseName);
        if (!ico.isNull()) {
            QMutexLocker locker(&s_cacheMutex);
            s_resolvedThemeNameCache[cleanId] = baseName;
            return ico;
        }
    }
    QString lowerBase = baseName.toLower();
    if (lowerBase != baseName && QIcon::hasThemeIcon(lowerBase)) {
        QIcon ico = QIcon::fromTheme(lowerBase);
        if (!ico.isNull()) {
            QMutexLocker locker(&s_cacheMutex);
            s_resolvedThemeNameCache[cleanId] = lowerBase;
            return ico;
        }
    }

    // 6. Pixmaps directory search (/share/pixmaps)
    const QStringList pixDirs = getPixmapsDirs();
    for (const QString &dir : pixDirs) {
        QStringList candidates;
        if (cleanId != baseName) {
            candidates << (dir + "/" + cleanId);
        }
        candidates << (dir + "/" + baseName + ".png")
                   << (dir + "/" + baseName + ".svg")
                   << (dir + "/" + baseName + ".xpm")
                   << (dir + "/" + baseName);
        if (lowerBase != baseName) {
            candidates << (dir + "/" + lowerBase + ".png")
                       << (dir + "/" + lowerBase + ".svg")
                       << (dir + "/" + lowerBase + ".xpm");
        }

        for (const QString &cand : candidates) {
            if (QFile::exists(cand) && QFileInfo(cand).isFile()) {
                QMutexLocker locker(&s_cacheMutex);
                s_resolvedPathCache[cleanId] = cand;
                return QIcon(cand);
            }
        }
    }

    // 7. Non-standard icon theme subdirectories (e.g. 1024x1024, scalable, 512x512)
    const QStringList iconDirs = getIconSearchDirs();
    const QString currentTheme = QIcon::themeName();
    QStringList themeSubdirs = {
        "hicolor/1024x1024/apps",
        "hicolor/512x512/apps",
        "hicolor/256x256/apps",
        "hicolor/128x128/apps",
        "hicolor/scalable/apps",
        "hicolor/48x48/apps",
        "hicolor/32x32/apps"
    };
    if (!currentTheme.isEmpty() && currentTheme != "hicolor") {
        themeSubdirs.prepend(currentTheme + "/apps/scalable");
        themeSubdirs.prepend(currentTheme + "/apps/48");
        themeSubdirs.prepend(currentTheme + "/apps/128");
    }

    for (const QString &iconRoot : iconDirs) {
        for (const QString &sub : themeSubdirs) {
            QString prefix = iconRoot + "/" + sub + "/";
            QStringList candidates = {
                prefix + baseName + ".png",
                prefix + baseName + ".svg",
                prefix + baseName + ".xpm"
            };
            if (lowerBase != baseName) {
                candidates << (prefix + lowerBase + ".png")
                           << (prefix + lowerBase + ".svg");
            }

            for (const QString &cand : candidates) {
                if (QFile::exists(cand) && QFileInfo(cand).isFile()) {
                    QMutexLocker locker(&s_cacheMutex);
                    s_resolvedPathCache[cleanId] = cand;
                    return QIcon(cand);
                }
            }
        }
    }

    // 8. Fallback to generic desktop/executable icon
    {
        QMutexLocker locker(&s_cacheMutex);
        s_notFoundCache.insert(cleanId);
    }

    QIcon fallback = QIcon::fromTheme("application-x-executable");
    if (fallback.isNull()) fallback = QIcon::fromTheme("application-x-desktop");
    if (fallback.isNull()) fallback = QIcon::fromTheme("system-run");
    return fallback;
}

QPixmap ThemeIconProvider::findPixmap(const QString &id, const QSize &requestedSize) {
    int w = requestedSize.width()  > 0 ? requestedSize.width()  : 48;
    int h = requestedSize.height() > 0 ? requestedSize.height() : 48;

    QIcon icon = findIcon(id);
    QPixmap pixmap = icon.pixmap(w, h);

    if (pixmap.isNull()) {
        QString cachedPath;
        {
            QMutexLocker locker(&s_cacheMutex);
            cachedPath = s_resolvedPathCache.value(id);
        }
        if (!cachedPath.isEmpty()) {
            QPixmap diskPixmap(cachedPath);
            if (!diskPixmap.isNull()) {
                pixmap = diskPixmap.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
    }

    if (pixmap.isNull()) {
        QIcon generic = QIcon::fromTheme("application-x-executable");
        if (generic.isNull()) generic = QIcon::fromTheme("application-x-desktop");
        pixmap = generic.pixmap(w, h);
    }

    return pixmap;
}

QPixmap ThemeIconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) {
    QPixmap pixmap = findPixmap(id, requestedSize);
    if (size) *size = pixmap.size();
    return pixmap;
}
