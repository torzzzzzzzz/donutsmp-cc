// Nero Tweaks - native Windows app (Win32 + GDI, plain C)
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <powrprof.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define S(x) ((int)((x) * g_sc + 0.5))
#define HIST 60
#define NTWEAKS 81

static double g_sc = 1.0;
static HWND g_hwnd, g_edit;
static HFONT fTitle, fH2, fBody, fSmall, fStat, fNav, fNavSub, fBtn;
static HBITMAP g_logo;
static int g_logoW, g_logoH;
static WCHAR g_name[64];
static WCHAR g_ini[MAX_PATH];
static int g_page = 0, g_scroll = 0, g_contentH = 0, g_welcome = 0;
static int g_mx = -1, g_my = -1;
static WCHAR g_status[200] = L"";

#define RGBW(v) RGB(v, v, v)
#define C_BG RGB(0, 0, 0)
#define C_SIDE RGB(7, 7, 7)
#define C_PANEL RGB(13, 13, 13)
#define C_LINE RGB(42, 42, 42)
#define C_DIM RGB(138, 138, 138)
#define C_WHITE RGB(255, 255, 255)

// ---------- data ----------
typedef struct { const WCHAR *name, *desc; int on; } Item;
typedef struct { const WCHAR *section, *prefix; int n; Item *items; } List;

static Item tweaks[NTWEAKS];
static WCHAR extraName[NTWEAKS][32];
static const WCHAR *tw[][2] = {
 {L"Disable Fullscreen Optimizations", L"Stops Windows from interfering with fullscreen games."},
 {L"High Performance Power Plan", L"Keeps your CPU running at full speed."},
 {L"Disable Game DVR", L"Turns off background recording that costs FPS."},
 {L"Disable Xbox Game Bar", L"Removes the overlay that can cause stutters."},
 {L"Set GPU Priority High", L"Gives your game first pick of the graphics card."},
 {L"Disable Core Parking", L"Keeps all CPU cores awake and ready."},
 {L"Set Fortnite CPU Priority", L"Makes Windows favor Fortnite over other apps."},
 {L"Disable Power Throttling", L"Stops Windows slowing the CPU to save power."},
 {L"Optimize Visual Effects", L"Turns off animations to free up resources."},
 {L"Disable Transparency", L"Removes see-through windows to save GPU power."},
 {L"Disable Nagle's Algorithm", L"Sends game data right away for lower ping."},
 {L"Optimize TCP ACK Frequency", L"Cuts small delays in network replies."},
 {L"Flush DNS Cache", L"Clears stale network lookups."},
 {L"Disable Update Delivery", L"Stops your PC uploading updates to others."},
 {L"Disable Mouse Acceleration", L"Makes your aim consistent and predictable."},
 {L"Reduce Input Latency", L"Lowers the delay between click and action."},
 {L"Timer Resolution 0.5ms", L"Makes the system clock more precise."},
 {L"Optimize MSI Mode", L"Speeds up how devices talk to the CPU."},
 {L"Disable HPET", L"Can reduce stutter on some systems."},
 {L"Disable Telemetry", L"Stops Windows sending usage data."},
 {L"Disable Cortana", L"Turns off the voice assistant."},
 {L"Disable Windows Tips", L"Hides tips and suggestions."},
 {L"Clear Shader Cache", L"Removes old graphics files."},
 {L"Disable Hibernation", L"Frees disk space used by hibernate."},
 {L"Disable SysMain", L"Stops background preloading that uses your disk."},
 {L"Disable Background Apps", L"Stops apps running when you're not using them."},
 {L"Disable Startup Delay", L"Makes apps launch at login faster."},
 {L"Disable Search Indexing", L"Reduces background disk activity."},
 {L"Optimize Page File", L"Tunes virtual memory for gaming."},
 {L"Disable Spectre Mitigations", L"Small speed boost, lowers some security protection."},
};
static Item games[] = {
 {L"Fortnite - Performance", L"Highest FPS, lowest visuals.", 0},
 {L"Fortnite - Competitive", L"Low input delay, clear visibility.", 0}};
static Item addons[] = {
 {L"FPS Overlay", L"Show your frame rate on screen.", 0},
 {L"Shader Cache Cleaner", L"Clears old graphics files.", 0},
 {L"Background App Closer", L"Closes apps that slow your game.", 0}};
static List lists[] = {
 {L"tweaks", L"t", NTWEAKS, tweaks},
 {L"games", L"g", 2, games},
 {L"addons", L"a", 3, addons}};

static const WCHAR *navName[] = {L"Home", L"Restore Point", L"Game Library", L"Optimizations", L"Addons", L"Ultimate Mode", L"AI Chat"};
static const WCHAR *navSub[] = {L"Your PC at a glance", L"Back up first", L"Pick a game", L"Turn tweaks on/off", L"Optional extras", L"Everything at once", L"Quick answers"};

