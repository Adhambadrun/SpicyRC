/**
 * Spicy Lamar main-process integration for RingCentral.
 *
 * The main side owns the only operating-system action: setAlwaysOnTop is sent
 * exclusively to the BrowserWindow that is running RingCentral. Renderer
 * actions are injected only into RingCentral's own document; no global input,
 * hotkeys, external helper process, or other desktop window is touched.
 */
(function () {
  'use strict';
  if (typeof require === 'undefined') return;

  var electron;
  try { electron = require('electron'); } catch (error) { return; }
  var app = electron.app;
  var BrowserWindow = electron.BrowserWindow;
  var ipcMain = electron.ipcMain;
  if (!app || !BrowserWindow || !ipcMain) return;

  var fs = require('fs');
  var path = require('path');
  var cfg = require('./spicy-config.js');
  var CHANNEL = 'spicy-lamar';
  var attachedWindow = null;
  var knownWindows = [];
  var installedContents = [];
  var configSource = null;
  var rendererSource = null;
  var rendererCss = null;
  var state = {
    autoAnswer: !cfg.autoAnswer || cfg.autoAnswer.enabledOnStartup !== false,
    pinned: cfg.pinOnTopDefault !== false
  };

  function log(message) {
    try { console.log('[SpicyLamar:main] ' + message); } catch (ignore) {}
  }

  function warn(message) {
    try { console.warn('[SpicyLamar:main] ' + message); } catch (ignore) {}
  }

  function uniquePush(list, value) {
    if (list.indexOf(value) === -1) list.push(value);
  }

  function settingsFile() {
    try { return path.join(app.getPath('userData'), 'spicy-lamar.json'); } catch (ignore) { return null; }
  }

  function loadState() {
    var file = settingsFile();
    if (!file) return;
    try {
      var saved = JSON.parse(fs.readFileSync(file, 'utf8'));
      if (typeof saved.autoAnswer === 'boolean') state.autoAnswer = saved.autoAnswer;
      if (typeof saved.pinned === 'boolean') state.pinned = saved.pinned;
    } catch (ignore) {}
  }

  function saveState() {
    var file = settingsFile();
    if (!file) return;
    try {
      fs.mkdirSync(path.dirname(file), { recursive: true });
      fs.writeFileSync(file, JSON.stringify({ autoAnswer: state.autoAnswer, pinned: state.pinned }, null, 2), 'utf8');
    } catch (error) {
      warn('could not save settings: ' + error.message);
    }
  }

  function snapshot() {
    return { autoAnswer: !!state.autoAnswer, pinned: !!state.pinned };
  }

  function isDestroyed(win) {
    try { return !win || win.isDestroyed(); } catch (ignore) { return true; }
  }

  function windowUrl(win) {
    try { return (win.webContents && win.webContents.getURL()) || ''; } catch (ignore) { return ''; }
  }

  function isRingCentralWindow(win) {
    if (isDestroyed(win)) return false;
    var url = windowUrl(win);
    var title = '';
    try { title = win.getTitle() || ''; } catch (ignore) {}
    // app:// and file:// are normal packaged RingCentral renderer locations.
    // Explicitly exclude DevTools windows even though they live in the same app.
    if (/devtools:\/\//i.test(url) || /devtools/i.test(title)) return false;
    return /^(file|app|ringcentral|rc):/i.test(url) ||
      /ringcentral|glip|ringme/i.test(url) ||
      /ringcentral|phone|dialer|glip/i.test(title);
  }

  function isInjectableUrl(url) {
    // Never inject into third-party auth/browser pages. A RingCentral packaged
    // page uses file/app/custom schemes or a RingCentral/Glip origin.
    return /^(file|app|ringcentral|rc):/i.test(url || '') ||
      /^https?:\/\/([^/]*\.)?(ringcentral|glip|ringme)\.com(?:[:/]|$)/i.test(url || '');
  }

  function applyPin(win) {
    if (isDestroyed(win) || !isRingCentralWindow(win)) return;
    try {
      win.setAlwaysOnTop(!!state.pinned, 'floating');
      log('Pin RingCentral window ' + (state.pinned ? 'ON' : 'OFF'));
    } catch (error) {
      warn('could not change RingCentral pin: ' + error.message);
    }
  }

  function broadcastState() {
    for (var i = knownWindows.length - 1; i >= 0; i--) {
      var win = knownWindows[i];
      if (isDestroyed(win)) {
        knownWindows.splice(i, 1);
        continue;
      }
      try { win.webContents.send(CHANNEL + ':state', snapshot()); } catch (ignore) {}
    }
  }

  function loadRendererAssets() {
    if (rendererSource !== null) return;
    try { configSource = fs.readFileSync(path.join(__dirname, 'spicy-config.js'), 'utf8'); }
    catch (error) { configSource = ''; warn('config asset missing: ' + error.message); }
    try { rendererSource = fs.readFileSync(path.join(__dirname, 'spicy-renderer.js'), 'utf8'); }
    catch (error) { rendererSource = ''; warn('renderer asset missing: ' + error.message); }
    try { rendererCss = fs.readFileSync(path.join(__dirname, '..', 'styles', 'spicy-button.css'), 'utf8'); }
    catch (ignore) { rendererCss = ''; }
  }

  function injectRenderer(win) {
    if (isDestroyed(win) || !win.webContents) return;
    var contents = win.webContents;
    if (installedContents.indexOf(contents.id) !== -1) return;
    uniquePush(installedContents, contents.id);
    var stateSyncTimer = null;

    function syncRendererState() {
      if (isDestroyed(win) || !isInjectableUrl(windowUrl(win))) return;
      // The preload bridge normally delivers changes immediately. This small
      // renderer-local fallback keeps Pin functional on RC builds whose
      // preload is bundled too deeply to patch. It reads only this feature's
      // two booleans from this RC document; it cannot observe the desktop.
      var readState = '(function(){try{return localStorage.getItem("spicy-lamar.state.v2");}catch(e){return null;}})()';
      try {
        Promise.resolve(contents.executeJavaScript(readState, true)).then(function (raw) {
          if (!raw || typeof raw !== 'string') return;
          var next;
          try { next = JSON.parse(raw); } catch (ignore) { return; }
          var changed = false;
          if (typeof next.autoAnswer === 'boolean' && next.autoAnswer !== state.autoAnswer) {
            state.autoAnswer = next.autoAnswer;
            changed = true;
          }
          if (typeof next.pinned === 'boolean' && next.pinned !== state.pinned) {
            state.pinned = next.pinned;
            applyPin(win);
            changed = true;
          }
          if (changed) {
            saveState();
            broadcastState();
          }
        }).catch(function () {});
      } catch (ignore) {}
    }

    function beginStateSync() {
      if (stateSyncTimer) clearInterval(stateSyncTimer);
      // Let the renderer load persisted state before the first fallback read.
      setTimeout(syncRendererState, 750);
      stateSyncTimer = setInterval(syncRendererState, 1000);
    }

    function runInjection() {
      if (isDestroyed(win)) return;
      var url = windowUrl(win);
      if (!isInjectableUrl(url)) return;
      loadRendererAssets();
      if (!rendererSource) return;

      // executeJavaScript runs within the current RC renderer document. The
      // renderer has an idempotency guard, so dom-ready/did-finish-load and an
      // optional index.html tag cannot produce duplicate UI or timers.
      var source = '';
      if (rendererCss) {
        source += '(function(){if(!document.getElementById("spicy-lamar-style")){var s=document.createElement("style");s.id="spicy-lamar-style";s.textContent=' + JSON.stringify(rendererCss) + ';(document.head||document.documentElement).appendChild(s);}})();\n';
      }
      source += '\n' + (configSource || '') + '\n' + rendererSource + '\n//# sourceURL=spicy-lamar-renderer.js';
      try {
        Promise.resolve(contents.executeJavaScript(source, true)).catch(function (error) {
          warn('renderer injection failed: ' + error.message);
        });
      } catch (error) {
        warn('renderer injection could not start: ' + error.message);
      }
    }

    contents.on('did-finish-load', function () {
      runInjection();
      beginStateSync();
    });
    contents.on('dom-ready', runInjection);
    contents.once('destroyed', function () {
      if (stateSyncTimer) clearInterval(stateSyncTimer);
      var index = installedContents.indexOf(contents.id);
      if (index !== -1) installedContents.splice(index, 1);
    });
    runInjection();
    beginStateSync();
  }

  function attach(win) {
    if (!isRingCentralWindow(win)) return false;
    uniquePush(knownWindows, win);
    attachedWindow = win;
    applyPin(win);
    injectRenderer(win);
    return true;
  }

  function attachBestWindow() {
    var windows = [];
    try { windows = BrowserWindow.getAllWindows(); } catch (ignore) {}
    for (var i = 0; i < windows.length; i++) {
      if (attach(windows[i])) return windows[i];
    }
    return null;
  }

  function senderWindow(event) {
    try {
      var win = BrowserWindow.fromWebContents(event.sender);
      return isRingCentralWindow(win) ? win : null;
    } catch (ignore) { return null; }
  }

  // This module can be required before app is ready. Read persisted state only
  // when Electron's userData directory is available, then apply it to RC.
  function start() {
    loadState();
    attachBestWindow();
    try {
      app.on('browser-window-created', function (_event, win) {
        // Attach after the initial navigation provides a useful title/URL.
        win.once('ready-to-show', function () { attach(win); });
        win.on('focus', function () { attach(win); });
        injectRenderer(win);
      });
      app.on('browser-window-focus', function (_event, win) { attach(win); });
    } catch (error) {
      warn('window lifecycle hook failed: ' + error.message);
    }
    log('main integration ready; actions are limited to RingCentral BrowserWindows');
  }

  // Renderer -> main. Every handler resolves the sender's BrowserWindow and
  // changes that RC window only. There is no global desktop side effect.
  ipcMain.on(CHANNEL + ':getState', function (event) {
    event.returnValue = snapshot();
  });
  ipcMain.on(CHANNEL + ':setAutoAnswer', function (_event, enabled) {
    state.autoAnswer = !!enabled;
    saveState();
    broadcastState();
    log('auto-answer ' + (state.autoAnswer ? 'ON' : 'OFF'));
  });
  ipcMain.on(CHANNEL + ':setPinned', function (event, enabled) {
    state.pinned = !!enabled;
    var win = senderWindow(event) || attachedWindow || attachBestWindow();
    if (win) applyPin(win);
    saveState();
    broadcastState();
  });

  // A compatibility notification is intentionally informational. In
  // particular, auto-answer changes can never accidentally unpin RC.
  ipcMain.on(CHANNEL + ':notify', function (_event, payload) {
    log('renderer notification: ' + ((payload && payload.name) || 'event'));
  });

  module.exports = {
    attach: attach,
    attachBestWindow: attachBestWindow,
    getState: snapshot,
    setPinned: function (enabled) {
      state.pinned = !!enabled;
      if (attachedWindow) applyPin(attachedWindow);
      saveState();
      broadcastState();
      return state.pinned;
    },
    setAutoAnswer: function (enabled) {
      state.autoAnswer = !!enabled;
      saveState();
      broadcastState();
      return state.autoAnswer;
    }
  };

  if (app.isReady && app.isReady()) start();
  else app.once('ready', start);
})();
