#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(VC_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VC_TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
set(VC_LIT_CONFIG_FILE ${VC_TEST_BINARY_DIR}/vc.lit.site.cfg.py)

igc_configure_lit_site_cfg(
  ${VC_TEST_SOURCE_DIR}/lit.site.cfg.py.in
  ${VC_LIT_CONFIG_FILE}
  MAIN_CONFIG
  ${VC_TEST_SOURCE_DIR}/lit.cfg.py
  )

set(TEST_DEPS
  # These are required by lit default substitutions.
  FileCheck
  count
  not
  llvm-dwarfdump
  lld

  BiFModuleOcl
  CMCLTranslatorTool
  VCBiFPreparation
  )

if(LLVM_ON_UNIX)
  list(APPEND TEST_DEPS
    opt
    llc
    VCBackendPlugin
    )
  if (${LLVM_VERSION_MAJOR} GREATER 15)
    list(APPEND TEST_DEPS NewPMPlugin)
  endif()
endif()

file(GLOB_RECURSE _tests "${VC_TEST_SOURCE_DIR}/*.ll" "${VC_TEST_SOURCE_DIR}/*.s")

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
  igc_add_lit_target(check-vc "${VC_TEST_BINARY_DIR}" "Running the vc-opt regression tests"
    EXTRA "--config-prefix=vc.lit" ""
    DEPENDS ${TEST_DEPS} ${_tests}
    SOURCES ${_tests} ${VC_TEST_SOURCE_DIR}/lit.cfg.py
    )
else()
  add_lit_testsuite(check-vc "Running the vc-opt regression tests"
    ${VC_TEST_BINARY_DIR}
    ARGS "--config-prefix=vc.lit" ""
    DEPENDS ${TEST_DEPS}
    )
endif()

