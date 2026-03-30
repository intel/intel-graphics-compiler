/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/SimdCoalescing.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-simd-coalescing"
#define PASS_DESCRIPTION "Coalesce SIMD1/SIMD2 uniform operations into wider SIMD"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SimdCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SimdCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char SimdCoalescing::ID = 0;

SimdCoalescing::SimdCoalescing() : FunctionPass(ID) { initializeSimdCoalescingPass(*PassRegistry::getPassRegistry()); }

// Check if two instructions are compatible for merging into a wider SIMD op.
// They must have the same opcode, same type, and matching operand structure.
bool SimdCoalescing::areCompatible(Instruction *A, Instruction *B) {
  if (A->getOpcode() != B->getOpcode())
    return false;
  if (A->getType() != B->getType())
    return false;

  // For FP ops, fast-math flags must match so the vector op is semantically
  // identical to the scalar pair.
  if (isa<FPMathOperator>(A) && A->getFastMathFlags() != B->getFastMathFlags())
    return false;

  return true;
}

// Check that no instruction in the bundle uses the result of another
// instruction in the same bundle (no intra-bundle data dependency).
bool SimdCoalescing::hasNoIntraBundleDeps(ArrayRef<Instruction *> Bundle) {
  SmallPtrSet<Instruction *, 8> BundleSet(Bundle.begin(), Bundle.end());
  for (Instruction *I : Bundle) {
    for (Value *Op : I->operands()) {
      if (auto *OpInst = dyn_cast<Instruction>(Op)) {
        if (BundleSet.count(OpInst))
          return false;
      }
    }
  }
  return true;
}

bool SimdCoalescing::canAvoidScatterExtract(ArrayRef<Instruction *> Bundle) {
  if (Bundle.size() < 2)
    return false;

  Value *CommonRoot = nullptr;
  SmallPtrSet<Instruction *, 8> BundleSet(Bundle.begin(), Bundle.end());
  SmallSet<unsigned, 8> UsedLanes;

  for (Instruction *I : Bundle) {
    // Requirement 1: single use
    if (!I->hasOneUse())
      return false;

    auto *User = dyn_cast<InsertElementInst>(*I->user_begin());
    if (!User)
      return false;

    // Requirement 2: constant lane index
    auto *IdxConst = dyn_cast<ConstantInt>(User->getOperand(2));
    if (!IdxConst)
      return false;

    unsigned Lane = IdxConst->getZExtValue();

    // Requirement 4: no duplicate lanes
    if (!UsedLanes.insert(Lane).second)
      return false;

    Value *Target = User->getOperand(0);
    while (auto *Prev = dyn_cast<InsertElementInst>(Target)) {
      // If Prev inserts a value that is itself a bundle member,
      // keep tracing - that InsertElement will be replaced too.
      if (auto *PrevVal = dyn_cast<Instruction>(Prev->getOperand(1))) {
        if (BundleSet.count(PrevVal)) {
          Target = Prev->getOperand(0);
          continue;
        }
      }
      break;
    }

    if (!CommonRoot)
      CommonRoot = Target;
    else if (CommonRoot != Target)
      return false;
  }

  if (CommonRoot) {
    if (auto *VTy = dyn_cast<FixedVectorType>(CommonRoot->getType())) {
      if (VTy->getNumElements() < Bundle.size())
        return false;
    }
  }

  return CommonRoot != nullptr;
}

