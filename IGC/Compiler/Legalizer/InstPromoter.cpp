/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "TypeLegalizer.h"
#include "InstPromoter.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/Debug.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/Local.h"
#include <llvm/IR/Intrinsics.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "Probe/Assertion.h"

#define DEBUG_TYPE "type-legalizer"

using namespace llvm;
using namespace IGC::Legalizer;

bool InstPromoter::promote(Instruction *I) {
  IRB->SetInsertPoint(&(*std::next(BasicBlock::iterator(I))));
  IRB->SetCurrentDebugLocation(I->getDebugLoc());
  Promoted = nullptr;

  if (!visit(*I))
    return false;

  if (Promoted) {
    TL->setLegalizedValues(I, Promoted);
    // In the case where we've only legalized the arguments to the instruction,
    // we can go ahead and replace it with the new promoted instruction since
    // the return types are the same.
    if (I->getType() == Promoted->getType()) {
      I->replaceAllUsesWith(Promoted);
    } else {
      // Need copy debug information for new legalized instruction
      replaceAllDbgUsesWith(*I, *Promoted, *I, TL->getDominatorTree());
    }
  }

  return true;
}

Value *InstPromoter::getSinglePromotedValueIfExist(Value *OriginalValue) {
  if (TL->ValueMap.find(OriginalValue) != TL->ValueMap.end()) {
    const ValueSeq LegalizedValues = TL->ValueMap[OriginalValue];
    // Instruction should be promoted only by one instruction, not by combination
    if (LegalizedValues.size() == 1)
      return (*LegalizedValues.begin());
  }
  return nullptr;
}

Type *InstPromoter::getSinglePromotedTypeIfExist(Type *OriginalType) {
  if (TL->TypeMap.find(OriginalType) != TL->TypeMap.end()) {
    const TypeSeq LegalizedTypes = TL->TypeMap[OriginalType];
    // Instruction should be promoted only by one instruction, not by combination
    if (LegalizedTypes.size() == 1)
      return (*LegalizedTypes.begin());
  }
  return nullptr;
}

// By default, capture all missing instructions!
bool InstPromoter::visitInstruction(Instruction &I) {
  LLVM_DEBUG(dbgs() << "PROMOTE: " << I << '\n');
  IGC_ASSERT_EXIT_MESSAGE(0, "UNKNOWN INSTRUCTION IS BEING PROMOTED!");
  return false;
}

/// Terminator instructions
///

bool InstPromoter::visitTerminatorInst(IGCLLVM::TerminatorInst &I) {
  // All terminators are handled specially.
  return false;
}

bool InstPromoter::visitSelectInst(SelectInst &I) {

  ValueSeq *Ops0, *Ops1;
  std::tie(Ops0, std::ignore) = TL->getLegalizedValues(I.getOperand(1), true);
  IGC_ASSERT(Ops0 != nullptr && Ops0->size() == 1);
  // we should get value from Ops0 here, as next getLegalizedValues call may grow ValueMap object
  // when inserting new pair with ValueMap.insert(e.g.when ValueMap.NumBuckets grows from 64 to 128)
  // and previously received ValueSeq objects will become invalid.
  Value *LHS = Ops0->front();

  std::tie(Ops1, std::ignore) = TL->getLegalizedValues(I.getOperand(2), true);
  IGC_ASSERT(Ops1 != nullptr && Ops1->size() == 1);
  Value *RHS = Ops1->front();

  Promoted = IRB->CreateSelect(I.getOperand(0), LHS, RHS, Twine(I.getName(), getSuffix()));

  return true;
}

