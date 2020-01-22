/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#include "igc_workaround.h"
#include "assert.h"
#include <string.h>
#include <stdlib.h>

namespace IGC
{

    void SetWorkaroundTable(SKU_FEATURE_TABLE* pSkuFeatureTable, CPlatform* platform)
    {
        WA_TABLE          waTable;
        memset(&waTable, 0, sizeof(WA_TABLE));
        WA_INIT_PARAM      stWaInitParam = {};
        stWaInitParam.ePlatformType = platform->getPlatformInfo().ePlatformType;
        stWaInitParam.usRevId = platform->getPlatformInfo().usRevId;
        stWaInitParam.usRevId_PCH = platform->getPlatformInfo().usRevId_PCH;
        GT_SYSTEM_INFO sysInfo = platform->GetGTSystemInfo();
        stWaInitParam.pGtSysInfo = &sysInfo;

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
            break;
        case IGFX_ICELAKE_LP:
            InitIclLpHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_LAKEFIELD:
            InitLkfHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        case IGFX_TIGERLAKE_LP:
            {
                InitTglLpHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            }
            break;
        case IGFX_JASPERLAKE:
            InitJslHwWaTable(&waTable, pSkuFeatureTable, &stWaInitParam);
            break;
        default:
            assert(false);
            break;
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
