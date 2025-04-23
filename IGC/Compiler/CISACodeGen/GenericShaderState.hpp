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
        GenericShaderState(const llvm::Function& Entry, CodeGenContext& Ctx);

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
        /// Dispatch size is the number of logical threads running in one hardware thread
        SIMDMode m_dispatchSize = SIMDMode::UNKNOWN;

        bool GetHasBarrier() const { return m_BarrierNumber > 0; }
        void SetHasBarrier() {
            if (m_BarrierNumber == 0)
                m_BarrierNumber = 1;
        }
        void SetBarrierNumber(int BarrierNumber) { m_BarrierNumber = BarrierNumber; }
        int  GetBarrierNumber() const { return m_BarrierNumber; }

        bool GetHasSampleGather4Load() const { return (m_HasSampleInst || m_HasGather4Inst || m_HasLoadInst); }
        bool GetHasSampleGather4()     const { return (m_HasSampleInst || m_HasGather4Inst); }
        void SetHasSampleInst()  { m_HasSampleInst  = true; }
        void SetHasGather4Inst() { m_HasGather4Inst = true; }
        void SetHasLoadInst()    { m_HasLoadInst    = true; }

        bool GetHasDPAS() const { return m_HasDPAS; }
        void SetHasDPAS() { m_HasDPAS = true; }

        uint32_t getGRFSize() const { return Ctx.platform.getGRFSize(); }

        // Shader has LSC store messages with cache controls specified in `ops`
        void HasLscStoreCacheControls(const LSC_CACHE_OPTS& opts)
        {
            if (opts.l1 != LSC_CACHING_DEFAULT)
            {
                m_HasLscStoresWithNonDefaultL1CacheControls = true;
            }
        };
        bool GetHasLscStoresWithNonDefaultL1CacheControls() const
        {
            return m_HasLscStoresWithNonDefaultL1CacheControls;
        };

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
        static void setScratchUsage(CodeGenContext &Ctx, SProgramOutput& Prog);
        void setScratchUsage(SProgramOutput& Prog);
        uint32_t GetShaderThreadUsageRate();

        const llvm::Function& Entry;
        CodeGenContext& Ctx;

        CodeGenContext& GetContext() const { return Ctx; }

        bool shouldDisablePreemption(unsigned NumInst) const
        {
            return (Ctx.platform.supportDisableMidThreadPreemptionSwitch() &&
                IGC_IS_FLAG_ENABLED(EnableDisableMidThreadPreemptionOpt) &&
                (Ctx.m_instrTypes.numLoopInsts == 0) &&
                (NumInst < IGC_GET_FLAG_VALUE(MidThreadPreemptionDisableThreshold)));
        }
    private:
        bool m_HasSampleInst = false;
        bool m_HasGather4Inst = false;
        bool m_HasLoadInst = false;
        int m_BarrierNumber = 0;
        bool m_HasDPAS = false;
        // Shader has LSC store messages with non-default L1 cache control
        bool m_HasLscStoresWithNonDefaultL1CacheControls = false;
    };
} //namespace IGC
