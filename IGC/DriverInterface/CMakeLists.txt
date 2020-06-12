
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
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/IRUpgrader/UpgraderResourceAccess.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/LoadBuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Patch/patch_parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_media_caps_g8.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/Platform/cmd_parser_g8.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/spp_g8.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/sp_g8.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/gtpin_igc_ocl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/sp_debug.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/util/BinaryStream.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/UnifyIROCL.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/cmc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/MoveStaticAllocas.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/zebin_builder.cpp"
  )


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
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorCommon/IRUpgrader/IRUpgrader.hpp"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/gtpin_igc_ocl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/sp_debug.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/util/BinaryStream.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/sp/zebin_builder.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/MoveStaticAllocas.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/cmc.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/igcmc.h"

    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/ErrorCode.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/SurfaceFormats.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_config.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_debugControl.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_gen7.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_gen8.h"
    "${IGC_BUILD__COMMON_COMPILER_DIR}/API/usc_gen9.h"
  )


  list(APPEND IGC_BUILD__SRC__DriverInterface
              "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/igc_features_and_workarounds_impl.cpp"
              "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/igc_ocl_device_ctx_impl.cpp"
              "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/igc_ocl_translation_ctx_impl.cpp"
              "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/ocl_gen_binary_impl.cpp"
              "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/ocl_translation_output_impl.cpp"
              "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/gt_system_info_impl.cpp"
              "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/impl/platform_impl.cpp"
              ${CIF_SOURCES_EXPORT_ABSOLUTE_PATH}
      )

# Resources.
if(MSVC)
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

igc_regex_escape(_reSrcDir "${CMAKE_CURRENT_SOURCE_DIR}")
igc_regex_escape(_reUscDir "${IGC_BUILD__COMMON_COMPILER_DIR}")

win_static_runtime()
