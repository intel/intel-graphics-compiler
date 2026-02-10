/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SubGroupReductionPattern.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/PatternMatch.h>

#include "common/igc_regkeys.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

using namespace llvm;
using namespace llvm::PatternMatch;
using namespace IGC;
using namespace IGCMD;

// Pattern of instructions:
//   %lane8 = call i32 <shuffle-op>(i32 %input, i32 8)
//   %value8 = <alu-op> i32 %input, %lane8
//   %lane4 = call i32 <shuffle-op>(i32 %value8, i32 4)
//   %value4 = <alu-op> i32 %value8, %lane4
//   %lane2 = call i32 <shuffle-op>(i32 %value4, i32 2)
//   %value2 = <alu-op> i32 %value4, %lane2
//   %lane1 = call i32 <shuffle-op>(i32 %value2, i32 1)
//   %result = <alu-op> i32 %value2, %lane1
//
// Where shuffle-op is a builtin returning a value from a different lane in the subgroup
// (like simdShuffleXor or WaveShuffleIndex).
class ShufflePattern {
public:
  struct PatternStep {
    PatternStep(Value *InputValue, GenIntrinsicInst *ShuffleOp, Instruction *Op, uint64_t Lane)
        : InputValue(InputValue), ShuffleOp(ShuffleOp), Op(Op), Lane(Lane) {}

    Value *InputValue;

    // Shuffle InputValue with other SIMD lane.
    GenIntrinsicInst *ShuffleOp;
    uint64_t Lane;

    // Op on InputValue and ShuffleOp result.
    Instruction *Op;
  };

  ShufflePattern(Value *InputValue, GenIntrinsicInst *ShuffleOp, Instruction *Op, WaveOps OpType, uint64_t Lane)
      : OpType(OpType) {
    Steps.emplace_back(InputValue, ShuffleOp, Op, Lane);
  }

  bool append(Value *InputValue, GenIntrinsicInst *ShuffleOp, Instruction *Op, WaveOps OpType, uint64_t Lane);

  WaveOps OpType;
  SmallVector<PatternStep, 8> Steps;
};

// A half of i64 shuffled as <2 x i32>.
// See SubGroupReductionPattern::matchVectorShufflePattern for details.
struct VectorShufflePattern {
  VectorShufflePattern(Instruction *Op, uint64_t Lane, uint64_t VectorIndex)
      : Op(Op), Lane(Lane), VectorIndex(VectorIndex) {}

  bool match(Instruction *Op, uint64_t Lane, uint64_t VectorIndex) {
    return this->Op == Op && this->Lane == Lane &&
           ((this->VectorIndex == 0 && VectorIndex == 1) || (this->VectorIndex == 1 && VectorIndex == 0));
  }

  Instruction *Op;
  uint64_t Lane;
  uint64_t VectorIndex;
};

// See SubGroupReductionPattern::getReductionType for explanation on each reduction type.
struct ReductionType {
  static ReductionType Invalid() { return {GenISAIntrinsic::no_intrinsic, 0, 0}; }
  static ReductionType WaveAll() { return {GenISAIntrinsic::GenISA_WaveAll, 0, 0}; }
  static ReductionType WaveClustered(int ClusterSize) {
    return {GenISAIntrinsic::GenISA_WaveClustered, ClusterSize, 0};
  }
  static ReductionType WaveInterleave(int InterleaveStep) {
    return {GenISAIntrinsic::GenISA_WaveInterleave, 0, InterleaveStep};
  }
  static ReductionType WaveClusteredInterleave(int ClusterSize, int InterleaveStep) {
    return {GenISAIntrinsic::GenISA_WaveClusteredInterleave, ClusterSize, InterleaveStep};
  }

  bool isValid() const { return IntrinsicType != GenISAIntrinsic::no_intrinsic; }

  const GenISAIntrinsic::ID IntrinsicType;
  const int ClusterSize;
  const int InterleaveStep;
};

// Pass for matching common manual subgroup reduction pattern and replacing them
// with corresponding GenISA.Wave* call.
class SubGroupReductionPattern : public llvm::FunctionPass, public llvm::InstVisitor<SubGroupReductionPattern> {
public:
  static char ID; // Pass identification, replacement for typeid

