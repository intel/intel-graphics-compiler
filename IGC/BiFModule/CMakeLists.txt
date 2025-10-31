#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================


# ======================================================================================================
# ================================================ UTILS ===============================================
# ======================================================================================================

# =============================================== Tools ================================================
add_subdirectory(Languages/OpenCL/PreRelease/Matrix)
include(BiFModuleCache)

# In new Clang versions VME types are built-in. Keep this flag until all OS's update Clang version to 8
if(NOT DEFINED VME_TYPES_DEFINED)
  set(VME_TYPES_DEFINED TRUE)
endif()

if(UNIX)
  if(NOT ${CCLANG_BUILD_INTREE_LLVM})
    # Get common clang library soname
    get_target_property(CCLANG_LIB_PATH opencl-clang-lib "IMPORTED_LOCATION")
    execute_process(
      COMMAND readelf -d ${CCLANG_LIB_PATH}
      RESULT_VARIABLE CCLANG_READELF_RESULT
      OUTPUT_VARIABLE CCLANG_READELF_CALL)
    if(CCLANG_READELF_RESULT AND NOT CCLANG_READELF_RESULT EQUAL 0)
      message(FATAL_ERROR "[IGC\\BiFModule] : Error occurred while executing readelf:  ${CCLANG_READELF_RESULT}")
    endif()

    # READ ELF CALL Output:
    # Dynamic section at offset 0x6957838 contains 35 entries:
    #   Tag        Type                         Name/Value
    #  0x0000000000000001 (NEEDED)             Shared library: [libz.so.1]
    #  0x0000000000000001 (NEEDED)             Shared library: [libzstd.so.1]
    #  0x0000000000000001 (NEEDED)             Shared library: [libstdc++.so.6]
    #  0x0000000000000001 (NEEDED)             Shared library: [libm.so.6]
    #  0x0000000000000001 (NEEDED)             Shared library: [libgcc_s.so.1]
    #  0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
    #  0x0000000000000001 (NEEDED)             Shared library: [ld-linux-x86-64.so.2]
    #  0x000000000000000e (SONAME)             Library soname: [libopencl-clang.so.16]
    #  0x0000000000000010 (SYMBOLIC)           0x0
    #  0x000000000000001d (RUNPATH)            Library runpath: [$ORIGIN/../lib]
    #  0x000000000000000c (INIT)               0x7ab000
    #  0x000000000000000d (FINI)               0x53ed664

    set(CLANG_LIB_REGEX "${COMMON_CLANG_LIB_NAME_WITH_PREFIX}([0-9]+|\\.|so)+")
    string(REGEX MATCH  "${CLANG_LIB_REGEX}" IMPORTED_SONAME "${CCLANG_READELF_CALL}")
    message("Extraced SONAME: ${IMPORTED_SONAME}")

    set_property(TARGET opencl-clang-lib PROPERTY "IMPORTED_SONAME" "${IMPORTED_SONAME}")
  endif()
  if (NOT CCLANG_FROM_SYSTEM)
    install(FILES $<TARGET_SONAME_FILE:opencl-clang-lib> DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-opencl)
    install(FILES $<TARGET_FILE:opencl-clang-lib> DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-opencl-devel)

    # Get header opencl-c.h directory
    get_filename_component(opencl-headers-dir ${opencl-header} DIRECTORY)

    set(opencl-headers "${opencl-header}")
    # Fix for the split of opencl-c.h into opencl-c.h and opencl-c-base.h
    # https://github.com/llvm/llvm-project/commit/af1c230e70aeb72ec9d6363f8d91e3c7db3ef9f2
    if(EXISTS "${opencl-headers-dir}/opencl-c-base.h")
      set(opencl-headers
       "${opencl-headers}"
       "${opencl-headers-dir}/opencl-c-base.h")
    endif()
    install(FILES ${opencl-headers} DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR} COMPONENT igc-opencl-devel)
  endif()
endif()

# End Clang section



# ========================================= Helper functions ===========================================

# ======================================================================================================



# ======================================================================================================

