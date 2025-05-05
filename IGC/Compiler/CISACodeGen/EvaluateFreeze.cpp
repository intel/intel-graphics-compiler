/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "EvaluateFreeze.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include <vector>

using namespace llvm;

class EvaluateFreeze : public FunctionPass {

public:
    static char ID;

    EvaluateFreeze() : FunctionPass(ID) {
        IGC::initializeEvaluateFreezePass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function&) override;

    void getAnalysisUsage(AnalysisUsage& AU) const override {
        AU.setPreservesCFG();
    }

private:
    void evaluateInBasicBlock(
        BasicBlock *,
        std::vector<llvm::FreezeInst *> &RemList);
    void evaluateFreezeInstUndef(FreezeInst*) const;
    void evaluateFreezeInstNotUndef(FreezeInst*) const;
};

char EvaluateFreeze::ID = 0;

#define PASS_FLAG     "igc-evaluate-freeze"
#define PASS_DESC     "Evaluate Freeze Instructions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(EvaluateFreeze, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_END(EvaluateFreeze, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

llvm::FunctionPass* IGC::createEvaluateFreezePass()
{
    return new EvaluateFreeze();
}

bool EvaluateFreeze::runOnFunction(Function& F) {
    std::vector<llvm::FreezeInst *> RemList;

    for (auto& BB : F)
        evaluateInBasicBlock(&BB, RemList);

    for (FreezeInst *FI : RemList)
        FI->eraseFromParent();

    return !RemList.empty();
}

// A freeze is idempotent on anything that is not %poison or %undef
// For those we return an arbitrary value.  We actually make it deterministic
// for all cases just for sanity sake.
//
//   %freeze_dst = freeze i32 %valid
//       => replace all uses of %freeze_dst with %valid
//
//   %freeze_dst = freeze T %undef
//       where T is:
//              iX = replace with constant some magic int (i1 is 0)
//              fX = replace with constant 0.0
//
void EvaluateFreeze::evaluateInBasicBlock(
    BasicBlock* BB,
    std::vector<llvm::FreezeInst *> &RemList)
{
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; /* EMPTY */) {
        Instruction* I = &(*BI++);
        if (FreezeInst *FI = dyn_cast<FreezeInst>(I)) {
            if (isa<UndefValue>(FI->getOperand(0)))
                evaluateFreezeInstUndef(FI);
            else
                evaluateFreezeInstNotUndef(FI);
            RemList.push_back(FI);
        }
    }

}
void EvaluateFreeze::evaluateFreezeInstUndef(FreezeInst* FI) const
{
    // Undef or poison value
    // Replace all uses with a fixed value.
    Value *Op = FI->getOperand(0);
    Type *OpTy = Op->getType();
    if (OpTy->isIntegerTy()) {
        IntegerType *IT = (IntegerType *)OpTy;
        //
        // Give the folks debugging raw assembly a fighting chance
        uint64_t MagicVal =
            IT->getBitWidth() >= 64 ? 0xF4EE7E00F4EE7E00ull :
            IT->getBitWidth() >= 32 ? 0xF4EE7E00ull :
            IT->getBitWidth() >= 16 ? 0xF47Eull :
            0x0; // branches get false
        ConstantInt *CI = ConstantInt::get(IT, MagicVal, false);
        FI->replaceAllUsesWith(CI);
    } else if (OpTy->isFloatingPointTy()) {
        Constant *C = Constant::getNullValue(OpTy);
        FI->replaceAllUsesWith(C);
    } else {
        IGC_ASSERT_MESSAGE(0, "unsupported type in freeze");
    }
}
void EvaluateFreeze::evaluateFreezeInstNotUndef(FreezeInst* FI) const
{
    // Not an undef value ==> just propagate the operand as the result
    llvm::Value *Op = FI->getOperand(0);
    FI->replaceAllUsesWith(Op);
}
