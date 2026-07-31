# Phonio

A minimalist, greyscale desktop music player for Linux with an offline music
library, dedicated Now Playing experience, and full metadata support.

Built with **C++20** and **Qt 6**.

## Features

- **Library** with instant search (title / artist / album / genre / composer) and sorting
- **Browse** by Artists, Albums, Genres, and Playlists
- **Queue** that persists between launches — drag & drop, reorder, save
- **Now Playing** page with large artwork, full metadata, and big seek bar
- **Lyrics** — synchronized LRC, auto-loaded beside the audio file or attached manually
- **Metadata editor** powered by TagLib (writes tags and embedded cover art directly)
- **Album art** resolution: embedded cover -> `folder.jpg` -> `cover.jpg` -> placeholder, cached on disk
- **Playback** — shuffle / repeat (Off / All / One), per-track position memory
- **History** — play counts, ratings, favorites, and last-played tracked in SQLite
- **Greyscale dark theme** throughout — no web technologies

## Usage

```
phonio                          Launch the player
phonio --screenshot <path>      Save a UI snapshot (developer)
phonio --page-nowplaying        Show Now Playing in the snapshot (developer)
```

## Build from source

### Prerequisites

| Requirement   | Minimum version | How to check            |
|---------------|-----------------|-------------------------|
| CMake         | 3.21            | `cmake --version`       |
| C++ compiler  | GCC 11+ / Clang 14+ | `g++ --version`    |
| Qt 6          | 6.5+            | `qmake6 --version`      |

### Install dependencies

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev \
    libqt6sql6-sqlite libtag1-dev

# Fedora
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtmultimedia-devel taglib-devel

# Arch Linux
sudo pacman -S base-devel cmake qt6-base qt6-multimedia taglib

# openSUSE
sudo zypper install gcc-c++ cmake qt6-base-devel qt6-multimedia-devel taglib-devel
```

### Build

```bash
git clone <repository-url>
cd Personal-Utilities/Phonio

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

> CMake detects Qt 6 automatically on most systems.
> If Qt 6 is installed in a custom path, add `-DCMAKE_PREFIX_PATH=/path/to/qt6`.

### Install (optional)

```bash
sudo cmake --install build
```

This copies the binary to `/usr/local/bin` and the desktop entry to
`/usr/local/share/applications/`.

### Run

```bash
./build/phonio
```

### Run tests (optional)

```bash
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Architecture

```
app/        App — dependency wiring (constructor injection, no globals)
core/       Domain types (Track, PlaylistInfo)
database/   SQLite schema + persistence
settings/   QSettings-backed preferences
metadata/   TagLib read/write of tags + embedded artwork
artwork/    Artwork resolution with layered fallback + disk cache
library/    Background scanner + in-memory library model
lyrics/     LRC parser, lyrics manager, synchronized lyrics widget
player/     AudioPlayer (QMediaPlayer wrapper) + PlaybackController
queue/      Persistent playback queue
playlists/  Playlist CRUD
ui/         MainWindow, pages, models, dialogs, theming
```
