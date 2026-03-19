#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* Read entire file into buf. Returns bytes read, or -1 on error. */
int read_file(const char *path, char *buf, size_t maxlen);

/* Read a small sysfs-style file (< 128 bytes) using low-level read().
   Trims trailing newline. Returns bytes read, or -1 on error. */
int read_file_small(const char *path, char *buf, size_t maxlen);

/* Count directory entries matching an optional suffix.
   If suffix is NULL, counts all non-dot entries.
   Returns count, or -1 on error. */
int count_dir_entries(const char *dirpath, const char *suffix);

/* Trim leading and trailing whitespace in-place. Returns pointer to trimmed start. */
char *trim(char *str);

/* Find the value for a key in a simple "KEY=VALUE" or KEY="VALUE" style file.
   Writes the value into out. Returns 0 on success, -1 if not found. */
int extract_key_value(const char *data, const char *key, char *out, size_t maxlen);

/* Safe snprintf wrapper that always null-terminates */
int safe_snprintf(char *buf, size_t size, const char *fmt, ...);

#endif /* UTIL_H */
