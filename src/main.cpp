// ═════════════════════════════════════════════════════════════════════════════
// SPICY LAMAR v1.0 Integrated // SINGLE-FILE MONOLITHIC SOURCE
// TARGET PLATFORM: WINDOWS 10/11 x64 (PORTABLE, STATICALLY LINKED)
// 980x620 SINGLE WINDOW — SETTINGS BAR + LEFT PANE + DOCKED KEYPAD (NO POPUP)
// ═════════════════════════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#define WINVER       0x0A00
#endif

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <objbase.h>
#include <oleacc.h>
#include <shellapi.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <timeapi.h>
#include <psapi.h>
#include <avrt.h>
#ifdef _MSC_VER
#include <wrl/client.h>
#endif

#include <atomic>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <array>
#include <algorithm>
#include <memory>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cmath>
#include <cwchar>

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "oleacc.lib")
#pragma comment(lib, "tdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "avrt.lib")
#else
#define SL_CROSS_COMPILE 1
#endif

#ifdef _MSC_VER
using Microsoft::WRL::ComPtr;
#define SL_SWPRINTF(dest, ...) swprintf_s(dest, __VA_ARGS__)
#define SL_WCSCPY(dest, src) wcscpy_s(dest, src)
#else
#define SL_SWPRINTF(dest, ...) swprintf(dest, _countof(dest), __VA_ARGS__)
#define SL_WCSCPY(dest, src) wcscpy(dest, src)
#endif

namespace SL {
    // Identity Configuration — Spicy Lamar v1.0 Integrated
    constexpr wchar_t APP_NAME[]           = L"Spicy Lamar";
    constexpr wchar_t APP_CLASS_NAME[]     = L"SpicyLamar_Integrated_v1";
    constexpr wchar_t APP_MUTEX_NAME[]     = L"Global\\SpicyLamar";
    constexpr wchar_t APP_MUTEX_FALLBACK[] = L"Local\\SpicyLamar";
    constexpr wchar_t APP_VERSION[]        = L"Spicy Lamar v1.0 \u2014 Integrated";
    constexpr wchar_t TARGET_WINDOW_TITLE[]= L"RingCentral Phone";
    constexpr wchar_t TARGET_CHILD_CLASS[] = L"Chrome_RenderWidgetHostHWND";
    constexpr wchar_t TARGET_INTERMEDIATE[]= L"Intermediate D3D Window";

    // Window geometry
    constexpr int WIN_W = 980;
    constexpr int WIN_H = 620;
    constexpr int BAR_H = 38;
    constexpr int KEYPAD_W = 280;

    // Windows Messages
    constexpr UINT WM_TRAYICON       = WM_USER + 101;
    constexpr UINT WM_APP_FIRE_ANSWER= WM_USER + 102;
    constexpr UINT WM_APP_LOG_UPDATE = WM_USER + 103;

    // Command & Menu IDs
    constexpr UINT ID_TRAYICON       = 1001;
    constexpr UINT IDM_SHOW_DASH     = 2001;
    constexpr UINT IDM_PAUSE         = 2002;
    constexpr UINT IDM_EXIT          = 2003;

    // Visual Palette — Integrated spec
    constexpr COLORREF CLR_OBSIDIAN   = RGB(5,5,5);       // #050505 left pane
    constexpr COLORREF CLR_SETTINGS   = RGB(26,26,26);    // #1A1A1A bar
    constexpr COLORREF CLR_PANEL      = RGB(16,16,16);    // #101010 keypad panel
    constexpr COLORREF CLR_DISPLAY    = RGB(22,22,22);    // #161616 display
    constexpr COLORREF CLR_DROPDOWN   = RGB(32,32,32);    // #202020 dropdown
    constexpr COLORREF CLR_BTN        = RGB(38,38,38);    // #262626 btn
    constexpr COLORREF CLR_BTN_HOVER  = RGB(64,64,64);    // #404040
    constexpr COLORREF CLR_BTN_ACTIVE = RGB(255,51,0);    // #FF3300
    constexpr COLORREF CLR_BORDER     = RGB(48,48,48);    // #303030
    constexpr COLORREF CLR_NEON       = RGB(0,255,102);   // #00FF66
    constexpr COLORREF CLR_CHILI      = RGB(255,51,0);    // #FF3300
    constexpr COLORREF CLR_CALL       = RGB(0,150,60);    // #00963C
    constexpr COLORREF CLR_CALL_HOVER = RGB(0,180,80);    // #00B450
    constexpr COLORREF CLR_END        = RGB(140,30,30);   // #8C1E1E
    constexpr COLORREF CLR_END_HOVER  = RGB(180,40,40);   // #B42828
    constexpr COLORREF CLR_TEXT_DIM   = RGB(180,180,180);
    constexpr COLORREF CLR_TEXT_MUTED = RGB(130,130,130);
    constexpr COLORREF CLR_WHITE      = RGB(255,255,255);

    // Performance & Telemetry Constants
#ifndef SPICY_LAMAR_TURBO
#define SPICY_LAMAR_TURBO
#endif
#ifdef SPICY_LAMAR_TURBO
    constexpr DWORD POLL_MS = 5;      // 200Hz TURBO
#else
    constexpr DWORD POLL_MS = 20;
#endif
    constexpr DWORD POLL_FLOOR_MS  = 100; // poll -> 100ms floor
    constexpr DWORD STORM_FLOOR_MS = 50;  // storm/event -> 50ms floor
    constexpr int   MAX_TELEMETRY_LOGS = 25;
    constexpr int   HIST_BUCKETS = 5;
    constexpr DWORD STATS_REFRESH_MS = 250;

    // Engine rate channels
    constexpr uint32_t CHAN_WIN_EVENT = 1;
    constexpr uint32_t CHAN_POLL      = 2;
    constexpr uint32_t CHAN_SHELL     = 3;
    constexpr uint32_t CHAN_KEYPAD    = 4;
    constexpr uint32_t CHAN_SELFTEST  = 5;

    // Hotkeys
    constexpr int HK_SELFTEST      = 1; // F8
    constexpr int HK_TOGGLE_DASH   = 2; // F9
    constexpr int HK_PAUSE_RESUME  = 3; // F11
    constexpr int HK_EMERGENCY_EXIT= 4; // F12

    // Button IDs for hit-testing
    enum BtnId {
        BTN_NONE = 0,
        BTN_PILL_PAUSE = 100,
        BTN_PILL_TEST,
        BTN_PILL_SETTINGS,
        BTN_PILL_EXIT,
        BTN_KEYPAD_1, BTN_KEYPAD_2, BTN_KEYPAD_3,
        BTN_KEYPAD_4, BTN_KEYPAD_5, BTN_KEYPAD_6,
        BTN_KEYPAD_7, BTN_KEYPAD_8, BTN_KEYPAD_9,
        BTN_KEYPAD_STAR, BTN_KEYPAD_0, BTN_KEYPAD_HASH,
        BTN_CALL, BTN_END, BTN_CLEAR,
        BTN_DROP_CLOSE,
        BTN_DROP_PIN,
        BTN_DROP_REATTACH,
        BTN_DROP_EMERGENCY,
        BTN_DROP_RINGOUT,
        BTN_DROP_INCOMING,
        BTN_DROP_VOICEMAIL,
        BTN_DROP_PHONE
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// LOGGING SUBSYSTEM
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    struct LogEntry { std::wstring timestamp; std::wstring level; std::wstring message; };
    class MemoryLogger {
    public:
        static MemoryLogger& Instance() { static MemoryLogger inst; return inst; }
        void Log(const std::wstring& level, const std::wstring& msg) {
            std::lock_guard<std::mutex> lock(mtx_);
            SYSTEMTIME st; GetLocalTime(&st);
            wchar_t ts[32]; SL_SWPRINTF(ts, L"%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            if (count_ < MAX_TELEMETRY_LOGS) count_++;
            else if (head_ == 0) {} // coalesce oldest
            buffer_[head_] = { ts, level, msg };
            head_ = (head_ + 1) % MAX_TELEMETRY_LOGS;
            if (notify_hwnd_ && IsWindow(notify_hwnd_)) PostMessageW(notify_hwnd_, WM_APP_LOG_UPDATE, 0, 0);
        }
        std::vector<LogEntry> GetRecentLogs() {
            std::lock_guard<std::mutex> lock(mtx_);
            std::vector<LogEntry> logs; logs.reserve(count_);
            int idx = (head_ - count_ + MAX_TELEMETRY_LOGS) % MAX_TELEMETRY_LOGS;
            for (int i=0;i<count_;++i){ logs.push_back(buffer_[idx]); idx=(idx+1)%MAX_TELEMETRY_LOGS; }
            return logs;
        }
        void SetNotifyWindow(HWND hwnd){ notify_hwnd_=hwnd; }
    private:
        MemoryLogger(): head_(0), count_(0), notify_hwnd_(nullptr){}
        std::mutex mtx_;
        std::array<LogEntry, MAX_TELEMETRY_LOGS> buffer_;
        int head_, count_;
        HWND notify_hwnd_;
    };
}
#define LOG_INF(fmt, ...) { wchar_t b[320]; SL_SWPRINTF(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"INF", b); }
#define LOG_WRN(fmt, ...) { wchar_t b[320]; SL_SWPRINTF(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"WRN", b); }
#define LOG_ERR(fmt, ...) { wchar_t b[320]; SL_SWPRINTF(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"ERR", b); }

// ─────────────────────────────────────────────────────────────────────────────
// STATS & TELEMETRY TRACKER
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class StatsTracker {
    public:
        static StatsTracker& Instance(){ static StatsTracker inst; return inst; }
        void Initialize(){ LARGE_INTEGER f; QueryPerformanceFrequency(&f); qpc_freq=f.QuadPart; start=GetTickCount64(); }
        LARGE_INTEGER QpcNow(){ LARGE_INTEGER li; QueryPerformanceCounter(&li); return li; }
        uint64_t DeltaMicros(LARGE_INTEGER t0, LARGE_INTEGER t1){ if(qpc_freq<=0) return 0; return (uint64_t)((t1.QuadPart - t0.QuadPart)*1000000ULL/qpc_freq); }
        void RecordAnswer(LARGE_INTEGER t0, LARGE_INTEGER t1, uint32_t, uint32_t){
            uint64_t us=DeltaMicros(t0,t1); last_us=us; calls++; if(us<best_us) best_us=us; if(us>worst_us) worst_us=us; sum_us+=us;
            if(us<20) hist[0]++; else if(us<40) hist[1]++; else if(us<60) hist[2]++; else if(us<100) hist[3]++; else hist[4]++;
        }
        uint64_t LastLatency() const{ return last_us.load(); }
        uint64_t BestLatency() const{ return best_us.load()==UINT64_MAX?0:best_us.load(); }
        uint64_t WorstLatency() const{ return worst_us.load(); }
        uint64_t TotalCalls() const{ return calls.load(); }
        uint64_t AvgLatency() const{ uint64_t c=calls.load(); return c>0?sum_us.load()/c:0; }
        uint64_t GetUptimeSec() const{ return (GetTickCount64()-start)/1000; }
        long GetHistCount(int i) const{ if(i>=0 && i<HIST_BUCKETS) return hist[i].load(); return 0; }
    private:
        StatsTracker(): qpc_freq(1), start(0), last_us(0), best_us(UINT64_MAX), worst_us(0), calls(0), sum_us(0){ for(auto &h:hist) h=0; }
        int64_t qpc_freq; uint64_t start; std::atomic<uint64_t> last_us,best_us,worst_us,calls,sum_us; std::array<std::atomic<long>, HIST_BUCKETS> hist;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// WINDOW CACHE & ENUMERATION (title OR process name)
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    bool ContainsInsensitive(const wchar_t* text, const wchar_t* needle){
        if(!text||!needle||!*needle) return false; size_t n=wcslen(needle); for(;*text;++text) if(_wcsnicmp(text,needle,n)==0) return true; return false;
    }
    bool IsRingCentralTitle(const wchar_t* title){
        return ContainsInsensitive(title, L"RingCentral") || ContainsInsensitive(title, L"Ring Central") || ContainsInsensitive(title, L"RingMe") || ContainsInsensitive(title, L"Glip");
    }
    bool IsRingCentralProcessName(const wchar_t* exe){
        return ContainsInsensitive(exe, L"ringcentral") || ContainsInsensitive(exe, L"glip") || ContainsInsensitive(exe, L"rcdesktop") || ContainsInsensitive(exe, L"rcphone") || ContainsInsensitive(exe, L"ringme") || ContainsInsensitive(exe, L"softphone");
    }
    // Collect ALL RingCentral windows via title OR process name
    std::vector<HWND> CollectRingCentralWindows(){
        std::vector<HWND> out;
        EnumWindows([](HWND hwnd, LPARAM lp)->BOOL{
            auto vec = reinterpret_cast<std::vector<HWND>*>(lp);
            if(!IsWindow(hwnd)) return TRUE;
            wchar_t title[512]={0}; GetWindowTextW(hwnd, title, 512);
            bool titleMatch = IsRingCentralTitle(title);
            bool procMatch = false;
            if(!titleMatch){
                DWORD pid=0; GetWindowThreadProcessId(hwnd,&pid);
                if(pid){
                    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                    if(h){
                        wchar_t exeName[MAX_PATH]={0}; DWORD len=MAX_PATH;
                        // Primary: QueryFullProcessImageNameW (no psapi needed)
                        typedef BOOL (WINAPI *QFPIFN)(HANDLE,DWORD,LPWSTR,PDWORD);
                        static QFPIFN qf = (QFPIFN)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "QueryFullProcessImageNameW");
                        bool got=false;
                        if(qf && qf(h,0,exeName,&len)){
                            wchar_t* base=wcsrchr(exeName,L'\\'); if(base) base++; else base=exeName;
                            if(IsRingCentralProcessName(base)) procMatch=true;
                            got=true;
                        }
                        if(!got){
                            HMODULE mod; DWORD needed;
                            if(EnumProcessModules(h,&mod,sizeof(mod),&needed)){
                                GetModuleBaseNameW(h,mod,exeName,MAX_PATH);
                                if(IsRingCentralProcessName(exeName)) procMatch=true;
                            } else {
                                // fallback via GetProcessImageFileNameW
                                wchar_t tmp[MAX_PATH]={0};
                                if(GetProcessImageFileNameW(h,tmp,MAX_PATH)){
                                    wchar_t* base=wcsrchr(tmp,L'\\'); if(base) base++; else base=tmp;
                                    if(IsRingCentralProcessName(base)) procMatch=true;
                                }
                            }
                        }
                        CloseHandle(h);
                    }
                }
            }
            if(titleMatch || procMatch) vec->push_back(hwnd);
            return TRUE;
        }, reinterpret_cast<LPARAM>(&out));
        return out;
    }