// ---------- config ----------
static void cfgPath(void) {
    WCHAR dir[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, dir))) {
        wcscat(dir, L"\\NeroTweaks");
        CreateDirectoryW(dir, NULL);
        swprintf(g_ini, MAX_PATH, L"%ls\\config.ini", dir);
    } else wcscpy(g_ini, L"nero.ini");
}
static void saveItem(int l, int i) {
    WCHAR k[16];
    swprintf(k, 16, L"%ls%d", lists[l].prefix, i);
    WritePrivateProfileStringW(lists[l].section, k, lists[l].items[i].on ? L"1" : L"0", g_ini);
}
static void loadConfig(void) {
    GetPrivateProfileStringW(L"user", L"name", L"", g_name, 64, g_ini);
    for (int l = 0; l < 3; l++)
        for (int i = 0; i < lists[l].n; i++) {
            WCHAR k[16];
            swprintf(k, 16, L"%ls%d", lists[l].prefix, i);
            lists[l].items[i].on = GetPrivateProfileIntW(lists[l].section, k, 0, g_ini);
        }
}
static int appliedCount(void) {
    int c = 0;
    for (int i = 0; i < NTWEAKS; i++) c += tweaks[i].on;
    return c;
}
static void setAll(int v) {
    for (int i = 0; i < NTWEAKS; i++) { tweaks[i].on = v; saveItem(0, i); }
}

// ---------- live stats ----------
typedef struct { double cpuTemp, cpuLoad, cpuSpeed, gpuTemp, gpuLoad, gpuMem, diskFree, diskUse, ram, netTotal, netSent, netRecv; } Stats;
static Stats st;
static volatile LONG gpuT = -1, gpuU = -1, gpuM = -1;
static float hist[5][HIST];

typedef struct { ULONG Number, MaxMhz, CurrentMhz, MhzLimit, MaxIdleState, CurrentIdleState; } PPI;
static void readCpu(void) {
    static ULONGLONG pi, pk, pu; static int init;
    FILETIME i, k, u;
    if (!GetSystemTimes(&i, &k, &u)) return;
    ULONGLONG I = ((ULONGLONG)i.dwHighDateTime << 32) | i.dwLowDateTime;
    ULONGLONG K = ((ULONGLONG)k.dwHighDateTime << 32) | k.dwLowDateTime;
    ULONGLONG U = ((ULONGLONG)u.dwHighDateTime << 32) | u.dwLowDateTime;
    if (init) {
        ULONGLONG di = I - pi, dt = (K - pk) + (U - pu);
        st.cpuLoad = dt ? 100.0 * (double)(dt - di) / (double)dt : 0;
    }
    pi = I; pk = K; pu = U; init = 1;
    SYSTEM_INFO si; GetSystemInfo(&si);
    int n = (int)si.dwNumberOfProcessors;
    PPI *p = calloc(n, sizeof *p);
    if (p && CallNtPowerInformation(ProcessorInformation, NULL, 0, p, n * sizeof *p) == 0) {
        double s = 0; for (int j = 0; j < n; j++) s += p[j].CurrentMhz;
        st.cpuSpeed = s / n / 1000.0;
    }
    free(p);
}
static void readNet(void) {
    static DWORD pin[512], pout[512], pidx[512]; static int pn; static DWORD pt; static int init;
    ULONG sz = 0;
    GetIfTable(NULL, &sz, FALSE);
    MIB_IFTABLE *t = malloc(sz);
    if (!t) return;
    if (GetIfTable(t, &sz, FALSE) != NO_ERROR) { free(t); return; }
    DWORD now = GetTickCount();
    double dIn = 0, dOut = 0;
    static DWORD nin[512], nout[512], nidx[512]; int nn = 0;
    for (DWORD i = 0; i < t->dwNumEntries && nn < 512; i++) {
        MIB_IFROW *r = &t->table[i];
        if (r->dwType == IF_TYPE_SOFTWARE_LOOPBACK || r->dwOperStatus != IF_OPER_STATUS_OPERATIONAL) continue;
        nidx[nn] = r->dwIndex; nin[nn] = r->dwInOctets; nout[nn] = r->dwOutOctets;
        for (int j = 0; j < pn; j++) if (pidx[j] == r->dwIndex) { dIn += (DWORD)(r->dwInOctets - pin[j]); dOut += (DWORD)(r->dwOutOctets - pout[j]); }
        nn++;
    }
    free(t);
    if (init && now > pt) {
        double dt = (now - pt) / 1000.0;
        st.netRecv = dIn * 8 / 1e6 / dt; st.netSent = dOut * 8 / 1e6 / dt; st.netTotal = st.netRecv + st.netSent;
    }
    memcpy(pin, nin, sizeof nin); memcpy(pout, nout, sizeof nout); memcpy(pidx, nidx, sizeof nidx);
    pn = nn; pt = now; init = 1;
}
static void readStats(void) {
    readCpu(); readNet();
    MEMORYSTATUSEX m = {sizeof m};
    if (GlobalMemoryStatusEx(&m)) st.ram = m.dwMemoryLoad;
    ULARGE_INTEGER fr, tot, tf;
    if (GetDiskFreeSpaceExW(L"C:\\", &fr, &tot, &tf)) {
        st.diskFree = (double)fr.QuadPart / 1073741824.0;
        st.diskUse = 100.0 * (1.0 - (double)fr.QuadPart / (double)tot.QuadPart);
    }
    st.gpuTemp = gpuT; st.gpuLoad = gpuU; st.gpuMem = gpuM >= 0 ? gpuM / 1024.0 : -1;
    double v[5] = {st.cpuLoad / 100, st.gpuLoad >= 0 ? st.gpuLoad / 100 : 0, st.diskUse / 100, st.ram / 100, st.netTotal / 100};
    if (v[4] > 1) v[4] = 1;
    for (int c = 0; c < 5; c++) {
        memmove(hist[c], hist[c] + 1, (HIST - 1) * sizeof(float));
        hist[c][HIST - 1] = (float)v[c];
    }
}

