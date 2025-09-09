#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================


# CMake settings:
#  - IGC_OPTION__LIBRARY_NAME
# FCL
#  - FCL_OPTION__LIBRARY_NAME
#  - IGC_OPTION__ARCHITECTURE_HOST
#  - IGC_OPTION__ARCHITECTURE_TARGET
#  - IGC_OPTION__BIF_LINK_BC
#  - IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS
#  - IGC_OPTION__OUTPUT_DIR

cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Default languages: C, C++.
project(IGC)

if(UNIX)
  include(GNUInstallDirs)
endif()



# Increment IGC_API_MAJOR_VERSION if there is ABI Change.
if(NOT DEFINED IGC_API_MAJOR_VERSION)
  set(IGC_API_MAJOR_VERSION 2)
endif()
if(NOT DEFINED IGC_API_MINOR_VERSION)
  set(IGC_API_MINOR_VERSION 20)
endif()
if(NOT DEFINED IGC_API_PATCH_VERSION)
  set(IGC_API_PATCH_VERSION 0)
endif()

# IGC_BUILD_METADATA
# Used as version suffix, to distinguish builds with the same semantic version
# Eg. libigc-1.2.3+x and libigc-1.2.3+y
if(NOT DEFINED IGC_BUILD_METADATA)
  if(DEFINED ENV{IGC_BUILD_METADATA})
    set(IGC_BUILD_METADATA $ENV{IGC_BUILD_METADATA})
  else()
    set(IGC_BUILD_METADATA 0)
  endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# ======================================================================================================
# ================================================ UTILS ===============================================
# ======================================================================================================


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/BiFModule/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(igc_flags_utils)
include(igc_arch_utils)
include(igc_resource_utils)
include(igc_source_group_utils)
include(igc_llvm_utils)
include(igc_imported_tools)

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================


# ======================================================================================================
# ==================================== BUILD CONFIGURATIONS (part 1) ===================================
# ======================================================================================================

# IGC project available configuration types:
set(IGC_CMAKE_CONFIGURATION_TYPES
  "Debug"
  "Release"
)
set(IGC_CMAKE_CONFIGURATION_TYPES ${IGC_CMAKE_CONFIGURATION_TYPES} CACHE STRING "IGC available build configurations." FORCE)

if(CMAKE_CONFIGURATION_TYPES)
  # Multi-configuration generator: use CMAKE_CONFIGURATION_TYPES variable
  set(CMAKE_CONFIGURATION_TYPES ${IGC_CMAKE_CONFIGURATION_TYPES})

  if(CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "CMAKE_BUILD_TYPE variable is not allowed while using multi-configuration generator!")
  endif()
else()
  # Single-configuration generator: use CMAKE_BUILD_TYPE variable
  if(DEFINED CMAKE_BUILD_TYPE)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" _buildType)
    string(STRIP "${_buildType}" _buildType)

    if(_buildType MATCHES "^release$")
      set(_buildType "Release")
    elseif(_buildType MATCHES "^debug$")
      set(_buildType "Debug")
    else()
      set(_buildType "Release")
      message(WARNING "CMAKE_BUILD_TYPE: Unknown build configuration. The following configurations are available: ${IGC_CMAKE_CONFIGURATION_TYPES}.\nThe \"${_buildType}\" configuration will be used.\nThis value has meaning only for single-configuration generators (like Make). It will be ignored for MSVC/XCode.")
    endif()
  else()
    set(_buildType "Release")
    message(WARNING "CMAKE_BUILD_TYPE: No build configuration specified. The following configurations are available: ${IGC_CMAKE_CONFIGURATION_TYPES}.\nThe \"${_buildType}\" configuration will be used.\nThis value has meaning only for single-configuration generators (like Make). It will be ignored for MSVC/XCode.")
  endif()
  set(CMAKE_BUILD_TYPE "${_buildType}")
  unset(_buildType)
endif()

# Patterns which identify debug and release configurations.
set(IGC_BUILD__DEBUG_PATTERN   "^Debug$")
set(IGC_BUILD__RELEASE_PATTERN "^Release$")

# The configuration variables used below are likely to have been most recently
# defined by the project statement's call to the toolchain file
# ${CMAKE_TOOLCHAIN_FILE}.

# Some known cmake variables describing compiler:
# - CMAKE_CXX_COMPILER_ID == "Clang..." -> Clang
# - CMAKE_CXX_COMPILER_ID == "GNU"   -> GCC
# - CMAKE_CXX_COMPILER_ID == "Intel" -> Intel C++
# - CMAKE_CXX_COMPILER_ID == "MSVC"  -> using Visual Studio C++

# Local variables describing compilers:
# _igc_compiler_is_gnu       - C/C++ is gcc/g++ (not clang)
# _igc_compiler_is_clang     - C/C++ is clang/clang++ (not gcc/g++)
# _igc_compiler_is_gnu_or_clang  = C/C++ is either gnu or clang

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(_igc_compiler_is_gnu 1)
else()
  set(_igc_compiler_is_gnu 0)
endif()

