/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "type-demote"
#include "Compiler/CISACodeGen/TypeDemote.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

    typedef IRBuilder<> BuilderType;

    class TypeDemote : public FunctionPass {
        BuilderType* IRB;

    public:
        static char ID;

        TypeDemote() : FunctionPass(ID), IRB(nullptr) {
            initializeTypeDemotePass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
        }

    private:
        bool demoteOnFunction(Function*) const;
        bool demoteOnBasicBlock(BasicBlock*) const;

        Value* getDemotedValue(Value* V, Type* DemotedTy, bool Unsigned) const;
    };

} // End anonymous namespace

FunctionPass* IGC::createTypeDemotePass() {
    return new TypeDemote();
}

char TypeDemote::ID = 0;

#define PASS_FLAG     "igc-type-demote"
#define PASS_DESC     "Demote type safely"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(TypeDemote, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_END(TypeDemote, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

bool TypeDemote::runOnFunction(Function& F) {
    BuilderType TheBuilder(F.getContext());
    IRB = &TheBuilder;

    bool Changed = false;
    bool LocalChanged = false;
    do {
        LocalChanged = demoteOnFunction(&F);
        Changed |= LocalChanged;
    } while (LocalChanged);

    return Changed;
}

bool TypeDemote::demoteOnFunction(Function* F) const {
    bool Changed = false;

    ReversePostOrderTraversal<Function*> RPOT(F);
    for (auto& BB : RPOT)
        Changed |= demoteOnBasicBlock(BB);

    return Changed;
}

