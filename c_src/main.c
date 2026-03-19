#include <stdio.h>
#include <string.h>
#include "config.h"
#include "modules.h"
#include "ascii.h"
#include "themes.h"
#include "renderer.h"

static void print_json(const ModuleResult *results, int count) {
    char header[256];
    get_header(header, sizeof(header));

    printf("{\n");
    printf("  \"header\": \"%s\",\n", header);
    printf("  \"info\": [\n");
    for (int i = 0; i < count; i++) {
        printf("    {\n");
        printf("      \"id\": \"%s\",\n", results[i].id);
        printf("      \"label\": \"%s\",\n", results[i].label);
        printf("      \"value\": \"%s\"\n", results[i].value);
        printf("    }%s\n", (i < count - 1) ? "," : "");
    }
    printf("  ]\n");
    printf("}\n");
}

int main(int argc, char **argv) {
    /* Load configuration: defaults → file → CLI args */
    Config cfg;
    config_load_defaults(&cfg);
    config_load_file(&cfg);
    config_parse_args(&cfg, argc, argv);

    /* Override modules if minimal mode */
    if (cfg.minimal) {
        const char *minimal_mods[] = {"os", "kernel", "cpu", "ram"};
        cfg.module_count = 4;
        for (int i = 0; i < 4; i++) {
            strncpy(cfg.modules[i], minimal_mods[i], MAX_MODULE_NAME - 1);
        }
    }

    /* Collect system info */
    const char *enabled[MAX_CONFIG_MODULES];
    for (int i = 0; i < cfg.module_count; i++) {
        enabled[i] = cfg.modules[i];
    }

    ModuleResult results[MAX_MODULES];
    int count = collect_info(enabled, cfg.module_count, results, MAX_MODULES);

    /* JSON output mode */
    if (cfg.json) {
        print_json(results, count);
        return 0;
    }

    /* Visual output */
    const Logo *logo = get_logo(cfg.logo);
    const Theme *theme = get_theme(cfg.theme);

    render_output(logo, results, count, theme);

    return 0;
}
