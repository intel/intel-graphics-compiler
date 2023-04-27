/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXIMadLegalization
/// --------------------
///
/// This pass performs the legalization on integer mad to ensure additive
/// operand is alway single-used so that it could be mapped to accumulator
/// register.
///
//===----------------------------------------------------------------------===//
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXModule.h"
#include "GenXUtil.h"
#include "vc/Utils/GenX/BreakConst.h"

#include "llvm/IR/Dominators.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "Probe/Assertion.h"

#define DEBUG_TYPE "GENX_IMAD_POST_LEGALIZATION"

using namespace llvm;
using namespace genx;

namespace {

class GenXIMadPostLegalization : public FunctionPass {
  DominatorTree *DT = nullptr;
  GenXBaling *Baling = nullptr;
public:
  static char ID;

  explicit GenXIMadPostLegalization() :
      FunctionPass(ID), DT(nullptr), Baling(nullptr) {}

  StringRef getPassName() const override {
    return "GenX IMAD post-legalization pass";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<GenXFuncBaling>();
    AU.addPreserved<GenXModule>();
  }

  bool runOnFunction(Function &F) override;

protected:
  bool fixMadChain(BasicBlock *);
};

} // end anonymous namespace

char GenXIMadPostLegalization::ID = 0;

namespace llvm {
void initializeGenXIMadPostLegalizationPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXIMadPostLegalization, "GenXIMadLegalization", "GenXIMadLegalization", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(GenXFuncBaling)
INITIALIZE_PASS_END(GenXIMadPostLegalization, "GenXIMadLegalization", "GenXIMadLegalization", false, false)

FunctionPass *llvm::createGenXIMadPostLegalizationPass() {
  initializeGenXIMadPostLegalizationPass(*PassRegistry::getPassRegistry());
  return new GenXIMadPostLegalization();
}

static bool isIntegerMadIntrinsic(Value *V) {
  switch (GenXIntrinsic::getGenXIntrinsicID(V)) {
  default: break;
  case GenXIntrinsic::genx_ssmad:
  case GenXIntrinsic::genx_sumad:
  case GenXIntrinsic::genx_usmad:
  case GenXIntrinsic::genx_uumad:
  case GenXIntrinsic::genx_ssmad_sat:
  case GenXIntrinsic::genx_sumad_sat:
  case GenXIntrinsic::genx_usmad_sat:
  case GenXIntrinsic::genx_uumad_sat:
    return true;
  }
  return false;
}

static bool isIntegerMulIntrinsic(Value *V) {
  switch (GenXIntrinsic::getGenXIntrinsicID(V)) {
  default: break;
  case GenXIntrinsic::genx_ssmul:
  case GenXIntrinsic::genx_sumul:
  case GenXIntrinsic::genx_usmul:
  case GenXIntrinsic::genx_uumul:
    return true;
  }
  return false;
}

static std::tuple<BasicBlock *, Instruction *>
findNearestInsertPt(DominatorTree *DT, ArrayRef<Instruction *> Users) {
  DenseMap<BasicBlock *, Instruction *> BBs;
  for (auto U : Users) {
    auto UseBB = U->getParent();
    auto MI = BBs.end();
    bool New = false;
    std::tie(MI, New) = BBs.insert(std::make_pair(UseBB, U));
    if (New)
      continue;
    // Find the earliest user if more than one users are in the same block.
    auto BI = UseBB->begin();
    for (; &*BI != U && &*BI != MI->second; ++BI)
      /* EMPTY */;
    MI->second = &*BI;
  }

  IGC_ASSERT_MESSAGE(BBs.size() != 0, "At least one BB should be found!");

  auto MI = BBs.begin();
  if (BBs.size() == 1)
    return std::make_tuple(MI->first, MI->second);

  auto BB = MI->first;
  auto ME = BBs.end();
  for (++MI; MI != ME; ++MI)
    BB = DT->findNearestCommonDominator(BB, MI->first);

  MI = BBs.find(BB);
  if (MI != BBs.end())
    return std::make_tuple(MI->first, MI->second);

  return std::make_tuple(BB, nullptr);
}

bool GenXIMadPostLegalization::runOnFunction(Function &F) {
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  Baling = &getAnalysis<GenXFuncBaling>();
  bool Changed = false;

  // After this point, we should not do constant folding.
  Changed |= vc::breakConstantExprs(&F, vc::LegalizationStage::Legalized);

  // The following algorithm runs very slowly on large blocks.
  if (skipOptWithLargeBlock(F))
    return Changed;

  SmallVector<Instruction *, 16> OrphanedMadSumOpndInst;
  for (auto &BB : F) {
    for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /* EMPTY */) {
      Instruction *MadInst = &*BI++;
      if (!isIntegerMadIntrinsic(MadInst))
        continue;
      //-------------------------------------------------------------------
      // Work with Mad summed operand (operand 2) if it's an instruction:
      // If it has one use, move it closer to Mad.
      // If it has more than one use and is Mul or RDRegion, try to duplicate it
      // closer to Mad where it's used as a summed operand.
      // For all the other uses try to move it closer.
      //-------------------------------------------------------------------
      // This is done to shorten acc live ranges. This is particularly important
      // when 32 bit integer multiplications are not native and acc
      // registers will be used to emulate them.
      //-------------------------------------------------------------------
      auto *MadSumOpndInst = dyn_cast<Instruction>(MadInst->getOperand(2));
      if (!MadSumOpndInst)
        continue;
      // If Mad summed operand has only single use, try to move it closer to
      // Mad.
      if (MadSumOpndInst->hasOneUse()) {
        if (MadSumOpndInst->getParent() != MadInst->getParent() ||
            MadSumOpndInst->mayHaveSideEffects() ||
            isa<PHINode>(MadSumOpndInst) ||
            MadSumOpndInst->getNextNode() == MadInst ||
            !genx::isSafeToMoveInstCheckGVLoadClobber(MadSumOpndInst, MadInst,
                                                      Baling))
          continue;
        MadSumOpndInst->moveBefore(MadInst);
        Changed = true;
        continue;
      }
      // Only for Mul and RDRegions ...
      if (!GenXIntrinsic::isRdRegion(MadSumOpndInst) &&
          !isIntegerMulIntrinsic(MadSumOpndInst))
        continue;

      // If Mad summed operand source has multiple uses,
      // try to duplicate it closer to Mad where it's used as a summed operand.
      // For any other uses also try to move closer, see below.
      SmallVector<Instruction *, 16> OtherUseCases;
      for (auto UseIt = MadSumOpndInst->use_begin();
           UseIt != MadSumOpndInst->use_end();
           /* EMPTY */) {
        auto &MadSumOpndUse = *UseIt++;
        auto *UserInst = cast<Instruction>(MadSumOpndUse.getUser());
        if (isIntegerMadIntrinsic(UserInst) &&
            MadSumOpndUse.getOperandNo() == 2 &&
            genx::isSafeToMoveInstCheckGVLoadClobber(MadSumOpndInst, UserInst,
                                                     Baling)) {
          auto *NewMadSumOpndInst = MadSumOpndInst->clone();
          NewMadSumOpndInst->setName(MadSumOpndInst->getName() + ".postimad");
          NewMadSumOpndInst->insertBefore(UserInst);
          MadSumOpndUse.set(NewMadSumOpndInst);
        } else {
          OtherUseCases.push_back(UserInst);
        }
      }

      // Try to sink the Mul or RDRegion instruction closer to users.
      if (!OtherUseCases.empty()) {
        BasicBlock *NBB = nullptr;
        Instruction *InsertPt = nullptr;
        std::tie(NBB, InsertPt) = findNearestInsertPt(DT, OtherUseCases);
        InsertPt = InsertPt ? InsertPt : NBB->getTerminator();
        if (genx::isSafeToMoveInstCheckGVLoadClobber(MadSumOpndInst, InsertPt,
                                                     Baling))
          MadSumOpndInst->moveBefore(InsertPt);
      } else
        OrphanedMadSumOpndInst.push_back(MadSumOpndInst);
      Changed = true;
    }
  }
  for (auto *I : OrphanedMadSumOpndInst)
    I->eraseFromParent();
  for (auto &BB : F)
    Changed |= fixMadChain(&BB);
  return Changed;
}

