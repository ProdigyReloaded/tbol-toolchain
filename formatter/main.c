/*
 * tbolfmt - TBOL source code formatter
 *
 * Usage: tbolfmt [options] [file]
 *   Reads from file or stdin, writes formatted source to stdout.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "tbol_fmt.h"

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] [file]\n", prog);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -w <N>           Indent width (default: 4, 0 = don't reindent)\n");
    fprintf(stderr, "  --no-labels      Don't strip unreferenced labels\n");
    fprintf(stderr, "  --no-cuddle      Don't cuddle ELSE\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
    fprintf(stderr, "\nReads stdin if no file specified.\n");
}

static char *read_all(FILE *f) {
    size_t cap = 8192, len = 0;
    char *buf = malloc(cap);
    size_t n;
    while ((n = fread(buf + len, 1, cap - len - 1, f)) > 0) {
        len += n;
        if (len + 1 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
    }
    buf[len] = '\0';
    return buf;
}

int main(int argc, char **argv) {
#ifdef _WIN32
    /* Emit raw LF so output matches reference files and pipelines that
     * expect POSIX line endings; avoids Windows stdout CRLF translation. */
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    TbolFmtOptions opts = tbol_fmt_defaults();
    const char *input_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            opts.indent_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--no-labels") == 0) {
            opts.strip_labels = false;
        } else if (strcmp(argv[i], "--no-cuddle") == 0) {
            opts.cuddle_else = false;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    FILE *in = stdin;
    if (input_file) {
        in = fopen(input_file, "r");
        if (!in) {
            fprintf(stderr, "Error: cannot open '%s'\n", input_file);
            return 1;
        }
    }

    char *source = read_all(in);
    if (in != stdin) fclose(in);

    char *formatted = tbol_fmt(source, &opts);
    free(source);

    if (formatted) {
        fputs(formatted, stdout);
        free(formatted);
    }

    return 0;
}
