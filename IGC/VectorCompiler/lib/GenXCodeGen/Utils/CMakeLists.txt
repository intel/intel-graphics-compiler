#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(CISA_GEN_INTRINSICS "${CMAKE_CURRENT_SOURCE_DIR}/cisa_gen_intrinsics.py")
set(CISA_JSON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cisa_gen_intrinsics.json")

set(CISA_OUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/../")
set(CISA_OUT_FILES "${CISA_OUT_PATH}/GenXIntrinsicInfoTable.inc"
                   "${CISA_OUT_PATH}/GenXIntrinsicsBuildMap.inc")
message(" >>${CISA_OUT_PATH}<<  ->  ${CMAKE_CURRENT_BINARY_DIR}")
message(" COMMAND ->  ${PYTHON_EXECUTABLE} ${CISA_GEN_INTRINSICS} ${CISA_JSON_FILE} ${CISA_OUT_PATH} <-")
message(" ${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_command(
    OUTPUT ${CISA_OUT_FILES}
    COMMAND ${PYTHON_EXECUTABLE} ${CISA_GEN_INTRINSICS} ${CISA_JSON_FILE} ${CISA_OUT_PATH}
    COMMENT "Building Cisa generators for GenXCisaBuilder."
    DEPENDS ${CISA_GEN_INTRINSICS} ${CISA_JSON_FILE}
    VERBATIM)

set_source_files_properties(
    ${CISA_OUT_FILES}
    PROPERTIES GENERATED TRUE
    )

add_custom_target(GenXUtilBuild ALL DEPENDS ${CISA_OUT_FILES})
