/**
 * inject-spicy-button.js
 * Drop-in for RingCentral Phone Electron renderer.
 * Finds the dialer container that renders "Enter a name or number" (your image-2.png)
 * and injects a 🌶 SPICY button that proxies to SpicyLamar's engine (Alt+F1 / DTMF).
 *
 * INSTALL:
 *   1. After `asar extract resources/app.asar app-src`, copy this file to:
 *        app-src/inject-spicy-button.js
 *   2. Copy styles/spicy-button.css to app-src/styles/spicy-button.css
 *   3. In app-src/renderer/index.js (or main renderer entry) add at top:
 *        require('../inject-spicy-button');
 *        // and in index.html <head>: <link rel="stylesheet" href="../styles/spicy-button.css"/>
 *   4. asar pack app-src resources/app.asar
 *
 * No rebuild of native code needed. Works alongside SpicyLamar.exe's
 * SetParent overlay host (ringcentral-patch/spicy-overlay-host.js) if you
 * prefer not to modify app.asar at all — the JS will find the window via
 * `window` object and inject.
 */

(function () {
  const CHILI = '#FF3300';
  const BTN_ID = 'spicy-lamar-btn';
  const MENU_ID = 'spicy-lamar-menu-item';

  function log(msg) { try { console.log('[SpicyLamar]', msg); } catch {} }

  // 6-shot Alt+F1 cascade proxied via IPC to main or via window messages
  // In renderer we can send via `window.postMessage` or `ipcRenderer.send`.
  // Fallback: dispatch keyboard event that SpicyLamar.exe's WindowCache will catch.
  function fireSpicyAction() {
    // Visual feedback
    const btn = document.getElementById(BTN_ID);
    if (btn) { btn.style.background = '#FF3300'; setTimeout(()=> btn.style.background = '#1A1A1A', 150); }

    // (a) If SpicyLamar.exe overlay host is present, call its exposed API
    if (window.spicyLamar && window.spicyLamar.tryAnswer) {
      window.spicyLamar.tryAnswer(); // bridged via preload.js
      log('fired via window.spicyLamar.tryAnswer()');
      return;
    }
    // (b) Electron IPC — main process can forward to our Engine::TryAnswer
    try {
      const { ipcRenderer } = require('electron');
      ipcRenderer.send('spicy-lamar:toggle', { chan: 4 });
      log('fired via ipcRenderer spicy-lamar:toggle');
    } catch {}
    // (c) Fallback: synthesize Alt+F1 for SpicyLamar.exe to catch via WindowCache
    // (our C++ engine watches for RingCentral window + Alt+F1)
    try {
      const ev1 = new KeyboardEvent('keydown', { key: 'F1', code: 'F1', keyCode: 112, altKey: true, bubbles: true });
      const ev2 = new KeyboardEvent('keyup', { key: 'F1', code: 'F1', keyCode: 112, altKey: true, bubbles: true });
      document.dispatchEvent(ev1); document.dispatchEvent(ev2);
    } catch {}
  }

  function injectMenuItem() {
    // Find the dropdown that contains "Reattach keypad" (image-1.png)
    const walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT);
    let node; let menuRoot = null;
    while ((node = walker.nextNode())) {
      if (node.textContent.includes('Reattach keypad')) {
        // climb to the menu container (usually a <div role="menu"> or <ul>)
        let el = node.parentElement;
        for (let i=0;i<6 && el; i++) {
          if (el.querySelector && el.querySelector('[role="menuitem"], .menu-item, li')) { menuRoot = el.closest('div, ul') || el.parentElement; break; }
          el = el.parentElement;
        }
        if (!menuRoot) menuRoot = node.parentElement.parentElement;
        break;
      }
    }
    if (!menuRoot) { log('menu root not found (Reattach keypad text missing yet)'); return false; }
    if (document.getElementById(MENU_ID)) return true;

    const isDark = true;
    const item = document.createElement('div');
    item.id = MENU_ID;
    item.setAttribute('role', 'menuitem');
    item.style.cssText = 'padding:10px 16px; cursor:pointer; color:#FF3300; display:flex; align-items:center; gap:8px; border-top:1px solid #303030; margin-top:8px; font-family: Segoe UI, sans-serif; font-size:13px;';
    item.innerHTML = '<span>🌶</span><span>Spicy Lamar — Auto-Answer</span><span id="spicy-status" style="margin-left:auto; color:#00FF66; font-size:12px;">[ON]</span>';
    item.addEventListener('click', fireSpicyAction);
    item.addEventListener('mouseenter', () => item.style.background = '#1A1A1A');
    item.addEventListener('mouseleave', () => item.style.background = 'transparent');
    // insert after "Phone settings"
    const phoneSettings = Array.from(menuRoot.querySelectorAll('*')).find(el => el.textContent && el.textContent.includes('Phone settings'));
    if (phoneSettings && phoneSettings.parentElement === menuRoot) {
      phoneSettings.insertAdjacentElement('afterend', item);
    } else {
      menuRoot.appendChild(item);
    }
    log('injected menu item');
    return true;
  }

  function injectKeypadButton() {
    if (document.getElementById(BTN_ID)) return true;

    // Find the field "Enter a name or number" (image-2.png)
    const field = Array.from(document.querySelectorAll('input, div, span')).find(el => {
      const p = el.getAttribute && el.getAttribute('placeholder');
      const t = el.textContent || '';
      return (p && p.includes('Enter a name or number')) || t.includes('Enter a name or number');
    });
    // Find CALL button (green) or keypad grid container
    const callBtn = document.querySelector('button[class*="call"], [aria-label*="Call"], .call-btn') ||
                    Array.from(document.querySelectorAll('button')).find(b => b.textContent.includes('Call') || getComputedStyle(b).backgroundColor.includes('0, 150, 60') || b.innerHTML.includes('Noteson'));

    let anchor = null;
    if (callBtn) anchor = callBtn.parentElement;
    else if (field) anchor = field.closest('div')?.parentElement;
    // Fallback: grid of digits 1 2 3
    if (!anchor) {
      const one = Array.from(document.querySelectorAll('button, div')).find(el => el.textContent.trim() === '1' && el.nextElementSibling && el.nextElementSibling.textContent.includes('2'));
      if (one) anchor = one.closest('div')?.parentElement?.parentElement;
    }
    if (!anchor) { log('anchor not found for keypad button'); return false; }

    const btn = document.createElement('button');
    btn.id = BTN_ID;
    btn.textContent = '🌶 SPICY';
    btn.title = 'Spicy Lamar — fire Alt+F1 cascade + DTMF';
    btn.style.cssText = [
      'background:#1A1A1A', 'color:#FF3300', 'border:1px solid #303030',
      'border-radius:8px', 'padding:10px 16px', 'font-family: Segoe UI, sans-serif',
      'font-weight:700', 'font-size:13px', 'cursor:pointer',
      'display:inline-flex', 'align-items:center', 'justify-content:center',
      'flex:1', 'min-width:90px', 'margin:0 8px'
    ].join(';');
    btn.addEventListener('click', fireSpicyAction);
    btn.addEventListener('mouseenter', () => btn.style.background = '#262626');
    btn.addEventListener('mouseleave', () => btn.style.background = '#1A1A1A');

    // Insert between Noteson and CALL (as in your image-2 bottom row)
    // If anchor has 2 children (Noteson + Call), insert in middle
    if (anchor.children.length >= 2) {
      anchor.insertBefore(btn, anchor.children[1]);
    } else {
      // Create a flex row wrapper
      const row = document.createElement('div');
      row.style.cssText = 'display:flex; gap:12px; margin-top:12px; align-items:center;';
      // move existing call/notes buttons into row if needed
      anchor.appendChild(row);
      row.appendChild(btn);
    }
    log('injected keypad button');
    return true;
  }

  function tryInject() {
    let a = injectMenuItem();
    let b = injectKeypadButton();
    return a || b;
  }

  // Retry until DOM ready (RingCentral lazy-loads dialer)
  let attempts = 0;
  const iv = setInterval(() => {
    attempts++;
    if (tryInject() || attempts > 40) {
      if (attempts > 40) log('injection retries exhausted — check selectors');
      clearInterval(iv);
    }
  }, 500);

  // Also observe mutations (menu opens dynamically)
  const obs = new MutationObserver(() => tryInject());
  obs.observe(document.documentElement, { childList: true, subtree: true });

  // Expose for manual testing in DevTools: `window.__spicyInject()`
  window.__spicyInject = tryInject;
  log('injection script loaded — will retry every 500ms until "Enter a name or number" appears');
})();
