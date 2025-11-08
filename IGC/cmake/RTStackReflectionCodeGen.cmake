#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Function to generate RT Stack reflection headers using clang and RTStackReflection tool
#
# This function compiles a C++ reflection file with clang and then processes it with
# RTStackReflection tool to generate public and private header files.
#
# Arguments:
#   SOURCE_FILE - The input C++ file to compile (e.g., reflection.cpp)
#   OUTPUT_DIR - Directory where generated headers will be placed
#   INCLUDE_DIRS - List of include directories for clang
#   DEPENDS - List of dependencies for the generation step
#
function(generate_rtstack_reflection_headers)
    set(options "")
    set(oneValueArgs SOURCE_FILE OUTPUT_DIR SETUP_MODE)
    set(multiValueArgs INCLUDE_DIRS DEPENDS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate arguments
    if(NOT ARG_SOURCE_FILE)
        message(FATAL_ERROR "SOURCE_FILE is required")
    endif()
    if(NOT ARG_OUTPUT_DIR)
        message(FATAL_ERROR "OUTPUT_DIR is required")
    endif()
    if(NOT ARG_SETUP_MODE)
        set(ARG_SETUP_MODE "OPENSOURCE")
    endif()

    # Find clang-tool or clang
    if(NOT TARGET clang-tool)
        message(STATUS "[RTStackReflection] clang-tool target is not set. Looking for clang")
        include(igc_find_opencl_clang)
    endif()

    # Set up mode-specific variables
    set(SETUP_DEFINES "")
    set(MANGLE_NAMES_FLAG "")
    set(DISCARD_VALUE_NAMES "")

    # Set up paths
    get_filename_component(SOURCE_NAME ${ARG_SOURCE_FILE} NAME_WE)
    set(TEMP_BC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_NAME}.bc)
    set(PRIVATE_HEADER_PATH ${ARG_OUTPUT_DIR}/AutoGenRTStackAccessPrivate.h)
    set(PUBLIC_HEADER_PATH ${ARG_OUTPUT_DIR}/AutoGenRTStackAccessPublic.h)

    # Build include directory flags
    set(INCLUDE_FLAGS "")
    foreach(INC_DIR ${ARG_INCLUDE_DIRS})
        list(APPEND INCLUDE_FLAGS -I${INC_DIR})
    endforeach()

    set(WARNING_SETTINGS "")
    if (WIN32)
        set(WARNING_SETTINGS -pedantic -Wall -Werror)
    endif()

    set(CLANG_HEADERS ${IGC_BUILD__GFX_DEV_SRC_DIR}/external/llvm/releases/${IGC_BUILD__CLANG_VERSION}/clang/lib/Headers)

    # Common clang options
    set(CLANG_OPTIONS
        -target x86_64-pc-windows
        ${WARNING_SETTINGS}
        -Wno-return-type-c-linkage
        -std=c++17
        -emit-llvm
        -c
        -x c++
        -I ${CLANG_HEADERS}
        ${INCLUDE_FLAGS}
        -O2
        -g0
        -fno-strict-aliasing
        -fno-slp-vectorize
        -ffp-contract=off
        ${DISCARD_VALUE_NAMES}
    )

    # Step 1: Compile the source file to LLVM bitcode
    add_custom_command(
        OUTPUT ${TEMP_BC_PATH}
        COMMAND
            $<TARGET_FILE:clang-tool>
            ${CLANG_OPTIONS}
            ${SETUP_DEFINES}
            ${ARG_SOURCE_FILE}
            -o ${TEMP_BC_PATH}
        COMMENT "[RTStackReflection] Compiling ${ARG_SOURCE_FILE}"
        DEPENDS ${ARG_SOURCE_FILE} ${ARG_DEPENDS}
        VERBATIM
    )

    # Step 2: Generate private header
    add_custom_command(
        OUTPUT ${PRIVATE_HEADER_PATH}
        COMMAND
            $<TARGET_FILE:RTStackReflection>
            --scope=private
            ${MANGLE_NAMES_FLAG}
            ${TEMP_BC_PATH}
            ${PRIVATE_HEADER_PATH}
        COMMENT "[RTStackReflection] Generating ${PRIVATE_HEADER_PATH}"
        DEPENDS ${TEMP_BC_PATH} RTStackReflection
        VERBATIM
    )

    # Step 3: Generate public header
    add_custom_command(
        OUTPUT ${PUBLIC_HEADER_PATH}
        COMMAND
            $<TARGET_FILE:RTStackReflection>
            --scope=public
            ${MANGLE_NAMES_FLAG}
            ${TEMP_BC_PATH}
            ${PUBLIC_HEADER_PATH}
        COMMENT "[RTStackReflection] Generating ${PUBLIC_HEADER_PATH}"
        DEPENDS ${TEMP_BC_PATH} RTStackReflection
        VERBATIM
    )

    # Set output variables in parent scope so the caller can reference the generated headers
    set(RTSTACK_GENERATED_HEADERS
        ${PRIVATE_HEADER_PATH}
        ${PUBLIC_HEADER_PATH}
        PARENT_SCOPE
    )

endfunction()
