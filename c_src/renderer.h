#ifndef RENDERER_H
#define RENDERER_H

#include "modules.h"
#include "ascii.h"
#include "themes.h"

/* Render the full output: ASCII art side-by-side with system info */
void render_output(const Logo *logo, const ModuleResult *results, int result_count, const Theme *theme);

#endif /* RENDERER_H */