if((CMAKE_C_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  set(_igc_compiler_is_clang 1)
else()
  set(_igc_compiler_is_clang 0)
endif((CMAKE_C_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))

if(_igc_compiler_is_gnu OR _igc_compiler_is_clang)
  set(_igc_compiler_is_gnu_or_clang 1)
else()
  set(_igc_compiler_is_gnu_or_clang 0)
endif(_igc_compiler_is_gnu OR _igc_compiler_is_clang)

#
#  First, Set IGC_OPTION__ARCHITECTURE_TARGET, IGC_OPTION__ARCHITECTURE_HOST,
#  and IGC_BUILD__CROSSCOMPILE_NEEDED.
#
igc_arch_detect(IGC_BUILD__DETECTED_ARCHITECTURE_TARGET IGC_BUILD__DETECTED_ARCHITECTURE_HOST)
igc_arch_validate(_detectedTargetArchValid "${IGC_BUILD__DETECTED_ARCHITECTURE_TARGET}")
igc_arch_validate(_detectedHostArchValid   "${IGC_BUILD__DETECTED_ARCHITECTURE_HOST}")

if(DEFINED IGC_OPTION__ARCHITECTURE_TARGET)
  igc_arch_normalize(_targetArchitecture "${IGC_OPTION__ARCHITECTURE_TARGET}")
elseif(_detectedTargetArchValid)
  set(_targetArchitecture "${IGC_BUILD__DETECTED_ARCHITECTURE_TARGET}")
  message(WARNING "IGC_OPTION__ARCHITECTURE_TARGET: Target architecture is not specified. Trying to deduce it from context.")
else()
  message(FATAL_ERROR "IGC_OPTION__ARCHITECTURE_TARGET: Target architecture is not specified and cannot be deduced from context.\nPlease specify one, e.g. Windows32, Linux64, Android32, ...")
endif()

if(DEFINED IGC_OPTION__ARCHITECTURE_HOST)
  igc_arch_normalize(_hostArchitecture "${IGC_OPTION__ARCHITECTURE_HOST}")
elseif(_detectedHostArchValid)
  set(_hostArchitecture "${IGC_BUILD__DETECTED_ARCHITECTURE_HOST}")
  if(CMAKE_CROSSCOMPILING)
    message(WARNING "IGC_OPTION__ARCHITECTURE_HOST: Host architecture is not specified. Trying to deduce it from context.")
  endif()
elseif(CMAKE_CROSSCOMPILING)
  message(FATAL_ERROR "IGC_OPTION__ARCHITECTURE_HOST: Host architecture is not specified and cannot be deduced from context.\nPlease specify one, e.g. Windows32, Linux64, Android32, ...")
else()
  set(_hostArchitecture "${_targetArchitecture}")
endif()

igc_arch_crosscompile_needed(IGC_BUILD__CROSSCOMPILE_NEEDED "${_targetArchitecture}" "${_hostArchitecture}")

set(IGC_OPTION__ARCHITECTURE_TARGET "${_targetArchitecture}" CACHE STRING "Target architecture for compilation." FORCE)
set(IGC_OPTION__ARCHITECTURE_HOST   "${_hostArchitecture}"   CACHE STRING "Architecture of host used during compilation. Only used when cross-compiling." FORCE)
unset(_detectedTargetArchValid)
unset(_detectedHostArchValid)
unset(_targetArchitecture)
unset(_hostArchitecture)


# GFX_DEVELOPMENT_DIR shall be provided by UFO build system
if(NOT GFX_DEVELOPMENT_DIR)
  get_filename_component(GFX_DEVELOPMENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../ ABSOLUTE)
endif()

find_program(PYTHON_EXECUTABLE NAMES "python3" "python")

set(IGC_CODEGEN_BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/autogen")
if(MSVC_IDE)
    set(IGC_CODEGEN_DIR "${IGC_CODEGEN_BASE_DIR}/${IGC_CMAKE_CFG_INTDIR}")
elseif(MSVC)
    set(IGC_CODEGEN_DIR "${IGC_CODEGEN_BASE_DIR}/${IGC_CMAKE_CFG_INTDIR}")
else()
    set(IGC_CODEGEN_DIR "${IGC_CODEGEN_BASE_DIR}")
endif()
add_subdirectory(MDAutogen ${IGC_CODEGEN_BASE_DIR}/MDAutogen)
set_source_files_properties(${IGC_CODEGEN_DIR}/MDNodeFunctions.gen PROPERTIES GENERATED TRUE)


# ============================================================================
# ========== BUILD CONFIGURATIONS (part 2) ===================================
# ============================================================================
# The section must be after project definition and its supported languages because otherwise
# used variables are not available yet.

# Generate debug symbols even in release mode, thus making it possible to produce
# separate Debian/RPM debug information packages.
# FIXME: Instead of hacking the release build flags, consider using CMake's
# RelWithDebInfo build type.
if(UNIX)
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -g")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -g")
endif()


# Populating global property with list of debug configurations (based on pattern).
set(_debugConfigs)
foreach(_configName ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
  if(_configName MATCHES "${IGC_BUILD__DEBUG_PATTERN}")
    list(APPEND _debugConfigs "${_configName}")
  endif()
endforeach()
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS ${_debugConfigs})
unset(_debugConfigs)

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================


# ======================================================================================================
# ========================================= BUILD CMAKE OPTIONS ========================================
# ======================================================================================================

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/igc_library_name.cmake")
set(IGC_LIBRARY_NAME ${IGC_LIBRARY_NAME} PARENT_SCOPE)

set(FCL_ENABLED ON)


if (FCL_ENABLED)
  #FCL
  if(DEFINED FCL_OPTION__LIBRARY_NAME)
    set(FCL_LIBRARY_NAME "${FCL_OPTION__LIBRARY_NAME}")
  else()
    set(FCL_LIBRARY_NAME "igdfcl")
  endif()
  set(FCL_LIBRARY_NAME ${FCL_LIBRARY_NAME} PARENT_SCOPE)

  set(_allowBifLink ON)
  set(IGC_OPTION__BIF_LINK_BC ${_allowBifLink} CACHE BOOL "Built-in Functions: Link .bc files into final shared libraries.")
  mark_as_advanced(IGC_OPTION__BIF_LINK_BC)
  unset(_allowBifLink)
endif()

# TODO: Remove this flag once only LLVM 16+ is supported (no possibility to enable typed pointers).
if(IGC_OPTION__LLVM_PREFERRED_VERSION VERSION_GREATER_EQUAL 16)
  option(IGC_OPTION__API_ENABLE_OPAQUE_POINTERS "[Experimental] Enable opaque pointers across the IGC stack.\
    Forces opaque pointers' usage in IGC/LLVM passes, OCL Clang FE and built-in functions' bitcode.\
    Enabled by default for LLVM 16+." ON)
else()
  option(IGC_OPTION__API_ENABLE_OPAQUE_POINTERS "[Experimental] Enable opaque pointers across the IGC stack.\
    Forces opaque pointers' usage in IGC/LLVM passes, OCL Clang FE and built-in functions' bitcode.\
    Enabled by default for LLVM 16+." OFF)
endif()

option(IGC_OPTION__ENABLE_LIT_TESTS "Enable lit testing for IGC compiler. May require additional tools like llvm lit and opt" OFF)

set(IGC_OPTION__BIF_SRC_OCL_DIR "${IGC_SOURCE_DIR}/BiFModule"
    CACHE PATH "Built-in Functions: Root directory where sources for OpenCL builtins are located.")
mark_as_advanced(IGC_OPTION__BIF_SRC_OCL_DIR)

if(IGC_BUILD__CROSSCOMPILE_NEEDED AND (NOT CMAKE_CROSSCOMPILING))
  message(FATAL_ERROR "IGC_OPTION__ARCHITECTURE_TARGET,\nIGC_OPTION__ARCHITECTURE_HOST: Current target / host architecture combination requires cross-compiling. Please specify correct toolchain.")
endif()

igc_arch_validate(_archValid "${IGC_OPTION__ARCHITECTURE_TARGET}")
if(NOT _archValid)
  message(FATAL_ERROR "IGC_OPTION__ARCHITECTURE_TARGET: Target architecture \"${IGC_OPTION__ARCHITECTURE_TARGET}\" is invalid.\nPlease specify correct one, e.g. Windows32, Linux64, Android32, ...")
endif()
igc_arch_validate(_archValid "${IGC_OPTION__ARCHITECTURE_HOST}")
if(NOT _archValid)
  message(FATAL_ERROR "IGC_OPTION__ARCHITECTURE_HOST: Host architecture \"${IGC_OPTION__ARCHITECTURE_HOST}\" is invalid.\nPlease specify correct one, e.g. Windows32, Linux64, Android32, ...")
endif()


set(IGC_OPTION__IST_IGCC_ONLY ON CACHE BOOL
    "Only building igcc part of IST (IGCStandalone).")

if(NOT DEFINED IGC_OPTION__BUILD_IGC_OPT)
    set(IGC_OPTION__BUILD_IGC_OPT OFF)
endif()

igc_arch_get_cpu(_cpuSuffix)
if(NOT DEFINED IGC_OPTION__OUTPUT_DIR)
set(IGC_OPTION__OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}" CACHE PATH "Output directory path where the final libraries will be stored.")
endif()
get_filename_component(IGC_OPTION__OUTPUT_DIR ${IGC_OPTION__OUTPUT_DIR} ABSOLUTE)
unset(_cpuSuffix)

if(NOT DEFINED IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS)
    set(IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS OFF)
endif()

set(IGC_OPTION__COMPILE_LINK_ALLOW_UNSAFE_SIZE_OPT ON CACHE BOOL "Compile/link: Allow unsafe size optimization like --gc-sections.")
mark_as_advanced(IGC_OPTION__COMPILE_LINK_ALLOW_UNSAFE_SIZE_OPT)

# IGC install time absolute path
if(IGC_OPTION__INSTALL_TIME_ROOT_DIR)
    get_filename_component(IGC_INSTALL_TIME_ROOT_DIR ${IGC_OPTION__INSTALL_TIME_ROOT_DIR} ABSOLUTE)
else()
    set(IGC_INSTALL_TIME_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
endif()

# ============================================= Status =================================================

message(STATUS "================================ IGC Project ================================")
if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Build types: ${CMAKE_CONFIGURATION_TYPES} (multi-configuration generator)")
else()
    message(STATUS "Build type:  ${CMAKE_BUILD_TYPE} (single-configuration generator)")
endif()
message(STATUS "Build tools: ${IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS}")
message(STATUS "")
message(STATUS "Output directory:")
message(STATUS "   \"${IGC_OPTION__OUTPUT_DIR}\"")
message(STATUS "Install root directory:")
message(STATUS "   \"${IGC_INSTALL_TIME_ROOT_DIR}\"")
message(STATUS "Architecture:")
message(STATUS " - target: ${IGC_OPTION__ARCHITECTURE_TARGET}    (detected: ${IGC_BUILD__DETECTED_ARCHITECTURE_TARGET})")
message(STATUS " - host:   ${IGC_OPTION__ARCHITECTURE_HOST}    (detected: ${IGC_BUILD__DETECTED_ARCHITECTURE_HOST})")
message(STATUS "")
message(STATUS "Cross-compilation needed: ${IGC_BUILD__CROSSCOMPILE_NEEDED}")
message(STATUS "Cross-compilation set:    ${CMAKE_CROSSCOMPILING}")
message(STATUS "")
message(STATUS "Advanced:")
message(STATUS " - Link BiF resources:              ${IGC_OPTION__BIF_LINK_BC}")
message(STATUS " - Building Windows Universal:      ${IGC_OPTION__UNIVERSAL_DRIVER}")
message(STATUS "=============================================================================")

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================

# ======================================================================================================
# ======================================== COMPILE/LINK SETTINGS =======================================
# ======================================================================================================

# =========================================== Helper variables =========================================

# We are building IGC.
set(IGC_BUILD YES)


if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(LLVM_ON_WIN32 ON)
else()
  set(LLVM_ON_WIN32 OFF)
endif()

# NOTE: LLVM_ON_UNIX is also true for Android (CMAKE_SYSTEM_NAME specified by Android toolchain file is also "Linux",
# but we should make sure so there is no errors in our assumptions).
if((CMAKE_SYSTEM_NAME MATCHES "Linux") OR ANDROID)
  set(LLVM_ON_UNIX ON)
else()
  set(LLVM_ON_UNIX OFF)
endif()


if(_igc_compiler_is_gnu_or_clang AND NOT IGC_BUILD_DISABLE_FLAGS_GNU_CLANG)
  set(IGC_BUILD__WHOLE_ARCHIVE    "-Wl,--whole-archive")
  set(IGC_BUILD__NO_WHOLE_ARCHIVE "-Wl,--no-whole-archive")
  set(IGC_BUILD__START_GROUP      "-Wl,--start-group")
  set(IGC_BUILD__END_GROUP        "-Wl,--end-group")
else()
  set(IGC_BUILD__WHOLE_ARCHIVE    "")
  set(IGC_BUILD__NO_WHOLE_ARCHIVE "")
  set(IGC_BUILD__START_GROUP      "")
  set(IGC_BUILD__END_GROUP        "")
endif()

set(IGC_BUILD__PROJ_NAME_PREFIX "")

#SPIRV consumption variable

set(IGC_BUILD__SPIRV_ENABLED ON)


set(IGC_OPTION__LLVM_INTERIM OFF CACHE BOOL "Configure IGC to build with a provisional LLVM revision")
message(STATUS "[IGC] IGC_OPTION__LLVM_INTERIM = ${IGC_OPTION__LLVM_INTERIM}")

# Recognize if we need to use LLVM interim mode and set appropriate definition
if (IGC_OPTION__LLVM_INTERIM)
    # Switch on IGC_BUILD_LLVM_INTERIM when -DIGC_OPTION__LLVM_INTERIM=1 option is provided for locally build
    set(IGC_BUILD_LLVM_INTERIM ON)
elseif($ENV{IGC_LLVM_INTERIM})
    # Switch on IGC_BUILD_LLVM_INTERIM when IGC_LLVM_INTERIM is set in environment e.g. yaml file
    set(IGC_BUILD_LLVM_INTERIM ON)
endif()

if(IGC_BUILD_LLVM_INTERIM)
  message(STATUS "[IGC] IGC_BUILD_LLVM_INTERIM = ${IGC_BUILD_LLVM_INTERIM}")
  add_compile_definitions(IGC_LLVM_TRUNK_REVISION)
endif()

# Enable vector compiler for Linux and Windows
# If user already defined this, honor decision
if(NOT DEFINED IGC_BUILD__VC_ENABLED)
  if(LLVM_ON_UNIX OR LLVM_ON_WIN32)
    set(IGC_BUILD__VC_ENABLED ON)
  endif()
endif()


if(IGC_BUILD_LLVM_INTERIM AND IGC_BUILD__VC_ENABLED)
  # Add vc-intrinsics patcher for Interim mode
  include("${IGC_SOURCE_DIR}/../external/vc-intrinsics/CMakeLists.txt")
endif()

if(NOT IGC_BUILD__VC_ENABLED OR NOT DEFINED IGC_BUILD__VC_ENABLED)
  set(IGC_BUILD__CMLIT_ENABLE OFF)
endif()
# ======================================== Path helper variables =======================================

set(IGC_BUILD__IGC_SRC_DIR          "${IGC_SOURCE_DIR}")
set(IGC_BUILD__IGC_COMMON_DIR       "${IGC_BUILD__IGC_SRC_DIR}/common")


set(IGC_BUILD__IGC_BIN_DIR          "${IGC_BINARY_DIR}")

set(IGC_BUILD__GFX_DEV_SRC_DIR      "${IGC_SOURCE_DIR}/..")
set(IGC_BUILD__GFX_DEV_TOOLS_DIR    "${IGC_SOURCE_DIR}/../../Tools")

set(IGC_BUILD__COMMON_DIR           "${IGC_BUILD__GFX_DEV_SRC_DIR}/Common")
set(IGC_BUILD__3D_COMMON_DIR        "${IGC_BUILD__GFX_DEV_SRC_DIR}/3d/common")

set(IGC_BUILD__INC_DIR              "${IGC_BUILD__GFX_DEV_SRC_DIR}/inc")
set(IGC_BUILD__COMMON_COMPILER_DIR  "${IGC_BUILD__GFX_DEV_SRC_DIR}/inc/common/Compiler")
set(IGC_BUILD__VISA_DIR             "${IGC_BUILD__GFX_DEV_SRC_DIR}/visa")

set(IGC_BUILD__TOOLS_IGC_DIR        "${IGC_BUILD__GFX_DEV_TOOLS_DIR}/IGC")
set(IGC_BUILD__TOOLS_IST_DIR        "${IGC_BUILD__GFX_DEV_TOOLS_DIR}/IST")


set(IGC_BUILD__BIF_DIR              "${IGC_OPTION__OUTPUT_DIR}/bif/${IGC_CMAKE_CFG_INTDIR}")
set(IGC_BUILD__TOOLS_OUTPUT_DIR     "${IGC_OPTION__OUTPUT_DIR}/Tools")
set(IGC_BUILD__UNIT_TEST_OUTPUT_DIR "${IGC_OPTION__OUTPUT_DIR}/UnitTests")
set(IGC_BUILD__CTH_DIR              "${IGC_OPTION__OUTPUT_DIR}/fcl")

# Including CIF
set(CIF_BASE_DIR "AdaptorOCL/cif")
add_subdirectory("${CIF_BASE_DIR}/cif" ${CMAKE_CURRENT_BINARY_DIR}/cif)

# ================================================ Outputs =============================================

set(EXECUTABLE_OUTPUT_PATH ${IGC_OPTION__OUTPUT_DIR})
set(LIBRARY_OUTPUT_PATH    ${IGC_OPTION__OUTPUT_DIR})

# =============================== Compiler options (generic configuration) =============================

# Compiler options:
#   = - Equal
#   ^ - Implicitly defined by differnt option (like /O2).
#   ! - Should not be changed (should not be defined in first place -> there is no non-Windows support).
#   * - Needs to be modified in CMake.
#   + - Needs to be added in CMake.
#   - - Needs to be removed in CMake.
#
#   MSVC:
#     =/analyze-   =/EHsc   =/errorReport:prompt   =/fp:precise   =/Gd   =/Gm-   ^/GF    /GR-   ^/GS   ^/Gy    /MP    /MT   =/nologo   =/O2   ^/Ob2   ^/Oi    /Os   =/Oy-          =/W3   !/Zc:forScope-   =/Zc:wchar_t    /Zi    /Zp8
#     ?/wd"4146"   ?/wd"4355"   ?/wd"4800"   ?/wd"4482"   ?/wd"4244"   ?/WX
#   CMake:
#     =/analyze-   =/EHsc   =/errorReport:prompt   =/fp:precise   =/Gd   =/Gm-   ^/GF   */GR    ^/GS   ^/Gy   +/MP   */MD   =/nologo   =/O2   ^/Ob2   ^/Oi   +/Os   =/Oy-   -/TP   =/W3    /Zc:forScope    =/Zc:wchar_t   +/Zi   ^/Zp8
#                                                                      ?/WX-
#
#   Notes:
#    - "/Zc:forScope-" is non-standard behavior which is not portable to other OSes. It was defined for MFC and should not be used in modern code.
#    - "/Zp8" is default max. alignment for struct members (this option and its behavior can be hard to port to other OSes).
#    - "/wd" some of the warning are critically useful (especially when warning level is less than 4):
#      4146 -> this can lead to problems with comparison; limits or numeric_limits usually allow to quickly correct the code.
#      4244 -> this can lead to hard to catch problems with loss of data (especially when /W2 or /W3 type occurred and in x86_64 architetures).
#      4355 -> warning: "this" passed to base class should be off by default (what enabled this?).
#      4482 -> C++11 compilers allow enum qualification without warning (should not be problem since MSVC 2013).
#      4800 -> can be disabled but it can be corrected pretty quick.
#      4267 -> var : conversion from 'size_t' to 'type', possible loss of data
#      4273 -> function' : inconsistent DLL linkage
#    - "/WX" should be re-enabled when all warnings are cleaned from code.
#    - Although "/O1" is usual set of settings for optmimization for size, but "/O2" "/Oi" "/Os" is preferred in IGC.
#    - we want to disable warning 4267: conversion from 'size_t' to 'uint32_t', possible loss of data
#    - Adding warning 4062 to be emiited at level 1: enumerator 'identifier' in switch of enum 'enumeration' is not handled


if(MSVC)
  igc_flag_register_settings(
      CompilerOptions

      GROUP NAME CallingConvention  "/G[drzv]"
      GROUP NAME CompileAs          "/T[CP]"
      GROUP NAME DebugInfoFormat    "/Z[7iI]"
      GROUP NAME Exceptions         "/EHa" "/EHsc?"
      GROUP NAME FaviorSizeSpeed    "/O[st]"
      GROUP NAME Optimization       "/O[d12x]"
      GROUP NAME RuntimeLibrary     "/M[TD]d?"
      GROUP NAME Rtti               "/GR-?"
      GROUP NAME TreatWarnAsError   "/WX-?"

      ALIAS AdditionalOptions          "/w14062" "/Wv:18"
      ALIAS CallingConvCdecl           "/Gd"
      ALIAS CallingConvFastcall        "/Gr"
      ALIAS CallingConvStdcall         "/Gz"
      ALIAS CallingConvVectorcall      "/Gv"
      ALIAS CompileAsC                 "/TC"
      ALIAS CompileAsCpp               "/TP"
      ALIAS CompileAsDefault           CompileAs REMOVE_GROUP
      ALIAS DebugInfoDisabled          DebugInfoFormat REMOVE_GROUP
      ALIAS DebugInfoProgramDatabase   "/Zi"
      ALIAS DisableSpecificWarnings    "/wd4267" "/wd4273" "/wd4091" "/wd4996" "/wd4291"
      ALIAS ExceptionsDisabled         Exceptions REMOVE_GROUP
      ALIAS ExceptionsEnabled          "/EHsc"
      ALIAS FaviorSize                 "/Os"
      ALIAS FaviorSpeed                "/Ot"
      ALIAS MultiProcessorCompilation  "/MP"
      ALIAS OptimizeDisabled           "/Od"
      ALIAS OptimizeSize               "/O2" "/Oi" "/Os"
      ALIAS OptimizeSpeed              "/O2" "/Oi" "/Ot"
      ALIAS OptimizeFull               "/Ox"
      ALIAS RtMultiThreadedStatic      "/MT"
      ALIAS RtMultiThreadedShared      "/MD"
      ALIAS RtMultiThreadedStaticDebug "/MTd"
      ALIAS RtMultiThreadedSharedDebug "/MDd"
      ALIAS RttiDisabled               "/GR-"
      ALIAS RttiEnabled                "/GR"
      ALIAS TreatWarnAsErrorDisabled   "/WX-"
      ALIAS TreatWarnAsErrorEnabled    "/WX"
      ALIAS UninitializedVarWarning    "/w14701"
      ALIAS UnreferencedVariable       "/w14189"
      ALIAS WholeProgramOptimization   "/GL"
      ALIAS ControlFlowGuard           "/guard:cf"

      ALIAS DeadCodeEliminate
      ALIAS DeadCodePreserve
    )
elseif(_igc_compiler_is_gnu)
  igc_flag_register_settings(
      CompilerOptions

      GROUP NAME DebugInfoFormat    "-g[0-3]?"
      GROUP NAME Exceptions         "-f(no-)?exceptions"
      GROUP NAME Optimization       "-O[0-3sg]?"
      GROUP NAME Rtti               "-f(no-)?rtti"
      GROUP NAME TreatWarnAsError   "-Werror"

      ALIAS AdditionalOptions
      ALIAS CallingConvCdecl
      ALIAS CallingConvFastcall
      ALIAS CallingConvStdcall
      ALIAS CallingConvVectorcall
      ALIAS CompileAsC
      ALIAS CompileAsCpp
      ALIAS CompileAsDefault
      ALIAS DebugInfoDisabled          "-g0"
      ALIAS DebugInfoProgramDatabase   "-g"
      ALIAS DisableSpecificWarnings
      ALIAS ExceptionsDisabled         "-fno-exceptions"
      ALIAS ExceptionsEnabled          "-fexceptions"
      ALIAS FaviorSize                 "-Os"
      ALIAS FaviorSpeed                "-O3"
      ALIAS MultiProcessorCompilation
      ALIAS OptimizeDisabled           "-O0"
      ALIAS OptimizeSize               "-Os"
      ALIAS OptimizeSpeed              "-O2"
      ALIAS OptimizeFull               "-O3"
      ALIAS RtMultiThreadedStatic
      ALIAS RtMultiThreadedShared
      ALIAS RtMultiThreadedStaticDebug
      ALIAS RtMultiThreadedSharedDebug
      ALIAS RttiDisabled               "-fno-rtti"
      ALIAS RttiEnabled                "-frtti"
      ALIAS TreatWarnAsErrorDisabled   TreatWarnAsError REMOVE_GROUP
      ALIAS TreatWarnAsErrorEnabled    "-Werror"
      ALIAS UnreferencedVariable
      ALIAS UninitializedVarWarning
      ALIAS WholeProgramOptimization
      ALIAS ControlFlowGuard

      ALIAS DeadCodeEliminate
      ALIAS DeadCodePreserve
    )

  if(IGC_OPTION__COMPILE_LINK_ALLOW_UNSAFE_SIZE_OPT)
    igc_flag_register_settings(
        CompilerOptions

        GROUP NAME EmitDataSec "-fdata-sections"
        GROUP NAME EmitFuncSec "-ffunction-sections"

        ALIAS DeadCodeEliminate  "-fdata-sections" "-ffunction-sections"
        ALIAS DeadCodePreserve   EmitDataSec EmitFuncSec REMOVE_GROUP
      )
  endif()
elseif(_igc_compiler_is_clang)
  igc_flag_register_settings(
      CompilerOptions

      GROUP NAME DebugInfoFormat    "-g(dwardf-[2-4]?)?"
      GROUP NAME Exceptions         "-f(no-)?exceptions"
      GROUP NAME Optimization       "-O([0-3]?|fast|s|z)?"
      GROUP NAME Rtti               "-f(no-)?rtti"
      GROUP NAME TreatWarnAsError   "-Werror"

      ALIAS AdditionalOptions
      ALIAS CallingConvCdecl
      ALIAS CallingConvFastcall
      ALIAS CallingConvStdcall
      ALIAS CallingConvVectorcall
      ALIAS CompileAsC
      ALIAS CompileAsCpp
      ALIAS CompileAsDefault
      ALIAS DebugInfoDisabled          DebugInfoFormat REMOVE_GROUP
      ALIAS DebugInfoProgramDatabase   "-g"
      ALIAS DisableSpecificWarnings
      ALIAS ExceptionsDisabled         Exceptions REMOVE_GROUP
      ALIAS ExceptionsEnabled          "-fexceptions"
      ALIAS FaviorSize                 "-Os"
      ALIAS FaviorSpeed                "-O3"
      ALIAS MultiProcessorCompilation
      ALIAS OptimizeDisabled           "-O0"
      ALIAS OptimizeSize               "-Os"
      ALIAS OptimizeSpeed              "-O2"
      ALIAS OptimizeFull               "-03"
      ALIAS RtMultiThreadedStatic
      ALIAS RtMultiThreadedShared
      ALIAS RtMultiThreadedStaticDebug
      ALIAS RtMultiThreadedSharedDebug
      ALIAS RttiDisabled               "-fno-rtti"
      ALIAS RttiEnabled                Rtti REMOVE_GROUP
      ALIAS TreatWarnAsErrorDisabled   TreatWarnAsError REMOVE_GROUP
      ALIAS TreatWarnAsErrorEnabled    "-Werror"
      ALIAS UnreferencedVariable
      ALIAS UninitializedVarWarning
      ALIAS WholeProgramOptimization
      ALIAS ControlFlowGuard

      ALIAS DeadCodeEliminate
      ALIAS DeadCodePreserve
    )
else()
  igc_flag_register_settings(
      CompilerOptions

      ALIAS AdditionalOptions
      ALIAS CallingConvCdecl
      ALIAS CallingConvFastcall
      ALIAS CallingConvStdcall
      ALIAS CallingConvVectorcall
      ALIAS CompileAsC
      ALIAS CompileAsCpp
      ALIAS CompileAsDefault
      ALIAS DebugInfoDisabled
      ALIAS DebugInfoProgramDatabase
      ALIAS DisableSpecificWarnings
      ALIAS ExceptionsDisabled
      ALIAS ExceptionsEnabled
      ALIAS FaviorSize
      ALIAS FaviorSpeed
      ALIAS MultiProcessorCompilation
      ALIAS OptimizeDisabled
      ALIAS OptimizeSize
      ALIAS OptimizeSpeed
      ALIAS OptimizeFull
      ALIAS RtMultiThreadedStatic
      ALIAS RtMultiThreadedShared
      ALIAS RtMultiThreadedStaticDebug
      ALIAS RtMultiThreadedSharedDebug
      ALIAS RttiDisabled
      ALIAS RttiEnabled
      ALIAS TreatWarnAsErrorDisabled
      ALIAS TreatWarnAsErrorEnabled
      ALIAS UnreferencedVariable
      ALIAS UninitializedVarWarning
      ALIAS WholeProgramOptimization
      ALIAS ControlFlowGuard

      ALIAS DeadCodeEliminate
      ALIAS DeadCodePreserve
    )
endif()

# ================================ Linker options (generic configuration) ==============================
if(MSVC)
  if(IGC_OPTION__UNIVERSAL_DRIVER)
    igc_flag_register_settings(
        LinkerOptions

        GROUP NAME DebugInfo         "/[dD][eE][bB][uU][gG]"
        GROUP NAME DeadCode          "/[oO][pP][tT]:([nN][oO])?[rR][eE][fF]"
        GROUP NAME IdenticalCodeData "/[oO][pP][tT]:([nN][oO])?[iI][cC][fF](=[0-9]+)?"

        ALIAS AdditionalOptions
        ALIAS DebugInfoDisabled          DebugInfo REMOVE_GROUP
        ALIAS DebugInfoProgramDatabase   "/DEBUG"
        ALIAS DeadCodeEliminate          "/OPT:REF"
        ALIAS DeadCodePreserve           "/OPT:NOREF"
        ALIAS IdenticalCodeDataFold      "/OPT:ICF"
        ALIAS IdenticalCodeDataNoFold    "/OPT:NOICF"
        ALIAS LinkTimeCodeGeneration     "/LTCG"
        ALIAS ControlFlowGuard           "/guard:cf"
        ALIAS NoDefaultLib               "/NODEFAULTLIB:kernel32.lib"
        ALIAS ControlFlowEnforcement     "/CETCOMPAT"
      )
  else()
    igc_flag_register_settings(
        LinkerOptions

        GROUP NAME DebugInfo         "/[dD][eE][bB][uU][gG]"
        GROUP NAME DeadCode          "/[oO][pP][tT]:([nN][oO])?[rR][eE][fF]"
        GROUP NAME IdenticalCodeData "/[oO][pP][tT]:([nN][oO])?[iI][cC][fF](=[0-9]+)?"

        ALIAS DebugInfoDisabled          DebugInfo REMOVE_GROUP
        ALIAS DebugInfoProgramDatabase   "/DEBUG"
        ALIAS DeadCodeEliminate          "/OPT:REF"
        ALIAS DeadCodePreserve           "/OPT:NOREF"
        ALIAS IdenticalCodeDataFold      "/OPT:ICF"
        ALIAS IdenticalCodeDataNoFold    "/OPT:NOICF"
        ALIAS LinkTimeCodeGeneration     "/LTCG"
        ALIAS ControlFlowGuard           "/guard:cf"
        ALIAS NoDefaultLib
        ALIAS ControlFlowEnforcement     "/CETCOMPAT"
      )
  endif()
else()
  igc_flag_register_settings(
      LinkerOptions

      ALIAS DebugInfoDisabled
      ALIAS DebugInfoProgramDatabase
      ALIAS DeadCodeEliminate
      ALIAS DeadCodePreserve
      ALIAS IdenticalCodeDataFold
      ALIAS IdenticalCodeDataNoFold
      ALIAS LinkTimeCodeGeneration
      ALIAS ControlFlowGuard
      ALIAS NoDefaultLib
      ALIAS ControlFlowEnforcement
    )

  if(IGC_OPTION__COMPILE_LINK_ALLOW_UNSAFE_SIZE_OPT)
    if(_igc_compiler_is_gnu_or_clang)
      igc_flag_register_settings(
          LinkerOptions

          GROUP NAME GcSections "-Wl,--gc-sections"

          ALIAS DeadCodeEliminate  "-Wl,--gc-sections"
          ALIAS DeadCodePreserve   GcSections REMOVE_GROUP
        )
    endif()
  endif()
endif()

# =========================================== Compiler options =========================================

foreach(_compilerFlagName IN ITEMS "CMAKE_CXX_FLAGS" "CMAKE_C_FLAGS")
  # Change some generic settings of compiler.
  # NOTE: Debug info generation is enabled for all build configuration, because it is separate on Windows
  #       and we will use "strip" command on Linux and Android (to separate it).

  if(IGC_OPTION__UNIVERSAL_DRIVER)
    igc_config_flag_apply_settings(
          CompilerOptions
          "${_compilerFlagName}"
          ALL_PATTERN ""
          SET
            CompileAsDefault
            DisableSpecificWarnings
            ExceptionsEnabled
            MultiProcessorCompilation
            DeadCodeEliminate
            CallingConvCdecl
    )
  else()
    igc_config_flag_apply_settings(
          CompilerOptions
          "${_compilerFlagName}"
          ALL_PATTERN ""
          SET
            CompileAsDefault

            DisableSpecificWarnings
            ExceptionsEnabled
            MultiProcessorCompilation
            DeadCodeEliminate
    )
  endif()

  igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      ALL_PATTERN_NOINHERIT ""
      SET
        DebugInfoProgramDatabase
        OptimizeSize
        RtMultiThreadedStatic
    )
  igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      PATTERN "${IGC_BUILD__DEBUG_PATTERN}"
      SET
        OptimizeDisabled
        RtMultiThreadedStaticDebug
        UnreferencedVariable
    )
  igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      PATTERN "${IGC_BUILD__RELEASE_PATTERN}"
      SET
        OptimizeSpeed
        ControlFlowGuard
        # Disable LTCG due to increase of release build time
        #WholeProgramOptimization
    )

  # Adding needed settings specific to MSVC.
  if(MSVC)
  # Adding needed settings specific to GCC.
  # NOTE: Following options can be needed in the future (although some not recommended: NR):
  # [NR] -fno-short-enums
  # [NR] -fno-tree-pre
  #      -fno-omit-frame-pointer
  #      -Wa,--noexecstack
  #      -fkeep-inline-functions
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
         SET
           AdditionalOptions
           RttiDisabled
           TreatWarnAsErrorEnabled
    )
  elseif( ANDROID )
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
        SET_RAW
          -pipe
          -fmessage-length=0
          -march=corei7
          -mstackrealign
          -fms-extensions
          -fvisibility=hidden
          -Werror
          -Wno-unused-parameter
          -Wno-missing-field-initializers
          -Wwrite-strings
          -Wno-long-long
          -Wswitch
          -Wno-sign-compare
          -Wno-unused-result
          -Wno-enum-compare
          -Wno-type-limits
          -Wno-ignored-qualifiers
          -Wno-format
          -Wno-format-security
          -Wno-extra
          -Wno-write-strings
          -finline
          -fno-strict-aliasing
          -msse
          -msse2
          -msse3
          -mssse3
          -msse4
          -msse4.1
          -msse4.2
          -Wno-unknown-pragmas
          -fPIC
          -Bsymbolic
    )
    if(_igc_compiler_is_gnu)
      igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      ALL_PATTERN ""
      SET_RAW
        -funswitch-loops
        -Wno-maybe-uninitialized
    )
    endif()
    if(_igc_compiler_is_gnu OR (_igc_compiler_is_clang AND NOT CMAKE_C_COMPILER_VERSION LESS "4.0"))
      igc_config_flag_apply_settings(
      CompilerOptions
      "${_compilerFlagName}"
      ALL_PATTERN ""
      SET_RAW
            -finline-functions
    )
    endif()

  elseif(_igc_compiler_is_gnu_or_clang)
    if("${IGC_OPTION__ARCHITECTURE_TARGET}" STREQUAL "LinuxARM")
      igc_config_flag_apply_settings(
          CompilerOptions
          "${_compilerFlagName}"
          ALL_PATTERN ""
          SET_RAW
            -pipe
            -fmessage-length=0
            #-march=armv8.2-a+fp+simd+sve2
            -march=armv8-a+fp+simd
            -fms-extensions
            -Werror
            -Wno-unused-parameter
            -Wno-missing-field-initializers
            -Wwrite-strings
            -Wno-long-long
            -Wswitch
            -Wno-sign-compare
            -Wno-unused-result
            -Wno-enum-compare
            -Wno-type-limits
            -Wno-ignored-qualifiers
            -Wno-shadow
            -Wformat
            -Wformat-security
            -Wno-extra
            -Wno-write-strings
            -finline
            -fno-strict-aliasing
            -Wno-unknown-pragmas
            -fPIC
            -Bsymbolic
      )
    elseif("${IGC_OPTION__ARCHITECTURE_TARGET}" STREQUAL "LinuxRISCV")
      igc_config_flag_apply_settings(
          CompilerOptions
          "${_compilerFlagName}"
          ALL_PATTERN ""
          SET_RAW
            -pipe
            -fmessage-length=0
            -march=rv64gc
            -fms-extensions
            -Werror
            -Wno-unused-parameter
            -Wno-missing-field-initializers
            -Wwrite-strings
            -Wno-long-long
            -Wswitch
            -Wno-sign-compare
            -Wno-unused-result
            -Wno-enum-compare
            -Wno-type-limits
            -Wno-ignored-qualifiers
            -Wno-shadow
            -Wformat
            -Wformat-security
            -Wno-extra
            -Wno-write-strings
            -finline
            -fno-strict-aliasing
            -Wno-unknown-pragmas
            -fPIC
            -Bsymbolic
      )
    else()
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
        SET_RAW
          -pipe
          -fmessage-length=0
          -march=corei7
          -mstackrealign
          -fms-extensions
          -Werror
          -Wno-unused-parameter
          -Wno-missing-field-initializers
          -Wwrite-strings
          -Wno-long-long
          -Wswitch
          -Wno-sign-compare
          -Wno-unused-result
          -Wno-enum-compare
          -Wno-type-limits
          -Wno-ignored-qualifiers
          -Wno-shadow
          -Wformat
          -Wformat-security
          -Wno-extra
          -Wno-write-strings
          -finline
          -fno-strict-aliasing
          -msse
          -msse2
          -msse3
          -mssse3
          -msse4
          -msse4.1
          -msse4.2
          -Wno-unknown-pragmas
          -fPIC
          -Bsymbolic
    )
    endif()
    string(FIND ${CMAKE_CXX_FLAGS} "-D_FORTIFY_SOURCE=3" __FORTIFY_SOURCE_3_SET)
    set(flags -fstack-protector)
    if(${__FORTIFY_SOURCE_3_SET} EQUAL -1)
      list(APPEND flags -D_FORTIFY_SOURCE=2)
    endif()
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        PATTERN "${IGC_BUILD__RELEASE_PATTERN}"
        SET_RAW
          ${flags}
    )
    if(_igc_compiler_is_gnu OR (_igc_compiler_is_clang AND NOT CMAKE_C_COMPILER_VERSION LESS "4.0"))
      igc_config_flag_apply_settings(
          CompilerOptions
          "${_compilerFlagName}"
          ALL_PATTERN ""
          SET_RAW
            -finline-functions
      )
    endif()
    if(NOT _igc_compiler_is_clang)
      set(flags -funswitch-loops -Wno-maybe-uninitialized)
      if(NOT ANDROID)
        list(APPEND flags -lrt)
      endif()
      igc_config_flag_apply_settings(
          CompilerOptions
          "${_compilerFlagName}"
          ALL_PATTERN ""
          SET_RAW
            ${flags}
      )
    endif()
  elseif(_igc_compiler_is_clang)
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
        SET_RAW
        -pipe
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -Wcovered-switch-default
        -Wno-unused-parameter
        -Wno-gnu
        -finline
        -fkeep-inline-functions
        -msse4.2
    )
    if(NOT CMAKE_C_COMPILER_VERSION LESS "4.0")
      igc_config_flag_apply_settings(
          CompilerOptions
         "${_compilerFlagName}"
          ALL_PATTERN ""
          SET_RAW
            -finline-functions
      )
    endif()
  endif()

