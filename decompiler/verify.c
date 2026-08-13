/*
 * verify.c - Speculative round-trip verification
 *
 * Iteratively refines decompiler output by compiling it with the
 * statically-linked tbolc compiler, comparing bytecodes, and ratcheting
 * down pattern expressiveness for CJ instructions that don't round-trip.
 */

#include "verify.h"
#include "baseline.h"
#include "bytecode/opcodes.h"
#include "bytecode/cod_file.h"
#include "compile.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#endif

/* Scratch temp directory (no trailing separator). The decompiler writes
 * its emitted source, recompiled .cod, and GEV scratch here. `/tmp` is a
 * native-Windows path to a usually-nonexistent <drive>:\tmp, so on Windows
 * use the real OS temp path; elsewhere honor $TMPDIR, falling back to /tmp. */
const char *tbol_tmp_dir(void) {
#ifdef _WIN32
    static char buf[MAX_PATH];
    DWORD n = GetTempPathA((DWORD)sizeof(buf), buf);
    if (n == 0 || n >= sizeof(buf)) return ".";
    while (n > 0 && (buf[n - 1] == '\\' || buf[n - 1] == '/'))
        buf[--n] = '\0';
    return buf;
#else
    const char *d = getenv("TMPDIR");
    return (d && *d) ? d : "/tmp";
#endif
}

/* Locate the single .cod the compiler wrote into `dir`. tbolc derives the
 * output name from a LOWERCASED input basename, so the exact case is not
 * predictable from src_path - and on a case-sensitive filesystem (Linux)
 * predicting the mixed-case mkstemp name fails to open the lowercase file
 * that was actually written (this passed on case-insensitive macOS by
 * accident). The verify temp dir is freshly created for one compile, so
 * scanning it for the lone .cod is robust. Fills `out`, returns true. */
static bool find_cod_in_dir(const char *dir, char *out, size_t outsz) {
    DIR *d = opendir(dir);
    if (!d) return false;
    bool found = false;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t l = strlen(e->d_name);
        if (l > 4 && strcmp(e->d_name + l - 4, ".cod") == 0) {
            snprintf(out, outsz, "%s/%s", dir, e->d_name);
            found = true;
            break;
        }
    }
    closedir(d);
    return found;
}

/* -- Bytecode comparison ---------------------------------------------- */

static uint8_t *load_code_section(const char *path, int *code_len) {
    Program *prog = cod_file_load(path);
    if (!prog) return NULL;
    if (!prog->code || prog->code_size == 0) {
        program_free(prog);
        return NULL;
    }
    uint8_t *code = malloc(prog->code_size);
    memcpy(code, prog->code, prog->code_size);
    *code_len = (int)prog->code_size;
    program_free(prog);
    return code;
}

static int compare_code(const uint8_t *a, int alen, const uint8_t *b, int blen) {
    int minlen = alen < blen ? alen : blen;
    for (int i = 0; i < minlen; i++)
        if (a[i] != b[i]) return i;
    if (alen != blen) return minlen;
    return -1;
}

/* -- Try a mode table: emit, compile, compare ------------------------- */

/*
 * Returns:
 *  -1  = round-trip identical (success)
 *   N  = first differing code offset
 *  -2  = compilation failed
 */
