#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__MetaDataApi
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCMetaDataHelper.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataApi.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SpirMetaDataApi.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PurgeMetaDataUtils.cpp"
  )
set(IGC_BUILD__SRC__Compiler_MetaDataApi
    ${IGC_BUILD__SRC__MetaDataApi}
    PARENT_SCOPE
  )

set(IGC_BUILD__HDR__MetaDataApi
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCMetaDataHelper.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MapList.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataApi.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataApiUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataIterator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataTraits.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataValue.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/SpirMetaDataApi.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/PurgeMetaDataUtils.hpp"
  )
set(IGC_BUILD__HDR__Compiler_MetaDataApi
    ${IGC_BUILD__HDR__MetaDataApi}
    PARENT_SCOPE
  )

igc_sg_register(
    Compiler__MetaDataApi
    "MetaDataApi"
    FILES
      ${IGC_BUILD__SRC__MetaDataApi}
      ${IGC_BUILD__HDR__MetaDataApi}
  )
