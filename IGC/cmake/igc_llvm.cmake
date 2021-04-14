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

include_guard(DIRECTORY)

if(NOT IGC_BUILD__LLVM_SOURCES)
  message(STATUS "[IGC] IGC will take prebuilt LLVM")
  message(STATUS "[IGC] Searching for prebuilt LLVM in ${LLVM_ROOT} and system directories")
  find_package(LLVM ${IGC_OPTION__LLVM_PREFERRED_VERSION} REQUIRED)
  message(STATUS "[IGC] Found LLVM: ${LLVM_DIR}")

  # Tell the build that we are using prebuilds.
  set(IGC_BUILD__LLVM_PREBUILDS ON)
endif()

if(LLVM_LINK_LLVM_DYLIB)
    # LLVM was built and configured in a way that tools (in our case IGC) should be linked
    # against single LLVM dynamic library.
    set(IGC_BUILD__LLVM_LIBS_TO_LINK "LLVM")
    message(STATUS "[IGC] Link against LLVM dynamic library")
else()
    # LLVM was built into multiple libraries (static or shared).
    message(STATUS "[IGC] Link against LLVM static or shared component libs")

    # Link targets/dependencies (in required link order).
    # NOTE: Since the libraries are grouped in the same link group (in GCC/CLANG),
    #       there is no longer need to order in most dependant first manner.
    set(IGC_BUILD__LLVM_LIBS_TO_LINK
        "LLVMipo"
        "LLVMIRReader"
        "LLVMBitWriter"
        "LLVMBinaryFormat"
        "LLVMAsmParser"
        "LLVMBitReader"
        "LLVMLinker"
        "LLVMCodeGen"
        "LLVMScalarOpts"
        "LLVMTransformUtils"
        "LLVMAnalysis"
        "LLVMTarget"
        "LLVMObjCARCOpts"
        "LLVMVectorize"
        "LLVMInstrumentation"
        "LLVMObject"
        "LLVMMCParser"
        "LLVMProfileData"
        "LLVMMC"
        "LLVMCore"
        "LLVMSupport"
        "LLVMDemangle"
        )

    if(LLVM_VERSION_MAJOR GREATER_EQUAL 8)
        list(APPEND IGC_BUILD__LLVM_LIBS_TO_LINK
          "LLVMInstCombine"
          )
    endif()

    if(LLVM_VERSION_MAJOR GREATER_EQUAL 9)
        list(APPEND IGC_BUILD__LLVM_LIBS_TO_LINK
          "LLVMBitstreamReader"
          )
    endif()
endif()

