# Visio

![screenshot](demo.png)

A cross-platform desktop YouTube browser — search, stream, and download videos with a full dark-theme Qt 6 GUI. Based on the same workflow as the [ytsurf](https://github.com/Stan-breaks/ytsurf) TUI.

Built with **C++20** and **Qt 6**.

## Features

- **Search** — search YouTube, click any result to see title, author, duration, views, and thumbnail
- **Play** — stream videos in mpv with quality selection (Best, 720p, 1080p, 2160p, Audio Only)
- **Download** — save videos or audio to your chosen directory
- **History** — automatic watch history with remove and clear
- **Queue** — build a playback queue, save it as a named playlist
- **Playlists** — save, load, and delete named playlists
- **Subscriptions** — subscribe to channels by search, view your feed
- **Dark theme** — full Qt dark palette shared with Chronos, Codex, and Logos

## Usage

```
visio_gui                        Launch GUI
```

Type a query in the search bar and press Enter. Click a result to see its details, then **Play**, **Download**, or **Add to Queue**. Use the tabs to browse **History**, **Queue**, **Subs**, and **Playlists**.

### Shortcuts / Tips

- **Search channels**: Open the **Subs** tab, click **Search Channels**, enter a name, pick from the list
- **Quality**: Select a quality preset from the dropdown before playing or downloading
- **Playlist**: Save your queue as a playlist, then open it later from the **Playlists** tab

## Build from source

### Prerequisites

| Requirement   | Minimum version | How to check            |
|---------------|-----------------|-------------------------|
| CMake         | 3.20            | `cmake --version`       |
| C++ compiler  | GCC 11+ / Clang 14+ | `g++ --version`    |
| Qt 6          | 6.5+            | `qmake6 --version`      |

Runtime tools (searched automatically, warned if missing):

| Tool     | Required for           |
|----------|------------------------|
| `curl`   | YouTube HTTP requests  |
| `yt-dlp` | Video metadata, downloading |
| `mpv`    | Video playback         |

### Install dependencies

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev curl yt-dlp mpv

# Fedora
sudo dnf install gcc-c++ cmake qt6-qtbase-devel curl yt-dlp mpv

# Arch Linux
sudo pacman -S base-devel cmake qt6-base curl yt-dlp mpv
```

### Build

```bash
git clone <repository-url>
cd Utilities/Visio

cmake -S . -B build
cmake --build build
```

> CMake detects Qt 6 automatically on most systems.  
> If Qt 6 is installed in a custom path, add `-DCMAKE_PREFIX_PATH=/path/to/qt6`.

### Run

```bash
./build/visio_gui
```

### Run tests (optional)

```bash
./build/visio_tests
```

## Project Structure

```
include/visio/
├── client.hpp       — Main Client class (PIMPL)
├── error.hpp        — Result<T>, Error, VisioException
├── types.hpp        — Video, Channel, Playlist structs
├── search.hpp       — Search URL builders and parsers
├── video.hpp        — VideoUtils (ID extraction, metadata)
├── channel.hpp      — ChannelUtils (videos, parsing)
├── playlist.hpp     — PlaylistUtils (save, load, list)
└── ui/
    └── Theme.h      — Qt dark palette (constexpr QColor) + full stylesheet

src/
├── client.cpp       — Full implementation of Client
├── search.cpp, video.cpp, channel.cpp, playlist.cpp
├── http_util.hpp    — Internal subprocess + JSON extraction
└── gui/
    ├── main.cpp          — QApplication entry point
    ├── MainWindow.hpp    — Main window header
    └── MainWindow.cpp    — Full GUI implementation

tests/
├── test_client.cpp
├── test_search.cpp
└── test_types.cpp

examples/
├── search_example.cpp
├── video_info_example.cpp
└── queue_playlist_example.cpp
```

## Install

### Quick (via install script)

```bash
git clone https://github.com/RJohn-Ezekiel/Utilities.git
cd Utilities/Visio
chmod +x install.sh
./install.sh
```

### Manual

1. Build (see [Build from source](#build-from-source))
2. Copy `build/visio_gui` to `~/.local/bin/visio`

### Uninstall

```bash
rm -f ~/.local/bin/visio ~/.local/bin/visio_gui
```

## CLI (Visio shell client)

A bash-based terminal client is included as `visio.sh` for use without Qt:

```bash
./visio.sh "lofi beats"
./visio.sh --help
```

## License

GNU General Public License v3.0 — see [`LICENSE`](LICENSE).
