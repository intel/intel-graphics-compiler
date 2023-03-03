#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGA_Frontend_Formatter
  ${CMAKE_CURRENT_SOURCE_DIR}/Floats.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Floats.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Formatter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Formatter.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/FormatterJSON.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/FormatterJSON2.cpp
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

