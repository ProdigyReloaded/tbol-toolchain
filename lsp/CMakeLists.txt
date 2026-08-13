# TBOL Language Server - tbol-lsp
# Links libtbolc for parsing + tbol_parse for document management

# Third-party cJSON intentionally uses sprintf; macOS SDK 26.x flags it
# as deprecated under sanitizer builds.  Suppress for this one file only.
set_source_files_properties(cJSON.c PROPERTIES
    COMPILE_OPTIONS -Wno-deprecated-declarations)

add_executable(tbol-lsp
    main.c
    server.c
    handlers.c
    jsonrpc.c
    document.c
    cJSON.c
    $<TARGET_OBJECTS:tbol_parse>
    $<TARGET_OBJECTS:tbol_fmt>
)
target_include_directories(tbol-lsp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/formatter
)
target_link_libraries(tbol-lsp PRIVATE libtbolc)
install(TARGETS tbol-lsp RUNTIME DESTINATION bin)

# --- Test harness ---
add_executable(test-lsp
    tests/test_main.c
    tests/test_jsonrpc.c
    tests/test_handlers.c
    tests/test_protocol.c
    tests/test_document.c
    server.c
    handlers.c
    jsonrpc.c
    document.c
    cJSON.c
    $<TARGET_OBJECTS:tbol_parse>
    $<TARGET_OBJECTS:tbol_fmt>
)
target_include_directories(test-lsp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/formatter
)
target_link_libraries(test-lsp PRIVATE libtbolc)
# GCC warns about snprintf in test paths
target_compile_options(test-lsp PRIVATE -Wno-format-truncation)

add_test(NAME lsp_tests COMMAND test-lsp)
