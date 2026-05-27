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
#ifndef TBOLDC_IR_H
#define TBOLDC_IR_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * Operand kinds
 */
typedef enum {
    OP_NONE,
    OP_REG_I,       /* I1-I8 */
    OP_REG_D,       /* D1-D8 */
    OP_REG_P,       /* P0-P8 */
    OP_RDA,         /* RDA0-RDA221 */
    OP_PEV,         /* &1-&256 */
    OP_GEV,         /* #1-#32000 */
    OP_LITERAL_NUM, /* Small numeric 1-8 */
    OP_LITERAL_STR, /* String literal */
    OP_LITERAL_BIN, /* Binary literal (from hex) */
} OperandKind;

/*
 * Decoded operand
 */
typedef struct Operand {
    OperandKind kind;
    int value;              /* Register number, slot number, or numeric value */
    char *str_value;        /* String literal value (owned) */
    int str_len;            /* String length */
    bool indexed;           /* Has runtime index */
    struct Operand *index;  /* Index operand (owned, if indexed) */
    bool is_hex;            /* Emit as hex (0x01) instead of string ('1') */
} Operand;

/*
 * Instruction mnemonics
 */
typedef enum {
    MNEM_UNKNOWN,
    MNEM_BREAK,
    MNEM_CJEQ, MNEM_CJNE, MNEM_CJLT, MNEM_CJGT, MNEM_CJLE, MNEM_CJGE,
    MNEM_JUMP,
    MNEM_MOVE, MNEM_MOVE_ABS,
    MNEM_ADD, MNEM_SUB, MNEM_MUL, MNEM_DIV,
    MNEM_STRING, MNEM_SUBSTR, MNEM_INSTR, MNEM_LENGTH, MNEM_UPPERCASE,
    MNEM_CALL, MNEM_RETURN, MNEM_EXIT,
    MNEM_CLEAR, MNEM_CLEAR_RANGE,
    MNEM_PUSH, MNEM_POP, MNEM_SWAP,
    MNEM_FILL, MNEM_SORT, MNEM_LOOKUP,
    MNEM_NOTE, MNEM_POINT,
    MNEM_SAVE, MNEM_SAVE_FIELDS, MNEM_RESTORE, MNEM_RELEASE,
    MNEM_MAKE_FORMAT,
    MNEM_DEFINE_FIELD, MNEM_SET_ATTRIBUTE,
    MNEM_TRANSFER, MNEM_LINK,
    MNEM_NAVIGATE, MNEM_SET_FUNCTION,
    MNEM_ERROR,
    MNEM_GOTO_DEPENDING_ON,
    MNEM_SOUND, MNEM_SET_SOUND,
    MNEM_SHOW_SCREEN, MNEM_UPLOAD, MNEM_DOWNLOAD,  /* Patent-only, kept for enum stability */
    MNEM_TRIG_FUNC,
    MNEM_SEND,          /* SEND instruction with trailing bytes */
    /* Add more as needed */
} Mnemonic;

/*
 * Decoded instruction
 */
typedef struct Instruction {
    uint16_t address;       /* Offset from code section start */
    uint16_t length;        /* Instruction length in bytes */
    uint8_t raw_opcode;     /* Original opcode byte */
    uint8_t opcode;         /* Opcode without complex bit */
    bool is_complex;        /* Had 0x80 bit set */
    uint8_t mode_byte;      /* Mode byte (if complex) */

    Mnemonic mnemonic;      /* Decoded mnemonic */
    const char *mnem_str;   /* Mnemonic string for output */

    Operand *operands;      /* Array of operands (owned) */
    int operand_count;

    /* For jump instructions */
    bool has_jump;
    int16_t jump_offset;    /* Raw offset from instruction */
    uint16_t jump_target;   /* Computed absolute address */

    /* For variable-arg instructions */
    int var_count;          /* Count byte value */

    /* For SEND instructions - trailing bytes after operands */
    int16_t send_timeout;   /* 2-byte timeout value (little-endian) */
    uint8_t send_flags;     /* 1-byte flags: PRIORITY=0x04, OPT_HDRS=0x02 */

    struct Instruction *next; /* Linked list */
} Instruction;

/*
 * Program structure
 */
typedef struct {
    char *program_name;
    char *date_time;
    char *version;

    uint16_t code_start;
    uint16_t code_size;
    uint8_t *code;          /* Raw code bytes (owned) */

    Instruction *instructions; /* Linked list of decoded instructions */
    int instr_count;
} Program;

/* Operand functions */
Operand *operand_new(OperandKind kind, int value);
Operand *operand_new_str(const char *str, int len);
Operand *operand_new_indexed(Operand *base, Operand *index);
void operand_free(Operand *op);
char *operand_to_string(Operand *op);

