#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

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
      CMAKE_C_FLAGS_RELEASEINTERNAL
      CMAKE_C_FLAGS_RELEASEINTERNALASSERTS
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELEASEINTERNAL
      CMAKE_CXX_FLAGS_RELEASEINTERNALASSERTS
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
      CMAKE_C_FLAGS_RELEASEINTERNAL
      CMAKE_C_FLAGS_RELEASEINTERNALASSERTS
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELEASEINTERNAL
      CMAKE_CXX_FLAGS_RELEASEINTERNALASSERTS
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    foreach(configuration ${configurations})
      if(${configuration} MATCHES "/MT")
        string(REGEX REPLACE "/MT" "/MD" ${configuration} "${${configuration}}")
      endif()
    endforeach()
  endif()
endmacro( win_dynamic_runtime)

function ( setup_executable target src_list exclude_all actual_name)
  if (${exclude_all})
    add_executable(${target} EXCLUDE_FROM_ALL ${src_list})
  else ()
    add_executable(${target} ${src_list})
  endif()

  if ( NOT (${actual_name} STREQUAL "None") )
    set_target_properties( ${target} PROPERTIES OUTPUT_NAME ${actual_name} )
  endif()
endfunction( setup_executable )
