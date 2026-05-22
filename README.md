# QuickBar

A **Plasma 6+** panel widget that shows the active application’s global menu (the same DBus app-menu protocol as KDE’s built-in **Global Menu**), with far more appearance and behavior options.

## Features

- Full menubar or compact single-button mode
- Custom font family, size, weight, colors, button spacing, and hover corner radius
- Fill remaining panel width, scroll when overflowing, hide when empty
- Optional application name before the menu (toggle, margins, and font; shows **Plasma** on the desktop when Finder-style desktop menu is enabled)
- Finder-style desktop menu: show Dolphin’s menu bar on the desktop when Dolphin is open on your Desktop folder (toggleable; does not launch Dolphin automatically)
- Visibility presets: always show, active window only, or hide when empty
- Filter visibility by active window
- Configurable hover-to-open behavior

## Requirements

- Plasma 6.0+ (KF6 / Qt6)
- Build deps: `cmake`, `extra-cmake-modules`, `libplasma`, `plasma-workspace` (LibTaskManager), KF6 dev packages, Qt6 dev packages

On Arch/CachyOS:

```bash
sudo pacman -S libplasma plasma-workspace kconfig kcoreaddons ki18n kwindowsystem kirigami \
  extra-cmake-modules cmake gcc qt6-base qt6-declarative
```

`libdbusmenuqt` is vendored from plasma-workspace (no separate Arch package required).

## Distribution (AUR, apt, dnf)

Pre-built packaging for several package managers lives under [`packaging/`](packaging/):

| Platform | Package name | Install |
|----------|--------------|---------|
| Arch / CachyOS (AUR) | `plasma6-applets-quickbar` | `yay -S plasma6-applets-quickbar` |
| Debian / Ubuntu (Plasma 6) | `quickbar` | build `.deb` from `packaging/debian/` |
| Fedora / openSUSE | `quickbar` | build from `packaging/rpm/quickbar.spec` |

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

Add **QuickBar** from the widget gallery (category: Windows and Tasks).

## Important: replace stock Global Menu

QuickBar registers the same `org.kde.kappmenuview` DBus service as Plasma’s Global Menu. **Do not run both** — remove the stock **Global Menu** widget from your panel before adding QuickBar.

## Configuration

Right-click the widget → **Configure QuickBar…**

| Tab | Options |
|-----|---------|
| **General** | Menu bar, visibility, buttons, text (fonts, margins, colors) |

## Project layout

```
quickbar/
├── CMakeLists.txt      # plasma_add_applet build
├── metadata.json
├── main.xml            # KConfig schema
├── src/
│   ├── appmenumodel.*  # DBus menu model (from plasma-workspace)
│   └── quickbarapplet.*# Menu trigger + kappmenuview registration
└── qml/
    ├── main.qml
    ├── MenuDelegate.qml
    └── config*.qml
```

## Roadmap

- [ ] Per-app visibility rules
- [ ] Optional window title inline
- [ ] Latte Dock bridge
- [ ] User-defined prefix actions (macros)
- [ ] JSON theme import/export

## License

GPL-2.0-or-later (C++ core derived from [KDE plasma-workspace appmenu](https://invent.kde.org/plasma/plasma-workspace/-/tree/master/applets/appmenu)).