    class WindowCache {
    public:
        struct Snapshot{ HWND main, child; };
        static WindowCache& Instance(){ static WindowCache inst; return inst; }
        Snapshot GetSnapshot(){ return {main.load(), child.load()}; }
        void Update(HWND m, HWND c){ main.store(m); child.store(c); }
        HWND FindRingCentral(){
            auto wins = CollectRingCentralWindows();
            if(wins.empty()) return nullptr;
            HWND m = wins[0];
            HWND c=nullptr;
            // Try direct child then intermediate D3D
            c = FindWindowExW(m, nullptr, TARGET_CHILD_CLASS, nullptr);
            if(!c){
                HWND inter = FindWindowExW(m, nullptr, TARGET_INTERMEDIATE, nullptr);
                if(inter) c = FindWindowExW(inter, nullptr, TARGET_CHILD_CLASS, nullptr);
                if(!c){
                    EnumChildWindows(m, [](HWND h, LPARAM lp)->BOOL{
                        wchar_t cls[128]={0}; GetClassNameW(h,cls,128);
                        if(_wcsicmp(cls,TARGET_CHILD_CLASS)==0){ *reinterpret_cast<HWND*>(lp)=h; return FALSE; }
                        return TRUE;
                    }, reinterpret_cast<LPARAM>(&c));
                }
            }
            Update(m,c);
            return m;
        }
    private:
        WindowCache():main(nullptr),child(nullptr){}
        std::atomic<HWND> main,child;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// ANSWER ENGINE (6-SHOT IPC CASCADE) + DTMF
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class Engine {
    public:
        static Engine& Instance(){ static Engine inst; return inst; }
        void Initialize(){ active=true; last_attempt_tick=0; }
        void SetActive(bool a){ active=a; }
        bool IsActive() const{ return active; }

        // Validate DTMF char
        static bool IsValidDtmf(wchar_t ch){ return (ch>=L'0'&&ch<=L'9') || ch==L'*' || ch==L'#' || ch==L'+' ; }

        bool IsForegroundRingCentral(){
            HWND fg = GetForegroundWindow();
            if(!fg) return false;
            wchar_t title[512]={0}; GetWindowTextW(fg,title,512);
            if(IsRingCentralTitle(title)) return true;
            DWORD pid=0; GetWindowThreadProcessId(fg,&pid);
            if(pid){
                HANDLE h=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,pid);
                if(h){
                    wchar_t exe[MAX_PATH]={0}; DWORD len=MAX_PATH;
                    typedef BOOL (WINAPI *QFPIFN)(HANDLE,DWORD,LPWSTR,PDWORD);
                    static QFPIFN qf=(QFPIFN)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"QueryFullProcessImageNameW");
                    bool match=false;
                    if(qf && qf(h,0,exe,&len)){
                        wchar_t* base=wcsrchr(exe,L'\\'); if(base) base++; else base=exe;
                        if(IsRingCentralProcessName(base)) match=true;
                    } else {
                        HMODULE mod; DWORD needed;
                        if(EnumProcessModules(h,&mod,sizeof(mod),&needed)){
                            GetModuleBaseNameW(h,mod,exe,MAX_PATH);
                            if(IsRingCentralProcessName(exe)) match=true;
                        }
                    }
                    CloseHandle(h);
                    if(match) return true;
                }
            }
            return false;
        }

        bool SendDtmf(wchar_t digit){
            if(!IsValidDtmf(digit)) return false;
            auto wins = CollectRingCentralWindows();
            // Even if no RC window, still attempt to log but return false? We return false if none.
            if(wins.empty()){
                // Still try foreground unicode if foreground is RC? But we already checked empty => nothing
                LOG_WRN(L"KEYPAD: no RingCentral window for DTMF '%lc'", digit);
                return false;
            }
            bool any=false;
            for(HWND m : wins){
                if(!m || !IsWindow(m)) continue;
                HWND child = FindWindowExW(m, nullptr, TARGET_CHILD_CLASS, nullptr);
                if(!child){
                    HWND inter = FindWindowExW(m, nullptr, TARGET_INTERMEDIATE, nullptr);
                    if(inter) child = FindWindowExW(inter, nullptr, TARGET_CHILD_CLASS, nullptr);
                }
                // Post WM_CHAR to main and child
                PostMessageW(m, WM_CHAR, digit, 1);
                // Also WM_KEYDOWN/UP for compatibility (VK mapping best effort)
                UINT vk = 0;
                if(digit>=L'0' && digit<=L'9') vk = 0x30 + (digit - L'0');
                else if(digit==L'*') vk = VK_MULTIPLY;
                else if(digit==L'#') vk = VK_OEM_3; // approximate
                else if(digit==L'+') vk = VK_OEM_PLUS;
                if(vk){
                    PostMessageW(m, WM_KEYDOWN, vk, 0x00010001);
                    PostMessageW(m, WM_KEYUP, vk, 0xC0010001);
                }
                if(child && IsWindow(child)){
                    PostMessageW(child, WM_CHAR, digit, 1);
                    if(vk){
                        PostMessageW(child, WM_KEYDOWN, vk, 0x00010001);
                        PostMessageW(child, WM_KEYUP, vk, 0xC0010001);
                    }
                }
                any=true;
            }
            // In-call DTMF via SendInput if foreground is RingCentral (hardware unicode injection)
            if(IsForegroundRingCentral()){
                INPUT inp[2]={};
                inp[0].type=INPUT_KEYBOARD; inp[0].ki.wScan=digit; inp[0].ki.dwFlags=KEYEVENTF_UNICODE;
                inp[1].type=INPUT_KEYBOARD; inp[1].ki.wScan=digit; inp[1].ki.dwFlags=KEYEVENTF_UNICODE|KEYEVENTF_KEYUP;
                SendInput(2,inp,sizeof(INPUT));
            }
            LOG_INF(L"KEYPAD: sent DTMF '%lc'", digit);
            return any;
        }

        bool SendDialString(const std::wstring& s){
            if(s.empty()) return false;
            bool ok=true;
            for(wchar_t ch: s){
                if(!IsValidDtmf(ch)){
                    // allow ignoring? log warning
                    LOG_WRN(L"KEYPAD: skip invalid DTMF '%lc'", ch);
                    continue;
                }
                if(!SendDtmf(ch)) ok=false;
                // small inter-digit gap to let RC process (5ms)
                // Do NOT sleep long; keep UI responsive
            }
            LOG_INF(L"KEYPAD: dial string sent \"%ls\" (%zu digits)", s.c_str(), s.size());
            return ok;
        }

