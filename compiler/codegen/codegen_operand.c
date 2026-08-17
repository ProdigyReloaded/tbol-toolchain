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
 * TBOL Compiler - Operand Encoding
 */

#include "codegen_internal.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*
 * Compute operand value for a simple (non-indexed) expression
 * Returns the 1-byte or 2-byte value to emit
 */
int compute_operand_value(AstNode *node) {
    if (!node) return 0;

    switch (node->kind) {
        case AST_REG_I:
            return 8 + node->data.reg.number;  /* I1=9, I2=10, ... I8=16 */

        case AST_REG_D:
            return 16 + node->data.reg.number; /* D1=17, D2=18, ... D8=24 */

        case AST_REG_P:
            return 25 + node->data.reg.number; /* P0=25, P1=26, ... P8=33 */

        case AST_IDENT: {
            Symbol *sym = symtab_lookup_var(node->data.ident.name);
            if (sym) {
                /* RDA slot encoding:
                 * Slots 0-157 -> simple encoding: 34-191 (0x22-0xBF)
                 * Slots 158-221 -> extended encoding: 256-319 (0x100-0x13F)
                 */
                int slot = VAR_SLOT(sym);
                if (slot <= 157) {
                    return 34 + slot;
                } else {
                    return 256 + (slot - 158);
                }
            }
            /* Check if it's a structure group name (e.g., dt) */
            StructureGroup *sg = symtab_lookup_structure(node->data.ident.name);
            if (sg) {
                /* Return the slot for the structure's starting RDA */
                int slot = sg->start_slot;
                if (slot <= 157) {
                    return 34 + slot;
                } else {
                    return 256 + (slot - 158);
                }
            }
            return 0;
        }

        case AST_PEV: {
            int num = node->data.ext_var.number;
            if (num <= 64) {
                return 191 + num;  /* &1=192, &64=255 */
            } else {
                return 255 + num;  /* &65=320, ... */
            }
        }

        case AST_GEV: {
            int num = node->data.ext_var.number;
            return 0x200 + num - 1;  /* #1=0x200, #2=0x201, ... */
        }

        case AST_RDA_SLOT: {
            /* RDA slot encoding:
             * Slots 0-157 -> simple: 34-191 (0x22-0xBF)
             * Slots 158-221 -> extended: 256-319 (0x100-0x13F)
             */
            int slot = node->data.ext_var.number;
            if (slot <= 157) {
                return 34 + slot;
            } else {
                return 256 + (slot - 158);
            }
        }

        default:
            return 0;
    }
}

/*
 * Compute operand value for the end of a range (SAVE_RANGE, CLEAR_RANGE).
 * For array variables, resolves to the last element (base + size - 1)
 * instead of the base slot. For all other operand types, behaves
 * identically to compute_operand_value().
 *
 * Verified against production bytecode: when the end operand of SAVE_RANGE
 * or CLEAR_RANGE is an array variable A(N), the original compiler encodes
 * it as the slot of A's last element (base_slot + N - 1).
 */
int compute_range_end_value(AstNode *node) {
    if (!node) return 0;
    if (node->kind == AST_IDENT) {
        Symbol *sym = symtab_lookup_var(node->data.ident.name);
        if (sym && sym->data.var.array_size > 0) {
            int slot = VAR_SLOT(sym) + sym->data.var.array_size - 1;
            if (slot <= 157) {
                return 34 + slot;
            } else {
                return 256 + (slot - 158);
            }
        }
        StructureGroup *sg = symtab_lookup_structure(node->data.ident.name);
        if (sg) {
            int slot = sg->start_slot + sg->count - 1;
            if (slot <= 157) {
                return 34 + slot;
            } else {
                return 256 + (slot - 158);
            }
        }
    }
    return compute_operand_value(node);
}

/*
 * Check if operand needs complex (extended) encoding
 */
bool needs_extended_encoding(AstNode *node) {
    if (!node) return false;

    /* Indexed access needs complex mode only for DYNAMIC index */
    if (node->kind == AST_INDEXED) {
        AstNode *index = node->children[1];
        /* Constant index is resolved at compile time - no complex mode needed */
        if (index->kind == AST_LITERAL_NUM) {
            /* Check if resolved value > 255 */
            AstNode *base = node->children[0];
            int idx = atoi(index->data.num_lit.text);

            /* Compute resolved slot for each base type */
            switch (base->kind) {
                case AST_REG_I:
                case AST_REG_D:
                case AST_REG_P:
                    return false;  /* I, D, P registers always < 256 */
                case AST_PEV: {
                    int num = base->data.ext_var.number + idx - 1;
                    return num > 64;
                }
                case AST_GEV:
                    return true;  /* GEV always needs extended */
                case AST_IDENT: {
                    Symbol *sym = symtab_lookup_var(base->data.ident.name);
                    if (sym) {
                        int slot = VAR_SLOT(sym) + idx - 1;
                        return slot > 157;  /* Extended encoding for slots 158-221 */
                    }
                    return false;
                }
                case AST_RDA_SLOT: {
                    int slot = base->data.ext_var.number + idx - 1;
                    return slot > 157;  /* Extended encoding for slots 158-221 */
                }
                default:
                    return false;
            }
        }
        /* Dynamic index always needs complex mode */
        return true;
    }

    int value = compute_operand_value(node);
    return value > 255;
}

