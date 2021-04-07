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
if(NOT TARGET igc_opt OR IGC_BUILD__LLVM_PREBUILDS)
  message("[check-igc] LIT tests disabled. Missing igc_opt target.")
elseif(NOT BS_ENABLE_ULT)
  message("[check-igc] LIT tests disabled. Set `BS_ENABLE_ULT` to enable.")
else()
  # Canonicalization will set undefined variable to false. This step is to make
  # sure that `configure_file` called later on will generate a correct Python
  # script.
  llvm_canonicalize_cmake_booleans(
    LLVM_TOOL_LTO_BUILD
    HAVE_OCAMLOPT
    HAVE_OCAML_OUNIT
    LLVM_INCLUDE_GO_TESTS
    LLVM_USE_INTEL_JITEVENTS
    HAVE_LIBZ
    HAVE_LIBXAR
    LLVM_ENABLE_DIA_SDK
    LLVM_ENABLE_FFI
    ENABLE_SHARED
    ENABLE_ASSERTIONS
    BUILD_SHARED_LIBS
    )

  # Variables set here are used by `configure_file` call and by
  # `add_lit_testsuite` later on.
  set(IGC_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
      CACHE STRING "desc" FORCE)
  set(LLVM_TOOLS_DIR ${CMAKE_CURRENT_BINARY_DIR}/lit-tools
      CACHE STRING "desc" FORCE)
  set(LLVM_LIT_OUTPUT_DIR ${LLVM_TOOLS_DIR}
      CACHE STRING "desc" FORCE)
  set(LLVM_MAIN_SRC_DIR ${LLVM_SOURCE_DIR}
      CACHE STRING "desc" FORCE)
  set(LIT_COMMAND "${PYTHON_EXECUTABLE};${LLVM_MAIN_SRC_DIR}/utils/lit/lit.py"
      CACHE STRING "desc" FORCE)

  # This file is basically used to transfer variables from CMake to LIT.
  set(IGC_LIT_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
    ${IGC_LIT_CONFIG_FILE}
    )

  # If any new tool is required by any of the LIT tests add it here:
  set(IGC_LIT_TEST_DEPENDS
    FileCheck
    llvm-config
    count
    not
    igc_opt
    )

  # This will create a target called `check-igc`, which will run all tests from
  # IGC/Compiler/tests directory. The tests will be run on files in the source
  # directory, since they are not modified, there doesn't seem to be any reason
  # for copying them.
  add_lit_testsuite(check-igc "Running the IGC LIT tests"
    ${CMAKE_CURRENT_SOURCE_DIR}
    PARAMS llvm_site_config=${IGC_LIT_CONFIG_FILE}
    DEPENDS ${IGC_LIT_TEST_DEPENDS}
    )

  # LIT will be using binaires from `LLVM_TOOLS_DIR`. The target below will
  # populate this directory.
  add_custom_command(
    TARGET check-igc
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LLVM_TOOLS_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "\"$<TARGET_FILE:igc_opt>\"" ${LLVM_TOOLS_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "\"$<TARGET_FILE:FileCheck>\"" ${LLVM_TOOLS_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "\"$<TARGET_FILE:llvm-config>\"" ${LLVM_TOOLS_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "\"$<TARGET_FILE:not>\"" ${LLVM_TOOLS_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "\"$<TARGET_FILE:count>\"" ${LLVM_TOOLS_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "\"$<TARGET_FILE_DIR:FileCheck>/llvm-lit.py\"" ${LLVM_TOOLS_DIR}
  )

  # Tests should not be excluded from "Build Solution" in VS.
  set_target_properties(check-igc PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD OFF)

  # Two lines below are just used to group LIT reated targets in single directory
  # in IDE. They are completely optional.
  set_target_properties(check-igc PROPERTIES FOLDER "LIT Tests")
endif()
