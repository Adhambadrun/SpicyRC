/**
 * spicy-preload.js — contextBridge between RingCentral's renderer and main.
 * --------------------------------------------------------------------------
 * To use, require/inject this as a preload on the BrowserWindow that shows the
 * dialer (see spicy-main.js — it attaches it to the focused window). It exposes
 * a minimal, safe API on `window.spicyLamar` so the injected UI in
 * spicy-renderer.js can talk to the main process without touching Node.
 *
 * If RingCentral already uses its own contextBridge and we cannot add a second
 * preload without rebuilding, the renderer still works fully standalone — it
 * simply logs and skips the main-process calls (pin stays renderer-only best
 * effort). Nothing here is required for the auto-answer / DTMF UI to function.
 */

const { contextBridge, ipcRenderer } = require('electron');

try {
  contextBridge.exposeInMainWorld('spicyLamar', {
    // Renderer -> main notifications.
    notify: (eventName, payload) => {
      try { ipcRenderer.send('spicy-lamar:' + eventName, payload || {}); } catch (e) {}
    },
    // Renderer can query whether it should be pinned.
    isPinned: () => {
      try { return ipcRenderer.sendSync('spicy-lamar:isPinned'); } catch (e) { return true; }
    },
    // Ask main to pin/unpin the RC window (WS_EX_TOPMOST equivalent via Electron).
    setPinned: (on) => {
      try { ipcRenderer.send('spicy-lamar:setPinned', !!on); } catch (e) {}
    }
  });
} catch (e) {
  // contextBridge unavailable (e.g. running as a plain web page during dev) —
  // renderer engine continues without main-process pinning.
  try { console.warn('[SpicyLamar] preload bridge not available: ' + e.message); } catch (e2) {}
}