/*
 * Check if a literal operand should be displayed as an unquoted integer.
 * Returns true for string literals that contain only digits (or digits with
 * leading minus), in contexts where the value is clearly numeric:
 * - Destination is an integer register (I1-I8)
 * - Destination is a decimal register (D1-D8)
 * - Arithmetic instruction operands
 * - SUBSTR position/length operands (3rd, 4th)
 * - FILL count operand (3rd)
 * - LOOKUP key length and result position operands
 */
bool operand_is_numeric_literal(Operand *op);

/*
 * Format operand as unquoted integer if it's a numeric literal string.
 * Returns malloc'd string. Falls back to operand_to_string if not numeric.
 */
char *operand_to_numeric_string(Operand *op);

/* Instruction functions */
Instruction *instruction_new(uint16_t address);
void instruction_free(Instruction *instr);
void instruction_add_operand(Instruction *instr, Operand *op);

/* Program functions */
Program *program_new(void);
void program_free(Program *prog);

/*
 * Structure kinds for decompiler
 */
typedef enum {
    STRUCT_NONE,
    STRUCT_IF,              /* Simple IF (no ELSE) */
    STRUCT_IF_ELSE,         /* IF with ELSE */
    STRUCT_ELSE_IF,         /* Part of ELSE IF chain */
    STRUCT_WHILE,           /* WHILE loop header */
    STRUCT_AND_CHAIN,       /* AND chain start */
    STRUCT_OR_CHAIN,        /* OR chain start */
} StructureKind;

/*
 * Basic Block for CFG
 */
typedef struct BasicBlock {
    int id;                     /* Block number */
    uint16_t start_addr;        /* Address of first instruction */
    uint16_t end_addr;          /* Address after last instruction */

    Instruction *first_instr;   /* First instruction in block */
    Instruction *last_instr;    /* Last instruction in block */
    int instr_count;

    /* CFG edges */
    struct BasicBlock *fall_through;   /* Fall-through successor */
    struct BasicBlock *jump_target;    /* Jump target successor */

    struct BasicBlock **predecessors;  /* Array of predecessors */
    int pred_count;
    int pred_cap;

    /* Flags */
    bool is_entry;              /* Entry block */
    bool is_exit;               /* Exit block (has EXIT/RETURN) */
    bool ends_with_jump;        /* Ends with unconditional JUMP */
    bool ends_with_cond;        /* Ends with conditional jump */

    /* Structure recovery */
    StructureKind structure;    /* Identified structure type */
    struct BasicBlock *struct_end;  /* End of structure (for IF/WHILE) */
    struct BasicBlock *else_block;  /* ELSE block (for IF_ELSE) */
    bool is_loop_header;        /* Target of back edge */
    bool is_loop_exit;          /* First block after loop */

    /* Dominator info */
    struct BasicBlock *idom;    /* Immediate dominator */
    int dom_depth;              /* Depth in dominator tree */

    /* Merge point tracking for code deduplication */
    bool is_merge_point;        /* Multiple JUMPs target this block */
    bool emitted;               /* Already emitted in region tree */
} BasicBlock;

/*
 * Control Flow Graph
 */
typedef struct {
    BasicBlock **blocks;        /* Array of basic blocks (owned) */
    int block_count;
    int block_cap;

    BasicBlock *entry;          /* Entry block */

    /* Jump targets for quick lookup */
    uint16_t *targets;          /* Sorted array of jump target addresses */
    int target_count;
} CFG;

/* CFG functions */
CFG *cfg_build(Program *prog);
CFG *cfg_build_range(Program *prog, uint16_t start_addr, uint16_t end_addr);
void cfg_free(CFG *cfg);
BasicBlock *cfg_find_block(CFG *cfg, uint16_t addr);
void cfg_print_dot(CFG *cfg, FILE *out);

/* Dominator analysis */
void cfg_compute_dominators(CFG *cfg);
void cfg_compute_dom_depth(CFG *cfg);
bool cfg_dominates(BasicBlock *a, BasicBlock *b);
void cfg_find_natural_loops(CFG *cfg);


/*
 * Expression tree for boolean expression reconstruction
 */
typedef enum {
    EXPR_AND,           /* left AND right */
    EXPR_OR,            /* left OR right */
    EXPR_LEAF           /* Single condition (comparison) */
} ExprKind;

typedef struct Expr {
    ExprKind kind;
    struct Expr *left;      /* For AND/OR: left operand */
    struct Expr *right;     /* For AND/OR: right operand */
    Instruction *instr;     /* For LEAF: the conditional jump instruction */
} Expr;

Expr *expr_leaf_new(Instruction *instr);
Expr *expr_binary_new(ExprKind kind, Expr *left, Expr *right);
void expr_free(Expr *expr);

#endif /* TBOLDC_IR_H */
