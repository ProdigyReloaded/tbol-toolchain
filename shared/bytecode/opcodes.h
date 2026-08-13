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
#ifndef TBOL_OPCODES_H
#define TBOL_OPCODES_H

/* Control flow */
#define OP_BREAK        0x00
#define OP_CJEQ         0x01
#define OP_CJNE         0x02
#define OP_CJLT         0x03
#define OP_CJGT         0x04
#define OP_CJLE         0x05
#define OP_CJGE         0x06
#define OP_JUMP         0x07

/* Field definition */
#define OP_DEF_FIELD    0x08
#define OP_DEF_FIELD_ST 0x09  /* DEFINE_FIELD with state (7 operands) */
#define OP_SET_ATT      0x0A

/* File I/O
 * READ/WRITE have two forms distinguished by operand count:
 *   OP_READ  (0x0E): READ file, dest;        2 operands - reads a line (PAL read_line)
 *   OP_READ3 (0x0F): READ file, dest, len;   3 operands - reads len bytes (PAL read_rec)
 *   OP_WRITE  (0x10): WRITE file, data;       2 operands - writes data + line terminator (PAL write_line)
 *   OP_WRITE3 (0x11): WRITE file, data, len;  3 operands - writes len bytes (PAL write_rec)
 * In TBOL source, both forms use the READ/WRITE verb; the opcode is selected by
 * the compiler based on operand count (2 or 3).
 * Note: the TBOL patent interpreter source called 0x0F "OP_READ_LINE" and 0x11
 * "OP_WRITE_LINE", but those names are misleading - 0x0F reads a fixed record,
 * not a line. Renamed to OP_READ3/OP_WRITE3 to avoid confusion.
 */
#define OP_OPEN         0x0B
#define OP_CLOSE_ALL    0x0C
#define OP_CLOSE        0x0D
#define OP_READ         0x0E
#define OP_READ3        0x0F
#define OP_WRITE        0x10
#define OP_WRITE3       0x11

/* Communication */
#define OP_CONNECT      0x12
#define OP_DISCONNECT   0x13
#define OP_SEND         0x14
#define OP_SEND_ID      0x15
#define OP_RECEIVE      0x16
#define OP_CANCEL       0x17

/* Navigation */
#define OP_NAVIGATE     0x18
#define OP_NAV_NEXT     0x19
#define OP_NAV_BACK     0x1A
#define OP_NAV_FIRST    0x1B
#define OP_NAV_LAST     0x1C
#define OP_FETCH        0x1D
#define OP_FETCH_RQ     0x1E

/* Window management */
#define OP_OPEN_WINDOW  0x1F
#define OP_OPEN_ERR_WIN 0x20
#define OP_CLOSE_WINDOW 0x21
#define OP_CLOSE_OPEN_WIN 0x22
#define OP_KILL         0x23
#define OP_PURGE        0x24

/* Data manipulation */
#define OP_MOVE         0x25
#define OP_MOVE_ABS     0x26
#define OP_SWAP         0x27
#define OP_ADD          0x28
#define OP_SUB          0x29
#define OP_MUL          0x2A
#define OP_DIV          0x2B
#define OP_DIV_REM      0x2C
#define OP_FILL         0x2D

/* Logical */
#define OP_AND          0x2E
#define OP_OR           0x2F
#define OP_XOR          0x30
#define OP_TEST         0x31

/* String / formatting */
#define OP_LENGTH       0x32
#define OP_FORMAT       0x33
#define OP_MAKE_FORMAT  0x34
#define OP_EDIT         0x35
#define OP_STRING       0x36
#define OP_SUBSTR       0x37
#define OP_INSTR        0x38
#define OP_UPPER        0x39

/* Stack */
#define OP_PUSH         0x3A
#define OP_POP          0x3B

/* Patent-only opcodes: appear in interpreter source but never fully implemented.
 * Kept as comments for reference. */
/* #define OP_SYNC_SAVE    0x3C */
/* #define OP_SYNC_REL     0x3D */
/* #define OP_TIMER_ON     0x3E */

/* Timer / process control */
#define OP_WAIT         0x3F
#define OP_START        0x40
#define OP_STOP         0x41

/* Key / function binding */
#define OP_SET_KEY      0x42
#define OP_SET_KEY_PGM  0x43
#define OP_SET_FUNC     0x44
#define OP_SET_FUNC_PGM 0x45

/* Procedure linkage */
#define OP_CALL         0x46
#define OP_LINK         0x47
#define OP_RETURN       0x48
#define OP_TRANSFER     0x49
#define OP_EXIT         0x4A
#define OP_GO_DEP       0x4B
#define OP_ERROR        0x4C

/* Data persistence */
#define OP_SAVE         0x4D
#define OP_SAVE_RANGE   0x4E
#define OP_RESTORE      0x4F
#define OP_RELEASE      0x50
#define OP_CLEAR        0x51
#define OP_CLEAR_RANGE  0x52

/* Miscellaneous */
#define OP_NOTE         0x53
#define OP_POINT        0x54
#define OP_SOUND        0x55
#define OP_SOUND_ARGS   0x56
#define OP_SORT         0x57
#define OP_LOOKUP       0x58

/* #define OP_SET_BG       0x59 */

#define OP_TRIG_FUNC    0x5A

/* Patent-only opcodes: stubbed or unimplemented in client runtime.
 * Zero usage in the 203-file Prodigy production corpus. */
/* #define OP_FILE_SCREEN  0x5B */
/* #define OP_SHOW_SCREEN  0x5C */
/* #define OP_UPLOAD       0x5D */
/* #define OP_DOWNLOAD     0x5E */
/* #define OP_ACCESS       0x5F */
/* #define OP_NOP          0x60 */
/* #define OP_TIMER_OFF    0x61 */

/* Extended opcodes (0x62+) */
#define OP_MOVE_BLOCK   0x62
#define OP_RETURN_RC    0x63
#define OP_EXIT_RC      0x64
#define OP_REFRESH      0x65
#define OP_ERASE        0x66
#define OP_SET_CURSOR   0x67

/* #define OP_TRACK        0x68 */
/* #define OP_LOG          0x69 */

#define OP_OPEN_ERR_WIN2 0x6A
#define OP_SET_FUNC2    0x6C
#define OP_OPEN_ERR_WIN3 0x6D
#define OP_DELETE       0x6E

/* Complex mode flag */
#define OP_COMPLEX      0x80

#endif /* TBOL_OPCODES_H */