# NOTE: Increase bracket-depth to 1024 for clang because we exceed the default limit of 256.
#       This resulted in errors like:
#       "instantiating fold expression with ... arguments exceeded expression nesting limit of 256"
  if(_igc_compiler_is_clang)
    igc_config_flag_apply_settings(
        CompilerOptions
        "${_compilerFlagName}"
        ALL_PATTERN ""
        SET_RAW
        -fbracket-depth=1024
    )
  endif()
endforeach()

if( ANDROID OR _igc_compiler_is_gnu_or_clang)
  igc_config_flag_apply_settings(
      LinkerOptions
      CMAKE_SHARED_LINKER_FLAGS
      ALL_PATTERN ""
      SET_RAW
    -Wl,--no-undefined
    )
endif()

# NOTE: [WA] Workaround for OpenGL-IGC crash problem. Due to different handling of static objects destructors
#            the destructor of PassRegistry was called two times during exiting of OpenGL.
#            This switch is not conformant with C++ standard (order of destruction of static elements is not met) but
#            it is used in Android build system (possibliy due to Android bionic __cxa_atexit limitations).
if(_igc_compiler_is_gnu_or_clang)
  if(ANDROID)
    igc_config_flag_apply_settings(
        CompilerOptions
        CMAKE_CXX_FLAGS
        ALL_PATTERN ""
        SET_RAW
          -fno-use-cxa-atexit
          -fvisibility-inlines-hidden
          -fpermissive
          -fno-rtti
      )
  endif()
