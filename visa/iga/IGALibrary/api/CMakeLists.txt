#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2019-2021 Intel Corporation
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

set(IGA_API
  ${CMAKE_CURRENT_SOURCE_DIR}/iga.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kv.cpp
  PARENT_SCOPE
)

set(IGA_API_HeadersOpen
  ${CMAKE_CURRENT_SOURCE_DIR}/iga.h
  ${CMAKE_CURRENT_SOURCE_DIR}/iga.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/iga_bxml_ops.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/iga_bxml_enums.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/iga_types_ext.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/iga_types_swsb.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/igad.h
  ${CMAKE_CURRENT_SOURCE_DIR}/igax.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kv.h
  ${CMAKE_CURRENT_SOURCE_DIR}/kv.hpp
)

set(IGA_API_Source
  ${CMAKE_CURRENT_SOURCE_DIR}/iga_bxml_enums.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/iga_types_swsb.cpp
  PARENT_SCOPE
)

set(IGA_API_Headers
  ${IGA_API_HeadersOpen}
  PARENT_SCOPE
  )

set(IGA_API_EncoderInterface
  ${CMAKE_CURRENT_SOURCE_DIR}/igaEncoderWrapper.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/igaEncoderWrapper.cpp
  PARENT_SCOPE
)