  SubGroupReductionPattern();

  virtual llvm::StringRef getPassName() const override { return "SubGroupReductionPattern"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  virtual bool runOnFunction(Function &F) override;

  void visitCallInst(llvm::CallInst &C);

private:
  void visitSimdShuffleXor(GenIntrinsicInst &ShuffleOp);
  void visitWaveShuffleIndex(GenIntrinsicInst &ShuffleOp);

  void matchShufflePattern(GenIntrinsicInst &ShuffleOp, uint64_t Lane);
  void matchVectorShufflePattern(GenIntrinsicInst &ShuffleOp, uint64_t Lane);
  void addShufflePattern(Value *InputValue, GenIntrinsicInst &ShuffleOp, Instruction *Op, uint64_t Lane);

  bool reduce(ShufflePattern &Pattern);
  ReductionType getReductionType(ShufflePattern &Pattern);
  bool isSupportedWaveClusteredInterleave(ShufflePattern &Pattern, int ClusterSize, int InterleaveStep);

  static WaveOps getWaveOp(Instruction *Op);

  CodeGenContext *CGC = nullptr;

  int SubGroupSize = 0;
  bool Modified = false;

  SmallVector<ShufflePattern, 8> Matches;

  // For i64 shuffle done as two i32 shuffles (vector <2 x i32>), each
  // of i32 shuffle is matched as separate pattern. This map temporary
  // holds first of the matched pair.
  DenseMap<Instruction *, VectorShufflePattern> VectorShufflePatterns;
};

SubGroupReductionPattern::SubGroupReductionPattern() : FunctionPass(ID) {
  initializeSubGroupReductionPatternPass(*PassRegistry::getPassRegistry());
}

void SubGroupReductionPattern::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<MetaDataUtilsWrapper>();
  AU.setPreservesCFG();
}

bool SubGroupReductionPattern::runOnFunction(llvm::Function &F) {
  if (F.hasOptNone())
    return false;

  CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  auto MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto FII = MDU->findFunctionsInfoItem(&F);
  if (FII == MDU->end_FunctionsInfo())
    return false;

  auto SubGroupSizeMD = FII->second->getSubGroupSize();
  if (!SubGroupSizeMD->hasValue())
    return false;

  SubGroupSize = SubGroupSizeMD->getSIMDSize();

  Modified = false;
  Matches.clear();

  // Collect matches.
  visit(F);

  // Replace matches.
  for (auto &Match : Matches) {
    Modified |= reduce(Match);
  }

  return Modified;
}

void SubGroupReductionPattern::visitCallInst(llvm::CallInst &C) {
  if (GenIntrinsicInst *I = llvm::dyn_cast<GenIntrinsicInst>(&C)) {
    switch (I->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_simdShuffleXor:
      return visitSimdShuffleXor(*I);
    case GenISAIntrinsic::GenISA_WaveShuffleIndex:
      return visitWaveShuffleIndex(*I);
    default:
      return;
    }
  }
}

void SubGroupReductionPattern::visitSimdShuffleXor(GenIntrinsicInst &ShuffleOp) {
  // Expected pattern:
  //   %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %value, i32 8)
  //   %result = <op> i32 %value, %simdShuffleXor

  ConstantInt *Lane = nullptr;

  if (match(ShuffleOp.getOperand(1), m_ConstantInt(Lane)))
    matchShufflePattern(ShuffleOp, Lane->getZExtValue());
}

void SubGroupReductionPattern::visitWaveShuffleIndex(GenIntrinsicInst &ShuffleOp) {
  // Expected pattern:
  //   %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  //   %0 = xor i16 %simdLaneId, 16
  //   %1 = zext i16 %xor16 to i32
  //   %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %value, i32 %1, i32 0)
  //   %result = <op> i32 %value, %simdShuffle

  ConstantInt *HelperLanes = dyn_cast<ConstantInt>(ShuffleOp.getOperand(2));
  if (!HelperLanes || HelperLanes->getZExtValue() != 0)
    return;

  Value *SimdLaneId = nullptr;
  ConstantInt *Lane = nullptr;

  if (match(ShuffleOp.getOperand(1), m_ZExt(m_c_Xor(m_Value(SimdLaneId), m_ConstantInt(Lane)))) ||
      match(ShuffleOp.getOperand(1), m_c_Xor(m_ZExt(m_Value(SimdLaneId)), m_ConstantInt(Lane)))) {
    if (GenIntrinsicInst *I = dyn_cast<GenIntrinsicInst>(SimdLaneId)) {
      if (I->getIntrinsicID() == GenISAIntrinsic::GenISA_simdLaneId)
        matchShufflePattern(ShuffleOp, Lane->getZExtValue());
    }
  }
}

