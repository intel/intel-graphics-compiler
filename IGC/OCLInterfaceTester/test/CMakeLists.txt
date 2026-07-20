#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(INTERFACE_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(INTERFACE_TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
set(INTERFACE_LIT_CONFIG_FILE ${INTERFACE_TEST_BINARY_DIR}/lit.site.cfg.py)

set(INTERFACE_TESTER_DIR "$<TARGET_FILE_DIR:IGCOCLInterfaceTester>")
set(IGC_LIBRARY_DIR      "$<TARGET_FILE_DIR:${IGC_BUILD__PROJ__igc_dll}>")
set(IGC_LIBRARY_FILE     "$<TARGET_FILE:${IGC_BUILD__PROJ__igc_dll}>")

igc_configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${INTERFACE_LIT_CONFIG_FILE}
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

# igc_dll is a required dependency, becasue we load libigc.so/dll,
# so it must be built before the tests are run
set(INTERFACE_TEST_DEPENDS
  FileCheck
  count
  not
  IGCOCLInterfaceTester
  "${IGC_BUILD__PROJ__igc_dll}"
)

file(GLOB_RECURSE _tests CONFIGURE_DEPENDS "${INTERFACE_TEST_SOURCE_DIR}/*.test")

# Create a lit target
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
  igc_add_lit_target(check-interfaces "${INTERFACE_TEST_BINARY_DIR}" "Running the NEO-IGC interface tests"
    DEPENDS ${INTERFACE_TEST_DEPENDS} ${_tests}
    SOURCES ${_tests} ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
    )
else()
  add_lit_testsuite(check-interfaces "Running the NEO-IGC interface tests"
    ${INTERFACE_TEST_BINARY_DIR}
    DEPENDS
    ${INTERFACE_TEST_DEPENDS}
  )
endif()

# Target should be included in default build (when you do ninja/make
# without specifing target or in VS "Build Solution")
set_target_properties(check-interfaces
  PROPERTIES
  EXCLUDE_FROM_DEFAULT_BUILD OFF
  EXCLUDE_FROM_ALL OFF
)

# Group this target in a directory with other LIT targets in IDE
set_target_properties(check-interfaces PROPERTIES FOLDER "Tests/LIT Tests")
