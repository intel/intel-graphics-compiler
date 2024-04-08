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
    string(REGEX MATCH "\\[${COMMON_CLANG_LIB_FULL_NAME}\\.([0-9](\\.[0-9]*)*[a-zA-Z0-9]*)\\]" CCLANG_SONAME_VERSION "${CCLANG_READELF_CALL}")
    set(CCLANG_SONAME_VERSION "${CMAKE_MATCH_1}")

    # Check if common clang library is newer than 5.0.0 version on which we have SPIR-V support
    if("${CCLANG_SONAME_VERSION}" VERSION_GREATER "5.0.0")
      set_property(TARGET opencl-clang-lib PROPERTY "IMPORTED_SONAME" "${COMMON_CLANG_LIB_FULL_NAME}.${CCLANG_SONAME_VERSION}")
    else()
      message(FATAL_ERROR "[IGC\\BiFModule] : Version ${CCLANG_SONAME_VERSION} of library ${COMMON_CLANG_LIB_FULL_NAME} is below version 5.0.0 (where it's starts support of SPIR-V), please upgrade this library at least to version 5.0.0")
    endif()
  endif()
  if (NOT CCLANG_FROM_SYSTEM)
    install(FILES $<TARGET_FILE:opencl-clang-lib> DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-opencl)
    install(FILES $<TARGET_SONAME_FILE:opencl-clang-lib> DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-opencl)

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