void SubGroupReductionPattern::matchShufflePattern(GenIntrinsicInst &ShuffleOp, uint64_t Lane) {
  if (Lane != 1 && Lane % 2 != 0)
    return;

  if (!ShuffleOp.hasOneUse())
    return;

  Instruction *Op = ShuffleOp.user_back();

  if (isa<InsertElementInst>(Op)) {
    return matchVectorShufflePattern(ShuffleOp, Lane);
  }

  if (getWaveOp(Op) == WaveOps::UNDEF)
    return;

  Value *InputValue = ShuffleOp.getOperand(0);
  if (((Op->getOperand(0) == InputValue && Op->getOperand(1) == &ShuffleOp) ||
       (Op->getOperand(0) == &ShuffleOp && Op->getOperand(1) == InputValue)) == false)
    return;

  addShufflePattern(InputValue, ShuffleOp, Op, Lane);
}

void SubGroupReductionPattern::addShufflePattern(Value *InputValue, GenIntrinsicInst &ShuffleOp, Instruction *Op,
                                                 uint64_t Lane) {
  WaveOps OpType = getWaveOp(Op);
  if (OpType == WaveOps::UNDEF)
    return;

  // Check if type is supported.
  if (Op->getType()->isIntegerTy(64) && CGC->platform.need64BitEmulation())
    return;

  // Continues previous pattern?
  for (auto &Match : Matches) {
    if (Match.append(InputValue, &ShuffleOp, Op, OpType, Lane))
      return;
  }

  // New pattern.
  Matches.emplace_back(InputValue, &ShuffleOp, Op, OpType, Lane);
}