static int try_mode(Program *prog, ProcList *procs, GEVTable *gev,
                     ModeTable *mt, const char **include_paths, int include_count,
                     const uint8_t *orig_code, int orig_len, int iter) {
    char *tmp_src = NULL, *tmp_dir = NULL, *tmp_cod = NULL;
    asprintf(&tmp_src, "%s/tboldc_v%d.src", tbol_tmp_dir(), iter);
    asprintf(&tmp_dir, "%s/tboldc_v%d/", tbol_tmp_dir(), iter);
#ifdef _WIN32
    _mkdir(tmp_dir);
#else
    mkdir(tmp_dir, 0755);
#endif

    FILE *tmp = fopen(tmp_src, "w");
    if (!tmp) { free(tmp_src); free(tmp_dir); return -2; }
    emit_baseline(tmp, prog, procs, gev, mt, NULL);
    fclose(tmp);

    int rc = tbolc_compile_file(tmp_src, tmp_dir, include_paths, include_count);
    if (rc != 0) {
        unlink(tmp_src);
        rmdir(tmp_dir);
        free(tmp_src); free(tmp_dir);
        return -2;
    }

    asprintf(&tmp_cod, "%stboldc_v%d.cod", tmp_dir, iter);
    int recomp_len;
    uint8_t *recomp_code = load_code_section(tmp_cod, &recomp_len);
    unlink(tmp_src);
    unlink(tmp_cod);
    rmdir(tmp_dir);
    free(tmp_src); free(tmp_dir); free(tmp_cod);

    if (!recomp_code) return -2;

    int diff = compare_code(orig_code, orig_len, recomp_code, recomp_len);
    free(recomp_code);
    return diff;
}

/* -- Collect CJ addresses in a proc ----------------------------------- */

static int collect_cjs(Program *prog, ProcBoundary *pb,
                        uint16_t *addrs, int max) {
    int n = 0;
    for (Instruction *i = prog->instructions; i; i = i->next) {
        if (i->address < pb->start_addr || i->address >= pb->end_addr) continue;
        if (i->mnemonic >= MNEM_CJEQ && i->mnemonic <= MNEM_CJGE) {
            if (n < max) addrs[n++] = i->address;
        }
    }
    return n;
}

/* -- Public API ------------------------------------------------------- */

int emit_verified(FILE *out, Program *prog, ProcList *procs, GEVTable *gev,
                  const char **include_paths, int include_path_count,
                  const char *original_cod, int max_iter,
                  const char *input_path) {
    int orig_len;
    uint8_t *orig_code = load_code_section(original_cod, &orig_len);
    if (!orig_code) {
        fprintf(stderr, "verify: cannot load original, emitting unverified\n");
        emit_baseline(out, prog, procs, gev, NULL, input_path);
        return 1;
    }

    ModeTable *mt = mode_table_new();
    int iter = 0;
    bool success = false;

    /* First try: full structural mode */
    int diff = try_mode(prog, procs, gev, mt, include_paths,
                         include_path_count, orig_code, orig_len, iter++);

    if (diff == -1) {
        /* Perfect on first try */
        emit_baseline(out, prog, procs, gev, mt, input_path);
        success = true;
    } else if (diff >= 0 && iter < max_iter) {
        /* Mismatch - find the proc containing the diff and try per-CJ ratcheting */
        for (int p = 0; p < procs->count && !success; p++) {
            ProcBoundary *pb = &procs->procs[p];
            if (diff < pb->start_addr || (uint16_t)diff >= pb->end_addr)
                continue;

            /* Collect all CJs in this proc */
            uint16_t cj_addrs[1024];
            int ncj = collect_cjs(prog, pb, cj_addrs, 1024);

            /* Try each CJ individually: ratchet one at a time, keep if it helps */
            for (int c = 0; c < ncj && iter < max_iter; c++) {
                /* Try PMODE_NO_ELSE for this CJ */
                mode_table_set(mt, cj_addrs[c], PMODE_NO_ELSE);
                int result = try_mode(prog, procs, gev, mt, include_paths,
                                       include_path_count, orig_code, orig_len, iter++);

                if (result == -1) {
                    /* This single ratchet fixed everything */
                    fprintf(stderr, "verify: %s round-trip achieved after %d iteration(s)\n",
                            prog->program_name, iter);
                    emit_baseline(out, prog, procs, gev, mt, input_path);
                    success = true;
                    break;
                } else if (result == -2) {
                    /* Compilation failed - this CJ can't be NO_ELSE, revert it */
                    mode_table_set(mt, cj_addrs[c], PMODE_FULL);
                } else {
                    /* Still mismatches but compiles - keep if closer, revert if not */
                    /* For now, keep it (it may help in combination with others) */
                }
            }

            if (!success && iter < max_iter) {
                /* Individual NO_ELSE wasn't enough. Check current state. */
                diff = try_mode(prog, procs, gev, mt, include_paths,
                                 include_path_count, orig_code, orig_len, iter++);
                if (diff == -1) {
                    fprintf(stderr, "verify: %s round-trip achieved after %d iteration(s)\n",
                            prog->program_name, iter);
                    emit_baseline(out, prog, procs, gev, mt, input_path);
                    success = true;
                } else if (diff >= 0) {
                    /* Try PMODE_FLAT for remaining NO_ELSE CJs that didn't help */
                    for (int c = 0; c < ncj && iter < max_iter; c++) {
                        if (mode_table_get(mt, cj_addrs[c]) != PMODE_NO_ELSE)
                            continue;
                        mode_table_set(mt, cj_addrs[c], PMODE_FLAT);
                        int r = try_mode(prog, procs, gev, mt, include_paths,
                                          include_path_count, orig_code, orig_len, iter++);
                        if (r == -1) {
                            fprintf(stderr, "verify: %s round-trip achieved after %d iteration(s)\n",
                                    prog->program_name, iter);
                            emit_baseline(out, prog, procs, gev, mt, input_path);
                            success = true;
                            break;
                        } else if (r == -2) {
                            mode_table_set(mt, cj_addrs[c], PMODE_NO_ELSE);
                        }
                    }
                }
            }
        }
    }

    if (!success) {
        if (diff == -2)
            fprintf(stderr, "verify: %s compilation failed, emitting best effort\n",
                    prog->program_name);
        else if (diff >= 0)
            fprintf(stderr, "verify: %s unresolved diff at code+%d after %d iterations\n",
                    prog->program_name, diff, iter);
        emit_baseline(out, prog, procs, gev, mt, input_path);
    }

    free(orig_code);
    mode_table_free(mt);
    return success ? 0 : 1;
}

