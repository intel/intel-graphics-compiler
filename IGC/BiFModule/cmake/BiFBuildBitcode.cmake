include(${CMAKE_CURRENT_LIST_DIR}/BiFMCConst.cmake)

string (REPLACE " " ";" IGC_BUILD__BIF_OCL_INCLUDES "${IGC_BUILD__BIF_OCL_INCLUDES}")
string (REPLACE " " ";" IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE "${IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE}")
string (REPLACE " " ";" CL_OPTIONS "${CL_OPTIONS}")


option(IGC_OPTION__ENABLE_BF16_BIF "Enables BF16 support in BiF (requires patched Clang)" ON)
# Clang patches needed for BF16 support can be found here (example for Clang 16):
# https://github.com/intel/opencl-clang/tree/ocl-open-160/patches/clang


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
#     [OPTIMIZE            useOpt]
#     [OPTIONS             lang [option [...]]]
#     [OPTIONS             lang [option [...]]]
#     [...]
#     [OPTIONS             lang [option [...]]]
#     [UPDATE_IR           precompiledIRPath]
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
#                            - "BC"      - Sources treated as LLVM Bitcode files (LLVM-LINK will be used).
#                            - "LL"      - Sources treated as LLVM assembly files (LLVM-AS will be used).
#                           If not specified, "DEFAULT" is used.
# @param pchFilePath        Includes precompiled header. Ignored for other files than "CL".
# @param incFilePath        Includes header for each compiled source. Ignored for other files than "CL".
# @param includeDir         Adds include directory. Ignored for other files than "CL".
# @param define             Additional preprocessor definition (multiple can be specified). Ignored for other files than "CL".
#                           build will not be put into optimizer).  Default is FALSE.
# @param lang               Language for which options are defined (currently "CL", "LL", "BC" and "OPT" for optimizer;
#                           "DEFAULT" will add options to all lanuages).
# @param option             Additional compilation option (multiple can be specified).
# @param precompiledIRPath  Path to precompiled IR file to be updated with newly built textual IR representation.
#
function(igc_bif_build_bc)
  set(_mainSingleKw
    OUTPUT
    TRIPLE
    LANGUAGE
    UPDATE_IR
    )
  set(_mainMultiKw
    SOURCES
    PRECOMPILED
    FORCE_INCLUDE
    INCLUDE_DIRECTORIES
    DEFINES
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
  foreach(optLang IN LISTS _optionsMultiKw)
    set(_options_${optLang} ${OPTIONS_${optLang}})
  endforeach()

  get_filename_component(_outBcFileName "${_outBcFilePath}" NAME)
  get_filename_component(_outBcFileDir  "${_outBcFilePath}" PATH)

  message("[IGC\\BiFModule] - Generating ${_outBcFileName}")

  # Grouping source files by language.
  foreach(_allowedLang ${__allowedLanguages})
    set("_srcFilePaths_${_allowedLang}")
  endforeach()
  if(_srcLanguage MATCHES "^DEFAULT$")
    foreach(_srcFilePath ${_srcFilePaths})
      string(REPLACE ";" "\;" _srcFilePath "${_srcFilePath}")
      if(_srcFilePath MATCHES "\\.[bB][cC]$")
        set(_srcFileLang "BC")
      elseif(_srcFilePath MATCHES "\\.[lL][lL]$")
        set(_srcFileLang "LL")
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
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
        COMMAND ${clang-tool} -cc1 ${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG} -x cl -fblocks -fpreserve-vec3-type -opencl-builtins "-triple=${_archTriple}" -w -emit-llvm-bc -discard-value-names -o "${_bcTempFilePath}" ${_pchFlags} ${_incFileFlags} ${_includeDirsFlags} ${_defineFlags} ${_options_DEFAULT} ${_options_CL} "${_srcFilePath}"
    COMMAND_ECHO STDOUT
      )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
    COMMAND_ECHO STDOUT
      )

    list(APPEND _bcFiles "${_bcIntFilePath}")
  endforeach()

  foreach(_srcFilePath ${_srcFilePaths_LL})
    string(REPLACE ";" "\;" _srcFilePath "${_srcFilePath}")

    get_filename_component(_srcFileName      "${_srcFilePath}" NAME)
    get_filename_component(_srcFileNameWoExt "${_srcFilePath}" NAME_WE)
    set(_bcIntFilePath  "${_outBcFilePath}_${_srcFileNameWoExt}__ll__${_bcFileId}.bc")
    set(_bcTempFilePath "${_bcIntFilePath}.tmp")

    file(TO_CMAKE_PATH "${_bcIntFilePath}" _bcIntFilePath)
    file(TO_CMAKE_PATH "${_bcTempFilePath}" _bcTempFilePath)

    math(EXPR _bcFileId "${_bcFileId} + 1")

    # LLVM assembly is triggered by host LLVM package change (where tools are), change of source files or change of additional dependencies.
    execute_process(
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
      COMMAND "${bif-llvm-as_exe}" ${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT} -o "${_bcTempFilePath}" ${_options_DEFAULT} ${_options_LL} "${_srcFilePath}"
      COMMAND_ECHO STDOUT
    )
    execute_process(
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
      COMMAND_ECHO STDOUT
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
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
        COMMAND "${bif-llvm-link_exe}" ${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT} -o "${_bcTempFilePath}" ${_options_DEFAULT} ${_options_BC} ${_bcFiles}
    COMMAND_ECHO STDOUT
      )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
    COMMAND_ECHO STDOUT
      )

    set(_bcFiles "${_bcIntFilePath}")
  elseif(_bcFilesCount LESS 1)
    return()
  endif()

  # Final copy.
  execute_process(
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${_bcFiles} "${_outBcFilePath}"
   COMMAND_ECHO STDOUT
    )

  # Updating precompiled IR file if requested. The update is only performed if
  # enabled explicitly by the IGC_OPTION__BIF_UPDATE_IR flag.
  if (ARG_UPDATE_IR)
    if (NOT IGC_OPTION__BIF_UPDATE_IR)
      message(STATUS "IGC_OPTION__BIF_UPDATE_IR is OFF - skipping updating precompiled IR file.")
      return()
    endif()

    if (NOT EXISTS "${bif-llvm-dis_exe}")
      message(WARNING "Cannot update precompiled IR file because llvm-dis tool is not available.")
      return()
    endif()

    set(_precompiledIRPath ${ARG_UPDATE_IR})
    execute_process(
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
      COMMAND_ECHO STDOUT
    )

    file(WRITE "${_precompiledIRPath}" "; This file is autogenerated - do not edit!\n")
    execute_process(
      COMMAND "${bif-llvm-dis_exe}" ${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT} -o - "${_outBcFilePath}"
      COMMAND grep -Ev "(ModuleID|source_filename)"
      OUTPUT_FILE "${_precompiledIRPath}"
      COMMAND_ECHO STDOUT
    )
  endif()
