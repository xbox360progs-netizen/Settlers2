# Platform Audit — Settlers2 Xbox 360 → Win32

## 0. Architectural Vision

### Invariant: engine is multi-threaded on all platforms

The Xbox 360 uses a Core0/Core1 split (Simulation→Render→GPU).
Win32 must preserve the identical logical thread architecture:

```
Xbox 360                          Win32
────────                          ─────
Thread 0 → Simulation             Thread 0 → Simulation
Thread 1 → Render (→ GPU)        Thread 1 → Render (→ GPU)
Thread 2 → Streaming/Loading     Thread 2 → Streaming/Loading
Thread n → Worker pool (Jobs)    Thread n → Worker pool (Jobs)
```

Physical core count may differ; logical threading model is identical.
This guarantees:
- Same concurrency bugs reproduce on Win32 (debuggable)
- Profiling results are representative
- No platform-specific race conditions

### Platform abstraction layer

```
GameEngine
     │
     ├── SimulationCore     (platform-independent)
     ├── Renderer           (uses Platform::Window, Platform::D3D9Context)
     ├── SceneManager       (uses Platform::Thread, Platform::Event, Platform::CommandBuffer)
     └── ResourceSystem     (uses Platform::FileSystem)
     
All above use:
     Platform::Thread / Mutex / Event / Semaphore / Atomic
     Platform::Affinity
     Platform::Timer
     Platform::FileSystem
          │
     ┌────┴────┐
 Win32Impl   Xbox360Impl
```

### Design rule

**No `#ifdef _XBOX` in game logic.** All platform variance lives in `Platform/` implementations.
The engine code calls only `Platform::Thread`, `Platform::Mutex`, etc.
Two exceptions: Renderer device creation (D3D9 params differ) and startup code (Win32 needs a window).

### Invariant: no raw platform APIs outside Platform/

Code outside `Platform/` must not use platform-specific APIs directly:
`CRITICAL_SECTION`, `CreateThread`, `XSetThreadProcessor`, `WaitForSingleObject`,
`CreateEvent`, `HANDLE`, `IDirect3D***`, `xboxkrnl`, or any `_XBOX`-conditioned
native call. All platform dependencies pass through `Platform/`.

**Status as of July 2026 cleanup:**

| Raw API | Replacement | Status |
|---------|-------------|--------|
| `CRITICAL_SECTION` | `Platform::Lock` | ✅ All 6 users migrated |
| `CreateThread` + `HANDLE` | `Platform::Thread` | ✅ JobManager migrated |
| `CreateEvent` + `WaitForSingleObject` | `Platform::Event` | ✅ JobManager migrated |
| `InterlockedIncrement`/`Decrement` | `Platform::Atomic*` | ✅ JobManager migrated |
| `MemoryBarrier` | `Platform::MemoryFence` | ✅ JobManager migrated |
| `XSetThreadProcessor` | `Platform::SetThreadAffinity` | ✅ JobManager + GameEngine |
| `IDirect3DAsyncCommandBufferCall9` | — (dead code, removed) | ✅ SceneManager cleaned |
| `GetTickCount`/`Sleep` | `Platform::GetTickCount`/`Platform::Sleep` | ✅ GameEngine migrated |

---

## 1. Priority Order (corrected)

### P0 — Win32 startup + Renderer device creation (blocking)

Before anything else, the Win32 application must be able to start and show a window:

| What | Why |
|------|-----|
| Create Win32 window (HWND) | D3D9 device needs a window handle |
| Create D3D9 device (`Direct3DCreate9` + `CreateDevice`) | Currently hardcoded to Xbox params |
| Message loop (`PeekMessage`/`DispatchMessage`) | Win32 requires it; Xbox has none |
| Handle window events (WM_SIZE, WM_DESTROY, WM_KEYDOWN, etc.) | Keyboard/mouse input, resize, close |

**Deliverable:** `Settlers2.exe` starts, shows a window, clears to a colour, handles close.

### P1 — SceneManager threading model

