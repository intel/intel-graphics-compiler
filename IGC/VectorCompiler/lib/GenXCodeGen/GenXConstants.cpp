/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXConstants
/// -------------
///
/// GenXConstants is not in itself a pass. It contains utility functions and a
/// class used by other passes for constant loading.
///
/// loadNonSimpleConstants
/// ^^^^^^^^^^^^^^^^^^^^^^
///
/// The GenXPostLegalization pass calls loadNonSimpleConstants to insert a load
/// for any operand that is a non-simple constant. (A non-simple constant is one
/// that is too big or an invalid value for a constant operand.)
///
/// It is called in two places:
///
/// 1. in the GenXPostLegalization pass, run after legalization but
///    before CSE, so CSE has an opportunity to common up loaded non-simple
///    constants;
/// 2. later on in GenXCategory, to mop up non-simple constant operands
///    created by CSE's constant propagation.
///
/// This does not insert a load if the constant is "big simple" (that is, it is
/// illegally wide but each legalized part of it is simple) and it is used in
/// the "old value" operand of a wrregion, or as a call arg.  Inserting a load
/// of such a constant here would allow the load to be CSEd, which would be
/// counter productive as some of the uses would not be kill uses and so
/// coalescing would fail there.
///
/// Phi incoming constants are not loaded here; they are loaded in
/// loadPhiConstants called from GenXCategory. Phi constant loads do not need to
/// participate in CSE as loadPhiConstants has its own commoning up tailored for
/// phi nodes.
///
/// loadConstants
/// ^^^^^^^^^^^^^
///
/// This is called from GenXCategory.  It inserts a load for each constant
/// operand that is not allowed to be constant, but remains after
/// loadNonSimpleConstants.
///
/// Phi incoming constants are not loaded here; they are loaded in
/// loadPhiConstants called from GenXCategory.
///
/// loadPhiConstants
/// ^^^^^^^^^^^^^^^^
///
/// This is called from GenXCategory, and it inserts loads for constant phi
/// incomings, commoning up when possible and sensible.
///
/// Commoning up (inserting one load for multiple phi incomings with the same
/// constant, across one or more phi nodes) proceeds as follows:
///
/// Firstly, we divide the phi nodes into _webs_, where each web is the maximal
/// set of phi nodes that are related through phi nodes and two address
/// instructions, so will be coalesced later on in the flow.
///
/// Secondly, for a single web, we look for multiple uses of the same constant.
/// Such a constant has a load instruction inserted just once, at the end of the
/// nearest common dominator of all the corresponding incoming blocks.
///
/// If that insert point is in an empty split critical edge block, we instead
/// insert in the block above that, in the hope that the split critical edge
/// block can be removed later.
///
/// ConstantLoader
/// ^^^^^^^^^^^^^^
///
/// ConstantLoader is a class that represents a constant and information on how
/// to load it. This is where analysis happens of whether it is a legal packed
/// vector, or whether it needs multiple instructions to load it. It then has
/// methods to insert the code to load the constant.
///
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "GENX_CONSTANTS"

#include "GenXConstants.h"
#include "GenXGotoJoin.h"
#include "GenXIntrinsics.h"
#include "GenXUtil.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"

#include "Probe/Assertion.h"

#include "llvmWrapper/IR/Constants.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/MathExtras.h"
#include "llvmWrapper/Support/TypeSize.h"

using namespace llvm;
using namespace genx;

/***********************************************************************
 * loadConstantStruct : insert instructions to load a constant struct
 */
static Value *loadConstantStruct(
    Constant *C, Instruction *InsertPt, const GenXSubtarget &Subtarget,
    const DataLayout &DL,
    SmallVectorImpl<Instruction *> *AddedInstructions = nullptr) {
  auto ST = cast<StructType>(C->getType());
  Value *Agg = UndefValue::get(ST);
  for (unsigned i = 0, e = ST->getNumElements(); i != e; ++i) {
    Constant *El = C->getAggregateElement(i);
    if (isa<UndefValue>(El))
      continue;
    Value *LoadedEl = nullptr;
    if (isa<StructType>(El->getType()))
      LoadedEl =
          loadConstantStruct(El, InsertPt, Subtarget, DL, AddedInstructions);
    else {
      LoadedEl = ConstantLoader(El, Subtarget, DL, nullptr, AddedInstructions)
                     .loadBig(InsertPt);
    }
    auto *InsertInst =
        InsertValueInst::Create(Agg, LoadedEl, i, "loadstruct", InsertPt);
    Agg = InsertInst;
    if (AddedInstructions)
      AddedInstructions->push_back(InsertInst);
  }
  return Agg;
}

/***********************************************************************
 * loadNonSimpleConstants : for any non-simple or illegal size constant in
 *      an instruction, load it.
 *
 * Enter:   Inst = instruction to find constant operands in
 *          AddedInstructions = 0 else vector to push added instructions onto
 *
 * Return:  whether code was modified
 *
 * This does not load constants in a phi nodes. That is done in
 * loadPhiConstants.
 */
bool genx::loadNonSimpleConstants(
    Instruction *Inst, const GenXSubtarget &Subtarget, const DataLayout &DL,
    SmallVectorImpl<Instruction *> *AddedInstructions) {
  bool Modified = false;
  if (isa<PHINode>(Inst))
    return Modified;
  // Omit call target operand of a call.
  unsigned NumArgs = Inst->getNumOperands();
  auto CI = dyn_cast<CallInst>(Inst);
  if (CI)
    NumArgs = IGCLLVM::getNumArgOperands(CI);
  unsigned IID = vc::getAnyIntrinsicID(Inst);
  // Do not proceed loading of genx.alloca argument since its value doesn't
  // needed (only type matters) and always null.
  if (IID == GenXIntrinsic::genx_alloca)
    return Modified;
  for (unsigned i = 0; i != NumArgs; ++i) {
    if (isa<Constant>(Inst->getOperand(i))) {
      Use *U = &Inst->getOperandUse(i);
      Constant *C = dyn_cast<Constant>(*U);
      if (!C)
        continue;
      if (isa<UndefValue>(C))
        continue;
      if (opMustBeConstant(Inst, i))
        continue;
      if (C->getType()->isStructTy()) {
        *U = loadConstantStruct(C, Inst, Subtarget, DL, AddedInstructions);
        Modified = true;
        continue;
      }

      ConstantLoader CL(C, Subtarget, DL, Inst, AddedInstructions);
      if (CL.needFixingSimple()) {
        Modified = true;
        CL.fixSimple(i);
        continue;
      }
      if (CL.isSimple())
        continue;
      // Do not load a "big simple" constant for the "old value of vector"
      // input of a wrregion, so it does not get CSEd. CSEing it is
      // counter-productive because, if it has multiple uses, it will
      // need to be two-address copied by GenXCoalescing anyway.
      if (GenXIntrinsic::isWrRegion(IID)
          && i == GenXIntrinsic::GenXRegion::OldValueOperandNum
          && CL.isBigSimple())
        continue;
      // Similarly, do not load a "big simple" constant for a call arg.
      if (CI && IID == GenXIntrinsic::not_any_intrinsic && CL.isBigSimple())
        continue;
      *U = CL.loadBig(Inst);
      Modified = true;
    }
  }
  return Modified;
}

bool genx::loadConstantsForInlineAsm(
    CallInst *CI, const GenXSubtarget &Subtarget, const DataLayout &DL,
    SmallVectorImpl<Instruction *> *AddedInstructions) {
  IGC_ASSERT_MESSAGE(CI->isInlineAsm(), "Inline asm expected");
  bool Modified = false;
  auto ConstraintsInfo = genx::getGenXInlineAsmInfo(CI);
  Use *U;
  for (unsigned i = 0, e = ConstraintsInfo.size(), ArgNo = 0; i != e; ++i) {
    auto &Info = ConstraintsInfo[i];
    if (Info.isOutput())
      continue;
    U = &CI->getOperandUse(ArgNo);
    ArgNo++;
    if (auto C = dyn_cast<Constant>(*U)) {
      if (!isa<UndefValue>(C)) {
        switch (Info.getConstraintType()) {
        default:
          *U = ConstantLoader(C, Subtarget, DL, nullptr, AddedInstructions)
                   .load(CI);
          Modified = true;
          break;
        case ConstraintType::Constraint_n:
        case ConstraintType::Constraint_i:
        case ConstraintType::Constraint_F:
          break;
        }
      }
    }
  }
  return Modified;
}

/***********************************************************************
 * loadConstants : load constants as required for an instruction
 *
 * This handles operands that are not allowed to be constant. A constant
 * operand that needs loading because it is a non-simple constant is
 * handled in loadNonSimpleConstants.
 *
 * This does not load constants in a phi nodes. That is done in
 * loadPhiConstants.
 */
