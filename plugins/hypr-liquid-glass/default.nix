{
  lib,
  hyprland,
  hyprlandPlugins,
}:
hyprlandPlugins.mkHyprlandPlugin {
  pluginName = "hypr-liquid-glass";
  version = "0.1.0";
  src = ./.;

  inherit (hyprland) nativeBuildInputs;

  meta = with lib; {
    homepage = "https://github.com/heisenshark/drmenu";
    description = "Apple Liquid Glass & Chromatic Aberration plugin for Hyprland";
    license = licenses.bsd3;
    platforms = platforms.linux;
  };
}
