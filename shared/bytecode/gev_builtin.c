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
#include "gev.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int number;
    const char *name;
} GEVBuiltinEntry;

static const GEVBuiltinEntry builtin_gevs[] = {
    /* System GEVs #1-#90 */
    {  1, "SYS_RETURN_CODE" },
    {  2, "SYS_API_EVENT" },
    {  3, "SYS_LOGICAL_KEY" },
    {  5, "SYS_TONE_PULSE" },
    {  6, "SYS_LINE_STATUS" },
    {  7, "SYS_KEYWORD_LENGTH" },
    {  9, "SYS_SCROLL_INCREMENT" },
    { 10, "SYS_CURRENT_FIELD" },
    { 11, "SYS_DATE" },
    { 12, "SYS_TIME" },
    { 13, "SYS_CURRENT_PAGE" },
    { 14, "SYS_SELECTED_OBJ_ID" },
    { 15, "SYS_NAVIGATE_OBJ_ID" },
    { 16, "SYS_SCROLL_ROW" },
    { 17, "SYS_SCROLL_COL" },
    { 18, "SYS_PATH" },
    { 19, "SYS_TTX_PHONE" },
    { 20, "SYS_TOTAL_PAGES" },
    { 21, "SYS_PAGE_NUMBER" },
    { 22, "SYS_BASE_OBJ_ID" },
    { 23, "SYS_WINDOW_ID" },
    { 24, "SYS_PATH_PTR" },
    { 25, "SYS_KEYWORDS" },
    { 26, "SYS_CURRENT_CURSOR_POS" },
    { 27, "SYS_CURRENT_BACKGROUND_COLOR" },
    { 28, "SYS_CURRENT_FOREGROUND_COLOR" },
    { 29, "SYS_HARDWARE_STATUS" },
    { 30, "SYS_NOCOMM" },
    { 31, "SYS_UM_DIA_HEADER" },
    { 32, "SYS_UM_MESSAGE_TEXT" },
    { 33, "SYS_CA_ERROR_TRACK_INFO" },
    { 34, "SYS_ASSISANT_CURRENT_INFO" },
    { 35, "SYS_SCREEN_DATA_TABLE" },
    { 36, "SYS_AD_LIST" },
    { 37, "SYS_CURRENT_KEYWORD" },
    { 38, "SYS_PREVIOUS_KEYWORD" },
    { 39, "SYS_GUIDE" },
    { 40, "SYS_PREVIOUS_MENU" },
    { 41, "SYS_PREVIOUS_SEEN_MENU" },
    { 42, "SYS_SCAN_LIST" },
    { 43, "SYS_SCAN_LIST_POINTER" },
    { 44, "SYS_PATH_NAME" },
    { 45, "SYS_NAVIGATE_KEYWORD" },
    { 46, "KEYWORD_TABLE" },
    { 47, "KEYWORD_DISP" },
    { 48, "KEYWORD_TABLE_ENTRY_LENGTH" },
    { 49, "KEYWORD_LENGTH" },
    { 50, "EXT_TABLE" },
    { 51, "SYS_DATA_COLLECT" },
    { 52, "SYS_FM0_TXHDR" },
    { 53, "SYS_FM0_TXDID" },
    { 54, "SYS_FM0_TXRID" },
    { 55, "SYS_FM4_TXHDR" },
    { 56, "SYS_FM4_TXUSEID" },
    { 57, "SYS_FM4_TXCORID" },
    { 58, "SYS_FM64_TXHDR" },
    { 59, "SYS_FM64_TXDATA" },
    { 60, "SYS_FM0_RXHDR" },
    { 61, "SYS_FM4_RXHDR" },
    { 62, "SYS_FM4_RXUSEID" },
    { 63, "SYS_FM4_RXCORID" },
    { 64, "SYS_FM64_RXHDR" },
    { 65, "SYS_FM64_RXDATA" },
    { 66, "SYS_SURROGATE" },
    { 67, "SYS_LEAVE" },
    { 68, "SYS_ZIP" },
    { 69, "SYS_INT_REGS" },
    { 70, "SYS_TTX_HELP_ID" },
    { 71, "SYS_SELECTOR_DATA" },
    { 72, "SYS_SELECTOR_PATH" },
    { 73, "SYS_LOGICAL_EVENT" },
    { 74, "SYS_USER_ID" },
    { 75, "SYS_HELP_APPL" },
    { 76, "SYS_HELP_HUB_APPL_PTO" },
    { 77, "SYS_ACCESS_KEY_OBJ_ID" },
    { 78, "SYS_WORD_WRAP" },
    { 79, "SYS_MESSAGING_STATUS" },
    { 80, "SYS_VERSION" },
    { 81, "SYS_LEADER_AD_ID" },
    { 82, "SYS_BAUD_RATE" },
    { 83, "SYS_COM_PORT" },
    { 84, "SYS_OBJ_HEADER" },
    { 85, "SYS_SESSION_STATUS" },
    { 86, "SYS_FETCH_RETURN" },
    { 90, "SYS_COMMAND_LINE" },
    /* High system GEVs #31929-#32000 */
    { 31929, "CUG_LOGON_CS" },
    { 31930, "ALT_PHONE_NUMBER" },
    { 31931, "APPL_VERS_CHECK" },
    { 31932, "SYS_LOGON_CHOICE" },
    { 31933, "SYS_DRIVERS_AVAIL" },
    { 31934, "AD_REVIEW_IDS" },
    { 31935, "SYS_ZIP_KEYWORD" },
    { 31936, "SYS_CURRENT_AD_LIST" },
    { 31937, "SYS_APPL_AD_LIST" },
    { 31938, "SYS_APPL_RANDOM" },
    { 31939, "SYS_DEFAULT_RANDOM" },
    { 31980, "TTX_ASST_ZIP_FLAG" },
    { 31981, "TTX_ASST_EVENT" },
    { 31982, "TTX_ASST_ZIPPABLE_PTO" },
    { 31983, "SYS_ZIP_SMPARM" },
    { 31984, "SYS_CURRENT_CATEGORY" },
    { 31985, "SYS_SAFE_PAGE" },
    { 31986, "SYS_SAFE_PAGE_2" },
    { 31987, "SYS_ERROR_TEXT" },
    { 31988, "SYS_QMENU_FLAGS" },
    { 31989, "SYS_REVIEW_KWS" },
    { 31990, "SYS_REVIEW_PTR" },
    { 31991, "STND_MENU_VAR10" },
    { 31992, "STND_MENU_VAR9" },
    { 31993, "STND_MENU_VAR8" },
    { 31994, "STND_MENU_VAR7" },
    { 31995, "STND_MENU_VAR6" },
    { 31996, "STND_MENU_VAR5" },
    { 31997, "STND_MENU_VAR4" },
    { 31998, "STND_MENU_VAR3" },
    { 31999, "STND_MENU_VAR2" },
    { 32000, "STND_MENU_VAR1" },
};

