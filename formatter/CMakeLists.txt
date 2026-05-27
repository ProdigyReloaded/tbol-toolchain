# TBOL Formatter — tbol_fmt object library + tbolfmt standalone binary

add_library(tbol_fmt OBJECT tbol_fmt.c)
target_include_directories(tbol_fmt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(tbolfmt main.c $<TARGET_OBJECTS:tbol_fmt>)
install(TARGETS tbolfmt RUNTIME DESTINATION bin)

# --- Tests ---
add_test(NAME formatter_tests
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
set_tests_properties(formatter_tests PROPERTIES
    ENVIRONMENT "TBOLFMT=$<TARGET_FILE:tbolfmt>"
)