static bool feedsSendInstruction(Instruction *I) {
  constexpr unsigned MaxDepth = 3;
  constexpr unsigned MaxVisited = 16;
  SmallPtrSet<Instruction *, 16> Visited;
  SmallVector<std::pair<Instruction *, unsigned>, 4> Worklist;
  Worklist.push_back({I, 0});

  while (!Worklist.empty()) {
    auto [Curr, Depth] = Worklist.pop_back_val();

    if (!Visited.insert(Curr).second)
      continue;
    if (Visited.size() > MaxVisited)
      return false; // Conservative: assume no send if graph is too large

    for (User *U : Curr->users()) {
      // Stores are scalar data operands - the extract replaces the
      // original value without affecting send payload layout.
      if (isa<StoreInst>(U))
        continue;

      // Multi-operand intrinsics where CoalescingEngine arranges
      // payload registers - an extract here disrupts register coalescing.
      if (auto *CI = dyn_cast<CallInst>(U)) {
        if (isa<GenIntrinsicInst>(CI))
          return true;
      }

      if (Depth < MaxDepth) {
        if (auto *UI = dyn_cast<Instruction>(U)) {
          // Only follow transparent forwarding instructions that are
          // likely part of a payload chain. Avoid SelectInst/PHINode
          // which indicate control-flow-dependent usage.
          if (isa<CastInst>(UI) || isa<GetElementPtrInst>(UI) || isa<InsertElementInst>(UI)) {
            Worklist.push_back({UI, Depth + 1});
          }
        }
      }
    }
  }
  return false;
}

bool SimdCoalescing::isProfitable(ArrayRef<Instruction *> Bundle) {
  if (Bundle.size() < 2)
    return false;

  // Reject bundles where the instructions are too far apart in the BB.
  Instruction *First = Bundle[0];
  Instruction *Last = Bundle[0];
  for (unsigned i = 1; i < Bundle.size(); ++i) {
    if (Bundle[i]->comesBefore(First))
      First = Bundle[i];
    if (Last->comesBefore(Bundle[i]))
      Last = Bundle[i];
  }

  unsigned Span = 0;
  for (auto It = First->getIterator(), End = Last->getIterator(); It != End; ++It)
    ++Span;

  constexpr unsigned MaxSpanPerElement = 10;
  if (Span > Bundle.size() * MaxSpanPerElement)
    return false;

  unsigned PackCost = 0;
  bool HasAllConstSlot = false;
  bool HasAllSameSlot = false;
  for (unsigned OpIdx = 0; OpIdx < Bundle[0]->getNumOperands(); ++OpIdx) {
    bool AllConst = true;
    bool AllSame = true;
    for (unsigned j = 0; j < Bundle.size(); ++j) {
      if (!isa<Constant>(Bundle[j]->getOperand(OpIdx)))
        AllConst = false;
      if (j > 0 && Bundle[j]->getOperand(OpIdx) != Bundle[0]->getOperand(OpIdx))
        AllSame = false;
    }
    if (AllConst)
      HasAllConstSlot = true;
    if (AllSame)
      HasAllSameSlot = true;
    if (!AllConst && !AllSame)
      PackCost += 1;
  }

  if (HasAllConstSlot && HasAllSameSlot && Bundle.size() >= 4) {
    if (canAvoidScatterExtract(Bundle))
      return true;
  }

  SmallPtrSet<Instruction *, 8> BundleSet(Bundle.begin(), Bundle.end());
  unsigned ExtractCount = 0;
  unsigned SendFeedingExtractCount = 0;
  unsigned TotalExternalUses = 0;
  for (Instruction *I : Bundle) {
    unsigned ExternalUseCount = 0;
    for (User *U : I->users()) {
      if (!BundleSet.count(cast<Instruction>(U))) {
        ++ExternalUseCount;
      }
    }
    if (ExternalUseCount > 0) {
      // Count each use, not just each member - reflects actual extract
      // live range span needed to reach all use sites.
      ExtractCount += ExternalUseCount;
      TotalExternalUses += ExternalUseCount;
      if (feedsSendInstruction(I))
        SendFeedingExtractCount += ExternalUseCount;
    }
  }

  unsigned NonSendExtracts = ExtractCount - SendFeedingExtractCount;
  unsigned ScaledTotalCost = PackCost * 2 + NonSendExtracts + SendFeedingExtractCount * 2;
  unsigned ScaledSaving = (Bundle.size() - 1) * 2;
  if (ScaledTotalCost > ScaledSaving)
    return false;

  if (TotalExternalUses > Bundle.size() * 2)
    return false;

  return true;
}