bool genx::loadConstants(Instruction *Inst, const GenXSubtarget &Subtarget,
                         const DataLayout &DL) {
  bool Modified = false;
  Use *U;
  if (isa<PHINode>(Inst))
    return Modified;
  if (isa<BinaryOperator>(Inst) &&
      Inst->getType()->getScalarType()->isIntegerTy(1)) {
    // Predicate binary operator: disallow constant operands, except
    // that xor with -1 is allowed.
    for (unsigned oi = 0; oi != 2; ++oi)
      if (auto C = dyn_cast<Constant>(Inst->getOperand(oi))) {
        auto IsNot = [=]() {
          if (oi != 1)
            return false;
          if (Inst->getOpcode() != Instruction::Xor)
            return false;
          if (!C->getType()->isVectorTy())
            return C->isAllOnesValue();
          Constant *C1 = C->getSplatValue();
          return C1 && C1->isAllOnesValue();
        };
        if (!IsNot()) {
          Inst->setOperand(oi, ConstantLoader(C, Subtarget, DL).load(Inst));
          Modified = true;
        }
      }
  }
  if (isa<SelectInst>(Inst)) {
    // select: disallow constant selector
    U = &Inst->getOperandUse(0);
    if (auto C = dyn_cast<Constant>(*U)) {
      *U = ConstantLoader(C, Subtarget, DL).load(Inst);
      Modified = true;
    }
    return Modified;
  }
  if (isa<InsertValueInst>(Inst)) {
    // insertvalue (inserting a value into a struct): disallow constant
    // on element operand.
    U = &Inst->getOperandUse(1);
    if (auto C = dyn_cast<Constant>(*U)) {
      *U = ConstantLoader(C, Subtarget, DL).load(Inst);
      Modified = true;
    }
    // Also disallow constant (other than undef) on old struct value operand.
    // We need to load each non-undef element separately.
    U = &Inst->getOperandUse(0);
    if (auto C = dyn_cast<Constant>(*U))
      if (!isa<UndefValue>(C))
        *U = loadConstantStruct(C, Inst, Subtarget, DL);
    return Modified;
  }
  if (auto Br = dyn_cast<BranchInst>(Inst)) {
    // Conditional branch: disallow constant condition.
    if (Br->isConditional()) {
      if (auto C = dyn_cast<Constant>(Br->getCondition())) {
        Br->setCondition(ConstantLoader(C, Subtarget, DL).load(Br));
        Modified = true;
      }
    }
    return Modified;
  }
  if (auto Ret = dyn_cast<ReturnInst>(Inst)) {
    // Return: disallow constant return value in a subroutine (internal
    // linkage).
    if (Ret->getNumOperands() && Ret->getParent()->getParent()->getLinkage()
          == GlobalValue::InternalLinkage) {
      if (auto C = dyn_cast<Constant>(Ret->getOperand(0))) {
        if (!C->getType()->isVoidTy() && !isa<UndefValue>(C)) {
          Ret->setOperand(0, ConstantLoader(C, Subtarget, DL).load(Ret));
          Modified = true;
        }
      }
    }
    return Modified;
  }
  auto CI = dyn_cast<CallInst>(Inst);
  if (!CI)
    return Modified;
  if (CI->isInlineAsm())
    return loadConstantsForInlineAsm(CI, Subtarget, DL, nullptr);
  int IntrinsicID = vc::getAnyIntrinsicID(CI);
  switch (IntrinsicID) {
    case GenXIntrinsic::not_any_intrinsic:
    case Intrinsic::fma:
    case GenXIntrinsic::genx_ssmad:
    case GenXIntrinsic::genx_sumad:
    case GenXIntrinsic::genx_usmad:
    case GenXIntrinsic::genx_uumad:
    case GenXIntrinsic::genx_output:
    case GenXIntrinsic::genx_output_1:
      // load all args for subroutine and some intrinsic calls.
      for (unsigned i = 0, e = IGCLLVM::getNumArgOperands(CI); i != e; ++i) {
        U = &CI->getOperandUse(i);
        if (auto C = dyn_cast<Constant>(*U)) {
          if (!isa<UndefValue>(C)) {
            *U = ConstantLoader(C, Subtarget, DL).loadBig(CI);
            Modified = true;
          }
        }
      }
      break;
    case GenXIntrinsic::genx_constanti:
    case GenXIntrinsic::genx_constantf:
      break;
    case GenXIntrinsic::genx_absi:
    case GenXIntrinsic::genx_absf:
      // abs modifier: disallow constant input.
      U = &CI->getOperandUse(0);
      if (auto C = dyn_cast<Constant>(*U)) {
        *U = ConstantLoader(C, Subtarget, DL).load(CI);
        Modified = true;
      }
      break;
    case GenXIntrinsic::genx_rdpredregion:
    case GenXIntrinsic::genx_any:
    case GenXIntrinsic::genx_all:
      // rdpredregion, any, all: disallow constant input
      U = &CI->getOperandUse(0);
      if (auto C = dyn_cast<Constant>(*U)) {
        *U = ConstantLoader(C, Subtarget, DL).load(CI);
        Modified = true;
      }
      break;
    case GenXIntrinsic::genx_rdregioni:
    case GenXIntrinsic::genx_rdregionf:
      // rdregion: disallow constant input
      U = &CI->getOperandUse(0);
      if (auto C = dyn_cast<Constant>(*U)) {
        *U = ConstantLoader(C, Subtarget, DL).loadBig(CI);
        Modified = true;
      }
      // Also disallow constant vector index (constant scalar OK).
      U = &CI->getOperandUse(GenXIntrinsic::GenXRegion::RdIndexOperandNum);
      if (auto C = dyn_cast<Constant>(*U)) {
        if (isa<VectorType>(C->getType())) {
          *U = ConstantLoader(C, Subtarget, DL).load(CI);
          Modified = true;
        }
      }
      break;
    case GenXIntrinsic::genx_wrpredpredregion:
      // wrpredpred: disallow constant "old vector" input unless undef
      U = &CI->getOperandUse(0);
      if (auto C = dyn_cast<Constant>(*U)) {
        if (!isa<UndefValue>(C)) {
          *U = ConstantLoader(C, Subtarget, DL).loadBig(CI);
          Modified = true;
        }
      }
      break;
    case GenXIntrinsic::genx_wrregioni:
    case GenXIntrinsic::genx_wrregionf:
      // wrregion: disallow constant "old vector" input unless undef
      U = &CI->getOperandUse(0);
      if (auto C = dyn_cast<Constant>(*U)) {
        if (!isa<UndefValue>(C)) {
          *U = ConstantLoader(C, Subtarget, DL).loadBig(CI);
          Modified = true;
        }
      }
      // Also disallow constant vector index (constant scalar OK).
      U = &CI->getOperandUse(GenXIntrinsic::GenXRegion::WrIndexOperandNum);
      if (auto C = dyn_cast<Constant>(*U)) {
        if (isa<VectorType>(C->getType())) {
          *U = ConstantLoader(C, Subtarget, DL).load(CI);
          Modified = true;
        }
      }
      // Also disallow constant predicate unless all ones.
      U = &CI->getOperandUse(GenXIntrinsic::GenXRegion::PredicateOperandNum);
      if (auto C = dyn_cast<Constant>(*U)) {
        if (!C->isAllOnesValue()) {
          *U = ConstantLoader(C, Subtarget, DL).load(CI);
          Modified = true;
        }
      }
      break;
    case GenXIntrinsic::genx_simdcf_goto:
      // goto: disallow constant predicate input, unless it is all 0. We want to
      // allow constant all 0, as it is the encoding used for an "else", and
      // loading the constant into a predicate register stops the finalizer's
      // structurizer working.
      U = &CI->getOperandUse(2);
      if (auto C = dyn_cast<Constant>(*U)) {
        if (!C->isNullValue()) {
          *U = ConstantLoader(C, Subtarget, DL).load(CI);
          Modified = true;
        }
      }
      break;
    default:
      // Intrinsic: check intrinsic descriptor to see where constant args
      // are allowed.
      // Iterate through each field in the intrinsic info.
      GenXIntrinsicInfo II(IntrinsicID);
      // Intrinsic not found.
      if (II.isNull())
        return Modified;
      unsigned MaxRawOperands = II.getTrailingNullZoneStart(CI);
      for (auto AI : II.getInstDesc()) {
        if (!AI.isArgOrRet() || AI.isRet())
          continue;
        // This field relates to an operand.
        U = &CI->getOperandUse(AI.getArgIdx());
        auto C = dyn_cast<Constant>(*U);
        if (!C)
          continue;
        // Operand is constant.
        // Allow constant if it is i1 or vector of i1 set to all ones; this
        // represents an "all true" predication field.
        if (C->getType()->getScalarType()->isIntegerTy(1) && C->isAllOnesValue())
          continue;
        // Allow constant if intrinsic descriptor allows it for this arg.
        if (!AI.isImmediateDisallowed())
          continue;
        // If it is a RAW operand, allow the constant if it's in the trailing
        // null region (it must be a null constant if so), or if the value
        // is undefined and RAW_NULLALLOWED is enabled.
        if (AI.isRaw()) {
          if ((unsigned)AI.getArgIdx() >= MaxRawOperands) {
            IGC_ASSERT(C->isNullValue());
            continue;
          }
          if (isa<UndefValue>(C) && AI.rawNullAllowed())
            continue;
        }
        // Also allow constant if it is undef in a TWOADDR
        if (isa<UndefValue>(C) && AI.getCategory() == GenXIntrinsicInfo::TWOADDR)
          continue;
        // Also allow constant if it is a reserved surface index.
        if (AI.getCategory() == GenXIntrinsicInfo::SURFACE &&
            visa::isReservedSurfaceIndex(visa::convertToSurfaceIndex(C))) {
          continue;
        }
        // Operand is not allowed to be constant. Insert code to load it.
        *U = ConstantLoader(C, Subtarget, DL).loadBig(CI);
        Modified = true;
      }
      break;
  }
  return Modified;
}

