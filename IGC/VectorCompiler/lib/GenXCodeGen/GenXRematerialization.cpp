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
//
/// GenXRematerialization
/// ---------------------
///
/// This pass performs rematerialization to reduce register pressure.
///
//===----------------------------------------------------------------------===//
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXLiveness.h"
#include "GenXModule.h"
#include "GenXNumbering.h"
#include "GenXPressureTracker.h"
#include "GenXUtil.h"
#include "llvm/Pass.h"

using namespace llvm;
using namespace genx;

namespace {

class GenXRematerialization : public FunctionGroupPass {
  GenXBaling *Baling = nullptr;
  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  bool Modified = false;

public:
  static char ID;
  explicit GenXRematerialization() : FunctionGroupPass(ID) {}
  StringRef getPassName() const override {
    return "GenX rematerialization pass";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunctionGroup(FunctionGroup &FG) override;

private:
  void remat(Function *F, PressureTracker &RP);
};

} // namespace

namespace llvm { void initializeGenXRematerializationPass(PassRegistry &); }
char GenXRematerialization::ID = 0;
INITIALIZE_PASS_BEGIN(GenXRematerialization, "GenXRematerialization", "GenXRematerialization", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBaling)
INITIALIZE_PASS_DEPENDENCY(GenXLiveness)
INITIALIZE_PASS_DEPENDENCY(GenXNumbering)
INITIALIZE_PASS_END(GenXRematerialization, "GenXRematerialization", "GenXRematerialization", false, false)

FunctionGroupPass *llvm::createGenXRematerializationPass() {
  initializeGenXRematerializationPass(*PassRegistry::getPassRegistry());
  return new GenXRematerialization;
}

void GenXRematerialization::getAnalysisUsage(AnalysisUsage &AU) const {
  FunctionGroupPass::getAnalysisUsage(AU);
  AU.addRequired<GenXGroupBaling>();
  AU.addRequired<GenXLiveness>();
  AU.addRequired<GenXNumbering>();
  AU.addPreserved<GenXModule>();
  AU.addPreserved<FunctionGroupAnalysis>();
  AU.setPreservesCFG();
}

bool GenXRematerialization::runOnFunctionGroup(FunctionGroup &FG) {
  if (skipOptWithLargeBlock(FG))
    return false;

  Modified = false;
  Baling = &getAnalysis<GenXGroupBaling>();
  Liveness = &getAnalysis<GenXLiveness>();
  Numbering = &getAnalysis<GenXNumbering>();
  PressureTracker RP(FG, Liveness);
  for (auto fgi = FG.begin(), fge = FG.end(); fgi != fge; ++fgi)
    remat(*fgi, RP);
  return Modified;
}

void GenXRematerialization::remat(Function *F, PressureTracker &RP) {
  // Collect rematerialization candidates.
  std::vector<Use *> Candidates;
  for (auto &BB : F->getBasicBlockList()) {
    for (auto &Inst : BB.getInstList()) {
      // (1) upward cast
      if (auto CI = dyn_cast<CastInst>(&Inst)) {
        if (CI->getOpcode() != Instruction::UIToFP &&
            CI->getOpcode() != Instruction::SIToFP)
          continue;
        if (!CI->getType()->isVectorTy())
          continue;
        if (CI->getSrcTy()->getScalarSizeInBits() >=
            CI->getDestTy()->getScalarSizeInBits())
          continue;
        if (Inst.isUsedOutsideOfBlock(&BB) || Inst.getNumUses() <= 2)
          continue;
        LiveRange *LR = Liveness->getLiveRangeOrNull(CI);
        if (!LR || LR->value_size() != 1)
          continue;
        assert(*LR->value_begin() == CI);
        unsigned B = Numbering->getNumber(CI);
        for (auto &U : CI->uses()) {
          auto UI = U.getUser();
          unsigned E = Numbering->getNumber(UI);
          if (E > B && RP.intersectWithRedRegion(B, E))
            Candidates.push_back(&U);
        }
      }
    }
  }

  // Do rematerialization.
  for (auto U : Candidates) {
    Instruction *Inst = cast<Instruction>(U->get());
    Instruction *UI = cast<Instruction>(U->getUser());
    Instruction *Clone = Inst->clone();
    Clone->insertBefore(UI);
    U->set(Clone);
    Modified = true;
  }
}