endfunction()

function(check_clang_tool RESNAME TEST_PROG)
  set(srcname "${IGC_BUILD__BIF_DIR}/${RESNAME}_test.c")
  set(bcname "${IGC_BUILD__BIF_DIR}/${RESNAME}_test.bc")
  file(WRITE "${srcname}" "${TEST_PROG}")
  execute_process(
    COMMAND ${clang-tool} -cc1 -x c -emit-llvm-bc -triple=spir64 -o "${bcname}" "${srcname}"
    RESULT_VARIABLE res
    OUTPUT_QUIET
    ERROR_QUIET
  )

  if(${res} EQUAL 0)
    set(${RESNAME} TRUE PARENT_SCOPE)
  else()
    set(${RESNAME} FALSE PARENT_SCOPE)
  endif()
endfunction()


execute_process( COMMAND ${CMAKE_COMMAND} -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH}
                 RESULT_VARIABLE compare_result
)

if( compare_result EQUAL 1)
message("[IGC\\BiFModuleCache] - Buidling from source start")

set(_concatScript   "${IGC_SOURCE_DIR}/BiFModule/concat.py")
set(IGC_BUILD__BIF_OCL_FORCE_INC "${IGC_BUILD__BIF_DIR}/opencl_cth.h")
list(APPEND IGC_BUILD__BIF_OCL_INCLUDES ${IGC_BUILD__BIF_OCL_FORCE_INC})

