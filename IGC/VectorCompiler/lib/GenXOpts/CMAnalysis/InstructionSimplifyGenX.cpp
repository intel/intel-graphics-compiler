/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file defines a routine for simplifying a GenX intrinsic call to a
// constant or one of the operands. This is for cases where not all operands
// are constant; the constant operand cases are handled in ConstantFoldGenX.cpp.
//
//===----------------------------------------------------------------------===//

#include "llvmWrapper/Analysis/CallGraph.h"
#include "llvmWrapper/Analysis/InstructionSimplify.h"
#include "llvmWrapper/IR/CallSite.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include <llvmWrapper/IR/Type.h>

#include "vc/GenXOpts/GenXAnalysis.h"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"
#include "vc/Utils/GenX/Region.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/TypeSize.h"

#include "Probe/Assertion.h"

#define DEBUG_TYPE "GenXSimplify"

using namespace llvm;

namespace llvm {
namespace genx {
bool isSafeToReplace_CheckAVLoadKillOrForbiddenUser(
    const Instruction *const I, const Instruction *const To,
    const DominatorTree *const DT);
};
}; // namespace llvm

static cl::opt<bool>
    GenXEnablePeepholes("genx-peepholes", cl::init(true), cl::Hidden,
                        cl::desc("apply additional peephole optimizations"));

// Takes a constant and checks if it can be used as a RHS for mulDDQ operation
// A constant can be used as such if every component of it can be represented
// as signed/unsigned 32-bit integer.
// LHSigned represents the preferred representation for the 32-bit ints
static std::pair<Value *, bool> transformConstantToMulDDQOperand(bool LHSigned,
                                                                 Constant *C) {
  Type *Ty32 = Type::getInt32Ty(C->getContext());
  // Checks that every value from the input array can be converted to
  // desired type (signed/unsigned) 32-bit constant
  auto ConvertableToI32 = [Ty32](const ArrayRef<uint64_t> &UVs, bool Signed) {
    return std::all_of(UVs.begin(), UVs.end(), [Signed, Ty32](uint64_t U) {
      if (Signed) {
        auto S = static_cast<int64_t>(U);
        return ConstantInt::isValueValidForType(Ty32, S);
      }
      return ConstantInt::isValueValidForType(Ty32, U);
    });
  };
  SmallVector<uint64_t, 16> UVs;
  bool IsVector = C->getType()->isVectorTy();
  if (IsVector) {
    auto *CDS = cast<ConstantDataSequential>(C);
    for (unsigned i = 0, num = CDS->getNumElements(); i < num; ++i)
      UVs.push_back(CDS->getElementAsInteger(i));
  } else {
    UVs.push_back(cast<ConstantInt>(C)->getZExtValue());
  }
  bool RHSigned = false;
  if (ConvertableToI32(UVs, LHSigned)) {
    RHSigned = LHSigned;
  } else if (ConvertableToI32(UVs, !LHSigned)) {
    RHSigned = !LHSigned;
  } else
    return {nullptr, false}; // Constant can't be expressed as i32 type

  SmallVector<uint32_t, 16> CnvData;
  std::transform(UVs.begin(), UVs.end(), std::back_inserter(CnvData),
                 [](uint64_t V) { return static_cast<uint32_t>(V); });

  Value *V = !IsVector ? ConstantInt::get(Ty32, CnvData.front(), RHSigned)
                       : ConstantDataVector::get(Ty32->getContext(), CnvData);
  return {V, RHSigned};
}
// simplifyMulDDQ:
// Tries to detect cases when we do 64-bit mulitiplication which can be
// replaced by multiplication of 32-bit integers to generate a more efficient
// vISA.
// Currently, the following patterns are detected:
// DxD->Q:
//   l = sext op1 to i64
//   r = sext op2 to i64
//  -> res = genx_ssmul(op1, op2)
// UDxUD->UQ:
//   l = zext op1 to i64
//   r = zext op2 to i64
//   res = mul l, r
//  -> res = genx_uumul(op1, op2)
// One of op1/op2 can be constant
static Value *simplifyMulDDQ(BinaryOperator &Mul) {

  Value *LH = nullptr;
  Value *RH = nullptr;
  Constant *C = nullptr;

  // skip non 64-bit mulitplication
  if (Mul.getType()->getScalarSizeInBits() != 64)
    return nullptr;

  using namespace llvm::PatternMatch;
  if (!match(&Mul,
             m_c_Mul(m_ZExtOrSExt(m_Value(LH)),
                     m_CombineOr(m_ZExtOrSExt(m_Value(RH)), m_Constant(C)))))
    return nullptr;

  bool LHSigned = isa<SExtInst>(Mul.getOperand(0));
  bool RHSigned = isa<SExtInst>(Mul.getOperand(1));
  // If one of the operands is constant - we must make sure that we can convert
  // it to i32 type, so we can use 32-bit multiplication
  if (C) {
    // One of the operand is Constant => SExt can be anywhere
    LHSigned = LHSigned || RHSigned;
    std::tie(RH, RHSigned) = transformConstantToMulDDQOperand(LHSigned, C);
    if (!RH)
      return nullptr;
  }

  if (LH->getType()->getScalarSizeInBits() > 32 ||
      RH->getType()->getScalarSizeInBits() > 32)
    return nullptr;

  // Currently we do not support case when operands are of different signes.
  // It is possible to handle such cases, but in that case we should make
  // sure that such cases are handled properly by GenXLowering
  if (LHSigned != RHSigned)
    return nullptr;

  IRBuilder<> Builder(&Mul);
  if (LH->getType() != RH->getType()) {
    auto TryUpcast = [](IRBuilder<> &B, Value *V, Type *To, bool Sign) {
      if (V->getType()->getScalarSizeInBits() >= To->getScalarSizeInBits())
        return V;

      if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType()))
        To = IGCLLVM::FixedVectorType::get(To, VTy->getNumElements());

      return Sign ? B.CreateSExt(V, To, V->getName() + ".sext")
                  : B.CreateZExt(V, To, V->getName() + ".zext");
    };
    Type *Ty32 = Type::getInt32Ty(Mul.getContext());
    // TODO: probably we could upcast to RH->getType()/LH->getType()
    LH = TryUpcast(Builder, LH, Ty32, LHSigned);
    RH = TryUpcast(Builder, RH, Ty32, RHSigned);
  }

  auto *Ty64 = Mul.getType();
  auto *OpType = LH->getType();
  auto IID = GenXIntrinsic::getGenXMulIID(LHSigned, RHSigned);
  auto *FIMul =
      GenXIntrinsic::getGenXDeclaration(Mul.getModule(), IID, {Ty64, OpType});
  auto *Result = Builder.CreateCall(FIMul, {LH, RH}, Mul.getName() + ".imul");
  return Result;
}

