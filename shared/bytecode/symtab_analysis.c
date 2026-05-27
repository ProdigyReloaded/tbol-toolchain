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
#include "symtab_analysis.h"
#include "opcodes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Symbol table functions
 */
SymbolTable *symbol_table_new(void) {
    SymbolTable *st = calloc(1, sizeof(SymbolTable));
    st->max_slot = -1;
    /* Initialize bounds_certain to true - it becomes false when we see runtime index */
    for (int i = 0; i < MAX_RDA_SLOTS; i++) {
        st->slots[i].bounds_certain = true;
    }
    return st;
}

void symbol_table_free(SymbolTable *st) {
    free(st);
}

/*
 * Recursively scan an operand for RDA references
 */
static void scan_operand(SymbolTable *st, Operand *op) {
    if (!op) return;

    if (op->kind == OP_RDA) {
        int slot = op->value;
        if (slot >= 0 && slot < MAX_RDA_SLOTS) {
            st->slots[slot].used = true;
            if (slot > st->max_slot) {
                st->max_slot = slot;
            }

            if (!op->indexed || !op->index) {
                st->slots[slot].direct_access = true;
            } else {
                st->slots[slot].is_array = true;

                /* Analyze the index to determine array bounds */
                Operand *idx = op->index;
                int index_val = -1;
                bool is_literal = false;

                if (idx->kind == OP_LITERAL_NUM) {
                    index_val = idx->value;
                    is_literal = true;
                } else if (idx->kind == OP_LITERAL_STR && idx->str_value) {
                    index_val = atoi(idx->str_value);
                    is_literal = true;
                }

                if (is_literal && index_val >= 0) {
                    if (index_val > st->slots[slot].max_index) {
                        st->slots[slot].max_index = index_val;
                    }
                } else {
                    /* Runtime index - bounds are uncertain */
                    st->slots[slot].bounds_certain = false;

                    /* Track which I register is used for indexing */
                    if (idx->kind == OP_REG_I && idx->value >= 1 && idx->value <= 8) {
                        int ireg = idx->value;
                        if (st->slots[slot].index_ireg == 0) {
                            st->slots[slot].index_ireg = ireg;
                        } else if (st->slots[slot].index_ireg != ireg) {
                            /* Multiple different I registers used - can't infer */
                            st->slots[slot].index_ireg = -1;
                        }
                    }
                }

                scan_operand(st, idx);
            }
        }
    }

    /* Scan index operands for non-RDA bases (e.g., P1(RDA54)) */
    if (op->kind != OP_RDA && op->indexed && op->index) {
        scan_operand(st, op->index);
    }
}

/*
 * Get integer value from an operand (literal)
 */
static int get_literal_value(Operand *op) {
    if (!op) return -1;
    if (op->kind == OP_LITERAL_NUM) return op->value;
    if (op->kind == OP_LITERAL_STR && op->str_value) return atoi(op->str_value);
    return -1;
}