bool genx::areConstantsEqual(const Constant *C1, const Constant *C2) {
  IGC_ASSERT(C1 && C2);
  // If these are same constants then it's obviously true
  if (C1 == C2)
    return true;

  Type *C1Ty = C1->getType();
  Type *C2Ty = C2->getType();

  bool SameType = C1Ty == C2Ty;
  // If types are not the same then compare if types are bitcastable
  if (!SameType) {
    if (!C1Ty->canLosslesslyBitCastTo(C2Ty))
      return false;
  }

  // Most common case: check for zero initializers
  if (C1->isZeroValue() && C2->isZeroValue())
    return true;

  auto *GC1 = dyn_cast<GlobalValue>(C1);
  auto *GC2 = dyn_cast<GlobalValue>(C2);
  // TODO: check for specific versions of each global
  if (GC1 || GC2)
    return false;

  if (C1->getValueID() != C2->getValueID())
    return false;

  // Check contents

  if (const auto *C1Seq = dyn_cast<ConstantDataSequential>(C1)) {
    const auto *C2Seq = cast<ConstantDataSequential>(C2);
    StringRef C1RawData = C1Seq->getRawDataValues();
    StringRef C2RawData = C2Seq->getRawDataValues();
    if (C1RawData.size() == C2RawData.size())
      return (C1RawData.compare(C2RawData) == 0);
    return false;
  }

  switch (C1->getValueID()) {
  default:
    // Otherwise be conservative
    return false;
  case Value::ConstantIntVal: {
    const APInt &C1Int = cast<ConstantInt>(C1)->getValue();
    const APInt &C2Int = cast<ConstantInt>(C2)->getValue();
    return C1Int == C2Int;
  }
  case Value::ConstantFPVal: {
    const APFloat &C1FP = cast<ConstantFP>(C1)->getValueAPF();
    const APFloat &C2FP = cast<ConstantFP>(C2)->getValueAPF();
    return C1FP.bitcastToAPInt() == C2FP.bitcastToAPInt();
  }
  case Value::ConstantVectorVal: {
    const ConstantVector *C1CV = cast<ConstantVector>(C1);
    const ConstantVector *C2CV = cast<ConstantVector>(C2);
    unsigned NumElementsC1 =
        cast<IGCLLVM::FixedVectorType>(C1Ty)->getNumElements();
    unsigned NumElementsC2 =
        cast<IGCLLVM::FixedVectorType>(C2Ty)->getNumElements();
    if (NumElementsC1 != NumElementsC2)
      return false;
    for (uint64_t i = 0; i < NumElementsC1; ++i)
      if (!areConstantsEqual(cast<Constant>(C1CV->getOperand(i)),
                             cast<Constant>(C2CV->getOperand(i))))
        return false;
    return true;
  }
  case Value::ConstantArrayVal: {
    const ConstantArray *C1A = cast<ConstantArray>(C1);
    const ConstantArray *C2A = cast<ConstantArray>(C2);
    uint64_t NumElementsC1 = cast<ArrayType>(C1Ty)->getNumElements();
    uint64_t NumElementsC2 = cast<ArrayType>(C2Ty)->getNumElements();
    if (NumElementsC1 != NumElementsC2)
      return false;
    for (uint64_t i = 0; i < NumElementsC1; ++i)
      if (!areConstantsEqual(cast<Constant>(C1A->getOperand(i)),
                             cast<Constant>(C2A->getOperand(i))))
        return false;
    return true;
  }
  }
}

/***********************************************************************
 * cleanupConstantLoads : remove all genx.constant* intrinsics that have
 * non-constant source operand
 */
bool genx::cleanupConstantLoads(Function *F) {
  bool Modified = false;
  for (auto I = inst_begin(F), E = inst_end(F); I != E;) {
    auto *CI = dyn_cast<CallInst>(&*I++);
    if (!CI)
      continue;
    auto IID = vc::getAnyIntrinsicID(CI);
    if (IID != GenXIntrinsic::genx_constanti &&
        IID != GenXIntrinsic::genx_constantf &&
        IID != GenXIntrinsic::genx_constantpred)
      continue;
    if (isa<Constant>(CI->getOperand(0)))
      continue;
    CI->replaceAllUsesWith(CI->getOperand(0));
    CI->eraseFromParent();
    Modified = true;
  }
  return Modified;
}

/***********************************************************************
 * loadPhiConstants : load constant incomings in phi nodes, commoning up
 *      if appropriate
 */
