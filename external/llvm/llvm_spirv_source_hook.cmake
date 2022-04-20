#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Handle spirv translator library.
# Main variable for SPIRV translator handling is:
#  IGC_OPTION__SPIRV_TRANSLATOR_MODE -- SPIRV translator usage model. Can be
#  Source, Prebuilds or empty. If it is empty, then sources are tried first,
#  then fallback to prebuilds is performed is no sources are available.
#
# Prebuilds mode is handled in IGC and uses SPIRVLLVMTranslator_ROOT variable
# to find prebuild package. Here only minimal processing is done to ensure that
# sources are not found or not needed.
#
# Source mode is handled in this file by registering llvm hook for adding external
# projects. If Source mode was selected or SPIRV sources were found during standard
# search procedure then project will be register and then handled by IGC-LLVM cmake.
# Source mode has suboption:
#  IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR -- path to spirv translator sources.

include_guard(DIRECTORY)

option(IGC_OPTION__LINK_KHRONOS_SPIRV_TRANSLATOR "Enable link against Khronos SPIRV-LLVM-Translator, instead of legacy versions" ON)


if(IGC_OPTION__LINK_KHRONOS_SPIRV_TRANSLATOR)

llvm_define_mode_variable(SPIRV IGC_OPTION__SPIRV_TRANSLATOR_MODE)

# Guess default spirv translator path.
include(llvm_spirv_source_path)

# Handle dependent options for Source mode.
if(IGC_OPTION__SPIRV_TRANSLATOR_MODE STREQUAL SOURCE_MODE_NAME)
  set(IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR "${DEFAULT_SPIRV_TRANSLATOR_SOURCE_DIR}" CACHE PATH "Path to SPIRV translator sources")
  # Tell the build that we are using sources.
  set(IGC_BUILD__SPIRV_TRANSLATOR_SOURCES ON)
endif()

# Handle dependent options for Prebuild mode.
if(IGC_OPTION__SPIRV_TRANSLATOR_MODE STREQUAL PREBUILDS_MODE_NAME)
  message(STATUS "[SPIRV] Will use SPIRV translator prebuilds")
  # Set spirv translator source dir, as it is needed when using prebuild translator later
  set(IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR "${DEFAULT_SPIRV_TRANSLATOR_SOURCE_DIR}" CACHE PATH "Path to SPIRV translator sources")
  return()
endif()

# No mode was specified, start searching.
if(NOT IGC_OPTION__SPIRV_TRANSLATOR_MODE)
  message(STATUS "[SPIRV] Using default procedure to determine SPIRV mode")
  # OpenCL Clang and Vector Compiler share the library Spirv Translator, so we cannot build Spriv Translator
  # if OpenCL Clang is taken from the system.
  if(IGC_TRY_TO_TAKE_CLANG_FROM_SYSTEM)
    message(STATUS "[SPIRV] No mode was specified for SPIRV. Clang is trying to be taken from the system, so the SPIRV translator not will be built from the source")
    return()
  endif()
  if(EXISTS "${DEFAULT_SPIRV_TRANSLATOR_SOURCE_DIR}")
    set(IGC_BUILD__SPIRV_TRANSLATOR_SOURCES ON)
    set(IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR ${DEFAULT_SPIRV_TRANSLATOR_SOURCE_DIR})
  else()
    return()
  endif()
endif()

message(STATUS "[SPIRV] SPIRV translator will be built from sources")
message(STATUS "[SPIRV] Using SPIRV sources: ${IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR}")

# Only sources are left here. Other code paths executed return.
if(NOT EXISTS "${IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR}")
  message(FATAL_ERROR
    "[SPIRV] Cannot find SPIRV translator sources, please provide sources path by IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR flag")
endif()

register_llvm_external_project(spirv ${IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR})

endif()
