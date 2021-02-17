#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2020-2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
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
