/**
 * electron-main-patch.js
 * Patch for RingCentral Electron main process (main.js / background.js)
 * Adds IPC handler for spicy-lamar:toggle and exposes window.spicyLamar via preload.
 *
 * APPLY:
 *   In app-src/main.js (or background.js) near other ipcMain handlers, add:
 *
 *     const { ipcMain } = require('electron');
 *     // --- SpicyLamar bridge ---
 *     ipcMain.on('spicy-lamar:toggle', (event, arg) => {
 *       // Option A: forward to your SpicyLamar.exe via named pipe / window message
 *       // Option B: just log and let the renderer synthesize Alt+F1
 *       console.log('[SpicyLamar] toggle', arg);
 *       // If you host RingCentral inside SpicyLamar via SetParent, you can
 *       // call native addon here to SendInput Alt+F1
 *       try { require('./spicy-native').fireAltF1(); } catch {}
 *     });
 *
 *   In app-src/preload.js (or where contextBridge is), add:
 *
 *     const { contextBridge, ipcRenderer } = require('electron');
 *     contextBridge.exposeInMainWorld('spicyLamar', {
 *       tryAnswer: () => ipcRenderer.send('spicy-lamar:toggle', { chan: 4 }),
 *       isPinned: () => ipcRenderer.sendSync('spicy-lamar:isPinned'),
 *       setPinned: (v) => ipcRenderer.send('spicy-lamar:setPinned', v)
 *     });
 *
 * This mirrors our C++ App::SetTopmost() + Engine::TryAnswer(CHAN_KEYPAD)
 * so Pin window on top syncs to WS_EX_TOPMOST.
 */

// Example native addon stub (spicy-native.node) — optional
// If you compile SpicyLamar's Engine as a Node addon, expose fireAltF1() here.
// For pure JS injection, you don't need this — the renderer will just
// dispatch KeyboardEvent and our C++ WindowCache will catch it via Poll (5ms TURBO).

module.exports = {
  // keep empty — this file is documentation; copy the snippets above into your main.js/preload.js
};
