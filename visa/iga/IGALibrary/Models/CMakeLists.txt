#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================


set(IGA_ModelsOpen
  ${CMAKE_CURRENT_SOURCE_DIR}/Models.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Models.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/OpSpec.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/OpSpec.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/ModelGen7p5.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/ModelGen8.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/ModelGen9.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/ModelGen10.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/ModelGen11.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/ModelXe.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/ModelXeHP.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/ModelXeHPG.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/ModelXeHPC.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bxml/ModelXe2.hpp
)
set(IGA_ModelsExteneded )

set(IGA_Models
  ${IGA_ModelsOpen}
  ${IGA_ModelsExteneded}
  PARENT_SCOPE
)