static bool usesBothOperands(const ArrayRef<int> &Mask, int InputLen) {
  int OperandIdx = -1;
  for (int Index : Mask) {
    if (Index < 0)
      continue; // skip undef

    int CurrentIdx = (Index >= InputLen) ? 1 : 0;
    if (OperandIdx == -1) {
      OperandIdx = CurrentIdx;
    } else if (OperandIdx != CurrentIdx) {
      return true; // Both operands are used
    }
  }
  return false;
}

static void checkInputsForMask(ArrayRef<int> &Mask1, ArrayRef<int> &Mask2,
                               ArrayRef<int> &CurrentMask, bool &UsesInput1,
                               bool &UsesInput2) {
  for (auto Index : CurrentMask) {
    if (Index < 0) {
      continue; // Undef index.
    } else if (static_cast<unsigned>(Index) < Mask1.size()) {
      UsesInput1 = true;
    } else if (static_cast<unsigned>(Index) < Mask1.size() + Mask2.size()) {
      UsesInput2 = true;
    } else {
      IGC_ASSERT_EXIT(false && "Unexpected index in mask");
    }
  }
}

static int getInputLen(ShuffleVectorInst *CheckShuffle) {
  auto *Type =
      cast<IGCLLVM::FixedVectorType>(CheckShuffle->getOperand(0)->getType());
  return static_cast<int>(Type->getNumElements());
}

