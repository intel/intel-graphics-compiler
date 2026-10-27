/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/KnownBits.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <optional>

#include "common/debug/DebugMacros.hpp"
#include "common/IGCIRBuilder.h"
#include "common/MDFrameWork.h"
#include "common/Types.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"

#include "RegisterAccessBoundsChecks.hpp"

#define PASS_FLAG "igc-register-access-bounds-checks"
#define PASS_DESCRIPTION "Add bounds checks to prevent OOB register access"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

using namespace llvm;
namespace IGC {
////////////////////////////////////////////////////////////////////////////////
// This pass adds runtime bounds checks to prevent out-of-bounds register (GRF)
// access. In many APIs out-of-bounds register access (e.g., to vector elements)
// is undefined behavior, e.g., see
// https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html#OpVectorExtractDynamic
// However, applications do have bugs and the goal of this pass is to avoid
// crashes in such scenarios.
//
// Note: the pass does not assure that out-of-bounds read access returns zero.
//
// Potential future improvements:
// - use code assumptions to avoid adding out-of-bounds checks when the index
//   is known to be inbounds
//
// Code example 1:
// Before:
//     %elem = extractelement <4 x i32> %vec, i32 %idx
// After:
//     %0 = icmp ule i32 %idx, 3
//     %1 = select i1 %0, i32 %idx, i32 3
//     %elem = extractelement <4 x i32> %vec, i32 %1
//
// Code example 2:
// Before:
//    Label-Entry:
//     %vec1 = insertelement <4 x i32> %vec, i32 %elem, i32 %idx
//     ret <4 x i32> %vec1
// After:
//   Label-Entry:
//     %0 = icmp ule i32 %idx, 3
//     br i1 %0, label %Label-True, label %Label-Merge
//   Label-True:
//     %vec1 = insertelement <4 x i32> %vec, i32 %elem, i32 %idx
//     br label %Label-Merge
//  Label-Merge:
//    %vec11 = phi <4 x i32> [ %vec, %Label-Entry ], [ %vec1, %Label-True ]
//    ret <4 x i32> %vec11
//
// Code example 3:
// Before:
//   %ret = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %data, i32 %idx, i32 2)
// After:
//   %simdSize = call i32 @llvm.genx.GenISA.simdSize()
//   %simdMask = sub i32 %simdSize, 1
//   %idxInBounds = and i32 %idx, %simdMask
//   %ret = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %data, i32 %idxInBounds, i32 2)
//
////////////////////////////////////////////////////////////////////////////////
class RegisterAccessBoundsChecks : public llvm::FunctionPass {
public:
  static char ID;

  RegisterAccessBoundsChecks();
  ~RegisterAccessBoundsChecks() {};

  bool runOnFunction(llvm::Function &F) override;

