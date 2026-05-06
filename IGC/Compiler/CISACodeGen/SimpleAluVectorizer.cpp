/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/SimpleAluVectorizer.hpp"
#include "Compiler/CISACodeGen/RegisterEstimator.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/MathExtras.h"

using namespace llvm;
using namespace IGC;

#define DEBUG_TYPE "igc-simple-alu-vectorizer"

namespace {

/// Check if the opcode is one of the supported ALU operations.
static bool isSupportedAluOpcode(unsigned Opcode) {
  switch (Opcode) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::Shl:
  case Instruction::FAdd:
  case Instruction::FMul:
    return true;
  default:
    return false;
  }
}

/// Check if a type is valid for vectorization.
static bool isValidElementType(Type *Ty) { return VectorType::isValidElementType(Ty); }

/// Encode (opcode, type) into a key for grouping instructions.
using GroupKey = std::pair<unsigned, Type *>;

//===----------------------------------------------------------------------===//
// Simple cost model
//===----------------------------------------------------------------------===//

/// Estimate the cost of the uniform scalar code being replaced.
/// For uniform values each scalar ALU op executes on SIMD1, so the cost
/// is 1 per instruction (the dispatch overhead is negligible).
static int getUniformScalarCost(unsigned VF) { return static_cast<int>(VF); }

