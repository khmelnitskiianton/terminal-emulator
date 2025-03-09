#ifndef UTIL_H
#define UTIL_H

void get_options(term_t *term, pty_t *pty, int argc, char **argv);
void print_help();

bool is_valid_hex_color(const char *str);

#endif