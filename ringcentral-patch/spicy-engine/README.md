# Spicy Lamar in-RingCentral engine

These modules run *inside the patched RingCentral Electron application*. They
are not a standalone dialer or a desktop automation helper.

| File | Runs in | Responsibility |
|---|---|---|
| `spicy-config.js` | RingCentral main + renderer | RC-version-specific selectors, timing, colours, defaults. |
| `spicy-main.js` | RingCentral Electron main | Injects after RC renderer loads; persists state; pins only the requesting RC BrowserWindow. |
| `spicy-preload.js` | Existing RC preload | Tiny `window.spicyLamar` state bridge. No arbitrary IPC, input, shell, or filesystem API. |
| `spicy-renderer.js` | RingCentral dialer | Inserts the button/menu controls, auto-answers through RC's visible Answer control. |

## Safety boundary

The engine has no global input or desktop automation path:

- It does not create a BrowserWindow, run an EXE, register a global shortcut,
  call `SendInput`, issue Alt+F1, or move focus.
- The renderer only clicks/query-controls in its own RingCentral document.
- It only auto-clicks Answer when both a visible incoming-call surface and a
  visible Answer/Accept control are present.
- Its Pin command is sent through the preload bridge to `spicy-main.js`, which
  calls `setAlwaysOnTop` on the relevant RingCentral BrowserWindow only.

## Install

Run `..\apply-patch.ps1` (or the root `build.bat`) with an installed
RingCentral app, app folder, zip, or `resources\app.asar`. The patcher copies
this folder into `app.asar`, discovers the main and preload files, repacks the
archive, and starts RingCentral.

## Selector tuning

RingCentral DOM details vary across releases. If an in-app control does not
appear, use RingCentral DevTools and read `[SpicyLamar]` console messages. Tune
`spicy-config.js`, rerun the patcher, then restart RingCentral. The renderer's
idempotency guard prevents duplication when both the main injector and a
compatibility `index.html` tag run.