// Runs a hidden command and captures stdout. Returns exit code, or -1 if it could not start.
static int runHidden(WCHAR *cmd, char *out, int outLen) {
    SECURITY_ATTRIBUTES sa = {sizeof sa, NULL, TRUE};
    HANDLE rd, wr;
    if (!CreatePipe(&rd, &wr, &sa, 0)) return -1;
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si = {sizeof si}; PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; si.hStdOutput = wr; si.hStdError = wr;
    if (!CreateProcessW(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(rd); CloseHandle(wr); return -1;
    }
    CloseHandle(wr);
    int n = 0; DWORD got;
    while (n < outLen - 1 && ReadFile(rd, out + n, outLen - 1 - n, &got, NULL) && got) n += got;
    out[n] = 0;
    WaitForSingleObject(pi.hProcess, 60000);
    DWORD code = 1; GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread); CloseHandle(rd);
    return (int)code;
}
static DWORD WINAPI gpuThread(LPVOID p) {
    (void)p;
    for (;;) {
        WCHAR cmd[] = L"nvidia-smi.exe --query-gpu=temperature.gpu,utilization.gpu,memory.used --format=csv,noheader,nounits";
        char buf[256]; int t, u, m;
        if (runHidden(cmd, buf, sizeof buf) == 0 && sscanf(buf, "%d, %d, %d", &t, &u, &m) == 3) {
            gpuT = t; gpuU = u; gpuM = m;
        } else { gpuT = gpuU = gpuM = -1; }
        Sleep(2000);
    }
    return 0;
}
static DWORD WINAPI restoreThread(LPVOID p) {
    (void)p;
    WCHAR cmd[] = L"powershell.exe -NoProfile -Command \"Checkpoint-Computer -Description 'Nero Tweaks' -RestorePointType MODIFY_SETTINGS -ErrorAction Stop\"";
    char buf[1024];
    int rc = runHidden(cmd, buf, sizeof buf);
    if (rc == 0) wcscpy(g_status, L"Restore point created.");
    else wcscpy(g_status, L"Could not create it. Close Nero Tweaks, right-click it and choose Run as administrator, then try again. (Windows also allows only one restore point per 24 hours.)");
    PostMessageW(g_hwnd, WM_APP, 0, 0);
    return 0;
}

// ---------- drawing helpers ----------
enum { K_NAV, K_STEP, K_TOG, K_BTN, K_NAME };
enum { B_ALLON, B_ALLOFF, B_RESTORE, B_ULTIMATE, B_WELCOME };
typedef struct { RECT r; int kind, a, b; } Hit;
static Hit hits[400]; static int nhits;
static void addHit(RECT r, int kind, int a, int b) {
    if (nhits < 400) { hits[nhits].r = r; hits[nhits].kind = kind; hits[nhits].a = a; hits[nhits].b = b; nhits++; }
}
static int hovered(RECT r) { POINT p = {g_mx, g_my}; return PtInRect(&r, p); }

static int Text(HDC dc, const WCHAR *s, int x, int y, int w, HFONT f, COLORREF c, int glow, UINT fmt) {
    RECT r = {x, y, x + w, y + 4000}, m = r;
    SelectObject(dc, f);
    DrawTextW(dc, s, -1, &m, fmt | DT_CALCRECT | DT_NOPREFIX);
    int h = m.bottom - m.top;
    if (glow) {
        SetTextColor(dc, RGB(GetRValue(c) * 3 / 10, GetGValue(c) * 3 / 10, GetBValue(c) * 3 / 10));
        int d[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (int i = 0; i < 4; i++) { RECT g = r; OffsetRect(&g, d[i][0] * S(1), d[i][1] * S(1)); DrawTextW(dc, s, -1, &g, fmt | DT_NOPREFIX); }
    }
    SetTextColor(dc, c);
    DrawTextW(dc, s, -1, &r, fmt | DT_NOPREFIX);
    return h;
}
static void Box(HDC dc, RECT r, COLORREF fill, COLORREF line, int rad) {
    HPEN p = CreatePen(PS_SOLID, line == C_WHITE ? S(2) : 1, line);
    HBRUSH b = CreateSolidBrush(fill);
    HGDIOBJ op = SelectObject(dc, p), ob = SelectObject(dc, b);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, rad, rad);
    SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(p); DeleteObject(b);
}
static void GlowBox(HDC dc, RECT r, int rad) {
    for (int i = 3; i >= 1; i--) {
        RECT g = r; InflateRect(&g, S(i), S(i));
        HPEN p = CreatePen(PS_SOLID, 1, RGBW(30 + (3 - i) * 15));
        HGDIOBJ op = SelectObject(dc, p), ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
        RoundRect(dc, g.left, g.top, g.right, g.bottom, rad + S(i), rad + S(i));
        SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(p);
    }
    Box(dc, r, C_PANEL, C_WHITE, rad);
}
static void Button(HDC dc, RECT r, const WCHAR *label, int id) {
    int hv = hovered(r);
    if (hv) { Box(dc, r, C_WHITE, C_WHITE, S(12)); Text(dc, label, r.left, r.top + (r.bottom - r.top - S(18)) / 2, r.right - r.left, fBtn, C_BG, 0, DT_CENTER | DT_SINGLELINE); }
    else { GlowBox(dc, r, S(12)); Text(dc, label, r.left, r.top + (r.bottom - r.top - S(18)) / 2, r.right - r.left, fBtn, C_WHITE, 1, DT_CENTER | DT_SINGLELINE); }
    addHit(r, K_BTN, id, 0);
}
static void Switch(HDC dc, int x, int y, int on) {
    int w = S(38), h = S(20);
    RECT r = {x, y, x + w, y + h};
    Box(dc, r, on ? RGBW(34) : RGBW(28), on ? C_WHITE : RGBW(80), h);
    int cx = on ? x + w - h + S(4) : x + S(4), d = h - S(8);
    HBRUSH b = CreateSolidBrush(on ? C_WHITE : RGBW(120));
    HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, GetStockObject(NULL_PEN));
    if (on) { HBRUSH gb = CreateSolidBrush(RGBW(90)); SelectObject(dc, gb); Ellipse(dc, cx - S(2), y + S(2), cx + d + S(2), y + h - S(2)); SelectObject(dc, b); DeleteObject(gb); }
    Ellipse(dc, cx, y + S(4), cx + d, y + h - S(4));
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b);
}
static void Graph(HDC dc, RECT r, float *h) {
    HPEN grid = CreatePen(PS_SOLID, 1, RGBW(24));
    HGDIOBJ op = SelectObject(dc, grid);
    for (int i = 1; i < 4; i++) { int y = r.top + (r.bottom - r.top) * i / 4; MoveToEx(dc, r.left, y, NULL); LineTo(dc, r.right, y); }
    POINT pts[HIST];
    for (int i = 0; i < HIST; i++) {
        pts[i].x = r.left + (r.right - r.left) * i / (HIST - 1);
        float v = h[i]; if (v < 0) v = 0; if (v > 1) v = 1;
        pts[i].y = r.bottom - S(3) - (int)(v * (r.bottom - r.top - S(6)));
    }
    int wd[3] = {S(7), S(4), S(2)}, cl[3] = {22, 70, 255};
    for (int k = 0; k < 3; k++) {
        HPEN p = CreatePen(PS_SOLID, wd[k], RGBW(cl[k]));
        SelectObject(dc, p); Polyline(dc, pts, HIST); SelectObject(dc, grid); DeleteObject(p);
    }
    SelectObject(dc, op); DeleteObject(grid);
}

