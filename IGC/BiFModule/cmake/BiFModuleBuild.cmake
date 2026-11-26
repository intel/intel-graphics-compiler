#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Commands which build the BiF package, see BiFManager/README.md. They do
# nothing when the package was restored from the prebuild cache (BiFMCRun.cmake).

include(BiFSources)

set(IGC_BIF_KHR_DEFINES "cl_khr_f16" "cl_khr_fp64" "cl_khr_gl_msaa_sharing" "cl_khr_mipmap_image" "cl_khr_depth_images" "cl_intel_subgroups_short"
                "cl_intel_subgroups_char" "cl_intel_subgroups_long" "cl_intel_subgroup_local_block_io" "cl_intel_64bit_global_atomics_placeholder"
                "cl_khr_subgroup_extended_types" "cl_khr_subgroup_non_uniform_vote" "cl_khr_subgroup_ballot" "cl_khr_subgroup_shuffle"
                "cl_khr_subgroup_shuffle_relative" "cl_khr_subgroup_non_uniform_arithmetic" "cl_khr_subgroup_clustered_reduce"
                "cl_khr_extended_bit_ops" "cl_intel_bit_instructions" "cl_intel_global_float_atomics" "cl_intel_subgroup_buffer_prefetch" "cl_khr_kernel_clock"
                "cl_khr_subgroup_rotate")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_subgroup_matrix_multiply_accumulate" "cl_intel_subgroup_split_matrix_multiply_accumulate")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_rt_production")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_subgroup_matrix_multiply_accumulate_tf32")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_subgroup_extended_block_read")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_pvc_lsc_validation")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_subgroup_extended_block_read_cacheopts")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_subgroup_extended_block_write_cacheopts")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_subgroup_2d_block_io")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_subgroup_matrix_multiply_accumulate_bf8")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_subgroup_matrix_multiply_accumulate_fp8")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_concurrent_dispatch")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_stochastic_rounding")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_bfloat16_atomics")
set(IGC_BIF_KHR_DEFINES ${IGC_BIF_KHR_DEFINES} "cl_intel_sigmoid")

# Whether clang compiles the bfloat16 sources: an imported clang is asked
# whether it supports the __bf16 type; a clang built here is the patched one.
function(igc_bif_clang_supports_bfloat result)
  get_target_property(imported clang-tool IMPORTED)
  if(NOT imported)
    set(${result} TRUE PARENT_SCOPE)
    return()
  endif()
  get_target_property(clang clang-tool IMPORTED_LOCATION)
  set(src "${CMAKE_CURRENT_BINARY_DIR}/bif_bfloat_test.c")
  file(WRITE "${src}" "__bf16 test_var = 0.0;")
  execute_process(
      COMMAND "${clang}" -cc1 -x c -emit-llvm-bc -triple=spir64 -o "${src}.bc" "${src}"
      RESULT_VARIABLE res OUTPUT_QUIET ERROR_QUIET)
  if(res EQUAL 0)
    set(${result} TRUE PARENT_SCOPE)
  else()
    set(${result} FALSE PARENT_SCOPE)
  endif()
endfunction()

