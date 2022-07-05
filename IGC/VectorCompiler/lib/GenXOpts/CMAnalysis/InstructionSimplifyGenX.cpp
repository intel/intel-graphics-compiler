/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file defines a routine for simplifying a GenX intrinsic call to a
// constant or one of the operands. This is for cases where not all operands
// are constant; the constant operand cases are handled in ConstantFoldGenX.cpp.
//
//===----------------------------------------------------------------------===//

#include "llvmWrapper/Analysis/CallGraph.h"
#include "llvmWrapper/IR/CallSite.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/DerivedTypes.h"

#include "vc/GenXOpts/GenXAnalysis.h"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Utils/GenX/Region.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/Analysis/InstructionSimplify.h>
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

#include "llvmWrapper/Support/TypeSize.h"
#include "llvmWrapper/IR/DerivedTypes.h"

#include "Probe/Assertion.h"

#define DEBUG_TYPE "genx-simplify"

using namespace llvm;

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

static Value *GenXSimplifyInstruction(llvm::Instruction *Inst) {
  IGC_ASSERT(Inst);
  if (!GenXEnablePeepholes)
    return nullptr;
  if (Inst->getOpcode() == Instruction::Mul) {
    return simplifyMulDDQ(*cast<BinaryOperator>(Inst));
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
    IGC_ASSERT_EXIT_MESSAGE(0, "wrong argument: write region intrinsics are expected");
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
  IGC_ASSERT_MESSAGE(isWriteWithUndefInput(Inst),
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
Value *llvm::SimplifyGenXIntrinsic(unsigned IID, Type *RetTy, Use *ArgBegin,
                                   Use *ArgEnd, const DataLayout &DL) {
  switch (IID) {
    case GenXIntrinsic::genx_rdregioni:
    case GenXIntrinsic::genx_rdregionf:
      // Identity rdregion can be simplified to its "old value" input.
      if (RetTy
          == ArgBegin[GenXIntrinsic::GenXRegion::OldValueOperandNum]->getType()) {
        unsigned NumElements = dyn_cast<IGCLLVM::FixedVectorType>(RetTy)->getNumElements();
        unsigned Width = cast<ConstantInt>(
              ArgBegin[GenXIntrinsic::GenXRegion::RdWidthOperandNum])
            ->getZExtValue();
        auto IndexV = dyn_cast<Constant>(
          ArgBegin[GenXIntrinsic::GenXRegion::RdIndexOperandNum]);
        if (!IndexV)
          return nullptr;
        unsigned Index = 0;
        if (!isa<VectorType>(IndexV->getType()))
          Index = dyn_cast<ConstantInt>(IndexV)->getZExtValue() /
                  (DL.getTypeSizeInBits(RetTy->getScalarType()) / 8);
        else
          return nullptr;
        if ((Index == 0 || Index >= NumElements) &&
            (Width == NumElements || Width == cast<ConstantInt>(ArgBegin[
             GenXIntrinsic::GenXRegion::RdVStrideOperandNum])->getSExtValue()))
          if (NumElements == 1 || cast<ConstantInt>(ArgBegin[
                GenXIntrinsic::GenXRegion::RdStrideOperandNum])->getSExtValue())
            return ArgBegin[GenXIntrinsic::GenXRegion::OldValueOperandNum];
      }
      // rdregion with splatted constant input can be simplified to a constant of
      // the appropriate type, ignoring the possibly variable index.
      if (auto C = dyn_cast<Constant>(
            ArgBegin[GenXIntrinsic::GenXRegion::OldValueOperandNum]))
        if (auto Splat = C->getSplatValue()) {
          if (auto VT = dyn_cast<IGCLLVM::FixedVectorType>(RetTy))
            return ConstantVector::getSplat(
                IGCLLVM::getElementCount(VT->getNumElements()), Splat);
          return Splat;
        }
      break;
    case GenXIntrinsic::genx_wrregioni:
    case GenXIntrinsic::genx_wrregionf:
      // The wrregion case specifically excludes genx_wrconstregion.
      // Identity wrregion can be simplified to its "new value" input.
      if (RetTy
          == ArgBegin[GenXIntrinsic::GenXRegion::NewValueOperandNum]->getType()) {
        if (auto CMask = dyn_cast<Constant>(ArgBegin[
              GenXIntrinsic::GenXRegion::PredicateOperandNum])) {
          if (CMask->isAllOnesValue()) {
            unsigned NumElements = dyn_cast<IGCLLVM::FixedVectorType>(RetTy)->getNumElements();
            unsigned Width = cast<ConstantInt>(
                  ArgBegin[GenXIntrinsic::GenXRegion::WrWidthOperandNum])
                ->getZExtValue();
            auto IndexV = dyn_cast<Constant>(
              ArgBegin[GenXIntrinsic::GenXRegion::WrIndexOperandNum]);
            if (!IndexV)
              return nullptr;
            unsigned Index = 0;
            if (!isa<VectorType>(IndexV->getType()))
              Index = dyn_cast<ConstantInt>(IndexV)->getZExtValue() /
                      (DL.getTypeSizeInBits(RetTy->getScalarType()) / 8);
            else
              return nullptr;
            if ((Index == 0 || Index >= NumElements) &&
                (Width == NumElements || Width == cast<ConstantInt>(ArgBegin[
                 GenXIntrinsic::GenXRegion::WrVStrideOperandNum])->getSExtValue()))
              if (NumElements == 1 || cast<ConstantInt>(ArgBegin[
                    GenXIntrinsic::GenXRegion::WrStrideOperandNum])->getSExtValue())
                return ArgBegin[GenXIntrinsic::GenXRegion::NewValueOperandNum];
          }
        }
      }
      // Wrregion with constant 0 predicate can be simplified to its "old value"
      // input.
      if (auto CMask = dyn_cast<Constant>(ArgBegin[
            GenXIntrinsic::GenXRegion::PredicateOperandNum]))
        if (CMask->isNullValue())
          return ArgBegin[GenXIntrinsic::GenXRegion::OldValueOperandNum];
      // Wrregion writing a value that has just been read out of the same
      // region in the same vector can be simplified to its "old value" input.
      // This works even if the predicate is not all true.
      if (auto RdR = dyn_cast<CallInst>(ArgBegin[
            GenXIntrinsic::GenXRegion::NewValueOperandNum])) {
        if (auto RdRFunc = RdR->getCalledFunction()) {
          Value *OldVal = ArgBegin[GenXIntrinsic::GenXRegion::OldValueOperandNum];
          if ((GenXIntrinsic::getGenXIntrinsicID(RdRFunc) ==
                   GenXIntrinsic::genx_rdregioni ||
               GenXIntrinsic::getGenXIntrinsicID(RdRFunc) ==
                   GenXIntrinsic::genx_rdregionf) &&
              RdR->getArgOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum)
                == OldVal) {
            // Check the region parameters match between the rdregion and
            // wrregion. There are 4 region parameters: vstride, width, stride,
            // index.
            bool CanSimplify = true;
            for (unsigned i = 0; i != 4; ++i) {
              if (ArgBegin[GenXIntrinsic::GenXRegion::WrVStrideOperandNum + i]
                  != RdR->getArgOperand(
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
  Value *V = IGCLLVM::getCalledValue(I);
  Type *Ty = V->getType();
  if (auto *PTy = dyn_cast<PointerType>(Ty))
    Ty = PTy->getPointerElementType();
  auto *FTy = cast<FunctionType>(Ty);
  auto *F = dyn_cast<Function>(V);
  if (!F)
    return nullptr;

  LLVM_DEBUG(dbgs() << "Trying to simplify " << *I << "\n");
  auto GenXID = GenXIntrinsic::getGenXIntrinsicID(F);
  if (Value *Ret = SimplifyGenXIntrinsic(GenXID, FTy->getReturnType(),
                                         I->arg_begin(), I->arg_end(), DL)) {
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
public:
  static char ID;

  GenXSimplify() : FunctionPass(ID) {
    initializeGenXSimplifyPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addPreserved<DominatorTreeWrapperPass>();
  }

private:
  std::vector<CallInst *> WorkSet;

  bool processGenXIntrinsics(Function &F);
  bool simplifyGenXLscAtomic(CallInst &CI, const DominatorTree &DT);
};
} // namespace

bool GenXSimplify::runOnFunction(Function &F) {
  const DataLayout &DL = F.getParent()->getDataLayout();
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

      if (GenXIntrinsic::isGenXIntrinsic(Inst)) {
        if (Value *V = SimplifyGenX(cast<CallInst>(Inst), DL)) {
          Changed |= replaceWithNewValue(*Inst, *V);
          continue;
        }
      }

      // Do general LLVM simplification
      if (Value *V = SimplifyInstruction(Inst, DL)) {
        Changed |= replaceWithNewValue(*Inst, *V);
        continue;
      }

      // Do GenX-specific Instruction simplification
      if (Value *V = GenXSimplifyInstruction(Inst)) {
        Changed |= replaceWithNewValue(*Inst, *V);
        continue;
      }
    }
  }
  Changed |= processGenXIntrinsics(F);
  Changed |= simplifyWritesWithUndefInput(F);
  return Changed;
}

bool GenXSimplify::processGenXIntrinsics(Function &F) {

  if (!GenXEnablePeepholes) {
    LLVM_DEBUG(dbgs() << "genx-specific peepholes disabled\n");
    return false;
  }

  bool Changed = false;

  for (Instruction &Inst : instructions(F))
    if (GenXIntrinsic::isGenXIntrinsic(&Inst))
      WorkSet.push_back(cast<CallInst>(&Inst));

  const auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  while (!WorkSet.empty()) {
    auto *CI = WorkSet.back();
    WorkSet.pop_back();

    auto GenXID = GenXIntrinsic::getGenXIntrinsicID(CI);
    switch (GenXID) {
    case GenXIntrinsic::genx_lsc_atomic_bti:
      Changed |= simplifyGenXLscAtomic(*CI, DT);
      LLVM_DEBUG(dbgs() << "finished <lsc atomic> processing\n");
      break;
    default:
      (void)CI; // do nothing
    }
  }

  return Changed;
}
/***********************************************************************
 * simplifyGenXLscAtomic : transforms partial updates of a value
 * by a predicated lsc_atomic operation to be more bale-friendly.
 * This allows us to produce more efficient code for such partial updates.
 * Transformation works like this:
 *   Before:
 *      OldValue = ...
 *      Predicate = ...
 *      LscResult = lsc_atomic(Predicate, ..., undef)
 *      UpdatedValue = select(Predicate, LscResult, OldValue)
 *   After:
 *      OldValue = ...
 *      Predicate = ...
 *      UpdatedValue = lsc_atomic(Predicate, ..., OldValue)
 * The last argument of such atomic operation represents the "previous value"
 * which gets updated by the operation. See "TWOADDR" property for more details
 */
bool GenXSimplify::simplifyGenXLscAtomic(CallInst &CI,
                                         const DominatorTree &DT) {

  IGC_ASSERT(GenXIntrinsic::getGenXIntrinsicID(&CI) ==
         GenXIntrinsic::genx_lsc_atomic_bti);

  LLVM_DEBUG(dbgs() << "processing <lsc atomic>: " << CI << "\n");

  if (!isa<UndefValue>(CI.getArgOperand(IGCLLVM::getNumArgOperands(&CI) - 1))) {
    LLVM_DEBUG(dbgs() << "  skipping as instruction already has some " <<
               "\"previous value\" set\n");
    return false;
  }
  if (!CI.hasOneUse()) {
    LLVM_DEBUG(dbgs() << "  skipping as instruction has more than one use\n");
    return false;
  }

  auto *Select = dyn_cast<SelectInst>(CI.user_back());
  if (!Select) {
    LLVM_DEBUG(dbgs() << "  skipping as a user of atomic is not a select\n");
    return false;
  }

  auto *SelectCondition = Select->getCondition();
  auto *InstPredicate = CI.getArgOperand(0);

  LLVM_DEBUG(dbgs() << "SelectCondition: " << *SelectCondition << "\n");
  LLVM_DEBUG(dbgs() << "InstPredicate: " << *InstPredicate << "\n");

  if (SelectCondition != InstPredicate) {
    // Are these instructoins ?
    if (isa<Instruction>(SelectCondition) && isa<Instruction>(InstPredicate)) {
      if (!cast<Instruction>(SelectCondition)
               ->isIdenticalTo(cast<Instruction>(InstPredicate))) {
        return false;
      }

      LLVM_DEBUG(dbgs() << "  condition is equivalent (as an instructions)!\n");

    } else {
      LLVM_DEBUG(dbgs() << "  condition does not match!\n");
      return false;
    }
  }

  LLVM_DEBUG(dbgs() << "  condition match!\n");

  auto *TrueInst = Select->getTrueValue();
  auto *FalseInst = Select->getFalseValue();

  Value *PrevValue = nullptr;
  if (&CI == TrueInst) {
    PrevValue = FalseInst;
  } else if (&CI == FalseInst) {
    PrevValue = TrueInst;
  }
  // Given that we've already determined that the Select is a user of our
  // instruction we do an assertion test to find a candidate for
  // the previous value
  IGC_ASSERT_MESSAGE(PrevValue,
    "candidate for the previous value must be not found");

  if (isa<Instruction>(PrevValue) &&
      !DT.dominates(cast<Instruction>(PrevValue), &CI)) {
    LLVM_DEBUG(dbgs() << "previous value does not dominate candidate!\n");
    return false;
  }
  CI.setArgOperand(IGCLLVM::getNumArgOperands(&CI) - 1, PrevValue);

  Select->replaceAllUsesWith(&CI);
  Select->eraseFromParent();

  LLVM_DEBUG(dbgs() << "  updated instr: " << CI << "\n");
  return true;
}

char GenXSimplify::ID = 0;
INITIALIZE_PASS_BEGIN(GenXSimplify, "genx-simplify",
                      "simplify genx specific instructions", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_END(GenXSimplify, "genx-simplify",
                    "simplify genx specific instructions", false, false)

FunctionPass *llvm::createGenXSimplifyPass() { return new GenXSimplify; }
