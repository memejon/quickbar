# AUR: `plasma6-applets-quickbar`

The AUR package name follows other Plasma 6 applets (e.g. `plasma6-applets-appgrid`) so users can find it easily.

Copy-paste text for the AUR package page (long description, keywords): [`AUR.md`](AUR.md).

## Updating a release

1. Bump `pkgver` / `pkgrel` in `PKGBUILD` to match the new git tag (`vX.Y.Z`).
2. Run `updpkgsums` in the AUR git checkout.
3. `makepkg -si` to verify.
4. Regenerate `.SRCINFO`: `makepkg --printsrcinfo > .SRCINFO`
5. Commit and push to `aur@aur.archlinux.org:plasma6-applets-quickbar.git`.

## Runtime dependencies

The applet links against Plasma, LibTaskManager (`plasma-workspace`), and KF6/Qt6 libraries listed in `depends=`. `libdbusmenuqt` is vendored in the source tree and does not need a separate Arch package.
