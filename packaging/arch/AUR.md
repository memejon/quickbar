# AUR package text (`plasma6-applets-quickbar`)

Copy the sections below when creating or updating the AUR package. The short line is already in `PKGBUILD` as `pkgdesc`.

## Short description (`pkgdesc` in PKGBUILD)

```text
Configurable Plasma 6 global menu panel widget (QuickBar)
```

## Long description (first AUR package comment / wiki-style README)

```markdown
QuickBar is a Plasma 6+ panel widget that shows the active application's global menu
(DBus app-menu protocol), with more appearance and behavior options than KDE's stock
**Global Menu** widget.

**Features**
- Full menubar or compact single-button mode
- Custom fonts, colors, spacing, and hover styling
- Fill panel width, scroll when overflowing, hide when empty
- Optional application name before the menu
- Finder-style desktop menu (Dolphin on the desktop)
- Visibility presets and hover-to-open behavior

**Requirements**
- Plasma 6.0+ (KF6 / Qt6)
- `libplasma`, `plasma-workspace`, KF6, Qt6 (see PKGBUILD `depends`)

**Important**
QuickBar registers the same `org.kde.kappmenuview` DBus service as Plasma's Global Menu.
**Remove the stock Global Menu widget** from your panel before adding QuickBar.

**After install:** restart Plasma (`kquitapp6 plasmashell && plasmashell &`), then add **QuickBar** from the widget gallery (category: Windows and Tasks).

**Upstream:** https://github.com/kevinbudz/quickbar
**Issues:** https://github.com/kevinbudz/quickbar/issues
**License:** GPL-2.0-or-later
```

## Optional `optdepends` (add to PKGBUILD if desired)

```bash
optdepends=(
    'plasma-meta: full Plasma desktop'
    'dolphin: Finder-style desktop menu shows Dolphin's menu bar'
)
```

## Search keywords (for discoverability)

Users may search: global menu, appmenu, menubar, plasma 6, panel widget, quickbar
