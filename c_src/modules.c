#include "modules.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ctype.h>
#include <dirent.h>
#include <glob.h>
#include <sys/socket.h>

/* ───────────────────────── OS ───────────────────────── */
int mod_os(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "OS");
    char buf[4096];
    if (read_file("/etc/os-release", buf, sizeof(buf)) > 0) {
        char pretty[256] = {0};
        if (extract_key_value(buf, "PRETTY_NAME", pretty, sizeof(pretty)) == 0) {
            struct utsname u;
            uname(&u);
            snprintf(value, vsz, "%s %s", pretty, u.machine);
            return 0;
        }
    }
    snprintf(value, vsz, "Linux (Unknown)");
    return 0;
}

/* ───────────────────────── Host ──────────────────────── */
int mod_host(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Host");
    char name[128] = {0}, version[128] = {0};
    int got_name = (read_file_small("/sys/devices/virtual/dmi/id/product_name", name, sizeof(name)) > 0);
    int got_ver  = (read_file_small("/sys/devices/virtual/dmi/id/product_version", version, sizeof(version)) > 0);

    if (got_name) {
        if (got_ver && version[0] && strcmp(version, "Unknown") != 0) {
            snprintf(value, vsz, "%s (%s)", name, version);
        } else {
            snprintf(value, vsz, "%s", name);
        }
    } else {
        snprintf(value, vsz, "Unknown");
    }
    return 0;
}

/* ───────────────────────── Kernel ────────────────────── */
int mod_kernel(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Kernel");
    struct utsname u;
    uname(&u);
    snprintf(value, vsz, "Linux %s", u.release);
    return 0;
}

/* ───────────────────────── Uptime ────────────────────── */
int mod_uptime(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Uptime");

    /* Read /proc/uptime directly — faster than sysinfo() */
    char buf[64];
    if (read_file_small("/proc/uptime", buf, sizeof(buf)) > 0) {
        double up_secs = atof(buf);
        long up = (long)up_secs;
        int days  = (int)(up / 86400);
        int hours = (int)((up % 86400) / 3600);
        int mins  = (int)((up % 3600) / 60);

        char parts[4][32];
        int n = 0;
        if (days > 0)  { snprintf(parts[n], 32, "%d days", days);  n++; }
        if (hours > 0) { snprintf(parts[n], 32, "%d hours", hours); n++; }
        if (mins > 0)  { snprintf(parts[n], 32, "%d mins", mins);  n++; }

        if (n == 0) {
            snprintf(value, vsz, "< 1 min");
        } else {
            value[0] = '\0';
            for (int i = 0; i < n; i++) {
                if (i > 0) strncat(value, ", ", vsz - strlen(value) - 1);
                strncat(value, parts[i], vsz - strlen(value) - 1);
            }
        }
        return 0;
    }

    snprintf(value, vsz, "Unknown");
    return 0;
}

/* ───────────────────────── Packages ──────────────────── */

/* Read a SQLite varint from data at position pos. Returns value, advances pos. */
static unsigned long long sqlite_varint(const unsigned char *data, int *pos) {
    unsigned long long result = 0;
    for (int i = 0; i < 9; i++) {
        unsigned char byte = data[(*pos)++];
        if (i < 8) {
            result = (result << 7) | (byte & 0x7f);
            if (byte < 128) return result;
        } else {
            result = (result << 8) | byte;
            return result;
        }
    }
    return result;
}

/* Count rows in a SQLite B-tree given root page number.
   Reads pages from the open file descriptor. */
static int sqlite_count_btree(int fd, int root_page, int page_size) {
    unsigned char *page = malloc((size_t)page_size);
    if (!page) return 0;

    int offset = (root_page == 1) ? 100 : 0;
    if (lseek(fd, (off_t)(root_page - 1) * page_size, SEEK_SET) < 0) { free(page); return 0; }
    if (read(fd, page, (size_t)page_size) < page_size) { free(page); return 0; }

    unsigned char page_type = page[offset];
    int cell_count = (page[offset + 3] << 8) | page[offset + 4];

    if (page_type == 0x0d) {
        /* Leaf table page — cells = rows */
        free(page);
        return cell_count;
    } else if (page_type == 0x05) {
        /* Interior table page — traverse children */
        int right_child = (page[offset + 8] << 24) | (page[offset + 9] << 16) |
                          (page[offset + 10] << 8) | page[offset + 11];
        int ptr_start = offset + 12;

        int total = 0;
        for (int i = 0; i < cell_count; i++) {
            int cptr = (page[ptr_start + i * 2] << 8) | page[ptr_start + i * 2 + 1];
            int child = (page[cptr] << 24) | (page[cptr + 1] << 16) |
                        (page[cptr + 2] << 8) | page[cptr + 3];
            total += sqlite_count_btree(fd, child, page_size);
        }
        total += sqlite_count_btree(fd, right_child, page_size);
        free(page);
        return total;
    }

    free(page);
    return 0;
}

