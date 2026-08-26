#pragma once

#include <QString>
#include <QVariantMap>

class ThemeManager {
public:
    // Resolves a theme name + optional custom style override map into a complete style QVariantMap.
    // Theme presets: "blender" (default), "pie", "wheel", "shotgun", "semicircle", "cyber-fan",
    //                "cyberpunk", "nord", "catppuccin", "dracula", "tokyo-night", "glassmorphism",
    //                "apple-glass", "apple-glass-light", "visionos", "apple-glass-pie",
    //                "apple-lens", "apple-lens-light", "visionos-lens",
    //                "liquid-glass", "liquid-glass-light", "monochrome", "light"
    static QVariantMap resolveStyle(const QString &themeName, const QVariantMap &customOverrides = {});

    // Get list of built-in theme names
    static QStringList availableThemes();
};
