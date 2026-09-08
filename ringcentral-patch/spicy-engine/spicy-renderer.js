/**
 * spicy-renderer.js — Spicy Lamar IN-APP feature (runs inside RingCentral's own renderer)
 * ---------------------------------------------------------------------------------------
 * This is the in-app replacement for the standalone SpicyLamar.exe. It does NOT
 * open a new window and does NOT post Alt+F1 to external windows. Instead it:
 *
 *   1. Injects a "🌶 SPICY" button into RingCentral's own dialer row and a
 *      "🌶 Spicy Lamar — Auto-Answer [ON/OFF]" entry into the ⚙ menu, inside
 *      the existing RC window.
 *   2. Auto-answers incoming calls by DOM-clicking RC's own Answer control
 *      (no separate helper process).
 *   3. Sends DTMF / places calls by driving RC's own keypad / dial field.
 *   4. Bridges to the main process (via window.spicyLamar, installed by
 *      spicy-preload.js) for pin-on-top and a central status line.
 *
 * Load it AFTER React mounts — the script self-injects and retries until the
 * dialer appears (RingCentral lazy-loads it), exactly like the previous
 * inject-spicy-button.js did, but with the full engine instead of a stub.
 *
 * NOTE: DOM selectors are RC-version specific — tune them in spicy-config.js.
 * Verify with the [SpicyLamar] console logs after launching.
 */