endif()

# Unconditionally disable rtti (C++ only)
igc_config_flag_apply_settings(
    CompilerOptions
    CMAKE_CXX_FLAGS
    ALL_PATTERN ""
    SET
       RttiDisabled
  )


if(_igc_compiler_is_gnu_or_clang)
    igc_config_flag_apply_settings(
        CompilerOptions
        CMAKE_CXX_FLAGS
        ALL_PATTERN ""
        SET_RAW
          -fvisibility=hidden
          -fvisibility-inlines-hidden
    )
endif()

# ============================================ Linker options ==========================================

foreach(_linkerFlagName IN ITEMS "CMAKE_EXE_LINKER_FLAGS" "CMAKE_SHARED_LINKER_FLAGS")
  # Change some generic settings of linker.
  # NOTE: Debug info generation is enabled for all build configuration, because it is separate on Windows
  #       and we will use "strip" command on Linux and Android (to separate it).

  igc_config_flag_apply_settings(
      LinkerOptions
      "${_linkerFlagName}"
      ALL_PATTERN ""
      SET
        DebugInfoProgramDatabase
        NoDefaultLib
    )

  igc_config_flag_apply_settings(
      LinkerOptions
      "${_linkerFlagName}"
      ALL_PATTERN_NOINHERIT ""
      SET
        DeadCodeEliminate
        IdenticalCodeDataFold
    )
    igc_config_flag_apply_settings(
      LinkerOptions
      "${_linkerFlagName}"
      PATTERN "${IGC_BUILD__RELEASE_PATTERN}"
      SET
        ControlFlowGuard
        ControlFlowEnforcement
        # Disable LTCG due to release build time increase
        #LinkTimeCodeGeneration
    )
    igc_config_flag_apply_settings(
      LinkerOptions
      "${_linkerFlagName}"
      PATTERN "${IGC_BUILD__DEBUG_PATTERN}"
      SET
        DeadCodePreserve
        IdenticalCodeDataNoFold
    )
endforeach()

if(MSVC)
igc_config_flag_apply_settings(
    LinkerOptions
    CMAKE_SHARED_LINKER_FLAGS
    ALL_PATTERN ""
    SET_RAW
      "/MANIFEST:NO"
      "/ignore:4099"
      "/ignore:4197"
    )

set(IGC_BUILD__EXPORT_SYMBOLS_LINK_FLAGS_DEBUG
  "${IGC_BUILD__EXPORT_SYMBOLS_LINK_FLAGS_DEBUG}"
  "/ignore:4099")

  # Disable LTCG due to release build time increase
  #igc_config_flag_apply_settings(
  #    LinkerOptions
  #    CMAKE_STATIC_LINKER_FLAGS
  #    PATTERN "${IGC_BUILD__RELEASE_PATTERN}"
  #    SET
  #      LinkTimeCodeGeneration
  #)
# Force static linking of common libraries on Android for shared objects.
elseif(_igc_compiler_is_gnu_or_clang)
  if(NOT ANDROID AND NOT IGC_BUILD_DISABLE_FLAGS_GNU_CLANG)
    igc_config_flag_apply_settings(
        LinkerOptions
        CMAKE_SHARED_LINKER_FLAGS
        ALL_PATTERN ""
        SET_RAW
         -Wl,-z,noexecstack
         -Wl,-z,relro
         -Wl,-z,now
      )
  endif()
endif()

if(_igc_compiler_is_clang)
  igc_config_flag_apply_settings(
      LinkerOptions
      CMAKE_SHARED_LINKER_FLAGS
      ALL_PATTERN ""
      SET_RAW
        -shared
        -Wl,-undefined,dynamic_lookup
        -Wl,-headerpad_max_install_names
    )
endif()



# ================================== Compiler preprocessor definitions =================================

