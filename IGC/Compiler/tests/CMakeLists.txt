#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#
#

if(NOT TARGET igc_opt)
  message("[check-igc] LIT tests disabled. Missing igc_opt target.")
  return()
endif()
if(NOT IGC_OPTION__ENABLE_LIT_TESTS)
  return()
endif()

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
  "${IGC_BUILD__PROJ__igc_opt}"
  )


# Note:
# This command must be consistent with lit.cfg.py in the following points:
# 1) the path to tests must be consistent with config.test_source_root
# 2) the suffix must be consistent with config.suffixes
# The consistency can be achived by calling a process which returns the list of tests.
file(GLOB_RECURSE _tests "${IGC_TEST_SOURCE_DIR}/*.ll")
if(MSVC)
  source_group(TREE "${IGC_TEST_SOURCE_DIR}" PREFIX "tests" FILES ${_tests})
endif()

# This will create a target called `check-igc`, which will run all tests from
# IGC/Compiler/tests directory. The tests will be run on files in the source
# directory, since they are not modified, there doesn't seem to be any reason
# for copying them.
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
  igc_add_lit_target(check-igc "${IGC_TEST_BINARY_DIR}" "Running the IGC LIT tests"
    DEPENDS ${IGC_LIT_TEST_DEPENDS} ${_tests}
    SOURCES ${_tests} ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
    )
else()
  add_lit_testsuite(check-igc "Running the IGC LIT tests"
    ${IGC_TEST_BINARY_DIR}
    DEPENDS ${IGC_LIT_TEST_DEPENDS}
    )
endif()

# Tests should not be excluded from "Build Solution" in VS.
set_target_properties(check-igc
  PROPERTIES
    EXCLUDE_FROM_DEFAULT_BUILD OFF
    EXCLUDE_FROM_ALL OFF
  )

# Line below is just used to group LIT reated targets in single directory
# in IDE. This is completely optional.
set_target_properties(check-igc PROPERTIES FOLDER "LIT Tests")