# Creates custom command which builds LLVM Bitcode file for built-in functions.
#
# igc_bif_build_bc(
#     OUTPUT               outBcFilePath
#     TRIPLE               archTriple
#     SOURCES              srcFilePath [srcFilePath [...]]
#     [LANGUAGE            srcLanguage]
#     [PRECOMPILED         pchFilePath]
#     [FORCE_INCLUDE       incFilePath [incFilePath [...]]]
#     [INCLUDE_DIRECTORIES includeDir [includeDir [...]]]
#     [DEFINES             define [define [...]]]
#     [DEPENDS             dependency [dependency [...]]]
#     [OPTIMIZE            useOpt]
#     [OPTIONS             lang [option [...]]]
#     [OPTIONS             lang [option [...]]]
#     [...]
#     [OPTIONS             lang [option [...]]]
#   )
#
# @param outBcFilePath      Full path where output bitcode file should be written.
#                           Output file is only updated if it does not exist or it was changed.
# @param archTriple         Identifier for target architecture for compiled sources.
# @param srcFilePath        Path to source file which will be compiled into .bc (multiple can be specified).
#                           If multiple files are specified, each file is converted to .bc and output .bc
#                           is created by linking intermediate .bc files.
# @param srcLanguage        Language used to compile/build source files. The following are supported:
#                            - "DEFAULT" - determined by extension (.ll -> "LL"; .bc -> "BC"; .* (other) -> "CL").
#                            - "CL"      - Sources treated as OpenCL C sources (CLANG will be used).
#                            - "LL"      - Sources treated as LLVM IR text files (LLVM-AS will be used).
#                            - "BC"      - Sources treated as LLVM Bitcode files (LLVM-LINK will be used).
#                           If not specified, "DEFAULT" is used.
# @param pchFilePath        Includes precompiled header. Ignored for other files than "CL".
# @param incFilePath        Includes header for each compiled source. Ignored for other files than "CL".
# @param includeDir         Adds include directory. Ignored for other files than "CL".
# @param define             Additional preprocessor definition (multiple can be specified). Ignored for other files than "CL".
# @param useOpt             Use optmizer (boolean value). Only final created .bc is optimized (intermediate .bc created during
#                           build will not be put into optimizer).  Default is FALSE.
# @param dependency         Additional files / targets that source files depends on (multiple can be specified).
# @param lang               Language for which options are defined (currently "CL", "LL", "BC" and "OPT" for optimizer;
#                           "DEFAULT" will add options to all lanuages).
# @param option             Additional compilation option (multiple can be specified).
function(igc_bif_build_bc)
  set(_mainOpts
    OPTIMIZE
    )
  set(_mainSingleKw
    OUTPUT
    TRIPLE
    LANGUAGE
    )
  set(_mainMultiKw
    SOURCES
    PRECOMPILED
    FORCE_INCLUDE
    INCLUDE_DIRECTORIES
    DEFINES
    DEPENDS
    OPTIONS
    )
  cmake_parse_arguments(ARG
    "${_mainOpts}"
    "${_mainSingleKw}"
    "${_mainMultiKw}"
    ${ARGN}
    )
  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  set(__allowedLanguages DEFAULT CL LL BC)
  set(_optionsMultiKw ${__allowedLanguages} OPT)
  # OPTIMIZE lang suboptions.
  cmake_parse_arguments(OPTIONS
    ""
    ""
    "${_optionsMultiKw}"
    ${ARG_OPTIONS}
    )
  if(OPTIONS_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown options arguments: ${OPTIONS_UNPARSED_ARGUMENTS}")
  endif()


  # Required arguments.
  if(NOT ARG_OUTPUT)
    message(FATAL_ERROR "Output is not specified for igc_bif_build_bc")
  endif()
  set(_outBcFilePath ${ARG_OUTPUT})
  if(NOT ARG_TRIPLE)
    message(FATAL_ERROR "Triple is not specified for igc_bif_build_bc")
  endif()
  set(_archTriple ${ARG_TRIPLE})
  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "Sources are not specified for igc_bif_build_bc")
  endif()
  set(_srcFilePaths ${ARG_SOURCES})

  # Optional arguments.
  set(_srcLanguage "DEFAULT")
  if(ARG_LANGUAGE)
    if(NOT (ARG_LANGUAGE IN_LIST __allowedLanguages))
      message(FATAL_ERROR "Language '${ARG_LANGUAGE}' is not supported. Supported languages: ${__allowedLanguages}")
    endif()
    set(_srcLanguage ${ARG_LANGUAGE})
  endif()
  set(_incFilePaths ${ARG_FORCE_INCLUDE})
  set(_includeDirs ${ARG_INCLUDE_DIRECTORIES})
  set(_defines ${ARG_DEFINES})
  set(_dependencies ${ARG_DEPENDS})
  set(_useOpt ${ARG_OPTIMIZE})
  foreach(optLang IN LISTS _optionsMultiKw)
    set(_options_${optLang} ${OPTIONS_${optLang}})
  endforeach()

  get_filename_component(_outBcFileName "${_outBcFilePath}" NAME)
  get_filename_component(_outBcFileDir  "${_outBcFilePath}" PATH)

  # Grouping source files by language.
  foreach(_allowedLang ${__allowedLanguages})
    set("_srcFilePaths_${_allowedLang}")
  endforeach()
  if(_srcLanguage MATCHES "^DEFAULT$")
    foreach(_srcFilePath ${_srcFilePaths})
      string(REPLACE ";" "\;" _srcFilePath "${_srcFilePath}")
      if(_srcFilePath MATCHES "\\.[lL][lL]$")
        set(_srcFileLang "LL")
      elseif(_srcFilePath MATCHES "\\.[bB][cC]$")
        set(_srcFileLang "BC")
      else()
        set(_srcFileLang "CL")
      endif()
      list(APPEND "_srcFilePaths_${_srcFileLang}" "${_srcFilePath}")
    endforeach()
  else()
    set("_srcFilePaths_${_srcLanguage}" ${_srcFilePaths})
  endif()

  if(_pchFilePathSet)
    set(_pchFlags "-include-pch" "${_pchFilePath}")
  else()
    set(_pchFlags)
  endif()

  set(_incFileFlags)
  foreach(_incFilePath ${_incFilePaths})
    string(REPLACE ";" "\;" _incFilePath "${_incFilePath}")
    file(TO_CMAKE_PATH "${_incFilePath}" _incFilePath)
    list(APPEND _incFileFlags "-include" "${_incFilePath}")
  endforeach()

  set(_includeDirsFlags)
  foreach(_includeDir ${_includeDirs})
    string(REPLACE ";" "\;" _includeDir "${_includeDir}")
    list(APPEND _includeDirsFlags "-I" "${_includeDir}")
  endforeach()

  set(_defineFlags)
  foreach(_define ${_defines})
    string(REPLACE ";" "\;" _define "${_define}")
    list(APPEND _defineFlags "-D${_define}")
  endforeach()


  # Adding custom build commands for each language.
  set(_bcFiles)
  set(_bcFileId 0)
  foreach(_srcFilePath ${_srcFilePaths_CL})
    string(REPLACE ";" "\;" _srcFilePath "${_srcFilePath}")

    get_filename_component(_srcFileName      "${_srcFilePath}" NAME)
    get_filename_component(_srcFileNameWoExt "${_srcFilePath}" NAME_WE)
    set(_bcIntFilePath  "${_outBcFilePath}_${_srcFileNameWoExt}__cl__${_bcFileId}.bc")
    set(_bcTempFilePath "${_bcIntFilePath}.tmp")

    file(TO_CMAKE_PATH "${_bcIntFilePath}" _bcIntFilePath)
    file(TO_CMAKE_PATH "${_bcTempFilePath}" _bcTempFilePath)

    math(EXPR _bcFileId "${_bcFileId} + 1")

    # OpenCL source compilation is triggered by CLANG change, change of source files, change of precompiled header, change of
    # forcibly included headers or change of additional dependencies.

    add_custom_command(
        OUTPUT "${_bcTempFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "$<TARGET_FILE:clang-tool>" -cc1 ${IGC_LLVM_DEPENDENT_CLANG_FLAGS} -x cl -fblocks -fpreserve-vec3-type -opencl-builtins "-triple=${_archTriple}" -w -emit-llvm-bc -o "${_bcTempFilePath}" ${_pchFlags} ${_incFileFlags} ${_includeDirsFlags} ${_defineFlags} ${_options_DEFAULT} ${_options_CL} "${_srcFilePath}"
        DEPENDS clang-tool ${_pchFilePath} ${_incFilePaths} "${_srcFilePath}" ${_dependencies}
        COMMENT "BiF: \"${_outBcFileName}\": Compiling OpenCL source: \"${_srcFileName}\""
      )
    add_custom_command(
        OUTPUT "${_bcIntFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
        DEPENDS "${_bcTempFilePath}"
        COMMENT "BiF: \"${_outBcFileName}\": Updating intermediate .bc for \"${_srcFileName}\" if changed."
      )

    list(APPEND _bcFiles "${_bcIntFilePath}")
  endforeach()

  set(bif-llvm-as_exe ${LLVM_AS_EXE})
  set(bif-llvm-link_exe ${LLVM_LINK_EXE})
  set(bif-llvm-opt_exe ${LLVM_OPT_EXE})

  if(NOT (IGC_OPTION__LLVM_MODE STREQUAL PREBUILDS_MODE_NAME AND WIN32))
    if(NOT EXISTS ${LLVM_AS_EXE})
      set(bif-llvm-as_exe $<TARGET_FILE:${LLVM_AS_EXE}>)
    endif()
    if(NOT EXISTS ${LLVM_LINK_EXE})
      set(bif-llvm-link_exe $<TARGET_FILE:${LLVM_LINK_EXE}>)
    endif()
    if(NOT EXISTS ${LLVM_OPT_EXE})
      set(bif-llvm-opt_exe $<TARGET_FILE:${LLVM_OPT_EXE}>)
    endif()
  endif()

  foreach(_srcFilePath ${_srcFilePaths_LL})
    string(REPLACE ";" "\;" _srcFilePath "${_srcFilePath}")

    get_filename_component(_srcFileName      "${_srcFilePath}" NAME)
    get_filename_component(_srcFileNameWoExt "${_srcFilePath}" NAME_WE)
    set(_bcIntFilePath  "${_outBcFilePath}_${_srcFileNameWoExt}__ll__${_bcFileId}.bc")
    set(_bcTempFilePath "${_bcIntFilePath}.tmp")
    math(EXPR _bcFileId "${_bcFileId} + 1")

    # LLVM assembling is triggered by host LLVM package change (where tools are), change of source files or change of additional dependencies.
    # Makes sure that LLVM is unzipped before.
    add_custom_command(
        OUTPUT "${_bcTempFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${bif-llvm-as_exe}" -o "${_bcTempFilePath}" ${_options_DEFAULT} ${_options_LL} "${_srcFilePath}"
        DEPENDS "${_srcFilePath}" ${_dependencies}
        COMMENT "BiF: \"${_outBcFileName}\": Compiling OpenCL source: \"${_srcFileName}\""
      )
    add_custom_command(
        OUTPUT "${_bcIntFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
        DEPENDS "${_bcTempFilePath}"
        COMMENT "BiF: \"${_outBcFileName}\": Updating intermediate .bc for \"${_srcFileName}\" if changed."
      )

    list(APPEND _bcFiles "${_bcIntFilePath}")
  endforeach()

  foreach(_srcFilePath ${_srcFilePaths_BC})
    string(REPLACE ";" "\;" _srcFilePath "${_srcFilePath}")
    list(APPEND _bcFiles "${_srcFilePath}")
  endforeach()

  # Determining whether implicit link is required.
  list(LENGTH _bcFiles _bcFilesCount)
  if(_bcFilesCount GREATER 1)
    set(_bcIntFilePath  "${_outBcFilePath}__link__${_bcFileId}.bc")
    set(_bcTempFilePath "${_bcIntFilePath}.tmp")
    math(EXPR _bcFileId "${_bcFileId} + 1")

    # LLVM linking is triggered by host LLVM package change (where tools are), change of intermediate .bc files or change of additional dependencies.
    # Makes sure that LLVM is unzipped before.
    add_custom_command(
        OUTPUT "${_bcTempFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${bif-llvm-link_exe}" -o "${_bcTempFilePath}" ${_options_DEFAULT} ${_options_BC} ${_bcFiles}
        DEPENDS ${_bcFiles} ${_dependencies}
        COMMENT "BiF: \"${_outBcFileName}\": Linking intermediate .bc into output .bc."
      )
    add_custom_command(
        OUTPUT "${_bcIntFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
        DEPENDS "${_bcTempFilePath}"
        COMMENT "BiF: \"${_outBcFileName}\": Updating output .bc if changed (after linking)."
      )

    set(_bcFiles "${_bcIntFilePath}")
  elseif(_bcFilesCount LESS 1)
    return()
  endif()

  if(_useOpt)
    set(_bcIntFilePath  "${_outBcFilePath}__opt__${_bcFileId}.bc")
    set(_bcTempFilePath "${_bcIntFilePath}.tmp")

    # LLVM optmizer is triggered by host OPT change, change of .bc file to optimize or change of additional dependencies.
    # Makes sure that LLVM is unzipped before.
    add_custom_command(
        OUTPUT "${_bcTempFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${bif-llvm-opt_exe}" -O2 -o "${_bcTempFilePath}" ${IGC_LLVM_DEPENDENT_OPT_ARGS} ${_options_DEFAULT} ${_options_OPT} ${_bcFiles}
        DEPENDS ${_bcFiles} ${_dependencies}
        COMMENT "BiF: \"${_outBcFileName}\": Optmizing output .bc."
      )
    add_custom_command(
        OUTPUT "${_bcIntFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
        DEPENDS "${_bcTempFilePath}"
        COMMENT "BiF: \"${_outBcFileName}\": Updating output .bc if changed (after optimization)."
      )

    set(_bcFiles "${_bcIntFilePath}")
  endif()

  # Final copy.
  add_custom_command(
      OUTPUT "${_outBcFilePath}"
      COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${CMAKE_COMMAND}" -E copy_if_different ${_bcFiles} "${_outBcFilePath}"
      DEPENDS ${_bcFiles}
      COMMENT "BiF: \"${_outBcFileName}\": Copying output .bc."
    )
