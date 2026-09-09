/**
 * Compatibility loader for manual RingCentral patches.
 *
 * Prefer apply-patch.ps1: it installs the complete in-app engine through the
 * Electron main process. This small loader exists only for older manual setups
 * and loads the same renderer-local engine. It never sends global keyboard
 * input, opens a popup, or communicates with another desktop application.
 */
(function () {
  'use strict';
  if (window.__spicyLamarRendererLoaded) return;

  function load(source, done) {
    var script = document.createElement('script');
    script.src = source;
    script.onload = done || function () {};
    script.onerror = function () {
      try { console.warn('[SpicyLamar] could not load ' + source); } catch (ignore) {}
    };
    (document.head || document.documentElement).appendChild(script);
  }

  // Both files are part of the app.asar patch and execute in RingCentral's
  // existing renderer document.
  load('./spicy-engine/spicy-config.js', function () {
    load('./spicy-engine/spicy-renderer.js');
  });
})();