/* Count RPM packages by parsing rpmdb.sqlite B-tree directly.
   No external dependencies needed — reads raw SQLite file format. */
static int count_rpm_packages(void) {
    int fd = open("/var/lib/rpm/rpmdb.sqlite", O_RDONLY);
    if (fd < 0) return 0;

    /* Read SQLite header: page size at offset 16 (2 bytes, big-endian) */
    unsigned char header[100];
    if (read(fd, header, 100) < 100) { close(fd); return 0; }
    int page_size = (header[16] << 8) | header[17];
    if (page_size == 1) page_size = 65536;

    /* Read page 1 to find the Packages table root page from the schema.
     * Schema is stored in the sqlite_schema table (the B-tree rooted at page 1).
     * We search leaf pages under page 1 for a record where name == 'Packages'. */
    unsigned char *page1 = malloc((size_t)page_size);
    if (!page1) { close(fd); return 0; }
    lseek(fd, 0, SEEK_SET);
    if (read(fd, page1, (size_t)page_size) < page_size) { free(page1); close(fd); return 0; }

    int pkg_root = 0;
    unsigned char schema_type = page1[100];

    /* Helper to scan a leaf page for the Packages table entry */
    #define SCAN_LEAF_FOR_PACKAGES(pdata, offset_val) do { \
        int _ncells = (pdata[(offset_val) + 3] << 8) | pdata[(offset_val) + 4]; \
        int _pstart = (offset_val) + 8; \
        for (int _ci = 0; _ci < _ncells && !pkg_root; _ci++) { \
            int _cptr = (pdata[_pstart + _ci * 2] << 8) | pdata[_pstart + _ci * 2 + 1]; \
            int _pos = _cptr; \
            sqlite_varint(pdata, &_pos); /* payload len */ \
            sqlite_varint(pdata, &_pos); /* rowid */ \
            int _hdr_start = _pos; \
            unsigned long long _hdr_size = sqlite_varint(pdata, &_pos); \
            int _hdr_end = _hdr_start + (int)_hdr_size; \
            unsigned long long _stypes[8]; \
            int _nst = 0; \
            while (_pos < _hdr_end && _nst < 8) { \
                _stypes[_nst++] = sqlite_varint(pdata, &_pos); \
            } \
            /* Skip to column 1 (name) — column 0 is type */ \
            int _dp = _hdr_end; \
            if (_nst >= 5) { \
                /* Skip column 0 (type) */ \
                if (_stypes[0] >= 13 && _stypes[0] % 2 == 1) _dp += (int)((_stypes[0] - 13) / 2); \
                else if (_stypes[0] >= 1 && _stypes[0] <= 4) _dp += (int)_stypes[0]; \
                /* Column 1 = name (text) */ \
                if (_stypes[1] >= 13 && _stypes[1] % 2 == 1) { \
                    int _slen = (int)((_stypes[1] - 13) / 2); \
                    if (_slen == 8 && memcmp(pdata + _dp, "Packages", 8) == 0) { \
                        /* Found! Skip to column 3 (rootpage) */ \
                        _dp += _slen; \
                        if (_stypes[2] >= 13 && _stypes[2] % 2 == 1) _dp += (int)((_stypes[2] - 13) / 2); \
                        else if (_stypes[2] >= 1 && _stypes[2] <= 4) _dp += (int)_stypes[2]; \
                        /* rootpage is an integer */ \
                        if (_stypes[3] == 1) pkg_root = pdata[_dp]; \
                        else if (_stypes[3] == 2) pkg_root = (pdata[_dp] << 8) | pdata[_dp + 1]; \
                        else if (_stypes[3] == 3) pkg_root = (pdata[_dp] << 16) | (pdata[_dp + 1] << 8) | pdata[_dp + 2]; \
                        else if (_stypes[3] == 4) pkg_root = (pdata[_dp] << 24) | (pdata[_dp + 1] << 16) | (pdata[_dp + 2] << 8) | pdata[_dp + 3]; \
                    } \
                } \
            } \
        } \
    } while(0)

    if (schema_type == 0x0d) {
        /* Schema fits in one leaf page */
        SCAN_LEAF_FOR_PACKAGES(page1, 100);
    } else if (schema_type == 0x05) {
        /* Schema is an interior page — traverse children */
        int ncells = (page1[103] << 8) | page1[104];
        int right_child = (page1[108] << 24) | (page1[109] << 16) | (page1[110] << 8) | page1[111];
        int child_pages[64];
        int nchildren = 0;
        for (int i = 0; i < ncells && nchildren < 63; i++) {
            int cptr = (page1[112 + i * 2] << 8) | page1[113 + i * 2];
            child_pages[nchildren++] = (page1[cptr] << 24) | (page1[cptr + 1] << 16) |
                                       (page1[cptr + 2] << 8) | page1[cptr + 3];
        }
        child_pages[nchildren++] = right_child;

        unsigned char *child = malloc((size_t)page_size);
        if (child) {
            for (int i = 0; i < nchildren && !pkg_root; i++) {
                lseek(fd, (off_t)(child_pages[i] - 1) * page_size, SEEK_SET);
                if (read(fd, child, (size_t)page_size) >= page_size && child[0] == 0x0d) {
                    SCAN_LEAF_FOR_PACKAGES(child, 0);
                }
            }
            free(child);
        }
    }
    #undef SCAN_LEAF_FOR_PACKAGES

    free(page1);

    int result = 0;
    if (pkg_root > 0) {
        result = sqlite_count_btree(fd, pkg_root, page_size);
    }
    close(fd);
    return result;
}

