/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __WA_DEF_H__
#define __WA_DEF_H__

#if defined(__KCH)
    #include "ct.h"
    #include <ntddk.h>
    #include "vidmini.h"
#else // if !defined(__KCH)*/
    #include "../inc/common/sku_wa.h"
    #include "../inc/common/igfxfmid.h"
#endif // if !defined(__KCH)
#include "gtsysinfo.h"

#define SIWA_TRUE               0x00000001    // force enable
#define SIWA_FALSE              0x00000000    // force disable
#define FUTURE_PROJECT          2147483647    // INT_MAX

#define BXT_REV_ID_A0           SI_REV_ID(0,2)
#define BXT_REV_ID_B0           SI_REV_ID(3,3)
#define BXT_REV_ID_B0_PRIME     SI_REV_ID(6,6)
#define BXT_REV_ID_B1           SI_REV_ID(4,4)
#define BXT_REV_ID_B1_PRIME     SI_REV_ID(7,7)
#define BXT_REV_ID_BX           SI_REV_ID(5,5)
#define BXT_REV_ID_BX_PRIME     SI_REV_ID(8,8)
#define BXT_REV_ID_C0           SI_REV_ID(9,9)
#define BXT_REV_ID_CX_B1        SI_REV_ID(10,10) //PRQ Candidate- APL
#define BXT_REV_ID_CX_B2        SI_REV_ID(11,11)
#define BXT_REV_ID_D0           SI_REV_ID(12,12) //PRQ Candidate - BXT
#define BXT_REV_ID_DX           SI_REV_ID(13,13)

///////////////////////////////////////// new definitions SKL+  //////////////////////////////////////
//
// for handling revID upto 16 bit values
//

// BXT onwards, each Stepping like A0 can have more than 1 stepping ID's.
// So we define the highest cap and lower cap of stepping IDs

#define SI_REV_ID(lo,hi) (lo | hi<<16) // LO represents the lowest revision ID for that stepping, HI is the highest for that stepping

#define SI_REV_HI(SteppingID) ((SteppingID & 0xFFFF0000) >> 16)
#define SI_REV_LO(SteppingID) (SteppingID & 0xFFFF)

#define SI_WA_ENABLE(wa, HWBugLink, HWSightingLink, ulPlatformMask, bEnable) \
{ \
    pWaTable->wa = ((pWaParam->ePlatformType & (ulPlatformMask)) || ulPlatformMask == PLATFORM_ALL)  ? bEnable : 0;  \
}

#define SI_WA_ONLY(ulRevID,STEPPING) ((ulRevID <= (int)SI_REV_HI(STEPPING)) && (ulRevID >= (int)SI_REV_LO(STEPPING)))
#define SI_WA_AFTER(ulRevID, STEPPING) (ulRevID > (int)SI_REV_HI(STEPPING))                       // Current Rev ID is higher than the highest Rev ID for that STEPPING
#define SI_WA_BEFORE(ulRevID, STEPPING) (ulRevID < (int)SI_REV_LO(STEPPING))                     // Current Rev ID is lower than the lowest Rev ID for that STEPPING
#define SI_WA_UNTIL(ulRevID, STEPPING) (ulRevID <= (int)SI_REV_HI(STEPPING))                    // Current Rev ID is lower than or equal to the highest Rev ID for that STEPPING
#define SI_WA_FROM(ulRevID, STEPPING) (ulRevID >= (int)SI_REV_LO(STEPPING))                    // Current Rev ID is higher than or equal to the lowest Rev ID for that STEPPING
#define SI_WA_BETWEEN(ulRevID, StepOld, StepNew) ((ulRevID <= (int)SI_REV_HI(StepNew)) && (ulRevID >= (int)SI_REV_LO(StepOld))) // Current Rev ID is between the StepOld and StepNew (inclusive)
#define SI_WA_FOR_EVER (SIWA_TRUE)
#define SI_WA_NEVER (SIWA_FALSE)


///////////////////////////////////////// new definitions SKL+  //////////////////////////////////////

#define SIWA_ONLY_A0            0x0fff0001u
#define SIWA_ONLY_A1            0x0fff0002u
#define SIWA_ONLY_A2            0x0fff0004u
#define SIWA_ONLY_A3            0x0fff0008u
#define SIWA_ONLY_A4            0x0fff0010u
#define SIWA_ONLY_A5            0x0fff0020u
#define SIWA_ONLY_A6            0x0fff0040u
#define SIWA_ONLY_A7            0x0fff0080u
#define SIWA_ONLY_A8            0x0fff0100u
#define SIWA_ONLY_A9            0x0fff0200u
#define SIWA_ONLY_AA            0x0fff0400u
#define SIWA_ONLY_AB            0x0fff0800u
#define SIWA_ONLY_AC            0x0fff1000u