bool InstPromoter::visitICmpInst(ICmpInst &I) {
  bool isSigned = I.isSigned();

  ValueSeq *Ops0, *Ops1;
  std::tie(Ops0, std::ignore) = TL->getLegalizedValues(I.getOperand(0), isSigned);
  IGC_ASSERT(Ops0 != nullptr && Ops0->size() == 1);
  // we should get value from Ops0 here, as next getLegalizedValues call may grow ValueMap object
  // when inserting new pair with ValueMap.insert(e.g.when ValueMap.NumBuckets grows from 64 to 128)
  // and previously received ValueSeq objects will become invalid.
  Value *LHS = Ops0->front();

  std::tie(Ops1, std::ignore) = TL->getLegalizedValues(I.getOperand(1), isSigned);
  IGC_ASSERT(Ops1 != nullptr && Ops1->size() == 1);
  Value *RHS = Ops1->front();

  unsigned initialWidth = I.getOperand(0)->getType()->getIntegerBitWidth();
  unsigned finalWidth = LHS->getType()->getIntegerBitWidth();

  if (!isSigned) {
    APInt M = APInt::getLowBitsSet(finalWidth, initialWidth);
    auto *MaskC = ConstantInt::get(LHS->getType(), M);
    LHS = IRB->CreateAnd(LHS, MaskC);
    RHS = IRB->CreateAnd(RHS, MaskC);
  } else {
    IGC_ASSERT(finalWidth >= initialWidth);

    unsigned shiftAmt = finalWidth - initialWidth;

    LHS = IRB->CreateShl(LHS, shiftAmt);
    LHS = IRB->CreateAShr(LHS, shiftAmt);
    RHS = IRB->CreateShl(RHS, shiftAmt);
    RHS = IRB->CreateAShr(RHS, shiftAmt);
  }

  Promoted = IRB->CreateICmp(I.getPredicate(), LHS, RHS, Twine(I.getName(), getSuffix()));

  return true;
}

/// Standard binary operators
///

bool InstPromoter::visitBinaryOperator(BinaryOperator &I) {
  ValueSeq *Ops0, *Ops1;
  std::tie(Ops0, std::ignore) = TL->getLegalizedValues(I.getOperand(0));
  IGC_ASSERT(Ops0 != nullptr && Ops0->size() == 1);
  // we should get value from Ops0 here, as next getLegalizedValues call may grow ValueMap object
  // when inserting new pair with ValueMap.insert(e.g.when ValueMap.NumBuckets grows from 64 to 128)
  // and previously received ValueSeq objects will become invalid.
  Value *LHS = Ops0->front();

  std::tie(Ops1, std::ignore) = TL->getLegalizedValues(I.getOperand(1));
  IGC_ASSERT(Ops1 != nullptr && Ops1->size() == 1);
  Value *RHS = Ops1->front();

  switch (I.getOpcode()) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
    break;
  case Instruction::UDiv:
  case Instruction::URem:
    std::tie(LHS, RHS) = TL->zext(LHS, RHS, I.getType());
    break;
  case Instruction::SDiv:
  case Instruction::SRem:
    std::tie(LHS, RHS) = TL->sext(LHS, RHS, I.getType());
    break;
  case Instruction::Shl:
    RHS = TL->zext(RHS, I.getType());
    break;
  case Instruction::LShr:
    std::tie(LHS, RHS) = TL->zext(LHS, RHS, I.getType());
    break;
  case Instruction::AShr:
    LHS = TL->sext(LHS, I.getType());
    RHS = TL->zext(RHS, I.getType());
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "UNKNOWN BINARY OPERATOR IS BEING PROMOTED!");
  }

  Promoted = TL->createBinOpAsGiven(&I, LHS, RHS, Twine(I.getName(), getSuffix()));

  return true;
}

/// Memory operators
///

bool InstPromoter::visitAllocaInst(AllocaInst &I) {
  Type *OrigTy = I.getAllocatedType();
  TypeSeq *TySeq = TL->getPromotedTypeSeq(OrigTy);
  IGC_ASSERT(TySeq != nullptr);

  Type *PromotedTy = TySeq->front();

  AllocaInst *PromotedVal = IRB->CreateAlloca(PromotedTy);
  PromotedVal->setAlignment(IGCLLVM::getAlign(I));
  PromotedVal->setName(Twine(I.getName(), ".promotedAlloca"));

  Promoted = PromotedVal;
  return true;
}

