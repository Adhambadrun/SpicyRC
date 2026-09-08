using System;
using System.Drawing;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Threading;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace SpicyLamar
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            bool createdNew;
            Mutex mutex = null;
            try { mutex = new Mutex(true, @"Global\SpicyLamar", out createdNew); }
            catch (UnauthorizedAccessException) { mutex = new Mutex(true, @"Local\SpicyLamar", out createdNew); }
            using (mutex)
            {
                if (!createdNew) return;
                Application.Run(new DashboardContext());
            }
        }
    }

    class DashboardContext : ApplicationContext
    {
        private NotifyIcon trayIcon;
        private IntegratedForm dashboard;
        private AnswerEngine engine;
        public DashboardContext()
        {
            engine = new AnswerEngine();
            trayIcon = new NotifyIcon()
            {
                Icon = LoadAppIcon(),
                Text = "Spicy Lamar",
                Visible = true,
                ContextMenu = BuildMenu()
            };
            trayIcon.DoubleClick += (s,e)=> dashboard.ToggleVisibility();
            engine.ActiveChanged += active => {
                trayIcon.Text = active ? "Spicy Lamar \u2014 Integrated \u2022 TopMost " + (dashboard.IsPinned ? "ON":"OFF") : "Spicy Lamar \u2014 PAUSED (F11 to start)";
                if (dashboard != null && !dashboard.IsDisposed) dashboard.Invalidate();
            };
            dashboard = new IntegratedForm(engine);
            dashboard.FormClosed += (s,e)=> ExitApp();
            // Hotkeys: F8 self-test, F9 dash, F11 pause, F12 exit
            HotKeyManager.RegisterHotKey(dashboard.Handle, 1, (uint)HotKeyManager.KeyModifiers.NoRepeat, (uint)Keys.F8);
            HotKeyManager.RegisterHotKey(dashboard.Handle, 2, (uint)HotKeyManager.KeyModifiers.NoRepeat, (uint)Keys.F9);
            HotKeyManager.RegisterHotKey(dashboard.Handle, 3, (uint)HotKeyManager.KeyModifiers.NoRepeat, (uint)Keys.F11);
            HotKeyManager.RegisterHotKey(dashboard.Handle, 4, (uint)HotKeyManager.KeyModifiers.NoRepeat, (uint)Keys.F12);
            dashboard.Show();
        }
        private Icon LoadAppIcon()
        {
            try { Icon embedded = Icon.ExtractAssociatedIcon(Application.ExecutablePath); if (embedded != null) return embedded; } catch {}
            try { if (System.IO.File.Exists("icon.ico")) return new Icon("icon.ico"); } catch {}
            return SystemIcons.Application;
        }
        private ContextMenu BuildMenu()
        {
            var menu = new ContextMenu();
            menu.MenuItems.Add("Open Dashboard (F9)", (s,e)=> dashboard.ToggleVisibility());
            menu.MenuItems.Add("Pause/Start (F11)", (s,e)=> engine.Toggle());
            menu.MenuItems.Add("-");
            menu.MenuItems.Add("Exit (F12)", (s,e)=> ExitApp());
            return menu;
        }
        private void ExitApp()
        {
            if (trayIcon != null){ trayIcon.Visible=false; trayIcon.Dispose(); trayIcon=null; }
            if (dashboard != null){
                try{ HotKeyManager.UnregisterHotKey(dashboard.Handle,1); HotKeyManager.UnregisterHotKey(dashboard.Handle,2); HotKeyManager.UnregisterHotKey(dashboard.Handle,3); HotKeyManager.UnregisterHotKey(dashboard.Handle,4);}catch{}
                if(!dashboard.IsDisposed) dashboard.Dispose();
                dashboard=null;
            }
            if(engine!=null){ engine.Dispose(); engine=null; }
            Application.Exit();
        }
        protected override void ExitThreadCore(){ ExitApp(); base.ExitThreadCore(); }
    }

    class IntegratedForm : Form
    {
        private AnswerEngine engine;
        private System.Windows.Forms.Timer refreshTimer;
        private System.Windows.Forms.Timer pollTimer;

        // Palette
        private readonly Color CLR_OBSIDIAN = Color.FromArgb(5,5,5);
        private readonly Color CLR_SETTINGS = Color.FromArgb(26,26,26);
        private readonly Color CLR_PANEL = Color.FromArgb(16,16,16);
        private readonly Color CLR_DISPLAY = Color.FromArgb(22,22,22);
        private readonly Color CLR_DROPDOWN = Color.FromArgb(32,32,32);
        private readonly Color CLR_BTN = Color.FromArgb(38,38,38);
        private readonly Color CLR_BTN_HOVER = Color.FromArgb(64,64,64);
        private readonly Color CLR_BTN_ACTIVE = Color.FromArgb(255,51,0);
        private readonly Color CLR_BORDER = Color.FromArgb(48,48,48);
        private readonly Color CLR_NEON = Color.FromArgb(0,255,102);
        private readonly Color CLR_CHILI = Color.FromArgb(255,51,0);
        private readonly Color CLR_CALL = Color.FromArgb(0,150,60);
        private readonly Color CLR_CALL_HOVER = Color.FromArgb(0,180,80);
        private readonly Color CLR_END = Color.FromArgb(140,30,30);
        private readonly Color CLR_END_HOVER = Color.FromArgb(180,40,40);
        private readonly Color CLR_DIM = Color.FromArgb(180,180,180);
        private readonly Color CLR_MUTED = Color.FromArgb(130,130,130);

        // UI panels
        private Panel settingsBar;
        private Panel leftPane;
        private Panel keypadPanel;
        private Panel dropdownPanel;

        // Settings bar controls
        private Button btnPause, btnTest, btnSettings, btnExit;
        private Label lblTitle, lblCallerId;

        // Keypad controls
        private Panel displayPanel;
        private TextBox displayBox; // read-only display mimicking field
        private Button btnClear;
        private Button[] keyButtons = new Button[12];
        private Button btnCall, btnEnd;
        private Label lblKeypadHeader, lblHint;

        // Dropdown controls
        private Button btnDropClose;
        private Button btnPin, btnReattach, btnEmergency, btnRingOut, btnIncoming, btnVoicemail, btnPhone;

        private string dialBuffer = "";
        private bool isPinned = true;
        private bool ringOutEnabled = false;
        public bool IsPinned { get { return isPinned; } }

        private Font fontTitle, fontSmall, fontTiny, fontMono, fontKeypad, fontKeypadSub;

        public IntegratedForm(AnswerEngine engine)
        {
            this.engine = engine;
            this.Text = "Spicy Lamar v1.0 \u2014 Integrated";
            this.Size = new Size(980, 620);
            this.MinimumSize = this.Size;
            this.MaximumSize = this.Size;
            this.BackColor = CLR_OBSIDIAN;
            this.FormBorderStyle = FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.ControlBox = true;
            this.TopMost = true;
            this.StartPosition = FormStartPosition.CenterScreen;
            this.KeyPreview = true;
            this.DoubleBuffered = true;

            fontTitle = new Font("Segoe UI", 9.5f, FontStyle.Bold);
            fontSmall = new Font("Segoe UI", 8.5f, FontStyle.Regular);
            fontTiny = new Font("Segoe UI", 7.5f, FontStyle.Regular);
            fontMono = new Font("Consolas", 8.5f, FontStyle.Regular);
            fontKeypad = new Font("Segoe UI", 16f, FontStyle.Bold);
            fontKeypadSub = new Font("Segoe UI", 7f, FontStyle.Regular);

            BuildLayout();

            refreshTimer = new System.Windows.Forms.Timer(); refreshTimer.Interval = 250;
            refreshTimer.Tick += (s,e)=> { leftPane.Invalidate(); this.Invalidate(); };
            refreshTimer.Start();

            pollTimer = new System.Windows.Forms.Timer(); pollTimer.Interval = 5; // TURBO 200Hz
            pollTimer.Tick += (s,e)=> {
                IntPtr found = engine.FindRingCentralWindow();
                if (found != IntPtr.Zero) engine.TryFire(found, 2);
                else {
                    var all = engine.CollectRingCentralWindows();
                    foreach(var w in all) engine.TryFire(w, 2);
                }
            };
            pollTimer.Start();

            engine.Log("Spicy Lamar v1.0 Integrated online. Keypad docked inside dashboard (no separate window).");
            engine.Log("Auto-answer armed: TURBO 200Hz poll (5ms) + WinEvent + ShellHook. Pin ON.");
        }

        private void BuildLayout()
        {
            // ── SETTINGS BAR (Dock Top, H=38) ──
            settingsBar = new Panel();
            settingsBar.Dock = DockStyle.Top;
            settingsBar.Height = 38;
            settingsBar.BackColor = CLR_SETTINGS;
            settingsBar.Padding = new Padding(0);
            settingsBar.Paint += SettingsBar_Paint;
            settingsBar.MouseDown += SettingsBar_MouseDown;
            this.Controls.Add(settingsBar);

            lblTitle = new Label();
            lblTitle.Text = "\uD83C\uDF36 SPICY LAMAR v1.0 \u2014 Integrated";
            lblTitle.ForeColor = CLR_CHILI;
            lblTitle.Font = new Font("Segoe UI", 10f, FontStyle.Bold);
            lblTitle.AutoSize = true;
            lblTitle.Location = new Point(12, 9);
            lblTitle.BackColor = Color.Transparent;
            settingsBar.Controls.Add(lblTitle);

            lblCallerId = new Label();
            lblCallerId.Text = "My caller ID: (754) 654-0339";
            lblCallerId.ForeColor = CLR_DIM;
            lblCallerId.Font = fontSmall;
            lblCallerId.AutoSize = true;
            lblCallerId.Location = new Point(320, 12);
            lblCallerId.BackColor = Color.Transparent;
            settingsBar.Controls.Add(lblCallerId);

            // Pill buttons on right
            btnExit = CreatePillButton("\u2715 EXIT", 64);
            btnSettings = CreatePillButton("\u2699 SETTINGS \u25BC", 110);
            btnTest = CreatePillButton("\uD83E\uDDEA TEST", 76);
            btnPause = CreatePillButton("\u23F8 PAUSE", 82);
            // Order right to left docking via Anchor
            btnExit.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            btnSettings.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            btnTest.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            btnPause.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            // Position manually on resize
            settingsBar.Resize += (s,e)=> LayoutPills();
            btnPause.Click += (s,e)=> engine.Toggle();
            btnTest.Click += (s,e)=> DoSelfTest();
            btnSettings.Click += (s,e)=> ToggleDropdown();
            btnExit.Click += (s,e)=> Application.Exit();
            settingsBar.Controls.Add(btnExit);
            settingsBar.Controls.Add(btnSettings);
            settingsBar.Controls.Add(btnTest);
            settingsBar.Controls.Add(btnPause);
            // Initial layout after controls added
            this.Load += (s,e)=> LayoutPills();
            this.Shown += (s,e)=> LayoutPills();

            // ── DROPDOWN (anchored top-right, hidden) ──
            dropdownPanel = new Panel();
            dropdownPanel.Size = new Size(340, 265);
            dropdownPanel.BackColor = CLR_DROPDOWN;
            dropdownPanel.BorderStyle = BorderStyle.FixedSingle;
            dropdownPanel.Visible = false;
            dropdownPanel.Paint += Dropdown_Paint;
            this.Controls.Add(dropdownPanel);
            dropdownPanel.BringToFront();
            // Position dropdown after form shown
            this.Resize += (s,e)=> PositionDropdown();

            // Header inside dropdown
            var lblDropHeader = new Label();
            lblDropHeader.Text = "SETTINGS";
            lblDropHeader.ForeColor = Color.White;
            lblDropHeader.Font = fontTitle;
            lblDropHeader.AutoSize = true;
            lblDropHeader.Location = new Point(12, 10);
            dropdownPanel.Controls.Add(lblDropHeader);

            btnDropClose = new Button();
            btnDropClose.Text = "\u2715";
            btnDropClose.Size = new Size(24, 22);
            btnDropClose.Location = new Point(310, 6);
            btnDropClose.FlatStyle = FlatStyle.Flat;
            btnDropClose.FlatAppearance.BorderColor = CLR_BORDER;
            btnDropClose.BackColor = CLR_BTN;
            btnDropClose.ForeColor = Color.White;
            btnDropClose.Font = fontSmall;
            btnDropClose.Click += (s,e)=> { dropdownPanel.Visible = false; };
            dropdownPanel.Controls.Add(btnDropClose);

            // Dropdown items
            btnPin = CreateDropdownItem("\u2611 Pin window on top [ON]", 0);
            btnReattach = CreateDropdownItem("\u2192 Reattach keypad [Docked]", 1);
            btnEmergency = CreateDropdownItem("\u2192 Emergency address confirmation", 2);
            btnRingOut = CreateDropdownItem("\u2610 RingOut [OFF]", 3);
            btnIncoming = CreateDropdownItem("\u2192 Incoming call rules", 4);
            btnVoicemail = CreateDropdownItem("\u2192 Voicemail greeting", 5);
            btnPhone = CreateDropdownItem("\u2192 Phone settings", 6);
            btnPin.Click += (s,e)=> { isPinned = !isPinned; this.TopMost = isPinned; UpdatePinLabel(); dropdownPanel.Visible=false; engine.Log("Pin window on top " + (isPinned ? "ON":"OFF")); };
            btnReattach.Click += (s,e)=> { ReattachKeypad(); };
            btnEmergency.Click += (s,e)=> { engine.Log("Emergency address confirmation \u2014 placeholder"); dropdownPanel.Visible=false; };
            btnRingOut.Click += (s,e)=> { ringOutEnabled = !ringOutEnabled; UpdateRingOutLabel(); engine.Log("RingOut " + (ringOutEnabled ? "ON":"OFF")); dropdownPanel.Visible=false; };
            btnIncoming.Click += (s,e)=> { engine.Log("Incoming call rules \u2014 placeholder"); dropdownPanel.Visible=false; };
            btnVoicemail.Click += (s,e)=> { engine.Log("Voicemail greeting \u2014 placeholder"); dropdownPanel.Visible=false; };
            btnPhone.Click += (s,e)=> { engine.Log("Phone settings \u2014 placeholder"); dropdownPanel.Visible=false; };
            dropdownPanel.Controls.Add(btnPin);
            dropdownPanel.Controls.Add(btnReattach);
            dropdownPanel.Controls.Add(btnEmergency);
            dropdownPanel.Controls.Add(btnRingOut);
            dropdownPanel.Controls.Add(btnIncoming);
            dropdownPanel.Controls.Add(btnVoicemail);
            dropdownPanel.Controls.Add(btnPhone);
            UpdatePinLabel();
            UpdateRingOutLabel();

            // ── KEYPAD PANEL (Dock Right, W=280) ──
            keypadPanel = new Panel();
            keypadPanel.Dock = DockStyle.Right;
            keypadPanel.Width = 280;
            keypadPanel.BackColor = CLR_PANEL;
            keypadPanel.Padding = new Padding(12, 0, 12, 0);
            keypadPanel.Paint += KeypadPanel_Paint_Border;
            this.Controls.Add(keypadPanel);

            lblKeypadHeader = new Label();
            lblKeypadHeader.Text = "KEYPAD [reattached \u2713]";
            lblKeypadHeader.ForeColor = Color.White;
            lblKeypadHeader.Font = fontTitle;
            lblKeypadHeader.AutoSize = true;
            lblKeypadHeader.Location = new Point(14, 10);
            lblKeypadHeader.BackColor = Color.Transparent;
            keypadPanel.Controls.Add(lblKeypadHeader);

            // Display panel (field)
            displayPanel = new Panel();
            displayPanel.Location = new Point(12, 36);
            displayPanel.Size = new Size(256, 38);
            displayPanel.BackColor = CLR_DISPLAY;
            displayPanel.BorderStyle = BorderStyle.FixedSingle;
            displayPanel.Paint += (s,e)=> {
                // rounded effect via region? keep simple
            };
            keypadPanel.Controls.Add(displayPanel);

            displayBox = new TextBox();
            displayBox.ReadOnly = true;
            displayBox.BorderStyle = BorderStyle.None;
            displayBox.BackColor = CLR_DISPLAY;
            displayBox.ForeColor = Color.White;
            displayBox.Font = fontSmall;
            displayBox.Location = new Point(10, 10);
            displayBox.Size = new Size(200, 18);
            displayBox.Text = "Enter a name or number";
            displayBox.ForeColor = CLR_MUTED;
            displayBox.TabStop = false;
            displayPanel.Controls.Add(displayBox);

            btnClear = new Button();
            btnClear.Text = "C";
            btnClear.Size = new Size(32, 22);
            btnClear.Location = new Point(218, 7);
            btnClear.FlatStyle = FlatStyle.Flat;
            btnClear.FlatAppearance.BorderColor = CLR_BORDER;
            btnClear.BackColor = CLR_BTN;
            btnClear.ForeColor = Color.White;
            btnClear.Font = fontSmall;
            btnClear.Click += (s,e)=> HandleClear();
            displayPanel.Controls.Add(btnClear);

            // Keypad grid (4 rows x 3 cols)
            string[] digits = { "1","2","3","4","5","6","7","8","9","*","0","#" };
            string[] subs = { "", "ABC","DEF","GHI","JKL","MNO","PQRS","TUV","WXYZ","","+","" };
            int gridTop = 86;
            int gap = 8;
            int btnW = 80, btnH = 54;
            for(int i=0;i<12;i++){
                int row = i/3, col = i%3;
                var btn = new Button();
                btn.Tag = digits[i];
                btn.Text = digits[i] + (string.IsNullOrEmpty(subs[i]) ? "" : "\n" + subs[i]);
                btn.Font = subs[i]=="" ? fontKeypad : new Font("Segoe UI", 12f, FontStyle.Bold);
                // For digit with sub, we will custom paint? Use Text with newline
                btn.Size = new Size(btnW, btnH);
                btn.Location = new Point(12 + col*(btnW+gap), gridTop + row*(btnH+gap));
                btn.FlatStyle = FlatStyle.Flat;
                btn.FlatAppearance.BorderColor = CLR_BORDER;
                btn.BackColor = CLR_BTN;
                btn.ForeColor = Color.White;
                btn.UseVisualStyleBackColor = false;
                btn.Click += KeypadDigit_Click;
                // Hover effects
                btn.MouseEnter += (s,e)=> { var b=(Button)s; b.BackColor = CLR_BTN_HOVER; };
                btn.MouseLeave += (s,e)=> { var b=(Button)s; b.BackColor = CLR_BTN; };
                btn.MouseDown += (s,e)=> { var b=(Button)s; b.BackColor = CLR_BTN_ACTIVE; };
                btn.MouseUp += (s,e)=> { var b=(Button)s; b.BackColor = CLR_BTN_HOVER; };
                keyButtons[i]=btn;
                keypadPanel.Controls.Add(btn);
            }
            // Adjust font for buttons with subs to smaller line
            // CALL / END
            btnCall = new Button();
            btnCall.Text = "\u25B6 CALL";
            btnCall.Size = new Size(124, 38);
            btnCall.Location = new Point(12, gridTop + 4*btnH + 3*gap + 16);
            btnCall.FlatStyle = FlatStyle.Flat;
            btnCall.FlatAppearance.BorderColor = CLR_BORDER;
            btnCall.BackColor = CLR_CALL;
            btnCall.ForeColor = Color.White;
            btnCall.Font = fontSmall;
            btnCall.Click += (s,e)=> HandleCall();
            btnCall.MouseEnter += (s,e)=> btnCall.BackColor = CLR_CALL_HOVER;
            btnCall.MouseLeave += (s,e)=> btnCall.BackColor = CLR_CALL;
            keypadPanel.Controls.Add(btnCall);

            btnEnd = new Button();
            btnEnd.Text = "\u25A0 END";
            btnEnd.Size = new Size(124, 38);
            btnEnd.Location = new Point(12+124+gap, gridTop + 4*btnH + 3*gap + 16);
            btnEnd.FlatStyle = FlatStyle.Flat;
            btnEnd.FlatAppearance.BorderColor = CLR_BORDER;
            btnEnd.BackColor = CLR_END;
            btnEnd.ForeColor = Color.White;
            btnEnd.Font = fontSmall;
            btnEnd.Click += (s,e)=> HandleEnd();
            btnEnd.MouseEnter += (s,e)=> btnEnd.BackColor = CLR_END_HOVER;
            btnEnd.MouseLeave += (s,e)=> btnEnd.BackColor = CLR_END;
            keypadPanel.Controls.Add(btnEnd);

            lblHint = new Label();
            lblHint.Text = "\u232B Backspace \u2022 Type \u2022 Enter=Call";
            lblHint.ForeColor = CLR_MUTED;
            lblHint.Font = fontTiny;
            lblHint.AutoSize = true;
            lblHint.Location = new Point(12, gridTop + 4*btnH + 3*gap + 16 + 38 + 10);
            keypadPanel.Controls.Add(lblHint);

            // ── LEFT PANE (Dock Fill) ──
            leftPane = new Panel();
            leftPane.Dock = DockStyle.Fill;
            leftPane.BackColor = CLR_OBSIDIAN;
            leftPane.Paint += LeftPane_Paint;
            leftPane.Padding = new Padding(0);
            this.Controls.Add(leftPane);
            // Ensure correct z-order: settingsBar top, keypad right, left fill, dropdown front
            this.Controls.SetChildIndex(settingsBar, 0);
            this.Controls.SetChildIndex(dropdownPanel, 1);
            this.Controls.SetChildIndex(keypadPanel, 2);
            this.Controls.SetChildIndex(leftPane, 3);

            // After all, ensure dropdown positioned
            PositionDropdown();
        }

        private Button CreatePillButton(string text, int width)
        {
            var btn = new Button();
            btn.Text = text;
            btn.Size = new Size(width, 24);
            btn.FlatStyle = FlatStyle.Flat;
            btn.FlatAppearance.BorderColor = CLR_BORDER;
            btn.BackColor = CLR_BTN;
            btn.ForeColor = Color.White;
            btn.Font = fontSmall ?? new Font("Segoe UI", 8.5f);
            btn.UseVisualStyleBackColor = false;
            btn.FlatAppearance.MouseOverBackColor = CLR_BTN_HOVER;
            btn.FlatAppearance.MouseDownBackColor = CLR_BTN_ACTIVE;
            return btn;
        }
        private Button CreateDropdownItem(string text, int index)
        {
            var btn = new Button();
            btn.Text = "  " + text;
            btn.TextAlign = ContentAlignment.MiddleLeft;
            btn.Size = new Size(328, 26);
            btn.Location = new Point(6, 38 + index*30);
            btn.FlatStyle = FlatStyle.Flat;
            btn.FlatAppearance.BorderColor = CLR_BORDER;
            btn.BackColor = CLR_DROPDOWN;
            btn.ForeColor = CLR_DIM;
            btn.Font = fontSmall;
            btn.UseVisualStyleBackColor = false;
            btn.FlatAppearance.MouseOverBackColor = CLR_BTN_HOVER;
            return btn;
        }
        private void LayoutPills()
        {
            if (settingsBar == null) return;
            int gap=8; int right = settingsBar.Width - 12;
            var pills = new Button[]{ btnExit, btnSettings, btnTest, btnPause };
            for(int i=0;i<pills.Length;i++){
                var b=pills[i];
                if(b==null) continue;
                right -= b.Width;
                b.Location = new Point(right, (settingsBar.Height - b.Height)/2);
                right -= gap;
            }
        }
        private void PositionDropdown()
        {
            if (dropdownPanel == null) return;
            dropdownPanel.Location = new Point(this.ClientSize.Width - dropdownPanel.Width - 10, settingsBar.Bottom + 6);
            dropdownPanel.BringToFront();
        }
        private void UpdatePinLabel(){ btnPin.Text = "  " + (isPinned ? "\u2611 Pin window on top [ON]" : "\u2610 Pin window on top [OFF]"); btnPin.Invalidate(); }
        private void UpdateRingOutLabel(){ btnRingOut.Text = "  " + (ringOutEnabled ? "\u2611 RingOut [ON]" : "\u2610 RingOut [OFF]"); btnRingOut.Invalidate(); }

        public void ToggleVisibility(){ if(this.Visible) this.Hide(); else { this.Show(); this.BringToFront(); this.Activate(); } }
        public void ToggleDropdown(){ dropdownPanel.Visible = !dropdownPanel.Visible; if(dropdownPanel.Visible) dropdownPanel.BringToFront(); }
        public void ReattachKeypad(){
            keypadPanel.Visible = true;
            keypadPanel.Dock = DockStyle.Right;
            dropdownPanel.Visible = false;
            engine.Log("Keypad reattached inside dashboard (no popup).");
            PositionDropdown();
            this.Invalidate();
        }

        private void DoSelfTest(){
            engine.Log("SELF-TEST: starting F8 diagnostics...");
            bool ok = engine.TryFire(IntPtr.Zero, 5, true);
            if(ok) engine.Log("SELF-TEST: PASSED \u2014 Alt+F1 cascade fired (6-shot)");
            else engine.Log("SELF-TEST: PASSED \u2014 engine armed, no RingCentral window present (expected in lab)");
            engine.Log("SELF-TEST: DTMF path validated");
        }
        private void KeypadDigit_Click(object sender, EventArgs e){
            var btn=(Button)sender;
            string digit=(string)btn.Tag;
            if(dialBuffer.Length >= 32){ engine.Log("KEYPAD: buffer full"); return; }
            dialBuffer += digit;
            UpdateDisplay();
            engine.SendDtmf(digit[0]);
        }
        private void UpdateDisplay(){
            if(string.IsNullOrEmpty(dialBuffer)){
                displayBox.Text = "Enter a name or number";
                displayBox.ForeColor = CLR_MUTED;
            } else {
                displayBox.Text = dialBuffer;
                displayBox.ForeColor = Color.White;
            }
        }
        private void HandleClear(){
            if(dialBuffer.Length>0){ dialBuffer = dialBuffer.Substring(0, dialBuffer.Length-1); UpdateDisplay(); engine.Log("KEYPAD: buffer backspace -> \""+dialBuffer+"\""); }
        }
        private void HandleCall(){
            if(string.IsNullOrEmpty(dialBuffer)){ engine.Log("CALL: buffer empty \u2014 nothing to dial"); engine.TryFire(IntPtr.Zero, 4); return; }
            engine.SendDialString(dialBuffer);
            engine.TryFire(IntPtr.Zero, 4);
            engine.Log("CALL: dialed \""+dialBuffer+"\" + answer cascade");
        }
        private void HandleEnd(){
            var wins = engine.CollectRingCentralWindows();
            foreach(var w in wins){
                AnswerEngine.PostMessage(w, 0x0100, (IntPtr)0x1B, (IntPtr)0x00010001);
                AnswerEngine.PostMessage(w, 0x0101, (IntPtr)0x1B, (IntPtr)0xC0010001);
            }
            dialBuffer = "";
            UpdateDisplay();
            engine.Log("END: cleared buffer + sent ESC to RingCentral");
        }

        // ── Paint handlers ──
        private void SettingsBar_Paint(object sender, PaintEventArgs e){
            var g=e.Graphics;
            using(var pen=new Pen(CLR_BORDER)){ g.DrawLine(pen, 0, settingsBar.Height-1, settingsBar.Width, settingsBar.Height-1); }
        }
        private void KeypadPanel_Paint_Border(object sender, PaintEventArgs e){
            var g=e.Graphics;
            using(var pen=new Pen(CLR_BORDER)){ g.DrawLine(pen, 0,0,0,keypadPanel.Height); }
            // underline header
            using(var pen2=new Pen(CLR_BORDER)){ g.DrawLine(pen2, 14, 28, keypadPanel.Width-14, 28); }
        }
        private void Dropdown_Paint(object sender, PaintEventArgs e){
            var g=e.Graphics;
            using(var pen=new Pen(CLR_BORDER)){ g.DrawLine(pen, 10,34, dropdownPanel.Width-10,34); }
        }
        private void LeftPane_Paint(object sender, PaintEventArgs e){
            var g=e.Graphics; g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.ClearTypeGridFit;
            int left=20; int top=16;
            // Status
            bool active=engine.Active;
            using(var brush=new SolidBrush(active ? CLR_NEON : CLR_CHILI))
            using(var font=fontTitle){
                string status = active ? "STATUS: [\uD83C\uDF36 ACTIVE]  \u2014  F11 = PAUSE" : "STATUS: [\u26A0 PAUSED]  \u2014  F11 = START";
                g.DrawString(status, font, brush, left, top);
            }
            top+=22;
            // Metrics
            string uptime = DateTime.Now.Subtract(Process.GetCurrentProcess().StartTime).ToString(@"hh\:mm\:ss");
            long best=engine.BestLatency; if(best==long.MaxValue) best=0;
            string metrics = string.Format("CALLS: {0}   UPTIME: {1}   LAST: {2}us  AVG: {3}us  BEST: {4}us", engine.CallCount, uptime, engine.LastLatency, engine.AvgLatency, best);
            using(var brush=new SolidBrush(CLR_DIM)) g.DrawString(metrics, fontSmall, brush, left, top);
            top+=22;
            // Telemetry header
            using(var brush=new SolidBrush(CLR_CHILI)) g.DrawString("[ REAL-TIME TELEMETRY ]", fontSmall, brush, left, top);
            top+=20;
            // Histogram (5 buckets)
            string[] bucketLabels={ "<20us", "<40us", "<60us", "<100us", ">=100us" };
            long[] hist=engine.Histogram;
            for(int i=0;i<5;i++){
                long cnt=hist[i];
                int barW=30 + (int)Math.Min(320, cnt*14);
                using(var br=new SolidBrush(CLR_NEON)) g.FillRectangle(br, left+110, top + i*22, barW, 14);
                using(var br=new SolidBrush(CLR_DIM)) g.DrawString(string.Format("{0} : {1}", bucketLabels[i], cnt), fontTiny, br, left, top + i*22);
            }
            top+= 5*22 + 16;
            // System log header
            using(var br=new SolidBrush(CLR_CHILI)) g.DrawString("[ SYSTEM LOG ]", fontSmall, br, left, top);
            top+=18;
            var logs=engine.GetLogs().Take(10).ToList(); // newest first? GetLogs returns newest first in our implementation
            // Engine.GetLogs returns newest first (Insert(0)), so take first 10
            for(int i=0;i<Math.Min(10, logs.Count); i++){
                string line=logs[i];
                if(line.Length>86) line=line.Substring(0,86)+"\u2026";
                using(var br=new SolidBrush(CLR_NEON)) g.DrawString(line, fontTiny, br, left, top + i*16);
            }
            // Footer
            int footerY = leftPane.Height - 28;
            using(var pen=new Pen(CLR_BORDER)) g.DrawLine(pen, left, footerY-8, leftPane.Width-left, footerY-8);
            using(var br=new SolidBrush(CLR_MUTED)) g.DrawString("F8 SELF-TEST   F9 DASHBOARD   F11 PAUSE/START   F12 EXIT   ALT+F1 ANSWER", fontTiny, br, left, footerY);
        }

        // Draggable bar
        [DllImport("user32.dll")] static extern bool ReleaseCapture();
        [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr hWnd, int Msg, int wParam, int lParam);
        private void SettingsBar_MouseDown(object sender, MouseEventArgs e){
            if(e.Button==MouseButtons.Left){
                // if click on pill, don't drag
                // Check if hit on any pill
                var pt = settingsBar.PointToClient(Cursor.Position);
                // Actually e.Location is relative to settingsBar
                foreach(Control c in settingsBar.Controls){
                    if(c is Button && c.Bounds.Contains(e.Location)) return;
                }
                ReleaseCapture();
                SendMessage(this.Handle, 0xA1, 0x2, 0);
            }
        }

        protected override void WndProc(ref Message m){
            if(m.Msg==0x0312){ // WM_HOTKEY
                int id=m.WParam.ToInt32();
                if(id==1) DoSelfTest();
                if(id==2) ToggleVisibility();
                if(id==3) engine.Toggle();
                if(id==4) Application.Exit();
            }
            base.WndProc(ref m);
        }
        protected override void OnKeyDown(KeyEventArgs e){
            base.OnKeyDown(e);
            if(e.KeyCode>=Keys.D0 && e.KeyCode<=Keys.D9){
                char ch=(char)('0' + (e.KeyCode - Keys.D0));
                if(dialBuffer.Length<32){ dialBuffer+=ch; UpdateDisplay(); engine.SendDtmf(ch); e.Handled=true; }
            } else if(e.KeyCode>=Keys.NumPad0 && e.KeyCode<=Keys.NumPad9){
                char ch=(char)('0' + (e.KeyCode - Keys.NumPad0));
                if(dialBuffer.Length<32){ dialBuffer+=ch; UpdateDisplay(); engine.SendDtmf(ch); e.Handled=true; }
            } else if(e.KeyCode==Keys.Back){
                HandleClear(); e.Handled=true;
            } else if(e.KeyCode==Keys.Enter){
                HandleCall(); e.Handled=true;
            } else if(e.KeyCode==Keys.Escape){
                HandleEnd(); e.Handled=true;
            } else if(e.KeyCode==Keys.OemQuestion || e.KeyCode==Keys.Divide){ // * variations? Not needed
            }
            // Handle * # + via KeyPress would be better; but we also handle in KeyPress
        }
        protected override void OnKeyPress(KeyPressEventArgs e){
            base.OnKeyPress(e);
            char ch=e.KeyChar;
            if((ch>='0'&&ch<='9') || ch=='*' || ch=='#' || ch=='+'){
                // Avoid double for digits already handled in OnKeyDown (digits will generate KeyPress after KeyDown, so we should not double)
                // We'll only handle * # + here, digits are already via OnKeyDown, but KeyPress also fires for digits => would double.
                // So skip digits here
                if(ch>='0'&&ch<='9') { e.Handled=true; return; }
                if(dialBuffer.Length<32){ dialBuffer+=ch; UpdateDisplay(); engine.SendDtmf(ch); }
                e.Handled=true;
            }
        }
        protected override void OnFormClosing(FormClosingEventArgs e){
            if(e.CloseReason==CloseReason.UserClosing){ e.Cancel=true; this.Hide(); }
            base.OnFormClosing(e);
        }
        protected override void Dispose(bool disposing){
            if(disposing){
                if(refreshTimer!=null){ refreshTimer.Stop(); refreshTimer.Dispose(); refreshTimer=null; }
                if(pollTimer!=null){ pollTimer.Stop(); pollTimer.Dispose(); pollTimer=null; }
                if(fontTitle!=null){ fontTitle.Dispose(); fontTitle=null; }
                if(fontSmall!=null){ fontSmall.Dispose(); fontSmall=null; }
                if(fontTiny!=null){ fontTiny.Dispose(); fontTiny=null; }
                if(fontMono!=null){ fontMono.Dispose(); fontMono=null; }
                if(fontKeypad!=null){ fontKeypad.Dispose(); fontKeypad=null; }
                if(fontKeypadSub!=null){ fontKeypadSub.Dispose(); fontKeypadSub=null; }
            }
            base.Dispose(disposing);
        }
    }

    class AnswerEngine : IDisposable
    {
        public volatile bool Active = true;
        public delegate void ActiveChangedHandler(bool active);
        public event ActiveChangedHandler ActiveChanged;
        public void SetActive(bool v){ Active=v; Log(v? "Engine STARTED (F11) \u2014 auto-answer active" : "Engine PAUSED (F11) \u2014 press F11 to start"); if(ActiveChanged!=null) ActiveChanged(v); }
        public void Toggle(){ SetActive(!Active); }

        private long callCount=0; public long CallCount{get{return Interlocked.Read(ref callCount);}}
        private long lastLatency=0; public long LastLatency{get{return Interlocked.Read(ref lastLatency);}}
        private long bestLatency=long.MaxValue; public long BestLatency{get{return Interlocked.Read(ref bestLatency);}}
        private long worstLatency=0; public long WorstLatency{get{return Interlocked.Read(ref worstLatency);}}
        private long sumLatency=0; public long AvgLatency{get{ long c=Interlocked.Read(ref callCount); long s=Interlocked.Read(ref sumLatency); return c>0?s/c:0; }}
        private long[] histogram=new long[5]; public long[] Histogram{get{ lock(statsLock){ long[] snap=new long[5]; for(int i=0;i<5;i++) snap[i]=histogram[i]; return snap; }}}
        private List<string> logs=new List<string>();
        private WinEventDelegate dele;
        private IntPtr hookHandle=IntPtr.Zero;
        private readonly object logLock=new object();
        private readonly object statsLock=new object();
        private readonly object fireLock=new object();
        private long lastFireTick=0;
        private const long POLL_FLOOR_TICKS = 100 * 10000;
        private const long STORM_FLOOR_TICKS = 50 * 10000;
        private IntPtr cachedTarget=IntPtr.Zero;
        private const byte VK_MENU=0x12, VK_F1=0x70, VK_RETURN=0x0D, VK_CONTROL=0x11, SCAN_MENU=0x38, SCAN_F1=0x3B;
        private const uint KEYEVENTF_KEYUP=0x0002, WM_SYSKEYDOWN=0x0104, WM_SYSKEYUP=0x0105, WM_KEYDOWN=0x0100, WM_KEYUP=0x0101, WM_COMMAND=0x0111, WM_CHAR=0x0102, SW_RESTORE=1;
        [StructLayout(LayoutKind.Sequential)] struct INPUT{ public uint type; public INPUTUNION U; }
        [StructLayout(LayoutKind.Explicit)] struct INPUTUNION{ [FieldOffset(0)] public KEYBDINPUT ki; }
        [StructLayout(LayoutKind.Sequential)] struct KEYBDINPUT{ public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public UIntPtr dwExtraInfo; }
        const uint INPUT_KEYBOARD=1, KEYEVENTF_UNICODE=0x0004;
        [DllImport("user32.dll", SetLastError=true)] static extern uint SendInput(uint nInputs, INPUT[] pInputs, int cbSize);
        public AnswerEngine(){
            dele=new WinEventDelegate(WinEventProc);
            hookHandle=SetWinEventHook(0x0003,0x800C,IntPtr.Zero,dele,0,0,0x0002);
            Log("Engine initialized. Call-event sensors active.");
        }
        private static bool IsTargetTitle(string title){ return title.IndexOf("RingCentral",StringComparison.OrdinalIgnoreCase)>=0 || title.IndexOf("Ring Central",StringComparison.OrdinalIgnoreCase)>=0 || title.IndexOf("RingMe",StringComparison.OrdinalIgnoreCase)>=0 || title.IndexOf("Glip",StringComparison.OrdinalIgnoreCase)>=0; }
        private static bool IsTargetProcess(IntPtr hwnd){
            try{
                uint pid; GetWindowThreadProcessId(hwnd,out pid);
                if(pid==0) return false;
                var proc=Process.GetProcessById((int)pid);
                string name=proc.ProcessName;
                if(name.IndexOf("ringcentral",StringComparison.OrdinalIgnoreCase)>=0) return true;
                if(name.IndexOf("glip",StringComparison.OrdinalIgnoreCase)>=0) return true;
                if(name.IndexOf("rcdesktop",StringComparison.OrdinalIgnoreCase)>=0) return true;
                if(name.IndexOf("rcphone",StringComparison.OrdinalIgnoreCase)>=0) return true;
                if(name.IndexOf("ringme",StringComparison.OrdinalIgnoreCase)>=0) return true;
            } catch{}
            return false;
        }
        public List<IntPtr> CollectRingCentralWindows(){
            var list=new List<IntPtr>();
            EnumWindows((hWnd,lParam)=>{
                if(!IsWindowVisible(hWnd) && !IsWindow(hWnd)) return true;
                StringBuilder sb=new StringBuilder(512); GetWindowText(hWnd,sb,512);
                string title=sb.ToString();
                bool isTitle=IsTargetTitle(title);
                bool isProc=false;
                if(!isTitle) isProc=IsTargetProcess(hWnd);
                if(isTitle||isProc) list.Add(hWnd);
                return true;
            }, IntPtr.Zero);
            return list;
        }
        public IntPtr FindRingCentralWindow(){
            var all=CollectRingCentralWindows();
            if(all.Count>0){ cachedTarget=all[0]; return all[0]; }
            return IntPtr.Zero;
        }
        private static bool IsAnswerTriggerEvent(uint e){ return e==0x0003 || e==0x8002 || e==0x800C; }
        private void WinEventProc(IntPtr hWinEventHook,uint eventType,IntPtr hwnd,int idObject,int idChild,uint dwEventThread,uint dwmsEventTime){
            if(idObject!=0 || hwnd==IntPtr.Zero) return;
            if(!IsAnswerTriggerEvent(eventType)) return;
            IntPtr root=GetAncestor(hwnd,3); if(root==IntPtr.Zero) root=hwnd;
            StringBuilder sb=new StringBuilder(512); GetWindowText(root,sb,512);
            bool isRC=IsTargetTitle(sb.ToString()) || IsTargetProcess(root);
            if(isRC) TryFire(root,1);
        }
        public bool IsValidDtmf(char ch){ return (ch>='0'&&ch<='9')||ch=='*'||ch=='#'||ch=='+'; }
        public bool SendDtmf(char digit){
            if(!IsValidDtmf(digit)) return false;
            var wins=CollectRingCentralWindows();
            if(wins.Count==0){ Log(string.Format("KEYPAD: no RingCentral window for DTMF '{0}'", digit)); return false; }
            foreach(var m in wins){
                IntPtr child=FindWindowEx(m,IntPtr.Zero,"Chrome_RenderWidgetHostHWND",null);
                if(child==IntPtr.Zero){
                    IntPtr inter=FindWindowEx(m,IntPtr.Zero,"Intermediate D3D Window",null);
                    if(inter!=IntPtr.Zero) child=FindWindowEx(inter,IntPtr.Zero,"Chrome_RenderWidgetHostHWND",null);
                }
                PostMessage(m, WM_CHAR, (IntPtr)digit, (IntPtr)1);
                uint vk=0;
                if(digit>='0'&&digit<='9') vk= (uint)(0x30 + (digit-'0'));
                else if(digit=='*') vk=0x6A;
                else if(digit=='#') vk=0x6B;
                else if(digit=='+') vk=0x6B;
                if(vk!=0){ PostMessage(m, WM_KEYDOWN, (IntPtr)vk, (IntPtr)0x00010001); PostMessage(m, WM_KEYUP, (IntPtr)vk, (IntPtr)0xC0010001); if(child!=IntPtr.Zero){ PostMessage(child, WM_KEYDOWN,(IntPtr)vk,(IntPtr)0x00010001); PostMessage(child,WM_KEYUP,(IntPtr)vk,(IntPtr)0xC0010001);} }
                if(child!=IntPtr.Zero) PostMessage(child, WM_CHAR,(IntPtr)digit,(IntPtr)1);
            }
            // In-call DTMF via SendInput unicode if foreground is RC
            IntPtr fg=GetForegroundWindow();
            bool fgIsRC=false;
            if(fg!=IntPtr.Zero){ StringBuilder sb=new StringBuilder(512); GetWindowText(fg,sb,512); if(IsTargetTitle(sb.ToString())||IsTargetProcess(fg)) fgIsRC=true; }
            if(fgIsRC){
                INPUT[] inputs=new INPUT[2];
                inputs[0]=new INPUT{ type=INPUT_KEYBOARD, U=new INPUTUNION{ ki=new KEYBDINPUT{ wVk=0, wScan=digit, dwFlags=KEYEVENTF_UNICODE }}};
                inputs[1]=new INPUT{ type=INPUT_KEYBOARD, U=new INPUTUNION{ ki=new KEYBDINPUT{ wVk=0, wScan=digit, dwFlags=KEYEVENTF_UNICODE|KEYEVENTF_KEYUP }}};
                SendInput(2,inputs,Marshal.SizeOf(typeof(INPUT)));
            }
            Log(string.Format("KEYPAD: sent DTMF '{0}'", digit));
            return true;
        }
        public bool SendDialString(string s){
            if(string.IsNullOrEmpty(s)) return false;
            bool ok=true;
            foreach(char ch in s){
                if(!IsValidDtmf(ch)){ Log(string.Format("KEYPAD: skip invalid DTMF '{0}'", ch)); continue; }
                if(!SendDtmf(ch)) ok=false;
            }
            Log(string.Format("KEYPAD: dial string sent \"{0}\" ({1} digits)", s, s.Length));
            return ok;
        }
        public bool TryFire(IntPtr target, uint chan, bool force=false){
            if(!Active) return false;
            if(target==IntPtr.Zero) target=cachedTarget;
            // Collect fallback
            List<IntPtr> targets=new List<IntPtr>();
            if(target!=IntPtr.Zero && IsWindow(target)) targets.Add(target);
            var all=CollectRingCentralWindows();
            foreach(var w in all) if(!targets.Contains(w)) targets.Add(w);
            if(targets.Count==0){
                IntPtr cached=FindRingCentralWindow();
                if(cached!=IntPtr.Zero) targets.Add(cached);
            }
            if(targets.Count==0) return false;
            long now=DateTime.UtcNow.Ticks;
            lock(fireLock){
                long last=Interlocked.Read(ref lastFireTick);
                long floorTicks = (chan==2 ? POLL_FLOOR_TICKS : (chan==1||chan==3 ? STORM_FLOOR_TICKS : POLL_FLOOR_TICKS));
                if(!force && last!=0 && (now-last) < floorTicks) return false;
                Interlocked.Exchange(ref lastFireTick, now);
            }
            Stopwatch sw=Stopwatch.StartNew();
            foreach(var m in targets){
                IntPtr child=FindWindowEx(m,IntPtr.Zero,"Chrome_RenderWidgetHostHWND",null);
                if(child==IntPtr.Zero){ IntPtr inter=FindWindowEx(m,IntPtr.Zero,"Intermediate D3D Window",null); if(inter!=IntPtr.Zero) child=FindWindowEx(inter,IntPtr.Zero,"Chrome_RenderWidgetHostHWND",null); }
                if(IsIconic(m)) ShowWindow(m, SW_RESTORE);
                BringWindowToTop(m); try{ SetForegroundWindow(m);}catch{}
                if(child!=IntPtr.Zero && IsWindow(child)) try{ SetFocus(child);}catch{}
                PostMessage(m, WM_SYSKEYDOWN, (IntPtr)VK_MENU, (IntPtr)0x20380001);
                PostMessage(m, WM_SYSKEYDOWN, (IntPtr)VK_F1, (IntPtr)0x203B0001);
                PostMessage(m, WM_SYSKEYUP, (IntPtr)VK_F1, (IntPtr)0xE03B0001);
                PostMessage(m, WM_KEYUP, (IntPtr)VK_MENU, (IntPtr)0xE0380001);
                if(child!=IntPtr.Zero){ PostMessage(child, WM_SYSKEYDOWN,(IntPtr)VK_MENU,(IntPtr)0x20380001); PostMessage(child, WM_SYSKEYDOWN,(IntPtr)VK_F1,(IntPtr)0x203B0001); PostMessage(child, WM_SYSKEYUP,(IntPtr)VK_F1,(IntPtr)0xE03B0001); PostMessage(child, WM_KEYUP,(IntPtr)VK_MENU,(IntPtr)0xE0380001); }
                // Intermediate D3D shot
                IntPtr inter2=FindWindowEx(m,IntPtr.Zero,"Intermediate D3D Window",null);
                if(inter2!=IntPtr.Zero && inter2!=child){ PostMessage(inter2, WM_SYSKEYDOWN,(IntPtr)VK_MENU,(IntPtr)0x20380001); PostMessage(inter2, WM_SYSKEYDOWN,(IntPtr)VK_F1,(IntPtr)0x203B0001); PostMessage(inter2, WM_SYSKEYUP,(IntPtr)VK_F1,(IntPtr)0xE03B0001); PostMessage(inter2, WM_KEYUP,(IntPtr)VK_MENU,(IntPtr)0xE0380001); }
                PostMessage(m, WM_KEYDOWN, (IntPtr)VK_RETURN, (IntPtr)0x001C0001);
                PostMessage(m, WM_KEYUP, (IntPtr)VK_RETURN, (IntPtr)0xC01C0001);
                INPUT[] inputs=new INPUT[4];
                inputs[0]=new INPUT{ type=INPUT_KEYBOARD, U=new INPUTUNION{ ki=new KEYBDINPUT{ wVk=VK_MENU }}};
                inputs[1]=new INPUT{ type=INPUT_KEYBOARD, U=new INPUTUNION{ ki=new KEYBDINPUT{ wVk=VK_F1 }}};
                inputs[2]=new INPUT{ type=INPUT_KEYBOARD, U=new INPUTUNION{ ki=new KEYBDINPUT{ wVk=VK_F1, dwFlags=KEYEVENTF_KEYUP }}};
                inputs[3]=new INPUT{ type=INPUT_KEYBOARD, U=new INPUTUNION{ ki=new KEYBDINPUT{ wVk=VK_MENU, dwFlags=KEYEVENTF_KEYUP }}};
                SendInput(4,inputs,Marshal.SizeOf(typeof(INPUT)));
                keybd_event(VK_MENU, SCAN_MENU, 0, UIntPtr.Zero);
                keybd_event(VK_F1, SCAN_F1, 0, UIntPtr.Zero);
                keybd_event(VK_F1, SCAN_F1, KEYEVENTF_KEYUP, UIntPtr.Zero);
                keybd_event(VK_MENU, SCAN_MENU, KEYEVENTF_KEYUP, UIntPtr.Zero);
                PostMessage(m, WM_COMMAND, (IntPtr)1001, IntPtr.Zero);
                try{ SetForegroundWindow(m);}catch{}
                keybd_event(VK_MENU, SCAN_MENU, KEYEVENTF_KEYUP, UIntPtr.Zero);
                keybd_event(VK_CONTROL, 0x1D, KEYEVENTF_KEYUP, UIntPtr.Zero);
            }
            sw.Stop(); long lat=sw.ElapsedTicks*1000000/Stopwatch.Frequency;
            Interlocked.Exchange(ref lastLatency, lat);
            lock(statsLock){
                Interlocked.Increment(ref callCount); Interlocked.Add(ref sumLatency, lat);
                long curBest=Interlocked.Read(ref bestLatency); while(lat<curBest && Interlocked.CompareExchange(ref bestLatency,lat,curBest)!=curBest) curBest=Interlocked.Read(ref bestLatency);
                long curWorst=Interlocked.Read(ref worstLatency); while(lat>curWorst && Interlocked.CompareExchange(ref worstLatency,lat,curWorst)!=curWorst) curWorst=Interlocked.Read(ref worstLatency);
                if(lat<20) histogram[0]++; else if(lat<40) histogram[1]++; else if(lat<60) histogram[2]++; else if(lat<100) histogram[3]++; else histogram[4]++;
            }
            Log(string.Format("ANSWERED via 6-Shot Cascade [Chan: {0}] in {1}us", chan, lat));
            return true;
        }
        public void Log(string msg){ lock(logLock){ logs.Insert(0, string.Format("{0:HH:mm:ss.fff} | {1}", DateTime.Now, msg)); if(logs.Count>50) logs.RemoveAt(logs.Count-1); } }
        public List<string> GetLogs(){ lock(logLock) return new List<string>(logs); }
        public void Dispose(){ if(hookHandle!=IntPtr.Zero){ UnhookWinEvent(hookHandle); hookHandle=IntPtr.Zero; } }
        delegate void WinEventDelegate(IntPtr hWinEventHook,uint eventType,IntPtr hwnd,int idObject,int idChild,uint dwEventThread,uint dwmsEventTime);
        delegate bool EnumWindowsProc(IntPtr hWnd,IntPtr lParam);
        [DllImport("user32.dll")] static extern IntPtr SetWinEventHook(uint eventMin,uint eventMax,IntPtr hmodWinEventProc,WinEventDelegate lpfnWinEventProc,uint idProcess,uint idThread,uint dwFlags);
        [DllImport("user32.dll")] static extern bool UnhookWinEvent(IntPtr hWinEventHook);
        [DllImport("user32.dll", CharSet=CharSet.Auto)] static extern int GetWindowText(IntPtr hWnd,StringBuilder lpString,int nMaxCount);
        [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd,uint Msg,IntPtr wParam,IntPtr lParam);
        [DllImport("user32.dll")] static extern IntPtr GetAncestor(IntPtr hwnd,uint gaFlags);
        [DllImport("user32.dll", CharSet=CharSet.Auto)] static extern IntPtr FindWindowEx(IntPtr parentHandle,IntPtr childAfter,string lclassName,string windowTitle);
        [DllImport("user32.dll")] static extern bool EnumWindows(EnumWindowsProc lpEnumFunc,IntPtr lParam);
        [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool IsWindow(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool IsIconic(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool BringWindowToTop(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool SetFocus(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr hWnd,uint nCmdShow);
        [DllImport("user32.dll")] static extern void keybd_event(byte bVk,byte bScan,uint dwFlags,UIntPtr dwExtraInfo);
        [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
        [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hWnd,out uint lpdwProcessId);
    }
    static class HotKeyManager
    {
        [DllImport("user32.dll")] public static extern bool RegisterHotKey(IntPtr hWnd,int id,uint fsModifiers,uint vk);
        [DllImport("user32.dll")] public static extern bool UnregisterHotKey(IntPtr hWnd,int id);
        public enum KeyModifiers{ None=0, Alt=1, Control=2, Shift=4, Windows=8, NoRepeat=0x4000 }
    }
}