//Definition of SIWA_UNTIL_An:
//  The workaround is to be applied to all revisions UP TO AND INCLUDING An.
#define SIWA_UNTIL_A0            (SIWA_ONLY_A0)
#define SIWA_UNTIL_A1            (SIWA_UNTIL_A0 | SIWA_ONLY_A1)
#define SIWA_UNTIL_A2            (SIWA_UNTIL_A1 | SIWA_ONLY_A2)
#define SIWA_UNTIL_A3            (SIWA_UNTIL_A2 | SIWA_ONLY_A3)
#define SIWA_UNTIL_A4            (SIWA_UNTIL_A3 | SIWA_ONLY_A4)
#define SIWA_UNTIL_A5            (SIWA_UNTIL_A4 | SIWA_ONLY_A5)
#define SIWA_UNTIL_A6            (SIWA_UNTIL_A5 | SIWA_ONLY_A6)
#define SIWA_UNTIL_A7            (SIWA_UNTIL_A6 | SIWA_ONLY_A7)
#define SIWA_UNTIL_A8            (SIWA_UNTIL_A7 | SIWA_ONLY_A8)
#define SIWA_UNTIL_A9            (SIWA_UNTIL_A8 | SIWA_ONLY_A9)
#define SIWA_UNTIL_AA            (SIWA_UNTIL_A9 | SIWA_ONLY_AA)
#define SIWA_UNTIL_AB            (SIWA_UNTIL_AA | SIWA_ONLY_AB)
#define SIWA_UNTIL_AC            (SIWA_UNTIL_AB | SIWA_ONLY_AC)

//Definition of SIWA_AFTER_An:
//  The workaround is to be applied to all revisions AFTER An.
#define SIWA_AFTER_A0            (0x0fff0000 | (~SIWA_UNTIL_A0))
#define SIWA_AFTER_A1            (0x0fff0000 | (~SIWA_UNTIL_A1))
#define SIWA_AFTER_A2            (0x0fff0000 | (~SIWA_UNTIL_A2))
#define SIWA_AFTER_A3            (0x0fff0000 | (~SIWA_UNTIL_A3))
#define SIWA_AFTER_A4            (0x0fff0000 | (~SIWA_UNTIL_A4))
#define SIWA_AFTER_A5            (0x0fff0000 | (~SIWA_UNTIL_A5))
#define SIWA_AFTER_A6            (0x0fff0000 | (~SIWA_UNTIL_A6))
#define SIWA_AFTER_A7            (0x0fff0000 | (~SIWA_UNTIL_A7))
#define SIWA_AFTER_A8            (0x0fff0000 | (~SIWA_UNTIL_A8))
#define SIWA_AFTER_A9            (0x0fff0000 | (~SIWA_UNTIL_A9))
#define SIWA_AFTER_AA            (0x0fff0000 | (~SIWA_UNTIL_AA))
#define SIWA_AFTER_AB            (0x0fff0000 | (~SIWA_UNTIL_AB))
#define SIWA_AFTER_AC            (0x0fff0000 | (~SIWA_UNTIL_AC))

//Definition of SIWA_FROM_An:
//  The workaround is to be applied to all revisions AFTER An AND An.
#define SIWA_FROM_A0            (SIWA_AFTER_A0 | SIWA_ONLY_A0)
#define SIWA_FROM_A1            (SIWA_AFTER_A1 | SIWA_ONLY_A1)
#define SIWA_FROM_A2            (SIWA_AFTER_A2 | SIWA_ONLY_A2)
#define SIWA_FROM_A3            (SIWA_AFTER_A3 | SIWA_ONLY_A3)
#define SIWA_FROM_A4            (SIWA_AFTER_A4 | SIWA_ONLY_A4)
#define SIWA_FROM_A5            (SIWA_AFTER_A5 | SIWA_ONLY_A5)
#define SIWA_FROM_A6            (SIWA_AFTER_A6 | SIWA_ONLY_A6)
#define SIWA_FROM_A7            (SIWA_AFTER_A7 | SIWA_ONLY_A7)
#define SIWA_FROM_A8            (SIWA_AFTER_A8 | SIWA_ONLY_A8)
#define SIWA_FROM_A9            (SIWA_AFTER_A9 | SIWA_ONLY_A9)
#define SIWA_FROM_AA            (SIWA_AFTER_AA | SIWA_ONLY_AA)
#define SIWA_FROM_AB            (SIWA_AFTER_AB | SIWA_ONLY_AB)
#define SIWA_FROM_AC            (SIWA_AFTER_AC | SIWA_ONLY_AC)

#define SIWA_FOREVER            0xffffffff

#define SI_REV_A0                (SIWA_ONLY_A0 & 0xffff)
#define SI_REV_A1                (SIWA_ONLY_A1 & 0xffff)
#define SI_REV_A2                (SIWA_ONLY_A2 & 0xffff)
#define SI_REV_A3                (SIWA_ONLY_A3 & 0xffff)
#define SI_REV_A4                (SIWA_ONLY_A4 & 0xffff)
#define SI_REV_A5                (SIWA_ONLY_A5 & 0xffff)
#define SI_REV_A6                (SIWA_ONLY_A6 & 0xffff)
#define SI_REV_A7                (SIWA_ONLY_A7 & 0xffff)
#define SI_REV_A8                (SIWA_ONLY_A8 & 0xffff)
#define SI_REV_A9                (SIWA_ONLY_A9 & 0xffff)
#define SI_REV_AA                (SIWA_ONLY_AA & 0xffff)

