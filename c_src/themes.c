#include "themes.h"
#include <string.h>
#include <strings.h>

#define RST "\033[0m"
#define BOLD "\033[1m"

/* ANSI 256/truecolor escapes matching the JS chalk colors */

static const Theme themes[] = {
    {
        .name = "default",
        .title     = BOLD "\033[36m",       /* bold cyan */
        .separator = "\033[90m",            /* gray */
        .label     = BOLD "\033[34m",       /* bold blue */
        .ascii     = "\033[36m",            /* cyan */
        .reset     = RST
    },
    {
        .name = "dracula",
        .title     = BOLD "\033[38;2;189;147;249m",   /* #bd93f9 */
        .separator = "\033[38;2;98;114;164m",          /* #6272a4 */
        .label     = BOLD "\033[38;2;139;233;253m",    /* #8be9fd */
        .ascii     = "\033[38;2;189;147;249m",         /* #bd93f9 */
        .reset     = RST
    },
    {
        .name = "nord",
        .title     = BOLD "\033[38;2;136;192;208m",   /* #88c0d0 */
        .separator = "\033[38;2;76;86;106m",           /* #4c566a */
        .label     = BOLD "\033[38;2;129;161;193m",    /* #81a1c1 */
        .ascii     = "\033[38;2;136;192;208m",         /* #88c0d0 */
        .reset     = RST
    },
    {
        .name = "gruvbox",
        .title     = BOLD "\033[38;2;254;128;25m",    /* #fe8019 */
        .separator = "\033[38;2;168;153;132m",         /* #a89984 */
        .label     = BOLD "\033[38;2;250;189;47m",     /* #fabd2f */
        .ascii     = "\033[38;2;254;128;25m",          /* #fe8019 */
        .reset     = RST
    },
    {
        .name = "catppuccin",
        .title     = BOLD "\033[38;2;203;166;247m",   /* #cba6f7 */
        .separator = "\033[38;2;108;112;134m",         /* #6c7086 */
        .label     = BOLD "\033[38;2;137;180;250m",    /* #89b4fa */
        .ascii     = "\033[38;2;203;166;247m",         /* #cba6f7 */
        .reset     = RST
    }
};

#define THEME_COUNT (int)(sizeof(themes) / sizeof(themes[0]))

const Theme *get_theme(const char *name) {
    if (!name || !name[0]) return &themes[0];
    for (int i = 0; i < THEME_COUNT; i++) {
        if (strcasecmp(name, themes[i].name) == 0)
            return &themes[i];
    }
    return &themes[0]; /* default */
}
