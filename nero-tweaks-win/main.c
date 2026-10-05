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
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <wctype.h>
#include <string.h>

#define S(x) ((int)((x) * g_sc + 0.5))
#define HIST 60
#define NTWEAKS 18

static double g_sc = 1.0;
static HWND g_hwnd, g_edit, g_chatEdit;
static int g_modalMode;
static DWORD g_fadeStart;
static RECT g_chatRect; static int g_chatShow;
static HFONT fTitle, fH2, fBody, fSmall, fStat, fNav, fNavSub, fBtn;
static HBITMAP g_logo;
static int g_logoW, g_logoH;
static WCHAR g_name[64];
static WCHAR g_ini[MAX_PATH];
static int g_page = 0, g_scroll = 0, g_contentH = 0, g_welcome = 0;
static void setPage(int p) {
    g_page = p; g_scroll = p == 6 ? (1 << 20) : 0; g_fadeStart = GetTickCount();
    SetTimer(g_hwnd, 3, 16, NULL);
}
static int g_mx = -1, g_my = -1;
static WCHAR g_status[400] = L"";
static HWND g_overlay;

#define RGBW(v) RGB(v, v, v)
#define C_BG RGB(0, 0, 0)
#define C_SIDE RGB(7, 7, 7)
#define C_PANEL RGB(13, 13, 13)
#define C_LINE RGB(42, 42, 42)
#define C_DIM RGB(138, 138, 138)
#define C_WHITE RGB(255, 255, 255)

// ---------- data ----------
typedef struct { const WCHAR *name, *desc; int on, act; } Item;
typedef struct { const WCHAR *section, *prefix; int n; Item *items; } List;

static Item tweaks[NTWEAKS];
static Item games[] = {
 {L"Fortnite - Performance", L"Lowest graphics settings, no VSync. Close Fortnite first.", 0, 0},
 {L"Fortnite - Competitive", L"Low effects but full view distance for spotting enemies.", 0, 0}};
static Item addons[] = {
 {L"Performance Overlay", L"Shows CPU, GPU and RAM use on screen. Works in Windowed Fullscreen.", 0, 0},
 {L"Shader Cache Cleaner", L"Deletes old GPU shader caches (first launch after may stutter once).", 0, 1},
 {L"Background App Closer", L"Closes OneDrive, Teams, Skype and other apps that slow games.", 0, 1},
 {L"Temp File Cleaner", L"Deletes temporary files to free disk space.", 0, 1},
 {L"Flush DNS Cache", L"Clears stale network lookups.", 0, 1}};
static List lists[] = {
 {L"tweaks", L"t", NTWEAKS, tweaks},
 {L"games", L"g", 2, games},
 {L"addons", L"a", 5, addons}};

static const WCHAR *navName[] = {L"Home", L"Presets", L"Game Library", L"Optimizations", L"Addons", L"Ultimate Mode", L"Nero Assistant", L"Get Pro"};
static const WCHAR *navSub[] = {L"Your PC at a glance", L"Save your setups", L"Pick a game", L"Turn tweaks on/off", L"Optional extras", L"Everything at once", L"Ask or analyze my PC", L"Unlock everything"};

// ---------- config ----------
static void cfgPath(void) {
    WCHAR dir[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, dir))) {
        wcscat(dir, L"\\NeroTweaks");
        CreateDirectoryW(dir, NULL);
        swprintf(g_ini, MAX_PATH, L"%ls\\config.ini", dir);
    } else wcscpy(g_ini, L"nero.ini");
}
static void loadConfig(void) {
    GetPrivateProfileStringW(L"user", L"name", L"", g_name, 64, g_ini);
    for (int l = 1; l < 3; l++)
        for (int i = 0; i < lists[l].n; i++) {
            WCHAR k[16];
            swprintf(k, 16, L"%ls%d", lists[l].prefix, i);
            lists[l].items[i].on = lists[l].items[i].act ? 0 : GetPrivateProfileIntW(lists[l].section, k, 0, g_ini);
        }
}
static int appliedCount(void) {
    int c = 0;
    for (int i = 0; i < NTWEAKS; i++) c += tweaks[i].on;
    return c;
}

// ---------- live stats ----------
typedef struct { double cpuMax, cpuTemp, cpuLoad, cpuSpeed, gpuTemp, gpuLoad, gpuMem, diskFree, diskUse, ram, netTotal, netSent, netRecv; } Stats;
static Stats st;
static volatile LONG gpuT = -1, gpuU = -1, gpuM = -1;
static float hist[5][HIST];

typedef struct { ULONG Number, MaxMhz, CurrentMhz, MhzLimit, MaxIdleState, CurrentIdleState; } PPI;
typedef struct { LARGE_INTEGER Idle, Kernel, User, Rsv[2]; ULONG Rsv2; } SPPI;
typedef LONG (NTAPI *NtQSI)(ULONG, PVOID, ULONG, PULONG);
static void readCores(void) {
    static NtQSI q; static SPPI prev[256]; static int init;
    if (!q) q = (NtQSI)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation");
    if (!q) return;
    SYSTEM_INFO si; GetSystemInfo(&si);
    int n = (int)si.dwNumberOfProcessors; if (n > 256) n = 256;
    SPPI cur[256]; ULONG got = 0;
    if (q(8, cur, (ULONG)(n * sizeof(SPPI)), &got) != 0) return;
    n = (int)(got / sizeof(SPPI));
    double mx = 0;
    if (init) for (int i = 0; i < n; i++) {
        double dI = (double)(cur[i].Idle.QuadPart - prev[i].Idle.QuadPart);
        double dT = (double)((cur[i].Kernel.QuadPart - prev[i].Kernel.QuadPart) + (cur[i].User.QuadPart - prev[i].User.QuadPart));
        double l = dT > 0 ? 100.0 * (dT - dI) / dT : 0;
        if (l > mx) mx = l;
    }
    memcpy(prev, cur, n * sizeof(SPPI)); init = 1;
    st.cpuMax = mx;
}
static void readCpu(void) {
    readCores();
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
    double v[5] = {st.cpuMax / 100, st.gpuLoad >= 0 ? st.gpuLoad / 100 : 0, st.diskUse / 100, st.ram / 100, st.netTotal / 100};
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
#include "license.inc"
#include "engine.inc"
#include "features.inc"

enum { K_NAV, K_STEP, K_TOG, K_BTN, K_NAME };
enum { B_REPLAY, B_MAKECODE, B_COPYCODE, B_PSAVE, B_PAPPLY, B_PICON, B_PDEL, B_ICONPICK, B_ICONCLEAR, B_ICONBACK, B_SEARCH, B_SEND, B_ANALYZE, B_UPDATE, B_LAT, B_MCANCEL, B_COPYID, B_ACTIVATE, B_FPS, B_ALLON, B_ALLOFF, B_RESTORE, B_ULTIMATE, B_WELCOME };
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
static void ButtonX(HDC dc, RECT r, const WCHAR *label, int id, int bArg) {
    int hv = hovered(r);
    if (hv) { Box(dc, r, C_WHITE, C_WHITE, S(12)); Text(dc, label, r.left, r.top + (r.bottom - r.top - S(18)) / 2, r.right - r.left, fBtn, C_BG, 0, DT_CENTER | DT_SINGLELINE); }
    else { GlowBox(dc, r, S(12)); Text(dc, label, r.left, r.top + (r.bottom - r.top - S(18)) / 2, r.right - r.left, fBtn, C_WHITE, 1, DT_CENTER | DT_SINGLELINE); }
    addHit(r, K_BTN, id, bArg);
}
static void Button(HDC dc, RECT r, const WCHAR *label, int id) { ButtonX(dc, r, label, id, 0); }
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
    static const WCHAR *st1[3] = {L"1  Save your setup", L"2  Pick your tweaks", L"3  Or do it all at once"};
    static const WCHAR *st2[3] = {L"Save tweak setups as named presets and switch in one click.", L"Switch on what you want. Each one explains itself.", L"Ultimate Mode turns on every tweak."};
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
    Field cpu[] = {{L"Usage", &st.cpuLoad, 0, L"%"}, {L"Busiest core", &st.cpuMax, 0, L"%"}};
    Field gpu[] = {{L"Temperature", &st.gpuTemp, 0, L"\u00b0C"}, {L"Usage", &st.gpuLoad, 0, L"%"}, {L"Memory", &st.gpuMem, 1, L" GB"}};
    Field dsk[] = {{L"Used", &st.diskUse, 0, L"%"}, {L"Free", &st.diskFree, 0, L" GB"}};
    Field ram[] = {{L"Usage", &st.ram, 0, L"%"}};
    Field net[] = {{L"Total", &st.netTotal, 1, L" Mbps"}, {L"Sent", &st.netSent, 1, L" Mbps"}, {L"Received", &st.netRecv, 1, L" Mbps"}};
    const WCHAR *titles[5] = {L"PROCESSOR", L"GRAPHICS CARD", L"DISK", L"RAM", L"NETWORK"};
    Field *fs[5] = {cpu, gpu, dsk, ram, net}; int nfs[5] = {2, 3, 2, 1, 3};
    int cols = (w + gap) / (S(300) + gap); if (cols < 1) cols = 1; if (cols > 3) cols = 3;
    cw = (w - gap * (cols - 1)) / cols; ch = S(170);
    for (int i = 0; i < 5; i++) {
        int c = i % cols, rw = i / cols;
        RECT r = {x + c * (cw + gap), y + rw * (ch + gap), x + c * (cw + gap) + cw, y + rw * (ch + gap) + ch};
        Card(dc, r, titles[i], fs[i], nfs[i], hist[i]);
    }
    y += ((5 + cols - 1) / cols) * (ch + gap);
    y += Text(dc, L"Busiest core matters for Fortnite: if it sits near 100% while total CPU looks low, your CPU is the limit. The graph shows the busiest core. GPU stats show for NVIDIA cards only.", x, y, w, fSmall, RGBW(100), 0, DT_WORDBREAK) + S(20);
    return y;
}

