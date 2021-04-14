#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2021-2021 Intel Corporation
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

set(EMU_DIVREM_BOILERPLATE_FILE ${CMAKE_CURRENT_BINARY_DIR}/emulation_divrem_boilerplate.h)
set(EMU_DIVREM_BOILERPLATE_GEN ${CMAKE_CURRENT_SOURCE_DIR}/emulation_divrem_boilerplate.py)

add_custom_command(OUTPUT ${EMU_DIVREM_BOILERPLATE_FILE}
    COMMAND ${PYTHON_EXECUTABLE} ${EMU_DIVREM_BOILERPLATE_GEN} ${EMU_DIVREM_BOILERPLATE_FILE}
    COMMENT "generation of boilerplate for integer division emulation"
    DEPENDS ${EMU_DIVREM_BOILERPLATE_GEN})

add_custom_target(VCEmuDivremBoilerplate SOURCES ${EMU_DIVREM_BOILERPLATE_FILE})

add_library(VCBifUtils INTERFACE)
target_include_directories(VCBifUtils INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(VCBifUtils VCEmuDivremBoilerplate)