/// Estimate the cost of the vectorized sequence for uniform operands.
/// Each insertelement for LHS/RHS generates a MOV instruction unless the
/// scalar source operand has exactly one use (the insertelement itself), in
/// which case the register allocator can alias it into the vector slot for
/// free.  The extractelement instructions are typically zero-cost via
/// subreg aliasing.
///
/// Cost breakdown:
///   - 1 vector ALU instruction
///   - (VF - aliasable_LHS) MOVs for LHS insertelements
///   - (VF - aliasable_RHS) MOVs for RHS insertelements
///   - GRF-splitting penalty for wide vectors
///   - VF-scaled register pressure penalty for larger vectors
static int getUniformVectorCost(ArrayRef<BinaryOperator *> Slice, unsigned VF, unsigned GRFBits,
                                const SmallPtrSetImpl<const Instruction *> *Candidates) {
  int Cost = 0;

  // Vector ALU instruction.
  Cost += 1;

  // Detect splat operands: if all scalar instructions share the same LHS
  // (or RHS) value, only a single broadcast MOV is needed instead of VF
  // individual insertelements.  This mirrors the SLP Vectorizer's
  // isSplat() check and SK_Broadcast cost model (getEntryCost lines
  // ~5179-5188 and ~5585-5615 for constant operand classification).
  Value *CommonLHS = Slice[0]->getOperand(0);
  Value *CommonRHS = Slice[0]->getOperand(1);
  bool LHSIsSplat = true;
  bool RHSIsSplat = true;
  for (unsigned I = 1; I < VF; ++I) {
    if (Slice[I]->getOperand(0) != CommonLHS)
      LHSIsSplat = false;
    if (Slice[I]->getOperand(1) != CommonRHS)
      RHSIsSplat = false;
  }

  // Classify RHS operands for better cost estimation (inspired by SLP
  // getEntryCost OK_UniformConstantValue / OK_NonUniformConstantValue /
  // OK_AnyValue classification at lines ~5587-5615).
  bool RHSIsUniformConst = true;
  bool RHSIsPowerOf2 = true;
  ConstantInt *FirstCInt = nullptr;
  for (unsigned I = 0; I < VF; ++I) {
    auto *CI = dyn_cast<ConstantInt>(Slice[I]->getOperand(1));
    if (!CI) {
      RHSIsUniformConst = false;
      RHSIsPowerOf2 = false;
      break;
    }
    if (!CI->getValue().isPowerOf2())
      RHSIsPowerOf2 = false;
    if (!FirstCInt)
      FirstCInt = CI;
    else if (FirstCInt != CI)
      RHSIsUniformConst = false;
  }

  // For commutative operations, also classify LHS constants.
  bool LHSIsUniformConst = false;
  if (Slice[0]->isCommutative()) {
    LHSIsUniformConst = true;
    ConstantInt *FirstLHSCInt = nullptr;
    for (unsigned I = 0; I < VF; ++I) {
      auto *CI = dyn_cast<ConstantInt>(Slice[I]->getOperand(0));
      if (!CI) {
        LHSIsUniformConst = false;
        break;
      }
      if (!FirstLHSCInt)
        FirstLHSCInt = CI;
      else if (FirstLHSCInt != CI) {
        LHSIsUniformConst = false;
        break;
      }
    }
  }

  // InsertElement overhead: count MOVs needed to build vector operands.
  // Constants and arguments pass the !isa<Instruction> check in the
  // general case below, so they are naturally treated as aliasable
  // (zero-cost) without needing dedicated branches.
  if (LHSIsSplat) {
    // Splat: one broadcast MOV, unless the source is a non-instruction
    // (constant/argument) which can be inlined as an immediate.
    Cost += isa<Instruction>(CommonLHS) ? 1 : 0;
  } else if (LHSIsUniformConst) {
    // Uniform constant LHS on a commutative op: single immediate broadcast.
    Cost += 0;
  } else {
    unsigned AliasableLHS = 0;
    for (unsigned I = 0; I < VF; ++I) {
      Value *LHS = Slice[I]->getOperand(0);
      if (LHS->hasOneUse() || !isa<Instruction>(LHS))
        ++AliasableLHS;
    }
    Cost += static_cast<int>(VF - AliasableLHS);
  }

  if (RHSIsSplat) {
    // Splat: one broadcast MOV (or zero if the value can be aliased).
    Cost += (!isa<Instruction>(CommonRHS) || CommonRHS->hasOneUse()) ? 0 : 1;
  } else if (RHSIsUniformConst) {
    // Uniform constant: can be materialized as a single immediate broadcast.
    // Shifts with uniform constant power-of-2 RHS are especially cheap on
    // Gen (the backend can encode the shift amount as an immediate).
    Cost += (RHSIsPowerOf2 && (Slice[0]->getOpcode() == Instruction::Shl || Slice[0]->getOpcode() == Instruction::Mul))
                ? 0
                : 1;
  } else {
    unsigned AliasableRHS = 0;
    for (unsigned I = 0; I < VF; ++I) {
      Value *RHS = Slice[I]->getOperand(1);
      if (RHS->hasOneUse() || !isa<Instruction>(RHS))
        ++AliasableRHS;
    }
    Cost += static_cast<int>(VF - AliasableRHS);
  }

  // Extract cost: charge per-lane for users that will force the extract
  // to materialize as a real MOV.  An extract is "free" (sub-register
  // alias) when its consumer can read directly from a vector lane:
  //   - another vectorization candidate (will become a vector op),
  //   - a vector-typed consumer (e.g. a vector binop), or
  //   - a scalar store (Gen stores can address a vector sub-register
  //     directly, no MOV needed).
  // Materializing consumers: insertelement / shufflevector (consume
  // SCALAR inputs into a different vector layout), same-BB scalar
  // non-candidate instructions, and any cross-BB user.
  auto consumerForcesMaterialization = [Candidates](Instruction *Producer, Instruction *UI) {
    if (UI->getParent() != Producer->getParent())
      return true;
    if (Candidates && Candidates->count(UI))
      return false;
    if (isa<InsertElementInst>(UI) || isa<ShuffleVectorInst>(UI))
      return true;
    if (isa<StoreInst>(UI))
      return false; // store can read a vector sub-register directly
    if (UI->getType()->isVectorTy())
      return false;
    return true;
  };

  for (unsigned I = 0; I < VF; ++I) {
    for (auto *U : Slice[I]->users()) {
      auto *UI = dyn_cast<Instruction>(U);
      if (!UI)
        continue;
      if (consumerForcesMaterialization(Slice[I], UI)) {
        Cost += 1;
        break; // charge at most once per slice element
      }
    }
  }

  // Register pressure penalty: only penalize larger vectors (VF > 4) where
  // the widened live ranges meaningfully increase GRF demand.  For VF=2,
  // the pressure increase is negligible (2 DWORDs vs 1 DWORD per operand),
  // and the BB-level isBBHighPressure() gate already rejects high-pressure
  // blocks.  For VF=4+, charge a scaled penalty.
  if (VF > 4)
    Cost += static_cast<int>(VF / 4);

  // Penalize types that are wider than the GRF: they require splitting.
  Type *ScalarTy = Slice[0]->getType();
  unsigned VecBits = ScalarTy->getScalarSizeInBits() * VF;
  if (VecBits > GRFBits)
    Cost += static_cast<int>(VecBits / GRFBits);

  return Cost;
}

class SimpleAluVectorizer : public FunctionPass {
public:
  static char ID;

