#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Order of chosing way how to take opencl-clang
#1. CCLANG_FROM_SYSTEM - use installed on system opencl-clang toolchain
#2. CCLANG_BUILD_PREBUILDS - use prebuilt opencl-clang toolchain
#   CCLANG_BUILD_PREBUILDS_DIR - set path to prebuilt cclang folder
#3. CCLANG_BUILD_INTREE_LLVM - use sources of opencl-clang toolchain
#
# If defined CCLANG_INSTALL_PREBUILDS_DIR, opencl-clang will be force
# installed from the given location. BiF compilation still follows
# scheme above
#
# IGC_OPTION__OPENCL_HEADER_PATH - Specify path to opencl-c.h header

if(NOT DEFINED COMMON_CLANG_LIBRARY_NAME)
  set(COMMON_CLANG_LIBRARY_NAME opencl-clang)
endif()

set(COMMON_CLANG_LIB_NAME_WITH_PREFIX "lib${COMMON_CLANG_LIBRARY_NAME}")
set(COMMON_CLANG_LIB_FULL_NAME "${COMMON_CLANG_LIB_NAME_WITH_PREFIX}*${CMAKE_SHARED_LIBRARY_SUFFIX}")

find_library(CCLANG_FROM_SYSTEM ${COMMON_CLANG_LIBRARY_NAME})

### Check if user manual setup some of flag
if(NOT CCLANG_BUILD_PREBUILDS)
  set(CCLANG_BUILD_PREBUILDS FALSE)
else()
  set(CCLANG_FROM_SYSTEM FALSE)
  set(CCLANG_BUILD_INTREE_LLVM FALSE)
endif()

if(NOT CCLANG_BUILD_INTREE_LLVM)
  set(CCLANG_BUILD_INTREE_LLVM FALSE)
else()
  set(CCLANG_BUILD_PREBUILDS FALSE)
  set(CCLANG_FROM_SYSTEM FALSE)
endif()
###

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  string(TOLOWER "${OS_NAME}" OS_NAME_LOWER_CASE)
  set(LINUX_PATH_GENERIC "prebuild-opencl-clang-linux/linux/${OS_NAME_LOWER_CASE}/${OS_VERSION_NUMBER}")
  set(LINUX_PATH_VERSIONED "prebuild-opencl-clang-linux-${OS_VERSION_NUMBER}/linux/${OS_NAME_LOWER_CASE}/${OS_VERSION_NUMBER}")

  if (EXISTS ${IGC_BUILD__GFX_DEV_SRC_DIR}/../../${LINUX_PATH_VERSIONED})
    set(LINUX_PATH "${LINUX_PATH_VERSIONED}")
  else()
    set(LINUX_PATH "${LINUX_PATH_GENERIC}")
  endif()
  message("Prebuild OpenCL Clang Linux Path: ${LINUX_PATH}")
else()
  set(WINDOWS_PATH "prebuild-opencl-clang_${IGC_BUILD__CLANG_VERSION}/windows/Release/${cpuSuffix}")
  message("Prebuild OpenCL Clang Windows Path: ${WINDOWS_PATH}")
endif()
### Check by order first available way to link with opencl-clang
if(NOT CCLANG_FROM_SYSTEM)
  if(NOT CCLANG_BUILD_PREBUILDS_DIR)
    # Detect CPU architecture
    igc_arch_get_cpu(cpuSuffix)
    if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND EXISTS "${IGC_BUILD__GFX_DEV_SRC_DIR}/../../${WINDOWS_PATH}")
      set(CCLANG_BUILD_PREBUILDS_DIR "${IGC_BUILD__GFX_DEV_SRC_DIR}/../../${WINDOWS_PATH}")
    endif()

    if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND EXISTS "${IGC_BUILD__GFX_DEV_SRC_DIR}/../../${LINUX_PATH}")
      set(CCLANG_BUILD_PREBUILDS_DIR "${IGC_BUILD__GFX_DEV_SRC_DIR}/../../${LINUX_PATH}")
    endif()
    if(NOT EXISTS ${CCLANG_BUILD_PREBUILDS_DIR})
      if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND EXISTS "${IGC_BUILD__GFX_DEV_SRC_DIR}/../${WINDOWS_PATH}")
        set(CCLANG_BUILD_PREBUILDS_DIR "${IGC_BUILD__GFX_DEV_SRC_DIR}/../${WINDOWS_PATH}")
      endif()
    if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND EXISTS "${IGC_BUILD__GFX_DEV_SRC_DIR}/../${LINUX_PATH}")
      set(CCLANG_BUILD_PREBUILDS_DIR "${IGC_BUILD__GFX_DEV_SRC_DIR}/../${LINUX_PATH}")
    endif()
    endif()
  endif()
  set(CCLANG_BUILD_INTREE_LLVM_DIR ${LLVM_SOURCE_DIR}/projects/opencl-clang)
  ### Check if user by choosing some way of linking with opencl-clang provided required folders
  if(${CCLANG_BUILD_PREBUILDS})
    if(NOT EXISTS ${CCLANG_BUILD_PREBUILDS_DIR})
      unset(CCLANG_BUILD_PREBUILDS_DIR)
      set(CCLANG_BUILD_PREBUILDS_DIR "/opt/intel-cclang-static-${IGC_BUILD__CLANG_VERSION}")
      if(NOT EXISTS ${CCLANG_BUILD_PREBUILDS_DIR})
        message(FATAL_ERROR "[IGC] : User setup to use prebuilt opencl-clang but not found folder : ${CCLANG_BUILD_PREBUILDS_DIR}")
        set(CCLANG_BUILD_PREBUILDS FALSE)
      endif()
    endif()
  elseif(${CCLANG_BUILD_INTREE_LLVM})
    if(NOT EXISTS ${CCLANG_BUILD_INTREE_LLVM_DIR})
      message(FATAL_ERROR "[IGC] : User setup to use sources of opencl-clang but not found folder : ${CCLANG_BUILD_INTREE_LLVM_DIR}")
      set(CCLANG_BUILD_INTREE_LLVM FALSE)
    endif()
  ###
  ### User didn't define nothing, then pick the method 2 or 3.
  elseif(EXISTS ${CCLANG_BUILD_PREBUILDS_DIR})
    set(CCLANG_BUILD_PREBUILDS TRUE)
  elseif(EXISTS ${CCLANG_BUILD_INTREE_LLVM_DIR})
    message(STATUS "[IGC] : opencl-clang will be taken from sources")
    set(CCLANG_BUILD_INTREE_LLVM TRUE)
  else()
    message(FATAL_ERROR "[IGC] : Cannot find opencl-clang tool-chain, please provide sources or install it on system.")
  endif()
  ###

