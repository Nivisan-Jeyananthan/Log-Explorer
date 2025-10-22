# Intro

Unified Log Explorer (GTK4 + SQLite)

Purpose

A small prototype that indexes systemd journal and /var/log files into a SQLite database with FTS5 for fast full-text search. Provides a GTK4 UI for searching, tagging, and quick alerts.

Scope

This repository is an initial prototype focusing on:
- Project scaffold (Meson)
- SQLite DB with FTS5 schema
- Simple indexer that can read from `journalctl` (requires systemd) and plain text files under /var/log
- GTK4 UI: search entry, list of results, details pane, tag/annotate support

Requirements

- Linux with GTK4 development packages
- Meson, Ninja
- gcc/clang
- sqlite3 with FTS5 enabled
- libgtk-4-dev, libgranite-dev (optional)

# How to build

Install dependencies (Debian/Ubuntu example):   
```
$ sudo apt install build-essential meson ninja-build libgtk-4-dev libglib2.0-dev libsqlite3-dev libsystemd-dev
$ meson setup build  
$ meson compile -C build
```

# How to run
For reading journalctl you may need root or appropriate capabilities.   
```./build/log-explorer```  

# Notes
This is a minimal prototype: production-quality indexing, permission handling, and alerting are left as TODOs.