#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#   IGC_OPTION__LLD_DIR - Specify additional directories for searching lld libraries and executables
#   IGC_OPTION__LLDELF_H_DIR - Specify additional directories for searching lldELF headers

function(add_lld_library LIB_NAME)
  if(WIN32)
    find_library(${LIB_NAME}_PATH_RELEASE
      ${LIB_NAME}
      PATHS "${IGC_OPTION__LLD_DIR}/Release/lib"
      PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/lib")

    if("${${LIB_NAME}_PATH_RELEASE}" STREQUAL "${LIB_NAME}_PATH_RELEASE-NOTFOUND")
      message(FATAL_ERROR
      "Cannot find ${LIB_NAME} library in Release version in ${IGC_OPTION__LLD_DIR}")
    endif()

    find_library(${LIB_NAME}_PATH_DEBUG
      ${LIB_NAME}
      PATHS "${IGC_OPTION__LLD_DIR}/Debug/lib"
      PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/lib")

    if("${${LIB_NAME}_PATH_DEBUG}" STREQUAL "${LIB_NAME}_PATH_DEBUG-NOTFOUND")
      message(FATAL_ERROR
      "Cannot find ${LIB_NAME} library in Debug version in ${IGC_OPTION__LLD_DIR}")
    endif()

    add_library(${LIB_NAME} UNKNOWN IMPORTED GLOBAL)
    set_target_properties(${LIB_NAME} PROPERTIES
      IMPORTED_LOCATION_RELEASE "${${LIB_NAME}_PATH_RELEASE}"
      IMPORTED_LOCATION_RELEASEINTERNAL "${${LIB_NAME}_PATH_RELEASE}"
      IMPORTED_LOCATION_DEBUG "${${LIB_NAME}_PATH_DEBUG}"
      )
  else()
    find_library(${LIB_NAME}_PATH
      ${LIB_NAME}
      PATHS "${IGC_OPTION__LLDELF_LIB_DIR}"
      PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/lib")

    if(${LIB_NAME}_PATH-NOTFOUND)
      message(FATAL_ERROR
      "Cannot find ${LIB_NAME} library, please install missing library or provide the path by IGC_OPTION__LLDELF_LIB_DIR")
    endif()
    add_library(${LIB_NAME} UNKNOWN IMPORTED GLOBAL)
    set_target_properties(${LIB_NAME} PROPERTIES IMPORTED_LOCATION ${${LIB_NAME}_PATH})
  endif()
endfunction()

function(add_lld_executable EXE_NAME)
  if(WIN32)
    find_program(${EXE_NAME}_PATH_RELEASE
      ${EXE_NAME}
      PATHS "${IGC_OPTION__LLD_DIR}/Release/bin"
      PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/bin")

    if("${${EXE_NAME}_PATH_RELEASE}" STREQUAL "${EXE_NAME}_PATH_RELEASE-NOTFOUND")
      message(FATAL_ERROR
      "Cannot find ${EXE_NAME} executable in Release version in ${IGC_OPTION__LLD_DIR}")
    endif()

    find_program(${EXE_NAME}_PATH_DEBUG
      ${EXE_NAME}
      PATHS "${IGC_OPTION__LLD_DIR}/Debug/bin"
      PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/bin")

    if("${${EXE_NAME}_PATH_DEBUG}" STREQUAL "${EXE_NAME}_PATH_DEBUG-NOTFOUND")
      message(FATAL_ERROR
      "Cannot find ${EXE_NAME} executable in Debug version in ${IGC_OPTION__LLD_DIR}")
    endif()

    add_executable(${EXE_NAME} IMPORTED GLOBAL)
    set_target_properties(${EXE_NAME} PROPERTIES
      IMPORTED_LOCATION_RELEASE "${${EXE_NAME}_PATH_RELEASE}"
      IMPORTED_LOCATION_RELEASEINTERNAL "${${EXE_NAME}_PATH_RELEASE}"
      IMPORTED_LOCATION_DEBUG "${${EXE_NAME}_PATH_DEBUG}"
      )
  else()
    find_program(${EXE_NAME}_PATH
      ${EXE_NAME}
      PATHS "${IGC_OPTION__LLD_BIN_DIR}"
      PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/bin")

    if(${EXE_NAME}_PATH-NOTFOUND)
      message(FATAL_ERROR "Cannot find ${EXE_NAME} executable, please install missing executable or provide the path by IGC_OPTION__LLD_BIN_DIR")
    endif()

    add_executable(${EXE_NAME} IMPORTED GLOBAL)
    set_target_properties(${EXE_NAME} PROPERTIES IMPORTED_LOCATION ${${EXE_NAME}_PATH})
  endif()
endfunction()

if(IGC_BUILD__LLVM_SOURCES)
  get_target_property(lldELF_SRC_DIR lldELF SOURCE_DIR)
  set(LLD_INCLUDE_DIR "${lldELF_SRC_DIR}/../include")
elseif(IGC_BUILD__LLVM_PREBUILDS)
  add_lld_executable(lld)
  add_lld_library(lldELF)
  add_lld_library(lldCommon)
  igc_get_llvm_targets(LLD_COMMON_LLVM_DEPS
    Codegen
    Core
    DebugInfoDWARF
    Demangle
    MC
    Option
    Support
    Target)
  igc_get_llvm_targets(LLD_ELF_LLVM_DEPS
    ${LLVM_TARGETS_TO_BUILD}
    Analysis
    AsmPrinter
    BinaryFormat
    BitWriter
    Codegen
    Core
    DebugInfoDWARF
    Demangle
    GlobalISel
    LTO
    MC
    MCDisassembler
    Object
    Option
    Passes
    SelectionDAG
    Support)
  target_link_libraries(lldCommon INTERFACE
    ${LLD_COMMON_LLVM_DEPS})
  target_link_libraries(lldELF INTERFACE
    ${LLD_ELF_LLVM_DEPS}
    lldCommon)

  find_path(
    LLD_INCLUDE_DIR
    NAMES "Driver.h"
    PATHS "${IGC_OPTION__LLDELF_H_DIR}"
    PATH_SUFFIXES
      "llvm-${LLVM_VERSION_MAJOR}/include/lld/Common/"
      "include/lld/Common/"
  )

  if(LLD_INCLUDE_DIR-NOTFOUND)
    message(FATAL_ERROR
    "Cannot find 'lld/Common/Driver.h' header, please install missing header or provide the path by IGC_OPTION__LLDELF_H_DIR")
  endif()
  set(LLD_INCLUDE_DIR "${LLD_INCLUDE_DIR}/../../")
endif()

list(APPEND IGC_BUILD__LLVM_LIBS_TO_LINK
  lldELF)

include_directories(${LLD_INCLUDE_DIR})
