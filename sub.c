#include <ctype.h>

#include "sub.h"

typedef enum {
    ATTR_LOWERCASE = 0x01,      /* 'l' */
    ATTR_UPPERCASE = 0x02,      /* 'u' */
    ATTR_EXECUTE = 0x04,        /* 'x' */
    ATTR_NO_NEWLINE = 0x08,     /* 'n' */

    ATTR_HAS_DEFAULT = 0x10,
    ATTR_CASE_MASK = 0x03,
} attribute;

typedef enum {
    MODE_VERBATIM,
    MODE_ESCAPE,
    MODE_ATTR_CHECK,
    MODE_ATTRIBUTES,
    MODE_VAR,
    MODE_DEFAULT,
    MODE_EXPAND,
} sub_mode;

typedef uint32_t attr_t;

static void print_env_var(config *cfg, char *varname,
    attr_t attrs, char *default_buf);
static void execute_pattern(config *cfg, char *cmd, attr_t attrs);
static void apply_attributes(char *buf, attr_t attrs);

void sub_line(config *cfg, const char *line) {
    char buf[BUF_SZ];
    char var_buf[BUF_SZ];
    size_t i = 0;
    size_t sub_i = 0;
    size_t buf_i = 0;
    size_t var_i = 0;
    sub_mode mode = MODE_VERBATIM;
    char c = '\0';
    attr_t attrs = 0;

    memset(buf, 0, sizeof(buf));
    memset(var_buf, 0, sizeof(var_buf));
    
    const char *sub_open = cfg->sub_open;
    const char *sub_close = cfg->sub_close;
    int sub_open_len = strlen(sub_open);
    int sub_close_len = strlen(sub_close);

    c = line[i];

    while (c) {
        if (0) {
            fprintf(stderr, "%zd: mode %d, got '%c', buf_i %zd, sub_i %zd, var_i %zd\n",
                i, mode, c, buf_i, sub_i, var_i);
        }

        switch (mode) {
        case MODE_VERBATIM:
            if (c == sub_open[sub_i]) {
                sub_i++;
                if (sub_i == sub_open_len) {
                    mode = MODE_ATTR_CHECK;
                    sub_i = 0;
                    buf[buf_i - sub_open_len + 1] = '\0';
                    printf("%s", buf);
                    buf_i = 0;
                } else {
                    buf[buf_i] = c;
                    buf_i++;
                }
            } else if (c == '\\') {
                mode = MODE_ESCAPE;
                break;
            } else {
                sub_i = 0;
                buf[buf_i] = c;
                buf_i++;
            }
            break;

        case MODE_ESCAPE:
            if (c != sub_open[sub_i]) {
                buf[buf_i] = '\\';
                buf_i++;
            }
            buf[buf_i] = c;
            buf_i++;
            mode = MODE_VERBATIM;
            break;

        case MODE_ATTR_CHECK:
            if (c == ':') {
                mode = MODE_ATTRIBUTES;
            } else {
                var_i = 0;
                var_buf[var_i] = c;
                var_i++;
                sub_i = 0;
                mode = MODE_VAR;
            }
            break;

        case MODE_ATTRIBUTES:
            switch (c) {
            case 'l':
                attrs = (attrs & ~ATTR_CASE_MASK) | ATTR_LOWERCASE;
                break;
            case 'u':
                attrs = (attrs & ~ATTR_CASE_MASK) | ATTR_UPPERCASE;
                break;
            case 'n':
                attrs |= ATTR_NO_NEWLINE;
                break;
            case 'x':
                attrs |= ATTR_EXECUTE;
                break;
            default:
                fprintf(stderr, "Bad attribute: '%c'\n", c);
                break;
            case ':':
                var_i = 0;
                mode = MODE_VAR;
                break;
            }
            break;
        case MODE_VAR:
            if (c == ':' && !(attrs & ATTR_EXECUTE)) {
                buf_i = 0;
                mode = MODE_DEFAULT;
            } else if (c == sub_close[sub_i]) {
                sub_i++;
                if (sub_i == sub_close_len) {
                    mode = MODE_EXPAND;
                }
            } else if (c == '_' || isalnum(c) || (attrs & ATTR_EXECUTE)) {
                var_buf[var_i] = c;
                var_i++;
            } else {
                fprintf(stderr, "Invalid character in variable name: '%c'\n", c);
            }
            break;

        case MODE_DEFAULT:
            if (c == sub_close[sub_i]) {
                sub_i++;
                if (sub_i == sub_close_len) {
                    buf[buf_i - sub_close_len + 1] = '\0';
                    mode = MODE_EXPAND;
                } else {
                    buf[buf_i] = c;
                    buf_i++;
                }
            } else {
                buf[buf_i] = c;
                buf_i++;
            }
            break;

        case MODE_EXPAND:
            var_buf[var_i] = '\0';
            if (attrs & ATTR_EXECUTE) {
                if (cfg->exec_patterns) {
                    execute_pattern(cfg, var_buf, attrs);
                } else {
                    fprintf(stderr, "Execute attribute not enabled, check and run with -x flag.\n");
                }
            } else {
                if (buf_i > 0) {
                    attrs |= ATTR_HAS_DEFAULT;
                }
                print_env_var(cfg, var_buf, attrs, buf);
            }
            var_i = 0;
            buf_i = 0;
            sub_i = 0;
            i--;
            mode = MODE_VERBATIM;
            break;
        }

        i++;
        c = line[i];
    }

    switch (mode) {
    case MODE_ESCAPE:
        buf[buf_i] = '\\';
        buf_i++;
        /* fall through */
    case MODE_VERBATIM:
        /* flush output buffer */
        buf[buf_i] = '\0';
        printf("%s", buf);
        break;
    default:
        fprintf(stderr, "Warning: End of stream in expansion.\n");
        break;
    }
}

