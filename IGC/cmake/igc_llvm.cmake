#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_guard(DIRECTORY)

if(NOT IGC_BUILD__LLVM_SOURCES)
  message(STATUS "[IGC] IGC will take prebuilt LLVM")
  message(STATUS "[IGC] Searching for prebuilt LLVM in ${LLVM_ROOT} and system directories")
  find_package(LLVM ${IGC_OPTION__LLVM_PREFERRED_VERSION}
    REQUIRED
    PATHS ${LLVM_ROOT}/bin_and_cmake
    )
  message(STATUS "[IGC] Found LLVM: ${LLVM_DIR}")

  set(LLVM_DIR "${LLVM_DIR}" PARENT_SCOPE)

  set(LLVM_DIR_CONFIGURATION_ADJUSTED "${LLVM_DIR}/../$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>$<$<CONFIG:ReleaseInternal>:Release>/${cpuSuffix}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>$<$<CONFIG:ReleaseInternal>:Release>")

  # Tell the build that we are using prebuilds.
  set(IGC_BUILD__LLVM_PREBUILDS ON)
  set(IGC_OPTION__LLDELF_LIB_DIR ${LLVM_ROOT} PARENT_SCOPE)
endif()

# Include commonly used modules.
set(CMAKE_MODULE_PATH
  ${LLVM_CMAKE_DIR}
  ${CMAKE_MODULE_PATH}
  )

if(IGC_OPTION__LLVM_MODE STREQUAL PREBUILDS_MODE_NAME AND WIN32)
  set(IGC_OPTION__LLVM_MODE "${IGC_OPTION__LLVM_MODE}" CACHE STRING "")

  set(IGC_OPTION__LLD_DIR "${LLVM_DIR}")
  set(IGC_OPTION__LLDELF_H_DIR "${DEFAULT_IGC_LLVM_SOURCES_DIR}/lld")

  #No need to use anything but Release versions of these binaries.
  #This should speed up overall build time
  set(LLVM_LINK_EXE "${LLVM_DIR}/bin/llvm-link.exe" CACHE STRING "")
  set(LLVM_AS_EXE "${LLVM_DIR}/bin/llvm-as.exe" CACHE STRING "")
  set(LLVM_DIS_EXE "${LLVM_DIR}/bin/llvm-dis.exe" CACHE STRING "")
  set(LLVM_OPT_EXE "${LLVM_DIR}/bin/opt.exe" CACHE STRING "")
  set(LLVM_TABLEGEN_EXE "${LLVM_DIR}/bin/llvm-tblgen.exe" CACHE STRING "")
  set(LLVM_INCLUDE_DIRS "${LLVM_DIR_CONFIGURATION_ADJUSTED}/include;${DEFAULT_IGC_LLVM_SOURCES_DIR}/llvm/include")
  set(LLVM_INCLUDE_DIRS "${LLVM_INCLUDE_DIRS}" PARENT_SCOPE)
else()
  set(LLVM_LINK_EXE "llvm-link" CACHE STRING "")
  set(LLVM_AS_EXE "llvm-as" CACHE STRING "")
  set(LLVM_DIS_EXE "llvm-dis" CACHE STRING "")
  set(LLVM_OPT_EXE "opt" CACHE STRING "")

  set(LLVM_TABLEGEN_EXE "llvm-tblgen")
  if(CMAKE_CROSSCOMPILING)
    if(DEFINED LLVM_TABLEGEN)
      set(LLVM_TABLEGEN_EXE ${LLVM_TABLEGEN})
    else()
      find_program(LLVM_TABLEGEN_EXE "llvm-tblgen" ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
    endif()
  endif()
endif()

include(AddLLVM)
include(TableGen)
# Set LLVM_TABLEGEN_FLAGS manually based on include dirs.
list(TRANSFORM LLVM_INCLUDE_DIRS PREPEND "-I=" OUTPUT_VARIABLE LLVM_TABLEGEN_FLAGS)

# Add major version definition for llvm wrapper.
add_compile_definitions(LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR})

# Based on the default behavior for opaque/typed pointers, propagate
# corresponding options to all the in-tree calls of clang/opt tools.



if(LLVM_VERSION_MAJOR GREATER_EQUAL 17)
  message("Force Enable Opaque Pointers due to LLVM >= 17")
  SET(IGC_OPTION__API_ENABLE_OPAQUE_POINTERS ON)
else()
  set(IGC_BUILD__OPAQUE_POINTERS_ENABLE_OPT "-opaque-pointers=1")
  set(IGC_BUILD__OPAQUE_POINTERS_DISABLE_OPT "-opaque-pointers=0")
  set(IGC_BUILD__OPAQUE_POINTERS_ENABLE_CLANG "-opaque-pointers")

  if(IGC_BUILD__CLANG_VERSION_MAJOR GREATER_EQUAL 15)
    set(IGC_BUILD__OPAQUE_POINTERS_DISABLE_CLANG "-no-opaque-pointers")
  endif()
endif()

if(IGC_OPTION__API_ENABLE_OPAQUE_POINTERS)
  if(LLVM_VERSION_MAJOR GREATER_EQUAL 14)
    set(IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT ${IGC_BUILD__OPAQUE_POINTERS_ENABLE_OPT})
    set(IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG ${IGC_BUILD__OPAQUE_POINTERS_ENABLE_CLANG})
    add_compile_definitions(__IGC_OPAQUE_POINTERS_API_ENABLED=true)
  endif()
else(IGC_OPTION__API_ENABLE_OPAQUE_POINTERS)
  set(IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT ${IGC_BUILD__OPAQUE_POINTERS_DISABLE_OPT})
  set(IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG ${IGC_BUILD__OPAQUE_POINTERS_DISABLE_CLANG})
  add_compile_definitions(__IGC_OPAQUE_POINTERS_API_ENABLED=false)
endif()

message("OPT OpaquePtrs Status: ${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT}")
message("Clang OpaquePtrs Status: ${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG}")
add_compile_definitions(__IGC_OPAQUE_POINTERS_DEFAULT_ARG_CLANG="${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG}")

# Include LLVM headers as system ones.
# This will disable warnings on linux.
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
# Disable warnings from LLVM headers for msvc.
if(MSVC_IDE)
  add_compile_options(/experimental:external)
  foreach(INCDIR IN LISTS LLVM_INCLUDE_DIRS)
    add_compile_options("SHELL:/external:I ${INCDIR}")
  endforeach()
  add_compile_options(/external:W0)
endif()

if(IGC_OPTION__ENABLE_LIT_TESTS)
  # Lit testing infrastructure. Find lit tools for tests.
  igc_find_external_lit()
endif()

set(IGC_LLVM_COMPONENTS
  AggressiveInstCombine
  Analysis
  AsmParser
  BinaryFormat
  BitReader
  BitWriter
  CodeGen
  Core
  Demangle
  InstCombine
  Instrumentation
  Ipo
  IRReader
  Linker
  MC
  MCParser
  ObjCARCOpts
  Object
  ProfileData
  Remarks
  ScalarOpts
  Support
  Target
  TransformUtils
  Vectorize
  )

  list(APPEND IGC_LLVM_COMPONENTS
    InstCombine
    )

if(LLVM_VERSION_MAJOR GREATER_EQUAL 9)
  list(APPEND IGC_LLVM_COMPONENTS
    BitstreamReader
    )
endif()

igc_get_llvm_targets(IGC_BUILD__LLVM_LIBS_TO_LINK ${IGC_LLVM_COMPONENTS})
