# PROMPT: Create New Repo `SpicyLamar-Integrated` — Merge SpicyLamar + RingCentral Phone Into ONE App Window

## ROLE
You are a senior Windows desktop engineer. You will create a NEW repository from scratch that merges two codebases into ONE portable Windows app.

## CONTEXT
Current state (see screenshot `image-1.png`):
- **RingCentral Phone** is a separate Electron app (`Chrome_WidgetWin_1` / `Chrome_RenderWidgetHostHWND`). Its keypad is DETACHED — when you detach it shows "The keypad is now open in a new window." with a menu:
  - `Reattach keypad [↗]`
  - `Pin window on top [toggle ON]`
  - `Emergency address confirmation`
  - `RingOut [toggle OFF]`
  - `Incoming call rules`
  - `Voicemail greeting`
  - `Phone settings`
  - Keypad grid `1 2 3 / ABC DEF` with field `Enter a name or number` and `My caller ID: (754) 654-0339`

- **SpicyLamar** is a separate portable C++ (src/main.cpp, 980 LOC) + C# (SpicyLamar.cs) tray utility that auto-answers RingCentral calls by watching for the RingCentral window and firing a 6-shot Alt+F1 cascade (PostMessage WM_SYSKEYDOWN/UP + child Intermediate D3D + SendInput + WM_COMMAND, NEVER WM_SYSCHAR or Alt+A). Features: TURBO 200Hz poll (5ms), HIGH priority, 0.5ms NT timer, MMCSS Pro Audio, 100ms poll floor / 50ms storm floor, WindowCache + CollectRingCentralWindows (title OR process name ringcentral/glip/rcdesktop), tray + dashboard (F8 self-test, F9 dashboard, F11 pause/start, F12 exit).

**Problem:** Two separate windows, two repos, keypad pops out to a new window, user has to Pin on top manually.

**Target:** ONE new repo, ONE window, ONE exe (SpicyLamar.exe). No stable v1 exists — this new repo BECOMES stable v1.

## GOAL FOR NEW REPO
Create `SpicyLamar-Integrated` where RingCentral Phone's UI is HOSTED/INTEGRATED INSIDE SpicyLamar's dashboard itself.

- ELIMINATE the flow "The keypad is now open in a new window." 
- Clicking `Reattach keypad` must NOT open a new window. It Docks the keypad as a right panel INSIDE the main app.
- `Pin window on top` must sync to the main window's `WS_EX_TOPMOST` (SetWindowPos) and to the Settings Bar toggle.
- All RingCentral menu items become the **Settings Bar** inside the app.

## REQUIREMENTS — NEW REPO STRUCTURE

### 1. Single Window Layout (980x620, WS_POPUP | WS_CAPTION | WS_SYSMENU, WS_EX_TOPMOST toggleable, draggable by settings bar)
```
┌─ SETTINGS BAR (H=38, #1A1A1A, bottom border #303030) ──────────────────┐
│ 🌶 SPICY LAMAR v1.0 — Integrated  My caller ID: (754) 654-0339 │
│                              [⏸ PAUSE] [🧪 TEST] [⚙ SETTINGS ▼] [✕ EXIT] │
├──────────────────────────────┬─────────────────────────────────────────┤
│ LEFT PANE (#050505)          │ RIGHT KEYPAD PANEL (W=280, #101010)     │
│ - Status: ACTIVE/PAUSED      │ - Header: KEYPAD [reattached ✓]         │
│ - CALLS / UPTIME / LAST      │ - Field: Enter a name or number | [C]   │
│ - Telemetry histogram        │ - Grid: 1 2 3 / 4 5 6 / 7 8 9 / * 0 #  │
│   neon green bars            │   (subs ABC DEF GHI JKL MNO PQRS TUV)   │
│ - SYSTEM LOG (10 lines)      │ - [▶ CALL green] [■ END red]            │
│ - Footer hotkeys             │ - Hint: ⌫ Backspace • Type • Enter=Call │
└──────────────────────────────┴─────────────────────────────────────────┘
SETTINGS DROPDOWN (340x ~220, #202020, anchor Top-Right, appears on ⚙):
┌─────────────────────────────────┐
│ SETTINGS ✕                      │
│ ☑ Pin window on top [ON]        │
│ → Reattach keypad [Docked]      │
│ → Emergency address confirmation│
│ ☑ RingOut [OFF]                 │
│ → Incoming call rules           │
│ → Voicemail greeting            │
│ → Phone settings                │
└─────────────────────────────────┘
```

### 2. Integration Logic
- **Host RingCentral inside:** Do NOT launch RingCentral.exe as separate window. Either:
  a) Host its `Chrome_RenderWidgetHostHWND` as a child via `SetParent()` / `FindWindowEx`, OR 
  b) Re-implement its dialer UI natively (preferred for portability) and proxy actions to RingCentral via window messages. The keypad you render IS the RingCentral keypad — keep its exact visuals (1/ABC, 2/DEF layout, Enter a name or number placeholder).

