#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2021 Intel Corporation
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

# Handle spirv in-tree build.

include_guard(DIRECTORY)

if(IGC_OPTION__USE_KHRONOS_SPIRV_TRANSLATOR)

if(DEFINED SPIRV_TRANSLATOR_SOURCE_DIR)
    if(NOT EXISTS ${SPIRV_TRANSLATOR_SOURCE_DIR})
        message(FATAL_ERROR
            "[LLVM_PATCHER] : SPIRV_TRANSLATOR_SOURCE_DIR was set to \"${SPIRV_TRANSLATOR_SOURCE_DIR}\", "
            "but this directory does not exist")
    endif()
    if(SPIRV_TRANSLATOR_DIR)
        message(FATAL_ERROR
            "[LLVM_PATCHER] : Both SPIRV_TRANSLATOR_SOURCE_DIR and SPIRV_TRANSLATOR_DIR "
            "cmake variables were set to non-empty values. Only one of them can be non-empty. "
            "SPIRV_TRANSLATOR_SOURCE_DIR was set to \"${SPIRV_TRANSLATOR_SOURCE_DIR}\". "
            "SPIRV_TRANSLATOR_DIR was set to \"${SPIRV_TRANSLATOR_DIR}\". "
            "Hint: SPIRV_TRANSLATOR_SOURCE_DIR is path to SPIRV-LLVM-Translator sources. "
            "Hint: SPIRV_TRANSLATOR_DIR is path to SPIRV-LLVM-Translator prebuilt package.")
    endif()
endif()

# Directory where to copy sources
set(SPIRV_TRANSLATOR_IN_LLVM ${IGC_LLVM_SOURCE_DIR}/projects/SPIRV-LLVM-Translator/)
set(SPIRV_TRANSLATOR_IN_LLVM ${SPIRV_TRANSLATOR_IN_LLVM} PARENT_SCOPE)
if(NOT SPIRV_TRANSLATOR_DIR AND NOT DEFINED SPIRV_TRANSLATOR_SOURCE_DIR)
    message(STATUS "[LLVM_PATCHER] : No SPIRV-LLVM-Translator was provided with cmake parameters. "
                   "Hint: Translator can be provided with SPIRV_TRANSLATOR_DIR or SPIRV_TRANSLATOR_SOURCE_DIR cmake parameters. "
                   "Try to find SPIRV-LLVM-Translator in default paths... ")
    find_path(SPIRV_TRANSLATOR_SOURCE_DIR
        "CMakeLists.txt"
        PATHS
        # First, try to search within current repo
        ${SPIRV_TRANSLATOR_IN_LLVM}/
        ${CMAKE_CURRENT_SOURCE_DIR}/../SPIRV-LLVM-Translator
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../SPIRV-LLVM-Translator
        ${CMAKE_CURRENT_SOURCE_DIR}/../llvm-spirv
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../llvm-spirv
        # If not found let's take a look at system paths
        /opt/src/SPIRV-LLVM-Translator
        /opt/src/llvm-spirv
        NO_DEFAULT_PATH)
    if(NOT SPIRV_TRANSLATOR_SOURCE_DIR)
        message(STATUS "[LLVM_PATCHER] : Fail! SPIRV-LLVM-Translator will not be built from sources.")
    else()
        message(STATUS "[LLVM_PATCHER] : Success! Found in \"${SPIRV_TRANSLATOR_SOURCE_DIR}\"")
    endif()
endif()

if(SPIRV_TRANSLATOR_SOURCE_DIR)
    if(NOT EXISTS ${SPIRV_TRANSLATOR_IN_LLVM})
        # Copy stock LLVM-SPIRV Translator to IGC_LLVM_SOURCE_DIR/projects
        message(STATUS "[LLVM_PATCHER] : Copying SPIRV-LLVM-Translator from \"${SPIRV_TRANSLATOR_SOURCE_DIR}\" to \"${SPIRV_TRANSLATOR_IN_LLVM}\"")
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${SPIRV_TRANSLATOR_SOURCE_DIR} ${SPIRV_TRANSLATOR_IN_LLVM} ERROR_VARIABLE SPIRV_COPY_ERROR)
        # cmake 3.19 supports COMMAND_ERROR_IS_FATAL
        if(SPIRV_COPY_ERROR)
            message(FATAL_ERROR "${SPIRV_COPY_ERROR}")
        endif()
    else()
        message(WARNING "[LLVM_PATCHER] : SPIRV-LLVM-Translator will not be copied from \"${SPIRV_TRANSLATOR_SOURCE_DIR}\" to \"${SPIRV_TRANSLATOR_IN_LLVM}\" "
                        "because directory already exists")
    endif()
endif()

endif()
