/*
 * Copyright 2025-2026, Phillip Heller
 *
 * This file is part of Prodigy Reloaded.
 *
 * Prodigy Reloaded is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * Prodigy Reloaded is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Prodigy Reloaded. If not,
 * see <https://www.gnu.org/licenses/>.
 */
/*
 * TBOL Compiler - Control Flow Code Generation
 */

#include "codegen_internal.h"
#include <string.h>
#include <stdio.h>

/* Internal label counters (reset per compilation unit) */
static int or_counter = 0;
static int if_counter = 0;
static int while_counter = 0;

/*
 * Reset control flow counters (called from codegen_generate)
 */
void control_reset_counters(void) {
    or_counter = 0;
    if_counter = 0;
    while_counter = 0;
}

/*
 * Get inverse comparison opcode (for false branch)
 */
static uint8_t get_inverse_cmp_opcode(CmpOp op) {
    switch (op) {
        case CMP_EQ: return OP_CJNE;
        case CMP_NE: return OP_CJEQ;
        case CMP_LT: return OP_CJGE;
        case CMP_GT: return OP_CJLE;
        case CMP_LE: return OP_CJGT;
        case CMP_GE: return OP_CJLT;
        default: return OP_CJNE;
    }
}


/*
 * Get direct comparison opcode (for true branch / IF-GOTO optimization)
 */
static uint8_t get_direct_cmp_opcode(CmpOp op) {
    switch (op) {
        case CMP_EQ: return OP_CJEQ;
        case CMP_NE: return OP_CJNE;
        case CMP_LT: return OP_CJLT;
        case CMP_GT: return OP_CJGT;
        case CMP_LE: return OP_CJLE;
        case CMP_GE: return OP_CJGE;
        default: return OP_CJEQ;
    }
}

/*
 * Check if statement is a simple GOTO (or DO block containing just GOTO)
 * Returns the target label if so, NULL otherwise
 */
static const char *get_simple_goto_target(AstNode *stmt) {
    if (!stmt) return NULL;

    if (stmt->kind == AST_GOTO) {
        return stmt->data.goto_stmt.label;
    }

    /* Check for DO block with single GOTO child */
    if (stmt->kind == AST_DO_BLOCK && stmt->child_count == 1) {
        AstNode *child = stmt->children[0];
        if (child && child->kind == AST_GOTO) {
            return child->data.goto_stmt.label;
        }
    }

    return NULL;
}

/*
 * Forward declaration for condition generator
 */
static void gen_condition_false_jump(AstNode *cond, const char *false_label);

/*
 * Collect all terms from an OR chain into a flat array
 * For (A OR B) OR C, returns [A, B, C]
 */
static int collect_or_terms(AstNode *node, AstNode **terms, int max_terms) {
    if (!node || max_terms <= 0) return 0;

    if (node->kind == AST_LOGIC_OR) {
        int left_count = collect_or_terms(node->children[0], terms, max_terms);
        int right_count = collect_or_terms(node->children[1], terms + left_count, max_terms - left_count);
        return left_count + right_count;
    } else {
        terms[0] = node;
        return 1;
    }
}

/*
 * Generate code for a condition, jumping to label if FALSE
 */
