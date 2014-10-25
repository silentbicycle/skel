/* 
 * Copyright (c) 2012-14 Scott Vokes <vokes.s@gmail.com>
 *  
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <limits.h>

#define SKEL_VERSION_MAJOR 0
#define SKEL_VERSION_MINOR 1
#define SKEL_VERSION_PATCH 1

#define BUF_SZ MAX_INPUT

typedef enum { MODE_VERBATIM, MODE_IN_PATTERN } sub_mode;

#define DEF_OPEN_PATTERN "#{"
#define DEF_CLOSE_PATTERN "}"

#ifndef DEF_HOME_PATH
#define DEF_HOME_PATH "~/.dem_bones"
#endif

#ifndef DEF_SYSTEM_PATH
#define DEF_SYSTEM_PATH "/usr/local/share/skel"
#endif

/* Global settings */
static FILE *template = NULL;
static char *skel_path = NULL;
static char escape = '\\';
static char *sub_open = DEF_OPEN_PATTERN;
static char *sub_close = DEF_CLOSE_PATTERN;
static char *defaults_file = NULL;
static int abort_on_undef = 0;     /* abort on undefined substitution */

static void usage() {
    fprintf(stderr,
        "skel %u.%u.%u by Scott Vokes <vokes.s@gmail.com>\n"
        "usage: \n"
        "  env FOO=\"definition\" \\\n"
        "    BAR=\"other definition\" \\\n"
        "    skel [-h] [-o OPENER] [-c CLOSER] [-d FILE] [-p PATH] [-e] [TEMPLATE]\n"
        " -h:        help\n"
        " -o OPENER: set substitution open pattern (default \"" DEF_OPEN_PATTERN "\")\n"
        " -c CLOSER: set substitution close pattern (default \"" DEF_CLOSE_PATTERN "\")\n"
        " -d FILE:   set defaults file (a file w/ a list of \"KEY rest_of_line\" pairs)\n"
        " -p PATH:   path to skeleton files (closet)\n"
        " -e:        treat undefined variable as an error\n",
        SKEL_VERSION_MAJOR, SKEL_VERSION_MINOR, SKEL_VERSION_PATCH);
    exit(1);
}

static void print_env_var(char *varname) {
    char *var = NULL;
    var = getenv(varname);
    if (var == NULL) {
        if (abort_on_undef) {
            fprintf(stderr, "Undefined variable: '%s'\n", varname);
            exit(2);
        } else {
            printf("%s%s%s", sub_open, varname, sub_close);
        }
    } else {
        printf("%s", var);
    }
}

static void sub_line(const char *line) {
    static char buf[BUF_SZ], var_buf[BUF_SZ];
    int i = 0, sub_i = 0, buf_i = 0, var_i = 0;
    sub_mode mode = MODE_VERBATIM;
    char c = '\0';
    
    while ((c = line[i])) {
        if (mode == MODE_VERBATIM) {
            /* This could be factored better... */
            if (c == escape) {
                buf[buf_i++] = line[++i];
            } else if (c == sub_open[0]) {     /* check for 'sub_open' match */
                for (sub_i = 0; sub_open[sub_i] == line[i + sub_i]; sub_i++) {
                    ;
                }

                if (sub_open[sub_i] == '\0') { /* reached end of pattern */
                    i += sub_i - 1;
                    mode = MODE_IN_PATTERN;
                    buf[buf_i] = '\0';
                    printf("%s", buf);
                    buf_i = sub_i = 0;
                } else {                       /* not a match */
                    memcpy(buf + buf_i, line + i, sub_i);
                    i += sub_i - 1;
                    buf_i += sub_i;
                    sub_i = 0;
                }
            } else {
                buf[buf_i++] = c;
                if (buf_i == BUF_SZ - 1) {     /* full; flush. */
                    buf[buf_i] = '\0';
                    printf("%s", buf);
                    buf_i = 0;
                }
                if (c == '\n') break;   /* c == '\0' is broken by the while */
            }
        } else if (mode == MODE_IN_PATTERN) {
            if (c == sub_close[0]) { /* check for 'sub_close' match */
                for (sub_i = 1; sub_close[sub_i] == line[i + sub_i]; sub_i++) {
                    ;
                }
                if (sub_close[sub_i] == '\0') { /* matched */
                    i += sub_i - 1;
                    var_buf[var_i] = '\0';
                    print_env_var(var_buf);
                    var_i = sub_i = 0;
                    mode = MODE_VERBATIM;
                } else {                       /* match fail */
                    memcpy(var_buf + var_i, line + i, sub_i + 1);
                    i += sub_i + 1;
                    buf_i += sub_i + 1;
                    sub_i = 0;
                }
            } else {
                var_buf[var_i++] = c;
            }
        }
        i++;
    }

    if (mode == MODE_IN_PATTERN) {
        fprintf(stderr, "End of file in pattern.\n");
        exit(1);
    }

    /* Print remaining. */
    if (buf_i > 0) { buf[buf_i] = '\0'; printf("%s", buf); }
}