# Adds the commands compiling the sources of a group and appends the modules
# to the list in out_var.
#   GROUP: name of the group, the directory of its modules
#   SOURCES: sources relative to IGC_OPTION__BIF_SRC_OCL_DIR
#   TRIPLE, HEADER, INCLUDE_DIRECTORIES, DEFINES, DEPENDS
function(igc_bif_add_group out_var)
  cmake_parse_arguments(ARG "" "GROUP;TRIPLE;HEADER" "SOURCES;INCLUDE_DIRECTORIES;DEFINES;DEPENDS" ${ARGN})
  set(flags -cc1 ${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG} -x cl -fblocks)
  if(LLVM_VERSION_MAJOR LESS 20)
    list(APPEND flags -fpreserve-vec3-type)
  endif()
  list(APPEND flags -opencl-builtins "-triple=${ARG_TRIPLE}" -w -emit-llvm-bc -discard-value-names)
  foreach(include IN LISTS IGC_BUILD__BIF_OCL_INCLUDES ARG_HEADER)
    list(APPEND flags -include "${include}")
  endforeach()
  foreach(dir IN LISTS ARG_INCLUDE_DIRECTORIES)
    list(APPEND flags -I "${dir}")
  endforeach()
  foreach(define IN LISTS ARG_DEFINES)
    list(APPEND flags "-D${define}")
  endforeach()
  string(REPLACE " " ";" cl_options "${CL_OPTIONS}")
  list(APPEND flags ${cl_options} -cl-std=CL2.0)

  set(modules ${${out_var}})
  foreach(src IN LISTS ARG_SOURCES)
    set(out "${IGC_BIF_MODULE_DIR}/${ARG_GROUP}/${src}.bc")
    get_filename_component(out_dir "${out}" DIRECTORY)
    add_custom_command(
        OUTPUT "${out}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${out_dir}"
        COMMAND "${CMAKE_COMMAND}" "-DIGC_BIF_STATE=${IGC_BIF_STATE}" "-DIGC_BIF_CREATE=${out}"
            -P "${IGC_BIF_CMAKE_DIR}/BiFMCRun.cmake" --
            "${clang-tool}" ${flags} -o "${out}" "${IGC_OPTION__BIF_SRC_OCL_DIR}/${src}"
        DEPENDS "${IGC_OPTION__BIF_SRC_OCL_DIR}/${src}" "${IGC_BIF_STATE}" ${IGC_BIF_HEADERS} ${ARG_DEPENDS} clang-tool
        COMMENT "BiF: compiling ${src}"
        VERBATIM)
    list(APPEND modules "${out}")
  endforeach()
  set(${out_var} ${modules} PARENT_SCOPE)
endfunction()

# Writes a list of modules for BiFManager-bin, one path per line, to
# BiFModules_<name>-<config>.txt next to the build config file.
function(igc_bif_write_list name)
  string(REPLACE ";" "\n" content "${ARGN}")
  foreach(placeholder "$(Configuration)" "\${CONFIGURATION}")
    string(REPLACE "${placeholder}" "$<CONFIG>" content "${content}")
  endforeach()
  file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/BiFModules_${name}-$<CONFIG>.txt" CONTENT "${content}\n")
endfunction()