bool GenXIMadPostLegalization::fixMadChain(BasicBlock *BB) {
  // Given the bale 'B', collect all its operand instructions in the same basic
  // block.
  auto collectUnbaledOpndInsts = [](BasicBlock *BB, Bale &B) {
    std::vector<Instruction *> Opnds;
    Instruction *In = nullptr;
    // Collect operand instructions not baled yet.
    for (auto I = B.begin(), E = B.end(); I != E; ++I) {
      bool isFMA = vc::getAnyIntrinsicID(I->Inst) == Intrinsic::fma;
      for (unsigned i = 0, e = I->Inst->getNumOperands(); i != e; ++i) {
        // Skip if that operand is baled.
        if (I->Info.isOperandBaled(i))
          continue;
        auto Op = dyn_cast<Instruction>(I->Inst->getOperand(i));
        // Skip if it's not an instruction or from the same BB.
        if (Op && Op->getParent() == BB) {
          Opnds.push_back(Op);
          if (isFMA && i == 2)
            In = Op;
        }
      }
      // Bail out once 'maininst' is processed. The 'maininst' is usually baled
      // in 'wrregion', 'sat' and similar stuffs, which usually doesn't require
      // additional operands.
      if (I->Info.Type == BaleInfo::MAININST)
        break;
    }
    return std::make_pair(In, Opnds);
  };

  // Given two instructions, 'A' and 'B', in the same basic block, check
  // whether 'A' dominates 'B'.
  auto dominates = [](const Instruction *A, const Instruction *B) {
    const BasicBlock *BB = A->getParent();
    IGC_ASSERT(BB == B->getParent());

    BasicBlock::const_iterator BI = BB->begin();
    for (; &*BI != A && &*BI != B; ++BI)
      /*EMPTY*/;

    return &*BI == A;
  };

  bool Changed = false;
  std::set<Instruction *> FMAs; // 'fma' already handled.
  for (auto BI = BB->rbegin(), BE = BB->rend(); BI != BE; ++BI) {
    auto Inst = &*BI;
    Bale OutB;
    Baling->buildBale(Inst, &OutB);
    // Skip bale non-FMA bale.
    if (!OutB.getMainInst())
      continue;
    auto CandidateInsn = OutB.getMainInst()->Inst;
    IGC_ASSERT(CandidateInsn);
    if (vc::getAnyIntrinsicID(CandidateInsn) != Intrinsic::fma)
      continue;
    // Skip if it's already handled.
    if (FMAs.count(CandidateInsn))
      continue;
    // Collection of all inputs for the chain curently discovered.
    std::set<Instruction *> Inputs;
    // The mad chain itself.
    std::vector<Bale> BaleChain;
    BaleChain.push_back(OutB);
    FMAs.insert(CandidateInsn);
    do {
      auto &OutB = BaleChain.back();
      Instruction *In = nullptr;
      std::vector<Instruction *> Opnds;
      // Collect all operands so that we could grow the chain through the
      // chain-in.
      std::tie(In, Opnds) = collectUnbaledOpndInsts(BB, OutB);
      if (!In || !In->hasOneUse())
        break;
      // Check whether all inputs collected so far dominates 'In' so that we
      // won't add extra register pressure.
      for (auto &I : Inputs) {
        if (dominates(I, In))
          continue;
        In = nullptr;
        break;
      }
      // Skip chain building if there are inputs won't be dominated by the new
      // chain-in.
      if (!In)
        break;
      // Check inputs from the tip of chain, i.e. the current chain-out.
      for (auto &OpI : Opnds) {
        // Skip the chain-in.
        if (OpI == In)
          continue;
        // Skip if that input dominates the chain-in but record it as inputs.
        //
        // FIXME: revisit the following check. This stops sinking non-mad bales
        // which may increase register pressure and inserts non-mad instructions
        // among mads.
        if (true || !OpI->hasOneUse() || dominates(OpI, In)) {
          Inputs.insert(OpI);
          continue;
        }
        // TODO: So far, only traverse one step further from that chain-out
        // operands.
        Bale OpB;
        Baling->buildBale(OpI, &OpB);
        std::vector<Instruction *> SubOpnds;
        std::tie(std::ignore, SubOpnds) = collectUnbaledOpndInsts(BB, OpB);
        for (auto &SubI : SubOpnds) {
          if (dominates(SubI, In)) {
            Inputs.insert(SubI);
            continue;
          }
          // Stop chaining as 'SubI' intervenes between 'In' and 'Out'.
          In = nullptr;
          break;
        }
        if (!In)
          break;
        BaleChain.push_back(OpB);
      }
      if (!In)
        break;
      // Grow the chain by appending this chain-in.
      Bale InB;
      Baling->buildBale(In, &InB);
      BaleChain.push_back(InB);
      // Stop chaining if it's not mad any more.
      if (!InB.getMainInst())
        break;
      auto CandidateInst = InB.getMainInst()->Inst;
      IGC_ASSERT(CandidateInst);
      if (vc::getAnyIntrinsicID(CandidateInst) != Intrinsic::fma)
        break;
      FMAs.insert(CandidateInst);
    } while (1);

    // Cluster the discovered chain together.
    if (FMAs.size() > 1) {
      Instruction *Pos = nullptr;
      for (auto Bale = BaleChain.begin(), BaleChainEnd = BaleChain.end();
           Bale != BaleChainEnd; ++Bale) {
        for (auto BaleInst = Bale->rbegin(), BaleEnd = Bale->rend();
             BaleInst != BaleEnd; ++BaleInst) {
          if (!Pos) {
            Pos = BaleInst->Inst;
            continue;
          }
          // Skip movement of a bale sourcing a global volatile load predecessor
          // if it can result in potential clobbering by an intervening vstore.
          // Skip phi which is not movable.
          if (!genx::isSafeToMoveBaleCheckGVLoadClobber(*Bale, Pos) ||
              isa<PHINode>(BaleInst->Inst))
            break;
          BaleInst->Inst->moveBefore(Pos);
          Pos = BaleInst->Inst;
          Changed = true;
        }
      }
    }
  }
  return Changed;
}
