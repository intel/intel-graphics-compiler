#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#============================ end_copyright_notice =============================

# Order of choosing way how to take LLVM
#1. IGC_OPTION__LLVM_SOURCES - use llvm sources to build
#   IGC_OPTION__LLVM_STOCK_SOURCES - use LLVM stock sources or patched stock sources // By default False
#   IGC_OPTION__LLVM_SOURCES_DIR - set path to llvm sources folder
#2. IGC_OPTION__LLVM_PREBUILDS - use prebuilt llvm toolchain
#   IGC_OPTION__LLVM_PREBUILDS_DIR - set path to prebuilt llvm folder
#3. IGC_OPTION__LLVM_FROM_SYSTEM - use LLVM from system

#   IGC_OPTION__LLVM_PREFERRED_VERSION - define which version of llvm to pick, ex. "7.1.0" // By default 10.0.0


option(IGC_OPTION__LLVM_SOURCES "Use LLVM sources for build in-tree" OFF)
option(IGC_OPTION__LLVM_PREBUILDS "Use LLVM prebuild package" OFF)
option(IGC_OPTION__LLVM_FROM_SYSTEM "Use LLVM from system" OFF)

# Sanity check that only one mode enabled (or nothing).
if((IGC_OPTION__LLVM_SOURCES AND IGC_OPTION__LLVM_PREBUILDS) OR
    (IGC_OPTION__LLVM_SOURCES AND IGC_OPTION__LLVM_FROM_SYSTEM) OR
    (IGC_OPTION__LLVM_PREBUILDS AND IGC_OPTION__LLVM_FROM_SYSTEM))
  message(FATAL_ERROR "Only one LLVM mode can be selected explicitly for IGC!")
endif()


set(IGC_LLVM_TOOLS_DIR ${CMAKE_CURRENT_LIST_DIR})

### Get preferred version of LLVM ###
include(${IGC_LLVM_TOOLS_DIR}/llvm_preferred_version.cmake)
set(IGC_OPTION__LLVM_PREFERRED_VERSION ${DEFAULT_IGC_LLVM_VERSION} CACHE STRING "Preferred version of LLVM to use")

# Get default source dir.
include(${IGC_LLVM_TOOLS_DIR}/llvm_source_path.cmake)
# Handle dependent options for source build.
if(IGC_OPTION__LLVM_SOURCES)
  option(IGC_OPTION__LLVM_STOCK_SOURCES "Use stock or patched sources" OFF)
  set(IGC_OPTION__LLVM_SOURCES_DIR ${DEFAULT_IGC_LLVM_SOURCES_DIR} CACHE PATH "Path to LLVM sources")
endif()

# Get default prebuild dir.
include(${IGC_LLVM_TOOLS_DIR}/llvm_prebuilt_path.cmake)
# Handle dependent options for prebuild build.
if(IGC_OPTION__LLVM_PREBUILDS)
  set(IGC_OPTION__LLVM_PREBUILDS_DIR ${DEFAULT_IGC_LLVM_PREBUILDS_DIR} CACHE PATH "Path to LLVM prebuild")
endif()

# Nothing was specified, start searching.
if(NOT (IGC_OPTION__LLVM_SOURCES OR IGC_OPTION__LLVM_PREBUILDS OR IGC_OPTION__LLVM_FROM_SYSTEM))
  message(STATUS "No LLVM mode was selected explicitly")
  message(STATUS "IGC will search for LLVM sources first, then try prebuild and after try to take system LLVM")
  set(IGC_LOOKING_FOR_LLVM TRUE)
endif()

### Check by order first available way to link with LLVM ###
if(IGC_LOOKING_FOR_LLVM)
  if(EXISTS "${DEFAULT_IGC_LLVM_SOURCES_DIR}")
    set(IGC_FOUND_SOURCES TRUE)
    set(IGC_OPTION__LLVM_SOURCES_DIR ${DEFAULT_IGC_LLVM_SOURCES_DIR})
  elseif(EXISTS "${DEFAULT_IGC_LLVM_PREBUILDS_DIR}")
    set(IGC_FOUND_PREBUILDS TRUE)
    set(IGC_OPTION__LLVM_PREBUILDS_DIR ${DEFAULT_IGC_LLVM_PREBUILDS_DIR})
  else()
    set(IGC_USES_SYSTEM_LLVM TRUE)
  endif()
