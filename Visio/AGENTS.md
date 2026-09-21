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

| Token         | Hex       | Purpose                     |
|---------------|-----------|-----------------------------|
| background    | `#111111` | Main window / primary bg    |
| panel         | `#242424` | Panel / sidebar / card      |
| readingArea   | `#181818` | Reading pane / video list   |
| toolbar       | `#242424` | Toolbar, status bar         |
| input         | `#333333` | Input fields (search bar)   |
| hover         | `#2E2E2E` | Hover state                 |
| selection     | `#3A3A3A` | Selected item highlight     |
| border        | `#353535` | Borders, separators         |
| primaryText   | `#C4C4C4` | Primary / heading text      |
| secondaryText | `#A0A0A0` | Secondary / muted text      |
| accent        | `#D0D0D0` | Accent — grey               |
| accentDim     | `#B0B0B0` | Accent dimmed (pressed)     |
| success       | `#6AA06A` | Success / positive          |
| warning       | `#C4A050` | Warning / caution           |
| error         | `#C45050` | Error / destructive         |
| errorBg       | `#3A1A1A` | Error background            |

Defined in `include/visio/ui/Theme.h` as `constexpr QColor`. This is the unified Arete spec palette, also used by Logos, Chronos, and Codex.

## Public API

```cpp
visio::Client yt;
auto results = yt.search("query");
auto meta = yt.getVideo("VIDEO_ID");   // yt-dlp --dump-json
auto dl = yt.download(video, dir, Quality::Best);
auto dlMp3 = yt.downloadAudio(video, dir);
auto upd = yt.updateYtDlp();           // runs `yt-dlp -U`
```

GUI note: list thumbnails load asynchronously via `MainWindow::loadThumbnail` (per-URL cache in `m_thumbnailCache`; items carry the URL in `Qt::UserRole + 1`). "Failed to fetch"-style download errors usually mean a stale `yt-dlp` — the toolbar has an **Update yt-dlp** button and download failures hint at it.

## Code Conventions

- C++20, camelCase methods, PascalCase types
- `[[nodiscard]]`, `noexcept`, `constexpr` where correct
- `Result<T>` for fallible operations (never raw error codes)
- PIMPL for `Client`, composition over inheritance
- No raw owning pointers
