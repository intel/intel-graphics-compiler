#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# - Try to find LLVMSPIRVLib and optionally llvm-spirv
# Once done this will define
#  SPIRVLLVMTranslator_Library_FOUND -- library is found
#  SPIRVLLVMTranslator_Library -- path to LLVMSPIRVLib library
#  SPIRVLLVMTranslator_INCLUDE_DIR -- include directories for LLVMSPIRVLib
# LLVMSPIRVLib target will be created with all needed dependencies.
# Optionally, llvm-spirv tool will be checked and LLVMSPIRVTranslator_Tool_FOUND
# will be set to true and llvm-spirv executable target will be defined.

if(WIN32)
  find_library(SPIRVLLVMTranslator_Library_Release LLVMSPIRVLib PATHS "${LLVM_DIR}/Release/lib")
  find_library(SPIRVLLVMTranslator_Library_Debug LLVMSPIRVLib PATHS "${LLVM_DIR}/Debug/lib")

  find_path(SPIRVLLVMTranslator_INCLUDE_DIR "LLVMSPIRVLib.h"
    PATH_SUFFIXES LLVMSPIRVLib
    PATHS "${IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR}/include"
    )

  # Try to find version and llvm-spirv.
  if(SPIRVLLVMTranslator_Library_Release AND SPIRVLLVMTranslator_Library_Debug)
    set(SPIRVLLVMTranslator_Library_FOUND YES)

    get_filename_component(_libdir_Release ${SPIRVLLVMTranslator_Library_Release} DIRECTORY)
    get_filename_component(_libdir_Debug ${SPIRVLLVMTranslator_Library_Debug} DIRECTORY)

    # Get library version. Manual parsing is used because it is quite easy and
    # will work for both linux and windows (which is lacking pkg-config).
    file(STRINGS "${_libdir_Release}/pkgconfig/LLVMSPIRVLib.pc" _version_file_release
      REGEX "Version: .*")
    file(STRINGS "${_libdir_Debug}/pkgconfig/LLVMSPIRVLib.pc" _version_file_debug
      REGEX "Version: .*")

    if(NOT _version_file_release OR NOT _version_file_debug)
      message(AUTHOR_WARNING "LLVMSPIRVLib is found, but LLVMSPIRVLib.pc is missing!")
    endif()
    string(REPLACE "Version: " "" SPIRVLLVMTranslator_VERSION_Release "${_version_file_release}")
    string(REPLACE "Version: " "" SPIRVLLVMTranslator_VERSION_Debug "${_version_file_debug}")
    set(SPIRVLLVMTranslator_VERSION_Release ${SPIRVLLVMTranslator_VERSION_Release} CACHE STRING "SPIRVLLVMTranslator release version")
    set(SPIRVLLVMTranslator_VERSION_Debug ${SPIRVLLVMTranslator_VERSION_Debug} CACHE STRING "SPIRVLLVMTranslator debug version")

    # Try to find llvm-spirv.
    find_program(SPIRVLLVMTranslator_Tool_Release llvm-spirv
      PATHS ${_libdir_Release}/../bin
      NO_DEFAULT_PATH
      )
    find_program(SPIRVLLVMTranslator_Tool_Debug llvm-spirv
      PATHS ${_libdir_Debug}/../bin
      NO_DEFAULT_PATH
      )

    if(SPIRVLLVMTranslator_Tool_Release AND SPIRVLLVMTranslator_Tool_Debug)
      set(SPIRVLLVMTranslator_Tool_FOUND YES)
    endif()

    unset(_libdir)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SPIRVLLVMTranslator_Release
    REQUIRED_VARS SPIRVLLVMTranslator_Library_Release SPIRVLLVMTranslator_INCLUDE_DIR
    VERSION_VAR SPIRVLLVMTranslator_VERSION_Release
    HANDLE_COMPONENTS
    )
  find_package_handle_standard_args(SPIRVLLVMTranslator_Debug
    REQUIRED_VARS SPIRVLLVMTranslator_Library_Debug SPIRVLLVMTranslator_INCLUDE_DIR
    VERSION_VAR SPIRVLLVMTranslator_VERSION_Debug
    HANDLE_COMPONENTS
    )

  # Mark all cached variables as advanced.
  if(SPIRVLLVMTranslator_Release_FOUND)
    mark_as_advanced(SPIRVLLVMTranslator_Library_Release)
    mark_as_advanced(SPIRVLLVMTranslator_INCLUDE_DIR)
    mark_as_advanced(SPIRVLLVMTranslator_VERSION_Release)
  endif()

  if(SPIRVLLVMTranslator_Debug_FOUND)
    mark_as_advanced(SPIRVLLVMTranslator_Library_Debug)
    mark_as_advanced(SPIRVLLVMTranslator_INCLUDE_DIR)
    mark_as_advanced(SPIRVLLVMTranslator_VERSION_Debug)
  endif()

  if(SPIRVLLVMTranslator_Tool_Release_FOUND)
    mark_as_advanced(SPIRVLLVMTranslator_Tool_Release)
  endif()

  if(SPIRVLLVMTranslator_Tool_Debug_FOUND)
  mark_as_advanced(SPIRVLLVMTranslator_Tool_Debug)
  endif()

  # Add interface target for library.
  if(SPIRVLLVMTranslator_FOUND AND NOT TARGET LLVMSPIRVLib)
    add_library(LLVMSPIRVLib IMPORTED UNKNOWN)
    set_target_properties(LLVMSPIRVLib PROPERTIES
      IMPORTED_LOCATION_RELEASE "${SPIRVLLVMTranslator_Library_Release}"
      IMPORTED_LOCATION_RELEASEINTERNAL "${SPIRVLLVMTranslator_Library_Release}"
      IMPORTED_LOCATION_DEBUG "${SPIRVLLVMTranslator_Library_Debug}"
      INTERFACE_INCLUDE_DIRECTORIES ${SPIRVLLVMTranslator_INCLUDE_DIR}
      )
  endif()

  if (NOT IGC_BUILD__SPIRV_TRANSLATOR_SOURCES)
    set_target_properties(LLVMSPIRVLib PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${SPIRVLLVMTranslator_INCLUDE_DIR}
      )
  endif()

  # Add interface target for llvm-spirv.
  if(SPIRVLLVMTranslator_Tool_FOUND AND NOT TARGET llvm-spirv)
    add_executable(llvm-spirv IMPORTED)
    set_target_properties(llvm-spirv PROPERTIES
      IMPORTED_LOCATION_RELEASE "${SPIRVLLVMTranslator_Tool_Release}"
      IMPORTED_LOCATION_RELEASEINTERNAL "${SPIRVLLVMTranslator_Tool_Release}"
      IMPORTED_LOCATION_DEBUG "${SPIRVLLVMTranslator_Tool_Debug}"
      )
  endif()