endfunction()

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

# ============================================ Building .bc =============================================

# All:
#     -cc1 -x cl -opencl-builtins -w -emit-llvm-bc
#     -include .\IGCBiFModule\opencl_cth.h
#     -I .\IGCBiFModule\shared -I .\IGCBiFModule\PointerSize
#     -triple=(spir or spir64 - consistent with PCH)
### -include-pch OCL_32                 -D__EXECUTION_MODEL_DEBUG=1 -D__OPENCL_C_VERSION__=200                                      .\IGCBiFModule\shared\IBiF_Impl.cl                   -o .\IBiF_Impl_int.bc
### -include-pch OCL_32 -cl-enable-half -D__EXECUTION_MODEL_DEBUG=1 -D__OPENCL_C_VERSION__=120 -Dcl_khr_fp16 -I                     .\IGCBiFModule\PointerSize\IBiF_size_t.cl            -o .\IGCsize_t_32.bc
### -include-pch OCL_64 -cl-enable-half -D__EXECUTION_MODEL_DEBUG=1 -D__OPENCL_C_VERSION__=120 -Dcl_khr_fp16 -I                     .\IGCBiFModule\PointerSize\IBiF_size_t.cl            -o .\IGCsize_t_64.bc

# Other:
#     %LLVM_LINK_EXE% .\IBiF_Impl_int.bc .\IGCBiF_Impl_ll.bc -o .\OCLBiFImpl.bc


