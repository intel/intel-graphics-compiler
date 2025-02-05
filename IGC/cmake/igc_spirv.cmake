#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_guard(DIRECTORY)

# SPIRV translator was added with LLVM (source build or prebuild).

if(NOT TARGET LLVMSPIRVLib OR (WIN32 AND NOT IGC_BUILD__SPIRV_TRANSLATOR_SOURCES))
  message(STATUS "[IGC] Trying to find prebuilt SPIRV library")
  find_package(SPIRVLLVMTranslator ${LLVM_VERSION_MAJOR} REQUIRED)
else()
  message(STATUS "[IGC] Using LLVMSPIRVLib that comes with LLVM")

  # Guess location of header files.
  get_target_property(_is_imported LLVMSPIRVLib IMPORTED)
  if(_is_imported)
    # Imported location property for LLVM can have one of these suffixes.
    set(_prop_types
      ""
      "_RELEASE"
      "_DEBUG"
      "_RELWITHDEBINFO"
      "_MINSIZEREL"
      )
    foreach(t IN LISTS _prop_types)
      get_target_property(_lib_loc LLVMSPIRVLib IMPORTED_LOCATION${t})
      if(_lib_loc)
        break()
      endif()
    endforeach()
    # Installed spirv package has the following directory layout:
    # |-lib/LLVMSPIRVLib.a
    # `-include/LLVMSPIRVLib/<headers>
    # So get include directories based on location of imported library.
    get_filename_component(_inc_dir ${_lib_loc} DIRECTORY)
    get_filename_component(_inc_dir ${_inc_dir}/../include/LLVMSPIRVLib ABSOLUTE)
    unset(_lib_loc)
  else()
    # SPIRV sources has the following directory layout:
    # |-lib/SPIRV/CMakeLists.txt with LLVMSPIRVLib target
    # `-include/<headers>
    # Similarly to the imported target get required include dirs.
    get_target_property(_src_dir LLVMSPIRVLib SOURCE_DIR)
    get_filename_component(_inc_dir ${_src_dir}/../../include ABSOLUTE)
    unset(_srcdir)
  endif()
  unset(_is_imported)

  # Add headers. Since target can be imported, use property.
  # Additionally, for in-tree build, do not set install interface
  # since it can be polluted with build directory artifact.
  set_target_properties(LLVMSPIRVLib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${_inc_dir};${_inc_dir}/../lib>"
    )
  unset(_inc_dir)
endif()