// Simplify ShuffleVector one-instruction chain
// Transform from:
//  %shuffle1 = shufflevector <16 x i1> %input1, <16 x i1> poison,
//     <256 x i32> <i32 undef (224 times), i32 0-15 (16), i32 undef (16 times)>
//  %combinedShuffle = shufflevector <256 x i1>  %shuffle1, <256 x i1> poison,
//     <256 x i32> <i32 undef (224 times), i32 224-239 & 256-271 (32)>
// To:
//  %finalShuffle = shufflevector <16 x i1> %input1, <16 x i1> poison,
//      <32 x i32> <i32 undef, ..., i32 0-15>
static Value *propagateShuffleVector(ShuffleVectorInst *Shuffle) {
  LLVM_DEBUG(dbgs() << "Simplifying shufflevector: " << *Shuffle << "\n");

  auto *Input1 = dyn_cast<ShuffleVectorInst>(Shuffle->getOperand(0));
  auto *Input2 = dyn_cast<ShuffleVectorInst>(Shuffle->getOperand(1));

  if (!Input1 && !Input2) {
    LLVM_DEBUG(
        dbgs()
        << "propagateShuffleVector: No chain detected, nothing to optimize.\n");
    return Shuffle;
  }

  bool UseInput1 = !Input2;
  auto *CheckShuffle = Input1 ? Input1 : Input2;
  ArrayRef<int> Mask = CheckShuffle->getShuffleMask();
  ArrayRef<int> CurrentMask = Shuffle->getShuffleMask();

  bool UsesInput1 = false;
  bool UsesInput2 = false;
  checkInputsForMask(Mask, Mask, CurrentMask, UsesInput1, UsesInput2);

  if (UsesInput1 && UsesInput2 || (UsesInput1 && !UseInput1) ||
      (UsesInput2 && UseInput1)) {
    LLVM_DEBUG(dbgs() << "Expected only one use in shuffle.\n");
    return Shuffle;
  }

  auto InputLen = getInputLen(CheckShuffle);

  SmallVector<int, 32> CombinedMask;
  // Combine the masks into a single mask.
  for (auto Index : CurrentMask) {
    if (Index < 0) {
      CombinedMask.push_back(-1); // Undef index.
    } else if (static_cast<unsigned>(Index) < Mask.size()) {
      CombinedMask.push_back(Mask[Index]);
    } else {
      CombinedMask.push_back(Mask[Index - Mask.size()] + InputLen);
    }
  }

  LLVM_DEBUG(dbgs() << "Combined mask: "; for (int Val
                                               : CombinedMask) dbgs()
                                          << Val << " ";
             dbgs() << "\n");

  // Create the final shuffle vector with the combined mask.
  IRBuilder<> Builder(Shuffle);
  auto *NewShuffle = Builder.CreateShuffleVector(
      CheckShuffle->getOperand(0), CheckShuffle->getOperand(1), CombinedMask);

  LLVM_DEBUG(dbgs() << "Created new shufflevector: " << *NewShuffle << "\n");

  return NewShuffle;
}

static bool checkInputsForMaskIndex(int InputLen, ArrayRef<int> Mask,
                                    int &InputOperandIdx) {
  for (auto Index : Mask) {
    if (Index < 0)
      continue; // skip undef
    int OperandIdx = 0;
    if (Index >= InputLen) {
      OperandIdx = 1;
    }
    if (InputOperandIdx == -1) {
      InputOperandIdx = OperandIdx;
    } else if (InputOperandIdx != OperandIdx) {
      // Both operands are used in the first shuffle.
      return false;
    }
  }
  return true;
}

