# quickbar

A Plasma 6+ panel widget that shows the active application's global menu (the same DBus app-menu protocol as KDE's built-in Global Menu), with far more appearance and behavior options.

## Requirements

- Plasma 6.0+ (KF6 / Qt6)
- Build deps: `cmake`, `extra-cmake-modules`, `gcc`, `libplasma`, `plasma-workspace` (LibTaskManager), `qt6-base`, `qt6-declarative`, `kconfig`, `kcoreaddons`, `ki18n`, `kwindowsystem`, `kirigami`

`libdbusmenuqt` is vendored from plasma-workspace (no separate package required).

## Distribution (AUR, apt, dnf)

Currently there is an AUR for quickbar that goes under the name `plasma6-applets-quickbar`, for other distributions, you are going to have to run the install script shown below. I plan on having binaries for most distros in the coming days.

```bash
yay -S plasma6-applets-quickbar
``` 


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
