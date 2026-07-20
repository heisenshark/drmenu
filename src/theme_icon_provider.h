#pragma once

#include <QQuickImageProvider>

// Serves system icon theme icons to QML via the "image://icon/<name>" URL scheme.
// Register with: engine.addImageProvider("icon", new ThemeIconProvider);
class ThemeIconProvider : public QQuickImageProvider {
public:
    ThemeIconProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
