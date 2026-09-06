#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QMutex>

class ConfigEditorBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isIndexingIcons READ isIndexingIcons NOTIFY isIndexingIconsChanged)
    Q_PROPERTY(int totalIconsCount READ totalIconsCount NOTIFY iconsIndexed)

public:
    explicit ConfigEditorBackend(QObject *parent = nullptr);
    ~ConfigEditorBackend() override;

    bool isIndexingIcons() const { return m_isIndexing; }
    int totalIconsCount() const;

    Q_INVOKABLE QStringList availableConfigFiles();
    Q_INVOKABLE QString defaultConfigPath();
    Q_INVOKABLE QVariantMap loadConfig(const QString &filePath);
    Q_INVOKABLE bool saveConfig(const QString &filePath, const QVariantMap &configData);
    Q_INVOKABLE QVariantList installedApps();
    Q_INVOKABLE QVariantMap resolveApp(const QString &appId);
    Q_INVOKABLE QStringList searchSystemIcons(const QString &query, int limit = 120);
    Q_INVOKABLE QStringList availableThemes();
    Q_INVOKABLE void testMenu(const QString &menuName, const QString &configPath);

signals:
    void isIndexingIconsChanged();
    void iconsIndexed();

private:
    void startIndexingIcons();

    bool m_isIndexing = false;
    mutable QMutex m_mutex;
    QStringList m_cachedIcons;
    QVariantList m_cachedApps;
};
