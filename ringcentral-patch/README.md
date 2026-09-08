# RingCentral Phone — Put Spicy Lamar INSIDE the app (and launch it)

> **Updated (v1.0 in-app):** this is now the primary build. Running **`build.bat`** (repo root) calls
> `apply-patch.ps1`, which unpacks `resources/app.asar`, installs the **full in-app engine**
> (`spicy-engine/`), repacks, and **launches the patched RingCentral** — so `build.bat` actually opens
> something. The full feature (🌶 SPICY dialer button + ⚙ menu entry + auto-answer + DTMF + pin)
> now lives inside RingCentral's own app — no separate `SpicyLamar.exe`, no popup.

Everything below the old files (`inject-spicy-button.js`, `patch.diff`, etc.) is kept for reference and
as a fallback manual-patch route; the recommended path is `spicy-engine/` + `apply-patch.ps1`.
See **`spicy-engine/README.md`** for how the in-app modules are wired.

---

This patch adds a **“🌶 Spicy Lamar”** button directly inside the RingCentral Phone dialer UI you showed in `image-1.png` / `image-2.png`.

> You sent `RingCentral.zip` (≈456 MB, Git LFS). That zip is currently a pointer (object not on server, `404 Object does not exist`), so this folder contains the **exact files to adjust** once you have the unpacked app locally. Apply the patch and re-pack — no separate popup window needed.

## Where is the dialer UI?

After you unzip `RingCentral.zip` locally you will see:

```
RingCentral/
├── RingCentral.exe              (Electron shell, Chrome_WidgetWin_1)
├── resources/
│   ├── app.asar                 ← ALL UI IS IN HERE (bundled HTML/JS/CSS)
│   ├── app.asar.unpacked/       (native modules)
│   └── ...
└── locales/ ...
```

**All dialer HTML/JS is inside `resources/app.asar`.** You must unpack it:

```powershell
# 1. Install asar tool once
npm i -g asar

# 2. Unpack (from the RingCentral install folder)
asar extract resources/app.asar app-src
```

Inside `app-src` you will find one of these paths (depending on RingCentral version):

| Version | File that renders the screenshots |
|---------|-----------------------------------|
| New Electron (Phone 24.x) | `app-src/renderer/modules/dialer/DialerView.tsx` <br> or `app-src/build/renderer.js` containing string `Enter a name or number` |
| Classic (Glip/RingCentral) | `app-src/src/modules/Phone/components/DialerPanel/index.tsx` |
| Minified bundle | `app-src/dist/renderer.bundle.js` — search for `Reattach keypad` |

Quick way to locate it:

```powershell
Select-String -Path app-src -Pattern "Enter a name or number" -Recurse
Select-String -Path app-src -Pattern "Reattach keypad" -Recurse
Select-String -Path app-src -Pattern "My caller ID" -Recurse
```

That file is the **only file you need to edit** to add your button.

## What to adjust — 3 insertion points (matches your screenshots)

### 1. Settings dropdown menu — image-1.png
File: the component that renders the menu you screenshotted:

```tsx
// BEFORE (excerpt from DialerMenu.tsx)
<MenuItem onClick={onReattach}>Reattach keypad <ExternalIcon/></MenuItem>
<MenuItem><Toggle checked={pinOnTop}/> Pin window on top</MenuItem>
<MenuItem>Emergency address confirmation</MenuItem>
<MenuItem><Toggle checked={ringOut}/> RingOut</MenuItem>
<MenuItem>Incoming call rules</MenuItem>
<MenuItem>Voicemail greeting</MenuItem>
<MenuItem>Phone settings</MenuItem>
```

**Add after `Phone settings`:**

```tsx
// PATCH — add Spicy Lamar menu item (copy/paste)
<MenuItem onClick={onSpicyLamar} style={{color:'#FF3300'}}>
  🌶 Spicy Lamar — Auto-Answer {spicyEnabled ? '[ON]' : '[OFF]'}
</MenuItem>
```

The handler `onSpicyLamar` toggles the same engine our integrated `SpicyLamar.exe` uses (see `inject-spicy-button.js`).

### 2. Keypad panel + CALL bar — image-2.png
File: same Dialer component, near the green CALL button (`Noteson` + green phone icon).