endif(IGC_LOOKING_FOR_LLVM)

if(IGC_OPTION__LLVM_SOURCES OR IGC_FOUND_SOURCES)
  if(NOT EXISTS "${IGC_OPTION__LLVM_SOURCES_DIR}")
    message(FATAL_ERROR "[IGC] Cannot find LLVM sources, please provide sources path by IGC_OPTION__LLVM_SOURCES_DIR flag")
  endif()

  message(STATUS "[IGC] IGC will build LLVM from sources.")
  message(STATUS "[IGC] LLVM sources folder : ${IGC_OPTION__LLVM_SOURCES_DIR}")
  message(STATUS "[IGC] LLVM sources in stock version : ${IGC_OPTION__LLVM_STOCK_SOURCES}")
  add_subdirectory(${IGC_LLVM_TOOLS_DIR} ${CMAKE_CURRENT_BINARY_DIR}/llvm/build)
  set(IGC_OPTION__LLVM_SOURCES ON)
endif()

if(IGC_OPTION__LLVM_PREBUILDS OR IGC_FOUND_PREBUILDS)
  if(NOT EXISTS "${IGC_OPTION__LLVM_PREBUILDS_DIR}")
    message(FATAL_ERROR "[IGC] Cannot find LLVM prebuilts, please provide path by IGC_OPTION__LLVM_PREBUILDS_DIR flag")
  endif()
  message(STATUS "[IGC] IGC will take LLVM prebuilts.")
  message(STATUS "[IGC] LLVM prebuilts folder : ${IGC_OPTION__LLVM_PREBUILDS_DIR}")
  set(LLVM_ROOT ${IGC_OPTION__LLVM_PREBUILDS_DIR})
  find_package(LLVM ${IGC_OPTION__LLVM_PREFERRED_VERSION} REQUIRED)
  set(IGC_OPTION__LLVM_PREBUILDS ON)
endif()

if(IGC_OPTION__LLVM_FROM_SYSTEM OR IGC_USES_SYSTEM_LLVM)
  message(STATUS "[IGC] IGC will take LLVM from system")
  find_package(LLVM ${IGC_OPTION__LLVM_PREFERRED_VERSION} REQUIRED)
  set(IGC_OPTION__LLVM_FROM_SYSTEM ON)
endif()


if(LLVM_LINK_LLVM_DYLIB)
    # LLVM was built and configured in a way that tools (in our case IGC) should be linked
    # against single LLVM dynamic library.
    set(IGC_BUILD__LLVM_LIBS_TO_LINK "LLVM")
    message(STATUS "[IGC] Link against LLVM dynamic library")
else()
    # LLVM was built into multiple libraries (static or shared).
    message(STATUS "[IGC] Link against LLVM static or shared component libs")

    # Link targets/dependencies (in required link order).
    # NOTE: Since the libraries are grouped in the same link group (in GCC/CLANG),
    #       there is no longer need to order in most dependant first manner.
    set(IGC_BUILD__LLVM_LIBS_TO_LINK
        "LLVMipo"
        "LLVMIRReader"
        "LLVMBitWriter"
        "LLVMBinaryFormat"
        "LLVMAsmParser"
        "LLVMBitReader"
        "LLVMLinker"
        "LLVMCodeGen"
        "LLVMScalarOpts"
        "LLVMTransformUtils"
        "LLVMAnalysis"
        "LLVMTarget"
        "LLVMObjCARCOpts"
        "LLVMVectorize"
        "LLVMInstrumentation"
        "LLVMObject"
        "LLVMMCParser"
        "LLVMProfileData"
        "LLVMMC"
        "LLVMCore"
        "LLVMSupport"
        "LLVMDemangle"
        )

    if(LLVM_VERSION_MAJOR GREATER_EQUAL 8)
        list(APPEND IGC_BUILD__LLVM_LIBS_TO_LINK
          "LLVMInstCombine"
          )
    endif()

    if(LLVM_VERSION_MAJOR GREATER_EQUAL 9)
        list(APPEND IGC_BUILD__LLVM_LIBS_TO_LINK
          "LLVMBitstreamReader"
          )
    endif()
endif()
