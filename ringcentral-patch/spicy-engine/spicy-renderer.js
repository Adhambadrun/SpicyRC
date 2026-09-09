/**
 * Spicy Lamar renderer integration.
 *
 * This script runs in RingCentral's own renderer and intentionally has a small
 * safety boundary: it only clicks visible controls in this document. It does
 * not create windows, register global hotkeys, focus another application, or
 * operate the keyboard outside RingCentral.
 */
(function () {
  'use strict';

  // The main process can inject this after every renderer navigation. Do not
  // create a second timer/observer when both the main injector and index.html
  // load it.
  if (window.__spicyLamarRendererLoaded) return;
  window.__spicyLamarRendererLoaded = true;

  var fallbackConfig = {
    autoAnswer: { enabledOnStartup: true, pollIntervalMs: 250, answerFiredCooldownMs: 2000, dtmfDigitDelayMs: 80 },
    pinOnTopDefault: true,
    answerSelectors: [
      'button[aria-label="Answer"]', 'button[aria-label="Accept call"]',
      '[data-testid="answer-call"]', '[class*="incoming"] button[class*="answer"]'
    ],
    incomingCallSelectors: [
      '[data-testid*="incoming-call"]', '[aria-label*="Incoming call"]', '[class*="incoming-call"]'
    ],
    dialFieldSelectors: [
      'input[placeholder*="Enter a name or number"]', 'input[type="tel"]', 'input[inputmode="tel"]'
    ],
    callButtonSelectors: [
      'button[aria-label="Call"]', '[data-testid="call-button"]', 'button[class*="callButton"]', 'button[class*="call-btn"]'
    ],
    dtmfPadSelectors: ['[data-testid*="dtmf"]', '[class*="dtmf"]', '[class*="keypad"]', '[class*="dialpad"]'],
    menuItemTextAnchor: 'Phone settings'
  };

  var cfg = window.spicyConfig || fallbackConfig;
  var BUTTON_ID = 'spicy-lamar-btn';
  var MENU_ID = 'spicy-lamar-menu-item';
  var PIN_MENU_ID = 'spicy-lamar-pin-menu-item';
  var STORAGE_KEY = 'spicy-lamar.state.v2';
  var state = {
    autoAnswer: cfg.autoAnswer ? cfg.autoAnswer.enabledOnStartup !== false : true,
    pinned: cfg.pinOnTopDefault !== false,
    lastAnswerAt: 0
  };

  function log(message) {
    try { console.log('[SpicyLamar] ' + message); } catch (ignore) {}
  }

  function warn(message) {
    try { console.warn('[SpicyLamar] ' + message); } catch (ignore) {}
  }

  function bridge() {
    try { return window.spicyLamar || null; } catch (ignore) { return null; }
  }

  function loadLocalState() {
    try {
      var saved = JSON.parse(window.localStorage.getItem(STORAGE_KEY) || '{}');
      if (typeof saved.autoAnswer === 'boolean') state.autoAnswer = saved.autoAnswer;
      if (typeof saved.pinned === 'boolean') state.pinned = saved.pinned;
    } catch (ignore) {}

    // The preload bridge is optional. If it is present, the RingCentral main
    // process is authoritative for pin state and persisted settings.
    try {
      var api = bridge();
      if (api && typeof api.getState === 'function') {
        var fromMain = api.getState();
        if (fromMain && typeof fromMain.autoAnswer === 'boolean') state.autoAnswer = fromMain.autoAnswer;
        if (fromMain && typeof fromMain.pinned === 'boolean') state.pinned = fromMain.pinned;
      }
    } catch (ignore) {}
  }

  function saveLocalState() {
    try {
      window.localStorage.setItem(STORAGE_KEY, JSON.stringify({
        autoAnswer: state.autoAnswer,
        pinned: state.pinned
      }));
    } catch (ignore) {}
  }

  function isVisible(element) {
    if (!element || element.nodeType !== 1 || element.disabled) return false;
    var style = window.getComputedStyle(element);
    if (!style || style.display === 'none' || style.visibility === 'hidden' || style.opacity === '0') return false;
    var rect = element.getBoundingClientRect();
    return rect.width > 0 && rect.height > 0;
  }

  function firstVisible(selectors, scope) {
    var root = scope || document;
    if (!selectors) return null;
    for (var i = 0; i < selectors.length; i++) {
      var nodes;
      try { nodes = root.querySelectorAll(selectors[i]); } catch (ignore) { nodes = []; }
      for (var j = 0; j < nodes.length; j++) {
        if (isVisible(nodes[j])) return nodes[j];
      }
    }
    return null;
  }

  function textElement(needle, scope) {
    var root = scope || document;
    if (!root || !document.createTreeWalker) return null;
    var walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
    var node;
    while ((node = walker.nextNode())) {
      if ((node.textContent || '').trim().indexOf(needle) !== -1) return node.parentElement;
    }
    return null;
  }

  function buttonFrom(element) {
    if (!element) return null;
    if (element.tagName === 'BUTTON') return element;
    try { return element.querySelector('button'); } catch (ignore) { return null; }
  }

  // React-controlled inputs ignore a plain assignment to element.value. Use
  // the native setter then dispatch the same input/change notifications that
  // RingCentral's own field receives. These events remain in this RC document.
  function setControlledValue(input, value) {
    if (!input) return false;
    try {
      var proto = input instanceof HTMLTextAreaElement ? HTMLTextAreaElement.prototype : HTMLInputElement.prototype;
      var descriptor = Object.getOwnPropertyDescriptor(proto, 'value');
      if (descriptor && descriptor.set) descriptor.set.call(input, value);
      else input.value = value;
      input.dispatchEvent(new Event('input', { bubbles: true }));
      input.dispatchEvent(new Event('change', { bubbles: true }));
      return true;
    } catch (error) {
      warn('could not update RingCentral dial field: ' + error.message);
      return false;
    }
  }

  function controlLabel(element) {
    return ((element.getAttribute && (element.getAttribute('aria-label') || element.getAttribute('title'))) || element.textContent || '')
      .replace(/\s+/g, ' ').trim();
  }

  function findDtmfButton(digit) {
    var containers = [];
    var selectors = cfg.dtmfPadSelectors || [];
    for (var i = 0; i < selectors.length; i++) {
      var found;
      try { found = document.querySelectorAll(selectors[i]); } catch (ignore) { found = []; }
      for (var j = 0; j < found.length; j++) {
        if (isVisible(found[j])) containers.push(found[j]);
      }
    }

    for (var a = 0; a < containers.length; a++) {
      var root = containers[a];
      var candidates = root.tagName === 'BUTTON' ? [root] : root.querySelectorAll('button, [role="button"]');
      for (var b = 0; b < candidates.length; b++) {
        var label = controlLabel(candidates[b]);
        // "2\nABC" is an RC keypad key; match its first token only.
        if (label === digit || label.split(' ')[0] === digit) return candidates[b];
      }
    }
    return null;
  }

  function sendDtmf(digit) {
    digit = String(digit || '');
    if (!/^[0-9*#+]$/.test(digit)) {
      warn('refused invalid DTMF character: ' + digit);
      return false;
    }

    var key = findDtmfButton(digit);
    if (key) {
      key.click();
      log('DTMF ' + digit + ' sent through RingCentral keypad');
      return true;
    }

    var field = firstVisible(cfg.dialFieldSelectors);
    if (field && setControlledValue(field, String(field.value || '') + digit)) {
      log('DTMF ' + digit + ' sent through RingCentral dial field');
      return true;
    }

    warn('no RingCentral keypad or dial field available for DTMF ' + digit);
    return false;
  }

  function placeCall(digits) {
    var value = String(digits || '');
    if (!value || !/^[0-9*#+]+$/.test(value)) {
      warn('refused invalid dial string');
      return false;
    }

    var field = firstVisible(cfg.dialFieldSelectors);
    if (!field || !setControlledValue(field, value)) {
      warn('RingCentral dial field is not available');
      return false;
    }

    var callControl = firstVisible(cfg.callButtonSelectors);
    var callButton = buttonFrom(callControl);
    if (!callButton || !isVisible(callButton)) {
      warn('RingCentral CALL control is not available');
      return false;
    }
    callButton.click();
    log('CALL pressed in RingCentral for ' + value);
    return true;
  }

  function autoAnswerTick() {
    if (!state.autoAnswer) return;
    var now = Date.now();
    var autoCfg = cfg.autoAnswer || fallbackConfig.autoAnswer;
    if (now - state.lastAnswerAt < autoCfg.answerFiredCooldownMs) return;

    var incoming = firstVisible(cfg.incomingCallSelectors);
    var answer = firstVisible(cfg.answerSelectors);
    // Never click a generic button merely because its class happens to contain
    // "answer". An incoming surface must be visible in the RC renderer.
    if (!incoming || !answer) return;

    var answerButton = buttonFrom(answer) || answer;
    if (!isVisible(answerButton)) return;
    answerButton.click();
    state.lastAnswerAt = now;
    log('answered incoming RingCentral call using its own Answer control');
    flash(document.getElementById(BUTTON_ID));
  }

  function setAutoAnswer(enabled, source) {
    state.autoAnswer = !!enabled;
    saveLocalState();
    try {
      var api = bridge();
      if (api && typeof api.setAutoAnswer === 'function') api.setAutoAnswer(state.autoAnswer);
    } catch (ignore) {}
    log('auto-answer ' + (state.autoAnswer ? 'ON' : 'OFF') + (source ? ' (' + source + ')' : ''));
    renderState();
  }

  function setPinned(enabled) {
    state.pinned = !!enabled;
    saveLocalState();
    try {
      var api = bridge();
      if (api && typeof api.setPinned === 'function') {
        api.setPinned(state.pinned);
      } else {
        warn('pin bridge unavailable; no operating-system window was changed');
      }
    } catch (error) {
      warn('could not update RingCentral pin state: ' + error.message);
    }
    renderState();
  }

  function flash(button) {
    if (!button) return;
    var old = button.style.background;
    button.style.background = '#00B450';
    setTimeout(function () {
      button.style.background = old;
      renderState();
    }, 180);
  }

  function renderState() {
    var button = document.getElementById(BUTTON_ID);
    if (button) {
      button.textContent = '🌶 ' + (state.autoAnswer ? 'SPICY ON' : 'SPICY OFF');
      button.style.background = state.autoAnswer ? '#FF3300' : '#1A1A1A';
      button.style.color = state.autoAnswer ? '#FFFFFF' : '#FF3300';
      button.setAttribute('aria-pressed', state.autoAnswer ? 'true' : 'false');
    }
    var status = document.getElementById('spicy-status');
    if (status) {
      status.textContent = state.autoAnswer ? '[ON]' : '[OFF]';
      status.style.color = state.autoAnswer ? '#00FF66' : '#FF3300';
    }
    var pinStatus = document.getElementById('spicy-pin-status');
    if (pinStatus) {
      pinStatus.textContent = state.pinned ? '[ON]' : '[OFF]';
      pinStatus.style.color = state.pinned ? '#00FF66' : '#FF3300';
    }
  }

  function injectedMenuItem(id, label, statusId, onClick) {
    var item = document.createElement('div');
    item.id = id;
    item.setAttribute('role', 'menuitem');
    item.setAttribute('tabindex', '0');
    item.style.cssText = [
      'padding:10px 16px', 'cursor:pointer', 'display:flex', 'align-items:center', 'gap:8px',
      'border-top:1px solid #303030', 'font-family:"Segoe UI",sans-serif', 'font-size:13px',
      'color:#FF3300', 'user-select:none'
    ].join(';');
    item.innerHTML = '<span>🌶</span><span style="flex:1">' + label + '</span>' +
      '<span id="' + statusId + '" style="margin-left:auto;font-size:12px">[ON]</span>';
    item.addEventListener('click', function (event) {
      event.preventDefault();
      event.stopPropagation();
      onClick();
    });
    item.addEventListener('keydown', function (event) {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault();
        onClick();
      }
    });
    item.addEventListener('mouseenter', function () { item.style.background = '#1A1A1A'; });
    item.addEventListener('mouseleave', function () { item.style.background = 'transparent'; });
    return item;
  }

  function menuRootFrom(anchor) {
    if (!anchor) return null;
    var direct = anchor.closest && anchor.closest('[role="menu"], ul');
    if (direct) return direct;

    // Some RC versions use divs without ARIA roles. Use the first parent with
    // several visible button/menu-item siblings, rather than appending to body.
    var node = anchor;
    for (var i = 0; i < 7 && node; i++, node = node.parentElement) {
      var siblingControls = node.querySelectorAll && node.querySelectorAll('[role="menuitem"], button, [role="button"]');
      if (siblingControls && siblingControls.length >= 3) return node;
    }
    return anchor.parentElement || null;
  }

  function directMenuChild(root, anchor) {
    var child = anchor;
    while (child && child.parentElement && child.parentElement !== root) child = child.parentElement;
    return child || anchor;
  }

  function injectMenuItems() {
    var existingAuto = document.getElementById(MENU_ID);
    var existingPin = document.getElementById(PIN_MENU_ID);
    if (existingAuto && existingPin) return true;

    var anchor = textElement(cfg.menuItemTextAnchor || 'Phone settings');
    var root = menuRootFrom(anchor);
    if (!anchor || !root || root === document.body || root === document.documentElement) return false;

    var reference = directMenuChild(root, anchor);
    if (!existingAuto) {
      var autoItem = injectedMenuItem(MENU_ID, 'Spicy Lamar — Auto-Answer', 'spicy-status', function () {
        setAutoAnswer(!state.autoAnswer, 'RingCentral menu');
      });
      reference.insertAdjacentElement('afterend', autoItem);
      reference = autoItem;
      log('Auto-Answer control inserted in RingCentral menu');
    }
    if (!existingPin) {
      var pinItem = injectedMenuItem(PIN_MENU_ID, 'Pin RingCentral on top', 'spicy-pin-status', function () {
        setPinned(!state.pinned);
      });
      reference.insertAdjacentElement('afterend', pinItem);
      log('Pin control inserted in RingCentral menu');
    }
    renderState();
    return true;
  }

  function injectDialerButton() {
    if (document.getElementById(BUTTON_ID)) return true;

    var callControl = firstVisible(cfg.callButtonSelectors);
    var callButton = buttonFrom(callControl);
    if (!callButton || !callButton.parentElement) return false;

    var button = document.createElement('button');
    button.id = BUTTON_ID;
    button.type = 'button';
    button.title = 'Toggle RingCentral in-app auto-answer';
    button.setAttribute('aria-label', 'Toggle Spicy Lamar auto-answer');
    button.style.cssText = [
      'background:#FF3300', 'color:#FFFFFF', 'border:1px solid #303030',
      'border-radius:8px', 'padding:10px 16px', 'font-family:"Segoe UI",sans-serif',
      'font-weight:700', 'font-size:13px', 'cursor:pointer', 'display:inline-flex',
      'align-items:center', 'justify-content:center', 'min-width:104px', 'margin:0 8px',
      'box-shadow:0 0 6px rgba(255,51,0,.35)'
    ].join(';');
    button.addEventListener('click', function (event) {
      event.preventDefault();
      event.stopPropagation();
      setAutoAnswer(!state.autoAnswer, 'RingCentral dialer');
    });
    button.addEventListener('mouseenter', function () {
      if (state.autoAnswer) button.style.background = '#FF5533';
      else button.style.background = '#262626';
    });
    button.addEventListener('mouseleave', renderState);

    // The button is an immediate sibling of RC's call control, so it remains
    // in the dialer action row and cannot be placed in another application UI.
    callButton.parentElement.insertBefore(button, callButton);
    renderState();
    log('SPICY control inserted in RingCentral dialer');
    return true;
  }

  function inject() {
    var dialer = injectDialerButton();
    var menu = injectMenuItems();
    return dialer || menu;
  }

  function installBridgeSubscription() {
    try {
      var api = bridge();
      if (!api || typeof api.onState !== 'function') return;
      api.onState(function (next) {
        if (!next) return;
        if (typeof next.autoAnswer === 'boolean') state.autoAnswer = next.autoAnswer;
        if (typeof next.pinned === 'boolean') state.pinned = next.pinned;
        saveLocalState();
        renderState();
      });
    } catch (ignore) {}
  }

  loadLocalState();
  installBridgeSubscription();
  inject();

  // RingCentral lazily mounts both the dialer and the settings menu. Observe
  // only this renderer document; no system-wide listener is installed.
  try {
    var pending = false;
    var observer = new MutationObserver(function () {
      if (pending) return;
      pending = true;
      setTimeout(function () { pending = false; inject(); }, 0);
    });
    observer.observe(document.documentElement, { childList: true, subtree: true });
  } catch (ignore) {}

  var interval = (cfg.autoAnswer && cfg.autoAnswer.pollIntervalMs) || 250;
  window.setInterval(autoAnswerTick, interval);

  // Deliberately scoped diagnostic helpers. They act only on controls found in
  // this RingCentral document and are useful when tuning selectors in DevTools.
  window.__spicyInject = function () { return inject(); };
  window.__spicySet = function (key, value) {
    if (key === 'autoAnswer') setAutoAnswer(!!value, 'DevTools');
    if (key === 'pinned') setPinned(!!value);
    return { autoAnswer: state.autoAnswer, pinned: state.pinned };
  };
  window.__spicySendDtmf = sendDtmf;
  window.__spicyPlaceCall = placeCall;

  log('in-app renderer loaded: controls are scoped to this RingCentral window');
})();