# ?  CL_NUMBER                < env:CL_NUMBER if env:CL_NUMBER != ''
# ?* BUILD_NUMBER             < env:BUILD_NUMBER if env:BUILD_NUMBER != ''
# M  IGC_EXPORTS=1            if !IGCStandalone
# M  _COMPILER_DLL_           if Compiler|| GenISAIntrinisics
# *  _CONSOLE                 if visa
# +  _CRT_SECURE_NO_WARNINGS
# +  _DEBUG                   if cfg:Debug
# +  _IGC_
# *  _LIB                     if build:static library
# +  _SCL_SECURE_NO_WARNINGS
# +* DEBUG                    if cfg:Debug
# +  DLL_MODE                 if visa
# M  GHAL3D_SHADERDUMP_ENABLE if IGCStandalone
# +  GHAL3D=USC
# +  ICBE_LHDM                if platform:Windows
# +* INSIDE_PLUGIN
# M  iSTD=USC_iSTD            if IGCStanalone
# +  ISTDLIB_UMD              ?
# +  NDEBUG                   if !cfg:Debug
# +  NOMINMAX
# M* RIIGC                    if IGCStanalone
# +  USC_DXX                  if cfg:Debug
# +  USC_EXPORTS
# +  VER_H                    if !platform:Windows
# M  WIN32                    if platform:Windows && bits:32
# +  WIN64                    if platform:Windows && bits:64
# +  i386=1                   if bits:32
# +  _X86_=1                  if bits:32
# +  _AMD64_                  if bits:64
# + ENABLE_REGISTRY_READ      if cfg:Debug

add_compile_definitions(
    _IGC_
    GHAL3D=USC
    _HAS_EXCEPTIONS=0
    IGC_EXPORTS=1
    USC_EXPORTS=1
    ISTDLIB_UMD
    IGC_CMAKE
    __STDC_CONSTANT_MACROS
    __STDC_LIMIT_MACROS
    _SCL_SECURE_NO_WARNINGS
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
    STD_CALL
    INSIDE_PLUGIN
    _COMPILER_DLL_
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Debug>:ENABLE_REGISTRY_READ>
    $<$<CONFIG:Debug>:USC_DXX>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>:_X86_=1>
    $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>:i386=1>
    $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:_AMD64_>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Linux>:ICBE_LINUX>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Linux>:LINUX>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},Linux32>:USE_MMX>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},Linux32>:USE_SSE>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},Linux32>:USE_SSE2>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},Linux32>:USE_SSE3>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},Linux32>:USE_SSSE3>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},Linux64>:USE_MMX>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},Linux64>:USE_SSE>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},Linux64>:USE_SSE2>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},Linux64>:USE_SSE3>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},Linux64>:USE_SSSE3>
    $<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},LinuxARM>:USE_NEON>
    #$<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},LinuxARM>:USE_SVE>
    #$<$<STREQUAL:${IGC_OPTION__ARCHITECTURE_TARGET},LinuxARM>:USE_SVE2>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:ICBE_LHDM>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:_WIN32>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:"WIN32">
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:WIN32_LEAN_AND_MEAN=1>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:WINNT=1>
    $<$<AND:$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>,$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>>:_WIN64>
    $<$<AND:$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>,$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>>:WIN64>
    )

if(IGC_OPTION__BIF_LINK_BC)
  add_compile_definitions(
    BIF_LINK_BC
  )
endif()

if(DEFINED VME_TYPES_DEFINED)
add_compile_definitions(
      VME_TYPES_DEFINED=${VME_TYPES_DEFINED}
    )
endif()

#SPIRV consumption flag
if(IGC_BUILD__SPIRV_ENABLED)
  add_compile_definitions(
      IGC_SPIRV_ENABLED
    )
endif()

#VC OPT switch on/off
if(IGC_BUILD__VC_ENABLED)
  add_compile_definitions(
      IGC_VC_ENABLED
    )
endif()

# Additional definitions for vISA projects.
set(IGC_BUID__VISA_COMPILE_DEFINITIONS
    DLL_MODE
  )

if(NOT ANDROID)
  add_compile_definitions(CL_KHR_FP64_EXT)
endif()

if(DEFINED ENV{BUILD_ID})
  add_compile_definitions(
   DRIVER_BUILD_ID=$ENV{BUILD_ID}
  )
endif()

if(DEFINED ENV{TEST_P4_CHANGELIST})
  add_compile_definitions(
    TB_BUILD_ID=$ENV{TEST_P4_CHANGELIST}
  )
endif()

# ======================================================================================================

if(IGC_BUILD__SHOW_CFLAGS)
  get_cmake_property(_compilerOptionsVarNames VARIABLES)
  foreach(_compilerOptionsVarName ${_compilerOptionsVarNames})
    if(_compilerOptionsVarName MATCHES "^CMAKE_.*_FLAGS(_.*)?$")
      message("[F] ${_compilerOptionsVarName}:    ${${_compilerOptionsVarName}}")
    endif()
  endforeach()
endif()



# ==================================== LLVM package ====================================================

include("${CMAKE_CURRENT_SOURCE_DIR}/../external/llvm/llvm_deps.cmake")
set(IGC_BUILD__CLANG_VERSION ${IGC_OPTION__LLVM_PREFERRED_VERSION} CACHE STRING "Preferred version of OpenCL Clang to use")
string(REPLACE "." ";" CLANG_VERSION_LIST "${IGC_BUILD__CLANG_VERSION}")
list(GET CLANG_VERSION_LIST 0 IGC_BUILD__CLANG_VERSION_MAJOR)
include(igc_llvm)

# ==================================== LLD package =====================================================

message(STATUS "[lld] option = ${IGC_OPTION__LLVM_LLD}")
if(IGC_OPTION__LLVM_LLD)
  include(igc_find_liblldELF)
endif()

# ==================================== SPIRV-LLVM-Translator package ===================================

if(IGC_OPTION__LINK_KHRONOS_SPIRV_TRANSLATOR)
  include(igc_spirv)
endif()

# ==================================== WrapperLLVM package =============================================

add_subdirectory(WrapperLLVM)
igc_sg_define(IGC__WrapperLLVM)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/WrapperLLVM/include")

# ======================================== Include directories =========================================

include_directories(
    "${IGC_CODEGEN_DIR}"

    "${IGC_BUILD__IGC_SRC_DIR}"
    "${IGC_BUILD__IGC_COMMON_DIR}"

    "${IGC_BUILD__GFX_DEV_SRC_DIR}"

    "${IGC_BUILD__COMMON_DIR}"
    "${IGC_BUILD__3D_COMMON_DIR}"

    "${IGC_BUILD__INC_DIR}"
    "${IGC_BUILD__INC_DIR}/common"

    "${IGC_BUILD__COMMON_COMPILER_DIR}"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API"

    "${IGC_BUILD__VISA_DIR}/include"

    "${IGC_OPTION__OUTPUT_DIR}"

    "${IGC_BUILD__IGC_BIN_DIR}/${IGC_CMAKE_CFG_INTDIR}"

    "${IGC_BUILD__IGC_SRC_DIR}/AdaptorOCL/ocl_igc_shared/executable_format"
    "${IGC_BUILD__IGC_SRC_DIR}/AdaptorOCL"
    "${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/ocl_igc_interface/impl"

    "${IGC_BUILD__IGC_SRC_DIR}/Options"
    "${IGC_BUILD__IGC_SRC_DIR}/Options/include"
    "${CMAKE_CURRENT_BINARY_DIR}/Options/include"
    )


include_directories(
  "${IGC_BUILD__COMMON_COMPILER_DIR}/common"
)
include_directories(
  "${CIF_INCLUDE_DIR}"
)

if(LLVM_ON_WIN32)
  include_directories(
      "${IGC_BUILD__COMMON_COMPILER_DIR}/adapters/d3d9/api"
      "${IGC_BUILD__COMMON_COMPILER_DIR}/adapters/d3d9"
      "${IGC_BUILD__COMMON_COMPILER_DIR}/adapters/d3d10/API"
    )
endif()

if (WIN32)
  include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../SPIRV-LLVM-Translator_${IGC_OPTION__LLVM_PREFERRED_VERSION}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../SPIRV-LLVM-Translator_${IGC_OPTION__LLVM_PREFERRED_VERSION}/lib/SPIRV"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../SPIRV-LLVM-Translator_${IGC_OPTION__LLVM_PREFERRED_VERSION}/lib/SPIRV/Mangler"
    )
endif (WIN32)
if (NOT WIN32)
  include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../SPIRV-LLVM-Translator/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../SPIRV-LLVM-Translator/lib/SPIRV"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../SPIRV-LLVM-Translator/lib/SPIRV/Mangler"
    )
endif (NOT WIN32)

# ==================================== ZE Binary Writer ================================================