  SimpleAluVectorizer() : FunctionPass(ID) { initializeSimpleAluVectorizerPass(*PassRegistry::getPassRegistry()); }

  bool runOnFunction(Function &F) override;

  StringRef getPassName() const override { return "IGC Simple ALU Vectorizer"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<WIAnalysis>();
    AU.addRequired<RegisterEstimator>();
  }

private:
  unsigned GRFBits = 256;
  uint16_t BestGuessSIMD = 16;
  uint32_t RegPressureThreshold = 64;
  WIAnalysis *WIA = nullptr;
  RegisterEstimator *RPE = nullptr;

  /// Set of instructions in the current BB that are candidates for
  /// vectorization (i.e., belong to some group of size >= 2).  Used by
  /// vectorizeAluChain() to decide whether at least one downstream user
  /// is likely to also be vectorized; if every user is a plain scalar
  /// instruction, the extracts will materialize and the transform is
  /// unlikely to pay off.
  SmallPtrSet<const Instruction *, 32> VectorizableCandidates;

  /// Return true if the instruction and all its operands are uniform.
  bool isUniformWithOperands(BinaryOperator *BO) const;

  /// Return true if BB already has high register pressure.
  bool isBBHighPressure(BasicBlock *BB) const;

  /// Return true if at least one user of any instruction in \p Slice is
  /// either another vectorization candidate or already produces/consumes
  /// a vector value.
  bool hasVectorFriendlyConsumer(ArrayRef<BinaryOperator *> Slice) const;

  /// Scan a basic block for groups of isomorphic uniform ALU instructions.
  bool vectorizeBlock(BasicBlock *BB, unsigned MaxVecRegBits);

  /// Try to vectorize a group of binary operators that share the same
  /// opcode and scalar type.
  bool tryVectorizeGroup(ArrayRef<BinaryOperator *> Group, unsigned MaxVecRegBits);

  /// Vectorize exactly \p VF binary operators from \p Group starting at
  /// index \p StartIdx.
  bool vectorizeAluChain(ArrayRef<BinaryOperator *> Group, unsigned StartIdx, unsigned VF);
};

} // End anonymous namespace

char SimpleAluVectorizer::ID = 0;

