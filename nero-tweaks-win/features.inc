// ---------- presets, Fortnite item icons, offline assistant, updater ----------
// Included from main.c after engine.inc. No drawing code here, only data and logic.
#include <urlmon.h>

#define APP_VERSION L"1.1.0"
#define UPDATE_BASE L"https://raw.githubusercontent.com/torzzzzzzzz/donutsmp-cc/claude/great-lovelace-d0dsez/nero-tweaks-win/"
#define EXE_NAME L"NeroTweaks-FreeVersion.exe"
#define FREE_PRESETS 2

// ---- GDI+ flat API (declared by hand so plain C can use it) ----
typedef struct { UINT32 GdiplusVersion; void *cb; BOOL noBg, noCodecs; } GpStartIn;
extern int WINAPI GdiplusStartup(ULONG_PTR *tok, const GpStartIn *in, void *out);
extern int WINAPI GdipCreateBitmapFromFile(const WCHAR *f, void **bmp);
extern int WINAPI GdipCreateHBITMAPFromBitmap(void *bmp, HBITMAP *h, DWORD bg);
extern int WINAPI GdipDisposeImage(void *img);

static void gpInit(void) {
    static ULONG_PTR tok; GpStartIn in = {1, NULL, FALSE, FALSE};
    if (!tok) GdiplusStartup(&tok, &in, NULL);
}
static void dataDir(WCHAR *out, const WCHAR *sub) {
    WCHAR d[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, d))) wcscpy(d, L".");
    swprintf(out, MAX_PATH, L"%ls\\NeroTweaks", d); CreateDirectoryW(out, NULL);
    if (sub) { wcscat(out, L"\\"); wcscat(out, sub); CreateDirectoryW(out, NULL); }
}
static void safeId(WCHAR *dst, const WCHAR *src) {
    int n = 0;
    for (; src[n] && n < 60; n++) dst[n] = (iswalnum(src[n]) || src[n] == L'_' || src[n] == L'-') ? src[n] : L'_';
    dst[n] = 0;
}
static void iconPath(WCHAR *out, const WCHAR *id) {
    WCHAR d[MAX_PATH], s[64]; dataDir(d, L"icons"); safeId(s, id);
    swprintf(out, MAX_PATH, L"%ls\\%ls.png", d, s);
}

// ---- icon cache ----
typedef struct { WCHAR id[64]; HBITMAP bmp; } IconEnt;
static IconEnt iconCache[160]; static int nIcons;
static HBITMAP iconGet(const WCHAR *id) {
    if (!id || !id[0]) return NULL;
    for (int i = 0; i < nIcons; i++) if (!wcscmp(iconCache[i].id, id)) return iconCache[i].bmp;
    WCHAR p[MAX_PATH]; iconPath(p, id);
    if (GetFileAttributesW(p) == INVALID_FILE_ATTRIBUTES) return NULL;
    gpInit();
    void *bm = NULL; HBITMAP hb = NULL;
    if (GdipCreateBitmapFromFile(p, &bm) == 0 && bm) { GdipCreateHBITMAPFromBitmap(bm, &hb, 0xFF0D0D0D); GdipDisposeImage(bm); }
    if (hb && nIcons < 160) { wcscpy(iconCache[nIcons].id, id); iconCache[nIcons].bmp = hb; nIcons++; }
    return hb;
}

