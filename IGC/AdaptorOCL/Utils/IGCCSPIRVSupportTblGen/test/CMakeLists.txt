#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

if(NOT IGC_OPTION__ENABLE_LIT_TESTS)
  return()
endif()

set(SPIRV_TBLGEN_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SPIRV_TBLGEN_TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
set(SPIRV_TBLGEN_LIT_CONFIG_FILE ${SPIRV_TBLGEN_TEST_BINARY_DIR}/lit.site.cfg.py)
set(SPIRV_EXTENSIONS_INC_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ocl_igc_interface/extensions)

igc_configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${SPIRV_TBLGEN_LIT_CONFIG_FILE}
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
  )

set(SPIRV_TBLGEN_TEST_DEPENDS
  FileCheck
  not
  igcc-spirv-support-tblgen
  )

file(GLOB_RECURSE _tests "${SPIRV_TBLGEN_TEST_SOURCE_DIR}/*.td")

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
  igc_add_lit_target(check-spirv-tblgen "${SPIRV_TBLGEN_TEST_BINARY_DIR}"
    "Running the SPIR-V extension support tblgen regression tests"
    DEPENDS ${SPIRV_TBLGEN_TEST_DEPENDS} ${_tests}
    SOURCES ${_tests} ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
    )
else()
  add_lit_testsuite(check-spirv-tblgen
    "Running the SPIR-V extension support tblgen regression tests"
    ${SPIRV_TBLGEN_TEST_BINARY_DIR}
    DEPENDS
    ${SPIRV_TBLGEN_TEST_DEPENDS}
  )
endif()

set_target_properties(check-spirv-tblgen
  PROPERTIES
    EXCLUDE_FROM_DEFAULT_BUILD OFF
    EXCLUDE_FROM_ALL OFF
  )

set_target_properties(check-spirv-tblgen PROPERTIES FOLDER "Tests/LIT Tests")
