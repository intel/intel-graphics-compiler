#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

add_subdirectory ("source")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/source")
add_library(zebinlib STATIC ${ZE_INFO_SOURCE_FILE} ${ZE_INFO_INCLUDE_FILE})
add_dependencies(zebinlib intrinsics_gen)

set(ZEBIN_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/source/autogen")

include_directories("${ZEBIN_INCLUDE_PATH}")


if(MSVC)
    set(ZEBIN_COMPILER_OPTIONS
        /w44141
        /w44146
        /w44244
    )
    target_compile_options(zebinlib PRIVATE ${ZEBIN_COMPILER_OPTIONS})
endif()
