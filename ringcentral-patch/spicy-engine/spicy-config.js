/**
 * Spicy Lamar configuration for the RingCentral renderer.
 *
 * These selectors intentionally target controls that already belong to the
 * RingCentral window. The engine never installs a system-wide keyboard hook,
 * sends global input, or opens another window.
 */
(function (root, factory) {
  if (typeof module !== 'undefined' && module.exports) module.exports = factory();
  else root.spicyConfig = factory();
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  return {
    autoAnswer: {
      enabledOnStartup: true,
      pollIntervalMs: 250,
      answerFiredCooldownMs: 2000,
      dtmfDigitDelayMs: 80
    },

    // This is applied only to the RingCentral BrowserWindow that hosts the
    // patched renderer. It never changes another application window.
    pinOnTopDefault: true,

    // Incoming-call controls. Keep these specific: the renderer will only
    // click a visible Answer/Accept button when an incoming surface is visible.
    answerSelectors: [
      'button[aria-label="Answer"]',
      'button[aria-label="Accept call"]',
      'button[aria-label*="Answer call"]',
      '[data-testid="answer-call"]',
      '[data-testid*="answer"] button',
      '[data-test-id*="answer"] button',
      '[class*="incoming"] button[class*="answer"]',
      '[class*="incoming"] button[class*="accept"]'
    ],
    incomingCallSelectors: [
      '[data-testid*="incoming-call"]',
      '[data-test-id*="incoming-call"]',
      '[aria-label*="Incoming call"]',
      '[class*="incoming-call"]',
      '[class*="incomingCall"]',
      '[class*="call-incoming"]'
    ],

    dialFieldSelectors: [
      'input[placeholder*="Enter a name or number"]',
      'input[placeholder*="Enter name"]',
      'input[type="tel"]',
      'input[inputmode="tel"]'
    ],
    callButtonSelectors: [
      'button[aria-label="Call"]',
      'button[aria-label*="Make a call"]',
      '[data-testid="call-button"]',
      '[data-test-id="call-button"]',
      'button[class*="callButton"]',
      'button[class*="call-btn"]'
    ],

    // A selector may resolve to either the keypad container or one of its
    // keys. The renderer handles both shapes and clicks only the RC key whose
    // visible label exactly matches the requested DTMF digit.
    dtmfPadSelectors: [
      '[data-testid*="dtmf"]',
      '[data-test-id*="dtmf"]',
      '[class*="dtmf"]',
      '[class*="keypad"]',
      '[class*="dialpad"]'
    ],

    menuItemTextAnchor: 'Phone settings',
    palette: {
      chili: '#FF3300',
      neon: '#00FF66',
      panel: '#1A1A1A',
      border: '#303030',
      text: '#E6E6E6'
    },

    log: function (message) {
      try { console.log('[SpicyLamar] ' + message); } catch (ignore) {}
    },
    warn: function (message) {
      try { console.warn('[SpicyLamar] ' + message); } catch (ignore) {}
    }
  };
});
