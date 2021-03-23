/*========================== begin_copyright_notice ============================

Copyright (c) 2020-2021 Intel Corporation

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
#include "irkl_rev_id.h"

//******************* Main Wa Initializer for RKL          ********************

void InitRklHwWaTable(PWA_TABLE pWaTable, PSKU_FEATURE_TABLE pSkuTable, PWA_INIT_PARAM pWaParam)
{
    int iStepId_RKL = (int)pWaParam->usRevId;

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_1606932921,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc visa
    SI_WA_ENABLE(
        Wa_1407528679,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_1407901919,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_14010017096,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1607610283,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1808850743,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute igc
    SI_WA_ENABLE(
        Wa_1808326800,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, RKL_GT_REV_ID_C0));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_14010595310,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, RKL_GT_REV_ID_C0));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1807084924,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1604402567,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_220856683,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1406664125,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc visa
    SI_WA_ENABLE(
        Wa_1406872168,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1607871015,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: igc
    SI_WA_ENABLE(
        Wa_1809626530,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));

    // Components affected: compute
    SI_WA_ENABLE(
        Wa_22010594632,
        "No Link provided",
        "No HWSightingLink provided",
        PLATFORM_ALL,
        SI_WA_BETWEEN(iStepId_RKL, RKL_GT_REV_ID_A0, FUTURE_PROJECT));
}
