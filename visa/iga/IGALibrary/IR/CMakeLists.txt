#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
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

