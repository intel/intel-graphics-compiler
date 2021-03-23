/*========================== begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "wa_def.h"

//******************* Main Wa Initializer for Device Id ********************

#define ICL_LP_REV_ID_A0   SI_REV_ID(0,0)
#define ICL_LP_REV_ID_A2   SI_REV_ID(1,1)
#define ICL_LP_REV_ID_B0   SI_REV_ID(3,3)
#define ICL_LP_REV_ID_B2   SI_REV_ID(4,4)
#define ICL_LP_REV_ID_C0   SI_REV_ID(5,5)

void InitIclLpHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_ICL_LP = (int)pWaParam->usRevId;

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_220856683,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1406306137,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc visa
    SI_WA_ENABLE(
        Wa_1406950495,
        "No Link provided",
        "No Link provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1805992985,
        "No Link provided",
        "No Link provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: visa
    SI_WA_ENABLE(
        Wa_2201674230,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_ICL_LP, ICL_LP_REV_ID_A0, ICL_LP_REV_ID_C0));

}
