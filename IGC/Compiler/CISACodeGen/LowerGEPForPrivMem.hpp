/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include <vector>
#include <memory>

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

    struct SOALayoutInfo
    {
        /// Signifies if SOA layout be applied to this variable.
        bool canUseSOALayout;
        /// The base type that all memory instructions in the use graph operate
        /// upon. Can be a compound type as long as no instruction in the chain
        /// reads from/stores into the underlying smaller strides of the
        /// primitive type.
        llvm::Type* baseType;
        /// Signifies if all memory instructions that operate on a
        /// compound-typed variable are vector-typed.
        bool allUsesAreVector;

        SOALayoutInfo() : canUseSOALayout(false), baseType(nullptr),
                          allUsesAreVector(false) {}
        SOALayoutInfo(bool canUseSOALayout, llvm::Type* baseType,
                      bool allUsesAreVector) :
            canUseSOALayout(canUseSOALayout), baseType(baseType),
            allUsesAreVector(false) {}
        SOALayoutInfo(SOALayoutInfo&) = default;
        ~SOALayoutInfo() = default;
    };

    /// Conceptually, this class is quite similar to 'llvm::PtrUseVisitor', but
    /// looks to ensure that actual boolean values are returned by 'visit()'
    /// calls. Through this depth-first traversal of the alloca's use-graph, we
    /// aim to determine if "structure of arrays" layout can be applied to the
    /// initialized variable.
    ///
    /// TODO: Consider generalizing the traversal logic as a parent interface
    /// (e.g. 'InstUseVisitorBase') so that it could be re-used across IGC.
    /// Long-term, we could also propose this general use-based visitor to the
    /// LLVM community as an extension/replacement for 'PtrUseVisitor'.
    ///
    /// TODO: Given that PrivateMemoryResolution also relies on SOA layout
    /// evaluation, it would be better to factor this out into a separate
    /// analysis pass.
    class SOALayoutChecker : public llvm::InstVisitor<SOALayoutChecker, bool>
    {
    public:
        friend llvm::InstVisitor<SOALayoutChecker, bool>;

        SOALayoutChecker(llvm::AllocaInst& allocaToCheck) : allocaRef(allocaToCheck) {}
        SOALayoutChecker() = delete;
        ~SOALayoutChecker() = default;
        SOALayoutChecker(SOALayoutChecker&) = delete;

        SOALayoutInfo getOrGatherInfo();

    private:
        llvm::AllocaInst& allocaRef;
        std::unique_ptr<SOALayoutInfo> pInfo;

        bool isVectorSOA = true;
        llvm::Instruction* parentLevelInst = nullptr;

        /// This method visits the direct users of the instruction, abrupting
        /// traversal if a false value is returned by any of the visits.
        ///
        /// TODO: Consider a worklist-based implementation instead. If achieved, it
        /// would make sense to rename the method into `enqueueUsers', in adherence with
        /// the convention set by llvm::PtrUseVisitor.
        bool checkUsers(llvm::Instruction& I);

        /// Given the purpose of the analysis, we can only handle memory
        /// instructions. Default to 'false'.
        bool visitInstruction(llvm::Instruction& I) { return false; }

        bool visitBitCastInst(llvm::BitCastInst&);
        bool visitGetElementPtrInst(llvm::GetElementPtrInst&);
        bool visitIntrinsicInst(llvm::IntrinsicInst&);
        bool visitLoadInst(llvm::LoadInst&);
        bool visitStoreInst(llvm::StoreInst&);
    };

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
