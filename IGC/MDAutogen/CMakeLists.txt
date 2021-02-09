#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2018-2021 Intel Corporation
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

cmake_minimum_required(VERSION 2.8.12)

set(_autogenScript "${IGC_SOURCE_DIR}/common/autogen.py")
set(_autogenSource "${IGC_SOURCE_DIR}/common/MDFrameWork.h")
set(_autogenTarget "${IGC_CODEGEN_DIR}/MDNodeFunctions.gen")

add_custom_command(OUTPUT "${_autogenTarget}"
                   COMMAND "${PYTHON_EXECUTABLE}" ${_autogenScript} ${_autogenSource} ${_autogenTarget}
                   MAIN_DEPENDENCY ${_autogenSource}
                   DEPENDS ${_autogenScript}
                   COMMENT "Running autogen.py for autogenerating MDFrameWork Implementation.")

add_custom_target(MDAutogen
                  DEPENDS "${_autogenTarget}"
                  COMMENT "Building custom target for Metadata Framework")

set_property(TARGET MDAutogen
             PROPERTY PROJECT_LABEL "MetadataFrameWorkAutogen")
