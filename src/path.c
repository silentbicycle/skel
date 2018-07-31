#include "path.h"

/* In order, check:
 *   ${NAME}
 *   ${SKEL_CLOSET:-~/.dem_bones}/${NAME}
 *   ${SKEL_SYSTEM_CLOSET:-/usr/local/share/skel}/${NAME}
 * */
FILE *path_open_skel_file(config *cfg, const char *name) {
    if (0 == strcmp("-", name)) { return stdin; }

    static char pathbuf[PATH_MAX];
    char *home = getenv("HOME");
    FILE *f = NULL;
    if (cfg->skel_path == NULL) { cfg->skel_path = getenv("SKEL_CLOSET"); }
    if (cfg->skel_path == NULL) { cfg->skel_path = DEF_HOME_PATH; }

    f = fopen(name, "r");
    #define TRY_PATH(fmt, ...) \
        if (PATH_MAX <= snprintf(pathbuf, PATH_MAX, fmt, __VA_ARGS__)) {\
            return NULL;                                                \
        }                                                               \
        if (0) { printf("trying path '%s'...\n", pathbuf); }            \
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
