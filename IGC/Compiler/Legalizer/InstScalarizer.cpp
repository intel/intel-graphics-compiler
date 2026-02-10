/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "TypeLegalizer.h"
#include "InstScalarizer.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/Types.hpp"
#include "Probe/Assertion.h"

#define DEBUG_TYPE "type-legalizer"

using namespace llvm;
using namespace IGC::Legalizer;

bool InstScalarizer::scalarize(Instruction *I) {
  IRB->SetInsertPoint(&(*std::next(BasicBlock::iterator(I))));
  IRB->SetCurrentDebugLocation(I->getDebugLoc());
  Scalarized.clear();

  if (!visit(*I))
    return false;

  if (!Scalarized.empty())
    TL->setLegalizedValues(I, Scalarized);

  return true;
}

// By default, capture all missing instructions!
bool InstScalarizer::visitInstruction(Instruction &I) {
  LLVM_DEBUG(dbgs() << "SCALARIZE: " << I << '\n');
  IGC_ASSERT_EXIT_MESSAGE(0, "UNKNOWN INSTRUCTION IS BEING SCALARIZED!");
  return false;
}

/// Terminator instructions
///

bool InstScalarizer::visitTerminatorInst(IGCLLVM::TerminatorInst &I) {
  // All terminators are handled specially.
  return false;
}

/// Standard binary operators
///

bool InstScalarizer::visitBinaryOperator(BinaryOperator &I) {
  ValueSeq *Ops0, *Ops1;
  std::tie(Ops0, std::ignore) = TL->getLegalizedValues(I.getOperand(0));
  // we should get copy of Ops0 here, as next getLegalizedValues call may grow ValueMap object
  // when inserting new pair with ValueMap.insert(e.g.when ValueMap.NumBuckets grows from 64 to 128)
  // and previously received ValueSeq objects will become invalid.
  ValueSeq Ops0Copy(*Ops0);

  std::tie(Ops1, std::ignore) = TL->getLegalizedValues(I.getOperand(1));
  IGC_ASSERT(Ops0Copy.size() == Ops1->size());

  for (unsigned i = 0, e = Ops0Copy.size(); i != e; ++i) {
    Value *LHS = (Ops0Copy)[i];
    Value *RHS = (*Ops1)[i];
    Scalarized.push_back(TL->createBinOpAsGiven(&I, LHS, RHS, Twine(I.getName(), getSuffix()) + Twine(i)));
  }

  return true;
}

/// Memory operators
///

