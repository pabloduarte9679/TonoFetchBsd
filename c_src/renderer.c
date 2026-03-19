#include "renderer.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* Build entire output in one buffer, then do a single fwrite.
   This avoids hundreds of printf syscalls. */

#define OUT_BUF_SIZE 16384

static int buf_append(char *buf, int pos, int max, const char *fmt, ...) __attribute__((format(printf, 4, 5)));

static int buf_append(char *buf, int pos, int max, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int written = vsnprintf(buf + pos, (size_t)(max - pos), fmt, ap);
    va_end(ap);
    if (written < 0) return pos;
    int new_pos = pos + written;
    return new_pos < max ? new_pos : max - 1;
}

void render_output(const Logo *logo, const ModuleResult *results, int result_count, const Theme *theme) {
    char buf[OUT_BUF_SIZE];
    int pos = 0;

    char header[256];
    get_header(header, sizeof(header));
    int header_len = (int)strlen(header);

    /* Separator */
    char sep[256];
    memset(sep, '-', (size_t)header_len);
    sep[header_len] = '\0';

    int right_count = 2 + result_count + 1 + 2;

    /* Find max ASCII art width */
    int max_ascii_width = 0;
    for (int i = 0; i < logo->count; i++) {
        int len = (int)strlen(logo->lines[i]);
        if (len > max_ascii_width) max_ascii_width = len;
    }

    int total_lines = logo->count > right_count ? logo->count : right_count;

    /* Color block strings */
    static const char *colors_row1[] = {
        "\033[40m", "\033[41m", "\033[42m", "\033[43m",
        "\033[44m", "\033[45m", "\033[46m", "\033[47m"
    };
    static const char *colors_row2[] = {
        "\033[100m", "\033[101m", "\033[102m", "\033[103m",
        "\033[104m", "\033[105m", "\033[106m", "\033[107m"
    };

    for (int i = 0; i < total_lines; i++) {
        /* ASCII art line */
        if (i < logo->count) {
            pos = buf_append(buf, pos, OUT_BUF_SIZE, "%s%-*s%s",
                             theme->ascii, max_ascii_width, logo->lines[i], theme->reset);
        } else {
            pos = buf_append(buf, pos, OUT_BUF_SIZE, "%-*s", max_ascii_width, "");
        }

        pos = buf_append(buf, pos, OUT_BUF_SIZE, "  ");

        /* Right-side info */
        if (i == 0) {
            pos = buf_append(buf, pos, OUT_BUF_SIZE, "%s%s%s",
                             theme->title, header, theme->reset);
        } else if (i == 1) {
            pos = buf_append(buf, pos, OUT_BUF_SIZE, "%s%s%s",
                             theme->separator, sep, theme->reset);
        } else if (i >= 2 && i < 2 + result_count) {
            int idx = i - 2;
            pos = buf_append(buf, pos, OUT_BUF_SIZE, "%s%s%s: %s",
                             theme->label, results[idx].label, theme->reset, results[idx].value);
        } else if (i == 2 + result_count) {
            /* blank line */
        } else if (i == 2 + result_count + 1) {
            for (int c = 0; c < 8; c++)
                pos = buf_append(buf, pos, OUT_BUF_SIZE, "%s   ", colors_row1[c]);
            pos = buf_append(buf, pos, OUT_BUF_SIZE, "\033[0m");
        } else if (i == 2 + result_count + 2) {
            for (int c = 0; c < 8; c++)
                pos = buf_append(buf, pos, OUT_BUF_SIZE, "%s   ", colors_row2[c]);
            pos = buf_append(buf, pos, OUT_BUF_SIZE, "\033[0m");
        }

        pos = buf_append(buf, pos, OUT_BUF_SIZE, "\n");
    }

    /* Single write to stdout */
    fwrite(buf, 1, (size_t)pos, stdout);
}