file(GLOB_RECURSE _PRE_RELEASE_CL "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PreRelease/*.cl")


set(IGC_BUILD__BIF_OCL_SHARED_INC                "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/opencl_cth_released.h")
set(IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PreRelease/opencl_cth_pre_release.h")

set(_concatScript   "${IGC_SOURCE_DIR}/BiFModule/concat.py")
set(IGC_BUILD__BIF_OCL_FORCE_INC "${IGC_BUILD__BIF_DIR}/opencl_cth.h")
set(IGC_BUILD__BIF_OCL_INCLUDES ${IGC_BUILD__BIF_OCL_INCLUDES} ${IGC_BUILD__BIF_OCL_FORCE_INC})

add_custom_command(
    OUTPUT "${IGC_BUILD__BIF_OCL_FORCE_INC}"
    COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${CMAKE_COMMAND}" -E make_directory "${IGC_BUILD__BIF_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH} || "${PYTHON_EXECUTABLE}"
    ARGS ${_concatScript} -new ${IGC_BUILD__BIF_OCL_FORCE_INC} ${IGC_BUILD__BIF_OCL_SHARED_INC} ${IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE}
    DEPENDS ${_concatScript}
    DEPENDS ${IGC_BUILD__BIF_OCL_SHARED_INC}
    DEPENDS ${IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE}
    COMMENT "Running concat.py for autogenerating opencl_cth.h"
    )

set(IGC_BUILD__BIF_OCL_COMMON_INC_DIRS
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PointerSize"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/Raytracing"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Headers"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_shared"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_shared/device_enqueue"
  )

set(IGC_BUILD__BIF_SPIRV_COMMON_INC_DIRS
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Headers"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/device_enqueue"
  )

file(GLOB_RECURSE _MATH_SRC_BC      "${IGC_OPTION__BIF_SRC_OCL_DIR}/*.bc")

file(GLOB_RECURSE SVML_FILES "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/BinaryReleaseOnly/*.cl" )

set(FLAG "")
if(${VME_TYPES_DEFINED})
    list(APPEND FLAG "__VME_TYPES_DEFINED__")
endif(${VME_TYPES_DEFINED})
if(IGC_OPTION__USE_KHRONOS_SPIRV_TRANSLATOR_IN_SC)
    list(APPEND FLAG "__USE_KHRONOS_SPIRV_TRANSLATOR__")
endif(IGC_OPTION__USE_KHRONOS_SPIRV_TRANSLATOR_IN_SC)

igc_bif_find_cl_files(IGC_BUILD__BIF_OCL_COMMON_DEPENDS ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS} "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation")

