# drmenu

A radial/pie menu launcher for Wayland compositors built with Qt6 and LayerShellQt.

---

## Requirements & Dependencies

### Arch Linux Dependencies
- **Build time:** `cmake`, `ninja`, `gcc` (or `clang`), `git`
- **Runtime:** `qt6-base`, `qt6-declarative`, `qt6-svg`, `layer-shell-qt`
- **Optional:** `playerctl` (for `drmenu-media` script controls)

---

## Installation & Building on Arch Linux

### Option 1: Using `yay` or `makepkg` (Recommended)

You can build and install `drmenu` as a native Arch package using `yay` or `makepkg`:

```bash
# Clone the repository
git clone https://github.com/heisenshark/drmenu.git
cd drmenu

# Install with yay
yay -Bi .

# Or install using makepkg
makepkg -si
```

---

### Option 2: Using `./build.sh` Helper Script

The repository includes a `./build.sh` script for convenience:

* **Build binary locally:**
  ```bash
  ./build.sh build
  ```
  *(Output executable will be in `build/bin/drmenu`)*

* **Build & install Arch package via `makepkg`:**
  ```bash
  ./build.sh pkg
  ```

* **Build & install system-wide to `/usr/local/bin`:**
  ```bash
  ./build.sh install
  ```

* **Clean build artifacts:**
  ```bash
  ./build.sh clean
  ```

---

### Option 3: Manual CMake Build

```bash
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
ninja
sudo ninja install
```

---

## Nix / NixOS

If you use Nix:
```bash
nix build
# or run directly with flake
nix run
```

---

## License

This project is licensed under the [MIT License](LICENSE).