bool InstScalarizer::visitLoadInst(LoadInst &I) {
  Type *OrigTy = I.getType();

  TypeSeq *TySeq;
  std::tie(TySeq, std::ignore) = TL->getLegalizedTypes(OrigTy);

  StringRef Name = I.getName();

  unsigned AS = I.getPointerAddressSpace();

  Type *EltTy = cast<VectorType>(OrigTy)->getElementType();

  if (TL->preferVectorLoad(OrigTy)) {
    // Skip if this load is already legalized.
    if (TL->hasLegalizedValues(&I))
      return false;

    for (unsigned i = 0, e = TySeq->size(); i != e; ++i) {
      Value *Idx = IRB->getInt32(i);
      Scalarized.push_back(IRB->CreateExtractElement(&I, Idx, Twine(Name, getSuffix()) + Twine(i)));
    }

    return true;
  }

  const auto &ProfitLengths = TL->getProfitLoadVectorLength(EltTy);
  if (!ProfitLengths.empty()) {
    // Break vector loads into profitable vector loads and extract all
    // elements.

    // NOTE: It's assumed the element in this case is byte-addressable;
    // otherwise, it's broken.
    IGC_ASSERT(TL->getTypeSizeInBits(EltTy) == TL->getTypeStoreSizeInBits(EltTy));

    unsigned NumElts = (unsigned)cast<IGCLLVM::FixedVectorType>(OrigTy)->getNumElements();
    unsigned Elt = 0;

    Type *NewPtrTy = PointerType::get(EltTy, AS);
    Value *OldPtr = I.getPointerOperand();
    Value *NewBasePtr = IRB->CreatePointerCast(OldPtr, NewPtrTy, Twine(OldPtr->getName(), getSuffix()) + Twine(Elt));

    // Try all possible profitable lengths.
    unsigned Off = 0;
    for (auto PLI = ProfitLengths.rbegin(), PLE = ProfitLengths.rend(); PLI != PLE; ++PLI) {
      unsigned PL = *PLI;
      IGC_ASSERT(PL > 0);

      for (; NumElts >= PL; NumElts -= PL) {
        Value *NewPtr =
            TL->getPointerToElt(NewBasePtr, Elt, NewPtrTy, Twine(OldPtr->getName(), getSuffix()) + Twine(Elt));
        if (PL != 1) {
          // Load <PL x EltTy>
          Type *VecTy = IGCLLVM::FixedVectorType::get(EltTy, PL);
          Type *VecPtrTy = PointerType::get(VecTy, AS);
          NewPtr = IRB->CreatePointerCast(NewPtr, VecPtrTy, Twine(NewPtr->getName(), ".ptrcast"));
          LoadInst *NewLd = IRB->CreateLoad(NewPtr, Twine(Name, ".vec") + Twine(Elt));
          TL->dupMemoryAttribute(NewLd, &I, Off);
          for (unsigned i = 0; i != PL; ++i) {
            Value *Idx = IRB->getInt32(i);
            Scalarized.push_back(IRB->CreateExtractElement(NewLd, Idx, Twine(Name, getSuffix()) + Twine(Elt + i)));
          }
          Off += TL->getTypeStoreSizeInBits(VecTy);
        } else {
          LoadInst *NewLd = IRB->CreateLoad(NewPtr, Twine(Name, getSuffix()) + Twine(Elt));
          TL->dupMemoryAttribute(NewLd, &I, Off);
          Scalarized.push_back(NewLd);
          Off += TL->getTypeStoreSizeInBits(EltTy);
        }
        Elt += PL;
      }

      // Early out if all elements are extracted.
      if (NumElts == 0)
        break;
    }

    return true;
  }

  unsigned Width = TL->getTypeStoreSizeInBits(OrigTy);

  Type *NewTy = TL->getIntNTy(Width);
  Type *NewPtrTy = PointerType::get(NewTy, AS);

  // Load all bits.
  Value *OldPtr = I.getPointerOperand();
  Value *NewPtr = IRB->CreatePointerCast(OldPtr, NewPtrTy, Twine(OldPtr->getName(), ".ptrcast"));

  LoadInst *Chunk = IRB->CreateLoad(NewPtr, Twine(Name, ".chunk"));
  TL->dupMemoryAttribute(Chunk, &I, 0);

  Type *EltITy = TL->getIntNTy(TL->getTypeSizeInBits(EltTy));

  unsigned Elt = 0;
  Scalarized.push_back(IRB->CreateBitCast(IRB->CreateTrunc(Chunk, EltITy, Twine(Name, ".trunc") + Twine(Elt)), EltTy,
                                          Twine(Name, getSuffix()) + Twine(Elt)));

  unsigned ShAmt = 0;
  for (++Elt; Elt != TySeq->size(); ++Elt) {
    ShAmt += TL->getTypeSizeInBits(EltTy);
    Scalarized.push_back(IRB->CreateBitCast(
        IRB->CreateTrunc(IRB->CreateLShr(Chunk, TL->getIntN(Width, ShAmt), Twine(Name, ".lshr") + Twine(Elt)), EltITy,
                         Twine(Name, ".trunc") + Twine(Elt)),
        EltTy, Twine(Name, getSuffix()) + Twine(Elt)));
  }

  return true;
}