// ---- presets ----
#define MAXPRESETS 24
typedef struct { WCHAR name[40]; WCHAR icon[64]; char mask[NTWEAKS + 1]; } Preset;
static Preset presets[MAXPRESETS]; static int npresets;
static void presetKey(WCHAR *k, const WCHAR *f, int i) { swprintf(k, 24, L"%ls%d", f, i); }
static void presetsSave(void) {
    WCHAR k[24], v[16];
    swprintf(v, 16, L"%d", npresets); WritePrivateProfileStringW(L"presets", L"n", v, g_ini);
    for (int i = 0; i < MAXPRESETS; i++) {
        presetKey(k, L"name", i); WritePrivateProfileStringW(L"presets", k, i < npresets ? presets[i].name : NULL, g_ini);
        presetKey(k, L"icon", i); WritePrivateProfileStringW(L"presets", k, i < npresets ? presets[i].icon : NULL, g_ini);
        presetKey(k, L"mask", i);
        WCHAR m[NTWEAKS + 1]; for (int t = 0; t < NTWEAKS; t++) m[t] = presets[i].mask[t] ? presets[i].mask[t] : L'0'; m[NTWEAKS] = 0;
        WritePrivateProfileStringW(L"presets", k, i < npresets ? m : NULL, g_ini);
    }
}
static void presetsLoad(void) {
    WCHAR k[24], m[NTWEAKS + 8];
    npresets = GetPrivateProfileIntW(L"presets", L"n", 0, g_ini);
    if (npresets > MAXPRESETS) npresets = MAXPRESETS;
    for (int i = 0; i < npresets; i++) {
        presetKey(k, L"name", i); GetPrivateProfileStringW(L"presets", k, L"Preset", presets[i].name, 40, g_ini);
        presetKey(k, L"icon", i); GetPrivateProfileStringW(L"presets", k, L"", presets[i].icon, 64, g_ini);
        presetKey(k, L"mask", i); GetPrivateProfileStringW(L"presets", k, L"", m, NTWEAKS + 8, g_ini);
        for (int t = 0; t < NTWEAKS; t++) presets[i].mask[t] = (t < (int)wcslen(m) && m[t] == L'1') ? '1' : '0';
        presets[i].mask[NTWEAKS] = 0;
    }
}
static int presetCount(const Preset *p) { int c = 0; for (int t = 0; t < NTWEAKS; t++) c += p->mask[t] == '1'; return c; }
static int presetAdd(const WCHAR *name) {
    if (npresets >= MAXPRESETS) return -1;
    Preset *p = &presets[npresets];
    memset(p, 0, sizeof *p);
    wcsncpy(p->name, name, 39);
    for (int t = 0; t < NTWEAKS; t++) p->mask[t] = tweaks[t].on ? '1' : '0';
    npresets++; presetsSave();
    return npresets - 1;
}
static void presetDelete(int i) {
    for (int j = i; j < npresets - 1; j++) presets[j] = presets[j + 1];
    npresets--; presetsSave();
}
static void presetApply(int i) {
    int on = 0, off = 0, locked = 0, bad = 0;
    for (int t = 0; t < NTWEAKS; t++) {
        int want = presets[i].mask[t] == '1';
        if (want == tweaks[t].on) continue;
        if (want && tweakLocked(t)) { locked++; continue; }
        if (want) applyTweak(t); else revertTweak(t);
        tweaks[t].on = isTweakOn(t);
        if (tweaks[t].on == want) { if (want) on++; else off++; } else bad++;
    }
    swprintf(g_status, 400, L"Preset \"%ls\" applied: %d turned on, %d turned off.%ls%ls", presets[i].name, on, off,
             locked ? L" Some tweaks in it need Pro." : L"", bad ? L" Some couldn't be changed - run as administrator." : L"");
}

// ---- Fortnite item search (public item list, fortnite-api.com) ----
#include "itemparse.inc"
static ItemRes results[24]; static volatile int nres, g_searching;
static WCHAR g_picStatus[200] = L"Type a Fortnite item name (try: raven, peely, drift) and press Enter.";
static int g_pickPreset = -1;

