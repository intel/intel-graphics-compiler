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



if(LLVM_ON_WIN32)
  if (MSVC_IDE)
    add_compile_options(/experimental:external)
    foreach(INCDIR ${LLVM_INCLUDE_DIRS})
      add_compile_options("SHELL:/external:I ${INCDIR}")
    endforeach()
    add_compile_options(/external:W0)
  endif()

  # disable 32/64 warnings
  add_compile_options(/wd4244)

  #  disable unary minus to unsigned type warning
  add_compile_options(/wd4146)

  # disable implicitly deleted dtor warning
  add_compile_options(/wd4624)
endif()

add_library(AdaptorCM INTERFACE)

igc_get_llvm_targets(LLVM_LIBS
  Support
)

message(STATUS "Linking CMFE with ${LLVM_LIBS}")

target_link_libraries(AdaptorCM
  INTERFACE
  ${LLVM_LIBS}
)

target_compile_definitions(AdaptorCM
  INTERFACE
  "-DCMFE_WRAPPER_NAME=\"${INSTALL_CMFE_NAME}\""
  )

target_include_directories(AdaptorCM INTERFACE "${CMFE_INCLUDE_DIRS}")
