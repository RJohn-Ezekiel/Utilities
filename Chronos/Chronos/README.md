# Chronos

A desktop focus timer app built with C++20 and Qt6. Features Pomodoro-style timers, task management, health reminders, and session statistics.

## Features

- **Focus Timer** – Pomodoro sessions with configurable focus/break durations
- **Task Management** – Create tasks and start focus sessions for them
- **Health Reminders** – Periodic reminders to drink water, stand, stretch, and rest your eyes
- **Statistics & History** – Track focus time, sessions completed, and streaks
- **Mini Mode** – Compact always-on-top window for minimal distraction
- **Notifications** – Toast notifications and sound alerts
- **Dark Theme** – Clean dark UI throughout

## Building

### Dependencies

- CMake 3.22+
- Qt6 (Core, Gui, Widgets, Multimedia)
- C++20 compiler

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Install (optional – makes `chronos` available from anywhere)

```bash
sudo cmake --install build
```

Or create a symlink manually:

```bash
ln -sf "$(pwd)/build/chronos" ~/.local/bin/chronos
```

## Usage

Run the app:

```bash
./build/chronos
```

Keyboard shortcuts:

| Key       | Action              |
|-----------|---------------------|
| `Space`   | Start / Pause       |
| `Ctrl+S`  | Stop                |
| `Ctrl+K`  | Skip break          |
| `Ctrl+M`  | Toggle mini mode    |
| `Ctrl+Q`  | Quit                |

## Project Structure

```
src/
├── main.cpp
├── core/          – Timer engine logic
├── models/        – Data models (Settings, Task, Session, Statistics)
├── storage/       – JSON file persistence
├── services/      – Timer, audio, notification, reminder services
├── ui/            – Qt widgets (dashboard, timer, settings, etc.)
└── cli/           – CLI argument parsing
```

## License

MIT