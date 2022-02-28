#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGC_BUILD__PROJ__Compiler       "${IGC_BUILD__PROJ_NAME_PREFIX}Compiler")
set(IGC_BUILD__PROJ__Compiler       "${IGC_BUILD__PROJ__Compiler}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__Compiler "${IGC_BUILD__PROJ__Compiler}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

if(MSVC)
add_compile_options(/w14701) #Unintialized Variable Warning
else()
add_compile_options(-Wuninitialized) #Unintialized Variable Warning
endif()

add_subdirectory(CISACodeGen)
igc_sg_define(Compiler__CISACodeGen)

add_subdirectory(DebugInfo)
igc_sg_define(Compiler__DebugInfo)

add_subdirectory(Legalizer)
igc_sg_define(Compiler__Legalizer)

add_subdirectory(MetaDataApi)
igc_sg_define(Compiler__MetaDataApi)

add_subdirectory(Optimizer)
igc_sg_define(Compiler__Optimizer)

set(IGC_BUILD__SRC__Compiler
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenContext.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenContextWrapper.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler_caps.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CustomSafeOptPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CustomUnsafeOptPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CustomLoopOpt.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/BlendToDiscard.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PromoteResourceToDirectAS.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PromoteStatelessToBindless.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenTTI.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FixInvalidFuncNamePass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FixResourcePtr.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FixLdOffsets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/HandleLoadStoreInstructions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_workaround.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LegalizationPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LowPrecisionOptPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataUtilsWrapper.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataUtilsWrapperInitializer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SampleCmpToDiscard.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SPIRMetaDataTranslation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ACLPrintfTranslation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WorkaroundAnalysisPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CPSMSAAOMaskWA.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ThreadCombining.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenUpdateCB.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ConvertMSAAPayloadTo16Bit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MSAAInsertDiscard.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/DynamicTextureFolding.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SampleMultiversioning.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/HandleFRemInstructions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenRotate.cpp"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ibdw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ichv_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ibxt_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iCnl_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iskl_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ikbl_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/icfl_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iglk_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iglv_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iicl_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iicl_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iicl_lp_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iicl_lp_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ilkf_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ilkf_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/itgl_lp_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/itgl_lp_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/idg1_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/idg1_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ixehp_sdv_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ixehp_sdv_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iacm_g10_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iacm_g10_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iacm_g11_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iacm_g11_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ipvc_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ipvc_xt_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/irkl_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/irkl_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iadls_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iadls_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iadlp_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iadlp_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iehl_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/iehl_sw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ijsl_hw_wa.c"
    "${IGC_BUILD__GFX_DEV_SRC_DIR}/skuwa/ijsl_sw_wa.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/SamplerPerfOptPass.cpp"
    ${IGC_BUILD__SRC__Compiler_CISACodeGen}
    ${IGC_BUILD__SRC__Compiler_DebugInfo}
    ${IGC_BUILD__SRC__Compiler_Legalizer}
    ${IGC_BUILD__SRC__Compiler_MetaDataApi}
    ${IGC_BUILD__SRC__Compiler_Optimizer}
  )


if(NOT ANDROID)
  set(IGC_BUILD__SRC_Win_Lin_Dar__Compiler
      "${CMAKE_CURRENT_SOURCE_DIR}/VerificationPass.cpp"
    )
endif()

set(IGC_BUILD__HDR__Compiler
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenContextWrapper.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenPublic.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenPublicEnums.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler_caps.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/CustomSafeOptPass.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CustomUnsafeOptPass.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CustomLoopOpt.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PromoteResourceToDirectAS.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/PromoteStatelessToBindless.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenTTI.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/FixInvalidFuncNamePass.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FixResourcePtr.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FixLdOffsets.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/HandleLoadStoreInstructions.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IGC_IR_spec.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/igc_workaround.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCPassSupport.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InitializePasses.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/LegalizationPass.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LowPrecisionOptPass.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataUtilsWrapper.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MetaDataUtilsWrapperInitializer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/SampleCmpToDiscard.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/SPIRMetaDataTranslation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ACLPrintfTranslation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ThreadCombining.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenUpdateCB.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ConvertMSAAPayloadTo16Bit.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MSAAInsertDiscard.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WorkaroundAnalysisPass.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/CPSMSAAOMaskWA.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/DynamicTextureFolding.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/SampleMultiversioning.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/HandleFRemInstructions.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GenRotate.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SamplerPerfOptPass.hpp"
    ${IGC_BUILD__HDR__Compiler_CISACodeGen}
    ${IGC_BUILD__HDR__Compiler_DebugInfo}
    ${IGC_BUILD__HDR__Compiler_Legalizer}
    ${IGC_BUILD__HDR__Compiler_MetaDataApi}
    ${IGC_BUILD__HDR__Compiler_Optimizer}
  )
if(NOT ANDROID)
  set(IGC_BUILD__HDR_Win_Lin_Dar__Compiler
      "${CMAKE_CURRENT_SOURCE_DIR}/VerificationPass.hpp"
    )
endif()


add_library("${IGC_BUILD__PROJ__Compiler}" STATIC
    ${IGC_BUILD__SRC__Compiler}
    ${IGC_BUILD__SRC_Win_Lin_Dar__Compiler}
    ${IGC_BUILD__HDR__Compiler}
    ${IGC_BUILD__HDR_Win_Lin_Dar__Compiler}
  )
set_property(TARGET "${IGC_BUILD__PROJ__Compiler}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__Compiler}")

add_dependencies("${IGC_BUILD__PROJ__Compiler}" "${IGC_BUILD__PROJ__GenISAIntrinsics}")