static int count_dpkg_packages(void) {
    return count_dir_entries("/var/lib/dpkg/info", ".list");
}

static int count_flatpak_packages(void) {
    int runtime = count_dir_entries("/var/lib/flatpak/runtime", NULL);
    int app = count_dir_entries("/var/lib/flatpak/app", NULL);
    if (runtime < 0) runtime = 0;
    if (app < 0) app = 0;
    int total = runtime + app;
    /* Also check user flatpaks */
    const char *home = getenv("HOME");
    if (home) {
        char path[512];
        snprintf(path, sizeof(path), "%s/.local/share/flatpak/app", home);
        int user_app = count_dir_entries(path, NULL);
        if (user_app > 0) total += user_app;
        snprintf(path, sizeof(path), "%s/.local/share/flatpak/runtime", home);
        int user_rt = count_dir_entries(path, NULL);
        if (user_rt > 0) total += user_rt;
    }
    return total;
}

static int count_snap_packages(void) {
    int count = count_dir_entries("/snap", NULL);
    if (count > 0) count--; /* Subtract "README" or similar */
    return count > 0 ? count : 0;
}

int mod_packages(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Packages");

    int dpkg    = count_dpkg_packages();
    int rpm     = count_rpm_packages();
    int flatpak = count_flatpak_packages();
    int snap    = count_snap_packages();

    char parts[4][64];
    int n = 0;
    if (dpkg > 0)    { snprintf(parts[n], 64, "%d (dpkg)", dpkg);       n++; }
    if (rpm > 0)     { snprintf(parts[n], 64, "%d (rpm)", rpm);         n++; }
    if (flatpak > 0) { snprintf(parts[n], 64, "%d (flatpak)", flatpak); n++; }
    if (snap > 0)    { snprintf(parts[n], 64, "%d (snap)", snap);       n++; }

    if (n == 0) {
        snprintf(value, vsz, "Unknown");
    } else {
        value[0] = '\0';
        for (int i = 0; i < n; i++) {
            if (i > 0) strncat(value, ", ", vsz - strlen(value) - 1);
            strncat(value, parts[i], vsz - strlen(value) - 1);
        }
    }
    return 0;
}

/* ───────────────────────── Shell ─────────────────────── */
int mod_shell(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Shell");
    const char *shell_path = getenv("SHELL");
    if (!shell_path || !shell_path[0]) {
        snprintf(value, vsz, "Unknown");
        return 0;
    }

    const char *shell_name = strrchr(shell_path, '/');
    shell_name = shell_name ? shell_name + 1 : shell_path;

    if (strcmp(shell_name, "bash") == 0) {
        const char *ver = getenv("BASH_VERSION");
        if (ver) {
            char v[64];
            snprintf(v, sizeof(v), "%s", ver);
            char *p = strchr(v, '(');
            if (p) *p = '\0';
            snprintf(value, vsz, "bash %s", v);
            return 0;
        }
    } else if (strcmp(shell_name, "zsh") == 0) {
        const char *ver = getenv("ZSH_VERSION");
        if (ver) { snprintf(value, vsz, "zsh %s", ver); return 0; }
    } else if (strcmp(shell_name, "fish") == 0) {
        const char *ver = getenv("FISH_VERSION");
        if (ver) { snprintf(value, vsz, "fish %s", ver); return 0; }
    }

    snprintf(value, vsz, "%s", shell_name);
    return 0;
}

