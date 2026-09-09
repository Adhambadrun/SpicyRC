# Spicy Lamar inside RingCentral

`apply-patch.ps1` installs Spicy Lamar into **RingCentral's existing Electron
application**. The result is a button in RingCentral's dialer and two controls
in RingCentral's Settings menu—not a standalone Spicy Lamar window.

## Run

```powershell
# Auto-detect a common installed RingCentral location
.\ringcentral-patch\apply-patch.ps1

# Or provide the installed app folder, a RingCentral.zip, or app.asar directly
.\ringcentral-patch\apply-patch.ps1 -Source C:\path\to\RingCentral.zip
```

The script:

1. finds `resources\app.asar`;
2. extracts it with `@electron/asar` (installed automatically if necessary);
3. copies `spicy-engine/` into the app;
4. requires `spicy-main.js` from the app's declared Electron main entry;
5. adds `spicy-preload.js` to discovered existing preload files where possible;
6. repacks the app after making a one-time `app.asar.bak`; and
7. launches RingCentral unless `-SkipLaunch` was supplied.

It is safe to rerun: marker comments prevent duplicate main/preload hooks and
the renderer has its own per-document idempotency guard.

## Scope of the integration

The code is deliberately bounded to RingCentral:

- The renderer engine is injected only into packaged RingCentral pages or a
  RingCentral/Glip/RingMe origin, never third-party auth pages.
- It inserts `🌶 SPICY ON/OFF` next to RingCentral's own Call button.
- The Settings menu gains in-app **Auto-Answer** and **Pin RingCentral on top**
  items.
- Auto-answer only clicks a visible RC Answer/Accept control while an incoming
  RC call surface is visible.
- DTMF/call helpers use controls in the same renderer document.
- The sole window operation is Electron `setAlwaysOnTop` on the RingCentral
  BrowserWindow which sent the request.

There are no global hotkeys, OS-level keyboard injection, focus changes,
separate helper EXEs, or pop-up windows in this patch path.

## Module map

| Module | Scope | Purpose |
|---|---|---|
| `spicy-config.js` | RC main + renderer | Selectors, timing, and defaults. |
| `spicy-main.js` | RingCentral main process | Reliable renderer injection, persisted state, RC-window-only pin. |
| `spicy-preload.js` | Existing RC preload | Narrow state bridge (`getState`, `setAutoAnswer`, `setPinned`). |
| `spicy-renderer.js` | RingCentral dialer renderer | UI insertion and RC-local auto-answer/DTMF helpers. |

## Version tuning

RingCentral's DOM is proprietary and varies by release. If the dialer button
does not show after the patch, open RingCentral DevTools and inspect
`[SpicyLamar]` console messages. Update the selectors in
`spicy-engine/spicy-config.js`, rerun the patch, and restart RingCentral.

## Restore

Close RingCentral and replace `resources\app.asar` with the generated
`resources\app.asar.bak` backup.
