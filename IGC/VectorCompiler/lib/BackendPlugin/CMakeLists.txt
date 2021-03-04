#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2020-2021 Intel Corporation
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

set(BACKEND_PLUGIN_SOURCES
  BackendPlugin.cpp
  )

add_library(VCBackendPlugin
  MODULE
  ${BACKEND_PLUGIN_SOURCES}
  )

# Hack to avoid transitive LLVM dependencies that will break
# plugin because of duplicate global variables.
# 'CODEGEN_LIBS' will consist of 'VCCodeGen' and all its direct dependencies.
# This should be enough for now. In case of indirect dependencies on VectorCompiler
# libraries, searching algorithm needs to be improved.
# 'CODEGEN_LIBS_FILES' will be expanded to plain library names so cmake
# will not add any transitive dependencies when target is linked against them.
get_target_property(CODEGEN_LIBS VCCodeGen LINK_LIBRARIES)
set(CODEGEN_LIBS VCCodeGen ${CODEGEN_LIBS})
foreach(target ${CODEGEN_LIBS})
  # Filter out interface libraries -- these will not produce any files.
  set(tgt_type "$<TARGET_PROPERTY:${target},TYPE>")
  set(is_iface "$<STREQUAL:${tgt_type},INTERFACE_LIBRARY>")
  set(not_iface "$<NOT:${is_iface}>")

  set(tgt_file "$<TARGET_FILE:${target}>")
  set(whole_gen "$<${not_iface}:${tgt_file}>")
  list(APPEND CODEGEN_LIBS_FILES ${whole_gen})
endforeach()

# Cmake also does not add any dependencies for libraries
# that are linked this way.
add_dependencies(VCBackendPlugin
  ${CODEGEN_LIBS}
  )

target_link_libraries(VCBackendPlugin
  PRIVATE
  VCHeaders
  ${CODEGEN_LIBS_FILES}
  # GenX_IR actually should be linked to LLVMGenXCodeGen.
  GenX_IR
  )
