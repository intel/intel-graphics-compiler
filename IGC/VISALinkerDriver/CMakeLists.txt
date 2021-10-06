#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGC_BUILD__PROJ__VISALinkerDriver       "${IGC_BUILD__PROJ_NAME_PREFIX}VISALinkerDriver")
set(IGC_BUILD__PROJ__VISALinkerDriver       "${IGC_BUILD__PROJ__VISALinkerDriver}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__VISALinkerDriver "${IGC_BUILD__PROJ__VISALinkerDriver}")

set(VLD_SOURCE_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/VLD_SPIRVSplitter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/VLD.cpp
)

set(VLD_INCLUDE_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/VLD_SPIRVSplitter.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/VLD.hpp
)

add_library("${IGC_BUILD__PROJ__VISALinkerDriver}" STATIC ${VLD_SOURCE_FILE} ${VLD_INFO_INCLUDE_FILE})

# This is needed for llvm/IR/Attributes.inc: No such file or directory error
add_dependencies("${IGC_BUILD__PROJ__VISALinkerDriver}" "${IGC_BUILD__PROJ__GenISAIntrinsics}")

get_target_property(SPIRV_TOOLS_INCLUDE_DIRECTORIES ${IGC_BUILD__PROJ__SPIRV-Tools} INCLUDE_DIRECTORIES)

target_include_directories("${IGC_BUILD__PROJ__VISALinkerDriver}"
  PRIVATE
  "${SPIRV_TOOLS_INCLUDE_DIRECTORIES}"
)