- **Reattach keypad:** Remove `CreateWindowEx` for detached keypad. The button/ menu item Reattach keypad now just ensures `keypadPanel.Visible = true` + `Dock=Right` and logs `Keypad reattached inside dashboard (no popup).` No new HWND.

- **Pin window on top:** One source of truth. Toggle in dropdown AND settings bar must call `SetWindowPos(hwnd, (checked? HWND_TOPMOST:HWND_NOTOPMOST), 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE)` and update tray tooltip/log. Default ON (as in screenshot toggle is blue/ON).

- **DTMF / CALL:** Add to Engine: `bool SendDtmf(wchar_t digit)` + `SendDialString(wstring)` — validate 0-9*#+, collect ALL RingCentral windows via `CollectRingCentralWindows()` (title contains RingCentral/Glip/RingMe OR process name ringcentral/glip/rcdesktop), `PostMessage WM_CHAR` to main+child + `WM_KEYDOWN/UP`, plus `SendInput KEYEVENTF_UNICODE` if foreground is RingCentral (for in-call DTMF). `CALL` button does `SendDialString(buffer)` then `TryAnswer(RC, CHAN_KEYPAD)` (Alt+F1 cascade to answer incoming). `END` sends `VK_ESCAPE` + clears buffer.

- **Keep all SpicyLamar engine:** Turbo poll, WindowCache, WinEventHook + ShellHook multi-channel sensors, rate limiting (100ms poll / 50ms storm), log coalescing, F8 self-test, F11/F12 hotkeys. Log at startup: `Spicy Lamar v1.0 Integrated online. Keypad docked inside dashboard (no separate window).`

### 3. File Structure for NEW Repo
```
SpicyLamar-Integrated/
├── src/main.cpp // C++ monolith — single window + integrated keypad + settings bar + Engine::SendDtmf (980x620, 38px bar, 280px panel)
├── SpicyLamar.cs // C# WinForms mirror — Panel settingsBar Dock Top, Panel keypadPanel Dock Right, Panel leftPane Fill, dropdown Panel
├── build/build.ps1, build.bat, build_portable.bat
├── resources/app.manifest (asInvoker, no elevation), icon.ico
├── dist/README.txt
├── docs/INTEGRATED_PROMPT.md (this prompt)
├── docs/README.md, QUICKSTART.txt
└── README.md (overview + controls + build)
```

### 4. Visual Palette
Obsidian #050505, Settings #1A1A1A, Panel #101010, Display #161616, Btn #262626 Hover #404040 Active #FF3300, Border #303030, Neon #00FF66, Chili #FF3300, CALL green #00963C Hover #00B450, END red #8C1E1E Hover #B42828. Fonts: Consolas 14 telemetry / Segoe UI Bold 14-20 keypad / Segoe UI Bold 10 title.

### 5. Build
Static /MT, -DSPICY_LAMAR_TURBO default, zero deps, Win10/11 x64, manifest asInvoker. `build_portable.bat` → `dist/SpicyLamar.exe` + `SpicyLamar-Portable.zip`. Verified via `verify_artifact.ps1`.

### 6. ACCEPTANCE CHECKLIST
- [ ] `SpicyLamar.exe` launches ONE window (980x620, title Spicy Lamar v1.0 — Integrated), no second "keypad" window ever appears
- [ ] Settings bar at top shows 4 pills; PAUSE toggles engine + tray + log; TEST runs self-test; SETTINGS dropdown shows 7 items from screenshot (Reattach, Pin ON, Emergency, RingOut OFF, Incoming rules, Voicemail, Phone settings); EXIT quits
- [ ] Keypad docked right: display with (754) ID placeholder, 12 buttons, CALL/END, hint; clicking 2 appends 2 + logs KEYPAD: sent DTMF '2'
- [ ] Typing 5 on keyboard while focused appends+sends; Enter=CALL, Backspace=delete, Esc=END; Pin toggle actually pins (TopMost)
- [ ] Reattach keypad does NOT open new window — it just ensures panel docked
- [ ] CALL with buffer "123" sends 3 DTMFs + fires Alt+F1 cascade to every RingCentral popup (log shows ANSWERED)
- [ ] Auto-answer still works (F8 self-test passes) with process-name fallback
- [ ] No string "The keypad is now open in a new window." remains

## CONSTRAINTS
- Do not send Alt+A ever, do not send WM_SYSCHAR VK_F1 (types 'p')
- Preserve single-instance mutex Global\SpicyLamar → fallback Local\
- Keep High priority (not Realtime), MMCSS, 0.5ms timer
- No new external dependencies

## DELIVER
Full repo + both .cpp and .cs building, mockup screenshot, and log message proving integration.