/* ───────────────────────── Resolution ────────────────── */
int mod_resolution(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Display");

    /* Read from DRM sysfs — no popen needed */
    glob_t g;
    if (glob("/sys/class/drm/card*-*/modes", 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) {
            char mode[64];
            if (read_file_small(g.gl_pathv[i], mode, sizeof(mode)) > 0) {
                /* First line is the preferred mode, e.g. "1920x1200" */
                char *nl = strchr(mode, '\n');
                if (nl) *nl = '\0';
                if (mode[0] && strchr(mode, 'x')) {
                    snprintf(value, vsz, "%s", mode);
                    globfree(&g);
                    return 0;
                }
            }
        }
        globfree(&g);
    }

    snprintf(value, vsz, "Unknown");
    return 0;
}

/* ───────────────────────── DE ────────────────────────── */
int mod_de(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "DE");
    const char *de = getenv("XDG_CURRENT_DESKTOP");
    if (!de) de = getenv("DESKTOP_SESSION");
    if (!de) { snprintf(value, vsz, "Unknown"); return 0; }

    if (strcasestr(de, "kde") || strcasestr(de, "plasma")) {
        const char *ver = getenv("KDE_SESSION_VERSION");
        if (!ver) ver = getenv("PLASMA_VERSION");
        if (ver)
            snprintf(value, vsz, "KDE Plasma %s", ver);
        else
            snprintf(value, vsz, "KDE Plasma");
        return 0;
    }
    if (strcasestr(de, "gnome")) {
        snprintf(value, vsz, "GNOME");
        return 0;
    }
    snprintf(value, vsz, "%s", de);
    return 0;
}

/* ───────────────────────── WM ────────────────────────── */
int mod_wm(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "WM");
    const char *de = getenv("XDG_CURRENT_DESKTOP");
    if (!de) de = getenv("DESKTOP_SESSION");

    if (de) {
        const char *session_type = getenv("XDG_SESSION_TYPE");
        const char *suffix = "";
        if (session_type && strcmp(session_type, "wayland") == 0)
            suffix = " (Wayland)";
        else if (session_type)
            suffix = " (X11)";

        if (strcasestr(de, "plasma") || strcasestr(de, "kde")) {
            snprintf(value, vsz, "KWin%s", suffix);
            return 0;
        }
        if (strcasestr(de, "gnome"))    { snprintf(value, vsz, "Mutter");  return 0; }
        if (strcasestr(de, "xfce"))     { snprintf(value, vsz, "Xfwm4");  return 0; }
        if (strcasestr(de, "mate"))     { snprintf(value, vsz, "Marco");   return 0; }
        if (strcasestr(de, "cinnamon")) { snprintf(value, vsz, "Muffin");  return 0; }
    }

    const char *session_type = getenv("XDG_SESSION_TYPE");
    if (session_type && strcmp(session_type, "wayland") == 0) {
        snprintf(value, vsz, "Wayland Compositor");
        return 0;
    }

    const char *sd = getenv("XDG_SESSION_DESKTOP");
    snprintf(value, vsz, "%s", sd ? sd : "Unknown");
    return 0;
}

/* ───────────────────────── WM Theme ─────────────────── */
int mod_wmtheme(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "WM Theme");
    char path[512], buf[4096];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.config/kwinrc", home);
        if (read_file(path, buf, sizeof(buf)) > 0) {
            char theme_name[128] = {0};
            if (extract_key_value(buf, "theme", theme_name, sizeof(theme_name)) == 0 && theme_name[0]) {
                snprintf(value, vsz, "%s", theme_name);
                return 0;
            }
        }
    }
    snprintf(value, vsz, "Breeze");
    return 0;
}

/* ───────────────────────── Theme ─────────────────────── */
int mod_theme(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Theme");
    char path[512], buf[8192];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.config/kdeglobals", home);
        if (read_file(path, buf, sizeof(buf)) > 0) {
            char lnf[128] = {0};
            if (extract_key_value(buf, "LookAndFeelPackage", lnf, sizeof(lnf)) == 0 && lnf[0]) {
                char *last_dot = strrchr(lnf, '.');
                if (last_dot && strcmp(last_dot, ".desktop") == 0) *last_dot = '\0';
                char *name_start = strrchr(lnf, '.');
                if (name_start) name_start++; else name_start = lnf;
                if (name_start[0]) name_start[0] = (char)toupper((unsigned char)name_start[0]);
                snprintf(value, vsz, "%s [Qt/GTK]", name_start);
                return 0;
            }
        }
    }
    snprintf(value, vsz, "Breeze (Dark) [Qt], Breeze [GTK3]");
    return 0;
}

