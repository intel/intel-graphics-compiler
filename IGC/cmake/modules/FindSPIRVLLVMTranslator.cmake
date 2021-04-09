#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2021-2021 Intel Corporation
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

# - Try to find LLVMSPIRVLib and optionally llvm-spirv
# Once done this will define
#  SPIRVLLVMTranslator_Library_FOUND -- library is found
#  SPIRVLLVMTranslator_Library -- path to LLVMSPIRVLib library
#  SPIRVLLVMTranslator_INCLUDE_DIR -- include directories for LLVMSPIRVLib
# LLVMSPIRVLib target will be created with all needed dependencies.
# Optionally, llvm-spirv tool will be checked and LLVMSPIRVTranslator_Tool_FOUND
# will be set to true and llvm-spirv executable target will be defined.

find_library(SPIRVLLVMTranslator_Library LLVMSPIRVLib)
find_path(SPIRVLLVMTranslator_INCLUDE_DIR "LLVMSPIRVLib.h"
  PATH_SUFFIXES LLVMSPIRVLib
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

# Add interface target for llvm-spirv.
if(SPIRVLLVMTranslator_Tool_FOUND AND NOT TARGET llvm-spirv)
  add_executable(llvm-spirv IMPORTED)
  set_target_properties(llvm-spirv PROPERTIES
    IMPORTED_LOCATION ${SPIRVLLVMTranslator_Tool}
    )
endif()
