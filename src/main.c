/* 
 * Copyright (c) 2012-18 Scott Vokes <vokes.s@gmail.com>
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

#include "types.h"
#include "path.h"
#include "sub.h"

static void usage(void);
static void substitute_template(struct config *cfg);
static void handle_args(struct config *cfg, int *argc, char ***argv);
static void read_defaults(struct config *cfg);

/* 0.3.0 */
#define SKEL_VERSION_MAJOR 0
#define SKEL_VERSION_MINOR 3
#define SKEL_VERSION_PATCH 0

static void usage(void) {
    fprintf(stderr,
        "skel %u.%u.%u by Scott Vokes <vokes.s@gmail.com>\n"
        "usage: \n"
        "  env FOO=\"definition\" \\\n"
        "      BAR=\"other definition\" \\\n"
        "    skel [-h] [-o OPENER] [-c CLOSER] [-d FILE]\n"
        "         [-p PATH] [-x] [-e] [TEMPLATE] [OUT]\n"
        "\n"
        " -h:        help\n"
        " -o OPENER: set substitution open pattern (default \"" DEF_OPEN_PATTERN "\")\n"
        " -c CLOSER: set substitution close pattern (default \"" DEF_CLOSE_PATTERN "\")\n"
        " -x:        exec expansions with 'x' attribute and insert result.\n"
        "            Example: `echo '#{:xn:date +%%Y}' | skel -x` => \"2015\".\n"
        "            (Disabled by default to avoid unexpected commands.)\n"
        " -d FILE:   set defaults file (a file w/ a list of \"KEY rest_of_line\" pairs)\n"
        " -p PATH:   path to skeleton files (closet)\n"
        " -e:        treat undefined variable as an error\n",
        SKEL_VERSION_MAJOR, SKEL_VERSION_MINOR, SKEL_VERSION_PATCH);
    exit(1);
}

static void check_for_colon(const char *optarg, const char *name) {
    if (strchr(optarg, ':') != NULL) {
        fprintf(stderr, "%s cannot contain ':'.\n", name);
        exit(1);
    }
}

static void handle_args(struct config *cfg, int *argc, char ***argv) {
    int f = 0;
    while ((f = getopt(*argc, *argv, "ho:c:d:p:ex")) != -1) {
        switch (f) {
        case 'h':               /* help */
            usage();
            break;
        case 'o':               /* set substitution opener */
            check_for_colon(optarg, "Opener");
            cfg->sub_open = optarg;
            break;
        case 'c':               /* set substitution closer */
            check_for_colon(optarg, "Closer");
            cfg->sub_close = optarg;
            break;
        case 'd':               /* load defaults file */
            cfg->defaults_file = optarg;
            break;
        case 'p':               /* path to closet */
            cfg->skel_path = optarg;
            break;
        case 'e':               /* error on NULL substitution */
            cfg->abort_on_undef = true;
            break;
        case 'x':               /* execute patterns */
            cfg->exec_patterns = true;
            break;
        case '?':
        default:
            usage();
            break;
        }
    }
    *argc -= optind;
    *argv += optind;
}

int main(int argc, char **argv) {
    struct config cfg = {
        .sub_open = DEF_OPEN_PATTERN,
        .sub_close = DEF_CLOSE_PATTERN,
        .escape = '\\',
    };

    cfg.defaults_file = getenv("SKEL_DEFAULTS");
    handle_args(&cfg, &argc, &argv);

    char *skel_path = (argc > 0 ? argv[0] : "-");
    cfg.template = path_open_skel_file(&cfg, skel_path);
    if (NULL == cfg.template) { err(1, "skeleton file '%s'", argv[0]); }

    if (argc > 1) {
        cfg.out = fopen(argv[1], "w");
        if (cfg.out == NULL) {
            err(1, "fopen: %s", argv[1]);
        }
    } else {
        cfg.out = stdout;
    }

    if (cfg.defaults_file) { read_defaults(&cfg); }
    substitute_template(&cfg);
    return 0;
}

static void read_defaults(struct config *cfg) {
    static char buf[LINE_BUF_SZ];
    char *line = NULL;
    FILE *df = fopen(cfg->defaults_file, "r");
    char *key = NULL, *value = NULL;
    if (df == NULL) { err(1, "defaults file '%s'", cfg->defaults_file); }
    while ((line = fgets(buf, sizeof(buf), df))) {
        if (line[0] == '#') { continue; }
        if ((key = strtok(line, " \t\n"))) {
            if ((value = strtok(NULL, "\n"))) {
                if (setenv(key, value, 0) < 0) { err(1, "setenv '%s'", line); }
            }
        }
    }
}

/* Read the template from the file stream, line by line,
 * and print it (with variable substitutions). */
static void substitute_template(struct config *cfg) {
    static char tbuf[LINE_BUF_SZ];
    char *line = NULL;
    while ((line = fgets(tbuf, sizeof(tbuf), cfg->template))) {
        sub_and_print_line(cfg, line);
    }
}
