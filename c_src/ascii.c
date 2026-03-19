#include "ascii.h"
#include "util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const Logo logo_tux = {
    .lines = {
        "   .--.   ",
        "  |o_o |  ",
        "  |:_/ |  ",
        " //   \\ \\ ",
        "(|     | )",
        " /'\\_   _/`\\",
        " \\___)=(___/"
    },
    .count = 7
};

static const Logo logo_ubuntu = {
    .lines = {
        "            .-/+oossssoo+/-.               ",
        "        `:+ssssssssssssssssss+:`           ",
        "      -+ssssssssssssssssssyyssss+-         ",
        "    .ossssssssssssssssssdMMMNysssso.       ",
        "   /ssssssssssshdmmNNmmyNMMMMhssssss/      ",
        "  +ssssssssshmydMMMMMMMNddddyssssssss+     ",
        " /sssssssshNMMMyhhyyyyhmNMMMNhssssssss/    ",
        ".ssssssssdMMMNhsssssssssshNMMMdssssssss.   ",
        "+sssshhhyNMMNyssssssssssssyNMMMysssssss+   ",
        "ossyNMMMNyMMhsssssssssssssshmmmhssssssso   ",
        "ossyNMMMNyMMhsssssssssssssshmmmhssssssso   ",
        "+sssshhhyNMMNyssssssssssssyNMMMysssssss+   ",
        ".ssssssssdMMMNhsssssssssshNMMMdssssssss.   ",
        " /sssssssshNMMMyhhyyyyhdNMMMNhssssssss/    ",
        "  +sssssssssdmydMMMMMMMMddddyssssssss+     ",
        "   /ssssssssssshdmmNNmmyNMMMMhssssss/      ",
        "    .ossssssssssssssssssdMMMNysssso.       ",
        "      -+sssssssssssssssssyyyssss+-         ",
        "        `:+ssssssssssssssssss+:`           ",
        "            .-/+oossssoo+/-.               "
    },
    .count = 20
};

static const Logo logo_tonofetch = {
    .lines = {
        "            .-/+oossssoo+/-.               ",
        "        `:+ssssssssssssssssss+:`           ",
        "      -+ssssssssssssssssssssssss+-         ",
        "    .osssssdMMMMMMMMMMMMMMMMMMdssso.       ",
        "   /ssssssshMMMMMMMMMMMMMMMMMMhsssss/      ",
        "  +ssssssssyNMMMMMMMMMMMMMMMMNyssssss+     ",
        " /ssssssssssshhhhhNMMMMNhhhhhsssssssss/    ",
        ".sssssssssssssssssNMMMMNsssssssssssssss.   ",
        "+sssssssssssssssssNMMMMNsssssssssssssss+   ",
        "osssssssssssssssssNMMMMNssssssssssssssso   ",
        "osssssssssssssssssNMMMMNssssssssssssssso   ",
        "+sssssssssssssssssNMMMMNsssssssssssssss+   ",
        ".sssssssssssssssssNMMMMNsssssssssssssss.   ",
        " /ssssssssssssssssNMMMMNssssssssssssss/    ",
        "  +ssssssssssssssshMMMMhsssssssssssss+     ",
        "   /ssssssssssssssyNNNNyssssssssssss/      ",
        "    .osssssssssssssssssssssssssssso.       ",
        "      -+ssssssssssssssssssssssss+-         ",
        "        `:+ssssssssssssssssss+:`           ",
        "            .-/+oossssoo+/-.               "
    },
    .count = 20
};

/* For loading from file */
static Logo file_logo;
static char file_logo_buf[4096];
static char *file_logo_lines[MAX_LOGO_LINES];

const Logo *get_logo(const char *name_or_path) {
    if (!name_or_path || !name_or_path[0])
        return &logo_tonofetch;

    if (strcasecmp(name_or_path, "tux") == 0)       return &logo_tux;
    if (strcasecmp(name_or_path, "ubuntu") == 0)     return &logo_ubuntu;
    if (strcasecmp(name_or_path, "tonofetch") == 0)  return &logo_tonofetch;

    /* Try to load from file */
    if (read_file(name_or_path, file_logo_buf, sizeof(file_logo_buf)) > 0) {
        int count = 0;
        char *p = file_logo_buf;
        while (*p && count < MAX_LOGO_LINES) {
            file_logo_lines[count] = p;
            char *nl = strchr(p, '\n');
            if (nl) {
                *nl = '\0';
                p = nl + 1;
            } else {
                p += strlen(p);
            }
            file_logo.lines[count] = file_logo_lines[count];
            count++;
        }
        file_logo.count = count;
        return &file_logo;
    }

    return &logo_tonofetch;
}
