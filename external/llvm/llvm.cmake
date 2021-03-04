#===================== begin_copyright_notice ==================================

#Copyright (c) 2017 Intel Corporation

#Permission is hereby granted, free of charge, to any person obtaining a
#copy of this software and associated documentation files (the
#"Software"), to deal in the Software without restriction, including
#without limitation the rights to use, copy, modify, merge, publish,
#distribute, sublicense, and/or sell copies of the Software, and to
#permit persons to whom the Software is furnished to do so, subject to
#the following conditions:

#The above copyright notice and this permission notice shall be included
#in all copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#======================= end_copyright_notice ==================================

# Order of choosing way how to take LLVM
#1. IGC_OPTION__LLVM_SOURCES - use llvm sources to build
#   IGC_OPTION__LLVM_STOCK_SOURCES - use LLVM stock sources or patched stock sources // By default False
#   IGC_OPTION__LLVM_SOURCES_DIR - set path to llvm sources folder
#2. IGC_OPTION__LLVM_PREBUILDS - use prebuilt llvm toolchain
#   IGC_OPTION__LLVM_PREBUILDS_DIR - set path to prebuilt llvm folder
#3. IGC_OPTION__LLVM_FROM_SYSTEM - use LLVM from system

#   IGC_OPTION__LLVM_PREFERRED_VERSION - define which version of llvm to pick, ex. "7.1.0" // By default 10.0.0

### Check if user manual setup some of flag
if(NOT IGC_OPTION__LLVM_SOURCES)
  set(IGC_OPTION__LLVM_SOURCES FALSE)
elseif(${IGC_OPTION__LLVM_SOURCES})
  set(IGC_OPTION__LLVM_PREBUILDS FALSE)
  set(IGC_OPTION__LLVM_FROM_SYSTEM FALSE)
endif()

if(NOT IGC_OPTION__LLVM_PREBUILDS)
  set(IGC_OPTION__LLVM_PREBUILDS FALSE)
elseif(${IGC_OPTION__LLVM_PREBUILDS})
  set(IGC_OPTION__LLVM_FROM_SYSTEM FALSE)
endif()

if(NOT IGC_OPTION__LLVM_FROM_SYSTEM)
  set(IGC_OPTION__LLVM_FROM_SYSTEM FALSE)
endif()
###

if(NOT IGC_OPTION__LLVM_STOCK_SOURCES)
  set(IGC_OPTION__LLVM_STOCK_SOURCES FALSE)
endif()

set(IGC_LLVM_TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/llvm)

### Get preferred version of LLVM ###
if(NOT IGC_OPTION__LLVM_PREFERRED_VERSION)
  include(${IGC_LLVM_TOOLS_DIR}/llvm_preferred_version.cmake)
endif()

set(IGC_LOOKING_FOR_LLVM TRUE)

### Check by order first available way to link with LLVM ###
if(${IGC_OPTION__LLVM_SOURCES} OR (NOT ${IGC_OPTION__LLVM_PREBUILDS} AND NOT ${IGC_OPTION__LLVM_FROM_SYSTEM}))
  ### Get LLVM source code path
  if(NOT IGC_OPTION__LLVM_SOURCES_DIR)
    include(${IGC_LLVM_TOOLS_DIR}/llvm_source_path.cmake)
  endif()
  
  if(NOT EXISTS ${IGC_OPTION__LLVM_SOURCES_DIR})
    if(${IGC_OPTION__LLVM_SOURCES})
      # User wants to build from sources, but we couldn't find them.
      message(FATAL_ERROR "[IGC] : Cannot find LLVM sources, please provide sources path by IGC_OPTION__LLVM_SOURCES_DIR flag.")
    endif()
  else()
    set(IGC_LOOKING_FOR_LLVM FALSE)
    set(IGC_OPTION__LLVM_SOURCES TRUE)
  endif()
endif()
if(${IGC_LOOKING_FOR_LLVM} AND (${IGC_OPTION__LLVM_PREBUILDS} OR (NOT ${IGC_OPTION__LLVM_FROM_SYSTEM})))
  ### Get LLVM prebuilts path
  if(NOT IGC_OPTION__LLVM_PREBUILDS_DIR)
    include(${IGC_LLVM_TOOLS_DIR}/llvm_prebuilt_path.cmake)
  endif()

  if(NOT EXISTS ${IGC_OPTION__LLVM_PREBUILDS_DIR})
    if(${IGC_OPTION__LLVM_PREBUILDS})
      # User wants to build from prebuilts, but we couldn't find them.
      message(FATAL_ERROR "[IGC] : Cannot find LLVM prebuilts, please provide path by IGC_OPTION__LLVM_PREBUILDS_DIR flag.")
    endif()
  else()
    set(IGC_LOOKING_FOR_LLVM FALSE)
    set(IGC_OPTION__LLVM_PREBUILDS TRUE)
  endif()