# Adding BiFModuleCacheInit to start before any of the custom target attached to IGC_BUILD__PROJ__BiFModule_OCL
list(APPEND IGC_BUILD__BIF_OCL_COMMON_DEPENDS ${BiFModule_Init})

set(KHR_DEFINES "cl_khr_f16" "cl_khr_fp64" "cl_khr_gl_msaa_sharing" "cl_khr_mipmap_image" "cl_khr_depth_images" "cl_intel_subgroups_short"
                "cl_intel_subgroups_char" "cl_intel_subgroups_long" "cl_intel_subgroup_local_block_io" "cl_intel_64bit_global_atomics_placeholder"
                "cl_khr_subgroup_extended_types" "cl_khr_subgroup_non_uniform_vote" "cl_khr_subgroup_ballot" "cl_khr_subgroup_shuffle"
                "cl_khr_subgroup_shuffle_relative" "cl_khr_subgroup_non_uniform_arithmetic" "cl_khr_subgroup_clustered_reduce"
                "cl_khr_extended_bit_ops" "cl_intel_bit_instructions" "cl_intel_global_float_atomics")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_matrix_multiply_accumulate" "cl_intel_subgroup_split_matrix_multiply_accumulate")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_rt_production")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_matrix_multiply_accumulate_tf32")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_extended_block_read")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_pvc_lsc_validation")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_extended_block_read_cacheopts")

igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/IBiF_Impl.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )

igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/IBiF_Impl.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_SPIRV_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
  if ( NOT "${_PRE_RELEASE_CL}" STREQUAL "" )
   igc_bif_build_bc(
      OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_PreRelease_int.bc"
      TRIPLE               spir64
      SOURCES              ${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PreRelease/IBIF_PreRelease_Impl.cl
      FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
      INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
      DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} "cl_intel_device_side_avc_vme_enable" "cl_intel_device_side_avc_motion_estimation" ${FLAG}
      DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
      OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
     )
 endif()
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_32.bc"
    TRIPLE               spir
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/pointersize.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__32bit__=1" "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_64.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/pointersize.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_32_int.bc"
    TRIPLE               spir
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PointerSize/IBiF_size_t.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__32bit__=1" "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_64_int.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PointerSize/IBiF_size_t.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_32.bc"
    TRIPLE               spir
    SOURCES              "${IGC_BUILD__BIF_DIR}/IGCsize_t_32_int.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_32.bc"
    FORCE_INCLUDE         ${IGC_BUILD__BIF_OCL_INCLUDES}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS}
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_64.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_BUILD__BIF_DIR}/IGCsize_t_64_int.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_64.bc"
    FORCE_INCLUDE         ${IGC_BUILD__BIF_OCL_INCLUDES}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS}
  )

if ("${FLAG}" STREQUAL "")
    unset(FLAG)
endif()

# TODO:
# Math source( svml files) is not released as a part of open source bundle. Instead, we release .bc.
# We basically look for .bc and only if .bc is not present we build from source.
#Currently, not sure where exactly the .bc will be placed. But ones we have all
# the info, we need to make sure all the dependencies are handled. Like for example if any
# file uses svml files, we need to prototype the headers and during linking it gets resolved.

if ("${_PRE_RELEASE_CL}" STREQUAL "" AND ( NOT "${_MATH_SRC_BC}" STREQUAL "" ) )
  igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_Math_Src_spirv.bc"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES}
    OPTIONS CL           ${CL_OPTIONS}
  )
else()
  igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_PreRelease_int.bc"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES}
    OPTIONS CL           ${CL_OPTIONS}
  )
endif()

# =========================================== Custom targets ============================================

set(IGC_BUILD__PROJ__BiFModule_OCL       "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleOcl")
set(IGC_BUILD__PROJ__BiFModule_OCL       "${IGC_BUILD__PROJ__BiFModule_OCL}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__BiFModule_OCL "BiFModule-OCL")
set(IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL clang-tool "${IGC_BUILD__BIF_OCL_FORCE_INC}" "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc" "${IGC_BUILD__BIF_DIR}/IGCsize_t_32.bc" "${IGC_BUILD__BIF_DIR}/IGCsize_t_64.bc" "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc")
set(IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL "${IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL}" PARENT_SCOPE)


add_custom_target("${IGC_BUILD__PROJ__BiFModule_OCL}"
    DEPENDS ${IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL}
    SOURCES ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
  )
set_property(TARGET "${IGC_BUILD__PROJ__BiFModule_OCL}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__BiFModule_OCL}")

generate_bif_src_checksum(${IGC_BUILD__PROJ__BiFModule_OCL})

generate_bif_prebuild_pack(${IGC_BUILD__PROJ__BiFModule_OCL} "${IGC_BUILD__PROJ_DEPENDS_BiFModule_OCL}")
set(IGC_BUILD__PROJ__BiFModuleCache_OCL       "${IGC_BUILD__PROJ__BiFModuleCache_OCL}" PARENT_SCOPE)


# =========================================== SPIRV Headers ============================================

add_library(IGCSPIRVHeaders INTERFACE)
target_include_directories(IGCSPIRVHeaders
  INTERFACE
  ${IGC_OPTION__BIF_SRC_OCL_DIR}/Headers
  )

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================
