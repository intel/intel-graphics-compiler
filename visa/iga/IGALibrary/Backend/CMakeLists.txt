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

##################################################################
# common to both encoders
set(IGA_Backend
  ${CMAKE_CURRENT_SOURCE_DIR}/BitProcessor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/BitProcessor.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/DecoderOpts.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/EncoderOpts.hpp
  PARENT_SCOPE
)
set(IGA_Backend_Messages
  ${CMAKE_CURRENT_SOURCE_DIR}/Messages/MessageDecoder.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Messages/MessageDecoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Messages/MessageDecoderHDC.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Messages/MessageDecoderOther.cpp
  PARENT_SCOPE)

##################################################################
# GED encoder
set(IGA_Backend_GED_EncoderOnly
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Encoder.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/GEDBitProcessor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/GEDBitProcessor.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/GEDToIGATranslation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/GEDUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/GEDUtil.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/IGAToGEDTranslation.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Interface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Interface.hpp
  PARENT_SCOPE)


##################################################################
# GED decoder

set(IGA_Backend_GED_Decoder
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Decoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GED/Decoder.hpp
  PARENT_SCOPE
)

##################################################################
# native encoder
set(IGA_Backend_Native
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/Field.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/InstCompactor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/InstCompactor.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/InstDecoder.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/InstEncoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/InstEncoder.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/Interface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/Interface.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Native/MInst.hpp
  PARENT_SCOPE
)