bool genx::loadPhiConstants(Function &F, DominatorTree *DT,
                            const GenXSubtarget &Subtarget,
                            const DataLayout &DL, bool ExcludePredicate) {
  bool Modified = false;
  std::set<Instruction *> Done;
  for (BasicBlock &BB : F) {
    for (auto bi = BB.begin();; ++bi) {
      auto Phi = dyn_cast<PHINode>(&*bi);
      if (!Phi)
        break;
      if (!Done.insert(Phi).second)
        continue; // phi node already processed in some web
      // Gather the web of phi nodes and two address instructions related to
      // this one.  This is an approximation to the web of instructions that
      // will or could be coalesced.
      // (Use Web as a worklist of phi nodes and two address instructions to
      // use to find other phi nodes and two address instructions.)
      //
      // We process a web of related phi nodes at a time, rather than all phi
      // nodes that use the constant, to avoid this situation:
      // we try and common up two phi nodes in the same basic block (e.g. two
      // variables both initialized to 0 before a loop), but end up having to
      // insert a copy for one of them anyway in coalescing.
      SmallVector<Instruction *, 4> Web;
      Web.push_back(Phi);
      for (unsigned wi = 0; wi != Web.size(); ++wi) {
        auto Inst = Web[wi];
        unsigned oi = 0, oe = 0;
        if ((Phi = dyn_cast<PHINode>(Inst))) {
          // Phi node: process each incoming.
          oe = Phi->getNumIncomingValues();
        } else {
          if (auto *CI = dyn_cast<CallInst>(Inst)) {
            // Two address instruction: process just the two address operand.
            oi = *getTwoAddressOperandNum(CI);
            oe = oi + 1;
          } else {
            IGC_ASSERT(isa<CastInst>(Inst));
            oi = 0;
            oe = 1;
          }
        }

        auto IsPhiOrTwoAddress = [=](Value *V) {
          if (isa<PHINode>(V))
            return true;
          if (auto CI = dyn_cast<CallInst>(V))
            return getTwoAddressOperandNum(CI).hasValue();
          return false;
        };

        // For each incoming:
        for (; oi != oe; ++oi ) {
          auto Incoming = Inst->getOperand(oi);
          // If it is a phi node or two address instruction, push it into the
          // web for processing later.
          if (IsPhiOrTwoAddress(Incoming)) {
            auto IncomingInst = cast<Instruction>(Incoming);
            if (Done.insert(IncomingInst).second)
              Web.push_back(IncomingInst);
          } else if (!isa<Constant>(Incoming)) {
            // For any other inst or arg, see if it has any other use in a phi
            // node or two address inst, and push that into the web.
            for (auto ui = Incoming->use_begin(), ue = Incoming->use_end();
                ui != ue; ++ui) {
              auto User = cast<Instruction>(ui->getUser());
              // Add bitcasts into the web to process their users too
              if (IsPhiOrTwoAddress(User) ||
                  (isa<CastInst>(User) && cast<CastInst>(User)->isNoopCast(DL)))
                if (Done.insert(User).second)
                  Web.push_back(User);
            }
          }
        }
        // Now process each use of the result of the phi node or two address
        // instruction. If the use is in a phi node or is a two address operand,
        // push the user into the web.
        for (auto ui = Inst->use_begin(), ue = Inst->use_end(); ui != ue; ++ui) {
          auto User = cast<Instruction>(ui->getUser());
          if (IsPhiOrTwoAddress(User))
            if (Done.insert(User).second)
              Web.push_back(User);
        }
      }
      LLVM_DEBUG(
        dbgs() << "loadPhiConstants: Web of phi nodes and two address insts:\n";
        for (auto wi = Web.begin(), we = Web.end(); wi != we; ++wi)
          dbgs() << **wi << "\n"
      );
      // Now process the web, ignoring anything other than phi nodes.
      // Gather the distinct constants, and every use for each one in a phi
      // node.
      std::map<Constant *, SmallVector<Use *, 4>> ConstantUses;
      SmallVector<Constant *, 8> DistinctConstants;

      // Fill ConstantUses map
      // Process phis with larger types first to make sure that wider
      // constant goes to ConstantUses map first
      auto WebPhisRange = make_filter_range(
          Web, [](Instruction *I) { return isa<PHINode>(I); });
      SmallVector<Instruction *, 4> WebPhis(WebPhisRange);
      std::sort(WebPhis.begin(), WebPhis.end(),
                [&DL](Instruction *I1, Instruction *I2) {
                  return DL.getTypeSizeInBits(I1->getType()) >
                         DL.getTypeSizeInBits(I2->getType());
                });

      for (auto *Inst : WebPhis) {
        auto *Phi = cast<PHINode>(Inst);
        for (unsigned oi = 0, oe = Phi->getNumIncomingValues(); oi != oe; ++oi) {
          Use *U = &Phi->getOperandUse(oi);
          auto *C = dyn_cast<Constant>(*U);
          if (!C || isa<UndefValue>(C))
            continue;
          // when doing this transform in pattern matching phase
          if (ExcludePredicate) {
            if (C->getType()->getScalarType()->isIntegerTy(1))
              continue;
            if (DL.getTypeSizeInBits(C->getType()) <= 256)
              continue;
            auto IncomingBlock = Phi->getIncomingBlock(oi);
            if (GotoJoin::isBranchingJoinLabelBlock(IncomingBlock))
              continue;
          }

          // Merge uses if constants are bitcastable.
          auto EqualC = llvm::find_if(DistinctConstants, [&C](Constant *C2) {
            return genx::areConstantsEqual(C, C2);
          });
          if (EqualC != DistinctConstants.end())
            C = *EqualC;

          auto Entry = &ConstantUses[C];
          if (!Entry->size())
            DistinctConstants.push_back(C);
          Entry->push_back(U);
        }
      }
      // Handle each distinct constant.
      for (unsigned dci = 0, dce = DistinctConstants.size(); dci != dce; ++dci) {
        Constant *C = DistinctConstants[dci];
        auto Entry = &ConstantUses[C];
        if (Entry->size() != 1) {
          LLVM_DEBUG(
            dbgs() << "multiple use of " << *C << "\n";
            for (unsigned ei = 0, ee = Entry->size(); ei != ee; ++ei)
              dbgs() << *(*Entry)[ei]->getUser() << "\n"
          );
        }
        // Find the closest common dominator of the incoming blocks of all phi
        // uses of the constant. That is where we want to insert the constant
        // load.
        Use *U = (*Entry)[0];
        auto InsertBB = cast<PHINode>(U->getUser())
            ->getIncomingBlock(U->getOperandNo());
        for (unsigned ei = 1, ee = Entry->size(); ei != ee; ++ei) {
          U = (*Entry)[ei];
          auto Phi = cast<PHINode>(U->getUser());
          auto IncomingBB = Phi->getIncomingBlock(U->getOperandNo());
          InsertBB = DT->findNearestCommonDominator(InsertBB, IncomingBB);
        }
        // If that location is an empty split critical edge block, go up to its
        // predecessor (which is also its immediate dominator) if this block is
        // "true" successor of branching simd cf block. In this case we cannot
        // insert anything in current block and have to create partial
        // redundancy.
        IGC_ASSERT(InsertBB);
        auto *InsertTerm = InsertBB->getTerminator();
        auto *SinglePred = InsertBB->getSinglePredecessor();
        if (InsertTerm->getNumSuccessors() == 1 &&
            InsertTerm == &InsertBB->front() && SinglePred &&
            GotoJoin::isBranchingGotoJoinBlock(SinglePred))
          InsertBB = SinglePred;

        // Insert the constant load.
        ConstantLoader CL(C, Subtarget, DL);
        Value *Load = nullptr;
        Instruction *InsertBefore = InsertBB->getTerminator();
        if (!CL.isSimple())
          Load = CL.loadBig(InsertBefore);
        else
          Load = CL.load(InsertBefore);
        Modified = true;
        // Modify the uses.

        SmallDenseMap<Type *, Value *, 4> CastMap;
        // Create cast of specific type of given value or reuse it
        // if exists
        auto CreateOrReuseCast = [&CastMap](Value *V, Type *Ty,
                                            Instruction *InsertBefore) {
          // No cast needed
          if (V->getType() == Ty)
            return V;
          // Assume bitcastable for now
          if (!CastMap.count(Ty))
            CastMap[Ty] =
                CastInst::Create(Instruction::BitCast, V, Ty,
                                 V->getName() + ".cast", InsertBefore);
          return CastMap[Ty];
        };

        for (unsigned ei = 0, ee = Entry->size(); ei != ee; ++ei) {
          auto *U = (*Entry)[ei];
          *U = CreateOrReuseCast(Load, U->get()->getType(), InsertBefore);
        }
        // replace other non-phi uses that are also dominated by the InsertBB
        for (unsigned wi = 0, we = Web.size(); wi != we; ++wi) {
          if (isa<PHINode>(Web[wi]))
            continue;
          auto CI = dyn_cast<CallInst>(Web[wi]);
          if (CI && getTwoAddressOperandNum(CI)) {
            auto oi = *getTwoAddressOperandNum(CI);
            Use *U = &CI->getOperandUse(oi);
            auto *UC = dyn_cast<Constant>(*U);
            if (UC && UC == C) {
              if (CI->getParent() != InsertBB && DT->dominates(InsertBB, CI->getParent()))
                *U = CreateOrReuseCast(Load, U->get()->getType(), InsertBefore);
            }
          }
        }
      }
    }
  }
  return Modified;
}

bool genx::isReplicatedConstantVector(
    const ConstantVector *Orig, const ConstantVector *ReplicateCandidate) {
  IGC_ASSERT(Orig && ReplicateCandidate);
  // First compare for same element type
  if (Orig->getType()->getElementType() !=
      ReplicateCandidate->getType()->getElementType())
    return false;

  unsigned OrigNumElements = Orig->getType()->getNumElements();
  unsigned CandidateNumElements =
      ReplicateCandidate->getType()->getNumElements();

  // Check replicate possibility by size: candidate should be
  // at least larger and it's size is divisible by the size of
  // original vector
  if ((OrigNumElements >= CandidateNumElements) ||
      ((CandidateNumElements % OrigNumElements) != 0))
    return false;

  // Get slices
  unsigned NumReplicates = CandidateNumElements / OrigNumElements;
  SmallVector<Constant *, 4> Slices;
  for (unsigned i = 0; i < NumReplicates; i++)
    Slices.push_back(genx::getConstantSubvector(
        ReplicateCandidate, i * OrigNumElements, OrigNumElements));

  // Compare all slices
  return llvm::all_of(Slices,
                      [Orig](Constant *Slice) { return Slice == Orig; });
}

void ConstantLoader::fixSimple(int OperandIdx) {
  IGC_ASSERT_MESSAGE(User, "user must be provided");
  IGC_ASSERT_MESSAGE(NewC, "no need to fix simple case");
  IGC_ASSERT_MESSAGE(User->getOperand(OperandIdx) == C,
    "wrong arguments: wrong operand index was provided");
  User->setOperand(OperandIdx, NewC);
  C = NewC;
  // indicate that we no longer need fix
  NewC = nullptr;
}

/***********************************************************************
 * ConstantLoader::loadNonSimple : load a non-simple constant
 *
 * Enter:   C = constant to lower if necessary
 *          Inst = instruction it is used in (also used to insert new
 *                 code before)
 *
 * Return:  new instruction
 */
