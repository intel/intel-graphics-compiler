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
