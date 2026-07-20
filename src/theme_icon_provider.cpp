#include "theme_icon_provider.h"
#include <QIcon>
#include <QPixmap>

ThemeIconProvider::ThemeIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

QPixmap ThemeIconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) {
    int w = requestedSize.width()  > 0 ? requestedSize.width()  : 48;
    int h = requestedSize.height() > 0 ? requestedSize.height() : 48;

    QIcon icon = QIcon::fromTheme(id);
    // Fallback to a generic app icon if the name wasn't found in the theme
    if (icon.isNull()) icon = QIcon::fromTheme("application-x-executable");
    if (icon.isNull()) icon = QIcon::fromTheme("application-x-desktop");

    QPixmap pixmap = icon.pixmap(w, h);
    if (size) *size = pixmap.size();
    return pixmap;
}
