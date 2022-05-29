#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${IGC_BUILD__IGC_SRC_DIR}")

set(IGC_BUILD__SRC__DriverInterface
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/AddImplicitArgs.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/customApi.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/dllInterfaceCompute.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/ImplicitArgs.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/ProcessFuncAttributes.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/TypesLegalizationPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/LegalizeFunctionSignatures.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/LoadBuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Patch/patch_parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_media_caps_g8.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_parser_g8.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/spp_g8.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/sp_g8.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/sp_debug.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/util/BinaryStream.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/UnifyIROCL.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/MoveStaticAllocas.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/zebin_builder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/LowerInvokeSIMD.cpp"
  )

if(IGC_OPTION__USE_KHRONOS_SPIRV_TRANSLATOR_IN_SC)
  list(APPEND IGC_BUILD__SRC__DriverInterface
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/preprocess_spvir/PreprocessSPVIR.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/preprocess_spvir/ConvertUserSemanticDecoratorOnFunctions.cpp"
    )
endif()

  set(IGC_BUILD__SRC__RAYTRACING
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/MemRegionAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingAddressSpaceAliasAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingIntrinsicAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingIntrinsicResolution.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTArgs.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTBuilder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTLoggingManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTStackFormat.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingPredicatedStackIDReleasePass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingInterface.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/PromoteToScratchPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/EarlyRematPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/LateRematPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayInfoCSEPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingIntrinsicLoweringPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/StackIDRetirement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/StackFrameInfo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/ContinuationUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/TraceRayInlineLoweringPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/PrivateToGlobalPointerPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/LowerIntersectionAnyHit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/KernelArgLoweringPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/SplitPreparePass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/SplitAsyncPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/SplitAsyncUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingFinalizePass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/InlineMergeCalls.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingPrintfPostProcess.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/AllocaTracking.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/PrologueShaders.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/PayloadSinkingPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/DeadPayloadStoreEliminationPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/ShaderProperties.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/Interval.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/CrossingAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/TraceRayInlinePrepPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/TraceRayInlineLatencySchedulerPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/FuseContinuations.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/Utils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/InlineDataPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/LowerGlobalRootSignaturePass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingConstantCoalescingPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingMemDSEPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/StackIDSchedulingPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTSpillShrinkPass.cpp"
    )
  list(APPEND IGC_BUILD__SRC__DriverInterface ${IGC_BUILD__SRC__RAYTRACING})

if(IGC_BUILD__SPIRV_ENABLED)
  list(APPEND IGC_BUILD__SRC__DriverInterface
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/libSPIRV/SPIRVBasicBlock.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/libSPIRV/SPIRVDebug.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/libSPIRV/SPIRVDecorate.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/libSPIRV/SPIRVEntry.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/libSPIRV/SPIRVFunction.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/libSPIRV/SPIRVInstruction.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/libSPIRV/SPIRVModule.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/libSPIRV/SPIRVStream.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/libSPIRV/SPIRVType.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/libSPIRV/SPIRVValue.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/SPIRVReader.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/SPIRV/SPIRVUtil.cpp"
    )
endif(IGC_BUILD__SPIRV_ENABLED)


set(IGC_BUILD__HDR__DriverInterface
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/customApi.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/KernelAnnotations.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/CommandStream/SamplerTypes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/CommandStream/SurfaceTypes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Patch/patch_parser.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_3d_def_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_3d_enum_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_3d_init_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_enum.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_enum_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_init_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_media_caps_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_media_def_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_media_enum_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_media_init_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_mi_def_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_mi_enum_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_mi_init_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_parser_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_shared_def_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_shared_enum_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_shared_init_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/sp_convert_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/sp_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/sp_types.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/spp_g8.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/sp_debug.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/util/BinaryStream.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/zebin_builder.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/MoveStaticAllocas.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/LowerInvokeSIMD.hpp"

    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/ErrorCode.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/SurfaceFormats.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_config.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_debugControl.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_gen7.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_gen8.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_gen9.h"
  )

  set(IGC_BUILD__HDR__RAYTRACING
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/HitGroups.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/MemRegionAnalysis.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingAddressSpaceAliasAnalysis.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingConstantsEnums.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingPasses.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingRayDispatchGlobalData.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTArgs.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTBuilder.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTCompileOptions.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTLoggingManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTStackFormat.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingInterface.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/StackFrameInfo.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/ContinuationUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/AllocaTracking.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/PrologueShaders.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/ShaderProperties.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/Interval.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/CrossingAnalysis.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/SplitAsyncUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/FuseContinuations.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/Utils.h"
    )

  set(IGC_BUILD__HDR__RAYTRACING_ADT
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/ADT/Array.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/ADT/Optional.h"
    )

list(APPEND IGC_BUILD__SRC__DriverInterface
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/igc_features_and_workarounds_impl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/igc_ocl_device_ctx_impl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/igc_ocl_translation_ctx_impl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/ocl_gen_binary_impl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/ocl_translation_output_impl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/gt_system_info_impl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/platform_impl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/igc_builtins_impl.cpp"
  ${CIF_SOURCES_EXPORT_ABSOLUTE_PATH}
  )


if (DX_ONLY OR VK_ONLY)
    set(IGC_BUILD__SRC__RAYTRACING_DX
        ${IGC_BUILD__SRC__RAYTRACING}
        PARENT_SCOPE
    )
    set(IGC_BUILD__HDR__RAYTRACING_DX
        ${IGC_BUILD__HDR__RAYTRACING}
        PARENT_SCOPE
    )
endif()

# Resources.
if(MSVC AND NOT (DX_ONLY OR VK_ONLY))
    igc_rc_register_resource(
        DriverInterface__igc_dll
        FILE                "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Resource/BuiltinResource.rc"
        INCLUDE_DIRECTORIES "${IGC_BUILD__BIF_DIR}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL"
      )
endif()


set(IGC_BUILD__SRC__IGC_DriverInterface__igc_dll
    ${IGC_BUILD__SRC__DriverInterface}
    ${IGC_BUILD__SRC_OGL__DriverInterface}
    PARENT_SCOPE
  )

set(IGC_BUILD__HDR__IGC_DriverInterface__igc_dll
    ${IGC_BUILD__HDR__DriverInterface}
    ${IGC_BUILD__HDR_OGL__DriverInterface}
    PARENT_SCOPE
  )
  igc_sg_register(
    IGC__RayTracing
    "DriverInterface\\RayTracing"
    FILES
      ${IGC_BUILD__SRC__RAYTRACING}
      ${IGC_BUILD__HDR__RAYTRACING}
  )
  igc_sg_register(
    IGC__RayTracing__ADT
    "DriverInterface\\RayTracing\\ADT"
    FILES
      ${IGC_BUILD__HDR__RAYTRACING_ADT}
  )

igc_regex_escape(_reSrcDir "${CMAKE_CURRENT_SOURCE_DIR}")
igc_regex_escape(_reUscDir "${IGC_BUILD__COMMON_COMPILER_DIR}")
