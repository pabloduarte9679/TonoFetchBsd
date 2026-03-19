#ifndef CONFIG_H
#define CONFIG_H

#define MAX_CONFIG_MODULES 32
#define MAX_MODULE_NAME    32

typedef struct {
    char theme[32];
    char logo[256];
    char modules[MAX_CONFIG_MODULES][MAX_MODULE_NAME];
    int module_count;
    int minimal;
    int json;
} Config;

/* Load config with defaults. Then overlay from config file if it exists. */
void config_load_defaults(Config *cfg);

/* Load from ~/.config/tonofetch/config.json, overlaying onto cfg. */
void config_load_file(Config *cfg);

/* Parse CLI args, overlaying onto cfg. */
void config_parse_args(Config *cfg, int argc, char **argv);

#endif /* CONFIG_H */
