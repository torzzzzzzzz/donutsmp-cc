// ---------- tweak engine: real, reversible Windows changes ----------
// Included from main.c. Every tweak can detect its own state (isTweakOn),
// apply, and revert to the value that was there before.

typedef struct {
    HKEY hive; const WCHAR *key, *name;
    int isStr; DWORD onD; const WCHAR *onS;   // value when the tweak is ON
    int offD; const WCHAR *offS;               // fallback when reverting (offD -1 and offS NULL = delete)
} RegOp;

#define HKLM_ HKEY_LOCAL_MACHINE
#define HKCU_ HKEY_CURRENT_USER
#define D(h, k, n, on, off) {h, k, n, 0, on, NULL, off, NULL}
#define STR(h, k, n, on, off) {h, k, n, 1, 0, on, -1, off}

#define K_GCS L"System\\GameConfigStore"
#define K_GBAR L"Software\\Microsoft\\GameBar"
#define K_PROF L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile"
#define K_GAMES K_PROF L"\\Tasks\\Games"
#define K_FN L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\FortniteClient-Win64-Shipping.exe\\PerfOptions"

static const RegOp opsGameMode[] = {
    D(HKCU_, K_GBAR, L"AutoGameModeEnabled", 1, -1), D(HKCU_, K_GBAR, L"AllowAutoGameMode", 1, -1)};
static const RegOp opsDvr[] = {
    D(HKCU_, K_GCS, L"GameDVR_Enabled", 0, 1),
    D(HKCU_, L"Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR", L"AppCaptureEnabled", 0, 1),
    D(HKLM_, L"SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR", L"AllowGameDVR", 0, -1)};
static const RegOp opsBar[] = {
    D(HKCU_, K_GBAR, L"UseNexusForGameBarEnabled", 0, 1), D(HKCU_, K_GBAR, L"ShowStartupPanel", 0, 1)};
static const RegOp opsFso[] = {
    D(HKCU_, K_GCS, L"GameDVR_FSEBehaviorMode", 2, 0), D(HKCU_, K_GCS, L"GameDVR_HonorUserFSEBehaviorMode", 1, 0),
    D(HKCU_, K_GCS, L"GameDVR_FSEBehavior", 2, 0), D(HKCU_, K_GCS, L"GameDVR_DXGIHonorFSEWindowsCompatible", 1, 0)};
static const RegOp opsThrottle[] = {D(HKLM_, L"SYSTEM\\CurrentControlSet\\Control\\Power\\PowerThrottling", L"PowerThrottlingOff", 1, -1)};
static const RegOp opsSched[] = {
    D(HKLM_, K_PROF, L"SystemResponsiveness", 0, 20),
    D(HKLM_, K_GAMES, L"GPU Priority", 8, 8), D(HKLM_, K_GAMES, L"Priority", 6, 2),
    STR(HKLM_, K_GAMES, L"Scheduling Category", L"High", L"Medium"), STR(HKLM_, K_GAMES, L"SFIO Priority", L"High", L"Normal")};
