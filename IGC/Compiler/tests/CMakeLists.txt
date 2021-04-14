#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2010-2021 Intel Corporation
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

#
#

if(NOT TARGET igc_opt)
  message("[check-igc] LIT tests disabled. Missing igc_opt target.")
  return()
endif()

set(CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR} ${CMAKE_MODULE_PATH})
include(AddLLVM)

igc_find_external_lit()

# Variables set here are used by `configure_file` call and by
# `add_lit_testsuite` later on.
set(IGC_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(IGC_TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
set(IGC_LIT_CONFIG_FILE ${IGC_TEST_BINARY_DIR}/lit.site.cfg.py)

igc_configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${IGC_LIT_CONFIG_FILE}
  MAIN_CONFIG
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
  )

# If any new tool is required by any of the LIT tests add it here:
set(IGC_LIT_TEST_DEPENDS
  FileCheck
  count
  not
  igc_opt
  )

# This will create a target called `check-igc`, which will run all tests from
# IGC/Compiler/tests directory. The tests will be run on files in the source
# directory, since they are not modified, there doesn't seem to be any reason
# for copying them.
add_lit_testsuite(check-igc "Running the IGC LIT tests"
  ${IGC_TEST_BINARY_DIR}
  DEPENDS ${IGC_LIT_TEST_DEPENDS}
  )

# Tests should not be excluded from "Build Solution" in VS.
set_target_properties(check-igc PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD OFF)

# Line below is just used to group LIT reated targets in single directory
# in IDE. This is completely optional.
set_target_properties(check-igc PROPERTIES FOLDER "LIT Tests")