static void print_env_var(config *cfg, char *varname,
        attr_t attrs, char *default_buf) {
    char *var = NULL;
    var = getenv(varname);
    if (var == NULL) {
        if (cfg->abort_on_undef) {
            fprintf(stderr, "Undefined variable: '%s'\n", varname);
            exit(1);
        } else if (attrs & ATTR_HAS_DEFAULT) {
            apply_attributes(default_buf, attrs);
            printf("%s", default_buf);
        } else {
            printf("%s%s%s", cfg->sub_open, varname, cfg->sub_close);
        }
    } else {
        if (attrs & ATTR_NO_NEWLINE) {
            int len = strlen(var);
            if (len > 0 && var[len - 1] == '\n') {
                var[len - 1] = '\0';
            }
        }
        apply_attributes(var, attrs);
        printf("%s", var);
    }
}

static void execute_pattern(config *cfg, char *cmd, attr_t attrs) {
    FILE *child = popen(cmd, "r");
    if (child == NULL) {
        fprintf(stderr, "popen failure for command '%s'\n", cmd);
        exit(2);
    } else {
        char prev[BUF_SZ];
        char buf[BUF_SZ];
        prev[0] = '\0';
        buf[BUF_SZ - 1] = '\0';
        for (;;) {
            char *line = fgets(buf, sizeof(buf) - 1, child);
            if (line) {
                /* Buffer one line so we can drop the last line's '\n'. */
                if (prev[0] != '\0') {
                    apply_attributes(prev, attrs);
                    printf("%s", prev);
                }
                int len = strlen(line);
                memcpy(prev, buf, len);
                prev[len] = '\0';
            } else {
                break;
            }
        }

        if (prev[0] != '\0') {
            if (attrs & ATTR_NO_NEWLINE) {
                int len = strlen(prev);
                if (len > 0 && prev[len - 1] == '\n') {
                    prev[len - 1] = '\0';
                }
            }
            apply_attributes(prev, attrs);
            printf("%s", prev);
        }
        pclose(child);
    }
}

static void apply_attributes(char *buf, attr_t attrs) {
    if ((attrs & ATTR_CASE_MASK) == 0) { return; }
    size_t i = 0;
    if (attrs & ATTR_UPPERCASE) {
        while (buf[i] != '\0') {
            buf[i] = toupper(buf[i]);
            i++;
        }
    } else if (attrs & ATTR_LOWERCASE) {
        while (buf[i] != '\0') {
            buf[i] = tolower(buf[i]);
            i++;
        }
    }
}