/* ───────────────────────── Icons ─────────────────────── */
int mod_icons(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Icons");
    char path[512], buf[4096];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.config/kdeglobals", home);
        if (read_file(path, buf, sizeof(buf)) > 0) {
            char theme[128] = {0};
            if (extract_key_value(buf, "Theme", theme, sizeof(theme)) == 0 && theme[0]) {
                snprintf(value, vsz, "%s [Qt], %s [GTK3/4]", theme, theme);
                return 0;
            }
        }
    }
    snprintf(value, vsz, "breeze-dark [Qt], breeze-dark [GTK3/4]");
    return 0;
}

/* ───────────────────────── Font ──────────────────────── */
int mod_font(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Font");
    char path[512], buf[8192];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.config/kdeglobals", home);
        if (read_file(path, buf, sizeof(buf)) > 0) {
            char font[128] = {0};
            if (extract_key_value(buf, "font", font, sizeof(font)) == 0 && font[0]) {
                snprintf(value, vsz, "%s [Qt/GTK]", font);
                return 0;
            }
        }
    }
    snprintf(value, vsz, "Noto Sans (10pt) [Qt], Noto Sans (10pt) [GTK3/4]");
    return 0;
}

/* ───────────────────────── Cursor ────────────────────── */
int mod_cursor(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Cursor");
    const char *xcursor = getenv("XCURSOR_THEME");
    const char *size = getenv("XCURSOR_SIZE");
    if (xcursor && xcursor[0]) {
        if (size && size[0])
            snprintf(value, vsz, "%s (%spx)", xcursor, size);
        else
            snprintf(value, vsz, "%s", xcursor);
        return 0;
    }
    snprintf(value, vsz, "breeze (24px)");
    return 0;
}

/* ───────────────────────── Terminal ──────────────────── */
int mod_terminal(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Terminal");
    const char *term = getenv("TERM_PROGRAM");
    if (!term) term = getenv("COLORTERM");
    if (!term) term = getenv("TERM");
    if (!term) { snprintf(value, vsz, "Unknown"); return 0; }

    if (strcasestr(term, "konsole")) {
        const char *ver = getenv("KONSOLE_VERSION");
        if (ver && strlen(ver) >= 5) {
            int v = atoi(ver);
            snprintf(value, vsz, "konsole %d.%d.%d", v / 10000, (v % 10000) / 100, v % 100);
        } else {
            snprintf(value, vsz, "konsole");
        }
        return 0;
    }
    if (strcasestr(term, "vscode"))    { snprintf(value, vsz, "vscode");    return 0; }
    if (strcasestr(term, "kitty"))     { snprintf(value, vsz, "kitty");     return 0; }
    if (strcasestr(term, "alacritty")) { snprintf(value, vsz, "Alacritty"); return 0; }

    snprintf(value, vsz, "%s", term);
    return 0;
}

/* ───────────────────────── CPU ───────────────────────── */
int mod_cpu(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "CPU");
    char buf[32768]; /* /proc/cpuinfo can be 20KB+ on multi-core systems */
    if (read_file("/proc/cpuinfo", buf, sizeof(buf)) <= 0) {
        snprintf(value, vsz, "Unknown");
        return 0;
    }

    /* Get model name */
    char model[256] = {0};
    char *line = strstr(buf, "model name");
    if (line) {
        char *colon = strchr(line, ':');
        if (colon) {
            colon++;
            while (*colon == ' ' || *colon == '\t') colon++;
            char *eol = strchr(colon, '\n');
            size_t len = eol ? (size_t)(eol - colon) : strlen(colon);
            if (len >= sizeof(model)) len = sizeof(model) - 1;
            memcpy(model, colon, len);
            model[len] = '\0';
        }
    }

    /* Remove " w/ ..." suffix */
    char *w = strstr(model, " w/ ");
    if (w) *w = '\0';

    /* Count cores */
    int cores = 0;
    char *p = buf;
    while ((p = strstr(p, "processor")) != NULL) {
        cores++;
        p += 9;
    }

    /* Get max frequency from sysfs (fast) */
    double max_ghz = 0.0;
    char freq_buf[64];
    if (read_file_small("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", freq_buf, sizeof(freq_buf)) > 0) {
        max_ghz = atol(freq_buf) / 1000000.0;
    } else {
        /* Fallback: read from cpuinfo */
        char *mhz = strstr(buf, "cpu MHz");
        if (mhz) {
            char *colon = strchr(mhz, ':');
            if (colon) max_ghz = atof(colon + 1) / 1000.0;
        }
    }

    if (model[0]) {
        snprintf(value, vsz, "%s (%d) @ %.2f GHz", model, cores, max_ghz);
    } else {
        snprintf(value, vsz, "Unknown CPU (%d cores)", cores);
    }
    return 0;
}

