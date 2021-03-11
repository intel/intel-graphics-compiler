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

set(IGA_Frontend_Formatter
  ${CMAKE_CURRENT_SOURCE_DIR}/Floats.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Floats.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Formatter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Formatter.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/FormatterJSON.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/FormatterJSON.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/IRToString.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/IRToString.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SendDescriptorDecoding.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SendDescriptorDecoding.hpp
  PARENT_SCOPE
)

# The parser component is optional since it requires exceptions and IGC
# has exceptions disabled.  Hence we split these logically
set(IGA_Frontend_Parser
  ${CMAKE_CURRENT_SOURCE_DIR}/BufferedLexer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/KernelParser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/KernelParser.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Lexemes.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Parser.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lex.yy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lex.yy.hpp
  PARENT_SCOPE
)

