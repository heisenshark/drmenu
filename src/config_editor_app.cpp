#include "config_editor_app.h"
#include "config_editor_backend.h"
#include "theme_icon_provider.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QIcon>
#include <QTextStream>

int ConfigEditorApp::run(int argc, char *argv[], const QString &initialConfigPath) {
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    app.setApplicationName("drmenu-config");
    app.setApplicationDisplayName("drmenu Configuration Editor");
    app.setWindowIcon(QIcon::fromTheme("preferences-system"));

    QQmlApplicationEngine engine;
    const char* envQml = getenv("QML2_IMPORT_PATH");
    if (envQml && envQml[0]) {
        for (const QString &p : QString(envQml).split(':', Qt::SkipEmptyParts)) {
            engine.addImportPath(p);
        }
    }

    engine.addImageProvider("icon", new ThemeIconProvider);

    ConfigEditorBackend backend;
    engine.rootContext()->setContextProperty("configBackend", &backend);
    engine.rootContext()->setContextProperty("initialConfigPath", initialConfigPath);

    const QUrl url(QStringLiteral("qrc:/drmenu/src/qml/ConfigEditorWindow.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QTextStream(stderr) << "drmenu: Failed to load ConfigEditorWindow.qml\n";
                QCoreApplication::exit(-1);
            }
            if (auto *win = qobject_cast<QQuickWindow*>(obj)) {
                win->show();
            }
        },
        Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
