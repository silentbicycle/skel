#include "path.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static void check_for_directory(const char *path);

/* In order, check:
 *   ${NAME}
 *   ${SKEL_CLOSET:-~/.dem_bones}/${NAME}
 *   ${SKEL_SYSTEM_CLOSET:-/usr/local/share/skel}/${NAME}
 * */
FILE *path_open_skel_file(struct config *cfg, const char *name) {
    if (0 == strcmp("-", name)) { return stdin; }

    static char pathbuf[PATH_MAX];
    char *home = getenv("HOME");
    FILE *f = NULL;
    if (cfg->skel_path == NULL) { cfg->skel_path = getenv("SKEL_CLOSET"); }
    if (cfg->skel_path == NULL) { cfg->skel_path = DEF_HOME_PATH; }

    f = fopen(name, "r");

    #define TRY_PATH(fmt, ...)                                          \
        if (PATH_MAX <= snprintf(pathbuf, PATH_MAX, fmt, __VA_ARGS__)) {\
            return NULL;                                                \
        }                                                               \
        check_for_directory(pathbuf);                                   \
        f = fopen(pathbuf, "r")

    /* not found => try user's closet */
    if (f == NULL) {
        if (cfg->skel_path[0] == '~' && cfg->skel_path[1] == '/') {
            TRY_PATH("%s/%s/%s", home, cfg->skel_path + 2, name);
        } else {
            TRY_PATH("%s/%s", cfg->skel_path, name);
        }
    }

    /* still not found => try global closet */
    if (f == NULL) {
        cfg->skel_path = getenv("SKEL_SYSTEM_CLOSET");
        if (cfg->skel_path == NULL) { cfg->skel_path = DEF_SYSTEM_PATH; }
        TRY_PATH("%s/%s", cfg->skel_path, name);
    }

    /* not found => fail */
    if (f == NULL) { return NULL; }

    errno = 0;                  /* suppress intemediate failures */
    return f;
    #undef TRY_PATH
}

static void check_for_directory(const char *path) {
    struct stat st;
    if (-1 == stat(path, &st)) {
        if (errno == ENOENT) {
            errno = 0;
            return;
        } else {
            err(1, "stat: %s\n", path);
        }
    } else {
        if (S_ISDIR(st.st_mode)) {
            char cmd[] = "ls -1 ";
            char cmd_buf[PATH_MAX + strlen(cmd)];
            if (sizeof(cmd_buf) < (size_t)snprintf(cmd_buf, sizeof(cmd_buf),
                    "%s%s", cmd, path)) {
                exit(1);
            }
            fprintf(stderr, "%s is a directory, with:\n", path);
            if (-1 == system(cmd_buf)) { err(1, "system"); }
            exit(1);
        }
    }
}
