/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "CISACodeGen/helper.h"
#include "common/IGCIRBuilder.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Probe/Assertion.h"
#include "AddRequiredMemoryFences.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Support/Debug.h"
#include "common/LLVMWarningsPop.hpp"

#define DEBUG_TYPE "igc-add-required-memory-fences"

using namespace llvm;

namespace IGC {
////////////////////////////////////////////////////////////////////////////////
// @brief This pass inserts SLM fences after the last SLM store or SLM atomic
// instruction(s) in the function.
class AddRequiredMemoryFences : public llvm::FunctionPass {
public:
  static char ID;

  AddRequiredMemoryFences();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.addPreserved<PostDominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
  }

  StringRef getPassName() const { return "AddRequiredMemoryFences"; }

  bool runOnFunction(Function &F);
};
char AddRequiredMemoryFences::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-add-required-memory-fences"
#define PASS_DESCRIPTION "Add memory fences required by the HW memory model"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(AddRequiredMemoryFences, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(AddRequiredMemoryFences, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_ANALYSIS
#undef PASS_CFG_ONLY
#undef PASS_DESCRIPTION
#undef PASS_FLAG

////////////////////////////////////////////////////////////////////////////////
AddRequiredMemoryFences::AddRequiredMemoryFences() : FunctionPass(ID) {
  initializeAddRequiredMemoryFencesPass(*PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////////////
FunctionPass *createAddRequiredMemoryFencesPass() { return new AddRequiredMemoryFences(); }

////////////////////////////////////////////////////////////////////////////////
inline bool IsSlmFence(Instruction *inst) {
  if (GenIntrinsicInst *intr = dyn_cast<GenIntrinsicInst>(inst)) {
    const GenISAIntrinsic::ID id = intr->getIntrinsicID();
    if (id == GenISAIntrinsic::GenISA_LSCFence && LSC_SFID::LSC_SLM == getImmValueEnum<LSC_SFID>(intr->getOperand(0))) {
      return true;
    } else if (id == GenISAIntrinsic::GenISA_memoryfence && false == getImmValueBool(intr->getOperand(5))) {
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
inline bool IsSlmStoreOrAtomic(Instruction *inst) {
  Instruction *store = nullptr;
  if (GenIntrinsicInst *intr = dyn_cast<GenIntrinsicInst>(inst)) {
    const GenISAIntrinsic::ID id = intr->getIntrinsicID();
    if (id == GenISAIntrinsic::GenISA_LSCStore || id == GenISAIntrinsic::GenISA_LSCStoreBlock ||
        id == GenISAIntrinsic::GenISA_simdBlockWrite) {
      store = intr;
    }
    // This pass assumes that the input shader is optimized, only
    // instructions with no uses are considered as needing the fence.
    else if (intr->getNumUses() == 0 && IsStatelessMemAtomicIntrinsic(*intr, id)) {
      store = intr;
    }
  } else {
    store = dyn_cast<StoreInst>(inst);
  }
  if (store) {
    Value *ptr = GetBufferOperand(store);
    IGC_ASSERT(ptr && ptr->getType()->isPointerTy());
    if (ptr && ptr->getType()->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL) {
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool AddRequiredMemoryFences::runOnFunction(Function &F) {
  PostDominatorTree *const PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  LoopInfo *const LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  bool modified = false;

  // The high-level algorithm is:
  // for each function exit:
  //  - walk the post-dominator tree in the depth-first order, for each basic
  //    block check instructions, start from the last instruction:
  //    - break if a basic block contains an SLM fence
  //    - or break is a basic block contains an SLM store or SLM atomic
  //      instruction, remember the store/atomic instruction
  //  - find the common post-dominator block for all unfenced SLM store
  //    or atomic instructions
  //  - if the common post-dominator block is in a loop find the outermost
  //    loop, and find the common post-dominator block for all loop exits
  //  - insert an SLM fence at the end of the common post-dominator block
  for (BasicBlock *rootBB : PDT->roots()) {
    if (isa<UnreachableInst>(rootBB->getTerminator())) {
      continue;
    }
    SmallPtrSet<BasicBlock *, 16> seen{rootBB};
    SmallVector<BasicBlock *, 16> worklist{rootBB};
    SmallVector<BasicBlock *, 8> unfenced;
    while (!worklist.empty()) {
      bool hasUnfencedSlmStore = false;
      bool hasSlmFence = false;
      BasicBlock *BB = worklist.back();
      worklist.pop_back();
      seen.insert(BB);
      for (auto II = BB->rbegin(), IE = BB->rend(); II != IE; ++II) {
        if (IsSlmFence(&(*II))) {
          hasSlmFence = true;
          break;
        } else if (IsSlmStoreOrAtomic(&(*II))) {
          hasUnfencedSlmStore = true;
          break;
        }
      }
      if (hasUnfencedSlmStore) {
        unfenced.push_back(BB);
      } else if (!hasSlmFence) {
        for (BasicBlock *pred : predecessors(BB)) {
          if (seen.count(pred) == 0) {
            worklist.push_back(pred);
          }
        }
      }
    }
    if (!unfenced.empty()) {
      // Lambda finds a common post-dominator block for a set of basic blocks.
      // Returns nullptr if blocks is empty or no common dominator exists.
      auto FindPostDominator = [&PDT](const auto &blocks) -> BasicBlock * {
        if (blocks.empty()) {
          return nullptr;
        }
        auto it = blocks.begin();
        BasicBlock *postDomBB = *it++;
        for (; it != blocks.end() && postDomBB != nullptr; ++it) {
          postDomBB = PDT->findNearestCommonDominator(postDomBB, *it);
        }
        return postDomBB;
      };
      BasicBlock *postDomBB = FindPostDominator(unfenced);
      LLVM_DEBUG(dbgs() << "AddRequiredMemoryFences: " << unfenced.size()
                        << " unfenced SLM store block(s) in function "
                        << F.getName() << "\n");
      if (postDomBB != nullptr) {
        Loop *L = LI->getLoopFor(postDomBB);
        if (L) {
          while (!L->isOutermost()) {
            L = L->getParentLoop();
          }
          SmallVector<BasicBlock *, 4> exitBlocks;
          L->getUniqueExitBlocks(exitBlocks);
          LLVM_DEBUG(dbgs() << "AddRequiredMemoryFences: outermost loop has "
                            << exitBlocks.size() << " unique exit block(s)\n");
          if (!exitBlocks.empty()) {
            postDomBB = FindPostDominator(exitBlocks);
          }
          // If the loop has no exit blocks (infinite loop / unreachable exits),
          // keep the postDomBB from above — placing the fence at the
          // post-dominator of the unfenced stores is still correct.
        }
      }
      if (postDomBB == nullptr) {
        // Common post-dominator may not exist if kernel has the unreachable
        // instruction.
        postDomBB = rootBB;
      }
      IGC_ASSERT(postDomBB);
      LLVM_DEBUG(dbgs() << "AddRequiredMemoryFences: inserting SLM fence in BB "
                        << postDomBB->getName() << "\n");
      IGCIRBuilder<> IRB(postDomBB->getTerminator());
      Function *fenceFuncPtr = GenISAIntrinsic::getDeclaration(F.getParent(), GenISAIntrinsic::GenISA_LSCFence);
      Value *args[] = {IRB.getInt32(LSC_SLM), IRB.getInt32(LSC_SCOPE_GROUP), IRB.getInt32(LSC_FENCE_OP_NONE)};
      IRB.CreateCall(fenceFuncPtr, args);
      modified = true;
    }
  }
  return modified;
}
} // namespace IGC
