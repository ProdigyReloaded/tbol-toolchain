/*
 * tbol_fmt.c - TBOL source code formatter
 *
 * Pipeline of text-based passes that transform TBOL source for readability.
 * Each pass takes a malloc'd string and returns a new malloc'd string.
 */

#include "tbol_fmt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

TbolFmtOptions tbol_fmt_defaults(void) {
    return (TbolFmtOptions){
        .indent_width = 4,
        .strip_labels = true,
        .cuddle_else = true,
        .hex_literals = true,
    };
}

/* -- Helper: growable string buffer ----------------------------------- */

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buf;

static void buf_init(Buf *b) {
    b->cap = 4096;
    b->data = malloc(b->cap);
    b->len = 0;
    b->data[0] = '\0';
}

static void buf_append(Buf *b, const char *s, size_t n) {
    if (b->len + n + 1 > b->cap) {
        while (b->len + n + 1 > b->cap) b->cap *= 2;
        b->data = realloc(b->data, b->cap);
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void buf_putc(Buf *b, char c) { buf_append(b, &c, 1); }

static char *buf_detach(Buf *b) {
    char *r = b->data;
    b->data = NULL;
    b->len = b->cap = 0;
    return r;
}

/* -- Pass 1: Strip unreferenced labels -------------------------------- */

char *pass_strip_labels(const char *src) {
    /* Collect referenced labels */
    bool referenced[65536] = {false};
    const char *p = src;
    while (*p) {
        const char *m = strstr(p, "label_");
        if (!m) break;
        const char *num = m + 6;
        if (*num < '0' || *num > '9') { p = num; continue; }
        int addr = 0;
        const char *end = num;
        while (*end >= '0' && *end <= '9') addr = addr * 10 + (*end++ - '0');
        if (*end != ':')
            referenced[addr & 0xFFFF] = true;
        p = end;
    }

    Buf out;
    buf_init(&out);
    const char *line = src;
    while (*line) {
        const char *eol = strchr(line, '\n');
        if (!eol) eol = line + strlen(line);
        size_t line_len = (eol - line) + (*eol == '\n' ? 1 : 0);

        bool skip = false;
        const char *s = line;
        while (s < eol && (*s == ' ' || *s == '\t')) s++;
        if (s + 6 < eol && strncmp(s, "label_", 6) == 0) {
            const char *n = s + 6;
            if (*n >= '0' && *n <= '9') {
                int addr = 0;
                const char *e = n;
                while (e < eol && *e >= '0' && *e <= '9') addr = addr * 10 + (*e++ - '0');
                if (*e == ':' && (e + 1 >= eol || *(e + 1) == '\n'))
                    if (!referenced[addr & 0xFFFF])
                        skip = true;
            }
        }

        if (!skip) buf_append(&out, line, line_len);
        line = *eol ? eol + 1 : eol;
    }
    return buf_detach(&out);
}

/* -- Pass 2: Cuddle ELSE ---------------------------------------------- */

/*
 * Transform:
 *     END;\n
 *     <indent>ELSE ...
 * Into:
 *     END; ELSE ...
 */
char *pass_cuddle_else(const char *src) {
    Buf out;
    buf_init(&out);

    const char *line = src;
    while (*line) {
        const char *eol = strchr(line, '\n');
        if (!eol) eol = line + strlen(line);
        size_t line_len = eol - line;

        /* Check if this line is purely "<indent>END;" */
        const char *s = line;
        while (s < eol && (*s == ' ' || *s == '\t')) s++;
        bool is_end_line = (eol - s == 4 && strncmp(s, "END;", 4) == 0);

        if (is_end_line && *eol == '\n') {
            /* Peek at next line: does it start with ELSE? */
            const char *next = eol + 1;
            const char *ns = next;
            while (*ns == ' ' || *ns == '\t') ns++;
            if (strncmp(ns, "ELSE ", 5) == 0 || strncmp(ns, "ELSE\n", 5) == 0) {
                /* Cuddle: emit "END; " then the ELSE line content (without indent) */
                buf_append(&out, line, line_len);  /* the END; with its indent */
                buf_putc(&out, ' ');
                /* Find end of the ELSE line */
                const char *neol = strchr(next, '\n');
                if (!neol) neol = next + strlen(next);
                buf_append(&out, ns, neol - ns);
                buf_putc(&out, '\n');
                line = *neol ? neol + 1 : neol;
                continue;
            }
        }

        buf_append(&out, line, line_len);
        if (*eol == '\n') buf_putc(&out, '\n');
        line = *eol ? eol + 1 : eol;
    }
    return buf_detach(&out);
}

/* -- Pass 3: Normalize indentation ------------------------------------ */

/* Case-insensitive prefix match */
static bool starts_with(const char *s, const char *kw) {
    return strncasecmp(s, kw, strlen(kw)) == 0;
}

/* True when `s` begins a top-level (column-0) construct. */
static bool is_top_level(const char *s) {
    return starts_with(s, "PROGRAM ") ||
           starts_with(s, "COPY ") ||
           starts_with(s, "DEFINE ") ||
           starts_with(s, "PROC ") ||
           starts_with(s, "END_PROC");
}

/*
 * Test whether the line ends with `THEN DO` or `ELSE DO` (with optional
 * trailing whitespace and an optional trailing semicolon-comment that
 * the formatter doesn't see today). Used to decide whether the line
 * opens a new DO block.
 */
static bool ends_with_then_do(const char *s, size_t len) {
    if (len < 7) return false;
    const char *end = s + len;
    while (end > s && (end[-1] == ' ' || end[-1] == '\t')) end--;
    if (end - s < 7) return false;
    if (strncasecmp(end - 7, "THEN DO", 7) != 0) return false;
    /* Make sure THEN is a word (preceded by space) so we don't match
     * something like FOOTHEN DO. */
    if (end - s == 7) return true;
    return end[-8] == ' ';
}

static bool ends_with_else_do(const char *s, size_t len) {
    if (len < 7) return false;
    const char *end = s + len;
    while (end > s && (end[-1] == ' ' || end[-1] == '\t')) end--;
    if (end - s < 7) return false;
    if (strncasecmp(end - 7, "ELSE DO", 7) != 0) return false;
    if (end - s == 7) return true;
    return end[-8] == ' ';
}

/* Is `s` a bare `DO` line (just `DO` with optional trailing whitespace)?  */
static bool is_bare_do(const char *s, size_t len) {
    if (len < 2) return false;
    if (strncasecmp(s, "DO", 2) != 0) return false;
    /* Anything after DO must be whitespace only. */
    for (size_t i = 2; i < len; i++)
        if (s[i] != ' ' && s[i] != '\t') return false;
    return true;
}

/*
 * Trim a count of leading bytes off a content view. Helper for matching
 * against the part of the line after a known prefix.
 */
static void skip_chars(const char **s, size_t *len, size_t n) {
    if (n > *len) n = *len;
    *s += n;
    *len -= n;
}

char *pass_indent(const char *src, int width) {
    if (width <= 0) return strdup(src);

    Buf out;
    buf_init(&out);

    /* Scope stack for PROC and DO frames. */
    int depth = 0;

    /* When the previous emitted line ends with bare THEN or ELSE (no
     * DO opener), the next non-blank line is the single-statement body
     * of that conditional and gets one additional indent. */
    int implicit_extra = 0;

    /* DATA section state. We exit DATA only when a top-level keyword
     * appears, not on blank lines or `;` (DATA can have multiple groups
     * separated by `;` and may contain blank-line separated groups). */
    bool in_data = false;

    const char *line = src;
    while (*line) {
        const char *eol = strchr(line, '\n');
        if (!eol) eol = line + strlen(line);

        const char *s = line;
        while (s < eol && (*s == ' ' || *s == '\t')) s++;
        size_t content_len = eol - s;

        /* Blank line - pass through, no state change. */
        if (content_len == 0) {
            buf_putc(&out, '\n');
            line = *eol ? eol + 1 : eol;
            continue;
        }

        /* Top-level constructs always emit at column 0 and exit DATA. */
        if (is_top_level(s)) {
            in_data = false;
            /* `END_PROC` closes the PROC frame before we emit. */
            if (starts_with(s, "END_PROC") && depth > 0) depth--;
            buf_append(&out, s, content_len);
            buf_putc(&out, '\n');
            /* `PROC name =` opens a frame for its body. */
            if (starts_with(s, "PROC ")) depth++;
            line = *eol ? eol + 1 : eol;
            continue;
        }

        /* `DATA` itself is a top-level keyword, but is_top_level treats
         * it specially: enter DATA mode, emit at col 0. */
        if (starts_with(s, "DATA") &&
            (content_len == 4 || s[4] == ' ' || s[4] == '\t')) {
            in_data = true;
            buf_append(&out, s, content_len);
            buf_putc(&out, '\n');
            line = *eol ? eol + 1 : eol;
            continue;
        }

        /* DATA section: group header (`name =`) at indent 1, items at
         * indent 2. A `;` closes the group but stays in DATA mode. */
        if (in_data) {
            const char *trim_end = s + content_len;
            while (trim_end > s && (trim_end[-1] == ' ' || trim_end[-1] == '\t'))
                trim_end--;
            bool is_group_hdr = (trim_end > s && trim_end[-1] == '=');
            int ind = is_group_hdr ? 1 : 2;
            for (int i = 0; i < ind * width; i++) buf_putc(&out, ' ');
            buf_append(&out, s, content_len);
            buf_putc(&out, '\n');
            line = *eol ? eol + 1 : eol;
            continue;
        }

        /* From here, we're in a normal (PROC body) scope.  Compute the
         * effective emit-time depth, taking closer tokens into account. */
        const char *body = s;
        size_t body_len = content_len;

        bool starts_with_end_semi = (body_len >= 4 && strncasecmp(body, "END;", 4) == 0);
        bool starts_with_cuddled_else = starts_with_end_semi &&
            ((body_len >= 9 && strncasecmp(body, "END; ELSE", 9) == 0));

        int emit_depth = depth + implicit_extra;
        if (starts_with_end_semi && emit_depth > 0) emit_depth--;

        for (int i = 0; i < emit_depth * width; i++) buf_putc(&out, ' ');
        buf_append(&out, body, body_len);
        buf_putc(&out, '\n');

        /* Apply the actual depth changes for this line. */
        if (starts_with_end_semi && depth > 0) depth--;

        /* Re-open if this line carries an opener (THEN DO / ELSE DO /
         * bare DO / cuddled-else with DO).  Only one DO can open per
         * line, so we check in order of specificity. */
        const char *open_check = body;
        size_t open_len = body_len;
        if (starts_with_cuddled_else) {
            /* Skip past "END;" so we look at the ELSE side for an opener. */
            skip_chars(&open_check, &open_len, 4);
            while (open_len > 0 && *open_check == ' ') skip_chars(&open_check, &open_len, 1);
        }

        bool opens_block = is_bare_do(open_check, open_len) ||
                           ends_with_else_do(open_check, open_len) ||
                           ends_with_then_do(open_check, open_len);
        if (opens_block) depth++;

        /* Detect bare THEN / ELSE at end of line (no DO).  The next
         * non-blank line is the single-stmt body and indents one
         * deeper.  Trim trailing whitespace AND any trailing
         * `{ comment }` so a trailing comment after THEN doesn't hide
         * the keyword from this check. */
        const char *tail = body + body_len;
        while (tail > body) {
            while (tail > body && (tail[-1] == ' ' || tail[-1] == '\t')) tail--;
            if (tail > body && tail[-1] == '}') {
                /* Walk backward to find matching '{'. */
                const char *t = tail - 2;
                while (t > body && *t != '{') t--;
                if (t >= body && *t == '{') tail = t;
                else break;
            } else break;
        }
        bool ends_then = (tail - body >= 4 &&
                          strncasecmp(tail - 4, "THEN", 4) == 0 &&
                          (tail - body == 4 || tail[-5] == ' '));
        bool ends_else = (tail - body >= 4 &&
                          strncasecmp(tail - 4, "ELSE", 4) == 0 &&
                          (tail - body == 4 || tail[-5] == ' '));

        /* The body line we just emitted consumes any implicit_extra
         * unless this line is itself the keyword line that produces a
         * new implicit_extra (in which case we were at base depth). */
        if (ends_then || ends_else) {
            implicit_extra = 1;
        } else if (!opens_block) {
            implicit_extra = 0;
        } else {
            implicit_extra = 0;
        }

        line = *eol ? eol + 1 : eol;
    }
    return buf_detach(&out);
}

/* -- Pass: Break THEN / ELSE bodies onto their own line -------------- */

/*
 * Project style: when an IF or WHILE has a single-statement body (no
 * DO/END block), the body sits on its own line under THEN.  Likewise
 * for a standalone or cuddled ELSE whose body is a single statement.
 *
 *   IF cond THEN stmt;            ->  IF cond THEN
 *                                         stmt;
 *   END; ELSE stmt;               ->  END; ELSE
 *                                         stmt;
 *
 * Lines that already split (THEN / ELSE alone at end of line), or that
 * open a DO block (THEN DO / ELSE DO), are left untouched.
 *
 * The split inserts a bare newline.  pass_indent runs later and will
 * indent the new line correctly based on the current scope depth.
 */

/*
 * Locate the keyword `kw` in `line` such that the keyword is preceded
 * by a space (or starts at `line`) and followed by a space.  Returns a
 * pointer to the first byte of the keyword or NULL.  Skips over single-
 * quoted strings and brace-delimited comments.
 */
static const char *find_keyword(const char *line, size_t len, const char *kw) {
    size_t kl = strlen(kw);
    const char *p = line;
    const char *end = line + len;
    bool in_str = false;
    int brace_depth = 0;
    while (p < end) {
        if (in_str) {
            if (*p == '\'') in_str = false;
            p++; continue;
        }
        if (brace_depth > 0) {
            if (*p == '}') brace_depth--;
            else if (*p == '{') brace_depth++;
            p++; continue;
        }
        if (*p == '\'') { in_str = true; p++; continue; }
        if (*p == '{') { brace_depth++; p++; continue; }
        if (p + kl <= end && strncasecmp(p, kw, kl) == 0) {
            bool left_ok = (p == line) || p[-1] == ' ' || p[-1] == '\t';
            bool right_ok = (p + kl == end) || p[kl] == ' ' || p[kl] == '\t';
            if (left_ok && right_ok) return p;
        }
        p++;
    }
    return NULL;
}

/*
 * After matching `kw` at `kw_pos` inside `line`, return a pointer to
 * the first non-space character of the body (what follows kw).  If the
 * body is empty (kw is the last word of the line) or begins with `DO`
 * as a word, returns NULL - the line should not be split.
 */
static const char *body_start_after_kw(const char *kw_pos, size_t kw_len,
                                       const char *line_end) {
    const char *p = kw_pos + kw_len;
    /* Skip whitespace and TBOL `{ ... }` comments.  A trailing comment
     * after THEN/ELSE doesn't constitute a body. */
    while (p < line_end) {
        if (*p == ' ' || *p == '\t') { p++; continue; }
        if (*p == '{') {
            const char *end = p + 1;
            while (end < line_end && *end != '}') end++;
            if (end < line_end) p = end + 1;
            else p = line_end;  /* unterminated comment - treat as eaten */
            continue;
        }
        break;
    }
    if (p >= line_end) return NULL;  /* nothing but comments after kw */
    /* "DO" alone or "DO " indicates a DO block - don't split. */
    if (p + 2 <= line_end &&
        strncasecmp(p, "DO", 2) == 0 &&
        (p + 2 == line_end || p[2] == ' ' || p[2] == '\t')) {
        return NULL;
    }
    return p;
}

/*
 * Find the FIRST splittable THEN or ELSE in `line`.  Returns true and
 * fills *split_after / *body if a split is needed; returns false
 * otherwise.
 */
static bool find_split_point(const char *line, size_t len,
                             const char **split_after,
                             size_t *split_after_len,
                             const char **body) {
    static const char *keywords[] = { "THEN", "ELSE" };
    const char *first_kw = NULL;
    const char *first_body = NULL;
    for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
        const char *kw = find_keyword(line, len, keywords[i]);
        if (!kw) continue;
        const char *b = body_start_after_kw(kw, 4, line + len);
        if (!b) continue;
        if (!first_kw || kw < first_kw) {
            first_kw = kw;
            first_body = b;
        }
    }
    if (!first_kw) return false;
    *split_after = first_kw;
    *split_after_len = 4;
    *body = first_body;
    return true;
}

char *pass_then_newline(const char *src) {
    Buf out;
    buf_init(&out);

    const char *line = src;
    while (*line) {
        const char *eol = strchr(line, '\n');
        if (!eol) eol = line + strlen(line);

        /* Strip trailing whitespace from the line for matching purposes. */
        const char *trim_end = eol;
        while (trim_end > line && (trim_end[-1] == ' ' || trim_end[-1] == '\t'))
            trim_end--;
        size_t content_len = trim_end - line;

        /* Iteratively split; each split shrinks the remainder.  Per
         * line we may produce up to four pieces, e.g. for
         * "IF cond THEN body1; ELSE body2;":
         *   "IF cond THEN" / "body1;" / "ELSE" / "body2;" */
        const char *cursor = line;
        size_t remaining = content_len;
        while (true) {
            const char *split_after;
            size_t split_after_len;
            const char *body;
            if (!find_split_point(cursor, remaining, &split_after,
                                  &split_after_len, &body)) {
                buf_append(&out, cursor, remaining);
                break;
            }

            /* If the keyword is ELSE and there's non-whitespace content
             * BEFORE it on this segment, that content is the THEN body
             * and belongs on its own line.  Emit it first, then ELSE. */
            bool kw_is_else = (split_after_len == 4 &&
                               strncasecmp(split_after, "ELSE", 4) == 0);
            if (kw_is_else) {
                const char *pre = cursor;
                while (pre < split_after && (*pre == ' ' || *pre == '\t')) pre++;
                /* Trim trailing whitespace from the pre-ELSE chunk. */
                const char *pre_end = split_after;
                while (pre_end > pre && (pre_end[-1] == ' ' || pre_end[-1] == '\t'))
                    pre_end--;
                /* "END;" before ELSE is cuddled-else - leave it. */
                bool is_cuddled_end = (pre_end - pre == 4 &&
                                       strncasecmp(pre, "END;", 4) == 0);
                if (pre_end > pre && !is_cuddled_end) {
                    buf_append(&out, cursor, pre - cursor);     /* original indent */
                    buf_append(&out, pre, pre_end - pre);
                    buf_putc(&out, '\n');
                    /* Step cursor up to the start of ELSE so the next
                     * append picks up just "ELSE\n" with no leading
                     * whitespace.  pass_indent will indent it. */
                    size_t consumed = split_after - cursor;
                    cursor += consumed;
                    remaining -= consumed;
                    /* Continue the loop without emitting yet - the next
                     * iteration sees ELSE at the start. */
                    continue;
                }
            }

            buf_append(&out, cursor, (split_after - cursor) + split_after_len);
            buf_putc(&out, '\n');
            size_t consumed = (body - cursor);
            cursor += consumed;
            remaining -= consumed;
        }
        if (*eol == '\n') buf_putc(&out, '\n');
        line = *eol ? eol + 1 : eol;
    }
    return buf_detach(&out);
}

/* -- Pass: Blank lines around IF/WHILE blocks ------------------------ */

/*
 * Project style: an IF/WHILE block is visually separated from sibling
 * statements by blank lines on both sides, BUT a block flush against
 * its enclosing DO's opener or closer gets no blank (the brace itself
 * is the separator).  Examples:
 *
 *   PROC main =                    IF outer THEN DO
 *       stmt1;                         IF inner THEN
 *                                          body;
 *       IF cond THEN                 END;
 *           body;
 *
 *       stmt2;
 *
 * This pass runs AFTER pass_indent, so depth is read from the leading
 * whitespace count divided by `width`.
 */

typedef struct {
    const char *start;   /* pointer into source */
    size_t      len;     /* line content + terminating newline */
    int         depth;   /* indent-derived depth, or -1 for blank */
    bool        is_blank;
    bool        is_if_or_while;       /* starts with IF or WHILE */
    bool        is_end_semi;          /* starts with "END;" (exact or cuddled) */
    bool        is_cuddled_else;      /* "END; ELSE ..." */
    bool        is_block_boundary;    /* PROC, END_PROC, DO, ELSE DO, THEN DO */
    bool        ends_then_do_or_else_do; /* line opens a DO body */
    bool        is_data_start;        /* DATA at depth 0 */
    bool        is_proc_start;        /* PROC ... = at depth 0 */
    bool        is_end_proc;          /* END_PROC at depth 0 */
} LineInfo;

static int leading_indent_chars(const char *s, size_t len) {
    int n = 0;
    while ((size_t)n < len && (s[n] == ' ' || s[n] == '\t')) n++;
    return n;
}

static bool ci_starts(const char *s, size_t len, const char *kw) {
    size_t kl = strlen(kw);
    if (len < kl) return false;
    if (strncasecmp(s, kw, kl) != 0) return false;
    if (len == kl) return true;
    char c = s[kl];
    /* A word boundary is any non-identifier character.  TBOL identifiers
     * are alpha/digit/underscore. */
    return !(isalnum((unsigned char)c) || c == '_');
}

char *pass_blank_lines(const char *src, int width) {
    if (width <= 0) return strdup(src);

    /* First scan: collect line metadata. */
    int line_count = 0;
    for (const char *p = src; *p; p++) if (*p == '\n') line_count++;
    if (src[strlen(src)-1] != '\n' && *src) line_count++;

    LineInfo *lines = calloc(line_count + 1, sizeof(LineInfo));
    int li = 0;
    const char *p = src;
    while (*p) {
        const char *eol = strchr(p, '\n');
        if (!eol) eol = p + strlen(p);
        size_t llen = eol - p + (*eol == '\n' ? 1 : 0);
        lines[li].start = p;
        lines[li].len = llen;
        int ind = leading_indent_chars(p, eol - p);
        const char *content = p + ind;
        size_t content_len = (eol - p) - ind;
        lines[li].is_blank = (content_len == 0);
        if (lines[li].is_blank) {
            lines[li].depth = -1;
        } else {
            lines[li].depth = ind / width;
            lines[li].is_if_or_while = ci_starts(content, content_len, "IF") ||
                                       ci_starts(content, content_len, "WHILE");
            /* "END;" exactly (no trailing content beyond whitespace). */
            const char *trim_end = content + content_len;
            while (trim_end > content && (trim_end[-1] == ' ' || trim_end[-1] == '\t' ||
                                          trim_end[-1] == '\n' || trim_end[-1] == '\r'))
                trim_end--;
            /* "starts with END;" - covers both bare END; and cuddled
             * forms ("END; ELSE", "END; ELSE DO", "END; ELSE stmt;"). */
            lines[li].is_end_semi = (trim_end - content >= 4 &&
                                     strncasecmp(content, "END;", 4) == 0);
            lines[li].is_cuddled_else = (trim_end - content >= 9 &&
                                         strncasecmp(content, "END; ELSE", 9) == 0);
            lines[li].is_block_boundary =
                ci_starts(content, content_len, "PROC") ||
                ci_starts(content, content_len, "END_PROC") ||
                ci_starts(content, content_len, "DO");
            if (lines[li].depth == 0) {
                lines[li].is_data_start = ci_starts(content, content_len, "DATA");
                lines[li].is_proc_start = ci_starts(content, content_len, "PROC");
                lines[li].is_end_proc   = ci_starts(content, content_len, "END_PROC");
            }
            /* Does the line END with "THEN DO" or "ELSE DO"? */
            lines[li].ends_then_do_or_else_do =
                ends_with_then_do(content, trim_end - content) ||
                ends_with_else_do(content, trim_end - content);
        }
        li++;
        p = *eol ? eol + 1 : eol;
    }

    /* Identify which END; lines close an IF/WHILE block (track a stack
     * of opener kinds: 1 = IF/WHILE-with-DO, 2 = other DO).  Single-
     * statement IF/WHILE without DO doesn't open a frame. */
    bool *end_closes_if = calloc(line_count, sizeof(bool));
    /* Mark lines that are the LAST line of a single-statement IF/WHILE
     * block (no DO).  We need a blank after these too. */
    bool *is_single_stmt_block_end = calloc(line_count, sizeof(bool));

    int stack[256];
    int sp = 0;
    for (int i = 0; i < line_count; i++) {
        if (lines[i].is_blank) continue;
        if (lines[i].is_end_semi) {
            int popped_kind = -1;
            if (sp > 0) {
                popped_kind = stack[--sp];
            }
            /* For cuddled "END; ELSE ..." the line both closes the THEN
             * frame and opens an ELSE continuation. */
            if (lines[i].is_cuddled_else) {
                if (lines[i].ends_then_do_or_else_do) {
                    /* "END; ELSE DO" - re-push, the IF/ELSE block
                     * will close at the matching END;. */
                    stack[sp++] = popped_kind;
                } else if (popped_kind == 1) {
                    /* "END; ELSE single-stmt-body;" - the body lives on
                     * the next non-blank line at depth+1, and that's
                     * the end of the IF/ELSE block. */
                    int d = lines[i].depth;
                    int last = i;
                    for (int j = i + 1; j < line_count; j++) {
                        if (lines[j].is_blank) continue;
                        if (lines[j].depth > d) { last = j; continue; }
                        break;
                    }
                    is_single_stmt_block_end[last] = true;
                }
            } else if (popped_kind == 1) {
                end_closes_if[i] = true;
            }
            continue;
        }
        if (lines[i].is_if_or_while && lines[i].ends_then_do_or_else_do) {
            stack[sp++] = 1;
        } else if (lines[i].ends_then_do_or_else_do ||
                   ci_starts(lines[i].start + leading_indent_chars(lines[i].start, lines[i].len),
                             lines[i].len, "DO")) {
            stack[sp++] = 2;
        } else if (lines[i].is_if_or_while && !lines[i].ends_then_do_or_else_do) {
            /* Single-statement IF/WHILE.  The block extends through the
             * body line(s) at depth+1, plus any ELSE chain.  Find the
             * last line of the block. */
            int d = lines[i].depth;
            int last = i;
            for (int j = i + 1; j < line_count; j++) {
                if (lines[j].is_blank) continue;
                if (lines[j].depth > d) { last = j; continue; }
                if (lines[j].depth < d) break;
                /* Same depth.  ELSE keeps the block open.  Anything else
                 * starts a new sibling. */
                const char *c = lines[j].start +
                    leading_indent_chars(lines[j].start, lines[j].len);
                size_t cl = lines[j].len -
                    leading_indent_chars(lines[j].start, lines[j].len);
                if (ci_starts(c, cl, "ELSE")) {
                    last = j;
                    continue;
                }
                break;
            }
            is_single_stmt_block_end[last] = true;
        }
    }

    /* Second pass: emit lines, inserting blanks before IF/WHILE openers
     * and after IF/WHILE-closing END;s when the adjacent same-depth
     * sibling is not blank or a block boundary. */
    Buf out;
    buf_init(&out);

    for (int i = 0; i < line_count; i++) {
        bool insert_blank_before = false;
        bool insert_blank_after = false;

        if (!lines[i].is_blank && lines[i].is_if_or_while) {
            /* Look back for previous non-blank line. */
            for (int j = i - 1; j >= 0; j--) {
                if (lines[j].is_blank) {
                    /* Already a blank - no need to add. */
                    break;
                }
                if (lines[j].depth == lines[i].depth) {
                    insert_blank_before = true;
                }
                break;
            }
        }

        /* Top-level section boundaries: DATA and PROC each start with a
         * blank line separating them from prior content (PROGRAM,
         * COPY/DEFINE, the previous PROC, or the prior DATA group). */
        if (!lines[i].is_blank &&
            (lines[i].is_data_start || lines[i].is_proc_start)) {
            for (int j = i - 1; j >= 0; j--) {
                if (lines[j].is_blank) break;        /* already blank */
                /* Any non-blank predecessor warrants a blank line. */
                insert_blank_before = true;
                break;
            }
        }

        bool is_block_end_line = !lines[i].is_blank &&
            ((lines[i].is_end_semi && end_closes_if[i]) ||
             is_single_stmt_block_end[i]);
        if (is_block_end_line) {
            /* For block-end lookups we use the depth of the OPENER, not
             * the body line. For single-stmt blocks, lines[i] is the
             * body (at depth+1) but the sibling check is at the IF's
             * depth. */
            int sibling_depth = lines[i].depth;
            if (is_single_stmt_block_end[i] && !lines[i].is_end_semi) {
                /* Walk back to find the IF/WHILE depth. */
                for (int j = i - 1; j >= 0; j--) {
                    if (lines[j].is_blank) continue;
                    if (lines[j].is_if_or_while && !lines[j].ends_then_do_or_else_do) {
                        sibling_depth = lines[j].depth;
                        break;
                    }
                    if (lines[j].depth < lines[i].depth) {
                        sibling_depth = lines[j].depth;
                        break;
                    }
                }
            }
            /* Look ahead for next non-blank line. */
            for (int j = i + 1; j < line_count; j++) {
                if (lines[j].is_blank) break;  /* already a blank */
                if (lines[j].depth == sibling_depth) {
                    /* Sibling at same depth.  Skip if it's an enclosing
                     * closer (END;/END_PROC at same depth as our END;
                     * means same-level - treat as sibling) or the closer
                     * of the enclosing block (which would be at depth-1,
                     * already excluded by the depth check). */
                    if (lines[j].is_end_semi ||
                        ci_starts(lines[j].start + leading_indent_chars(lines[j].start, lines[j].len),
                                  lines[j].len, "END_PROC")) {
                        /* Sibling END;/END_PROC at same depth - treat
                         * as block boundary, no blank. */
                        break;
                    }
                    insert_blank_after = true;
                }
                break;
            }
        }

        /* Avoid emitting a double blank: if the previous emit ended
         * with two newlines (i.e. last char and char before are both
         * '\n'), the blank line is already present. */
        bool already_blank = (out.len >= 2 &&
                              out.data[out.len - 1] == '\n' &&
                              out.data[out.len - 2] == '\n');
        if (insert_blank_before && !already_blank) buf_putc(&out, '\n');
        buf_append(&out, lines[i].start, lines[i].len);
        if (insert_blank_after) {
            /* Peek at the next non-blank line.  If it's an IF/WHILE
             * opener at the same depth, that line will insert its own
             * blank-before; we'd double it.  Skip our blank-after. */
            bool next_will_blank = false;
            for (int j = i + 1; j < line_count; j++) {
                if (lines[j].is_blank) continue;
                if (lines[j].depth == lines[i].depth && lines[j].is_if_or_while)
                    next_will_blank = true;
                break;
            }
            if (!next_will_blank) buf_putc(&out, '\n');
        }
    }

    free(lines);
    free(end_closes_if);
    free(is_single_stmt_block_end);
    return buf_detach(&out);
}

/* -- Pass 4: Convert hex escape string literals to 0x notation ------- */

/*
 * Convert string literals containing \xNN escapes to 0x hex notation.
 *
 * Rules:
 * 1. Pure hex ('\xNN\xNN...') -> 0xNNNN...
 * 2. Mixed strings with any \xNN escapes and <8 leading printables ->
 *    convert entire string (printables + escapes) to hex.
 *    E.g., '\x00"\x00' -> 0x002200
 *    E.g., '\x00 ' -> 0x0020
 * 3. Strings with 8+ leading printables are object references handled
 *    by the decompiler's DEFINE extraction - not touched here.
 * 4. Strings with no \xNN escapes are left unchanged.
 */
char *pass_hex_literals(const char *src) {
    Buf out;
    buf_init(&out);

    const char *p = src;
    while (*p) {
        if (*p == '\'') {
            p++;  /* skip opening quote */

            /* Find closing quote */
            const char *close = p;
            while (*close && *close != '\'') {
                if (close[0] == '\\' && close[1] == 'x' &&
                    isxdigit((unsigned char)close[2]) &&
                    isxdigit((unsigned char)close[3]))
                    close += 4;  /* skip \xNN */
                else
                    close++;
            }
            if (*close != '\'') {
                /* Unterminated string - emit quote and move on */
                buf_putc(&out, '\'');
                continue;
            }

            /* Check if string contains any \xNN escapes */
            bool has_hex = false;
            for (const char *s = p; s < close; ) {
                if (s[0] == '\\' && s[1] == 'x' &&
                    isxdigit((unsigned char)s[2]) &&
                    isxdigit((unsigned char)s[3])) {
                    has_hex = true;
                    break;
                }
                s++;
            }

            if (!has_hex) {
                /* No hex escapes - emit unchanged */
                buf_putc(&out, '\'');
                buf_append(&out, p, close - p);
                buf_putc(&out, '\'');
                p = close + 1;
                continue;
            }

            /* Count leading printable chars (before first \xNN) */
            int leading_printable = 0;
            const char *s = p;
            while (s < close && !(s[0] == '\\' && s[1] == 'x')) {
                leading_printable++;
                s++;
            }

            if (leading_printable >= 8) {
                /* Object reference - leave for DEFINE extraction (case 3 skipped) */
                buf_putc(&out, '\'');
                buf_append(&out, p, close - p);
                buf_putc(&out, '\'');
                p = close + 1;
                continue;
            }

            /* Convert entire string content to hex */
            buf_append(&out, "0x", 2);
            s = p;
            while (s < close) {
                if (s[0] == '\\' && s[1] == 'x' &&
                    isxdigit((unsigned char)s[2]) &&
                    isxdigit((unsigned char)s[3])) {
                    buf_putc(&out, s[2]);
                    buf_putc(&out, s[3]);
                    s += 4;
                } else {
                    /* Printable char - emit as 2-digit hex */
                    char hex[3];
                    snprintf(hex, sizeof(hex), "%02x", (unsigned char)*s);
                    buf_append(&out, hex, 2);
                    s++;
                }
            }
            p = close + 1;  /* skip closing quote */
        } else {
            buf_putc(&out, *p);
            p++;
        }
    }

    return buf_detach(&out);
}

/* -- Pipeline --------------------------------------------------------- */

char *tbol_fmt(const char *source, const TbolFmtOptions *opts) {
    if (!source) return NULL;

    char *cur = strdup(source);

    if (opts->hex_literals) {
        char *next = pass_hex_literals(cur);
        free(cur);
        cur = next;
    }

    if (opts->strip_labels) {
        char *next = pass_strip_labels(cur);
        free(cur);
        cur = next;
    }

    if (opts->cuddle_else) {
        char *next = pass_cuddle_else(cur);
        free(cur);
        cur = next;
    }

    {
        char *next = pass_then_newline(cur);
        free(cur);
        cur = next;
    }

    if (opts->indent_width > 0) {
        char *next = pass_indent(cur, opts->indent_width);
        free(cur);
        cur = next;

        next = pass_blank_lines(cur, opts->indent_width);
        free(cur);
        cur = next;
    }

    return cur;
}