/* -- Final verification ----------------------------------------------- */

int verify_roundtrip(const char *src_path, const char *original_cod,
                     const char **include_paths, int include_path_count) {
    int orig_len;
    uint8_t *orig_code = load_code_section(original_cod, &orig_len);
    if (!orig_code) return 1;

    char tmp_dir[512];
    snprintf(tmp_dir, sizeof(tmp_dir), "%s/tboldc_final_XXXXXX", tbol_tmp_dir());
    if (!mkdtemp(tmp_dir)) { free(orig_code); return 1; }

    int rc = tbolc_compile_file(src_path, tmp_dir,
                                 include_paths, include_path_count);
    if (rc != 0) {
        fprintf(stderr, "verify: final compilation failed\n");
        rmdir(tmp_dir);
        free(orig_code);
        return 1;
    }

    char cod_path[512];
    if (!find_cod_in_dir(tmp_dir, cod_path, sizeof(cod_path))) {
        fprintf(stderr, "verify: cannot find recompiled output\n");
        rmdir(tmp_dir);
        free(orig_code);
        return 1;
    }

    int recomp_len;
    uint8_t *recomp_code = load_code_section(cod_path, &recomp_len);
    unlink(cod_path);
    rmdir(tmp_dir);

    if (!recomp_code) {
        fprintf(stderr, "verify: cannot load recompiled output\n");
        free(orig_code);
        return 1;
    }

    int diff = compare_code(orig_code, orig_len, recomp_code, recomp_len);
    free(orig_code);
    free(recomp_code);

    if (diff < 0) {
        fprintf(stderr, "verify: IDENTICAL\n");
        return 0;
    } else {
        fprintf(stderr, "verify: MISMATCH at code+%d\n", diff);
        return 1;
    }
}