endif()
###

#1. CCLANG_FROM_SYSTEM - use installed on system opencl-clang toolchain
if(CCLANG_FROM_SYSTEM)
  message(STATUS "[IGC] : opencl-clang will be taken from system")

  find_library(SYSTEM_COMMON_CLANG ${COMMON_CLANG_LIBRARY_NAME})

  add_library(opencl-clang-lib SHARED IMPORTED GLOBAL)
  if(DEFINED CCLANG_INSTALL_PREBUILDS_DIR)
    set_property(TARGET opencl-clang-lib PROPERTY "IMPORTED_LOCATION" "${CCLANG_INSTALL_PREBUILDS_DIR}/${COMMON_CLANG_LIB_FULL_NAME}")
  else()
    set_property(TARGET opencl-clang-lib PROPERTY "IMPORTED_LOCATION" "${SYSTEM_COMMON_CLANG}")
  endif()
  find_program(CLANG_EXE clang-${IGC_BUILD__CLANG_VERSION_MAJOR})
  if(CLANG_EXE)
    message(STATUS "[IGC] Found clang-${IGC_BUILD__CLANG_VERSION_MAJOR} executable: ${CLANG_EXE}")

    add_executable(clang-tool IMPORTED GLOBAL)
    set_property(TARGET clang-tool PROPERTY "IMPORTED_LOCATION" "${CLANG_EXE}")
    set(CL_OPTIONS "-finclude-default-header")

    if(DEFINED IGC_OPTION__OPENCL_HEADER_PATH)
      message(STATUS "[IGC] : IGC_OPTION__OPENCL_HEADER_PATH is set to ${IGC_OPTION__OPENCL_HEADER_PATH} therefore try to search in user defined path")
      if(NOT EXISTS "${IGC_OPTION__OPENCL_HEADER_PATH}")
        message(FATAL_ERROR "[IGC] : couldn't find opencl-c.h in user defined path IGC_OPTION__OPENCL_HEADER_PATH=${IGC_OPTION__OPENCL_HEADER_PATH}")
      endif() # NOT EXISTS "${IGC_OPTION__OPENCL_HEADER_PATH}"
      set(opencl-header "${IGC_OPTION__OPENCL_HEADER_PATH}")
      message(STATUS "[IGC] Found opencl-c.h: ${opencl-header}")
    else(DEFINED IGC_OPTION__OPENCL_HEADER_PATH)
      # Get path to opencl-c.h based on the location of CLANG_EXE
      get_filename_component(CLANG_EXE_PARENT_DIR ${CLANG_EXE} DIRECTORY)
      file(GLOB_RECURSE opencl-header ${CLANG_EXE_PARENT_DIR}/../*opencl-c.h)
      if(opencl-header)
        message(STATUS "[IGC] Found opencl-c.h: ${opencl-header}")
      else(opencl-header)
        message(FATAL_ERROR "[IGC] : Couldn't find opencl-c.h, please provide it.")
      endif(opencl-header)
    endif() # DEFINED IGC_OPTION__OPENCL_HEADER_PATH
  else(CLANG_EXE)
    message(FATAL_ERROR "[IGC] : Couldn't find clang-${IGC_BUILD__CLANG_VERSION_MAJOR} executable, please install it.")
  endif(CLANG_EXE)
###
#2. CCLANG_BUILD_PREBUILDS - use prebuilt opencl-clang toolchain
elseif(${CCLANG_BUILD_PREBUILDS})
  message(STATUS "[IGC] : opencl-clang will be taken from prebuilds")

  # Find CLANG_TOOL recursively in CCLANG_BUILD_PREBUILDS_DIR
  file(GLOB_RECURSE CLANG_TOOL_PATH ${CCLANG_BUILD_PREBUILDS_DIR}/*clang${CMAKE_EXECUTABLE_SUFFIX})
  message(STATUS "[IGC] : Find CLANG_TOOL in : ${CLANG_TOOL_PATH}")
  set(LLVM_PACKAGE_VERSION "${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH}${LLVM_VERSION_SUFFIX}")

  if(CMAKE_CROSSCOMPILING)
    # In case of cross compilation we could not execute prebuilt opencl library,
    # so we trust version number provided externally or assume it's the same as
    # LLVM version
    if(NOT DEFINED CLANG_TOOL_VERSION)
      set(CLANG_TOOL_VERSION "${LLVM_PACKAGE_VERSION}")
    endif()
  else(CMAKE_CROSSCOMPILING)
    # Get clang-tool version
    execute_process(
      COMMAND ${CLANG_TOOL_PATH} -v
      ERROR_VARIABLE CLANG_TOOL_V_CALL)
    string(REGEX MATCH "clang version ([0-9]*\\.[0-9]*\\.[0-9]*[a-zA-Z0-9]*)" CLANG_TOOL_VERSION "${CLANG_TOOL_V_CALL}")
    set(CLANG_TOOL_VERSION "${CMAKE_MATCH_1}")

    # Check if we parse clang tool version correctly
    if(NOT CLANG_TOOL_VERSION)
      message(FATAL_ERROR "[IGC] : Cannot read version of clang tool, please check the output of execution `clang -v` : ${CLANG_TOOL_V_CALL}")
    endif()
  endif(CMAKE_CROSSCOMPILING)

  # Check if llvm version for IGC is newer or equal with the clang-tool version
  if(${LLVM_PACKAGE_VERSION} VERSION_GREATER ${CLANG_TOOL_VERSION} OR
     ${LLVM_PACKAGE_VERSION} EQUAL ${CLANG_TOOL_VERSION})
    add_library(opencl-clang-lib SHARED IMPORTED GLOBAL)

    if(DEFINED CCLANG_INSTALL_PREBUILDS_DIR)
      set_property(TARGET opencl-clang-lib PROPERTY "IMPORTED_LOCATION" "${CCLANG_INSTALL_PREBUILDS_DIR}/${COMMON_CLANG_LIB_FULL_NAME}")
    else()
      # Find opencl-clang-lib recursively in CCLANG_BUILD_PREBUILDS_DIR
      file(GLOB_RECURSE OPENCL-CLANG_PATH ${CCLANG_BUILD_PREBUILDS_DIR}/*${COMMON_CLANG_LIB_FULL_NAME})
      message(STATUS "[IGC] : Find opencl-clang-lib in : ${OPENCL-CLANG_PATH}")
      set_property(TARGET opencl-clang-lib PROPERTY "IMPORTED_LOCATION" "${OPENCL-CLANG_PATH}")
    endif()

    add_executable(clang-tool IMPORTED GLOBAL)
    set_property(TARGET clang-tool PROPERTY "IMPORTED_LOCATION" "${CLANG_TOOL_PATH}")

    # Find opencl-header recursively in CCLANG_BUILD_PREBUILDS_DIR
    file(GLOB_RECURSE opencl-header ${CCLANG_BUILD_PREBUILDS_DIR}/*opencl-c.h)
  else()
    message(FATAL_ERROR "[IGC] : The clang-tool(${CLANG_TOOL_VERSION}) from prebuilts is newer than llvm(${LLVM_PACKAGE_VERSION}) version for IGC.")
  endif()
###
#3. CCLANG_BUILD_INTREE_LLVM - use sources of opencl-clang toolchain
elseif(${CCLANG_BUILD_INTREE_LLVM})
  message(STATUS "[IGC] : opencl-clang will be taken from sources")
  if(DEFINED CCLANG_INSTALL_PREBUILDS_DIR)
    add_library(opencl-clang-lib SHARED IMPORTED GLOBAL)
    set_property(TARGET opencl-clang-lib PROPERTY "IMPORTED_LOCATION" "${CCLANG_INSTALL_PREBUILDS_DIR}/${COMMON_CLANG_LIB_FULL_NAME}")
  else()
    add_library(opencl-clang-lib ALIAS ${COMMON_CLANG_LIBRARY_NAME})
  endif()
  add_executable(clang-tool ALIAS clang)
  get_target_property(CLANG_SOURCE_DIR clang SOURCE_DIR)
  set(opencl-header "${CLANG_SOURCE_DIR}/../../lib/Headers/opencl-c.h")
endif()
###