// Simplify ShuffleVector multi-instructions chain
// Transform from:
//  %shuffle1 = shufflevector <16 x i1> %input1, <16 x i1> poison,
//     <256 x i32> <i32 undef (224 times), i32 0-15 (16), i32 undef (16 times)>
//  %shuffle2 = shufflevector <16 x i1> %input2, <16 x i1> poison,
//     <256 x i32> <i32 0-15 (16), i32 undef (240 times)>
//  %combinedShuffle = shufflevector <256 x i1>  %shuffle1, <256 x i1>
//  %shuffle2,
//     <256 x i32> <i32 undef (224 times), i32 224-239 & 256-271 (32)>
// To:
//  %finalShuffle = shufflevector <16 x i1> %input1, <16 x i1> %input2,
//      <32 x i32> <i32 undef, ..., i32 0-15, i32 16-31>
static Value *simplifyShuffleVectorChain(ShuffleVectorInst *Shuffle) {
  LLVM_DEBUG(dbgs() << "Simplifying shufflevector: " << *Shuffle << "\n");

  auto *Input1 = dyn_cast<ShuffleVectorInst>(Shuffle->getOperand(0));
  auto *Input2 = dyn_cast<ShuffleVectorInst>(Shuffle->getOperand(1));

  if (!Input1 || !Input2) {
    LLVM_DEBUG(dbgs() << "simplifyShuffleVectorChain: No chain detected, "
                         "nothing to optimize.\n");
    return Shuffle;
  }

  ArrayRef<int> Mask1 = Input1->getShuffleMask();
  ArrayRef<int> Mask2 = Input2->getShuffleMask();
  ArrayRef<int> CurrentMask = Shuffle->getShuffleMask();

  bool UsesInput1 = false;
  bool UsesInput2 = false;

  checkInputsForMask(Mask1, Mask2, CurrentMask, UsesInput1, UsesInput2);

  if (!UsesInput1 || !UsesInput2) {
    LLVM_DEBUG(dbgs() << "Only one input used in shuffle.\n");
    return Shuffle;
  }

  int Input1OperandIdx = -1;
  auto Input1Len = getInputLen(Input1);
  if (!checkInputsForMaskIndex(Input1Len, Mask1, Input1OperandIdx))
    return Shuffle;

  int Input2OperandIdx = -1;
  auto Input2Len = getInputLen(Input2);
  if (!checkInputsForMaskIndex(Input2Len, Mask2, Input2OperandIdx))
    return Shuffle;

  // If we have only one operand used in the first shuffle, we need to
  // set the other operand to 0.
  if (Input1OperandIdx == -1)
    Input1OperandIdx = 0;
  if (Input2OperandIdx == -1)
    Input2OperandIdx = 0;

  if (Input1->getOperand(Input1OperandIdx)->getType() !=
      Input2->getOperand(Input2OperandIdx)->getType()) {
    // The types of the operands are different.
    return Shuffle;
  }

  SmallVector<int, 32> CombinedMask;
  // Combine the masks into a single mask.
  for (auto Index : CurrentMask) {
    if (Index < 0) {
      CombinedMask.push_back(-1); // Undef index.
    } else if (static_cast<unsigned>(Index) < Mask1.size()) {
      CombinedMask.push_back(Mask1[Index] - Input1Len * Input1OperandIdx);
    } else {
      CombinedMask.push_back(Mask2[Index - Mask1.size()] +
                             Input1Len * (1 - Input2OperandIdx));
    }
  }

  LLVM_DEBUG(dbgs() << "Combined mask: "; for (int Val
                                               : CombinedMask) dbgs()
                                          << Val << " ";
             dbgs() << "\n");

  // Create the final shuffle vector with the combined mask.
  IRBuilder<> Builder(Shuffle);
  auto *NewShuffle = Builder.CreateShuffleVector(
      Input1->getOperand(Input1OperandIdx),
      Input2->getOperand(Input2OperandIdx), CombinedMask);

  LLVM_DEBUG(dbgs() << "Created new shufflevector: " << *NewShuffle << "\n");

  return NewShuffle;
}

static inline bool isBitcastFits(BitCastInst *BC) {
  return BC->getSrcTy()->isVectorTy() && BC->getDestTy()->isIntegerTy();
}

static inline bool isTrunkFits(TruncInst *TC) {
  return TC->getSrcTy()->isVectorTy() && TC->getDestTy()->isVectorTy() &&
         TC->getDestTy()->getScalarType()->isIntegerTy(1);
}