#define BUILTIN_GEV_COUNT (sizeof(builtin_gevs) / sizeof(builtin_gevs[0]))

void gev_table_load_builtin(GEVTable *gt) {
    if (!gt) return;

    for (size_t i = 0; i < BUILTIN_GEV_COUNT; i++) {
        if (gt->count >= gt->capacity) {
            gt->capacity *= 2;
            GEVEntry *tmp = realloc(gt->entries, gt->capacity * sizeof(GEVEntry));
            if (!tmp) { fprintf(stderr, "out of memory\n"); exit(1); }
            gt->entries = tmp;
        }
        gt->entries[gt->count].number = builtin_gevs[i].number;
        gt->entries[gt->count].name = strdup(builtin_gevs[i].name);
        gt->count++;
    }
    gt->loaded = true;
}

/*
 * Non-GEV constants from XXCGTSYS (return codes, key codes, events, etc.)
 * These are DEFINE'd without the # prefix and are needed by the recompiler
 * when the decompiler emits symbolic names.
 */
typedef struct {
    const char *name;
    const char *value;
} XXCGTSYSDefine;

static const XXCGTSYSDefine builtin_defines[] = {
    /* Return codes */
    {"RET_OK", "'0'"},
    {"RET_HARDWARE_ERROR", "'1'"},
    {"RET_TIMEOUT", "'2'"},
    {"RET_BEYOND_EOF", "'3'"},
    {"RET_PRINTER_OUT_OF_PAPER", "'4'"},
    {"RET_OVERFLOW", "'5'"},
    {"RET_UNDERFLOW", "'6'"},
    {"RET_ZERO", "'7'"},
    {"RET_NOT_ZERO", "'8'"},
    {"RET_ALL_ONES", "'9'"},
    {"RET_SOME_ONES", "'10'"},
    {"RET_NO_ONES", "'11'"},
    {"RET_NOT_OPEN", "'13'"},
    {"RET_NOT_FOUND", "'14'"},
    {"RET_DISK_FULL", "'15'"},
    {"RET_BAD_LENGTH", "'16'"},
    {"RET_BAD_POINTER", "'17'"},
    {"RET_END_OF_FILE", "'18'"},
    {"RET_STACK_EMPTY", "'19'"},
    {"RET_STACK_FULL", "'20'"},
    {"RET_NOT_ARRIVED", "'21'"},
    {"RET_ALPHA", "'22'"},
    {"RET_INTEGER", "'23'"},
    {"RET_DECIMAL", "'24'"},
    {"RET_DIVIDE_BY_ZERO", "'25'"},
    {"RET_ALREADY_OPEN", "'26'"},
    {"RET_NOT_ENOUGH_SLOTS", "'27'"},
    /* SET_FUNCTION actions */
    {"NORMAL", "'00'"},
    {"DISABLE", "'16'"},
    {"FILTER", "'32'"},
    {"FILTER_ON", "'64'"},
    /* Misc */
    {"HI", "'1'"},
    {"LO", "'0'"},
    {"INPUT", "'r'"},
    {"OUTPUT", "'w'"},
    {"I_O", "'r+'"},
    {"APPEND", "'a'"},
    {"SYS_DELIMITER", "'~'"},
    /* Physical key codes */
    {"NULL_KEY", "0"},
    {"TEXT_KEY", "1"},
    {"A_KEY", "2"}, {"B_KEY", "3"}, {"C_KEY", "4"}, {"D_KEY", "5"},
    {"E_KEY", "6"}, {"F_KEY", "7"}, {"G_KEY", "8"}, {"H_KEY", "9"},
    {"I_KEY", "10"}, {"J_KEY", "11"}, {"K_KEY", "12"}, {"L_KEY", "13"},
    {"M_KEY", "14"}, {"N_KEY", "15"}, {"O_KEY", "16"}, {"P_KEY", "17"},
    {"Q_KEY", "18"}, {"R_KEY", "19"}, {"S_KEY", "20"}, {"T_KEY", "21"},
    {"U_KEY", "22"}, {"V_KEY", "23"}, {"W_KEY", "24"}, {"X_KEY", "25"},
    {"Y_KEY", "26"}, {"Z_KEY", "27"},
    {"LA_KEY", "28"}, {"LB_KEY", "29"}, {"LC_KEY", "30"}, {"LD_KEY", "31"},
    {"LE_KEY", "32"}, {"LF_KEY", "33"}, {"LG_KEY", "34"}, {"LH_KEY", "35"},
    {"LI_KEY", "36"}, {"LJ_KEY", "37"}, {"LK_KEY", "38"}, {"LL_KEY", "39"},
    {"LM_KEY", "40"}, {"LN_KEY", "41"}, {"LO_KEY", "42"}, {"LP_KEY", "43"},
    {"LQ_KEY", "44"}, {"LR_KEY", "45"}, {"LS_KEY", "46"}, {"LT_KEY", "47"},
    {"LU_KEY", "48"}, {"LV_KEY", "49"}, {"LW_KEY", "50"}, {"LX_KEY", "51"},
    {"LY_KEY", "52"}, {"LZ_KEY", "53"},
    {"N0_KEY", "54"}, {"N1_KEY", "55"}, {"N2_KEY", "56"}, {"N3_KEY", "57"},
    {"N4_KEY", "58"}, {"N5_KEY", "59"}, {"N6_KEY", "60"}, {"N7_KEY", "61"},
    {"N8_KEY", "62"}, {"N9_KEY", "63"},
    {"BACKSPACE_KEY", "64"}, {"INSERT_KEY", "65"}, {"DELETE_KEY", "66"},
    {"CURSOR_UP_KEY", "67"}, {"CURSOR_DOWN_KEY", "68"},
    {"CURSOR_LEFT_KEY", "69"}, {"CURSOR_RIGHT_KEY", "70"},
    {"TAB_LEFT_KEY", "71"}, {"TAB_RIGHT_KEY", "72"},
    {"TAB_UP_KEY", "73"}, {"TAB_DOWN_KEY", "74"},
    {"TAB_FORWARD_KEY", "75"}, {"TAB_BACK_KEY", "76"},
    {"HOME_KEY", "77"}, {"END_KEY", "78"},
    {"SCROLL_UP_KEY", "79"}, {"SCROLL_DOWN_KEY", "80"},
    {"CLOSE_WINDOW_KEY", "81"}, {"COMMIT_KEY", "82"},
    {"HELP_KEY", "83"}, {"NEXT_KEY", "84"}, {"BACK_KEY", "85"},
    {"PATH_KEY", "86"}, {"SCAN_KEY", "87"}, {"SPACE_BAR", "88"},
    {"PAGE_HELP_KEY", "89"}, {"ACTION_KEY", "90"},
    {"VIEWPATH_KEY", "91"}, {"GUIDE_KEY", "92"}, {"INDEX_KEY", "93"},
    {"KEEP_KEY", "94"}, {"PREVIOUS_MENU_KEY", "95"},
    {"UNDO_KEY", "96"}, {"PRT_KEY", "97"}, {"JUMP_KEY", "98"},
    {"RESET_KEY", "255"},
    /* Logical events / trigger functions */
    {"NO_EVENT", "0"},
    {"ADD_TEXT", "1"}, {"ADD_TEXT_AND_FIELD_END", "2"},
    {"BACKSPACE", "3"}, {"DELETE_TEXT", "4"}, {"TOGGLE_INSERT", "5"},
    {"HELP", "6"}, {"INSERT_TEXT", "7"}, {"FIELD_HELP", "8"},
    {"PAGE_HELP", "9"}, {"NAVIGATION", "10"}, {"INTERFIELD_CURSOR", "11"},
    {"NEXT_PAGE", "12"}, {"BACK_PAGE", "13"},
    {"NEXT", "12"}, {"BACK", "13"},
    {"PATH", "14"}, {"JUMP", "15"}, {"ACTION", "16"},
    {"FIELD_END", "17"}, {"ELEMENT_END", "18"}, {"PAGE_END", "19"},
    {"BYE", "20"}, {"LOGON", "21"}, {"VIEWPATH", "22"},
    {"KEYWORD", "23"}, {"GUIDE", "24"}, {"RECALL", "25"}, {"UNDO", "26"},
    {"WHERE", "27"}, {"SRNPRT", "28"}, {"SMFILE", "29"}, {"SCAN", "30"},
    {"TOOLS", "31"}, {"DIRECTORY", "32"}, {"INDEX", "33"},
    {"FIND", "34"}, {"MARK", "34"}, {"LEAVE", "35"}, {"ZIP", "36"},
    {"NEW_LINE", "37"}, {"SCROLL_UP", "38"}, {"SCROLL_DOWN", "39"},
    {"SET_CLOSE_WINDOW", "40"}, {"SET_OPEN_WINDOW", "41"},
    {"CURSOR", "42"}, {"LOGOFF", "43"}, {"LOOK", "44"}, {"TRAVEL", "45"},
    {"FIRST_PAGE", "46"}, {"LAST_PAGE", "47"}, {"PREVIOUS_MENU", "48"},
    {"LOGICAL_FUNCTIONS", "49"},
};

#define BUILTIN_DEFINE_COUNT (sizeof(builtin_defines) / sizeof(builtin_defines[0]))

bool gev_write_xxcgtsys(const char *dir) {
    if (!dir) return false;

    char path[1024];
    snprintf(path, sizeof(path), "%s/XXCGTSYS", dir);

    FILE *f = fopen(path, "w");
    if (!f) return false;

    /* GEV definitions (with # prefix) */
    for (size_t i = 0; i < BUILTIN_GEV_COUNT; i++) {
        fprintf(f, "DEFINE %s ,#%d;\n", builtin_gevs[i].name, builtin_gevs[i].number);
    }

    /* Non-GEV constants */
    for (size_t i = 0; i < BUILTIN_DEFINE_COUNT; i++) {
        fprintf(f, "DEFINE %s ,%s;\n", builtin_defines[i].name, builtin_defines[i].value);
    }

    fclose(f);
    return true;
}
