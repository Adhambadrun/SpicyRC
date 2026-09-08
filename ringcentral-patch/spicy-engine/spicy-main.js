/**
 * spicy-main.js — Spicy Lamar main-process side (loads inside RingCentral's main).
 * ---------------------------------------------------------------------------------
 * Responsibilities that must live in the Electron main process:
 *   • Pin the RC window on top (Electron BrowserWindow.setAlwaysOnTop, the
 *     in-app equivalent of the old WS_EX_TOPMOST native call).
 *   • Optionally forward an incoming-call "answer now" to the dialer window.
 *   • Persist the auto-answer on/off toggle so it survives window reloads.
 *
 * Wiring (see apply-patch.ps1): this file is appended/required from RingCentral's
 * main entry (main.js or background.js) and is given the BrowserWindow used for
 * the phone/dialer UI. If RC already created its windows before we load, we scan
 * for the window whose URL/title matches the phone UI and attach to it.
 *
 * IMPORTANT: we can NOT assume RingCentral exposes its call API, so this stays a
 * thin, safe shim — all feature logic lives in spicy-renderer.js (DOM based).
 * Every IPC handler is guarded with try/catch so a failed hook never breaks RC.
 */

(function () {
  'use strict';
  if (typeof require === 'undefined') return; // not in Electron main

  const { ipcMain, BrowserWindow } = require('electron');
  const cfg = require('./spicy-config.js');

  const CHAN = 'spicy-lamar';
  let pinned = cfg.pinOnTopDefault !== false;
  let attachedWindow = null;

  function log(m)  { try { console.log('[SpicyLamar:main] ' + m); } catch (e) {} }
  function warn(m) { try { console.warn('[SpicyLamar:main] ' + m); } catch (e) {} }

  function isPhoneWindow(win) {
    if (!win || win.isDestroyed()) return false;
    try {
      const u = (win.webContents && win.webContents.getURL()) || '';
      const t = win.getTitle ? win.getTitle() : '';
      return /ringcentral|glip|ringme/i.test(u) ||
             /call|phone|dialer/i.test(t) ||
             /dialer|phone/i.test(u);
    } catch (e) { return false; }
  }

  function attach(win) {
    if (!win || attachedWindow === win) return;
    attachedWindow = win;
    try { win.setAlwaysOnTop(pinned, 'floating'); } catch (e) {}
    log('attached to window "' + (win.getTitle ? win.getTitle() : '?') + '"');
  }

  function attachBestWindow() {
    try {
      const wins = BrowserWindow.getAllWindows();
      for (const w of wins) if (isPhoneWindow(w)) { attach(w); return; }
      // fallback: attach the most recently focused window
      const f = BrowserWindow.getFocusedWindow ? BrowserWindow.getFocusedWindow() : null;
      if (f && !f.isDestroyed()) attach(f);
    } catch (e) { warn('attachBestWindow: ' + e.message); }
  }

  function applyPin() {
    try {
      if (attachedWindow && !attachedWindow.isDestroyed()) {
        attachedWindow.setAlwaysOnTop(pinned, 'floating');
        log('Pin window on top ' + (pinned ? 'ON' : 'OFF'));
      }
    } catch (e) {}
  }

  // ---- IPC: renderer engine <-> main ----
  try {
    ipcMain.on(CHAN + ':toggle',   (e, p) => { pinned = !!p.pinned; applyPin(); cfg.log('toggle received'); });
    ipcMain.on(CHAN + ':setPinned',(e, v) => { pinned = !!v; applyPin(); });
    ipcMain.on(CHAN + ':answered', (e, p) => cfg.log('answered @ ' + (p && p.at)));
    ipcMain.on(CHAN + ':dtmf',     (e, p) => cfg.log('dtmf ' + (p && p.d)));
    ipcMain.on(CHAN + ':notify',   (e, p) => cfg.log('event ' + (p && p.name)));

    ipcMain.on(CHAN + ':isPinned', (e) => { e.returnValue = pinned; });
  } catch (e) { warn('ipc setup failed: ' + e.message); }

  // React to newly created / focused windows so pin follows the phone UI.
  try {
    BrowserWindow.on('focus', attachBestWindow);
  } catch (e) {}

  // Expose the installer for apply-patch.ps1 / manual integration: call
  //   require('./spicy-engine/spicy-main.js').attach(windowOrNothing)
  module.exports = {
    attach,
    attachBestWindow,
    isPinned: function () { return pinned; },
    setPinned: function (v) { pinned = !!v; applyPin(); return pinned; }
  };

  // Auto-attach shortly after load (windows may not exist yet).
  setTimeout(attachBestWindow, 1500);
})();