bool InstPromoter::visitLoadInst(LoadInst &I) {
  Value *OldPtr = I.getPointerOperand();

  // Check if Load operand was legalized before
  if (Value *LegalizedNewPtr = getSinglePromotedValueIfExist(OldPtr)) {
    Type *NewType = getSinglePromotedTypeIfExist(I.getType());
    IGC_ASSERT(NewType);
    LoadInst *NewLoad = IRB->CreateLoad(NewType, LegalizedNewPtr, Twine(I.getName(), ".promotedLoad"));
    NewLoad->setAlignment(IGCLLVM::getAlign(I));
    Promoted = NewLoad;
    return true;
  }

  Type *OrigTy = I.getType();

  TypeSeq *TySeq;
  std::tie(TySeq, std::ignore) = TL->getLegalizedTypes(OrigTy);
  IGC_ASSERT(TySeq != nullptr && TySeq->size() == 1);

  unsigned AS = I.getPointerAddressSpace();

  Type *PromotedTy = TySeq->front();

  Value *NewBasePtr = IRB->CreatePointerCast(OldPtr, IRB->getInt8PtrTy(AS), Twine(OldPtr->getName(), ".ptrcast"));

  // Different from promotion of regular instructions, such as 'add', promotion
  // of load is required to split the original load into small ones and
  // concatenate them together, e.g. i56 needs splitting into loads of i32,
  // i16, and i8. It's because, without alignment checking, out-of-bound load
  // may be generated if the promoted type is used directly.

  Value *PromotedVal = Constant::getNullValue(PromotedTy);
  unsigned Off = 0;
  unsigned Part = 0;
  for (unsigned TotalLoadBits = TL->getTypeStoreSizeInBits(OrigTy), ActualLoadBits = 0; TotalLoadBits != 0;
       TotalLoadBits -= ActualLoadBits) {
    // Get the largest integer type but not bigger than total load bits.
    ActualLoadBits = TL->getLargestLegalIntTypeSize(TotalLoadBits);

    Type *NewTy = TL->getIntNTy(ActualLoadBits);
    Type *NewPtrTy = PointerType::get(NewTy, AS);

    Value *NewPtr = TL->getPointerToElt(NewBasePtr, Off, NewPtrTy, Twine(NewBasePtr->getName(), ".off") + Twine(Off));

    LoadInst *NewLd = IRB->CreateLoad(NewTy, NewPtr, Twine(I.getName(), getSuffix()) + Twine(Part));
    TL->dupMemoryAttribute(NewLd, &I, Off);

    Value *NewVal = IRB->CreateZExt(NewLd, PromotedTy, Twine(NewLd->getName(), ".zext"));
    NewVal = TL->shl(NewVal, Off << 3);
    PromotedVal = IRB->CreateOr(PromotedVal, NewVal, Twine(NewVal->getName(), ".concat"));

    IGC_ASSERT_MESSAGE((ActualLoadBits & 0x7) == 0, "LEGAL INTEGER TYPE IS NOT BYTE ADDRESSABLE!");
    Off += ActualLoadBits >> 3;
    ++Part;
  }

  Promoted = PromotedVal;
  return true;
}

