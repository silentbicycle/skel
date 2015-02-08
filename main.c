/* 
 * Copyright (c) 2012-15 Scott Vokes <vokes.s@gmail.com>
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
static void substitute_template(config *cfg);
static void handle_args(config *cfg, int *argc, char ***argv);
static void read_defaults(config *cfg);

#define SKEL_VERSION_MAJOR 0
#define SKEL_VERSION_MINOR 1
#define SKEL_VERSION_PATCH 2
/* 0.1.2 +EXEC_CHAR => 0.2.0 */

static void usage(void) {
    fprintf(stderr,
        "skel %u.%u.%u by Scott Vokes <vokes.s@gmail.com>\n"
        "usage: \n"
        "  env FOO=\"definition\" \\\n"
        "    BAR=\"other definition\" \\\n"
        "    skel [-h] [-o OPENER] [-c CLOSER] [-d FILE]\n"
        "         [-p PATH]  [-x EXEC] [-e] [TEMPLATE]\n"
        "\n"
        " -h:        help\n"
        " -o OPENER: set substitution open pattern (default \"" DEF_OPEN_PATTERN "\")\n"
        " -c CLOSER: set substitution close pattern (default \"" DEF_CLOSE_PATTERN "\")\n"
        " -x EXEC    exec patterns starting with EXEC char and insert result.\n"
        "            If doubled, trailing newlines will be stripped.\n"
        "            Example: -x %% '#{%%%%date +%%Y}' => '2015'.\n"
        " -d FILE:   set defaults file (a file w/ a list of \"KEY rest_of_line\" pairs)\n"
        " -p PATH:   path to skeleton files (closet)\n"
        " -e:        treat undefined variable as an error\n",
        SKEL_VERSION_MAJOR, SKEL_VERSION_MINOR, SKEL_VERSION_PATCH);
    exit(1);
}

static void handle_args(config *cfg, int *argc, char ***argv) {
    int f = 0;
    while ((f = getopt(*argc, *argv, "ho:c:d:p:ex")) != -1) {
        switch (f) {
        case 'h':               /* help */
            usage();
        case 'o':               /* set substitution opener */
            cfg->sub_open = optarg;
            break;
        case 'c':               /* set substitution closer */
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
        }
    }
    *argc -= optind;
    *argv += optind;
}

int main(int argc, char **argv) {
    config cfg = {
        .sub_open = DEF_OPEN_PATTERN,
        .sub_close = DEF_CLOSE_PATTERN,
        .escape = '\\',
    };

    cfg.defaults_file = getenv("SKEL_DEFAULTS");
    handle_args(&cfg, &argc, &argv);

    char *skel_path = (argc > 0 ? argv[0] : "-");
    cfg.template = path_open_skel_file(&cfg, skel_path);
    if (NULL == cfg.template) { err(1, "skeleton file '%s'", argv[0]); }

    if (cfg.defaults_file) { read_defaults(&cfg); }
    substitute_template(&cfg);
    return 0;
}

static void read_defaults(config *cfg) {
    static char buf[BUF_SZ];
    char *line = NULL;
    FILE *df = fopen(cfg->defaults_file, "r");
    char *key = NULL, *value = NULL;
    if (df == NULL) { err(1, "defaults file '%s'", cfg->defaults_file); }
    while ((line = fgets(buf, BUF_SZ, df))) {
        if (line[0] == '#') { continue; }
        if ((key = strtok(line, " \t\n"))) {
            if ((value = strtok(NULL, " \t\n"))) {
                if (setenv(key, value, 0) < 0) { err(1, "setenv '%s'", line); }
            }
        }
    }
}

/* Read the template from the file stream, line by line,
 * and print it (with variable substitutions). */
static void substitute_template(config *cfg) {
    static char buf[BUF_SZ];
    char *pbuf = NULL;
    while ((pbuf = fgets(buf, BUF_SZ, cfg->template))) {
        sub_line(cfg, pbuf);
    }
}