static inline bool simplifyBinOp(BinaryOperator *BinOp) {
  auto Op = BinOp->getOpcode();
  // Only logical operations without overflow are expected
  if ((Op != Instruction::And) && (Op != Instruction::Or) &&
      (Op != Instruction::Xor))
    return false;
  bool Changed = false;

  auto *Bitcast1 = dyn_cast<BitCastInst>(BinOp->getOperand(0));
  auto *Bitcast2 = dyn_cast<BitCastInst>(BinOp->getOperand(1));
  if (Bitcast1 && Bitcast2 && (Bitcast1->getSrcTy() == Bitcast2->getSrcTy())) {
    if (isBitcastFits(Bitcast1) && isBitcastFits(Bitcast2)) {
      for (auto *User : BinOp->users()) {
        if (auto *BitcastBack = dyn_cast<BitCastInst>(User)) {
          if (BitcastBack->getDestTy()->isVectorTy()) {
            IRBuilder<> Builder(BinOp);
            auto *NewOp =
                Builder.CreateBinOp(BinOp->getOpcode(), Bitcast1->getOperand(0),
                                    Bitcast2->getOperand(0));
            BitcastBack->replaceAllUsesWith(NewOp);
            Changed = true;
            BinOp = cast<BinaryOperator>(NewOp);
            break;
          }
        }
      }
    }
  }
  auto *Trunc1 = dyn_cast<TruncInst>(BinOp->getOperand(0));
  auto *Trunc2 = dyn_cast<TruncInst>(BinOp->getOperand(1));
  if (Trunc1 && Trunc2 && (Trunc1->getSrcTy() == Trunc2->getSrcTy())) {
    if (isTrunkFits(Trunc1) && isTrunkFits(Trunc2)) {
      for (auto *User : BinOp->users()) {
        if (auto *ZextInst = dyn_cast<ZExtInst>(User)) {
          if (auto *DestVecType = dyn_cast<VectorType>(ZextInst->getDestTy())) {
            if (DestVecType->getElementType()->isIntegerTy()) {
              IRBuilder<> Builder(BinOp);
              auto *NewOp =
                  Builder.CreateBinOp(BinOp->getOpcode(), Trunc1->getOperand(0),
                                      Trunc2->getOperand(0));
              ZextInst->replaceAllUsesWith(NewOp);
              Changed = true;
              break;
            }
          }
        }
      }
    }
  }
  return Changed;
}

static Value *GenXSimplifyInstruction(llvm::Instruction *Inst) {
  IGC_ASSERT(Inst);
  if (!GenXEnablePeepholes)
    return nullptr;
  if (Inst->getOpcode() == Instruction::Mul)
    return simplifyMulDDQ(*cast<BinaryOperator>(Inst));

  if (auto *Shuffle = dyn_cast<ShuffleVectorInst>(Inst)) {
    auto *Chain = simplifyShuffleVectorChain(Shuffle);
    if (Chain != Shuffle) {
      return Chain;
    }
    return propagateShuffleVector(Shuffle);
  }

  return nullptr;
}

// isWriteWithUndefInput - checks whether provided \p Inst is a write
// intrinsic (currently wrregion, wrpredregion) and it's input value
// (new value) is undef (undef value is written into a vector).
static bool isWriteWithUndefInput(const Instruction &Inst) {
  switch (GenXIntrinsic::getAnyIntrinsicID(&Inst)) {
  default:
    return false;
  case GenXIntrinsic::genx_wrregioni:
  case GenXIntrinsic::genx_wrregionf:
    return isa<UndefValue>(
        Inst.getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));
  case GenXIntrinsic::genx_wrpredregion:
    return isa<UndefValue>(Inst.getOperand(vc::WrPredRegionOperand::NewValue));
  }
}

static Value &getWriteOldValueOperand(Instruction &Inst) {
  switch (GenXIntrinsic::getAnyIntrinsicID(&Inst)) {
  default:
    IGC_ASSERT_EXIT_MESSAGE(
        0, "wrong argument: write region intrinsics are expected");
  case GenXIntrinsic::genx_wrregioni:
  case GenXIntrinsic::genx_wrregionf:
    return *Inst.getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
  case GenXIntrinsic::genx_wrpredregion:
    return *Inst.getOperand(vc::WrPredRegionOperand::OldValue);
  }
}