bool InstPromoter::visitStoreInst(StoreInst &I) {
  Value *OrigVal = I.getValueOperand();
  Value *OldPtr = I.getPointerOperand();

  // Check if Store operands were legalized before, or stored value is constant
  if (Value *LegalizedNewPtr = getSinglePromotedValueIfExist(OldPtr)) {
    Value *NewStoredValue = nullptr;
    if (NewStoredValue != getSinglePromotedValueIfExist(OrigVal)) {
      if (auto *ConstantIntAsStoredValue = dyn_cast<ConstantInt>(OrigVal)) {
        auto NumberStoredInValue = ConstantIntAsStoredValue->getSExtValue();
        Type *OrigConstTy = ConstantIntAsStoredValue->getType();
        TypeSeq *TySeq = TL->getPromotedTypeSeq(OrigConstTy);
        IGC_ASSERT(TySeq != nullptr);
        Type *PromotedConstTy = TySeq->front();
        auto *NewConstInt = ConstantInt::get(PromotedConstTy, NumberStoredInValue, false);
        NewStoredValue = NewConstInt;
      }
    }

    if (NewStoredValue) {
      StoreInst *NewStore = nullptr;
      NewStore = IRB->CreateStore(NewStoredValue, LegalizedNewPtr);
      NewStore->setAlignment(IGCLLVM::getAlign(I));
      Promoted = NewStore;
      return true;
    }
  }

  Type *OrigTy = OrigVal->getType();

  ValueSeq *ValSeq;
  std::tie(ValSeq, std::ignore) = TL->getLegalizedValues(OrigVal);
  IGC_ASSERT(ValSeq != nullptr && ValSeq->size() == 1);

  unsigned AS = I.getPointerAddressSpace();

  Value *PromotedVal = ValSeq->front();

  Value *NewBasePtr = IRB->CreatePointerCast(OldPtr, IRB->getInt8PtrTy(AS), Twine(OldPtr->getName(), ".ptrcast"));

  unsigned Off = 0;
  for (unsigned TotalStoreBits = TL->getTypeStoreSizeInBits(OrigTy), ActualStoreBits = 0; TotalStoreBits != 0;
       TotalStoreBits -= ActualStoreBits) {

    // Get the largest integer type but not bigger than total store bits.
    ActualStoreBits = TL->getLargestLegalIntTypeSize(TotalStoreBits);

    Type *NewTy = TL->getIntNTy(ActualStoreBits);
    Type *NewPtrTy = PointerType::get(NewTy, AS);

    Value *NewPtr = TL->getPointerToElt(NewBasePtr, Off, NewPtrTy, Twine(NewBasePtr->getName(), ".off") + Twine(Off));

    Value *NewVal = TL->lshr(PromotedVal, Off << 3);
    NewVal = IRB->CreateTrunc(NewVal, NewTy, Twine(NewVal->getName(), ".trunc"));

    StoreInst *NewSt = IRB->CreateStore(NewVal, NewPtr);
    TL->dupMemoryAttribute(NewSt, &I, Off);

    IGC_ASSERT_MESSAGE((ActualStoreBits & 0x7) == 0, "LEGAL INTEGER TYPE IS NOT BYTE ADDRESSABLE!");
    Off += ActualStoreBits >> 3;
  }

  return true;
}

/// Cast operators

bool InstPromoter::visitTruncInst(TruncInst &I) {
  auto [ValSeq, ValAct] = TL->getLegalizedValues(I.getOperand(0));

  Value *Val = I.getOperand(0);
  if (ValAct != Legal)
    Val = ValSeq->front();

  auto [TySeq, Act] = TL->getLegalizedTypes(I.getDestTy());
  IGC_ASSERT(Act == Legal || Act == Promote);

  if (Act == Legal) {
    if (Val->getType() == I.getType())
      I.replaceAllUsesWith(Val);
    else
      I.setOperand(0, Val);

    return true;
  }

  IGC_ASSERT(TySeq->size() == 1);

  Type *PromotedTy = TySeq->front();

  [[maybe_unused]] unsigned PromotedBitWidth = cast<IntegerType>(PromotedTy->getScalarType())->getBitWidth();
  [[maybe_unused]] unsigned ValBitWidth = cast<IntegerType>(Val->getType()->getScalarType())->getBitWidth();
  IGC_ASSERT(PromotedBitWidth <= ValBitWidth);

  Value *PromVal = IRB->CreateTrunc(Val, PromotedTy, Twine(I.getName(), getSuffix()));
  uint64_t mask = (1ULL << I.getType()->getScalarType()->getIntegerBitWidth()) - 1;
  Promoted = IRB->CreateAnd(PromVal, mask);

  return true;
}

bool InstPromoter::visitSExtInst(SExtInst &I) {
  auto *ExtendedValue = I.getOperand(0);
  // Check if SExt operand was legalized before
  if (Value *LegalizedSingleValue = getSinglePromotedValueIfExist(ExtendedValue)) {
    if (LegalizedSingleValue->getType() == I.getDestTy()) {
      Promoted = LegalizedSingleValue;

      // Extend the legalized value
      unsigned shiftAmt = I.getDestTy()->getIntegerBitWidth() - I.getSrcTy()->getIntegerBitWidth();
      Promoted = IRB->CreateShl(Promoted, shiftAmt);
      Promoted = IRB->CreateAShr(Promoted, shiftAmt);
      return true;
    }
  }
  return false;
}

