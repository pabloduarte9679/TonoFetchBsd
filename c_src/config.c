#include "config.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Default enabled modules — same as JS version */
static const char *default_modules[] = {
    "os", "host", "kernel", "uptime", "packages", "shell",
    "resolution", "de", "wm", "wmtheme", "theme", "icons",
    "font", "cursor", "terminal", "cpu", "gpu", "ram",
    "swap", "disk", "localip", "battery", "locale"
};
#define DEFAULT_MODULE_COUNT (int)(sizeof(default_modules) / sizeof(default_modules[0]))

void config_load_defaults(Config *cfg) {
    memset(cfg, 0, sizeof(Config));
    strncpy(cfg->theme, "default", sizeof(cfg->theme));
    strncpy(cfg->logo, "tonofetch", sizeof(cfg->logo));
    cfg->minimal = 0;
    cfg->json = 0;
    cfg->module_count = DEFAULT_MODULE_COUNT;
    for (int i = 0; i < DEFAULT_MODULE_COUNT; i++) {
        strncpy(cfg->modules[i], default_modules[i], MAX_MODULE_NAME - 1);
    }
}

/* Minimal JSON parser — extracts known keys from flat JSON */
static void json_extract_string(const char *json, const char *key, char *out, size_t maxlen) {
    /* Search for "key": "value" */
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return;
    p += strlen(search);
    /* Skip whitespace and colon */
    while (*p && (*p == ' ' || *p == ':' || *p == '\t')) p++;
    if (*p != '"') return;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < maxlen - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
}

static int json_extract_bool(const char *json, const char *key) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return -1;
    p += strlen(search);
    while (*p && (*p == ' ' || *p == ':' || *p == '\t')) p++;
    if (strncmp(p, "true", 4) == 0) return 1;
    if (strncmp(p, "false", 5) == 0) return 0;
    return -1;
}

static int json_extract_string_array(const char *json, const char *key,
                                     char out[][MAX_MODULE_NAME], int max_count) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return 0;
    p += strlen(search);
    while (*p && *p != '[') p++;
    if (*p != '[') return 0;
    p++;

    int count = 0;
    while (*p && *p != ']' && count < max_count) {
        while (*p && *p != '"' && *p != ']') p++;
        if (*p != '"') break;
        p++;
        size_t i = 0;
        while (*p && *p != '"' && i < MAX_MODULE_NAME - 1) {
            out[count][i++] = *p++;
        }
        out[count][i] = '\0';
        if (*p == '"') p++;
        count++;
    }
    return count;
}

void config_load_file(Config *cfg) {
    const char *home = getenv("HOME");
    if (!home) return;

    char path[512];
    snprintf(path, sizeof(path), "%s/.config/tonofetch/config.json", home);

    char buf[4096];
    if (read_file(path, buf, sizeof(buf)) <= 0) return;

    /* Extract fields */
    char theme[32] = {0};
    json_extract_string(buf, "theme", theme, sizeof(theme));
    if (theme[0]) strncpy(cfg->theme, theme, sizeof(cfg->theme));

    char logo[256] = {0};
    json_extract_string(buf, "logo", logo, sizeof(logo));
    if (logo[0]) strncpy(cfg->logo, logo, sizeof(cfg->logo));

    int minimal = json_extract_bool(buf, "minimal");
    if (minimal >= 0) cfg->minimal = minimal;

    int json_mode = json_extract_bool(buf, "json");
    if (json_mode >= 0) cfg->json = json_mode;

    char modules[MAX_CONFIG_MODULES][MAX_MODULE_NAME];
    memset(modules, 0, sizeof(modules));
    int n = json_extract_string_array(buf, "modules", modules, MAX_CONFIG_MODULES);
    if (n > 0) {
        cfg->module_count = n;
        memcpy(cfg->modules, modules, sizeof(modules));
    }
}

static void print_help(void) {
    printf("\n"
        "TonoFetch - Custom CLI System Info Tool\n"
        "\n"
        "Usage: tonofetch [options]\n"
        "\n"
        "Options:\n"
        "  --minimal       Show only essential system info\n"
        "  --json          Export info in JSON format\n"
        "  --theme <name>  Set color theme (default, dracula, nord, gruvbox, catppuccin)\n"
        "  --logo <name>   Set ASCII logo (ubuntu, tux, tonofetch) or path to a file\n"
        "  --help          Show this help message\n"
        "\n"
        "Configuration:\n"
        "  You can create a config file at ~/.config/tonofetch/config.json\n"
        "  Example content:\n"
        "  {\n"
        "    \"theme\": \"dracula\",\n"
        "    \"logo\": \"tux\",\n"
        "    \"modules\": [\"os\", \"kernel\", \"uptime\", \"shell\", \"cpu\", \"ram\"],\n"
        "    \"minimal\": false\n"
        "  }\n"
        "\n");
    exit(0);
}

void config_parse_args(Config *cfg, int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_help();
        } else if (strcmp(argv[i], "--minimal") == 0) {
            cfg->minimal = 1;
        } else if (strcmp(argv[i], "--json") == 0) {
            cfg->json = 1;
        } else if (strcmp(argv[i], "--theme") == 0 && i + 1 < argc) {
            strncpy(cfg->theme, argv[++i], sizeof(cfg->theme) - 1);
        } else if (strcmp(argv[i], "--logo") == 0 && i + 1 < argc) {
            strncpy(cfg->logo, argv[++i], sizeof(cfg->logo) - 1);
        }
    }
}