Instruction *ConstantLoader::loadNonSimple(Instruction *Inst) {
  IGC_ASSERT(!isSimple());
  if (!isLegalSize())
    return loadBig(Inst);
  if (PackedFloat) {
    unsigned NumElts =
        cast<IGCLLVM::FixedVectorType>(C->getType())->getNumElements();
    SmallVector<Instruction *, 4> Quads;
    for (unsigned i = 0, e = NumElts; i != e; i += 4) {
      SmallVector<Constant *, 4> Quad;
      for (unsigned j = 0; j != 4 && (i + j) < NumElts; ++j)
        Quad.push_back(C->getAggregateElement(i + j));
      ConstantLoader Packed(ConstantVector::get(Quad), Subtarget, DL);
      Quads.push_back(Packed.load(Inst));
    }
    Value *V = UndefValue::get(C->getType());
    unsigned Offset = 0;
    auto DbgLoc = Inst->getDebugLoc();
    for (auto &Q : Quads) {
      auto *VTy = cast<IGCLLVM::FixedVectorType>(Q->getType());
      Region R(V, &DL);
      R.getSubregion(Offset, VTy->getNumElements());
      V = R.createWrRegion(V, Q, "constant.quad" + Twine(Offset), Inst, DbgLoc);
      Offset += VTy->getNumElements();
    }
    return cast<Instruction>(V);
  }
  if (PackedIntScale) {
    auto PackTy = C->getType()->getScalarType();
    // limit the constant-type to 32-bit because we do not want 64-bit operation
    if (DL.getTypeSizeInBits(PackTy) > 32)
      PackTy = Type::getInt32Ty(Inst->getContext());
    // Load as a packed int vector with scale and/or adjust.
    SmallVector<Constant *, 32> PackedVals;
    for (unsigned
             i = 0,
             e = cast<IGCLLVM::FixedVectorType>(C->getType())->getNumElements();
         i != e; ++i) {
      int64_t Val = 0;
      if (auto CI = dyn_cast<ConstantInt>(C->getAggregateElement(i))) {
        Val = CI->getSExtValue();
        Val -= PackedIntAdjust;
        Val /= PackedIntScale;
      }
      PackedVals.push_back(ConstantInt::get(PackTy, Val, /*isSigned=*/true));
      IGC_ASSERT(cast<ConstantInt>(PackedVals.back())->getSExtValue() >= -8
          && cast<ConstantInt>(PackedVals.back())->getSExtValue() <= 15);
    }

    ConstantLoader Packed(ConstantVector::get(PackedVals), Subtarget, DL);
    auto *LoadPacked = Packed.loadNonPackedIntConst(Inst);
    if (PackedIntScale != 1) {
      auto *SplatVal =
          ConstantInt::get(PackTy, PackedIntScale, /*isSigned=*/true);
      auto *CVTy = cast<IGCLLVM::FixedVectorType>(C->getType());
      auto ElemCount = IGCLLVM::getElementCount(CVTy->getNumElements());
      auto *Op1 = ConstantVector::getSplat(ElemCount, SplatVal);
      LoadPacked = BinaryOperator::Create(Instruction::Mul, LoadPacked, Op1,
                                          "constantscale", Inst);
    }
    if (PackedIntAdjust) {
      auto *SplatVal =
          ConstantInt::get(PackTy, PackedIntAdjust, /*isSigned=*/true);
      auto *CVTy = cast<IGCLLVM::FixedVectorType>(C->getType());
      auto ElemCount = IGCLLVM::getElementCount(CVTy->getNumElements());
      auto *Op1 = ConstantVector::getSplat(ElemCount, SplatVal);
      LoadPacked = BinaryOperator::Create(Instruction::Add, LoadPacked, Op1,
                                          "constantadjust", Inst);
    }
    if (DL.getTypeSizeInBits(PackTy) <
        DL.getTypeSizeInBits(C->getType()->getScalarType())) {
      LoadPacked = CastInst::CreateSExtOrBitCast(LoadPacked, C->getType(),
                                                 "constantsext", Inst);

      bool IsI64 =
          C->getType()->getScalarType() == Type::getInt64Ty(Inst->getContext());
      if (IsI64 && !allowI64Ops()) {
        if (LoadPacked->getOpcode() == Instruction::CastOps::SExt) {
          LoadPacked = genx::emulateI64Operation(&Subtarget, LoadPacked,
                                                 EmulationFlag::RAUWE);
        }
      }
    }
    return LoadPacked;
  }
  if (auto CC = getConsolidatedConstant(C)) {
    // We're loading a vector of byte or short (but not i1). Use int so the
    // instruction does not use so many channels. This may also save it being
    // split by legalization.
    ConstantLoader CCL(CC, Subtarget, DL);
    Instruction *NewInst = nullptr;
    if (CCL.isSimple())
      NewInst = CCL.load(Inst);
    else
      NewInst = CCL.loadNonSimple(Inst);
    NewInst = CastInst::Create(Instruction::BitCast, NewInst, C->getType(),
        "constant", Inst);
    if (AddedInstructions)
      AddedInstructions->push_back(NewInst);
    return NewInst;
  }
  auto *VT = cast<IGCLLVM::FixedVectorType>(C->getType());
  unsigned NumElements = VT->getNumElements();
  SmallVector<Constant *, 32> Elements;
  unsigned UndefBits = 0;
  if (ConstantDataVector *CDV = dyn_cast<ConstantDataVector>(C)) {
    // Gather the elements.
    for (unsigned i = 0; i != NumElements; ++i) {
      Constant *El = CDV->getElementAsConstant(i);
      IGC_ASSERT_MESSAGE(!isa<UndefValue>(El), "CDV element can't be undef");
      Elements.push_back(El);
    }
  } else {
    ConstantVector *CV = cast<ConstantVector>(C);
    // Gather the elements.
    for (unsigned i = 0; i != NumElements; ++i) {
      Constant *El = CV->getOperand(i);
      if (isa<UndefValue>(El))
        UndefBits |= 1 << i;
      Elements.push_back(El);
    }
  }
  unsigned RemainingBits = ~UndefBits
      & ((NumElements == 32 ? 0 : 1 << NumElements) - 1);
  if (!RemainingBits) {
    // All elements are undef. This should have been simplified away earlier,
    // but we need to cope with it in case it was not. Just load the first
    // element.
    RemainingBits = 1;
  }
  Instruction *Result = 0;
  // If it is wider than 8 elements, see if we can load any group of 8 as a
  // packed vector.
  if (NumElements > 8) {
    for (unsigned Idx = 0; Idx < NumElements - 4; Idx += 8) {
      unsigned Size = std::min(8U, NumElements - Idx);
      Constant *SubC = getConstantSubvector(C, Idx, Size);
      if (isa<UndefValue>(SubC))
        continue;
      ConstantLoader SubLoader(SubC, Subtarget, DL);
      if (SubLoader.PackedIntScale == 0 && !SubLoader.isPackedFloatVector())
        continue;
      Region R(C, &DL);
      R.getSubregion(Idx, Size);
      if (SubLoader.isSimple()) {
        Value *SubV = SubC;
        Result = R.createWrConstRegion(
            Result ? (Value *)Result : (Value *)UndefValue::get(C->getType()),
            SubV, "constant.split" + Twine(Idx), Inst, Inst->getDebugLoc());
      } else {
        Value* SubV = SubLoader.loadNonSimple(Inst);
        Result = R.createWrRegion(
            Result ? (Value *)Result : (Value *)UndefValue::get(C->getType()),
            SubV, "constant.split" + Twine(Idx), Inst, Inst->getDebugLoc());
      }
      if (AddedInstructions)
        AddedInstructions->push_back(Result);
      RemainingBits &= ~(255 << Idx);
    }
    if (!RemainingBits)
      return Result;
  }

  // Build the splat sets, that is, the sets of elements of identical value.
  SmallVector<unsigned, 32> SplatSets;
  {
    ValueMap<Constant *, unsigned> SplatSetFinder;
    for (unsigned i = 0; i != NumElements; ++i) {
      Constant *El = Elements[i];
      if (!isa<UndefValue>(El)) {
        std::pair<ValueMap<Constant *, unsigned>::iterator, bool> Created
            = SplatSetFinder.insert(std::pair<Constant *, unsigned>(El,
                  SplatSets.size()));
        if (Created.second) {
          // First time this Constant has been seen.
          SplatSets.push_back(1 << i);
        } else {
          // Add on to existing splat set.
          SplatSets[Created.first->second] |= 1 << i;
        }
      }
    }
  }
  // Remove any splat set with only a single element.
  unsigned NewSize = 0;
  for (unsigned i = 0, e = SplatSets.size(); i != e; ++i) {
    if (countPopulation(SplatSets[i]) >= 2)
      SplatSets[NewSize++] = SplatSets[i];
  }
  SplatSets.resize(NewSize);
  // Determine which elements are suitable for inclusion in a packed vector.
  // FIXME Not implemented yet. For an int vector constant, we need to
  // determine whether the instruction expects the operand to be signed
  // or unsigned.

  // Loop constructing the constant until it is complete.
  do {
    // Find the splat set that will contribute the most elements
    // to the vector, taking into account what elements we can access
    // in a 1D region write. (Initialize BestSplatSetBits so, if no best
    // splat is found, we just do a single element out of RemainingBits.)
    //
    // Note that we are looking for the splat set that sets the most elements,
    // not the one that _usefully_ sets the most elements. For example,
    // Examples/sepia has a constant vector of the form
    // < A, B, C, 0, 0, A, B, C >
    // We have four splat sets {0,5} {1,6} {2,7} {3,4}, each of which
    // has two elements. What we want to do is set one of the A, B or C
    // sets first, rather than the 0s, because region restrictions mean that
    // we can only set such a pair if we do it first. If the loop below were
    // to find the splat set that _usefully_ sets the most elements, all four
    // sets would say "2" and we would arbitrarily pick one of them. But, if
    // we ask each splat set how many elements it sets, even uselessly, then
    // the A, B and C sets say "8" and the 0 set says "2", and we ensure that
    // we do one of the A, B or C sets first.
    // So we end up setting the constant in this order (arbitrarily picking
    // A first):
    //     < A, A, A, A, A, A, A, A >
    //     <          0, 0          >
    //     <    B                   >
    //     <                   B    >
    //     <       C                >
    //     <                      C >
    // giving five wrregion instructions rather than six.
    unsigned BestSplatSetBits = 1 << genx::log2(RemainingBits);
    unsigned BestSplatSetUsefulBits = BestSplatSetBits;
    unsigned BestSplatSetCount = 1;
    Constant *BestSplatSetConst = Elements[genx::log2(RemainingBits)];
    for (unsigned i = 0, e = SplatSets.size(); i != e; ++i) {
      unsigned Bits = getRegionBits(SplatSets[i] & RemainingBits,
          SplatSets[i] | RemainingBits | UndefBits, NumElements);
      unsigned Count = countPopulation(Bits);
      // For this splat set, Bits is a bitmap of the vector elements that
      // we can set in this splat set in a legal 1D region (possibly including
      // elements already set and undef elements), and Count is how many
      // elements that still need setting the region will set.
      if (Count > BestSplatSetCount) {
        BestSplatSetBits = Bits;
        BestSplatSetUsefulBits = Bits & SplatSets[i];
        BestSplatSetCount = Count;
        BestSplatSetConst = Elements[genx::log2(SplatSets[i])];
      }
    }
    // Now BestSplatSetBits is a bitmap of the vector elements to include in
    // the best splat. Set up the splatted constant.
    if (!Result) {
      // For the first time round the loop, just splat the whole vector,
      // whatever BestSplatBits says.
      Result = loadConstant(
          ConstantVector::getSplat(IGCLLVM::getElementCount(NumElements),
                                   BestSplatSetConst),
          Inst, Subtarget, DL, AddedInstructions);
      Result->setDebugLoc(Inst->getDebugLoc());
    } else {
      // Not the first time round the loop. Set up the splatted subvector,
      // and write it as a region.
      Region R(BestSplatSetBits,
               DL.getTypeSizeInBits(VT->getElementType()) / genx::ByteBits);
      Constant *NewConst = ConstantVector::getSplat(
          IGCLLVM::getElementCount(R.NumElements), BestSplatSetConst);
      Result = R.createWrConstRegion(Result, NewConst, "constant", Inst,
                                     Inst->getDebugLoc());
      if (AddedInstructions)
        AddedInstructions->push_back(Result);
    }
    RemainingBits &= ~BestSplatSetUsefulBits;
  } while (RemainingBits);
  return Result;
}

