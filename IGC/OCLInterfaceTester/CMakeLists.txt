#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

add_executable(IGCOCLInterfaceTester
  OCLInterfaceTester.cpp
  IgcChecks.cpp
  )

# CIF and ocl_igc_interface headers are needed to build time
target_include_directories(IGCOCLInterfaceTester PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL
  ${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/cif
  ${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_shared/executable_format
  # to get platforms from igfxfmid.h
  ${IGC_BUILD__INC_DIR}/common
  )

# The tester uses only the C++ standard library plus the
# platform dynamic loader (dlopen/LoadLibrary). CMAKE_DL_LIBS is the dl library
# where required (empty on Windows and on glibc >= 2.34).
target_link_libraries(IGCOCLInterfaceTester ${CMAKE_DL_LIBS})

# Take the absolute path of the freshly built libigc in at configure time,
# so the tester always loads that library (the same one the lit tests use)
# rather than a system-installed one
target_compile_definitions(IGCOCLInterfaceTester PRIVATE
  IGC_TESTER_LIBRARY_PATH="$<TARGET_FILE:${IGC_BUILD__PROJ__igc_dll}>")

# The check-interfaces lit target requires the lit infrastructure, which is only configured when lit testing is enabled
if(IGC_OPTION__ENABLE_LIT_TESTS)
  add_subdirectory(test)
endif()
