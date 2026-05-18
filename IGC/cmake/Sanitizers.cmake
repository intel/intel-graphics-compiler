# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

# Locate the ASan shared runtime library via the compiler's -print-file-name.
function(get_asan_runtime_library var)
  set(${var} "" PARENT_SCOPE)

  set(_compiler "${CMAKE_CXX_COMPILER}")
  if(ARGC GREATER 1)
    set(_compiler "${ARGV1}")
  endif()

  set(_candidates "")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND _candidates "libclang_rt.asan-x86_64${CMAKE_SHARED_LIBRARY_SUFFIX}")
  endif()
  list(APPEND _candidates "libasan${CMAKE_SHARED_LIBRARY_SUFFIX}")

  foreach(_name IN LISTS _candidates)
    execute_process(
      COMMAND ${_compiler} -print-file-name=${_name}
      OUTPUT_VARIABLE _output
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    if(EXISTS "${_output}")
      get_filename_component(_lib "${_output}" REALPATH)
      set(${var} "${_lib}" PARENT_SCOPE)
      message(STATUS "ASan runtime library: ${_lib}")
      return()
    endif()
  endforeach()

  message(WARNING "ASan enabled but runtime library not found (${_candidates})")
endfunction()

function(igc_target_enable_address_sanitizer _target)
  if(NOT IGC_OPTION__ENABLE_ADDRESS_SANITIZER OR NOT TARGET "${_target}")
    return()
  endif()

  get_target_property(_type "${_target}" TYPE)

  set(_use_shared FALSE)
  if(_type STREQUAL "SHARED_LIBRARY" OR _type STREQUAL "MODULE_LIBRARY")
    set(_use_shared TRUE)
  elseif(_type STREQUAL "EXECUTABLE")
    get_target_property(_linkage "${_target}" IGC_ADDRESS_SANITIZER_RUNTIME_LINKAGE)
    if(_linkage STREQUAL "shared")
      set(_use_shared TRUE)
    endif()
  else()
    return()
  endif()

  if(_use_shared AND IGC_BUILD__ASAN_RUNTIME_DIR)
    set_property(TARGET "${_target}" APPEND PROPERTY BUILD_RPATH "${IGC_BUILD__ASAN_RUNTIME_DIR}")
    set_property(TARGET "${_target}" APPEND PROPERTY INSTALL_RPATH "${IGC_BUILD__ASAN_RUNTIME_DIR}")
  endif()

  if(_use_shared)
    if(_igc_compiler_is_clang)
      target_link_options("${_target}" PRIVATE -shared-libasan)
    endif()
  elseif(_type STREQUAL "EXECUTABLE")
    if(_igc_compiler_is_clang)
      target_link_options("${_target}" PRIVATE -static-libsan)
    else()
      target_link_options("${_target}" PRIVATE -static-libasan)
    endif()
  endif()

  if(_type STREQUAL "SHARED_LIBRARY")
    target_link_options("${_target}" PRIVATE -Wl,--allow-shlib-undefined)
  endif()
endfunction()