static bool hasMemoryBarrierBetween(Instruction *A, Instruction *B) {
  // A must come before B in the same basic block.
  IGC_ASSERT(A->getParent() == B->getParent());
  for (auto It = std::next(A->getIterator()), End = B->getIterator(); It != End; ++It) {
    Instruction &I = *It;
    if (I.isAtomic() || isa<FenceInst>(I))
      return true;
  }
  return false;
}

bool SimdCoalescing::vectorizeBundle(ArrayRef<Instruction *> Bundle, LLVMContext &Ctx) {
  unsigned BundleSize = Bundle.size();
  IGC_ASSERT(BundleSize >= 2 && BundleSize <= MaxCoalescingWidth);

  Instruction *First = Bundle[0];
  Type *ScalarTy = First->getType();
  VectorType *VecTy = FixedVectorType::get(ScalarTy, BundleSize);
  BasicBlock *BB = First->getParent();

  // Find insertion point: after the last operand definition in this BB.
  // Start from the last bundle instruction in program order.
  Instruction *InsertPt = Bundle[0];
  for (unsigned k = 1; k < BundleSize; ++k) {
    if (InsertPt->comesBefore(Bundle[k]))
      InsertPt = Bundle[k];
  }
  for (Instruction *I : Bundle) {
    for (Value *Op : I->operands()) {
      if (auto *OpInst = dyn_cast<Instruction>(Op)) {
        if (OpInst->getParent() == BB && InsertPt->comesBefore(OpInst))
          InsertPt = OpInst;
      }
    }
  }
  Instruction *InsertAfterLastDef = InsertPt->getNextNonDebugInstruction();

  // Find the earliest user of any bundle instruction in this BB.
  // Exclude users that are themselves in the bundle (they will be erased).
  SmallPtrSet<Instruction *, 8> BundlePtrSet(Bundle.begin(), Bundle.end());
  Instruction *EarliestUser = nullptr;
  for (Instruction *I : Bundle) {
    for (User *U : I->users()) {
      if (auto *UserInst = dyn_cast<Instruction>(U)) {
        if (UserInst->getParent() == BB && !BundlePtrSet.count(UserInst)) {
          if (!EarliestUser || UserInst->comesBefore(EarliestUser))
            EarliestUser = UserInst;
        }
      }
    }
  }

  // The insertion point must satisfy both constraints:
  //   1. After all operand definitions (so operands are available)
  //   2. Before the earliest user (so the result is available to users)
  // If these conflict, the bundle cannot be safely coalesced.
  if (EarliestUser && EarliestUser->comesBefore(InsertAfterLastDef))
    return false;

  Instruction *FinalInsertPt = EarliestUser ? EarliestUser : InsertAfterLastDef;

  if (hasMemoryBarrierBetween(InsertPt, FinalInsertPt))
    return false;

  IRBuilder<> Builder(FinalInsertPt);

  // --- Pack operands into vectors ---
  SmallVector<Value *, 2> VecOperands;
  unsigned NumOps = First->getNumOperands();
  for (unsigned i = 0; i < NumOps; ++i) {
    // Check if all bundle elements share the same operand for this slot.
    bool AllSame = true;
    bool AllConstant = isa<Constant>(First->getOperand(i));
    for (unsigned j = 1; j < BundleSize; ++j) {
      if (Bundle[j]->getOperand(i) != First->getOperand(i))
        AllSame = false;
      if (!isa<Constant>(Bundle[j]->getOperand(i)))
        AllConstant = false;
    }

    if (AllConstant) {
      // Build a ConstantVector directly - zero packing cost.
      SmallVector<Constant *, 8> Elts;
      for (unsigned j = 0; j < BundleSize; ++j)
        Elts.push_back(cast<Constant>(Bundle[j]->getOperand(i)));
      VecOperands.push_back(ConstantVector::get(Elts));
    } else if (AllSame) {
      // Broadcast / splat using a single insertelement + shufflevector.
      // This avoids N insertelement instructions that extend the operand
      // live range and increase instruction count (sends regression).
      Value *Val = First->getOperand(i);
      Value *Vec = Builder.CreateInsertElement(PoisonValue::get(VecTy), Val, Builder.getInt32(0), "coal.ins");
      SmallVector<int, 8> Mask(BundleSize, 0);
      Vec = Builder.CreateShuffleVector(Vec, Mask, "coal.splat");
      VecOperands.push_back(Vec);
    } else {
      // General case: pack each lane individually.
      Value *Vec = PoisonValue::get(VecTy);
      for (unsigned j = 0; j < BundleSize; ++j)
        Vec =
            Builder.CreateInsertElement(Vec, Bundle[j]->getOperand(i), Builder.getInt32(j), j == 0 ? "coal.pack" : "");
      VecOperands.push_back(Vec);
    }
  }

  // --- Create the vector ALU op ---
  Value *VecResult = nullptr;
  unsigned Opc = First->getOpcode();

  if (Instruction::isBinaryOp(Opc)) {
    VecResult =
        Builder.CreateBinOp(static_cast<Instruction::BinaryOps>(Opc), VecOperands[0], VecOperands[1], "coal.vec");
    if (auto *NewInst = dyn_cast<Instruction>(VecResult)) {
      NewInst->copyIRFlags(First);
    }
  } else {
    IGC_ASSERT_UNREACHABLE();
  }

  // --- Extract results and replace original instructions ---
  // Sink each extract to immediately before its earliest user in this BB,
  // rather than grouping all extracts at the bundle insertion point. This
  // shortens the vector result's live range and reduces register pressure.
  for (unsigned j = 0; j < BundleSize; ++j) {
    if (Bundle[j]->use_empty()) {
      Bundle[j]->eraseFromParent();
      continue;
    }

    // If VecResult was constant-folded (e.g. both operands were ConstantVectors),
    // CreateExtractElement will also constant-fold, so no insertion point is needed.
    if (isa<Constant>(VecResult)) {
      Value *extract =
          ConstantExpr::getExtractElement(cast<Constant>(VecResult), ConstantInt::get(Type::getInt32Ty(Ctx), j));
      Bundle[j]->replaceAllUsesWith(extract);
      Bundle[j]->eraseFromParent();
      continue;
    }

    // Find earliest user of this specific bundle member in this BB.
    Instruction *EarliestUseOfJ = nullptr;
    for (User *U : Bundle[j]->users()) {
      if (auto *UI = dyn_cast<Instruction>(U)) {
        if (UI->getParent() == BB) {
          if (!EarliestUseOfJ || UI->comesBefore(EarliestUseOfJ))
            EarliestUseOfJ = UI;
        }
      }
    }

    // Place extract right before its first user, but not before the vector op.
    Instruction *ExtractPt = EarliestUseOfJ;
    auto *VecResultInst = cast<Instruction>(VecResult);
    if (!ExtractPt || ExtractPt->comesBefore(VecResultInst))
      ExtractPt = VecResultInst->getNextNonDebugInstruction();

    // Avoid placing the extract immediately before a payload-coalesced
    // intrinsic (sample, URB write, RT write). CoalescingEngine walks
    // operand definitions in program order; inserting an extract between
    // other payload operand defs and the intrinsic can create false
    // interferences in CCTuple, forcing extra MOVs in PrepareExplicitPayload.
    if (auto *UserCall = dyn_cast<CallInst>(ExtractPt)) {
      if (isa<GenIntrinsicInst>(UserCall)) {
        // Place extract one instruction earlier, after the vector op.
        ExtractPt = VecResultInst->getNextNonDebugInstruction();
      }
    }

    IRBuilder<> ExtBuilder(ExtractPt);
    std::string Name = "coal.ext" + std::to_string(j);
    Value *Extract = ExtBuilder.CreateExtractElement(VecResult, ExtBuilder.getInt32(j), Name);
    if (auto *I = dyn_cast<Instruction>(Extract))
      I->setDebugLoc(Bundle[j]->getDebugLoc());

    Bundle[j]->replaceAllUsesWith(Extract);
    Bundle[j]->eraseFromParent();
  }

  return true;
}

