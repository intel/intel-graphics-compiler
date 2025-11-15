#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Include the RTStackReflection code generation function
include(RTStackReflectionCodeGen)

# Set up paths
set(RTSTACK_REFLECTION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../RTStackReflection)
set(RT_ROOT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})
set(RT_SOURCE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(REFLECTION_SOURCE ${RTSTACK_REFLECTION_DIR}/reflection.cpp)

# Set up include directories for clang
set(RTSTACK_INCLUDE_DIRS
    ${RT_SOURCE_FOLDER}
)

# Set up dependencies - the reflection.cpp file and any headers it includes
set(RTSTACK_DEPENDENCIES
    ${REFLECTION_SOURCE}
    # Add any header files that reflection.cpp depends on
)

# Generate RT Stack reflection headers
generate_rtstack_reflection_headers(
    SOURCE_FILE ${REFLECTION_SOURCE}
    OUTPUT_DIR ${RT_ROOT_FOLDER}
    INCLUDE_DIRS ${RTSTACK_INCLUDE_DIRS}
    DEPENDS ${RTSTACK_DEPENDENCIES}
)

# Create an interface library that other targets can depend on
# The generated headers are added as sources so the custom commands will be triggered
add_library(RTStackReflectionIRBG INTERFACE
    ${RTSTACK_GENERATED_HEADERS}
)

# Make the generated headers available to consumers of this library
target_include_directories(RTStackReflectionIRBG INTERFACE ${RT_ROOT_FOLDER})

# Add a dependency on the RTStackReflection tool itself
add_dependencies(RTStackReflectionIRBG RTStackReflection)

set_target_properties(RTStackReflectionIRBG PROPERTIES FOLDER "IGC/Misc")