// ---------- pages ----------
static void fmtVal(WCHAR *o, double v, int dec, const WCHAR *unit) {
    if (v < 0) swprintf(o, 32, L"--"); else swprintf(o, 32, L"%.*f%ls", dec, v, unit);
}
typedef struct { const WCHAR *label; double *v; int dec; const WCHAR *unit; } Field;
static void Card(HDC dc, RECT r, const WCHAR *title, Field *f, int nf, float *h) {
    Box(dc, r, C_PANEL, hovered(r) ? C_WHITE : C_LINE, S(14));
    HBRUSH b = CreateSolidBrush(C_WHITE); HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, GetStockObject(NULL_PEN));
    int cx = r.left + S(22), cy = r.top + S(26);
    Ellipse(dc, cx - S(3), cy - S(3), cx + S(4), cy + S(4));
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b);
    Text(dc, title, r.left + S(34), r.top + S(16), r.right - r.left, fH2, C_WHITE, 1, DT_SINGLELINE);
    int fw = (r.right - r.left - S(36)) / nf;
    for (int i = 0; i < nf; i++) {
        WCHAR v[32]; fmtVal(v, *f[i].v, f[i].dec, f[i].unit);
        int x = r.left + S(18) + i * fw;
        Text(dc, f[i].label, x, r.top + S(44), fw, fSmall, C_DIM, 0, DT_SINGLELINE);
        Text(dc, v, x, r.top + S(62), fw, fStat, C_WHITE, 1, DT_SINGLELINE);
    }
    RECT gr = {r.left + S(14), r.top + S(104), r.right - S(14), r.bottom - S(10)};
    Graph(dc, gr, h);
}

