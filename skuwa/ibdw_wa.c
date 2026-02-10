/*========================== begin_copyright_notice ============================

Copyright (C) 2012-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"

#define SIWA_ONLY_BDW_A0    SIWA_ONLY_A0
#define SIWA_UNTIL_BDW_A0   SIWA_UNTIL_A0
#define SIWA_FROM_BDW_A0    SIWA_FROM_A0
#define SIWA_AFTER_BDW_A0   SIWA_AFTER_A0

#define SIWA_ONLY_BDW_A1    SIWA_ONLY_A0
#define SIWA_UNTIL_BDW_A1   SIWA_UNTIL_A0
#define SIWA_FROM_BDW_A1    SIWA_FROM_A0
#define SIWA_AFTER_BDW_A1   SIWA_AFTER_A0


#define SIWA_ONLY_BDW_B0    SIWA_ONLY_A3
#define SIWA_UNTIL_BDW_B0   SIWA_UNTIL_A3
#define SIWA_FROM_BDW_B0    SIWA_FROM_A3
#define SIWA_AFTER_BDW_B0   SIWA_AFTER_A3


#define SIWA_ONLY_BDW_D0    SIWA_ONLY_A4
#define SIWA_UNTIL_BDW_D0   SIWA_UNTIL_A4
#define SIWA_FROM_BDW_D0    SIWA_FROM_A4
#define SIWA_AFTER_BDW_D0   SIWA_AFTER_A4

#define SIWA_ONLY_BDW_E0    SIWA_ONLY_A5
#define SIWA_UNTIL_BDW_E0   SIWA_UNTIL_A5
#define SIWA_FROM_BDW_E0    SIWA_FROM_A5
#define SIWA_AFTER_BDW_E0   SIWA_AFTER_A5

#define SIWA_ONLY_BDW_F0    SIWA_ONLY_A6
#define SIWA_UNTIL_BDW_F0   SIWA_UNTIL_A6
#define SIWA_FROM_BDW_F0    SIWA_FROM_A6
#define SIWA_AFTER_BDW_F0   SIWA_AFTER_A6

#define SIWA_ONLY_BDW_G0    SIWA_ONLY_A7
#define SIWA_UNTIL_BDW_G0   SIWA_UNTIL_A7
#define SIWA_FROM_BDW_G0    SIWA_FROM_A7
#define SIWA_AFTER_BDW_G0   SIWA_AFTER_A7

#define SIWA_ONLY_BDW_G1    SIWA_ONLY_A8
#define SIWA_UNTIL_BDW_G1   SIWA_UNTIL_A8
#define SIWA_FROM_BDW_G1    SIWA_FROM_A8
#define SIWA_AFTER_BDW_G1   SIWA_AFTER_A8

#define SIWA_ONLY_BDW_H0    SIWA_ONLY_A9
#define SIWA_UNTIL_BDW_H0   SIWA_UNTIL_A9
#define SIWA_FROM_BDW_H0    SIWA_FROM_A9
#define SIWA_AFTER_BDW_H0   SIWA_AFTER_A9

#define SIWA_ONLY_BDW_J0    SIWA_ONLY_AA
#define SIWA_UNTIL_BDW_J0   SIWA_UNTIL_AA
#define SIWA_FROM_BDW_J0    SIWA_FROM_AA
#define SIWA_AFTER_BDW_J0   SIWA_AFTER_AA


#define SIWA_ONLY_PCH_BDW_A0      SIWA_ONLY_A0
#define SIWA_FROM_PCH_BDW_A0      SIWA_FROM_A0
#define SIWA_UNTIL_PCH_BDW_A0     SIWA_UNTIL_A0
#define SIWA_AFTER_PCH_BDW_A0     SIWA_AFTER_A0


void InitBdwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    unsigned int ulStepId_BDW, ulStepId_PCH;
    (void)ulStepId_PCH;
    int platformForIndirectDispatch = SI_WA_NEVER;

#ifdef __KCH
    KCHASSERT(NULL != pWaParam);
#endif
    ulStepId_BDW = (1 << pWaParam->usRevId);
    ulStepId_PCH = (1 << pWaParam->usRevId_PCH);


    WA_ENABLE(
        ulStepId_BDW,
        WaNoA32ByteScatteredStatelessMessages,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    if (pSkuTable->FtrGT3)
    {

    }


    WA_ENABLE(
        ulStepId_BDW,
        WaThreadSwitchAfterCall,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    WA_ENABLE(
        ulStepId_BDW,
        WaOCLEnableFMaxFMinPlusZero,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);


    WA_ENABLE(
        ulStepId_BDW,
        WaDisableIndirectDataForIndirectDispatch,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        platformForIndirectDispatch);


    WA_ENABLE(
        ulStepId_BDW,
        WaLimitSizeOfSDEPolyFifo,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_UNTIL_BDW_A1);

    if (pWaTable->WaLimitSizeOfSDEPolyFifo)
    {

    }


    if (pSkuTable->FtrGT3)
    {


    }


    if (pSkuTable->FtrGpGpuMidThreadLevelPreempt)
    {

    }


    WA_ENABLE(
        ulStepId_BDW,
        WaForceMinMaxGSThreadCount,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    if (pSkuTable->FtrGT3 || pSkuTable->FtrGT4) {

    }


    WA_ENABLE(
        ulStepId_BDW,
        WaClearArfDependenciesBeforeEot,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


    if (pWaParam->ePCHProductFamily == PCH_LPT)
    {

    }


    if (pSkuTable->FtrIoMmuPageFaulting)
    {
        WA_ENABLE(
            ulStepId_BDW,
            WADisableWriteCommitForPageFault,
            "No Link Provided",
            "No HWSightingLink provided",
            PLATFORM_ALL,
            SIWA_FOREVER);
    }


    WA_ENABLE_NO_PLATFORM_CHECK(
        ulStepId_BDW,
        WaPruneModeWithIncorrectHsyncOffset,
        "No HWBugLink provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SIWA_FOREVER);


}

#ifdef __KCH
void InitBdwHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    unsigned int ulStepId_BDW;
    (void)ulStepId_BDW;

    ulStepId_BDW = (1 << pWaParam->usRevId);


}
#endif
