#!/bin/bash
set -euo pipefail

MANIFEST="$(dirname "$0")/../flatpak/org.logexplorer.LogExplorer.json"
BUILD_DIR="build-flatpak"
REPO_DIR="repo-flatpak"

if ! command -v flatpak-builder >/dev/null 2>&1; then
  echo "flatpak-builder not installed. Install it and re-run this script. On Debian/Ubuntu: sudo apt install flatpak-builder"
  exit 1
fi

# Clean previous build
rm -rf "$BUILD_DIR" "$REPO_DIR"

echo "Building Flatpak in $BUILD_DIR using manifest $MANIFEST"
# Ensure the Flathub remote exists (try user remote first)
FLATHUB_URL="https://flathub.org/repo/flathub.flatpakrepo"
if ! flatpak remote-list --columns=name | grep -q "^flathub$"; then
  echo "Flathub remote not found — attempting to add as user remote..."
  if ! flatpak remote-add --if-not-exists --user flathub "$FLATHUB_URL" 2>/dev/null; then
    echo "Could not add Flathub as user remote. You may need to add it as a system remote (requires sudo):"
    echo "  sudo flatpak remote-add --if-not-exists flathub $FLATHUB_URL"
    echo "Or add it manually and re-run this script. Aborting."
    exit 1
  fi
fi

# Build the app (this will download runtimes and SDKs as needed)
flatpak-builder --user --force-clean --install-deps-from=flathub "$BUILD_DIR" "$MANIFEST"

# Optional: create a local repo containing the build
flatpak-builder --user --repo="$REPO_DIR" --force-clean "$BUILD_DIR" "$MANIFEST"

cat <<EOF
Build finished.
- To install locally for testing:
    flatpak --user remote-delete local-repo || true
    flatpak --user remote-add --no-gpg-verify local-repo "$REPO_DIR"
    flatpak --user install local-repo org.logexplorer.LogExplorer
- Or run directly without installing:
    flatpak-builder --run "$BUILD_DIR" "$MANIFEST" log-explorer

Note: You need Flatpak and flatpak-builder installed. If the build fails due to runtime version mismatches, change "runtime-version" in the manifest (flatpak/org.logexplorer.LogExplorer.json) to a compatible GNOME runtime available on flathub (e.g., "44", "43", or "41").
EOF