static int pageHome(HDC dc, int x, int y, int w) {
    WCHAR hello[128]; swprintf(hello, 128, L"Hello, %ls", g_name[0] ? g_name : L"Gamer");
    int h = Text(dc, hello, x, y, w, fTitle, C_WHITE, 1, DT_SINGLELINE);
    RECT nr = {x, y, x + w, y + h}; addHit(nr, K_NAME, 0, 0);
    y += h + S(6);
    y += Text(dc, L"Welcome back - let's get you smoother Fortnite with fewer lag spikes.  (click your name to change it)", x, y, w, fBody, C_DIM, 0, DT_WORDBREAK) + S(26);
    y += Text(dc, L"GETTING STARTED", x, y, w, fH2, RGBW(210), 1, DT_SINGLELINE) + S(12);
    static const WCHAR *st1[3] = {L"1  Make a safety backup", L"2  Pick your tweaks", L"3  Or do it all at once"};
    static const WCHAR *st2[3] = {L"One click, and you can undo everything later.", L"Switch on what you want. Each one explains itself.", L"Ultimate Mode turns on every tweak."};
    static const int tgt[3] = {1, 3, 5};
    int n = 3, gap = S(14), cw = (w - gap * (n - 1)) / n, ch = S(92);
    for (int i = 0; i < n; i++) {
        RECT r = {x + i * (cw + gap), y, x + i * (cw + gap) + cw, y + ch};
        Box(dc, r, C_PANEL, hovered(r) ? C_WHITE : C_LINE, S(14));
        Text(dc, st1[i], r.left + S(16), r.top + S(14), cw - S(32), fBody, C_WHITE, 1, DT_SINGLELINE);
        Text(dc, st2[i], r.left + S(16), r.top + S(42), cw - S(32), fSmall, C_DIM, 0, DT_WORDBREAK);
        addHit(r, K_STEP, tgt[i], 0);
    }
    y += ch + S(30);
    y += Text(dc, L"YOUR PC RIGHT NOW", x, y, w, fH2, RGBW(210), 1, DT_SINGLELINE) + S(12);
    Field cpu[] = {{L"Temperature", &st.cpuTemp, 0, L"\u00b0C"}, {L"Usage", &st.cpuLoad, 0, L"%"}, {L"Speed", &st.cpuSpeed, 2, L" GHz"}};
    Field gpu[] = {{L"Temperature", &st.gpuTemp, 0, L"\u00b0C"}, {L"Usage", &st.gpuLoad, 0, L"%"}, {L"Memory", &st.gpuMem, 1, L" GB"}};
    Field dsk[] = {{L"Used", &st.diskUse, 0, L"%"}, {L"Free", &st.diskFree, 0, L" GB"}};
    Field ram[] = {{L"Usage", &st.ram, 0, L"%"}};
    Field net[] = {{L"Total", &st.netTotal, 1, L" Mbps"}, {L"Sent", &st.netSent, 1, L" Mbps"}, {L"Received", &st.netRecv, 1, L" Mbps"}};
    const WCHAR *titles[5] = {L"PROCESSOR", L"GRAPHICS CARD", L"DISK", L"RAM", L"NETWORK"};
    Field *fs[5] = {cpu, gpu, dsk, ram, net}; int nfs[5] = {3, 3, 2, 1, 3};
    int cols = (w + gap) / (S(300) + gap); if (cols < 1) cols = 1; if (cols > 3) cols = 3;
    cw = (w - gap * (cols - 1)) / cols; ch = S(170);
    for (int i = 0; i < 5; i++) {
        int c = i % cols, rw = i / cols;
        RECT r = {x + c * (cw + gap), y + rw * (ch + gap), x + c * (cw + gap) + cw, y + rw * (ch + gap) + ch};
        Card(dc, r, titles[i], fs[i], nfs[i], hist[i]);
    }
    y += ((5 + cols - 1) / cols) * (ch + gap);
    y += Text(dc, L"Temperatures need a sensor driver Windows does not provide, so CPU temp shows --. GPU stats show for NVIDIA cards (via nvidia-smi).", x, y, w, fSmall, RGBW(100), 0, DT_WORDBREAK) + S(20);
    return y;
}

static int pageList(HDC dc, int x, int y, int w, int li, int bulk) {
    List *L = &lists[li];
    if (bulk) {
        RECT a = {x, y, x + S(150), y + S(40)}, b = {x + S(166), y, x + S(316), y + S(40)};
        Button(dc, a, L"Turn all on", B_ALLON); Button(dc, b, L"Turn all off", B_ALLOFF);
        y += S(60);
    }
    int cols = w >= S(760) ? 2 : 1, gap = S(10), cw = (w - gap * (cols - 1)) / cols, rh = S(66);
    for (int i = 0; i < L->n; i++) {
        int c = i % cols, rw = i / cols;
        RECT r = {x + c * (cw + gap), y + rw * (rh + gap), x + c * (cw + gap) + cw, y + rw * (rh + gap) + rh};
        Item *it = &L->items[i];
        Box(dc, r, C_PANEL, it->on ? C_WHITE : (hovered(r) ? RGBW(120) : RGBW(35)), S(10));
        Text(dc, it->name, r.left + S(14), r.top + S(12), cw - S(80), fBody, C_WHITE, it->on, DT_SINGLELINE | DT_END_ELLIPSIS);
        Text(dc, it->desc, r.left + S(14), r.top + S(36), cw - S(80), fSmall, C_DIM, 0, DT_SINGLELINE | DT_END_ELLIPSIS);
        Switch(dc, r.right - S(54), r.top + (rh - S(20)) / 2, it->on);
        addHit(r, K_TOG, li, i);
    }
    return y + ((L->n + cols - 1) / cols) * (rh + gap) + S(10);
}

static int pageRestore(HDC dc, int x, int y, int w) {
    RECT b = {x, y, x + S(240), y + S(42)};
    Button(dc, b, L"+ Create restore point", B_RESTORE);
    y += S(62);
    if (g_status[0]) y += Text(dc, g_status, x, y, w > S(700) ? S(700) : w, fBody, C_WHITE, 1, DT_WORDBREAK) + S(12);
    y += Text(dc, L"This asks Windows to save a System Restore point. If something feels off later, open \"Create a restore point\" in Windows and roll back.", x, y, w > S(700) ? S(700) : w, fBody, C_DIM, 0, DT_WORDBREAK);
    return y + S(20);
}
static int pageUltimate(HDC dc, int x, int y, int w) {
    int pw = w > S(720) ? S(720) : w;
    y += Text(dc, L"Turns on all 81 tweaks for the best possible performance. Make a restore point first so you can undo it.", x, y, pw, fBody, C_DIM, 0, DT_WORDBREAK) + S(20);
    RECT a = {x, y, x + S(200), y + S(42)}, b = {x + S(216), y, x + S(456), y + S(42)};
    Button(dc, a, L"Make backup first", B_RESTORE + 100);
    Button(dc, b, L"Enable Ultimate Mode", B_ULTIMATE);
    return y + S(70);
}
static int pageChat(HDC dc, int x, int y, int w) {
    static const WCHAR *qa[][2] = {
        {L"I get low FPS. What should I do first?", L"Open Optimizations and turn on the Performance tweaks. Make a restore point first."},
        {L"How do I reduce lag spikes?", L"Try the Network tweaks (Nagle's Algorithm, TCP ACK) and close background apps."},
        {L"My aim feels floaty.", L"Turn on Disable Mouse Acceleration and Reduce Input Latency in the Input tweaks."},
        {L"Can I undo everything?", L"Yes - turn tweaks off again, or roll back with your restore point."}};
    int pw = w > S(760) ? S(760) : w;
    y += Text(dc, L"A live AI assistant needs an online service and API key, which this offline app doesn't have yet. Here are quick answers for now:", x, y, pw, fBody, C_DIM, 0, DT_WORDBREAK) + S(20);
    for (int i = 0; i < 4; i++) {
        int h1 = S(86); RECT r = {x, y, x + pw, y + h1};
        Box(dc, r, C_PANEL, C_LINE, S(12));
        Text(dc, qa[i][0], r.left + S(16), r.top + S(12), pw - S(32), fBody, C_WHITE, 1, DT_WORDBREAK);
        Text(dc, qa[i][1], r.left + S(16), r.top + S(42), pw - S(32), fSmall, C_DIM, 0, DT_WORDBREAK);
        y += h1 + S(10);
    }
    return y;
}

