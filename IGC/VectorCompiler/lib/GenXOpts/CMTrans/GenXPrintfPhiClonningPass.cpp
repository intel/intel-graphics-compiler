/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPrintfPhiClonningPass
/// --------------------
/// This pass finds every call to the printf function and verifies that printf
/// format def-chain does not contain phi operands. Otherwise, it clones the
/// block and duplicates the implementation so that the printf doesn't use phi
/// node.
/// Before:
///    br i1 %cond, label %1, label %2
///  1:
///    br label %2
///  2:
///    %3 = phi i8* [ @str1, %1 ], [ @str0, %0 ]
///    %print = call spir_func i32 (i8*, ...) @printf(i8* %phi)
///    ret
/// After:
///    br i1 %cond, label %1, label %2
///  1:
///    br label %3
///  2:         ; prev 0
///    %print = call spir_func i32 (i8*, ...) @printf(i8* %str0)
///    ret
///  3:         ; prev 1
///    %print = call spir_func i32 (i8*, ...) @printf(i8* %str1)
///    ret
//===----------------------------------------------------------------------===//

#include "llvmWrapper/IR/Operator.h"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/Printf.h"
#include "vc/Utils/General/IRBuilder.h"
#include <llvm/Transforms/Utils/Cloning.h>
#include <map>
#include "llvmWrapper/IR/Function.h"

#define DEBUG_TYPE "print-phi-clonning"

using namespace llvm;
using namespace vc;

namespace {
class GenXPrintfPhiClonning final : public ModulePass {
public:
  static char ID;
  GenXPrintfPhiClonning() : ModulePass(ID) {}
  StringRef getPassName() const override {
    return "GenX printf phi clonning pass";
  }
  void getAnalysisUsage(AnalysisUsage &) const override;
  bool runOnModule(Module &) override;

private:
  using PhiCandidatesType = ValueMap<Value *, PHINode *>;
  using EraseListType = SmallVector<PHINode *, 2>;
  using ClonsListType = SmallVector<BasicBlock *, 2>;

  void cloneBBs(CallInst *, PHINode *);
  void fillPhiCandidates(BasicBlock *, PhiCandidatesType &, unsigned);
  void updatePhiCandidates(BasicBlock *, ValueToValueMapTy &,
                           PhiCandidatesType &);
  void doRAUWOrigBB(BasicBlock *, PhiCandidatesType &, EraseListType &);
  void doRAUWClones(EraseListType &, ClonsListType &BBClons);
};
} // namespace

char GenXPrintfPhiClonning::ID = 0;
namespace llvm {
void initializeGenXPrintfPhiClonningPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXPrintfPhiClonning, "GenXPrintfPhiClonning",
                      "GenXPrintfPhiClonning", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXPrintfPhiClonning, "GenXPrintfPhiClonning",
                    "GenXPrintfPhiClonning", false, false)

namespace llvm {
ModulePass *createGenXPrintfPhiClonningPass() {
  initializeGenXPrintfPhiClonningPass(*PassRegistry::getPassRegistry());
  return new GenXPrintfPhiClonning;
}
} // namespace llvm

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses
GenXPrintfPhiClonningPass::run(llvm::Module &M,
                               llvm::AnalysisManager<llvm::Module> &) {
  GenXPrintfPhiClonning GenXPhi;
  if (GenXPhi.runOnModule(M)) {
    return PreservedAnalyses::all();
  }
  return PreservedAnalyses::none();
}
#endif

void GenXPrintfPhiClonning::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenXBackendConfig>();
}

Value *getFirstPhi(Value *NonPhi) {
  if (auto *BCO = dyn_cast<BitCastOperator>(NonPhi))
    return getFirstPhi(BCO->getOperand(0));
  if (auto *BCI = dyn_cast<BitCastInst>(NonPhi))
    return getFirstPhi(BCI->getOperand(0));
  if (isCastToGenericAS(*NonPhi))
    return getFirstPhi(
        cast<IGCLLVM::AddrSpaceCastOperator>(NonPhi)->getPointerOperand());

  if (isa<PHINode>(NonPhi))
    return NonPhi;

  return nullptr;
}

// For each value-to-value-map element create phi in succ:
//     before
//        - create phi and map value-phi (PhiCandidates)
//     in a loop when creating clones
//        - use map and set succ to phi
//     after loop
//        - do rauw for each phi in cloned block
//        - do rauw for map in orig bb
void GenXPrintfPhiClonning::fillPhiCandidates(BasicBlock *BB,
                                              PhiCandidatesType &PhiCandidates,
                                              unsigned CloneCount) {
  auto *SuccBB = BB->getUniqueSuccessor();
  // Exit-block
  if (!SuccBB)
    return;
  auto SuccPredsCount = std::distance(pred_begin(SuccBB), pred_end(SuccBB));
  IRBuilder<> IB(&SuccBB->front());

  std::set<Instruction *> RAUWDone;
  for (Instruction &It : *BB) {
    for (auto *User : It.users()) {
      // Here just check uses place, use itself do not needed
      if (auto *Inst = dyn_cast<Instruction>(User)) {
        if (Inst->getParent() == BB)
          continue;

        if (auto *PHI = dyn_cast<PHINode>(Inst)) {
          IGC_ASSERT(PHI->getNumIncomingValues() == CloneCount);
          PhiCandidates[&It] = PHI;
          continue;
        }

        IGC_ASSERT(SuccPredsCount == 1);
        auto Ty = It.getType();
        auto *PHI = IB.CreatePHI(Ty, CloneCount);
        PhiCandidates[&It] = PHI;
        // do not set succ in PHI now
        PHI->addIncoming(&It, BB);
        It.replaceUsesOutsideBlock(PHI, BB);
        break;
        // for current instruction we are done
      }
    }
  }
}

