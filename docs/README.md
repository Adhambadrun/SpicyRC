# Documentation

The supported workflow is to put Spicy Lamar **inside RingCentral**:

```bat
build.bat
```

`build.bat` calls `ringcentral-patch\apply-patch.ps1`, which patches
RingCentral's `resources\app.asar`, keeps an `app.asar.bak` backup, and starts
RingCentral. See the root [README](../README.md) for full instructions.

## Behaviour

- The `🌶 SPICY ON/OFF` control is in RingCentral's dialer.
- Auto-Answer and Pin controls are in RingCentral's own Settings menu.
- The integration does not open another app/window, use global shortcuts, send
  OS-wide keyboard input, or control applications other than RingCentral.
- `Pin RingCentral on top` calls Electron's `setAlwaysOnTop` only for the
  RingCentral BrowserWindow that requested it.

`INAPP_GUIDE.html` opens automatically when the patcher cannot locate a
RingCentral app bundle. `INTEGRATED_PROMPT.md` and `INTEGRATION_LOG.txt` are
historical design records, not directions for the supported in-app workflow.
