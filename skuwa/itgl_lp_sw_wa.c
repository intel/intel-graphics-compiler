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
#include "wa_def.h"

#define TGL_REV_ID_A0   SI_REV_ID(0,0)
#define TGL_REV_ID_B0   SI_REV_ID(3,3)

//******************* Main Wa Initializer for Device Id ********************
// Initialize COMMON/DESKTOP/MOBILE WA using PLATFORM_STEP_APPLICABLE() macro.

void InitTglLpSwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int StepId_TGL = (int)pWaParam->usRevId;
#ifdef __KCH
    // compilation issue with UTF: KCHASSERT(NULL != pWaParam);
#endif

    //=================================================================================================================
    //
    //              TGL SW WA for all platforms
    //
    //=================================================================================================================

    //TODO: Add WA as needed
    SI_WA_ENABLE(
        WaMixModeSelInstDstNotPacked,
        "No Link Provided",
        "No Link Provided",
        PLATFORM_ALL,
        SI_WA_FOR_EVER);

}

#ifdef __KCH
void InitTgllpHASWaTable(PHW_DEVICE_EXTENSION pKchContext, PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    //TODO: Add WA as needed
}
#endif // __KCH