bool InstPromoter::visitZExtInst(ZExtInst &I) {
  Value *Val = I.getOperand(0);
  // Check if ZExt operand was legalized before
  if (Value *LegalizedSingleValue = getSinglePromotedValueIfExist(Val)) {
    if (LegalizedSingleValue->getType() == I.getDestTy()) {
      Promoted = LegalizedSingleValue;
      return true;
    }
  }

  auto [ValSeq, ValAct] = TL->getLegalizedValues(I.getOperand(0));

  if (ValAct != Legal)
    Val = ValSeq->front();

  auto [TySeq, Act] = TL->getLegalizedTypes(I.getDestTy());
  IGC_ASSERT(Act == Legal || Act == Promote);

  if (Act == Legal) {
    // Reset insert position as we don't create a new instruction from the
    // original one due to the legal return type.
    IRB->SetInsertPoint(&I);

    if (ValAct != Legal)
      Val = TL->zext(Val, I.getSrcTy());

    if (Val->getType() == I.getType())
      I.replaceAllUsesWith(Val);
    else
      I.setOperand(0, Val);

    return true;
  }

  IGC_ASSERT(TySeq->size() == 1);

  Type *PromotedTy = TySeq->front();

  IGC_ASSERT(cast<IntegerType>(PromotedTy)->getBitWidth() >= cast<IntegerType>(Val->getType())->getBitWidth());

  if (ValAct != Legal)
    Val = TL->zext(Val, I.getSrcTy());

  Promoted = IRB->CreateZExt(Val, PromotedTy, Twine(I.getName(), getSuffix()));

  return true;
}

bool InstPromoter::visitBitCastInst(BitCastInst &I) {
  auto [ValSeq, ValAct] = TL->getLegalizedValues(I.getOperand(0));
  Value *Val = I.getOperand(0);
  if (ValAct != Legal && ValSeq != nullptr)
    Val = ValSeq->front();

  auto [TySeq, Act] = TL->getLegalizedTypes(I.getDestTy(), true);
  IGC_ASSERT(Act == Legal || Act == Promote);

  // Promote bitcast i24 %0 to <3 x i8>
  // -->
  // %1 = bitcast i32 %0 to <4 x i8>
  // %2 = shufflevector <4 x i8> %1, <4 x i8> zeroinitializer, <i32 0, i32 1, i32 2>
  if (Act == Legal && ValAct == Promote && Val->getType()->isIntegerTy() && I.getType()->isVectorTy()) {
    Type *DestTy = I.getDestTy();
    unsigned N = Val->getType()->getScalarSizeInBits() / DestTy->getScalarSizeInBits();
    Value *BC = IRB->CreateBitCast(Val, IGCLLVM::FixedVectorType::get(DestTy->getScalarType(), N));

    std::vector<Constant *> Vals;
    for (unsigned i = 0; i < (unsigned)cast<IGCLLVM::FixedVectorType>(DestTy)->getNumElements(); i++)
      Vals.push_back(IRB->getInt32(i));

    Value *Mask = ConstantVector::get(Vals);
    Value *Zero = Constant::getNullValue(BC->getType());
    Promoted = IRB->CreateShuffleVector(BC, Zero, Mask, Twine(I.getName(), getSuffix()));
    return true;
  }

  // Promote bitcast <3 x i8> %0 to i24
  // -->
  // %1 = shufflevector <3 x i8> %0, <3 x i8> zeroinitializer, <i32 0, i32 1, i32 2, i32 3>
  // %2 = bitcast <4 x i8> %1 to i32
  if (Act == Promote && Val->getType()->isVectorTy() && I.getType()->isIntegerTy()) {
    Type *PromotedTy = TySeq->front();
    unsigned N = PromotedTy->getScalarSizeInBits() / Val->getType()->getScalarSizeInBits();
    unsigned MaxMaskIdx = 2 * (unsigned)cast<IGCLLVM::FixedVectorType>(Val->getType())->getNumElements();
    std::vector<Constant *> Vals;
    for (unsigned i = 0; i < N; i++)
      Vals.push_back(IRB->getInt32(i < MaxMaskIdx ? i : MaxMaskIdx - 1));

    Value *Mask = ConstantVector::get(Vals);
    Value *Zero = Constant::getNullValue(Val->getType());
    Value *NewVal = IRB->CreateShuffleVector(Val, Zero, Mask, Twine(I.getName(), getSuffix()));
    Promoted = IRB->CreateBitCast(NewVal, PromotedTy);
    return true;
  }

  // Promote bitcast with illegal src and dst both requiring to be promoted
  // to the same type. Example:
  //
  // bitcast i12 %v to <3 x i4>
  //
  // '%v' may just be used as a promoted value, since both i12 and <3 x i4> are
  // required to be promoted to i16 type.
  if (Act == Promote && ValAct == Promote && Val->getType() == TySeq->front()) {
    Promoted = Val;
    return true;
  }

  // Unsupported legalization action.
  return false;
}