/***********************************************************************
 * getRegionBits : determine which vector elements we can set with a
 *                 1D region
 *
 * Enter:   NeededBits = bits for vector elements we need to set
 *          OptionalBits = bits for vector elements we could set
 *          VecWidth = number of elements in vector
 *
 * Return:  bits for vector elements to set as a legal 1D region,
 *          maximizing how many of NeededBits are set
 */
unsigned ConstantLoader::getRegionBits(unsigned NeededBits,
    unsigned OptionalBits, unsigned VecWidth) {
  if (!NeededBits)
    return 0;
  // Get the first and last element numbers in NeededBits.
  unsigned FirstNeeded = countTrailingZeros(NeededBits, ZB_Undefined);
  unsigned LastNeeded = 31 - countLeadingZeros((uint32_t)NeededBits, ZB_Undefined);
  // Set the max width to the min size including both those elements
  // rounded up to the next power of two.
  unsigned MaxWidth = LastNeeded - FirstNeeded + 1;
  unsigned LogMaxWidth = genx::log2(MaxWidth);
  if (MaxWidth != 1U << LogMaxWidth) {
    ++LogMaxWidth;
    MaxWidth = 1U << LogMaxWidth;
  }
  // Special case NeededBits only having one element.
  if (LogMaxWidth == 0)
    return NeededBits;
  // Now find the best region.
  unsigned BestBits = 0;
  unsigned BestCount = 0;
  // Try each stride.
  static const unsigned StrideBitsTable[] = { 0xffffffffU, 0x55555555U, 0x11111111U };
  for (unsigned LogStride = 0, Stride = 1;
      LogStride <= 2U && LogStride < LogMaxWidth;
      ++LogStride, Stride <<= 1U) {
    // Try each width (not including 1).
    for (unsigned Width = 1U << (LogMaxWidth - LogStride); Width > 1; Width >>= 1) {
      if (Width <= BestCount)
        break;
      // Try each start index.
      for (unsigned Idx = 0; Idx + (Width - 1) * Stride < VecWidth; ++Idx) {
        if (Idx + Width > VecWidth)
          break;
        // Calculate which indexes the region will set.
        unsigned Bits = StrideBitsTable[LogStride];
        if (Width != 32)
          Bits &= (1 << Width) - 1;
        Bits <<= Idx;
        // See if it sets any elements that we are not allowed to set.
        if (Bits & ~(NeededBits | OptionalBits))
          continue;
        // See if it sets all of NeededBits.
        if ((Bits & NeededBits) == NeededBits)
          return Bits;
        // See if it is the best one we have seen so far.
        unsigned Count = countPopulation(Bits & NeededBits);
        if (Count > BestCount) {
          BestCount = Count;
          BestBits = Bits;
          if (BestCount == Width)
            break;
        }
      }
    }
  }
  if (!BestCount) {
    // We could not find any region that includes more than one of NeededBits.
    // Just do a single element.
    return 1 << genx::log2(NeededBits);
  }
  return BestBits;
}

Instruction *ConstantLoader::loadSplatConstant(Instruction *InsertPos) {
  // Skip scalar types, vector type with just one element, or boolean vector.
  auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(C->getType());
  if (!VTy ||
      VTy->getNumElements() == 1 ||
      VTy->getScalarType()->isIntegerTy(1))
    return nullptr;
  // Skip non-splat vector.
  Constant *C1 = IGCLLVM::Constant::getSplatValue(C, /* AllowUndefs */ true);
  if (!C1)
    return nullptr;
  // Create <1 x T> constant and broadcast it through rdregion.
  Constant *CV = ConstantVector::get(C1);
  // Load that scalar constant first.
  ConstantLoader L(CV, Subtarget, DL);
  Value *V = L.load(InsertPos);
  // Broadcast through rdregion.
  Region R(V, &DL);
  R.Width = R.NumElements = VTy->getNumElements();
  R.Stride = 0;
  R.VStride = 0;
  R.Offset = 0;
  Instruction *NewInst = R.createRdRegion(V, ".constsplat", InsertPos, DebugLoc());
  if (AddedInstructions)
    AddedInstructions->push_back(NewInst);
  return NewInst;
}

/***********************************************************************
 * ConstantLoader::load : insert instruction to load a constant
 *
 * We use llvm.genx.constant, rather than bitcast, because CSE has a habit
 * of propagating a constant bitcast back into our operand that is not
 * allowed to be constant.
 *
 * Enter:   C = constant to load
 *          InsertBefore = insert new instruction before here
 *
 * Return:  new instruction
 */
Instruction *ConstantLoader::load(Instruction *InsertBefore) {
  IGC_ASSERT(isSimple());
  // Do not splat load on byte data as HW does not support byte imm source.
  if (!C->getType()->getScalarType()->isIntegerTy(8))
    if (auto NewInst = loadSplatConstant(InsertBefore))
      return NewInst;

  if (!PackedFloat && !PackedIntScale && !isa<UndefValue>(C)) { // not packed int constant or undef
    if (auto CC = getConsolidatedConstant(C)) {
      // We're loading a vector of byte or short (but not i1). Use int so the
      // instruction does not use so many channels. This may also save it being
      // split by legalization.
      Instruction *NewInst =
          loadConstant(CC, InsertBefore, Subtarget, DL, AddedInstructions);
      NewInst = CastInst::Create(Instruction::BitCast, NewInst, C->getType(),
          "constant", InsertBefore);
      if (AddedInstructions)
        AddedInstructions->push_back(NewInst);
      return NewInst;
    }
  }

  // Load the constant as normal.
  Value *Args[] = { C };   // Args to new llvm.genx.constant
  Type *OverloadedTypes[] = { C->getType() };
  GenXIntrinsic::ID IntrinsicID = GenXIntrinsic::genx_constanti;
  if (C->getType()->isFPOrFPVectorTy())
    IntrinsicID = GenXIntrinsic::genx_constantf;
  else if (C->getType()->getScalarType()->isIntegerTy(1))
    IntrinsicID = GenXIntrinsic::genx_constantpred;
  Module *M = InsertBefore->getParent()->getParent()->getParent();
  Function *Decl = GenXIntrinsic::getGenXDeclaration(M, IntrinsicID, OverloadedTypes);
  Instruction *NewInst = CallInst::Create(Decl, Args, "constant", InsertBefore);
  if (AddedInstructions)
    AddedInstructions->push_back(NewInst);
  return NewInst;
}

/***********************************************************************
 * ConstantLoader::loadNonPackedIntConst : insert instruction to load a constant
 *                               that are not packed because they have width > 8.
 *
 * Enter:   C = constant to load
 *          InsertBefore = insert new instruction before here
 *
 * Return:  new instruction
 */
