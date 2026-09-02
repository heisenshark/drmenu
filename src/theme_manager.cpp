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
        "apple-glass",
        "apple-glass-light",
        "visionos",
        "apple-glass-pie",
        "apple-lens",
        "apple-lens-light",
        "visionos-lens",
        "liquid-glass",
        "liquid-glass-light",
        "user-glass",
        "monochrome",
        "light"
    };
}

QVariantMap ThemeManager::resolveStyle(const QString &themeName, const QVariantMap &customOverrides) {
    QVariantMap style;
    QString name = themeName.toLower().trimmed();

    // ── Default Theme (Blender Slate & Orange) ─────────────────────────────────
    style["theme"]                = name;
    style["glass"]                = (name.contains("glass") || name.contains("liquid") || name.contains("lens") || name == "visionos");
    style["useGlass"]             = style["glass"];
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
        style["showCenterArc"]        = true;
        style["centerArcAngle"]       = 90;
        style["centerArcColor"]       = "#3b82f6";
        style["centerArcWidth"]       = 4;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "blender") {
        style["layout"]               = "pie";
        style["centerLayout"]         = "torus";
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.0;
        style["pieBackgroundColor"]   = "#000000";
        style["pieBackgroundOpacity"] = 0.15;
        style["pieOuterBorderColor"]  = "transparent";
        style["pieOuterBorderWidth"]  = 0;
        style["innerRadius"]          = 50;
        style["outerRadius"]          = 220;
        style["delimiterWidth"]       = 0;
        style["delimiterColor"]       = "transparent";
        style["pieSliceColor"]        = "transparent";
        style["pieSliceHoverColor"]   = "transparent";
        style["pieSliceSubmenuColor"] = "transparent";
        style["pieSliceSubmenuHoverColor"] = "transparent";
        style["accentColor"]          = "#3b82f6";
        style["submenuAccent"]        = "#60a5fa";
        style["textColor"]            = "#cfcfcf";
        style["textHoverColor"]       = "#ffffff";
        style["fontSize"]             = 14;
        style["iconSize"]             = 22;
        style["torusRadius"]          = 26;
        style["centerTorusThickness"] = 8;
        style["centerTorusColor"]     = "#383838";
        style["centerColor"]          = "transparent";
        style["centerBorderColor"]     = "transparent";
        style["centerBorderHoverColor"]= "transparent";
        style["centerBorderWidth"]    = 0;
        style["centerDotColor"]       = "#808080";
        style["centerDotHoverColor"]  = "#3b82f6";
        style["showCenterArc"]        = true;
        style["centerArcAngle"]       = 90;
        style["centerArcColor"]       = "#3b82f6";
        style["centerArcWidth"]       = 8;
        style["highlightOptionRect"]  = true;
        style["optionRectColor"]       = "#222226";
        style["optionRectHoverColor"] = "#2b5b88";
        style["optionRectBorder"]     = "#333338";
        style["optionRectHoverBorder"]= "#3b82f6";
        style["optionRectRadius"]     = 6;
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
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 26.0;
        style["blurStrength"]         = 26.0;
        style["refractionStrength"]   = 0.85;
        style["chromaticAberration"]  = 1.5;
        style["specularStrength"]     = 0.80;
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.0;
        style["pillColor"]            = "#30ffffff";
        style["pillHoverColor"]       = "#50ffffff";
        style["pillBorderColor"]      = "#45ffffff";
        style["pillBorderHoverColor"] = "#a0ffffff";
        style["pillSubmenuColor"]     = "#25ffffff";
        style["pillSubmenuHoverColor"]= "#45ffffff";
        style["pillSubmenuBorder"]    = "#55ffffff";
        style["pillSubmenuBorderHover"] = "#c0ffffff";
        style["textColor"]            = "#f0f0f0";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#ffffff";
        style["iconColor"]            = "#e0e0e0";
        style["iconHoverColor"]       = "#ffffff";
        style["submenuAccent"]        = "#ffffff";
        style["centerColor"]          = "transparent";
        style["centerBorder"]         = "#45ffffff";
        style["centerBorderHover"]    = "transparent";
        style["centerDotColor"]       = "#60ffffff";
        style["centerDotHoverColor"]  = "#a0ffffff";
        style["guideRingColor"]       = "#ffffff";
        style["guideRingOpacity"]     = 0.15;
        style["breadcrumbColor"]      = "#cccccc";
        style["fontFamily"]           = "SF Pro Display, Inter, Cantarell, Sans";
        style["fontSize"]             = 13;
        style["pillHeight"]           = 44;
        style["pillRadius"]           = 22;
        style["borderWidth"]          = 1;
        style["borderHoverWidth"]     = 2;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "apple-glass" || name == "apple" || name == "apple-dark" || name == "macos-glass") {
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 24.0;
        style["blurStrength"]         = 24.0;
        style["blurHover"]            = 36.0;
        style["refractionStrength"]   = 0.90;
        style["refractionHover"]      = 1.30;
        style["chromaticAberration"]  = 1.4;
        style["chromaticHover"]       = 2.6;
        style["specularStrength"]     = 0.85;
        style["specularHover"]        = 1.20;
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.0;
        style["pillColor"]            = "#35202028";
        style["pillHoverColor"]       = "#55323242";
        style["pillBorderColor"]      = "#35ffffff";
        style["pillBorderHoverColor"] = "#90ffffff";
        style["pillSubmenuColor"]     = "#351e1e2c";
        style["pillSubmenuHoverColor"]= "#552c2c42";
        style["pillSubmenuBorder"]    = "#458282ff";
        style["pillSubmenuBorderHover"] = "#90a0a0ff";
        style["textColor"]            = "#f5f5f7";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#0a84ff";
        style["iconColor"]            = "#ebebf5";
        style["iconHoverColor"]       = "#0a84ff";
        style["submenuAccent"]        = "#bf5af2";
        style["centerColor"]          = "transparent";
        style["centerBorder"]         = "#45ffffff";
        style["centerBorderHover"]    = "transparent";
        style["centerDotColor"]       = "#60ffffff";
        style["centerDotHoverColor"]  = "#a0ffffff";
        style["guideRingColor"]       = "#ffffff";
        style["guideRingOpacity"]     = 0.08;
        style["breadcrumbColor"]      = "#86868b";
        style["fontFamily"]           = "SF Pro Display, Inter, Cantarell, Sans";
        style["fontSize"]             = 13;
        style["pillHeight"]           = 44;
        style["pillRadius"]           = 22;
        style["borderWidth"]          = 1;
        style["borderHoverWidth"]     = 2;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "apple-glass-light" || name == "apple-light" || name == "macos-light") {
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 22.0;
        style["blurStrength"]         = 22.0;
        style["blurHover"]            = 32.0;
        style["refractionStrength"]   = 0.80;
        style["refractionHover"]      = 1.20;
        style["chromaticAberration"]  = 1.2;
        style["chromaticHover"]       = 2.2;
        style["specularStrength"]     = 0.95;
        style["specularHover"]        = 1.25;
        style["backgroundColor"]      = "#ffffff";
        style["backgroundOpacity"]    = 0.0;
        style["pillColor"]            = "#60ffffff";
        style["pillHoverColor"]       = "#90ffffff";
        style["pillBorderColor"]      = "#60ffffff";
        style["pillBorderHoverColor"] = "#0071e3";
        style["pillSubmenuColor"]     = "#60f2f2f7";
        style["pillSubmenuHoverColor"]= "#90e5e5ea";
        style["pillSubmenuBorder"]    = "#55d1d1d6";
        style["pillSubmenuBorderHover"] = "#5856d6";
        style["textColor"]            = "#1d1d1f";
        style["textHoverColor"]       = "#000000";
        style["accentColor"]          = "#0071e3";
        style["iconColor"]            = "#424245";
        style["iconHoverColor"]       = "#0071e3";
        style["submenuAccent"]        = "#5856d6";
        style["centerColor"]          = "transparent";
        style["centerBorder"]         = "#60ffffff";
        style["centerBorderHover"]    = "transparent";
        style["centerDotColor"]       = "#601d1d1f";
        style["centerDotHoverColor"]  = "#a01d1d1f";
        style["guideRingColor"]       = "#000000";
        style["guideRingOpacity"]     = 0.06;
        style["breadcrumbColor"]      = "#86868b";
        style["fontFamily"]           = "SF Pro Display, Inter, Cantarell, Sans";
        style["fontSize"]             = 13;
        style["pillHeight"]           = 44;
        style["pillRadius"]           = 22;
        style["borderWidth"]          = 1;
        style["borderHoverWidth"]     = 2;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "visionos" || name == "vision-glass" || name == "vision") {
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 28.0;
        style["blurStrength"]         = 28.0;
        style["blurHover"]            = 42.0;
        style["refractionStrength"]   = 0.95;
        style["refractionHover"]      = 1.40;
        style["chromaticAberration"]  = 1.6;
        style["chromaticHover"]       = 3.0;
        style["specularStrength"]     = 0.90;
        style["specularHover"]        = 1.30;
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.0;
        style["pillColor"]            = "#30ffffff";
        style["pillHoverColor"]       = "#55ffffff";
        style["pillBorderColor"]      = "#40ffffff";
        style["pillBorderHoverColor"] = "#c0ffffff";
        style["pillSubmenuColor"]     = "#28ffffff";
        style["pillSubmenuHoverColor"]= "#48ffffff";
        style["pillSubmenuBorder"]    = "#50ffffff";
        style["pillSubmenuBorderHover"] = "#64d2ff";
        style["textColor"]            = "#f5f5f7";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#64d2ff";
        style["iconColor"]            = "#d1d1d6";
        style["iconHoverColor"]       = "#64d2ff";
        style["submenuAccent"]        = "#bf5af2";
        style["centerColor"]          = "transparent";
        style["centerBorder"]         = "#45ffffff";
        style["centerBorderHover"]    = "transparent";
        style["centerDotColor"]       = "#60ffffff";
        style["centerDotHoverColor"]  = "#a0ffffff";
        style["guideRingColor"]       = "#64d2ff";
        style["guideRingOpacity"]     = 0.08;
        style["breadcrumbColor"]      = "#8e8e93";
        style["fontFamily"]           = "SF Pro Display, Inter, Cantarell, Sans";
        style["fontSize"]             = 13;
        style["pillHeight"]           = 44;
        style["pillRadius"]           = 16;
        style["borderWidth"]          = 1;
        style["borderHoverWidth"]     = 2;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "apple-glass-pie" || name == "apple-pie") {
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 24.0;
        style["blurStrength"]         = 24.0;
        style["refractionStrength"]   = 0.85;
        style["chromaticAberration"]  = 1.4;
        style["specularStrength"]     = 0.80;
        style["layout"]               = "pie";
        style["centerLayout"]         = "torus";
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.0;
        style["pieBackgroundColor"]   = "#40121218";
        style["pieBackgroundOpacity"] = 1.0;
        style["pieOuterBorderColor"]  = "#40ffffff";
        style["pieOuterBorderWidth"]  = 1;
        style["innerRadius"]          = 55;
        style["outerRadius"]          = 225;
        style["delimiterWidth"]       = 1;
        style["delimiterColor"]       = "#30ffffff";
        style["pieSliceColor"]        = "#25ffffff";
        style["pieSliceHoverColor"]   = "#48ffffff";
        style["pieSliceSubmenuColor"] = "#25ffffff";
        style["pieSliceSubmenuHoverColor"] = "#48ffffff";
        style["highlightOptionRect"]  = true;
        style["optionRectColor"]       = "#30ffffff";
        style["optionRectHoverColor"] = "#600a84ff";
        style["optionRectBorder"]     = "#45ffffff";
        style["optionRectHoverBorder"]= "#0a84ff";
        style["optionRectRadius"]     = 12;
        style["accentColor"]          = "#0a84ff";
        style["submenuAccent"]        = "#bf5af2";
        style["textColor"]            = "#f5f5f7";
        style["textHoverColor"]       = "#ffffff";
        style["fontSize"]             = 14;
        style["iconSize"]             = 22;
        style["torusRadius"]          = 28;
        style["centerTorusThickness"] = 6;
        style["centerTorusColor"]     = "#40ffffff";
        style["showCenterArc"]        = true;
        style["centerArcAngle"]       = 90;
        style["centerArcColor"]       = "#0a84ff";
        style["centerArcWidth"]       = 6;
        style["centerColor"]          = "transparent";
        style["centerBorderColor"]     = "transparent";
        style["centerBorderHoverColor"]= "transparent";
        style["centerBorderWidth"]    = 0;
        style["centerDotColor"]       = "#60ffffff";
        style["centerDotHoverColor"]  = "#a0ffffff";
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "apple-lens" || name == "apple-optical" || name == "lens-dark") {
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 0.0;
        style["blurStrength"]         = 0.0;
        style["blurHover"]            = 0.0;
        style["refractionStrength"]   = 1.15;
        style["refractionHover"]      = 1.60;
        style["chromaticAberration"]  = 1.8;
        style["chromaticHover"]       = 3.5;
        style["specularStrength"]     = 0.90;
        style["specularHover"]        = 1.35;
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.0;
        style["pillColor"]            = "#18ffffff";
        style["pillHoverColor"]       = "#35ffffff";
        style["pillBorderColor"]      = "#45ffffff";
        style["pillBorderHoverColor"] = "#0a84ff";
        style["pillSubmenuColor"]     = "#18ffffff";
        style["pillSubmenuHoverColor"]= "#35ffffff";
        style["pillSubmenuBorder"]    = "#45bf5af2";
        style["pillSubmenuBorderHover"] = "#bf5af2";
        style["textColor"]            = "#f5f5f7";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#0a84ff";
        style["iconColor"]            = "#ebebf5";
        style["iconHoverColor"]       = "#0a84ff";
        style["submenuAccent"]        = "#bf5af2";
        style["centerColor"]          = "transparent";
        style["centerBorder"]         = "#45ffffff";
        style["centerBorderHover"]    = "transparent";
        style["centerDotColor"]       = "#60ffffff";
        style["centerDotHoverColor"]  = "#a0ffffff";
        style["fontFamily"]           = "SF Pro Display, Inter, Cantarell, Sans";
        style["fontSize"]             = 13;
        style["pillHeight"]           = 44;
        style["pillRadius"]           = 22;
        style["borderWidth"]          = 1;
        style["borderHoverWidth"]     = 2;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "apple-lens-light" || name == "lens-light") {
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 0.0;
        style["blurStrength"]         = 0.0;
        style["blurHover"]            = 0.0;
        style["refractionStrength"]   = 1.10;
        style["refractionHover"]      = 1.50;
        style["chromaticAberration"]  = 1.6;
        style["chromaticHover"]       = 3.2;
        style["specularStrength"]     = 1.00;
        style["specularHover"]        = 1.40;
        style["backgroundColor"]      = "#ffffff";
        style["backgroundOpacity"]    = 0.0;
        style["pillColor"]            = "#25ffffff";
        style["pillHoverColor"]       = "#50ffffff";
        style["pillBorderColor"]      = "#60ffffff";
        style["pillBorderHoverColor"] = "#0071e3";
        style["pillSubmenuColor"]     = "#25ffffff";
        style["pillSubmenuHoverColor"]= "#50ffffff";
        style["pillSubmenuBorder"]    = "#55d1d1d6";
        style["pillSubmenuBorderHover"] = "#5856d6";
        style["textColor"]            = "#1d1d1f";
        style["textHoverColor"]       = "#000000";
        style["accentColor"]          = "#0071e3";
        style["iconColor"]            = "#424245";
        style["iconHoverColor"]       = "#0071e3";
        style["submenuAccent"]        = "#5856d6";
        style["centerColor"]          = "transparent";
        style["centerBorder"]         = "#60ffffff";
        style["centerBorderHover"]    = "transparent";
        style["centerDotColor"]       = "#601d1d1f";
        style["centerDotHoverColor"]  = "#a01d1d1f";
        style["fontFamily"]           = "SF Pro Display, Inter, Cantarell, Sans";
        style["fontSize"]             = 13;
        style["pillHeight"]           = 44;
        style["pillRadius"]           = 22;
        style["borderWidth"]          = 1;
        style["borderHoverWidth"]     = 2;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "visionos-lens" || name == "vision-lens") {
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 0.0;
        style["blurStrength"]         = 0.0;
        style["blurHover"]            = 0.0;
        style["refractionStrength"]   = 1.20;
        style["refractionHover"]      = 1.70;
        style["chromaticAberration"]  = 2.0;
        style["chromaticHover"]       = 4.0;
        style["specularStrength"]     = 0.95;
        style["specularHover"]        = 1.40;
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.0;
        style["pillColor"]            = "#20ffffff";
        style["pillHoverColor"]       = "#40ffffff";
        style["pillBorderColor"]      = "#40ffffff";
        style["pillBorderHoverColor"] = "#64d2ff";
        style["pillSubmenuColor"]     = "#20ffffff";
        style["pillSubmenuHoverColor"]= "#40ffffff";
        style["pillSubmenuBorder"]    = "#40ffffff";
        style["pillSubmenuBorderHover"] = "#64d2ff";
        style["textColor"]            = "#f5f5f7";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#64d2ff";
        style["iconColor"]            = "#d1d1d6";
        style["iconHoverColor"]       = "#64d2ff";
        style["submenuAccent"]        = "#bf5af2";
        style["centerColor"]          = "transparent";
        style["centerBorder"]         = "#45ffffff";
        style["centerBorderHover"]    = "transparent";
        style["centerDotColor"]       = "#60ffffff";
        style["centerDotHoverColor"]  = "#a0ffffff";
        style["fontFamily"]           = "SF Pro Display, Inter, Cantarell, Sans";
        style["fontSize"]             = 13;
        style["pillHeight"]           = 44;
        style["pillRadius"]           = 16;
        style["borderWidth"]          = 1;
        style["borderHoverWidth"]     = 2;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "liquid-glass" || name == "apple-liquid" || name == "liquid" || name == "liquid-dark") {
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 22.0;
        style["blurStrength"]         = 22.0;
        style["blurHover"]            = 32.0;
        style["refractionStrength"]   = 0.85;
        style["refractionHover"]      = 1.25;
        style["chromaticAberration"]  = 1.4;
        style["chromaticHover"]       = 2.6;
        style["specularStrength"]     = 0.75;
        style["specularHover"]        = 1.15;
        style["backgroundColor"]      = "#000000";
        style["backgroundOpacity"]    = 0.0;
        style["pillColor"]            = "#25121218";
        style["pillHoverColor"]       = "#4520202c";
        style["pillBorderColor"]      = "#45ffffff";
        style["pillBorderHoverColor"] = "#b0ffffff";
        style["pillSubmenuColor"]     = "#25121218";
        style["pillSubmenuHoverColor"]= "#4520202c";
        style["pillSubmenuBorder"]    = "#45ffffff";
        style["pillSubmenuBorderHover"] = "#b0ffffff";
        style["textColor"]            = "#f5f5f7";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "#ffffff";
        style["iconColor"]            = "#ebebf5";
        style["iconHoverColor"]       = "#ffffff";
        style["submenuAccent"]        = "transparent";
        style["showSubmenuAccent"]    = false;
        style["centerColor"]          = "transparent";
        style["centerBorder"]         = "#45ffffff";
        style["centerBorderHover"]    = "transparent";
        style["centerDotColor"]       = "#60ffffff";
        style["centerDotHoverColor"]  = "#a0ffffff";
        style["fontFamily"]           = "SF Pro Display, -apple-system, Inter, Cantarell, Sans";
        style["fontSize"]             = 13;
        style["pillHeight"]           = 46;
        style["pillRadius"]           = 18;
        style["borderWidth"]          = 1;
        style["borderHoverWidth"]     = 2;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "liquid-glass-light" || name == "liquid-light") {
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 20.0;
        style["blurStrength"]         = 20.0;
        style["blurHover"]            = 30.0;
        style["refractionStrength"]   = 0.80;
        style["refractionHover"]      = 1.20;
        style["chromaticAberration"]  = 1.3;
        style["chromaticHover"]       = 2.4;
        style["specularStrength"]     = 0.90;
        style["specularHover"]        = 1.25;
        style["backgroundColor"]      = "#ffffff";
        style["backgroundOpacity"]    = 0.0;
        style["pillColor"]            = "#55fcfcfd";
        style["pillHoverColor"]       = "#85ffffff";
        style["pillBorderColor"]      = "#70ffffff";
        style["pillBorderHoverColor"] = "#0071e3";
        style["pillSubmenuColor"]     = "#55fcfcfd";
        style["pillSubmenuHoverColor"]= "#85ffffff";
        style["pillSubmenuBorder"]    = "#70ffffff";
        style["pillSubmenuBorderHover"] = "#0071e3";
        style["textColor"]            = "#1d1d1f";
        style["textHoverColor"]       = "#000000";
        style["accentColor"]          = "#0071e3";
        style["iconColor"]            = "#3a3a3c";
        style["iconHoverColor"]       = "#0071e3";
        style["submenuAccent"]        = "transparent";
        style["showSubmenuAccent"]    = false;
        style["centerColor"]          = "transparent";
        style["centerBorder"]         = "#60ffffff";
        style["centerBorderHover"]    = "transparent";
        style["centerDotColor"]       = "#601d1d1f";
        style["centerDotHoverColor"]  = "#a01d1d1f";
        style["fontFamily"]           = "SF Pro Display, -apple-system, Inter, Cantarell, Sans";
        style["fontSize"]             = 13;
        style["pillHeight"]           = 46;
        style["pillRadius"]           = 18;
        style["borderWidth"]          = 1;
        style["borderHoverWidth"]     = 2;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
    } else if (name == "user-glass" || name == "user_glass" || name == "user") {
        style["glass"]                = true;
        style["useGlass"]             = true;
        style["blur"]                 = 1.5;
        style["blurStrength"]         = 1.5;
        style["blurHover"]            = 48.0;
        style["refractionStrength"]   = 0.30;
        style["refractionHover"]      = 0.60;
        style["chromaticAberration"]  = 0.5;
        style["chromaticHover"]       = 2.0;
        style["specularStrength"]     = 0.01;
        style["specularHover"]        = 0.35;
        style["backgroundColor"]      = "transparent";
        style["backgroundOpacity"]    = 0.0;
        style["pillColor"]            = "#0f000000";
        style["pillHoverColor"]       = "transparent";
        style["pillBorderColor"]      = "transparent";
        style["pillBorderHoverColor"] = "transparent";
        style["pillSubmenuColor"]     = "#0f000000";
        style["pillSubmenuHoverColor"]= "transparent";
        style["pillSubmenuBorder"]    = "transparent";
        style["pillSubmenuBorderHover"] = "transparent";
        style["textColor"]            = "#f5f5f7";
        style["textHoverColor"]       = "#ffffff";
        style["accentColor"]          = "transparent";
        style["iconColor"]            = "#ebebf5";
        style["iconHoverColor"]       = "#ffffff";
        style["submenuAccent"]        = "transparent";
        style["showSubmenuAccent"]    = false;
        style["centerColor"]          = "transparent";
        style["centerBorder"]         = "transparent";
        style["centerBorderHover"]    = "transparent";
        style["centerDotColor"]       = "#60ffffff";
        style["centerDotHoverColor"]  = "#a0ffffff";
        style["fontFamily"]           = "SF Pro Display, -apple-system, Inter, Cantarell, Sans";
        style["fontSize"]             = 13;
        style["pillHeight"]           = 44;
        style["pillRadius"]           = 22;
        style["radiusDistance"]       = 200;
        style["borderWidth"]          = 0;
        style["borderHoverWidth"]     = 0;
        style["hoverDuration"]        = 200;
        style["showGuideRing"]        = false;
        style["showPointerLine"]      = false;
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

    // Apply custom overrides on top of resolved preset with alias normalization
    for (auto it = customOverrides.cbegin(); it != customOverrides.cend(); ++it) {
        QString key = it.key();
        QVariant val = it.value();
        style[key] = val;

        // Normalise snake_case <-> camelCase aliases for liquid glass & blur controls
        if (key == "blur" || key == "blur_radius" || key == "blurRadius" || key == "blur_strength" || key == "blurStrength" ||
            key == "glass_blur_radius" || key == "glassBlurRadius" || key == "glass_blur" || key == "glassBlur" ||
            key == "screencopy_blur_radius" || key == "screencopyBlurRadius") {
            style["screencopyBlurRadius"] = val;
            style["blurRadius"]           = val;
            style["blur_radius"]          = val;
            style["blurStrength"]         = val;
            style["blur_strength"]        = val;
            style["blur"]                 = val;
            style["glassBlurRadius"]      = val;
        }
        if (key == "screencopy_vibrancy" || key == "screencopyVibrancy" || key == "vibrancy" ||
            key == "glass_vibrancy" || key == "glassVibrancy") {
            style["screencopyVibrancy"] = val;
            style["vibrancy"]           = val;
        }
        if (key == "glass" || key == "use_glass" || key == "useGlass") {
            style["useGlass"]           = val;
            style["glass"]              = val;
        }
        if (key == "screencopy_glass" || key == "useScreencopyGlass") {
            style["useScreencopyGlass"] = val;
        }
        if (key == "live" || key == "live_blur" || key == "liveBlur" || key == "screencopy_live" || key == "screencopyLive" || key == "live_capture" || key == "liveCapture") {
            style["screencopyLive"] = val;
            style["screencopy_live"] = val;
            style["liveBlur"]       = val;
            style["live_blur"]      = val;
            style["live"]           = val;
        }
        if (key == "fps" || key == "screencopy_fps" || key == "screencopyFps") {
            style["screencopyFps"] = val;
            style["fps"]           = val;
        }
        if (key == "chromatic_aberration" || key == "chromaticAberration" || key == "chromatic_dispersion" || key == "chromaticDispersion") {
            style["chromaticAberration"] = val;
            style["chromatic_aberration"] = val;
        }
        if (key == "chromatic_opacity")      style["chromaticOpacity"]     = val;
        if (key == "chromaticOpacity")       style["chromatic_opacity"]    = val;
        if (key == "chromatic_border_width") style["chromaticBorderWidth"] = val;
        if (key == "chromaticBorderWidth")   style["chromatic_border_width"] = val;
        if (key == "specular" || key == "specular_strength" || key == "specularStrength") {
            style["specularStrength"]  = val;
            style["specular_strength"] = val;
            style["specular"]          = val;
        }
        if (key == "blur_strength")          style["blurStrength"]         = val;
        if (key == "blurStrength")           style["blur_strength"]        = val;
        if (key == "refraction_strength")    style["refractionStrength"]   = val;
        if (key == "refractionStrength")     style["refraction_strength"]  = val;
        if (key == "corner_radius" || key == "pill_radius") {
            style["pillRadius"] = val;
            style["corner_radius"] = val;
        }
        if (key == "pillRadius") {
            style["corner_radius"] = val;
        }
        if (key == "pill_height")            style["pillHeight"]           = val;
        if (key == "pillHeight")             style["pill_height"]          = val;
        if (key == "pill_color")             style["pillColor"]            = val;
        if (key == "pillColor")              style["pill_color"]           = val;
        if (key == "pill_hover_color")       style["pillHoverColor"]       = val;
        if (key == "pillHoverColor")         style["pill_hover_color"]     = val;
        if (key == "border_color")           style["pillBorderColor"]      = val;
        if (key == "border_width")           style["borderWidth"]          = val;
        if (key == "border_hover_width")     style["borderHoverWidth"]     = val;
        if (key == "blur_hover" || key == "blurHover" || key == "blur_strength_hover" || key == "blurStrengthHover" || key == "blur_radius_hover" || key == "blurRadiusHover") {
            style["blurHover"]           = val;
            style["blur_hover"]          = val;
            style["blurStrengthHover"]   = val;
            style["blur_strength_hover"] = val;
        }
        if (key == "refraction_hover" || key == "refractionHover" || key == "refraction_strength_hover" || key == "refractionStrengthHover") {
            style["refractionHover"]           = val;
            style["refraction_hover"]          = val;
            style["refractionStrengthHover"]   = val;
            style["refraction_strength_hover"] = val;
        }
        if (key == "chromatic_hover" || key == "chromaticHover" || key == "chromatic_aberration_hover" || key == "chromaticAberrationHover") {
            style["chromaticHover"]             = val;
            style["chromatic_hover"]            = val;
            style["chromaticAberrationHover"]   = val;
            style["chromatic_aberration_hover"] = val;
        }
        if (key == "specular_hover" || key == "specularHover" || key == "specular_strength_hover" || key == "specularStrengthHover") {
            style["specularHover"]           = val;
            style["specular_hover"]          = val;
            style["specularStrengthHover"]   = val;
            style["specular_strength_hover"] = val;
        }
        if (key == "hover_duration" || key == "hoverDuration" || key == "transition_duration" || key == "transitionDuration" || key == "animation_duration" || key == "animationDuration") {
            style["hoverDuration"]        = val;
            style["hover_duration"]       = val;
            style["transitionDuration"]   = val;
            style["transition_duration"]  = val;
        }
        if (key == "hover_scale_duration" || key == "hoverScaleDuration" || key == "scale_duration" || key == "scaleDuration") {
            style["hoverScaleDuration"]   = val;
            style["hover_scale_duration"] = val;
            style["scaleDuration"]        = val;
            style["scale_duration"]       = val;
        }
        if (key == "text_shadow" || key == "textShadow" || key == "show_text_shadow" || key == "showTextShadow") {
            style["showTextShadow"]       = val;
            style["show_text_shadow"]      = val;
        }
        if (key == "text_shadow_color" || key == "textShadowColor") {
            style["textShadowColor"]      = val;
            style["text_shadow_color"]     = val;
        }
        if (key == "font_size" || key == "fontSize") {
            style["fontSize"]  = val;
            style["font_size"] = val;
        }
        if (key == "icon_size" || key == "iconSize") {
            style["iconSize"]  = val;
            style["icon_size"] = val;
        }
        if (key == "font_family" || key == "fontFamily") {
            style["fontFamily"]  = val;
            style["font_family"] = val;
        }
        if (key == "pie_slice_color" || key == "pieSliceColor") {
            style["pieSliceColor"] = val;
            style["pie_slice_color"] = val;
        }
        if (key == "pie_slice_hover_color" || key == "pieSliceHoverColor") {
            style["pieSliceHoverColor"] = val;
            style["pie_slice_hover_color"] = val;
        }
        if (key == "pie_background_color" || key == "pieBackgroundColor") {
            style["pieBackgroundColor"] = val;
            style["pie_background_color"] = val;
        }
        if (key == "pie_background_opacity" || key == "pieBackgroundOpacity") {
            style["pieBackgroundOpacity"] = val;
            style["pie_background_opacity"] = val;
        }
        if (key == "pie_outer_border_color" || key == "pieOuterBorderColor") {
            style["pieOuterBorderColor"] = val;
            style["pie_outer_border_color"] = val;
        }
        if (key == "pie_outer_border_width" || key == "pieOuterBorderWidth") {
            style["pieOuterBorderWidth"] = val;
            style["pie_outer_border_width"] = val;
        }
        if (key == "delimiter_color" || key == "delimiterColor") {
            style["delimiterColor"] = val;
            style["delimiter_color"] = val;
        }
        if (key == "delimiter_width" || key == "delimiterWidth") {
            style["delimiterWidth"] = val;
            style["delimiter_width"] = val;
        }
    }

    if (style["glass"] == true || style["useGlass"] == true) {
        if (!style.contains("pieBackgroundColor")) style["pieBackgroundColor"] = "transparent";
        if (!style.contains("pieBackgroundOpacity")) style["pieBackgroundOpacity"] = 0.0;
        if (!style.contains("pieSliceColor")) style["pieSliceColor"] = "#18ffffff";
        if (!style.contains("pieSliceHoverColor")) style["pieSliceHoverColor"] = "#45ffffff";
        if (!style.contains("pieOuterBorderColor")) style["pieOuterBorderColor"] = "#40ffffff";
        if (!style.contains("pieOuterBorderWidth")) style["pieOuterBorderWidth"] = 1;
        if (!style.contains("delimiterColor")) style["delimiterColor"] = "#30ffffff";
        if (!style.contains("delimiterWidth")) style["delimiterWidth"] = 1;
        if (!style.contains("optionRectColor")) style["optionRectColor"] = "#25ffffff";
        if (!style.contains("optionRectHoverColor")) style["optionRectHoverColor"] = "#600a84ff";
        if (!style.contains("optionRectBorder")) style["optionRectBorder"] = "#35ffffff";
        if (!style.contains("optionRectHoverBorder")) style["optionRectHoverBorder"] = "#0a84ff";
    }

    return style;
}
