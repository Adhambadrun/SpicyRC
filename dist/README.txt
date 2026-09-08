SPICY LAMAR v1.0 Integrated - Portable Edition
=============================================================
Single window 980x620 | Settings bar 38px | Keypad docked 280px right | TURBO 200Hz

This bundle contains ONE file that matters:
"SpicyLamar.exe" — the entire app. No second "keypad" window ever appears.

WHAT'S NEW IN v1.0 Integrated
  * RingCentral Phone UI is HOSTED INSIDE the dashboard itself.
  * No "The keypad is now open in a new window." flow — eliminated.
  * "Reattach keypad" now just docks the panel inside (no popup, no HWND).
  * "Pin window on top" syncs to main window WS_EX_TOPMOST + tray tooltip.
  * Right panel IS the RingCentral keypad: 1 2 3 / ABC DEF layout,
    display "Enter a name or number", CALL (green) + END (red).

USAGE
  1. Copy "SpicyLamar.exe" anywhere (USB, Desktop, anywhere).
  2. Double-click it. No install, no admin, no UAC, no DLLs needed.
  3. One window appears (980x620): settings bar on top, telemetry left,
     keypad docked right. Tray icon named "Spicy Lamar".

CONTROLS
  F8            Self-test (F8 diagnostics + Alt+F1 cascade check)
  F9            Show/Hide dashboard
  F11           Pause/Start answering (also PAUSE pill + tray menu)
  F12           Exit
  Click digits  Appends to buffer + sends DTMF via CollectRingCentralWindows
  Enter         CALL — SendDialString(buffer) + Alt+F1 cascade
  Backspace     Delete last digit / [C] clears
  Esc           END — sends VK_ESCAPE to RingCentral + clears buffer
  Pin toggle    Settings dropdown [ON] ↔ main WS_EX_TOPMOST
  Reattach      Logged as "Keypad reattached inside dashboard (no popup)."

ANSWER BEHAVIOR (TURBO)
  * 200Hz poll (5ms, TURBO) — throttled 100ms poll floor / 50ms storm floor
  * Hunts via WindowCache + CollectRingCentralWindows (title OR process name
    ringcentral/glip/rcdesktop) — covers every popup
  * Fires 6-shot Alt+F1 cascade (PostMessage WM_SYSKEYDOWN/UP to main+child
    Intermediate D3D + SendInput + WM_COMMAND, NEVER WM_SYSCHAR or Alt+A)
  * DTMF: PostMessage WM_CHAR to main+child + SendInput UNICODE if foreground is RC
  * Log at startup: "Spicy Lamar v1.0 Integrated online. Keypad docked..."

BUILD
  * C++: double-click build_portable.bat → dist\SpicyLamar.exe (static /MT, x64)
  * C#:  double-click build.bat → dist\SpicyLamar.exe (WinForms mirror)
  * Manifest asInvoker (no elevation), verify via build\verify_artifact.ps1

NOTES
  * Windows 10 / 11 x64. Single file, zero external dependencies.
  * If SmartScreen warns on first run: More info -> Run anyway
    (or right-click the exe -> Properties -> Unblock).
