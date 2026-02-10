#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_guard(DIRECTORY)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../external/llvm/llvm_utils.cmake")

llvm_define_mode_variable(VCIntrinsics IGC_OPTION__VC_INTRINSICS_MODE)

if(IGC_OPTION__VC_INTRINSICS_MODE STREQUAL PREBUILDS_MODE_NAME)

  set(VC_INTRINSICS_PREBUILDS_PACKAGE_NAME VCIntrinsics${LLVM_VERSION_MAJOR})
  message(STATUS "[VC] : Searching prebuilt vc-intrinsics package")
  find_package(${VC_INTRINSICS_PREBUILDS_PACKAGE_NAME} REQUIRED)
  message(STATUS "[VC] : Found prebuilt vc-intrinsics package in: ${${VC_INTRINSICS_PREBUILDS_PACKAGE_NAME}_CONFIG}")

  # override default link libraries by list from vc-intrinsics build
  set(LLVM_COMPONENTS
      CodeGen
      Support
      Core
      Analysis
     )
  igc_get_llvm_targets(LLVM_LIBS ${LLVM_COMPONENTS})
  set_target_properties(LLVMGenXIntrinsics PROPERTIES
    INTERFACE_LINK_LIBRARIES "${LLVM_LIBS}"
  )
else()

  if(DEFINED VC_INTRINSICS_SRC)
    set(INTRSRC "${VC_INTRINSICS_SRC}/GenXIntrinsics")
  endif()

  if(NOT DEFINED INTRSRC)
    set(INTRSRC "${CMAKE_CURRENT_SOURCE_DIR}/../../../vc-intrinsics/GenXIntrinsics")
  endif()

  message(STATUS "[VC] Using vc-intrinsics source from: ${INTRSRC}")
  # Trick intrinsics.
  set(BUILD_EXTERNAL YES)
  # We are building intrinsics.
  set(INTRBUILD "${CMAKE_CURRENT_BINARY_DIR}/intrbuild")
  # Add -Wno-error to avoid build errors from llvm's header deprecation warnings and
  # unused variables/functions coming from vc-intrinsics.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    string(APPEND CMAKE_CXX_FLAGS " -Wno-error")
  endif()
  add_subdirectory(${INTRSRC} ${INTRBUILD} EXCLUDE_FROM_ALL)
endif() #VC_INTRINSICS_MODE