else()
  find_library(SPIRVLLVMTranslator_Library LLVMSPIRVLib PATHS ${LLVM_LIB_DIR})
  find_path(SPIRVLLVMTranslator_INCLUDE_DIR "LLVMSPIRVLib.h"
    PATH_SUFFIXES LLVMSPIRVLib
    PATHS "${IGC_OPTION__SPIRV_TRANSLATOR_SOURCE_DIR}/include"
    )

  # Try to find version and llvm-spirv.
  if(SPIRVLLVMTranslator_Library)
    set(SPIRVLLVMTranslator_Library_FOUND YES)

    get_filename_component(_libdir ${SPIRVLLVMTranslator_Library} DIRECTORY)

    # Get library version. Manual parsing is used because it is quite easy and
    # will work for both linux and windows (which is lacking pkg-config).
    file(STRINGS "${_libdir}/pkgconfig/LLVMSPIRVLib.pc" _version_file
      REGEX "Version: .*")
    if(NOT _version_file)
      message(AUTHOR_WARNING "LLVMSPIRVLib is found, but LLVMSPIRVLib.pc is missing!")
    endif()
    string(REPLACE "Version: " "" SPIRVLLVMTranslator_VERSION "${_version_file}")
    set(SPIRVLLVMTranslator_VERSION ${SPIRVLLVMTranslator_VERSION} CACHE STRING "SPIRVLLVMTranslator version")

    # Try to find llvm-spirv.
    find_program(SPIRVLLVMTranslator_Tool llvm-spirv
      PATHS ${_libdir}/../bin
      NO_DEFAULT_PATH
      )
    if(SPIRVLLVMTranslator_Tool)
      set(SPIRVLLVMTranslator_Tool_FOUND YES)
    endif()

    unset(_libdir)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SPIRVLLVMTranslator
    REQUIRED_VARS SPIRVLLVMTranslator_Library SPIRVLLVMTranslator_INCLUDE_DIR
    VERSION_VAR SPIRVLLVMTranslator_VERSION
    HANDLE_COMPONENTS
    )

  # Mark all cached variables as advanced.
  if(SPIRVLLVMTranslator_FOUND)
    mark_as_advanced(SPIRVLLVMTranslator_Library)
    mark_as_advanced(SPIRVLLVMTranslator_INCLUDE_DIR)
    mark_as_advanced(SPIRVLLVMTranslator_VERSION)
  endif()

  if(SPIRVLLVMTranslator_Tool_FOUND)
    mark_as_advanced(SPIRVLLVMTranslator_Tool)
  endif()

  # Add interface target for library.
  if(SPIRVLLVMTranslator_FOUND AND NOT TARGET LLVMSPIRVLib)
    add_library(LLVMSPIRVLib IMPORTED UNKNOWN)
    set_target_properties(LLVMSPIRVLib PROPERTIES
      IMPORTED_LOCATION ${SPIRVLLVMTranslator_Library}
      INTERFACE_INCLUDE_DIRECTORIES ${SPIRVLLVMTranslator_INCLUDE_DIR}
      )
  endif()

  if (NOT IGC_BUILD__SPIRV_TRANSLATOR_SOURCES)
    set_target_properties(LLVMSPIRVLib PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${SPIRVLLVMTranslator_INCLUDE_DIR}
      )
  endif()

  # Add interface target for llvm-spirv.
  if(SPIRVLLVMTranslator_Tool_FOUND AND NOT TARGET llvm-spirv)
    add_executable(llvm-spirv IMPORTED)
    set_target_properties(llvm-spirv PROPERTIES
      IMPORTED_LOCATION ${SPIRVLLVMTranslator_Tool}
      )
  endif()
endif()