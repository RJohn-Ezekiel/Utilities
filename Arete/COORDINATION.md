# Coordination — shared note for agents working in /home/magesh/Utilities

Last updated: 2026-08-03

## Who am I
Agent A — building the **Arete** personal-OS app in `/home/magesh/Utilities/Arete/`.

## What I am doing (Agent A)
- Building the Arete shell: Home / Today / Projects / Kanban / Calendar /
  Chronos / Codex / Logos / Phonio / Journal / Statistics / Settings tabs,
  top bar, command palette, universal search, quick capture.
- **Vendored snapshots** of the sibling apps' engines into
  `Arete/src/vendor/{chronos,logos,codex,phonio}/` (static libraries,
  embedded via `takeCentralWidget()`).
- Arete's own data layer is SQLite (`src/data/`): tasks, projects, events,
  journal. Focus sessions come from the vendored Chronos engine.

## My boundaries
- I only write inside `/home/magesh/Utilities/Arete/`.
- I do not commit; nothing is staged by me.

## Requests to other agents (please)
1. If you edit Chronos/Logos/Codex/Phonio sources, my vendored copies in
   `Arete/src/vendor/*` do NOT auto-sync. That is fine, but please do not
   rename/delete files under `Arete/src/vendor/*` — I may refresh them at the
   end.
2. Do not run `git add`/`git commit` while I am mid-work, and do not `git
   checkout` / revert files, to avoid clobbering each other.
3. I depend on these vendored APIs (changing them in the *siblings* is fine,
   but tell me if you also touch `Arete/src/vendor/*`):
   - chronos: `TimerService`, `TaskService`, `StatisticsService`,
     `StorageManager`, `SidebarWidget`, `MainWindow(...)` ctor signature.
   - logos: `MainWindow`, `VerseService::dailyVerse`, `SearchService`,
     `Loader::loadAll`, `Theme::apply` (I removed the qApp-stylesheet calls in
     the vendored copies).
   - codex: `VaultManager`, `Editor`, `MainWindow`.
   - phonio: `App`, `PlaybackController`, `QueueManager`, `MainWindow`.

## State as of now
- Arete scaffold + data layer + services compile target set up; UI modules
  are being written. Build dir: `/home/magesh/Utilities/Arete/build`.
