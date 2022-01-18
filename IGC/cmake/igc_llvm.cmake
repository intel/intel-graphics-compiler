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
  find_package(LLVM ${IGC_OPTION__LLVM_PREFERRED_VERSION} REQUIRED)
  message(STATUS "[IGC] Found LLVM: ${LLVM_DIR}")

  # Tell the build that we are using prebuilds.
  set(IGC_BUILD__LLVM_PREBUILDS ON)
endif()

# Include commonly used modules.
set(CMAKE_MODULE_PATH
  ${LLVM_CMAKE_DIR}
  ${CMAKE_MODULE_PATH}
  )

set(LLVM_TABLEGEN_EXE "llvm-tblgen")
if(CMAKE_CROSSCOMPILING)
  if(DEFINED LLVM_TABLEGEN)
    set(LLVM_TABLEGEN_EXE ${LLVM_TABLEGEN})
  else()
    find_program(LLVM_TABLEGEN_EXE "llvm-tblgen" ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
  endif()
endif()

set(LLVM_LINK_EXE "llvm-link" CACHE STRING "")

include(AddLLVM)
include(TableGen)
# Set LLVM_TABLEGEN_FLAGS manually based on include dirs.
list(TRANSFORM LLVM_INCLUDE_DIRS PREPEND "-I=" OUTPUT_VARIABLE LLVM_TABLEGEN_FLAGS)

# Add major version definition for llvm wrapper.
add_compile_definitions(LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR})

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
  "ipo"
  "IRReader"
  "BitWriter"
  "BinaryFormat"
  "AsmParser"
  "BitReader"
  "Linker"
  "CodeGen"
  "ScalarOpts"
  "TransformUtils"
  "Analysis"
  "Target"
  "ObjCARCOpts"
  "Vectorize"
  "Instrumentation"
  "Object"
  "MCParser"
  "ProfileData"
  "MC"
  "Core"
  "Support"
  "Demangle"
  )

if(LLVM_VERSION_MAJOR GREATER_EQUAL 8)
  list(APPEND IGC_LLVM_COMPONENTS
    "InstCombine"
    )
endif()

if(LLVM_VERSION_MAJOR GREATER_EQUAL 9)
  list(APPEND IGC_LLVM_COMPONENTS
    "BitstreamReader"
    )
endif()

igc_get_llvm_targets(IGC_BUILD__LLVM_LIBS_TO_LINK ${IGC_LLVM_COMPONENTS})