/*
 * Emit a resolved RDA slot number as an operand.
 *
 * Slots 0-157 use the simple one-byte form (34 + slot); 158 and above use
 * the two-byte extended form, matching emit_struct_clear_save() and what
 * needs_extended_encoding() promises the caller for these operands. Getting
 * this wrong is not a mis-addressed slot but a desynchronised instruction
 * stream, since the operand is also a byte short.
 */
static void emit_rda_slot_operand(int slot) {
    if (slot > 157) {
        emit_word_be(256 + (slot - 158));
    } else {
        emit_byte(34 + slot);
    }
}

/*
 * Emit an operand
 */
void emit_operand(AstNode *node, bool use_extended) {
    if (!node) return;

    /* Handle literals first */
    if (node->kind == AST_LITERAL_STR) {
        emit_string_n(node->data.str_lit.value, node->data.str_lit.length);
        return;
    }

    if (node->kind == AST_LITERAL_NUM) {
        /* Emit numeric as string */
        emit_string(node->data.num_lit.text);
        return;
    }

    if (node->kind == AST_LITERAL_HEX) {
        /* Emit hex literal as binary bytes with length prefix.
         * The text is like "0x58584F50..." - we decode each pair of hex
         * digits to a byte. This is how TBOL developers embedded binary
         * data (like program names with trailing bytes) in DEFINE statements.
         */
        const char *hex_str = node->data.num_lit.text;
        if (hex_str[0] == '0' && (hex_str[1] == 'x' || hex_str[1] == 'X')) {
            hex_str += 2;  /* Skip "0x" prefix */
        }

        /* Count hex digits and calculate byte count */
        int hex_len = strlen(hex_str);
        int byte_count = (hex_len + 1) / 2;  /* Round up for odd lengths */

        /* Emit length prefix (0x00 marker + length byte) */
        emit_byte(0x00);
        emit_byte((uint8_t)byte_count);

        /* Decode and emit hex pairs as bytes.
         * For odd-length hex strings, left-pad with a zero nibble
         * so "38000b9000a" becomes "038000b9000a" (pairs from left).
         */
        int start = 0;
        if (hex_len % 2 != 0) {
            /* Odd length: first byte is just the first nibble */
            char hex_pair[3] = {hex_str[0], 0, 0};
            emit_byte((uint8_t)strtol(hex_pair, NULL, 16));
            start = 1;
        }
        for (int i = start; i < hex_len; i += 2) {
            char hex_pair[3] = {hex_str[i], hex_str[i + 1], 0};
            uint8_t byte_val = (uint8_t)strtol(hex_pair, NULL, 16);
            emit_byte(byte_val);
        }
        return;
    }

    /* Handle indexed access */
    if (node->kind == AST_INDEXED) {
        AstNode *base = node->children[0];
        AstNode *index = node->children[1];

        /* Check if index is constant - resolve at compile time */
        if (index->kind == AST_LITERAL_NUM) {
            int idx = atoi(index->data.num_lit.text);

            /* Compute resolved operand */
            switch (base->kind) {
                case AST_REG_I:
                    /* I1(2) -> I2 */
                    emit_byte(8 + idx);
                    return;
                case AST_REG_D:
                    emit_byte(16 + idx);
                    return;
                case AST_REG_P:
                    emit_byte(25 + idx);
                    return;
                case AST_PEV: {
                    int num = base->data.ext_var.number + idx - 1;
                    if (num <= 64) {
                        emit_byte(191 + num);
                    } else {
                        emit_word_be(255 + num);
                    }
                    return;
                }
                case AST_GEV: {
                    int num = base->data.ext_var.number + idx - 1;
                    emit_word_be(0x200 + num - 1);
                    return;
                }
                case AST_IDENT: {
                    Symbol *sym = symtab_lookup_var(base->data.ident.name);
                    if (sym) {
                        emit_rda_slot_operand(VAR_SLOT(sym) + idx - 1);
                    }
                    return;
                }
                case AST_RDA_SLOT: {
                    /* RDA_FIRST(n) -> slot 0+(n-1), RDA_LAST(n) -> slot 221+(n-1) */
                    emit_rda_slot_operand(base->data.ext_var.number + idx - 1);
                    return;
                }
                default:
                    break;
            }
        }

        /* Dynamic index - emit base with high bit set, then index */
        int base_val = compute_operand_value(base);
        emit_word_be(base_val | 0x8000);

        /* Dynamic index uses linear single-byte encoding (34 + slot),
         * not the extended 2-byte encoding that compute_operand_value
         * returns for RDA slots >= 158 */
        int idx_val;
        if (index->kind == AST_IDENT) {
            Symbol *sym = symtab_lookup_var(index->data.ident.name);
            if (sym) {
                idx_val = 34 + VAR_SLOT(sym);
            } else {
                idx_val = compute_operand_value(index);
            }
        } else {
            idx_val = compute_operand_value(index);
        }
        emit_byte(idx_val);
        return;
    }

    /* Simple operand */
    int value = compute_operand_value(node);
    if (use_extended || value > 255) {
        emit_word_be(value);
    } else {
        emit_byte(value);
    }
}
