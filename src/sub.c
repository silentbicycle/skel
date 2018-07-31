#include <ctype.h>

#include "sub.h"

#define LOG 0

enum attribute {
    ATTR_LOWERCASE = 0x01,      /* 'l' */
    ATTR_UPPERCASE = 0x02,      /* 'u' */
    ATTR_EXECUTE = 0x04,        /* 'x' */
    ATTR_NO_NEWLINE = 0x08,     /* 'n' */

    ATTR_HAS_DEFAULT = 0x10,
    ATTR_CASE_MASK = 0x03,
    ATTR_NONE = 0x00,
};

enum sub_mode {
    MODE_VERBATIM,              /* pass normal input through */
    MODE_ESCAPE,                /* may be escaping opener */
    MODE_ATTR_CHECK,            /* check for expansion attributes */
    MODE_ATTRIBUTES,            /* save attributes */
    MODE_VAR,                   /* read variable name / command */
    MODE_DEFAULT,               /* read default, if any */
    MODE_EXPAND,                /* expand variable / command */
};

struct buffer {
    char *buf;
    size_t i;
    size_t ceil;
};

static void print_env_var(struct config *cfg, char *varname,
    enum attribute attrs, char *default_buf);
static void execute_pattern(char *cmd, enum attribute attrs);
static bool process_attribute_char(char c, enum attribute *attr);
static void apply_attributes(char *buf, enum attribute attrs);

static void append(struct buffer *b, char c);
static void flush(struct buffer *b);
static void discard(struct buffer *b, size_t len);
static void clear(struct buffer *b);

void sub_and_print_line(struct config *cfg, const char *line) {
    char var_buf[VAR_BUF_SZ];
    size_t input_i = 0;
    size_t sub_i = 0;           /* substitution marker offset */
    size_t var_i = 0;           /* variable name offset */
    enum sub_mode mode = MODE_VERBATIM;
    char c = '\0';
    enum attribute attrs = ATTR_NONE;
    struct buffer buf = { .ceil = 0, };

    memset(var_buf, 0, sizeof(var_buf));
    
    const char *sub_open = cfg->sub_open;
    const char *sub_close = cfg->sub_close;
    const size_t sub_open_len = strlen(sub_open);
    const size_t sub_close_len = strlen(sub_close);

    c = line[input_i];

    while (c) {
        #if LOG
            fprintf(stderr, "%zd: mode %d, got '%c', buf_i %zd, sub_i %zd, var_i %zd\n",
                input_i, mode, c, buf_i, sub_i, var_i);
        #endif

        switch (mode) {
        case MODE_VERBATIM:
            if (c == sub_open[sub_i]) {
                sub_i++;
                if (sub_i == sub_open_len) {
                    mode = MODE_ATTR_CHECK;
                    sub_i = 0;
                    discard(&buf, sub_open_len - 1);
                    flush(&buf);
                } else {
                    append(&buf, c);
                }
            } else if (c == '\\') {
                mode = MODE_ESCAPE;
                break;
            } else {
                sub_i = 0;
                append(&buf, c);
            }
            break;

        case MODE_ESCAPE:
            if (c != sub_open[sub_i]) { /* only escape opener */
                append(&buf, '\\');
            }
            append(&buf, c);
            mode = MODE_VERBATIM;
            break;

        case MODE_ATTR_CHECK:
            if (c == ':') {
                mode = MODE_ATTRIBUTES;
            } else {
                var_i = 0;
                var_buf[var_i] = c;
                var_i++;
                if (var_i == VAR_BUF_SZ) {
                    fprintf(stderr, "Variable name too long\n");
                    exit(1);
                }
                sub_i = 0;
                mode = MODE_VAR;
            }
            break;

        case MODE_ATTRIBUTES:
            if (!process_attribute_char(c, &attrs)) {
                var_i = 0;
                mode = MODE_VAR;
            }
            break;

        case MODE_VAR:
            if (c == ':' && !(attrs & ATTR_EXECUTE)) {
                clear(&buf);
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
                    discard(&buf, sub_close_len - 1);
                    append(&buf, '\0');
                    mode = MODE_EXPAND;
                } else {
                    append(&buf, c);
                }
            } else {
                append(&buf, c);
            }
            break;

        case MODE_EXPAND:
            var_buf[var_i] = '\0';
            if (attrs & ATTR_EXECUTE) {
                if (cfg->exec_patterns) {
                    execute_pattern(var_buf, attrs);
                } else {
                    fprintf(stderr, "Execute attribute not enabled, check and run with -x flag.\n");
                }
            } else {
                if (buf.i > 0) { attrs |= ATTR_HAS_DEFAULT; }
                print_env_var(cfg, var_buf, attrs, buf.buf);
            }
            var_i = 0;
            clear(&buf);
            sub_i = 0;
            input_i--;                /* re-process this char */
            mode = MODE_VERBATIM;
            break;
        }

        input_i++;
        c = line[input_i];
    }

    switch (mode) {
    case MODE_ESCAPE:
        append(&buf, '\\');
        /* fall through */
    case MODE_VERBATIM:
        flush(&buf);
        break;
    default:
        fprintf(stderr, "Warning: End of stream in expansion.\n");
        break;
    }

    if (buf.buf != NULL) { free(buf.buf); }
}

static void append(struct buffer *b, char c) {
    if (b->i == b->ceil) {
        size_t nceil = (b->ceil == 0 ? DEF_BUF_SZ : 2*b->ceil);
        char *nbuf = realloc(b->buf, nceil);
        assert(nbuf != NULL);
        b->buf = nbuf;
        b->ceil = nceil;
    }
    b->buf[b->i] = c;
    b->i++;
}

static void flush(struct buffer *b) {
    append(b, '\0');
    printf("%s", b->buf);
    clear(b);
}

static void discard(struct buffer *b, size_t nlen) {
    b->i -= nlen;
}

static void clear(struct buffer *b) {
    b->i = 0;
}

static void print_env_var(struct config *cfg, char *varname,
        enum attribute attrs, char *default_buf) {
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

static void execute_pattern(char *cmd, enum attribute attrs) {
    FILE *child = popen(cmd, "r");
    if (child == NULL) {
        fprintf(stderr, "popen failure for command '%s'\n", cmd);
        exit(2);
    } else {
        char prev[LINE_BUF_SZ];
        char buf[LINE_BUF_SZ];
        prev[0] = '\0';
        buf[sizeof(buf) - 1] = '\0';
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

static bool process_attribute_char(char c, enum attribute *attrs) {
    switch (c) {
    case 'l':
        *attrs = (*attrs & ~ATTR_CASE_MASK) | ATTR_LOWERCASE;
        break;
    case 'u':
        *attrs = (*attrs & ~ATTR_CASE_MASK) | ATTR_UPPERCASE;
        break;
    case 'n':
        *attrs |= ATTR_NO_NEWLINE;
        break;
    case 'x':
        *attrs |= ATTR_EXECUTE;
        break;
    default:
        fprintf(stderr, "Bad attribute: '%c'\n", c);
        break;
    case ':':
        return false;
    }
    return true;
}

static void apply_attributes(char *buf, enum attribute attrs) {
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
