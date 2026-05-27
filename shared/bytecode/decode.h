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
#ifndef TBOLDC_DECODE_H
#define TBOLDC_DECODE_H

#include "ir.h"

/*
 * Decode all instructions in a program's code section.
 * Populates prog->instructions linked list.
 * Returns 0 on success, -1 on error.
 */
int decode_program(Program *prog);

/*
 * Get mnemonic string for an instruction
 */
const char *mnemonic_str(Mnemonic m);

#endif /* TBOLDC_DECODE_H */
