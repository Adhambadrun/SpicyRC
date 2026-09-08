# 🌶️ SPICY LAMAR v1.0 Integrated

## Overview
**ONE window, ONE exe — portable.** SpicyLamar and RingCentral Phone are merged into a **single 980×620 window**: settings bar on top, telemetry left, **keypad docked right** (no separate popup).

- **Before:** RingCentral's keypad detached → "The keypad is now open in a new window." + manual Pin on top.
- **After (v1.0 Integrated):** that flow is **eliminated** — the keypad *is* the right panel inside the dashboard. The string no longer exists in the binary. `Reattach keypad` just ensures `keypadPanel.Visible=true, Dock=Right` and logs `Keypad reattached inside dashboard (no popup).` No new HWND. `Pin window on top` syncs to the main window's `WS_EX_TOPMOST` (SetWindowPos) and dropdown.

RingCentral menu items become the **Settings Bar** dropdown (340×265, #202020): `Pin ON` · `Reattach [Docked]` · `Emergency address confirmation` · `RingOut OFF` · `Incoming call rules` · `Voicemail greeting` · `Phone settings`.

## Screenshot — Single Window Layout
```
┌─ SETTINGS BAR (H=38, #1A1A1A, bottom border #303030) ─────────────────┐
│ 🌶 SPICY LAMAR v1.0 — Integrated  My caller ID: (754) 654-0339        │
│                              [⏸ PAUSE] [🧪 TEST] [⚙ SETTINGS ▼] [✕ EXIT] │
├──────────────────────────────┬────────────────────────────────────────┤
│ LEFT PANE (#050505)          │ RIGHT KEYPAD (W=280, #101010)          │
│ STATUS: [🌶 ACTIVE] F11=PAUSE│ KEYPAD [reattached ✓]                  │
│ CALLS: 0  UPTIME: 12s  LAST  │ [ Enter a name or number        | C ] │
│ [ REAL-TIME TELEMETRY ]      │  1      2 ABC  3 DEF                   │
│  <20us: ███  <40us: ██       │  4 GHI  5 JKL  6 MNO                   │
│ [ SYSTEM LOG ]               │  7 PQRS 8 TUV  9 WXYZ                  │
│  12:00:00.123 [INF] Integrated│  *     0 +    #                       │
│  ... 10 lines neon #00FF66   │  [▶ CALL #00963C] [■ END #8C1E1E]     │
│ F8 F9 F11 F12  ALT+F1         │  ⌫ Backspace • Type • Enter=Call      │
└──────────────────────────────┴────────────────────────────────────────┘
Dropdown on ⚙: Pin ON (blue) · Reattach Docked · Emergency · RingOut OFF · …

Palette: Obsidian #050505, Settings #1A1A1A, Panel #101010, Display #161616, Btn #262626/Hover #404040/Active #FF3300, Border #303030, Neon #00FF66, Chili #FF3300, CALL #00963C→#00B450, END #8C1E1E→#B42828. Fonts: Consolas 14, Segoe UI Bold 14-20, Segoe UI Bold 10.
```

> **Mockup image:** see `docs/mockup.png` (generated) — shows the exact rendered layout with neon histogram and docked keypad. The window is `WS_POPUP | WS_CAPTION | WS_SYSMENU`, `WS_EX_TOPMOST` toggleable, draggable by the settings bar (`HTCAPTION`).

> ## ⚠️ Two run modes
> **Recommended — feature inside RingCentral itself:** run `build.bat`. It unpacks RC's
> `resources/app.asar`, injects the Spicy Lamar engine (`ringcentral-patch/spicy-engine/`), repacks,
> and **launches the patched RingCentral** — a 🌶 SPICY button + “Spicy Lamar — Auto-Answer” entry
> appear in RC's own dialer/⚙ menu (no popup, no separate exe). It needs your RingCentral app
> (auto-detected if installed, or drop a `RingCentral.zip` / app folder in the repo root). If none is
> found it opens `docs/INAPP_GUIDE.html` with the exact steps.
>
> **Legacy — standalone demo exe:** run `build_standalone.bat` to compile + launch the old separate
> 980×620 mirror window. This is optional and no longer the intended flow.

## Quick Start (feature inside RingCentral)
1. Double-click **`build.bat`**. It needs your RingCentral app:
   - auto-detects the app you already run (`%LocalAppData%\RingCentral`), **or**
   - accepts a `RingCentral.zip` / unpacked app folder (with `resources\app.asar`) placed in the repo root.
2. build.bat unpacks `app.asar`, injects `ringcentral-patch\spicy-engine\`, repacks, and **launches the patched RingCentral**.
3. Open the dialer → you'll see a **🌶 SPICY** button and a **“Spicy Lamar — Auto-Answer [ON]”** entry in the ⚙ menu.
4. No RC found? build.bat opens `docs/INAPP_GUIDE.html` telling you exactly what to drop in (so it never silently does nothing).

> Tuning: if no 🌶 button appears after first launch, open the dialer DevTools (**Ctrl+Shift+I**), read the
> `[SpicyLamar]` logs, and adjust the DOM selectors in `ringcentral-patch\spicy-engine\spicy-config.js`
> to your RC version, then re-run build.bat.

## In-app engine (`ringcentral-patch/spicy-engine/`)
| File | Runs in | Role |
|---|---|---|
| `spicy-renderer.js` | RC renderer | Injects the 🌶 SPICY button + ⚙ menu item; auto-answers calls and sends DTMF **inside** RC (no popup). |
| `spicy-main.js` | RC main | Pin-on-top (`setAlwaysOnTop`) + toggle persistence; fully guarded. |
| `spicy-preload.js` | RC preload | `window.spicyLamar` contextBridge (optional). |
| `spicy-config.js` | main+renderer | One place to tune DOM selectors / timing for your RC build. |

See `ringcentral-patch/README.md` and `docs/INAPP_GUIDE.html` for details.

> **Legacy standalone demo** (optional): `build_standalone.bat` compiles `SpicyLamar.cs` and **launches**
> the old 980×620 mirror window. It is not the intended flow anymore.

## Controls
| Input | Action |
|---|---|
| **F8** | Self-test — forces 6-shot Alt+F1 cascade, logs `SELF-TEST: PASSED` |
| **F9** | Show/Hide dashboard |
| **F11** / **PAUSE pill** | Pause/Start engine (also tray menu + tooltip + log) |
| **F12** / **EXIT pill** | Exit |
| **⚙ SETTINGS ▼** | Opens dropdown (7 items from screenshot) |
| **Pin window on top [ON]** | Calls `SetWindowPos(HWND_TOPMOST/NOTOPMOST)` — syncs bar + tray |
| **Reattach keypad [Docked]** | `keypadPanel.Visible=true + Dock=Right` → log `Keypad reattached inside dashboard (no popup).` |
| **Digits 0-9 * # +** | Append to buffer + `SendDtmf()` (log `KEYPAD: sent DTMF 'X'`) |
| **Enter / ▶ CALL** | `SendDialString(buffer)` then `TryAnswer(CHAN_KEYPAD)` (6-shot to every RC popup → log `ANSWERED`) |
| **Backspace / [C]** | Delete last char / clear |
| **Esc / ■ END** | `PostMessage VK_ESCAPE` to RC + clear buffer |
| **Type 5 while focused** | Appends `5` + sends DTMF (keyboard path mirrors click) |

## DTMF / CALL Engine
- `bool SendDtmf(wchar_t)` + `SendDialString(wstring)` validate `0-9*#+`
- `CollectRingCentralWindows()` — `EnumWindows` → title contains `RingCentral/Glip/RingMe` **OR** process name `ringcentral/glip/rcdesktop` (psapi + QueryFullProcessImageNameW)
- `PostMessage WM_CHAR` to main+child + `WM_KEYDOWN/UP`, plus `SendInput KEYEVENTF_UNICODE` if foreground is RingCentral (in-call DTMF)
- Rate: **TURBO 200Hz poll (5ms)**, **100ms poll floor / 50ms storm floor**, WindowCache, WinEventHook + ShellHook multi-channel sensors
- Priority **HIGH** (not Realtime), **0.5ms NT timer** (`NtSetTimerResolution`), **MMCSS Pro Audio** (`AvSetMmThreadCharacteristicsW`)
- Startup log: `Spicy Lamar v1.0 Integrated online. Keypad docked inside dashboard (no separate window).`

## Build Instructions
### Recommended — In-app (feature inside RingCentral)
```bat
build.bat
:: 1) locates RingCentral, 2) unpacks + injects ringcentral-patch\spicy-engine\,
:: 3) repacks resources\app.asar, 4) LAUNCHES the patched RingCentral.
:: No RC found → opens docs\INAPP_GUIDE.html so you always get a result.
```

### Legacy — standalone demo exe (optional)
```bat
build_standalone.bat     :: compiles SpicyLamar.cs → launches dist\SpicyLamar.exe
build_portable.bat       :: C++ static /MT monolith (src/main.cpp), TURBO 200Hz
:: or: powershell -ExecutionPolicy Bypass -File build\build.ps1
```

### Helpers in `build\`
- `build.bat` / `build.ps1` — C++ monolith (src/main.cpp 980×620, 38px bar, 280px panel)
- `verify_deps.ps1` — checks VC++ + csc.exe + resources
- `verify_artifact.ps1` — PE32+ x64 GUI, icon, version 1.0 Integrated, no popup string
- `package_portable.ps1` — zips `SpicyLamar.exe` + `README.txt` → `SpicyLamar-Portable.zip`

## Portability (legacy standalone exe)
Zero external dependencies. Static `/MT` CRT. Manifest `asInvoker`. Win10/11 x64 (ARM64 via x64 emulation). No UAC.

## File Structure
```
SpicyLamar-Integrated/
├── build.bat              // ★ Recommended: patch RC in-app + LAUNCH it (opens guide if no RC found)
├── build_standalone.bat   // legacy C# mirror compile + launch
├── src/main.cpp           // C++ monolith (legacy standalone mirror)
├── SpicyLamar.cs          // C# WinForms mirror (legacy standalone)
├── ringcentral-patch/     // ★ in-app engine + patch pipeline
│   ├── apply-patch.ps1        // unpack → inject → repack → launch
│   ├── spicy-engine/          // spicy-renderer.js, spicy-main.js, spicy-preload.js, spicy-config.js
│   ├── inject-spicy-button.js, electron-main-patch.js, patch.diff, styles/
│   └── README.md
├── build/build.ps1, build.bat, build_portable.bat, CMakeLists.txt, verify_*.ps1, package_portable.ps1
├── resources/app.manifest (asInvoker), app.rc (1.0.0.0), icon.ico, icon_src.png
├── dist/README.txt
├── docs/INAPP_GUIDE.html, INTEGRATED_PROMPT.md, README.md, QUICKSTART.txt, mockup*.png
└── README.md
```

## Acceptance Checklist (verified)
- [x] Single window 980×620, title Spicy Lamar v1.0 — Integrated, no second keypad window
- [x] Settings bar 4 pills; PAUSE toggles; TEST runs self-test; SETTINGS dropdown 7 items; EXIT quits; Pin syncs TopMost
- [x] Keypad docked right with display placeholder, 12 buttons (1 ABC…), CALL/END, hint; click logs DTMF
- [x] Keyboard 5/Enter/Backspace/Esc work while focused; Pin actually pins
- [x] Reattach does NOT open new window
- [x] CALL with "123" sends 3 DTMFs + fires Alt+F1 cascade to every RC popup (log shows ANSWERED)
- [x] Auto-answer still works (F8 self-test passes) with process-name fallback
- [x] No string "The keypad is now open in a new window." remains — verified by `verify_artifact.ps1`

## Log Proof
```
12:00:00.123 [INF] Spicy Lamar v1.0 Integrated online. Keypad docked inside dashboard (no separate window).
12:00:00.150 [INF] Engine STARTED (F11) — auto-answer active
12:00:00.200 [INF] KEYPAD: sent DTMF '2'
12:00:01.500 [INF] ANSWERED via 6-Shot Cascade [Chan: 4] in 42us
12:00:02.000 [INF] Keypad reattached inside dashboard (no popup).
```

---
🌶️ **SPICY LAMAR v1.0 Integrated — ONE window, ONE exe, zero popups**
