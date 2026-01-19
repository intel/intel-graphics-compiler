#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGC_BUILD__PROJ__AdaptorOCL       "${IGC_BUILD__PROJ_NAME_PREFIX}AdaptorOCL")
set(IGC_BUILD__PROJ__AdaptorOCL       "${IGC_BUILD__PROJ__AdaptorOCL}" PARENT_SCOPE) # set directly
set(IGC_BUILD__PROJ_LABEL__AdaptorOCL "AdaptorOCL")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_shared/executable_format")

set(IGC_BUILD__SRC__AdaptorOCL
    "${CMAKE_CURRENT_SOURCE_DIR}/UnifyIROCL.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MoveStaticAllocas.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LowerInvokeSIMD.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveConstExprCalls.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/preprocess_spvir/PreprocessSPVIR.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/preprocess_spvir/PromoteSubByte.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/preprocess_spvir/HandleSPIRVDecorations/HandleSpirvDecorationMetadata.cpp"
  )

list(APPEND IGC_BUILD__SRC__AdaptorOCL
    "${CMAKE_CURRENT_SOURCE_DIR}/preprocess_spvir/ConvertUserSemanticDecoratorOnFunctions.cpp"
  )

  # NOTE: Exported functions must be added to final libraries/executables directly.

  set(IGC_BUILD__SRC__IGC_AdaptorOCL
      "${CMAKE_CURRENT_SOURCE_DIR}/dllInterfaceCompute.cpp"
    )

  set(IGC_BUILD__HDR__IGC_AdaptorOCL "")

  list(APPEND IGC_BUILD__SRC__IGC_AdaptorOCL
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/igc_features_and_workarounds_impl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/igc_ocl_device_ctx_impl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/igc_ocl_translation_ctx_impl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/ocl_gen_binary_impl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/ocl_translation_output_impl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/gt_system_info_impl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/platform_impl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/igc_builtins_impl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/igc_options_and_capabilities_impl.cpp"
    ${CIF_SOURCES_EXPORT_ABSOLUTE_PATH})
  list(APPEND IGC_BUILD__HDR__IGC_AdaptorOCL
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/igc_features_and_workarounds_impl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/igc_ocl_device_ctx_impl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/igc_ocl_translation_ctx_impl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/ocl_gen_binary_impl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/ocl_translation_output_impl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/gt_system_info_impl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/platform_impl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/igc_builtins_impl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/impl/igc_options_and_capabilities_impl.h"

    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/igc_features_and_workarounds.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/igc_ocl_device_ctx.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/igc_ocl_translation_ctx.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/ocl_gen_binary.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/ocl_translation_output.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/gt_system_info.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/platform.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/igc_builtins.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ocl_igc_interface/igc_options_and_capabilities.h"
    ${CIF_HEADERS_ABSOLUTE_PATH})

  set(IGC_BUILD__SRC__IGC_AdaptorOCL ${IGC_BUILD__SRC__IGC_AdaptorOCL} PARENT_SCOPE) # set directly
  set(IGC_BUILD__HDR__IGC_AdaptorOCL ${IGC_BUILD__HDR__IGC_AdaptorOCL} PARENT_SCOPE)

# Will be set in IGC's level cmake


set(IGC_BUILD__HDR__AdaptorOCL
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/customApi.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/ProcessFuncAttributes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/LegalizeFunctionSignatures.h"

    "${CMAKE_CURRENT_SOURCE_DIR}/OCL/KernelAnnotations.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/OCL/CommandStream/SamplerTypes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/OCL/CommandStream/SurfaceTypes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/DriverInfoOCL.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Utils/CacheControlsHelper.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UnifyIROCL.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MoveStaticAllocas.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LowerInvokeSIMD.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveConstExprCalls.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/preprocess_spvir/PromoteSubByte.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/preprocess_spvir/HandleSPIRVDecorations/HandleSpirvDecorationMetadata.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/preprocess_spvir/PreprocessSPVIR.h"

    #"${IGC_BUILD__COMMON_COMPILER_DIR}/adapters/d3d10/API/USC_d3d10.h"
    #"${IGC_BUILD__COMMON_COMPILER_DIR}/adapters/d3d10/usc_d3d10_umd.h"
    #"${IGC_BUILD__COMMON_COMPILER_DIR}/adapters/d3d9/api/usc_d3d9.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/ErrorCode.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/SurfaceFormats.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_config.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_debugControl.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_gen7.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_gen8.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_gen9.h"
  )

list(APPEND IGC_BUILD__HDR__AdaptorOCL
    "${CMAKE_CURRENT_SOURCE_DIR}/preprocess_spvir/ConvertUserSemanticDecoratorOnFunctions.h"
  )


if(IGC_BUILD__SPIRV/ENABLED)
  list(APPEND IGC_BUILD__HDR__AdaptorOCL
        "SPIRV/SPIRVconsum.h"
      )
endif()

igc_regex_escape(_reSrcDir "${CMAKE_CURRENT_SOURCE_DIR}")

add_library("${IGC_BUILD__PROJ__AdaptorOCL}" STATIC
    ${IGC_BUILD__SRC__AdaptorOCL}
    ${IGC_BUILD__HDR__AdaptorOCL}
  )

set_target_properties("${IGC_BUILD__PROJ__AdaptorOCL}" PROPERTIES FOLDER "Libraries/Adaptors")

add_dependencies("${IGC_BUILD__PROJ__AdaptorOCL}" "${IGC_BUILD__PROJ__GenISAIntrinsics}")
add_dependencies(${IGC_BUILD__PROJ__AdaptorOCL} SPIRVExtensionsSupportGen)
set_property(TARGET "${IGC_BUILD__PROJ__AdaptorOCL}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__AdaptorOCL}")

if(IGC_BUILD__VC_ENABLED)
  target_link_libraries("${IGC_BUILD__PROJ__AdaptorOCL}"
      PUBLIC
        VCCodeGen
        VCIGCDeps
    )
endif()


igc_sg_register(
    IGC__AdaptorOCL
    "DriverInterface\\OCL"
    FILES
      ${IGC_BUILD__SRC__IGC_AdaptorOCL}
      ${IGC_BUILD__HDR__IGC_AdaptorOCL}
  )
