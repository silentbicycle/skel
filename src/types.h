#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

#define LINE_BUF_SZ 4096
#define DEF_BUF_SZ 256
#define VAR_BUF_SZ 256

#define DEF_OPEN_PATTERN "#{"
#define DEF_CLOSE_PATTERN "}"

#ifndef DEF_HOME_PATH
#define DEF_HOME_PATH "~/.dem_bones"
#endif

#ifndef DEF_SYSTEM_PATH
#define DEF_SYSTEM_PATH "/usr/local/share/skel"
#endif

struct config {
    FILE *template;
    FILE *out;
    char *skel_path;
    char escape;
    char *sub_open;
    char *sub_close;
    char *defaults_file;
    bool abort_on_undef;
    bool exec_patterns;
};

#endif
