# TBOL Compiler - tbolc binary + libtbolc static library

# --- Flex/Bison generation (with fallback to committed files) ---
if(FLEX_FOUND AND BISON_FOUND)
    # Generate into binary dir mirroring source layout so
    # #include "parser/tbol.tab.h" works from both source and binary dirs
    set(BISON_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/parser)
    set(FLEX_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/lexer)
    file(MAKE_DIRECTORY ${BISON_OUT_DIR} ${FLEX_OUT_DIR})

    BISON_TARGET(tbol_parser
        ${CMAKE_CURRENT_SOURCE_DIR}/parser/tbol.y
        ${BISON_OUT_DIR}/tbol.tab.c
        DEFINES_FILE ${BISON_OUT_DIR}/tbol.tab.h
    )
    FLEX_TARGET(tbol_lexer
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/tbol.l
        ${FLEX_OUT_DIR}/lex.yy.c
    )
    ADD_FLEX_BISON_DEPENDENCY(tbol_lexer tbol_parser)
    set(GEN_SRCS ${FLEX_tbol_lexer_OUTPUTS} ${BISON_tbol_parser_OUTPUTS})
else()
    # Use committed generated files
    set(GEN_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/lex.yy.c
        ${CMAKE_CURRENT_SOURCE_DIR}/parser/tbol.tab.c
    )
endif()

# --- Compiler sources (everything except main.c) ---
set(COMPILER_SRCS
    options.c
    compile.c
    diag/diag.c
    lexer/preproc.c
    lexer/preproc_event.c
    sema/symtab.c
    sema/sema.c
    codegen/emit.c
    codegen/sdb.c
    codegen/codegen.c
    codegen/codegen_operand.c
    codegen/codegen_control.c
    codegen/codegen_verbs.c
)

# --- Static library (used by decompiler and LSP) ---
add_library(libtbolc STATIC
    ${COMPILER_SRCS}
    ${GEN_SRCS}
    $<TARGET_OBJECTS:tbol_ast>
)
set_target_properties(libtbolc PROPERTIES OUTPUT_NAME tbolc)

target_include_directories(libtbolc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/shared
    # Binary dir for generated parser/lexer headers
    ${CMAKE_CURRENT_BINARY_DIR}
)
# The lexer includes "tokens.h" and other headers relative to lexer/
# and the parser includes headers relative to the compiler root
target_include_directories(libtbolc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
)

# Relax warnings on flex-generated code
if(FLEX_FOUND AND BISON_FOUND)
    set_source_files_properties(${FLEX_tbol_lexer_OUTPUTS} PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-function;-Wno-unknown-pragmas"
    )
else()
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer/lex.yy.c PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-function;-Wno-unknown-pragmas"
    )
endif()

# --- tbolc executable ---
add_executable(tbolc main.c)
target_link_libraries(tbolc PRIVATE libtbolc)
install(TARGETS tbolc RUNTIME DESTINATION bin)

# --- Tests ---
add_test(NAME compiler_tests
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
set_tests_properties(compiler_tests PROPERTIES
    ENVIRONMENT "TBOLC=$<TARGET_FILE:tbolc>"
)
