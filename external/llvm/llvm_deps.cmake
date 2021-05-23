#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
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
set(IGC_OPTION__LLVM_LLD ON)

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
