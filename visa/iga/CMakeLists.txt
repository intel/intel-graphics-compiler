#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# IGC depends on this; so it's safe to use here
cmake_minimum_required(VERSION 3.13)
message(" cmake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")

if(NOT IGC_BUILD)
  enable_testing()
endif()

project(IGA)

include (BuildSetup.cmake)
include (BuildFunctions.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# do static build for IGA standalone build
set(LINK_AS_STATIC_LIB TRUE)

include_directories(include)
include_directories("../../inc")

set(GED_BRANCH GED_external)
add_subdirectory(GEDLibrary/${GED_BRANCH})
add_subdirectory(IGALibrary)
add_subdirectory(IGAExe)