# Adds the commands building the package. Expects the variables of
# build_bif_bitcode(): the tools, IGC_BIF_STATE and IGC_BIF_STAMP.
function(igc_bif_add_package_commands)
  set(src_dir "${IGC_OPTION__BIF_SRC_OCL_DIR}")
  set(defines "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${IGC_BIF_KHR_DEFINES})
  set(ocl_dirs "${src_dir}/Languages/OpenCL" "${src_dir}/Languages/OpenCL/PointerSize" "${src_dir}/Languages/OpenCL/Raytracing"
               "${src_dir}/Headers" "${src_dir}/../AdaptorOCL/ocl_igc_shared" "${src_dir}/../AdaptorOCL/ocl_igc_shared/device_enqueue")
  set(spirv_dirs "${src_dir}/Implementation" "${src_dir}/Headers" "${src_dir}/../AdaptorOCL/ocl_igc_interface/device_enqueue")
  set(ocl_header "${src_dir}/Languages/OpenCL/IBiF_Header.h")
  set(spirv_header "${src_dir}/Implementation/IBiF_Header.h")
  set(bfloat_header "${src_dir}/Implementation/bfloat/IBiF_BFloat_Header.h")

  file(GLOB_RECURSE IGC_BIF_HEADERS CONFIGURE_DEPENDS "${src_dir}/*.h")
  list(APPEND IGC_BIF_HEADERS ${IGC_BUILD__BIF_OCL_INCLUDES})

  set(bfloat_source FALSE)
  if(IGC_OPTION__ENABLE_BF16_BIF)
    list(APPEND defines "IGC_SPV_EXT_bfloat16_arithmetic" "IGC_SPV_KHR_bfloat16")
    igc_bif_clang_supports_bfloat(bfloat_source)
    if(bfloat_source)
      message(STATUS "[IGC] BF16 extensions enabled (using patched Clang with __bf16 support)")
    else()
      message(WARNING "[IGC] BF16 extensions enabled, but Clang does not support __bf16 type. Using precompiled LLVM IR builtins.")
    endif()
  else()
    message(STATUS "[IGC] BF16 extensions disabled (compatible with upstream Clang)")
  endif()

  set(main)
  igc_bif_add_group(main GROUP ocl TRIPLE spir64 SOURCES ${IGC_BIF_OCL_SOURCES}
      HEADER "${ocl_header}" INCLUDE_DIRECTORIES ${ocl_dirs} DEFINES ${defines})
  igc_bif_add_group(main GROUP spirv TRIPLE spir64 SOURCES ${IGC_BIF_SPIRV_SOURCES}
      HEADER "${spirv_header}" INCLUDE_DIRECTORIES ${spirv_dirs} DEFINES ${defines})
  if(NOT "${_PRE_RELEASE_CL}" STREQUAL "")
    igc_bif_add_group(main GROUP prerelease TRIPLE spir64 SOURCES ${IGC_BIF_PRERELEASE_SOURCES}
        HEADER "${ocl_header}" INCLUDE_DIRECTORIES ${ocl_dirs} "${CMAKE_CURRENT_BINARY_DIR}/Languages/OpenCL/PreRelease/Matrix"
        DEFINES ${defines} DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Languages/OpenCL/PreRelease/Matrix/IBiF_matrix_generated.h")
  endif()
  set(bfloat_modules)
  if(IGC_OPTION__ENABLE_BF16_BIF AND bfloat_source)
    igc_bif_add_group(bfloat_modules GROUP bfloat TRIPLE spir64 SOURCES ${IGC_BIF_BFLOAT_SOURCES}
        HEADER "${bfloat_header}" INCLUDE_DIRECTORIES ${spirv_dirs} DEFINES ${defines})
  elseif(IGC_OPTION__ENABLE_BF16_BIF)
    # The precompiled bfloat16 built-ins are a single module.
    set(ll "${src_dir}/Implementation/bfloat/IBiF_BFloat.ll")
    set(out "${IGC_BIF_MODULE_DIR}/bfloat/IBiF_BFloat.bc")
    add_custom_command(
        OUTPUT "${out}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${IGC_BIF_MODULE_DIR}/bfloat"
        COMMAND "${CMAKE_COMMAND}" "-DIGC_BIF_STATE=${IGC_BIF_STATE}" "-DIGC_BIF_CREATE=${out}"
            -P "${IGC_BIF_CMAKE_DIR}/BiFMCRun.cmake" --
            "${bif-llvm-as_exe}" ${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT} -o "${out}" "${ll}"
        DEPENDS "${ll}" "${IGC_BIF_STATE}" ${IGC_BIF_LLVM_AS_DEP}
        COMMENT "BiF: assembling IBiF_BFloat.ll"
        VERBATIM)
    set(bfloat_modules "${out}")
  endif()
  list(APPEND main ${bfloat_modules})

  set(size32)
  set(size64)
  foreach(bits 32 64)
    set(size_defines ${defines})
    set(triple spir64)
    if(bits EQUAL 32)
      set(size_defines "__32bit__=1" ${defines})
      set(triple spir)
    endif()
    igc_bif_add_group(size${bits} GROUP spirv_size_t_${bits} TRIPLE ${triple} SOURCES ${IGC_BIF_SPIRV_SIZE_T_SOURCES}
        HEADER "${spirv_header}" INCLUDE_DIRECTORIES ${ocl_dirs} DEFINES ${size_defines})
    igc_bif_add_group(size${bits} GROUP ocl_size_t_${bits} TRIPLE ${triple} SOURCES ${IGC_BIF_OCL_SIZE_T_SOURCES}
        HEADER "${ocl_header}" INCLUDE_DIRECTORIES ${ocl_dirs} DEFINES ${size_defines})
  endforeach()

  igc_bif_write_list(main ${main})
  igc_bif_write_list(size32 ${size32})
  igc_bif_write_list(size64 ${size64})
  set(all_modules ${main} ${size32} ${size64})
  set(lists "${CMAKE_CURRENT_BINARY_DIR}/BiFModules_")

  set(run "${CMAKE_COMMAND}" "-DIGC_BIF_STATE=${IGC_BIF_STATE}")
  set(symbols "${IGC_BIF_MODULE_DIR}/BiFSymbols.json")
  add_custom_command(
      OUTPUT "${symbols}"
      COMMAND ${run} "-DIGC_BIF_CREATE=${symbols}" -P "${IGC_BIF_CMAKE_DIR}/BiFMCRun.cmake" --
          "${BiFManager-bin}" -symbols -main "${lists}main-$<CONFIG>.txt"
          -size32 "${lists}size32-$<CONFIG>.txt" -size64 "${lists}size64-$<CONFIG>.txt"
          -o "${symbols}"
      DEPENDS ${all_modules} "${lists}main-$<CONFIG>.txt" "${lists}size32-$<CONFIG>.txt" "${lists}size64-$<CONFIG>.txt"
          "${IGC_BIF_STATE}" ${bif-tool-deps}
      COMMENT "BiF: collecting the symbols of the modules"
      VERBATIM)

  set(header "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.h")
  set(order "${IGC_BIF_MODULE_DIR}/BiFModules_order.txt")
  set(shared "${IGC_BIF_MODULE_DIR}/BiFSharedGlobals.txt")
  add_custom_command(
      OUTPUT "${header}" "${order}" "${shared}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${IGC_BUILD__BIF_DIR}"
      COMMAND ${run} "-DIGC_BIF_CREATE=${order}|${shared}" "-DIGC_BIF_KEEP=${header}"
          -P "${IGC_BIF_CMAKE_DIR}/BiFMCRun.cmake" --
          "${PYTHON_EXECUTABLE}" "${IGC_SOURCE_DIR}/BiFModule/depgraph.py" "${symbols}"
          --header "${header}" --order "${order}" --shared-globals "${shared}"
      DEPENDS "${symbols}" "${IGC_SOURCE_DIR}/BiFModule/depgraph.py" "${IGC_BIF_STATE}"
      COMMENT "BiF: building the dependency graph of the modules"
      VERBATIM)

  set(package "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bifbc")
  add_custom_command(
      OUTPUT "${package}"
      COMMAND ${run} "-DIGC_BIF_KEEP=${package}" -P "${IGC_BIF_CMAKE_DIR}/BiFMCRun.cmake" --
          "${BiFManager-bin}" -pack "${order}" -shared-globals "${shared}" -o "${package}"
      DEPENDS "${order}" "${shared}" ${all_modules} "${IGC_BIF_STATE}" ${bif-tool-deps}
      COMMENT "BiF: packing the modules"
      VERBATIM)

  set(cth "${IGC_BUILD__BIF_DIR}/opencl_cth.h")
  add_custom_command(
      OUTPUT "${cth}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${IGC_BUILD__BIF_DIR}"
      COMMAND ${run} "-DIGC_BIF_KEEP=${cth}" -P "${IGC_BIF_CMAKE_DIR}/BiFMCRun.cmake" --
          "${PYTHON_EXECUTABLE}" "${IGC_SOURCE_DIR}/BiFModule/concat.py" -new "${cth}"
          ${IGC_BUILD__BIF_OCL_SHARED_INC} ${IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE}
      DEPENDS ${IGC_BUILD__BIF_OCL_SHARED_INC} ${IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE}
          "${IGC_SOURCE_DIR}/BiFModule/concat.py" "${IGC_BIF_STATE}"
      COMMENT "BiF: generating opencl_cth.h"
      VERBATIM)

  if(IGC_OPTION__BIF_UPDATE_IR AND bfloat_modules AND bfloat_source)
    # Regenerates the precompiled bfloat16 built-ins from the sources.
    set(stamp "${IGC_BIF_MODULE_DIR}/bfloat_update_ir.stamp")
    add_custom_command(
        OUTPUT "${stamp}"
        COMMAND "${CMAKE_COMMAND}" "-DLLVM_LINK=${bif-llvm-link_exe}" "-DLLVM_DIS=${bif-llvm-dis_exe}"
            "-DOPAQUE_ARG=${IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT}" "-DOUTPUT=${src_dir}/Implementation/bfloat/IBiF_BFloat.ll"
            "-DSTAMP=${stamp}" -P "${IGC_BIF_CMAKE_DIR}/BiFMCUpdateIR.cmake" -- ${bfloat_modules}
        DEPENDS ${bfloat_modules}
        COMMENT "BiF: updating IBiF_BFloat.ll"
        VERBATIM)
    set(IGC_BIF_UPDATE_IR_STAMP "${stamp}" PARENT_SCOPE)
  endif()
endfunction()