/* ───────────────────────── GPU ───────────────────────── */
/* Lookup PCI vendor:device name from /usr/share/hwdata/pci.ids */
static int lookup_pci_name(const char *vendor_id, const char *device_id, char *out, size_t outsz) {
    char buf[262144]; /* pci.ids can be ~1MB, read a good chunk */
    int n = read_file("/usr/share/hwdata/pci.ids", buf, sizeof(buf));
    if (n <= 0) {
        /* Try alternate location */
        n = read_file("/usr/share/misc/pci.ids", buf, sizeof(buf));
        if (n <= 0) return -1;
    }

    /* Find vendor line: must start at column 0 as "XXXX  Vendor Name" */
    char vsearch[16];
    snprintf(vsearch, sizeof(vsearch), "\n%s  ", vendor_id);
    char *vline = NULL;
    /* Check if file starts with this vendor */
    if (strncmp(buf, vendor_id, strlen(vendor_id)) == 0 && buf[strlen(vendor_id)] == ' ') {
        vline = buf;
    } else {
        char *found = strstr(buf, vsearch);
        if (found) vline = found + 1; /* skip the \n */
    }
    if (!vline) return -1;

    /* Extract vendor name */
    char *vname_start = vline + strlen(vendor_id);
    while (*vname_start == ' ' || *vname_start == '\t') vname_start++;
    char *vname_end = strchr(vname_start, '\n');
    char vendor_name[256] = {0};
    if (vname_end) {
        size_t len = (size_t)(vname_end - vname_start);
        if (len >= sizeof(vendor_name)) len = sizeof(vendor_name) - 1;
        memcpy(vendor_name, vname_start, len);
    }

    /* Find device line: starts with "\tXXXX  " under this vendor section */
    char dsearch[16];
    snprintf(dsearch, sizeof(dsearch), "\t%s  ", device_id);
    char *search_start = vname_end ? vname_end : vline;
    char *dline = strstr(search_start, dsearch);
    if (dline) {
        /* Make sure we haven't crossed into another vendor section (line starting without tab) */
        char *check = search_start;
        while (check < dline) {
            if (*check == '\n' && check[1] != '\t' && check[1] != '#' && check[1] != '\n') {
                dline = NULL; /* Crossed vendor boundary */
                break;
            }
            check++;
        }
    }

    if (dline) {
        char *dname_start = dline + 1 + strlen(device_id);
        while (*dname_start == ' ' || *dname_start == '\t') dname_start++;
        char *dname_end = strchr(dname_start, '\n');
        char device_name[256] = {0};
        if (dname_end) {
            size_t len = (size_t)(dname_end - dname_start);
            if (len >= sizeof(device_name)) len = sizeof(device_name) - 1;
            memcpy(device_name, dname_start, len);
        } else {
            snprintf(device_name, sizeof(device_name), "%s", dname_start);
        }
        snprintf(out, outsz, "%s %s", vendor_name, device_name);
    } else {
        snprintf(out, outsz, "%s [%s:%s]", vendor_name, vendor_id, device_id);
    }
    return 0;
}

int mod_gpu(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "GPU");

    /* Scan PCI devices via sysfs — no popen needed */
    DIR *d = opendir("/sys/bus/pci/devices");
    if (!d) { snprintf(value, vsz, "Unknown"); return 0; }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        char class_path[512], class_buf[16];
        snprintf(class_path, sizeof(class_path), "/sys/bus/pci/devices/%s/class", ent->d_name);
        if (read_file_small(class_path, class_buf, sizeof(class_buf)) <= 0) continue;

        /* VGA compatible controller: class 0x030000, 3D: 0x030200 */
        unsigned long pci_class = strtoul(class_buf, NULL, 16);
        unsigned int base_class = (pci_class >> 16) & 0xFF;
        if (base_class != 0x03) continue; /* Not a display controller */

        /* Found a GPU — read vendor and device IDs from uevent */
        char uevent_path[512], uevent[1024];
        snprintf(uevent_path, sizeof(uevent_path), "/sys/bus/pci/devices/%s/uevent", ent->d_name);
        if (read_file(uevent_path, uevent, sizeof(uevent)) <= 0) continue;

        /* Parse PCI_ID=VENDOR:DEVICE */
        char *pci_id_line = strstr(uevent, "PCI_ID=");
        if (!pci_id_line) continue;
        pci_id_line += 7;
        char vendor_id[8] = {0}, device_id[8] = {0};
        char *colon = strchr(pci_id_line, ':');
        if (!colon) continue;
        size_t vlen = (size_t)(colon - pci_id_line);
        if (vlen > 4) vlen = 4;
        memcpy(vendor_id, pci_id_line, vlen);
        /* Lowercase for pci.ids lookup */
        for (size_t i = 0; i < vlen; i++) vendor_id[i] = (char)tolower((unsigned char)vendor_id[i]);

        char *dstart = colon + 1;
        char *dend = dstart;
        while (*dend && *dend != '\n' && *dend != ' ') dend++;
        size_t dlen = (size_t)(dend - dstart);
        if (dlen > 4) dlen = 4;
        memcpy(device_id, dstart, dlen);
        for (size_t i = 0; i < dlen; i++) device_id[i] = (char)tolower((unsigned char)device_id[i]);

        /* Look up name from pci.ids */
        if (lookup_pci_name(vendor_id, device_id, value, vsz) == 0) {
            closedir(d);
            return 0;
        }

        /* Fallback: just show IDs */
        snprintf(value, vsz, "PCI %s:%s", vendor_id, device_id);
        closedir(d);
        return 0;
    }

    closedir(d);
    snprintf(value, vsz, "Unknown");
    return 0;
}