// processWriteWithUndefInput - removes provided \p Inst, replaces its uses
// with the old value. If this replacement produced new context (write
// intrinsic's input value was  replaced with undef), those writes are put into
// \p ToProcess output iterator.
template <typename OutIter>
void processWriteWithUndefInput(Instruction &Inst, OutIter ToProcess) {
  IGC_ASSERT_MESSAGE(
      isWriteWithUndefInput(Inst),
      "wrong argument: write intrinsic with undef input was expected");
  auto *OldVal = &getWriteOldValueOperand(Inst);
  Inst.replaceAllUsesWith(OldVal);
  // As a result of operand promotion we can get new suitable instructions.
  // Using additional copy_if instead of make_filter_range as workaround,
  // because user_iterator returns pointer instead of reference.
  std::vector<User *> UsersToProcess;
  std::copy_if(Inst.user_begin(), Inst.user_end(),
               std::back_inserter(UsersToProcess), [](User *Usr) {
                 return isa<Instruction>(Usr) &&
                        isWriteWithUndefInput(*cast<Instruction>(Usr));
               });
  std::transform(UsersToProcess.begin(), UsersToProcess.end(), ToProcess,
                 [](User *Usr) { return cast<Instruction>(Usr); });
  Inst.eraseFromParent();
}

bool llvm::simplifyWritesWithUndefInput(Function &F) {
  using WorkListT = std::vector<Instruction *>;
  WorkListT WorkList;
  auto WorkListRange =
      make_filter_range(instructions(F), [](const Instruction &Inst) {
        return isWriteWithUndefInput(Inst);
      });
  llvm::transform(WorkListRange, std::back_inserter(WorkList),
                  [](Instruction &Inst) { return &Inst; });
  bool Modified = !WorkList.empty();
  while (!WorkList.empty()) {
    WorkListT CurrentWorkList = std::move(WorkList);
    WorkList = WorkListT{};
    auto WorkListInserter = std::back_inserter(WorkList);
    std::for_each(CurrentWorkList.begin(), CurrentWorkList.end(),
                  [WorkListInserter](Instruction *Inst) {
                    processWriteWithUndefInput(*Inst, WorkListInserter);
                  });
  }
  return Modified;
}

/***********************************************************************
 * SimplifyGenXIntrinsic : given a GenX intrinsic and a set of arguments,
 * see if we can fold the result.
 *
 * ConstantFoldingGenX.cpp handles pure constant folding cases. This code
 * only handles cases where not all operands are constant, but we can do
 * some folding anyway.
 *
 * If this call could not be simplified, returns null.
 */