// Shuffle of i64 type can be split into two shuffles of i32 type. This method handles
// such case; it matches the following pattern:
//
//   %3 = bitcast i64 %value to <2 x i32>
//   %value1 = extractelement <2 x i32> %3, i64 0
//   %value2 = extractelement <2 x i32> %3, i64 1
//   %simdShuffleXor1 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %value1, i32 8)
//   %simdShuffleXor2 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %value2, i32 8)
//   %shuffledVec1 = insertelement <2 x i32> undef, i32 %simdShuffleXor1, i64 0
//   %shuffledVec2 = insertelement <2 x i32> %shuffledVec1, i32 %simdShuffleXor2, i64 1
//   %shuffled = bitcast <2 x i32> %shuffledVec2 to i64
//   %result = <op> i64 %value, %shuffled
void SubGroupReductionPattern::matchVectorShufflePattern(GenIntrinsicInst &ShuffleOp, uint64_t Lane) {
  auto CheckVectorType = [](Type *Ty) {
    if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty))
      return VTy->getNumElements() == 2;
    return false;
  };

  // Match instructions that happen before shuffle, that is:
  //
  //   %3 = bitcast i64 %value to <2 x i32>
  //   %value1 = extractelement <2 x i32> %3, i64 0
  //   %value2 = extractelement <2 x i32> %3, i64 1
  //   %simdShuffleXor1 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %value1, i32 8)
  //   %simdShuffleXor2 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %value2, i32 8)
  //
  // Collect:
  //    1. Input value.
  //    2. BitCast instruction (to validate type).
  //    2. ExtractElement index.

  Value *InputValue = nullptr;
  Instruction *BitCast = nullptr;
  uint64_t VectorIndex = 0;

  if (!match(ShuffleOp.getOperand(0),
             m_OneUse(m_ExtractElt(m_CombineAnd(m_Instruction(BitCast), m_BitCast(m_Value(InputValue))),
                                   m_ConstantInt(VectorIndex)))))
    return;

  if (VectorIndex != 0 && VectorIndex != 1)
    return;

  if (InputValue->getType()->isVectorTy() || !CheckVectorType(BitCast->getType()))
    return;

  // Match instructions that happen after shuffle, that is:
  //
  //   %simdShuffleXor1 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %value1, i32 8)
  //   %simdShuffleXor2 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %value2, i32 8)
  //   %shuffledVec1 = insertelement <2 x i32> undef, i32 %simdShuffleXor1, i64 0
  //   %shuffledVec2 = insertelement <2 x i32> %shuffledVec1, i32 %simdShuffleXor2, i64 1
  //   %shuffled = bitcast <2 x i32> %shuffledVec2 to i64
  //   %result = <op> i64 %value, %shuffled
  //
  // Collect:
  //    1. InsertElement instruction (to validate type).

  Instruction *InsertElement = nullptr;

  // First pattern - shuffle is used in second insertelement.
  auto Pattern1 = m_OneUse(m_BitCast(
      m_OneUse(m_CombineAnd(m_Instruction(InsertElement),
                            m_InsertElt(m_Value(), m_OneUse(m_Specific(&ShuffleOp)), m_SpecificInt(VectorIndex))))));

  // Second pattern - shuffle is used in first insertelement.
  auto Pattern2 = m_OneUse(m_BitCast(m_OneUse(m_InsertElt(
      m_OneUse(m_CombineAnd(m_Instruction(InsertElement),
                            m_InsertElt(m_Undef(), m_OneUse(m_Specific(&ShuffleOp)), m_SpecificInt(VectorIndex)))),
      m_Value(), m_SpecificInt(VectorIndex ? 0 : 1)))));

  for (auto *User : InputValue->users()) {
    Instruction *Op = cast<Instruction>(User);

    if (match(Op, m_c_BinOp(m_Specific(InputValue), Pattern1)) ||
        match(Op, m_c_BinOp(m_Specific(InputValue), Pattern2))) {
      if (!CheckVectorType(InsertElement->getType()))
        return;

      // Now that pattern is matched, check if this is the first or second shuffle of the pair.
      if (VectorShufflePatterns.count(Op)) {
        if (VectorShufflePatterns.find(Op)->second.match(Op, Lane, VectorIndex)) {
          // Collected two parts of i64 shuffle, create new pattern.
          addShufflePattern(InputValue, ShuffleOp, Op, Lane);
        }
      } else {
        // First part of i64 shuffle, store for later matching.
        VectorShufflePatterns.try_emplace(Op, Op, Lane, VectorIndex);
      }

      return;
    }
  }
}

bool ShufflePattern::append(Value *InputValue, GenIntrinsicInst *ShuffleOp, Instruction *Op, WaveOps OpType,
                            uint64_t Lane) {
  if (this->OpType != OpType)
    return false;

  Instruction *PreviousValue = Steps.back().Op;

  if (PreviousValue->getNumUses() != 2)
    return false;

  if (InputValue != PreviousValue)
    return false;

  if (Op->getOperand(0) != InputValue && Op->getOperand(1) != InputValue)
    return false;

  Steps.emplace_back(InputValue, ShuffleOp, Op, Lane);
  return true;
}

