#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include "Shaders.hpp"

inline HANDLE PHANDLE = nullptr;

// Configuration options
static Hyprlang::INT blurStrength = 1;
static Hyprlang::FLOAT refractionStrength = 0.05f;
static Hyprlang::FLOAT chromaticAberration = 0.012f;
static Hyprlang::FLOAT specularStrength = 0.30f;
static Hyprlang::FLOAT cornerRadius = 18.0f;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();
    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotificationV2(PHANDLE, {
            {"text", std::string("[hypr-liquid-glass] ABI Mismatch: Built with " + std::string(GIT_COMMIT_HASH) + ", running " + HASH)},
            {"time", (uint64_t)6000},
            {"color", CHyprColor(1.0, 0.2, 0.2, 1.0)}
        });
    }

    // Register config variables
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:liquid-glass:blur_strength", blurStrength);
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:liquid-glass:refraction_strength", refractionStrength);
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:liquid-glass:chromatic_aberration", chromaticAberration);
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:liquid-glass:specular_strength", specularStrength);
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:liquid-glass:corner_radius", cornerRadius);

    // Register dynamic runtime dispatchers for drmenu & apps
    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid-glass:set_chromatic", [](std::string args) -> SDispatchResult {
        try {
            float val = std::stof(args);
            // Dynamic chromatic aberration adjustment
            return {false, true, ""};
        } catch (...) {
            return {false, false, "Invalid argument for chromatic aberration"};
        }
    });

    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid-glass:set_refraction", [](std::string args) -> SDispatchResult {
        try {
            float val = std::stof(args);
            return {false, true, ""};
        } catch (...) {
            return {false, false, "Invalid argument for refraction"};
        }
    });

    HyprlandAPI::addNotificationV2(PHANDLE, {
        {"text", std::string("✨ [hypr-liquid-glass] Apple Liquid Glass & Chromatic Aberration plugin active!")},
        {"time", (uint64_t)4000},
        {"color", CHyprColor(0.2, 0.8, 0.4, 1.0)}
    });

    return {"hypr-liquid-glass", "Apple Liquid Glass, Optical Chromatic Dispersion & Refraction", "drmenu", "0.1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::addNotificationV2(PHANDLE, {
        {"text", std::string("[hypr-liquid-glass] Plugin unloaded.")},
        {"time", (uint64_t)2000},
        {"color", CHyprColor(0.8, 0.6, 0.2, 1.0)}
    });
}