bool InstPromoter::visitExtractElementInst(ExtractElementInst &I) {
  Value *Val = I.getOperand(0);
  auto [ValSeq, ValAct] = TL->getLegalizedValues(Val);

  if (ValAct != Legal)
    Val = ValSeq->front();

  auto [TySeq, Act] = TL->getLegalizedTypes(I.getType(), true);
  IGC_ASSERT(Act == Legal || Act == Promote);

  auto SetBits = [](uint64_t &Data, uint32_t From, uint32_t To) {
    IGC_ASSERT(From < To);
    uint64_t N = To - From;
    uint64_t Mask = (0xFFFFFFFFFFFFFFFFu >> (64 - N));
    Data |= (Mask << From);
  };

  if (Act == Promote && Val->getType()->isIntegerTy()) {
    if (ConstantInt *Index = dyn_cast<ConstantInt>(I.getIndexOperand())) {
      const uint32_t IndexImm = int_cast<uint32_t>(Index->getZExtValue());
      const uint32_t IllegalElemTyWidth = I.getOperand(0)->getType()->getScalarSizeInBits();

      // Mask preparation
      uint64_t Mask = 0;
      const uint32_t From = IndexImm * IllegalElemTyWidth;
      const uint32_t To = From + IllegalElemTyWidth;
      SetBits(Mask, From, To);

      // Clear bits representing source vector elements that are not queried by
      // extract element instruction
      Value *And = IRB->CreateAnd(Val, Mask);

      // Shift relevant bits to put sign bit on it's place
      Type *LegalReturnTy = TySeq->front();
      const uint32_t LegalReturnTyWidth = LegalReturnTy->getIntegerBitWidth();
      const uint32_t CurrSignPos = To - 1;
      const uint32_t TargetSignPos = LegalReturnTyWidth - 1;
      Value *Shift = nullptr;
      if (CurrSignPos == TargetSignPos) {
        // Continue operating on 'and' instruction. Shifting is not required,
        // since sign bit is already in place.
        Shift = And;
      } else {
        if (CurrSignPos < TargetSignPos) {
          Shift = IRB->CreateShl(And, TargetSignPos - CurrSignPos);
        } else if (CurrSignPos > TargetSignPos) {
          Shift = IRB->CreateLShr(And, CurrSignPos - TargetSignPos);
        }
      }

      // Truncate result to a legalized type
      Value *Trunc = IRB->CreateTrunc(Shift, LegalReturnTy);

      // Shift bits back to put relevant ones at the begging of an integer
      IGC_ASSERT(LegalReturnTyWidth > IllegalElemTyWidth);
      Promoted = IRB->CreateAShr(Trunc, LegalReturnTyWidth - IllegalElemTyWidth);

      return true;
    } else {
      IGC_ASSERT_MESSAGE(0, "Cannot legalize extract element instruction with non-constant index!");
      return false;
    }
  }

  return false;
}

