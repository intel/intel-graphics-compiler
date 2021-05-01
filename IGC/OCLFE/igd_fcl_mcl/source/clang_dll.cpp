/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/


#include "../headers/clang_tb.h"
#include "secure_mem.h"

namespace TC
{

TRANSLATION_BLOCK_API void Register(
    STB_RegisterArgs* pRegisterArgs )
{
    pRegisterArgs->Version = TC::STB_VERSION;
    uint32_t maxNumTranslationCodes =
        sizeof(g_cClangTranslationCodes) /
        sizeof(STB_TranslationCode);

    if (pRegisterArgs->pTranslationCodes != NULL)
    {
        memcpy_s(
            pRegisterArgs->pTranslationCodes,
            pRegisterArgs->NumTranslationCodes * sizeof(STB_TranslationCode),
            g_cClangTranslationCodes,
            sizeof(g_cClangTranslationCodes));
    }
    else
    {
        pRegisterArgs->NumTranslationCodes = maxNumTranslationCodes;
    }
}

TRANSLATION_BLOCK_API CTranslationBlock* Create(
    STB_CreateArgs* pCreateArgs )
{
    CClangTranslationBlock*  pClangTranslationBlock;
    STB_TranslateOutputArgs pOutputArgs;

    CClangTranslationBlock::Create(
        pCreateArgs,
        &pOutputArgs,
        pClangTranslationBlock );

    return pClangTranslationBlock;
}

TRANSLATION_BLOCK_API void Delete(
    CTranslationBlock* pTranslationBlock )
{
    CClangTranslationBlock*  pClangTranslationBlock =
        (CClangTranslationBlock*)pTranslationBlock;

    CClangTranslationBlock::Delete(
        pClangTranslationBlock );
}


} // namespace TC
