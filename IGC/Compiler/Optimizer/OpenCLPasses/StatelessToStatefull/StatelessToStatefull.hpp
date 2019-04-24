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

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Analysis/AssumptionCache.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class StatelessToStatefull : public llvm::FunctionPass, public llvm::InstVisitor<StatelessToStatefull>
    {
    public:
        typedef llvm::DenseMap<const KernelArg*, int> ArgInfoMap;

        static char ID;

        StatelessToStatefull(bool NoNegOffset = false);

        ~StatelessToStatefull() {}

        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<llvm::AssumptionCacheTracker>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual llvm::StringRef getPassName() const override
        {
            return "StatelessToStatefull";
        }

        virtual bool runOnFunction(llvm::Function &F) override;

        void visitLoadInst(llvm::LoadInst &I);
        void visitStoreInst(llvm::StoreInst &I);
        void visitCallInst(llvm::CallInst &I);

    private:
        llvm::CallInst* createBufferPtr(
            unsigned addrSpace, llvm::Constant* argNumber, llvm::Instruction* InsertBefore);
        bool pointerIsPositiveOffsetFromKernelArgument(
            llvm::Function* F, llvm::Value* V, llvm::Value*& offset, unsigned int& argNumber);
        bool getOffsetFromGEP(
            llvm::Function* F, llvm::SmallVector<llvm::GetElementPtrInst*, 4> GEPs,
            uint32_t argNumber, bool isImplicitArg, llvm::Value*& offset);
        llvm::Argument* getBufferOffsetArg(llvm::Function* F, uint32_t ArgNumber);
        void setPointerSizeTo32bit(int32_t AddrSpace, llvm::Module* M);

        void updateArgInfo(const KernelArg *KA, bool IsPositive);
        void finalizeArgInitialValue(llvm::Function *F);

        const KernelArg* getKernelArg(llvm::Value *Arg)
        {
            assert(m_pKernelArgs && "Should initialize it before use!");
            for (const KernelArg& arg : *m_pKernelArgs) {
                if (arg.getArg() == Arg) {
                    return &arg;
                }
            }
            return nullptr;
        }

        const KernelArg* getBufferOffsetKernelArg(const KernelArg *KA)
        {
            assert(m_pKernelArgs && "KernelArgs: should initialize it before use!");
            int argno = KA->getAssociatedArgNo();
            for (const KernelArg& arg : *m_pKernelArgs) {
                if (arg.getArgType() == KernelArg::ArgType::IMPLICIT_BUFFER_OFFSET &&
                    arg.getAssociatedArgNo() == argno) {
                    return &arg;
                }
            }
            return nullptr;
        }

        const bool m_hasBufferOffsetArg;

        // When m_hasBufferOffsetArg is true, optional buffer offset
        // can be on or off, which is indicated by this boolean flag.
        bool       m_hasOptionalBufferOffsetArg;

        llvm::AssumptionCacheTracker *m_ACT;
        llvm::AssumptionCache *getAC(llvm::Function *F)
        {
            return (m_ACT != nullptr ? &m_ACT->getAssumptionCache(*F)
                                     : nullptr) ;
        }

        ImplicitArgs *m_pImplicitArgs;
        KernelArgs   *m_pKernelArgs;
        ArgInfoMap   m_argsInfo;
        bool m_changed;
    };

}
