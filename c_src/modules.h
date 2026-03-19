#ifndef MODULES_H
#define MODULES_H

#include <stddef.h>

#define MAX_LABEL  128
#define MAX_VALUE  256
#define MAX_MODULES 23

typedef struct {
    char id[32];
    char label[MAX_LABEL];
    char value[MAX_VALUE];
} ModuleResult;

/* Individual module getters — return 0 on success, -1 to skip */
int mod_os(char *label, size_t lsz, char *value, size_t vsz);
int mod_host(char *label, size_t lsz, char *value, size_t vsz);
int mod_kernel(char *label, size_t lsz, char *value, size_t vsz);
int mod_uptime(char *label, size_t lsz, char *value, size_t vsz);
int mod_packages(char *label, size_t lsz, char *value, size_t vsz);
int mod_shell(char *label, size_t lsz, char *value, size_t vsz);
int mod_resolution(char *label, size_t lsz, char *value, size_t vsz);
int mod_de(char *label, size_t lsz, char *value, size_t vsz);
int mod_wm(char *label, size_t lsz, char *value, size_t vsz);
int mod_wmtheme(char *label, size_t lsz, char *value, size_t vsz);
int mod_theme(char *label, size_t lsz, char *value, size_t vsz);
int mod_icons(char *label, size_t lsz, char *value, size_t vsz);
int mod_font(char *label, size_t lsz, char *value, size_t vsz);
int mod_cursor(char *label, size_t lsz, char *value, size_t vsz);
int mod_terminal(char *label, size_t lsz, char *value, size_t vsz);
int mod_cpu(char *label, size_t lsz, char *value, size_t vsz);
int mod_gpu(char *label, size_t lsz, char *value, size_t vsz);
int mod_ram(char *label, size_t lsz, char *value, size_t vsz);
int mod_swap(char *label, size_t lsz, char *value, size_t vsz);
int mod_disk(char *label, size_t lsz, char *value, size_t vsz);
int mod_localip(char *label, size_t lsz, char *value, size_t vsz);
int mod_battery(char *label, size_t lsz, char *value, size_t vsz);
int mod_locale(char *label, size_t lsz, char *value, size_t vsz);

/* Module registry entry */
typedef int (*ModuleFunc)(char *label, size_t lsz, char *value, size_t vsz);

typedef struct {
    const char *id;
    ModuleFunc func;
} ModuleEntry;

/* Get the full module registry. Returns count. */
int get_all_modules(const ModuleEntry **out);

/* Collect info for enabled module ids. Returns number of results filled. */
int collect_info(const char *enabled[], int enabled_count, ModuleResult *results, int max_results);

/* Build header string: user@hostname */
void get_header(char *buf, size_t sz);

#endif /* MODULES_H */