add_subdirectory(ZEBinWriter/zebin)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ZEBinWriter/zebin/source")
set(ZEBIN_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ZEBinWriter/zebin/source/autogen")
include_directories("${ZEBIN_INCLUDE_PATH}")
add_subdirectory(PCH)
# ===================================== Projects and source groups =====================================

# Suffixes of main projects.
# __igc_common - contains sources/headers/resources/settings common for all main projects (see list below).
# __igc_dll    - dynamic library for IGC.
# __igc_lib    - static library for IGC.
# Static libraries are only used for some tools.
# NOTE: Due to limitation of Windows resources and exports cannot be passed
#       easily from static library to dynamic. For resources, please use


set(IGC_BUILD__MAIN_IGC_LIB_SUFFIXES
    "__igc_dll"
)

set(IGC_BUILD__PROJ__igc_dll              "${IGC_BUILD__PROJ_NAME_PREFIX}${IGC_LIBRARY_NAME}_dll")
set(ENV{igc_dll}                          "${IGC_BUILD__PROJ__igc_dll}")
set(IGC_BUILD__PROJ_LABEL__igc_dll        "${IGC_BUILD__PROJ__igc_dll}")

if(UNIX)

    set(IGC_BUILD__PROJ_SYMBOL_SCRIPT_NAME "igc_create_linker_script.sh")
    set(IGC_BUILD__PROJ_SYMBOL_SCRIPT_FULL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/${IGC_BUILD__PROJ_SYMBOL_SCRIPT_NAME})
    set(IGC_BUILD__PROJ_SYMBOL_SCRIPT_COPY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${IGC_BUILD__PROJ_SYMBOL_SCRIPT_NAME})

    set(IGC_BUILD__PROJ_LINKER_SCRIPT_NAME igc_symbols.ld)
    set(IGC_BUILD__PROJ_LINKER_SCRIPT_FULL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${IGC_BUILD__PROJ_LINKER_SCRIPT_NAME})

    set(IGC_BUILD__PROJ_BIF_LIBRARY_NAME "null")
    set(IGC_BUILD__PROJ_DX10_LIBRARY_NAME "null")
    set(IGC_BUILD__PROJ_DXIL_LIBRARY_NAME "null")
    set(IGC_BUILD__PROJ_VULKAN_FE_LIBRARY_NAME "null")

endif()

#FCL
set(IGC_BUILD__PROJ__fcl_dll              "${IGC_BUILD__PROJ_NAME_PREFIX}fcl_dll")
set(IGC_BUILD__PROJ_LABEL__fcl_dll        "${IGC_BUILD__PROJ__fcl_dll}")

set(IGC_BUILD__PROJ__igc_lib              "${IGC_BUILD__PROJ_NAME_PREFIX}${IGC_LIBRARY_NAME}_lib")
set(IGC_BUILD__PROJ_LABEL__igc_lib        "${IGC_BUILD__PROJ__igc_lib}")

set(IGC_BUILD__PROJ_LABEL__GenX_IR        "${IGC_BUILD__PROJ_NAME_PREFIX}vISA")

add_subdirectory(common)
igc_sg_define(IGC__common)
igc_sg_define(IGC__Common_CLElfLib)


# Add protobuf
include(igc_find_protobuf)

set(IGC_BUILD__SPIRV_TOOLS_ENABLED ON)

if(IGC_BUILD__SPIRV_TOOLS_ENABLED)
    include("${IGC_SOURCE_DIR}/../external/SPIRV-Tools/CMakeLists.txt")
    include_directories("${IGC_BUILD__SPIRV-Headers_DIR}/include")
    include_directories("${IGC_BUILD__SPIRV-Tools_DIR}/include")
    add_compile_definitions("$<$<CONFIG:Debug>:IGC_SPIRV_TOOLS_ENABLED>")
    add_compile_definitions("$<$<CONFIG:Release>:IGC_SPIRV_TOOLS_ENABLED>")
    igc_sg_define(IGC__SPIRV-Tools)
    if(NOT TARGET ${IGC_BUILD__PROJ__SPIRV-Tools})
        message(FATAL_ERROR "Error while adding SPIR-V Tools project. Please make sure ${IGC_BUILD__SPIRV-Tools_DIR} is present.")
    endif()
endif()

add_subdirectory(GenISAIntrinsics)

add_subdirectory(Options)

if(LLVM_ON_WIN32)
  add_subdirectory(AdaptorOCL)
  igc_sg_define(IGC__AdaptorOCL)

endif()



if(FCL_ENABLED)
  include(igc_find_opencl_clang)
endif()

if(IGC_BUILD__VC_ENABLED)
  add_subdirectory(CMFE)
endif()

if (FCL_ENABLED)
  # Built-in Functions (and resources).
  if(IGC_OPTION__BIF_LINK_BC)
      add_subdirectory(BiFModule)
      add_subdirectory(BiFManager)
  endif()

  add_subdirectory(OCLFE)
endif()

if(IGC_BUILD__VC_ENABLED)
  set(IGC_BUILD__CMLIT_ENABLE_DEFAULT OFF)


  if(IGC_OPTION__ENABLE_LIT_TESTS AND LLVM_ON_UNIX)
    set(IGC_BUILD__CMLIT_ENABLE_DEFAULT ON)
  endif()

  set(IGC_BUILD__CMLIT_ENABLE ${IGC_BUILD__CMLIT_ENABLE_DEFAULT} CACHE BOOL "Whether VC lit testing is enabled")
  if(IGC_BUILD__CMLIT_ENABLE)
    # Path to llvm-opt and llvm lib built with LLVM_LINK_LLVM_DYLIB option
    # Required to load vc-be plugin in opt
    set(IGC_BUILD__CMOPT_DEFAULT_DIR ${IGC_BUILD__GFX_DEV_SRC_DIR}/../cmc/cm-opt/llvm-${LLVM_VERSION_MAJOR})
    set(IGC_BUILD__CMOPT_DIR ${IGC_BUILD__CMOPT_DEFAULT_DIR} CACHE PATH
        "Path to opt utility for VC lit testing")
    message(STATUS "VC LIT tests enabled. Opt in ${IGC_BUILD__CMOPT_DIR}")
  endif()
  add_subdirectory(VectorCompiler)
endif()

add_subdirectory(Compiler)
add_subdirectory(DebugInfo)
add_subdirectory(DriverInterface)
igc_sg_define(IGC__DriverInterface)
igc_sg_define(IGC__LivenessUtils)

add_subdirectory(VISALinkerDriver)

### Will add this glue later as we start transitioning igc to new repo ###
#if(MSVC)
#  set(IGC2_SRC_ORIG ${BS_DIR_EXTERNAL_COMPONENTS}/igc2)
#  set(IGC2_WORKSPACE ${CMAKE_CURRENT_BINARY_DIR}/../igc2)
#  message(STATUS "[IGC2] igc2 dll will be built from sources")
#  message(STATUS "[IGC2] Copying igc2 sources ${IGC2_SRC_ORIG} to ${IGC2_WORKSPACE}")
#  message("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
#  message("^^^^^********^^^^^^$$$$$$$$$$^^^^^^^^^^^^^^^^&&&&&&&&&&&^^^^^^^^^&&&&&&&&")
#  if(NOT EXISTS "${IGC2_WORKSPACE}")
#    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC2_SRC_ORIG} ${IGC2_WORKSPACE})
#  endif()
#  add_subdirectory(${IGC2_WORKSPACE} ${IGC2_WORKSPACE}/bin)
#endif()

igc_sg_define(IGC__VISALinkerDriver)
if(MSVC)
  set(IGC_BUILD__RES__IGC__igc_common
      "${CMAKE_CURRENT_SOURCE_DIR}/igc.rc"
    )
  igc_rc_register_resource(IGC__igc_dll RESOURCES DriverInterface__igc_dll FILE "${IGC_BUILD__RES__IGC__igc_common}") # [WA#2]

  igc_rc_get_resource(IGC_BUILD__RES__IGC__igc_dll IGC__igc_dll)
  # Static libraries on Windows cannot pass resources to final libraries.
  source_group("Resources" FILES
    ${IGC_BUILD__RES__IGC__igc_dll}
  )
endif()

#Metrcis
include(Metrics/protobuf_metrics.cmake)

# Sources/headers for main libraries.
set(IGC_BUILD__SRC__IGC__igc_common
    ${IGC_BUILD__SRC__IGC_AdaptorOCL}
    ${IGC_BUILD__SRC__IGC_common}
    ${IGC_BUILD__SRC__IGC_Common_CLElfLib}
  )


set(IGC_BUILD__SRC__IGC__igc_dll
    ${IGC_BUILD__SRC__IGC__igc_common}
    ${IGC_BUILD__SRC__IGC_DriverInterface__igc_dll}
  )


set(IGC_BUILD__SRC__IGC__igc_lib
    ${IGC_BUILD__SRC__IGC__igc_dll})

set(IGC_BUILD__HDR__IGC__igc_common
    ${IGC_BUILD__HDR__IGC_AdaptorOCL} # currently empty
    ${IGC_BUILD__HDR__IGC_common}
    ${IGC_BUILD__HDR__IGC_Common_CLElfLib}
    )

set(IGC_BUILD__HDR__IGC__igc_dll
    ${IGC_BUILD__HDR__IGC__igc_common}
    ${IGC_BUILD__HDR__IGC_DriverInterface__igc_dll}
    ${IGC_WrapperLLVM_HDR}
  )


if (FCL_ENABLED)
  #FCL
  set(IGC_BUILD__SRC__IGC__fcl_dll
      ${IGC_BUILD__SRC__FCL}
    )

  set(IGC_BUILD__HDR__IGC__fcl_dll
      ${IGC_BUILD__HDR__FCL}
    )

  set(IGC_BUILD__RES__IGC__fcl_dll
    ${IGC_BUILD__RES__FCL}
  )

  add_library("${IGC_BUILD__PROJ__fcl_dll}" SHARED
        ${IGC_BUILD__SRC__IGC__fcl_dll}
        ${IGC_BUILD__HDR__IGC__fcl_dll}
        ${IGC_BUILD__RES__IGC__fcl_dll}
      )

  set_target_properties("${IGC_BUILD__PROJ__fcl_dll}" PROPERTIES FOLDER "Libraries")

  add_dependencies("${IGC_BUILD__PROJ__fcl_dll}" opencl-clang-lib)
endif()

set(IGC_BUILD__HDR__IGC__igc_lib
    ${IGC_BUILD__HDR__IGC__igc_dll}
)

# Main library targets (library names).
add_library("${IGC_BUILD__PROJ__igc_dll}" SHARED
    ${IGC_BUILD__SRC__IGC__igc_dll}
    ${IGC_BUILD__HDR__IGC__igc_dll}
    ${IGC_BUILD__RES__IGC__igc_dll}
)

target_compile_definitions(${IGC_BUILD__PROJ__igc_dll} PRIVATE VER_ORIGINALNAME="$<TARGET_FILE_NAME:${IGC_BUILD__PROJ__igc_dll}>")


if(UNIX)

  #Copying script to current directory to elevate permissions - replaces file if already exists
  file(
      COPY ${IGC_BUILD__PROJ_SYMBOL_SCRIPT_FULL_PATH}
      FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
      DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
  )

  if(TARGET ${IGC_BUILD__PROJ__AdaptorDx10})
      set(IGC_BUILD__PROJ_DX10_LIBRARY_NAME "$<TARGET_FILE:${IGC_BUILD__PROJ__AdaptorDx10}>")
  endif()

  if(TARGET ${IGC_BUILD__PROJ__AdaptorDXIL})
      set(IGC_BUILD__PROJ_DXIL_LIBRARY_NAME "$<TARGET_FILE:${IGC_BUILD__PROJ__AdaptorDXIL}>")
  endif()

  if(TARGET ${IGC_BUILD__PROJ__BiFLib_OCL})
      set(IGC_BUILD__PROJ_BIF_LIBRARY_NAME "$<TARGET_FILE:${IGC_BUILD__PROJ__BiFLib_OCL}>")
  endif()

  if(TARGET ${IGC_BUILD__PROJ__VulkanFrontend})
      set(IGC_BUILD__PROJ_VULKAN_FE_LIBRARY_NAME "$<TARGET_FILE:${IGC_BUILD__PROJ__VulkanFrontend}>")
  endif()

  add_custom_command(
      TARGET "${IGC_BUILD__PROJ__igc_dll}"
      PRE_LINK
      COMMAND ${IGC_BUILD__PROJ_SYMBOL_SCRIPT_COPY_PATH} ${IGC_BUILD__PROJ_LINKER_SCRIPT_NAME} ${IGC_BUILD__PROJ_BIF_LIBRARY_NAME} ${IGC_BUILD__PROJ_DX10_LIBRARY_NAME} ${IGC_BUILD__PROJ_DXIL_LIBRARY_NAME} ${IGC_BUILD__PROJ_VULKAN_FE_LIBRARY_NAME} $<TARGET_OBJECTS:${IGC_BUILD__PROJ__igc_dll}> --
      COMMAND_EXPAND_LISTS
      COMMENT "Creating linker script for custom symbol exporting"
  )

  #Using just created linker script from add_custom_command
  target_link_libraries("${IGC_BUILD__PROJ__igc_dll}" PRIVATE -Wl,--version-script="${IGC_BUILD__PROJ_LINKER_SCRIPT_FULL_PATH}")

endif()

if(IGC_OPTION__LINK_KHRONOS_SPIRV_TRANSLATOR)
  target_link_libraries("${IGC_BUILD__PROJ__igc_dll}" PRIVATE LLVMSPIRVLib)
endif()

if(IGC_BUILD__LLVM_SOURCES AND IGC_OPTION__LLVM_LLD)
  if(UNIX)
    if(NOT ${CMAKE_BUILD_TYPE} EQUAL "Release")
      add_dependencies("${IGC_BUILD__PROJ__igc_dll}" lld)
    endif()
  else()
    add_dependencies("${IGC_BUILD__PROJ__igc_dll}" lld)
  endif()
endif()


target_include_directories(${IGC_BUILD__PROJ__igc_dll} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_shared"
  "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_shared/device_enqueue"
  "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_shared/executable_format"
  "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/cif"
  "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/"
  "${CMAKE_CURRENT_SOURCE_DIR}/"
  "${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/"
)


#IGC_DLL
igc_target_flag_property_add_once_config_var("${IGC_BUILD__PROJ__igc_dll}" LINK_FLAGS IGC_BUILD__EXPORT_SYMBOLS_LINK_FLAGS)
set_property(TARGET "${IGC_BUILD__PROJ__igc_dll}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__igc_dll}")

if (FCL_ENABLED)
  #FCL
  #igc_target_flag_property_add_once_config_var("${IGC_BUILD__PROJ__fcl_dll}" LINK_FLAGS IGC_BUILD__EXPORT_SYMBOLS_LINK_FLAGS)
  set_property(TARGET "${IGC_BUILD__PROJ__fcl_dll}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__fcl_dll}")
endif()

#Set Universal properties if this is universal build
if(MSVC AND IGC_OPTION__UNIVERSAL_DRIVER)
    bs_set_wdk(${IGC_BUILD__PROJ__igc_dll})
    set_target_properties(${IGC_BUILD__PROJ__igc_dll} PROPERTIES VS_PLATFORM_TOOLSET WindowsApplicationForDrivers10.0)
    set_target_properties(${IGC_BUILD__PROJ__igc_dll} PROPERTIES VS_GLOBAL_DriverTargetPlatform Universal)
    set_target_properties(${IGC_BUILD__PROJ__igc_dll} PROPERTIES VS_TARGET_PLATFORM_VERSION "")
    windows_libs_props_universal(${IGC_BUILD__PROJ__igc_dll})
    bs_set_wdk(${IGC_BUILD__PROJ__fcl_dll})
    set_target_properties(${IGC_BUILD__PROJ__fcl_dll} PROPERTIES VS_PLATFORM_TOOLSET WindowsApplicationForDrivers10.0)
    set_target_properties(${IGC_BUILD__PROJ__fcl_dll} PROPERTIES VS_GLOBAL_DriverTargetPlatform Universal)
    set_target_properties(${IGC_BUILD__PROJ__fcl_dll} PROPERTIES VS_TARGET_PLATFORM_VERSION "")
    windows_libs_props_universal(${IGC_BUILD__PROJ__fcl_dll})
endif()

if(IGC_OPTION__BUILD_IGC_OPT OR IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS)
  # NOTE: Remember to always pass resources when linking with static library.
  #       Use igc_rc_get_resource() with resource identifier like IGC__igc_lib
  #       Also use IGC_BUILD__LINK_LINE__igc_lib variable to get target_link_libraries()'s line which
  #       links igc_lib correctly (with correct resolve of circular dependencies).
  add_library("${IGC_BUILD__PROJ__igc_lib}" STATIC
      ${IGC_BUILD__SRC__IGC__igc_lib}
      ${IGC_BUILD__HDR__IGC__igc_lib}
    )
  set_target_properties("${IGC_BUILD__PROJ__igc_lib}" PROPERTIES FOLDER "Libraries")

  if(IGC_OPTION__LINK_KHRONOS_SPIRV_TRANSLATOR)
    target_link_libraries("${IGC_BUILD__PROJ__igc_lib}" PRIVATE LLVMSPIRVLib)
  endif()
  if(MSVC)
    bs_set_wdk(${IGC_BUILD__PROJ__igc_lib})
    windows_libs_props_universal(${IGC_BUILD__PROJ__igc_lib})
  endif()
  set_property(TARGET "${IGC_BUILD__PROJ__igc_lib}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__igc_lib}")
  set_property(TARGET "${IGC_BUILD__PROJ__igc_lib}" PROPERTY EXCLUDE_FROM_ALL YES)
  set(IGC_BUILD__MAIN_IGC_LIB_SUFFIXES
      ${IGC_BUILD__MAIN_IGC_LIB_SUFFIXES}
      "__igc_lib"
    )
endif()

# Output names.
if(LLVM_ON_UNIX)
  set(_cpuSuffix "")
else()

  igc_arch_get_cpu(_cpuSuffix)
endif()
set_target_properties("${IGC_BUILD__PROJ__igc_dll}" PROPERTIES OUTPUT_NAME "${IGC_LIBRARY_NAME}${_cpuSuffix}")
set_target_properties("${IGC_BUILD__PROJ__igc_dll}" PROPERTIES
                      VERSION "${IGC_API_MAJOR_VERSION}.${IGC_API_MINOR_VERSION}.${IGC_API_PATCH_VERSION}+${IGC_BUILD_METADATA}"
                      SOVERSION "${IGC_API_MAJOR_VERSION}")
unset(_cpuSuffix)

if (FCL_ENABLED)
  #FCL
  if(LLVM_ON_UNIX)
    set(_cpuSuffix "")
  else()
    igc_arch_get_cpu(_cpuSuffix)
  endif()
  set_target_properties("${IGC_BUILD__PROJ__fcl_dll}" PROPERTIES OUTPUT_NAME "${FCL_LIBRARY_NAME}${_cpuSuffix}")
  set_target_properties("${IGC_BUILD__PROJ__fcl_dll}" PROPERTIES
                        VERSION "${IGC_API_MAJOR_VERSION}.${IGC_API_MINOR_VERSION}.${IGC_API_PATCH_VERSION}+${IGC_BUILD_METADATA}"
                        SOVERSION "${IGC_API_MAJOR_VERSION}")
  unset(_cpuSuffix)
endif()

# Injecting external vISA project.
add_subdirectory("${IGC_BUILD__VISA_DIR}" visa)
set_property(TARGET "GenX_IR" APPEND PROPERTY COMPILE_DEFINITIONS
    ${IGC_BUID__VISA_COMPILE_DEFINITIONS}
  )
set_property(TARGET "GenX_IR" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__GenX_IR}")


# =================================== Link targets and dependencies ====================================

foreach(_libBuildSuffix ${IGC_BUILD__MAIN_IGC_LIB_SUFFIXES})
  add_dependencies("${IGC_BUILD__PROJ${_libBuildSuffix}}"     "${IGC_BUILD__PROJ__GenISAIntrinsics}")
  add_dependencies("${IGC_BUILD__PROJ${_libBuildSuffix}}"     MDAutogen)

  if(MSVC AND IGC_OPTION__BIF_LINK_BC)
    add_dependencies("${IGC_BUILD__PROJ${_libBuildSuffix}}"   "${IGC_BUILD__PROJ__BiFModule_OCL}")
    add_dependencies("${IGC_BUILD__PROJ${_libBuildSuffix}}"   "${IGC_BUILD__PROJ__BiFModuleCache_OCL}")
    add_dependencies("${IGC_BUILD__PROJ${_libBuildSuffix}}"   "${IGC_BUILD__PROJ__BiFManager}")
  endif()
endforeach()

if (FCL_ENABLED)
  #FCL
  add_dependencies("${IGC_BUILD__PROJ__fcl_dll}"     "${IGC_BUILD__PROJ__BiFModule_OCL}")
  add_dependencies("${IGC_BUILD__PROJ__fcl_dll}"     "${IGC_BUILD__PROJ__BiFManager}")
endif()


foreach(_libBuildSuffix ${IGC_BUILD__MAIN_IGC_LIB_SUFFIXES})
  get_property(_targetType TARGET "${IGC_BUILD__PROJ${_libBuildSuffix}}" PROPERTY TYPE)
  if(_targetType MATCHES "^STATIC_LIBRARY$")
    set(_targetIsStatic YES)
  else()
    set(_targetIsStatic NO)
  endif()
  unset(_targetType)

  # list of libraries linked in all configurations
  set(_targetLinkLineCommon)

  # Generated link line for IGC static library allows to properly use it in other projects.
  if(_targetIsStatic)
    list(APPEND _targetLinkLineCommon
        "${IGC_BUILD__PROJ${_libBuildSuffix}}"
      )
  endif()

  if(LLVM_ON_WIN32)
    list(APPEND _targetLinkLineCommon
        "${IGC_BUILD__PROJ__AdaptorOCL}"
    )
  endif()

  #Metrics
  list(APPEND _targetLinkLineCommon
      igc_metric
  )

list(APPEND _targetLinkLineCommon zebinlib)
list(APPEND _targetLinkLineCommon IGCOptions)

list(APPEND _targetLinkLineCommon "${IGC_BUILD__PROJ__VISALinkerDriver}")



    list(APPEND _targetLinkLineCommon
        "${IGC_BUILD__PROJ__BiFManager}"
  )



  if(IGC_BUILD__SPIRV_TOOLS_ENABLED)
    list(APPEND _targetLinkLineCommon
        "$<$<CONFIG:Debug>:${IGC_BUILD__PROJ__SPIRV-Tools}>"
        "$<$<CONFIG:Release>:${IGC_BUILD__PROJ__SPIRV-Tools}>"
    )
  endif()


  list(APPEND _targetLinkLineCommon
      "${IGC_BUILD__PROJ__Compiler}"
      "${IGC_BUILD__PROJ__GenISAIntrinsics}"
      GenX_IR
      GenXDebugInfo
    )

  if(LLVM_ON_UNIX AND IGC_OPTION__BIF_LINK_BC)
    list(APPEND _targetLinkLineCommon
        "${IGC_BUILD__WHOLE_ARCHIVE}"
        "${IGC_BUILD__PROJ__BiFLib_OCL}"
        "${IGC_BUILD__NO_WHOLE_ARCHIVE}"
      )
  endif()

  # Generated link line for IGC static library allows to properly use it in other projects.
  if(_targetIsStatic)
    list(APPEND _targetLinkLineCommon
        "${IGC_BUILD__PROJ${_libBuildSuffix}}"
      )
  endif()

  if(IGC_BUILD__VC_ENABLED)
    list(APPEND _targetLinkLineCommon
      ${IGC_BUILD__PROJ_VC_LIBS_TO_LINK}
      )

    if(_targetIsStatic)
      # Add some VC headers for static library manually since there is
      # no private target_link_libraries for static igc library and
      # include directories are not propagated.
      target_link_libraries("${IGC_BUILD__PROJ${_libBuildSuffix}}"
          PRIVATE
            VCHeaders
        )
      get_target_property(VC_IGCDEPS_HEADERS VCIGCDeps INTERFACE_INCLUDE_DIRECTORIES)
      target_include_directories("${IGC_BUILD__PROJ${_libBuildSuffix}}"
          PRIVATE
            ${VC_IGCDEPS_HEADERS}
        )
    endif()
  endif()

  list(APPEND _targetLinkLineCommon
      "${IGC_BUILD__START_GROUP}"
      ${IGC_BUILD__LLVM_LIBS_TO_LINK}
      "${IGC_BUILD__END_GROUP}"
    )
  if(UNIX)
    list(APPEND _targetLinkLineCommon
      pthread
    )
  endif()
  if(UNIX)
    list(APPEND _targetLinkLineCommon
      -Wl,--exclude-libs,liblldELF.a
    )
  endif()

  if(_targetIsStatic)
    # Link line for projects that use IGC as static library (allows to resolve multi-circular dependencies between libraries).
    set("IGC_BUILD__LINK_LINE_RELEASE${_libBuildSuffix}"
        ${_targetLinkLineCommon}
      )
  else()
    target_link_libraries("${IGC_BUILD__PROJ${_libBuildSuffix}}" PRIVATE
        "${_targetLinkLineCommon}"
        "${CMAKE_DL_LIBS}"
      )
    # Android binary needs to link static libstdc++ and liblog
    if(ANDROID)
        find_package(Threads REQUIRED)
        target_link_libraries("${IGC_BUILD__PROJ${_libBuildSuffix}}" PRIVATE Threads::Threads -static-libstdc++ -llog)
    endif()
    # Link line for shared / dynamic library requires only library project (all static libs are linked inside).
    set_property(TARGET "${IGC_BUILD__PROJ${_libBuildSuffix}}" PROPERTY LINK_INTERFACE_LIBRARIES "")
    set("IGC_BUILD__LINK_LINE${_libBuildSuffix}"
        "${IGC_BUILD__PROJ${_libBuildSuffix}}"
      )
  endif()
endforeach()

if (FCL_ENABLED AND IGC_OPTION__BIF_LINK_BC)
  # Built-in Functions (and resources).
  target_link_libraries("${IGC_BUILD__PROJ__Compiler}" "${IGC_BUILD__PROJ__BiFManager}")
  add_dependencies("${IGC_BUILD__PROJ__Compiler}" "${IGC_BUILD__PROJ__BiFManager}")
endif()

include(igc_pch_reuse)

if (FCL_ENABLED)
#FCL
  get_property(_targetType TARGET "${IGC_BUILD__PROJ__fcl_dll}" PROPERTY TYPE)
  if(_targetType MATCHES "^STATIC_LIBRARY$")
    set(_targetIsStatic YES)
  else()
    set(_targetIsStatic NO)
  endif()
  unset(_targetType)

  set(_targetLinkLine)

  list(APPEND _targetLinkLine IGCOptions)

  # Generated link line for IGC/IGC RS static library allows to properly use it in other projects.
  if(_targetIsStatic)
    list(APPEND _targetLinkLine
        "${IGC_BUILD__PROJ${_libBuildSuffix}}"
      )
  endif()

  # Generated link line for IGC/IGC RS static library allows to properly use it in other projects.
  if(_targetIsStatic)
    list(APPEND _targetLinkLine
        "${IGC_BUILD__PROJ__fcl_dll}"
      )
  endif()

    if(LLVM_ON_UNIX)
      list(APPEND _targetLinkLine
          "${IGC_BUILD__WHOLE_ARCHIVE}"
          "${IGC_BUILD__PROJ__CTHLib_FCL}"
          "${IGC_BUILD__NO_WHOLE_ARCHIVE}"
        )
    endif()

  list(APPEND _targetLinkLine
      "${IGC_BUILD__START_GROUP}"
      ${IGC_BUILD__LLVM_LIBS_TO_LINK}
      "${IGC_BUILD__END_GROUP}"
    )

list(REMOVE_ITEM _targetLinkLine
    IGC_BUILD__LLVM_LIBS_TO_LINK
    )

if(_targetIsStatic)
    # Link line for projects that use IGC as static library (allows to resolve multi-circular dependencies between libraries).
    set("IGC_BUILD__LINK_LINE__fcl_dll"
        ${_targetLinkLine}
      )
  else()
    if (UNIX)
        set(COMMON_CLANG opencl-clang-lib)
    else()
        set(COMMON_CLANG "")
    endif()

    target_link_libraries("${IGC_BUILD__PROJ__fcl_dll}"
        ${_targetLinkLine}
        ${COMMON_CLANG}
        ${CMAKE_DL_LIBS}
      )
    if(IGC_BUILD__VC_ENABLED)
      target_link_libraries("${IGC_BUILD__PROJ__fcl_dll}" AdaptorCM)
    endif()
    # Link line for shared / dynamic library requires only library project (all static libs are linked inside).
    set_property(TARGET "${IGC_BUILD__PROJ__fcl_dll}" PROPERTY LINK_INTERFACE_LIBRARIES "")
    set("IGC_BUILD__LINK_LINE__fcl_dll"
        "${IGC_BUILD__PROJ__fcl_dll}"
      )
  endif()

unset(_targetIsStatic)
unset(_targetLinkLine)
endif() #FCL_ENABLED


if (IGC_OPTION__BUILD_IGC_OPT)
  add_subdirectory(igc_opt)
endif()

if(IGC_OPTION__INCLUDE_IGC_COMPILER_TOOLS)
  # TODO: If we want IGCStandalone on Linux, someone must clean the code, so it will be compiling.
  if(LLVM_ON_UNIX)
    add_subdirectory("${IGC_BUILD__TOOLS_IGC_DIR}" tools)
  endif()
  if(LLVM_ON_WIN32)
    add_subdirectory("${IGC_BUILD__TOOLS_IGC_DIR}" ist)
  endif()
endif()

if(MSVC AND IGC_OPTION__BUILD_IGC_OPT)
  add_custom_command( TARGET ${IGC_BUILD__PROJ__igc_dll}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${IGC_BUILD__PROJ__igc_dll}>" "$<TARGET_FILE_DIR:${IGC_BUILD__PROJ__igc_opt}>"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_PDB_FILE:${IGC_BUILD__PROJ__igc_dll}>" "$<TARGET_FILE_DIR:${IGC_BUILD__PROJ__igc_opt}>"
    )
endif()

# ============================================== LIT TESTS =============================================

if(DEFINED IGC_BUILD__PROJ__igc_opt AND TARGET ${IGC_BUILD__PROJ__igc_opt})
  add_subdirectory(Compiler/tests)
endif()

# ============================================ VC LIT TESTS =============================================

# If VC LITs are enabled, we need main target depend from it
if(IGC_BUILD__CMLIT_ENABLE)
  add_dependencies("${IGC_BUILD__PROJ__igc_dll}" "check-vc")
endif()


if(IGC_OPTION__ENABLE_OCLOC_LIT_TESTS)
  add_subdirectory(ocloc_tests)
endif()

# ======================================================================================================

# Workaround for MSVS10 to avoid the "Dialog Hell"
# NOTE: This could be removed with future version of CMake.
if(MSVC_VERSION EQUAL 1600)
  set(IGC_SLN_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/IGC.sln")
  if (EXISTS "${IGC_SLN_FILENAME}")
    file(APPEND "${IGC_SLN_FILENAME}" "\n# This should be regenerated!\n")
  endif (EXISTS "${IGC_SLN_FILENAME}")
endif()

########################################################################
# cpack to create deb package
########################################################################

set(IGC_PC_PACKAGE_RELEASE "")
if(DEFINED IGC_BUILD_METADATA)
  set(IGC_PC_PACKAGE_RELEASE "-${IGC_BUILD_METADATA}")
endif()
include(cmake/igc_version.cmake)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/igc.opencl.h.in ${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/igc.opencl.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/igc-opencl.pc.in ${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/igc-opencl.pc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_interface/impl/version_in.h ${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/ocl_igc_interface/impl/version.h)

if(UNIX)
  install(TARGETS ${IGC_BUILD__PROJ__igc_dll} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-core NAMELINK_SKIP)
  install(TARGETS ${IGC_BUILD__PROJ__igc_dll} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-core-devel NAMELINK_ONLY)
  if (FCL_ENABLED)
    install(TARGETS ${IGC_BUILD__PROJ__fcl_dll} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-opencl NAMELINK_SKIP)
    install(TARGETS ${IGC_BUILD__PROJ__fcl_dll} LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-opencl-devel NAMELINK_ONLY)
  endif()
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_shared DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc COMPONENT igc-opencl-devel)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/ocl_igc_interface DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc COMPONENT igc-opencl-devel)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/OCLAPI DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc COMPONENT igc-opencl-devel)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/AdaptorOCL/cif DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc COMPONENT igc-opencl-devel)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/common/StateSaveAreaHeader.h DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc/common COMPONENT igc-opencl-devel)


  if(IGC_BUILD__LLVM_SOURCES AND IGC_OPTION__LLVM_LLD)
    # For now lld could be included only via building from LLVM sources
    if(NOT ${CMAKE_BUILD_TYPE} EQUAL "Release")
      install(TARGETS lld RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} COMPONENT igc-opencl-devel)
    endif()
  endif()

  install(
    CODE "file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/postinst \"/sbin/ldconfig\n\" )"
    CODE "file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/postrm \"/sbin/ldconfig\n\" )"
    CODE "file( COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/postinst DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )"
    CODE "file( COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/postrm DESTINATION ${CMAKE_CURRENT_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )"
    COMPONENT igc-core
  )

  install(FILES ${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/igc-opencl.pc DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig COMPONENT igc-opencl-devel)
  install(FILES ${IGC_BUILD__IGC_BIN_DIR}/AdaptorOCL/igc.opencl.h DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc COMPONENT igc-opencl-devel)
   if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../NOTICES.txt)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../NOTICES.txt DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/igc2 COMPONENT igc-core)
  endif()
  install(FILES ${IGC_BUILD__VISA_DIR}/include/RelocationInfo.h DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/visa COMPONENT igc-opencl-devel)
  install(DIRECTORY ${IGC_BUILD__VISA_DIR}/iga/IGALibrary/api/ DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/iga COMPONENT igc-core-devel FILES_MATCHING PATTERN "*.h*")
  install(FILES ${ZEBIN_INCLUDE_PATH}/../ZEELF.h DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/igc/zebin COMPONENT igc-core-devel)