bool TypeDemote::demoteOnBasicBlock(BasicBlock* BB) const {
    bool Changed = false;

    Type* DemotedTy = IRB->getInt8Ty();
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; /* EMPTY */) {
        Instruction* I = &(*BI++);
        // So far, we only demote unsigned i32 to i8.
        // TODO: Consider demote to i16 as well as i8 may not be supported natively
        // on future Gen.
        if (PHINode * PN = dyn_cast<PHINode>(I)) {
            // Skip non-i32 so far.
            if (!PN->getType()->isIntegerTy(32))
                continue;

            unsigned N = PN->getNumIncomingValues();
            SmallVector<Value*, 8> DemotedValues(N, nullptr);
            bool SafeToDemote = true;
            for (unsigned i = 0; i != N; ++i) {
                Value* DemotedVal = getDemotedValue(PN->getIncomingValue(i), DemotedTy, true);
                if (!DemotedVal) {
                    SafeToDemote = false;
                    break;
                }
                DemotedValues[i] = DemotedVal;
            }
            if (!SafeToDemote)
                continue;

            Type* OriginTy = PN->getType();
            // Demote the original `phi` to i8 one followed a `zext`.
            PN->mutateType(DemotedTy);
            for (unsigned i = 0; i != N; ++i)
                PN->setIncomingValue(i, DemotedValues[i]);
            // Create the unsigned extension.
            BuilderType::InsertPointGuard Guard(*IRB);
            IRB->SetInsertPoint(BB->getFirstNonPHI());
            Value* V = IRB->CreateZExt(PN, OriginTy, ".demoted.zext");
            for (auto UI = PN->use_begin(), UE = PN->use_end(); UI != UE; /* EMPTY */) {
                Use& U = *UI++;
                if (U.getUser() == V)
                    continue;
                U.set(V);
            }
            Changed = true;
            BI = std::next(BasicBlock::iterator(PN));
            continue;
        }

        if (ICmpInst * Cmp = dyn_cast<ICmpInst>(I)) {
            if (!Cmp->getOperand(0)->getType()->isIntegerTy(32))
                continue;
            if (!Cmp->isUnsigned())
                continue;
            Value* DemotedLHS = getDemotedValue(Cmp->getOperand(0), DemotedTy, true);
            if (!DemotedLHS)
                continue;
            Value* DemotedRHS = getDemotedValue(Cmp->getOperand(1), DemotedTy, true);
            if (!DemotedRHS)
                continue;
            Cmp->setOperand(0, DemotedLHS);
            Cmp->setOperand(1, DemotedRHS);
            Changed = true;
            continue;
        }

        if (SelectInst * SI = dyn_cast<SelectInst>(I)) {
            // Check for min or max before doing this
            if (ICmpInst * cmp = dyn_cast<ICmpInst>(SI->getOperand(0)))
            {
                if ((cmp->getOperand(0) == SI->getOperand(1) && cmp->getOperand(1) == SI->getOperand(2)) ||
                    (cmp->getOperand(0) == SI->getOperand(2) && cmp->getOperand(1) == SI->getOperand(1)))
                {
                    continue;
                }
            }
            if (!SI->getType()->isIntegerTy(32))
                continue;
            Value* DemotedTrueVal = getDemotedValue(SI->getTrueValue(), DemotedTy, true);
            if (!DemotedTrueVal)
                continue;
            Value* DemotedFalseVal = getDemotedValue(SI->getFalseValue(), DemotedTy, true);
            if (!DemotedFalseVal)
                continue;

            Type* OriginTy = SI->getType();
            SI->setOperand(1, DemotedTrueVal);
            SI->setOperand(2, DemotedFalseVal);
            SI->mutateType(DemotedTy);
            // Create the unsigned extension.
            BuilderType::InsertPointGuard Guard(*IRB);
            IRB->SetInsertPoint(&(*std::next(BasicBlock::iterator(I))));
            Value* V = IRB->CreateZExt(SI, OriginTy, ".demoted.zext");
            for (auto UI = SI->use_begin(), UE = SI->use_end(); UI != UE; /* EMPTY */) {
                Use& U = *UI++;
                if (U.getUser() == V)
                    continue;
                U.set(V);
            }
            Changed = true;
            BI = std::next(BasicBlock::iterator(SI));
            continue;
        }

        if (BinaryOperator * BO = dyn_cast<BinaryOperator>(I)) {
            // Skip if it's already demoted.
            if (BO->getType() == DemotedTy)
                continue;
            switch (BO->getOpcode()) {
            default:
                continue;
                // TODO: Only handle 'and', 'or', 'xor' so far.
            case Instruction::And:
            case Instruction::Or:
            case Instruction::Xor:
                break;
            }
            unsigned src0BitSize = BO->getOperand( 0 )->getType()->getScalarSizeInBits();
            unsigned src1BitSize = BO->getOperand( 1 )->getType()->getScalarSizeInBits();
            unsigned demotedBitSize = DemotedTy->getScalarSizeInBits();
            if( src0BitSize <= demotedBitSize || src1BitSize <= demotedBitSize )
                continue;

            Value* DemotedLHS = getDemotedValue(BO->getOperand(0), DemotedTy, true);
            if (!DemotedLHS)
                continue;

            Value* DemotedRHS = getDemotedValue(BO->getOperand(1), DemotedTy, true);
            if (!DemotedRHS)
                continue;

            Type* OriginTy = BO->getType();
            BO->setOperand(0, DemotedLHS);
            BO->setOperand(1, DemotedRHS);
            BO->mutateType(DemotedTy);
            // Create the unsigned extension.
            BuilderType::InsertPointGuard Guard(*IRB);
            IRB->SetInsertPoint(&(*std::next(BasicBlock::iterator(I))));
            Value* V = IRB->CreateZExt(BO, OriginTy, ".demoted.zext");
            for (auto UI = BO->use_begin(), UE = BO->use_end(); UI != UE; /* EMPTY */) {
                Use& U = *UI++;
                if (U.getUser() == V)
                    continue;
                U.set(V);
            }
            BI = std::next(BasicBlock::iterator(BO));
            if (BO->getOpcode() == Instruction::And) {
                if (ConstantInt * CI = dyn_cast<ConstantInt>(DemotedRHS))
                    if (CI->isAllOnesValue()) {
                        BO->replaceAllUsesWith(DemotedLHS);
                        BO->eraseFromParent();
                    }
            }
            Changed = true;
            continue;
        }

        if (TruncInst * TI = dyn_cast<TruncInst>(I)) {
            if (!TI->getType()->isIntegerTy(8))
                continue;
            if (!TI->getSrcTy()->isIntegerTy(32))
                continue;
            Value* DemotedVal = getDemotedValue(TI->getOperand(0), DemotedTy, true);
            if (!DemotedVal)
                continue;

            TI->replaceAllUsesWith(DemotedVal);
            TI->eraseFromParent();

            Changed = true;
            continue;
        }

        if (ZExtInst * ZEI = dyn_cast<ZExtInst>(I)) {
            if (!ZEI->getSrcTy()->isIntegerTy(8))
                continue;
            // TODO
        }

        // j.0163 = phi i16 [ 0, %._crit_edge155 ], [ %337, %..lr.ph_crit_edge ]
        // %57 = zext i16 %j.0163 to i64
        // %58 = extractelement <32 x i16> <i16 3, ..., i16 44, i16 50>, i64 %57
        //
        if (auto EEI = dyn_cast<ExtractElementInst>(I)) {
            Value* Index = EEI->getIndexOperand();
            CastInst* CI = dyn_cast<CastInst>(Index);
            if (CI && (CI->getOpcode() == Instruction::ZExt ||
                CI->getOpcode() == Instruction::SExt)) {
                unsigned VS = (unsigned)cast<IGCLLVM::FixedVectorType>(EEI->getVectorOperandType())->getNumElements();
                unsigned N = (unsigned int)CI->getSrcTy()->getPrimitiveSizeInBits();
                unsigned Bound = (N < 32) ? (1U << N) : UINT32_MAX;
                if (VS <= Bound) {
                    EEI->setOperand(1, CI->getOperand(0));
                    if (CI->use_empty())
                        CI->eraseFromParent();
                    Changed = true;
                }
            }
            continue;
        }
    }

    return Changed;
}

Value* TypeDemote::getDemotedValue(Value* V, Type* DemotedTy, bool Unsigned) const {
    IGC_ASSERT(nullptr != DemotedTy);
    IGC_ASSERT_MESSAGE(DemotedTy->isIntegerTy(8), "Only support demotion to i8!");
    IGC_ASSERT_MESSAGE(Unsigned, "Only support demotion to i8!");

    if (ConstantInt * CI = dyn_cast<ConstantInt>(V)) {
        if (!CI->getValue().isIntN(8))
            return nullptr;
        V = IRB->CreateTrunc(V, DemotedTy);
        IGC_ASSERT_MESSAGE(isa<ConstantInt>(V), "Constant folding is failed!");
        return V;
    }

    if (ZExtInst * ZEI = dyn_cast<ZExtInst>(V)) {
        if (!ZEI->getSrcTy()->isIntegerTy(8))
            return nullptr;
        return ZEI->getOperand(0);
    }

    return nullptr;
}
