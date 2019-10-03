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
#pragma once
#include "TranslationBlock.h"
#include "Compiler/CodeGenPublic.h"
#include "gtsysinfo.h"

namespace TC
{

/*****************************************************************************\

Class:
    CICBETranslationBlock

Description:

\*****************************************************************************/
class CIGCTranslationBlock
    : public CTranslationBlock
{
public:
    static bool     Create(
                        const STB_CreateArgs* pCreateArgs,
                        CIGCTranslationBlock* &pTranslationBlock );

    static void     Delete(
                        CIGCTranslationBlock* &pTranslationBlock );

    virtual bool    Translate(
                        const STB_TranslateInputArgs* pInputArgs,
                        STB_TranslateOutputArgs* pOutputArgs );

    virtual bool    FreeAllocations(
                        STB_TranslateOutputArgs* pOutputArgs );

protected:
    CIGCTranslationBlock( void );

    virtual ~CIGCTranslationBlock( void );

    bool            Initialize(
                        const STB_CreateArgs* pCreateArgs );

    bool ProcessElfInput(
        STB_TranslateInputArgs &InputArgs,
        STB_TranslateOutputArgs &OutputArgs,
        IGC::OpenCLProgramContext &Context);

    // State
    PLATFORM m_Platform;
    SKU_FEATURE_TABLE m_SkuTable;
    GT_SYSTEM_INFO    m_SysInfo;

    // Type of input
    TB_DATA_FORMAT m_DataFormatInput;
    TB_DATA_FORMAT m_DataFormatOutput;

    float          m_ProfilingTimerResolution;
};

} // namespace TC
