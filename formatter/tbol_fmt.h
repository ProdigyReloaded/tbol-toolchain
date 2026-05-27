/*
 * tbol_fmt.h — TBOL source code formatter
 *
 * Text-based formatting passes. Safe for user-written source with comments.
 * Used by tboldc, tbol-lsp, and standalone tbolfmt.
 */
#ifndef TBOL_FMT_H
#define TBOL_FMT_H

#include <stdbool.h>

typedef struct {
    int indent_width;        /* Spaces per indent level (default 4; 0 = don't reindent) */
    bool strip_labels;       /* Remove unreferenced label definitions */
    bool cuddle_else;        /* END; ELSE on same line */
    bool hex_literals;       /* Convert '\xNN\xNN...' to 0xNNNN... */
} TbolFmtOptions;

/* Return default options with all passes enabled. */
TbolFmtOptions tbol_fmt_defaults(void);

/*
 * Format TBOL source text.
 * Returns a malloc'd string with the formatted result.
 * Caller must free(). Returns NULL on allocation failure.
 * The input source is not modified.
 */
char *tbol_fmt(const char *source, const TbolFmtOptions *opts);

#endif
