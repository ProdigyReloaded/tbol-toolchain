# TBOL Decompiler - tboldc
# Links: bytecode + formatter + libtbolc (for recompilation/verification)

add_executable(tboldc
    main.c
    baseline.c
    operand_fmt.c
    structure.c
    verify.c
    $<TARGET_OBJECTS:tbol_bytecode>
    $<TARGET_OBJECTS:tbol_fmt>
)
target_include_directories(tboldc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/shared
    ${CMAKE_SOURCE_DIR}/shared/util
    ${CMAKE_SOURCE_DIR}/formatter
)
target_link_libraries(tboldc PRIVATE libtbolc)
install(TARGETS tboldc RUNTIME DESTINATION bin)

# --- Tests ---

# Self-contained: decompile compiler golden .cod files, verify round-trip
add_test(NAME decompiler_tests
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
set_tests_properties(decompiler_tests PROPERTIES
    ENVIRONMENT "TBOLDC=$<TARGET_FILE:tboldc>"
)
