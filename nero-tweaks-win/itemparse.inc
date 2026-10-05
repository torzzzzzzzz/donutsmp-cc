// Reads the Fortnite item list JSON (fortnite-api.com cosmetics search) without a JSON library.
typedef struct { WCHAR id[64], name[64]; } ItemRes;
static char *jfind(char *s, const char *key) { return s ? strstr(s, key) : NULL; }
static int jstr(const char *p, WCHAR *out, int max) { // p points just after the opening quote
    char tmp[512]; int n = 0;
    while (*p && *p != '"' && n < 500) {
        if (*p == '\\' && p[1] == 'u' && isxdigit((unsigned char)p[2]) && isxdigit((unsigned char)p[3]) && isxdigit((unsigned char)p[4]) && isxdigit((unsigned char)p[5])) {
            char h[5] = {p[2], p[3], p[4], p[5], 0}; unsigned c = (unsigned)strtoul(h, NULL, 16); p += 6;
            if (c < 0x80) tmp[n++] = (char)c;
            else if (c < 0x800) { tmp[n++] = (char)(0xC0 | (c >> 6)); tmp[n++] = (char)(0x80 | (c & 0x3F)); }
            else { tmp[n++] = (char)(0xE0 | (c >> 12)); tmp[n++] = (char)(0x80 | ((c >> 6) & 0x3F)); tmp[n++] = (char)(0x80 | (c & 0x3F)); }
            continue;
        }
        if (*p == '\\' && p[1]) p++;
        tmp[n++] = *p++;
    }
    tmp[n] = 0;
    return MultiByteToWideChar(CP_UTF8, 0, tmp, -1, out, max) > 0;
}
static int parseItems(char *buf, ItemRes *out, WCHAR (*iconUrl)[300], int max) {
    int n = 0; char *pos = buf;
    while (n < max) {
        char *id = jfind(pos, "\"id\":\""); if (!id) break;
        char *nx = jfind(id + 6, "\"id\":\"");
        char *nm = jfind(id, "\"name\":\""), *ic = jfind(id, "\"smallIcon\":\"");
        pos = nx ? nx : id + 6;
        if (!nm || !ic || (nx && (nm > nx || ic > nx))) continue;
        jstr(id + 6, out[n].id, 64); jstr(nm + 8, out[n].name, 64); jstr(ic + 13, iconUrl[n], 300);
        if (out[n].id[0] && iconUrl[n][0]) n++;
    }
    return n;
}
