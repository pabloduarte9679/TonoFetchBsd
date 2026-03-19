#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

int read_file(const char *path, char *buf, size_t maxlen) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    size_t total = 0;
    while (total < maxlen - 1) {
        ssize_t n = read(fd, buf + total, maxlen - 1 - total);
        if (n <= 0) break;
        total += (size_t)n;
    }
    close(fd);
    buf[total] = '\0';
    return (int)total;
}

int read_file_small(const char *path, char *buf, size_t maxlen) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, buf, maxlen - 1);
    close(fd);
    if (n < 0) { buf[0] = '\0'; return -1; }
    buf[n] = '\0';
    /* Trim trailing newline */
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) {
        buf[--n] = '\0';
    }
    return (int)n;
}

int count_dir_entries(const char *dirpath, const char *suffix) {
    DIR *d = opendir(dirpath);
    if (!d) return -1;
    int count = 0;
    size_t slen = suffix ? strlen(suffix) : 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (suffix) {
            size_t nlen = strlen(ent->d_name);
            if (nlen >= slen && strcmp(ent->d_name + nlen - slen, suffix) == 0)
                count++;
        } else {
            count++;
        }
    }
    closedir(d);
    return count;
}

char *trim(char *str) {
    if (!str) return str;
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

int extract_key_value(const char *data, const char *key, char *out, size_t maxlen) {
    char search[256];
    snprintf(search, sizeof(search), "%s=", key);

    const char *pos = strstr(data, search);
    if (!pos) return -1;

    pos += strlen(search);

    int quoted = 0;
    if (*pos == '"') {
        quoted = 1;
        pos++;
    }

    size_t i = 0;
    while (*pos && i < maxlen - 1) {
        if (quoted && *pos == '"') break;
        if (!quoted && (*pos == '\n' || *pos == '\r')) break;
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return 0;
}

int safe_snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    if (size > 0) buf[size - 1] = '\0';
    return ret;
}
