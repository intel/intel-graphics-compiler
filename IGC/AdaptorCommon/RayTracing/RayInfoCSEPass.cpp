/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This is really just a stripped down version of the CSE pass.  If we do CSE
/// in general prior to continuation splitting, the increased live ranges of
/// CSE'd values can create spills that we don't want.  Here, we CSE just the
/// values that we had to hoist to minimize duplicated spills.
///
//===----------------------------------------------------------------------===//

#include "RTBuilder.h"
#include "Compiler/IGCPassSupport.h"
#include "iStdLib/utility.h"
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

#define DEBUG_TYPE "rayinfo-cse"

class RayInfoCSEPass : public FunctionPass
{
public:
    RayInfoCSEPass() : FunctionPass(ID)
    {
        initializeRayInfoCSEPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function &F) override;
    StringRef getPassName() const override
    {
        return "RayInfoCSEPass";
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
    }

    static char ID;
private:
    bool Changed;
};

char RayInfoCSEPass::ID = 0;

struct CSEVal
{
    Instruction* Inst;

    CSEVal(Instruction* I) : Inst(I) { }

    bool isSentinel() const
    {
        return Inst == DenseMapInfo<Instruction*>::getEmptyKey() ||
               Inst == DenseMapInfo<Instruction*>::getTombstoneKey();
    }
};

namespace llvm {
    // Tell DenseMap how to compare instructions to cheaply tell whether we
    // can merge an instruction with one we've already seen.
template <> struct DenseMapInfo<CSEVal> {
    static inline CSEVal getEmptyKey() {
        return DenseMapInfo<Instruction*>::getEmptyKey();
    }

    static inline CSEVal getTombstoneKey() {
        return DenseMapInfo<Instruction*>::getTombstoneKey();
    }

    static unsigned getHashValue(CSEVal Val);
    static bool isEqual(CSEVal LHS, CSEVal RHS);
};

} // end namespace llvm

unsigned DenseMapInfo<CSEVal>::getHashValue(CSEVal Val) {
  Instruction *Inst = Val.Inst;

  // Mix in the opcode.
  return hash_combine(
      Inst->getOpcode(),
      hash_combine_range(Inst->value_op_begin(), Inst->value_op_end()));
}

bool DenseMapInfo<CSEVal>::isEqual(CSEVal LHS, CSEVal RHS)
{
    Instruction* LHSI = LHS.Inst, * RHSI = RHS.Inst;

    if (LHS.isSentinel() || RHS.isSentinel())
        return LHSI == RHSI;

    if (LHSI->getOpcode() != RHSI->getOpcode())
        return false;
    if (LHSI->isIdenticalToWhenDefined(RHSI))
        return true;

    return false;
}

// Register pass to igc-opt
#define PASS_FLAG "rayinfo-cse"
#define PASS_DESCRIPTION "Lightweight CSE on entry block rayinfo intrinsics"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RayInfoCSEPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(RayInfoCSEPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool RayInfoCSEPass::runOnFunction(Function &F)
{
    Changed = false;
    auto& EntryBB = F.getEntryBlock();

    uint32_t MemGeneration = 0;

    DenseMap<CSEVal, Instruction*> AvailableValues;
    DenseMap<CSEVal, std::pair<Instruction*, uint32_t>> AvailableCalls;

    for (auto II = EntryBB.begin(); II != EntryBB.end(); /* empty */)
    {
        auto* I = &*II++;
        if (I->mayWriteToMemory())
        {
            // Conservatively, don't CSE values that read from memory that
            // have a write in between them.
            MemGeneration++;
            continue;
        }

        auto* GII = dyn_cast<GenIntrinsicInst>(I);
        if (!GII)
            continue;

        switch (GII->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_RayInfo:
        case GenISAIntrinsic::GenISA_RayTCurrent:
        case GenISAIntrinsic::GenISA_LocalBufferPointer:
            break;
        default:
            continue;
        }

        if (GII->doesNotAccessMemory())
        {
            // If this is a simple instruction that we can value number, process it.
            // See if the instruction has an available value.  If so, use it.
            if (Value *V = AvailableValues.lookup(I)) {
                I->replaceAllUsesWith(V);
                I->eraseFromParent();
                Changed = true;
                continue;
            }

            // Otherwise, just remember that this value is available.
            AvailableValues.insert(std::make_pair(I, I));
            continue;
        }
        else if (GII->onlyReadsMemory())
        {
            // If we have an available version of this call, and if it is the right
            // generation, replace this instruction.
            std::pair<Instruction*, unsigned> InVal = AvailableCalls.lookup(I);
            if (InVal.first != nullptr && (MemGeneration == InVal.second))
            {
                I->replaceAllUsesWith(InVal.first);
                I->eraseFromParent();
                Changed = true;
                continue;
            }

            // Otherwise, remember that we have this instruction.
            AvailableCalls.insert(
                std::make_pair(I, std::make_pair(I, MemGeneration)));
            continue;
        }
    }

    return Changed;
}

namespace IGC
{

Pass* createRayInfoCSEPass(void)
{
    return new RayInfoCSEPass();
}

} // namespace IGC