void GenXPrintfPhiClonning::updatePhiCandidates(
    BasicBlock *Clone, ValueToValueMapTy &VMap,
    PhiCandidatesType &PhiCandidates) {
  for (const auto &[Inst, Phi] : PhiCandidates) {
    Value *CloneInst = VMap[Inst];
    Phi->addIncoming(CloneInst, Clone);
  }
}

void GenXPrintfPhiClonning::doRAUWOrigBB(BasicBlock *BB,
                                         PhiCandidatesType &PhiCandidates,
                                         EraseListType &ToErase) {
  // 1-st setIncomming for mapped values
  for (const auto &[Inst, Phi] : PhiCandidates) {
    Phi->setIncomingValueForBlock(BB, Inst);
    IGC_ASSERT(Phi->isComplete());
  }

  for (PHINode &PHI : BB->phis()) {
    auto *IncVal = PHI.getIncomingValue(0);
    PHI.replaceAllUsesWith(IncVal);
    ToErase.push_back(&PHI);
  }
}

void GenXPrintfPhiClonning::doRAUWClones(EraseListType &ToErase,
                                         ClonsListType &BBClons) {
  for (size_t i = 1; i < BBClons.size() + 1; ++i) {
    auto *BB = BBClons[i - 1];
    for (PHINode &PHI : BB->phis()) {
      auto *IncVal = PHI.getIncomingValue(i);
      PHI.replaceAllUsesWith(IncVal);
      ToErase.push_back(&PHI);
    }
  }
}
/*
Example of clonning. Before:
        BB1
      /  BB2
      |   |  BB3
      V   V   V
        CurrBB
         Phi1 = [BB1 a, BB2 a1, BB3 a2]
         Phi2 = [BB1 b, BB2 b1, BB3 B2]

After:
    BB1       BB2       BB3
     V         V         V
  CurrBB    CurrBB1   CurrBB2
  use  a    use  a1   use  a2
  use  b    use  b1   use  b2

CurrBB - the current block remains at index `0`
Clones indexes start with 1. The incoming values are also taken with the same
indexes. Phi in succ-blocks will updated via PhiCandidates
*/
void GenXPrintfPhiClonning::cloneBBs(CallInst *CI, PHINode *Phi) {
  auto *BB = CI->getParent();
  IGC_ASSERT_EXIT_MESSAGE(BB->getUniqueSuccessor() ||
                              dyn_cast<ReturnInst>(BB->getTerminator()),
                          "Multiple successors for the printf instruction "
                          "blocks are not yet supported");

  // Mapping orig BB inst -> phi in succ block
  PhiCandidatesType PhiCandidates;
  ClonsListType BBClons;
  EraseListType ToErase;

  unsigned CloneCount = Phi->getNumIncomingValues();

  // 1-st create phis for initial bb
  fillPhiCandidates(BB, PhiCandidates, CloneCount);

  // Clone blocks for each incomming
  for (unsigned i = 1; i < CloneCount; ++i) {
    ValueToValueMapTy VMap;
    auto *CloneBB = CloneBasicBlock(BB, VMap);

    // Fix phis from incoming for every block
    IGCLLVM::insertBasicBlock(BB->getParent(), BB->getIterator(), CloneBB);

    for (auto &Inst : *CloneBB)
      RemapInstruction(&Inst, VMap,
                       RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);

    auto *IncBB = Phi->getIncomingBlock(i);
    auto *PrevTerm = cast<BranchInst>(IncBB->getTerminator());

    int EdgeNum = 0;
    for (;; ++EdgeNum)
      if (PrevTerm->getSuccessor(EdgeNum) == BB)
        break;
    PrevTerm->setSuccessor(EdgeNum, CloneBB);

    updatePhiCandidates(CloneBB, VMap, PhiCandidates);
    BBClons.push_back(CloneBB);
  }

  doRAUWOrigBB(BB, PhiCandidates, ToErase);
  doRAUWClones(ToErase, BBClons);

  for (auto *PHI : ToErase)
    PHI->eraseFromParent();
}

bool GenXPrintfPhiClonning::runOnModule(Module &M) {
  SmallVector<std::pair<CallInst *, PHINode *>, 0> ClonningCandidates;

  for (Function &F : M) {
    if (F.isDeclaration() && isPrintfName(F.getName()))
      for (auto *User : F.users()) {
        auto *C = dyn_cast<CallInst>(User);
        if (!C || C->getCalledFunction() != &F)
          continue;
        auto *Phi = getFirstPhi(C->getOperand(0));
        if (!Phi)
          continue;
        if (cast<PHINode>(Phi)->getParent() != C->getParent())
          vc::fatal(Phi->getContext(), "GenXPrintfPhiClonning",
                    "Def-chain for printf is too long", Phi);
        // Check current BB does not added
        if (llvm::any_of(ClonningCandidates, [&](auto &Pair) {
              return C->getParent() == Pair.first->getParent();
            }))
          continue;
        ClonningCandidates.push_back({C, cast<PHINode>(Phi)});
      }
  }
  if (ClonningCandidates.empty())
    return false;

  // We get here very, very rarely
  for (auto &[CallInst, Phi] : ClonningCandidates) {
    LLVM_DEBUG(dbgs() << "GenXPrintfPhiClonning:: For function "
                      << CallInst->getFunction()->getName() << "\n"
                      << *Phi << "\n"
                      << *CallInst << "\n");
    cloneBBs(CallInst, Phi);
  }
  return true;
}