elseif (WIN32)
  install(TARGETS ${IGC_BUILD__PROJ__igc_dll}
          RUNTIME DESTINATION Release/bin          CONFIGURATIONS Release)
  install(TARGETS ${IGC_BUILD__PROJ__igc_dll}
          RUNTIME DESTINATION Debug/bin            CONFIGURATIONS Debug)

  install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__igc_dll}>
          DESTINATION Release/pdb              CONFIGURATIONS Release)
  install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__igc_dll}>
          DESTINATION Debug/pdb                CONFIGURATIONS Debug)

  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # Only copy 64 build

    if(IGC_BUILD__LLVM_SOURCES AND IGC_OPTION__LLVM_LLD)
      # For now lld could be included only via building from LLVM sources
      install(TARGETS lld RUNTIME
              DESTINATION ../Test_Tools/Release/x64/Standalones CONFIGURATIONS Release)
      install(TARGETS lld RUNTIME
              DESTINATION ../Test_Tools/Debug/x64/Standalones            CONFIGURATIONS Debug)
    endif()

    install(TARGETS ${IGC_BUILD__PROJ__igc_dll} RUNTIME
            DESTINATION ../Test_Tools/Release/x64/Standalones          CONFIGURATIONS Release)
    install(TARGETS ${IGC_BUILD__PROJ__igc_dll} RUNTIME
            DESTINATION ../Test_Tools/Debug/x64/Standalones            CONFIGURATIONS Debug)


  endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

  if (FCL_ENABLED)
    install(TARGETS ${IGC_BUILD__PROJ__fcl_dll}
            RUNTIME DESTINATION Release/bin          CONFIGURATIONS Release)
    install(TARGETS ${IGC_BUILD__PROJ__fcl_dll}
            RUNTIME DESTINATION Debug/bin            CONFIGURATIONS Debug)

    install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__fcl_dll}>
            DESTINATION Release/pdb              CONFIGURATIONS Release)
    install(FILES $<TARGET_PDB_FILE:${IGC_BUILD__PROJ__fcl_dll}>
            DESTINATION Debug/pdb                CONFIGURATIONS Debug)
  endif()