The async command buffer (`IDirect3DAsyncCommandBufferCall9`) was prepared in SceneManager but never wired into the render pipeline. It has been removed as dead code.

A future two-thread render pipeline must be built from scratch using Platform/ primitives:

| Concept | Implementation |
|---------|---------------|
| Cross-core command handoff | `Platform::Event` (double-buffered) |
| Render queue protection | `Platform::Lock`-protected `RenderQueue` |
| Producer thread (Core1) | Writes commands, signals event |
| Consumer thread (Core0) | Waits on event, consumes commands |

**Deliverable:** Identical two-thread render pipeline on both platforms, differing only in
`Platform/` implementation.

### P2 — Platform abstraction layer expansion

| Abstraction | Xbox impl | Win32 impl | Consumers |
|-------------|-----------|------------|-----------|
| `Platform::Thread` | `CreateThread` + XSetThreadProcessor | `CreateThread` + `SetThreadAffinityMask` | JobManager, LoadingScene, SceneManager |
| `Platform::Mutex` | `CRITICAL_SECTION` | `CRITICAL_SECTION` | World/Map, TextureRegistry, ShaderManager, SceneManager, AICommandQueue |
| `Platform::Event` | `CreateEvent`/`SetEvent`/`WaitForSingleObject` | Same (Win32 API) | JobManager worker wake |
| `Platform::Semaphore` | Xbox semaphore API | `CreateSemaphore` | Resource streaming |
| `Platform::Atomic` | `InterlockedExchange`/`InterlockedCompareExchange` | Same (intrinsics) | JobManager queue, LoadingScene progress |
| `Platform::Affinity` | `XSetThreadProcessor` | `SetThreadAffinityMask` | Core assignment |
| `Platform::Timer` | `GetTickCount()` + `QueryPerformanceCounter` | Same | GameEngine frame timing, FPS counter |
| `Platform::FileSystem` | `"game:\\Media\\..."` | Relative `"Media/..."` | Shader/Texture/Config paths |
| `Platform::Window` | Not needed (Xboot) | `CreateWindowEx`, HWND | Renderer, InputManager, message loop |

### P3 — Filesystem paths

Replace all hardcoded `"game:\\Media\\..."` paths with `Platform::FileSystem::GetMediaPath()`.
Both platforms define their own root. This is mechanical (5 files).

### P4 — Input abstraction

| Aspect | Xbox | Win32 |
|--------|------|-------|
| Gamepad | `XInputGetState` | XInput (same API) |
| Keyboard | Not available | `WM_KEYDOWN`/`WM_KEYUP` |
| Mouse | Not available | `WM_MOUSEMOVE`/`WM_LBUTTONDOWN` |
| ScreenKeyboard | D3D9-rendered | Same + IME support |

---

## 2. Current Xbox Dependencies

### Build System
| Aspect | Detail | Impact |
|--------|--------|--------|
| Project keyword | `Xbox360Proj` | Must add Win32 configurations |
| Preprocessor | `_XBOX` defined in all configs | Must add `_WIN32` / `WIN32` configs |
| SDK libraries | `xapilib`, `d3d9`, `xgraphics`, `xboxkrnl`, `xnet`, `xaudio`, `xact`, `x3daudio`, `xmcore`, `xbdm`, `vcomp` | 12 Xbox SDK libs — each has a Win32 equivalent or stub |
| Post-build | `ImageXex` (XEX packaging) | Not applicable on Win32 |

### Entry Point
| File | Detail | Impact |
|------|--------|--------|
| `Settlers2.cpp:11` | `int main(int argc, char** argv)` | Already portable — no `WinMain`/`wWinMain` |

