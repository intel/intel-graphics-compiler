/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/GenericShaderState.hpp"

using namespace llvm;

namespace IGC
{
    GenericShaderState::GenericShaderState(
        const llvm::Function& Entry, CodeGenContext& Ctx) : Entry(Entry), Ctx(Ctx)
    {
        m_numBlocks = Entry.size();
    }

    unsigned int GenericShaderState::GetSamplerCount(unsigned int samplerCount) const
    {
        if (samplerCount > 0)
        {
            if (samplerCount <= 4)
                return 1; // between 1 and 4 samplers used
            else if (samplerCount >= 5 && samplerCount <= 8)
                return 2; // between 5 and 8 samplers used
            else if (samplerCount >= 9 && samplerCount <= 12)
                return 3; // between 9 and 12 samplers used
            else if (samplerCount >= 13 && samplerCount <= 16)
                return 4; // between 13 and 16 samplers used
            else
                // Samplers count out of range. Force value 0 to avoid undefined behavior.
                return 0;
        }
        return 0;
    }

    void GenericShaderState::CreateGatherMap()
    {
        auto& pushInfo = Ctx.getModuleMetaData()->pushInfo;
        int index = -1;
        gatherMap.reserve(pushInfo.constants.size());
        for (auto I = pushInfo.constants.begin(), E = pushInfo.constants.end(); I != E; I++)
        {
            unsigned int address = (I->first.bufId * 256 * 4) + (I->first.eltId);
            unsigned int cstOffset = address / 4;
            unsigned int cstChannel = address % 4;
            if (cstOffset != index)
            {
                USC::SConstantGatherEntry entry;
                entry.GatherEntry.Fields.constantBufferOffset = cstOffset % 256;
                entry.GatherEntry.Fields.channelMask = BIT(cstChannel);
                // with 3DSTATE_DX9_CONSTANT if buffer is more than 4Kb,
                //  the constant after 255 can be accessed in constant buffer 1
                int CBIndex = cstOffset / 256;
                entry.GatherEntry.Fields.constantBufferIndex = CBIndex;
                m_constantBufferMask |= BIT(CBIndex);
                gatherMap.push_back(entry);
                index = cstOffset;
            }
            else
            {
                gatherMap[gatherMap.size() - 1].GatherEntry.Fields.channelMask |= BIT(cstChannel);
            }
        }

        // The size of the gather map must be even
        if (gatherMap.size() % 2 != 0)
        {
            USC::SConstantGatherEntry entry;
            entry.GatherEntry.Value = 0;
            gatherMap.push_back(entry);
        }
    }

    void  GenericShaderState::CreateConstantBufferOutput(SKernelProgram* pKernelProgram)
    {
        pKernelProgram->ConstantBufferMask = m_constantBufferMask;
        pKernelProgram->gatherMapSize = gatherMap.size();
        if (pKernelProgram->gatherMapSize > 0)
        {
            pKernelProgram->gatherMap = new char[pKernelProgram->gatherMapSize * sizeof(USC::SConstantGatherEntry)];
            memcpy_s(pKernelProgram->gatherMap, pKernelProgram->gatherMapSize *
                sizeof(USC::SConstantGatherEntry),
                &gatherMap[0],
                gatherMap.size() * sizeof(USC::SConstantGatherEntry));
            pKernelProgram->ConstantBufferLength = m_ConstantBufferLength / getMinPushConstantBufferAlignmentInBytes();
        }

        if (m_cbSlot != -1)
        {
            pKernelProgram->bufferSlot = m_cbSlot;
            pKernelProgram->statelessCBPushedSize = m_statelessCBPushedSize;
        }

        auto& pushInfo = Ctx.getModuleMetaData()->pushInfo;
        // for simple push
        for (unsigned int i = 0; i < pushInfo.simplePushBufferUsed; i++)
        {
            pKernelProgram->simplePushInfoArr[i].m_cbIdx = pushInfo.simplePushInfoArr[i].cbIdx;
            pKernelProgram->simplePushInfoArr[i].m_pushableAddressGrfOffset = pushInfo.simplePushInfoArr[i].pushableAddressGrfOffset;
            pKernelProgram->simplePushInfoArr[i].m_pushableOffsetGrfOffset = pushInfo.simplePushInfoArr[i].pushableOffsetGrfOffset;
            pKernelProgram->simplePushInfoArr[i].m_offset = pushInfo.simplePushInfoArr[i].offset;
            pKernelProgram->simplePushInfoArr[i].m_size = pushInfo.simplePushInfoArr[i].size;
            pKernelProgram->simplePushInfoArr[i].isStateless = pushInfo.simplePushInfoArr[i].isStateless;
            pKernelProgram->simplePushInfoArr[i].isBindless = pushInfo.simplePushInfoArr[i].isBindless;
        }

        if (GetContext().m_ConstantBufferReplaceShaderPatterns)
        {
            pKernelProgram->m_ConstantBufferReplaceShaderPatterns = GetContext().m_ConstantBufferReplaceShaderPatterns;
            pKernelProgram->m_ConstantBufferReplaceShaderPatternsSize = GetContext().m_ConstantBufferReplaceShaderPatternsSize;
            pKernelProgram->m_ConstantBufferUsageMask = GetContext().m_ConstantBufferUsageMask;
            pKernelProgram->m_ConstantBufferReplaceSize = GetContext().m_ConstantBufferReplaceSize;
        }
    }

    void GenericShaderState::setScratchUsage(CodeGenContext& Ctx, SProgramOutput& Prog)
    {
        bool SepSpillPvtSS = SeparateSpillAndScratch(&Ctx);
        bool SeparateScratchWA =
            IGC_IS_FLAG_ENABLED(EnableSeparateScratchWA) &&
            !Ctx.getModuleMetaData()->disableSeparateScratchWA;
        Prog.init(!Ctx.platform.hasScratchSurface(),
            Ctx.platform.maxPerThreadScratchSpace(
            ),
            Ctx.getModuleMetaData()->compOpt.UseScratchSpacePrivateMemory,
            SepSpillPvtSS, SeparateScratchWA);
    }

    void GenericShaderState::setScratchUsage(SProgramOutput& Prog)
    {
        setScratchUsage(Ctx, Prog);
    }

    uint32_t GenericShaderState::GetShaderThreadUsageRate()
    {
        uint32_t grfNum = GetContext().getNumGRFPerThread();
        // prevent callee divide by zero
        return std::max<uint32_t>(1, grfNum / CodeGenContext::DEFAULT_TOTAL_GRF_NUM);
    }
} // namespace IGC
