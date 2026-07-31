# Chronos

![screenshot](demo.png)

A cross-platform desktop focus timer with Pomodoro sessions, task management, health reminders, session statistics, and a compact mini-mode.

Built with **C++20** and **Qt 6**.

## Features

- **Focus Timer** — Pomodoro-style sessions with configurable focus and break durations
- **Task Management** — create tasks and start focus sessions tied to them
- **Health Reminders** — periodic reminders to drink water, stand, stretch, and rest your eyes
- **Statistics & History** — total focus time, sessions completed, daily streaks
- **Mini Mode** — compact always-on-top window for minimal distraction
- **Notifications** — toast notifications and sound alerts at session boundaries
- **Water Meter** — visual hydration tracker
- **Greyscale dark theme** throughout

## Usage

```
chronos                         Launch GUI
chronos --stats                 Display today's statistics and exit
chronos --help                  Display help
chronos --version               Show version
```

### Keyboard shortcuts

| Key       | Action              |
|-----------|---------------------|
| `Space`   | Start / Pause       |
| `Ctrl+S`  | Stop                |
| `Ctrl+K`  | Skip break          |
| `Ctrl+M`  | Toggle mini mode    |
| `Ctrl+Q`  | Quit                |

## Build from source

### Prerequisites

| Requirement   | Minimum version | How to check            |
|---------------|-----------------|-------------------------|
| CMake         | 3.22            | `cmake --version`       |
| C++ compiler  | GCC 11+ / Clang 14+ | `g++ --version`    |
| Qt 6          | 6.5+            | `qmake6 --version`      |

### Install dependencies

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev

# Fedora
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtmultimedia-devel

# Arch Linux
sudo pacman -S base-devel cmake qt6-base qt6-multimedia

# openSUSE
sudo zypper install gcc-c++ cmake qt6-base-devel qt6-multimedia-devel
```

### Build

```bash
git clone https://github.com/RJohn-Ezekiel/Utilities.git
cd Utilities/Chronos

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

> CMake detects Qt 6 automatically on most systems.
> If Qt 6 is installed in a custom path, add `-DCMAKE_PREFIX_PATH=/path/to/qt6`.

### Run

```bash
./build/src/chronos
```

### Run tests (optional)

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
./build/tests/test_timer_engine
```

## Project Structure

```
src/
├── main.cpp
├── core/          — Timer engine logic
├── models/        — Data models (Settings, Task, Session, Statistics)
├── storage/       — JSON file persistence
├── services/      — Timer, audio, notification, reminder services
├── ui/            — Qt widgets (dashboard, timer, settings, etc.)
└── cli/           — CLI argument parsing
```

## Install

### Quick (via install script)

```bash
cd Utilities/Chronos
./install.sh
```

Installs the binary to `~/.local/bin/chronos`. Pass a directory to change the location (default `~/.local/bin`).

### Manual

1. Build (see [Build from source](#build-from-source))
2. Copy `build/src/chronos` to `~/.local/bin/chronos.bin`
3. Create `~/.local/bin/chronos` wrapper:
   ```bash
   cat > ~/.local/bin/chronos << 'EOF'
   #!/bin/bash
   export QT_LOGGING_RULES="kf.*.warning=false"
   export QT_QPA_PLATFORMTHEME=""
   exec "$0.bin" "$@"
   EOF
   chmod +x ~/.local/bin/chronos
   ```
4. Ensure `~/.local/bin` is in your `PATH` (add `export PATH="$HOME/.local/bin:$PATH"` to `~/.bashrc` or `~/.zshrc`)

### Uninstall

```bash
rm -f ~/.local/bin/chronos ~/.local/bin/chronos.bin
```

## License

MIT — see [`LICENSE`](LICENSE).