#define PASS_FLAG "igc-simple-alu-vectorizer"
#define PASS_DESC "IGC Simple ALU Vectorizer"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SimpleAluVectorizer, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(RegisterEstimator)
IGC_INITIALIZE_PASS_END(SimpleAluVectorizer, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC {
FunctionPass *createSimpleAluVectorizerPass() { return new SimpleAluVectorizer(); }
} // namespace IGC

//===----------------------------------------------------------------------===//
// Pass implementation
//===----------------------------------------------------------------------===//

bool SimpleAluVectorizer::isUniformWithOperands(BinaryOperator *BO) const {
  if (!WIA->isUniform(BO))
    return false;
  if (!WIA->isUniform(BO->getOperand(0)))
    return false;
  if (!WIA->isUniform(BO->getOperand(1)))
    return false;
  return true;
}

bool SimpleAluVectorizer::isBBHighPressure(BasicBlock *BB) const {
  if (!RPE)
    return false;

  // No worry if getMaxLiveGRFAtBB returns 0.
  uint32_t maxLiveGRF = RPE->getMaxLiveGRFAtBB(BB, BestGuessSIMD);
  uint32_t threshold = RegPressureThreshold;
  bool high = maxLiveGRF >= threshold;
  LLVM_DEBUG(if (high) dbgs() << "IGC SimpleAluVec: Skipping high-pressure BB (" << maxLiveGRF << " >= " << threshold
                              << " GRFs)\n");
  return high;
}

bool SimpleAluVectorizer::runOnFunction(Function &F) {
  CodeGenContext *CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  WIA = &getAnalysis<WIAnalysis>();
  RPE = &getAnalysis<RegisterEstimator>();

  if (!CGCtx || !WIA || !RPE)
    return false;

  if (!CGCtx->m_retryManager->IsFirstTry())
    return false;

  // Use the GRF size in bits from the platform as the vector register bit width.
  // Fall back to 128 bits if unavailable.
  unsigned MaxVecRegBits = 128;
  if (CGCtx->platform.getMinDispatchMode() != SIMDMode::BEGIN) {
    // Use a conservative register size based on GRF width (256 bits on Gen12+).
    unsigned PlatformGRFBits = CGCtx->platform.getGRFSize() * 8; // getGRFSize() returns bytes
    if (PlatformGRFBits > 0)
      MaxVecRegBits = PlatformGRFBits;
  }

  GRFBits = MaxVecRegBits;

  // Best-guess SIMD for register pressure queries.
  // TODO: consider other shader types later
  // Pixel shaders typically compile at SIMD8/16 even on XE2+.
  // if (CGCtx->type == ShaderType::PIXEL_SHADER)
  //   BestGuessSIMD = 16;
  BestGuessSIMD = CGCtx->platform.isProductChildOf(IGFX_PVC) ? 32 : 16;

  if (!CGCtx->platform.isCoreChildOf(IGFX_XE3_CORE))
    RegPressureThreshold = 64;
  else if (!CGCtx->platform.isCoreChildOf(IGFX_XE3P_CORE))
    // PTL/Xe3 (non-Xe3P): tighter than the original 96 to avoid
    // vectorizing near-spill BBs that increase send count.
    RegPressureThreshold = 72;

  // Compute register pressure estimates for per-BB queries.
  RPE->calculate();

  bool Changed = false;
  for (BasicBlock &BB : F)
    Changed |= vectorizeBlock(&BB, MaxVecRegBits);

  return Changed;
}

bool SimpleAluVectorizer::hasVectorFriendlyConsumer(ArrayRef<BinaryOperator *> Slice) const {
  // A consumer is "vector-friendly" if it is either:
  //   (a) another vectorization candidate in the same BB (it may itself
  //       be replaced by a vector op, so the extract feeding it can be
  //       elided / forwarded), or
  //   (b) an instruction that itself produces a vector value (e.g. a
  //       vector binop or vector intrinsic).  Such consumers already
  //       operate in vector territory, so the extract feeding them is
  //       likely to fold away during codegen.
  //
  // Note: insertelement / shufflevector produce vectors but consume
  // SCALAR elements from us, so they must NOT count as friendly --
  // the extract still materializes.  They are filtered out below
  // before we test the consumer's result type.
  for (BinaryOperator *BO : Slice) {
    for (Use &U : BO->uses()) {
      auto *UI = dyn_cast<Instruction>(U.getUser());
      if (!UI)
        continue;
      if (VectorizableCandidates.count(UI))
        return true;
      // Skip insert/shuffle: they consume scalars from us.
      if (isa<InsertElementInst>(UI) || isa<ShuffleVectorInst>(UI))
        continue;
      // Consumer that itself produces a vector value.
      if (UI->getType()->isVectorTy())
        return true;
    }
  }
  return false;
}

bool SimpleAluVectorizer::vectorizeBlock(BasicBlock *BB, unsigned MaxVecRegBits) {
  // Skip blocks with high register pressure to avoid increasing spills.
  if (isBBHighPressure(BB))
    return false;

  // Group uniform ALU instructions by (opcode, scalar type).
  // MapVector preserves insertion order for determinism.
  MapVector<GroupKey, SmallVector<BinaryOperator *, 8>> Groups;

  for (Instruction &I : *BB) {
    auto InstType = I.getType();
    if (InstType->isVectorTy() || InstType->isVoidTy() || InstType->isIntegerTy(1) || InstType->isIntegerTy(64) ||
        InstType->isDoubleTy()) {
      continue;
    }

    auto *BO = dyn_cast<BinaryOperator>(&I);
    if (!BO)
      continue;

    if (!isSupportedAluOpcode(BO->getOpcode()))
      continue;

    Type *Ty = BO->getType();
    // Skip already-vector types and unsupported element types.
    if (!isValidElementType(Ty))
      continue;

    // Only handle single-use instructions to avoid generating extracts
    // that negate the benefit of vectorization.
    if (!BO->hasOneUse())
      continue;

    // Only consider uniform instructions with uniform operands.
    if (!isUniformWithOperands(BO))
      continue;

    Groups[{BO->getOpcode(), Ty}].push_back(BO);
  }

  // Build the candidate set: every instruction that lives in a group of
  // size >= 2 (i.e., has at least one isomorphic peer it could be
  // vectorized with).  Consulted by vectorizeAluChain() to gate on
  // whether downstream users are likely to also become vector ops.
  VectorizableCandidates.clear();
  for (auto &KV : Groups) {
    if (KV.second.size() >= 2) {
      for (BinaryOperator *BO : KV.second)
        VectorizableCandidates.insert(BO);
    }
  }

  bool Changed = false;
  for (auto &KV : Groups) {
    if (KV.second.size() >= 2)
      Changed |= tryVectorizeGroup(KV.second, MaxVecRegBits);
  }

  VectorizableCandidates.clear();
  return Changed;
}

bool SimpleAluVectorizer::tryVectorizeGroup(ArrayRef<BinaryOperator *> Group, unsigned MaxVecRegBits) {
  bool Changed = false;

  unsigned EltBits = Group[0]->getType()->getScalarSizeInBits();
  if (EltBits == 0)
    return false;

  unsigned MaxVF = MaxVecRegBits / EltBits;
  if (MaxVF < 2)
    return false;

  // Clamp to the largest power-of-2 that fits in the group.
  unsigned VF =
      static_cast<unsigned>(IGCLLVM::bit_floor(std::min<unsigned>(static_cast<unsigned>(Group.size()), MaxVF)));
  if (VF < 2)
    return false;

  // Track which indices have been consumed (erased) so we never revisit them.
  unsigned NumElts = static_cast<unsigned>(Group.size());
  SmallVector<bool, 16> Consumed(NumElts, false);

  // Build a compacted list of unconsumed indices for each VF attempt.
  while (VF >= 2) {
    // Collect surviving (unconsumed) instructions in order.
    SmallVector<BinaryOperator *, 8> Remaining;
    SmallVector<unsigned, 8> RemainingIdx;
    for (unsigned I = 0; I < NumElts; ++I) {
      if (!Consumed[I]) {
        Remaining.push_back(Group[I]);
        RemainingIdx.push_back(I);
      }
    }

    // Try to vectorize consecutive windows of VF from the remaining set.
    for (unsigned I = 0, E = static_cast<unsigned>(Remaining.size()); I + VF <= E; I += VF) {
      if (vectorizeAluChain(Remaining, I, VF)) {
        Changed = true;
        // Mark these indices as consumed so no later iteration touches them.
        for (unsigned J = I; J < I + VF; ++J)
          Consumed[RemainingIdx[J]] = true;
      }
    }
    VF /= 2;
  }

  return Changed;
}

bool SimpleAluVectorizer::vectorizeAluChain(ArrayRef<BinaryOperator *> Group, unsigned StartIdx, unsigned VF) {
  IGC_ASSERT(VF >= 2 && isPowerOf2_32(VF));
  IGC_ASSERT(StartIdx + VF <= Group.size());

  ArrayRef<BinaryOperator *> Slice = Group.slice(StartIdx, VF);

  // All instructions must be in the same basic block.
  BasicBlock *BB = Slice[0]->getParent();
  for (unsigned I = 1; I < VF; ++I)
    if (Slice[I]->getParent() != BB)
      return false;

  // Reject if any instruction in the slice uses the result of another
  // instruction in the slice - vectorizing them would create a cycle.
  SmallPtrSet<Value *, 8> SliceSet(Slice.begin(), Slice.end());
  for (unsigned I = 0; I < VF; ++I) {
    if (SliceSet.count(Slice[I]->getOperand(0)) || SliceSet.count(Slice[I]->getOperand(1)))
      return false;
  }

  // --- Cost-model gate ---
  int SCost = getUniformScalarCost(VF);
  int VCost = getUniformVectorCost(Slice, VF, GRFBits, &VectorizableCandidates);
  if (VCost >= SCost) {
    LLVM_DEBUG(dbgs() << "IGC SimpleAluVec: Skipping unprofitable VF=" << VF << " group\n");
    return false;
  }

  // Consumer gate
  // Require at least one downstream user that is either another
  // vectorization candidate or already vector-typed.  Exception: when
  // an operand is a splat, the vector form is a broadcast + vector ALU,
  // which is a clear win even with purely scalar consumers, because the
  // insertelements collapse to a single MOV.  The cost model still
  // rejects bad cases (e.g. extracts feeding insertelements into an
  // unrelated vector layout) via the per-lane materialization penalty.
  bool HasSplatOperand = false;
  {
    Value *L0 = Slice[0]->getOperand(0);
    Value *R0 = Slice[0]->getOperand(1);
    bool LHSSplat = true, RHSSplat = true;
    for (unsigned I = 1; I < VF; ++I) {
      if (Slice[I]->getOperand(0) != L0)
        LHSSplat = false;
      if (Slice[I]->getOperand(1) != R0)
        RHSSplat = false;
    }
    HasSplatOperand = LHSSplat || RHSSplat;
  }

  if (!HasSplatOperand && !hasVectorFriendlyConsumer(Slice)) {
    LLVM_DEBUG(dbgs() << "IGC SimpleAluVec: Skipping VF=" << VF
                      << " group: no splat operand and no vector-friendly consumer\n");
    return false;
  }

  unsigned Opcode = Slice[0]->getOpcode();
  Type *ScalarTy = Slice[0]->getType();
  auto *VecTy = IGCLLVM::FixedVectorType::get(ScalarTy, VF);

  Instruction *InsertPt = Slice[0];
  for (unsigned I = 0; I < VF; ++I) {
    for (unsigned OpIdx = 0; OpIdx < 2; ++OpIdx) {
      if (auto *OpInst = dyn_cast<Instruction>(Slice[I]->getOperand(OpIdx))) {
        if (OpInst->getParent() == BB && InsertPt->comesBefore(OpInst))
          InsertPt = OpInst;
      }
    }
  }
  InsertPt = InsertPt->getNextNode();

  // Ensure InsertPt is not in the middle of PHI nodes.
  // Inserting non-PHI instructions before all PHIs have ended produces
  // invalid IR.
  if (InsertPt && isa<PHINode>(InsertPt))
    InsertPt = BB->getFirstNonPHI();

  // Find the earliest user of any instruction in the slice.
  // The extract instructions must be placed before this point so that
  // WIAnalysis (which processes instructions in program order) sees
  // the extracts before their users.
  Instruction *EarliestUser = nullptr;
  for (unsigned I = 0; I < VF; ++I) {
    for (auto *U : Slice[I]->users()) {
      if (auto *UI = dyn_cast<Instruction>(U)) {
        if (UI->getParent() == BB) {
          if (!EarliestUser || UI->comesBefore(EarliestUser))
            EarliestUser = UI;
        }
      }
    }
  }

  // If any user comes before our intended insertion point, we cannot
  // safely vectorize because we can't place the vector ops early enough
  // (their operands may not be defined yet).
  if (EarliestUser && EarliestUser->comesBefore(InsertPt))
    return false;

  IRBuilder<> Builder(InsertPt);

  // Build vector operands (LHS and RHS) via insertelement.
  Value *LHSVec = PoisonValue::get(VecTy);
  Value *RHSVec = PoisonValue::get(VecTy);
  for (unsigned I = 0; I < VF; ++I) {
    LHSVec = Builder.CreateInsertElement(LHSVec, Slice[I]->getOperand(0), Builder.getInt32(I));
    RHSVec = Builder.CreateInsertElement(RHSVec, Slice[I]->getOperand(1), Builder.getInt32(I));
  }

  // Create the vector ALU instruction.
  Value *VecResult = Builder.CreateBinOp(static_cast<Instruction::BinaryOps>(Opcode), LHSVec, RHSVec, "valu");

  // Propagate IR flags (nsw, nuw, exact) - use intersection of all scalar ops.
  if (auto *VecInst = dyn_cast<Instruction>(VecResult)) {
    VecInst->copyIRFlags(Slice[0]);
    for (unsigned I = 1; I < VF; ++I)
      VecInst->andIRFlags(Slice[I]);

    // Tag the instruction as produced by SimpleAluVectorizer so that
    // EmitPass can detect it and emit accordingly.
    VecInst->setMetadata("igc.simple.alu.vectorized", llvm::MDNode::get(VecInst->getContext(), {}));
  }

  // Extract results, replace scalar uses, and erase originals.
  // First, create all extracts before erasing any original instruction,
  // because InsertPt may be one of the slice instructions and the
  // IRBuilder's insertion point would become invalid after erasure.
  SmallVector<Value *, 8> Extracts(VF);
  for (unsigned I = 0; I < VF; ++I) {
    Extracts[I] = Builder.CreateExtractElement(VecResult, Builder.getInt32(I));
  }
  for (unsigned I = 0; I < VF; ++I) {
    Slice[I]->replaceAllUsesWith(Extracts[I]);
    Slice[I]->eraseFromParent();
  }

  LLVM_DEBUG(dbgs() << "IGC SimpleAluVec: Vectorized " << VF << " x " << Instruction::getOpcodeName(Opcode) << " ("
                    << *ScalarTy << ") [uniform]\n");
  return true;
}