# QuickBar

A **Plasma 6+** panel widget that shows the active application’s global menu (the same DBus app-menu protocol as KDE’s built-in **Global Menu**), with far more appearance and behavior options.

## Features

- Full menubar or compact single-button mode
- Layout styles: native Plasma SVG, flat, or pill buttons
- Custom font family, size, weight, colors, and item spacing
- Fill remaining panel width, scroll when overflowing, hide when empty
- Optional application name before the menu (separate font)
- macOS-style sticky menubar (keep bar when app is unfocused; on desktop shows **Plasma** with Dolphin’s menu for `~/Desktop`)
- Filter visibility by active window
- Configurable hover-to-open behavior

## Requirements

- Plasma 6.0+ (KF6 / Qt6)
- Build deps: `cmake`, `extra-cmake-modules`, `plasma-framework` / `libplasma`, `libplasma` (LibTaskManager), Qt6 dev packages

On Arch/CachyOS:

```bash
sudo pacman -S plasma-framework libplasma kconfig kcoreaddons extra-cmake-modules cmake gcc qt6-base qt6-declarative
```

`libdbusmenuqt` is vendored from plasma-workspace (no separate Arch package required).

## Build & install

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
| **General** | Compact vs full bar, all screens, fill width, hide when empty, application name and margins |
| **Appearance** | Style, native SVG, spacing, menu & app name fonts, colors, max visible items |
| **Behavior** | Sticky menubar (macOS style), active-window filter, hover opens menu |

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
