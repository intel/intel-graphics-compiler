#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__GenericCastToPtrOpt
    "${CMAKE_CURRENT_SOURCE_DIR}/GenericCastToPtrOpt.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_GenericCastToPtrOpt ${IGC_BUILD__SRC__GenericCastToPtrOpt} PARENT_SCOPE)

set(IGC_BUILD__HDR__GenericCastToPtrOpt
    "${CMAKE_CURRENT_SOURCE_DIR}/GenericCastToPtrOpt.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_GenericCastToPtrOpt ${IGC_BUILD__HDR__GenericCastToPtrOpt} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_GenericCastToPtrOpt
    "GenericCastToPtrOpt"
    FILES
      ${IGC_BUILD__SRC__GenericCastToPtrOpt}
      ${IGC_BUILD__HDR__GenericCastToPtrOpt}
  )

