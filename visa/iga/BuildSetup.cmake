#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Enable organisation of targets into folders. This is primarily used when generating the Visual
# Studio solutions and means that the projects can be partitioned into seperate sections (folders)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Idempotent include implementation
set(BUILD_SETUP_INC ON)

# Distinguish between 32 and 64 bits
# The string that is set is used to modify the target names of some of the libraries generated
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(TARGET_MODIFIER "32")
  set(PB_PATH_MODIFIER "x86")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(TARGET_MODIFIER "64")
  set(PB_PATH_MODIFIER "x64")
else()
  message(FATAL_ERROR "unexpected platform")
endif()

if (${CMAKE_GENERATOR} MATCHES "Ninja" OR ${CMAKE_GENERATOR} MATCHES "Unix Makefiles" OR ${CMAKE_GENERATOR} MATCHES "MSYS Makefiles" OR ${CMAKE_GENERATOR} MATCHES "MinGW Makefiles")
  set(UNIX_MAKEFILE_GENERATOR 1)
elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio")
  set(MSVC_GENERATOR 1)
else()
  message("UNEXPECTED GENERATOR: ${CMAKE_GENERATOR}")
endif()

if (UNIX_MAKEFILE_GENERATOR AND NOT WIN32)
  message("**************** PLATFORM is GNU Linux ${TARGET_MODIFIER}-bit (${CMAKE_BUILD_TYPE}) ****************")
  set(LINUX 1)
elseif (MSVC AND UNIX_MAKEFILE_GENERATOR MATCHES 1)
  message("**************** PLATFORM is MSVC ${TARGET_MODIFIER}-bit (${CMAKE_BUILD_TYPE}) **************")
elseif (UNIX_MAKEFILE_GENERATOR AND WIN32)
  message("**************** PLATFORM is Cygwin/MinGW ${TARGET_MODIFIER}-bit (${CMAKE_BUILD_TYPE}) **************")
elseif (MSVC_GENERATOR)
  message("**************** PLATFORM is MSVC ${TARGET_MODIFIER}-bit (${CMAKE_BUILD_TYPE}) ****************")
else()
  message(FATAL_ERROR "**************** PLATFORM is UNKNOWN ****************")
endif()

# for debugging: list all variables
#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#  message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

if(UNIX_MAKEFILE_GENERATOR AND NOT MSVC)
  set(COMMON_C_FLAGS "-fno-strict-aliasing -msse4.1 -std=gnu++0x -Wno-unused-function -fpermissive")
  if(LINUX)
    set(COMMON_C_FLAGS "${COMMON_C_FLAGS} -DLINUX -fPIC")
  endif()

  set(CMAKE_CXX_FLAGS ${COMMON_C_FLAGS})
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fstack-protector-all -D_FORTIFY_SOURCE=2")
  set(CMAKE_CXX_FLAGS_DEBUG   "-D_DEBUG -D__DEBUG -O0 -g")

  set(CMAKE_C_FLAGS ${COMMON_C_FLAGS})
  set(CMAKE_C_FLAGS_RELEASE "-O2")
  set(CMAKE_C_FLAGS_DEBUG   "-D_DEBUG -D__DEBUG -O0 -g")

  set(GCC_SECURE_LINK_FLAGS "-z relro -z now")
  set(CMAKE_SKIP_RPATH ON)
elseif(MSVC)
  # Need release build to create pdb at obj creation (e.g. c/c++ phase rather than link)
  # /Zi does this
  # Also need /FS as multiple cl commands write to the same pdb simultaneously
  set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   /Zi /Od /sdl /GR-")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /Gy /Oi /sdl /GR-")

  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi /FS")
  set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_RELEASE} /Zi /FS")
endif()

