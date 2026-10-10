# KONATA

KONATA is a **desktop authoring tool for fictional chat and story content**. It is a Qt Widgets application where you write characters, conversations, and scenes as structured data, preview them through a pixel-faithful chat renderer (first skin: Apple Messages), and export the result as PNG images at exact canvas sizes.

> **Status:** early development — the CMake project is version `0.1.0` and the application reports `0.1A`. Core authoring, rendering, `.konata` project persistence, and PNG export are implemented and covered by checks; the timeline panel is a placeholder and several renderer details are not yet built (see [Known limitations](#known-limitations)).

## Contents

- [Features](#features)
- [Architecture](#architecture)
- [Project structure](#project-structure)
- [The `.konata` project format](#the-konata-project-format)
- [Technology stack](#technology-stack)
- [Prerequisites](#prerequisites)
- [Build and run](#build-and-run)
- [Testing](#testing)
- [Known limitations](#known-limitations)
- [Roadmap (planned)](#roadmap-planned)
- [License](#license)

## Features

- **Structured story model** — characters (name, username, avatar reference, presence), conversations (private/group, participants, ordered messages with reactions and attachments), and scenes (ordered events: send-message, wait, start/stop typing, notification). All UI-independent in `KonataCore`.
- **Authoring UI** — a three-pane main window (project tree, live preview, inspector) plus five dedicated field editors for characters, conversations, messages, scenes, and scene events.
- **Directly editable preview** — the central preview shows the selected conversation as one continuous message stream: click to select, double-click to edit, with context actions (edit, duplicate, delete, move, copy) and an add-message bar. Every mutation flows through `ProjectDocument` into the real core data.
- **Deterministic renderer** — `KonataRenderer` consumes core data and produces a render model without mutating it. One painter (`CanvasChatPainter`) serves both the on-screen preview and PNG export, so what you see is what you export.
- **Apple Messages skin** — the first app configuration (dark and light, plus a neutral Konata variant), built purely from a generic app-UI vocabulary so other chat apps can follow without renderer forks. Two reference mockups live in `Image Refrence/Apple Imessages/`.
- **Canvas presets** — 1080-based output sizes (square 1:1, portrait 4:5 and 9:16, landscape 16:9; portrait 9:16 is the default) plus custom dimensions with validation.
- **`.konata` persistence** — whole projects (story, app definition, canvas, custom UI design) save and load as versioned JSON; new projects start with structured sample data so every panel works immediately.
- **PNG export** — renders the selected conversation to an exact-size PNG containing only the chat surface (header, messages, composer), with no window or editor chrome.
- **Developer Mode** — a separate canvas for editing a custom `UiDesign` element list (bubbles, avatars, text, composer, …), persisted inside the `.konata` file.

## Architecture

Four static libraries plus the executable, with strict layering (UI never owns story data):

```text
KONATA.exe ──▶ ui/                  # MainWindow, panels, authoring editors, Developer Mode
    ├──▶ document/ProjectDocument   # QObject bridge: widgets → ProjectManager → KonataProject
    ├──▶ KonataExport               # ExportService: project + conversation → PNG
    │       └──▶ KonataRendering    # KonataRenderer → RenderModel → CanvasChatPainter (+ Apple skin)
    │               └──▶ KonataCore # Story/Character/Conversation/Message/Scene/AppDefinition/…
    └──▶ KonataSerialization        # .konata JSON save/load (nlohmann/json), ProjectManager
```

- `KonataCore` links only `Qt6::Core` — no widgets, no rendering.
- `KonataRendering` adds `Qt6::Gui` (painting, skins).
- `KonataExport` combines rendering and serialization for file output.
- `ProjectDocument` holds stable core IDs (never raw pointers into widgets) and emits `projectChanged` / `selectionChanged` / `dirtyChanged` signals the panels subscribe to.

Entry point: `src/main.cpp` — `QApplication` named `KONATA`, main window opened at 1280×800.

## Project structure

```text
KONATA/
├── CMakeLists.txt            # version 0.1.0; all targets and checks
├── src/
│   ├── main.cpp              # entry point
│   ├── core/                 # UI-independent model (Id, Character, Message, Conversation,
│   │                         #   Scene(+Event), Story, AppDefinition, CanvasSettings, UiDesign…)
│   ├── document/             # ProjectDocument: QObject bridge between UI and core
│   ├── serialization/        # KonataProject, ProjectSerializer (.konata JSON), ProjectManager
│   ├── rendering/            # KonataRenderer, RenderModel, CanvasLayout, CanvasChatPainter,
│   │                         #   skins/ (AppleMessagesSkin + geometry/palette/typography/icons)
│   ├── export/               # ExportService: conversation → PNG
│   └── ui/                   # MainWindow, ProjectPanel, PreviewArea, InspectorPanel,
│                             #   TimelinePanel (placeholder), RoundedBubble,
│                             #   authoring/ (5 editor widgets), DeveloperMode/DeveloperCanvas
├── resources/
│   ├── resources.qrc         # bundles styles/dark.qss
│   └── styles/dark.qss       # dark theme
├── tests/                    # 12 lightweight check programs (see Testing)
└── Image Refrence/Apple Imessages/   # visual reference mockups for the Apple skin
```

## The `.konata` project format

Projects are saved with the `.konata` extension (constants in `src/serialization/ProjectSerializer.h`):

- UTF-8 JSON document with `"format": "konata-project"` and `"formatVersion": 1`.
- Payload: `story` (characters, conversations with messages/reactions/attachments, scenes with events), `appDefinition` (theme, navigation, chat/message/notification styling), plus optional `canvas` and `uiDesign` sections that fall back to defaults when absent.
- Loading is strict: a mismatched format version or an unknown enum value is an error, never a silent default — so files that fail to load say why instead of opening corrupted.

## Technology stack

- **C++17** (`CMAKE_CXX_STANDARD 17`, extensions off), warnings-clean intent.
- **Qt 6.8**: Widgets (UI), Gui (rendering/export), Core (model/serialization), with `AUTOMOC`/`AUTOUIC`/`AUTORCC`.
- **nlohmann/json 3.11.3** (header-only), fetched at configure time via `FetchContent` — not vendored.
- **CMake ≥ 3.21**, developed with MSVC 2022 x64.

## Prerequisites

- Windows with MSVC 2022 (x64) and CMake ≥ 3.21.
- A Qt 6.8 `msvc2022_64` kit. Inside an [AETHER](https://github.com/adamsalawu/AETHER) checkout the default in `CMakeLists.txt` already points at `C:/Aether/devtools/Qt/6.8.1/msvc2022_64`; standalone clones must pass their own kit with `-DCMAKE_PREFIX_PATH=...`.
- Network access on the first configure (to fetch `nlohmann/json` v3.11.3).

## Build and run

From a Visual Studio x64 developer prompt, in the repository root:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Aether/devtools/Qt/6.8.1/msvc2022_64"
cmake --build build --config Release
.\build\Release\KONATA.exe
```

(Exact output paths vary with the CMake generator; `Release\` applies to the default Visual Studio multi-config generator. These commands mirror `CMakeLists.txt`; a full clean rebuild was not performed while writing this README, but a prior local `build/` tree with these same targets exists.)

The app opens with a fresh untitled project so the tree, editors, and preview all have structured data immediately. Use **File → New / Open / Save / Save As** for `.konata` files, **File → Export PNG** for the current conversation, and the **View** menu for skins, panels, and Developer Mode.

## Testing

There is no CTest/GTest harness. Instead, `tests/` holds 12 lightweight `main()`-style check programs, built as `Konata*Check` executables, each printing `[PASS]`/`[FAIL]` lines and exiting non-zero on failure:

| Check program | Test source | Covers |
|---|---|---|
| `KonataCoreCheck` | `tests/CoreSmokeCheck.cpp` | IDs, characters, messages, reactions, attachments, conversation ordering, scene events, story, app definition |
| `KonataSerializationCheck` | `tests/SerializationRoundTrip.cpp` | Memory + file round-trip, ID continuity, corrupt-file and bad-version handling |
| `KonataAuthoringCheck` | `tests/AuthoringFlowCheck.cpp` | Document-level authoring flow and panel wiring |
| `KonataMessageEventCheck` | `tests/MessageEventCheck.cpp` | Message/event authoring and nested wiring |
| `KonataChatEditorCheck` | `tests/ChatEditorCheck.cpp` | Selection, manipulation, and the add-message bar |
| `KonataRendererCheck` | `tests/RendererCheck.cpp` | Renderer model and preview integration |
| `KonataRendererRefinementCheck` | `tests/RendererRefinementCheck.cpp` | Avatars and image positioning |
| `KonataAppUiCheck` | `tests/AppUiCheck.cpp` | Generic app-UI architecture |
| `KonataAppleMessagesCheck` | `tests/AppleMessagesCheck.cpp` | Apple visual foundation |
| `KonataAppleMessagesExactCheck` | `tests/AppleMessagesExactCheck.cpp` | Exact Apple reconstruction |
| `KonataCanvasCheck` | `tests/CanvasCheck.cpp` | Canvas aspects and sizes |
| `KonataExportCheck` | `tests/ExportCheck.cpp` | PNG dimensions, determinism, no-chrome rule, empty/group/image/system cases |

They build alongside the app (`cmake --build build --config Release`), then run each executable, e.g. `.\build\Release\KonataCoreCheck.exe`. On a headless machine, GUI-touching checks may need `QT_QPA_PLATFORM=offscreen`.

## Known limitations

- **Timeline is a placeholder.** `TimelinePanel` shows a context readout ("Timeline region — functionality arrives in a later milestone"); scenes have no playback logic in core.
- **No asset pipeline.** Image/file message attachments and avatar references are stored as references only; delivery/read state is not implemented.
- **Preview composer is display-only** (its buttons are disabled in this milestone).
- **Version strings are mid-flux** (`0.1.0` in CMake vs `0.1A` reported by the app) — treat all of them as pre-release.
- **Windows- and MSVC-centric.** Developed against the MSVC 2022 x64 + Qt 6.8 kit; other toolchains and platforms are unverified, and the default Qt path in `CMakeLists.txt` assumes the AETHER checkout layout (overridable via `-DCMAKE_PREFIX_PATH`).

## Roadmap (planned)

Explicitly not implemented yet — listed here so the direction is visible, not to claim otherwise:

- Timeline playback for scenes (send/wait/typing events over time).
- An asset pipeline so image/file attachments and avatars resolve to real pixels.
- Delivery/read-state modeling and composer input in the preview.
- More app skins beyond Apple Messages / neutral, driven by the generic app-UI vocabulary.

## License

This repository currently includes no `LICENSE` file, so no open-source license grant (MIT, GPL, Apache, or otherwise) should be assumed.
