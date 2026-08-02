/*
 * tboldc - TBOL Decompiler
 *
 * Produces human-readable TBOL source from compiled bytecode.
 * Output round-trips: tboldc -> tbolc -> identical bytecode.
 *
 * Usage: tboldc [options] <input.cod|input.pgm>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bytecode/cod_file.h"
#include "bytecode/decode.h"
#include "bytecode/gev.h"
#include "bytecode/proc.h"
#include "baseline.h"
#include "verify.h"
#include "tbol_fmt.h"

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <input.cod|input.pgm>\n", prog);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -I <path>        Include path for XXCGTSYS (GEV name resolution)\n");
    fprintf(stderr, "  -o <file>        Output file (default: stdout)\n");
    fprintf(stderr, "  -n               No verification (skip round-trip refinement)\n");
    fprintf(stderr, "  -f               Force output even if verification fails\n");
    fprintf(stderr, "  -m <N>           Max verification iterations (default: 64)\n");
    fprintf(stderr, "  --no-xxcgtsys    Disable GEV name resolution entirely\n");
    fprintf(stderr, "  --no-format      Skip the tbolfmt pass on the output\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

int main(int argc, char **argv) {
    const char *input_file = NULL;
    const char *output_file = NULL;
    bool verify = true;
    bool force_output = false;
    bool no_xxcgtsys = false;
    bool format_output = true;
    int max_iter = 64;
    const char *include_paths[32];
    int include_path_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-I") == 0 && i + 1 < argc) {
            if (include_path_count < 32)
                include_paths[include_path_count++] = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-n") == 0) {
            verify = false;
        } else if (strcmp(argv[i], "-f") == 0) {
            force_output = true;
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            max_iter = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--no-xxcgtsys") == 0) {
            no_xxcgtsys = true;
        } else if (strcmp(argv[i], "--no-format") == 0) {
            format_output = false;
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

    if (!input_file) {
        fprintf(stderr, "Error: no input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Load and decode bytecode */
    Program *prog = cod_file_load(input_file);
    if (!prog) return 1;

    if (decode_program(prog) != 0) {
        fprintf(stderr, "Error: failed to decode program\n");
        program_free(prog);
        return 1;
    }

    /* GEV name resolution:
     *   1. Try loading XXCGTSYS from -I paths
     *   2. If not found, use built-in definitions (unless --no-xxcgtsys)
     *
     * When using built-in GEVs, write a temp XXCGTSYS for the recompiler
     * and inject the temp dir as an include path. */
    GEVTable *gev = NULL;
    char *builtin_tmp_dir = NULL;

    if (!no_xxcgtsys) {
        gev = gev_table_new();

        /* Try explicit include paths first */
        if (include_path_count > 0) {
            gev_table_load(gev, include_paths, include_path_count);
        }

        /* Fall back to built-in definitions */
        if (!gev->loaded) {
            gev_table_load_builtin(gev);

            /* Write temp XXCGTSYS for recompiler verification */
            char tmp_template[] = "/tmp/tboldc_gev_XXXXXX";
            builtin_tmp_dir = mkdtemp(tmp_template);
            if (builtin_tmp_dir) {
                builtin_tmp_dir = strdup(builtin_tmp_dir);
                gev_write_xxcgtsys(builtin_tmp_dir);
                if (include_path_count < 32)
                    include_paths[include_path_count++] = builtin_tmp_dir;
            }
        }
    }

    /* Detect procedure boundaries */
    ProcList *procs = proc_list_new();
    find_procedures(prog, procs);

    /* Open output */
    FILE *out = stdout;
    if (output_file) {
        out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "Error: cannot open '%s' for writing\n", output_file);
            proc_list_free(procs);
            if (gev) gev_table_free(gev);
            program_free(prog);
            free(builtin_tmp_dir);
            return 1;
        }
    }

    /* Emit source to a temp file so we can always verify, then copy to output */
    char tmp_out[] = "/tmp/tboldc_out_XXXXXX";
    int tmp_fd = mkstemp(tmp_out);
    FILE *tmp = fdopen(tmp_fd, "w");

    if (verify) {
        emit_verified(tmp, prog, procs, gev,
                      include_paths, include_path_count,
                      input_file, max_iter, input_file);
    } else {
        emit_baseline(tmp, prog, procs, gev, NULL, input_file);
    }
    fclose(tmp);

    /* Final verification: compile temp output and compare to original */
    int vrc = verify_roundtrip(tmp_out, input_file,
                                include_paths, include_path_count);

    /* Only emit output if verification passed (or -f forces it).  Run
     * the result through tbol_fmt() (the same library tbolfmt uses)
     * unless --no-format was given.  Formatting only changes whitespace,
     * blank-line layout, and comment placement - it cannot affect the
     * recompiled bytecode, so it's safe to apply after verification. */
    if (vrc == 0 || force_output) {
        FILE *sf = fopen(tmp_out, "r");
        if (sf) {
            fseek(sf, 0, SEEK_END);
            long sz = ftell(sf);
            fseek(sf, 0, SEEK_SET);
            char *raw = malloc(sz + 1);
            if (raw) {
                size_t got = fread(raw, 1, sz, sf);
                raw[got] = '\0';
                if (format_output) {
                    TbolFmtOptions opts = tbol_fmt_defaults();
                    char *fmtd = tbol_fmt(raw, &opts);
                    if (fmtd) {
                        fputs(fmtd, out);
                        free(fmtd);
                    } else {
                        fwrite(raw, 1, got, out);  /* fallback */
                    }
                } else {
                    fwrite(raw, 1, got, out);
                }
                free(raw);
            }
            fclose(sf);
        }
    }
    unlink(tmp_out);

    /* Cleanup */
    if (builtin_tmp_dir) {
        char tmp_xxcg[1024];
        snprintf(tmp_xxcg, sizeof(tmp_xxcg), "%s/XXCGTSYS", builtin_tmp_dir);
        unlink(tmp_xxcg);
        rmdir(builtin_tmp_dir);
        free(builtin_tmp_dir);
    }
    if (out != stdout) fclose(out);
    proc_list_free(procs);
    if (gev) gev_table_free(gev);
    program_free(prog);
    return 0;
}