static void drawLogo(HDC dc, int x, int y, int w) {
    if (!g_logo) return;
    int h = w * g_logoH / g_logoW;
    HDC m = CreateCompatibleDC(dc); HGDIOBJ o = SelectObject(m, g_logo);
    BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    int old = SetStretchBltMode(dc, HALFTONE);
    AlphaBlend(dc, x, y, w, h, m, 0, 0, g_logoW, g_logoH, bf);
    SetStretchBltMode(dc, old);
    SelectObject(m, o); DeleteDC(m);
}

static void layoutEdit(int W, int H) {
    int bw = S(380), bh = S(300);
    int x = (W - bw) / 2, y = (H - bh) / 2;
    MoveWindow(g_edit, x + S(30), y + S(190), bw - S(60), S(36), TRUE);
}

static void paint(HDC wdc, int W, int H) {
    HDC dc = CreateCompatibleDC(wdc);
    HBITMAP bmp = CreateCompatibleBitmap(wdc, W, H);
    HGDIOBJ obmp = SelectObject(dc, bmp);
    SetBkMode(dc, TRANSPARENT);
    nhits = 0;
    RECT all = {0, 0, W, H};
    HBRUSH bg = CreateSolidBrush(C_BG); FillRect(dc, &all, bg); DeleteObject(bg);

    int sw = S(250);
    // main content (scrolls)
    int pad = S(36), x = sw + pad, w = W - sw - pad * 2;
    if (w < S(300)) w = S(300);
    int hitStart = nhits; (void)hitStart;
    HRGN clip = CreateRectRgn(sw, 0, W, H); SelectClipRgn(dc, clip); DeleteObject(clip);
    int y = S(34) - g_scroll;
    const WCHAR *titles[] = {NULL, L"Restore Point", L"Game Library", L"Optimizations", L"Addons", L"Ultimate Mode", L"AI Chat"};
    const WCHAR *taglines[] = {NULL, L"A backup of your settings. If anything feels off, you can go back.", L"Choose which game profile to optimize.", L"Click any tweak to turn it on or off.", L"Optional extras you can add on.", L"One click, every tweak.", L"Not sure what to pick? Start here."};
    int end;
    if (g_page == 0) end = pageHome(dc, x, y, w);
    else {
        y += Text(dc, titles[g_page], x, y, w, fTitle, C_WHITE, 1, DT_SINGLELINE) + S(6);
        y += Text(dc, taglines[g_page], x, y, w, fBody, C_DIM, 0, DT_WORDBREAK) + S(24);
        switch (g_page) {
            case 1: end = pageRestore(dc, x, y, w); break;
            case 2: end = pageList(dc, x, y, w, 1, 0); break;
            case 3: end = pageList(dc, x, y, w, 0, 1); break;
            case 4: end = pageList(dc, x, y, w, 2, 0); break;
            case 5: end = pageUltimate(dc, x, y, w); break;
            default: end = pageChat(dc, x, y, w);
        }
    }
    g_contentH = end + g_scroll + S(30);
    SelectClipRgn(dc, NULL);

    // sidebar
    RECT sb = {0, 0, sw, H};
    HBRUSH sbb = CreateSolidBrush(C_SIDE); FillRect(dc, &sb, sbb); DeleteObject(sbb);
    HPEN lp = CreatePen(PS_SOLID, 1, C_LINE); HGDIOBJ op = SelectObject(dc, lp);
    MoveToEx(dc, sw - 1, 0, NULL); LineTo(dc, sw - 1, H); SelectObject(dc, op); DeleteObject(lp);
    drawLogo(dc, S(10), S(14), sw - S(20));
    int lh = (sw - S(20)) * g_logoH / g_logoW;
    Text(dc, L"T  W  E  A  K  S", S(26), S(14) + lh - S(4), sw, fSmall, C_DIM, 0, DT_SINGLELINE);
    int ny = S(14) + lh + S(36);
    for (int i = 0; i < 7; i++) {
        RECT r = {S(14), ny, sw - S(14), ny + S(54)};
        int act = i == g_page, hv = hovered(r);
        if (act) { GlowBox(dc, r, S(12)); }
        else if (hv) Box(dc, r, RGBW(17), RGBW(17), S(12));
        Text(dc, navName[i], r.left + S(18), r.top + S(8), r.right - r.left - S(24), fNav, act || hv ? C_WHITE : RGBW(185), act, DT_SINGLELINE);
        Text(dc, navSub[i], r.left + S(18), r.top + S(31), r.right - r.left - S(24), fNavSub, act ? RGBW(170) : RGBW(100), 0, DT_SINGLELINE);
        addHit(r, K_NAV, i, 0);
        ny += S(58);
    }
    int ac = appliedCount();
    WCHAR t[64]; swprintf(t, 64, L"%d/%d Tweaks Applied", ac, NTWEAKS);
    HPEN lp2 = CreatePen(PS_SOLID, 1, C_LINE); op = SelectObject(dc, lp2);
    MoveToEx(dc, S(14), H - S(76), NULL); LineTo(dc, sw - S(14), H - S(76)); SelectObject(dc, op); DeleteObject(lp2);
    Text(dc, t, S(22), H - S(62), sw, fBody, C_WHITE, 1, DT_SINGLELINE);
    RECT bar = {S(22), H - S(30), sw - S(22), H - S(26)};
    HBRUSH bb = CreateSolidBrush(RGBW(28)); FillRect(dc, &bar, bb); DeleteObject(bb);
    RECT fill = bar; fill.right = bar.left + (bar.right - bar.left) * ac / NTWEAKS;
    HBRUSH fb = CreateSolidBrush(C_WHITE); FillRect(dc, &fill, fb); DeleteObject(fb);

    // welcome overlay
    if (g_welcome) {
        HBRUSH ov = CreateSolidBrush(RGB(2, 2, 2)); FillRect(dc, &all, ov); DeleteObject(ov);
        int bw = S(380), bh = S(300), bx = (W - bw) / 2, by = (H - bh) / 2;
        RECT card = {bx, by, bx + bw, by + bh};
        GlowBox(dc, card, S(18));
        int lw = S(190); drawLogo(dc, bx + (bw - lw) / 2, by + S(20), lw);
        Text(dc, L"Welcome to Nero Tweaks", bx, by + S(112), bw, fStat, C_WHITE, 1, DT_CENTER | DT_SINGLELINE);
        Text(dc, L"What should we call you?", bx, by + S(150), bw, fBody, C_DIM, 0, DT_CENTER | DT_SINGLELINE);
        RECT b = {bx + S(30), by + S(240), bx + bw - S(30), by + S(282)};
        Button(dc, b, L"Let's go", B_WELCOME);
    }
    BitBlt(wdc, 0, 0, W, H, dc, 0, 0, SRCCOPY);
    SelectObject(dc, obmp); DeleteObject(bmp); DeleteDC(dc);
}

