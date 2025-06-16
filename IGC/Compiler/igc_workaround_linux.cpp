/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "igc_workaround.h"
#include <string.h>
#include <stdlib.h>
#include "Probe/Assertion.h"

namespace IGC
{

    void SetWorkaroundTable(SKU_FEATURE_TABLE* pSkuFeatureTable, CPlatform* platform)
    {
        WA_TABLE          waTable;
        memset(&waTable, 0, sizeof(WA_TABLE));
        WA_INIT_PARAM      stWaInitParam = {};
        stWaInitParam.ePlatformType = platform->getPlatformInfo().ePlatformType;
        if (IGC_GET_FLAG_VALUE(OverrideRevIdForWA)!=0xff)
            platform->OverrideRevId(IGC_GET_FLAG_VALUE(OverrideRevIdForWA));
        if (IGC_GET_FLAG_VALUE(OverrideDeviceIdForWA))
            platform->OverrideDeviceId(IGC_GET_FLAG_VALUE(OverrideDeviceIdForWA));
        stWaInitParam.usRevId = platform->getPlatformInfo().usRevId;
        stWaInitParam.usRenderRevID = GFX_GET_GMD_REV_ID_RENDER(platform->getPlatformInfo());
        stWaInitParam.usRevId_PCH = platform->getPlatformInfo().usRevId_PCH;
        GT_SYSTEM_INFO sysInfo = platform->GetGTSystemInfo();
        stWaInitParam.pGtSysInfo = &sysInfo;
        if (IGC_GET_FLAG_VALUE(OverrideProductFamilyForWA))
            platform->OverrideProductFamily(IGC_GET_FLAG_VALUE(OverrideProductFamilyForWA));

        switch (platform->getPlatformInfo().eProductFamily)
        {
        case IGFX_BROADWELL:
            InitBdwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_CHERRYVIEW:
            InitChvWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_BROXTON:
            InitBxtWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_SKYLAKE:
        case IGFX_GENNEXT:
            InitSklWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_CANNONLAKE:
            InitCnlWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_KABYLAKE:
            InitKblDisplayWaTable(&waTable, pSkuFeatureTable, &stWaInitParam); //Display WA only
            InitKblNonDisplayWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);//Non Display WA
            break;
        case IGFX_COFFEELAKE:
            InitKblDisplayWaTable(&waTable, pSkuFeatureTable, &stWaInitParam); //Display WA only
            InitCflNonDisplayWaTable(&waTable, pSkuFeatureTable, &stWaInitParam); //Non Display WA
            break;
        case IGFX_GEMINILAKE:
            InitGlkWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_ICELAKE:
            InitIclHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            InitIclSwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_ICELAKE_LP:
            InitIclLpHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            InitIclLpSwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_LAKEFIELD:
            InitLkfHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            InitLkfSwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_TIGERLAKE_LP:
            {
                InitTglLpHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                InitTglLpSwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            }
            break;
        case IGFX_JASPERLAKE:
            InitJslHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            InitJslSwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_DG1:
            InitDg1HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            InitDg1SwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_ROCKETLAKE:
            InitRklHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            InitRklSwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_ALDERLAKE_S:
            InitAdlsHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            InitAdlsSwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_ALDERLAKE_P:
            InitAdlpHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            InitAdlpSwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_XE_HP_SDV:
            InitXeHPSDVHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            InitXeHPSDVSwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_ALDERLAKE_N:
            InitAdlnHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            InitAdlnSwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_DG2:
            /* 128 */
            if (TRUE == GFX_IS_DG2_G11_CONFIG(platform->getPlatformInfo().usDeviceID))
            {
                InitAcm_G11HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                InitAcm_G11SwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            }
            /* 256 */
            else if (TRUE == GFX_IS_DG2_G12_CONFIG(platform->getPlatformInfo().usDeviceID))
            {
                InitAcm_G12HwWaTable(&waTable,pSkuFeatureTable, &stWaInitParam);
                InitAcm_G12SwWaTable(&waTable,pSkuFeatureTable, &stWaInitParam);
            }
            /* 512 */
            else
            {
                InitAcm_G10HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                InitAcm_G10SwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            }
            break;
        case IGFX_PVC:
            InitPvcHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            stWaInitParam.usRevId &= 0b111; // ComputeChiplet CT RevID is [2:0]
            if (stWaInitParam.usRevId == 0x1) // PVC XL A0p CT RevID
            {
                // changing PVC XL A0p RevID 0x1 to 0x0 REVISION_A0
                // as A0 and A0p has identical set of features
                stWaInitParam.usRevId = 0x0; // PVC XL A0 CT IGC internal stepping REVISION_A0
            }
            platform->OverrideRevId(stWaInitParam.usRevId);

            // Temporary solution to allow enabling the PVC-B0 WAs.
            if (IGC_IS_FLAG_ENABLED(Enable_Wa1807084924))
                waTable.Wa_1807084924 = 1;
            if (IGC_IS_FLAG_ENABLED(Enable_Wa1507979211))
                waTable.Wa_1507979211 = 1;
            if (IGC_IS_FLAG_ENABLED(Enable_Wa14010017096))
                waTable.Wa_14010017096 = 1;
            if (IGC_IS_FLAG_ENABLED(Enable_Wa22010487853))
                waTable.Wa_22010487853 = 1;
            if (IGC_IS_FLAG_ENABLED(Enable_Wa22010493955))
                waTable.Wa_22010493955 = 1;

            break;
        case IGFX_METEORLAKE:
        case IGFX_ARROWLAKE:
        case IGFX_LUNARLAKE:
        case IGFX_BMG:
        case IGFX_PTL:
        /* This is just a place holder the WA application has moved below and changed
        its no longer based on platform */
            break;
        default:
            IGC_ASSERT(0);
            break;
        }
        if (GFX_GET_CURRENT_PRODUCT(platform->getPlatformInfo()) >= IGFX_METEORLAKE)
        {

            stWaInitParam.usDisplayRevID = (unsigned short)GFX_GET_GMD_REV_ID_DISPLAY(platform->getPlatformInfo());
            stWaInitParam.usRenderRevID = (unsigned short)GFX_GET_GMD_REV_ID_RENDER(platform->getPlatformInfo());
            stWaInitParam.usMediaRevID = (unsigned short)GFX_GET_GMD_REV_ID_MEDIA(platform->getPlatformInfo());

             // Applying GT WAs
            switch (GFX_GET_CURRENT_RENDERCORE(platform->getPlatformInfo()))
            {
            case IGFX_XE_HPG_CORE:
            {
                if (GFX_GET_GMD_RELEASE_VERSION_RENDER(platform->getPlatformInfo()) == GFX_GMD_ARCH_12_RELEASE_XE_LP_MD)
                {
                    InitGt_12_70HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                }
                else if (GFX_GET_GMD_RELEASE_VERSION_RENDER(platform->getPlatformInfo()) == GFX_GMD_ARCH_12_RELEASE_XE_LP_LG)
                {
                    InitGt_12_71HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                }
                else if (GFX_GET_GMD_RELEASE_VERSION_RENDER(platform->getPlatformInfo()) == GFX_GMD_ARCH_12_RELEASE_XE_LPG_PLUS_1274)
                {
                    InitGt_12_74HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                }
                InitGt_12_70SwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                break;
            }
            case IGFX_XE2_HPG_CORE:
            {
                switch (GFX_GET_GMD_RELEASE_VERSION_RENDER(platform->getPlatformInfo()))
                {
                case GFX_GMD_ARCH_20_RELEASE_XE2_HPG_2001:
                    InitGt_20_01HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                    break;
                case GFX_GMD_ARCH_20_RELEASE_XE2_HPG_2002:
                    InitGt_20_02HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                    break;
                case GFX_GMD_ARCH_20_RELEASE_XE2_LPG:
                    InitGt_20_04HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                    break;
                default:
                    IGC_ASSERT(0);
                    break;
                }
                break;
            }
            case IGFX_XE3_CORE:
            {
                stWaInitParam.usWaIpShift = WA_BIT_GT;
                switch (GFX_GET_GMD_RELEASE_VERSION_RENDER(platform->getPlatformInfo()))
                {
                case GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3000:
                    InitGt_30_00HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                    break;
                case GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3001:
                    InitGt_30_01HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                    break;
                case GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3003:
                    InitGt_30_03HwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                    break;
                default:
                    IGC_ASSERT_MESSAGE(0, "unknown IP");
                    break;
                }
                stWaInitParam.usWaIpShift = WA_BIT_GT;
                InitGt_30_00_SwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
                break;
            }
            default:
                // SKUs with no GT IP. So do nothing.
                break;
            }
        }
        platform->SetWATable(waTable);
        platform->SetSkuTable(*pSkuFeatureTable);
    }

    // Function to workaround some API send us the usc SKU structure, convert it to the common
    // sku feature structure used by all the driver to not depend on USC structure
    void ConvertSkuTable(const SUscSkuFeatureTable* pUSCSkuFeatureTable, SKU_FEATURE_TABLE& SkuFeatureTable)
    {
        memset(&SkuFeatureTable, 0, sizeof(SKU_FEATURE_TABLE));
        SkuFeatureTable.FtrDesktop = pUSCSkuFeatureTable->FtrDesktop;
        SkuFeatureTable.FtrGtBigDie = pUSCSkuFeatureTable->FtrGtBigDie;
        SkuFeatureTable.FtrGtMediumDie = pUSCSkuFeatureTable->FtrGtMediumDie;
        SkuFeatureTable.FtrGtSmallDie = pUSCSkuFeatureTable->FtrGtSmallDie;
        SkuFeatureTable.FtrGT1 = pUSCSkuFeatureTable->FtrGT1;
        SkuFeatureTable.FtrGT1_5 = pUSCSkuFeatureTable->FtrGT1_5;
        SkuFeatureTable.FtrGT2 = pUSCSkuFeatureTable->FtrGT2;
        SkuFeatureTable.FtrGT3 = pUSCSkuFeatureTable->FtrGT3;
        SkuFeatureTable.FtrGT4 = pUSCSkuFeatureTable->FtrGT4;
        SkuFeatureTable.FtrIVBM0M1Platform = pUSCSkuFeatureTable->FtrIVBM0M1Platform;
        SkuFeatureTable.FtrSGTPVSKUStrapPresent = pUSCSkuFeatureTable->FtrSGTPVSKUStrapPresent;
        SkuFeatureTable.FtrGTA = pUSCSkuFeatureTable->FtrGTA;
        SkuFeatureTable.FtrGTC = pUSCSkuFeatureTable->FtrGTC;
        SkuFeatureTable.FtrGTX = pUSCSkuFeatureTable->FtrGTX;
        SkuFeatureTable.Ftr5Slice = pUSCSkuFeatureTable->Ftr5Slice;
        SkuFeatureTable.FtrGpGpuMidThreadLevelPreempt = pUSCSkuFeatureTable->FtrGpGpuMidThreadLevelPreempt;
        SkuFeatureTable.FtrIoMmuPageFaulting = pUSCSkuFeatureTable->FtrIoMmuPageFaulting;
        SkuFeatureTable.FtrWddm2Svm = pUSCSkuFeatureTable->FtrWddm2Svm;
        SkuFeatureTable.FtrPooledEuEnabled = pUSCSkuFeatureTable->FtrPooledEuEnabled;
        SkuFeatureTable.FtrLocalMemory = pUSCSkuFeatureTable->FtrLocalMemory;
    }

    void SetWorkaroundTable(const SUscSkuFeatureTable* pSkuFeatureTable, CPlatform* platform)
    {
        SKU_FEATURE_TABLE SkuFeatureTable;
        ConvertSkuTable(pSkuFeatureTable, SkuFeatureTable);
        SetWorkaroundTable(&SkuFeatureTable, platform);
    }

    void SetGTSystemInfo(const SUscGTSystemInfo* pGTSystemInfo, CPlatform* platform)
    {
        platform->SetGTSystemInfo(*pGTSystemInfo);
    }


    void SetGTSystemInfo(const GT_SYSTEM_INFO* pGTSystemInfo, CPlatform* platform)
    {
        platform->SetGTSystemInfo(*pGTSystemInfo);
    }

}
