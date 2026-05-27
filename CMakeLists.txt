cmake_minimum_required(VERSION 3.20)
project(tbol-tools VERSION 4.21 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror -pedantic)
add_compile_definitions(_GNU_SOURCE)

# When consumed via FetchContent (e.g. by reception-system),
# only build the shared libraries, not the tools.
option(BUILD_TBOL_TOOLS "Build tbol compiler, disassembler, decompiler, LSP, formatter" ON)

add_subdirectory(shared)

if(BUILD_TBOL_TOOLS)
    find_package(FLEX)
    find_package(BISON)

    enable_testing()

    add_subdirectory(compiler)
    add_subdirectory(disassembler)
    add_subdirectory(formatter)
    add_subdirectory(decompiler)
    add_subdirectory(lsp)

    # ----------------------------------------------------------------------
    # CPack: produce distributable binary archives via
    #   cmake --build <build-dir> --target package
    # Output files land in <build-dir>/ as:
    #   tbol-tools-<version>-<os>-<arch>.tar.gz   (Unix)
    #   tbol-tools-<version>-<os>-<arch>.zip      (all platforms)
    # Each archive contains a top-level tbol-tools-<version>-<os>-<arch>/
    # directory with bin/{tbolc,tboldc,tboldasm,tbolfmt,tbol-lsp}.
    # ----------------------------------------------------------------------
    set(CPACK_PACKAGE_NAME "tbol-tools")
    set(CPACK_PACKAGE_VENDOR "Prodigy Reloaded")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "TBOL toolchain: compiler, decompiler, disassembler, formatter, language server")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

    # Normalize OS / ARCH names so cross-platform CI artifacts are predictable.
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(_tbol_os "macos")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(_tbol_os "linux")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(_tbol_os "windows")
    else()
        string(TOLOWER "${CMAKE_SYSTEM_NAME}" _tbol_os)
    endif()
    # On macOS honor an explicit single-arch CMAKE_OSX_ARCHITECTURES so a
    # cross-compiled archive gets the right -<arch> suffix (e.g. when you
    # build x86_64 from an arm64 host). Multi-arch lists (universal builds)
    # fall back to the host CMAKE_SYSTEM_PROCESSOR for now.
    list(LENGTH CMAKE_OSX_ARCHITECTURES _osx_arch_count)
    if(_osx_arch_count EQUAL 1)
        list(GET CMAKE_OSX_ARCHITECTURES 0 _tbol_arch)
        string(TOLOWER "${_tbol_arch}" _tbol_arch)
    else()
        string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _tbol_arch)
    endif()
    set(CPACK_PACKAGE_FILE_NAME
        "tbol-tools-${PROJECT_VERSION}-${_tbol_os}-${_tbol_arch}")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)

    if(WIN32)
        set(CPACK_GENERATOR "ZIP")
    else()
        set(CPACK_GENERATOR "TGZ;ZIP")
    endif()

    include(CPack)
endif()
