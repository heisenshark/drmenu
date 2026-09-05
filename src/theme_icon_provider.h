#pragma once

#include <QQuickImageProvider>
#include <QIcon>

// Serves system icon theme icons to QML via the "image://icon/<name>" URL scheme.
// Register with: engine.addImageProvider("icon", new ThemeIconProvider);
class ThemeIconProvider : public QQuickImageProvider {
public:
    ThemeIconProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

    // Resolves an icon name or file path to a QIcon using multi-stage resolution
    static QIcon findIcon(const QString &id);
    // Resolves and scales a pixmap to the requested size
    static QPixmap findPixmap(const QString &id, const QSize &requestedSize);
    // Auto-detects and configures the system icon theme and search paths
    static void initTheme();
};
