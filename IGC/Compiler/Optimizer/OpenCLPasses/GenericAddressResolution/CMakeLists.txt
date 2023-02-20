#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__GenericAddressResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/GenericAddressDynamicResolution.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GASResolving.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GASRetValuePropagator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/StaticGASResolution.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LowerGPCallArg.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GASPropagator.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_GenericAddressResolution
    ${IGC_BUILD__SRC__GenericAddressResolution}
    ${IGC_BUILD__SRC__GenericAddressSpaceStaticResolution}
    PARENT_SCOPE
  )

set(IGC_BUILD__HDR__GenericAddressResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/GenericAddressDynamicResolution.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GASResolving.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/GASRetValuePropagator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/StaticGASResolution.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LowerGPCallArg.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/GASPropagator.h"
  )
set(IGC_BUILD__HDR__OpenCLPasses_GenericAddressResolution
    ${IGC_BUILD__HDR__GenericAddressResolution}
    ${IGC_BUILD__HDR__GenericAddressSpaceStaticResolution}
    PARENT_SCOPE
  )


igc_sg_register(
    Compiler__OpenCLPasses_GenericAddressResolution
    "GenericAddressResolution"
    FILES
      ${IGC_BUILD__SRC__GenericAddressResolution}
      ${IGC_BUILD__HDR__GenericAddressResolution}
  )
igc_sg_register(
    Compiler__OpenCLPasses_GenericAddressSpaceStaticResolution
    "GenericAddressSpaceStaticResolution"
    FILES
      ${IGC_BUILD__SRC__GenericAddressSpaceStaticResolution}
      ${IGC_BUILD__HDR__GenericAddressSpaceStaticResolution}
  )
