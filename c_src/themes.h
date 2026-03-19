#ifndef THEMES_H
#define THEMES_H

typedef struct {
    const char *name;
    const char *title;      /* ANSI escape for title text */
    const char *separator;  /* ANSI escape for separator */
    const char *label;      /* ANSI escape for labels */
    const char *ascii;      /* ANSI escape for ASCII art */
    const char *reset;      /* Reset escape */
} Theme;

/* Get a theme by name. Returns default if not found. */
const Theme *get_theme(const char *name);

#endif /* THEMES_H */
