# Maintainer: heisenshark <heisenshark@gmail.com>
pkgname=drmenu-git
_pkgname=drmenu
pkgver=0.1.0.r22.gdb50944
pkgrel=1
pkgdesc="A radial/pie menu launcher for Wayland compositors"
arch=('x86_64' 'aarch64')
url="https://github.com/heisenshark/drmenu"
license=('MIT')
depends=(
    'qt6-base'
    'qt6-declarative'
    'qt6-svg'
    'layer-shell-qt'
)
makedepends=(
    'cmake'
    'ninja'
    'git'
)
optdepends=(
    'playerctl: support media player controls in drmenu-media script'
)
provides=("${_pkgname}")
conflicts=("${_pkgname}")
source=("git+file://${PWD}#branch=main")
sha256sums=('SKIP')

pkgver() {
    cd "${srcdir}/${_pkgname}"
    printf "0.1.0.r%s.g%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
    cmake -B build -S "${srcdir}/${_pkgname}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -G Ninja
    cmake --build build
}

package() {
    DESTDIR="${pkgdir}" cmake --install build
}
