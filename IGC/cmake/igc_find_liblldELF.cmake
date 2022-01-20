#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#   IGC_OPTION__LLDELF_LIB_DIR - Specify additional directories for searching lldELF library
#   IGC_OPTION__LLDELF_H_DIR - Specify additional directories for searching lldELF headers
#   IGC_OPTION__LLD_BIN_DIR - Specify additional directories for searching lld executable

function(add_lld_library LIB_NAME)
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
endfunction()

function(add_lld_executable EXE_NAME)
  find_program(${EXE_NAME}_PATH
    ${EXE_NAME}
    PATHS "${IGC_OPTION__LLD_BIN_DIR}"
    PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/bin")

  if(${EXE_NAME}_PATH-NOTFOUND)
    message(FATAL_ERROR
    "Cannot find ${EXE_NAME} executable, please install missing executable or provide the path by IGC_OPTION__LLD_BIN_DIR")
  endif()
  add_executable(${EXE_NAME} IMPORTED GLOBAL)
  set_target_properties(${EXE_NAME} PROPERTIES IMPORTED_LOCATION ${${EXE_NAME}_PATH})
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
    BinaryFormat
    BitWriter
    Core
    DebugInfoDWARF
    Demangle
    LTO
    MC
    Object
    Option
    Passes
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