### Rendering (Direct3D 9)
| File | Dependency | Notes |
|------|-----------|-------|
| `Graphics/Renderer.cpp` | `Direct3DCreate9`, `CreateDevice`, `Present`, `BeginScene/EndScene`, `Clear` | D3D9 is available on Win32 |
| `Graphics/ShaderManager.cpp` | `ID3DXEffect*`, `D3DXCreateEffectFromFileA`, shader root paths | D3DX9 available via legacy DirectX SDK |
| `Graphics/TextureLoader.cpp` | `#ifdef _XBOX` — `LoadAtlas` path, `D3DPOOL_DEFAULT` vs `D3DPOOL_MANAGED` | Pool selection differs per platform |
| `Scene/SceneManager.h/.cpp` | — | **Cleaned** — async command buffer types removed (dead code, never wired up) |
| `Graphics/Xbox360EDRAM.h/.cpp` | Xbox 360 EDRAM budget/resolve | **Xbox 360 exclusive** — guarded by `#ifdef _XBOX` |
| `Graphics/SpriteAtlas.cpp:81` | `#ifdef _XBOX` — `loader.LoadAtlas()` vs `loader.Load()` | Different atlas loading paths |
| 41 files | `#include <d3d9.h>` | D3D9 header is available on Win32 |

### Input
| File | Dependency | Notes |
|------|-----------|-------|
| `Input/Gamepad.cpp:55` | `XInputGetState()` | XInput is available on Win32 (DirectX SDK) |
| `Input/InputManager.h:17` | `HWND m_hWnd` | HWND is Win32-native — unused on Xbox |
| `Input/ScreenKeyboard.h` | `#include <d3d9.h>` | Uses D3D9 for rendering only |

### Audio
| File | Dependency | Notes |
|------|-----------|-------|
| Project libs | `xaudiod2.lib`, `xactd3.lib`, `x3daudiod.lib` | Xbox XAudio2/XACT/X3DAudio libraries |
| Source files | Not located | Likely under `Audio/` directory |

