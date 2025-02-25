/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/PullConstantHeuristics.hpp"
#include "ShaderCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "FunctionUpgrader.h"

#include <map>

namespace IGC
{
    void initializePushAnalysisPass(llvm::PassRegistry&);
    struct SimplePushData :SimplePushInfo
    {
        std::vector<std::pair<llvm::Instruction*, unsigned int>> Load;
    };
    class PushAnalysis : public llvm::ModulePass
    {
        const llvm::DataLayout* m_DL;
        static const uint32_t MaxConstantBufferIndexSize;
        uint32_t MaxNumOfPushedInputs;

        bool m_funcTypeChanged;
        std::map <llvm::Function*, bool> m_isFuncTypeChanged;

        llvm::Function* m_pFunction = nullptr;
        IGCMD::MetaDataUtils* m_pMdUtils;
        llvm::PostDominatorTree* m_PDT;
        llvm::DominatorTree* m_DT;

        PullConstantHeuristics* m_pullConstantHeuristics;

        CodeGenContext* m_context;
        llvm::DenseMap<llvm::Instruction*, bool> m_statelessLoads;
        uint    m_cbToLoad;
        uint    m_maxStatelessOffset;
        int    m_argIndex;
        std::vector < llvm::Value* > m_argList;
        // List of all arguments (promoted runtime values) that contain dynamic
        // buffer offsets.
        std::set <llvm::Value*> m_dynamicBufferOffsetArgs;
        // Map of shader arguments -> runtime value index. Arguments in this map
        // can be used in surface state offset computations and identify buffers
        // that can be pushed.
        std::map<llvm::Value*, unsigned int> m_bindlessPushArgs;
        FunctionUpgrader m_pFuncUpgrade;
        //Collecting all Simple push
        std::map<unsigned int, SimplePushData> CollectAllSimplePushInfoArr;
        unsigned int numSimplePush = 0;
        // Helper function
        /// Return true if the constant is in the range which we are allowed to push
        bool IsPushableShaderConstant(
            llvm::Instruction* inst,
            SimplePushInfo& info);

        /// Checks if stateless buffer load is not under flow control.
        bool IsSafeToPushNonStaticBufferLoad(llvm::Instruction* inst);

        // Checks if pAddress is a RuntimeValue and is on the list of pushable
        // runtime values.
        bool IsPushableAddress(
            llvm::Instruction* inst,
            llvm::Value* pAddress,
            int& pushableAddressGrfOffset,
            int& pushableOffsetGrfOffset);

        bool GetConstantOffsetForDynamicUniformBuffer(
            llvm::Value* offsetValue,
            uint& relativeOffsetInBytes);

        /// process simple push for the function
        void BlockPushConstants();

        /// Try to push allocate space for the constant to be pushed
        void AllocatePushedConstant(
            llvm::Instruction* load,
            const SimplePushInfo& newChunk,
            const unsigned int maxSizeAllowed);

        /// promote the load to function argument
        void PromoteLoadToSimplePush(llvm::Instruction* load, SimplePushInfo& info, unsigned int offset);

        /// return true if the inputs are uniform
        bool AreUniformInputsBasedOnDispatchMode();
        /// return true if we are allowed to push constants
        bool CanPushConstants();

        /// Return true if the Load is a stateless.
        bool IsStatelessCBLoad(
            llvm::Instruction* inst,
            int& pushableAddressGrfOffset,
            int& pushableOffsetGrfOffset,
            unsigned int& offset);

        bool IsPushableBindlessLoad(
            llvm::Instruction* inst,
            int& grfOffset,
            unsigned int& cbIdx,
            unsigned int& offset);

        /// return the maximum number of inputs pushed for this kernel
        unsigned int GetMaxNumberOfPushedInputs();
        unsigned int GetHSMaxNumberOfPushedInputs();
        bool DispatchGRFHardwareWAForHSAndGSDisabled();

        /// return the push constant mode supported based on driver and platform support
        PushConstantMode GetPushConstantMode();

        void processGather(llvm::Instruction* inst, uint bufId, uint eltId);
        void processInput(llvm::Instruction* inst, bool gsInstancingUsed);
        void processInputVec(llvm::Instruction* inst, bool gsInstancingUsed);
        void processURBRead(llvm::Instruction* inst, bool gsInstancingUsed,
            bool vsHasConstantBufferIndexedWithInstanceId,
            uint32_t vsUrbReadIndexForInstanceIdSGV);
        void processRuntimeValue(llvm::GenIntrinsicInst* intrinsic);

        int getGRFSize() const { return m_context->platform.getGRFSize(); }
        int getMinPushConstantBufferAlignmentInBytes() const { return m_context->platform.getMinPushConstantBufferAlignment() * sizeof(DWORD); }
        unsigned int GetSizeInBits(llvm::Type* type) const;

    public:
        static char ID;
        PushAnalysis();
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequired<PullConstantHeuristics>();
        }

        llvm::Value* addArgumentAndMetadata(llvm::Type* pType, std::string argName, IGC::WIAnalysis::WIDependancy dependency);
        bool runOnModule(llvm::Module&) override;
        void ProcessFunction();
        void AnalyzeFunction(llvm::Function* F)
        {
            m_pFunction = F;
            m_PDT = nullptr;
            m_DT = nullptr;

            // We need to initialize m_argIndex and m_argList appropriately as there might be some arguments added before push analysis stage
            m_argIndex = 0;
            m_argList.clear();
            for (auto arg = m_pFunction->arg_begin(); arg != m_pFunction->arg_end(); ++arg)
            {
                m_argList.push_back(&(*arg));
                m_argIndex++;
            }
            m_argIndex = m_argIndex - 1;

            if (m_pMdUtils->findFunctionsInfoItem(F) != m_pMdUtils->end_FunctionsInfo())
            {
                // TODO: when doing codegen for cps shader.  We will run CodeGen twice,
                // first for coarse_phase, then pixel_phase, see CodeGen().
                // While the pushinfo metadata is not stored using function as index.
                // So when doing codege for pixel_phase, we need to clear legacy
                // pushinfo metadata from coarse_phase. A better solution is to store
                // pushinfo with function index. It's tricky here, refactor in future.
                ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
                modMD->pushInfo.pushAnalysisWIInfos.clear();

                ProcessFunction();
            }
        }

        virtual llvm::StringRef getPassName() const override {
            return "PushAnalysis";
        }
    };

}//namespace IGC
