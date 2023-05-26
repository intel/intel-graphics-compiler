#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
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
    PATHS ${LLVM_ROOT}
    )
  message(STATUS "[IGC] Found LLVM: ${LLVM_DIR}")

  set(LLVM_DIR "${LLVM_DIR}" PARENT_SCOPE)

  set(LLVM_DIR_CONFIGURATION_ADJUSTED "${LLVM_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>$<$<CONFIG:ReleaseInternal>:Release>")

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
  set(LLVM_LINK_EXE "${LLVM_DIR}/Release/bin/llvm-link.exe" CACHE STRING "")
  set(LLVM_AS_EXE "${LLVM_DIR}/Release/bin/llvm-as.exe" CACHE STRING "")
  set(LLVM_OPT_EXE "${LLVM_DIR}/Release/bin/opt.exe" CACHE STRING "")
  set(LLVM_TABLEGEN_EXE "${LLVM_DIR}/Release/bin/llvm-tblgen.exe" CACHE STRING "")

  set(LLVM_INCLUDE_DIRS "${LLVM_DIR_CONFIGURATION_ADJUSTED}/include;${DEFAULT_IGC_LLVM_SOURCES_DIR}/llvm/include")
  set(LLVM_INCLUDE_DIRS "${LLVM_INCLUDE_DIRS}" PARENT_SCOPE)
else()
  set(LLVM_LINK_EXE "llvm-link" CACHE STRING "")
  set(LLVM_AS_EXE "llvm-as" CACHE STRING "")
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

set(IGC_LLVM_DEPENDENT_CLANG_FLAGS "")
set(IGC_LLVM_DEPENDENT_OPT_FLAGS "")

# Disable the opaque pointers' usage explicitly, unless the block below deems that unnecessary
set(IGC_OPAQUE_POINTERS_FORCE_DISABLED ON)
if(IGC_OPTION__LLVM_OPAQUE_POINTERS_ENABLED)
  if(LLVM_VERSION_MAJOR LESS 14)
    message(WARNING "IGC_OPTION__LLVM_OPAQUE_POINTERS_ENABLED ignored: opaque pointers are not available prior to LLVM 14")
  endif()
  set(IGC_OPAQUE_POINTERS_FORCE_DISABLED OFF)
elseif(LLVM_VERSION_MAJOR LESS 15)
  # Opaque pointers are either absent (LLVM <14) or disabled by default. No need to force-disable
  set(IGC_OPAQUE_POINTERS_FORCE_DISABLED OFF)
endif(IGC_OPTION__LLVM_OPAQUE_POINTERS_ENABLED)

if(IGC_OPAQUE_POINTERS_FORCE_DISABLED)
  # Once we've figured out that explicit disabling is needed, propagate
  # corresponding options to all the in-tree calls of clang/opt tools.
  list(APPEND IGC_LLVM_DEPENDENT_CLANG_FLAGS "-no-opaque-pointers")
  list(APPEND IGC_LLVM_DEPENDENT_OPT_FLAGS "-opaque-pointers=0")
  # Also inform the preprocessor.
  add_compile_definitions(__IGC_OPAQUE_POINTERS_FORCE_DISABLED__)
endif()

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
