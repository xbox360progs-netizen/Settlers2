## Goal
- Implement full map save/load system with binary file serialization, 10 save slots, and a text‑based menu for Save/Load/Return to MenuScene.

## Progress

### Done
- **Coordinate system & cursor fixes**
  - Reverted `CoordinateSystem.h:105` back to original formula.
  - Fixed `Map::GetTileUnderMouse` / `Map::GetTileAt` to compare distance to cell center.
  - Changed `MapEditor::SetCursorWorldPosition` to use `GetTileAt` with center snapping for non‑Ground layers.
  - Added sprite pivot offset to non‑Ground cursor rendering.
  - Changed Resources layer A‑button: pressing A on an existing resource now updates its amount directly using `m_resourceAmount`.

- **Map serialization** (`World/MapSerializer.cpp`)
  - Binary format: magic `"SMAP"`, version 1, compact per‑tile writes (type, position, uv, flags, atlasName).
  - Save: writes all 7 layers (Ground, Roads, Nodes, Placement, Resources, Objects, Overlay) + resource node map (weight/type/amount/visibility).
  - Load: clears layers, reads header, restores tiles with dimension bounds checks.
  - Uses **Xbox 360 native file API**: `CreateFileA` / `ReadFile` / `WriteFile` / `CloseHandle` (replaced `fopen_s`/`fread`/`fwrite`/`fclose`).

- **Save/Load menu** (`Scene/EditorScene.cpp` / `.h`)
  - `UpdateSaveLoadMenu()`: switch‑based state machine — main menu (Save / Load / Main Menu / Close), save slot selection with overwrite confirmation, load file list via `FindFirstFileA`.
  - `RenderSaveLoadMenu()`: draws overlay + menu text on top of editor.
  - File‑existence checks use `CreateFileA` with `OPEN_EXISTING` (avoids `INVALID_FILE_ATTRIBUTES` which was undeclared on Xbox 360).
  - Fixed an illegal `break` in the load‑list render section (was inside `if`‑`else` chain, not a `switch`).
  - `RenderSaveLoadMenu` called at end of `Render()`.
  - Menu triggered by Start button; B closes menu.

### In Progress
- (nothing blocked)

### Next Steps
1. Build with Xbox 360 SDK and test: Start opens menu, Save writes to slot, Load reads list and loads selected file, Main Menu transitions to `MenuScene`.
2. Verify save files are written to `game:\Media\Maps\slot_%02d.bin` and load list scans `game:\Media\Maps\*.bin`.

## Key Decisions
- **Binary format** over text — standard Xbox 360 file APIs (`CreateFileA`/`WriteFile`/`ReadFile`), no extra dependencies.
- **File I/O** uses `CreateFileA` / `WriteFile` / `ReadFile` / `CloseHandle` (Xbox 360 XDK style), not C stdio.
- **Directory scanning** uses `FindFirstFileA` / `FindNextFileA` (provided by `<xtl.h>`).
- **Menu** is inline in `EditorScene` (not a separate class), rendering with `TextManager::DrawTextToScreen`.
- **Save slot path**: `game:\Media\Maps\slot_%02d.bin` (10 slots).
- **Load list**: scans `game:\Media\Maps\*.bin` and displays matching files.

## Relevant Files
- `World/MapSerializer.cpp` / `.h`: Binary save/load (7 layers + resource map) using Xbox 360 file API.
- `Scene/EditorScene.cpp`: `UpdateMapEditor` (menu trigger), `UpdateSaveLoadMenu` (state machine), `Render` (menu draw call), `RenderSaveLoadMenu` (menu rendering).
- `Scene/EditorScene.h`: Menu state variables (`m_saveLoadMenuActive`, `m_saveLoadMenuSection`, `m_saveLoadMenuSelection`, `m_saveLoadMenuPendingSlot`, `SAVE_SLOT_COUNT`).
- `Editor/MapEditor.cpp` `SaveMap`/`LoadMap`: delegates to `MapSerializer`.
