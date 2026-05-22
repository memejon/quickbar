#!/usr/bin/env bash
# Finish publishing quickbar 0.2.1 to GitHub and the AUR.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
AUR="${ROOT}/plasma6-applets-quickbar"

echo "==> Pushing GitHub (main + tag v0.2.1)..."
cd "$ROOT"
git push origin main
git push origin v0.2.1

echo "==> Updating AUR checksums and .SRCINFO..."
cd "$AUR"
cp "$ROOT/packaging/arch/PKGBUILD" PKGBUILD
updpkgsums
makepkg --printsrcinfo > .SRCINFO

echo "==> Verifying AUR package build..."
makepkg -f --noconfirm

git add PKGBUILD .SRCINFO
if git diff --cached --quiet; then
    echo "AUR: no changes to commit."
else
    git commit -m "Update to 0.2.1"
fi
git push origin master

echo "Done. Tag: https://github.com/kevinbudz/quickbar/releases/tag/v0.2.1"
