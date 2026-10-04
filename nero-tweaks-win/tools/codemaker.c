// Nero Code Maker: turns a customer's PC ID into their one-time Pro code.
// Needs nero_private_key.bin in the same folder as this exe. Keep that file secret.
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int sha256(const void *d, ULONG n, UCHAR out[32]) {
    BCRYPT_ALG_HANDLE a; int ok = 0;
    if (BCryptOpenAlgorithmProvider(&a, BCRYPT_SHA256_ALGORITHM, NULL, 0) == 0) {
        ok = BCryptHash(a, NULL, 0, (PUCHAR)d, n, out, 32) == 0;
        BCryptCloseAlgorithmProvider(a, 0);
    }
    return ok;
}
static int validId(const char *s) {
    if (strlen(s) != 24 || strncmp(s, "NERO-", 5)) return 0;
    for (int i = 5; i < 24; i++) {
        if ((i - 5) % 5 == 4) { if (s[i] != '-') return 0; }
        else if (!isxdigit((unsigned char)s[i])) return 0;
    }
    return 1;
}
static void b32(const UCHAR *in, int n, char *out) {
    static const char A[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    unsigned buf = 0; int bits = 0, o = 0, c = 0;
    for (int i = 0; i < n; i++) {
        buf = (buf << 8) | in[i]; bits += 8;
        while (bits >= 5) { bits -= 5; if (c && c % 8 == 0) out[o++] = '-'; out[o++] = A[(buf >> bits) & 31]; c++; }
    }
    if (bits) { if (c && c % 8 == 0) out[o++] = '-'; out[o++] = A[(buf << (5 - bits)) & 31]; }
    out[o] = 0;
}
static void clip(const char *t) {
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();
    HGLOBAL m = GlobalAlloc(GMEM_MOVEABLE, strlen(t) + 1);
    if (m) { memcpy(GlobalLock(m), t, strlen(t) + 1); GlobalUnlock(m); SetClipboardData(CF_TEXT, m); }
    CloseClipboard();
}
int main(void) {
    char path[MAX_PATH]; GetModuleFileNameA(NULL, path, MAX_PATH);
    char *sl = strrchr(path, '\\'); if (sl) *sl = 0;
    strcat(path, "\\nero_private_key.bin");
    UCHAR blob[104]; DWORD got = 0;
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE || !ReadFile(f, blob, sizeof blob, &got, NULL) || got != sizeof blob) {
        printf("Could not find nero_private_key.bin next to this program.\nPut it in the same folder as NeroCodeMaker.exe.\n\nPress Enter to close."); getchar(); return 1;
    }
    CloseHandle(f);
    BCRYPT_ALG_HANDLE alg; BCRYPT_KEY_HANDLE key;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0) != 0 ||
        BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &key, blob, sizeof blob, 0) != 0) {
        printf("The key file is not valid.\n\nPress Enter to close."); getchar(); return 1;
    }
    printf("=== Nero Code Maker ===\n");
    for (;;) {
        char line[200];
        printf("\nPaste the customer's PC ID (or press Enter to quit): ");
        if (!fgets(line, sizeof line, stdin)) break;
        char id[64]; int n = 0;
        for (char *p = line; *p && n < 60; p++) if (!isspace((unsigned char)*p)) id[n++] = (char)toupper((unsigned char)*p);
        id[n] = 0;
        if (!n) break;
        if (!validId(id)) { printf("That doesn't look like a PC ID. It should look like NERO-XXXX-XXXX-XXXX-XXXX.\n"); continue; }
        char msg[80]; snprintf(msg, sizeof msg, "NERO1|%s", id);
        UCHAR h[32], sig[64]; ULONG sn = 0;
        if (!sha256(msg, (ULONG)strlen(msg), h) || BCryptSignHash(key, NULL, h, 32, sig, 64, &sn, 0) != 0 || sn != 64) { printf("Signing failed.\n"); continue; }
        char code[200]; b32(sig, 64, code);
        printf("\nPRO CODE for %s:\n\n%s\n\n(Copied to your clipboard - just paste it to the customer.)\n", id, code);
        clip(code);
    }
    return 0;
}