static const RegOp opsNet[] = {D(HKLM_, K_PROF, L"NetworkThrottlingIndex", 0xFFFFFFFF, 10)};
static const RegOp opsDO[] = {D(HKLM_, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization\\Config", L"DODownloadMode", 0, -1)};
static const RegOp opsMouse[] = {
    STR(HKCU_, L"Control Panel\\Mouse", L"MouseSpeed", L"0", L"1"),
    STR(HKCU_, L"Control Panel\\Mouse", L"MouseThreshold1", L"0", L"6"),
    STR(HKCU_, L"Control Panel\\Mouse", L"MouseThreshold2", L"0", L"10")};
static const RegOp opsSticky[] = {
    STR(HKCU_, L"Control Panel\\Accessibility\\StickyKeys", L"Flags", L"506", L"510"),
    STR(HKCU_, L"Control Panel\\Accessibility\\Keyboard Response", L"Flags", L"122", L"126"),
    STR(HKCU_, L"Control Panel\\Accessibility\\ToggleKeys", L"Flags", L"58", L"62")};
static const RegOp opsFortnite[] = {D(HKLM_, K_FN, L"CpuPriorityClass", 3, -1)};
static const RegOp opsBg[] = {
    D(HKCU_, L"Software\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications", L"GlobalUserDisabled", 1, 0),
    D(HKLM_, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy", L"LetAppsRunInBackground", 2, -1)};

static const int recSet[] = {0, 2, 3, 5, 6, 12, 13};
static int isRec(int t) { for (int i = 0; i < (int)(sizeof recSet / sizeof recSet[0]); i++) if (recSet[i] == t) return 1; return 0; }
enum { SP_REG, SP_POWER, SP_NAGLE, SP_HIBER, SP_MOUSE };
typedef struct { const WCHAR *name, *desc; const RegOp *ops; int nops; int special; } TweakDef;
#define OPS(a) a, (int)(sizeof(a) / sizeof(a[0]))
static const TweakDef tdefs[NTWEAKS] = {
    {L"High Performance Power Plan", L"Stops your CPU slowing down to save power.", NULL, 0, SP_POWER},
    {L"Enable Game Mode", L"Windows favors your game. Small, safe.", OPS(opsGameMode), SP_REG},
    {L"Disable Game DVR", L"Turns off background recording that costs FPS.", OPS(opsDvr), SP_REG},
    {L"Disable Xbox Game Bar", L"Removes the overlay that can cause stutters.", OPS(opsBar), SP_REG},
    {L"Disable Fullscreen Optimizations", L"Mixed results. Turn off if it feels worse.", OPS(opsFso), SP_REG},
    {L"Disable Power Throttling", L"Stops Windows throttling apps to save battery.", OPS(opsThrottle), SP_REG},
    {L"Prioritize Games (CPU Scheduling)", L"Gives games the highest slice of CPU and GPU time.", OPS(opsSched), SP_REG},
    {L"Disable Network Throttling", L"Lifts a Windows packet cap. Small effect.", OPS(opsNet), SP_REG},
    {L"Disable Nagle's Algorithm", L"TCP only (launcher, downloads). No effect on Fortnite ping.", NULL, 0, SP_NAGLE},
    {L"Stop Windows Update Sharing", L"Stops your PC using bandwidth to upload updates to others.", OPS(opsDO), SP_REG},
    {L"Disable Mouse Acceleration", L"Consistent aim: same movement, same distance.", OPS(opsMouse), SP_MOUSE},
    {L"Disable Sticky Keys Popup", L"Stops the Shift-key popup from minimizing your game.", OPS(opsSticky), SP_REG},
    {L"Fortnite High CPU Priority", L"Makes Windows always run Fortnite at High priority.", OPS(opsFortnite), SP_REG},
    {L"Disable Background Apps", L"Stops apps running behind the scenes and using CPU.", OPS(opsBg), SP_REG},
    {L"Disable Hibernation", L"Frees several GB of disk space and skips Fast Startup.", NULL, 0, SP_HIBER}};

// ---- registry helpers ----
static int regRead(const RegOp *o, int *exists, DWORD *d, WCHAR *s, int slen) {
    HKEY k; *exists = 0;
    if (RegOpenKeyExW(o->hive, o->key, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &k) != ERROR_SUCCESS) return 0;
    DWORD type, sz;
    if (o->isStr) { sz = (DWORD)(slen * sizeof(WCHAR)); if (RegQueryValueExW(k, o->name, NULL, &type, (BYTE *)s, &sz) == ERROR_SUCCESS && type == REG_SZ) *exists = 1; }
    else { sz = sizeof(DWORD); if (RegQueryValueExW(k, o->name, NULL, &type, (BYTE *)d, &sz) == ERROR_SUCCESS && type == REG_DWORD) *exists = 1; }
    RegCloseKey(k); return 1;
}
static int regWrite(const RegOp *o, DWORD d, const WCHAR *s) {
    HKEY k;
    if (RegCreateKeyExW(o->hive, o->key, 0, NULL, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &k, NULL) != ERROR_SUCCESS) return 0;
    LONG r = o->isStr ? RegSetValueExW(k, o->name, 0, REG_SZ, (const BYTE *)s, (DWORD)((wcslen(s) + 1) * sizeof(WCHAR)))
                      : RegSetValueExW(k, o->name, 0, REG_DWORD, (const BYTE *)&d, sizeof d);
    RegCloseKey(k); return r == ERROR_SUCCESS;
}
static int regDelete(const RegOp *o) {
    HKEY k;
    if (RegOpenKeyExW(o->hive, o->key, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &k) != ERROR_SUCCESS) return 1;
    RegDeleteValueW(k, o->name); RegCloseKey(k); return 1;
}
static int opIsOn(const RegOp *o) {
    int ex; DWORD d = 0; WCHAR s[64] = L"";
    regRead(o, &ex, &d, s, 64);
    if (!ex) return 0;
    return o->isStr ? wcscmp(s, o->onS) == 0 : d == o->onD;
}
static void origKey(WCHAR *k, int t, int i) { swprintf(k, 24, L"o%d_%d", t, i); }

// ---- power plan ----
#define GUID_HIGH L"8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"
#define GUID_BAL L"381b4222-f694-41f0-9685-ff5bb260df2e"
static int findGuid(const char *b, WCHAR *out) {
    for (int i = 0; b[i] && b[i + 36]; i++) {
        if (b[i + 8] == '-' && b[i + 13] == '-' && b[i + 18] == '-' && b[i + 23] == '-') {
            int ok = 1;
            for (int j = 0; j < 36; j++) { char c = b[i + j]; if (!(c == '-' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) { ok = 0; break; } }
            if (ok) { for (int j = 0; j < 36; j++) out[j] = (WCHAR)b[i + j]; out[36] = 0; return 1; }
        }
    }
    return 0;
}
static int activeScheme(WCHAR *g) {
    char buf[512]; WCHAR cmd[] = L"powercfg.exe /getactivescheme";
    if (runHidden(cmd, buf, sizeof buf) != 0) return 0;
    return findGuid(buf, g);
}
static int powerOn(void) {
    WCHAR g[40];
    if (!activeScheme(g)) return 0;
    return _wcsnicmp(g, L"8c5e7fda", 8) == 0 || _wcsnicmp(g, L"e9a42b02", 8) == 0;
}
static int setScheme(const WCHAR *g) {
    WCHAR cmd[200]; char buf[256];
    swprintf(cmd, 200, L"powercfg.exe /setactive %ls", g);
    return runHidden(cmd, buf, sizeof buf) == 0;
}
static int powerApply(void) {
    WCHAR cur[40];
    if (activeScheme(cur) && _wcsnicmp(cur, L"8c5e7fda", 8) != 0) WritePrivateProfileStringW(L"orig", L"power", cur, g_ini);
    if (setScheme(GUID_HIGH)) return 1;
    // High Performance plan missing (some laptops): create it from the template.
    char buf[512]; WCHAR cmd[200], ng[40];
    swprintf(cmd, 200, L"powercfg.exe /duplicatescheme %ls", GUID_HIGH);
    if (runHidden(cmd, buf, sizeof buf) == 0 && findGuid(buf, ng)) return setScheme(ng);
    return 0;
}
static int powerRevert(void) {
    WCHAR g[40];
    GetPrivateProfileStringW(L"orig", L"power", GUID_BAL, g, 40, g_ini);
    int ok = setScheme(g) || setScheme(GUID_BAL);
    WritePrivateProfileStringW(L"orig", L"power", NULL, g_ini);
    return ok;
}

// ---- Nagle (per network adapter) ----
#define K_IFACES L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces"
static int nagleEach(int mode) { // mode 0 = check, 1 = apply, 2 = revert
    HKEY root; int found = 0, ok = 1;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, K_IFACES, 0, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY, &root) != ERROR_SUCCESS) return 0;
    WCHAR name[128];
    for (DWORD i = 0; RegEnumKeyExW(root, i, name, &(DWORD){128}, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; i++) {
        HKEY k;
        if (RegOpenKeyExW(root, name, 0, KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_64KEY, &k) != ERROR_SUCCESS) { if (mode) ok = 0; continue; }
        DWORD d = 0, sz = sizeof d, one = 1;
        if (mode == 0) { if (RegQueryValueExW(k, L"TCPNoDelay", NULL, NULL, (BYTE *)&d, &sz) == ERROR_SUCCESS && d == 1) found = 1; }
        else if (mode == 1) {
            if (RegSetValueExW(k, L"TcpAckFrequency", 0, REG_DWORD, (BYTE *)&one, sizeof one) != ERROR_SUCCESS) ok = 0;
            if (RegSetValueExW(k, L"TCPNoDelay", 0, REG_DWORD, (BYTE *)&one, sizeof one) != ERROR_SUCCESS) ok = 0;
        } else { RegDeleteValueW(k, L"TcpAckFrequency"); RegDeleteValueW(k, L"TCPNoDelay"); }
        RegCloseKey(k);
    }
    RegCloseKey(root);
    return mode == 0 ? found : ok;
}

// ---- hibernation ----
static int hiberOn(void) { // returns 1 when hibernation is DISABLED (tweak on)
    DWORD d = 1, sz = sizeof d; HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Power", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &k) != ERROR_SUCCESS) return 0;
    RegQueryValueExW(k, L"HibernateEnabled", NULL, NULL, (BYTE *)&d, &sz); RegCloseKey(k);
    return d == 0;
}
static int hiberSet(int off) {
    char buf[256]; WCHAR cmd[100];
    swprintf(cmd, 100, L"powercfg.exe /hibernate %ls", off ? L"off" : L"on");
    return runHidden(cmd, buf, sizeof buf) == 0;
}

// ---- mouse: push registry values to the running session ----
static void mouseApply(void) {
    int v[3] = {0, 0, 0};
    for (int i = 0; i < 3; i++) {
        int ex; DWORD d; WCHAR s[16] = L"";
        regRead(&opsMouse[i], &ex, &d, s, 16);
        v[i] = ex ? _wtoi(s) : opsMouse[i].offS ? _wtoi(opsMouse[i].offS) : 0;
    }
    SystemParametersInfoW(SPI_SETMOUSE, 0, v, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

// ---- generic apply / revert / state ----
static int isTweakOn(int t) {
    const TweakDef *d = &tdefs[t];
    switch (d->special) {
        case SP_POWER: return powerOn();
        case SP_NAGLE: return nagleEach(0);
        case SP_HIBER: return hiberOn();
        default:
            for (int i = 0; i < d->nops; i++) if (!opIsOn(&d->ops[i])) return 0;
            return d->nops > 0;
    }
}
static int applyTweak(int t) {
    const TweakDef *d = &tdefs[t];
    if (d->special == SP_POWER) return powerApply();
    if (d->special == SP_NAGLE) return nagleEach(1);
    if (d->special == SP_HIBER) return hiberSet(1);
    int ok = 1;
    for (int i = 0; i < d->nops; i++) {
        const RegOp *o = &d->ops[i];
        if (!opIsOn(o)) {
            int ex; DWORD dv = 0; WCHAR sv[64] = L"", key[24], val[80];
            regRead(o, &ex, &dv, sv, 64); origKey(key, t, i);
            if (!ex) wcscpy(val, L"N"); else if (o->isStr) swprintf(val, 80, L"S:%ls", sv); else swprintf(val, 80, L"D:%lu", dv);
            WritePrivateProfileStringW(L"orig", key, val, g_ini);
        }
        if (!regWrite(o, o->onD, o->onS)) ok = 0;
    }
    if (d->special == SP_MOUSE) mouseApply();
    return ok;
}
static int revertTweak(int t) {
    const TweakDef *d = &tdefs[t];
    if (d->special == SP_POWER) return powerRevert();
    if (d->special == SP_NAGLE) return nagleEach(2);
    if (d->special == SP_HIBER) return hiberSet(0);
    int ok = 1;
    for (int i = 0; i < d->nops; i++) {
        const RegOp *o = &d->ops[i];
        WCHAR key[24], val[80]; origKey(key, t, i);
        GetPrivateProfileStringW(L"orig", key, L"?", val, 80, g_ini);
        if (val[0] == L'N') ok &= regDelete(o);
        else if (val[0] == L'D' && !o->isStr) ok &= regWrite(o, (DWORD)wcstoul(val + 2, NULL, 10), NULL);
        else if (val[0] == L'S' && o->isStr) ok &= regWrite(o, 0, val + 2);
        else if (o->isStr) ok &= o->offS ? regWrite(o, 0, o->offS) : regDelete(o);
        else ok &= o->offD >= 0 ? regWrite(o, (DWORD)o->offD, NULL) : regDelete(o);
        WritePrivateProfileStringW(L"orig", key, NULL, g_ini);
    }
    if (d->special == SP_MOUSE) mouseApply();
    return ok;
}
static void tweakToggle(int t) {
    int want = !tweaks[t].on;
    int ok = want ? applyTweak(t) : revertTweak(t);
    tweaks[t].on = isTweakOn(t);
    if (tweaks[t].on == want) swprintf(g_status, 400, L"%ls: %ls.", tdefs[t].name, want ? L"on" : L"off");
    else swprintf(g_status, 400, L"Couldn't change \"%ls\"%ls. Make sure Nero Tweaks is running as administrator.", tdefs[t].name, ok ? L" (Windows didn't accept it)" : L"");
}
static void applyRecommended(void) {
    int n = 0;
    for (int i = 0; i < NTWEAKS; i++) {
        if (!isRec(i) || tweaks[i].on) continue;
        applyTweak(i); tweaks[i].on = isTweakOn(i); n += tweaks[i].on;
    }
    swprintf(g_status, 400, L"FPS preset: %d tweak%ls applied. Restart your PC for best results.", n, n == 1 ? L"" : L"s");
}
static void setAll(int v) {
    int n = 0, bad = 0;
    for (int i = 0; i < NTWEAKS; i++) {
        if (tweaks[i].on == v) continue;
        if (v) applyTweak(i); else revertTweak(i);
        tweaks[i].on = isTweakOn(i);
        if (tweaks[i].on == v) n++; else bad++;
    }
    swprintf(g_status, 400, L"%d tweak%ls %ls.%ls", n, n == 1 ? L"" : L"s", v ? L"applied" : L"reverted", bad ? L" Some could not be changed - run as administrator." : L"");
}

// ---- Fortnite profiles (GameUserSettings.ini, backed up first) ----
static int fnPath(WCHAR *p) {
    WCHAR d[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, d))) return 0;
    swprintf(p, MAX_PATH, L"%ls\\FortniteGame\\Saved\\Config\\WindowsClient\\GameUserSettings.ini", d);
    return GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES;
}
static void iniSet(const WCHAR *file, const WCHAR *sec, const WCHAR *k, const WCHAR *v) { WritePrivateProfileStringW(sec, k, v, file); }
static void gameToggle(int i) {
    WCHAR p[MAX_PATH], bak[MAX_PATH + 8];
    if (!fnPath(p)) { wcscpy(g_status, L"Fortnite settings file not found. Launch Fortnite once, close it, then try again."); return; }
    swprintf(bak, MAX_PATH + 8, L"%ls.nero.bak", p);
    SetFileAttributesW(p, FILE_ATTRIBUTE_NORMAL);
    if (games[i].on) {
        if (CopyFileW(bak, p, FALSE)) { DeleteFileW(bak); wcscpy(g_status, L"Fortnite settings restored to how they were."); }
        else wcscpy(g_status, L"No backup found, so nothing was changed.");
        games[0].on = games[1].on = 0;
    } else {
        if (GetFileAttributesW(bak) == INVALID_FILE_ATTRIBUTES) CopyFileW(p, bak, TRUE);
        const WCHAR *sg = L"ScalabilityGroups", *us = L"/Script/FortniteGame.FortGameUserSettings";
        int perf = (i == 0);
        iniSet(p, sg, L"sg.ShadowQuality", L"0"); iniSet(p, sg, L"sg.EffectsQuality", L"0");
        iniSet(p, sg, L"sg.PostProcessQuality", L"0"); iniSet(p, sg, L"sg.FoliageQuality", L"0");
        iniSet(p, sg, L"sg.ShadingQuality", L"0"); iniSet(p, sg, L"sg.AntiAliasingQuality", L"0");
        iniSet(p, sg, L"sg.ViewDistanceQuality", perf ? L"0" : L"3");
        iniSet(p, sg, L"sg.TextureQuality", perf ? L"0" : L"2");
        iniSet(p, us, L"bUseVSync", L"False"); iniSet(p, us, L"FrameRateLimit", L"0.000000");
        games[0].on = games[1].on = 0; games[i].on = 1;
        wcscpy(g_status, perf ? L"Performance profile applied. Also set Rendering Mode to Performance in Fortnite (Settings > Video)." : L"Competitive profile applied. Also try Rendering Mode: Performance in Fortnite (Settings > Video).");
    }
    for (int k = 0; k < 2; k++) { WCHAR key[8]; swprintf(key, 8, L"g%d", k); WritePrivateProfileStringW(L"games", key, games[k].on ? L"1" : L"0", g_ini); }
}

// ---- addons ----
static ULONGLONG wipeDir(const WCHAR *dir, int removeSelf) {
    WCHAR pat[MAX_PATH + 4]; WIN32_FIND_DATAW f; ULONGLONG freed = 0;
    swprintf(pat, MAX_PATH + 4, L"%ls\\*", dir);
    HANDLE h = FindFirstFileW(pat, &f);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if (!wcscmp(f.cFileName, L".") || !wcscmp(f.cFileName, L"..")) continue;
        WCHAR full[MAX_PATH];
        swprintf(full, MAX_PATH, L"%ls\\%ls", dir, f.cFileName);
        if (f.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
        if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { freed += wipeDir(full, 1); }
        else { ULONGLONG sz = ((ULONGLONG)f.nFileSizeHigh << 32) | f.nFileSizeLow; SetFileAttributesW(full, FILE_ATTRIBUTE_NORMAL); if (DeleteFileW(full)) freed += sz; }
    } while (FindNextFileW(h, &f));
    FindClose(h);
    if (removeSelf) RemoveDirectoryW(dir);
    return freed;
}
static DWORD WINAPI addonThread(LPVOID p) {
    int id = (int)(INT_PTR)p;
    if (id == 1) { // shader caches
        WCHAR base[MAX_PATH], d[MAX_PATH]; ULONGLONG f = 0;
        const WCHAR *sub[] = {L"\\NVIDIA\\DXCache", L"\\NVIDIA\\GLCache", L"\\D3DSCache", L"\\AMD\\DxCache", L"\\AMD\\GLCache"};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, base)))
            for (int i = 0; i < 5; i++) { swprintf(d, MAX_PATH, L"%ls%ls", base, sub[i]); f += wipeDir(d, 0); }
        swprintf(g_status, 400, L"Shader cache cleaned: %.1f MB freed. Games rebuild it on next launch.", f / 1048576.0);
    } else if (id == 2) { // background apps
        const WCHAR *names[] = {L"OneDrive.exe", L"Teams.exe", L"ms-teams.exe", L"Skype.exe", L"SkypeApp.exe", L"Widgets.exe", L"PhoneExperienceHost.exe", L"GameBarPresenceWriter.exe", L"GoogleDriveFS.exe", L"Dropbox.exe", L"AdobeCollabSync.exe", L"msedgewebview2.exe"};
        int n = 0;
        HANDLE sn = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W pe = {sizeof pe};
        if (sn != INVALID_HANDLE_VALUE && Process32FirstW(sn, &pe)) do {
            for (int i = 0; i < (int)(sizeof names / sizeof names[0]); i++)
                if (_wcsicmp(pe.szExeFile, names[i]) == 0) {
                    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (h) { if (TerminateProcess(h, 0)) n++; CloseHandle(h); }
                }
        } while (Process32NextW(sn, &pe));
        if (sn != INVALID_HANDLE_VALUE) CloseHandle(sn);
        swprintf(g_status, 400, n ? L"Closed %d background process%ls." : L"No heavy background apps were running.", n, n == 1 ? L"" : L"es");
    } else if (id == 3) { // temp files
        WCHAR t[MAX_PATH], w[MAX_PATH]; ULONGLONG f = 0;
        DWORD n = GetTempPathW(MAX_PATH, t);
        if (n > 3 && t[n - 1] == L'\\') t[n - 1] = 0;
        f += wipeDir(t, 0);
        if (GetWindowsDirectoryW(w, MAX_PATH)) { wcscat(w, L"\\Temp"); f += wipeDir(w, 0); }
        swprintf(g_status, 400, L"Temp files cleaned: %.1f MB freed.", f / 1048576.0);
    } else if (id == 4) { // dns
        char buf[256]; WCHAR cmd[] = L"ipconfig.exe /flushdns";
        wcscpy(g_status, runHidden(cmd, buf, sizeof buf) == 0 ? L"DNS cache flushed." : L"Couldn't flush the DNS cache.");
    }
    PostMessageW(g_hwnd, WM_APP, 0, 0);
    return 0;
}

// ---- performance overlay window ----
static LRESULT CALLBACK overlayProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT r; GetClientRect(h, &r);
        HBRUSH b = CreateSolidBrush(RGB(0, 0, 0)); FillRect(dc, &r, b); DeleteObject(b);
        HPEN p = CreatePen(PS_SOLID, 1, RGB(255, 255, 255)); SelectObject(dc, p); SelectObject(dc, GetStockObject(NULL_BRUSH));
        Rectangle(dc, 0, 0, r.right, r.bottom); DeleteObject(p);
        SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fBtn);
        WCHAR t[3][48];
        swprintf(t[0], 48, L"CPU  %.0f%%  (core %.0f%%)", st.cpuLoad, st.cpuMax);
        if (st.gpuLoad >= 0) swprintf(t[1], 48, L"GPU  %.0f%%   %.0f°C", st.gpuLoad, st.gpuTemp); else wcscpy(t[1], L"GPU  --");
        swprintf(t[2], 48, L"RAM  %.0f%%", st.ram);
        for (int i = 0; i < 3; i++) TextOutW(dc, S(12), S(8) + i * S(22), t[i], (int)wcslen(t[i]));
        EndPaint(h, &ps); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
static void overlaySet(int on) {
    if (on && !g_overlay) {
        g_overlay = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            L"NeroOverlay", L"", WS_POPUP, S(16), S(16), S(210), S(82), NULL, NULL, GetModuleHandleW(NULL), NULL);
        SetLayeredWindowAttributes(g_overlay, 0, 215, LWA_ALPHA);
    }
    if (g_overlay) ShowWindow(g_overlay, on ? SW_SHOWNOACTIVATE : SW_HIDE);
}
static void addonClick(int i) {
    if (addons[i].act) {
        wcscpy(g_status, L"Working...");
        CreateThread(NULL, 0, addonThread, (LPVOID)(INT_PTR)i, 0, NULL);
    } else {
        addons[i].on = !addons[i].on;
        overlaySet(addons[i].on);
        WritePrivateProfileStringW(L"addons", L"a0", addons[i].on ? L"1" : L"0", g_ini);
        wcscpy(g_status, addons[i].on ? L"Overlay on (top-left of your screen)." : L"Overlay off.");
    }
}