void symbol_table_scan(SymbolTable *st, Program *prog) {
    for (Instruction *instr = prog->instructions; instr; instr = instr->next) {
        for (int i = 0; i < instr->operand_count; i++) {
            scan_operand(st, &instr->operands[i]);
        }

        /*
         * Track I register comparison bounds from conditional jumps.
         * Pattern: CJGT I1, '5', ... means I1 is compared to 5
         * This suggests I1 reaches up to 5 in loop contexts.
         */
        if (instr->operand_count >= 2) {
            bool is_cond_jump = (instr->mnemonic >= MNEM_CJEQ &&
                                 instr->mnemonic <= MNEM_CJGE);
            if (is_cond_jump) {
                Operand *op1 = &instr->operands[0];
                Operand *op2 = &instr->operands[1];
                int ireg = -1, lit_val = -1;

                /* Check if comparing I register to literal */
                if (op1->kind == OP_REG_I) {
                    ireg = op1->value;
                    lit_val = get_literal_value(op2);
                } else if (op2->kind == OP_REG_I) {
                    ireg = op2->value;
                    lit_val = get_literal_value(op1);
                }

                if (ireg >= 1 && ireg <= 8 && lit_val > 0) {
                    if (lit_val > st->ireg_max[ireg]) {
                        st->ireg_max[ireg] = lit_val;
                    }
                }
            }
        }

        /*
         * Track CLEAR/SAVE range operations for structure inference.
         * These have var_count (number of slots) and operand[0] (start slot).
         */
        if ((instr->mnemonic == MNEM_CLEAR || instr->mnemonic == MNEM_SAVE) &&
            instr->var_count > 1 && instr->operand_count >= 1) {
            Operand *start_op = &instr->operands[0];
            if (start_op->kind == OP_RDA && !start_op->indexed) {
                int start = start_op->value;
                int count = instr->var_count;
                /* Add to range list if not duplicate */
                bool found = false;
                for (int r = 0; r < st->range_count; r++) {
                    if (st->ranges[r].start_slot == start &&
                        st->ranges[r].count == count) {
                        found = true;
                        break;
                    }
                }
                if (!found && st->range_count < 32) {
                    st->ranges[st->range_count].start_slot = start;
                    st->ranges[st->range_count].count = count;
                    st->range_count++;
                    /* Don't extend max_slot here — a bulk CLEAR/SAVE of the
                     * entire RDA space (e.g., CLEAR RDA_FIRST, RDA_LAST) is
                     * an initialization idiom, not a variable declaration.
                     * max_slot is extended later only for ranges that have
                     * confirmed interior access (real structs/arrays). */
                }
            }
        }

        /* Track MOVE_BLOCK src and dst as array ranges */
        if (instr->opcode == OP_MOVE_BLOCK &&
            instr->var_count > 1 && instr->operand_count >= 2) {
            for (int opidx = 0; opidx < 2; opidx++) {
                Operand *op = &instr->operands[opidx];
                if (op->kind == OP_RDA && !op->indexed) {
                    int start = op->value;
                    int count = instr->var_count;
                    bool found = false;
                    for (int r = 0; r < st->range_count; r++) {
                        if (st->ranges[r].start_slot == start &&
                            st->ranges[r].count == count) {
                            found = true;
                            break;
                        }
                    }
                    if (!found && st->range_count < 32) {
                        st->ranges[st->range_count].start_slot = start;
                        st->ranges[st->range_count].count = count;
                        st->range_count++;
                    }
                }
            }
        }
    }

    /*
     * Array killer heuristic: if slot S has indexed access but slot S+1
     * has direct access, then S is not a true dimensioned array
     * (indexed access is just for developer convenience/pointer arithmetic)
     *
     * We only check the immediate next slot to avoid false positives.
     */
    for (int i = 0; i < st->max_slot; i++) {
        if (st->slots[i].is_array && !st->slots[i].bounds_certain) {
            if (st->slots[i + 1].direct_access) {
                /* Slot i+1 is accessed directly, so slot i can't be a true array */
                st->slots[i].is_array = false;
            }
        }
    }

    /*
     * Loop bound inference: if an array uses I register N for indexing,
     * and we saw a comparison of I(N) to a literal value, use that as
     * the likely array bound.
     */
    for (int i = 0; i <= st->max_slot; i++) {
        if (st->slots[i].is_array && !st->slots[i].bounds_certain) {
            int ireg = st->slots[i].index_ireg;
            if (ireg >= 1 && ireg <= 8 && st->ireg_max[ireg] > 0) {
                /* Infer bounds from loop condition */
                st->slots[i].max_index = st->ireg_max[ireg];
                /* Mark as "likely" - not certain, but inferred */
                /* bounds_certain stays false to indicate this is inferred */
            }
        }
    }

    /*
     * RDA space extension: if RDA[i](I_n) with inferred max_index M,
     * the array spans slots i through i+M-1 (1-indexed).
     * Extend max_slot if needed.
     */
    for (int i = 0; i <= st->max_slot; i++) {
        if (st->slots[i].is_array && st->slots[i].max_index > 0) {
            int implied_max = i + st->slots[i].max_index - 1;
            if (implied_max > st->max_slot && implied_max < MAX_RDA_SLOTS) {
                st->max_slot = implied_max;
            }
        }
    }

    /*
     * Struct confirmation heuristic: if a CLEAR/SAVE range has indexed access
     * to an interior slot (not the first slot), then the range MUST be a struct
     * with named members, not a single dimensioned array.
     *
     * Example: CLEAR RDA3 with count=25, and RDA5(I1) seen in bytecode.
     * If RDA3 were RDA3(25), then RDA5 wouldn't exist as a symbol.
     * The presence of RDA5 as an index base proves RDA3 starts a struct.
     */
    for (int r = 0; r < st->range_count; r++) {
        int start = st->ranges[r].start_slot;
        int count = st->ranges[r].count;
        st->ranges[r].is_confirmed_struct = false;

        /* Check interior slots (start+1 to start+count-1) for any named access.
         * If any interior slot is accessed by name (either direct or indexed),
         * then the range MUST be a struct with named members.
         */
        for (int s = start + 1; s < start + count && s < MAX_RDA_SLOTS; s++) {
            if (st->slots[s].is_array || st->slots[s].direct_access) {
                /* Interior slot has named access - this proves struct */
                st->ranges[r].is_confirmed_struct = true;
                break;
            }
        }
    }

    /*
     * Extend max_slot from confirmed CLEAR/SAVE ranges.
     * Only ranges that are confirmed structs (interior slots accessed) or
     * non-struct arrays (where the start slot itself is used) contribute.
     * Bulk init patterns like CLEAR RDA_FIRST, RDA_LAST are skipped.
     */
    for (int r = 0; r < st->range_count; r++) {
        int start = st->ranges[r].start_slot;
        int count = st->ranges[r].count;
        bool start_used = st->slots[start].direct_access || st->slots[start].is_array;
        if (st->ranges[r].is_confirmed_struct || start_used) {
            int range_end = start + count - 1;
            if (range_end > st->max_slot && range_end < MAX_RDA_SLOTS) {
                st->max_slot = range_end;
            }
        }
    }
}
