#!/usr/bin/env bash
set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

usage() {
    echo "Usage: $0 [build|install|pkg|clean|help]"
    echo ""
    echo "Commands:"
    echo "  build    Build binary in release mode using CMake & Ninja (default)"
    echo "  install  Build and install binary to /usr/local via cmake --install"
    echo "  pkg      Build and install Arch Linux package using makepkg -si"
    echo "  clean    Remove build directories and makepkg package outputs"
    echo "  help     Show this help message"
}

case "${1:-build}" in
    build)
        echo "==> Building drmenu..."
        cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
        cmake --build "$BUILD_DIR"
        echo "==> Build complete: ${BUILD_DIR}/bin/drmenu"
        ;;
    install)
        echo "==> Building and installing drmenu..."
        cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
        cmake --build "$BUILD_DIR"
        sudo cmake --install "$BUILD_DIR"
        echo "==> Installation complete!"
        ;;
    pkg)
        echo "==> Building Arch package with makepkg..."
        cd "$PROJECT_DIR"
        makepkg -si
        ;;
    clean)
        echo "==> Cleaning build artifacts..."
        rm -rf "$BUILD_DIR" pkg/ src/drmenu/ *.pkg.tar.zst
        echo "==> Clean complete."
        ;;
    help|--help|-h)
        usage
        ;;
    *)
        echo "Error: Unknown command '$1'"
        usage
        exit 1
        ;;
esac
