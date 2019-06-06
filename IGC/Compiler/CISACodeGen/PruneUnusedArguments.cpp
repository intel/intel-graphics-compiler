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

#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace {

class PruneUnusedArguments : public ModulePass {
public:
  static char ID;
  PruneUnusedArguments();
  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CallGraphWrapperPass>();
    AU.addPreserved<CallGraphWrapperPass>();
    AU.setPreservesCFG();
  }
};

} // namespace

namespace IGC {
llvm::ModulePass *createPruneUnusedArgumentsPass() {
  return new PruneUnusedArguments();
}
} // namespace IGC

IGC_INITIALIZE_PASS_BEGIN(PruneUnusedArguments, "PruneUnusedArguments", "PruneUnusedArguments", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_END(PruneUnusedArguments, "PruneUnusedArguments", "PruneUnusedArguments", false, false)

char PruneUnusedArguments::ID = 0;

/// Optimize unused arguments, all indirectly unused arguments will be replaced
/// with undef and vISA emission will skip these copies. This is in fact a
/// cleanup to the unification passes, which run early in the pipeline. With
/// subroutines, this issue becomes worse.
///
/// Note that (1) we do not delete those dead arguments since there may have
/// metadata on those functions; (2) the way we do private memory resolution may
/// introduce uses to r0 or payloadHeader(!!). For this reason, we run this pass
/// after private meomry resolution and no other pass should introduce such uses
/// after. If r0 is given by an intrinsic, then this is not an issue by design.
///
bool PruneUnusedArguments::runOnModule(Module &M) {
  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

  // Visit functions in a post order DFS, i.e. reversed topological ordering. If
  // an argument is not used then let its callers pass undef.
  bool Changed = false;
  for (auto I = po_begin(CG.getExternalCallingNode()),
            E = po_end(CG.getExternalCallingNode());
       I != E; ++I) {
    auto CGNode = *I;
    // Skip external and indirect nodes.
    if (auto F = CGNode->getFunction()) {
      // Conservatively declarations use all arguments.
      if (F->isDeclaration())
        continue;
      // Ignore externally linked functions
      if (F->hasFnAttribute("ExternalLinkedFn"))
        continue;

      // Collect unused arguments and their indices.
      SmallVector<std::pair<Argument *, unsigned>, 8> UnusedArgs;
      unsigned Index = 0;
      for (auto &Arg : F->args()) {
        if (Arg.use_empty())
          UnusedArgs.push_back(std::make_pair(&Arg, Index));
        Index++;
      }

      if (UnusedArgs.empty())
        continue;

      // Update call sites.
      for (auto U : F->users()) {
        CallInst *CI = dyn_cast<CallInst>(U);
        if (!CI)
          continue;
        for (auto Item : UnusedArgs) {
          auto Arg = Item.first;
          auto Index = Item.second;
          if (!isa<UndefValue>(CI->getArgOperand(Index))) {
            CI->setArgOperand(Index, UndefValue::get(Arg->getType()));
            Changed = true;
          }
        }
      }
    }
  }

  return Changed;
}

PruneUnusedArguments::PruneUnusedArguments() : ModulePass(ID) {
  initializePruneUnusedArgumentsPass(*PassRegistry::getPassRegistry());
}