static int pageList(HDC dc, int x, int y, int w, int li, int bulk) {
    List *L = &lists[li];
    if (bulk) {
        RECT f = {x, y, x + S(190), y + S(40)}, g = {x + S(204), y, x + S(444), y + S(40)}, a = {x + S(458), y, x + S(598), y + S(40)}, b = {x + S(612), y, x + S(752), y + S(40)};
        Button(dc, f, L"Apply FPS preset", B_FPS); Button(dc, g, L"Apply low-latency preset", B_LAT); Button(dc, a, L"Turn all on", B_ALLON); Button(dc, b, L"Turn all off", B_ALLOFF);
        y += S(60);
    }
    int cols = w >= S(760) ? 2 : 1, gap = S(10), cw = (w - gap * (cols - 1)) / cols, rh = S(66);
    for (int i = 0; i < L->n; i++) {
        int c = i % cols, rw = i / cols;
        RECT r = {x + c * (cw + gap), y + rw * (rh + gap), x + c * (cw + gap) + cw, y + rw * (rh + gap) + rh};
        Item *it = &L->items[i];
        Box(dc, r, C_PANEL, it->on ? C_WHITE : (hovered(r) ? RGBW(120) : RGBW(35)), S(10));
        Text(dc, it->name, r.left + S(14), r.top + S(12), cw - S(100), fBody, C_WHITE, it->on, DT_SINGLELINE | DT_END_ELLIPSIS);
        if (li == 0 && (isRec(i) || isLat(i))) {
            SIZE sz; SelectObject(dc, fBody); GetTextExtentPoint32W(dc, it->name, (int)wcslen(it->name), &sz);
            int tx = r.left + S(14) + sz.cx + S(10);
            if (tx + S(50) < r.right - S(60)) Text(dc, isRec(i) ? L"FPS" : L"INPUT", tx, r.top + S(14), S(60), fSmall, RGBW(200), 1, DT_SINGLELINE);
        }
        Text(dc, it->desc, r.left + S(14), r.top + S(36), cw - S(100), fSmall, C_DIM, 0, DT_SINGLELINE | DT_END_ELLIPSIS);
        if (itemLocked(li, i)) {
            RECT pb = {r.right - S(76), r.top + (rh - S(28)) / 2, r.right - S(14), r.top + (rh + S(28)) / 2};
            Box(dc, pb, RGBW(20), RGBW(120), S(14));
            Text(dc, L"PRO", pb.left, pb.top + S(5), pb.right - pb.left, fSmall, RGBW(190), 1, DT_CENTER | DT_SINGLELINE);
        } else if (it->act) {
            RECT pb = {r.right - S(76), r.top + (rh - S(28)) / 2, r.right - S(14), r.top + (rh + S(28)) / 2};
            int hv = hovered(pb);
            Box(dc, pb, hv ? C_WHITE : RGBW(20), C_WHITE, S(14));
            Text(dc, L"Run", pb.left, pb.top + S(5), pb.right - pb.left, fSmall, hv ? C_BG : C_WHITE, 0, DT_CENTER | DT_SINGLELINE);
        } else Switch(dc, r.right - S(54), r.top + (rh - S(20)) / 2, it->on);
        addHit(r, K_TOG, li, i);
    }
    return y + ((L->n + cols - 1) / cols) * (rh + gap) + S(10);
}

