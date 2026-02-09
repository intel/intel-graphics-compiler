#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2025 Intel Corporation
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
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/FreezeIntDiv.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/ImplicitArgs.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/ProcessFuncAttributes.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/TypesLegalizationPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/LegalizeFunctionSignatures.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/ForceFunctionsToNop.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/DivergentBarrierPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/FastMathConstantHandling.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/MoveStaticAllocas.cpp"
  )

  set(IGC_BUILD__SRC__RAYTRACING
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/MemRegionAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingAddressSpaceAliasAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingIntrinsicAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingIntrinsicResolution.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTArgs.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTBuilder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTStackFormat.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingInterface.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/TraceRayInlineLoweringPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/NewTraceRayInlineLoweringPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/DynamicRayManagementPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/SplitAsyncUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/CrossingAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/TraceRayInlinePrepPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/TraceRayInlineLatencySchedulerPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/OverrideTMaxPass.cpp"
    )
  list(APPEND IGC_BUILD__SRC__DriverInterface ${IGC_BUILD__SRC__RAYTRACING})


set(IGC_BUILD__HDR__DriverInterface
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/customApi.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/DivergentBarrierPass.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/FastMathConstantHandling.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/MoveStaticAllocas.h"

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
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/ConstantsEnums.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/HitGroups.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/MemRegionAnalysis.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingAddressSpaceAliasAnalysis.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingMemoryStyle.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingMemoryStyleXe3Plus.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingPasses.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTArgs.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTBuilder.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RTStackFormat.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/RayTracingInterface.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/CrossingAnalysis.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/SplitAsyncUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/NewTraceRayInlineLoweringPass.h"
    )

  set(IGC_BUILD__HDR__RAYTRACING__API
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/API/API.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/API/BVHInfo.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/API/MemoryStyleEnum.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/API/RayDispatchGlobalData.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/API/AtomicPullSWTileWalkInterface.h"
    )
  set(IGC_BUILD__HDR__RAYTRACING_API__ADT
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/API/ADT/Array.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/API/ADT/UnorderedMap.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/RayTracing/API/ADT/Optional.h"
    )

  set(IGC_BUILD__HDR_LivenessUtils
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/LivenessUtils/MergeAllocas.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/LivenessUtils/AllocationLivenessAnalyzer.cpp"
  )
  list(APPEND IGC_BUILD__HDR__DriverInterface "${IGC_BUILD__HDR_LivenessUtils}")
  set(IGC_BUILD__SRC_LivenessUtils
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/LivenessUtils/MergeAllocas.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/LivenessUtils/AllocationLivenessAnalyzer.h"
  )
  list(APPEND IGC_BUILD__SRC__DriverInterface "${IGC_BUILD__SRC_LivenessUtils}")

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
    IGC__RayTracing__API
    "DriverInterface\\RayTracing\\API"
    FILES
      ${IGC_BUILD__HDR__RAYTRACING__API}
  )
  igc_sg_register(
    IGC__RayTracing__API__ADT
    "DriverInterface\\RayTracing\\API\\ADT"
    FILES
      ${IGC_BUILD__HDR__RAYTRACING_API__ADT}
  )

igc_regex_escape(_reSrcDir "${CMAKE_CURRENT_SOURCE_DIR}")
igc_regex_escape(_reUscDir "${IGC_BUILD__COMMON_COMPILER_DIR}")
