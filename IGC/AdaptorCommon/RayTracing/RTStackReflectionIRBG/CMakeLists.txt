#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include(IRBuilderGeneratorCodeGen)

# Set up paths
set(RT_ROOT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})
set(RT_SOURCE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(REFLECTION_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/reflection.cpp)
set(YAML_DESC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Desc.yaml)
set(IRBUILDER_GENERATOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../IRBuilderGenerator)

# Set up include directories for clang
set(RTSTACK_INCLUDE_DIRS
    ${RT_ROOT_FOLDER}
    ${RT_SOURCE_FOLDER}
    ${IRBUILDER_GENERATOR_DIR}
)

# Set up dependencies - the reflection.cpp file and any headers it includes
set(RTSTACK_DEPENDENCIES
    ${REFLECTION_SOURCE}
    ${YAML_DESC_PATH}
    # Add any header files that reflection.cpp depends on
)

# Generate RT Stack reflection headers
generate_irbuilder_headers(
    NAME RTStackReflection
    SOURCE_FILE ${REFLECTION_SOURCE}
    YAML_PATH ${YAML_DESC_PATH}
    OUTPUT_DIR ${RT_ROOT_FOLDER}
    INCLUDE_DIRS ${RTSTACK_INCLUDE_DIRS}
    DEPENDS ${RTSTACK_DEPENDENCIES}
)

add_library(RTStackReflectionIRBG STATIC
    ${GENERATED_HEADERS}
    ${REFLECTION_SOURCE}
    ${YAML_DESC_PATH}
)

# Mark the source files as HEADER_FILE_ONLY so they won't be compiled
set_source_files_properties(
    ${GENERATED_HEADERS}
    ${REFLECTION_SOURCE}
    ${YAML_DESC_PATH}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

target_include_directories(RTStackReflectionIRBG PUBLIC
    ${RT_ROOT_FOLDER}
    ${RT_SOURCE_FOLDER}
    ${IRBUILDER_GENERATOR_DIR}
    ${RT_SOURCE_FOLDER}/../common
)

add_dependencies(RTStackReflectionIRBG IRBuilderGenerator)

set_target_properties(RTStackReflectionIRBG PROPERTIES FOLDER "Misc")
