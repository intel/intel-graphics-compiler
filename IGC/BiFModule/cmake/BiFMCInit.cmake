#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include(${CMAKE_CURRENT_LIST_DIR}/BiFMCConst.cmake)

file(READ ${BiFModule_SRC_SHA_PATH} BiF_SRC_CHECKSUM)


if(EXISTS "${BiFModule_PREBUILD_SHA_PATH}")
    file(READ "${BiFModule_PREBUILD_SHA_PATH}" BiF_PREBUILD_CHECKSUM)
else()
    set(BiF_PREBUILD_CHECKSUM "Missing")
endif()

message("[IGC\\BiFModuleCache] Source checksum - [${BiF_SRC_CHECKSUM}]")
message("[IGC\\BiFModuleCache] Pre-build checksum - [${BiF_PREBUILD_CHECKSUM}]")

if("${BiF_SRC_CHECKSUM}" STREQUAL "${BiF_PREBUILD_CHECKSUM}")
    message("[IGC\\BiFModuleCache] Checksum equal - using prebuilds from ${BiFModule_PreBuild_PATH}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E remove_directory ${IGC_BUILD__BIF_DIR})
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_directory ${BiFModule_PreBuild_PATH} ${IGC_BUILD__BIF_DIR})
    message("[IGC\\BiFModuleCache] Copied BiFModuleCache to ${IGC_BUILD__BIF_DIR}")
else()
    message("[IGC\\BiFModuleCache] Checksum not equal - building from source")
endif()

file(TOUCH ${BiFModule_Init})
