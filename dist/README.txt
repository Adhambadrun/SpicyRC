SPICY LAMAR — RINGCENTRAL IN-APP EDITION
==========================================

No standalone SpicyLamar.exe is distributed or launched by the supported
workflow. Spicy Lamar is installed into RingCentral's own Electron app so its
buttons and automation remain inside RingCentral.

INSTALL
  1. Close RingCentral.
  2. Run the repository-root build.bat.
  3. It patches resources\app.asar, creates app.asar.bak, and starts
     RingCentral.
  4. In RingCentral's dialer, use 🌶 SPICY ON/OFF next to Call and open
     Settings for Auto-Answer and Pin RingCentral on top.

SCOPE
  * No separate dashboard or keypad popup.
  * No global shortcuts, Alt+F1 sequence, OS-wide input injection, or focus
    changes.
  * Auto-answer, DTMF helpers, and calls use only RingCentral DOM controls.
  * Pin-on-top targets only RingCentral's BrowserWindow.

The old portable/native sources are retained for repository history only.
Top-level build_portable.bat and build_standalone.bat redirect to build.bat so
all supported control paths are in-app.