  StringRef getPassName() const override { return "RegisterAccessBoundsChecks"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override { AU.addRequired<CodeGenContextWrapper>(); }

private:
  uint GetSimdSize() const;
  uint GetMinSimdSize() const;

private:
  SmallVector<ExtractElementInst *, 8> m_ExtractElements;
  SmallVector<InsertElementInst *, 8> m_InsertElements;
  SmallVector<GenIntrinsicInst *, 8> m_SubgroupShuffles;
};

IGC_INITIALIZE_PASS_BEGIN(RegisterAccessBoundsChecks, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(RegisterAccessBoundsChecks, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char RegisterAccessBoundsChecks::ID = 0;

RegisterAccessBoundsChecks::RegisterAccessBoundsChecks() : FunctionPass(RegisterAccessBoundsChecks::ID) {
  initializeRegisterAccessBoundsChecksPass(*PassRegistry::getPassRegistry());
}

// Returns the number of vector elements.
inline uint GetNumVectorElements(Type *ty) {
  IGC_ASSERT(ty->isVectorTy());
  auto *vecTy = cast<IGCLLVM::FixedVectorType>(ty);
  return int_cast<uint>(vecTy->getNumElements());
}

// Returns the number of vector elements of the vector object
// of the ExtractElement instruction.
inline uint GetIndexLimit(ExtractElementInst *ee) { return GetNumVectorElements(ee->getVectorOperand()->getType()); }

// Returns the number of vector elements of the vector object
// of the InsertElement instruction.
inline uint GetIndexLimit(InsertElementInst *ie) { return GetNumVectorElements(ie->getType()); }

// Returns the index operand of the ExtractElement instruction.
inline Value *GetIndexOperand(ExtractElementInst *ee) { return ee->getIndexOperand(); }

// Returns the index operand of the InsertElement instruction.
inline Value *GetIndexOperand(InsertElementInst *ie) { return ie->getOperand(2); }

// Returns the (dynamic) index operand of a subgroup shuffle
// instruction
inline Value *GetIndexOperand(GenIntrinsicInst *intr) {
  switch (intr->getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_WaveBroadcast:
  case GenISAIntrinsic::GenISA_WaveShuffleIndex:
    return intr->getOperand(1);
  default:
    IGC_ASSERT_MESSAGE(0, "Subgroup shuffle intrinsic expected");
    return nullptr;
  }
}

// Updates the index operand of a subgroup shuffle instruction
inline void SetIndexOperand(GenIntrinsicInst *intr, Value *index) {
  switch (intr->getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_WaveBroadcast:
  case GenISAIntrinsic::GenISA_WaveShuffleIndex:
    intr->setOperand(1, index);
    break;
  default:
    IGC_ASSERT_MESSAGE(0, "Subgroup shuffle intrinsic expected");
  }
}

inline bool IsSubgroupShuffleIntrinsic(const Instruction *I) {
  const GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(I);
  if (!GII) {
    return false;
  }
  // Shuffle down is bounded by EmitPass, as its in-range index includes the lane id.
  switch (GII->getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_WaveBroadcast:
  case GenISAIntrinsic::GenISA_WaveShuffleIndex:
    return true;
  default:
    return false;
  }
}

// Checks if the instruction is "inbounds" based on the static code analysis.
template <typename IntT> bool IsInBounds(IntT *inst, const DataLayout &DL) {
  Value *index = GetIndexOperand(inst);
  uint limit = GetIndexLimit(inst);
  if (isa<SelectInst>(index)) {
    Value *LHS;
    Value *RHS;
    auto result = matchSelectPattern(index, LHS, RHS);
    if (result.Flavor == SPF_SMIN || result.Flavor == SPF_UMIN) {
      ConstantInt *imm = isa<ConstantInt>(LHS) ? cast<ConstantInt>(LHS) : dyn_cast<ConstantInt>(RHS);
      if (imm && ((result.Flavor == SPF_SMIN && imm->getSExtValue() < limit) ||
                  (result.Flavor == SPF_UMIN && imm->getZExtValue() < limit))) {
        return true;
      }
    }
  }
  KnownBits knownBits = computeKnownBits(index, DL);
  auto maxValue = ~(knownBits.Zero); // can use knownBits.getMaxValue() in LLVM14+
  return maxValue.getZExtValue() < limit;
}

// Checks if the value is the maximum simd lane id, i.e., it matches
// the following patterns:
// Pattern #1:
//   %simdSize = call i32 @llvm.genx.GenISA.simdSize()
//   %v = sub i32 %simdSize, 1
// Pattern #2:
//   %simdSize = call i32 @llvm.genx.GenISA.simdSize()
//   %v = add i32 %simdSize, -1
inline bool IsMaxSimdLaneId(Value *v) {
  auto IsEqual = [](Value *a, int c) {
    ConstantInt *imm = dyn_cast<ConstantInt>(a);
    return imm && imm->getSExtValue() == c;
  };
  auto IsSimdSizeIntr = [](Value *a) {
    GenIntrinsicInst *intr = dyn_cast<GenIntrinsicInst>(a);
    return intr && intr->getIntrinsicID() == GenISAIntrinsic::GenISA_simdSize;
  };
  BinaryOperator *op = dyn_cast<BinaryOperator>(v);
  if (op) {
    Value *a = op->getOperand(0);
    Value *b = op->getOperand(1);
    if (op->getOpcode() == Instruction::Add) {
      return (IsSimdSizeIntr(a) && IsEqual(b, -1)) || (IsSimdSizeIntr(b) && IsEqual(a, -1));
    } else if (op->getOpcode() == Instruction::Sub) {
      return IsSimdSizeIntr(a) && IsEqual(b, 1);
    }
  }
  return false;
}

// Checks if the instruction is "inbounds" based on the static code analysis.
inline bool IsInBounds(GenIntrinsicInst *intr, uint minSimdSize, const DataLayout &DL) {
  Value *index = GetIndexOperand(intr);
  if (isa<SelectInst>(index)) {
    Value *LHS;
    Value *RHS;
    auto result = matchSelectPattern(index, LHS, RHS);
    if (result.Flavor == SPF_SMIN || result.Flavor == SPF_UMIN) {
      // Check for the following pattern:
      //   %isInBound = icmp ule i32 %idx, 7
      //   %idxClamped = select i1 %isInBound, i32 %idx, i32 7
      ConstantInt *imm = isa<ConstantInt>(LHS) ? cast<ConstantInt>(LHS) : dyn_cast<ConstantInt>(RHS);
      if (imm && ((result.Flavor == SPF_SMIN && imm->getSExtValue() < minSimdSize) ||
                  (result.Flavor == SPF_UMIN && imm->getZExtValue() < minSimdSize))) {
        return true;
      }
      // Check for the following pattern:
      //   %simdSize = call i32 @llvm.genx.GenISA.simdSize()
      //   %simdMask = sub i32 %simdSize, 1
      //   %isInBound = icmp ule i32 %idx, %simdMask
      //   %idxClamped = select i1 %isInBound, i32 %idx, i32 %simdMask
      if (IsMaxSimdLaneId(LHS) || IsMaxSimdLaneId(RHS)) {
        return true;
      }
    }
  }
  // Check for the following pattern:
  //   %simdSize = call i32 @llvm.genx.GenISA.simdSize()
  //   %simdMask = sub i32 %simdSize, 1
  //   %idxClamped = and i32 %idx, %simdMask
  if (BinaryOperator *op = dyn_cast<BinaryOperator>(index)) {
    Value *a = op->getOperand(0);
    Value *b = op->getOperand(1);
    if (op->getOpcode() == Instruction::And && (IsMaxSimdLaneId(a) || IsMaxSimdLaneId(b))) {
      return true;
    }
  }
  // Check for the following pattern:
  //   %activeMask = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  //   %electedLane = call i32 @llvm.genx.GenISA.firstbitLo(i32 %id28-)
  const GenIntrinsicInst *fbl = dyn_cast<GenIntrinsicInst>(index);
  if (fbl && fbl->getIntrinsicID() == GenISAIntrinsic::GenISA_firstbitLo && isa<GenIntrinsicInst>(fbl->getOperand(0))) {
    const GenIntrinsicInst *ballot = cast<GenIntrinsicInst>(fbl->getOperand(0));
    if (ballot->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveBallot && isa<ConstantInt>(ballot->getOperand(0)) &&
        cast<ConstantInt>(ballot->getOperand(0))->getZExtValue() == 1) {
      return true;
    }
  }
  // Check for the following pattern:
  //   %SubgroupLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  //   %SubgroupLaneIdI32 = zext i16 %9 to i32
  if (ZExtInst *zext = dyn_cast<ZExtInst>(index)) {
    if (const GenIntrinsicInst *simdLaneId = dyn_cast<GenIntrinsicInst>(zext->getOperand(0))) {
      if (simdLaneId->getIntrinsicID() == GenISAIntrinsic::GenISA_simdLaneId) {
        return true;
      }
    }
  }

  // Check if the maximum possible value is smaller than the min simd size
  KnownBits knownBits = computeKnownBits(index, DL);
  auto maxValue = ~(knownBits.Zero); // can use knownBits.getMaxValue() in LLVM14+
  return maxValue.getZExtValue() < minSimdSize;
}

// Returns the simd size if it can be derived from the shader type and/or
// metadata or 0.
uint RegisterAccessBoundsChecks::GetSimdSize() const {
  CodeGenContext *ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  switch (ctx->type) {
  case ShaderType::RAYTRACING_SHADER:
    IGC_ASSERT(ctx->knownSIMDSize().has_value());
    return ctx->knownSIMDSize().has_value() ? numLanes(ctx->knownSIMDSize().value()) : 0;

  case ShaderType::OPENCL_SHADER:
  default:
    return 0;
  }
}

// Returns the simd size if it can be derived from the shader type and/or
// metadata or the minimum simd size for the platform.
uint RegisterAccessBoundsChecks::GetMinSimdSize() const {
  const CodeGenContext *ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  uint minSimdSize = numLanes(ctx->platform.getMinDispatchMode());
  return std::max(minSimdSize, GetSimdSize());
}

bool RegisterAccessBoundsChecks::runOnFunction(Function &F) {
  m_ExtractElements.clear();
  m_InsertElements.clear();
  m_SubgroupShuffles.clear();
  bool modified = false;
  // Gather all ExtractElement and InsertElement instructions with
  // non-constant index operands.
  for (auto &I : instructions(F)) {
    if (isa<ExtractElementInst>(&I)) {
      ExtractElementInst *ee = cast<ExtractElementInst>(&I);
      if (!isa<Constant>(ee->getIndexOperand())) {
        m_ExtractElements.push_back(ee);
      }
    } else if (isa<InsertElementInst>(&I)) {
      InsertElementInst *ie = dyn_cast<InsertElementInst>(&I);
      if (!isa<Constant>(ie->getOperand(2))) {
        m_InsertElements.push_back(ie);
      }
    } else if (IsSubgroupShuffleIntrinsic(&I)) {
      GenIntrinsicInst *intr = cast<GenIntrinsicInst>(&I);
      if (!isa<Constant>(GetIndexOperand(intr))) {
        m_SubgroupShuffles.push_back(intr);
      }
    }
  }
  const DataLayout &DL = F.getParent()->getDataLayout();
  IGCIRBuilder<> builder(F.getContext());
  // Add runtime checks for ExtractElement instructions.
  for (auto ee : m_ExtractElements) {
    if (!IsInBounds(ee, DL)) {
      builder.SetInsertPoint(ee);
      Value *index = GetIndexOperand(ee);
      Value *clampedIndex;
      uint numElements = GetIndexLimit(ee);
      Constant *lastElemIndex = ConstantInt::get(index->getType(), numElements - 1);
      if (isPowerOf2_32(numElements)) {
        clampedIndex = builder.CreateAnd(index, lastElemIndex, VALUE_NAME(index->getName() + "InBounds"));
      } else {
        clampedIndex = builder.CreateSelect(builder.CreateICmpULE(index, lastElemIndex), index, lastElemIndex,
                                            VALUE_NAME(index->getName() + "InBounds"));
      }
      ee->replaceUsesOfWith(index, clampedIndex);
      modified = true;
    }
  }
  // Add runtime checks for InsertElement instructions.
  for (auto ie : m_InsertElements) {
    if (!IsInBounds(ie, DL)) {
      builder.SetInsertPoint(ie);
      Value *index = GetIndexOperand(ie);
      Constant *lastElemIndex = ConstantInt::get(index->getType(), GetIndexLimit(ie) - 1);
      Value *isInBounds = builder.CreateICmpULE(index, lastElemIndex);
      BasicBlock *headerBlock = ie->getParent();
      BasicBlock *trueBlock =
          headerBlock->splitBasicBlock(ie, VALUE_NAME(headerBlock->getName() + index->getName() + "InBounds"));
      BasicBlock *mergeBlock = trueBlock->splitBasicBlock(
          ie->getNextNode(), VALUE_NAME(headerBlock->getName() + index->getName() + "BoundsCheckMerge"));
      builder.SetInsertPoint(headerBlock->getTerminator());
      builder.CreateCondBr(isInBounds, trueBlock, mergeBlock);
      headerBlock->getTerminator()->eraseFromParent();
      builder.SetInsertPoint(&*mergeBlock->getFirstInsertionPt());
      llvm::PHINode *phi = builder.CreatePHI(ie->getType(), 2, VALUE_NAME(ie->getName()));
      ie->replaceAllUsesWith(phi);
      phi->addIncoming(ie->getOperand(0), headerBlock);
      phi->addIncoming(ie, trueBlock);
      modified = true;
    }
  }
  // Add runtime checks subgroup instructions
  for (auto intr : m_SubgroupShuffles) {
    uint minSimdSize = GetMinSimdSize();
    uint simdSize = GetSimdSize();
    if (!IsInBounds(intr, minSimdSize, DL)) {
      Value *index = GetIndexOperand(intr);
      builder.SetInsertPoint(intr);
      Value *simdMask = nullptr;
      if (simdSize > 0) {
        // There is a single simd size for this shader.
        simdMask = ConstantInt::get(index->getType(), simdSize - 1);
      } else {
        Function *const simdSizeIntr = GenISAIntrinsic::getDeclaration(F.getParent(), GenISAIntrinsic::GenISA_simdSize);
        simdMask = builder.CreateSub(builder.CreateCall(simdSizeIntr), ConstantInt::get(index->getType(), 1),
                                     VALUE_NAME("SimdMask"));
      }
      index = builder.CreateAnd(index, simdMask, VALUE_NAME(index->getName() + "Clamped"));
      SetIndexOperand(intr, index);
      modified = true;
    }
  }
  return modified;
}

bool needsRegisterAccessBoundsChecks(const CodeGenContext &Ctx) {
  return !Ctx.platform.supportsOutOfBoundsGrfAccess() && Ctx.m_DriverInfo.needsRegisterAccessBoundsChecks();
}

Pass *createRegisterAccessBoundsChecks() { return new RegisterAccessBoundsChecks(); }
} // namespace IGC
