# Logos

![screenshot](demo.png)

A cross-platform desktop Bible reader with offline translations, side-by-side comparison, full-text search, bookmarks, notes, and history.

Built with **C++20** and **Qt 6**.

## Features

- **Read** any chapter or verse — type a reference (e.g. `John 3:16`) and press Enter
- **Search** full text across the active translation
- **Compare** two translations side by side with synced scrolling
- **Bookmark** verses and browse recent history
- **Notes** per passage — save, edit, and delete
- **Random** and **daily verse** at one click
- **Tabbed** navigation for multiple passages
- **Greyscale dark theme** throughout

## Usage

```
logos                          Launch GUI
logos <reference>              Display passage
logos --search <query>         Full-text search
logos --random                 Random verse
logos --today                  Daily verse
logos --help                   Display help
logos --version                Show version
```

## Build from source

### Prerequisites

| Requirement   | Minimum version | How to check            |
|---------------|-----------------|-------------------------|
| CMake         | 3.20            | `cmake --version`       |
| C++ compiler  | GCC 11+ / Clang 14+ | `g++ --version`    |
| Qt 6          | 6.5+            | `qmake6 --version`      |

### Install dependencies

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev

# Fedora
sudo dnf install gcc-c++ cmake qt6-qtbase-devel

# Arch Linux
sudo pacman -S base-devel cmake qt6-base

# openSUSE
sudo zypper install gcc-c++ cmake qt6-base-devel
```

### Build

```bash
git clone https://github.com/RJohn-Ezekiel/Utilities.git
cd Utilities/Logos

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

> CMake detects Qt 6 automatically on most systems.
> If Qt 6 is installed in a custom path, add `-DCMAKE_PREFIX_PATH=/path/to/qt6`.

### Run

```bash
./build/logos
```

### Run tests (optional)

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
./build/tests/test_bible_explorer
```

## Project Structure

```
src/
├── main.cpp
├── cli/           — CLI argument parsing
├── data/          — Translation loading (JSON bibles)
├── services/      — Reference parser, search, verse service
├── storage/       — Bookmarks, history, notes persistence
├── core/          — Domain types
└── ui/            — MainWindow, reader pane, sidebar, notes, settings
```

## Install

### Quick (via install script)

```bash
cd Utilities/Logos
./install.sh
```

Installs the binary to `~/.local/bin/logos`. Pass a directory to change the location (default `~/.local/bin`).

### Manual

1. Build (see [Build from source](#build-from-source))
2. Copy `build/logos` to `~/.local/bin/logos.bin`
3. Create `~/.local/bin/logos` wrapper:
   ```bash
   cat > ~/.local/bin/logos << 'EOF'
   #!/bin/bash
   export QT_LOGGING_RULES="kf.*.warning=false"
   export QT_QPA_PLATFORMTHEME=""
   exec "$0.bin" "$@"
   EOF
   chmod +x ~/.local/bin/logos
   ```
4. Ensure `~/.local/bin` is in your `PATH` (add `export PATH="$HOME/.local/bin:$PATH"` to `~/.bashrc` or `~/.zshrc`)

### Uninstall

```bash
rm -f ~/.local/bin/logos ~/.local/bin/logos.bin
```

## License

GNU General Public License v3.0.
