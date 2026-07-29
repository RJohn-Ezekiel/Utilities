# Codex — Architecture & Design

## 1. Module Architecture

```
codex/
├── CMakeLists.txt          # Root build
├── DESIGN.md               # This file
├── src/
│   ├── CMakeLists.txt      # Library + executable build
│   ├── main_cli.cpp        # CLI entry point
│   ├── main_gui.cpp        # GUI entry point
│   ├── core/               # Types, config, utilities
│   │   ├── types.h         # Note, NoteId, VaultConfig, enums
│   │   ├── config.h/.cpp   # JSON config read/write
│   │   └── vault.h/.cpp    # VaultManager — CRUD for notes
│   ├── markdown/
│   │   ├── parser.h/.cpp   # Parse markdown → extracted metadata
│   │   └── wikilink.h/.cpp # Resolve [[Wiki Links]] to file paths
│   ├── index/
│   │   ├── search.h/.cpp   # Full-text search index
│   │   ├── backlinks.h/.cpp# Bidirectional backlink tracker
│   │   └── tags.h/.cpp     # Tag aggregator
│   ├── media/
│   │   └── media.h/.cpp    # Copy/organize media into vault
│   ├── export/
│   │   └── export.h/.cpp   # Export notes to HTML
│   ├── cli/
│   │   └── cli.h/.cpp      # Command-line dispatcher
│   └── ui/
│       ├── mainwindow.h/.cpp
│       ├── editor.h/.cpp   # QPlainTextEdit wrapper
│       ├── sidebar.h/.cpp
│       ├── backlinkspanel.h/.cpp
│       ├── tagspanel.h/.cpp
│       ├── fileinfopanel.h/.cpp
│       ├── searchbar.h/.cpp
│       ├── settingsdialog.h/.cpp
│       └── style.h         # Dark theme QSS
└── tests/
    └── CMakeLists.txt
```

## 2. Data Model

```cpp
struct Note {
    std::filesystem::path path;       // relative to vault root
    std::string title;                // derived from filename or first H1
    std::string content;              // full markdown source
    std::vector<std::string> tags;    // extracted #tags
    std::vector<std::string> wikiLinks; // [[linked]] note titles
    bool isJournal   = false;
    bool isTemplate  = false;
};
```

## 3. Vault Layout

```
CodexVault/
├── Notes/          # All user notes (flat or subfolder)
├── Media/
│   ├── Images/
│   ├── Videos/
│   ├── Audio/
│   └── Files/
├── Journal/        # Daily journal entries
├── Templates/      # Note templates
├── Exports/        # Exported HTML
├── Trash/          # Deleted notes (moved here)
├── Config/
│   ├── config.json
│   ├── search_index.json
│   ├── backlinks.json
│   └── tags.json
└── README.md
```

## 4. Key Classes

| Class | Module | Responsibility |
|-------|--------|----------------|
| `VaultManager` | core | Open/create vault, create/rename/delete/move notes |
| `Config` | core | Read/write JSON config |
| `Parser` | markdown | Extract headings, tags, wiki links from MD text |
| `WikiLinkResolver` | markdown | Resolve `[[Title]]` to file path |
| `SearchEngine` | index | Build/query/search index |
| `BacklinkManager` | index | Track which notes link to which |
| `TagIndexer` | index | Aggregate all tags across vault |
| `MediaManager` | media | Handle drag-drop media into vault |
| `ExportManager` | export | Export notes to HTML |
| `CLI` | cli | Parse args, delegate to core modules |
| `MainWindow` | ui | Assemble all panels |
| `Editor` | ui | QPlainTextEdit with autosave |
| `Sidebar` | ui | Tree of vault folders |
| `BacklinksPanel` | ui | Show incoming links |
| `TagsPanel` | ui | Tag cloud |
| `FileInfoPanel` | ui | Word/char count, cursor pos |
| `SearchBar` | ui | Inline search with instant results |
| `SettingsDialog` | ui | Vault path, font size, etc. |

## 5. GUI Wireframe

```
┌──────────────────────────────────────────────────┐
│  [New] [Open] [Save] [Search...] [Export] [Settings]          │  Toolbar
├────────┬─────────────────────────┬───────────────┤
│        │                         │  Backlinks     │
│  Notes  │  Markdown Editor       │  ────────────  │
│  Journal│  (QPlainTextEdit)       │  • Referenced  │
│  Temp.  │                         │    by Note A   │
│  Tags   │                         │    by Note B   │
│  Media  │                         │               │
│  Trash  │                         │  Tags          │
│        │                         │  ────────────  │
│        │                         │  #cpp #linux   │
│        │                         │               │
│        │                         │  File Info     │
│        │                         │  ────────────  │
│        │                         │  Words: 142    │
│        │                         │  Chars: 891    │
│        │                         │  Ln: 5  Col: 23│
├────────┴─────────────────────────┴───────────────┤
│  Notes/Networking.md  │  Words: 142  │  Auto-saved   │  Status
└──────────────────────────────────────────────────┘
```

## 6. Markdown Parsing Strategy

- Line-by-line scan (no full AST needed)
- Detect `[[Wiki Link]]` via regex
- Detect `#tag` at word boundaries (skip inside code blocks)
- Detect `---\n...\n---` frontmatter
- Preserve all original content verbatim

## 7. Wiki Link Resolution

- `[[Note Name]]` → strip brackets → find matching `.md` file:
  1. Exact match first
  2. Case-insensitive fallback
  3. Return first match
- If no match, mark as broken (red in UI)
- Click → open note or prompt to create

## 8. Backlink Strategy

- On save: re-parse note's wiki links
- For each target: add current note path to target's backlink list
- Remove stale backlinks (links that no longer exist in content)
- Persist in `Config/backlinks.json` as a map of `target → [sources]`

## 9. Search Strategy

- On vault open: async scan all `.md` files
- Index: `{ title, content, tags, wikiLinks }` per note
- Persist in `Config/search_index.json`
- Incremental update on save/rename/delete
- Query: case-insensitive substring match across all fields

## 10. Export Strategy

- Single note: render Markdown → HTML via custom parser
- Multiple notes: render each, link them via wiki link → HTML anchor
- Entire vault: export all notes, preserving folder structure
- Use a simple HTML template with embedded dark theme CSS

## 11. Development Roadmap

1. **Phase 1 — Core & Vault** — types, config, vault manager, CLI skeleton
2. **Phase 2 — Markdown** — parser, wiki link resolver
3. **Phase 3 — Index** — search, backlinks, tags
4. **Phase 4 — Media & Export** — media manager, export manager
5. **Phase 5 — CLI** — full CLI implementation
6. **Phase 6 — GUI** — main window, editor, sidebar, panels
7. **Phase 7 — Polish** — templates, journal, settings dialog, perf
8. **Phase 8 — Tests** — unit tests for core modules
