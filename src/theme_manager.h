#pragma once

#include <QString>
#include <QVariantMap>

class ThemeManager {
public:
    // Resolves a theme name + optional custom style override map into a complete style QVariantMap.
    // Theme presets: "blender" (default), "cyberpunk", "nord", "catppuccin", "dracula",
    //                "tokyo-night", "glassmorphism", "monochrome", "light"
    static QVariantMap resolveStyle(const QString &themeName, const QVariantMap &customOverrides = {});

    // Get list of built-in theme names
    static QStringList availableThemes();
};