endif()
if(${IGC_LOOKING_FOR_LLVM})
  # Try to find the LLVM in the system
  find_package(LLVM ${IGC_OPTION__LLVM_PREFERRED_VERSION})

  if(LLVM_FOUND)
    set(IGC_OPTION__LLVM_FROM_SYSTEM TRUE)
  else()
    if(${IGC_OPTION__LLVM_FROM_SYSTEM})
      # User wants to build using LLVM from system, but we couldn't find them.
      message(FATAL_ERROR "[IGC] : Cannot find LLVM in system in version ${IGC_OPTION__LLVM_PREFERRED_VERSION}. Please provide other version by flag IGC_OPTION__LLVM_PREFERRED_VERSION or install the missing one.")
	else()
	  message(FATAL_ERROR "[IGC] : Cannot find LLVM sources, prebuilt libraries or even installed package in system. Please provide LLVM.")
    endif()
  endif()
endif()
###

if(NOT DEFINED COMMON_CLANG_LIBRARY_NAME)
    set(COMMON_CLANG_LIBRARY_NAME opencl-clang)
endif()

if(WIN32)
    igc_arch_get_cpu(_cpuSuffix)
    set(COMMON_CLANG_LIBRARY_NAME ${COMMON_CLANG_LIBRARY_NAME}${_cpuSuffix})
    set(COMMON_CLANG_LIB_FULL_NAME "${COMMON_CLANG_LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")

    message(STATUS "OpenCL Clang library name to load: ${COMMON_CLANG_LIB_FULL_NAME}")

    add_compile_definitions(COMMON_CLANG_LIB_FULL_NAME=\"${COMMON_CLANG_LIB_FULL_NAME}\")
else()
    set(COMMON_CLANG_LIB_FULL_NAME "lib${COMMON_CLANG_LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

if(${IGC_OPTION__LLVM_SOURCES})
  message(STATUS "[IGC] IGC will build LLVM from sources.")
  message(STATUS "[IGC] LLVM sources folder : ${IGC_OPTION__LLVM_SOURCES_DIR}")
  message(STATUS "[IGC] LLVM sources in stock version : ${IGC_OPTION__LLVM_STOCK_SOURCES}")
  add_subdirectory(${IGC_LLVM_TOOLS_DIR} ${CMAKE_CURRENT_BINARY_DIR}/llvm/build)
elseif(${IGC_OPTION__LLVM_PREBUILDS})
  message(STATUS "[IGC] IGC will take LLVM prebuilts.")
  message(STATUS "[IGC] LLVM prebuilts folder : ${IGC_OPTION__LLVM_PREBUILDS_DIR}")
  include(${IGC_LLVM_PATCHER_DIR}/llvm_prebuilt.cmake)
elseif(${IGC_OPTION__LLVM_FROM_SYSTEM})
  message(STATUS "[IGC] IGC will take LLVM from system.")
endif()

if(LLVM_LINK_LLVM_DYLIB)
    # LLVM was built and configured in a way that tools (in our case IGC) should be linked
    # against single LLVM dynamic library.

    # SET_LLVM_LIB_PATH is a CMake variable that can be passed in to specify the location
    # to look for the LLVM .so. In some cases this is useful if multiple LLVM versions are installed
    if(SET_LLVM_LIB_PATH)
        set(IGC_BUILD__LLVM_LIBS_TO_LINK "${SET_LLVM_LIB_PATH}/libLLVM-${LLVM_VERSION_MAJOR}.so")
        message(STATUS "[IGC] Link against specified LLVM dylib ${IGC_BUILD__LLVM_LIBS_TO_LINK}")
    else()
        find_library(IGC_BUILD__LLVM_LIBS_TO_LINK "libLLVM-${LLVM_VERSION_MAJOR}.so")
        if(IGC_BUILD__LLVM_LIBS_TO_LINK)
          message(STATUS "[IGC] Link against found LLVM dylib ${IGC_BUILD__LLVM_LIBS_TO_LINK}")
        else()
          message(FATAL_ERROR "[IGC] Could not find the LLVM dylib. Aborting.")
        endif()
    endif()

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