static void frameRect(HDC dc, RECT r, COLORREF c) {
    HPEN p = CreatePen(PS_SOLID, 1, c); HGDIOBJ op = SelectObject(dc, p), ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, r.left, r.top, r.right, r.bottom); SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(p);
}
static void drawIcon(HDC dc, HBITMAP bm, RECT r) {
    HDC m = CreateCompatibleDC(dc); HGDIOBJ o = SelectObject(m, bm); BITMAP bi; GetObject(bm, sizeof bi, &bi);
    int old = SetStretchBltMode(dc, HALFTONE); SetBrushOrgEx(dc, 0, 0, NULL);
    StretchBlt(dc, r.left, r.top, r.right - r.left, r.bottom - r.top, m, 0, 0, bi.bmWidth, bi.bmHeight, SRCCOPY);
    SetStretchBltMode(dc, old); SelectObject(m, o); DeleteDC(m);
}
static int pagePresets(HDC dc, int x, int y, int w) {
    int pw = w > S(900) ? S(900) : w;
    if (g_status[0]) y += Text(dc, g_status, x, y, pw, fBody, C_WHITE, 1, DT_WORDBREAK) + S(14);
    RECT a = {x, y, x + S(320), y + S(42)}, b = {x + S(336), y, x + S(656), y + S(42)};
    ButtonX(dc, a, L"+ Save current setup as preset", B_PSAVE, 0);
    ButtonX(dc, b, L"Create Windows restore point", B_RESTORE, 0);
    y += S(56);
    WCHAR note[200];
    if (g_pro) swprintf(note, 200, L"%d saved. Pro: unlimited presets.", npresets);
    else swprintf(note, 200, L"%d of %d free presets used. Pro: unlimited.", npresets, FREE_PRESETS);
    y += Text(dc, note, x, y, pw, fSmall, RGBW(110), 0, DT_SINGLELINE) + S(16);
    if (!npresets) y += Text(dc, L"No presets yet. Turn on the tweaks you want in Optimizations, then save them here with a name and a Fortnite item as the icon. Switch setups in one click.", x, y, pw, fBody, C_DIM, 0, DT_WORDBREAK) + S(10);
    for (int i = 0; i < npresets; i++) {
        RECT r = {x, y, x + pw, y + S(88)};
        Box(dc, r, C_PANEL, hovered(r) ? RGBW(90) : C_LINE, S(14));
        RECT ib = {r.left + S(14), r.top + S(14), r.left + S(74), r.top + S(74)};
        HBITMAP bm = iconGet(presets[i].icon);
        if (bm) { drawIcon(dc, bm, ib); frameRect(dc, ib, hovered(ib) ? C_WHITE : RGBW(90)); }
        else {
            Box(dc, ib, RGBW(14), hovered(ib) ? C_WHITE : RGBW(70), S(10));
            Text(dc, L"+ icon", ib.left, ib.top + S(22), S(60), fSmall, RGBW(150), 0, DT_CENTER | DT_SINGLELINE);
        }
        addHit(ib, K_BTN, B_PICON, i);
        Text(dc, presets[i].name, r.left + S(92), r.top + S(18), pw - S(330), fStat, C_WHITE, 1, DT_SINGLELINE | DT_END_ELLIPSIS);
        WCHAR sub[64]; swprintf(sub, 64, L"%d tweak%ls on", presetCount(&presets[i]), presetCount(&presets[i]) == 1 ? L"" : L"s");
        Text(dc, sub, r.left + S(92), r.top + S(54), pw - S(330), fSmall, C_DIM, 0, DT_SINGLELINE);
        RECT ap = {r.right - S(212), r.top + S(24), r.right - S(112), r.top + S(64)}, dl = {r.right - S(100), r.top + S(24), r.right - S(14), r.top + S(64)};
        ButtonX(dc, ap, L"Apply", B_PAPPLY, i); ButtonX(dc, dl, L"Delete", B_PDEL, i);
        y += S(100);
    }
    return y + S(20);
}
static int pageUltimate(HDC dc, int x, int y, int w) {
    int pw = w > S(720) ? S(720) : w;
    WCHAR ut[200]; swprintf(ut, 200, L"Turns on all %d tweaks for the best possible performance. Make a restore point first so you can undo it.", NTWEAKS);
    y += Text(dc, ut, x, y, pw, fBody, C_DIM, 0, DT_WORDBREAK) + S(20);
    RECT a = {x, y, x + S(200), y + S(42)}, b = {x + S(216), y, x + S(456), y + S(42)};
    Button(dc, a, L"Make backup first", B_RESTORE + 100);
    Button(dc, b, L"Enable Ultimate Mode", B_ULTIMATE);
    return y + S(70);
}
static RECT g_keyRect; static int g_keyShow;
static int pageOwner(HDC dc, int x, int y, int w);
static int pageLicense(HDC dc, int x, int y, int w) {
    int pw = w > S(760) ? S(760) : w;
    if (IS_OWNER) return pageOwner(dc, x, y, w);
    if (g_status[0]) y += Text(dc, g_status, x, y, pw, fBody, C_WHITE, 1, DT_WORDBREAK) + S(16);
    if (g_pro) {
        RECT rp = {x, y + S(126), x + S(260), y + S(168)};
        Button(dc, rp, L"Replay Pro animation", B_REPLAY);
        RECT c = {x, y, x + pw, y + S(110)};
        GlowBox(dc, c, S(14));
        Text(dc, L"Pro is active on this PC", c.left + S(20), c.top + S(18), pw - S(40), fStat, C_WHITE, 1, DT_SINGLELINE);
        Text(dc, L"Every tweak, profile and addon is unlocked for life. Your code is tied to this PC, so it won't work on another one.", c.left + S(20), c.top + S(54), pw - S(40), fSmall, C_DIM, 0, DT_WORDBREAK);
        return y + S(190);
    }
    WCHAR lt[400]; swprintf(lt, 400, L"FREE gives you the live dashboard, %d saved presets, %d core tweaks, the assistant and 3 addons - forever. PRO unlocks all %d tweaks, unlimited presets, the FPS and low-latency presets, Fortnite profiles, Ultimate Mode and every addon, with one payment and no subscription.", FREE_PRESETS, (int)(sizeof freeTweaks / sizeof freeTweaks[0]), NTWEAKS);
    y += Text(dc, lt, x, y, pw, fBody, C_DIM, 0, DT_WORDBREAK) + S(22);
    y += Text(dc, L"1  YOUR PC ID", x, y, pw, fH2, RGBW(210), 1, DT_SINGLELINE) + S(10);
    RECT idr = {x, y, x + S(360), y + S(46)}; Box(dc, idr, C_PANEL, C_LINE, S(10));
    Text(dc, g_machine, idr.left, idr.top + S(10), idr.right - idr.left, fStat, C_WHITE, 1, DT_CENTER | DT_SINGLELINE);
    RECT cb = {x + S(376), y, x + S(536), y + S(46)}; Button(dc, cb, L"Copy ID", B_COPYID);
    y += S(62);
    y += Text(dc, BUY_INFO, x, y, pw, fSmall, C_DIM, 0, DT_WORDBREAK) + S(22);
    y += Text(dc, L"2  PASTE YOUR PRO CODE", x, y, pw, fH2, RGBW(210), 1, DT_SINGLELINE) + S(10);
    g_keyRect.left = x; g_keyRect.top = y; g_keyRect.right = x + pw; g_keyRect.bottom = y + S(38); g_keyShow = 1;
    y += S(54);
    RECT ab = {x, y, x + S(200), y + S(42)}; Button(dc, ab, L"Activate Pro", B_ACTIVATE);
    y += S(62);
    y += Text(dc, L"A Pro code only works on the PC it was made for, so it can't be shared. Moving to a new PC? Contact the seller for a new code.", x, y, pw, fSmall, RGBW(110), 0, DT_WORDBREAK);
    return y + S(20);
}
static int pageChat(HDC dc, int x, int y, int w) {
    int pw = w > S(860) ? S(860) : w;
    for (int i = 0; i < nmsgs; i++) {
        int maxw = pw * 8 / 10;
        RECT m = {0, 0, maxw - S(28), 4000};
        SelectObject(dc, fBody); DrawTextW(dc, msgs[i].t, -1, &m, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
        int bw = (m.right - m.left) + S(28), bh = (m.bottom - m.top) + S(24);
        int bx = msgs[i].me ? x + pw - bw : x;
        RECT r = {bx, y, bx + bw, y + bh};
        Box(dc, r, msgs[i].me ? RGBW(22) : C_PANEL, msgs[i].me ? C_WHITE : C_LINE, S(14));
        Text(dc, msgs[i].t, r.left + S(14), r.top + S(12), bw - S(28), fBody, msgs[i].me ? C_WHITE : RGBW(215), msgs[i].me, DT_WORDBREAK);
        y += bh + S(12);
    }
    return y + S(110);
}
static int pageOwner(HDC dc, int x, int y, int w) {
#ifdef OWNER_BUILD
    int pw = w > S(780) ? S(780) : w;
    if (g_status[0]) y += Text(dc, g_status, x, y, pw, fBody, C_WHITE, 1, DT_WORDBREAK) + S(14);
    RECT c = {x, y, x + pw, y + S(96)}; GlowBox(dc, c, S(14));
    Text(dc, L"Owner version - everything is unlocked", c.left + S(20), c.top + S(16), pw - S(40), fStat, C_WHITE, 1, DT_SINGLELINE);
    Text(dc, L"Every tweak, profile, preset and addon is on. This build is for you only - don't share it.", c.left + S(20), c.top + S(54), pw - S(40), fSmall, C_DIM, 0, DT_WORDBREAK);
    y += S(116);
    RECT rp = {x, y, x + S(260), y + S(42)}; Button(dc, rp, L"Replay Pro animation", B_REPLAY);
    y += S(70);
    y += Text(dc, L"MAKE A PRO CODE FOR A CUSTOMER", x, y, pw, fH2, RGBW(210), 1, DT_SINGLELINE) + S(10);
    y += Text(dc, L"Paste the PC ID the customer sends you, then click Make code. Needs nero_private_key.bin in the same folder as this exe.", x, y, pw, fSmall, C_DIM, 0, DT_WORDBREAK) + S(12);
    g_keyRect.left = x; g_keyRect.top = y; g_keyRect.right = x + pw - S(170); g_keyRect.bottom = y + S(38); g_keyShow = 1;
    RECT mb = {x + pw - S(158), y, x + pw, y + S(38)}; Button(dc, mb, L"Make code", B_MAKECODE);
    y += S(56);
    if (g_ownerCode[0]) {
        WCHAR shown[260]; int o = 0, groups = 0;
        for (const WCHAR *p = g_ownerCode; *p && o < 250; p++) {
            if (*p == L'-') { if (++groups % 4 == 0) { shown[o++] = L'\n'; continue; } }
            shown[o++] = *p;
        }
        shown[o] = 0;
        RECT m = {0, 0, pw - S(32), 4000}; SelectObject(dc, fBody); DrawTextW(dc, shown, -1, &m, DT_CALCRECT | DT_NOPREFIX);
        RECT cr = {x, y, x + pw, y + (m.bottom - m.top) + S(28)}; Box(dc, cr, C_PANEL, C_WHITE, S(12));
        Text(dc, shown, cr.left + S(16), cr.top + S(14), pw - S(32), fBody, C_WHITE, 1, 0);
        y = cr.bottom + S(14);
        RECT cb = {x, y, x + S(170), y + S(42)}; Button(dc, cb, L"Copy code", B_COPYCODE);
        y += S(60);
    }
    return y + S(20);
#else
    (void)dc; (void)x; (void)w; return y;
#endif
}
static int pageIcons(HDC dc, int x, int y, int w) {
    int pw = w > S(900) ? S(900) : w;
    WCHAR t[260]; const WCHAR *pn = (g_pickPreset >= 0 && g_pickPreset < npresets) ? presets[g_pickPreset].name : L"";
    swprintf(t, 260, L"Pick an icon for \"%ls\". Search any Fortnite item (skins, pickaxes, emotes, gliders...) and click it.", pn);
    y += Text(dc, t, x, y, pw, fBody, C_DIM, 0, DT_WORDBREAK) + S(18);
    g_keyRect.left = x; g_keyRect.top = y; g_keyRect.right = x + pw - S(150); g_keyRect.bottom = y + S(38); g_keyShow = 1;
    RECT sb = {x + pw - S(138), y, x + pw, y + S(38)}; ButtonX(dc, sb, L"Search", B_SEARCH, 0);
    y += S(56);
    y += Text(dc, g_picStatus, x, y, pw, fBody, C_WHITE, 1, DT_WORDBREAK) + S(16);
    int cell = S(118), cellH = S(140), gap = S(12), cols = (pw + gap) / (cell + gap); if (cols < 1) cols = 1;
    for (int i = 0; i < nres; i++) {
        int c = i % cols, rw = i / cols;
        RECT r = {x + c * (cell + gap), y + rw * (cellH + gap), x + c * (cell + gap) + cell, y + rw * (cellH + gap) + cellH};
        Box(dc, r, C_PANEL, hovered(r) ? C_WHITE : C_LINE, S(12));
        RECT ib = {r.left + S(19), r.top + S(10), r.left + S(99), r.top + S(90)};
        HBITMAP bm = iconGet(results[i].id);
        if (bm) drawIcon(dc, bm, ib); else Text(dc, L"...", ib.left, ib.top + S(28), S(80), fSmall, RGBW(120), 0, DT_CENTER | DT_SINGLELINE);
        Text(dc, results[i].name, r.left + S(6), r.top + S(96), cell - S(12), fSmall, C_WHITE, 0, DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
        addHit(r, K_BTN, B_ICONPICK, i);
    }
    y += ((nres + cols - 1) / cols) * (cellH + gap) + S(10);
    RECT rb = {x, y, x + S(170), y + S(40)}, bb = {x + S(184), y, x + S(324), y + S(40)};
    Button(dc, rb, L"Remove icon", B_ICONCLEAR); Button(dc, bb, L"Back", B_ICONBACK);
    return y + S(70);
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


// ---------- "Pro unlocked" celebration ----------
static int g_anim; static DWORD g_animStart;
#define ANIM_MS 4600
static double easeOut(double x) { if (x < 0) x = 0; if (x > 1) x = 1; return 1 - pow(1 - x, 3); }
static double easeBack(double x) { if (x < 0) x = 0; if (x > 1) x = 1; double c1 = 1.70158, c3 = c1 + 1; return 1 + c3 * pow(x - 1, 3) + c1 * pow(x - 1, 2); }
static void fillAlphaRect(HDC dc, int x, int y, int W, int H, int v, int a);
static void fillAlpha(HDC dc, int W, int H, int v, int a) { fillAlphaRect(dc, 0, 0, W, H, v, a); }
static void fillAlphaRect(HDC dc, int x0, int y0, int W, int H, int v, int a) {
    if (a <= 0) return;
    if (a > 255) a = 255;
    HDC m = CreateCompatibleDC(dc); HBITMAP b = CreateCompatibleBitmap(dc, 1, 1); HGDIOBJ o = SelectObject(m, b);
    SetPixel(m, 0, 0, RGB(v, v, v));
    BLENDFUNCTION bf = {AC_SRC_OVER, 0, (BYTE)a, 0};
    AlphaBlend(dc, x0, y0, W, H, m, 0, 0, 1, 1, bf);
    SelectObject(m, o); DeleteObject(b); DeleteDC(m);
}
static void disc(HDC dc, int cx, int cy, int r, int v) {
    HBRUSH b = CreateSolidBrush(RGBW(v)); HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, GetStockObject(NULL_PEN));
    Ellipse(dc, cx - r, cy - r, cx + r, cy + r); SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b);
}
static DWORD WINAPI chimeThread(LPVOID p) {
    (void)p; int f[] = {523, 659, 784, 1047, 1319}, d[] = {90, 90, 90, 130, 480};
    for (int i = 0; i < 5; i++) Beep(f[i], d[i]);
    return 0;
}
static void startUnlockAnim(void) {
    g_anim = 1; g_animStart = GetTickCount();
    SetTimer(g_hwnd, 2, 16, NULL);
    CreateThread(NULL, 0, chimeThread, NULL, 0, NULL);
}
static void drawUnlock(HDC dc, int W, int H) {
    double t = (GetTickCount() - g_animStart) / 1000.0;
    double f = t > 4.0 ? (4.6 - t) / 0.6 : 1; if (f < 0) f = 0;
    int cx = W / 2, cy = H / 2 - S(40);
    fillAlpha(dc, W, H, 0, (int)(255 * 0.97 * f * (t < 0.25 ? t / 0.25 : 1)));
    // glow behind the title
    double gr = easeOut(t / 0.9);
    for (int i = 9; i >= 1; i--) disc(dc, cx, cy + S(10), (int)(S(330) * gr * i / 9), (int)(i < 9 ? (9 - i) * 6 * f : 0));
    // rotating light rays
    double rot = t * 0.4, len = S(60) + easeOut(t / 1.3) * (W > H ? W : H) * 0.75;
    for (int i = 0; i < 28; i++) {
        double a = rot + i * 6.2831853 / 28, hw = (i % 2 ? 0.035 : 0.06), L = len * (i % 2 ? 0.8 : 1.0);
        POINT p[3] = {{cx, cy + S(10)}, {cx + (int)(cos(a - hw) * L), cy + S(10) + (int)(sin(a - hw) * L)}, {cx + (int)(cos(a + hw) * L), cy + S(10) + (int)(sin(a + hw) * L)}};
        int c = (int)((i % 2 ? 14 : 30) * f);
        HBRUSH b = CreateSolidBrush(RGBW(c)); HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, GetStockObject(NULL_PEN));
        Polygon(dc, p, 3); SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b);
    }
    // shockwave rings
    for (int k = 0; k < 3; k++) {
        double tk = t - 0.05 - 0.28 * k;
        if (tk <= 0 || tk >= 1.7) continue;
        double r = easeOut(tk / 1.7) * S(560), b = (1 - tk / 1.7) * f;
        int wd[3] = {S(9), S(5), S(2)}; double lv[3] = {38, 110, 255};
        for (int j = 0; j < 3; j++) {
            HPEN pn = CreatePen(PS_SOLID, wd[j], RGBW((int)(lv[j] * b)));
            HGDIOBJ op = SelectObject(dc, pn), ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
            Ellipse(dc, cx - (int)r, cy + S(10) - (int)r, cx + (int)r, cy + S(10) + (int)r);
            SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pn);
        }
    }
    // sparks
    static double pa[120], ps[120], pl[120], pz[120]; static int pk[120], inited;
    if (!inited) { srand(7); for (int i = 0; i < 120; i++) { pa[i] = rand() / (double)RAND_MAX * 6.2831853; ps[i] = 120 + rand() % 620; pl[i] = 1.4 + rand() / (double)RAND_MAX * 2.2; pz[i] = 2 + rand() % 4; pk[i] = rand() % 2; } inited = 1; }
    double age = t - 0.12;
    if (age > 0) for (int i = 0; i < 120; i++) {
        double b = (1 - age / pl[i]) * f; if (b <= 0) continue;
        double dist = ps[i] * easeOut(age / 2.2) * g_sc;
        int px = cx + (int)(cos(pa[i]) * dist), py = cy + S(10) + (int)(sin(pa[i]) * dist + age * age * S(70));
        int sz = S((int)pz[i]), v = (int)(255 * b);
        if (pk[i]) { POINT d[4] = {{px, py - sz * 2}, {px + sz, py}, {px, py + sz * 2}, {px - sz, py}}; HBRUSH br = CreateSolidBrush(RGBW(v)); HGDIOBJ ob = SelectObject(dc, br), op = SelectObject(dc, GetStockObject(NULL_PEN)); Polygon(dc, d, 4); SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(br); }
        else disc(dc, px, py, sz, v);
    }
    // logo + text
    if (t > 0.3) drawLogo(dc, cx - S(130), cy - S(190), S(260));
    if (t > 0.5) {
        double p = (t - 0.5) / 0.55; int px = (int)(S(66) * easeBack(p)); if (px < 2) px = 2;
        HFONT big = CreateFontW(-px, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        int v = (int)(255 * f);
        Text(dc, L"PRO UNLOCKED", 0, cy + S(20) + (S(66) - px) / 2, W, big, RGBW(v), 1, DT_CENTER | DT_SINGLELINE);
        DeleteObject(big);
    }
    if (t > 1.3) {
        double b = (t - 1.3) / 0.6; if (b > 1) b = 1; b *= f;
        Text(dc, L"WELCOME TO NERO TWEAKS PRO", 0, cy + S(112), W, fNav, RGBW((int)(235 * b)), 1, DT_CENTER | DT_SINGLELINE);
        Text(dc, L"Every tweak, profile and addon is now yours.", 0, cy + S(146), W, fBody, RGBW((int)(150 * b)), 0, DT_CENTER | DT_SINGLELINE);
    }
    if (t > 2.2) Text(dc, L"click anywhere to continue", 0, H - S(60), W, fSmall, RGBW((int)(90 * f)), 0, DT_CENTER | DT_SINGLELINE);
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
    int y = S(46) - g_scroll;
    const WCHAR *titles[] = {NULL, L"Presets", L"Game Library", L"Optimizations", L"Addons", L"Ultimate Mode", L"Nero Assistant", L"Get Pro", L"Pick an Icon"};
    const WCHAR *taglines[] = {NULL, L"Save your tweak setups with a name and a Fortnite item icon, then switch between them in one click.", L"Choose a Fortnite profile. For best results also set Rendering Mode to Performance in Fortnite (Settings > Video).", L"Click any tweak to turn it on or off. Tweaks tagged FPS help frame rate most; use Apply FPS preset for just those.", L"Optional extras you can add on.", L"One click, every tweak.", L"Runs on your PC, no internet needed. Ask a question or let me analyze your live stats.", L"One payment, yours forever on this PC.", L"Choose a Fortnite item to show on your preset."};
    int end;
    if (g_page == 0) end = pageHome(dc, x, y, w);
    else {
        const WCHAR *ttl = (IS_OWNER && g_page == 7) ? L"Owner Tools" : (g_pro && g_page == 7) ? L"Pro License" : titles[g_page];
        const WCHAR *tag = (IS_OWNER && g_page == 7) ? L"Make Pro codes for customers and replay the Pro animation." : (g_pro && g_page == 7) ? L"Thank you for supporting Nero Tweaks." : taglines[g_page];
        y += Text(dc, ttl, x, y, w, fTitle, C_WHITE, 1, DT_SINGLELINE) + S(6);
        y += Text(dc, tag, x, y, w, fBody, C_DIM, 0, DT_WORDBREAK) + S(24);
        if (g_page >= 2 && g_page <= 4 && g_status[0]) y += Text(dc, g_status, x, y, w, fBody, C_WHITE, 1, DT_WORDBREAK) + S(14);
        switch (g_page) {
            case 1: end = pagePresets(dc, x, y, w); break;
            case 2: end = pageList(dc, x, y, w, 1, 0); break;
            case 3: end = pageList(dc, x, y, w, 0, 1); break;
            case 4: end = pageList(dc, x, y, w, 2, 0); break;
            case 5: end = pageUltimate(dc, x, y, w); break;
            case 6: end = pageChat(dc, x, y, w); break;
            case 7: end = pageLicense(dc, x, y, w); break;
            default: end = pageIcons(dc, x, y, w);
        }
    }
    g_contentH = end + g_scroll + S(30);
    SelectClipRgn(dc, NULL);

    // header strip: plan, version, update
    RECT hb = {sw, 0, W, S(40)}; HBRUSH hbb = CreateSolidBrush(C_BG); FillRect(dc, &hb, hbb); DeleteObject(hbb);
    {
        WCHAR vt[80]; swprintf(vt, 80, L"%ls   v%ls", planName(), APP_VERSION);
        int right = W - S(24);
        if (g_updAvail) {
            WCHAR ut[48]; swprintf(ut, 48, g_updBusy ? L"Updating..." : L"Update to v%ls", g_updVer);
            RECT ub = {W - S(24) - S(170), S(7), W - S(24), S(33)};
            ButtonX(dc, ub, ut, B_UPDATE, 0); right = ub.left - S(14);
        }
        Text(dc, vt, sw, S(12), right - sw, fSmall, RGBW(120), 0, DT_RIGHT | DT_SINGLELINE);
    }
    g_chatShow = 0;
    if (g_page == 6) {
        RECT cb = {sw, H - S(104), W, H}; HBRUSH cbb = CreateSolidBrush(C_BG); FillRect(dc, &cb, cbb); DeleteObject(cbb);
        HPEN cp = CreatePen(PS_SOLID, 1, C_LINE); HGDIOBJ cop = SelectObject(dc, cp);
        MoveToEx(dc, sw, H - S(104), NULL); LineTo(dc, W, H - S(104)); SelectObject(dc, cop); DeleteObject(cp);
        int ew = W - sw - pad * 2 - S(110) - S(200) - S(20); if (ew < S(120)) ew = S(120);
        g_chatRect.left = x; g_chatRect.top = H - S(80); g_chatRect.right = x + ew; g_chatRect.bottom = H - S(42); g_chatShow = 1;
        RECT sb2 = {x + ew + S(10), H - S(82), x + ew + S(110), H - S(40)}, ab = {sb2.right + S(10), H - S(82), sb2.right + S(200), H - S(40)};
        Button(dc, sb2, L"Send", B_SEND); Button(dc, ab, L"Analyze my PC", B_ANALYZE);
    }

    // sidebar
    RECT sb = {0, 0, sw, H};
    HBRUSH sbb = CreateSolidBrush(C_SIDE); FillRect(dc, &sb, sbb); DeleteObject(sbb);
    HPEN lp = CreatePen(PS_SOLID, 1, C_LINE); HGDIOBJ op = SelectObject(dc, lp);
    MoveToEx(dc, sw - 1, 0, NULL); LineTo(dc, sw - 1, H); SelectObject(dc, op); DeleteObject(lp);
    drawLogo(dc, S(10), S(14), sw - S(20));
    int lh = (sw - S(20)) * g_logoH / g_logoW;
    Text(dc, IS_OWNER ? L"T  W  E  A  K  S   OWNER VERSION" : g_pro ? L"T  W  E  A  K  S   PRO VERSION" : L"T  W  E  A  K  S   FREE VERSION", S(26), S(14) + lh - S(4), sw, fSmall, C_DIM, 0, DT_SINGLELINE);
    int ny = S(14) + lh + S(36);
    for (int i = 0; i < 8; i++) {
        RECT r = {S(14), ny, sw - S(14), ny + S(54)};
        int act = i == g_page, hv = hovered(r);
        if (act) { GlowBox(dc, r, S(12)); RECT bar2 = {0, r.top + S(12), S(4), r.bottom - S(12)}; HBRUSH bb2 = CreateSolidBrush(C_WHITE); FillRect(dc, &bar2, bb2); DeleteObject(bb2); }
        else if (hv) Box(dc, r, RGBW(17), RGBW(17), S(12));
        Text(dc, i == 7 && g_pro ? (IS_OWNER ? L"Owner Tools" : L"Pro License") : navName[i], r.left + S(18), r.top + S(8), r.right - r.left - S(24), fNav, act || hv ? C_WHITE : RGBW(185), act, DT_SINGLELINE);
        Text(dc, i == 7 && g_pro ? (IS_OWNER ? L"Codes & animation" : L"Active - thank you") : navSub[i], r.left + S(18), r.top + S(31), r.right - r.left - S(24), fNavSub, act ? RGBW(170) : RGBW(100), 0, DT_SINGLELINE);
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

    {   // page transition: content fades in
        DWORD e = GetTickCount() - g_fadeStart;
        if (e < 260) fillAlphaRect(dc, sw, 0, W - sw, H, 0, (int)((1 - easeOut(e / 260.0)) * 255));
    }
    // modal overlay (welcome / name a preset)
    if (g_welcome) {
        HBRUSH ov = CreateSolidBrush(RGB(2, 2, 2)); FillRect(dc, &all, ov); DeleteObject(ov);
        int bw = S(380), bh = S(300), bx = (W - bw) / 2, by = (H - bh) / 2;
        RECT card = {bx, by, bx + bw, by + bh};
        GlowBox(dc, card, S(18));
        int lw = S(190); drawLogo(dc, bx + (bw - lw) / 2, by + S(20), lw);
        Text(dc, g_modalMode ? L"Name your preset" : L"Welcome to Nero Tweaks", bx, by + S(112), bw, fStat, C_WHITE, 1, DT_CENTER | DT_SINGLELINE);
        Text(dc, g_modalMode ? L"For example: Tournament, Low latency" : L"What should we call you?", bx, by + S(150), bw, fBody, C_DIM, 0, DT_CENTER | DT_SINGLELINE);
        if (g_modalMode) {
            RECT b = {bx + S(30), by + S(240), bx + bw / 2 - S(6), by + S(282)}, c = {bx + bw / 2 + S(6), by + S(240), bx + bw - S(30), by + S(282)};
            Button(dc, b, L"Save preset", B_WELCOME); Button(dc, c, L"Cancel", B_MCANCEL);
        } else {
            RECT b = {bx + S(30), by + S(240), bx + bw - S(30), by + S(282)};
            Button(dc, b, L"Let's go", B_WELCOME);
        }
    }
    if (g_anim) drawUnlock(dc, W, H);
    BitBlt(wdc, 0, 0, W, H, dc, 0, 0, SRCCOPY);
    SelectObject(dc, obmp); DeleteObject(bmp); DeleteDC(dc);
}

// ---------- logic ----------
static void showModal(int mode, const WCHAR *init) {
    g_welcome = 1; g_modalMode = mode;
    SendMessageW(g_edit, EM_SETLIMITTEXT, mode ? 30 : 24, 0);
    SetWindowTextW(g_edit, init);
    RECT rc; GetClientRect(g_hwnd, &rc);
    layoutEdit(rc.right, rc.bottom);
    ShowWindow(g_edit, SW_SHOW); SetFocus(g_edit);
    SendMessageW(g_edit, EM_SETSEL, 0, -1);
    InvalidateRect(g_hwnd, NULL, FALSE);
}
static void showWelcome(void) { showModal(0, g_name); }
static void acceptName(void) {
    WCHAR n[64]; GetWindowTextW(g_edit, n, 64);
    WCHAR *s = n; while (*s == L' ') s++;
    if (!*s) { SetFocus(g_edit); return; }
    if (g_modalMode == 1) {
        int idx = presetAdd(s);
        g_welcome = 0; ShowWindow(g_edit, SW_HIDE); SetFocus(g_hwnd);
        if (idx >= 0) {
            g_pickPreset = idx; nres = 0; setPage(8);
            wcscpy(g_picStatus, L"Preset saved! Now pick an icon (search a Fortnite item), or press Back to skip.");
        } else wcscpy(g_status, L"You've reached the preset limit.");
        InvalidateRect(g_hwnd, NULL, FALSE); return;
    }
    wcscpy(g_name, s);
    WritePrivateProfileStringW(L"user", L"name", g_name, g_ini);
    g_welcome = 0; ShowWindow(g_edit, SW_HIDE); SetFocus(g_hwnd);
    InvalidateRect(g_hwnd, NULL, FALSE);
}
static void updateTitle(void) {
    SetWindowTextW(g_hwnd, IS_OWNER ? L"Nero Tweaks - Owner Version" : g_pro ? L"Nero Tweaks - Pro Version" : L"Nero Tweaks - Free Version");
}
static void lockedMsg(void) {
    setPage(7);
    wcscpy(g_status, L"That's a Pro feature. Unlock it below with your Pro code.");
}
#ifdef OWNER_BUILD
static void copyText(const WCHAR *t) {
    size_t n = (wcslen(t) + 1) * sizeof(WCHAR);
    if (OpenClipboard(g_hwnd)) {
        EmptyClipboard();
        HGLOBAL m = GlobalAlloc(GMEM_MOVEABLE, n);
        if (m) { memcpy(GlobalLock(m), t, n); GlobalUnlock(m); SetClipboardData(CF_UNICODETEXT, m); }
        CloseClipboard();
        wcscpy(g_status, L"Copied to your clipboard.");
    }
}
#endif
static void copyId(void) {
    size_t n = (wcslen(g_machine) + 1) * sizeof(WCHAR);
    if (OpenClipboard(g_hwnd)) {
        EmptyClipboard();
        HGLOBAL m = GlobalAlloc(GMEM_MOVEABLE, n);
        if (m) { memcpy(GlobalLock(m), g_machine, n); GlobalUnlock(m); SetClipboardData(CF_UNICODETEXT, m); }
        CloseClipboard();
        wcscpy(g_status, L"PC ID copied. Send it to the seller.");
    }
}
static void activate(void) {
    WCHAR code[600]; GetWindowTextW(g_keyEdit, code, 600);
    if (licenseValid(code)) {
        WritePrivateProfileStringW(L"license", L"key", code, g_ini);
        g_pro = 1; SetWindowTextW(g_keyEdit, L""); updateTitle();
        wcscpy(g_status, L"Pro unlocked on this PC. Thank you!");
        startUnlockAnim();
    } else wcscpy(g_status, L"That code isn't valid for this PC. Check it was made for the PC ID shown here.");
}
static int confirmBulk(int on) {
    WCHAR m[300];
    if (on == 3) return MessageBoxW(g_hwnd, L"This turns on the low-latency tweaks (mouse acceleration off, precise timer, fast keyboard, foreground boost) and changes Windows settings.\n\nContinue?", L"Nero Tweaks", MB_YESNO | MB_ICONQUESTION) == IDYES;
    if (on == 2) return MessageBoxW(g_hwnd, L"This turns on the tweaks tagged FPS and changes Windows settings.\n\nMake a restore point first if you haven't. Continue?", L"Nero Tweaks", MB_YESNO | MB_ICONQUESTION) == IDYES;
    swprintf(m, 300, on ? L"This turns on all %d tweaks and changes Windows settings.\n\nMake a restore point first if you haven't. Continue?" : L"This turns off all tweaks and puts your settings back. Continue?", NTWEAKS);
    return MessageBoxW(g_hwnd, m, L"Nero Tweaks", MB_YESNO | MB_ICONQUESTION) == IDYES;
}
static void ownerHook(void) {
#ifdef OWNER_BUILD
    WCHAR q[120]; GetWindowTextW(g_keyEdit, q, 120); ownerMakeCode(q);
#endif
}
static void doSearch(void) { WCHAR q[200]; GetWindowTextW(g_keyEdit, q, 200); startSearch(q); }
static void runAnalyze(void) {
    static WCHAR buf[1400];
    chatAdd(L"Analyze my PC", 1); analyzePc(buf, 1400); chatAdd(buf, 0); g_scroll = 1 << 20;
}
static void sendChat(void) {
    WCHAR q[300]; static WCHAR buf[1400];
    GetWindowTextW(g_chatEdit, q, 300);
    WCHAR *s = q; while (*s == L' ') s++;
    if (!*s) return;
    chatAdd(s, 1); answer(s, buf, 1400); chatAdd(buf, 0);
    SetWindowTextW(g_chatEdit, L""); g_scroll = 1 << 20;
    InvalidateRect(g_hwnd, NULL, FALSE);
}
static void click(int mx, int my) {
    POINT p = {mx, my};
    if (g_anim) { if (GetTickCount() - g_animStart > 1500) { g_anim = 0; KillTimer(g_hwnd, 2); InvalidateRect(g_hwnd, NULL, FALSE); } return; }
    for (int i = nhits - 1; i >= 0; i--) {
        Hit *h = &hits[i];
        if (!PtInRect(&h->r, p)) continue;
        if (g_welcome && !(h->kind == K_BTN && (h->a == B_WELCOME || h->a == B_MCANCEL))) continue;
        if (h->kind != K_NAV && mx < S(250)) continue;
        switch (h->kind) {
            case K_NAV: setPage(h->a); g_status[0] = 0; break;
            case K_STEP: setPage(h->a); break;
            case K_NAME: showWelcome(); break;
            case K_TOG:
                if (itemLocked(h->a, h->b)) lockedMsg();
                else if (h->a == 0) tweakToggle(h->b); else if (h->a == 1) gameToggle(h->b); else addonClick(h->b);
                break;
            case K_BTN:
                if (h->a == B_REPLAY) startUnlockAnim();
#ifdef OWNER_BUILD
                else if (h->a == B_MAKECODE) { WCHAR q[120]; GetWindowTextW(g_keyEdit, q, 120); ownerMakeCode(q); }
                else if (h->a == B_COPYCODE) copyText(g_ownerCode);
#endif
                else if (h->a == B_PSAVE) {
                    if (!g_pro && npresets >= FREE_PRESETS) { lockedMsg(); wcscpy(g_status, L"Free includes 2 presets. Pro unlocks unlimited presets."); }
                    else if (npresets >= MAXPRESETS) wcscpy(g_status, L"You've reached the preset limit.");
                    else showModal(1, L"");
                }
                else if (h->a == B_PAPPLY) presetApply(h->b);
                else if (h->a == B_PICON) { g_pickPreset = h->b; setPage(8); wcscpy(g_picStatus, L"Search any Fortnite item and click it to use as the icon."); }
                else if (h->a == B_PDEL) { if (MessageBoxW(g_hwnd, L"Delete this preset? (Your current tweaks stay as they are.)", L"Nero Tweaks", MB_YESNO | MB_ICONQUESTION) == IDYES) presetDelete(h->b); }
                else if (h->a == B_ICONPICK) { if (g_pickPreset >= 0 && g_pickPreset < npresets && h->b < nres) { wcscpy(presets[g_pickPreset].icon, results[h->b].id); presetsSave(); wcscpy(g_status, L"Icon set."); setPage(1); } }
                else if (h->a == B_ICONCLEAR) { if (g_pickPreset >= 0 && g_pickPreset < npresets) { presets[g_pickPreset].icon[0] = 0; presetsSave(); } setPage(1); }
                else if (h->a == B_ICONBACK) setPage(1);
                else if (h->a == B_SEARCH) doSearch();
                else if (h->a == B_SEND) sendChat();
                else if (h->a == B_ANALYZE) runAnalyze();
                else if (h->a == B_UPDATE) { if (!g_updBusy) CreateThread(NULL, 0, updateInstallThread, NULL, 0, NULL); }
                else if (h->a == B_LAT) { if (confirmBulk(3)) applyLowLatency(); }
                else if (h->a == B_MCANCEL) { g_welcome = 0; ShowWindow(g_edit, SW_HIDE); SetFocus(g_hwnd); }
                else if (h->a == B_COPYID) copyId();
                else if (h->a == B_ACTIVATE) activate();
                else if (h->a == B_ALLON) { if (confirmBulk(1)) setAll(1); }
                else if (h->a == B_FPS) { if (confirmBulk(2)) applyRecommended(); }
                else if (h->a == B_ALLOFF) { if (confirmBulk(0)) setAll(0); }
                else if (h->a == B_ULTIMATE) { if (!g_pro) { lockedMsg(); } else if (confirmBulk(1)) { setAll(1); setPage(3); } }
                else if (h->a == B_RESTORE) { wcscpy(g_status, L"Working... this can take a few seconds."); CreateThread(NULL, 0, restoreThread, NULL, 0, NULL); }
                else if (h->a == B_RESTORE + 100) setPage(1);
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
        if (g_welcome && !(hits[i].kind == K_BTN && (hits[i].a == B_WELCOME || hits[i].a == B_MCANCEL))) continue;
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

static void placeEdit(HWND e, int show, RECT r) {
    if (show && !g_welcome && !g_anim) {
        RECT cur; GetWindowRect(e, &cur); MapWindowPoints(NULL, g_hwnd, (POINT *)&cur, 2);
        if (!EqualRect(&r, &cur) || !IsWindowVisible(e)) { MoveWindow(e, r.left, r.top, r.right - r.left, r.bottom - r.top, TRUE); ShowWindow(e, SW_SHOWNA); }
    } else if (IsWindowVisible(e)) ShowWindow(e, SW_HIDE);
}
static void placeKeyEdit(void) {
    placeEdit(g_keyEdit, g_keyShow && (g_page == 7 || g_page == 8), g_keyRect);
    placeEdit(g_chatEdit, g_chatShow && g_page == 6, g_chatRect);
}
static WNDPROC oldEdit;
static LRESULT CALLBACK editProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_KEYDOWN && w == VK_RETURN) { if (h == g_keyEdit) { if (g_page == 8) doSearch(); else if (IS_OWNER) { ownerHook(); } else activate(); InvalidateRect(g_hwnd, NULL, FALSE); } else if (h == g_chatEdit) sendChat(); else acceptName(); return 0; }
    if (m == WM_CHAR && w == VK_RETURN) return 0;
    return CallWindowProcW(oldEdit, h, m, w, l);
}
static HBRUSH editBrush;

static LRESULT CALLBACK wndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: SetTimer(h, 1, 1000, NULL); return 0;
    case WM_TIMER:
        if (w == 3) { if (GetTickCount() - g_fadeStart > 280) KillTimer(h, 3); InvalidateRect(h, NULL, FALSE); return 0; }
        if (w == 2) {
            if (GetTickCount() - g_animStart > ANIM_MS) { g_anim = 0; KillTimer(h, 2); }
            InvalidateRect(h, NULL, FALSE); return 0;
        }
        readStats(); InvalidateRect(h, NULL, FALSE); if (g_overlay) InvalidateRect(g_overlay, NULL, FALSE); return 0;
    case WM_APP: InvalidateRect(h, NULL, FALSE); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        clampScroll();
        g_keyShow = 0;
        paint(dc, rc.right, rc.bottom);
        placeKeyEdit();
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

static void selfTest(void) {
    FILE *f = _wfopen(L"selftest.log", L"w");
    for (int i = 0; i < NTWEAKS; i++) {
        int before = isTweakOn(i);
        applyTweak(i); int on = isTweakOn(i);
        revertTweak(i); int off = isTweakOn(i);
        fwprintf(f, L"%-36ls before=%d apply->%d revert->%d  %ls\n", tdefs[i].name, before, on, off, (on == 1 && off == before) || (on == 1 && off == 0) ? L"OK" : L"CHECK");
    }
    fclose(f);
}
int WINAPI wWinMain(HINSTANCE hi, HINSTANCE hp, PWSTR cmd, int show) {
    (void)hp;
    if (cmd && wcsstr(cmd, L"--selftest")) { cfgPath(); selfTest(); return 0; }
    SetProcessDPIAware();
    HDC sd = GetDC(NULL); g_sc = GetDeviceCaps(sd, LOGPIXELSX) / 96.0; ReleaseDC(NULL, sd);
    for (int i = 0; i < NTWEAKS; i++) { tweaks[i].name = tdefs[i].name; tweaks[i].desc = tdefs[i].desc; }
    cfgPath(); loadConfig(); loadLicense(); loadLogo(); presetsLoad();
    for (int i = 0; i < NTWEAKS; i++) tweaks[i].on = isTweakOn(i);
    fTitle = mkFont(30, FW_SEMIBOLD); fH2 = mkFont(13, FW_BOLD); fBody = mkFont(15, FW_SEMIBOLD);
    fSmall = mkFont(12, FW_NORMAL); fStat = mkFont(23, FW_SEMIBOLD); fNav = mkFont(16, FW_SEMIBOLD);
    fNavSub = mkFont(11, FW_NORMAL); fBtn = mkFont(14, FW_SEMIBOLD);
    editBrush = CreateSolidBrush(RGB(10, 10, 10));

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = wndProc; wc.hInstance = hi; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hi, MAKEINTRESOURCE(1)); wc.lpszClassName = L"NeroTweaks";
    wc.hbrBackground = CreateSolidBrush(C_BG);
    RegisterClassW(&wc);
    WNDCLASSW oc = {0}; oc.lpfnWndProc = overlayProc; oc.hInstance = hi; oc.lpszClassName = L"NeroOverlay"; oc.hbrBackground = CreateSolidBrush(C_BG);
    RegisterClassW(&oc);
    g_hwnd = CreateWindowW(L"NeroTweaks", L"Nero Tweaks", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, S(1220), S(800), NULL, NULL, hi, NULL);
    g_edit = CreateWindowW(L"EDIT", L"", WS_CHILD | ES_CENTER | ES_AUTOHSCROLL | WS_BORDER, 0, 0, 10, 10, g_hwnd, NULL, hi, NULL);
    SendMessageW(g_edit, WM_SETFONT, (WPARAM)fStat, TRUE);
    SendMessageW(g_edit, EM_SETLIMITTEXT, 24, 0);
    oldEdit = (WNDPROC)SetWindowLongPtrW(g_edit, GWLP_WNDPROC, (LONG_PTR)editProc);
    g_chatEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 0, 0, 10, 10, g_hwnd, NULL, hi, NULL);
    SendMessageW(g_chatEdit, WM_SETFONT, (WPARAM)fBody, TRUE);
    SendMessageW(g_chatEdit, EM_SETLIMITTEXT, 200, 0);
    SetWindowLongPtrW(g_chatEdit, GWLP_WNDPROC, (LONG_PTR)editProc);
    g_keyEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 0, 0, 10, 10, g_hwnd, NULL, hi, NULL);
    SendMessageW(g_keyEdit, WM_SETFONT, (WPARAM)fBody, TRUE);
    SendMessageW(g_keyEdit, EM_SETLIMITTEXT, 400, 0);
    SetWindowLongPtrW(g_keyEdit, GWLP_WNDPROC, (LONG_PTR)editProc);
    updateTitle();
    ShowWindow(g_hwnd, show); UpdateWindow(g_hwnd);
    SetWindowPos(g_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    st.cpuTemp = -1;
    if (GetPrivateProfileIntW(L"addons", L"a0", 0, g_ini)) { addons[0].on = 1; overlaySet(1); }
    readStats();
    chatAdd(L"Hi! I'm the Nero Assistant. I run on your PC with no internet needed. Press Analyze my PC and I'll read your live stats and recommend tweaks, or ask me about FPS, ping, input delay or stutter.", 0);
    if (!IS_OWNER) CreateThread(NULL, 0, updateCheckThread, NULL, 0, NULL);   // owner build must never self-replace with the public free build
    CreateThread(NULL, 0, gpuThread, NULL, 0, NULL);
    if (!g_name[0]) showWelcome();
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
