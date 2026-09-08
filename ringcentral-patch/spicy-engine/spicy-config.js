/**
 * spicy-config.js — Spicy Lamar in-app engine configuration
 * -----------------------------------------------------------
 * This is the ONE place to tune selectors / behavior for the RingCentral
 * build that is installed on YOUR machine.
 *
 * RingCentral is a proprietary Electron app; its DOM classes change between
 * versions. The selectors below are best-effort defaults based on the dialer
 * screens you provided (search text "Enter a name or number", green CALL
 * button, "Reattach keypad" menu). If your RC version differs, open DevTools
 * (Ctrl+Shift+I on the dialer window) and update the selectors here — the
 * engine logs what it finds with a [SpicyLamar] prefix so you can tune it.
 *
 * Because the actual app.asar was not present in this repo (the original
 * RingCentral.zip was a Git-LFS pointer with no object), these cannot be
 * auto-verified here — they are meant to be confirmed once the real app is
 * patched and launched.
 */

// UMD so this file works in both Electron main and renderer contexts.
(function (root, factory) {
  if (typeof module !== 'undefined' && module.exports) module.exports = factory();
  else root.spicyConfig = factory();
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  return {
    // Auto-answer master behaviour.
    autoAnswer: {
      enabledOnStartup: true,      // Spicy Lamar defaults to ON (matches "Pin ON / Auto-answer active")
      pollIntervalMs: 250,         // how often we look for an incoming call surface
      answerFiredCooldownMs: 2000, // don't click "Answer" again within this window
      dtmfDigitDelayMs: 80         // pause between dialed digits so RC registers each one
    },

    // Window pin-on-top default (matches the dashboard "Pin window on top [ON]").
    pinOnTopDefault: true,

    // Where to find the green Answer / Accept control when a call comes in.
    // The engine clicks the first element that matches ANY of these, in order.
    answerSelectors: [
      'button[aria-label="Answer"]',
      'button[aria-label*="Accept"]',
      'button[class*="answer"]',
      '[data-testid*="answer"]',
      '[class*="accept-call"] button',
      '[class*="incoming"] button[class*="answer"], [class*="incoming"] button[class*="accept"]'
    ],

    // The "an incoming call is showing right now" hint element(s). Only when
    // one of these is visible do we attempt auto-answer.
    incomingCallSelectors: [
      '[class*="incoming-call"]',
      '[class*="call-card"]',
      '[aria-label*="Incoming call"]',
      '[class*="answer-queue"]'
    ],

    // Outbound/dialer field used for DTMF typing fallback.
    dialFieldSelectors: [
      'input[placeholder*="Enter a name or number"]',
      'input[placeholder*="Enter name"]',
      'input[type="tel"]',
      'input[placeholder*="number"]'
    ],

    // The green CALL button (used only for a "soft" answer fallback / to make an outbound call).
    callButtonSelectors: [
      'button[aria-label*="Call"]',
      'button[class*="call-btn"], button[class*="callButton"], button[class*="call"]'
    ],

    // DTMF pad that appears during an active call. Each digit key is matched
    // by its text content ("1","2",…,"#","*","+"). If none is found, digits
    // are typed into the dial field instead.
    dtmfPadSelectors: [
      '[class*="keypad"] button, [class*="dtmf"] button, [class*="dialer"] button'
    ],

    // Menu anchor used by the renderer to inject the Spicy menu item.
    menuItemTextAnchor: 'Phone settings',

    // UI chrome colours — reuse the Spicy palette.
    palette: {
      chili: '#FF3300',
      neon: '#00FF66',
      panel: '#1A1A1A',
      border: '#303030',
      text: '#E6E6E6'
    },

    // Logging helper available everywhere in this engine (does not depend on
    // RingCentral internals).
    log: function (msg) {
      try { console.log('[SpicyLamar] ' + msg); } catch (e) {}
    },
    warn: function (msg) {
      try { console.warn('[SpicyLamar] ' + msg); } catch (e) {}
    }
  };
});
