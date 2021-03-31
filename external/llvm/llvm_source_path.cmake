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

# Use LLVM sources stored at fixed location
list(APPEND IGC_LLVM_PATHS
  ${CMAKE_CURRENT_SOURCE_DIR}/../../llvm-project
  /opt/src/llvm-project_${IGC_OPTION__LLVM_PREFERRED_VERSION}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../llvm-project_${IGC_OPTION__LLVM_PREFERRED_VERSION}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../llvm-project_${IGC_OPTION__LLVM_PREFERRED_VERSION}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../llvm-project_${IGC_OPTION__LLVM_PREFERRED_VERSION}
  )

find_path(DEFAULT_IGC_LLVM_SOURCES_DIR
  README.md
  PATHS ${IGC_LLVM_PATHS}
  NO_DEFAULT_PATH
  )
