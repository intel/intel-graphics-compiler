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

#include "VariableReuseAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;

char VariableReuseAnalysis::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(VariableReuseAnalysis, "VariableReuseAnalysis",
                          "VariableReuseAnalysis", false, true)
// IGC_INITIALIZE_PASS_DEPENDENCY(RegisterEstimator)
IGC_INITIALIZE_PASS_END(VariableReuseAnalysis, "VariableReuseAnalysis",
                        "VariableReuseAnalysis", false, true)

llvm::FunctionPass *IGC::createVariableReuseAnalysisPass() {
  return new VariableReuseAnalysis;
}

VariableReuseAnalysis::VariableReuseAnalysis()
    : FunctionPass(ID), m_RPE(nullptr), m_SimdSize(0),
      m_IsFunctionPressureLow(Status::Undef),
      m_IsBlockPressureLow(Status::Undef) {
  initializeVariableReuseAnalysisPass(*PassRegistry::getPassRegistry());
}

bool VariableReuseAnalysis::runOnFunction(Function &F) {
  // FIXME: enable RPE.
  // m_RPE = &getAnalysis<RegisterEstimator>();

  // Nothing but cleanup data from previous runs.
  reset();
  return false;
}

static unsigned getMaxReuseDistance(uint16_t size) {
  return (size == 8) ? 10 : 5;
}

bool VariableReuseAnalysis::checkUseInst(Instruction *UseInst, LiveVars *LV) {
  BasicBlock *CurBB = UseInst->getParent();
  if (UseInst->isUsedOutsideOfBlock(CurBB))
    return false;

  // This situation can occur:
  //
  //     ,------.
  //     |      |
  //     |      v
  //     |   t2 = phi ... t1 ...
  //     |      |
  //     |      v
  //     |   t1 = ...
  //     |  ... = ... t1 ...
  //     |      |
  //     `------'
  //
  // Disallow reuse if t1 has a phi use.
  // Disallow reuse if t1 has a far away use when the pressure is not low.
  unsigned DefLoc = LV->getDistance(UseInst);
  unsigned FarUseLoc = 0;
  for (auto UI : UseInst->users()) {
    if (isa<PHINode>(UI))
      return false;

    auto Inst = dyn_cast<Instruction>(UI);
    if (!Inst)
      return false;
    unsigned UseLoc = LV->getDistance(Inst);
    FarUseLoc = std::max(FarUseLoc, UseLoc);
  }

  // When the whole function or block pressure is low, skip the distance check.
  if (isCurFunctionPressureLow() || isCurBlockPressureLow())
    return true;

  // Use distance to limit reuse.
  const unsigned FarUseDistance = getMaxReuseDistance(m_SimdSize);
  return FarUseLoc <= DefLoc + FarUseDistance;
}

bool VariableReuseAnalysis::checkDefInst(Instruction *DefInst,
                                         Instruction *UseInst, LiveVars *LV) {
  assert(DefInst && UseInst);
  if (isa<PHINode>(DefInst))
    return false;

  if (auto CI = dyn_cast<CallInst>(DefInst)) {
    Function *F = CI->getCalledFunction();
    // Do not reuse the return symbol of subroutine/stack calls.
    if (!F || !F->isDeclaration())
      return false;

    if (isa<GenIntrinsicInst>(DefInst)) {
      // Just skip all gen intrinsic calls. Some intrinsic calls may have
      // special meaning.
      return false;
    }
  }

  // This is a block level reuse.
  BasicBlock *CurBB = UseInst->getParent();
  if (DefInst->getParent() != CurBB || DefInst->isUsedOutsideOfBlock(CurBB))
    return false;

  // Check whether UseInst is the last use of DefInst. If not, this source
  // variable cannot be reused.
  Instruction *LastUse = LV->getLVInfo(DefInst).findKill(CurBB);
  if (LastUse != UseInst)
    return false;

  // When the whole function or block pressure is low, skip the distance check.
  if (isCurFunctionPressureLow() || isCurBlockPressureLow())
    return true;

  // Use distance to limit far reuses.
  unsigned DefLoc = LV->getDistance(DefInst);
  unsigned UseLoc = LV->getDistance(UseInst);
  const unsigned FarDefDistance = getMaxReuseDistance(m_SimdSize);
  return UseLoc <= DefLoc + FarDefDistance;
}