endif()

if(UNIX)
  if(IGC_OPTION__CPACK_GENERATOR)
    set(CPACK_GENERATOR "${IGC_OPTION__CPACK_GENERATOR}")
  else()
    # If generators list was not defined build native package for current distro
    if(EXISTS "/etc/debian_version")
      set(CPACK_GENERATOR "DEB")
    elseif(EXISTS "/etc/redhat-release")
      set(CPACK_GENERATOR "RPM")
    else()
      set(CPACK_GENERATOR "TXZ")
      set(CPACK_SET_DESTDIR ON)
    endif()
  endif()

  # Architecture-related options
  if("${IGC_OPTION__ARCHITECTURE_TARGET}" STREQUAL "LinuxARM")
    set(CPACK_PACKAGE_ARCHITECTURE "arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "arm64")
  else()
    set(CPACK_PACKAGE_ARCHITECTURE "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
  endif()

  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "intel")
  set(CPACK_PACKAGE_NAME "intel")
  set(CPACK_PACKAGE_VERSION_MAJOR ${IGC_API_MAJOR_VERSION})
  set(CPACK_PACKAGE_VERSION_MINOR ${IGC_API_MINOR_VERSION})
  set(CPACK_PACKAGE_VERSION_PATCH ${IGC_API_PATCH_VERSION})
  set(CPACK_PACKAGE_VERSION_BUILD ${IGC_BUILD_METADATA})

  # Check if current build is a release build
  #
  # If build is a release build the version string will be MAJOR.MINOR.PATCH+RELEASE_METADATA
  # Else the version string becomes MAJOR.MINOR.PATCH-dev.BUILD_METADATA

  if(DEFINED IGC_RELEASE_METADATA AND NOT IGC_RELEASE_METADATA STREQUAL "")
    set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
    set(PACKAGE_VERSION_WTH_METADATA "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}+${IGC_RELEASE_METADATA}")
  else()
    set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-dev.${CPACK_PACKAGE_VERSION_BUILD}")
    set(PACKAGE_VERSION_WTH_METADATA "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-dev.${CPACK_PACKAGE_VERSION_BUILD}")
  endif()

  set(CPACK_PACKAGE_INSTALL_DIRECTORY ${IGC_INSTALL_TIME_ROOT_DIR})
  set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "postinst;postrm")
  set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Intel(R) Graphics Compiler for OpenCL(TM)\nBase revision: ${IGC_BUILD_METADATA+IGC_RELEASE_METADATA}")
  set(CPACK_RPM_PACKAGE_DESCRIPTION "Intel(R) Graphics Compiler for OpenCL(TM)\nBase revision: ${IGC_BUILD_METADATA+IGC_RELEASE_METADATA}")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_RPM_PACKAGE_RELOCATABLE TRUE)

  set(CPACK_DEBIAN_IGC-CORE_FILE_NAME "intel-igc-core-2_${PACKAGE_VERSION_WTH_METADATA}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
  set(CPACK_DEBIAN_IGC-CORE_PACKAGE_NAME "intel-igc-core-2")
  set(CPACK_DEBIAN_IGC-CORE-DEVEL_FILE_NAME "intel-igc-core-devel_${PACKAGE_VERSION_WTH_METADATA}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
  set(CPACK_DEBIAN_IGC-CORE-DEVEL_PACKAGE_NAME "intel-igc-core-devel")
  set(CPACK_DEBIAN_IGC-CORE-DEVEL_PACKAGE_DEPENDS "intel-igc-core-2(=${CPACK_PACKAGE_VERSION})")
  set(CPACK_DEBIAN_IGC-OPENCL_FILE_NAME "intel-igc-opencl-2_${PACKAGE_VERSION_WTH_METADATA}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
  set(CPACK_DEBIAN_IGC-OPENCL_PACKAGE_NAME "intel-igc-opencl-2")
  set(CPACK_DEBIAN_IGC-OPENCL_PACKAGE_DEPENDS "intel-igc-core-2(=${CPACK_PACKAGE_VERSION})")
  set(CPACK_DEBIAN_IGC-OPENCL-DEVEL_FILE_NAME "intel-igc-opencl-devel_${PACKAGE_VERSION_WTH_METADATA}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
  set(CPACK_DEBIAN_IGC-OPENCL-DEVEL_PACKAGE_DEPENDS "intel-igc-opencl-2(=${CPACK_PACKAGE_VERSION})")

  set(CPACK_RPM_IGC-CORE_FILE_NAME "intel-igc-core-2-${PACKAGE_VERSION_WTH_METADATA}.${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
  set(CPACK_RPM_IGC-CORE-DEVEL_FILE_NAME "intel-igc-core-devel_${PACKAGE_VERSION_WTH_METADATA}_${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
  set(CPACK_RPM_IGC-CORE-DEVEL_PACKAGE_NAME "intel-igc-core-devel")
  set(CPACK_RPM_IGC-CORE-DEVEL_PACKAGE_REQUIRES "intel-igc-core-2 = ${CPACK_PACKAGE_VERSION}")
  set(CPACK_RPM_IGC-CORE_PACKAGE_NAME "intel-igc-core-2")
  set(CPACK_RPM_IGC-OPENCL_FILE_NAME "intel-igc-opencl-2-${PACKAGE_VERSION_WTH_METADATA}.${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
  set(CPACK_RPM_IGC-OPENCL_PACKAGE_NAME "intel-igc-opencl-2")
  set(CPACK_RPM_IGC-OPENCL_PACKAGE_REQUIRES "intel-igc-core-2 = ${CPACK_PACKAGE_VERSION}")
  set(CPACK_RPM_IGC-OPENCL-DEVEL_FILE_NAME "intel-igc-opencl-devel-${PACKAGE_VERSION_WTH_METADATA}.${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
  set(CPACK_RPM_IGC-OPENCL-DEVEL_PACKAGE_REQUIRES "intel-igc-opencl-2 = ${CPACK_PACKAGE_VERSION}}")

  set(CPACK_ARCHIVE_IGC-CORE_FILE_NAME "intel-igc-core-2-${PACKAGE_VERSION_WTH_METADATA}.${CPACK_PACKAGE_ARCHITECTURE}")
  set(CPACK_ARCHIVE_IGC-OPENCL_FILE_NAME "intel-igc-opencl-2-${PACKAGE_VERSION_WTH_METADATA}.${CPACK_PACKAGE_ARCHITECTURE}")
  set(CPACK_ARCHIVE_IGC-OPENCL-DEVEL_FILE_NAME "intel-igc-opencl-devel-${PACKAGE_VERSION_WTH_METADATA}.${CPACK_PACKAGE_ARCHITECTURE}")
  set(CPACK_ARCHIVE_IGC-CORE-DEVEL_FILE_NAME "intel-igc-core-devel-${PACKAGE_VERSION_WTH_METADATA}.${CPACK_PACKAGE_ARCHITECTURE}")

  # Create separate debug symbol packages if the CMake/CPack version in use
  # provides stable support for that functionality.
  if (NOT CMAKE_VERSION VERSION_LESS 3.20)
    set(CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)
  endif()
  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_RPM_COMPONENT_INSTALL ON)
  set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
  set(CPACK_COMPONENTS_ALL igc-core igc-opencl igc-opencl-devel igc-core-devel)

  include (CPack)
endif(UNIX)
