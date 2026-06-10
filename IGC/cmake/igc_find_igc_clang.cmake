#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Locate the prebuilt igc-clang library (a SYCL-based clang build).
# igc-clang is used purely at runtime through the
# IGC_LibClangOverride regkey, exercised by the ocloc lit tests.
#
# This module creates an IMPORTED target `igc-clang-lib` pointing at the synced
# prebuild when it is present.

if(TARGET igc-clang-lib)
  return()
endif()

# OS_NAME / OS_VERSION_NUMBER (Linux) are not defined in the cross-target-testing
# build, so glob the os/version level rather than hard-coding it. The manifest
# fetches only one variant per build, so the glob is unambiguous.
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  igc_arch_get_cpu(cpuSuffix)
  set(IGC_CLANG_LIB_FULL_NAME "igc-clang${cpuSuffix}${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(IGC_CLANG_LIB_GLOB "prebuild-igc-clang/windows/Release/${cpuSuffix}/${IGC_CLANG_LIB_FULL_NAME}")
else()
  set(IGC_CLANG_LIB_FULL_NAME "libigc-clang${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(IGC_CLANG_LIB_GLOB "prebuild-igc-clang-linux/linux/*/*/${IGC_CLANG_LIB_FULL_NAME}*")
endif()

# Sync roots: GFX_DEV_SRC_DIR-relative for normal IGC builds;
# BS_DIR_EXTERNAL_COMPONENTS (workspace root) for cross-target-testing, where
# IGC_BUILD__GFX_DEV_SRC_DIR is undefined.
set(_igc_clang_roots "")
if(IGC_BUILD__GFX_DEV_SRC_DIR)
  list(APPEND _igc_clang_roots
       "${IGC_BUILD__GFX_DEV_SRC_DIR}/../.."
       "${IGC_BUILD__GFX_DEV_SRC_DIR}/..")
endif()
if(BS_DIR_EXTERNAL_COMPONENTS)
  list(APPEND _igc_clang_roots "${BS_DIR_EXTERNAL_COMPONENTS}")
endif()

# On Linux, sort so the versioned libigc-clang.so.<ver> is preferred over the
# bare symlink.
set(IGC_CLANG_LIB_PATH "")
foreach(_igc_clang_root ${_igc_clang_roots})
  file(GLOB _igc_clang_hits "${_igc_clang_root}/${IGC_CLANG_LIB_GLOB}")
  if(NOT _igc_clang_hits)
    continue()
  endif()
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    list(GET _igc_clang_hits 0 IGC_CLANG_LIB_PATH)
  else()
    list(SORT _igc_clang_hits)
    list(GET _igc_clang_hits -1 IGC_CLANG_LIB_PATH)
  endif()
  break()
endforeach()

if(NOT IGC_CLANG_LIB_PATH)
  message(STATUS "[IGC] : igc-clang prebuild not found (not fetched for this build) - igc-clang tests will be skipped")
  return()
endif()

get_filename_component(IGC_CLANG_PREBUILD_DIR "${IGC_CLANG_LIB_PATH}" DIRECTORY)

add_library(igc-clang-lib SHARED IMPORTED GLOBAL)
set_property(TARGET igc-clang-lib PROPERTY "IMPORTED_LOCATION" "${IGC_CLANG_LIB_PATH}")
if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
  get_filename_component(_igc_clang_soname "${IGC_CLANG_LIB_PATH}" NAME)
  set_property(TARGET igc-clang-lib PROPERTY "IMPORTED_SONAME" "${_igc_clang_soname}")
endif()

message(STATUS "[IGC] : igc-clang found: ${IGC_CLANG_LIB_PATH}")
