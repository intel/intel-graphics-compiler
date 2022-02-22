/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include <vector>
namespace llvm
{
    class FunctionPass;
    class AllocaInst;
    class Instruction;
    class Value;
    class GetElementPtrInst;
    class StoreInst;
    class IntrinsicInst;
    class Type;
}

namespace IGC
{
    enum StatusPrivArr2Reg
    {
        OK,
        IsDynamicAlloca,
        CannotUseSOALayout,
        IsNotNativeType,
        OutOfAllocSizeLimit,
        OutOfMaxGRFPressure
    };

    /// Tries to promote array in private memory to indexable vector
    /// Uses register pressure to make sure it won't cause spilling
    llvm::FunctionPass* createPromotePrivateArrayToReg();

    /// Array can be stored in SOA form
    /// Helper allowing function to detect if an array can stored in SOA form
    /// It is true
    bool CanUseSOALayout(llvm::AllocaInst* inst, llvm::Type*& baseType);

    class TransposeHelper
    {
    public:
        TransposeHelper(bool vectorIndex) : m_vectorIndex(vectorIndex) {}
        void HandleAllocaSources(
            llvm::Instruction* v,
            llvm::Value* idx);
        void handleGEPInst(
            llvm::GetElementPtrInst* pGEP,
            llvm::Value* idx);
        virtual void handleLoadInst(
            llvm::LoadInst* pLoad,
            llvm::Value* pScalarizedIdx) = 0;
        virtual void handleStoreInst(
            llvm::StoreInst* pStore,
            llvm::Value* pScalarizedIdx) = 0;
        virtual void handleLifetimeMark(llvm::IntrinsicInst* inst) = 0;
        void EraseDeadCode();
    protected:
        std::vector<llvm::Instruction*> m_toBeRemovedGEP;
    private:
        bool m_vectorIndex;
    };
}