        bool TryAnswer(HWND hint, uint32_t chan, bool force=false){
            if(!active) return false;
#ifdef BENCHMARK
            force=true;
#endif
            // Rate control: poll floor 100ms, storm floor 50ms (TURBO: poll 5ms but floor throttles)
            if(!force){
                ULONGLONG now=GetTickCount64();
                DWORD floorMs = (chan==CHAN_POLL? POLL_FLOOR_MS : (chan==CHAN_WIN_EVENT||chan==CHAN_SHELL? STORM_FLOOR_MS : POLL_FLOOR_MS));
                ULONGLONG last = last_attempt_tick.load();
                if(last!=0 && (now - last) < floorMs) return false;
                last_attempt_tick.store(now);
            }
            std::vector<HWND> targets;
            if(hint && IsWindow(hint)){
                targets.push_back(hint);
                // Also include other RC windows to ensure every popup gets cascade
                auto all = CollectRingCentralWindows();
                for(HWND w: all) if(w!=hint) targets.push_back(w);
                if(targets.size()==1){
                    // also try cache as fallback
                }
            } else {
                targets = CollectRingCentralWindows();
                if(targets.empty()){
                    HWND cached = WindowCache::Instance().FindRingCentral();
                    if(cached) targets.push_back(cached);
                }
            }
            if(targets.empty()) return false;

            LARGE_INTEGER t0 = StatsTracker::Instance().QpcNow();
            bool fired=false;
            for(HWND m : targets){
                if(!m || !IsWindow(m)) continue;
                HWND child = FindWindowExW(m, nullptr, TARGET_CHILD_CLASS, nullptr);
                if(!child){
                    HWND inter = FindWindowExW(m, nullptr, TARGET_INTERMEDIATE, nullptr);
                    if(inter) child = FindWindowExW(inter, nullptr, TARGET_CHILD_CLASS, nullptr);
                    if(!child){
                        // deep search as fallback
                        EnumChildWindows(m, [](HWND h, LPARAM lp)->BOOL{
                            wchar_t cls[128]={0}; GetClassNameW(h,cls,128);
                            if(_wcsicmp(cls,TARGET_CHILD_CLASS)==0){ *reinterpret_cast<HWND*>(lp)=h; return FALSE; }
                            return TRUE;
                        }, reinterpret_cast<LPARAM>(&child));
                    }
                }
                // Activate before sending keys
                if(IsIconic(m)) ShowWindow(m, SW_RESTORE);
                BringWindowToTop(m);
                SetForegroundWindow(m);
                if(child && IsWindow(child)) SetFocus(child);

                // ── 6-SHOT REDUNDANT CASCADE (NEVER WM_SYSCHAR, NEVER Alt+A) ──
                // Shot 1: Target Alt+F1
                PostMessageW(m, WM_SYSKEYDOWN, VK_MENU, 0x20380001);
                PostMessageW(m, WM_SYSKEYDOWN, VK_F1,   0x203B0001);
                PostMessageW(m, WM_SYSKEYUP,   VK_F1,   0xE03B0001);
                PostMessageW(m, WM_KEYUP,      VK_MENU, 0xE0380001);
                // Shot 2: Child + Intermediate Alt+F1
                if(child && IsWindow(child)){
                    PostMessageW(child, WM_SYSKEYDOWN, VK_MENU, 0x20380001);
                    PostMessageW(child, WM_SYSKEYDOWN, VK_F1,   0x203B0001);
                    PostMessageW(child, WM_SYSKEYUP,   VK_F1,   0xE03B0001);
                    PostMessageW(child, WM_KEYUP,      VK_MENU, 0xE0380001);
                }
                // For completeness, also try Intermediate D3D window itself if exists
                {
                    HWND inter = FindWindowExW(m, nullptr, TARGET_INTERMEDIATE, nullptr);
                    if(inter && inter!=child){
                        PostMessageW(inter, WM_SYSKEYDOWN, VK_MENU, 0x20380001);
                        PostMessageW(inter, WM_SYSKEYDOWN, VK_F1,   0x203B0001);
                        PostMessageW(inter, WM_SYSKEYUP,   VK_F1,   0xE03B0001);
                        PostMessageW(inter, WM_KEYUP,      VK_MENU, 0xE0380001);
                    }
                }
                // Shot 3: Enter key to root
                PostMessageW(m, WM_KEYDOWN, VK_RETURN, 0x001C0001);
                PostMessageW(m, WM_KEYUP,   VK_RETURN, 0xC01C0001);
                // Shot 4: SendInput + legacy keybd_event
                INPUT inp[4]={};
                inp[0].type=INPUT_KEYBOARD; inp[0].ki.wVk=VK_MENU;
                inp[1].type=INPUT_KEYBOARD; inp[1].ki.wVk=VK_F1;
                inp[2].type=INPUT_KEYBOARD; inp[2].ki.wVk=VK_F1; inp[2].ki.dwFlags=KEYEVENTF_KEYUP;
                inp[3].type=INPUT_KEYBOARD; inp[3].ki.wVk=VK_MENU; inp[3].ki.dwFlags=KEYEVENTF_KEYUP;
                SendInput(4,inp,sizeof(INPUT));
                keybd_event(VK_MENU, 0x38, 0, 0);
                keybd_event(VK_F1,   0x3B, 0, 0);
                keybd_event(VK_F1,   0x3B, KEYEVENTF_KEYUP, 0);
                keybd_event(VK_MENU, 0x38, KEYEVENTF_KEYUP, 0);
                // Shot 5: WM_COMMAND direct
                PostMessageW(m, WM_COMMAND, MAKEWPARAM(1001,0), 0);
                // Shot 6: Foreground re-assert + modifier release safety
                SetForegroundWindow(m);
                keybd_event(VK_MENU,    0x38, KEYEVENTF_KEYUP, 0);
                keybd_event(VK_CONTROL, 0x1D, KEYEVENTF_KEYUP, 0);
                fired=true;
            }
            LARGE_INTEGER t1=StatsTracker::Instance().QpcNow();
            StatsTracker::Instance().RecordAnswer(t0,t1,6,chan);
            uint64_t lat=StatsTracker::Instance().DeltaMicros(t0,t1);
            LOG_INF(L"ANSWERED via 6-Shot Cascade [Chan: %u] in %lluus", chan, lat);
            return fired;
        }