Instruction *ConstantLoader::loadNonPackedIntConst(Instruction *InsertBefore) {
  auto *CTy = cast<IGCLLVM::FixedVectorType>(C->getType());
  IGC_ASSERT(CTy->isIntOrIntVectorTy());
  // Simple vectors are already the correct size <= 8, process common load
  if (isSimple())
    return load(InsertBefore);

  unsigned NumElements = CTy->getNumElements();
  Instruction *Result = nullptr;
  for (unsigned Idx = 0; Idx != NumElements;) {
    unsigned Size =
        std::min(PowerOf2Floor(NumElements - Idx), (uint64_t)ImmIntVec::Width);
    Constant *SubC = getConstantSubvector(C, Idx, Size);
    Value *SubV = SubC;
    ConstantLoader SubLoader(SubC, Subtarget, DL);
    SubV = SubLoader.load(InsertBefore);

    Region R(C, &DL);
    R.getSubregion(Idx, Size);
    Result = R.createWrRegion(
        Result ? (Value *)Result : (Value *)UndefValue::get(C->getType()), SubV,
        "constant.split" + Twine(Idx), InsertBefore, DebugLoc());
    Idx += Size;
  }
  return Result;
}
/***********************************************************************
 * ConstantLoader::loadBig : insert instruction to load a constant that might
 *      be illegally sized
 */
Instruction *ConstantLoader::loadBig(Instruction *InsertBefore) {
  if (isLegalSize() || isa<UndefValue>(C)) {
    // Does not need legalizing.
    if (!isSimple())
      return loadNonSimple(InsertBefore);
    return load(InsertBefore);
  }
  IGC_ASSERT_MESSAGE(!C->getType()->getScalarType()->isIntegerTy(1),
    "not expecting predicate in here");
  if (Constant *Consolidated = getConsolidatedConstant(C)) {
    // Load as a consolidated constant, then bitcast to the correct type.
    auto Load =
        ConstantLoader(Consolidated, Subtarget, DL, nullptr, AddedInstructions)
            .loadBig(InsertBefore);
    IGC_ASSERT(Load);
    Load = CastInst::Create(Instruction::BitCast, Load, C->getType(),
        Load->getName() + ".cast", InsertBefore);
    if (AddedInstructions)
      AddedInstructions->push_back(Load);
    return Load;
  }
  auto VT = cast<IGCLLVM::FixedVectorType>(C->getType());
  const unsigned NumElements = VT->getNumElements();
  const unsigned GRFWidthInBits = Subtarget.getGRFByteSize() * genx::ByteBits;
  const unsigned ElementBits = DL.getTypeSizeInBits(VT->getElementType());
  unsigned MaxSize = 2 * GRFWidthInBits / ElementBits;
  MaxSize = std::min(MaxSize, 32U);
  Instruction *Result = nullptr;
  for (unsigned Idx = 0; Idx != NumElements; ) {
    unsigned Size = std::min(1U << genx::log2(NumElements - Idx), MaxSize);
    // Load this subvector constant if necessary, and insert into the overall
    // value with wrregion.
    Constant *SubC = getConstantSubvector(C, Idx, Size);
    Value *SubV = SubC;
    ConstantLoader SubLoader(SubC, Subtarget, DL);
    if (!SubLoader.isSimple())
      SubV = SubLoader.loadNonSimple(InsertBefore);
    Region R(C, &DL);
    R.getSubregion(Idx, Size);
    Result = R.createWrRegion(
        Result ? (Value *)Result : (Value *)UndefValue::get(C->getType()), SubV,
        "constant.split" + Twine(Idx), InsertBefore, DebugLoc());
    if (AddedInstructions)
      AddedInstructions->push_back(Result);
    Idx += Size;
  }
  return Result;
}

/***********************************************************************
 * ConstantLoader::isLegalSize : detect if a constant is a legal size
 */
bool ConstantLoader::isLegalSize() const {
  auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(C->getType());
  if (!VT)
    return true;
  const int NumBits = DL.getTypeSizeInBits(VT);
  if (!llvm::isPowerOf2_32(NumBits))
    return false;
  const int GRFSizeInBits = Subtarget.getGRFByteSize() * genx::ByteBits;
  if (NumBits > GRFSizeInBits * 2)
    return false; // bigger than 2 GRFs
  if (VT->getNumElements() > 32)
    return false; // 64 bytes not allowed
  return true;
}

/***********************************************************************
 * ConstantLoader::isBigSimple : detect if a constant is either simple,
 *    or would be simple after being split into legal sizes
 *
 * This does not do a thorough check so it misses some cases of a constant
 * that would split into simple constants.
 */
bool ConstantLoader::isBigSimple() const {
  IGC_ASSERT_MESSAGE(!needFixingSimple(),
    "simple case shall be fixed first before this call");
  if (isa<UndefValue>(C))
    return true; // undef is simple
  auto VT = dyn_cast<VectorType>(C->getType());
  if (!VT)
    return true; // scalar always simple
  if (IGCLLVM::Constant::getSplatValue(C, /* AllowUndefs */ true))
    return true; // splat constant always simple
  if (DL.getTypeSizeInBits(VT->getElementType()) == 1)
    return true; // predicate constant always simple
  return false;
}

/***********************************************************************
 * ConstantLoader::isSimple : detect if a constant is "simple"
 *
 * A simple constant is one we know can be a constant operand in an instruction.
 */
bool ConstantLoader::isSimple() const {
  IGC_ASSERT_MESSAGE(!needFixingSimple(),
    "simple case shall be fixed first before this call");
  if (isa<UndefValue>(C))
    return true; // undef is simple (and generates no vISA code)
  if (C->getType()->getScalarType()->isIntegerTy(1) && C->isAllOnesValue())
    return true; // all 1s predicate is simple
  if(User && User->isBinaryOp())
    if (isa<VectorType>(C->getType()))
      if (auto splat = C->getSplatValue())
        if (splat->isZeroValue())
          return true;
  if (!isLegalSize())
    return false; // Simple constant must be legally sized
  if (isBigSimple())
    return true; // a big simple constant that is legally sized is simple
  if (isPackedIntVector())
    return true;
  if (isPackedFloatVector())
    return true;
  return false;
}

bool ConstantLoader::allowI64Ops() const {
  if (!Subtarget.hasLongLong())
    return false;
  if (Subtarget.partialI64Emulation())
    return false;
  return true;
}
/***********************************************************************
 * ConstantLoader::isPackedIntVector : check for a packed int vector
 *    (having already done the analysis in the ConstantLoader constructor)
 */
bool ConstantLoader::isPackedIntVector() const {
  // Check for a packed int vector. Either the element type must be i16, or
  // the user (instruction using the constant) must be genx.constanti or
  // wrregion or wrconstregion. Not allowed if the user is a logic op.
  if (cast<IGCLLVM::FixedVectorType>(C->getType())->getNumElements() >
      ImmIntVec::Width)
    return false; // wrong width for packed vector
  if (PackedIntScale == 1 && (PackedIntAdjust == 0 || PackedIntAdjust == -8)) {
    if (!User)
      return true; // user not specified -- assume it is a mov, so wrong element
                   //  size is allowed
    if (!C->getType()->getScalarType()->isIntegerTy(16)
        && GenXIntrinsic::getGenXIntrinsicID(User) != GenXIntrinsic::genx_constanti
        && !GenXIntrinsic::isWrRegion(User))
      return false; // wrong element size when it is not a mov
    switch (User->getOpcode()) {
      case Instruction::And:
      case Instruction::Or:
      case Instruction::Xor:
        return false; // disallow packed vector in logic op
      default:
        break;
    }
    return true;
  }
  return false;
}

/***********************************************************************
 * ConstantLoader::isPackedFloatVector : check for a packed float vector
 *    (having already done the analysis in the ConstantLoader constructor)
 */
bool ConstantLoader::isPackedFloatVector() const {
  auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(C->getType());
  if (!VT)
    return false;
  if (VT->getNumElements() > 4)
    return false;
  return PackedFloat;
}

/***********************************************************************
 * ConstantLoader::getConsolidatedConstant : get the consolidated constant
 *        for the given constant
 *
 * A "consolidated constant" is one where a vector of byte or short is
 * turned into the equivalent (as if by bitcast) vector of int.
 */
Constant *ConstantLoader::getConsolidatedConstant(Constant *C) {
  if (isa<UndefValue>(C))
    return nullptr;
  auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(C->getType());
  if (!VT)
    return nullptr;
  const unsigned BytesPerElement =
      DL.getTypeSizeInBits(VT->getElementType()) / genx::ByteBits;
  const unsigned NumElements = VT->getNumElements();
  if (!BytesPerElement)
    return nullptr; // vector of i1
  if (BytesPerElement >= 4)
    return nullptr; // already vector of i32/i64/float/double
  if (NumElements * BytesPerElement & 3)
    return nullptr; // not a multiple of 4 bytes long
  // We're loading a vector of byte or short (but not i1). Use int so the
  // instruction does not use so many channels. This may also save it being
  // split by legalization.
  unsigned Compaction = BytesPerElement == 1 ? 4 : 2;
  unsigned Mask = BytesPerElement == 1 ? 0xff : 0xffff;
  SmallVector<Constant *, 8> Elements;
  Type *I32Ty = Type::getInt32Ty(C->getContext());
  for (unsigned i = 0; i != NumElements; i += Compaction) {
    unsigned Val = 0;
    bool IsUndef = true;
    for (unsigned j = 0; j != Compaction; ++j) {
      unsigned Bits = 0;
      Constant *El = C->getAggregateElement(i + j);
      // We assume that anything that is not ConstantInt is undefined. That
      // can include a constant expression with an undefined value in the
      // middle.
      if (auto CI = dyn_cast<ConstantInt>(El)) {
        Bits = CI->getSExtValue();
        IsUndef = false;
      }
      else if (auto CI = dyn_cast<ConstantFP>(El)) {
        APFloat V = CI->getValueAPF();
        Bits = V.bitcastToAPInt().getZExtValue();
        IsUndef = false;
      }
      Val |= (Bits & Mask) << (j * BytesPerElement * 8);
    }
    if (IsUndef)
      Elements.push_back(UndefValue::get(I32Ty));
    else
      Elements.push_back(ConstantInt::get(I32Ty, Val));
  }
  // Construct the constant with i32 element type.
  return ConstantVector::get(Elements);
}