```tsx
// BEFORE
<div className="dialer-keypad">
  {/* 1 2 3 / 4 5 6 / 7 8 9 / * 0 #  with subs ABC DEF...  */}
</div>
<Button className="call-btn" onClick={onCall}> {/* green circle */} </Button>
<Button className="notes-btn">Noteson</Button>

// AFTER — add Spicy button between Noteson and CALL
<div style={{display:'flex', gap:12, marginTop:12}}>
  <Button className="notes-btn">Noteson</Button>
  <Button onClick={onSpicyQuickDial}
          style={{
            background:'#1A1A1A', border:'1px solid #303030',
            color:'#FF3300', fontWeight:'bold', flex:1
          }}>
    🌶 SPICY
  </Button>
  <Button className="call-btn" style={{background:'#00963C'}} onClick={onCall}/>
</div>
```

Preview with patch applied: see `../docs/mockup-ringcentral-patched.png` (generated below — same dialer but with orange 🌶 button).

### 3. Global injection (no rebuild needed) — use SpicyLamar.exe to overlay
If you don't want to rebuild `app.asar`, our integrated `SpicyLamar.exe` can **host** RingCentral's `Chrome_RenderWidgetHostHWND` via `SetParent` and overlay a WPF/Win32 button on top — see `inject-spicy-button.js` + `spicy-overlay-host.js`. This is the `SpicyLamar-Integrated` approach (single window, no `RingCentral.exe` popup).

## Files in this folder

| File | Purpose |
|------|---------|
| `inject-spicy-button.js` | Drop-in JS: `require('./inject-spicy-button')` in renderer `index.js` — finds `Enter a name or number` field and injects the 🌶 button via DOM, no recompilation of C++ needed |
| `styles/spicy-button.css` | Chili palette `#FF3300` / hover `#FF5533`, matches RingCentral dark theme + our integrated palette |
| `electron-main-patch.js` | Patch for Electron `main.js` / `background.js` to allow `executeJavaScript` injection and `SetWindowPos` TOPMOST sync (mirrors our `Pin window on top`) |
| `patch.diff` | Unified diff you can `git apply` inside `app-src` |
| `apply-patch.ps1` | One-click PowerShell: unpacks `app.asar`, applies patch, repacks |

## One-click apply + launch (Windows, with your local RingCentral)

```powershell
# Explicit source (zip / unpacked app folder / app.asar):
.\ringcentral-patch\apply-patch.ps1 -Source .\RingCentral.zip

# Or just run it — auto-detects RingCentral installed under %LocalAppData%\RingCentral:
.\ringcentral-patch\apply-patch.ps1
# (build.bat at the repo root does the same and opens docs\INAPP_GUIDE.html if nothing is found.)

# What the script does:
#  1. Locates an app.asar (zip / folder / installed app)
#  2. asar extract → copies ringcentral-patch/spicy-engine/ in
#  3. Wires the renderer entry (index.html) + main process hook
#  4. asar pack (keeps app.asar.bak backup)
#  5. LAUNCHES the patched RingCentral
```

## After patch — what you get

- No `The keypad is now open in a new window.` flow is needed — Spicy button lives **inside** the existing dialer.
- Clicking `Reattach keypad` still works (now just ensures panel visible, `Dock=Right`, logs `Keypad reattached inside dashboard (no popup).`).
- `Pin window on top` now syncs to `WS_EX_TOPMOST` (our `SetWindowPos` toggle) and to the dropdown's `[ON]` state — default ON as in your screenshot.
- Pressing 🌶 SPICY fires the same 6-shot `Alt+F1` cascade + `SendDtmf` as our standalone `SpicyLamar.exe` (200 Hz poll, `CollectRingCentralWindows` title OR `ringcentral/glip/rcdesktop` process fallback).

## Need us to do it for you?

Re-upload the **actual unpacked** `resources/app.asar` (or the full `app-src` folder) — the current `RingCentral.zip` on GitHub is an LFS pointer without object (`404` on fetch, size 456,534,760, oid `ada5422…`). Once we have 10–50 MB of the unpacked `app-src` we can directly patch and return a patched `app.asar` you can drop in.

---
*Generated for SpicyLamar-Integrated v1.0 — single window 980×620, TURBO 200Hz, 6-shot cascade*
