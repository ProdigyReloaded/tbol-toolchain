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

# CALL-target recovery: mid-instruction CALL targets recover as _invalid_target
add_test(NAME decompiler_call_recovery
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/call_recovery.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
set_tests_properties(decompiler_call_recovery PROPERTIES
    ENVIRONMENT "TBOLDC=$<TARGET_FILE:tboldc>"
)

# Fold transforms: OR guards and else-if ladders fold to source shape
add_test(NAME decompiler_fold_transforms
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/fold_transforms.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
set_tests_properties(decompiler_fold_transforms PROPERTIES
    ENVIRONMENT "TBOLDC=$<TARGET_FILE:tboldc>"
)
