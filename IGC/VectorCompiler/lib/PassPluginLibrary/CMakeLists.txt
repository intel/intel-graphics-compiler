#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

if((${LLVM_VERSION_MAJOR} LESS 16))
    return()
endif()

if (NOT LLVM_ON_UNIX)
    return()
endif()

# TODO: Fix ext
set(LLVM_PLUGIN_EXT ".so")

add_llvm_library(
  NewPMPlugin
  COMPONENT_LIB
  Plugin.cpp

  $<TARGET_OBJECTS:VCCodeGen>
  $<TARGET_OBJECTS:VCSupport>
  $<TARGET_OBJECTS:LLVMGenXIntrinsics>
  $<TARGET_OBJECTS:VCGenXUtils>
  $<TARGET_OBJECTS:VCTransforms>
  $<TARGET_OBJECTS:VCGeneralUtils>
  $<TARGET_OBJECTS:VCInternalIntrinsics>
  $<TARGET_OBJECTS:${IGC_BUILD__PROJ__BiFManager}>
  $<TARGET_OBJECTS:${IGC_BUILD__PROJ__BiFLib_OCL}>
  MODULE
  PLUGIN_TOOL opt
)

# TODO: Fix path
target_include_directories(NewPMPlugin
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/"
)
