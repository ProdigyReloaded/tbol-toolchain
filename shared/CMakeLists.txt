# Shared TBOL libraries used by multiple tools and by reception-system

# AST library (used by compiler and LSP)
add_library(tbol_ast OBJECT ast.c)
target_include_directories(tbol_ast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Parse wrapper (used by LSP)
add_library(tbol_parse OBJECT tbol_parse.c)
target_include_directories(tbol_parse PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Bytecode library (used by disassembler, decompiler, reception-system)
add_library(tbol_bytecode OBJECT
    bytecode/cod_file.c
    bytecode/decode.c
    bytecode/ir.c
    bytecode/gev.c
    bytecode/gev_builtin.c
    bytecode/proc.c
    bytecode/symtab_analysis.c
    util/md5.c
    util/memfile.c
    util/source_header.c
)
target_include_directories(tbol_bytecode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/bytecode
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)
