#include "theme_manager.h"

QStringList ThemeManager::availableThemes() {
    return {
        "blender",
        "pie",
        "wheel",
        "shotgun",
        "semicircle",
        "cyber-fan",
        "cyberpunk",
        "nord",
        "catppuccin",
        "dracula",
        "tokyo-night",
        "glassmorphism",
        "monochrome",
        "light"
    };
}

QVariantMap ThemeManager::resolveStyle(const QString &themeName, const QVariantMap &customOverrides) {
    QVariantMap style;

    // ── Default Theme (Blender Slate & Orange) ─────────────────────────────────
    style["backgroundColor"]      = "#000000";
    style["backgroundOpacity"]    = 0.38;
    style["pillColor"]            = "#1a1a20";
    style["pillHoverColor"]       = "#2b2b36";
    style["pillBorderColor"]      = "#383842";
    style["pillBorderHoverColor"] = "#e67e22";
    style["pillSubmenuColor"]     = "#18151f";
    style["pillSubmenuHoverColor"]= "#2a2438";
    style["pillSubmenuBorder"]    = "#4a3060";
    style["pillSubmenuBorderHover"] = "#a855f7";
    style["textColor"]            = "#d0d0d5";
    style["textHoverColor"]       = "#ffffff";
    style["accentColor"]          = "#e67e22";
    style["iconColor"]            = "#a0a0ab";
    style["iconHoverColor"]       = "#f39c12";
    style["submenuAccent"]        = "#c084fc";
    style["centerColor"]          = "#18181c";
    style["centerBorder"]         = "#4a4a56";
    style["centerBorderHover"]    = "#e67e22";
    style["centerDotColor"]       = "#808090";
    style["centerDotHoverColor"]  = "#e67e22";
    style["guideRingColor"]       = "#ffffff";
    style["guideRingOpacity"]     = 0.07;
    style["breadcrumbColor"]      = "#606070";
    style["fontFamily"]           = "Sans";
    style["fontSize"]             = 13;
    style["iconSize"]             = 22;
    style["radiusDistance"]       = 185;
    style["pillHeight"]           = 42;
    style["pillRadius"]           = 21; // 21 = full rounded pill
    style["borderWidth"]          = 1;
    style["borderHoverWidth"]     = 2;
    style["showGuideRing"]        = true;
    style["showPointerLine"]      = true;
    style["showBreadcrumbs"]      = true;

    QString name = themeName.toLower().trimmed();

    if (name == "pie" || name == "wheel") {
        style["layout"]               = "pie";
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.50;
        style["pieBackgroundColor"]   = "#14141d";
        style["pieBackgroundOpacity"] = 0.95;
        style["pieOuterBorderColor"]  = "#5a5a72";
        style["pieOuterBorderWidth"]  = 3;
        style["innerRadius"]          = 65;
        style["outerRadius"]          = 230;
        style["delimiterWidth"]       = 2;
        style["delimiterColor"]       = "#383848";
        style["pieSliceColor"]        = "#1e1e2a";
        style["pieSliceHoverColor"]   = "#323246";
        style["pieSliceSubmenuColor"] = "#1e1e2a";
        style["pieSliceSubmenuHoverColor"] = "#323246";
        style["accentColor"]          = "#e67e22";
        style["submenuAccent"]        = "#c084fc";
        style["textColor"]            = "#e2e2ec";
        style["textHoverColor"]       = "#ffffff";
        style["fontSize"]             = 15;
        style["iconSize"]             = 26;
        style["centerColor"]          = "#111116";
        style["centerBorderColor"]     = "#5a5a72";
        style["centerBorderHoverColor"]= "#e67e22";
        style["centerBorderWidth"]    = 3;
        style["centerDotColor"]       = "#808090";
        style["centerDotHoverColor"]  = "#e67e22";
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "shotgun") {
        style["backgroundColor"]      = "#0b0402";
        style["backgroundOpacity"]    = 0.50;
        style["pillColor"]            = "#1c0c06";
        style["pillHoverColor"]       = "#361608";
        style["pillBorderColor"]      = "#54200a";
        style["pillBorderHoverColor"] = "#ff4400";
        style["textColor"]            = "#ffaa88";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#ff4400";
        style["iconColor"]            = "#ff5500";
        style["iconHoverColor"]       = "#ffaa00";
        style["centerColor"]          = "#1c0c06";
        style["centerBorder"]         = "#54200a";
        style["centerBorderHover"]    = "#ff4400";
        style["centerDotColor"]       = "#ff5500";
        style["centerDotHoverColor"]  = "#ffaa00";
        style["layout"]               = "shotgun";
        style["startAngle"]           = -150.0;
        style["spreadAngle"]          = 120.0;
        style["pillRadius"]           = 6;
        style["radiusDistance"]       = 170;
        style["showGuideRing"]        = false;
    } else if (name == "semicircle") {
        style["layout"]               = "semicircle";
        style["startAngle"]           = -180.0;
        style["spreadAngle"]          = 180.0;
        style["radiusDistance"]       = 190;
    } else if (name == "cyber-fan") {
        style["layout"]               = "arc-right";
        style["startAngle"]           = -45.0;
        style["spreadAngle"]          = 90.0;
        style["accentColor"]          = "#00f0ff";
        style["pillBorderHoverColor"] = "#ff007f";
        style["radiusDistance"]       = 200;
    } else if (name == "cyberpunk") {
        style["backgroundColor"]      = "#0a0412";
        style["backgroundOpacity"]    = 0.55;
        style["pillColor"]            = "#120924";
        style["pillHoverColor"]       = "#241042";
        style["pillBorderColor"]      = "#00f0ff";
        style["pillBorderHoverColor"] = "#ff007f";
        style["pillSubmenuColor"]     = "#1a0826";
        style["pillSubmenuHoverColor"]= "#380d52";
        style["pillSubmenuBorder"]    = "#bd00ff";
        style["pillSubmenuBorderHover"] = "#00ffff";
        style["textColor"]            = "#00f0ff";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#ff007f";
        style["iconColor"]            = "#ff007f";
        style["iconHoverColor"]       = "#00f0ff";
        style["submenuAccent"]        = "#bd00ff";
        style["centerColor"]          = "#120924";
        style["centerBorder"]         = "#00f0ff";
        style["centerBorderHover"]    = "#ff007f";
        style["centerDotColor"]       = "#00f0ff";
        style["centerDotHoverColor"]  = "#ff007f";
        style["guideRingColor"]       = "#ff007f";
        style["guideRingOpacity"]     = 0.15;
        style["breadcrumbColor"]      = "#bd00ff";
    } else if (name == "nord") {
        style["backgroundColor"]      = "#2e3440";
        style["backgroundOpacity"]    = 0.45;
        style["pillColor"]            = "#3b4252";
        style["pillHoverColor"]       = "#434c5e";
        style["pillBorderColor"]      = "#4c566a";
        style["pillBorderHoverColor"] = "#88c0d0";
        style["pillSubmenuColor"]     = "#2e3440";
        style["pillSubmenuHoverColor"]= "#3b4252";
        style["pillSubmenuBorder"]    = "#5e81ac";
        style["pillSubmenuBorderHover"] = "#81a1c1";
        style["textColor"]            = "#e5e9f0";
        style["textHoverColor"]       = "#eceff4";
        style["accentColor"]          = "#88c0d0";
        style["iconColor"]            = "#81a1c1";
        style["iconHoverColor"]       = "#88c0d0";
        style["submenuAccent"]        = "#b48ead";
        style["centerColor"]          = "#2e3440";
        style["centerBorder"]         = "#4c566a";
        style["centerBorderHover"]    = "#88c0d0";
        style["centerDotColor"]       = "#d8dee9";
        style["centerDotHoverColor"]  = "#88c0d0";
        style["guideRingColor"]       = "#88c0d0";
        style["guideRingOpacity"]     = 0.10;
        style["breadcrumbColor"]      = "#d8dee9";
    } else if (name == "catppuccin") {
        style["backgroundColor"]      = "#11111b";
        style["backgroundOpacity"]    = 0.45;
        style["pillColor"]            = "#1e1e2e";
        style["pillHoverColor"]       = "#313244";
        style["pillBorderColor"]      = "#45475a";
        style["pillBorderHoverColor"] = "#cba6f7";
        style["pillSubmenuColor"]     = "#181825";
        style["pillSubmenuHoverColor"]= "#313244";
        style["pillSubmenuBorder"]    = "#585b70";
        style["pillSubmenuBorderHover"] = "#f5c2e7";
        style["textColor"]            = "#cdd6f4";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#cba6f7";
        style["iconColor"]            = "#89b4fa";
        style["iconHoverColor"]       = "#f9e2af";
        style["submenuAccent"]        = "#f5c2e7";
        style["centerColor"]          = "#181825";
        style["centerBorder"]         = "#45475a";
        style["centerBorderHover"]    = "#cba6f7";
        style["centerDotColor"]       = "#a6adc8";
        style["centerDotHoverColor"]  = "#cba6f7";
        style["guideRingColor"]       = "#cba6f7";
        style["guideRingOpacity"]     = 0.10;
        style["breadcrumbColor"]      = "#a6adc8";
    } else if (name == "dracula") {
        style["backgroundColor"]      = "#282a36";
        style["backgroundOpacity"]    = 0.50;
        style["pillColor"]            = "#44475a";
        style["pillHoverColor"]       = "#6272a4";
        style["pillBorderColor"]      = "#6272a4";
        style["pillBorderHoverColor"] = "#ff79c6";
        style["pillSubmenuColor"]     = "#343746";
        style["pillSubmenuHoverColor"]= "#44475a";
        style["pillSubmenuBorder"]    = "#bd93f9";
        style["pillSubmenuBorderHover"] = "#ff79c6";
        style["textColor"]            = "#f8f8f2";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#ff79c6";
        style["iconColor"]            = "#8be9fd";
        style["iconHoverColor"]       = "#50fa7b";
        style["submenuAccent"]        = "#bd93f9";
        style["centerColor"]          = "#282a36";
        style["centerBorder"]         = "#6272a4";
        style["centerBorderHover"]    = "#ff79c6";
        style["centerDotColor"]       = "#f8f8f2";
        style["centerDotHoverColor"]  = "#ff79c6";
        style["guideRingColor"]       = "#bd93f9";
        style["guideRingOpacity"]     = 0.12;
        style["breadcrumbColor"]      = "#bd93f9";
    } else if (name == "tokyo-night") {
        style["backgroundColor"]      = "#1a1b26";
        style["backgroundOpacity"]    = 0.50;
        style["pillColor"]            = "#24283b";
        style["pillHoverColor"]       = "#414868";
        style["pillBorderColor"]      = "#565f89";
        style["pillBorderHoverColor"] = "#7aa2f7";
        style["pillSubmenuColor"]     = "#1f2335";
        style["pillSubmenuHoverColor"]= "#3b4261";
        style["pillSubmenuBorder"]    = "#bb9af7";
        style["pillSubmenuBorderHover"] = "#7dcfff";
        style["textColor"]            = "#c0caf5";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#7aa2f7";
        style["iconColor"]            = "#7dcfff";
        style["iconHoverColor"]       = "#e0af68";
        style["submenuAccent"]        = "#bb9af7";
        style["centerColor"]          = "#16161e";
        style["centerBorder"]         = "#565f89";
        style["centerBorderHover"]    = "#7aa2f7";
        style["centerDotColor"]       = "#a9b1d6";
        style["centerDotHoverColor"]  = "#7aa2f7";
        style["guideRingColor"]       = "#7aa2f7";
        style["guideRingOpacity"]     = 0.12;
        style["breadcrumbColor"]      = "#9aa5ce";
    } else if (name == "glassmorphism") {
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.25;
        style["pillColor"]            = "#20ffffff"; // 12% white translucent
        style["pillHoverColor"]       = "#40ffffff"; // 25% white translucent
        style["pillBorderColor"]      = "#30ffffff";
        style["pillBorderHoverColor"] = "#80ffffff";
        style["pillSubmenuColor"]     = "#18ffffff";
        style["pillSubmenuHoverColor"]= "#35ffffff";
        style["pillSubmenuBorder"]    = "#40ffffff";
        style["pillSubmenuBorderHover"] = "#a0ffffff";
        style["textColor"]            = "#f0f0f0";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#ffffff";
        style["iconColor"]            = "#e0e0e0";
        style["iconHoverColor"]       = "#ffffff";
        style["submenuAccent"]        = "#ffffff";
        style["centerColor"]          = "#20ffffff";
        style["centerBorder"]         = "#40ffffff";
        style["centerBorderHover"]    = "#ffffff";
        style["centerDotColor"]       = "#ffffff";
        style["centerDotHoverColor"]  = "#ffffff";
        style["guideRingColor"]       = "#ffffff";
        style["guideRingOpacity"]     = 0.15;
        style["breadcrumbColor"]      = "#cccccc";
    } else if (name == "monochrome") {
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.40;
        style["pillColor"]            = "#111111";
        style["pillHoverColor"]       = "#222222";
        style["pillBorderColor"]      = "#333333";
        style["pillBorderHoverColor"] = "#ffffff";
        style["pillSubmenuColor"]     = "#111111";
        style["pillSubmenuHoverColor"]= "#222222";
        style["pillSubmenuBorder"]    = "#555555";
        style["pillSubmenuBorderHover"] = "#ffffff";
        style["textColor"]            = "#aaaaaa";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#ffffff";
        style["iconColor"]            = "#aaaaaa";
        style["iconHoverColor"]       = "#ffffff";
        style["submenuAccent"]        = "#ffffff";
        style["centerColor"]          = "#111111";
        style["centerBorder"]         = "#333333";
        style["centerBorderHover"]    = "#ffffff";
        style["centerDotColor"]       = "#888888";
        style["centerDotHoverColor"]  = "#ffffff";
        style["guideRingColor"]       = "#ffffff";
        style["guideRingOpacity"]     = 0.08;
        style["breadcrumbColor"]      = "#888888";
    } else if (name == "light") {
        style["backgroundColor"]      = "#ffffff";
        style["backgroundOpacity"]    = 0.30;
        style["pillColor"]            = "#f0f0f3";
        style["pillHoverColor"]       = "#e4e4e9";
        style["pillBorderColor"]      = "#d0d0d8";
        style["pillBorderHoverColor"] = "#2563eb";
        style["pillSubmenuColor"]     = "#f5f3ff";
        style["pillSubmenuHoverColor"]= "#ede9fe";
        style["pillSubmenuBorder"]    = "#ddd6fe";
        style["pillSubmenuBorderHover"] = "#7c3aed";
        style["textColor"]            = "#1f2937";
        style["textHoverColor"]       = "#000000";
        style["accentColor"]          = "#2563eb";
        style["iconColor"]            = "#4b5563";
        style["iconHoverColor"]       = "#2563eb";
        style["submenuAccent"]        = "#7c3aed";
        style["centerColor"]          = "#ffffff";
        style["centerBorder"]         = "#d0d0d8";
        style["centerBorderHover"]    = "#2563eb";
        style["centerDotColor"]       = "#6b7280";
        style["centerDotHoverColor"]  = "#2563eb";
        style["guideRingColor"]       = "#000000";
        style["guideRingOpacity"]     = 0.08;
        style["breadcrumbColor"]      = "#4b5563";
    }

    // Apply custom overrides on top of resolved preset
    for (auto it = customOverrides.cbegin(); it != customOverrides.cend(); ++it) {
        style[it.key()] = it.value();
    }

    return style;
}
