# 🌶️ Spicy Lamar — RingCentral in-app integration

Spicy Lamar is installed **inside RingCentral Phone's own Electron app**. It adds
controls to the existing RingCentral dialer and Settings menu; it does not launch
`SpicyLamar.exe`, create a keypad popup, or operate a separate dashboard.

## What was fixed

The patch no longer relies on a fragile single `index.html` edit. It now:

1. injects the renderer engine through RingCentral's Electron main process after
   each RingCentral renderer load (works with hashed/bundled renderer files);
2. also patches a discovered existing preload so the renderer has a narrow,
   explicit bridge for its settings;
3. keeps state in RingCentral's Electron user-data folder and re-syncs it after
   renderer reloads; and
4. retains an `index.html` insertion only as an idempotent compatibility fallback.

## Scope and safety

**Every Spicy Lamar control is scoped to RingCentral.**

- `🌶 SPICY ON/OFF` is inserted immediately beside RingCentral's own **CALL**
  control. It toggles only the in-app auto-answer engine.
- RingCentral's Settings menu receives **Spicy Lamar — Auto-Answer** and
  **Pin RingCentral on top** entries.
- Auto-answer clicks only a visible **Answer/Accept** element inside a visible
  incoming-call surface in the current RingCentral renderer.
- DTMF and call helper paths click RingCentral's own keypad/CALL controls or
  update RingCentral's own controlled dial field. They do not synthesize input
  to the desktop.
- Pin-on-top calls Electron `setAlwaysOnTop` on the RingCentral BrowserWindow
  that sent the request—not on any other application window.
- The in-app path installs **no global hotkeys, no `SendInput`, no Alt+F1
  cascade, no focus stealing, and no extra process/window**.

The legacy native sources remain in the repository for historical reference, but
are **not used** by the supported flow. `build_portable.bat` and
`build_standalone.bat` intentionally redirect to `build.bat`, so the top-level
launchers cannot create a separate desktop control window.

## Install into RingCentral (Windows)

1. Close RingCentral so its next launch reads the patched app bundle.
2. Double-click **`build.bat`** from this repository.
   - It automatically looks in common RingCentral install locations.
   - Or use PowerShell with an explicit source:

   ```powershell
   .\ringcentral-patch\apply-patch.ps1 -Source C:\path\to\RingCentral.zip
   # source may also be an unpacked RingCentral folder or resources\app.asar
   ```

3. The script extracts `resources\app.asar`, makes an `app.asar.bak` backup,
   installs the in-app engine, repacks the archive, and launches RingCentral.
4. Open RingCentral's **dialer**. The `🌶 SPICY ON/OFF` button appears beside
   its Call control. Open its Settings menu for the Auto-Answer and Pin entries.

If no `app.asar` is found, the script changes nothing and `build.bat` opens
`docs/INAPP_GUIDE.html` with the source-location options.

## In-app controls

| RingCentral location | Control | Result |
|---|---|---|
| Dialer action row | `🌶 SPICY ON/OFF` | Enables/disables auto-answer in this RingCentral app. |
| Settings menu | `Spicy Lamar — Auto-Answer [ON/OFF]` | Same in-app auto-answer state. |
| Settings menu | `Pin RingCentral on top [ON/OFF]` | Pins/unpins only RingCentral's Electron window. |

The renderer exposes these **RingCentral-document-only** DevTools helpers when
selector tuning is needed:

```js
window.__spicyInject()          // rerun in-app UI insertion
window.__spicySet('autoAnswer', true)
window.__spicySet('pinned', false)
window.__spicySendDtmf('5')     // uses RC's visible keypad/field only
window.__spicyPlaceCall('5551234') // uses RC's field + Call button only
```

## Files

```text
ringcentral-patch/
├── apply-patch.ps1                 # extract → inject → repack → launch
├── spicy-engine/
│   ├── spicy-main.js                # RC BrowserWindow-only pin + injector
│   ├── spicy-preload.js             # narrow settings bridge
│   ├── spicy-renderer.js            # dialer/menu UI + in-RC auto-answer
│   └── spicy-config.js              # version-specific selectors and timing
└── styles/spicy-button.css          # styles isolated to #spicy-lamar-* IDs
```

## If a control does not appear

RingCentral's proprietary DOM changes between versions. Open RingCentral's
DevTools, inspect its console for `[SpicyLamar]`, and tune selectors in
`ringcentral-patch/spicy-engine/spicy-config.js`. The renderer is idempotent:
re-running the patch or `window.__spicyInject()` will not duplicate controls.

## Restore

To restore the original app bundle, close RingCentral and replace
`resources\app.asar` with `resources\app.asar.bak` created by the patcher.
