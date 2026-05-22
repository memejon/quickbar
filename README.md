# quickbar

a plasma 6+ panel widget that shows the active application's global menu (the same dbus app-menu protocol as kde's built-in global menu), with far more appearance and behavior options.

## requirements

- plasma 6.0+ (kf6 / qt6)
- build deps: `cmake`, `extra-cmake-modules`, `libplasma`, `plasma-workspace` (libtaskmanager), kf6 dev packages, qt6 dev packages

on arch/cachyos:

```bash
sudo pacman -S libplasma plasma-workspace kconfig kcoreaddons ki18n kwindowsystem kirigami \
  extra-cmake-modules cmake gcc qt6-base qt6-declarative
```

`libdbusmenuqt` is vendored from plasma-workspace (no separate arch package required).

## distribution (aur, apt, dnf)

pre-built packaging for several package managers lives under [`packaging/`](packaging/):

| platform | package name | install |
|----------|--------------|---------|
| arch / cachyos (aur) | `plasma6-applets-quickbar` | `yay -S plasma6-applets-quickbar` |
| debian / ubuntu (plasma 6) | `quickbar` | build `.deb` from `packaging/debian/` |
| fedora / opensuse | `quickbar` | build from `packaging/rpm/quickbar.spec` |

see [`packaging/README.md`](packaging/README.md) for publishing to the aur, building `.deb` / `.rpm` packages, and obs. aur listing text: [`packaging/arch/AUR.md`](packaging/arch/AUR.md). ci validates packaging on pull requests and release tags.

## build & install

from a git checkout, `./install.sh` detects your distro (arch, debian/ubuntu, fedora, opensuse) and installs any missing build dependencies before compiling:

```bash
./install.sh
```

manual build:

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

then restart plasma or run:

```bash
kquitapp6 plasmashell && plasmashell &
```

add quickbar from the widget gallery (category: windows and tasks).

## important: replace stock global menu

quickbar registers the same `org.kde.kappmenuview` dbus service as plasma's global menu. do not run both — remove the stock global menu widget from your panel before adding quickbar.

## configuration

right-click the widget → configure quickbar…

| tab | options |
|-----|---------|
| general | menu bar, visibility, buttons, text (fonts, margins, colors) |