void SimdCoalescing::coalesceInstructions(SmallVectorImpl<Instruction *> &Candidates, LLVMContext &Ctx,
                                          unsigned EffectiveMaxWidth) {
  if (Candidates.size() < 2)
    return;

  unsigned i = 0;
  while (i < Candidates.size()) {
    SmallVector<Instruction *, 8> Bundle;
    Bundle.push_back(Candidates[i]);

    for (unsigned j = i + 1; j < Candidates.size() && Bundle.size() < EffectiveMaxWidth; ++j) {
      if (!areCompatible(Bundle[0], Candidates[j]))
        continue;

      Bundle.push_back(Candidates[j]);
      if (!hasNoIntraBundleDeps(Bundle)) {
        Bundle.pop_back();
        continue;
      }
    }

    if (Bundle.size() < 2) {
      ++i;
      continue;
    }

    // Trim to largest power-of-2 size for HW efficiency.
    unsigned BestSize = 1;
    while (BestSize * 2 <= Bundle.size())
      BestSize *= 2;
    Bundle.resize(BestSize);

    if (!isProfitable(Bundle)) {
      ++i;
      continue;
    }

    if (vectorizeBundle(Bundle, Ctx)) {
      Changed = true;
      SmallPtrSet<Instruction *, 8> BundleSet(Bundle.begin(), Bundle.end());
      Candidates.erase(
          std::remove_if(Candidates.begin(), Candidates.end(), [&](Instruction *I) { return BundleSet.count(I); }),
          Candidates.end());
      // Do NOT advance i: the element that shifted into position i
      // has not been tried as a bundle leader yet.
    } else {
      ++i;
    }
  }
}