#ifndef __S_INLINE
    #define __S_INLINE static __inline
#endif

// WA Init parameters
typedef struct _WaInitParam
{
    unsigned short   usRevId;
    unsigned short   usRevId_PCH;
    PLATFORM_TYPE    ePlatformType;
    PCH_PRODUCT_FAMILY  ePCHProductFamily;
    const GT_SYSTEM_INFO   *pGtSysInfo;           // Required CNL+
    unsigned char    bWinDoD;  // BOOLEAN
} WA_INIT_PARAM, *PWA_INIT_PARAM;

/********* Common functions to intialize wa table ************\

    MACRO / Inline Function usage:

    1. To enable any WA based on both Platform and stepping type use the PLATFORM_STEP_APPLICABLE() macro
        e.g. pSkuTable->WAXYZ   = PLATFORM_STEP_APPLICABLE(ulStepId, PLATFORM_DESKTOP, SIWA_FOREVER);

    2. If the WaBoolean() is called directly it will be enabled on ALL platforms

    3. To enable any WA based on some boolean flag and platform use the following:
        e.g. pSkuTable->WAXYZ   = PLATFORM_STEP_APPLICABLE(ulStepId, PLATFORM_DESKTOP, bFlag);

\*************************************************************/
//Function to return TRUE or FALSE based on rev id and wa mask
__S_INLINE unsigned int WaBoolean(unsigned int ulStepId, unsigned int ulWaMask)
{
    //Check whether wa forced TRUE/FALSE
    if (ulWaMask < SIWA_UNTIL_A0)
    {
        return (ulWaMask != 0);
    }

    //Check if wa mask contains current rev id
    return ((ulStepId & ulWaMask) ? 1 : 0);
}


// Macro to call the WaBoolean function based on platformType
#define PLATFORM_STEP_APPLICABLE(ulStepId, ulPlatformMask, ulWaMask) \
    (pWaParam->ePlatformType & (ulPlatformMask)) ? WaBoolean((ulStepId), (ulWaMask)) : 0;


#define WA_ENABLE_NO_PLATFORM_CHECK( ulStepId, wa, HWBugLink, HWSightingLink, ulPlatformMask, ulWaMask )  \
{                                                                                                         \
    pWaTable->wa = WaBoolean(ulStepId, ulWaMask);                                                         \
}

#define WA_ENABLE( ulStepId, wa, HWBugLink, HWSightingLink, ulPlatformMask, ulWaMask )                    \
{                                                                                                         \
    pWaTable->wa = ((pWaParam->ePlatformType & (ulPlatformMask)) || ulPlatformMask == PLATFORM_ALL)  ? WaBoolean((ulStepId), (ulWaMask)) : 0;  \
}

#ifdef __cplusplus
extern "C" {
#endif

void InitBdwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitChvWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitSklWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitBxtWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitKblNonDisplayWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitKblDisplayWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitCflNonDisplayWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitGlkWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitGlvWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitGwlWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitCnlWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitIclHwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitIclSwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitIclLpHwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitIclLpSwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitLkfHwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitLkfSwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitEhlHwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitEhlSwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitJslHwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitJslSwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitTglLpHwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitTglLpSwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitDg1HwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitDg1SwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitRklHwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitRklSwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAdlsHwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAdlsSwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAdlpHwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAdlpSwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitXeHPSDVHwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitXeHPSDVSwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAcm_G10HwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAcm_G11HwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAcm_G11SwWaTable(
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAcm_G10SwWaTable(
       PWA_TABLE                       pWaTable,
       PSKU_FEATURE_TABLE              pSkuTable,
       PWA_INIT_PARAM                  pWaParam);
void InitPvcHwWaTable(
       PWA_TABLE                       pWaTable,
       PSKU_FEATURE_TABLE              pSkuTable,
       PWA_INIT_PARAM                  pWaParam);
void InitPvc_XtHwWaTable(
       PWA_TABLE                       pWaTable,
       PSKU_FEATURE_TABLE              pSkuTable,
       PWA_INIT_PARAM                  pWaParam);
#ifdef __cplusplus
} // extern "C"
#endif

#if defined(__KCH)
void InitChvHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitChvSLEWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitBxtHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitGlkHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitGlvHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitGwlHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitSklHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitKblHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitCflHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitCnlHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitIclHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitIclLpHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitLkfHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitEhlHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitJslHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitTgllpHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitDg1HASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitRklHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAdlsHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAdlpHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitXeHPSDVHASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitDg1HASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
void InitAcm_G10HASWaTable(
        PHW_DEVICE_EXTENSION            pKchContext,
        PWA_TABLE                       pWaTable,
        PSKU_FEATURE_TABLE              pSkuTable,
        PWA_INIT_PARAM                  pWaParam);
#endif // #if defined(__KCH)

#endif // #ifndef __WA_DEF_H__
