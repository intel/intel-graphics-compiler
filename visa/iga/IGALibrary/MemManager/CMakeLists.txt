#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#IGA/MemManager

set(IGA_MemManager
  ${CMAKE_CURRENT_SOURCE_DIR}/Arena.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Arena.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MemManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MemManager.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/StdArenaAllocator.hpp
  PARENT_SCOPE
)

#add_sources(Arena.cpp  Arena.hpp MemManager.cpp MemManager.hpp StdAreanaAllocator.hpp)
