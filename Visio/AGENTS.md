# Visio

## Build

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

## Run Tests

```bash
./build/visio_tests
```

## Color Palette

Shared across Chronos, Codex, Logos, and Visio projects.

| Token       | Hex       | Purpose                     |
|-------------|-----------|-----------------------------|
| background  | `#1B1B1B` | Main window / primary bg    |
| panel       | `#232323` | Panel / sidebar / card      |
| readingArea | `#202020` | Reading pane / video list   |
| toolbar     | `#252526` | Toolbar, status bar         |
| input       | `#333333` | Input fields (search bar)   |
| hover       | `#2E2E2E` | Hover state                 |
| selection   | `#3A3D41` | Selected item highlight     |
| border      | `#353535` | Borders, separators         |
| primaryText | `#D8D8D8` | Primary / heading text      |
| secondary   | `#A9A9A9` | Secondary / muted text      |
| accent      | `#7A8A9A` | Muted blue-grey accent      |
| accentDim   | `#5A6672` | Accent dimmed (pressed)     |
| success     | `#5A8A5A` | Success / positive          |
| warning     | `#B8A060` | Warning / caution           |
| error       | `#8A4A4A` | Error / destructive         |
| errorBg     | `#402020` | Error background            |

Defined in `include/visio/ui/Theme.h` as `constexpr QColor`.

## Public API

```cpp
visio::Client yt;
auto results = yt.search("query");
```

## Code Conventions

- C++20, camelCase methods, PascalCase types
- `[[nodiscard]]`, `noexcept`, `constexpr` where correct
- `Result<T>` for fallible operations (never raw error codes)
- PIMPL for `Client`, composition over inheritance
- No raw owning pointers