bool SimdCoalescing::runOnFunction(Function &F) {
  Changed = false;
  WI = &getAnalysis<WIAnalysis>();
  auto *CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  LLVMContext &Ctx = F.getContext();

  if (!CGCtx->m_retryManager->IsFirstTry())
    return false;

  // Use instruction-type statistics to estimate register pressure.
  // Shaders with many samples/loads have high baseline pressure from
  // send payloads; adding vector live ranges would likely cause spills.
  const auto &instrTypes = CGCtx->m_instrTypes;
  unsigned pressureEstimate = instrTypes.numSample * 4 +  // each sample ties up ~4 GRFs
                              instrTypes.numAtomics * 2 + // atomics lock payload registers
                              instrTypes.numLoadStore;    // each load/store needs payload setup
  unsigned GRFCount = CGCtx->platform.getMaxNumGRF(ShaderType::COMPUTE_SHADER);
  // Threshold scales with register file size: larger files tolerate
  // more pressure before coalescing becomes counterproductive.
  unsigned pressureThreshold = GRFCount * 2;
  if (pressureEstimate > pressureThreshold)
    return false;

  // Also skip very large functions as a safety net.
  constexpr unsigned MaxIRInstCount = 500;
  unsigned InstCount = 0;
  for (auto It = inst_begin(F), E = inst_end(F); It != E; ++It) {
    if (++InstCount > MaxIRInstCount)
      return false;
  }

  unsigned EffectiveMaxWidth = getEffectiveMaxWidth(CGCtx);

  for (BasicBlock &BB : F) {
    SmallVector<Instruction *, 16> Candidates;
    for (Instruction &I : BB) {
      if (I.isAtomic() || isa<FenceInst>(I) || I.mayWriteToMemory()) {
        coalesceInstructions(Candidates, Ctx, EffectiveMaxWidth);
        Candidates.clear();
        continue;
      }

      if (!WI->isUniform(&I) || !isCoalescable(&I))
        continue;

      bool AllOpsUniform = true;
      for (Value *Op : I.operands()) {
        if (isa<Constant>(Op))
          continue;
        if (!WI->isUniform(Op)) {
          AllOpsUniform = false;
          break;
        }
      }
      if (AllOpsUniform)
        Candidates.push_back(&I);
    }
    coalesceInstructions(Candidates, Ctx, EffectiveMaxWidth);
  }

  CGCtx->m_simdCoalescingDone = Changed;
  return Changed;
}
