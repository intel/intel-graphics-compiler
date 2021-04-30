#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__Image3dToImage2darray
    "${CMAKE_CURRENT_SOURCE_DIR}/Image3dToImage2darray.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_Image3dToImage2darray ${IGC_BUILD__SRC__Image3dToImage2darray} PARENT_SCOPE)

set(IGC_BUILD__HDR__Image3dToImage2darray
    "${CMAKE_CURRENT_SOURCE_DIR}/Image3dToImage2darray.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_Image3dToImage2darray ${IGC_BUILD__HDR__Image3dToImage2darray} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_Image3dToImage2darray
    "Image3dToImage2darray"
    FILES
      ${IGC_BUILD__SRC__Image3dToImage2darray}
      ${IGC_BUILD__HDR__Image3dToImage2darray}
  )
