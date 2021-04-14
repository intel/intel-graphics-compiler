/*****************************************************************************
** Copyright (c) Intel Corporation (2013-2016).                              *
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
** Contains Workaround table initializers for XeHP_SDV.                      *
**                                                                           *
*****************************************************************************/
#include "wa_def.h"

#define XE_HP_SDV_GT_REV_ID_A0   SI_REV_ID(0,0)

//******************* Main Wa Initializer for Device Id ********************
// Initialize COMMON/DESKTOP/MOBILE WA using PLATFORM_STEP_APPLICABLE() macro.

void InitXeHPSDVSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_XeHP_SDV = (int)pWaParam->usRevId;
#ifdef __KCH
    // compilation issue with UTF: KCHASSERT(NULL != pWaParam);
#endif

    //=================================================================================================================
    //
    //              XeHP_SDV SW WA for all platforms
    //
    //=================================================================================================================
}

#ifdef __KCH
void InitXeHPSDVHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
}
#endif // __KCH
