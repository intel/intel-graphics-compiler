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

add_subdirectory ("source")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/source")
add_library(zebinlib STATIC ${ZE_INFO_SOURCE_FILE} ${ZE_INFO_INCLUDE_FILE})


set(ZEBIN_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/source/autogen")

include_directories("${ZEBIN_INCLUDE_PATH}")


if(MSVC)
    set(ZEBIN_COMPILER_OPTIONS
        /w44141
        /w44146
        /w44244
    )
    target_compile_options(zebinlib PRIVATE ${ZEBIN_COMPILER_OPTIONS})
endif()
