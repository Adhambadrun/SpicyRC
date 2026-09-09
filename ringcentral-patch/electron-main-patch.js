/**
 * Manual Electron main-process hook (compatibility reference).
 *
 * The automatic patcher appends the equivalent guarded require to RingCentral's
 * declared main entry:
 *
 *   try { require('./spicy-engine/spicy-main.js'); } catch (error) {
 *     console.warn('[SpicyLamar] main integration hook failed', error);
 *   }
 *
 * spicy-main.js injects the renderer only into RingCentral pages and exposes
 * a narrow preload bridge. Its only native window action is
 * BrowserWindow.setAlwaysOnTop on the RingCentral BrowserWindow that sent a
 * Pin request. It does not use global shortcuts, synthetic desktop input, external helpers,
 * or a separate desktop window.
 */

module.exports = {};
