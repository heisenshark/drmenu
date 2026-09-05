#pragma once

#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <cstdlib>

class QtEnvironmentHelper {
public:
    static void setup(QQmlApplicationEngine &engine) {
#ifdef QT_DECLARATIVE_QML_PATH
        engine.addImportPath(QStringLiteral(QT_DECLARATIVE_QML_PATH));
#endif
#ifdef QT_DECLARATIVE_PLUGIN_PATH
        QCoreApplication::addLibraryPath(QStringLiteral(QT_DECLARATIVE_PLUGIN_PATH));
#endif

        // Standard Qt library paths
        QString libQml = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
        if (!libQml.isEmpty()) engine.addImportPath(libQml);
        QString libPlugins = QLibraryInfo::path(QLibraryInfo::PluginsPath);
        if (!libPlugins.isEmpty()) QCoreApplication::addLibraryPath(libPlugins);

        // NixOS system and user profile paths
        engine.addImportPath(QStringLiteral("/run/current-system/sw/lib/qt-6/qml"));
        engine.addImportPath(QDir::homePath() + QStringLiteral("/.nix-profile/lib/qt-6/qml"));
        engine.addImportPath(QDir::homePath() + QStringLiteral("/.local/state/nix/profile/lib/qt-6/qml"));
        engine.addImportPath(QStringLiteral("/etc/profiles/per-user/") + qgetenv("USER") + QStringLiteral("/lib/qt-6/qml"));

        QCoreApplication::addLibraryPath(QStringLiteral("/run/current-system/sw/lib/qt-6/plugins"));
        QCoreApplication::addLibraryPath(QDir::homePath() + QStringLiteral("/.nix-profile/lib/qt-6/plugins"));
        QCoreApplication::addLibraryPath(QDir::homePath() + QStringLiteral("/.local/state/nix/profile/lib/qt-6/plugins"));
        QCoreApplication::addLibraryPath(QStringLiteral("/etc/profiles/per-user/") + qgetenv("USER") + QStringLiteral("/lib/qt-6/plugins"));

        // Nix environment variables
        const char *varNames[] = {
            "NIXPKGS_QT6_QML_IMPORT_PATH",
            "NIXPKGS_QML_SEARCH_PATHS",
            "QML2_IMPORT_PATH"
        };
        for (const char *var : varNames) {
            const char *val = getenv(var);
            if (val && val[0]) {
                for (const QString &p : QString(val).split(':', Qt::SkipEmptyParts)) {
                    engine.addImportPath(p);
                }
            }
        }

        const char *pluginVal = getenv("QT_PLUGIN_PATH");
        if (pluginVal && pluginVal[0]) {
            for (const QString &p : QString(pluginVal).split(':', Qt::SkipEmptyParts)) {
                QCoreApplication::addLibraryPath(p);
            }
        }
    }
};
