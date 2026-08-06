# Utilities

A collection of cross-platform **C++20 / Qt 6** desktop applications under one
repository. Each app is self-contained, greyscale-dark themed, and local-first.

| App      | Purpose                                      |
|----------|----------------------------------------------|
| **Arete**    | Personal OS: Home, Chronos, Modules, Settings + an app store of optional modules |
| **Logos**    | Offline Bible reader with search, compare, notes |
| **Chronos**  | Focus-timer engine with sessions and statistics |
| **Codex**    | Markdown vault editor with a WYSIWYG split view |
| **Phonio**   | Music player and lyrics |
| **Visio**    | Video / image tooling |

Each app lives in its own directory with its own `README.md`, `CMakeLists.txt`
and `install.sh`. See *Arete/README.md* for the module system layout shared by
the newer apps.

## Shared build

A common `AreteCore` library (settings, logging, notifications, self-update)
lives in `AreteCore/`. Configure from the repository root so all targets share
a build tree:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The ready-to-run binaries are produced under `build/bin/`.

## Conventions

- C++20, Qt 6.5+
- Greyscale dark palette throughout
- Data stays local; no account or telemetry