/* ───────────────────────── RAM ───────────────────────── */
static long parse_meminfo_kb(const char *meminfo, const char *key) {
    char search[64];
    snprintf(search, sizeof(search), "%s:", key);
    const char *p = strstr(meminfo, search);
    if (!p) return 0;
    p += strlen(search);
    while (*p == ' ') p++;
    return atol(p);
}

int mod_ram(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Memory");
    char buf[4096];
    if (read_file("/proc/meminfo", buf, sizeof(buf)) > 0) {
        long total_kb = parse_meminfo_kb(buf, "MemTotal");
        long avail_kb = parse_meminfo_kb(buf, "MemAvailable");
        if (total_kb > 0 && avail_kb > 0) {
            long used_kb = total_kb - avail_kb;
            double used_gb = used_kb / 1048576.0;
            double total_gb = total_kb / 1048576.0;
            int pct = (int)((used_kb * 100) / total_kb);
            snprintf(value, vsz, "%.2f GiB / %.2f GiB (%d%%)", used_gb, total_gb, pct);
            return 0;
        }
    }
    snprintf(value, vsz, "Unknown");
    return 0;
}

/* ───────────────────────── Swap ──────────────────────── */
int mod_swap(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Swap");
    char buf[4096];
    if (read_file("/proc/meminfo", buf, sizeof(buf)) > 0) {
        long total_kb = parse_meminfo_kb(buf, "SwapTotal");
        long free_kb  = parse_meminfo_kb(buf, "SwapFree");
        if (total_kb > 0) {
            long used_kb = total_kb - free_kb;
            double used_gb = used_kb / 1048576.0;
            double total_gb = total_kb / 1048576.0;
            int pct = (int)((used_kb * 100) / total_kb);
            snprintf(value, vsz, "%.2f GiB / %.2f GiB (%d%%)", used_gb, total_gb, pct);
            return 0;
        }
    }
    snprintf(value, vsz, "Disabled");
    return 0;
}

/* ───────────────────────── Disk ──────────────────────── */
int mod_disk(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Disk (/)");

    /* Use statvfs() syscall — no popen needed */
    struct statvfs sv;
    if (statvfs("/", &sv) != 0) {
        snprintf(value, vsz, "Unknown");
        return 0;
    }

    unsigned long long total_bytes = (unsigned long long)sv.f_blocks * sv.f_frsize;
    unsigned long long free_bytes  = (unsigned long long)sv.f_bfree  * sv.f_frsize;
    unsigned long long used_bytes  = total_bytes - free_bytes;

    double used_gb  = used_bytes  / (1024.0 * 1024.0 * 1024.0);
    double total_gb = total_bytes / (1024.0 * 1024.0 * 1024.0);
    int pct = total_bytes > 0 ? (int)((used_bytes * 100) / total_bytes) : 0;

    /* Get filesystem type from /proc/mounts — no popen needed */
    char fs_type[32] = "unknown";
    char mounts[8192];
    if (read_file("/proc/mounts", mounts, sizeof(mounts)) > 0) {
        /* Find line ending with " / " (mount point = /) */
        char *line = mounts;
        while (line && *line) {
            /* Parse: device mountpoint fstype options ... */
            char dev[256], mp[256], fst[32];
            if (sscanf(line, "%255s %255s %31s", dev, mp, fst) >= 3) {
                if (strcmp(mp, "/") == 0) {
                    snprintf(fs_type, sizeof(fs_type), "%s", fst);
                    /* Don't break — last match wins (overlay mounts) */
                }
            }
            char *next = strchr(line, '\n');
            line = next ? next + 1 : NULL;
        }
    }

    snprintf(value, vsz, "%.2f GiB / %.2f GiB (%d%%) - %s", used_gb, total_gb, pct, fs_type);
    return 0;
}

