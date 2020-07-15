#
# Creates `target_branch` starting at the `base_revision` in the `repo_dir`.
# Then all patches from the `patches_dir` are committed to the `target_branch`.
# Does nothing if the `target_branch` is already checked out in the `repo_dir`.
#
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
        )
        execute_process( # Apply the pathces
            COMMAND ${GIT_EXECUTABLE} am --3way --ignore-whitespace ${patches}
            WORKING_DIRECTORY ${repo_dir}
        )
    else() # The target branch already exists
        execute_process( # Check it out
            COMMAND ${GIT_EXECUTABLE} checkout ${target_branch}
            WORKING_DIRECTORY ${repo_dir}
        )
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
if(NOT DEFINED SPIRV_PREBUILD_DIR AND NOT WIN32)
include(ExternalProject)
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

set(SPIRV_REV_PATCH 7c8443e3d2032af05b46571c12c7de1b5209f7fe)
set(SPRIV_PATCHES ${CMAKE_CURRENT_SOURCE_DIR}/spirv-patches-new/)
set(SPRIV_BRANCH_PATCH spirvdll_100)
find_program(MAKE_EXEC NAMES make gmake)

if(NOT EXISTS ${SPIRV_COPY})
  message(STATUS "[VC] : Copying stock SPIRV-Translator sources to ${SPIRV_COPY}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${SPIRV_SOURCES} ${SPIRV_COPY})
endif()

apply_patches(${SPIRV_COPY}
${SPRIV_PATCHES}
${SPIRV_REV_PATCH}
${SPRIV_BRANCH_PATCH}
)

if(IGC_OPTION__FORCE_SYSTEM_LLVM)

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

    add_dependencies(SPIRVDLL_EX VCCodeGen)

endif(IGC_OPTION__FORCE_SYSTEM_LLVM)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/spirv-install/lib/libSPIRVDLL.so
  DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
  COMPONENT igc-core
)

elseif(NOT TARGET SPIRVDLL)
  if(DEFINED SPIRV_PREBUILD_DIR)
    set(PREBUILT_SPIRVDLL_PATH "${SPIRV_PREBUILD_DIR}/lib" )
  endif()
  if(DEFINED WIN32)
    set(SPIRVDLL_NAME "SPIRVDLL.dll")
  else()
    set(SPIRVDLL_NAME "libSPIRVDLL.so")
  endif()
  find_file(SPIRVDLL_LIB
    ${SPIRVDLL_NAME}
    PATHS ${PREBUILT_SPIRVDLL_PATH}
    NO_DEFAULT_PATH
  )
  if(NOT SPIRVDLL_LIB)
    message(FATAL_ERROR "[VC] Cannot find SPIRVDLL in prebuilds")
  endif()
  message(STATUS "[VC] Found SPIRVDLL: ${SPIRVDLL_LIB}")
  if(WIN32)
    if ("${vc_uses_custom_spirv}" STREQUAL "True")
      set(INSTALL_SPRIRVDLL_NAME "SPIRVDLL.dll")
      if("${_cpuSuffix}" STREQUAL "32")
        set(INSTALL_SPRIRVDLL_NAME "SPIRVDLL32.dll")
      endif()
      install(FILES ${SPIRVDLL_LIB}
        CONFIGURATIONS Debug Release
        DESTINATION $<CONFIG>/lh64
        RENAME ${INSTALL_SPRIRVDLL_NAME}
      )
    endif()
  else()
    install(FILES
      ${SPIRVDLL_LIB}
      DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
      COMPONENT igc-core
      )
  endif()
else()
  get_target_property(SPIRVDLL_IMPORTED SPIRVDLL IMPORTED)
  if(SPIRVDLL_IMPORTED)
    message(STATUS "[VC] SPIRVDLL is already imported")
  else()
    message(STATUS "[VC] SPIRVDLL will be built in-tree")
    install(FILES
      $<TARGET_FILE:SPIRVDLL>
      DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
      COMPONENT igc-core
    )
  endif()
endif()
endif(INSTALL_SPIRVDLL)
