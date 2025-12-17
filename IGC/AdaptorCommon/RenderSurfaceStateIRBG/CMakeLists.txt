#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include(IRBuilderGeneratorCodeGen)

# Set up paths
set(RSS_ROOT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})
set(RSS_SOURCE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
set(RSS_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceStateAccessors.cpp)
set(RSS_YAML ${CMAKE_CURRENT_SOURCE_DIR}/AddressSpaces.yaml)
set(FORMAT_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceStateFormat.h)
set(BUILDER_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceStateBuilder.h)
set(MIXIN_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/RenderSurfaceStateBuilder.h)
set(IRBUILDER_GENERATOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../IRBuilderGenerator)

# Set up include directories for clang
set(RSS_INCLUDE_DIRS
    ${RSS_ROOT_FOLDER}
    ${RSS_SOURCE_FOLDER}
    ${IRBUILDER_GENERATOR_DIR}
)

# Set up dependencies
set(RSS_DEPENDENCIES
    ${RSS_SOURCE}
    ${RSS_YAML}
    ${FORMAT_HEADER}
    ${BUILDER_HEADER}
    ${MIXIN_HEADER}
)

# Generate RenderSurfaceState headers
generate_irbuilder_headers(
    NAME RenderSurfaceState
    SOURCE_FILE ${RSS_SOURCE}
    YAML_PATH ${RSS_YAML}
    OUTPUT_DIR ${RSS_ROOT_FOLDER}
    INCLUDE_DIRS ${RSS_INCLUDE_DIRS}
    DEPENDS ${RSS_DEPENDENCIES}
)

add_library(RenderSurfaceStateIRBG STATIC
    ${GENERATED_HEADERS}
    ${RSS_SOURCE}
    ${FORMAT_HEADER}
    ${BUILDER_HEADER}
    ${MIXIN_HEADER}
)

# Mark the source files as HEADER_FILE_ONLY so they won't be compiled
set_source_files_properties(
    ${GENERATED_HEADERS}
    ${RSS_SOURCE}
    ${FORMAT_HEADER}
    ${BUILDER_HEADER}
    ${MIXIN_HEADER}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

# Parent directory (AdaptorCommon) allows consumers to use:
#   #include "RenderSurfaceStateIRBG/RenderSurfaceStateBuilder.h"
set(ADAPTOR_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_include_directories(RenderSurfaceStateIRBG PUBLIC
    ${RSS_ROOT_FOLDER}
    ${RSS_SOURCE_FOLDER}
    ${ADAPTOR_COMMON_DIR}
    ${IRBUILDER_GENERATOR_DIR}
)

add_dependencies(RenderSurfaceStateIRBG IRBuilderGenerator)

set_target_properties(RenderSurfaceStateIRBG PROPERTIES FOLDER "Misc")
