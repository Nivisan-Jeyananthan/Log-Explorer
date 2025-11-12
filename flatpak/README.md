Flatpak build instructions for Log-Explorer

Overview

This folder contains a Flatpak manifest and a helper script to build a Flatpak bundle of Log-Explorer. Building the Flatpak isolates your app from the host's GLib/GTK versions so users with older distributions won't get symbol lookup errors (like missing `g_once_init_enter_pointer`).

Files

- org.logexplorer.LogExplorer.json — Flatpak manifest (runtime: org.gnome.Platform 44). Adjust `runtime-version` if you need a different GNOME runtime.
- ../tools/build_flatpak.sh — Helper script that calls `flatpak-builder` and creates a local repo for testing.

Prerequisites

Install Flatpak and flatpak-builder on your build machine. On Debian/Ubuntu:

```bash
sudo apt update
sudo apt install flatpak flatpak-builder
```

You may also want to add the Flathub remote if it's not present:

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

Build steps

Run the helper script from the workspace root:

```bash
./tools/build_flatpak.sh
```

This will:
- Run `flatpak-builder` to build the app in `build-flatpak` (downloads SDK/runtime as necessary).
- Create a local Flatpak repo in `repo-flatpak`.

Testing the build

- Run without installing:

```bash
flatpak-builder --run build-flatpak flatpak/org.logexplorer.LogExplorer.json log-explorer
```

- Install locally for testing:

```bash
flatpak --user remote-add --no-gpg-verify local-repo repo-flatpak
flatpak --user install local-repo org.logexplorer.LogExplorer
flatpak --user run org.logexplorer.LogExplorer
```

Notes

- If you see build failures related to runtime versions, change `"runtime-version"` in the manifest to one available on Flathub (e.g. "44", "43").
- Flatpak is the recommended distribution method for GTK apps that must run across many Linux distributions without the host providing newer GLib/GTK versions.

Licensing

Bundling GLib/GTK in Flatpak runtime does not change upstream licensing; respect the licenses of libraries. Flatpak runtime components usually come from Flathub or GNOME's runtime repos.
