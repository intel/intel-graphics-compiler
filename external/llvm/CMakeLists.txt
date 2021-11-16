#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#
# LLVM module
#

cmake_minimum_required(VERSION 3.13.4)

find_package(Python3 REQUIRED)
find_package(Patch REQUIRED)

include(llvm_utils.cmake)
include(llvm_flags.cmake)

# Handle registered projects.
get_llvm_external_projects(projects)
set_llvm_opt(LLVM_EXTERNAL_PROJECTS "${projects}" CACHE STRING "LLVM external projects" FORCE)
foreach(p IN LISTS projects)
  get_llvm_external_project_dir(${p} dir BASE_DIR ${IGC_OPTION__LLVM_SOURCES_DIR})
  string(TOUPPER ${p} up)
  set_llvm_opt(LLVM_EXTERNAL_${up}_SOURCE_DIR "${dir}" CACHE PATH "Path to ${p} project")
endforeach()

message(STATUS "[LLVM] : LLVM_SOURCE_URL   = ${IGC_OPTION__LLVM_SOURCES_DIR}")
message(STATUS "[LLVM] : LLVM_OPTIONS      = ${LLVM_OPTIONS}")

if(NOT IGC_OPTION__LLVM_STOCK_SOURCES)
  include(llvm_patch_sources.cmake)
else()
  set(IGC_LLVM_SOURCE_DIR ${IGC_OPTION__LLVM_SOURCES_DIR}/llvm)
endif()

if(CMAKE_CONFIGURATION_TYPES)
    # Multi-configuration generator.
    set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
    message(STATUS "[LLVM_PATCHER] : LLVM_CONFIGURATION_TYPES = ${CMAKE_CONFIGURATION_TYPES}")
else()
    # Single-configuration generator.
    message(STATUS "[LLVM_PATCHER] : LLVM_BUILD_TYPE   = ${CMAKE_BUILD_TYPE}")
endif()

# project() command sets global <project>_SOURCE_DIR and <project>_BINARY_DIR
# and we can use it later.
build_llvm_project(LLVM ${IGC_LLVM_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/src)

# Sanity check for version matching.
get_directory_property(LLVM_VERSION  DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_VERSION")
if(NOT ("${LLVM_VERSION}" VERSION_EQUAL "${IGC_OPTION__LLVM_PREFERRED_VERSION}"))
  message(FATAL_ERROR "Found LLVM version: ${LLVM_VERSION};"
    " however preferred was: ${IGC_OPTION__LLVM_PREFERRED_VERSION}\n"
    "Please set IGC_OPTION__LLVM_PREFERRED_VERSION to ${LLVM_VERSION}"
    " or pass IGC_OPTION__LLVM_SOURCES_DIR with preferred version")
endif()

# Get version of LLVM that we are currently using.
get_directory_property(LLVM_VERSION_MAJOR  DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_VERSION_MAJOR")
get_directory_property(LLVM_VERSION_MINOR  DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_VERSION_MINOR")
get_directory_property(LLVM_VERSION_PATCH  DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_VERSION_PATCH")
get_directory_property(LLVM_VERSION_SUFFIX DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_VERSION_SUFFIX")
# Make LLVM version accessible in parent scope.
set(LLVM_VERSION_MAJOR  ${LLVM_VERSION_MAJOR}  PARENT_SCOPE)
set(LLVM_VERSION_MINOR  ${LLVM_VERSION_MINOR}  PARENT_SCOPE)
set(LLVM_VERSION_PATCH  ${LLVM_VERSION_PATCH}  PARENT_SCOPE)
set(LLVM_VERSION_SUFFIX ${LLVM_VERSION_SUFFIX} PARENT_SCOPE)

set(LLVM_INCLUDE_DIRS "${LLVM_SOURCE_DIR}/include;${CMAKE_CURRENT_BINARY_DIR}/src/include")
set(LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS} PARENT_SCOPE)
