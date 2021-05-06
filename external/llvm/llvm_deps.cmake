#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#============================ end_copyright_notice =============================

# Top level cmake script for handling LLVM and LLVM-based projects.
# This script will add all LLVM hooks and LLVM script itself. Each of
# these files will handle Source and Prebuilds mode.
#
# There are two kinds of hooks: before LLVM -- source hook -- and
# after LLVM -- prebuilds hook. Source hook handles mode for one LLVM
# project and, if Source mode is picked, registers external LLVM project.
# Prebuild hook handles case when LLVM itself picked Prebuilds mode so
# external projects were not added to build. In this case if project
# wants to build from source it should handle build with prebuilt LLVM.

cmake_policy(VERSION 3.13.4)

include_guard(DIRECTORY)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_MODULE_PATH})
set(IGC_OPTION__LLVM_LLD OFF)

# Get preferred version of LLVM.
include(llvm_preferred_version)
set(IGC_OPTION__LLVM_PREFERRED_VERSION ${DEFAULT_IGC_LLVM_VERSION} CACHE STRING "Preferred version of LLVM to use")

set(IGC_LLVM_WORKSPACE ${CMAKE_CURRENT_BINARY_DIR}/llvm-deps)
set(IGC_LLVM_WORKSPACE_SRC ${IGC_LLVM_WORKSPACE}/src)

# Get useful macros for llvm hooks.
include(llvm_utils)

# Include Source hooks.
# Clang source hook. Currently it unconditionally sets Source mode.
include(llvm_clang_source_hook)

# SPIRV translator source hook.
include(llvm_spirv_source_hook)

# LLD source hook.
if(IGC_OPTION__LLVM_LLD)
  include(llvm_lld_source_hook)
endif()

# Process LLVM.
include(llvm)

# Include prebuild hooks after processing LLVM.
# SPIRV translator prebuilds hook.
include(llvm_spirv_prebuilds_hook)

# Clean up cmake module path from these scripts.
list(REMOVE_AT CMAKE_MODULE_PATH 0)
