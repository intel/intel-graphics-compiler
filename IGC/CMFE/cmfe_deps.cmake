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

# Module to provide useful paths to CM FE library.
# Defines CMFE_INCLUDE_DIRS and INSTALL_CMFE_NAME that
# are used to provide interface library.

set(CMFE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorCM")

if (WIN32 OR CYGWIN)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(INSTALL_CMFE_NAME "clangFEWrapper.dll")
  else()
    set(INSTALL_CMFE_NAME "clangFEWrapper32.dll")
  endif()
else()
  set(INSTALL_CMFE_NAME "libclangFEWrapper.so")
endif()
