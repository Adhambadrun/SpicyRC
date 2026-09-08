# 🌶️ SPICY LAMAR v1.0 Integrated

## Overview
Portable Windows tray utility — **ONE window, ONE exe** (980×620). RingCentral Phone's UI is **hosted inside** SpicyLamar's dashboard: settings bar on top, telemetry left, **keypad docked right** (no separate popup).

- Eliminates flow “The keypad is now open in a new window.” — that string no longer exists in the binary.
- **Reattach keypad** → `keypadPanel.Visible = true` + `Dock=Right` and log `Keypad reattached inside dashboard (no popup).` No new HWND.
- **Pin window on top** → single source of truth, calls `SetWindowPos(HWND_TOPMOST/NOTOPMOST)` and syncs droplet + tray tooltip. Default **ON**.
- Keypad IS the RingCentral keypad (1/ABC, 2/DEF… `Enter a name or number` placeholder, `My caller ID: (754) 654-0339`).

## Layout
```
┌─ SETTINGS BAR (H=38, #1A1A1A, border #303030) ─────────────────┐
│ 🌶 SPICY LAMAR v1.0 — Integrated  My caller ID: (754) 654-0339 │
│                              [⏸ PAUSE] [🧪 TEST] [⚙ SETTINGS ▼] [✕ EXIT] │
├──────────────────────────────┬─────────────────────────────────┤
│ LEFT PANE (#050505)          │ RIGHT KEYPAD (W=280, #101010)   │
│ STATUS / CALLS / UPTIME      │ KEYPAD [reattached ✓]           │
│ Telemetry histogram (neon)   │ [ Enter a name or number | C ]  │
│ SYSTEM LOG (10 lines)        │ 1 2 3 / 4 5 6 / 7 8 9 / * 0 #  │
│                              │ [▶ CALL] [■ END]                │
└──────────────────────────────┴─────────────────────────────────┘
Dropdown (340×265, #202020): Pin ON · Reattach · Emergency · RingOut OFF · Incoming · Voicemail · Phone settings
```

## Quick Start
Grab `dist\SpicyLamar.exe` (or `SpicyLamar-Portable.zip`), copy anywhere, double-click. No admin, no UAC, no install. Window appears with docked keypad. Hotkeys below.

## Controls
| Key / Click | Action |
|---|---|
| **F8** | Self-test (F8 diagnostics, fires Alt+F1 cascade with force) |
| **F9** | Show/Hide dashboard |
| **F11** | Pause/Start engine (also PAUSE pill + tray menu) |
| **F12** | Exit |
| Type `0-9 * # +` | Append to buffer + `SendDtmf()` via `CollectRingCentralWindows()` |
| **Enter** | CALL — `SendDialString(buffer)` then `TryAnswer(CHAN_KEYPAD)` |
| **Backspace / [C]** | Delete / clear buffer |
| **Esc / END** | Sends `VK_ESCAPE` to RC + clears buffer |

## DTMF / CALL Engine
- `bool SendDtmf(wchar_t)` + `SendDialString(wstring)` validate `0-9*#+`
- `CollectRingCentralWindows()` — `EnumWindows` + title contains RingCentral/Glip/RingMe **OR** process name `ringcentral/glip/rcdesktop`
- `PostMessage WM_CHAR` to main+child + `WM_KEYDOWN/UP`, plus `SendInput KEYEVENTF_UNICODE` if foreground is RingCentral (in-call DTMF)
- CALL → `SendDialString` then 6-shot Alt+F1 cascade to **every** RC popup (log `ANSWERED`)

## Build
- **C++** (recommended): double-click `build_portable.bat` → `dist\SpicyLamar.exe` (static /MT, `-DSPICY_LAMAR_TURBO`, `timeBeginPeriod(1)` + `NtSetTimerResolution(0.5ms)` + `AvSetMmThreadCharacteristicsW(Pro Audio)`)
- **C#**: double-click `build.bat` → `dist\SpicyLamar.exe` (WinForms mirror: `settingsBar` Dock Top, `keypadPanel` Dock Right, `leftPane` Fill, `dropdownPanel`)
- Verify: `powershell -ExecutionPolicy Bypass -File build\verify_artifact.ps1`

## Portability
Zero deps, static CRT, manifest `asInvoker`, Win10/11 x64.

## File Structure
```
SpicyLamar-Integrated/
├── src/main.cpp  // single window + integrated keypad + settings bar + Engine::SendDtmf (980x620, 38px bar, 280px panel)
├── SpicyLamar.cs // WinForms mirror — Panel settingsBar Dock Top, Panel keypadPanel Dock Right, Panel leftPane Fill, dropdown Panel
├── build/build.ps1, build.bat, build_portable.bat
├── resources/app.manifest, icon.ico
├── dist/README.txt
├── docs/INTEGRATED_PROMPT.md
└── README.md
```
