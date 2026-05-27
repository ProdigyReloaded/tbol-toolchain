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
#ifndef TBOLDC_COD_FILE_H
#define TBOLDC_COD_FILE_H

#include "ir.h"

/*
 * Load a .cod file and parse its header.
 * Returns a Program structure with header info and raw code bytes.
 * Returns NULL on error.
 */
Program *cod_file_load(const char *path);

#endif /* TBOLDC_COD_FILE_H */
