#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# This code is responsible for applying patches to vc-intrinsics LLVM Wrapper.

cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)

set(IGC_VC_INTRINSICS_PATCHES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/vc-intrinsics/patches")
set(IGC_VC_INTRINSICS_SOURCE_DIR "${BS_DIR_EXTERNAL_COMPONENTS}/cmc/vc-intrinsics")

message(STATUS "[VC-INTRINSICS PATCHER] Applying patches from ${IGC_VC_INTRINSICS_PATCHES_DIR}")

file(GLOB IGC_VC_INTRINSICS_PATCHES "${IGC_VC_INTRINSICS_PATCHES_DIR}/*.patch")

find_program(GIT NAMES git git.cmd)

if(NOT GIT)
    message(FATAL_ERROR "[VC-INTRINSICS PATCHER] Git not found")
endif()

foreach(PATCH_FILE ${IGC_VC_INTRINSICS_PATCHES})
    message(STATUS "[VC-INTRINSICS PATCHER] Applying patch ${PATCH_FILE} to ${IGC_VC_INTRINSICS_SOURCE_DIR}")
    execute_process(
        COMMAND ${GIT} apply --unsafe-paths --directory=${IGC_VC_INTRINSICS_SOURCE_DIR} ${PATCH_FILE}
        RESULT_VARIABLE PATCH_RESULT
    )
    if(NOT PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "[VC-INTRINSICS PATCHER] Failed to apply patch ${PATCH_FILE} to ${IGC_VC_INTRINSICS_SOURCE_DIR}")
    endif()
endforeach()
