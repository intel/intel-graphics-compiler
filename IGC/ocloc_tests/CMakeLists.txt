#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

if(NOT IGC_OPTION__ENABLE_OCLOC_LIT_TESTS)
  return()
endif()


  # Variables set here are used by `configure_file` call and by
  # `add_lit_testsuite` later on.
  set(IGC_LIBRARY_DIR "$<TARGET_FILE_DIR:igc_dll>")
  set(OPENCL_CLANG_LIB_DIR "$<TARGET_FILE_DIR:opencl-clang-lib>")
  set(IGC_OCLOC_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  set(IGC_OCLOC_TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)

  set(IGC_OCLOC_BINARY_DIR "$<$<TARGET_EXISTS:ocloc>:$<TARGET_FILE_DIR:ocloc>>")
  set(IGC_BUILD__PROJ__ocloc "$<$<TARGET_EXISTS:ocloc>:ocloc>")
  set(IGC_OCLOC_LIBRARY_DIR "$<$<TARGET_EXISTS:ocloc_lib>:$<TARGET_FILE_DIR:ocloc_lib>>")
  set(IGC_BUILD__PROJ__ocloc_lib "$<$<TARGET_EXISTS:ocloc_lib>:ocloc_lib>")

  if ((LLVM_VERSION_MAJOR LESS 14) OR SPIRV_SKIP_EXECUTABLES)
    set(IGC_BUILD__PROJ__llvm_spirv "")
    set(IGC_LLVM_SPIRV_DIR "")
  else()
    set(IGC_BUILD__PROJ__llvm_spirv "llvm-spirv")
    set(IGC_LLVM_SPIRV_DIR "$<TARGET_FILE_DIR:llvm-spirv>")
  endif()

  if (NOT SPIRV_SKIP_EXECUTABLES)
    set(IGC_BUILD__PROJ__spirv_as "spirv-as")
    set(IGC_SPIRV_AS_DIR "$<TARGET_FILE_DIR:spirv-as>")
  else()
    set(IGC_BUILD__PROJ__spirv_as "")
    set(IGC_SPIRV_AS_DIR "")
  endif()

  # If any new tool is required by any of the LIT tests add it here:
  set(IGC_OCLOC_TEST_DEPENDS
    FileCheck
    count
    not
    llvm-as
    llvm-dwarfdump
    "${IGC_BUILD__PROJ__ocloc}"
    "${IGC_BUILD__PROJ__ocloc_lib}"
    "${IGC_BUILD__PROJ__igc_dll}"
    "${IGC_BUILD__PROJ__fcl_dll}"
    "${IGC_BUILD__PROJ__spirv_as}"
    "${IGC_BUILD__PROJ__llvm_spirv}"
    "${COMMON_CLANG}"
    )

igc_find_external_lit()

igc_configure_lit_site_cfg(
  ${IGC_OCLOC_TEST_SOURCE_DIR}/lit.site.cfg.py.in
  ${IGC_OCLOC_TEST_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${IGC_OCLOC_TEST_SOURCE_DIR}/lit.cfg.py
  )

file(GLOB_RECURSE _tests "${IGC_OCLOC_TEST_SOURCE_DIR}/*.cl" "${IGC_OCLOC_TEST_SOURCE_DIR}/*.ll")

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
  igc_add_lit_target(check-ocloc "${IGC_OCLOC_TEST_BINARY_DIR}" "Running the offline compilation tests"
    DEPENDS ${IGC_OCLOC_TEST_DEPENDS} ${_tests}
    SOURCES ${_tests} ${IGC_OCLOC_TEST_SOURCE_DIR}/lit.cfg.py
    )
else()
  add_lit_testsuite(check-ocloc "Running the offline compilation tests"
    ${IGC_OCLOC_TEST_BINARY_DIR}
    DEPENDS ${IGC_OCLOC_TEST_DEPENDS}
    )
endif()

