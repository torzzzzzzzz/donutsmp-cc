// ---------- licensing: free tier + one-time Pro code locked to one PC ----------
// A Pro code is an ECDSA (P-256) signature of this PC's ID, made with a private key only the
// seller has. The app only holds the public key, so codes can't be forged, and a code made for
// one PC's ID does not validate on any other PC.
#include <bcrypt.h>
#include "pubkey.inc"

#define BUY_INFO L"To buy Pro: DM OddChoices on Discord, send them your PC ID above, and they will reply with your Pro code."

static int g_pro;
static WCHAR g_machine[32];
static HWND g_keyEdit;

static int sha256(const void *data, ULONG len, UCHAR out[32]) {
    BCRYPT_ALG_HANDLE a = NULL; int ok = 0;
    if (BCryptOpenAlgorithmProvider(&a, BCRYPT_SHA256_ALGORITHM, NULL, 0) == 0) {
        ok = BCryptHash(a, NULL, 0, (PUCHAR)data, len, out, 32) == 0;
        BCryptCloseAlgorithmProvider(a, 0);
    }
    return ok;
}
static void machineId(WCHAR *out) {
    WCHAR guid[128] = L""; DWORD sz = sizeof guid;
    RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid", RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY, NULL, guid, &sz);
    char a[400]; UCHAR h[32] = {0};
    snprintf(a, sizeof a, "nero-tweaks-v1|%ls", guid);
    sha256(a, (ULONG)strlen(a), h);
    swprintf(out, 32, L"NERO-%02X%02X-%02X%02X-%02X%02X-%02X%02X", h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7]);
}
static int b32decode(const WCHAR *s, UCHAR *out, int max) {
    unsigned buf = 0; int bits = 0, n = 0;
    for (; *s; s++) {
        int v;
        WCHAR c = *s;
        if (c >= L'a' && c <= L'z') c -= 32;
        if (c >= L'A' && c <= L'Z') v = c - L'A'; else if (c >= L'2' && c <= L'7') v = c - L'2' + 26; else continue;
        buf = (buf << 5) | (unsigned)v; bits += 5;
        if (bits >= 8) { bits -= 8; if (n >= max) return -1; out[n++] = (UCHAR)((buf >> bits) & 0xFF); }
    }
    return n;
}
static int licenseValid(const WCHAR *code) {
    UCHAR sig[80]; int n = b32decode(code, sig, 80);
    if (n != 64) return 0;
    WCHAR id[32]; machineId(id);
    char msg[80]; snprintf(msg, sizeof msg, "NERO1|%ls", id);
    UCHAR hash[32]; if (!sha256(msg, (ULONG)strlen(msg), hash)) return 0;
    BCRYPT_ALG_HANDLE alg = NULL; BCRYPT_KEY_HANDLE key = NULL; int ok = 0;
    UCHAR blob[8 + 64]; BCRYPT_ECCKEY_BLOB *hd = (BCRYPT_ECCKEY_BLOB *)blob;
    hd->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC; hd->cbKey = 32; memcpy(blob + 8, PUBKEY, 64);
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0) == 0) {
        if (BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &key, blob, sizeof blob, 0) == 0) {
            ok = BCryptVerifySignature(key, NULL, hash, 32, sig, 64, 0) == 0;
            BCryptDestroyKey(key);
        }
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    return ok;
}
#ifdef OWNER_BUILD
#define IS_OWNER 1
#else
#define IS_OWNER 0
#endif
static const WCHAR *planName(void) { return IS_OWNER ? L"OWNER VERSION" : g_pro ? L"PRO VERSION" : L"FREE VERSION"; }

static void loadLicense(void) {
    WCHAR code[600] = L"";
    machineId(g_machine);
#ifdef OWNER_BUILD
    g_pro = 1; (void)code; return;
#endif
    GetPrivateProfileStringW(L"license", L"key", L"", code, 600, g_ini);
    g_pro = code[0] && licenseValid(code);
}

