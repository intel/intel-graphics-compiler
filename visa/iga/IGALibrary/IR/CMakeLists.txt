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

add_subdirectory(Checker)

set(IGA_IR_Open
  ${CMAKE_CURRENT_SOURCE_DIR}/BitSet.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Block.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Block.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/DUAnalysis.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/DUAnalysis.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/EnumSendOpInfo.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ImmVal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ImmVal.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/InstBuilder.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Instruction.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Instruction.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Kernel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Kernel.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Loc.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Messages.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Messages.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Operand.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Operand.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/RegDeps.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/RegDeps.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/RegSet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/RegSet.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SWSBSetter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SWSBSetter.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Traversals.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Traversals.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Types.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Types.hpp
  ${IGA_IR_Checker}
)

set(IGA_IR
  ${IGA_IR_Open}
  PARENT_SCOPE
)

