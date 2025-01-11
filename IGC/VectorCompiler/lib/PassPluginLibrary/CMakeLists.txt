#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024-2025 Intel Corporation
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

add_llvm_library(NewPMPlugin
  COMPONENT_LIB Plugin.cpp
  MODULE
  PLUGIN_TOOL opt
  )

target_link_libraries(NewPMPlugin
  PRIVATE
  VCCodeGen
  "${IGC_BUILD__WHOLE_ARCHIVE}"
  "${IGC_BUILD__PROJ__BiFLib_OCL}"
  "${IGC_BUILD__NO_WHOLE_ARCHIVE}"
  )

set(VCLinkBEPluginScript
  ${CMAKE_CURRENT_SOURCE_DIR}/../BackendPlugin/LinkBackendPlugin.py
  )

set_target_properties(NewPMPlugin
  PROPERTIES
  LINK_DEPENDS ${VCLinkBEPluginScript}
  )

set_target_properties(NewPMPlugin
  PROPERTIES
  RULE_LAUNCH_LINK "${PYTHON_EXECUTABLE} ${VCLinkBEPluginScript}"
  )

target_include_directories(NewPMPlugin
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../GenXCodeGen/"
  "${CMAKE_CURRENT_BINARY_DIR}/../../lib/GenXCodeGen/"
  )
