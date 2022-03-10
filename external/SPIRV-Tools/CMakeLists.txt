#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

cmake_minimum_required(VERSION 2.8.12)

message(STATUS "============================ SPIRV-Tools project ============================")

if(IGC_OPTION__SPIRV_TOOLS_MODE STREQUAL PREBUILDS_MODE_NAME)

 message(STATUS "[SPIRV-Tools] : IGC_OPTION__SPIRV_TOOLS_MODE set to Prebuilds")
 OPTION(IGC__OPTION_USE_PREINSTALLED_SPRIV_HEADERS OFF)

 if(IGC_OPTION__USE_PREINSTALLED_SPRIV_HEADERS)
  message(STATUS "[SPIRV-Tools] : IGC__OPTION_USE_PREINSTALLED_SPRIV_HEADERS set to OFF, using preinstalled SPIRV-Headers")
  message(STATUS "[SPIRV-Tools] : Using preinstalled SPIRV-Headers")
  set(SPIRV-Headers_INCLUDE_DIR "/usr/include")
 else()
  message(STATUS "[SPIRV-Tools] : IGC__OPTION_USE_PREINSTALLED_SPRIV_HEADERS set to OFF")
  message(STATUS "[SPIRV-Tools] : Using default path ${CMAKE_CURRENT_SOURCE_DIR}/../../SPIRV-Headers")
  set(SPIRV-Headers_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../SPIRV-Headers/include")
 endif()

 message(STATUS "[SPIRV-Tools] : Searching prebuilt SPIRV-Tools package")
 find_package(SPIRV-Tools REQUIRED COMPONENTS SPIRV-Tools)
 message(STATUS "[SPIRV-Tools] : Found prebuilt SPIRV-Tools package in: ${SPIRV-Tools_CONFIG}")

 set(SPIRV-Tools_ROOT_DIR "${SPIRV-Tools_DIR}../../..")
 set(IGC_BUILD__SPIRV-Tools_DIR "${SPIRV-Tools_ROOT_DIR}")

 set(INCLUDE_DIRS_LIST "${SPIRV-Tools_ROOT_DIR}/include" "${SPIRV-Headers_INCLUDE_DIR}")
 set_target_properties(SPIRV-Tools-static PROPERTIES INCLUDE_DIRECTORIES "${INCLUDE_DIRS_LIST}")
 set(IGC_BUILD__PROJ__SPIRV-Tools SPIRV-Tools)

else() #By default use build from sources
 message(STATUS "[SPIRV-Tools] : IGC_OPTION__SPIRV_TOOLS_MODE set to Source")
 message(STATUS "[SPIRV-Tools] : IGC_OPTION__USE_PREINSTALLED_SPRIV_HEADERS set to ON")
 message(STATUS "[SPIRV-Tools] : Using preinstalled packages")
 message(STATUS "[SPIRV-Tools] : Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

 set(SPIRV-Headers_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../SPIRV-Headers") # used in subdirectory
 set(SPIRV-Tools_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../SPIRV-Tools")

 set(SPIRV-Tools_OUTPUT_DIR "${IGC_OPTION__OUTPUT_DIR}/external/SPIRV-Tools/build")
 set(IGC_BUILD__SPIRV-Headers_DIR "${SPIRV-Headers_SOURCE_DIR}")
 set(IGC_BUILD__SPIRV-Tools_DIR "${SPIRV-Tools_SOURCE_DIR}")

 # options used in subdirectory
 option(SKIP_SPIRV_TOOLS_INSTALL "Skip SPIRV-Tools installation" ON)
 option(SPIRV_SKIP_TESTS "Skip building SPIRV-Tools tests along with the library" ON)
 option(SPIRV_SKIP_EXECUTABLES "Skip building SPIRV-Tools executables and tests along with the library" ON)
 option(SPIRV_HEADERS_SKIP_INSTALL "Skip SPIRV-Headers installation" ON)
 option(SPIRV_HEADERS_SKIP_EXAMPLES "Skip building SPIRV-Headers examples" ON)

 message(STATUS "SPIRV-Tools source directory: ${SPIRV-Tools_SOURCE_DIR}")
 message(STATUS "SPIRV-Headers source directory: ${SPIRV-Headers_SOURCE_DIR}")
 message(STATUS "SPIRV-Tools output directory: ${SPIRV-Tools_OUTPUT_DIR}")
 message(STATUS "Option SKIP_SPIRV_TOOLS_INSTALL: ${SKIP_SPIRV_TOOLS_INSTALL}")
 message(STATUS "Option SPIRV_SKIP_TESTS: ${SPIRV_SKIP_TESTS}")
 message(STATUS "Option SPIRV_SKIP_EXECUTABLES: ${SPIRV_SKIP_EXECUTABLES}")
 message(STATUS "Option SPIRV_HEADERS_SKIP_INSTALL: ${SPIRV_HEADERS_SKIP_INSTALL}")
 message(STATUS "Option SPIRV_HEADERS_SKIP_EXAMPLES: ${SPIRV_HEADERS_SKIP_EXAMPLES}")

 add_subdirectory("${SPIRV-Tools_SOURCE_DIR}" "${SPIRV-Tools_OUTPUT_DIR}" EXCLUDE_FROM_ALL)
 set(IGC_BUILD__PROJ__SPIRV-Tools "SPIRV-Tools") # same name as in subdirectory

endif() #SPIRV_TOOLS_MODE

message(STATUS "=============================================================================")
