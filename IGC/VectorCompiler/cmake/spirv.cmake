#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Creates `target_branch` starting at the `base_revision` in the `repo_dir`.
# Then all patches from the `patches_dir` are committed to the `target_branch`.
# Does nothing if the `target_branch` is already checked out in the `repo_dir`.

function(apply_patches repo_dir patches_dir base_revision target_branch)
    file(GLOB patches ${patches_dir}/*.patch)
    if(NOT patches)
        message(STATUS "No patches in ${patches_dir}")
        return()
    endif()

    if(NOT DEFINED GIT_EXECUTABLE)
      find_program(GIT_EXECUTABLE git)
    endif()

    message(STATUS "[VC] ${repo_dir}:")
    # Check if the target branch already exists
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --verify --no-revs -q ${target_branch}
        WORKING_DIRECTORY ${repo_dir}
        RESULT_VARIABLE patches_needed
    )
    # Set up fake username just in case if we don't have one globally
    execute_process(
      COMMAND ${GIT_EXECUTABLE} config --local user.name "patcher"
      WORKING_DIRECTORY ${repo_dir}
    )
    execute_process(
      COMMAND ${GIT_EXECUTABLE} config --local user.email "patcher@intel.com"
      WORKING_DIRECTORY ${repo_dir}
    )
    if(patches_needed) # The target branch doesn't exist
        list(SORT patches)
        execute_process( # Create the target branch
            COMMAND ${GIT_EXECUTABLE} checkout -b ${target_branch} ${base_revision}
            WORKING_DIRECTORY ${repo_dir}
            RESULT_VARIABLE es
        )
        if(NOT es EQUAL 0)
          message(FATAL_ERROR "Could not checkout base revision")
        endif()
        execute_process( # Apply the pathces
            COMMAND ${GIT_EXECUTABLE} am --3way --ignore-whitespace ${patches}
            WORKING_DIRECTORY ${repo_dir}
            RESULT_VARIABLE es
        )
        if(NOT es EQUAL 0)
          message(FATAL_ERROR "Could not apply SPIRV patches")
        endif()
    else() # The target branch already exists
        execute_process( # Check it out
            COMMAND ${GIT_EXECUTABLE} checkout ${target_branch}
            WORKING_DIRECTORY ${repo_dir}
            RESULT_VARIABLE es
        )
        if(NOT es EQUAL 0)
          message(FATAL_ERROR "Could not checkout branch")
        endif()
    endif()
endfunction()

# User may switch spirv dll installation off
if(NOT DEFINED INSTALL_SPIRVDLL)
  set(INSTALL_SPIRVDLL 1)
endif()

# Handle installation of SPIRVDLL.
# Currently, release build of spirvdll is used to read spirv.
# For debugging, one has to build debug version locally and replace release library.
if(INSTALL_SPIRVDLL)
include(ExternalProject)
set(MAKE_EXEC ${CMAKE_MAKE_PROGRAM})
message(STATUS "[VC] SPIRVDLL_SRC = ${SPIRVDLL_SRC}")
message(STATUS "[VC] SPIRV_SRC = ${SPIRV_SRC}")

# LLVM DIR should be present in external build but can be missing with
# in-tree build so set it manually in this case.
if(NOT LLVM_DIR)
  set(LLVM_DIR ${LLVM_BINARY_DIR}/lib/cmake/llvm)
endif()
if(DEFINED SPIRVDLL_SRC)
  if(NOT EXISTS ${SPIRVDLL_SRC})
    message(FATAL_ERROR "[VC] Cannot find SPIRVDLL sources in ${SPIRVDLL_SRC}")
  endif()
  set(SPIRV_SOURCES ${SPIRVDLL_SRC})
  if(IGC_BUILD__LLVM_PREBUILDS)

    ExternalProject_Add(SPIRVDLL_EX
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/SPIRVDLL
        SOURCE_DIR ${SPIRV_SOURCES}
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/spirv-install
        BUILD_COMMAND ${MAKE_EXEC} SPIRVDLL
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/spirv-install
      )

  else()

    ExternalProject_Add(SPIRVDLL_EX
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/SPIRVDLL
        SOURCE_DIR ${SPIRV_SOURCES}
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/spirv-install -DLLVM_DIR=${LLVM_DIR}
        BUILD_COMMAND ${MAKE_EXEC} SPIRVDLL
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/spirv-install
      )

  endif(IGC_BUILD__LLVM_PREBUILDS)

  add_dependencies(SPIRVDLL_EX VCCodeGen)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/spirv-install/lib/libSPIRVDLL.so
    DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    COMPONENT igc-core
  )
else()
  set(SPIRV_COPY "${CMAKE_CURRENT_BINARY_DIR}/llvm-spirv-vc")
  if(DEFINED SPIRV_SRC)
    if(NOT EXISTS ${SPIRV_SRC})
      message(FATAL_ERROR "[VC] Cannot find SPIRVDLL sources in ${SPIRV_SRC}")
    endif()
    set(SPIRV_SOURCES ${SPIRV_SRC})
  else()
    set(SPIRV_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../../../llvm-project/llvm/projects/llvm-spirv")
    if(NOT EXISTS ${SPIRV_SOURCES})
      message(STATUS "[VC] Cannot find SPIRVDLL sources in ${SPIRV_SOURCES}")
      set(SPIRV_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../../../llvm-spirv")
    endif()
    if(NOT EXISTS ${SPIRV_SOURCES})
      message(FATAL_ERROR "[VC] Cannot find SPIRVDLL sources in ${SPIRV_SOURCES}")
    endif()
  endif()

  if(NOT DEFINED LLVM_VERSION_MAJOR)
    message(FATAL_ERROR "[VC] Cannot find LLVM version (LLVM_VERSION_MAJOR)")
  elseif(${LLVM_VERSION_MAJOR} EQUAL 9)
    message(STATUS "[VC] Found LLVM version 9")
    set(SPIRV_REV_PATCH 237b3dbb0f46894cc5331b141fbf40942c1f834e)
    set(SPRIV_PATCHES ${CMAKE_CURRENT_SOURCE_DIR}/spirv-patches-9/)
    set(SPRIV_BRANCH_PATCH spirvdll_90)
  elseif(${LLVM_VERSION_MAJOR} EQUAL 10)
    message(STATUS "[VC] Found LLVM version 10")
    set(SPIRV_REV_PATCH 82b17c4acfa8a1d20e7f56a1670da8f5a73d74c8)
    set(SPRIV_PATCHES ${CMAKE_CURRENT_SOURCE_DIR}/spirv-patches-10/)
    set(SPRIV_BRANCH_PATCH spirvdll_100)
  elseif(${LLVM_VERSION_MAJOR} EQUAL 11)
    message(STATUS "[VC] Found LLVM version 11")
    set(SPIRV_REV_PATCH e8fa70cf2df7645bde5d171d7b7d50ceb306b47d)
    set(SPRIV_PATCHES ${CMAKE_CURRENT_SOURCE_DIR}/spirv-patches-11/)
    set(SPRIV_BRANCH_PATCH spirvdll_110)
  elseif(${LLVM_VERSION_MAJOR} EQUAL 12)
    message(STATUS "[VC] Found LLVM version 12")
    set(SPIRV_REV_PATCH 9c93a4573f861e23ae49aace46296f94f90030f0)
    set(SPRIV_PATCHES ${CMAKE_CURRENT_SOURCE_DIR}/spirv-patches-12/)
    set(SPRIV_BRANCH_PATCH spirvdll_120)
  else()
    message(FATAL_ERROR "[VC] Found unsupported version of LLVM (LLVM_VERSION_MAJOR is set to ${LLVM_VERSION_MAJOR})")
  endif()

  if(NOT EXISTS ${SPIRV_COPY})
    message(STATUS "[VC] : Copying stock SPIRV-Translator sources to ${SPIRV_COPY}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${SPIRV_SOURCES} ${SPIRV_COPY})
  endif()

  apply_patches(${SPIRV_COPY}
  ${SPRIV_PATCHES}
  ${SPIRV_REV_PATCH}
  ${SPRIV_BRANCH_PATCH}
  )

  if(IGC_BUILD__LLVM_PREBUILDS)

    ExternalProject_Add(SPIRVDLL_EX
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/SPIRVDLL
        SOURCE_DIR ${SPIRV_COPY}
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/spirv-install
        BUILD_COMMAND ${MAKE_EXEC} SPIRVDLL
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/spirv-install
      )

  else()

    ExternalProject_Add(SPIRVDLL_EX
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/SPIRVDLL
        SOURCE_DIR ${SPIRV_COPY}
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/spirv-install -DLLVM_DIR=${LLVM_DIR}
        BUILD_COMMAND ${MAKE_EXEC} SPIRVDLL
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/spirv-install
      )

  endif(IGC_BUILD__LLVM_PREBUILDS)

  add_dependencies(SPIRVDLL_EX VCCodeGen)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/spirv-install/lib/libSPIRVDLL.so
    DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    COMPONENT igc-core
  )

endif(DEFINED SPIRVDLL_SRC)
endif(INSTALL_SPIRVDLL)