bool InstPromoter::visitInsertElementInst(InsertElementInst &I) {
  Value *Val = I.getOperand(1);
  auto [ValSeq, ValAct] = TL->getLegalizedValues(Val);

  if (ValAct != Legal)
    Val = ValSeq->front();

  Value *VecVal = I.getOperand(0);
  if (!isa<UndefValue>(VecVal) && !isa<Constant>(VecVal)) {
    auto [VecValSeq, VecValAct] = TL->getLegalizedValues(VecVal);

    if (VecValAct != Legal)
      VecVal = VecValSeq->front();
  }

  auto [TySeq, Act] = TL->getLegalizedTypes(I.getType(), true);
  IGC_ASSERT(Act == Legal || Act == Promote);

  if (Act == Promote && Val->getType()->isIntegerTy()) {
    if (ConstantInt *Index = dyn_cast<ConstantInt>(I.getOperand(2))) {
      // Extend value to be inserted to the legalized insert element return type
      Value *ZExt = IRB->CreateZExt(Val, TySeq->front());

      // Shift extended value to it's place based on index to imitate insert element behavior
      // Note: If an insert element index is zero, we don't need to generate shl instruction.
      const uint32_t IllegalElemTyWidth = I.getOperand(0)->getType()->getScalarSizeInBits();
      const uint32_t IndexImm = int_cast<uint32_t>(Index->getZExtValue());
      const uint32_t ShiftFactor = IndexImm * IllegalElemTyWidth;
      Promoted = (IndexImm > 0) ? IRB->CreateShl(ZExt, ShiftFactor) : ZExt;

      if (!isa<UndefValue>(VecVal) && !(isa<Constant>(VecVal) && dyn_cast<Constant>(VecVal)->isZeroValue())) {
        // Generate 'and' instruction to merge value returned by another insert element instruction
        // with value to be inserted.
        IGC_ASSERT(VecVal->getType()->isIntegerTy());
        Promoted = IRB->CreateAnd(VecVal, Promoted);
      }
    } else {
      IGC_ASSERT_MESSAGE(0, "Cannot legalize insert element instruction with non-constant index!");
      return false;
    }
  }

  return true;
}

/// Other operators

bool InstPromoter::visitGenIntrinsicInst(GenIntrinsicInst &I) {
  IGC_ASSERT_EXIT_MESSAGE(0, "UNKNOWN GEN INSTRINSIC INSTRUCTION IS BEING PROMOTED!");
  return false;
}

std::pair<Value *, Type *> InstPromoter::preparePromotedIntrinsicInst(IntrinsicInst &I) {
  SmallVector<Value *, 2> PromotedArgs;
  // -1 because we want to avoid the last operand which is intrinsic declaration
  for (unsigned i = 0; i < I.getNumOperands() - 1; i++) {
    auto [ValSeq, ValAct] = TL->getLegalizedValues(I.getOperand(i));
    Value *Val = I.getOperand(i);
    if (ValAct != Legal)
      Val = ValSeq->front();
    PromotedArgs.push_back(Val);
  }

  auto [TySeq, Act] = TL->getLegalizedTypes(I.getType());
  IGC_ASSERT(Act == Promote);
  IGC_ASSERT(TySeq->size() == 1);
  Type *PromotedTy = TySeq->front();

  [[maybe_unused]] unsigned PromotedBitWidth = cast<IntegerType>(PromotedTy->getScalarType())->getBitWidth();
  for (Value *Val : PromotedArgs) {
    [[maybe_unused]] unsigned ValBitWidth = cast<IntegerType>(Val->getType()->getScalarType())->getBitWidth();
    IGC_ASSERT(PromotedBitWidth == ValBitWidth);
  }

  Function *Func = Intrinsic::getDeclaration(I.getModule(), I.getIntrinsicID(), PromotedTy);
  return {IRB->CreateCall(Func, PromotedArgs), PromotedTy};
}

bool InstPromoter::visitLLVMIntrinsicInst(IntrinsicInst &I) {
  switch (I.getIntrinsicID()) {
  case Intrinsic::bitreverse: {
    auto [Call, PromotedTy] = preparePromotedIntrinsicInst(I);
    unsigned PromotedBitWidth = cast<IntegerType>(PromotedTy->getScalarType())->getBitWidth();
    unsigned shift = PromotedBitWidth - cast<IntegerType>(I.getType()->getScalarType())->getBitWidth();
    IGC_ASSERT(shift > 0);
    Promoted = IRB->CreateLShr(Call, shift);
    break;
  }
  case Intrinsic::smin:
  case Intrinsic::smax:
  case Intrinsic::umin:
  case Intrinsic::umax:
    Promoted = preparePromotedIntrinsicInst(I).first;
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "UNKNOWN INSTRINSIC INSTRUCTION IS BEING PROMOTED!");
  }
  return true;
}

bool InstPromoter::visitCallInst(CallInst &I) {
  if (isa<GenIntrinsicInst>(&I))
    return visitGenIntrinsicInst(static_cast<GenIntrinsicInst &>(I));
  if (isa<IntrinsicInst>(&I))
    return visitLLVMIntrinsicInst(static_cast<IntrinsicInst &>(I));
  IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
  return false;
}
