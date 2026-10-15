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
    set(path "$<TARGET_FILE:${tool}>")
    set(dep "${tool}")
  else()
    set(path "${tool}")
    if(NOT IS_ABSOLUTE "${tool}")
      find_program("IGC_BIF_TOOL_${tool}" "${tool}" HINTS "${LLVM_TOOLS_BINARY_DIR}")
      if(NOT IGC_BIF_TOOL_${tool})
        message(FATAL_ERROR "[IGC\\BiFModuleCache] - BiF tool not found: ${tool}")
      endif()
      set(path "${IGC_BIF_TOOL_${tool}}")
    endif()
    set(dep "${path}")
  endif()
  set("${output}" "${path}" PARENT_SCOPE)
  set(bif-tool-deps ${bif-tool-deps} "${dep}" PARENT_SCOPE)
endfunction()

function(build_bif_bitcode)
  set(bif-tool-deps "${IGC_BUILD__PROJ_NAME_PREFIX}BiFManager-bin")
  igc_bif_tool_path(bif-llvm-as_exe "${LLVM_AS_EXE}")
  igc_bif_tool_path(bif-llvm-link_exe "${LLVM_LINK_EXE}")
  igc_bif_tool_path(bif-llvm-opt_exe "${LLVM_OPT_EXE}")
  igc_bif_tool_path(clang-tool clang-tool)
  # Built binaries embed workspace paths and LLVM may be built or prebuilt
  # depending on the config, so the cache key names tools by version and
  # BiFManager sources; only an imported clang is hashed by content.
  set(IGC_BIF_TOOLS)
  get_target_property(clang-tool-imported clang-tool IMPORTED)
  if(clang-tool-imported)
    set(IGC_BIF_TOOLS "${clang-tool}")
  endif()
  set(BiFManager-bin "$<TARGET_FILE:${IGC_BUILD__PROJ_NAME_PREFIX}BiFManager-bin>")
  set(BIF_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  set(BIF_ARCH "${IGC_OPTION__ARCHITECTURE_TARGET}")
  set(BiFModuleCacheTarget "$<IF:$<CONFIG:Release>,Release,Non-Release>")
  file(GLOB IGC_BIF_TOOL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../BiFManager/*.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../BiFManager/*.hpp")
  if(IGC_OPTION__BIF_UPDATE_IR)
    igc_bif_tool_path(bif-llvm-dis_exe "${LLVM_DIS_EXE}")
  endif()
  # Values that change the produced bitcode. Paths are deliberately absent so
  # the checksum is shareable across workspaces and machines.
  set(IGC_BIF_HASHED_OPTIONS
      IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT
      IGC_OPTION__ENABLE_BF16_BIF CL_OPTIONS LLVM_VERSION_MAJOR LLVM_VERSION_MINOR LLVM_VERSION_PATCH)
  set(configVariables
      IGC_BUILD__BIF_ROOT_DIR IGC_BUILD__BIF_DIR
      IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_CLANG IGC_BUILD__OPAQUE_POINTERS_DEFAULT_ARG_OPT
      bif-llvm-as_exe bif-llvm-dis_exe bif-llvm-link_exe bif-llvm-opt_exe clang-tool BiFManager-bin
      IGC_BUILD__BIF_OCL_SHARED_INC IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE
      IGC_OPTION__BIF_SRC_OCL_DIR IGC_OPTION__ENABLE_BF16_BIF IGC_OPTION__BIF_UPDATE_IR
      _PRE_RELEASE_CL IGC_SOURCE_DIR CL_OPTIONS IGC_BUILD__BIF_OCL_INCLUDES PYTHON_EXECUTABLE
      BIF_BINARY_DIR LLVM_VERSION_MAJOR LLVM_VERSION_MINOR LLVM_VERSION_PATCH opencl-header BIF_ARCH BiFModuleCacheTarget IGC_BIF_TOOLS IGC_BIF_TOOL_SOURCES IGC_BIF_HASHED_OPTIONS)
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
  set(IGC_BIF_TOOLS "${IGC_BIF_TOOLS}" PARENT_SCOPE)
  set(IGC_BIF_TOOL_SOURCES "${IGC_BIF_TOOL_SOURCES}" PARENT_SCOPE)
  set(bif-tool-deps "${bif-tool-deps}" PARENT_SCOPE)

  add_custom_command(
      OUTPUT ${IGC_BIF_OUTPUTS}
      COMMAND "${CMAKE_COMMAND}" "-DIGC_BIF_CONFIG=${IGC_BIF_CONFIG}"
          -P "${IGC_BIF_CMAKE_DIR}/BiFBuildBitcode.cmake"
      DEPENDS "${BiFModule_SRC_SHA_PATH}" "${IGC_BIF_CONFIG}" ${bif-tool-deps}
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
      DEPENDS ${BiFModule_SRC_LIST} "${IGC_BIF_CONFIG}" ${bif-tool-deps}
          ${IGC_BIF_TOOL_SOURCES}
      COMMENT "Updating BiF source and tool checksum"
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