bool SubGroupReductionPattern::reduce(ShufflePattern &Pattern) {
  auto RTy = getReductionType(Pattern);
  if (!RTy.isValid())
    return false;

  auto *InputValue = Pattern.Steps.front().InputValue;
  auto *FirstShuffle = Pattern.Steps.front().ShuffleOp;
  auto *LastOp = Pattern.Steps.back().Op;

  IRBuilder<> IRB(FirstShuffle);
  IRB.SetCurrentDebugLocation(LastOp->getDebugLoc());

  SmallVector<Value *, 4> Args;
  Args.push_back(InputValue);
  Args.push_back(IRB.getInt8((uint8_t)Pattern.OpType));
  switch (RTy.IntrinsicType) {
  case GenISAIntrinsic::GenISA_WaveAll:
    Args.push_back(IRB.getInt1(true));
    break;
  case GenISAIntrinsic::GenISA_WaveClustered:
    Args.push_back(IRB.getInt32(RTy.ClusterSize));
    break;
  case GenISAIntrinsic::GenISA_WaveInterleave:
    Args.push_back(IRB.getInt32(RTy.InterleaveStep));
    break;
  case GenISAIntrinsic::GenISA_WaveClusteredInterleave:
    Args.push_back(IRB.getInt32(RTy.ClusterSize));
    Args.push_back(IRB.getInt32(RTy.InterleaveStep));
    break;
  default:
    break;
  }
  Args.push_back(IRB.getInt32(0));

  Function *WaveFunc = GenISAIntrinsic::getDeclaration(FirstShuffle->getCalledFunction()->getParent(),
                                                       RTy.IntrinsicType, Args[0]->getType());

  LastOp->replaceAllUsesWith(IRB.CreateCall(WaveFunc, Args));
  RecursivelyDeleteTriviallyDeadInstructions(LastOp);

  return true;
}

ReductionType SubGroupReductionPattern::getReductionType(ShufflePattern &Pattern) {
  // Check what shuffles are engaged in reduction.
  uint64_t XorMask = 0;
  for (auto &Step : Pattern.Steps) {
    if (XorMask & Step.Lane)
      return ReductionType::Invalid(); // Each xor must be unique
    XorMask |= Step.Lane;
  }

  if (XorMask == 0 || XorMask >= SubGroupSize)
    return ReductionType::Invalid();

  // WaveAll - Full reduction, all work items mixed. Xor mask has all relevant bits set.
  //
  // Example for SIMD8:
  //   XorMask = 111 => xors 1, 2, 4 (order doesn't matter)
  if (XorMask == (SubGroupSize - 1))
    return ReductionType::WaveAll();

  int LowBit = 0;
  for (; (XorMask & 1) == 0; XorMask >>= 1)
    LowBit += 1;

  int HighBit = LowBit;
  for (; (XorMask & 1) == 1; XorMask >>= 1)
    HighBit += 1;

  // Mask should be zero.
  if (XorMask)
    return ReductionType::Invalid();

  int ClusterSize = 1 << HighBit;
  int InterleaveStep = 1 << LowBit;

  // WaveClustered - Splits subgroup into smaller clusters of consecutive work items and reduces each cluster.
  //   Xor mask is expressed as lower N bits set.
  //
  // Example for SIMD8:
  //   XorMask = 011 => xors 1, 2 => cluster size 4
  //
  //   Work Item |       0 |       1 |       2 |       3 |       4 |       5 |       6 |       7 |
  //   xor 1     |     0,1 |     0,1 |     2,3 |     2,3 |     4,5 |     4,5 |     6,7 |     6,7 |
  //   xor 2     | 0,1,2,3 | 0,1,2,3 | 0,1,2,3 | 0,1,2,3 | 4,5,6,7 | 4,5,6,7 | 4,5,6,7 | 4,5,6,7 |
  if (InterleaveStep == 1)
    return ReductionType::WaveClustered(ClusterSize);

  // WaveInterleave - Reduces together every n-th work item, where n defines interleave step.
  //   Xor mask is expressed as upper N bits set.
  //
  // Example for SIMD8:
  //   XorMask = 110 => xors 2, 4 => interleave step 2
  //
  //   Work Item |       0 |       1 |       2 |       3 |       4 |       5 |       6 |       7 |
  //   xor 2     |     0,2 |     1,3 |     0,2 |     1,3 |     4,6 |     5,7 |     4,6 |     5,7 |
  //   xor 4     | 0,2,4,6 | 1,3,5,7 | 0,2,4,6 | 1,3,5,7 | 0,2,4,6 | 1,3,5,7 | 0,2,4,6 | 1,3,5,7 |
  if (SubGroupSize == ClusterSize)
    return ReductionType::WaveInterleave(InterleaveStep);

  // WaveClusteredInterleave - Splits subgroup into smaller clusters (WaveClustered) and executes
  //   interleave reduction on each cluster (WaveInterleave).
  //
  // Example for SIMD8:
  //   XorMask = 010 => xors 2 => cluster size 4, interleave step 2
  //   clusters: { 0, 1, 2, 3 }, { 4, 5, 6, 7 }
  //
  //   Work Item |       0 |       1 |       2 |       3 |       4 |       5 |       6 |       7 |
  //   xor 2     |     0,2 |     1,3 |     0,2 |     1,3 |     4,6 |     5,7 |     4,6 |     5,7 |
  if (isSupportedWaveClusteredInterleave(Pattern, ClusterSize, InterleaveStep))
    return ReductionType::WaveClusteredInterleave(ClusterSize, InterleaveStep);

  return ReductionType::Invalid();
}

