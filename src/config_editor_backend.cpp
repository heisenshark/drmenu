#include "config_editor_backend.h"
#include "config_loader.h"
#include "desktop_entry.h"
#include "theme_manager.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QProcess>
#include <QCoreApplication>
#include <thread>
#include <algorithm>

ConfigEditorBackend::ConfigEditorBackend(QObject *parent)
    : QObject(parent) {
    startIndexingIcons();
}

ConfigEditorBackend::~ConfigEditorBackend() = default;

int ConfigEditorBackend::totalIconsCount() const {
    QMutexLocker locker(&m_mutex);
    return m_cachedIcons.size();
}

QString ConfigEditorBackend::defaultConfigPath() {
    return ConfigLoader::defaultConfigPath();
}

QStringList ConfigEditorBackend::availableConfigFiles() {
    QString configDir = QDir::homePath() + "/.config/drmenu";
    QDir dir(configDir);
    if (!dir.exists()) return {};

    QStringList filters;
    filters << "*.json" << "*.jsonc";
    QFileInfoList list = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    QStringList result;
    for (const QFileInfo &fi : list) {
        // Skip schema files from normal editing choices
        if (fi.fileName().contains("schema")) continue;
        result.append(fi.absoluteFilePath());
    }

    if (result.isEmpty() && QFile::exists(ConfigLoader::defaultConfigPath())) {
        result.append(ConfigLoader::defaultConfigPath());
    }

    return result;
}

QVariantMap ConfigEditorBackend::loadConfig(const QString &filePath) {
    QString path = filePath.isEmpty() ? ConfigLoader::defaultConfigPath() : filePath;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ConfigEditorBackend: cannot open" << path;
        return {};
    }

    QByteArray raw = file.readAll();
    QByteArray clean = ConfigLoader::preprocessJsonc(raw);
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(clean, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "ConfigEditorBackend: JSON parse error in" << path << ":" << err.errorString();
        return {};
    }

    return doc.object().toVariantMap();
}

bool ConfigEditorBackend::saveConfig(const QString &filePath, const QVariantMap &configData) {
    QString path = filePath.isEmpty() ? ConfigLoader::defaultConfigPath() : filePath;
    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    // Create a backup of the previous config if it exists
    if (QFile::exists(path)) {
        QString backupPath = path + ".bak";
        QFile::remove(backupPath);
        QFile::copy(path, backupPath);
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "ConfigEditorBackend: cannot open file for writing:" << path;
        return false;
    }

    QJsonObject rootObj = QJsonObject::fromVariantMap(configData);
    QJsonDocument doc(rootObj);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Indented);

    qint64 written = file.write(jsonBytes);
    file.close();
    return written == jsonBytes.size();
}

QVariantList ConfigEditorBackend::installedApps() {
    if (m_cachedApps.isEmpty()) {
        const auto apps = DesktopEntry::listAllApplications();
        m_cachedApps.reserve(apps.size());
        for (const auto &app : apps) {
            m_cachedApps.append(app);
        }
    }
    return m_cachedApps;
}

QStringList ConfigEditorBackend::searchSystemIcons(const QString &query, int limit) {
    QMutexLocker locker(&m_mutex);
    QString q = query.trimmed().toLower();

    if (q.isEmpty()) {
        // Return a curated set of popular action / app icon names
        static const QStringList popularIcons = {
            "terminal", "utilities-terminal", "kitty", "alacritty", "org.wezfurlong.wezterm",
            "web-browser", "firefox", "zen-browser", "chromium", "google-chrome",
            "preferences-system", "settings", "system-settings",
            "system-shutdown", "system-reboot", "system-lock-screen", "system-log-out",
            "folder", "folder-home", "system-file-manager", "org.kde.dolphin", "nautilus",
            "applications-multimedia", "multimedia-player", "media-playback-start",
            "audio-volume-high", "audio-volume-muted",
            "applications-games", "steam", "discord", "vesktop", "telegram",
            "text-editor", "accessories-text-editor", "code", "visual-studio-code",
            "calculator", "accessories-calculator", "camera-photo", "image-viewer"
        };

        QStringList result;
        for (const QString &ico : popularIcons) {
            if (m_cachedIcons.contains(ico)) {
                result.append(ico);
            }
        }
        if (result.isEmpty() && !m_cachedIcons.isEmpty()) {
            return m_cachedIcons.mid(0, std::min<int>(limit, static_cast<int>(m_cachedIcons.size())));
        }
        return result;
    }

    QStringList matches;
    QStringList exactPrefixMatches;
    for (const QString &ico : m_cachedIcons) {
        if (ico.startsWith(q)) {
            exactPrefixMatches.append(ico);
            if (exactPrefixMatches.size() >= limit) break;
        } else if (ico.contains(q)) {
            matches.append(ico);
        }
    }

    QStringList combined = exactPrefixMatches;
    for (const QString &ico : matches) {
        if (combined.size() >= limit) break;
        combined.append(ico);
    }
    return combined;
}

QStringList ConfigEditorBackend::availableThemes() {
    return ThemeManager::availableThemes();
}

void ConfigEditorBackend::testMenu(const QString &menuName, const QString &configPath) {
    QStringList args;
    if (!menuName.isEmpty()) {
        args << "--menu" << menuName;
    }
    if (!configPath.isEmpty()) {
        args << "--config" << configPath;
    }

    QString program = QCoreApplication::applicationFilePath();
    QProcess::startDetached(program, args);
}

void ConfigEditorBackend::startIndexingIcons() {
    m_isIndexing = true;
    emit isIndexingIconsChanged();

    std::thread([this]() {
        QSet<QString> uniqueIcons;

        QStringList searchPaths;
        // Nix / System / User icon directories
        searchPaths << "/run/current-system/sw/share/icons"
                    << QDir::homePath() + "/.nix-profile/share/icons"
                    << QDir::homePath() + "/.local/share/icons"
                    << "/usr/share/icons"
                    << "/usr/share/pixmaps";

        for (const QString &base : searchPaths) {
            if (!QDir(base).exists()) continue;

            // FollowSymlinks to resolve Nix store symlinks
            QDirIterator it(base,
                            QDir::Files | QDir::Readable,
                            QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
            while (it.hasNext()) {
                it.next();
                QString fileName = it.fileName();
                if (fileName.endsWith(".png") || fileName.endsWith(".svg") || fileName.endsWith(".xpm")) {
                    int dotIdx = fileName.lastIndexOf('.');
                    QString baseName = fileName.left(dotIdx);
                    // Filter out purely numeric sizes (16, 24, 32, etc.) or @2x scale tags
                    if (!baseName.isEmpty() && !baseName.at(0).isDigit() && !baseName.contains('@')) {
                        uniqueIcons.insert(baseName);
                    }
                }
            }
        }

        QStringList sortedList = uniqueIcons.values();
        std::sort(sortedList.begin(), sortedList.end());

        {
            QMutexLocker locker(&m_mutex);
            m_cachedIcons = sortedList;
            m_isIndexing = false;
        }

        QMetaObject::invokeMethod(this, [this]() {
            emit isIndexingIconsChanged();
            emit iconsIndexed();
        }, Qt::QueuedConnection);
    }).detach();
}
