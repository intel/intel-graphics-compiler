#===================== begin_copyright_notice ==================================

#Copyright (c) 2017 Intel Corporation

#Permission is hereby granted, free of charge, to any person obtaining a
#copy of this software and associated documentation files (the
#"Software"), to deal in the Software without restriction, including
#without limitation the rights to use, copy, modify, merge, publish,
#distribute, sublicense, and/or sell copies of the Software, and to
#permit persons to whom the Software is furnished to do so, subject to
#the following conditions:

#The above copyright notice and this permission notice shall be included
#in all copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#======================= end_copyright_notice ==================================

if(EXISTS ${IGC_OPTION__LLVM_PREBUILDS_DIR}/include)
  list(APPEND CMAKE_MODULE_PATH "${IGC_OPTION__LLVM_PREBUILDS_DIR}/lib/cmake/llvm/")
   
  set(LLVM_DIR ${IGC_OPTION__LLVM_PREBUILDS_DIR}/lib/cmake/llvm)  
  include(${IGC_OPTION__LLVM_PREBUILDS_DIR}/lib/cmake/llvm/LLVMConfig.cmake)
  include(${IGC_OPTION__LLVM_PREBUILDS_DIR}/lib/cmake/llvm/AddLLVM.cmake)
  
  find_package(LLVM REQUIRED CONFIG)
  
  set(LLVM_INCLUDE_DIRS "${IGC_OPTION__LLVM_PREBUILDS_DIR}/include")
  
  set(IGC_OPTION__LLVM_PREBUILDS True)
  
  
  message(STATUS "[LLVM_PATCHER\\Prebuilt] : Found prebuilt of llvm in version ${PACKAGE_VERSION}")
endif()