bool SubGroupReductionPattern::isSupportedWaveClusteredInterleave(ShufflePattern &Pattern, int ClusterSize,
                                                                  int InterleaveStep) {
  // WaveClusteredInterleave has very limited opportunity to optimize over normal shuffles. The basic
  // rule is that the longer the pattern, the more likely it is WaveClusteredInterleave will be more
  // efficient. Require at least two shuffles.
  if (Pattern.Steps.size() < 2)
    return false;

  // Only HPC platforms are supported.
  bool Valid = CGC->platform.isProductChildOf(IGFX_PVC);

  // At the moment only a selected number of scenarios are supported.
  auto *Type = Pattern.Steps.front().Op->getType();
  bool Is32BitType = Type->isIntegerTy(32) || Type->isFloatTy();
  bool Is64BitType = Type->isIntegerTy(64) || Type->isDoubleTy();

  Valid &= (SubGroupSize == 32 && ClusterSize == 16 && InterleaveStep == 2 && (Is32BitType || Is64BitType)) ||
           (SubGroupSize == 32 && ClusterSize == 8 && InterleaveStep == 2 && Is32BitType);

  if (IGC_IS_FLAG_ENABLED(PrintWaveClusteredInterleave)) {
    std::string DebugStr;
    llvm::raw_string_ostream OS(DebugStr);
    OS << (Valid ? "Replacing" : "Ignoring") << " ClusteredInterleave pattern in ["
       << Pattern.Steps.front().Op->getParent()->getParent()->getName() << "]"
       << "; SubGroupSize=" << SubGroupSize << "; ClusterSize=" << ClusterSize << "; InterleaveStep=" << InterleaveStep
       << "; Type=";
    Pattern.Steps.front().Op->getType()->print(OS);
    CGC->EmitWarning(OS.str().c_str());
  }

  return Valid;
}

WaveOps SubGroupReductionPattern::getWaveOp(Instruction *Op) {

  if (IntrinsicInst *I = llvm::dyn_cast<IntrinsicInst>(Op)) {
    switch (I->getIntrinsicID()) {
    case Intrinsic::umin:
      return WaveOps::UMIN;
    case Intrinsic::umax:
      return WaveOps::UMAX;
    case Intrinsic::smin:
      return WaveOps::IMIN;
    case Intrinsic::smax:
      return WaveOps::IMAX;
    case Intrinsic::minnum:
      return WaveOps::FMIN;
    case Intrinsic::maxnum:
      return WaveOps::FMAX;
    default:
      return WaveOps::UNDEF;
    }
  }

  if (!isa<BinaryOperator>(Op))
    return WaveOps::UNDEF;

  switch (Op->getOpcode()) {
  case Instruction::Add:
    return WaveOps::SUM;
  case Instruction::Mul:
    return WaveOps::PROD;
  case Instruction::Or:
    return WaveOps::OR;
  case Instruction::Xor:
    return WaveOps::XOR;
  case Instruction::And:
    return WaveOps::AND;
  case Instruction::FAdd:
    return WaveOps::FSUM;
  case Instruction::FMul:
    return WaveOps::FPROD;
  default:
    return WaveOps::UNDEF;
  }
}

// Register pass to igc-opt
#define PASS_FLAG "igc-subgroup-reduction-pattern"
#define PASS_DESCRIPTION "Matches common patterns for subgroup reductions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SubGroupReductionPattern, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(SubGroupReductionPattern, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char SubGroupReductionPattern::ID = 0;

FunctionPass *IGC::createSubGroupReductionPatternPass() { return new SubGroupReductionPattern(); }