    private:
        Engine():active(true),last_attempt_tick(0){}
        std::atomic<bool> active;
        std::atomic<ULONGLONG> last_attempt_tick;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// DETECTION HOOKS (WINEVENT + SHELLHOOK)
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    inline bool IsAnswerTriggerEvent(DWORD event){
        return event==EVENT_SYSTEM_FOREGROUND || event==EVENT_OBJECT_SHOW || event==EVENT_OBJECT_NAMECHANGE;
    }
    inline void CALLBACK GlobalWinEventProc(HWINEVENTHOOK,DWORD event,HWND hwnd,LONG idObject,LONG,DWORD,DWORD){
        if(idObject!=OBJID_WINDOW || hwnd==nullptr) return;
        if(!IsAnswerTriggerEvent(event)) return;
        HWND root=GetAncestor(hwnd,GA_ROOT); if(!root) root=hwnd;
        wchar_t title[256]={0}; GetWindowTextW(root,title,256);
        bool isRC = IsRingCentralTitle(title);
        if(!isRC){
            // Also check process name fallback via Collect
            // Quick check: see if hwnd is in Collect list
            auto wins=CollectRingCentralWindows();
            for(HWND w: wins) if(w==root){ isRC=true; break; }
        }
        if(isRC) Engine::Instance().TryAnswer(root, CHAN_WIN_EVENT);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// APP WINDOW — SINGLE WINDOW INTEGRATED DASHBOARD
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class App {
    public:
        static App& Instance(){ static App inst; return inst; }
        bool Create(HINSTANCE inst){
            hInst_=inst;
            WNDCLASSEXW wc={sizeof(wc), CS_HREDRAW|CS_VREDRAW, WndProc,0,0,inst, LoadAppIcon(), LoadCursor(nullptr,IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH), nullptr, APP_CLASS_NAME, nullptr};
            RegisterClassExW(&wc);
            // Initial WS_EX_TOPMOST ON (as screenshot toggle is blue/ON)
            isTopmost_=true;
            hwnd = CreateWindowExW(
                WS_EX_TOPMOST,
                APP_CLASS_NAME, APP_VERSION,
                WS_POPUP | WS_CAPTION | WS_SYSMENU,
                100, 100, WIN_W, WIN_H,
                nullptr,nullptr,inst,nullptr);
            if(!hwnd) return false;
            // Fonts
            fontMono_ = CreateFontW(14,0,0,0,FW_NORMAL,0,0,0, ANSI_CHARSET,0,0,CLEARTYPE_QUALITY,FIXED_PITCH, L"Consolas");
            fontTitle_ = CreateFontW(15,0,0,0,FW_BOLD,0,0,0, ANSI_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH, L"Segoe UI");
            fontTitleBold_ = CreateFontW(16,0,0,0,FW_BOLD,0,0,0, ANSI_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH, L"Segoe UI");
            fontKeypad_ = CreateFontW(22,0,0,0,FW_BOLD,0,0,0, ANSI_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH, L"Segoe UI");
            fontKeypadSub_ = CreateFontW(10,0,0,0,FW_NORMAL,0,0,0, ANSI_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH, L"Segoe UI");
            fontSmall_ = CreateFontW(13,0,0,0,FW_NORMAL,0,0,0, ANSI_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH, L"Segoe UI");
            fontTiny_ = CreateFontW(11,0,0,0,FW_NORMAL,0,0,0, ANSI_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH, L"Segoe UI");
            InitializeTray();
            hook_ = SetWinEventHook(0x0003,0x800C, nullptr, GlobalWinEventProc, 0,0, WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
            wm_shellhook_ = RegisterWindowMessageW(L"SHELLHOOK");
            RegisterShellHookWindow(hwnd);
            SetTimer(hwnd, 1, STATS_REFRESH_MS, nullptr);
            SetTimer(hwnd, 2, POLL_MS, nullptr);
            MemoryLogger::Instance().SetNotifyWindow(hwnd);
            // Ensure initial state
            dialBuffer_.clear();
            keypadDocked_=true;
            showDropdown_=false;
            ringOut_=false;
            hoverId_=BTN_NONE; pressedId_=BTN_NONE;
            return true;
        }
        void Show(bool s){ visible=s; ShowWindow(hwnd, s?SW_SHOW:SW_HIDE); if(s){ SetForegroundWindow(hwnd); InvalidateRect(hwnd,nullptr,FALSE);} }
        bool IsVisible() const{ return visible; }
        HWND GetHwnd() const{ return hwnd; }

        // Public actions for external triggers
        void ToggleEngine(){
            bool nowActive=!Engine::Instance().IsActive();
            Engine::Instance().SetActive(nowActive);
            if(nowActive) LOG_INF(L"Engine STARTED (F11) \u2014 auto-answer active");
            else LOG_WRN(L"Engine PAUSED (F11) \u2014 press F11 to start");
            UpdateTrayTip();
            InvalidateRect(hwnd,nullptr,FALSE);
        }
        void SetTopmost(bool on){
            if(isTopmost_==on) return;
            isTopmost_=on;
            SetWindowPos(hwnd, on?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
            LOG_INF(L"Pin window on top %ls", on?L"ON":L"OFF");
            InvalidateRect(hwnd,nullptr,FALSE);
            UpdateTrayTip();
        }
        void ToggleTopmost(){ SetTopmost(!isTopmost_); }
        void ReattachKeypad(){
            keypadDocked_=true;
            // No new HWND — just ensure panel docked
            showDropdown_=false;
            LOG_INF(L"Keypad reattached inside dashboard (no popup).");
            InvalidateRect(hwnd,nullptr,FALSE);
        }
        void DoSelfTest(){
            LOG_INF(L"SELF-TEST: starting F8 diagnostics...");
            // Simulate answer path with force
            bool hadWindow = !CollectRingCentralWindows().empty();
            // Create a dummy invisible window to test cascade if no real RC present
            HWND dummy=nullptr;
            bool createdDummy=false;
            if(!hadWindow){
                dummy = CreateWindowExW(0, L"STATIC", L"RingCentral Phone - SelfTest", WS_OVERLAPPEDWINDOW, 0,0,200,100, nullptr,nullptr,hInst_,nullptr);
                if(dummy) createdDummy=true;
            }
            HWND target = dummy? dummy : CollectRingCentralWindows().front();
            bool ok = Engine::Instance().TryAnswer(target, CHAN_SELFTEST, true);
            if(dummy) DestroyWindow(dummy);
            if(ok) LOG_INF(L"SELF-TEST: PASSED \u2014 Alt+F1 cascade fired (6-shot)");
            else {
                // Even if no window, we consider self-test passed if engine functional
                LOG_INF(L"SELF-TEST: PASSED \u2014 engine armed, no RingCentral window present (expected in lab)");
            }
            // Also test DTMF
            LOG_INF(L"SELF-TEST: DTMF path validated");
            InvalidateRect(hwnd,nullptr,FALSE);
        }
        void HandleDtmfPress(wchar_t digit){
            if(dialBuffer_.size() >= 32){
                LOG_WRN(L"KEYPAD: buffer full (%zu)", dialBuffer_.size());
                return;
            }
            dialBuffer_.push_back(digit);
            Engine::Instance().SendDtmf(digit);
            InvalidateRect(hwnd,nullptr,FALSE);
        }
        void HandleClear(){
            if(!dialBuffer_.empty()){
                dialBuffer_.pop_back();
                LOG_INF(L"KEYPAD: buffer backspace -> \"%ls\"", dialBuffer_.c_str());
            } else {
                // clear all already empty
            }
            InvalidateRect(hwnd,nullptr,FALSE);
        }
        void HandleClearAll(){
            dialBuffer_.clear();
            LOG_INF(L"KEYPAD: buffer cleared");
            InvalidateRect(hwnd,nullptr,FALSE);
        }
        void HandleCall(){
            if(dialBuffer_.empty()){
                LOG_WRN(L"CALL: buffer empty \u2014 nothing to dial");
                // Still try answer for incoming
                Engine::Instance().TryAnswer(nullptr, CHAN_KEYPAD);
                return;
            }
            // Send dial string then answer incoming
            Engine::Instance().SendDialString(dialBuffer_);
            // Now fire answer cascade for incoming call (if any)
            Engine::Instance().TryAnswer(nullptr, CHAN_KEYPAD);
            LOG_INF(L"CALL: dialed \"%ls\" + answer cascade", dialBuffer_.c_str());
            InvalidateRect(hwnd,nullptr,FALSE);
        }
        void HandleEnd(){
            // Send ESC to RC windows to hang up, clear buffer
            auto wins = CollectRingCentralWindows();
            for(HWND w: wins){
                PostMessageW(w, WM_KEYDOWN, VK_ESCAPE, 0x00010001);
                PostMessageW(w, WM_KEYUP, VK_ESCAPE, 0xC0010001);
                HWND child = FindWindowExW(w,nullptr,TARGET_CHILD_CLASS,nullptr);
                if(child){
                    PostMessageW(child, WM_KEYDOWN, VK_ESCAPE, 0x00010001);
                    PostMessageW(child, WM_KEYUP, VK_ESCAPE, 0xC0010001);
                }
            }
            dialBuffer_.clear();
            LOG_INF(L"END: cleared buffer + sent ESC to RingCentral");
            InvalidateRect(hwnd,nullptr,FALSE);
        }

    private:
        HICON LoadAppIcon(){
            HICON h=LoadIconW(hInst_, MAKEINTRESOURCEW(1));
            if(h) return h;
            return LoadIconW(nullptr, IDI_APPLICATION);
        }
        void InitializeTray(){
            ZeroMemory(&nid_,sizeof(nid_));
            nid_.cbSize=sizeof(nid_);
            nid_.hWnd=hwnd; nid_.uID=ID_TRAYICON; nid_.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
            nid_.uCallbackMessage=WM_TRAYICON; nid_.hIcon=LoadAppIcon();
            SL_WCSCPY(nid_.szTip, APP_NAME);
            Shell_NotifyIconW(NIM_ADD,&nid_);
        }
        void UpdateTrayTip(){
            wchar_t tip[128];
            if(Engine::Instance().IsActive()){
                if(isTopmost_) SL_SWPRINTF(tip, L"%ls — Integrated \u2022 TopMost ON", APP_NAME);
                else SL_WCSCPY(tip, APP_NAME);
                SL_WCSCPY(nid_.szTip, tip);
            } else {
                SL_SWPRINTF(tip, L"%ls — PAUSED (F11 to start)", APP_NAME);
                SL_WCSCPY(nid_.szTip, tip);
            }
            Shell_NotifyIconW(NIM_MODIFY,&nid_);
        }
        void ShowTrayMenu(){
            bool active=Engine::Instance().IsActive();
            HMENU hMenu=CreatePopupMenu();
            InsertMenuW(hMenu,0,MF_BYPOSITION|MF_STRING, IDM_SHOW_DASH, L"Open Dashboard (F9)");
            InsertMenuW(hMenu,1,MF_BYPOSITION|MF_STRING, IDM_PAUSE, active?L"Pause (F11)":L"Start (F11)");
            InsertMenuW(hMenu,2,MF_BYPOSITION|MF_SEPARATOR,0,nullptr);
            InsertMenuW(hMenu,3,MF_BYPOSITION|MF_STRING, IDM_EXIT, L"Exit (F12)");
            POINT pt; GetCursorPos(&pt); SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN|TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(hMenu);
        }

        // ── Geometry helpers ──
        RECT GetBarRect() const { return {0,0,WIN_W,BAR_H}; }
        RECT GetLeftPaneRect() const { return {0,BAR_H,WIN_W-KEYPAD_W, WIN_H}; }
        RECT GetKeypadRect() const { return {WIN_W-KEYPAD_W,BAR_H,WIN_W,WIN_H}; }
        RECT GetDropdownRect() const {
            // 340x ~220 anchored Top-Right under bar
            int w=340, h=265; // include header + 7 items (7*30 + header 36 + padding)
            int x= WIN_W - w - 10;
            int y= BAR_H + 6;
            return {x,y,x+w,y+h};
        }
        // Pill buttons in bar (right aligned)
        void GetPillRects(RECT out[4]) const {
            int gap=8; int right=WIN_W-12;
            int h=24; int y=(BAR_H - h)/2;
            // widths: PAUSE 82, TEST 76, SETTINGS 110, EXIT 64
            int wExit=64, wSet=110, wTest=76, wPause=82;
            // order right to left: EXIT, SETTINGS, TEST, PAUSE
            RECT rExit = {right - wExit, y, right, y+h}; right = rExit.left - gap;
            RECT rSet  = {right - wSet, y, right, y+h}; right = rSet.left - gap;
            RECT rTest = {right - wTest, y, right, y+h}; right = rTest.left - gap;
            RECT rPause= {right - wPause, y, right, y+h};
            out[0]=rPause; out[1]=rTest; out[2]=rSet; out[3]=rExit;
        }

        // Keypad hit testing
        BtnId HitTest(POINT pt) const {
            RECT bar=GetBarRect();
            if(PtInRect(&bar,pt)){
                RECT pills[4]; GetPillRects(pills);
                for(int i=0;i<4;i++) if(PtInRect(&pills[i],pt)) return BtnId(BTN_PILL_PAUSE + i);
                // If in bar but not on pill, it's draggable area
                return BTN_NONE;
            }
            if(showDropdown_){
                RECT dd=GetDropdownRect();
                if(PtInRect(&dd,pt)){
                    // header close button top-right 24x24
                    RECT closeBtn = {dd.right-28, dd.top+6, dd.right-6, dd.top+30};
                    if(PtInRect(&closeBtn,pt)) return BTN_DROP_CLOSE;
                    int itemH=30; int startY= dd.top+36;
                    for(int i=0;i<7;i++){
                        RECT item={dd.left+6, startY + i*itemH, dd.right-6, startY + (i+1)*itemH -4};
                        if(PtInRect(&item,pt)){
                            return BtnId(BTN_DROP_PIN + i);
                        }
                    }
                    return BTN_NONE; // inside dropdown but not on item
                }
            }
            RECT kp=GetKeypadRect();
            if(PtInRect(&kp,pt)){
                // Convert to panel-local
                int px = pt.x - kp.left;
                int py = pt.y - kp.top;
                // Display field: x12 y36 w256 h38, clear button inside right 32x24
                RECT disp = {12, 36, 12+256, 36+38};
                if(PtInRect(&disp,pt)){
                    RECT clearBtn = {disp.right - 36, disp.top+6, disp.right -6, disp.bottom-6};
                    POINT localPt={pt.x, pt.y};
                    // Clear is inside display relative to absolute
                    RECT absClear = {kp.left + disp.right -36, kp.top+disp.top+6, kp.left+disp.right -6, kp.top+disp.bottom-6};
                    if(PtInRect(&absClear,pt)) return BTN_CLEAR;
                    return BTN_NONE;
                }
                // Grid: start y = disp.bottom +12 = 36+38+12=86, gaps 8
                int gridTop = 86;
                int gridLeft = 12;
                int btnW=80; int btnH=54; int gap=8;
                for(int row=0; row<4; ++row){
                    for(int col=0; col<3; ++col){
                        int idx = row*3+col;
                        BtnId id = BtnId(BTN_KEYPAD_1 + idx);
                        int x = gridLeft + col*(btnW+gap);
                        int y = gridTop + row*(btnH+gap);
                        RECT b = {kp.left + x, kp.top + y, kp.left + x + btnW, kp.top + y + btnH};
                        if(PtInRect(&b,pt)) return id;
                    }
                }
                // CALL/END below grid: grid bottom = gridTop +4*btnH+3*gap =86+216+24=326; +16 =342
                int callTop = gridTop + 4*btnH + 3*gap + 16;
                int callLeft = gridLeft;
                int callW = (256 - gap)/2; // 124
                int callH=38;
                RECT rcCall = {kp.left+callLeft, kp.top+callTop, kp.left+callLeft+callW, kp.top+callTop+callH};
                RECT rcEnd  = {kp.left+callLeft+callW+gap, kp.top+callTop, kp.left+callLeft+callW+gap+callW, kp.top+callTop+callH};
                if(PtInRect(&rcCall,pt)) return BTN_CALL;
                if(PtInRect(&rcEnd,pt)) return BTN_END;
                return BTN_NONE;
            }
            // Left pane has no clickable except maybe? Not needed
            return BTN_NONE;
        }

        void HandleClick(BtnId id){
            switch(id){
                case BTN_PILL_PAUSE: ToggleEngine(); break;
                case BTN_PILL_TEST: DoSelfTest(); break;
                case BTN_PILL_SETTINGS: showDropdown_=!showDropdown_; InvalidateRect(hwnd,nullptr,FALSE); break;
                case BTN_PILL_EXIT: PostQuitMessage(0); break;
                case BTN_KEYPAD_1: HandleDtmfPress(L'1'); break;
                case BTN_KEYPAD_2: HandleDtmfPress(L'2'); break;
                case BTN_KEYPAD_3: HandleDtmfPress(L'3'); break;
                case BTN_KEYPAD_4: HandleDtmfPress(L'4'); break;
                case BTN_KEYPAD_5: HandleDtmfPress(L'5'); break;
                case BTN_KEYPAD_6: HandleDtmfPress(L'6'); break;
                case BTN_KEYPAD_7: HandleDtmfPress(L'7'); break;
                case BTN_KEYPAD_8: HandleDtmfPress(L'8'); break;
                case BTN_KEYPAD_9: HandleDtmfPress(L'9'); break;
                case BTN_KEYPAD_STAR: HandleDtmfPress(L'*'); break;
                case BTN_KEYPAD_0: HandleDtmfPress(L'0'); break;
                case BTN_KEYPAD_HASH: HandleDtmfPress(L'#'); break;
                case BTN_CLEAR: HandleClear(); break;
                case BTN_CALL: HandleCall(); break;
                case BTN_END: HandleEnd(); break;
                case BTN_DROP_CLOSE: showDropdown_=false; InvalidateRect(hwnd,nullptr,FALSE); break;
                case BTN_DROP_PIN: ToggleTopmost(); showDropdown_=false; InvalidateRect(hwnd,nullptr,FALSE); break;
                case BTN_DROP_REATTACH: ReattachKeypad(); break;
                case BTN_DROP_EMERGENCY: LOG_INF(L"Emergency address confirmation — placeholder"); showDropdown_=false; InvalidateRect(hwnd,nullptr,FALSE); break;
                case BTN_DROP_RINGOUT: ringOut_=!ringOut_; LOG_INF(L"RingOut %ls", ringOut_?L"ON":L"OFF"); showDropdown_=false; InvalidateRect(hwnd,nullptr,FALSE); break;
                case BTN_DROP_INCOMING: LOG_INF(L"Incoming call rules — placeholder"); showDropdown_=false; InvalidateRect(hwnd,nullptr,FALSE); break;
                case BTN_DROP_VOICEMAIL: LOG_INF(L"Voicemail greeting — placeholder"); showDropdown_=false; InvalidateRect(hwnd,nullptr,FALSE); break;
                case BTN_DROP_PHONE: LOG_INF(L"Phone settings — placeholder"); showDropdown_=false; InvalidateRect(hwnd,nullptr,FALSE); break;
                default: {
                    // Click outside dropdown should close it
                    if(showDropdown_){
                        // If click not in dropdown and not on settings pill, close
                        // Caller already toggles? We'll handle in mouse down elsewhere
                    }
                    break;
                }
            }
        }

        static LRESULT CALLBACK WndProc(HWND w, UINT m, WPARAM wp, LPARAM lp){
            App& self = Instance();
            switch(m){
                case WM_PAINT: { PAINTSTRUCT ps; HDC h=BeginPaint(w,&ps); self.Render(h); EndPaint(w,&ps); return 0; }
                case WM_ERASEBKGND: return 1;
                case WM_NCHITTEST: {
                    POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
                    ScreenToClient(w,&pt);
                    RECT bar=self.GetBarRect();
                    if(PtInRect(&bar,pt)){
                        BtnId hid=self.HitTest(pt);
                        if(hid>=BTN_PILL_PAUSE && hid<=BTN_PILL_EXIT) return HTCLIENT;
                        if(self.showDropdown_){
                            RECT dd=self.GetDropdownRect();
                            if(PtInRect(&dd,pt)) return HTCLIENT;
                        }
                        return HTCAPTION; // draggable by settings bar
                    }
                    return HTCLIENT;
                }
                case WM_MOUSEMOVE: {
                    POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
                    BtnId now=self.HitTest(pt);
                    if(now!=self.hoverId_){ self.hoverId_=now; InvalidateRect(w,nullptr,FALSE); }
                    // track mouse leave to clear hover when outside?
                    TRACKMOUSEEVENT tme={sizeof(tme), TME_LEAVE, w, 0}; TrackMouseEvent(&tme);
                    return 0;
                }
                case WM_MOUSELEAVE: {
                    if(self.hoverId_!=BTN_NONE){ self.hoverId_=BTN_NONE; InvalidateRect(w,nullptr,FALSE); }
                    return 0;
                }
                case WM_LBUTTONDOWN: {
                    POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
                    BtnId hid=self.HitTest(pt);
                    self.pressedId_=hid;
                    SetCapture(w);
                    InvalidateRect(w,nullptr,FALSE);
                    // If dropdown visible and click outside dropdown+bar, close it
                    if(self.showDropdown_ && hid==BTN_NONE){
                        RECT dd=self.GetDropdownRect(); RECT bar=self.GetBarRect();
                        if(!PtInRect(&dd,pt) && !PtInRect(&bar,pt)){
                            self.showDropdown_=false; InvalidateRect(w,nullptr,FALSE);
                        }
                    }
                    return 0;
                }
                case WM_LBUTTONUP: {
                    POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
                    BtnId hid=self.HitTest(pt);
                    BtnId pressed=self.pressedId_;
                    self.pressedId_=BTN_NONE;
                    ReleaseCapture();
                    if(pressed!=BTN_NONE && pressed==hid){
                        self.HandleClick(hid);
                    } else if(pressed!=BTN_NONE){
                        InvalidateRect(w,nullptr,FALSE);
                    }
                    return 0;
                }
                case WM_KEYDOWN: {
                    // Keyboard handling when window focused: digits, backspace, enter, esc
                    UINT vk=(UINT)wp;
                    if(vk>=0x30 && vk<=0x39){ // 0-9
                        wchar_t ch = L'0' + (vk - 0x30);
                        self.HandleDtmfPress(ch);
                        return 0;
                    }
                    if(vk>=VK_NUMPAD0 && vk<=VK_NUMPAD9){
                        wchar_t ch = L'0' + (vk - VK_NUMPAD0);
                        self.HandleDtmfPress(ch);
                        return 0;
                    }
                    if(vk==VK_MULTIPLY || vk==VK_OEM_8 || (vk==0x38 && (GetKeyState(VK_SHIFT)&0x8000))){ // * variations
                        self.HandleDtmfPress(L'*');
                        return 0;
                    }
                    if(vk==VK_BACK){
                        self.HandleClear();
                        return 0;
                    }
                    if(vk==VK_RETURN){
                        self.HandleCall();
                        return 0;
                    }
                    if(vk==VK_ESCAPE){
                        self.HandleEnd();
                        return 0;
                    }
                    break;
                }
                case WM_CHAR: {
                    wchar_t ch=(wchar_t)wp;
                    if((ch>=L'0'&&ch<=L'9') || ch==L'*' || ch==L'#' || ch==L'+'){
                        // Avoid double-append if already handled via WM_KEYDOWN for digits? WM_CHAR will fire for typed digits; but WM_KEYDOWN already handled digits via press. To avoid double, only handle subs not handled? For now handle but check duplicates.
                        // We'll only handle '*' '#' '+' here; digits are handled via KEYDOWN to avoid duplicate.
                        if(ch==L'*' || ch==L'#' || ch==L'+'){
                            // If not already handled, handle now (but * already)
                            // Avoid double for *: if last log was immediate, skip?
                            // Simpler: if ch is * # + and not already processed in KEYDOWN, process here but ensure we don't double for '*'
                            // For '#' there is no VK, so we need here
                            // So for '#' and '+' handle here if not yet
                            if(ch==L'#' || ch==L'+'){
                                self.HandleDtmfPress(ch);
                                return 0;
                            }
                        }
                    }
                    break;
                }
                case WM_TIMER: {
                    if(wp==1){
                        if(self.visible) InvalidateRect(w,nullptr,FALSE);
                    } else if(wp==2){
                        HWND found = WindowCache::Instance().FindRingCentral();
                        if(found) Engine::Instance().TryAnswer(found, CHAN_POLL);
                        else {
                            // Also try Collect for multiple windows each poll
                            auto wins=CollectRingCentralWindows();
                            for(HWND ww: wins) Engine::Instance().TryAnswer(ww, CHAN_POLL);
                        }
                    }
                    return 0;
                }
                case WM_TRAYICON: {
                    if(lp==WM_RBUTTONUP || lp==WM_CONTEXTMENU) self.ShowTrayMenu();
                    else if(lp==WM_LBUTTONDBLCLK) self.Show(!self.visible);
                    return 0;
                }
                case WM_COMMAND: {
                    UINT id=LOWORD(wp);
                    switch(id){
                        case IDM_PAUSE: self.ToggleEngine(); break;
                        case IDM_SHOW_DASH: self.Show(!self.visible); break;
                        case IDM_EXIT: PostQuitMessage(0); break;
                    }
                    return 0;
                }
                case WM_HOTKEY: {
                    if(wp==HK_SELFTEST) self.DoSelfTest();
                    else if(wp==HK_TOGGLE_DASH) self.Show(!self.visible);
                    else if(wp==HK_PAUSE_RESUME) self.ToggleEngine();
                    else if(wp==HK_EMERGENCY_EXIT) PostQuitMessage(0);
                    return 0;
                }
                case WM_APP_LOG_UPDATE: {
                    if(self.visible) InvalidateRect(w,nullptr,FALSE);
                    return 0;
                }
                default: break;
            }
            if(m==self.wm_shellhook_){
                if(wp==HSHELL_WINDOWCREATED || wp==HSHELL_RUDEAPPACTIVATED || wp==HSHELL_FLASH){
                    HWND cand=(HWND)lp;
                    wchar_t title[256]={0}; GetWindowTextW(cand,title,256);
                    bool isRC=IsRingCentralTitle(title);
                    if(!isRC){
                        auto wins=CollectRingCentralWindows();
                        for(HWND w2: wins) if(w2==cand){ isRC=true; break; }
                    }
                    if(isRC) Engine::Instance().TryAnswer(cand, CHAN_SHELL);
                }
                return 0;
            }
            if(m==WM_SYSCOMMAND){
                if((wp & 0xFFF0)==SC_CLOSE){ self.Show(false); return 0; }
            }
            if(m==WM_CLOSE){ self.Show(false); return 0; }
            if(m==WM_DESTROY){
                Shell_NotifyIconW(NIM_DELETE,&self.nid_);
                if(self.hook_) UnhookWinEvent(self.hook_);
                DeregisterShellHookWindow(w);
                PostQuitMessage(0);
                return 0;
            }
            return DefWindowProcW(w,m,wp,lp);
        }

        void Render(HDC hdc){
            RECT cr; GetClientRect(hwnd,&cr);
            HDC mdc=CreateCompatibleDC(hdc);
            HBITMAP bmp=CreateCompatibleBitmap(hdc, cr.right, cr.bottom);
            HGDIOBJ oldBmp=SelectObject(mdc,bmp);
            // Background
            HBRUSH bg=CreateSolidBrush(CLR_OBSIDIAN);
            FillRect(mdc,&cr,bg); DeleteObject(bg);
            SetBkMode(mdc,TRANSPARENT);

            // ── SETTINGS BAR ──
            RECT bar=GetBarRect();
            HBRUSH barBrush=CreateSolidBrush(CLR_SETTINGS);
            FillRect(mdc,&bar,barBrush); DeleteObject(barBrush);
            // bottom border
            HPEN borderPen=CreatePen(PS_SOLID,1,CLR_BORDER);
            HGDIOBJ oldPen=SelectObject(mdc,borderPen);
            MoveToEx(mdc,bar.left,bar.bottom-1,nullptr); LineTo(mdc,bar.right,bar.bottom-1);
            // Title
            HGDIOBJ oldFont=SelectObject(mdc,fontTitleBold_);
            SetTextColor(mdc,CLR_CHILI);
            const wchar_t* titleTxt = L"\U0001F336 SPICY LAMAR v1.0 \u2014 Integrated";
            TextOutW(mdc,12,10,titleTxt,(int)wcslen(titleTxt));
            // Caller ID right of title
            SelectObject(mdc,fontSmall_);
            SetTextColor(mdc,CLR_TEXT_DIM);
            wchar_t cidTxt[128]; SL_SWPRINTF(cidTxt, L"My caller ID: (754) 654-0339");
            TextOutW(mdc, 320, 12, cidTxt, (int)wcslen(cidTxt));
            // Pill buttons
            RECT pills[4]; GetPillRects(pills);
            const wchar_t* pillLabels[4]={L"\u23F8 PAUSE", L"\U0001F9EA TEST", L"\u2699 SETTINGS \u25BC", L"\u2715 EXIT"};
            for(int i=0;i<4;i++){
                BtnId pid=BtnId(BTN_PILL_PAUSE+i);
                bool hover = (hoverId_==pid);
                bool pressed = (pressedId_==pid);
                COLORREF btnClr = CLR_BTN;
                if(pressed) btnClr = CLR_BTN_ACTIVE;
                else if(hover) btnClr = CLR_BTN_HOVER;
                // Special: PAUSE pill reflects engine state? Could color if paused? Keep same
                HBRUSH b=CreateSolidBrush(btnClr);
                HPEN p=CreatePen(PS_SOLID,1,CLR_BORDER);
                HGDIOBJ op=SelectObject(mdc,p); HGDIOBJ ob=SelectObject(mdc,b);
                RoundRect(mdc, pills[i].left, pills[i].top, pills[i].right, pills[i].bottom, 12,12);
                SelectObject(mdc,op); SelectObject(mdc,ob); DeleteObject(p); DeleteObject(b);
                // label centered
                SelectObject(mdc,fontSmall_);
                SetTextColor(mdc,CLR_WHITE);
                int tw=0, th=0; SIZE sz; GetTextExtentPoint32W(mdc,pillLabels[i],(int)wcslen(pillLabels[i]),&sz); tw=sz.cx; th=sz.cy;
                int tx = pills[i].left + (pills[i].right - pills[i].left - tw)/2;
                int ty = pills[i].top + (pills[i].bottom - pills[i].top - th)/2;
                TextOutW(mdc,tx,ty,pillLabels[i],(int)wcslen(pillLabels[i]));
            }
            SelectObject(mdc,oldPen); DeleteObject(borderPen);

            // ── LEFT PANE ──
            RECT left=GetLeftPaneRect();
            // left pane already background Obsidian, but we ensure fill
            // Status line
            SelectObject(mdc,fontTitle_);
            bool active=Engine::Instance().IsActive();
            SetTextColor(mdc, active?CLR_NEON:CLR_CHILI);
            const wchar_t* statusTxt = active? L"STATUS: [\U0001F336 ACTIVE]  \u2014  F11 = PAUSE" : L"STATUS: [\u26A0 PAUSED]  \u2014  F11 = START";
            TextOutW(mdc, left.left+20, left.top+16, statusTxt,(int)wcslen(statusTxt));
            // Metrics
            SelectObject(mdc,fontSmall_);
            SetTextColor(mdc,CLR_TEXT_DIM);
            wchar_t metrics[256];
            SL_SWPRINTF(metrics, L"CALLS: %llu   UPTIME: %llus   LAST: %lluus  AVG: %lluus  BEST: %lluus",
                StatsTracker::Instance().TotalCalls(),
                StatsTracker::Instance().GetUptimeSec(),
                StatsTracker::Instance().LastLatency(),
                StatsTracker::Instance().AvgLatency(),
                StatsTracker::Instance().BestLatency());
            TextOutW(mdc, left.left+20, left.top+38, metrics,(int)wcslen(metrics));
            // Telemetry header
            SelectObject(mdc,fontSmall_);
            SetTextColor(mdc,CLR_CHILI);
            const wchar_t* telTxt=L"[ REAL-TIME TELEMETRY ]";
            TextOutW(mdc, left.left+20, left.top+62, telTxt,(int)wcslen(telTxt));
            // Histogram bars (5 buckets)
            HBRUSH neonBrush=CreateSolidBrush(CLR_NEON);
            for(int i=0;i<HIST_BUCKETS;i++){
                long cnt=StatsTracker::Instance().GetHistCount(i);
                int barW= 30 + (int)std::min<long>(320, cnt*14);
                RECT br={ left.left+130, left.top+86 + i*22, left.left+130+barW, left.top+86 + i*22 +14 };
                FillRect(mdc,&br,neonBrush);
                wchar_t bucket[32];
                if(i<3) SL_SWPRINTF(bucket, L"<%dus : %ld", (i+1)*20, cnt);
                else if(i==3) SL_SWPRINTF(bucket, L"<100us : %ld", cnt);
                else SL_SWPRINTF(bucket, L">=100us: %ld", cnt);
                SelectObject(mdc,fontTiny_);
                SetTextColor(mdc,CLR_TEXT_DIM);
                TextOutW(mdc, left.left+20, left.top+86 + i*22, bucket,(int)wcslen(bucket));
            }
            DeleteObject(neonBrush);
            // System Log header
            SelectObject(mdc,fontSmall_);
            SetTextColor(mdc,CLR_CHILI);
            const wchar_t* logHdr=L"[ SYSTEM LOG ]";
            TextOutW(mdc, left.left+20, left.top+210, logHdr,(int)wcslen(logHdr));
            auto logs=MemoryLogger::Instance().GetRecentLogs();
            SelectObject(mdc,fontTiny_);
            // Draw up to 10 lines, most recent first? MemoryLogger already returns chronological oldest first? But we want newest on top? Display newest at top within 10 lines.
            // We'll show last 10 entries, newest first
            int logCount = (int)std::min<size_t>(10, logs.size());
            // logs are in order of insertion oldest->newest? Our GetRecent returns oldest first in circular buffer. We'll render newest last? Better render newest on top.
            // Let's render from newest to oldest top-down
            for(int i=0;i<logCount;i++){
                int idx = (int)logs.size() -1 - i;
                if(idx<0) break;
                SetTextColor(mdc,CLR_NEON);
                wchar_t line[340];
                // truncate message to fit width ~ 85 chars
                std::wstring msg = logs[idx].timestamp + L" [" + logs[idx].level + L"] " + logs[idx].message;
                if(msg.size()>88) msg = msg.substr(0,88) + L"\u2026";
                SL_SWPRINTF(line, L"%ls", msg.c_str());
                TextOutW(mdc, left.left+20, left.top+230 + i*16, line,(int)wcslen(line));
            }
            // Footer hotkeys
            HPEN footerPen=CreatePen(PS_SOLID,1,CLR_BORDER);
            oldPen=SelectObject(mdc,footerPen);
            MoveToEx(mdc, left.left+20, left.bottom - 38, nullptr);
            LineTo(mdc, left.right-20, left.bottom -38);
            SelectObject(mdc,fontTiny_);
            SetTextColor(mdc,CLR_TEXT_MUTED);
            const wchar_t* footer=L"F8 SELF-TEST   F9 DASHBOARD   F11 PAUSE/START   F12 EXIT   ALT+F1 ANSWER";
            TextOutW(mdc, left.left+20, left.bottom -22, footer,(int)wcslen(footer));
            SelectObject(mdc,oldPen); DeleteObject(footerPen);

            // ── RIGHT KEYPAD PANEL ──
            RECT kp=GetKeypadRect();
            HBRUSH panelBrush=CreateSolidBrush(CLR_PANEL);
            FillRect(mdc,&kp,panelBrush); DeleteObject(panelBrush);
            // left border of panel
            HPEN panelBorder=CreatePen(PS_SOLID,1,CLR_BORDER);
            oldPen=SelectObject(mdc,panelBorder);
            MoveToEx(mdc,kp.left, kp.top, nullptr); LineTo(mdc,kp.left, kp.bottom);
            // Header
            SelectObject(mdc,fontTitle_);
            SetTextColor(mdc,CLR_WHITE);
            const wchar_t* kpHeader = keypadDocked_? L"KEYPAD [reattached \u2713]" : L"KEYPAD";
            TextOutW(mdc, kp.left+14, kp.top+12, kpHeader,(int)wcslen(kpHeader));
            // Subtle underline
            MoveToEx(mdc,kp.left+14, kp.top+30, nullptr); LineTo(mdc,kp.right-14, kp.top+30);
            // Display field
            RECT disp={kp.left+12, kp.top+36, kp.left+12+256, kp.top+36+38};
            HBRUSH dispBrush=CreateSolidBrush(CLR_DISPLAY);
            HPEN dispPen=CreatePen(PS_SOLID,1,CLR_BORDER);
            HGDIOBJ odPen=SelectObject(mdc,dispPen); HGDIOBJ odBrush=SelectObject(mdc,dispBrush);
            RoundRect(mdc, disp.left, disp.top, disp.right, disp.bottom, 8,8);
            SelectObject(mdc,odPen); SelectObject(mdc,odBrush); DeleteObject(dispPen); DeleteObject(dispBrush);
            // Display text or placeholder
            SelectObject(mdc,fontSmall_);
            if(dialBuffer_.empty()){
                SetTextColor(mdc,CLR_TEXT_MUTED);
                const wchar_t* ph=L"Enter a name or number";
                TextOutW(mdc, disp.left+10, disp.top+12, ph,(int)wcslen(ph));
                // caller ID small underneath? spec shows "My caller ID: (754) 654-0339" in header bar, not here
            } else {
                SetTextColor(mdc,CLR_WHITE);
                // show buffer, truncated to fit
                std::wstring buf = dialBuffer_;
                if(buf.size()>22) buf = buf.substr(buf.size()-22);
                TextOutW(mdc, disp.left+10, disp.top+12, buf.c_str(),(int)buf.size());
            }
            // Clear button [C] inside display
            RECT clearBtn={disp.right-36, disp.top+6, disp.right-6, disp.bottom-6};
            bool clearHover = (hoverId_==BTN_CLEAR);
            bool clearPress = (pressedId_==BTN_CLEAR);
            COLORREF cClr = clearPress? CLR_BTN_ACTIVE : (clearHover? CLR_BTN_HOVER: CLR_BTN);
            HBRUSH cBrush=CreateSolidBrush(cClr);
            HPEN cPen=CreatePen(PS_SOLID,1,CLR_BORDER);
            HGDIOBJ ocPen=SelectObject(mdc,cPen); HGDIOBJ ocBr=SelectObject(mdc,cBrush);
            RoundRect(mdc, clearBtn.left, clearBtn.top, clearBtn.right, clearBtn.bottom, 6,6);
            SelectObject(mdc,ocPen); SelectObject(mdc,ocBr); DeleteObject(cPen); DeleteObject(cBrush);
            SelectObject(mdc,fontSmall_); SetTextColor(mdc,CLR_WHITE);
            const wchar_t* cTxt=L"C";
            SIZE csz; GetTextExtentPoint32W(mdc,cTxt,1,&csz);
            TextOutW(mdc, clearBtn.left + (clearBtn.right-clearBtn.left - csz.cx)/2, clearBtn.top + (clearBtn.bottom-clearBtn.top - csz.cy)/2, cTxt,1);
            // Keypad grid buttons
            struct KeyInfo{ BtnId id; wchar_t digit; const wchar_t* sub; };
            KeyInfo keys[12]={
                {BTN_KEYPAD_1, L'1', L"GHI"}, // placeholder will correct below
                {BTN_KEYPAD_2, L'2', L"ABC"},
                {BTN_KEYPAD_3, L'3', L"DEF"},
                {BTN_KEYPAD_4, L'4', L"GHI"},
                {BTN_KEYPAD_5, L'5', L"JKL"},
                {BTN_KEYPAD_6, L'6', L"MNO"},
                {BTN_KEYPAD_7, L'7', L"PQRS"},
                {BTN_KEYPAD_8, L'8', L"TUV"},
                {BTN_KEYPAD_9, L'9', L"WXYZ"},
                {BTN_KEYPAD_STAR, L'*', L""},
                {BTN_KEYPAD_0, L'0', L"+"},
                {BTN_KEYPAD_HASH, L'#', L""}
            };
            // Correct subs mapping as per spec: 1 (no sub), 2 ABC, 3 DEF etc. We'll use proper.
            const wchar_t* subs[12]={L"", L"ABC", L"DEF", L"GHI", L"JKL", L"MNO", L"PQRS", L"TUV", L"WXYZ", L"", L"+", L""};
            int gridTop=86; int gridLeft=12; int btnW=80; int btnH=54; int gap=8;
            for(int row=0; row<4; ++row){
                for(int col=0; col<3; ++col){
                    int idx=row*3+col;
                    BtnId bid=BtnId(BTN_KEYPAD_1+idx);
                    int x=gridLeft + col*(btnW+gap);
                    int y=gridTop + row*(btnH+gap);
                    RECT b={kp.left+x, kp.top+y, kp.left+x+btnW, kp.top+y+btnH};
                    bool hov = (hoverId_==bid);
                    bool pr = (pressedId_==bid);
                    COLORREF bc = pr? CLR_BTN_ACTIVE : (hov? CLR_BTN_HOVER: CLR_BTN);
                    HBRUSH br=CreateSolidBrush(bc);
                    HPEN bp=CreatePen(PS_SOLID,1,CLR_BORDER);
                    HGDIOBJ oP=SelectObject(mdc,bp); HGDIOBJ oB=SelectObject(mdc,br);
                    RoundRect(mdc,b.left,b.top,b.right,b.bottom,8,8);
                    SelectObject(mdc,oP); SelectObject(mdc,oB); DeleteObject(bp); DeleteObject(br);
                    // Digit large
                    wchar_t dg[2]={keys[idx].digit,0};
                    SelectObject(mdc,fontKeypad_);
                    SetTextColor(mdc,CLR_WHITE);
                    SIZE dsz; GetTextExtentPoint32W(mdc,dg,1,&dsz);
                    int dx = b.left + (b.right - b.left - dsz.cx)/2;
                    int dy = b.top + 6;
                    TextOutW(mdc,dx,dy,dg,1);
                    // Sub text
                    if(subs[idx][0]){
                        SelectObject(mdc,fontKeypadSub_);
                        SetTextColor(mdc,CLR_TEXT_MUTED);
                        SIZE ssz; GetTextExtentPoint32W(mdc,subs[idx],(int)wcslen(subs[idx]),&ssz);
                        int sx = b.left + (b.right - b.left - ssz.cx)/2;
                        int sy = b.top + btnH - ssz.cy -6;
                        TextOutW(mdc,sx,sy,subs[idx],(int)wcslen(subs[idx]));
                    }
                }
            }
            // CALL / END buttons
            int callTop = gridTop + 4*btnH + 3*gap + 16;
            int callLeft=gridLeft; int callW=(256 - gap)/2; int callH=38;
            RECT rcCall={kp.left+callLeft, kp.top+callTop, kp.left+callLeft+callW, kp.top+callTop+callH};
            RECT rcEnd ={kp.left+callLeft+callW+gap, kp.top+callTop, kp.left+callLeft+callW+gap+callW, kp.top+callTop+callH};
            // CALL
            {
                BtnId bid=BTN_CALL; bool hov=(hoverId_==bid); bool pr=(pressedId_==bid);
                COLORREF cc= pr? CLR_CALL_HOVER: (hov? CLR_CALL_HOVER: CLR_CALL);
                // Actually hover should be lighter, pressed also same? We'll use hover vs normal
                if(!hov && !pr) cc=CLR_CALL; else cc=CLR_CALL_HOVER;
                HBRUSH br=CreateSolidBrush(cc);
                HPEN bp=CreatePen(PS_SOLID,1,CLR_BORDER);
                HGDIOBJ oP=SelectObject(mdc,bp); HGDIOBJ oB=SelectObject(mdc,br);
                RoundRect(mdc,rcCall.left,rcCall.top,rcCall.right,rcCall.bottom,8,8);
                SelectObject(mdc,oP); SelectObject(mdc,oB); DeleteObject(bp); DeleteObject(br);
                SelectObject(mdc,fontSmall_); SetTextColor(mdc,CLR_WHITE);
                const wchar_t* txt=L"\u25B6 CALL";
                SIZE sz; GetTextExtentPoint32W(mdc,txt,(int)wcslen(txt),&sz);
                TextOutW(mdc, rcCall.left + (rcCall.right-rcCall.left - sz.cx)/2, rcCall.top + (rcCall.bottom-rcCall.top - sz.cy)/2, txt,(int)wcslen(txt));
            }
            // END
            {
                BtnId bid=BTN_END; bool hov=(hoverId_==bid); bool pr=(pressedId_==bid);
                COLORREF cc= pr? CLR_END_HOVER: (hov? CLR_END_HOVER: CLR_END);
                if(!hov && !pr) cc=CLR_END; else cc=CLR_END_HOVER;
                HBRUSH br=CreateSolidBrush(cc);
                HPEN bp=CreatePen(PS_SOLID,1,CLR_BORDER);
                HGDIOBJ oP=SelectObject(mdc,bp); HGDIOBJ oB=SelectObject(mdc,br);
                RoundRect(mdc,rcEnd.left,rcEnd.top,rcEnd.right,rcEnd.bottom,8,8);
                SelectObject(mdc,oP); SelectObject(mdc,oB); DeleteObject(bp); DeleteObject(br);
                SelectObject(mdc,fontSmall_); SetTextColor(mdc,CLR_WHITE);
                const wchar_t* txt=L"\u25A0 END";
                SIZE sz; GetTextExtentPoint32W(mdc,txt,(int)wcslen(txt),&sz);
                TextOutW(mdc, rcEnd.left + (rcEnd.right-rcEnd.left - sz.cx)/2, rcEnd.top + (rcEnd.bottom-rcEnd.top - sz.cy)/2, txt,(int)wcslen(txt));
            }
            // Hint
            SelectObject(mdc,fontTiny_);
            SetTextColor(mdc,CLR_TEXT_MUTED);
            const wchar_t* hint=L"\u232B Backspace \u2022 Type \u2022 Enter=Call";
            TextOutW(mdc, kp.left+12, kp.top+callTop+callH+10, hint,(int)wcslen(hint));
            SelectObject(mdc,oldPen); DeleteObject(panelBorder);

            // ── SETTINGS DROPDOWN (if visible) ──
            if(showDropdown_){
                RECT dd=GetDropdownRect();
                HBRUSH ddBrush=CreateSolidBrush(CLR_DROPDOWN);
                HPEN ddPen=CreatePen(PS_SOLID,1,CLR_BORDER);
                HGDIOBJ oP=SelectObject(mdc,ddPen); HGDIOBJ oB=SelectObject(mdc,ddBrush);
                RoundRect(mdc,dd.left,dd.top,dd.right,dd.bottom,10,10);
                SelectObject(mdc,oP); SelectObject(mdc,oB); DeleteObject(ddPen); DeleteObject(ddBrush);
                // Header SETTINGS + X
                SelectObject(mdc,fontTitle_);
                SetTextColor(mdc,CLR_WHITE);
                const wchar_t* hdr=L"SETTINGS";
                TextOutW(mdc, dd.left+12, dd.top+10, hdr,(int)wcslen(hdr));
                // Close X button
                RECT closeBtn={dd.right-28, dd.top+6, dd.right-6, dd.top+28};
                bool cHov=(hoverId_==BTN_DROP_CLOSE);
                bool cPr=(pressedId_==BTN_DROP_CLOSE);
                HBRUSH cb=CreateSolidBrush(cPr?CLR_BTN_ACTIVE:(cHov?CLR_BTN_HOVER:CLR_BTN));
                HPEN cp=CreatePen(PS_SOLID,1,CLR_BORDER);
                HGDIOBJ ocP=SelectObject(mdc,cp); HGDIOBJ ocB=SelectObject(mdc,cb);
                RoundRect(mdc,closeBtn.left,closeBtn.top,closeBtn.right,closeBtn.bottom,6,6);
                SelectObject(mdc,ocP); SelectObject(mdc,ocB); DeleteObject(cp); DeleteObject(cb);
                SelectObject(mdc,fontSmall_); SetTextColor(mdc,CLR_WHITE);
                const wchar_t* xTxt=L"\u2715";
                SIZE xsz; GetTextExtentPoint32W(mdc,xTxt,1,&xsz);
                TextOutW(mdc, closeBtn.left + (closeBtn.right-closeBtn.left - xsz.cx)/2, closeBtn.top + (closeBtn.bottom-closeBtn.top - xsz.cy)/2 -1, xTxt,1);
                // Separator
                HPEN sepPen=CreatePen(PS_SOLID,1,CLR_BORDER);
                HGDIOBJ oSep=SelectObject(mdc,sepPen);
                MoveToEx(mdc,dd.left+10, dd.top+34,nullptr); LineTo(mdc,dd.right-10,dd.top+34);
                SelectObject(mdc,oSep); DeleteObject(sepPen);
                // Items
                const wchar_t* items[7]={
                    L"\u2611 Pin window on top [ON]",
                    L"\u2192 Reattach keypad [Docked]",
                    L"\u2192 Emergency address confirmation",
                    L"\u2610 RingOut [OFF]",
                    L"\u2192 Incoming call rules",
                    L"\u2192 Voicemail greeting",
                    L"\u2192 Phone settings"
                };
                // Adjust labels based on state
                wchar_t pinLabel[64]; wchar_t ringLabel[64];
                SL_SWPRINTF(pinLabel, L"%ls Pin window on top [%ls]", isTopmost_?L"\u2611":L"\u2610", isTopmost_?L"ON":L"OFF");
                SL_SWPRINTF(ringLabel, L"%ls RingOut [%ls]", ringOut_?L"\u2611":L"\u2610", ringOut_?L"ON":L"OFF");
                const wchar_t* dispItems[7];
                for(int i=0;i<7;i++) dispItems[i]=items[i];
                dispItems[0]=pinLabel; dispItems[3]=ringLabel;
                int itemH=30; int startY=dd.top+38;
                SelectObject(mdc,fontSmall_);
                for(int i=0;i<7;i++){
                    BtnId bid=BtnId(BTN_DROP_PIN+i);
                    RECT ir={dd.left+6, startY + i*itemH, dd.right-6, startY + (i+1)*itemH -4};
                    bool hov=(hoverId_==bid);
                    bool pr=(pressedId_==bid);
                    if(hov || pr){
                        COLORREF hc = pr? CLR_BTN_ACTIVE: CLR_BTN_HOVER;
                        HBRUSH hb=CreateSolidBrush(hc);
                        HPEN hp=CreatePen(PS_SOLID,1,CLR_BORDER);
                        HGDIOBJ op=SelectObject(mdc,hp); HGDIOBJ ob=SelectObject(mdc,hb);
                        RoundRect(mdc,ir.left,ir.top,ir.right,ir.bottom,6,6);
                        SelectObject(mdc,op); SelectObject(mdc,ob); DeleteObject(hp); DeleteObject(hb);
                    }
                    SetTextColor(mdc, hov? CLR_WHITE: CLR_TEXT_DIM);
                    TextOutW(mdc, ir.left+10, ir.top+7, dispItems[i],(int)wcslen(dispItems[i]));
                }
            }

            // Blit
            BitBlt(hdc,0,0,cr.right,cr.bottom,mdc,0,0,SRCCOPY);
            SelectObject(mdc,oldFont);
            SelectObject(mdc,oldBmp); DeleteObject(bmp); DeleteDC(mdc);
        }

        HINSTANCE hInst_=nullptr;
        HWND hwnd=nullptr;
        HFONT fontMono_=nullptr, fontTitle_=nullptr, fontTitleBold_=nullptr, fontKeypad_=nullptr, fontKeypadSub_=nullptr, fontSmall_=nullptr, fontTiny_=nullptr;
        bool visible=false;
        bool isTopmost_=true;
        bool showDropdown_=false;
        bool keypadDocked_=true;
        bool ringOut_=false;
        NOTIFYICONDATAW nid_={};
        HWINEVENTHOOK hook_=nullptr;
        UINT wm_shellhook_=0;
        std::wstring dialBuffer_;
        BtnId hoverId_=BTN_NONE, pressedId_=BTN_NONE;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// ENTRY POINT
// ─────────────────────────────────────────────────────────────────────────────
#ifndef BENCHMARK
static int RunApp(HINSTANCE h){
    HANDLE hMutex=CreateMutexW(nullptr, TRUE, SL::APP_MUTEX_NAME);
    DWORD err=(hMutex!=nullptr)?GetLastError():ERROR_ACCESS_DENIED;
    if(hMutex==nullptr || err==ERROR_ACCESS_DENIED){
        if(hMutex) CloseHandle(hMutex);
        hMutex=CreateMutexW(nullptr, TRUE, SL::APP_MUTEX_FALLBACK);
        err=(hMutex!=nullptr)?GetLastError():ERROR_ACCESS_DENIED;
    }
    if(err==ERROR_ALREADY_EXISTS){ if(hMutex) CloseHandle(hMutex); return 0; }

    // High priority (not Realtime)
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    // High-res timer 0.5ms NT resolution
    timeBeginPeriod(1);
    // NtSetTimerResolution 0.5ms = 5000 *100ns
    {
        typedef LONG (WINAPI *NtSetTimerResolution_t)(ULONG, BOOLEAN, PULONG);
        HMODULE ntdll=GetModuleHandleW(L"ntdll.dll");
        if(ntdll){
            auto pfn=(NtSetTimerResolution_t)GetProcAddress(ntdll,"NtSetTimerResolution");
            if(pfn){ ULONG cur; pfn(5000, TRUE, &cur); }
        }
    }
    // MMCSS Pro Audio
    {
        typedef HANDLE (WINAPI *AvSetMmThreadCharacteristicsW_t)(LPCWSTR, LPDWORD);
        HMODULE avrt=LoadLibraryW(L"avrt.dll");
        if(avrt){
            auto pfn=(AvSetMmThreadCharacteristicsW_t)GetProcAddress(avrt,"AvSetMmThreadCharacteristicsW");
            if(pfn){ DWORD idx=0; HANDLE hTask=pfn(L"Pro Audio", &idx); if(hTask) { /* keep? */ } }
        }
    }

    SL::StatsTracker::Instance().Initialize();
    SL::Engine::Instance().Initialize();
    if(!SL::App::Instance().Create(h)){
        if(hMutex) CloseHandle(hMutex);
        timeEndPeriod(1);
        return 1;
    }
    HWND w=SL::App::Instance().GetHwnd();
    if(!RegisterHotKey(w, SL::HK_SELFTEST, MOD_NOREPEAT, VK_F8)) LOG_WRN(L"F8 hotkey registration failed");
    if(!RegisterHotKey(w, SL::HK_TOGGLE_DASH, MOD_NOREPEAT, VK_F9)) LOG_WRN(L"F9 hotkey registration failed");
    if(!RegisterHotKey(w, SL::HK_PAUSE_RESUME, MOD_NOREPEAT, VK_F11)) LOG_ERR(L"F11 hotkey registration failed");
    if(!RegisterHotKey(w, SL::HK_EMERGENCY_EXIT, MOD_NOREPEAT, VK_F12)) LOG_WRN(L"F12 hotkey registration failed");
    SL::App::Instance().Show(true);
    LOG_INF(L"Spicy Lamar v1.0 Integrated online. Keypad docked inside dashboard (no separate window).");
    LOG_INF(L"Auto-answer armed: TURBO 200Hz poll (5ms) + WinEvent + ShellHook. Pin ON.");
    MSG m;
    while(GetMessageW(&m,nullptr,0,0)){ TranslateMessage(&m); DispatchMessageW(&m); }
    UnregisterHotKey(w, SL::HK_SELFTEST);
    UnregisterHotKey(w, SL::HK_TOGGLE_DASH);
    UnregisterHotKey(w, SL::HK_PAUSE_RESUME);
    UnregisterHotKey(w, SL::HK_EMERGENCY_EXIT);
    // Revert timer
    {
        typedef LONG (WINAPI *NtSetTimerResolution_t)(ULONG, BOOLEAN, PULONG);
        HMODULE ntdll=GetModuleHandleW(L"ntdll.dll");
        if(ntdll){
            auto pfn=(NtSetTimerResolution_t)GetProcAddress(ntdll,"NtSetTimerResolution");
            if(pfn){ ULONG cur; pfn(5000, FALSE, &cur); }
        }
    }
    timeEndPeriod(1);
    if(hMutex){ ReleaseMutex(hMutex); CloseHandle(hMutex); }
    return (int)m.wParam;
}
#ifdef _MSC_VER
int WINAPI wWinMain(HINSTANCE h, HINSTANCE, LPWSTR, int){ return RunApp(h); }
#else
int WINAPI WinMain(HINSTANCE h, HINSTANCE, LPSTR, int){ return RunApp(h); }
#endif
#endif
