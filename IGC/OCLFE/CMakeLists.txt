#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

#---define Dir macros---
if (MSVC)
  set(HEADER_DIR windows)
else()
  set(HEADER_DIR linux)
endif()

set(REL_GFX_DEV_PATH ${IGC_SOURCE_DIR}/../)

set(COMMON_OCL_FILES ${REL_GFX_DEV_PATH}/OpenCL/Common/)
set(CLELF_SOURCE ${COMMON_OCL_FILES}/CLElfLib/)
set(COMMON_INC_FILES ${REL_GFX_DEV_PATH}/inc/common/)
set(SOURCE_INC_FILES ${REL_GFX_DEV_PATH}/inc/)
set(SOURCE_INSTALL_FILES ${REL_GFX_DEV_PATH}/install/)

set(PLATFORM_HEADERS ${REL_GFX_DEV_PATH}/Source/OpenCL/Runtime/${HEADER_DIR}/headers/)

#---End #define Dir macros---

#----Set Source and Header Files----
set(IGC_BUILD__SRC__FCL
  "${CMAKE_CURRENT_SOURCE_DIR}/igd_fcl_mcl/source/clang_dll.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/igd_fcl_mcl/source/clang_tb.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/igd_fcl_mcl/source/clang_debug.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/igd_fcl_mcl/source/LoadBuffer.cpp"
  "${IGC_BUILD__SRC__IGC_Common_CLElfLib}"
)

set(IGC_BUILD__HDR__FCL
  "${CMAKE_CURRENT_SOURCE_DIR}/igd_fcl_mcl/headers/clang_tb.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/igd_fcl_mcl/headers/clang_debug.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/igd_fcl_mcl/headers/resource.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/igd_fcl_mcl/headers/LoadBuffer.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/TranslationBlock.h"
  "${IGC_BUILD__HDR__IGC_Common_CLElfLib}"
)

list(APPEND IGC_BUILD__SRC__FCL ${CIF_SOURCES_EXPORT_ABSOLUTE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/fcl_ocl_translation_ctx_impl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/fcl_ocl_device_ctx_impl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/ocl_translation_output_impl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/platform_impl.cpp"
  )
list(APPEND IGC_BUILD__HDR__FCL ${CIF_HEADERS_ABSOLUTE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/fcl_ocl_translation_ctx_impl.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/fcl_ocl_device_ctx_impl.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/ocl_translation_output_impl.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/platform_impl.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/fcl_ocl_translation_ctx.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/fcl_ocl_device_ctx.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/ocl_translation_output.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/platform.h"
  )

set(IGC_BUILD__SRC__FCL ${IGC_BUILD__SRC__FCL} PARENT_SCOPE)
set(IGC_BUILD__HDR__FCL ${IGC_BUILD__HDR__FCL} PARENT_SCOPE)

if(LLVM_ON_UNIX)
add_subdirectory(embedder)
set(IGC_BUILD__PROJ__CTHLib_FCL "${IGC_BUILD__PROJ__CTHLib_FCL}" PARENT_SCOPE)
endif()

set(IGC_BUILD__RES__FCL
  "${CMAKE_CURRENT_SOURCE_DIR}/igd_fcl_mcl/resources/ClangResource.rc"
  )
set(IGC_BUILD__RES__FCL ${IGC_BUILD__RES__FCL} PARENT_SCOPE)

source_group(External
  FILES
    ${COMMON_OCL_FILES}/TranslationBlock.h
  )

source_group(Headers  FILES
  ${IGC_BUILD__HDR__FCL}
  )

source_group(Resource FILES
  ${IGC_BUILD__RES__FCL}
  )

source_group(Source FILES
    ${IGC_BUILD__SRC__FCL}
  )

###########################################

include_directories(
"${CMAKE_CURRENT_SOURCE_DIR}"
"../../../Source/OpenCL/Common"
"../../../Source/inc"
"../../../Source/inc/common "
"../../../Source/install/"
"../../../Source/OpenCL/Runtime/${HEADER_DIR}/headers/"
)
