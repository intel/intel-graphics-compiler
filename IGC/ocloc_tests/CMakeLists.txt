#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

if(NOT IGC_OPTION__ENABLE_OCLOC_LIT_TESTS)
  return()
endif()

if (IGC_BUILD__VC_ENABLED OR IGC_BUILD__FORCE_VC_TESTS)
  set(IGC_ENABLE_VC_OCLOC_TESTS ON)
endif()

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

function(_igc_get_ocloc_binary_name binary_dir output_name)
  set(_igc_ocloc_names ocloc)

  foreach(_igc_ocloc_name IN LISTS _igc_ocloc_names)
    if(EXISTS "${binary_dir}/${_igc_ocloc_name}${CMAKE_EXECUTABLE_SUFFIX}")
      set(${output_name} "${_igc_ocloc_name}${CMAKE_EXECUTABLE_SUFFIX}" PARENT_SCOPE)
      return()
    endif()
  endforeach()

  file(GLOB _igc_ocloc_versioned LIST_DIRECTORIES false "${binary_dir}/ocloc-[0-9][0-9].[0-9][0-9].[0-9]*${CMAKE_EXECUTABLE_SUFFIX}")
  if(_igc_ocloc_versioned)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
      list(SORT _igc_ocloc_versioned COMPARE NATURAL ORDER DESCENDING)
    else()
      list(SORT _igc_ocloc_versioned ORDER DESCENDING)
    endif()
    list(GET _igc_ocloc_versioned 0 _igc_ocloc_path)
    get_filename_component(_igc_ocloc_name "${_igc_ocloc_path}" NAME)
    set(${output_name} "${_igc_ocloc_name}" PARENT_SCOPE)
  endif()
endfunction()

  # Variables set here are used by `configure_file` call and by
  # `add_lit_testsuite` later on.
  set(IGC_LIBRARY_DIR "$<TARGET_FILE_DIR:${IGC_BUILD__PROJ__igc_dll}>")
  set(OPENCL_CLANG_LIB_DIR "$<TARGET_FILE_DIR:opencl-clang-lib>")
  set(IGC_OCLOC_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  set(IGC_OCLOC_TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)

  set(IGC_OCLOC_BINARY_NAME "$<$<TARGET_EXISTS:ocloc>:$<TARGET_FILE_BASE_NAME:ocloc>>")
  set(IGC_OCLOC_BINARY_DIR "$<$<TARGET_EXISTS:ocloc>:$<TARGET_FILE_DIR:ocloc>>")
  set(IGC_BUILD__PROJ__ocloc "$<$<TARGET_EXISTS:ocloc>:ocloc>")
  set(IGC_OCLOC_LIBRARY_DIR "$<$<TARGET_EXISTS:ocloc_lib>:$<TARGET_FILE_DIR:ocloc_lib>>")
  set(IGC_BUILD__PROJ__ocloc_lib "$<$<TARGET_EXISTS:ocloc_lib>:ocloc_lib>")

  # If any new tool is required by any of the LIT tests add it here:
  set(IGC_OCLOC_TEST_DEPENDS
    FileCheck
    count
    not
    llvm-as
    llvm-dwarfdump
    split-file
    "${IGC_BUILD__PROJ__ocloc}"
    "${IGC_BUILD__PROJ__ocloc_lib}"
    "${IGC_BUILD__PROJ__igc_dll}"
    "${IGC_BUILD__PROJ__fcl_dll}"
    "${IGC_BUILD__PROJ__spirv_as}"
    "${IGC_BUILD__PROJ__llvm_spirv}"
    "${COMMON_CLANG}"
    )

# Path overrides
set(IGC_OPTION__OCLOC_TEST_BIN_DIR "" CACHE PATH
 "Directory containing the ocloc binary used by the offline compilation tests")
set(IGC_OPTION__OCLOC_TEST_OCLOC_LIB_DIR "" CACHE PATH
 "Directory containing the ocloc_lib library used by the offline compilation tests")
set(IGC_OPTION__OCLOC_TEST_IGC_LIB_DIR "" CACHE PATH
 "Directory containing the igc library used by the offline compilation tests")
set(IGC_OPTION__OCLOC_TEST_CCLANG_LIB_DIR "" CACHE PATH
 "Directory containing the opencl-clang library used by the offline compilation tests")

if(IGC_OPTION__OCLOC_TEST_BIN_DIR)
  file(TO_CMAKE_PATH "${IGC_OPTION__OCLOC_TEST_BIN_DIR}" IGC_OCLOC_BINARY_DIR)
  _igc_get_ocloc_binary_name("${IGC_OCLOC_BINARY_DIR}" _igc_ocloc_override_name)
  if(_igc_ocloc_override_name)
    set(IGC_OCLOC_BINARY_NAME "${_igc_ocloc_override_name}")
    list(REMOVE_ITEM IGC_OCLOC_TEST_DEPENDS "${IGC_BUILD__PROJ__ocloc}")
  endif()
endif()
if(IGC_OPTION__OCLOC_TEST_OCLOC_LIB_DIR)
  file(TO_CMAKE_PATH "${IGC_OPTION__OCLOC_TEST_OCLOC_LIB_DIR}" IGC_OCLOC_LIBRARY_DIR)
  list(REMOVE_ITEM IGC_OCLOC_TEST_DEPENDS "${IGC_BUILD__PROJ__ocloc_lib}")
endif()
if(IGC_OPTION__OCLOC_TEST_IGC_LIB_DIR)
  file(TO_CMAKE_PATH "${IGC_OPTION__OCLOC_TEST_IGC_LIB_DIR}" IGC_LIBRARY_DIR)
  list(REMOVE_ITEM IGC_OCLOC_TEST_DEPENDS "${IGC_BUILD__PROJ__igc_dll}" "${IGC_BUILD__PROJ__fcl_dll}")
endif()
if(IGC_OPTION__OCLOC_TEST_CCLANG_LIB_DIR)
  file(TO_CMAKE_PATH "${IGC_OPTION__OCLOC_TEST_CCLANG_LIB_DIR}" OPENCL_CLANG_LIB_DIR)
  list(REMOVE_ITEM IGC_OCLOC_TEST_DEPENDS "${COMMON_CLANG}")
endif()

# Expose the optional igc-clang prebuild directory to the lit loader search path
# so the IGC_LibClangOverride tests can load it by name.
if(TARGET igc-clang-lib)
  set(IGC_CLANG_LIB_DIR "${IGC_CLANG_PREBUILD_DIR}")
else()
  set(IGC_CLANG_LIB_DIR "")
endif()

igc_find_external_lit()

igc_configure_lit_site_cfg(
  ${IGC_OCLOC_TEST_SOURCE_DIR}/lit.site.cfg.py.in
  ${IGC_OCLOC_TEST_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${IGC_OCLOC_TEST_SOURCE_DIR}/lit.cfg.py
  )

file(GLOB_RECURSE _tests "${IGC_OCLOC_TEST_SOURCE_DIR}/*.cl" "${IGC_OCLOC_TEST_SOURCE_DIR}/*.ll" "${IGC_OCLOC_TEST_SOURCE_DIR}/*.spv-test")

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

