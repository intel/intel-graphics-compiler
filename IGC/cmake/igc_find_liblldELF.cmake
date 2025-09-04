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
    igc_arch_get_cpu(cpuSuffix)
    set(LLD_LIB_PREBUILT_PATH_RELEASE "${IGC_OPTION__LLD_DIR}/../Release/${cpuSuffix}/Release/lib")
    set(LLD_LIB_PREBUILT_PATH_DEBUG "${IGC_OPTION__LLD_DIR}/../Debug/${cpuSuffix}/Debug/lib")
    find_library(${LIB_NAME}_PATH
      ${LIB_NAME}
      PATHS "${LLD_LIB_PREBUILT_PATH_RELEASE}"
            "${LLD_LIB_PREBUILT_PATH_DEBUG}"
      PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/lib")
    if("${${LIB_NAME}_PATH}" STREQUAL "${LIB_NAME}_PATH-NOTFOUND")
      message(FATAL_ERROR
      "Cannot find ${LIB_NAME} library in ${IGC_OPTION__LLD_DIR}")
    endif()

    add_library(${LIB_NAME} UNKNOWN IMPORTED GLOBAL)
    set_target_properties(${LIB_NAME} PROPERTIES
      IMPORTED_LOCATION_RELEASE "${LLD_LIB_PREBUILT_PATH_RELEASE}/${LIB_NAME}.lib"
      IMPORTED_LOCATION_RELEASEINTERNAL "${LLD_LIB_PREBUILT_PATH_RELEASE}/${LIB_NAME}.lib"
      IMPORTED_LOCATION_DEBUG "${LLD_LIB_PREBUILT_PATH_DEBUG}/${LIB_NAME}.lib"
      )
  else()
    if(NOT DEFINED IGC_OPTION__LLDELF_LIB_DIR)
      set(IGC_OPTION__LLDELF_LIB_DIR ${LLVM_ROOT})
    endif()
    message(STATUS "[${LIB_NAME}] searching in: ${IGC_OPTION__LLDELF_LIB_DIR}")
    find_library(${LIB_NAME}_PATH
      ${LIB_NAME}
      PATHS ${IGC_OPTION__LLDELF_LIB_DIR}
      PATH_SUFFIXES "lib" "llvm-${LLVM_VERSION_MAJOR}/lib")

    message(STATUS "[${LIB_NAME}] found = ${${LIB_NAME}_PATH}")
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
      PATHS "${IGC_OPTION__LLD_DIR}/bin"
      PATH_SUFFIXES "llvm-${LLVM_VERSION_MAJOR}/bin")

    if("${${EXE_NAME}_PATH_RELEASE}" STREQUAL "${EXE_NAME}_PATH_RELEASE-NOTFOUND")
      message(FATAL_ERROR
      "Cannot find ${EXE_NAME} executable in Release version in ${IGC_OPTION__LLD_DIR}")
    endif()

    #We only need the Release version of lld.exe
    add_executable(${EXE_NAME} IMPORTED GLOBAL)
    set_target_properties(${EXE_NAME} PROPERTIES
      IMPORTED_LOCATION "${${EXE_NAME}_PATH_RELEASE}"
      )
  else()
    if(NOT DEFINED IGC_OPTION__LLD_BIN_DIR)
      set(IGC_OPTION__LLD_BIN_DIR ${LLVM_ROOT})
    endif()
    message(STATUS "[${EXE_NAME}] searching in: ${IGC_OPTION__LLD_BIN_DIR}")
    find_program(${EXE_NAME}_PATH
      ${EXE_NAME}
      PATHS ${IGC_OPTION__LLD_BIN_DIR}
      PATH_SUFFIXES "bin" "llvm-${LLVM_VERSION_MAJOR}/bin")

    message(STATUS "[${EXE_NAME}] found = ${${EXE_NAME}_PATH}")
    if(${EXE_NAME}_PATH-NOTFOUND)
      message(FATAL_ERROR "Cannot find ${EXE_NAME} executable, please install missing executable or provide the path by IGC_OPTION__LLD_BIN_DIR")
    endif()

    add_executable(${EXE_NAME} IMPORTED GLOBAL)
    set_target_properties(${EXE_NAME} PROPERTIES IMPORTED_LOCATION ${${EXE_NAME}_PATH})
  endif()
endfunction()

if(IGC_BUILD__LLVM_SOURCES)
  message(STATUS "[lldELF] from sources: ${IGC_BUILD__LLVM_SOURCES}")
  get_target_property(lldELF_SRC_DIR lldELF SOURCE_DIR)
  set(LLD_INCLUDE_DIR "${lldELF_SRC_DIR}/../include")
elseif(IGC_BUILD__LLVM_PREBUILDS)
  message(STATUS "[lldELF] from prebuilds: ${IGC_BUILD__LLVM_PREBUILDS}")
  message(STATUS "[lldELF] extra dir: ${IGC_OPTION__LLD_DIR}")
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

  if(LLVM_ENABLE_ZLIB)
    find_package(ZLIB)
    target_link_libraries(lldELF INTERFACE ZLIB::ZLIB)
  endif()

  if(LLVM_ENABLE_ZSTD)
    find_package(zstd)
    if(zstd_LIBRARY MATCHES "${zstd_SHARED_LIBRARY_SUFFIX}$")
      target_link_libraries(lldELF INTERFACE zstd::libzstd_shared)
    elseif(zstd_STATIC_LIBRARY MATCHES "${zstd_STATIC_LIBRARY_SUFFIX}$")
      target_link_libraries(lldELF INTERFACE zstd::libzstd_static)
    else()
      message(FATAL_ERROR "Cannot find zstd library. Please install the missing library.")
    endif()
  endif()

  if(NOT DEFINED IGC_OPTION__LLDELF_H_DIR)
    set(IGC_OPTION__LLDELF_H_DIR ${LLVM_ROOT})
  endif()
  message(STATUS "[Driver.h] searching in: ${IGC_OPTION__LLDELF_H_DIR}")

  find_path(
    LLD_INCLUDE_DIR
    NAMES "Driver.h"
    PATHS ${IGC_OPTION__LLDELF_H_DIR}
    PATH_SUFFIXES
      "llvm-${LLVM_VERSION_MAJOR}/include/lld/Common/"
      "include/lld/Common/"
  )

  message(STATUS "[Driver.h] found = ${LLD_INCLUDE_DIR}")

  if(LLD_INCLUDE_DIR-NOTFOUND)
    message(FATAL_ERROR
    "Cannot find 'lld/Common/Driver.h' header, please install missing header or provide the path by IGC_OPTION__LLDELF_H_DIR")
  endif()
  get_filename_component(LLD_INCLUDE_DIR "${LLD_INCLUDE_DIR}/../.." ABSOLUTE)
else()
  message(STATUS "[lldELF] neither source nor prebuilds.")
endif()

list(APPEND IGC_BUILD__LLVM_LIBS_TO_LINK
  lldELF)

message(STATUS "[lldELF] include dir: ${LLD_INCLUDE_DIR}")
include_directories(${LLD_INCLUDE_DIR})