/* ───────────────────────── Local IP ──────────────────── */
int mod_localip(char *label, size_t lsz, char *value, size_t vsz) {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        snprintf(label, lsz, "Local IP");
        snprintf(value, vsz, "Unknown");
        return 0;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;

        struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));

        struct sockaddr_in *nm = (struct sockaddr_in *)ifa->ifa_netmask;
        unsigned int mask = ntohl(nm->sin_addr.s_addr);
        int prefix = 0;
        while (mask & 0x80000000) { prefix++; mask <<= 1; }

        snprintf(label, lsz, "Local IP (%s)", ifa->ifa_name);
        snprintf(value, vsz, "%s/%d", ip, prefix);
        freeifaddrs(ifaddr);
        return 0;
    }

    freeifaddrs(ifaddr);
    snprintf(label, lsz, "Local IP");
    snprintf(value, vsz, "Unknown");
    return 0;
}

/* ───────────────────────── Battery ───────────────────── */
int mod_battery(char *label, size_t lsz, char *value, size_t vsz) {
    char cap[16] = {0}, status[32] = {0}, model[64] = {0};
    int has_cap = (read_file_small("/sys/class/power_supply/BAT0/capacity", cap, sizeof(cap)) > 0);
    int has_status = (read_file_small("/sys/class/power_supply/BAT0/status", status, sizeof(status)) > 0);

    if (!has_cap) {
        snprintf(label, lsz, "Battery");
        snprintf(value, vsz, "Not found");
        return -1; /* skip */
    }

    if (read_file_small("/sys/class/power_supply/BAT0/model_name", model, sizeof(model)) <= 0) {
        if (read_file_small("/sys/class/power_supply/BAT0/manufacturer", model, sizeof(model)) <= 0) {
            snprintf(model, sizeof(model), "BAT0");
        }
    }

    snprintf(label, lsz, "Battery (%s)", model);
    snprintf(value, vsz, "%s%% [%s]", cap, has_status ? status : "Unknown");
    return 0;
}

/* ───────────────────────── Locale ────────────────────── */
int mod_locale(char *label, size_t lsz, char *value, size_t vsz) {
    snprintf(label, lsz, "Locale");
    const char *lang = getenv("LANG");
    snprintf(value, vsz, "%s", lang ? lang : "en_US.UTF-8");
    return 0;
}

/* ════════════════════ Module Registry ═════════════════ */

static const ModuleEntry registry[] = {
    { "os",        mod_os },
    { "host",      mod_host },
    { "kernel",    mod_kernel },
    { "uptime",    mod_uptime },
    { "packages",  mod_packages },
    { "shell",     mod_shell },
    { "resolution",mod_resolution },
    { "de",        mod_de },
    { "wm",        mod_wm },
    { "wmtheme",   mod_wmtheme },
    { "theme",     mod_theme },
    { "icons",     mod_icons },
    { "font",      mod_font },
    { "cursor",    mod_cursor },
    { "terminal",  mod_terminal },
    { "cpu",       mod_cpu },
    { "gpu",       mod_gpu },
    { "ram",       mod_ram },
    { "swap",      mod_swap },
    { "disk",      mod_disk },
    { "localip",   mod_localip },
    { "battery",   mod_battery },
    { "locale",    mod_locale },
};

#define REGISTRY_COUNT (int)(sizeof(registry) / sizeof(registry[0]))

int get_all_modules(const ModuleEntry **out) {
    *out = registry;
    return REGISTRY_COUNT;
}

int collect_info(const char *enabled[], int enabled_count, ModuleResult *results, int max_results) {
    int count = 0;
    for (int i = 0; i < enabled_count && count < max_results; i++) {
        for (int j = 0; j < REGISTRY_COUNT; j++) {
            if (strcmp(enabled[i], registry[j].id) == 0) {
                ModuleResult *r = &results[count];
                snprintf(r->id, sizeof(r->id), "%s", registry[j].id);
                int rc = registry[j].func(r->label, sizeof(r->label), r->value, sizeof(r->value));
                if (rc == 0 && strcmp(r->value, "Unknown") != 0) {
                    count++;
                }
                break;
            }
        }
    }
    return count;
}

void get_header(char *buf, size_t sz) {
    struct passwd *pw = getpwuid(getuid());
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    snprintf(buf, sz, "%s@%s", pw ? pw->pw_name : "user", hostname);
}