message("[IGC\\BiFModule] - Generating opencl_cth.h")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${IGC_BUILD__BIF_DIR}"
    COMMAND ${PYTHON_EXECUTABLE} ${_concatScript} -new ${IGC_BUILD__BIF_OCL_FORCE_INC} ${IGC_BUILD__BIF_OCL_SHARED_INC} ${IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE}
  COMMAND_ECHO STDOUT
    )


set(IGC_BUILD__BIF_OCL_COMMON_INC_DIRS
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PointerSize"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/Raytracing"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Headers"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/../AdaptorOCL/ocl_igc_shared"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/../AdaptorOCL/ocl_igc_shared/device_enqueue"
  )

set(IGC_BUILD__BIF_SPIRV_COMMON_INC_DIRS
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Headers"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/../AdaptorOCL/ocl_igc_interface/device_enqueue"
  )

file(GLOB_RECURSE _MATH_SRC_BC      "${IGC_OPTION__BIF_SRC_OCL_DIR}/*.bc")

file(GLOB_RECURSE SVML_FILES "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/BinaryReleaseOnly/*.cl" )

set(FLAG "")
if(${VME_TYPES_DEFINED} STREQUAL "TRUE")
    list(APPEND FLAG "__VME_TYPES_DEFINED__")
endif()

set(KHR_DEFINES "cl_khr_f16" "cl_khr_fp64" "cl_khr_gl_msaa_sharing" "cl_khr_mipmap_image" "cl_khr_depth_images" "cl_intel_subgroups_short"
                "cl_intel_subgroups_char" "cl_intel_subgroups_long" "cl_intel_subgroup_local_block_io" "cl_intel_64bit_global_atomics_placeholder"
                "cl_khr_subgroup_extended_types" "cl_khr_subgroup_non_uniform_vote" "cl_khr_subgroup_ballot" "cl_khr_subgroup_shuffle"
                "cl_khr_subgroup_shuffle_relative" "cl_khr_subgroup_non_uniform_arithmetic" "cl_khr_subgroup_clustered_reduce"
                "cl_khr_extended_bit_ops" "cl_intel_bit_instructions" "cl_intel_global_float_atomics" "cl_intel_subgroup_buffer_prefetch")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_matrix_multiply_accumulate" "cl_intel_subgroup_split_matrix_multiply_accumulate")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_rt_production")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_matrix_multiply_accumulate_tf32")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_extended_block_read")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_pvc_lsc_validation")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_extended_block_read_cacheopts")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_extended_block_write_cacheopts")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_2d_block_io")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_matrix_multiply_accumulate_bf8")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_subgroup_matrix_multiply_accumulate_fp8")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_concurrent_dispatch")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_stochastic_rounding")
set(KHR_DEFINES ${KHR_DEFINES} "cl_intel_bfloat16_atomics")

set(BFLOAT_SOURCE_ENABLED TRUE)
if(IGC_OPTION__ENABLE_BF16_BIF)
  set(KHR_DEFINES ${KHR_DEFINES} "IGC_SPV_INTEL_bfloat16_arithmetic")
  set(KHR_DEFINES ${KHR_DEFINES} "IGC_SPV_KHR_bfloat16")

  # Use source-code compilation for bfloat16 builtins if clang supports __bf16
  # type. Otherwise, use precompiled LLVM IR builtins.
  check_clang_tool(CLANG_SUPPORTS_BFLOAT "__bf16 test_var = 0.0;")

  if (CLANG_SUPPORTS_BFLOAT)
    message(STATUS "[IGC] BF16 extensions enabled (using patched Clang with __bf16 support)")
  else()
    message(WARNING "[IGC] BF16 extensions enabled, but Clang does not support __bf16 type. Using precompiled LLVM IR builtins.")
    set(BFLOAT_SOURCE_ENABLED FALSE)
  endif()
