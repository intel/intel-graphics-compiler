#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

cmake_minimum_required (VERSION 3.8)

set (ZEInfoReader_Source
     main.cpp
     Tester.cpp
     ZEInfoReader.cpp
)

set (ZEInfoReader_Header
     Tester.hpp
     ZEInfoReader.h
)

# Add source to this project's executable.
add_executable (ZEInfoReader ${ZEInfoReader_Source} ${ZEInfoReader_Header})

include_directories("../zebin/source")
set(ZEBIN_INCLUDE "../zebin/source/autogen")

include_directories("${ZEBIN_INCLUDE}")

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core object)

# Link against LLVM libraries
target_link_libraries(ZEInfoReader zebinlib ${llvm_libs})

if(MSVC)
    target_compile_options(ZEInfoReader PRIVATE
                           $<$<CONFIG:Debug>: ${VS_DEBUG_COMPILER_OPTIONS}>
    )
endif()
