#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

string(REPLACE "$(Configuration)" "" IGC_BUILD__BIF_ROOT_DIR ${IGC_BUILD__BIF_DIR})
message("[IGC\\BiFModuleCache] - IGC_BUILD__BIF_ROOT_DIR: ${IGC_BUILD__BIF_ROOT_DIR}")

set(IGC_BIF_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(IGC_BIF_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/BiFBuildConfig-$<CONFIG>.cmake")
include(BiFMCConst)
include(BiFMCGetListFiles)
get_bif_src_list("${opencl-header}" "${BiFModule_SRC}" BiFModule_SRC_LIST)

if(NOT EXISTS ${IGC_BUILD__BIF_ROOT_DIR})
    file(MAKE_DIRECTORY ${IGC_BUILD__BIF_ROOT_DIR})
endif()

set(IGC_BIF_OUTPUTS)
foreach(output IN LISTS BiFModule_PreBuild_FileList)
  if(NOT output STREQUAL "${BiFModule_CHECKSUM_FILE}")
    list(APPEND IGC_BIF_OUTPUTS "${IGC_BUILD__BIF_DIR}/${output}")
  endif()
endforeach()

function(igc_bif_tool_path output tool)
  if(TARGET "${tool}")
    set("${output}" "$<TARGET_FILE:${tool}>" PARENT_SCOPE)
  else()
    set("${output}" "${tool}" PARENT_SCOPE)
  endif()
endfunction()

function(build_bif_bitcode)
  igc_bif_tool_path(bif-llvm-as_exe "${LLVM_AS_EXE}")
  igc_bif_tool_path(bif-llvm-link_exe "${LLVM_LINK_EXE}")
  igc_bif_tool_path(bif-llvm-opt_exe "${LLVM_OPT_EXE}")
  igc_bif_tool_path(clang-tool clang-tool)
  set(BiFManager-bin "$<TARGET_FILE:${IGC_BUILD__PROJ_NAME_PREFIX}BiFManager-bin>")
  set(BIF_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  set(BIF_ARCH "${IGC_OPTION__ARCHITECTURE_TARGET}")
  set(BiFModuleCacheTarget "$<IF:$<CONFIG:Release>,Release,Non-Release>")
  if(IGC_OPTION__BIF_UPDATE_IR)
    igc_bif_tool_path(bif-llvm-dis_exe "${LLVM_DIS_EXE}")
  endif()
  set(configVariables
      IGC_BUILD__BIF_ROOT_DIR IGC_BUILD__BIF_DIR
      IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT
      bif-llvm-as_exe bif-llvm-dis_exe bif-llvm-link_exe bif-llvm-opt_exe clang-tool BiFManager-bin
      IGC_BUILD__BIF_OCL_SHARED_INC IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE
      IGC_OPTION__BIF_SRC_OCL_DIR IGC_OPTION__ENABLE_BF16_BIF IGC_OPTION__BIF_UPDATE_IR
      _PRE_RELEASE_CL IGC_SOURCE_DIR CL_OPTIONS IGC_BUILD__BIF_OCL_INCLUDES PYTHON_EXECUTABLE
      BIF_BINARY_DIR LLVM_VERSION_MAJOR opencl-header BIF_ARCH BiFModuleCacheTarget)
  set(config)
  foreach(variable IN LISTS configVariables)
    string(APPEND config "set(${variable} [==[${${variable}}]==])\n")
  endforeach()
  # Paths carry the generator's per-config placeholder; the scripts run
  # outside the build tool, so resolve it here.
  foreach(placeholder "$(Configuration)" "\${CONFIGURATION}")
    string(REPLACE "${placeholder}" "$<CONFIG>" config "${config}")
  endforeach()
  file(GENERATE OUTPUT "${IGC_BIF_CONFIG}" CONTENT "${config}")

  add_custom_command(
      OUTPUT ${IGC_BIF_OUTPUTS}
      COMMAND "${CMAKE_COMMAND}" "-DIGC_BIF_CONFIG=${IGC_BIF_CONFIG}"
          -P "${IGC_BIF_CMAKE_DIR}/BiFBuildBitcode.cmake"
      DEPENDS "${BiFModule_SRC_SHA_PATH}" "${IGC_BIF_CONFIG}" "${BiFManager-bin}"
          "${IGC_BUILD__PROJ__IBiF_matrix_generator}"
      COMMENT "Building BiF package"
      VERBATIM)
  set(target "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleCache")
  add_custom_target("${target}" DEPENDS ${IGC_BIF_OUTPUTS})
  set_target_properties("${target}" PROPERTIES FOLDER "Misc/BiF")
  add_dependencies("${target}" "${IGC_BUILD__PROJ_NAME_PREFIX}BiFManager-bin")
  set(IGC_BUILD__PROJ__BiFModuleCache_OCL "${target}" PARENT_SCOPE)
endfunction()

function(generate_bif_src_checksum bifModuleTgt)
  add_custom_command(
      OUTPUT "${BiFModule_SRC_SHA_PATH}"
      COMMAND "${CMAKE_COMMAND}" "-DIGC_BIF_CONFIG=${IGC_BIF_CONFIG}"
          -P "${IGC_BIF_CMAKE_DIR}/BiFMCChecksum.cmake"
      DEPENDS ${BiFModule_SRC_LIST} "${IGC_BIF_CONFIG}"
      COMMENT "Updating BiF source checksum"
      VERBATIM)
  set(target "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleCache_SRC_checksum")
  add_custom_target("${target}" DEPENDS "${BiFModule_SRC_SHA_PATH}")
  set_target_properties("${target}" PROPERTIES FOLDER "Misc/BiF")
  add_dependencies("${bifModuleTgt}" "${target}")
  set(IGC_BUILD__PROJ__BiFModuleCache_SRC_CHECKSUM_OCL "${target}" PARENT_SCOPE)
endfunction()

function(generate_bif_prebuild_pack bifModuleTgt)
  add_custom_command(
      OUTPUT "${BiFModule_PREBUILD_SHA_PATH}"
      COMMAND "${CMAKE_COMMAND}" "-DIGC_BIF_CONFIG=${IGC_BIF_CONFIG}"
          -P "${IGC_BIF_CMAKE_DIR}/BiFMCBuild.cmake"
      DEPENDS "${BiFModule_SRC_SHA_PATH}" ${IGC_BIF_OUTPUTS}
      COMMENT "Caching BiF package"
      VERBATIM)
  set(target "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleCacheBuildPack")
  add_custom_target("${target}" DEPENDS "${bifModuleTgt}" "${BiFModule_PREBUILD_SHA_PATH}")
  set_target_properties("${target}" PROPERTIES FOLDER "Misc/BiF")
  set(IGC_BUILD__PROJ__BiFModuleCacheBuildPack_OCL "${target}" PARENT_SCOPE)
endfunction()