/* Read the template from the file stream, line by line,
 * and print it (with variable substitutions). */
static void sub_template() {
    static char buf[BUF_SZ];
    char *pbuf = NULL;
    while ((pbuf = fgets(buf, BUF_SZ, template))) {
        sub_line(pbuf);
    }
}

static void handle_args(int *argc, char ***argv) {
    int f = 0;
    while ((f = getopt(*argc, *argv, "ho:c:d:p:e")) != -1) {
        switch (f) {
        case 'h':               /* help */
            usage();
        case 'o':               /* set substitution opener */
            sub_open = optarg;
            break;
        case 'c':               /* set substitution closer */
            sub_close = optarg;
            break;
        case 'd':               /* load defaults file */
            defaults_file = optarg;
            break;
        case 'p':               /* path to closet */
            skel_path = optarg;
            break;
        case 'e':               /* error on NULL substitution */
            abort_on_undef = 1;
            break;
        case '?':
        default:
            usage();
        }
    }
    *argc -= optind;
    *argv += optind;
}

/* In order, check:
 *   ${NAME}
 *   ${SKEL_CLOSET:-~/.dem_bones}/${NAME}
 *   ${SKEL_SYSTEM_CLOSET:-/usr/local/share/skel}/${NAME}
 * */
static FILE *open_skel_file(const char *name) {
    static char pathbuf[PATH_MAX];
    char *home = getenv("HOME");
    FILE *f = NULL;
    if (skel_path == NULL) skel_path = getenv("SKEL_CLOSET");
    if (skel_path == NULL) skel_path = DEF_HOME_PATH;

    f = fopen(name, "r");
    #define TRY_PATH(fmt, ...) \
        if (PATH_MAX <= snprintf(pathbuf, PATH_MAX, fmt, __VA_ARGS__)) {\
            return NULL;                                                \
        }                                                               \
        if (0) { printf("trying path '%s'...\n", pathbuf); }            \
        f = fopen(pathbuf, "r")

    /* not found => try user's closet */
    if (f == NULL) {
        if (skel_path[0] == '~' && skel_path[1] == '/') {
            TRY_PATH("%s/%s/%s", home, skel_path + 2, name);
        } else {
            TRY_PATH("%s/%s", skel_path, name);
        }
    }

    /* still not found => try global closet */
    if (f == NULL) {
        skel_path = getenv("SKEL_SYSTEM_CLOSET");
        if (skel_path == NULL) skel_path = DEF_SYSTEM_PATH;
        TRY_PATH("%s/%s", skel_path, name);
    }

    /* not found => fail */
    if (f == NULL) return NULL;

    errno = 0;                  /* suppress intemediate failures */
    return f;
    #undef TRY_PATH
}

static void read_defaults() {
    static char buf[BUF_SZ];
    char *line = NULL;
    FILE *df = fopen(defaults_file, "r");
    char *key = NULL, *value = NULL;
    if (df == NULL) err(1, "defaults file '%s'", defaults_file);
    while ((line = fgets(buf, BUF_SZ, df))) {
        if (line[0] == '#') continue;
        if ((key = strtok(line, " \t\n"))) {
            if ((value = strtok(NULL, " \t\n"))) {
                if (setenv(key, value, 0) < 0) err(1, "setenv '%s'", line);
            }
        }
    }
}

int main(int argc, char **argv) {
    defaults_file = getenv("SKEL_DEFAULTS");
    handle_args(&argc, &argv);

    if (argc == 0 || (argc >= 1 && 0 == strcmp("-", argv[0]))) {
        template = stdin;
    } else {
        template = open_skel_file(argv[0]);
        if (NULL == template) { err(1, "skeleton file '%s'", argv[0]); }
    }

    if (defaults_file) { read_defaults(); }
    sub_template();
    return 0;
}
