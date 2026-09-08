# Spicy Lamar — in‑RingCentral engine (`spicy-engine/`)

This folder is the **in‑app** Spicy Lamar. Unlike the old standalone
`SpicyLamar.exe` (which opened its own 980×620 window and posted Alt+F1 to RC
from outside), these modules run **inside the RingCentral app itself** — no
separate window, no popup, no second process. The feature is "put onto the RC
app" exactly as requested.

## How it works (file map)

| File | Runs in | Role |
|------|---------|------|
| `spicy-config.js` | main + renderer | **One place to tune** RC-version-specific DOM selectors, timing and palette. |
| `spicy-renderer.js` | RC renderer (dialer) | Injects the **🌶 SPICY** button into RC's own dialer row + a **"Spicy Lamar — Auto-Answer"** item into the ⚙ menu. Auto-answers incoming calls by clicking RC's own Answer control; sends DTMF / places calls through RC's own keypad & dial field. |
| `spicy-preload.js` | RC preload | `contextBridge` exposing `window.spicyLamar` so the injected UI can talk to main. **Optional** — renderer engine still works standalone. |
| `spicy-main.js` | RC main process | Pin RC window on top (`setAlwaysOnTop`, in-app mirror of the old `WS_EX_TOPMOST`), persist the on/off toggle, forward events. Thin, fully guarded shim. |

The renderer `<script>`/`<link>` tags plus the main/preload wiring are added
automatically by `apply-patch.ps1` (not by a build JS helper), so there are no
extra files to keep in sync.

## Install / run

Everything is driven from **`build.bat`** at the repo root (or
`.\ringcentral-patch\apply-patch.ps1` directly). Point it at your RingCentral:

```
# Option A — a RingCentral.zip / unpacked folder you provide
.\ringcentral-patch\apply-patch.ps1 -Source C:\path\to\RingCentral.zip

# Option B — the app you already run (auto-detected under %LocalAppData%)
.\ringcentral-patch\apply-patch.ps1
```

The script unpacks `resources/app.asar`, copies these modules in, patches the
renderer entry + main + preload, repacks `app.asar`, then **launches the
patched RingCentral**. When you open the dialer you'll see the 🌶 SPICY button
and the ⚙ menu entry; incoming calls are answered automatically while Spicy
Lamar is ON.

## The one important caveat

RingCentral is proprietary and its DOM classes change across versions. The
selectors in `spicy-config.js` are **best‑effort defaults** (matching the dialer
screens you shared — `Enter a name or number`, green CALL, `Phone settings`
menu). We could not verify them here because the original `RingCentral.zip` in
the repo was a Git‑LFS pointer with no object on the server, so the real
`app.asar` was never available.

After the first launch:

1. Open the dialer, press **Ctrl+Shift+I** (DevTools → Console).
2. Read the `[SpicyLamar]` logs — they say exactly which selectors matched or failed.
3. In `spicy-config.js`, fix any selector that didn't match **your** RC build.
4. Re-run `build.bat`.

That is a 2‑minute tune against the real app and is the only step that can't be
pre‑done from this repo. Everything else (UI injection, menu, auto-answer loop,
pin, DTMF, dial) is generic and version tolerant.

## DevTools helpers (after injection)

```js
window.__spicyInject()            // re-run UI injection now
window.__spicySet('autoAnswer', false)  // toggle engine on/off
```