### Filesystem
| File | Pattern | Notes |
|------|---------|-------|
| `Graphics/ShaderManager.cpp:13` | `"game:\\Media\\Shaders\\"` | Xbox path format (`game:\` prefix) |
| `Graphics/OverlayFPS.cpp:21` | `L"game:\\Media\\Fonts\\..."` | Xbox path format |
| `Graphics/TextureLoader.cpp:186` | `L"game:\\Media\\Textures\\"` | Xbox path format |
| `Scene/LoadingScene.cpp:294,314,320,325` | `"game:\\Media\\..."` | Xbox path format |
| `Scene/GameScene.cpp:169` | `"game:\\Media\\Config\\"` | Xbox path format |

### Threading & Synchronization (outside Platform/)
| File | APIs | Notes |
|------|------|-------|
| `Core/JobManager.cpp` | `CreateThread`, `CreateEvent`, `SetEvent`, `WaitForSingleObject`, `CloseHandle`, `InterlockedIncrement/Decrement/CompareExchange`, `MemoryBarrier`, `Sleep`, `XSetThreadProcessor` | Mix of Win32 + Xbox-specific core affinity |
| `Core/GameEngine.cpp` | `XSetThreadProcessor`, `GetTickCount`, `Sleep` | Core affinity + timing |
| `Core/ThreadSync.h` | `CRITICAL_SECTION` (guarded `#ifdef _XBOX` / `#else windows.h`) | Platform-conditional include selection |
| `Logic/AICommandQueue.h` | `CRITICAL_SECTION` | Direct CRITICAL_SECTION usage |
| `Scene/LoadingScene.cpp` | `WaitForSingleObject`, `XCloseHandle`, `InterlockedExchange/ExchangeAdd` | Thread join + progress |
| `World/Map.h/.cpp` | `CRITICAL_SECTION` | Direct CRITICAL_SECTION usage |
| `Graphics/TextureRegistry.h/.cpp` | `CRITICAL_SECTION` | Direct CRITICAL_SECTION usage |
| `Graphics/ShaderManager.h/.cpp` | `CRITICAL_SECTION` | Direct CRITICAL_SECTION usage |
| `Scene/SceneManager.h/.cpp` | `CRITICAL_SECTION`, `Lock/Unlock` | Direct CRITICAL_SECTION usage |
| `Graphics/RenderDebugOverlay.cpp` | `QueryPerformanceCounter` | Available on both platforms |

### Timing
| File | APIs | Notes |
|------|------|-------|
| `Core/GameEngine.cpp` | `GetTickCount()`, `Sleep()` | Available on both platforms |
| `Scene/EditorScene.cpp` | `GetTickCount()` | Available on both platforms |
| `Graphics/OverlayFPS.cpp` | `GetTickCount()` | Available on both platforms |

---

## 3. Already Portable Modules

| Module | Evidence |
|--------|----------|
| **SimulationCore** | Fully platform-independent (proven by `SimulationCore.sln`) |
| **World** (model) | No `_XBOX` ifdefs, no XDK includes, no D3D types |
| **AI logic** | Pure logic (except `AICommandQueue.h` — has CRITICAL_SECTION) |
| **Scene** (GameScene, EditorScene) | Use D3D types from Scene.h but contain minimal platform logic |
| **UI** | Uses D3D types from Widget.h but no platform code |
| **Editor** | No platform-specific code |

---

## 4. Platform Abstraction Layer Design

### Layout

```
Platform/
    Lock.h              [done]      — Mutex abstraction (PIMPL)
    Thread.h            [planned]   — Create, join, affinity
    Event.h             [planned]   — Wake/signal between threads
    Semaphore.h         [planned]   — Resource counting
    Atomic.h            [planned]   — Atomic ops (exchange, increment, barrier)
    Affinity.h          [planned]   — Core assignment
    Timer.h             [planned]   — Tick count, QPC
    FileSystem.h        [planned]   — Path root, file read
    Window.h            [planned]   — Win32 window creation (Xbox stub)

    Win32/
        Lock.cpp
        Thread.cpp
        Event.cpp
        Semaphore.cpp
        Atomic.cpp
        Affinity.cpp
        Timer.cpp
        FileSystem.cpp
        Window.cpp

    Xbox360/
        Lock.cpp
        Thread.cpp
        Event.cpp
        Semaphore.cpp
        Atomic.cpp
        Affinity.cpp
        Timer.cpp
        FileSystem.cpp
        Window.cpp          (stub — no window on Xbox)
```

### Thread API

```cpp
namespace Platform {

    // Opaque handle to an OS thread
    class Thread {
    public:
        Thread();
        ~Thread();

        // Start thread executing `entry` with `param`.
        // Returns false if thread creation failed.
        bool Start(unsigned int (WINAPI* entry)(void*), void* param);

        // Wait for thread to exit.
        void Join();

        // Get native handle (for SetThreadAffinityMask / XSetThreadProcessor).
        void* GetNativeHandle();

    private:
        struct Impl* m_impl;
    };

    // Set thread affinity (which core(s) this thread may run on).
    // On Xbox: maps to XSetThreadProcessor.
    // On Win32: maps to SetThreadAffinityMask.
    void SetThreadAffinity(void* nativeHandle, unsigned int processor);

} // namespace Platform
```

### Event API

```cpp
namespace Platform {

    // Manual-reset or auto-reset event for thread signalling.
    class Event {
    public:
        Event(bool manualReset);
        ~Event();

        void Signal();
        void Reset();
        bool Wait(unsigned int timeoutMs);  // false = timeout

    private:
        struct Impl* m_impl;
    };

} // namespace Platform
```

### Mutex usage rule

`Platform::Lock` already exists but is currently unused. All 7 files with raw
`CRITICAL_SECTION` usage should migrate to `Platform::Lock`:

- `Core/ThreadSync.h`
- `Logic/AICommandQueue.h`
- `World/Map.h/.cpp`
- `Graphics/TextureRegistry.h/.cpp`
- `Graphics/ShaderManager.h/.cpp`
- `Scene/SceneManager.h/.cpp`

---

## 5. Migration Steps

### Phase 1 — Platform abstraction layer

Build out `Platform/` so engine code can be written without `#ifdef _XBOX`:

1. [ ] `Platform/Thread.h` + Win32 impl + Xbox360 impl
2. [ ] `Platform/Event.h` + Win32 impl + Xbox360 impl
3. [ ] `Platform/Atomic.h` (header-only — intrinsics are the same)
4. [ ] `Platform/Affinity.h` (inline or trivial)
5. [ ] `Platform/Timer.h` (header-only — wraps GetTickCount/QPC)
6. [ ] `Platform/FileSystem.h` + Win32 impl + Xbox360 impl
7. [ ] `Platform/Window.h` + Win32 impl + Xbox360 (stub)

### Phase 2 — Refactor engine code to use Platform/

1. [ ] Refactor `Core/JobManager` → `Platform::Thread`, `Platform::Event`, `Platform::Atomic`
2. [ ] Refactor `Core/GameEngine` → `Platform::Timer`, `Platform::Affinity`
3. [ ] Refactor `Scene/SceneManager` → `Platform::Thread`, `Platform::Event` (command buffer abstraction)
4. [ ] Refactor `Scene/LoadingScene` → `Platform::Thread`, `Platform::Atomic`
5. [ ] Refactor 7 files with raw CRITICAL_SECTION → `Platform::Lock`
6. [ ] Refactor filesystem paths → `Platform::FileSystem::GetMediaPath()`
7. [ ] Refactor `Graphics/Renderer` → `Platform::Window` for HWND creation

### Phase 3 — Win32 build configuration

1. [ ] Add Debug|Win32, Release|Win32 to `Settlers2.vcxproj`
2. [ ] Add Win32 libs (d3d9.lib, d3dx9.lib, xinput.lib, winmm.lib, etc.)
3. [ ] Conditionally compile `Platform/Win32/*.cpp` vs `Platform/Xbox360/*.cpp`
4. [ ] Create Win32 window on startup (`Platform::Window`)
5. [ ] Add message loop (`PeekMessage` → `DispatchMessage`)
6. [ ] Handle keyboard/mouse input

### Phase 4 — Verification

1. [ ] Build Debug|Win32 of full `Settlers2.sln` with 0 errors
2. [ ] Application starts and shows window
3. [ ] Multi-threaded render pipeline works (Simulation thread + Render thread)
4. [ ] All scenes render correctly
5. [ ] Input works (keyboard, mouse, gamepad)
6. [ ] Filesystem paths resolve correctly (Media/ instead of game:\Media\)
7. [ ] No `#ifdef _XBOX` outside `Platform/` or Renderer device setup

---

## Dependency Matrix

| Subsystem | Portable | Work needed |
|-----------|----------|-------------|
| SimulationCore | ✅ Fully | None |
| World (model) | ✅ Fully | None |
| AI logic | ✅ Mostly | `AICommandQueue.h` → `Platform::Lock` |
| Scene (Game/Editor/Menu) | ✅ Mostly | Inherits D3D types from Renderer |
| SceneManager | ✅ Cleaned | Async command buffer removed (dead code); Lock → Platform::Lock |
| **Renderer** | ❌ | Win32 window creation + D3D9 device + message loop |
| **Graphics pipeline** | ⚠️ Partial | D3D9 on Win32 (same API, different params) |
| UI | ✅ Mostly | Uses D3D types from Renderer |
| Input | ❌ Partial | Keyboard/mouse (Win32 messages) + XInput gamepad |
| Audio | ❌ Unknown | Source files not located |
| Filesystem | ⚠️ Needs work | `game:\` → relative paths via `Platform::FileSystem` |
| Threading | ✅ Cleaned | All 6 CRITICAL_SECTION users → Platform::Lock; JobManager → Platform::Thread/Event/Atomic/Affinity |
| Build | ❌ Blocking | Xbox SDK libs → Win32 libs + configs |
| Entry point | ✅ Already portable | `main()` — no WinMain needed |

---

## Key Metrics

| Metric | Current | Target |
|--------|---------|--------|
| `#ifdef _XBOX` in game code | 18 instances (7 files) | 0 (all in Platform/) |
| Files with raw CRITICAL_SECTION | 7 | 0 (all use Platform::Lock) |
| Xbox SDK lib dependencies | 12 per config | 0 (Win32 native libs) |
| Thread model | Same on both platforms | Same — invariant preserved |
| Build configurations | 6 Xbox-only | 6 Xbox + 2 Win32 |
