#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# This cmake support file includes function definitions used in the cmake build

# Idempotent include implementation
set(BUILD_FUNCTIONS_INC ON)

# Functions to copy various libraries and support files into their required locations
# This is no longer a post-build step but rather a separate project which can be run independently
# of whether the original library has been built or not. This is primarily to ensure that the
# correct libraries for the chosen configuration are always copied (e.g. Debug or Release) - with
# this in place it is possible to switch between release and debug arbitrarily and know that the
# deployed libraries are the correct ones.
# Assumption is that there is only one of these per target (otherwise the custom target breaks)

# Macros to set the runtime to /MT under windows (statically link to runtime.
# Suitable for multi-threaded as well)
macro(  win_static_runtime )
  # Windows only
  # NOTE: this is a directory level setting so everything for a given
  # CMakeLists.txt must be either /MT or /MD
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    set(configurations
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    foreach(configuration ${configurations})
      if(${configuration} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${configuration} "${${configuration}}")
      endif()
    endforeach()
  endif()
endmacro( win_static_runtime)

# Macros to set the runtime to /MD under windows (dynamically link to runtime)
macro(  win_dynamic_runtime )
  # Windows only
  # NOTE: this is a directory level setting so everything for a given CMakeLists.txt
  # must be either /MT or /MD
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    set(configurations
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    foreach(configuration ${configurations})
      if(${configuration} MATCHES "/MT")
        string(REGEX REPLACE "/MT" "/MD" ${configuration} "${${configuration}}")
      endif()
    endforeach()
  endif()
endmacro( win_dynamic_runtime)

# Add resource files to project
# First argument is the target project
# Second argument is the source_list being constructed for the target
# Any subsequent argument(s) are resource files to be added
# This function is windows specific and guards the body of the function accordingly
function( win_add_resource source_list )
  if(WIN32)
    foreach( resource_file ${ARGN})
      source_group("Resource Files" FILES ${resource_file})
      set(${source_list} ${${source_list}} ${resource_file} PARENT_SCOPE)
    endforeach( )
  endif(WIN32)
endfunction( win_add_resource )

function ( add_project_dependencies_base new_target conditional src_list dst_list tgt_list)

  get_property( fixed_target_file GLOBAL PROPERTY ${dst_list} )
  get_property( fixed_source_file GLOBAL PROPERTY ${src_list} )
  get_property( fixed_target GLOBAL PROPERTY ${tgt_list} )

  # Create the new custom target
  # Build up the string required to invoke the commands
  list( LENGTH fixed_target_file len )
  math( EXPR len "${len} - 1" )
  foreach ( counter RANGE ${len} )
    list( GET fixed_target_file ${counter} target_file )
    list( GET fixed_source_file ${counter} source_file )
    list( GET fixed_target ${counter} raw_target )

    if (NOT ${conditional})
      # Build the copy command required
      if ( ${source_file} STREQUAL "none")
        if ( ${raw_target} STREQUAL "none")
          message("error - shouldn't get here : ${target_file} ${raw_target} ${source_file}\n")
          add_custom_command(TARGET ${new_target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${raw_target}> ${target_file})
        else()
          add_custom_command(TARGET ${new_target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${raw_target}> ${target_file}
            DEPENDS ${raw_target})
        endif()
      else()
        if ( ${raw_target} STREQUAL "none")
          add_custom_command(TARGET ${new_target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${source_file} ${target_file})
        else()
          add_custom_command(TARGET ${new_target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${source_file} ${target_file}
            DEPENDS ${raw_target})
        endif()
      endif()
    else()
      # Build the conditional copy command required
      if ( ${source_file} STREQUAL "none")
        if ( ${raw_target} STREQUAL "none")
          message("error - shouldn't get here : ${target_file} ${raw_target} ${source_file}\n")
          add_custom_command(TARGET ${new_target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -DMY_SRC=$<TARGET_FILE:${raw_target}> -DMY_DEST=${target_file}
            -P ${CMAKE_SOURCE_DIR}/ConditionalCopy.cmake
            )
        else()
          add_custom_command(TARGET ${new_target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -DMY_SRC=$<TARGET_FILE:${raw_target}> -DMY_DEST=${target_file}
            -P ${CMAKE_SOURCE_DIR}/ConditionalCopy.cmake
            DEPENDS ${raw_target})
        endif()
      else()
        if ( ${raw_target} STREQUAL "none")
          add_custom_command(TARGET ${new_target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -DMY_SRC=${source_file} -DMY_DEST=${target_file}
            -P ${CMAKE_SOURCE_DIR}/ConditionalCopy.cmake)
        else()
          add_custom_command(TARGET ${new_target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -DMY_SRC=${source_file} -DMY_DEST=${target_file}
            -P ${CMAKE_SOURCE_DIR}/ConditionalCopy.cmake
            DEPENDS ${raw_target})
        endif()
      endif()
    endif()
  endforeach()

endfunction( add_project_dependencies_base )

function ( add_project_dependencies new_target conditional_list src_list_list dst_list_list tgt_list_list)

  add_custom_target(${new_target} ALL ${CMAKE_COMMAND} -E echo "Building target ${new_target}")

  list( LENGTH src_list_list len )
  math( EXPR len "${len} - 1" )
  foreach ( counter RANGE ${len} )
    list( GET conditional_list ${counter} conditional )
    list( GET src_list_list ${counter} src_list )
    list( GET dst_list_list ${counter} dst_list )
    list( GET tgt_list_list ${counter} tgt_list )

    add_project_dependencies_base( ${new_target} ${conditional} ${src_list} ${dst_list} ${tgt_list} )
  endforeach()

endfunction( add_project_dependencies )

function ( setup_executable target src_list exclude_all actual_name)
  if (${exclude_all})
    add_executable(${target} EXCLUDE_FROM_ALL ${src_list})
  else ()
    add_executable(${target} ${src_list})
  endif()

  # Set the output directory for the release pdb file
  if (WIN32)
    # Disable CMP0026 warning for using LOCATION
    cmake_policy(SET CMP0026 OLD)
    get_property(output_fq_file TARGET ${target} PROPERTY LOCATION_RELEASE)
    get_filename_component(output_dir ${output_fq_file} DIRECTORY)
    if ( ${actual_name} STREQUAL "None" )
      get_filename_component(output_name_we ${output_fq_file} NAME_WE)
    else ( ${actual_name} STREQUAL "None" )
      set(output_name_we ${actual_name})
      set_target_properties( ${target} PROPERTIES OUTPUT_NAME ${actual_name} )
    endif( ${actual_name} STREQUAL "None" )
    set_target_properties( ${target} PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY_RELEASE ${output_dir})
    set_target_properties( ${target} PROPERTIES COMPILE_PDB_NAME_RELEASE ${output_name_we})
  endif (WIN32)
endfunction( setup_executable )

# If you update this function - make sure you also update the copy of this for the IGC build that is
# hard-coded into the CM_jitter CMakeLists.txt
function ( setup_library target src_list shared actual_name)
  if ( ${shared} )
    add_library(${target} SHARED ${src_list})
  else ( ${shared} )
    add_library(${target} ${src_list})
  endif ( ${shared} )

  # Set the output directory for the release pdb file
  if (WIN32)
    # Disable CMP0026 warning for using LOCATION
    cmake_policy(SET CMP0026 OLD)
    get_property(output_fq_file TARGET ${target} PROPERTY LOCATION_RELEASE)
    get_filename_component(output_dir ${output_fq_file} DIRECTORY)
    if ( ${actual_name} STREQUAL "None" )
      get_filename_component(output_name_we ${output_fq_file} NAME_WE)
    else ( ${actual_name} STREQUAL "None" )
      set(output_name_we ${actual_name})
      set_target_properties( ${target} PROPERTIES OUTPUT_NAME ${actual_name} )
    endif( ${actual_name} STREQUAL "None" )
    set_target_properties( ${target} PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY_RELEASE ${output_dir})
    set_target_properties( ${target} PROPERTIES COMPILE_PDB_NAME_RELEASE ${output_name_we})
  endif (WIN32)
endfunction( setup_library )