static void urlEncode(const WCHAR *q, char *out, int max) {
    char u[200]; WideCharToMultiByte(CP_UTF8, 0, q, -1, u, sizeof u, NULL, NULL);
    int n = 0;
    for (const unsigned char *p = (const unsigned char *)u; *p && n < max - 4; p++) {
        if (isalnum(*p)) out[n++] = (char)*p; else n += snprintf(out + n, 4, "%%%02X", *p);
    }
    out[n] = 0;
}
static DWORD WINAPI searchThread(LPVOID arg) {
    WCHAR *q = (WCHAR *)arg;
    g_searching = 1; nres = 0;
    WCHAR tmp[MAX_PATH], url[600]; char enc[300];
    dataDir(tmp, NULL); wcscat(tmp, L"\\search.json");
    urlEncode(q, enc, sizeof enc);
    swprintf(url, 600, L"https://fortnite-api.com/v2/cosmetics/br/search/all?name=%hs&matchMethod=contains&language=en", enc);
    wcscpy(g_picStatus, L"Searching...");
    PostMessageW(g_hwnd, WM_APP, 0, 0);
    DeleteFileW(tmp);
    if (URLDownloadToFileW(NULL, url, tmp, 0, NULL) != S_OK) {
        wcscpy(g_picStatus, L"Couldn't reach the Fortnite item list. Check your internet and try again (or no item matched that name).");
        g_searching = 0; free(q); PostMessageW(g_hwnd, WM_APP, 0, 0); return 0;
    }
    HANDLE f = CreateFileW(tmp, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    DWORD sz = f == INVALID_HANDLE_VALUE ? 0 : GetFileSize(f, NULL), got = 0;
    char *buf = (sz && sz < 8000000) ? malloc(sz + 1) : NULL;
    if (buf) { ReadFile(f, buf, sz, &got, NULL); buf[got] = 0; }
    if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
    WCHAR iconUrl[24][300];
    int n = buf ? parseItems(buf, results, iconUrl, 24) : 0;
    int formatOk = buf && strstr(buf, "\"data\"") != NULL;
    int hadBuf = buf != NULL; free(buf); free(q); (void)hadBuf;
    if (!n) { wcscpy(g_picStatus, formatOk ? L"No items found. Try a different name." : L"Couldn't read the Fortnite item list (no connection, or the service is down). Try again later."); g_searching = 0; PostMessageW(g_hwnd, WM_APP, 0, 0); return 0; }
    nres = n;
    swprintf(g_picStatus, 200, L"%d item%ls found - loading pictures... Click one to use it.", n, n == 1 ? L"" : L"s");
    PostMessageW(g_hwnd, WM_APP, 0, 0);
    for (int i = 0; i < n; i++) {
        WCHAR p[MAX_PATH]; iconPath(p, results[i].id);
        if (GetFileAttributesW(p) == INVALID_FILE_ATTRIBUTES) URLDownloadToFileW(NULL, iconUrl[i], p, 0, NULL);
        PostMessageW(g_hwnd, WM_APP, 0, 0);
    }
    swprintf(g_picStatus, 200, L"%d item%ls found. Click one to use it as the icon.", n, n == 1 ? L"" : L"s");
    g_searching = 0; PostMessageW(g_hwnd, WM_APP, 0, 0);
    return 0;
}
static void startSearch(const WCHAR *q) {
    if (g_searching) return;
    WCHAR *c = _wcsdup(q); if (!c) return;
    WCHAR *s = c; while (*s == L' ') s++;
    if (!*s) { free(c); return; }
    CreateThread(NULL, 0, searchThread, c, 0, NULL);
}

// ---- offline assistant ----
typedef struct { WCHAR t[1200]; int me; } Msg;
static Msg msgs[80]; static int nmsgs;
static void chatAdd(const WCHAR *t, int me) {
    if (nmsgs == 80) { memmove(msgs, msgs + 1, 79 * sizeof(Msg)); nmsgs = 79; }
    wcsncpy(msgs[nmsgs].t, t, 1199); msgs[nmsgs].t[1199] = 0; msgs[nmsgs].me = me; nmsgs++;
}
static void analyzePc(WCHAR *o, int n) {
    int len = 0;
#define ADD(...) len += swprintf(o + len, n - len, __VA_ARGS__)
    ADD(L"Here's what I see right now:\n");
    ADD(L"• CPU %.0f%% total, busiest core %.0f%%.", st.cpuLoad, st.cpuMax);
    if (st.cpuMax >= 90 && st.cpuLoad < 70) ADD(L" One core is maxed: Fortnite is held back by a single core. Set Rendering Mode to Performance in-game, and turn on High Performance Power Plan and Foreground App Boost.");
    else if (st.cpuLoad >= 90) ADD(L" The whole CPU is busy: close background apps (Background App Closer).");
    else if (st.cpuMax < 40) ADD(L" That's low. For a useful reading, run this while you're in a match.");
    ADD(L"\n");
    if (st.gpuLoad >= 0) { ADD(L"• GPU %.0f%%.", st.gpuLoad); ADD(st.gpuLoad >= 95 ? L" The GPU is the limit: lower graphics settings or resolution.\n" : L" The GPU has headroom.\n"); }
    else ADD(L"• No GPU readings (NVIDIA cards only).\n");
    ADD(L"• RAM %.0f%%.", st.ram); ADD(st.ram >= 85 ? L" Nearly full: close browsers and run Background App Closer.\n" : L" Fine.\n");
    if (st.diskUse >= 90) ADD(L"• Disk %.0f%% full (%.0f GB free): run Temp File Cleaner and Shader Cache Cleaner.\n", st.diskUse, st.diskFree);
    int off = 0; WCHAR list[400] = L"";
    for (int k = 0; k < (int)(sizeof recSet / sizeof recSet[0]); k++) {
        int t = recSet[k];
        if (!tweaks[t].on && !tweakLocked(t) && off < 4) { if (off) wcscat(list, L", "); wcscat(list, tdefs[t].name); off++; }
    }
    if (off) ADD(L"• FPS tweaks you can still turn on: %ls.\n", list);
    int lockedRec = 0; for (int k = 0; k < (int)(sizeof recSet / sizeof recSet[0]); k++) if (!tweaks[recSet[k]].on && tweakLocked(recSet[k])) lockedRec++;
    if (lockedRec) ADD(L"• Pro unlocks %d more FPS tweaks.\n", lockedRec);
    if (!off && !lockedRec) ADD(L"• All FPS tweaks are already on. \n");
    ADD(L"\nAsk me about FPS, ping, input delay or stutter for more.");
#undef ADD
}
static int has(const WCHAR *q, const WCHAR *w) { return wcsstr(q, w) != NULL; }
static void answer(const WCHAR *qin, WCHAR *o, int n) {
    WCHAR q[300]; wcsncpy(q, qin, 299); q[299] = 0; for (WCHAR *p = q; *p; p++) *p = (WCHAR)towlower(*p);
    if (has(q, L"analy") || has(q, L"my pc") || has(q, L"recommend") || has(q, L"what should") || has(q, L"scan") || has(q, L"check")) analyzePc(o, n);
    else if (has(q, L"ping") || has(q, L"latency") || has(q, L"lag")) wcsncpy(o, L"Ping is mostly your connection, not Windows. Use a wired Ethernet cable, pick the closest matchmaking region, and stop downloads or streaming while you play. Fortnite uses UDP, so TCP tweaks like Nagle's don't change it. Flush DNS Cache (Addons) can help a slow connect.", n);
    else if (has(q, L"input") || has(q, L"delay") || has(q, L"mouse") || has(q, L"aim") || has(q, L"click")) wcsncpy(o, L"For input delay: press Apply low-latency preset in Optimizations (mouse acceleration off, precise timer, fast keyboard response, foreground boost). Also set Rendering Mode to Performance, uncap FPS, turn VSync off, and use the highest refresh rate your monitor supports.", n);
    else if (has(q, L"stutter") || has(q, L"spike") || has(q, L"freeze") || has(q, L"hitch")) wcsncpy(o, L"Stutter usually means one busy core or background activity. Run Background App Closer, turn on Disable Background Apps and High Performance Power Plan, run Shader Cache Cleaner, and make sure you have free disk space. Check Home: if Busiest core is near 100%, that's the cause.", n);
    else if (has(q, L"fps") || has(q, L"frame") || has(q, L"perform")) wcsncpy(o, L"Biggest FPS wins: 1) Rendering Mode: Performance in Fortnite. 2) Fortnite - Performance profile (Game Library). 3) Apply FPS preset. 4) Update your GPU driver. Tweaks help a little; in-game settings matter most. Press Analyze my PC for advice on your numbers.", n);
    else if (has(q, L"cpu") || has(q, L"processor")) wcsncpy(o, L"Fortnite leans on one or two CPU cores, so watch Busiest core on Home, not just total. Near 100% means CPU-limited: lower CPU-heavy settings and use Rendering Mode: Performance.", n);
    else if (has(q, L"gpu") || has(q, L"graphics")) wcsncpy(o, L"If GPU usage sits near 100% you're GPU-limited: lower resolution or graphics settings. If it's low while FPS is low, the CPU is the limit instead.", n);
    else if (has(q, L"ram") || has(q, L"memory")) wcsncpy(o, L"RAM under about 85% is fine. Above that, close browsers and launchers and run Background App Closer.", n);
    else if (has(q, L"preset") || has(q, L"save") || has(q, L"undo") || has(q, L"revert") || has(q, L"backup")) wcsncpy(o, L"Presets saves your current tweaks under a name so you can switch setups in one click. Turn tweaks off to undo them, or use Create Windows restore point on the Presets page.", n);
    else if (has(q, L"pro") || has(q, L"buy") || has(q, L"price") || has(q, L"unlock")) wcsncpy(o, L"Pro unlocks every tweak, unlimited presets, Fortnite profiles and all addons for life on one PC. Open Get Pro, copy your PC ID, and DM OddChoices on Discord.", n);
    else if (has(q, L"hello") || has(q, L"hi") || has(q, L"hey")) wcsncpy(o, L"Hey! Press Analyze my PC, or ask about FPS, ping, input delay or stutter.", n);
    else wcsncpy(o, L"I can help with FPS, ping, input delay, stutter, CPU, GPU and RAM. Try: \"why is my FPS low\" or press Analyze my PC.", n);
    o[n - 1] = 0;
}

// ---- updater ----
static volatile int g_updAvail, g_updBusy; static WCHAR g_updVer[24];
static int verNewer(const WCHAR *a, const WCHAR *b) { // is a newer than b
    int x[3] = {0, 0, 0}, y[3] = {0, 0, 0};
    swscanf(a, L"%d.%d.%d", &x[0], &x[1], &x[2]); swscanf(b, L"%d.%d.%d", &y[0], &y[1], &y[2]);
    for (int i = 0; i < 3; i++) { if (x[i] != y[i]) return x[i] > y[i]; }
    return 0;
}
static DWORD WINAPI updateCheckThread(LPVOID p) {
    (void)p;
    WCHAR tmp[MAX_PATH], url[400]; dataDir(tmp, NULL); wcscat(tmp, L"\\version.txt");
    swprintf(url, 400, L"%lsversion.txt?t=%lu", UPDATE_BASE, GetTickCount());
    DeleteFileW(tmp);
    if (URLDownloadToFileW(NULL, url, tmp, 0, NULL) != S_OK) return 0;
    char b[64] = {0}; DWORD got = 0;
    HANDLE f = CreateFileW(tmp, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    ReadFile(f, b, 40, &got, NULL); CloseHandle(f);
    WCHAR v[24]; int n = 0, a1, a2, a3;
    for (int i = 0; i < 20 && (isdigit((unsigned char)b[i]) || b[i] == '.'); i++) v[n++] = (WCHAR)b[i];
    v[n] = 0;
    if (swscanf(v, L"%d.%d.%d", &a1, &a2, &a3) != 3) return 0;   // not a real version (e.g. a 404 page)
    if (n && verNewer(v, APP_VERSION)) { wcscpy(g_updVer, v); g_updAvail = 1; PostMessageW(g_hwnd, WM_APP, 0, 0); }
    return 0;
}
static DWORD WINAPI updateInstallThread(LPVOID p) {
    (void)p; g_updBusy = 1;
    wcscpy(g_status, L"Downloading the update...");
    PostMessageW(g_hwnd, WM_APP, 0, 0);
    WCHAR exe[MAX_PATH], nw[MAX_PATH + 8], url[400], bat[MAX_PATH];
    GetModuleFileNameW(NULL, exe, MAX_PATH);
    swprintf(nw, MAX_PATH + 8, L"%ls.new", exe);
    swprintf(url, 400, L"%ls%ls?t=%lu", UPDATE_BASE, EXE_NAME, GetTickCount());
    DeleteFileW(nw);
    int ok = URLDownloadToFileW(NULL, url, nw, 0, NULL) == S_OK;
    if (ok) { // sanity check: a real Windows program of plausible size
        HANDLE f = CreateFileW(nw, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        char mz[2] = {0}; DWORD got = 0, sz = 0;
        if (f != INVALID_HANDLE_VALUE) { sz = GetFileSize(f, NULL); ReadFile(f, mz, 2, &got, NULL); CloseHandle(f); }
        ok = sz > 200000 && mz[0] == 'M' && mz[1] == 'Z';
    }
    if (!ok) { DeleteFileW(nw); wcscpy(g_status, L"The update download failed. Check your internet and try again."); g_updBusy = 0; PostMessageW(g_hwnd, WM_APP, 0, 0); return 0; }
    GetTempPathW(MAX_PATH, bat); wcscat(bat, L"nero_update.bat");
    FILE *f = _wfopen(bat, L"w");
    if (f) {
        fprintf(f, "@echo off\r\nping 127.0.0.1 -n 3 >nul\r\nmove /y \"%ls\" \"%ls\" >nul\r\nstart \"\" \"%ls\"\r\ndel \"%%~f0\"\r\n", nw, exe, exe);
        fclose(f);
        WCHAR cmd[MAX_PATH + 40]; swprintf(cmd, MAX_PATH + 40, L"cmd.exe /c \"%ls\"", bat);
        STARTUPINFOW si = {sizeof si}; PROCESS_INFORMATION pi; si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
        if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            PostMessageW(g_hwnd, WM_CLOSE, 0, 0); return 0;
        }
    }
    wcscpy(g_status, L"Couldn't start the update."); g_updBusy = 0; PostMessageW(g_hwnd, WM_APP, 0, 0);
    return 0;
}