/***********************************************************************
 * ConstantLoader::analyze : analyze a constant value
 *
 * This analyzes whether a constant of no more than the right vector width
 * (integer 8 or fp 4) can be loaded as a packed vector, possibly scaled
 * and adjusted.
 */
void ConstantLoader::analyze() {
  auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(C->getType());
  if (!VT)
    return;
  if (C->getSplatValue())
    return; // don't analyze if already a splat
  unsigned NumElements = VT->getNumElements();
  if (VT->getElementType()->isIntegerTy()) {
    unsigned MaxSize = 2 * Subtarget.getGRFByteSize(); // element type is boolean
    if (!VT->getElementType()->isIntegerTy(1)) {
      unsigned ElmSz = VT->getScalarSizeInBits() / genx::ByteBits;
      MaxSize = 2 * Subtarget.getGRFByteSize() / ElmSz;
    }
    if (NumElements <= MaxSize)
      analyzeForPackedInt(NumElements);
  } else if (NumElements <= 8 && VT->getElementType()->isFloatingPointTy())
    analyzeForPackedFloat(NumElements);
}

void ConstantLoader::analyzeForPackedInt(unsigned NumElements) {
  // Get element values.
  int64_t Min = INT64_MAX;
  int64_t Max = INT64_MIN;
  SmallVector<int64_t, 32> Elements;
  Constant *SomeDefinedElement = nullptr;
  for (unsigned i = 0; i != NumElements; ++i) {
    auto El = C->getAggregateElement(i);
    if (isa<UndefValue>(El))
      continue;
    SomeDefinedElement = El;
    int64_t Element = cast<ConstantInt>(El)->getSExtValue();
    Elements.push_back(Element);
    Min = std::min(Min, Element);
    Max = std::max(Max, Element);
  }
  if (Elements.empty()) {
    // Constant is undef.
    IGC_ASSERT_MESSAGE(C == UndefValue::get(C->getType()),
      "constant consists only of undef elements only if it's undef itself");
    return;
  }
  if (Elements.size() == 1) {
    // if we don't have an immediate user - do not create new constant
    // (constant materilization expects that NewC is cleared)
    if (!User)
      return;
    // All but one element undef. If num elements equal 8
    // then turn it into a splat constant
    if (NumElements != ImmIntVec::Width)
      return;
    NewC = ConstantVector::getSplat(IGCLLVM::getElementCount(NumElements),
                                    SomeDefinedElement);
    return;
  }
  int64_t ResArith;
  if (IGCLLVM::SubOverflow(Max, Min, ResArith))
    return;
  if (ResArith <= ImmIntVec::MaxUInt) {
    if (Min >= ImmIntVec::MinUInt && Max <= ImmIntVec::MaxUInt) {
      // Values all in the range [MinUInt..MaxUInt]. We can do this with a packed
      // unsigned int with no extra scaling or adjustment.
      PackedIntScale = 1;
      PackedIntAdjust = 0;
      PackedIntMax = Max;
      return;
    }
    if (Min >= ImmIntVec::MinSInt && Max <= ImmIntVec::MaxSInt) {
      // Values all in the range [MinSInt..MaxSInt]. We can do this with a packed
      // unsigned int with no extra scaling or adjustment.
      PackedIntScale = 1;
      PackedIntAdjust = -8;
      PackedIntMax = Max + 8;
      return;
    }
    // Values all in the range [Min..Min+MaxUInt]. We can do this
    // with a packed int with an adjustment.
    PackedIntScale = 1;
    PackedIntAdjust = Min;
    PackedIntMax = Max - Min;
    return;
  }
  // Get unique absolute differences, so we can detect if we have a valid
  // packed int vector that is then scaled and has a splatted constant
  // added/subtracted.
  SmallVector<uint64_t, 31> Diffs;
  SmallSet<uint64_t, 31> DiffsSet;
  for (unsigned i = 0, e = Elements.size() - 1; i != e; ++i) {
    int64_t Diff;
    if (IGCLLVM::SubOverflow(Elements[i + 1], Elements[i], Diff))
      return;
    if (!Diff)
      continue;
    uint64_t AbsDiff = std::abs(Diff);
    if (AbsDiff > UINT_MAX)
      return;
    if (DiffsSet.insert(AbsDiff).second)
      Diffs.push_back(AbsDiff);
  }
  IGC_ASSERT_MESSAGE(!Diffs.empty(), "not expecting splatted constant");
  // Calculate the GCD (greatest common divisor) of the diffs
  uint64_t GCD = Diffs[0];
  if (Diffs.size() > 1) {
    for(unsigned i = 1; i < Diffs.size(); i++)
      GCD = GreatestCommonDivisor64(GCD, Diffs[i]);
  }
  // Scale should fit in signed integer
  if (GCD > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
    return;
  int64_t CurScale = static_cast<int64_t>(GCD);
  if (!IGCLLVM::MulOverflow(CurScale, static_cast<int64_t>(ImmIntVec::MaxUInt), ResArith) &&
      (Max - Min) > ResArith)
    return; // range of values too big
  PackedIntScale = CurScale;
  PackedIntMax = ImmIntVec::MaxUInt;
  // Special case adjust of 0 or -8 as then we can save doing an adjust at all
  // by using unsigned or signed packed vector respectively.
  if (!(Min % CurScale)) {
    if (Min >= ImmIntVec::MinUInt &&
        (!IGCLLVM::MulOverflow(CurScale, static_cast<int64_t>(ImmIntVec::MaxUInt), ResArith) &&
         Max <= ResArith)) {
      PackedIntAdjust = ImmIntVec::MinUInt;
      return;
    }
    if ((!IGCLLVM::MulOverflow(CurScale, static_cast<int64_t>(ImmIntVec::MinSInt), ResArith) &&
         Min >= ResArith) &&
        (!IGCLLVM::MulOverflow(CurScale, static_cast<int64_t>(ImmIntVec::MaxSInt), ResArith) &&
         Max <= ResArith)) {
      PackedIntAdjust = Min;
      PackedIntMax = ImmIntVec::MaxSInt;
      return;
    }
    // Special case all pre-scaled values being in [-15,0] as we can do that
    // by negating the scale and not needing to adjust.
    if ((!IGCLLVM::MulOverflow(CurScale, static_cast<int64_t>(-ImmIntVec::MaxUInt), ResArith) &&
         Min >= ResArith) &&
        Max <= -ImmIntVec::MinUInt) {
      PackedIntAdjust = ImmIntVec::MinUInt;
      PackedIntScale = -PackedIntScale;
      return;
    }
  }
  PackedIntAdjust = Min;
}

static bool is8bitPackedFloat(float f) {
  union {
    float f;
    unsigned u;
  } u;

  u.f = f;
  unsigned Exp = (u.u >> 23) & 0xFF;
  unsigned Frac = u.u & 0x7FFFFF;
  if (Exp == 0 && Frac == 0)
    return true;
  if (Exp < 124 || Exp > 131)
    return false;
  if ((Frac & 0x780000) != Frac)
    return false;
  Frac >>= 19;
  if (Exp == 124 && Frac == 0)
    return false;
  return true;
}

void ConstantLoader::analyzeForPackedFloat(unsigned NumElements) {
  if (!Subtarget.hasPackedFloat())
    return;

  for (unsigned i = 0; i != NumElements; ++i) {
    auto Elt = C->getAggregateElement(i);
    if (isa<UndefValue>(Elt))
      continue;
    ConstantFP *CFP = dyn_cast<ConstantFP>(Elt);
    // Bail out if any element cannot be analyzed.
    if (!CFP)
      return;
    const APFloat &FP = CFP->getValueAPF();
    // Bail out if it's not supported.
    // TODO: Only support single precision so far.
    if (&FP.getSemantics() != &APFloat::IEEEsingle())
      return;
    // Bail out if it's not finite.
    if (!FP.isFinite())
      return;
    // Check if it could be represented in 8-bit packed float.
    if (!is8bitPackedFloat(FP.convertToFloat()))
      return;
  }
  PackedFloat = true;
}
