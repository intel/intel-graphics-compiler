#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_guard(DIRECTORY)

# Convenience function to get list of LLVM components for
# target_link_library. If LLVM was configured with llvm dylib, then
# included in dylib llvm targets should be replaced with LLVM
# lib. Otherwise, just return passed libraries.
# ret -- name of variable with returned targets list. All other
# arguments are targets to process.
function(igc_get_llvm_targets RET)
  set(TARGETS ${ARGN})

  if (LLVM_LINK_LLVM_DYLIB)
    # Drop all components, though it is probably not right
    # and llvm_map_components_to_libnames should be used as filter.
    # However, in external build it returns empty list for "all"
    # so filtering is not really done.
    if ("${LLVM_DYLIB_COMPONENTS}" STREQUAL "all")
      set(TARGETS "")
    else()
      list(REMOVE_ITEM TARGETS ${LLVM_DYLIB_COMPONENTS})
    endif()
  endif()

  # Expand rest of components names to target names.
  llvm_map_components_to_libnames(TARGETS ${TARGETS})

  if (LLVM_LINK_LLVM_DYLIB)
    set(TARGETS ${TARGETS} LLVM)
  endif()

  if(IGC_OPTION__LLVM_MODE STREQUAL PREBUILDS_MODE_NAME AND WIN32)
    set(PRE_BUILT_TARGETS)
    foreach(tmp_name ${TARGETS})
        unset(found_name CACHE)

        find_library(found_name_release ${tmp_name} PATHS "${LLVM_DIR}/Release/lib")

        if("${found_name_release}" STREQUAL "${found_name_release}-NOTFOUND")
          message(FATAL_ERROR
          "Cannot find ${tmp_name} in Release version in ${LLVM_DIR}")
        endif()

        find_library(found_name_debug ${tmp_name} PATHS "${LLVM_DIR}/Debug/lib")

        if("${found_name_debug}" STREQUAL "${found_name_debug}-NOTFOUND")
          message(FATAL_ERROR
          "Cannot find ${tmp_name} in Debug version in ${LLVM_DIR}")
        endif()

        # Add lib as imported target if not already added
        if (NOT TARGET ${tmp_name})
          add_library(${tmp_name} UNKNOWN IMPORTED)
          set_target_properties(${tmp_name} PROPERTIES
            IMPORTED_LOCATION_RELEASE "${found_name_release}"
            IMPORTED_LOCATION_RELEASEINTERNAL "${found_name_release}"
            IMPORTED_LOCATION_DEBUG "${found_name_debug}"
            )
        endif()
        list(APPEND PRE_BUILT_TARGETS ${tmp_name})
    endforeach()

    set(TARGETS ${PRE_BUILT_TARGETS})
  endif()

  set(${RET} ${TARGETS} PARENT_SCOPE)
endfunction()

# Simple wrapper around configure_lit_site_cfg that allows
# generator expressions in configured lit config.
function(igc_configure_lit_site_cfg in out)
  set(temp_cfg ${CMAKE_CURRENT_BINARY_DIR}/temp.cfg.py)

  # Generate temporary site config with LLVM variables filled.
  configure_lit_site_cfg(
    ${in}
    ${temp_cfg}
    ${ARGN}
    )

  # Need to regenerate again to use generator expressions that are not
  # expanded in configure_file.
  file(GENERATE
    OUTPUT ${out}
    INPUT ${temp_cfg}
    )
endfunction()

# Helper macro to set LLVM_EXTERNAL_LIT variable for LLVM lit tests.
# Variable can be overridden from command line to set custom lit tool.
macro(igc_find_external_lit)
  list(APPEND paths_to_lit ${LLVM_SOURCE_DIR}/utils/lit)
  find_file(LLVM_EXTERNAL_LIT
    NAMES llvm-lit llvm-lit.py lit.py lit
    PATHS ${paths_to_lit}
    DOC "Path to lit utility"
    )

  if(NOT LLVM_EXTERNAL_LIT)
    message(FATAL_ERROR "llvm-lit is not found, please specify it with LLVM_EXTERNAL_LIT variable")
  endif()
endmacro()

# Helper macro to add tablegenning and set
# include flags for current directories.
macro(igc_tablegen)
  set(_old_flags ${LLVM_TABLEGEN_FLAGS})
  list(APPEND LLVM_TABLEGEN_FLAGS "-I=${CMAKE_CURRENT_SOURCE_DIR}" "-I=${CMAKE_CURRENT_BINARY_DIR}")
  tablegen(${ARGN})
  set(LLVM_TABLEGEN_FLAGS ${_old_flags})
  unset(_old_flags)
endmacro()