else(IGC_OPTION__ENABLE_BF16_BIF)
  message(STATUS "[IGC] BF16 extensions disabled (compatible with upstream Clang)")
endif()

set(BFLOAT_BUILTINS_CL_PATH "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/bfloat/IBiF_BFloat.cl")
set(BFLOAT_BUILTINS_LL_PATH "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/bfloat/IBiF_BFloat.ll")
set(BFLOAT_BUILTINS_BC_PATH "${IGC_BUILD__BIF_DIR}/IBiF_BFloat.bc")
if(BFLOAT_SOURCE_ENABLED)
  igc_bif_build_bc(
    OUTPUT               ${BFLOAT_BUILTINS_BC_PATH}
    TRIPLE               spir64
    SOURCES              ${BFLOAT_BUILTINS_CL_PATH}
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_SPIRV_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
    UPDATE_IR            ${BFLOAT_BUILTINS_LL_PATH}
  )
else()
  igc_bif_build_bc(
    OUTPUT  ${BFLOAT_BUILTINS_BC_PATH}
    TRIPLE  spir64
    SOURCES ${BFLOAT_BUILTINS_LL_PATH}
  )
endif()

igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/IBiF_Impl.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )

igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/IBiF_Impl.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_SPIRV_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
  if ( NOT "${_PRE_RELEASE_CL}" STREQUAL "" )
   igc_bif_build_bc(
      OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_PreRelease_int.bc"
      TRIPLE               spir64
      SOURCES              ${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PreRelease/IBIF_PreRelease_Impl.cl
      FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
      INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS} ${BIF_BINARY_DIR}/Languages/OpenCL/PreRelease/Matrix
      DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} "cl_intel_device_side_avc_vme_enable" "cl_intel_device_side_avc_motion_estimation" ${FLAG}
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
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_64.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/pointersize.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_32_int.bc"
    TRIPLE               spir
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PointerSize/IBiF_size_t.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__32bit__=1" "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_64_int.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PointerSize/IBiF_size_t.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_32.bc"
    TRIPLE               spir
    SOURCES              "${IGC_BUILD__BIF_DIR}/IGCsize_t_32_int.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_32.bc"
    FORCE_INCLUDE         ${IGC_BUILD__BIF_OCL_INCLUDES}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES} ${FLAG}
    OPTIONS CL           ${CL_OPTIONS}
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_64.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_BUILD__BIF_DIR}/IGCsize_t_64_int.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_64.bc"
    FORCE_INCLUDE         ${IGC_BUILD__BIF_OCL_INCLUDES}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES} ${FLAG}
    OPTIONS CL           ${CL_OPTIONS}
  )

if ("${FLAG}" STREQUAL "")
    unset(FLAG)
endif()


igc_bif_build_bc(
  OUTPUT               "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc"
  TRIPLE               spir64
  SOURCES              "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int.bc"
                       "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc"
                       "${IGC_BUILD__BIF_DIR}/IBiF_PreRelease_int.bc"
                       ${BFLOAT_BUILTINS_BC_PATH}
  FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
  DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES}
  OPTIONS CL           ${CL_OPTIONS}
)

execute_process(
    COMMAND ${BiFManager-bin} "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc" "${IGC_BUILD__BIF_DIR}/IGCsize_t_32.bc" "${IGC_BUILD__BIF_DIR}/IGCsize_t_64.bc" "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bifbc" "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.h"
  COMMAND_ECHO STDOUT
)
message("[IGC\\BiFModuleCache] - Buidling from source end")
elseif (compare_result EQUAL 0)
message("[IGC\\BiFModuleCache] - Reusing the BiFCache start")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E remove_directory ${IGC_BUILD__BIF_DIR})
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy_directory ${BiFModule_PreBuild_PATH} ${IGC_BUILD__BIF_DIR})
message("[IGC\\BiFModuleCache] Copied BiFModuleCache to ${IGC_BUILD__BIF_DIR}")
message("[IGC\\BiFModuleCache] - Reusing the BiFCache end")
else()
    message("Error while comparing the files.")
endif()
