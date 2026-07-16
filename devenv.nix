{ pkgs, lib, config, inputs, ... }:

let
  # Bundle Qt6 libraries into a single environment package to simplify paths
  qtEnv = pkgs.qt6.env "qt6-env" [
    pkgs.qt6.qtbase
    pkgs.qt6.qtdeclarative
    pkgs.qt6.qtsvg
    pkgs.kdePackages.layer-shell-qt
  ];
in {
  # https://devenv.sh/packages/
  packages = [
    qtEnv
    pkgs.kdePackages.layer-shell-qt
    pkgs.cmake
    pkgs.ninja
    pkgs.pkg-config
    pkgs.gcc
    pkgs.libGL
    pkgs.libGLU
    pkgs.xorg.libX11
    pkgs.xorg.libXext
    pkgs.xorg.libXrender
    pkgs.xorg.libXrandr
  ];

  # Set critical environment variables for running the Qt app during development
  env.QT_PLUGIN_PATH = "${qtEnv}/${pkgs.qt6.qtbase.qtPluginPrefix}";
  env.QML2_IMPORT_PATH = "${qtEnv}/${pkgs.qt6.qtbase.qtQmlPrefix}";
  
  # Default platform plugins (falls back to xcb if Wayland is not running)
  env.QT_QPA_PLATFORM = "wayland;xcb";

  # https://devenv.sh/basics/
  enterShell = ''
    echo "=================================================="
    echo "  drmenu Developer Environment Ready (NixOS)"
    echo "  Compiler: $(g++ --version | head -n1)"
    echo "  CMake: $(cmake --version | head -n1)"
    echo "  Qt6 environment loaded successfully."
    echo "=================================================="
  '';
}

