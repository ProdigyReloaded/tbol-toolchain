# TBOL Disassembler - tboldasm

add_executable(tboldasm
    tboldasm.c
    $<TARGET_OBJECTS:tbol_bytecode>
)
target_include_directories(tboldasm PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/shared
    ${CMAKE_SOURCE_DIR}/shared/util
)
install(TARGETS tboldasm RUNTIME DESTINATION bin)

# --- Tests ---
add_test(NAME disassembler_tests
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
set_tests_properties(disassembler_tests PROPERTIES
    ENVIRONMENT "TBOLDASM=$<TARGET_FILE:tboldasm>"
)
