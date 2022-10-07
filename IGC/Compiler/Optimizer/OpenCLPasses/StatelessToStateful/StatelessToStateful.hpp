/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Analysis/AssumptionCache.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

namespace IGC
{
    // Too many stateful promotion will overwhelm the surface state
    // cache(32 entries per HDC), which will significantly impact the
    // performance. Simply disable stateful promotion after 32 args.
    constexpr uint maxPromotionCount = 32;

    class StatelessToStateful : public llvm::FunctionPass, public llvm::InstVisitor<StatelessToStateful>
    {
    public:
        typedef llvm::DenseMap<const KernelArg*, int> ArgInfoMap;

        static char ID;

        StatelessToStateful(bool NoNegOffset = false);

        ~StatelessToStateful() {}

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<llvm::AssumptionCacheTracker>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual llvm::StringRef getPassName() const override
        {
            return "StatelessToStateful";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);
        void visitCallInst(llvm::CallInst& I);

    private:
        llvm::CallInst* createBufferPtr(
            unsigned addrSpace, llvm::Constant* argNumber, llvm::Instruction* InsertBefore);
        bool pointerIsPositiveOffsetFromKernelArgument(
            llvm::Function* F, llvm::Value* V, llvm::Value*& offset, unsigned int& argNumber, const KernelArg*& kernelArg);

        // Check if the given pointer value can be traced back to any kernel argument.
        // return the kernel argument if found, otherwise return nullptr.
        const KernelArg* getKernelArgFromPtr(const llvm::PointerType& ptrType, llvm::Value* pVal);

        // check if the given pointer can be traced back to any kernel argument
        bool pointerIsFromKernelArgument(llvm::Value& ptr);

        bool getOffsetFromGEP(
            llvm::Function* F, llvm::SmallVector<llvm::GetElementPtrInst*, 4> GEPs,
            uint32_t argNumber, bool isImplicitArg, llvm::Value*& offset);
        llvm::Argument* getBufferOffsetArg(llvm::Function* F, uint32_t ArgNumber);
        void setPointerSizeTo32bit(int32_t AddrSpace, llvm::Module* M);

        void updateArgInfo(const KernelArg* KA, bool IsPositive);
        void finalizeArgInitialValue(llvm::Function* F);

        const KernelArg* getKernelArg(llvm::Value* Arg)
        {
            IGC_ASSERT_MESSAGE(m_pKernelArgs, "Should initialize it before use!");
            for (const KernelArg& arg : *m_pKernelArgs) {
                if (arg.getArg() == Arg) {
                    return &arg;
                }
            }
            return nullptr;
        }

        const KernelArg* getBufferOffsetKernelArg(const KernelArg* KA)
        {
            IGC_ASSERT_MESSAGE(m_pKernelArgs, "KernelArgs: should initialize it before use!");
            int argno = KA->getAssociatedArgNo();
            for (const KernelArg& arg : *m_pKernelArgs) {
                if (arg.getArgType() == KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET &&
                    arg.getAssociatedArgNo() == argno) {
                    return &arg;
                }
            }
            return nullptr;
        }

        // When true, runtime can generate surface with buffer's original base (creation base)
        const bool m_hasBufferOffsetArg;

        // When m_hasBufferOffsetArg is true, optional buffer offset
        // can be on or off, which is indicated by this boolean flag.
        bool       m_hasOptionalBufferOffsetArg;

        // For historic reason, kernel ptrArgs, such as char*, short*, are assumed to
        // be aligned on DW (which is stronger than what OCL's natural alignment) in this
        // stateful optimization.  If this is not a case, this arg should be set to true!
        bool       m_hasSubDWAlignedPtrArg;

        // When true, every messages that are in ptrArg + offset will have offset >= 0.
        bool       m_hasPositivePointerOffset;

        // Handle non-gep pointer
        //   For historic reason (probably non-DW aligned arg), non-gep ptr isn't handled.
        //   If this field is true, non-gep ptr shall be handled.
        const bool m_supportNonGEPPtr = false;

        llvm::AssumptionCacheTracker* m_ACT;
        llvm::AssumptionCache* getAC(llvm::Function* F)
        {
            return (m_ACT != nullptr ? &m_ACT->getAssumptionCache(*F)
                : nullptr);
        }

        ImplicitArgs* m_pImplicitArgs;
        KernelArgs* m_pKernelArgs;
        ArgInfoMap   m_argsInfo;
        bool m_changed;
        std::unordered_set<const KernelArg*> m_promotedKernelArgs; // ptr args which have been promoted to stateful
    };

}