Value *llvm::SimplifyGenXIntrinsic(CallInst *CI, const DataLayout &DL) {
  auto IID = vc::getAnyIntrinsicID(CI);
  auto *RetTy = CI->getType();
  Use *Args = CI->arg_begin();

  switch (IID) {
  case GenXIntrinsic::genx_rdregioni:
  case GenXIntrinsic::genx_rdregionf: {
    // Identity rdregion can be simplified to its "old value" input.
    if (RetTy ==
        Args[GenXIntrinsic::GenXRegion::OldValueOperandNum]->getType()) {
      unsigned NumElements =
          cast<IGCLLVM::FixedVectorType>(RetTy)->getNumElements();
      unsigned Width =
          cast<ConstantInt>(Args[GenXIntrinsic::GenXRegion::RdWidthOperandNum])
              ->getZExtValue();
      auto IndexV = dyn_cast<Constant>(
          Args[GenXIntrinsic::GenXRegion::RdIndexOperandNum]);
      if (!IndexV)
        return nullptr;
      unsigned Index = 0;
      if (!isa<VectorType>(IndexV->getType()))
        Index = cast<ConstantInt>(IndexV)->getZExtValue() /
                (DL.getTypeSizeInBits(RetTy->getScalarType()) / 8);
      else
        return nullptr;
      if ((Index == 0 || Index >= NumElements) &&
          (Width == NumElements ||
           Width == cast<ConstantInt>(
                        Args[GenXIntrinsic::GenXRegion::RdVStrideOperandNum])
                        ->getSExtValue()))
        if (NumElements == 1 ||
            cast<ConstantInt>(
                Args[GenXIntrinsic::GenXRegion::RdStrideOperandNum])
                ->getSExtValue())
          return Args[GenXIntrinsic::GenXRegion::OldValueOperandNum];
    }
    // rdregion with splatted constant input can be simplified to a constant of
    // the appropriate type, ignoring the possibly variable index.
    if (auto C = dyn_cast<Constant>(
            Args[GenXIntrinsic::GenXRegion::OldValueOperandNum]))
      if (auto Splat = C->getSplatValue()) {
        if (auto VT = dyn_cast<IGCLLVM::FixedVectorType>(RetTy))
          return ConstantVector::getSplat(
              IGCLLVM::getElementCount(VT->getNumElements()), Splat);
        return Splat;
      }
  } break;
  case GenXIntrinsic::genx_wrregioni:
  case GenXIntrinsic::genx_wrregionf:
    // The wrregion case specifically excludes genx_wrconstregion.
    // Identity wrregion can be simplified to its "new value" input.
    if (RetTy ==
        Args[GenXIntrinsic::GenXRegion::NewValueOperandNum]->getType()) {
      if (auto CMask = dyn_cast<Constant>(
              Args[GenXIntrinsic::GenXRegion::PredicateOperandNum])) {
        if (CMask->isAllOnesValue()) {
          unsigned NumElements =
              cast<IGCLLVM::FixedVectorType>(RetTy)->getNumElements();
          unsigned Width =
              cast<ConstantInt>(
                  Args[GenXIntrinsic::GenXRegion::WrWidthOperandNum])
                  ->getZExtValue();
          auto IndexV = dyn_cast<Constant>(
              Args[GenXIntrinsic::GenXRegion::WrIndexOperandNum]);
          if (!IndexV)
            return nullptr;
          unsigned Index = 0;
          if (!isa<VectorType>(IndexV->getType()))
            Index = cast<ConstantInt>(IndexV)->getZExtValue() /
                    (DL.getTypeSizeInBits(RetTy->getScalarType()) / 8);
          else
            return nullptr;
          if ((Index == 0 || Index >= NumElements) &&
              (Width == NumElements ||
               Width ==
                   cast<ConstantInt>(
                       Args[GenXIntrinsic::GenXRegion::WrVStrideOperandNum])
                       ->getSExtValue()))
            if (NumElements == 1 ||
                cast<ConstantInt>(
                    Args[GenXIntrinsic::GenXRegion::WrStrideOperandNum])
                    ->getSExtValue())
              return Args[GenXIntrinsic::GenXRegion::NewValueOperandNum];
        }
      }
    }
    // Wrregion with constant 0 predicate can be simplified to its "old value"
    // input.
    if (auto CMask = dyn_cast<Constant>(
            Args[GenXIntrinsic::GenXRegion::PredicateOperandNum]))
      if (CMask->isNullValue())
        return Args[GenXIntrinsic::GenXRegion::OldValueOperandNum];
    // Wrregion writing a value that has just been read out of the same
    // region in the same vector can be simplified to its "old value" input.
    // This works even if the predicate is not all true.
    if (auto RdR = dyn_cast<CallInst>(
            Args[GenXIntrinsic::GenXRegion::NewValueOperandNum])) {
      if (auto RdRFunc = RdR->getCalledFunction()) {
        Value *OldVal = Args[GenXIntrinsic::GenXRegion::OldValueOperandNum];
        if ((GenXIntrinsic::getGenXIntrinsicID(RdRFunc) ==
                 GenXIntrinsic::genx_rdregioni ||
             GenXIntrinsic::getGenXIntrinsicID(RdRFunc) ==
                 GenXIntrinsic::genx_rdregionf) &&
            RdR->getArgOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum) ==
                OldVal) {
          // Check the region parameters match between the rdregion and
          // wrregion. There are 4 region parameters: vstride, width, stride,
          // index.
          bool CanSimplify = true;
          for (unsigned i = 0; i != 4; ++i) {
            if (Args[GenXIntrinsic::GenXRegion::WrVStrideOperandNum + i] !=
                RdR->getArgOperand(
                    GenXIntrinsic::GenXRegion::RdVStrideOperandNum + i)) {
              CanSimplify = false;
              break;
            }
          }
          if (CanSimplify)
            return OldVal;
        }
      }
    }
    break;
  }
  return nullptr;
}

