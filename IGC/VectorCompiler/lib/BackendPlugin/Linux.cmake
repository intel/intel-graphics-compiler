#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Enable only for make or ninja generators to use RULE_LAUNCH_LINK.
if(NOT (CMAKE_GENERATOR MATCHES "^(Unix Makefiles|Ninja)"))
  return()
endif()

set(BACKEND_PLUGIN_SOURCES
  BackendPlugin.cpp
  )

add_library(VCBackendPlugin
  MODULE
  ${BACKEND_PLUGIN_SOURCES}
  )

set(VCLinkBEPluginScript
  ${CMAKE_CURRENT_SOURCE_DIR}/LinkBackendPlugin.py
  )

set_target_properties(VCBackendPlugin
  PROPERTIES LINK_DEPENDS ${VCLinkBEPluginScript}
  )

set_target_properties(VCBackendPlugin PROPERTIES
  RULE_LAUNCH_LINK "${PYTHON_EXECUTABLE} ${VCLinkBEPluginScript}"
  )

target_link_libraries(VCBackendPlugin
  PRIVATE
  VCCodeGen
  )