// Free tier: these tweaks and addons always work. Everything else needs Pro.
static const int freeTweaks[] = {1, 2, 3, 10, 11, 13, 16};   // Game Mode, Game DVR, Game Bar, Mouse accel, Sticky keys, Background apps, Fast keyboard
static int tweakLocked(int t) {
    if (g_pro) return 0;
    for (int i = 0; i < (int)(sizeof freeTweaks / sizeof freeTweaks[0]); i++) if (freeTweaks[i] == t) return 0;
    return 1;
}
static int itemLocked(int li, int i) {
    if (g_pro) return 0;
    if (li == 0) return tweakLocked(i);
    if (li == 1) return 1;            // Fortnite profiles
    return i == 1 || i == 2;          // Shader Cache Cleaner, Background App Closer
}

#ifdef OWNER_BUILD
// ---- owner tools: make a Pro code for a customer's PC ID. Needs nero_private_key.bin next to this exe. ----
static WCHAR g_ownerCode[200];
static void b32enc(const UCHAR *in, int n, WCHAR *out) {
    static const char A[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    unsigned buf = 0; int bits = 0, o = 0, c = 0;
    for (int i = 0; i < n; i++) {
        buf = (buf << 8) | in[i]; bits += 8;
        while (bits >= 5) { bits -= 5; if (c && c % 8 == 0) out[o++] = L'-'; out[o++] = (WCHAR)A[(buf >> bits) & 31]; c++; }
    }
    if (bits) { if (c && c % 8 == 0) out[o++] = L'-'; out[o++] = (WCHAR)A[(buf << (5 - bits)) & 31]; }
    out[o] = 0;
}
static int ownerMakeCode(const WCHAR *idw) {
    char id[64]; int n = 0;
    for (const WCHAR *p = idw; *p && n < 60; p++) if (!iswspace(*p)) id[n++] = (char)towupper(*p);
    id[n] = 0;
    int valid = strlen(id) == 24 && strncmp(id, "NERO-", 5) == 0;
    for (int i = 5; valid && i < 24; i++) valid = ((i - 5) % 5 == 4) ? id[i] == '-' : isxdigit((unsigned char)id[i]) != 0;
    if (!valid) { wcscpy(g_status, L"That doesn't look like a PC ID. It should look like NERO-XXXX-XXXX-XXXX-XXXX."); g_ownerCode[0] = 0; return 0; }
    WCHAR path[MAX_PATH]; GetModuleFileNameW(NULL, path, MAX_PATH);
    WCHAR *sl = wcsrchr(path, L'\\'); if (sl) *sl = 0;
    wcscat(path, L"\\nero_private_key.bin");
    UCHAR blob[104]; DWORD got = 0;
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE || !ReadFile(f, blob, sizeof blob, &got, NULL) || got != sizeof blob) {
        if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
        wcscpy(g_status, L"Put nero_private_key.bin in the same folder as this exe, then try again."); g_ownerCode[0] = 0; return 0;
    }
    CloseHandle(f);
    char msg[80]; snprintf(msg, sizeof msg, "NERO1|%s", id);
    UCHAR h[32], sig[64]; ULONG sn = 0; int ok = 0;
    BCRYPT_ALG_HANDLE alg = NULL; BCRYPT_KEY_HANDLE key = NULL;
    if (sha256(msg, (ULONG)strlen(msg), h) && BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0) == 0) {
        if (BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &key, blob, sizeof blob, 0) == 0) {
            ok = BCryptSignHash(key, NULL, h, 32, sig, 64, &sn, 0) == 0 && sn == 64;
            BCryptDestroyKey(key);
        }
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    if (!ok) { wcscpy(g_status, L"The key file isn't valid, so no code was made."); g_ownerCode[0] = 0; return 0; }
    b32enc(sig, 64, g_ownerCode);
    wcscpy(g_status, L"Code made. Click Copy code and send it to the customer.");
    return 1;
}
#endif
