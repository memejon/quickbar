# quickbar

A Plasma 6+ panel widget that shows the active application's global menu (the same DBus app-menu protocol as KDE's built-in Global Menu), with far more appearance and behavior options.

## Requirements

- Plasma 6.0+ (KF6 / Qt6)
- `./install.sh` installs missing build dependencies for your distro

`libdbusmenuqt` is vendored from plasma-workspace (no separate package required).

## Distribution (AUR, apt, dnf)

Pre-built packaging for several package managers lives under [`packaging/`](packaging/):

| Platform | Package name | Install |
|----------|--------------|---------|
| Arch / CachyOS (AUR) | `plasma6-applets-quickbar` | `yay -S plasma6-applets-quickbar` |
| Debian / Ubuntu (Plasma 6) | `quickbar` | Build `.deb` from `packaging/debian/` |
| Fedora / openSUSE | `quickbar` | Build from `packaging/rpm/quickbar.spec` |

See [`packaging/README.md`](packaging/README.md) for publishing to the AUR, building `.deb` / `.rpm` packages, and OBS. AUR listing text: [`packaging/arch/AUR.md`](packaging/arch/AUR.md). CI validates packaging on pull requests and release tags.

## Build & install

From a git checkout, `./install.sh` detects your distro (Arch, Debian/Ubuntu, Fedora, openSUSE) and installs any missing build dependencies before compiling:

```bash
./install.sh
```

Manual build:

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

Then restart Plasma or run:

```bash
kquitapp6 plasmashell && plasmashell &
```

Add quickbar from the widget gallery (category: Windows and Tasks).
