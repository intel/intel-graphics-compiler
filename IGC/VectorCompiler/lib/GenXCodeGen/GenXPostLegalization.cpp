/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPostLegalization
/// --------------------
///
/// GenXPostLegalization is a function pass run after legalization with the
/// following purposes:
///
/// 1. It inserts a constant load for most constants that are not representable
///    as a constant operand in GenX code. See the GenXConstants section below.
//     (in the file GenXConstants.cpp)
///
/// 2. It calls GenXVectorDecomposer to perform vector decomposition. See the
///    GenXVectorDecomposer section below.
//     (in the file GenXVectorDecomposer.h)
///
/// Both of these things are done here because the results of them (constant
/// loads and decomposed vector operations) may benefit from CSE run after
/// this pass.
///
//===----------------------------------------------------------------------===//
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXConstants.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "GenXVectorDecomposer.h"
#include "vc/Utils/GenX/BreakConst.h"

#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "GENX_POST_LEGALIZATION"

#include <set>

using namespace llvm;
using namespace genx;
using namespace GenXIntrinsic::GenXRegion;
using namespace vc;

namespace {

// GenXPostLegalization : post-legalization pass
class GenXPostLegalization : public FunctionPass {
  const DataLayout *DL = nullptr;
  const GenXSubtarget *ST = nullptr;
public:
  static char ID;
  explicit GenXPostLegalization() : FunctionPass(ID) { }
  StringRef getPassName() const override {
    return "GenX post-legalization pass";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
};

} // end namespace llvm


char GenXPostLegalization::ID = 0;
namespace llvm { void initializeGenXPostLegalizationPass(PassRegistry &); }
INITIALIZE_PASS_BEGIN(GenXPostLegalization, "GenXPostLegalization",
                      "GenXPostLegalization", false, false)
INITIALIZE_PASS_END(GenXPostLegalization, "GenXPostLegalization",
                    "GenXPostLegalization", false, false)

FunctionPass *llvm::createGenXPostLegalizationPass() {
  initializeGenXPostLegalizationPass(*PassRegistry::getPassRegistry());
  return new GenXPostLegalization;
}

void GenXPostLegalization::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
}

/***********************************************************************
 * GenXPostLegalization::runOnFunction : process one function
 */
bool GenXPostLegalization::runOnFunction(Function &F)
{
  DL = &F.getParent()->getDataLayout();
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();

  VectorDecomposer VD(ST);

  bool Modified = false;
  Modified |= vc::breakConstantExprs(&F, vc::LegalizationStage::Legalized);

  for (Function::iterator fi = F.begin(), fe = F.end(); fi != fe; ++fi) {
    BasicBlock *BB = &*fi;
    for (BasicBlock::iterator bi = BB->begin(), be = BB->end(); bi != be; ++bi) {
      Instruction *Inst = &*bi;
      switch (vc::getAnyIntrinsicID(Inst)) {
      default:
        // Lower non-simple constant operands.
        Modified |= loadNonSimpleConstants(Inst, *ST, *DL, nullptr);
        break;
      case Intrinsic::fma:
      case GenXIntrinsic::genx_dpas:
      case GenXIntrinsic::genx_dpas2:
      case GenXIntrinsic::genx_dpas_nosrc0:
      case GenXIntrinsic::genx_dpasw:
      case GenXIntrinsic::genx_dpasw_nosrc0:
        Modified |= loadConstants(Inst, *ST, *DL);
        break;
      }

      // If this is a wrregion with constant input, or phi node input, give it
      // to the vector decomposer. (We could just give it all wrregions, but we
      // are trying to minimize the amount of work it has to do.)
      if (!ST->disableVectorDecomposition()) {
        if (GenXIntrinsic::isWrRegion(Inst)) {
          if (isa<Constant>(Inst->getOperand(0)))
            VD.addStartWrRegion(Inst);
          else if (isa<PHINode>(Inst->getOperand(0)))
            VD.addStartWrRegion(Inst);
        }
      }
    }
  }
  // Run the vector decomposer for this function.
  Modified |= VD.run(*DL);
  // Cleanup region reads and writes.
  Modified |= simplifyRegionInsts(&F, DL, ST);
  // Cleanup constant loads.
  std::vector<CallInst *> ConstList;
  Modified |= cleanupConstantLoads(&F, ConstList);
  // Replace shifted constants to add-instruction.
  Modified |= checkAddPattern(&F, ConstList, ST);
  // Legalize constants in return.
  for (auto FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
    BasicBlock *BB = &*FI;
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
      Instruction *Inst = &*BI;
      if (isa<ReturnInst>(Inst)) {
        Modified |= loadNonSimpleConstants(Inst, *ST, *DL, nullptr);
        Modified |= loadConstants(Inst, *ST, *DL);
      }
    }
  }

  return Modified;
}