bool InstScalarizer::visitStoreInst(StoreInst &I) {
  Value *OrigVal = I.getValueOperand();
  Type *OrigTy = OrigVal->getType();

  ValueSeq *ValSeq;
  std::tie(ValSeq, std::ignore) = TL->getLegalizedValues(OrigVal);
  IGC_ASSERT(ValSeq->size() == cast<IGCLLVM::FixedVectorType>(OrigTy)->getNumElements());

  StringRef Name = OrigVal->getName();

  unsigned AS = I.getPointerAddressSpace();

  Type *EltTy = cast<VectorType>(OrigTy)->getElementType();

  if (TL->preferVectorStore(OrigTy)) {
    // If the type of store value prefer to be vector, prepare its
    // legalized into vector.

    // Reset insert position as we don't create a new store from the
    // original store.
    IRB->SetInsertPoint(&I);

    Value *Vec = UndefValue::get(OrigTy);
    for (unsigned i = 0, e = ValSeq->size(); i != e; ++i) {
      Value *Idx = IRB->getInt32(i);
      Vec = IRB->CreateInsertElement(Vec, (*ValSeq)[i], Idx, Twine(Name, ".vec") + Twine(i));
    }

    I.setOperand(0, Vec);
    return true;
  }

  const auto &ProfitLengths = TL->getProfitStoreVectorLength(EltTy);

  if (!ProfitLengths.empty()) {
    // Otherwise, if the type of store value prefer to be stored in
    // vectors, break vector stores into profitable vector stores.

    // NOTE: It's assumed the element in this case is byte-addressable;
    // otherwise, it's broken.
    IGC_ASSERT(TL->getTypeSizeInBits(EltTy) == TL->getTypeStoreSizeInBits(EltTy));

    unsigned NumElts = (unsigned)cast<IGCLLVM::FixedVectorType>(OrigTy)->getNumElements();
    unsigned Elt = 0;

    Type *NewPtrTy = PointerType::get(EltTy, AS);
    Value *OldPtr = I.getPointerOperand();
    Value *NewBasePtr = IRB->CreatePointerCast(OldPtr, NewPtrTy, Twine(OldPtr->getName(), getSuffix()) + Twine(Elt));

    // Try all possible profitable lengths.
    unsigned Off = 0;
    for (auto PLI = ProfitLengths.rbegin(), PLE = ProfitLengths.rend(); PLI != PLE; ++PLI) {
      unsigned PL = *PLI;
      IGC_ASSERT(PL > 0);

      for (; NumElts >= PL; NumElts -= PL) {
        Value *NewPtr =
            TL->getPointerToElt(NewBasePtr, Elt, NewPtrTy, Twine(OldPtr->getName(), getSuffix()) + Twine(Elt));
        if (PL != 1) {
          // Store <PL x EltTy>
          Type *VecTy = IGCLLVM::FixedVectorType::get(EltTy, PL);
          // Prepare the shorter vector.
          Value *Vec = UndefValue::get(VecTy);
          for (unsigned i = 0; i != PL; ++i) {
            Value *Idx = IRB->getInt32(i);
            Vec = IRB->CreateInsertElement(Vec, (*ValSeq)[Elt + i], Idx, Twine(Name, ".vec") + Twine(Elt + i));
          }
          Type *VecPtrTy = PointerType::get(VecTy, AS);
          NewPtr = IRB->CreatePointerCast(NewPtr, VecPtrTy, Twine(NewPtr->getName(), ".ptrcast"));
          StoreInst *NewSt = IRB->CreateStore(Vec, NewPtr);
          TL->dupMemoryAttribute(NewSt, &I, Off);
          Off += TL->getTypeStoreSizeInBits(VecTy);
        } else {
          StoreInst *NewSt = IRB->CreateStore((*ValSeq)[Elt], NewPtr);
          TL->dupMemoryAttribute(NewSt, &I, Off);
          Off += TL->getTypeStoreSizeInBits(EltTy);
        }
        Elt += PL;
      }

      // Early out if all elements are extracted.
      if (NumElts == 0)
        break;
    }

    return true;
  }

  unsigned Width = TL->getTypeStoreSizeInBits(OrigTy);

  Type *NewTy = TL->getIntNTy(Width);
  Type *EltITy = TL->getIntNTy(TL->getTypeSizeInBits(EltTy));

  auto VI = ValSeq->begin();
  auto VE = ValSeq->end();

  unsigned Elt = 0;
  Value *Chunk = IRB->CreateZExt(IRB->CreateBitCast(*VI, EltITy, Twine((*VI)->getName(), ".bicast")), NewTy,
                                 Twine(Name, ".chunk") + Twine(Elt));
  unsigned ShAmt = 0;
  for (++VI, ++Elt; VI != VE; ++VI, ++Elt) {
    ShAmt += TL->getTypeSizeInBits(EltTy);
    Value *V = IRB->CreateZExt(IRB->CreateBitCast(*VI, EltITy, Twine((*VI)->getName(), "bitcast")), NewTy,
                               Twine((*VI)->getName(), ".zext"));
    Chunk = IRB->CreateOr(Chunk, IRB->CreateShl(V, TL->getIntN(Width, ShAmt), Twine(V->getName(), ".shl")),
                          Twine(Name, ".chunk") + Twine(Elt));
  }

  Type *NewPtrTy = PointerType::get(NewTy, AS);

  Value *OldPtr = I.getPointerOperand();
  Value *NewPtr = IRB->CreatePointerCast(OldPtr, NewPtrTy, Twine(OldPtr->getName(), ".ptrcast"));

  StoreInst *NewSt = IRB->CreateStore(Chunk, NewPtr);
  TL->dupMemoryAttribute(NewSt, &I, 0);

  return true;
}

bool InstScalarizer::visitGetElementPtrInst(GetElementPtrInst &I) { return false; }

/// Cast operators
///

bool InstScalarizer::visitBitCastInst(BitCastInst &I) {
  auto [ValSeq, ValAct] = TL->getLegalizedValues(I.getOperand(0));

  ValueSeq LegalVal;
  if (ValAct == Legal) {
    LegalVal.push_back(I.getOperand(0));
    ValSeq = &LegalVal;
  }

  auto [TySeq, Act] = TL->getLegalizedTypes(I.getDestTy());

  TypeSeq LegalTy;
  if (Act == Legal) {
    LegalTy.push_back(I.getDestTy());
    TySeq = &LegalTy;
  }

  ValueSeq Repacked;
  TL->repack(&Repacked, *TySeq, *ValSeq, I.getName() + getSuffix());

  if (Act == Legal) {
    IGC_ASSERT(Repacked.size() == 1);

    I.replaceAllUsesWith(Repacked.front());
    return true;
  }

  std::swap(Scalarized, Repacked);

  return true;
}