// ---------- logic ----------
static void showWelcome(void) {
    g_welcome = 1;
    SetWindowTextW(g_edit, g_name);
    RECT rc; GetClientRect(g_hwnd, &rc);
    layoutEdit(rc.right, rc.bottom);
    ShowWindow(g_edit, SW_SHOW); SetFocus(g_edit);
    SendMessageW(g_edit, EM_SETSEL, 0, -1);
    InvalidateRect(g_hwnd, NULL, FALSE);
}
static void acceptName(void) {
    WCHAR n[64]; GetWindowTextW(g_edit, n, 64);
    WCHAR *s = n; while (*s == L' ') s++;
    if (!*s) { SetFocus(g_edit); return; }
    wcscpy(g_name, s);
    WritePrivateProfileStringW(L"user", L"name", g_name, g_ini);
    g_welcome = 0; ShowWindow(g_edit, SW_HIDE); SetFocus(g_hwnd);
    InvalidateRect(g_hwnd, NULL, FALSE);
}
static void click(int mx, int my) {
    POINT p = {mx, my};
    for (int i = nhits - 1; i >= 0; i--) {
        Hit *h = &hits[i];
        if (!PtInRect(&h->r, p)) continue;
        if (g_welcome && !(h->kind == K_BTN && h->a == B_WELCOME)) continue;
        if (h->kind != K_NAV && mx < S(250)) continue;
        switch (h->kind) {
            case K_NAV: g_page = h->a; g_scroll = 0; break;
            case K_STEP: g_page = h->a; g_scroll = 0; break;
            case K_NAME: showWelcome(); break;
            case K_TOG: { Item *it = &lists[h->a].items[h->b]; it->on = !it->on; saveItem(h->a, h->b); break; }
            case K_BTN:
                if (h->a == B_ALLON) setAll(1);
                else if (h->a == B_ALLOFF) setAll(0);
                else if (h->a == B_ULTIMATE) { setAll(1); g_page = 3; g_scroll = 0; }
                else if (h->a == B_RESTORE) { wcscpy(g_status, L"Working... this can take a few seconds."); CreateThread(NULL, 0, restoreThread, NULL, 0, NULL); }
                else if (h->a == B_RESTORE + 100) { g_page = 1; g_scroll = 0; }
                else if (h->a == B_WELCOME) acceptName();
                break;
        }
        InvalidateRect(g_hwnd, NULL, FALSE);
        return;
    }
}
static int hitIndexAt(int mx, int my) {
    POINT p = {mx, my};
    for (int i = nhits - 1; i >= 0; i--) {
        if (!PtInRect(&hits[i].r, p)) continue;
        if (g_welcome && !(hits[i].kind == K_BTN && hits[i].a == B_WELCOME)) continue;
        if (hits[i].kind != K_NAV && mx < S(250)) continue;
        return i;
    }
    return -1;
}
static void clampScroll(void) {
    RECT rc; GetClientRect(g_hwnd, &rc);
    int max = g_contentH - rc.bottom; if (max < 0) max = 0;
    if (g_scroll > max) g_scroll = max;
    if (g_scroll < 0) g_scroll = 0;
}

