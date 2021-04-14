/*****************************************************************************
** Copyright (c) Intel Corporation (2012-2020).                              *
**                                                                           *
** INTEL MAKES NO WARRANTY OF ANY KIND REGARDING THE CODE.  THIS CODE IS     *
** LICENSED ON AN "AS IS" BASIS AND INTEL WILL NOT PROVIDE ANY SUPPORT,      *
** ASSISTANCE, INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL DOES NOT     *
** PROVIDE ANY UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY      *
** DISCLAIMS ANY WARRANTY OF MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR   *
** ANY PARTICULAR PURPOSE, OR ANY OTHER WARRANTY.  Intel disclaims all       *
** liability, including liability for infringement of any proprietary        *
** rights, relating to use of the code. No license, express or implied, by   *
** estoppel or otherwise, to any intellectual property rights is             *
** granted herein.                                                           *
**                                                                           *
** Contains Workaround table initializers for XeHP_SDV                       *
**                                                                           *
** AUTO-GENERATED FILE. DO NOT EDIT.                                         *
**                                                                           *
*****************************************************************************/

#include "wa_def.h"

#define XE_HP_SDV_GT_REV_ID_A0   SI_REV_ID(0,0)

//******************* Main Wa Initializer for XeHP_SDV    ********************

void InitXeHPSDVHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_XeHP_SDV = (int)pWaParam->usRevId;

    // Components affected: igc 
    SI_WA_ENABLE(
        Wa_14013341720,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_XeHP_SDV, XE_HP_SDV_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc 
    SI_WA_ENABLE(
        Wa_16011859583,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_XeHP_SDV, XE_HP_SDV_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc 
    SI_WA_ENABLE(
        Wa_22010811838,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_XeHP_SDV, XE_HP_SDV_GT_REV_ID_A0, FUTURE_PROJECT));

}