/***********************************************************************
 * SimplifyGenX : given a GenX related instruction, see if we can fold
 * the result.
 *
 * ConstantFoldingGenX.cpp handles pure constant folding cases. This code
 * also handles cases where not all operands are constant.
 *
 * If this instruction could not be simplified, returns null.
 */
Value *llvm::SimplifyGenX(CallInst *I, const DataLayout &DL) {
  LLVM_DEBUG(dbgs() << "Trying to simplify " << *I << "\n");

  if (Value *Ret = SimplifyGenXIntrinsic(I, DL)) {
    LLVM_DEBUG(dbgs() << "Simplified to " << *Ret << "\n");
    return Ret;
  }

  LLVM_DEBUG(dbgs() << "Failed to simplify, trying to constant fold\n");
  Constant *C = ConstantFoldGenX(I, DL);
  if (C)
    LLVM_DEBUG(dbgs() << "Successfully folded to " << *C << "\n");
  else
    LLVM_DEBUG(dbgs() << "Failed to constant fold instruction\n");
  return C;
}

namespace {
class GenXSimplify : public FunctionPass {
#if LLVM_VERSION_MAJOR >= 16
  DominatorTree &DT;
#endif
public:
  static char ID;

#if LLVM_VERSION_MAJOR < 16
  GenXSimplify() : FunctionPass(ID) {
#else
  GenXSimplify(DominatorTree &DT) : DT(DT), FunctionPass(ID) {
#endif
    initializeGenXSimplifyPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addPreserved<DominatorTreeWrapperPass>();
  }

private:
  bool processGenXIntrinsics(Function &F);
};
} // namespace

bool GenXSimplify::runOnFunction(Function &F) {
  const DataLayout &DL = F.getParent()->getDataLayout();
#if LLVM_VERSION_MAJOR < 16
  const auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
#endif
  bool Changed = false;

  auto replaceWithNewValue = [](Instruction &Inst, Value &V) {
    if (&Inst == &V)
      return false;
    Inst.replaceAllUsesWith(&V);
    Inst.eraseFromParent();
    return true;
  };

  for (auto &BB : F) {
    for (auto I = BB.begin(); I != BB.end();) {
      Instruction *Inst = &*I++;

      if (auto IID = vc::getAnyIntrinsicID(Inst);
          vc::isAnyNonTrivialIntrinsic(IID)) {
        auto *V = SimplifyGenX(cast<CallInst>(Inst), DL);
        if (!V)
          continue;

        auto *NewI = dyn_cast<Instruction>(V);
        if (NewI && !genx::isSafeToReplace_CheckAVLoadKillOrForbiddenUser(
                        Inst, NewI, &DT))
          continue;

        Changed |= replaceWithNewValue(*Inst, *V);
        continue;
      }

      // Do general LLVM simplification
      if (Value *V = IGCLLVM::simplifyInstruction(Inst, DL)) {
        Changed |= replaceWithNewValue(*Inst, *V);
        continue;
      }

      // Do GenX-specific Instruction simplification
      if (Value *V = GenXSimplifyInstruction(Inst)) {
        Changed |= replaceWithNewValue(*Inst, *V);
        continue;
      }
      if (auto *BO = dyn_cast<BinaryOperator>(Inst))
        Changed |= simplifyBinOp(BO);
    }
  }

  Changed |= simplifyWritesWithUndefInput(F);
  return Changed;
}

char GenXSimplify::ID = 0;
INITIALIZE_PASS_BEGIN(GenXSimplify, "GenXSimplify",
                      "simplify genx specific instructions", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_END(GenXSimplify, "GenXSimplify",
                    "simplify genx specific instructions", false, false)

#if LLVM_VERSION_MAJOR < 16
namespace llvm {
FunctionPass *createGenXSimplifyPass() { return new GenXSimplify; }
} // namespace llvm
#else
PreservedAnalyses GenXSimplifyPass::run(Function &F,
                                        FunctionAnalysisManager &AM) {
  auto &DT = AM.getResult<DominatorTreeAnalysis>(F);
  GenXSimplify GenXSimpl(DT);
  if (GenXSimpl.runOnFunction(F))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}
#endif
