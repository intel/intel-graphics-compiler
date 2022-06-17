/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "TranslationBlock.h"
#include "Compiler/CodeGenPublic.h"
#include "gtsysinfo.h"

namespace TC
{

class CIGCTranslationBlock : public CTranslationBlock
{
public:
    static bool Create(
        const STB_CreateArgs* pCreateArgs,
        CIGCTranslationBlock*& pTranslationBlock);

    static void Delete(CIGCTranslationBlock* pTranslationBlock);

    virtual bool Translate(
        const STB_TranslateInputArgs* pInputArgs,
        STB_TranslateOutputArgs* pOutputArgs);

    virtual bool FreeAllocations(STB_TranslateOutputArgs* pOutputArgs);

protected:
    CIGCTranslationBlock() = default;

    virtual ~CIGCTranslationBlock() = default;

    bool Initialize(const STB_CreateArgs* pCreateArgs);

    bool ProcessElfInput(
        STB_TranslateInputArgs& InputArgs,
        STB_TranslateOutputArgs& OutputArgs,
        IGC::OpenCLProgramContext& Context,
        float ProfilingTimerResolution);

    // State
    PLATFORM m_Platform;
    SKU_FEATURE_TABLE m_SkuTable;
    GT_SYSTEM_INFO m_SysInfo;

    // Type of input
    TB_DATA_FORMAT m_DataFormatInput;
    TB_DATA_FORMAT m_DataFormatOutput;

    float m_ProfilingTimerResolution;
};

} // namespace TC