static void gen_condition_false_jump(AstNode *cond, const char *false_label) {
    if (!cond) return;

    switch (cond->kind) {
        case AST_COMPARE: {
            /* Jump to false_label if condition is FALSE */
            AstNode *left = cond->children[0];
            AstNode *right = cond->children[1];
            uint8_t inv_opcode = get_inverse_cmp_opcode(cond->data.compare.op);
            emit_cond_jump(inv_opcode, left, right, false_label);
            break;
        }

        case AST_LOGIC_AND: {
            /* A AND B: if A is false, jump to false_label; if B is false, jump to false_label */
            gen_condition_false_jump(cond->children[0], false_label);
            gen_condition_false_jump(cond->children[1], false_label);
            break;
        }

        case AST_LOGIC_OR: {
            /* Flatten the OR chain and generate cascade pattern.
             *
             * For n-term OR (T1 OR T2 OR ... OR Tn):
             *
             * 2-term:
             *   CJNE T1, check_2; JUMP body
             *   check_2: CJNE T2, false_label
             *   body:
             *
             * 3+ terms:
             *   CJNE T1, check_2; JUMP cascade
             *   check_2: CJNE T2, check_3
             *   ...
             *   check_{n-1}: CJNE T_{n-1}, check_n
             *   cascade: JUMP body
             *   check_n: CJNE Tn, false_label
             *   body:
             */
            int my_id = or_counter++;

            /* Flatten the OR chain */
            AstNode *terms[32];
            int n = collect_or_terms(cond, terms, 32);

            if (n == 1) {
                /* Single term - shouldn't happen but handle gracefully */
                gen_condition_false_jump(terms[0], false_label);
                break;
            }

            char body[32], cascade[32];
            char checks[32][32];
            snprintf(body, sizeof(body), "__or_body_%d", my_id);
            snprintf(cascade, sizeof(cascade), "__or_casc_%d", my_id);
            for (int i = 1; i < n; i++) {
                snprintf(checks[i], sizeof(checks[i]), "__or_chk_%d_%d", my_id, i);
            }

            if (n == 2) {
                /* 2-term OR: T1 jumps directly to body */
                gen_condition_false_jump(terms[0], checks[1]);
                emit_jump(body);
                define_label(checks[1]);
                gen_condition_false_jump(terms[1], false_label);
                define_label(body);
            } else {
                /* 3+ terms: cascade pattern (matches original compiler)
                 *
                 * Each true-branch JUMP targets the label just before
                 * the next JUMP in the chain, so JUMPs are shared:
                 *   CJNE T1, check_1; JUMP casc_1
                 *   check_1: CJNE T2, check_2
                 *   casc_1: JUMP casc_2
                 *   check_2: CJNE T3, check_3
                 *   casc_2: JUMP body
                 *   check_3: CJNE T4, false_label
                 *   body:
                 */
                char cascs[32][32];
                for (int i = 0; i < n - 2; i++) {
                    snprintf(cascs[i], sizeof(cascs[i]),
                             "__or_casc_%d_%d", my_id, i);
                }

                /* First term: CJNE to check_1, JUMP to casc_0 */
                gen_condition_false_jump(terms[0], checks[1]);
                emit_jump(cascs[0]);

                /* Middle terms: CJNE to next check, then cascade label + JUMP */
                for (int i = 1; i < n - 1; i++) {
                    define_label(checks[i]);
                    gen_condition_false_jump(terms[i], checks[i + 1]);
                    define_label(cascs[i - 1]);
                    if (i < n - 2) {
                        emit_jump(cascs[i]);
                    } else {
                        emit_jump(body);
                    }
                }

                /* Last term: CJNE to false_label */
                define_label(checks[n - 1]);
                gen_condition_false_jump(terms[n - 1], false_label);

                define_label(body);
            }
            break;
        }

        default:
            break;
    }
}


/*
 * Generate code for IF statement
 */
void gen_if(AstNode *node) {
    int my_id = if_counter++;

    char else_label[32], end_label[32];
    snprintf(else_label, sizeof(else_label), "__else_%d", my_id);
    snprintf(end_label, sizeof(end_label), "__endif_%d", my_id);

    AstNode *cond = node->children[0];
    AstNode *then_stmt = node->children[1];
    AstNode *else_stmt = node->child_count > 2 ? node->children[2] : NULL;

    /*
     * Optimization: IF cond THEN GOTO target (with no ELSE)
     * Emit a single conditional jump directly to target instead of:
     *   CJNE cond, skip; JUMP target; skip:
     *
     * This matches older TBOL compiler behavior found in some production
     * bytecode. The TBOL 4.21 compiler does not use this optimization,
     * but earlier versions did.
     * Only applies to simple conditions (single comparison, no AND/OR).
     */
    if (g_options.if_goto_opt &&
        !else_stmt && cond && cond->kind == AST_COMPARE) {
        const char *goto_target = get_simple_goto_target(then_stmt);
        if (goto_target) {
            AstNode *left = cond->children[0];
            AstNode *right = cond->children[1];
            uint8_t opcode = get_direct_cmp_opcode(cond->data.compare.op);
            emit_cond_jump(opcode, left, right, goto_target);
            return;
        }
    }

    /* Standard IF generation: jump to else/end if condition is false */
    const char *false_target = else_stmt ? else_label : end_label;
    gen_condition_false_jump(cond, false_target);

    /* Generate then block */
    gen_statement(then_stmt);

    if (else_stmt) {
        /* Jump over else block */
        emit_jump(end_label);

        /* Else label */
        define_label(else_label);

        /* Generate else block */
        gen_statement(else_stmt);
    }

    /* End label */
    define_label(end_label);
}

/*
 * Generate code for WHILE statement
 */
void gen_while(AstNode *node) {
    int my_id = while_counter++;

    char top_label[32], end_label[32];
    snprintf(top_label, sizeof(top_label), "__while_%d", my_id);
    snprintf(end_label, sizeof(end_label), "__endwhile_%d", my_id);

    AstNode *cond = node->children[0];
    AstNode *body = node->children[1];

    /* Top of loop */
    define_label(top_label);

    /* Check condition, jump to end if false */
    gen_condition_false_jump(cond, end_label);

    /* Generate body */
    gen_statement(body);

    /* Jump back to top */
    emit_jump(top_label);

    /* End label */
    define_label(end_label);
}

/*
 * Generate code for DO block
 */
void gen_do_block(AstNode *node) {
    for (int i = 0; i < node->child_count; i++) {
        gen_statement(node->children[i]);
    }
}

/*
 * Generate code for GOTO
 */
void gen_goto(AstNode *node) {
    emit_jump(node->data.goto_stmt.label);
}
