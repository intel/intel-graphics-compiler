#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(INTERNAL_INTRINSICS_DESCRIPTION "InternalIntrinsicDescription.gen")
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${INTERNAL_INTRINSICS_DESCRIPTION}
    COMMAND ${PYTHON_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/Intrinsics.py
            ${CMAKE_CURRENT_SOURCE_DIR}/Intrinsic_definitions.py
            ${CMAKE_CURRENT_BINARY_DIR}/${INTERNAL_INTRINSICS_DESCRIPTION}
    DEPENDS
         ${CMAKE_CURRENT_SOURCE_DIR}/Intrinsics.py
         ${CMAKE_CURRENT_SOURCE_DIR}/Intrinsic_definitions.py
    COMMENT "Building ${INTERNAL_INTRINSICS_DESCRIPTION}..."
)

add_custom_target(VCInternalIntrinsicDescriptionGen
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${INTERNAL_INTRINSICS_DESCRIPTION}
)
add_custom_target(VCInternalIntrinsicsGen)
add_dependencies(VCInternalIntrinsicsGen VCInternalIntrinsicDescriptionGen)