static WNDPROC oldEdit;
static LRESULT CALLBACK editProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_KEYDOWN && w == VK_RETURN) { acceptName(); return 0; }
    if (m == WM_CHAR && w == VK_RETURN) return 0;
    return CallWindowProcW(oldEdit, h, m, w, l);
}
static HBRUSH editBrush;

static LRESULT CALLBACK wndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: SetTimer(h, 1, 1000, NULL); return 0;
    case WM_TIMER: readStats(); InvalidateRect(h, NULL, FALSE); return 0;
    case WM_APP: InvalidateRect(h, NULL, FALSE); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        clampScroll();
        paint(dc, rc.right, rc.bottom);
        EndPaint(h, &ps); return 0;
    }
    case WM_SIZE: layoutEdit(LOWORD(l), HIWORD(l)); InvalidateRect(h, NULL, FALSE); return 0;
    case WM_MOUSEMOVE: {
        int ox = g_mx, oy = g_my; g_mx = (short)LOWORD(l); g_my = (short)HIWORD(l);
        if (hitIndexAt(ox, oy) != hitIndexAt(g_mx, g_my)) InvalidateRect(h, NULL, FALSE);
        return 0;
    }
    case WM_SETCURSOR:
        if (LOWORD(l) == HTCLIENT) { SetCursor(LoadCursor(NULL, hitIndexAt(g_mx, g_my) >= 0 ? IDC_HAND : IDC_ARROW)); return TRUE; }
        break;
    case WM_LBUTTONDOWN: click((short)LOWORD(l), (short)HIWORD(l)); return 0;
    case WM_MOUSEWHEEL:
        if (!g_welcome) { g_scroll -= (short)HIWORD(w) * S(60) / 120; clampScroll(); InvalidateRect(h, NULL, FALSE); }
        return 0;
    case WM_CTLCOLOREDIT: {
        HDC d = (HDC)w; SetTextColor(d, C_WHITE); SetBkColor(d, RGB(10, 10, 10));
        return (LRESULT)editBrush;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static HFONT mkFont(int px, int weight) {
    return CreateFontW(-S(px), 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
}
static void loadLogo(void) {
    HRSRC r = FindResourceW(NULL, L"LOGO", RT_RCDATA);
    if (!r) return;
    const int *d = (const int *)LockResource(LoadResource(NULL, r));
    g_logoW = d[0]; g_logoH = d[1];
    BITMAPINFO bi = {{sizeof(BITMAPINFOHEADER), g_logoW, -g_logoH, 1, 32, BI_RGB}};
    void *bits; HDC sd = GetDC(NULL);
    g_logo = CreateDIBSection(sd, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(NULL, sd);
    if (g_logo) memcpy(bits, d + 2, (size_t)g_logoW * g_logoH * 4);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE hp, PWSTR cmd, int show) {
    (void)hp; (void)cmd;
    SetProcessDPIAware();
    HDC sd = GetDC(NULL); g_sc = GetDeviceCaps(sd, LOGPIXELSX) / 96.0; ReleaseDC(NULL, sd);
    for (int i = 0; i < NTWEAKS; i++) {
        if (i < (int)(sizeof tw / sizeof tw[0])) { tweaks[i].name = tw[i][0]; tweaks[i].desc = tw[i][1]; }
        else { swprintf(extraName[i], 32, L"Advanced Tweak #%d", i + 1); tweaks[i].name = extraName[i]; tweaks[i].desc = L"Extra fine-tuning for experienced users."; }
    }
    cfgPath(); loadConfig(); loadLogo();
    fTitle = mkFont(30, FW_SEMIBOLD); fH2 = mkFont(13, FW_BOLD); fBody = mkFont(15, FW_SEMIBOLD);
    fSmall = mkFont(12, FW_NORMAL); fStat = mkFont(23, FW_SEMIBOLD); fNav = mkFont(16, FW_SEMIBOLD);
    fNavSub = mkFont(11, FW_NORMAL); fBtn = mkFont(14, FW_SEMIBOLD);
    editBrush = CreateSolidBrush(RGB(10, 10, 10));

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = wndProc; wc.hInstance = hi; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hi, MAKEINTRESOURCE(1)); wc.lpszClassName = L"NeroTweaks";
    wc.hbrBackground = CreateSolidBrush(C_BG);
    RegisterClassW(&wc);
    g_hwnd = CreateWindowW(L"NeroTweaks", L"Nero Tweaks", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, S(1220), S(800), NULL, NULL, hi, NULL);
    g_edit = CreateWindowW(L"EDIT", L"", WS_CHILD | ES_CENTER | ES_AUTOHSCROLL | WS_BORDER, 0, 0, 10, 10, g_hwnd, NULL, hi, NULL);
    SendMessageW(g_edit, WM_SETFONT, (WPARAM)fStat, TRUE);
    SendMessageW(g_edit, EM_SETLIMITTEXT, 24, 0);
    oldEdit = (WNDPROC)SetWindowLongPtrW(g_edit, GWLP_WNDPROC, (LONG_PTR)editProc);
    ShowWindow(g_hwnd, show); UpdateWindow(g_hwnd);
    SetWindowPos(g_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    st.cpuTemp = -1;
    readStats();
    CreateThread(NULL, 0, gpuThread, NULL, 0, NULL);
    if (!g_name[0]) showWelcome();
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
