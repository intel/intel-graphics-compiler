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

//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//




#include "../headers/clang_dll.h"
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