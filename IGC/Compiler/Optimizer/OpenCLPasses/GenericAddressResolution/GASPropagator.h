/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Constant.h"
#include <llvm/IR/NoFolder.h>
#include "common/LLVMWarningsPop.hpp"

#define DEBUG_TYPE "gas-resolver"

namespace IGC{

    typedef IRBuilder<llvm::NoFolder> BuilderType;

    class GASPropagator : public InstVisitor<GASPropagator, bool> {
        friend class InstVisitor<GASPropagator, bool>;

        LoopInfo* const LI;
        BuilderType IRB;

        Use* TheUse;
        Value* TheVal;

        // Phi node being able to be resolved from its initial value.
        DenseSet<PHINode*> ResolvableLoopPHIs;

    public:
        GASPropagator(LLVMContext& Ctx, LoopInfo* LoopInfo)
            : IRB(Ctx), LI(LoopInfo), TheUse(nullptr), TheVal(nullptr) {
            populateResolvableLoopPHIs();
        }

        bool propagateToUser(Use* U, Value* V) {
            TheUse = U;
            TheVal = V;
            Instruction* I = cast<Instruction>(U->getUser());
            return visit(*I);
        }
        bool propagateToAllUsers(AddrSpaceCastInst* I);
        void propagate(Value* I);
    private:
        void populateResolvableLoopPHIs();
        void populateResolvableLoopPHIsForLoop(const Loop* L);
        bool isResolvableLoopPHI(PHINode* PN) const {
            return ResolvableLoopPHIs.count(PN) != 0;
        }
        bool isAddrSpaceResolvable(PHINode* PN, const Loop* L,
            BasicBlock* BackEdge) const;

        bool visitInstruction(Instruction& I);

        bool visitLoadInst(LoadInst&);
        bool visitStoreInst(StoreInst&);
        bool visitAddrSpaceCastInst(AddrSpaceCastInst&);
        bool visitBitCastInst(BitCastInst&);
        bool visitPtrToIntInst(PtrToIntInst&);
        bool visitGetElementPtrInst(GetElementPtrInst&);
        bool visitPHINode(PHINode&);
        bool visitICmp(ICmpInst&);
        bool visitSelect(SelectInst&);
        bool visitMemCpyInst(MemCpyInst&);
        bool visitMemMoveInst(MemMoveInst&);
        bool visitMemSetInst(MemSetInst&);
        bool visitCallInst(CallInst&);
        bool visitDbgDeclareInst(DbgDeclareInst&);
        bool visitDbgValueInst(DbgValueInst&);
    };
} // End namespace IGC