# Returns list common OpenCL C files from selected directories:
# - sources (.cl)
# - headers (.h)
#
# @param retVarName         Name of placeholder variable where results will be stored.
# @param [includeDir [...]] Full path to directory which will be searched for common files.
function(igc_bif_find_cl_files retVarName)
  set(_filePaths)
  foreach(_includeDir ${ARGN})
    string(REPLACE ";" "\;" _includeDir "${_includeDir}")
    file(GLOB_RECURSE _sourceFilePaths "${_includeDir}/*.[cC][lL]")
    file(GLOB_RECURSE _headerFilePaths "${_includeDir}/*.[hH]")
    list(APPEND _filePaths ${_sourceFilePaths} ${_headerFilePaths})
  endforeach()
  set("${retVarName}" ${_filePaths} PARENT_SCOPE)
endfunction()

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================

# ======================================================================================================
# ===================================== BUILD STEP CONFIGURATION =======================================
# ======================================================================================================

# ======================================== Precompiled headers =========================================
if(NOT CCLANG_FROM_SYSTEM)
  set(IGC_BUILD__BIF_OCL_INCLUDES ${opencl-header})
  set(PCH_OPTIONS "-include ${opencl-header}")
else()
  set(PCH_OPTIONS ${CL_OPTIONS})
endif()

file(GLOB_RECURSE _PRE_RELEASE_CL "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PreRelease/*.cl")

set(IGC_BUILD__BIF_OCL_SHARED_INC                "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/opencl_cth_released.h")
set(IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PreRelease/opencl_cth_pre_release.h")

set(IGC_BUILD__PROJ__BiFModule_OCL       "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleOcl")
set(IGC_BUILD__PROJ__BiFModule_OCL       "${IGC_BUILD__PROJ__BiFModule_OCL}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__BiFModule_OCL "BiFModule-OCL")
set(IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL clang-tool "${IGC_BUILD__BIF_OCL_FORCE_INC}" "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.h" "${IGC_BUILD__BIF_DIR}/opencl_cth.h" "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bifbc" "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc" "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc" "${IGC_BUILD__BIF_DIR}/IGCsize_t_32.bc" "${IGC_BUILD__BIF_DIR}/IGCsize_t_64.bc" "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc")
set(IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL "${IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL}" PARENT_SCOPE)


add_custom_target("${IGC_BUILD__PROJ__BiFModule_OCL}"
    DEPENDS ${IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL}
    SOURCES ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
  )
set_property(TARGET "${IGC_BUILD__PROJ__BiFModule_OCL}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__BiFModule_OCL}")

set_target_properties("${IGC_BUILD__PROJ__BiFModule_OCL}" PROPERTIES FOLDER "Misc/BiF")

generate_bif_src_checksum(${IGC_BUILD__PROJ__BiFModule_OCL})

build_bif_bitcode("${IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL}")

generate_bif_prebuild_pack(${IGC_BUILD__PROJ__BiFModuleCache_OCL} "${IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL}")

set(IGC_BUILD__PROJ__BiFModuleCache_OCL       "${IGC_BUILD__PROJ__BiFModuleCache_OCL}" PARENT_SCOPE)

add_dependencies("${IGC_BUILD__PROJ__BiFModule_OCL}" "${IGC_BUILD__PROJ__BiFModuleCacheBuildPack_OCL}")

if(LLVM_ON_UNIX)
  add_subdirectory(linux)
  add_dependencies("${IGC_BUILD__PROJ__BiFLib_OCL}" "${IGC_BUILD__PROJ__BiFModuleCache_OCL}")
  #NOTE: Transfer names of projects to parent scope.
  set(IGC_BUILD__PROJ__BiFLib_OCL "${IGC_BUILD__PROJ__BiFLib_OCL}" PARENT_SCOPE)
endif()

# =========================================== SPIRV Headers ============================================

add_library(IGCSPIRVHeaders INTERFACE)
target_include_directories(IGCSPIRVHeaders
  INTERFACE
  ${IGC_OPTION__BIF_SRC_OCL_DIR}/Headers
  )

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================
