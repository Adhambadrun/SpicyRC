/**
 * Narrow preload bridge for the in-RingCentral feature.
 *
 * It deliberately exposes no filesystem, shell, keyboard, or arbitrary IPC
 * access. The renderer can only change Spicy Lamar's two RC-local settings and
 * receive their current state.
 */
(function () {
  'use strict';

  try {
    var electron = require('electron');
    var contextBridge = electron.contextBridge;
    var ipcRenderer = electron.ipcRenderer;
    if (!contextBridge || !ipcRenderer) return;

    contextBridge.exposeInMainWorld('spicyLamar', {
      getState: function () {
        try { return ipcRenderer.sendSync('spicy-lamar:getState'); } catch (ignore) { return null; }
      },
      setAutoAnswer: function (enabled) {
        try { ipcRenderer.send('spicy-lamar:setAutoAnswer', !!enabled); } catch (ignore) {}
      },
      setPinned: function (enabled) {
        try { ipcRenderer.send('spicy-lamar:setPinned', !!enabled); } catch (ignore) {}
      },
      onState: function (callback) {
        if (typeof callback !== 'function') return;
        try {
          ipcRenderer.on('spicy-lamar:state', function (_event, next) {
            callback(next || null);
          });
        } catch (ignore) {}
      }
    });
  } catch (error) {
    try { console.warn('[SpicyLamar] preload bridge unavailable: ' + error.message); } catch (ignore) {}
  }
})();
