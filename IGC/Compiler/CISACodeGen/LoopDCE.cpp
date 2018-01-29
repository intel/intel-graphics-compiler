#define DEBUG_TYPE "slm-blocking"
#include "Compiler/CISACodeGen/LoopDCE.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsics.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

class LoopDeadCodeElimination : public FunctionPass {
  CodeGenContext *CGC;
  const DataLayout *DL;
  LoopInfo *LI;
  PostDominatorTree *PDT;

public:
  static char ID;

  LoopDeadCodeElimination() : FunctionPass(ID),
      CGC(nullptr), DL(nullptr), LI(nullptr), PDT(nullptr) {
    initializeLoopDeadCodeEliminationPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.addPreserved<PostDominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
  }

private:
  bool processLoop(Loop *L);
};

} // End anonymous namespace

char LoopDeadCodeElimination::ID = 0;

#define PASS_FLAG     "igc-loop-dce"
#define PASS_DESC     "Advanced DCE on loop"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
IGC_INITIALIZE_PASS_BEGIN(LoopDeadCodeElimination, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(LoopDeadCodeElimination, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

FunctionPass *IGC::createLoopDeadCodeEliminationPass() {
  return new LoopDeadCodeElimination();
}

bool LoopDeadCodeElimination::runOnFunction(Function &F) {
  // Skip non-kernel function.
  MetaDataUtils *MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto FII = MDU->findFunctionsInfoItem(&F);
  if (FII == MDU->end_FunctionsInfo())
    return false;

  CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  DL = &F.getParent()->getDataLayout();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  bool Changed = false;
  // DFT all loops.
  for (auto I = LI->begin(), E = LI->end(); I != E; ++I)
    for (auto L = df_begin(*I), F = df_end(*I); L != F; ++L)
      Changed |= processLoop(*L);

  return Changed;
}

static Instruction *getSingleUserInLoop(Value *V, Loop *L) {
  Instruction *UserInLoop = nullptr;
  for (auto U : V->users()) {
    auto I = dyn_cast<Instruction>(U);
    if (!I)
      return nullptr;
    if (!L->contains(I->getParent()))
      continue;
    if (UserInLoop)
      return nullptr;
    UserInLoop = I;
  }
  return UserInLoop;
}

bool LoopDeadCodeElimination::processLoop(Loop *L) {
  SmallVector<BasicBlock *, 8> ExitingBlocks;
  L->getExitingBlocks(ExitingBlocks);
  if (ExitingBlocks.empty())
    return false;

  bool Changed = false;
  for (auto BB : ExitingBlocks) {
    auto BI = dyn_cast<BranchInst>(BB->getTerminator());
    // Skip exiting block with non-conditional branch.
    if (!BI || !BI->isConditional())
      continue;
    bool ExitingOnTrue = !L->contains(BI->getSuccessor(0));
    auto Cond = BI->getCondition();
    for (auto U : Cond->users()) {
      auto SI = dyn_cast<SelectInst>(U);
      if (!SI)
        continue;
      // TODO: Handle the trivial case where 'select' is used as a loop-carried
      // value.
      auto I = getSingleUserInLoop(SI, L);
      if (!I)
        continue;
      auto PN = dyn_cast<PHINode>(I);
      if (!PN || L->getHeader() != PN->getParent())
        continue;
      // v2 := phi(v0/bb0, v1/bb1)
      // ...
      // bb1:
      // v1 := select(cond, va, vb);
      // br cond, out_of_loop
      //
      // replace all uses of v1 in loop with vb
      // replace all uses of v1 out of loop with va
      Value *NewValInLoop = SI->getFalseValue();
      Value *NewValOutLoop = SI->getTrueValue();
      if (!ExitingOnTrue)
        std::swap(NewValInLoop, NewValOutLoop);
      for (auto UI = SI->use_begin(), UE = SI->use_end(); UI != UE; /*EMPTY*/) {
        auto &Use = *UI++;
        auto I = cast<Instruction>(Use.getUser());
        auto NewVal =
            L->contains(I->getParent()) ? NewValInLoop : NewValOutLoop;
        Use.set(NewVal);
      }
    }
  }
  return Changed;
}
