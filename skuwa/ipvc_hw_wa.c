/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.


#include "wa_def.h"
#include "ipvc_rev_id.h"


void InitPvcHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_PVC_CT = (int)pWaParam->usRevId & 0b111;
    int iStepId_PVC_BD = ( (int)pWaParam->usRevId & 0b111000 ) >> 3;

    if (pWaParam->usDeviceID >= 0x0BE5)
    {


        iStepId_PVC_CT |= 0b1000;
        iStepId_PVC_BD |= 0b1000;
    }

    if (iStepId_PVC_CT == 0x1)
    {


        iStepId_PVC_CT = 0x0;
    }


    SI_WA_ENABLE(
        Wa_1507979211,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0P, PVC_GT_REV_ID_CTXT_A0));


    SI_WA_ENABLE(
        Wa_14010017096,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0P, PVC_GT_REV_ID_CTXT_A0));


    SI_WA_ENABLE(
        Wa_1608127078,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0P, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_1807084924,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0P, PVC_GT_REV_ID_CTXT_A0));


    SI_WA_ENABLE(
        Wa_14012420496,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0P, PVC_GT_REV_ID_CTXT_A0));


    SI_WA_ENABLE(
        Wa_14012437816,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0, PVC_GT_REV_ID_CTXT_A0));


    SI_WA_ENABLE(
        Wa_14013341720,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0P, PVC_GT_REV_ID_CTXT_B0));


    SI_WA_ENABLE(
        Wa_14013677893,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0, PVC_GT_REV_ID_CTXT_B0));


    SI_WA_ENABLE(
        Wa_16011698357,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CTXT_A0, PVC_GT_REV_ID_CTXT_B0));


    SI_WA_ENABLE(
        Wa_16012383669,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0P, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_16012725276,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CTXT_A0, PVC_GT_REV_ID_CTXT_B0));


    SI_WA_ENABLE(
        Wa_16013338947,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CTXT_A0, FUTURE_PROJECT));


    SI_WA_ENABLE(
        Wa_22010487853,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0P, PVC_GT_REV_ID_CTXT_A0));


    SI_WA_ENABLE(
        Wa_22010493955,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0P, PVC_GT_REV_ID_CTXT_A0));


    SI_WA_ENABLE(
        Wa_22010725011,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CT_A0P, PVC_GT_REV_ID_CTXT_A0));


    SI_WA_ENABLE(
        Wa_22013689345,
        "No Link Provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_PVC_CT, PVC_GT_REV_ID_CTXT_A0, PVC_GT_REV_ID_CTXT_C0));


}
