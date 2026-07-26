{
  lib,
  stdenv,
  cmake,
  ninja,
  pkg-config,
  qt6,
  kdePackages,
  wrapQtAppsHook ? qt6.wrapQtAppsHook,
}:

stdenv.mkDerivation {
  pname = "drmenu";
  version = "0.1.0";

  src = lib.cleanSource ./..;

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    wrapQtAppsHook
  ];

  buildInputs = [
    qt6.qtbase
    qt6.qtdeclarative
    qt6.qtsvg
    kdePackages.layer-shell-qt
  ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
  ];



  meta = with lib; {
    description = "A radial/pie menu launcher for Wayland compositors";
    homepage = "https://github.com/heisenshark/drmenu";
    license = licenses.mit;
    platforms = platforms.linux;
    mainProgram = "drmenu";
  };
}
