#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#   IGC_OPTION__LLDELF_LIB_DIR - Specify additional directories for searching lldELF library
#   IGC_OPTION__LLDELF_H_DIR - Specify additional directories for searching lldELF headers

function(find_lld_library VAR LIB_NAME)
  find_library(${VAR}
    ${LIB_NAME}
    PATHS "${IGC_OPTION__LLDELF_LIB_DIR}"
    PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/lib")

  if(${VAR}-NOTFOUND)
    message(FATAL_ERROR
    "Cannot find ${LIB_NAME} library, please install missing library or provide the path by IGC_OPTION__LLDELF_LIB_DIR")
  endif()
endfunction()

if(IGC_BUILD__LLVM_SOURCES)
  set(LLD_ELF_LIB lldELF)
  set(LLD_COM_LIB lldCommon)
  get_target_property(lldELF_SRC_DIR lldELF SOURCE_DIR)
  set(LLD_INCLUDE_DIR "${lldELF_SRC_DIR}/../include")
elseif(IGC_BUILD__LLVM_PREBUILDS)
  find_lld_library(LLD_ELF_LIB lldELF)
  find_lld_library(LLD_COM_LIB lldCommon)

  find_path(
    LLD_INCLUDE_DIR
    NAMES "Driver.h"
    PATHS "${IGC_OPTION__LLDELF_H_DIR}"
    PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/include/lld/Common/"
  )

  if(LLD_INCLUDE_DIR-NOTFOUND)
    message(FATAL_ERROR
    "Cannot find 'lld/Common/Driver.h' header, please install missing header or provide the path by IGC_OPTION__LLDELF_H_DIR")
  endif()
  set(LLD_INCLUDE_DIR "${LLD_INCLUDE_DIR}/../../")
endif()

list(APPEND IGC_BUILD__LLVM_LIBS_TO_LINK
  ${LLD_ELF_LIB}
  ${LLD_COM_LIB})

include_directories(${LLD_INCLUDE_DIR})