/// Special instructions
///

bool InstScalarizer::visitExtractElementInst(ExtractElementInst &I) {
  if (!isa<Constant>(I.getIndexOperand())) {
    // TODO: Add support for non-constant index. If we don't add special
    // intrinsics, we have to fall back to memory loads/stores, i.e.,
    //
    //  %elt = extractelement <N x eT> %vec, <iN> %idx
    //
    // is translated into
    //
    // %stk = alloca <N x eT>
    // store <N x eT> %vec, <N x eT>* %stk
    // %ptr = bitcast <N x eT>* %stk to eT*
    // %eptr = getelementptr eT* %ptr, <iN> %idx
    // %elt = load eT* %eptr
    //
    // It would be much more complicated if eT is not byte addressable.
    IGC_ASSERT_EXIT_MESSAGE(0, "NON-CONSTANT IDX IN EXTRACT-ELEMENT IS NOT SUPPORTED YET!");
  }

  ConstantInt *CI = cast<ConstantInt>(I.getIndexOperand());
  uint64_t Idx = CI->getValue().getZExtValue();

  ValueSeq *ValSeq;
  std::tie(ValSeq, std::ignore) = TL->getLegalizedValues(I.getVectorOperand());

  if (Idx >= ValSeq->size()) {
    // Undef if idx exceeds the length of the vector operand.
    Scalarized.push_back(UndefValue::get(I.getType()));
    return true;
  }

  Value *Val = (*ValSeq)[int_cast<unsigned>(Idx)];

  // Skip if it's itself, i.e. the extractelement insts added during
  // legalization of vector loads.
  if (Val == &I)
    return false;

  I.replaceAllUsesWith(Val);

  return true;
}

bool InstScalarizer::visitInsertElementInst(InsertElementInst &I) {
  if (!isa<Constant>(I.getOperand(2))) {
    // TODO: Add support for non-constant index. If we don't add special
    // intrinsics, we have to fall back to memory loads/stores, i.e.,
    //
    //  %vec1 = extractelement <N x eT> %vec0, eT %elt, <iN> %idx
    //
    // is translated into
    //
    // %stk = allca <N x eT>
    // store <N x eT> %vec, <N x eT>* %stk
    // %ptr = bitcast <N x eT>* %stk to eT*
    // %eptr = getelementptr eT* %ptr, <iN> %idx
    // store eT %elt,  eT* %eptr
    // %vec1 = load <N x eT>* %stk
    //
    // It would be much more complicated if eT is not byte addressable.
    IGC_ASSERT_EXIT_MESSAGE(0, "NON-CONSTANT IDX IN INSERT-ELEMENT IS NOT SUPPORTED YET!");
  }

  ConstantInt *CI = cast<ConstantInt>(I.getOperand(2));
  uint64_t Idx = CI->getValue().getZExtValue();

  ValueSeq *VecSeq;
  std::tie(VecSeq, std::ignore) = TL->getLegalizedValues(I.getOperand(0));
  // we should get copy of VecSeq here, as next getLegalizedValues call may grow ValueMap object
  // when inserting new pair with ValueMap.insert(e.g.when ValueMap.NumBuckets grows from 64 to 128)
  // and previously received ValueSeq objects will become invalid.
  ValueSeq VecSeqCopy(*VecSeq);

  auto [EltSeq, Act] = TL->getLegalizedValues(I.getOperand(1));

  ValueSeq LegalVal;
  if (Act == Legal) {
    LegalVal.push_back(I.getOperand(1));
    EltSeq = &LegalVal;
  }

  IGC_ASSERT(nullptr != EltSeq);
  IGC_ASSERT(EltSeq->size());
  IGC_ASSERT(VecSeqCopy.size() % EltSeq->size() == 0);

  unsigned NumElts = (unsigned)cast<IGCLLVM::FixedVectorType>(I.getOperand(0)->getType())->getNumElements();
  unsigned i = 0;
  for (unsigned Elt = 0; Elt != NumElts; ++Elt) {
    if (Elt == Idx) {
      for (auto *Val : *EltSeq) {
        Scalarized.push_back(Val);
        ++i;
      }
      continue;
    }
    for (unsigned j = 0, e = EltSeq->size(); j != e; ++i, ++j)
      Scalarized.push_back((VecSeqCopy)[i]);
  }

  return true;
}
