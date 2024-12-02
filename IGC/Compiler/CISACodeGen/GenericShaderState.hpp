/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/Types.hpp"

namespace IGC
{
    class GenericShaderState
    {
    public:
        uint m_constantBufferLoaded = 0;
        uint m_numBlocks = 0;
        uint64_t m_uavLoaded = 0;
        uint m_shaderResourceLoaded[4]{};
        uint m_renderTargetLoaded = 0;
        bool m_HasEval = false;
        uint m_NOSBufferSize = 0;
        bool isMessageTargetDataCacheDataPort = false;
        uint m_constantBufferMask = 0;
        std::vector<USC::SConstantGatherEntry> gatherMap;
        uint m_ConstantBufferLength = 0;
        int m_cbSlot = -1;
        uint m_statelessCBPushedSize = 0;
        // Holds max used binding table entry index.
        uint32_t m_BindingTableEntryCount = 0;
        // Holds binding table entries bitmap.
        uint32_t m_BindingTableUsedEntriesBitmap = 0;
        //true if any input is pulled, false otherwise
        bool isInputsPulled = false;

        GenericShaderState(const llvm::Function& Entry, CodeGenContext& Ctx);

        /// Evaluate the Sampler Count field value.
        unsigned int GetSamplerCount(unsigned int samplerCount) const;

        // in DWORDs
        uint32_t getMinPushConstantBufferAlignmentInBytes() const
        {
            return Ctx.platform.getMinPushConstantBufferAlignment() * sizeof(DWORD);
        }

        uint32_t GetMaxUsedBindingTableEntryCount(void) const
        {
            if (m_BindingTableUsedEntriesBitmap != 0)
            {
                // m_BindingTableEntryCount is index; '+ 1' due to calculate total used count.
                return (m_BindingTableEntryCount + 1);
            }
            return 0;
        }

        uint32_t GetBindingTableEntryBitmap(void) const
        {
            return m_BindingTableUsedEntriesBitmap;
        }

        bool GetHasEval() const { return m_HasEval; }
        void SetHasEval() { m_HasEval = true; }
        void CreateGatherMap();
        void CreateConstantBufferOutput(SKernelProgram* pKernelProgram);
        uint32_t GetShaderThreadUsageRate();

        const llvm::Function& Entry;
        CodeGenContext& Ctx;

        CodeGenContext& GetContext() const { return Ctx; }
    };
} //namespace IGC
