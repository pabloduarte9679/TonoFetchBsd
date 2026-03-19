#ifndef ASCII_H
#define ASCII_H

#define MAX_LOGO_LINES 25

typedef struct {
    const char *lines[MAX_LOGO_LINES];
    int count;
} Logo;

/* Get a built-in logo by name ("tux", "ubuntu", "tonofetch"), or load from file.
   Returns pointer to static or dynamic logo. */
const Logo *get_logo(const char *name_or_path);

#endif /* ASCII_H */