(function () {
  'use strict';

  // Load shared config (works when packed in app.asar next to spicy-engine/).
  // Falls back to inline defaults so the engine is self-contained even if the
  // config <script> was not loaded.
  var cfg = (typeof window !== 'undefined' && window.spicyConfig) ||
            (typeof module !== 'undefined' && module.require && module.require('./spicy-config.js')) || {
    autoAnswer: { enabledOnStartup: true, pollIntervalMs: 250, answerFiredCooldownMs: 2000, dtmfDigitDelayMs: 80 },
    pinOnTopDefault: true,
    answerSelectors: [
      'button[aria-label="Answer"]','button[aria-label*="Accept"]','button[class*="answer"]',
      '[data-testid*="answer"]','[class*="accept-call"] button',
      '[class*="incoming"] button[class*="answer"], [class*="incoming"] button[class*="accept"]'
    ],
    incomingCallSelectors: [
      '[class*="incoming-call"]','[class*="call-card"]','[aria-label*="Incoming call"]','[class*="answer-queue"]'
    ],
    dialFieldSelectors: [
      'input[placeholder*="Enter a name or number"]','input[placeholder*="Enter name"]','input[type="tel"]','input[placeholder*="number"]'
    ],
    callButtonSelectors: [
      'button[aria-label*="Call"]','button[class*="call-btn"], button[class*="callButton"], button[class*="call"]'
    ],
    dtmfPadSelectors: ['[class*="keypad"] button, [class*="dtmf"] button, [class*="dialer"] button'],
    menuItemTextAnchor: 'Phone settings',
    palette: { chili: '#FF3300', neon: '#00FF66', panel: '#1A1A1A', border: '#303030', text: '#E6E6E6' },
    log: function (m) { try { console.log('[SpicyLamar] ' + m); } catch (e) {} },
    warn: function (m) { try { console.warn('[SpicyLamar] ' + m); } catch (e) {} }
  };

  function log(m)   { try { console.log('[SpicyLamar] ' + m); } catch (e) {} }
  function warn(m)  { try { console.warn('[SpicyLamar] ' + m); } catch (e) {} }

  var BTN_ID  = 'spicy-lamar-btn';
  var MENU_ID = 'spicy-lamar-menu-item';
  var STATE   = {
    autoAnswer: cfg && cfg.autoAnswer ? cfg.autoAnswer.enabledOnStartup : true,
    pinned: cfg ? !!cfg.pinOnTopDefault : true,
    lastAnswerAt: 0
  };

  // ------------------------------------------------------------------ helpers

  function vis(el) {
    if (!el || el.nodeType !== 1) return false;
    var s = window.getComputedStyle(el);
    return s && s.display !== 'none' && s.visibility !== 'hidden' && el.offsetParent !== null;
  }

  function firstVisible(selectors, scope) {
    var root = scope || document;
    for (var i = 0; i < selectors.length; i++) {
      var s = selectors[i];
      var nodes;
      try { nodes = root.querySelectorAll(s); } catch (e) { nodes = []; }
      for (var j = 0; j < nodes.length; j++) {
        if (vis(nodes[j])) return nodes[j];
      }
    }
    return null;
  }

  function textFind(needle, scope) {
    var root = scope || document;
    var walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
    var n;
    while ((n = walker.nextNode())) {
      var t = (n.textContent || '').trim();
      if (t.indexOf(needle) !== -1) return n.parentElement;
    }
    return null;
  }

  function renderStatus() {
    var pill = document.getElementById('spicy-status');
    if (pill) {
      pill.textContent = STATE.autoAnswer ? '[ON]' : '[OFF]';
      pill.style.color = STATE.autoAnswer ? '#00FF66' : '#FF3300';
    }
    var btn = document.getElementById(BTN_ID);
    if (btn) {
      btn.style.background = STATE.autoAnswer ? '#FF3300' : '#1A1A1A';
      btn.style.color = STATE.autoAnswer ? '#FFFFFF' : '#FF3300';
      btn.textContent = '🌶 ' + (STATE.autoAnswer ? 'SPICY ON' : 'SPICY OFF');
    }
  }

  function flash(btn) {
    if (!btn) return;
    var old = btn.style.background;
    btn.style.background = '#00B450';
    setTimeout(function () { btn.style.background = old; renderStatus(); }, 200);
  }

  function notifyMain(eventName, payload) {
    try {
      if (window.spicyLamar && window.spicyLamar.notify) window.spicyLamar.notify(eventName, payload || {});
    } catch (e) {}
  }

  // ----------------------------------------------------------------- dtmf/call

  function pressDigitOnPad(digit) {
    var pad = firstVisible(cfg.dtmfPadSelectors);
    if (!pad) return null;
    // pad is the container element already matched; search its buttons for the digit text
    var btns = pad.querySelectorAll('button');
    for (var i = 0; i < btns.length; i++) {
      if ((btns[i].textContent || '').trim() === String(digit)) { btns[i].click(); return true; }
    }
    return null;
  }

  function sendDtmf(digit) {
    digit = String(digit);
    if (!/^[0-9*#+]$/.test(digit)) { warn('invalid DTMF ' + digit); return false; }
    // Prefer the real RC on-call DTMF pad; fall back to typing into the dial field.
    var padHit = pressDigitOnPad(digit);
    if (padHit) { log('DTMF \'' + digit + '\' sent via on-call pad'); notifyMain('dtmf', { d: digit }); return true; }
    var field = firstVisible(cfg.dialFieldSelectors);
    if (field) {
      field.value = (field.value || '') + digit;
      field.dispatchEvent(new Event('input', { bubbles: true }));
      field.dispatchEvent(new KeyboardEvent('keydown', { key: digit, bubbles: true }));
      field.focus();
      log('DTMF \'' + digit + '\' typed into dial field');
      notifyMain('dtmf', { d: digit });
      return true;
    }
    warn('no DTMF pad or dial field found for digit ' + digit);
    return false;
  }

  function placeCall(digits) {
    var s = String(digits);
    log('placing call with "' + s + '"');
    if (!s) return false;
    var field = firstVisible(cfg.dialFieldSelectors);
    if (field) {
      field.value = s;
      field.dispatchEvent(new Event('input', { bubbles: true }));
    }
    var callBtn = firstVisible(cfg.callButtonSelectors) ||
                  textFind('Call', document) ||
                  textFind('Noteson', document); // Noteson row hosts the green call circle
    if (callBtn && callBtn.tagName === 'BUTTON') callBtn.click();
    else if (callBtn) { var b = callBtn.querySelector('button'); if (b) b.click(); }
    log('CALL button pressed');
    return true;
  }

  // ---------------------------------------------------------------- auto-answer

  function autoAnswerTick() {
    if (!STATE.autoAnswer) return;
    var now = Date.now();
    if (now - STATE.lastAnswerAt < cfg.autoAnswer.answerFiredCooldownMs) return;

    // Only act when an incoming call surface is actually showing.
    var incoming = firstVisible(cfg.incomingCallSelectors);
    var answer = firstVisible(cfg.answerSelectors);
    if (!incoming && !answer) return;
    if (!answer) return; // incoming visible but no clickable answer yet

    answer.click();
    STATE.lastAnswerAt = now;
    log('ANSWERED incoming call (in-app click)');
    notifyMain('answered', { at: now });
    flash(document.getElementById(BTN_ID));
  }

  // ------------------------------------------------------------------ injection

  function injectKeypadButton() {
    if (document.getElementById(BTN_ID)) return true;

    var callBtn = firstVisible(cfg.callButtonSelectors) ||
                  textFind('Noteson', document);
    var anchor = null;
    if (callBtn) anchor = callBtn.parentElement || callBtn;
    else {
      var field = firstVisible(cfg.dialFieldSelectors);
      if (field) anchor = field.parentElement;
    }
    if (!anchor) { log('anchor not found for SPICY button'); return false; }

    var btn = document.createElement('button');
    btn.id = BTN_ID;
    btn.type = 'button';
    btn.title = 'Spicy Lamar — toggle auto-answer (left-click toggles)';
    btn.textContent = '🌶 SPICY';
    btn.style.cssText = [
      'background:#FF3300', 'color:#FFFFFF', 'border:1px solid #303030',
      'border-radius:8px', 'padding:10px 16px', 'font-family:"Segoe UI",sans-serif',
      'font-weight:700', 'font-size:13px', 'cursor:pointer',
      'display:inline-flex', 'align-items:center', 'justify-content:center',
      'min-width:96px', 'margin:0 8px', 'box-shadow:0 0 6px rgba(255,51,0,.35)'
    ].join(';');
    btn.addEventListener('click', function (e) {
      e.preventDefault(); e.stopPropagation();
      STATE.autoAnswer = !STATE.autoAnswer;
      log('Auto-answer ' + (STATE.autoAnswer ? 'ON' : 'OFF') + ' (clicked SPICY in dialer)');
      notifyMain('toggle', { on: STATE.autoAnswer });
      renderStatus();
    });
    btn.addEventListener('mouseenter', function () { btn.style.background = '#FF5533'; });
    btn.addEventListener('mouseleave', renderStatus);

    if (anchor.children && anchor.children.length >= 2) {
      // insert between Noteson and the green CALL button
      anchor.insertBefore(btn, anchor.children[1]);
    } else {
      anchor.appendChild(btn);
    }
    log('SPICY button injected into dialer');
    return true;
  }

  function injectMenuItem() {
    if (document.getElementById(MENU_ID)) return true;

    var anchor = textFind(cfg.menuItemTextAnchor || 'Phone settings', document);
    var menuRoot = anchor;
    if (menuRoot) {
      for (var i = 0; i < 6 && menuRoot; i++) {
        if (menuRoot.parentElement && (menuRoot.parentElement.getAttribute('role') === 'menu' ||
            menuRoot.parentElement.tagName === 'UL' || menuRoot.parentElement.querySelectorAll)) {
          var m = menuRoot.parentElement.getAttribute('role');
          if (m === 'menu' || menuRoot.parentElement.tagName === 'UL') { menuRoot = menuRoot.parentElement; break; }
        }
        menuRoot = menuRoot.parentElement;
      }
    }
    if (!menuRoot) { log('menu not visible yet (anchor text missing)'); return false; }

    var item = document.createElement('div');
    item.id = MENU_ID;
    item.setAttribute('role', 'menuitem');
    item.style.cssText = 'padding:10px 16px;cursor:pointer;display:flex;align-items:center;gap:8px;' +
      'border-top:1px solid #303030;margin-top:8px;font-family:"Segoe UI",sans-serif;font-size:13px;' +
      'color:#FF3300;user-select:none;';
    item.innerHTML = '<span>🌶</span><span style="flex:1">Spicy Lamar — Auto-Answer</span>' +
      '<span id="spicy-status" style="margin-left:auto;color:#00FF66;font-size:12px;">[ON]</span>';
    item.addEventListener('click', function () {
      STATE.autoAnswer = !STATE.autoAnswer;
      log('Auto-answer ' + (STATE.autoAnswer ? 'ON' : 'OFF') + ' (menu)');
      notifyMain('toggle', { on: STATE.autoAnswer });
      renderStatus();
    });
    item.addEventListener('mouseenter', function () { item.style.background = '#1A1A1A'; });
    item.addEventListener('mouseleave', function () { item.style.background = 'transparent'; });

    var phoneItem = textFind('Phone settings', menuRoot);
    if (phoneItem && phoneItem.parentElement) phoneItem.parentElement.insertBefore(item, phoneItem.nextSibling);
    else menuRoot.appendChild(item);
    log('SPICY menu item injected');
    return true;
  }

  function tryInject() {
    var a = injectKeypadButton();
    var b = injectMenuItem();
    return a || b;
  }

  // --------------------------------------------------------------- status badge

  function ensureStatusBadge() {
    // A small floating status pill in the corner of RC so the user can always
    // see whether Spicy Lamar is armed, even when the dialer is closed.
    if (document.getElementById('spicy-status-float')) return;
    if (!STATE.autoAnswer) return;
    var el = document.createElement('div');
    el.id = 'spicy-status-float';
    el.textContent = '🌶 SPICY LAMAR';
    el.style.cssText = 'position:fixed;right:12px;bottom:12px;z-index:2147483647;padding:6px 12px;' +
      'background:rgba(26,26,26,.92);color:#00FF66;border:1px solid #303030;border-radius:16px;' +
      'font:700 11px "Segoe UI",sans-serif;pointer-events:none;box-shadow:0 2px 8px rgba(0,0,0,.4);';
    document.body.appendChild(el);
  }

  // ------------------------------------------------------------------- boot

  function boot() {
    tryInject();
    ensureStatusBadge();
    renderStatus();
  }

  // Poll every 500ms for the dialer (RC lazy-loads), and on each tick run
  // auto-answer so we never miss an incoming call.
  var attempts = 0;
  var injectTimer = setInterval(function () {
    attempts++;
    boot();
    if (tryInject() && attempts > 4) clearInterval(injectTimer);
    if (attempts > 120) clearInterval(injectTimer); // 60s hard cap on injection retries
  }, 500);

  // Keep auto-answer running on a fast schedule independent of injection.
  setInterval(autoAnswerTick, (cfg && cfg.autoAnswer && cfg.autoAnswer.pollIntervalMs) || 250);

  // Mutation observer: inject as soon as the menu/dialer mounts.
  try {
    var obs = new MutationObserver(function () { boot(); });
    obs.observe(document.documentElement, { childList: true, subtree: true });
  } catch (e) {}

  // Manual re-inject hook for DevTools: window.__spicyInject()
  window.__spicyInject = function () { boot(); return tryInject(); };
  window.__spicySet = function (k, v) { if (k === 'autoAnswer') STATE.autoAnswer = !!v; renderStatus(); return STATE; };

  log('in-app engine loaded — will auto-answer + inject UI once the dialer mounts');
})();
