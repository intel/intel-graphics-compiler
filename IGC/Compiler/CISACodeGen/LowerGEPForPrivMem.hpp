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

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/RegisterPressureEstimate.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

#include <vector>
#include <set>

namespace IGC
{
    /// @brief  LowerGEPForPrivMem pass is used for lowering the allocas identified while visiting the alloca instructions
    ///         and then inserting insert/extract elements instead of load stores. This allows us
    ///         to store the data in registers instead of propagating it to scratch space.
    class LowerGEPForPrivMem : public llvm::FunctionPass, public llvm::InstVisitor<LowerGEPForPrivMem>
    {
    public:
        LowerGEPForPrivMem();

        ~LowerGEPForPrivMem() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "LowerGEPForPrivMem";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
        {
            AU.addRequired<RegisterPressureEstimate>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<WIAnalysis>();
            AU.setPreservesCFG();
        }

        virtual bool runOnFunction(llvm::Function &F) override;

        void visitAllocaInst(llvm::AllocaInst &I);

        unsigned int extractAllocaSize(llvm::AllocaInst* pAlloca);

    private:
        llvm::AllocaInst* createVectorForAlloca(
            llvm::AllocaInst *pAlloca,
            llvm::Type *pBaseType);

        // return true if the use of the pointer allow promotion
        bool ValidUses(llvm::Instruction* inst);

        void handleAllocaInst(llvm::AllocaInst *pAlloca);

        void HandleAllocaSources(
            llvm::Instruction* v, 
            llvm::AllocaInst* pVecAlloca,
            llvm::Value* idx);

        void handleGEPInst(
            llvm::GetElementPtrInst *pGEP,
            llvm::AllocaInst* pVecAlloca,
            llvm::Value* idx);

        void handleLoadInst(
            llvm::LoadInst *pLoad,
            llvm::AllocaInst *pVecAlloca,
            llvm::Value *pScalarizedIdx);

        void handleStoreInst(
            llvm::StoreInst *pStore,
            llvm::AllocaInst *pVecAlloca,
            llvm::Value *pScalarizedIdx);

        void handleLifetimeMark(llvm::IntrinsicInst *inst);

        bool CheckIfAllocaPromotable(llvm::AllocaInst* pAlloca);

        /// Conservatively check if a store allow an Alloca to be uniform
        bool IsUniformStore(llvm::StoreInst* pStore);
    public:
        static char ID;

    private:
        const llvm::DataLayout                              *m_pDL;
        CodeGenContext                                      *m_ctx;
        std::vector<llvm::Instruction*>                      m_toBeRemovedGEP;
        std::vector<llvm::Instruction*>                      m_toBeRemovedLoadStore;
        std::vector<llvm::AllocaInst*>                       m_allocasToPrivMem;
        RegisterPressureEstimate*                            m_pRegisterPressureEstimate;
        llvm::Function                                      *m_pFunc;

        /// Keep track of each BB affected by promoting MemtoReg and the current pressure at that block
        llvm::DenseMap<llvm::BasicBlock *, unsigned>         m_pBBPressure;
    };

    llvm::FunctionPass *createLowerGEPForPrivMem();

} // namespace IGC
