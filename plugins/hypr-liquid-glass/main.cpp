#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include "Shaders.hpp"

inline HANDLE PHANDLE = nullptr;

struct ConfigValues {
    SP<Config::Values::CIntValue>   blurStrength;
    SP<Config::Values::CFloatValue> refractionStrength;
    SP<Config::Values::CFloatValue> chromaticAberration;
    SP<Config::Values::CFloatValue> specularStrength;
    SP<Config::Values::CFloatValue> cornerRadius;
};

static ConfigValues g_Config;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

static void registerConfigValues() {
    g_Config.blurStrength = makeShared<Config::Values::CIntValue>(
        "plugin:liquid_glass:blur_strength", "Blur strength passes for liquid glass", 1);

    g_Config.refractionStrength = makeShared<Config::Values::CFloatValue>(
        "plugin:liquid_glass:refraction_strength", "Convex refraction intensity", 0.05f);

    g_Config.chromaticAberration = makeShared<Config::Values::CFloatValue>(
        "plugin:liquid_glass:chromatic_aberration", "Optical chromatic dispersion offset", 0.012f);

    g_Config.specularStrength = makeShared<Config::Values::CFloatValue>(
        "plugin:liquid_glass:specular_strength", "Specular Fresnel gloss intensity", 0.30f);

    g_Config.cornerRadius = makeShared<Config::Values::CFloatValue>(
        "plugin:liquid_glass:corner_radius", "Glass squircle corner radius", 18.0f);
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string hyprlandHash = __hyprland_api_get_hash();
    const std::string pluginHash   = __hyprland_api_get_client_hash();

    if (hyprlandHash != pluginHash) {
        HyprlandAPI::addNotificationV2(PHANDLE, {
            {"text", std::string("[hypr-liquid-glass] ABI Mismatch: Built with " + pluginHash + ", running " + hyprlandHash)},
            {"time", (uint64_t)6000},
            {"color", CHyprColor(1.0, 0.2, 0.2, 1.0)}
        });
        throw std::runtime_error("hypr-liquid-glass: Hyprland ABI version mismatch");
    }

    registerConfigValues();

    // Register runtime dispatchers
    HyprlandAPI::addDispatcherV2(PHANDLE, "liquid-glass:set_chromatic", [](std::string args) -> SDispatchResult {
        try {
            float val = std::stof(args);
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

    HyprlandAPI::reloadConfig();

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
