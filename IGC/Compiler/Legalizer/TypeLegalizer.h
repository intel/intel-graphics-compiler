/*========================== begin_copyright_notice ============================

Copyright (C) 2017 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/Analysis/InlineCost.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "Compiler/CISACodeGen/RegisterPressureEstimate.hpp"
#include "common/Types.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"

namespace IGC {

namespace Legalizer {

using namespace llvm;

enum LegalizeAction {
  Legal,
  Promote,
  /*
  Expand,
  Scalarize,
  Elementize,
  */
  SoftenFloat
};

typedef IGCLLVM::IRBuilder<TargetFolder> BuilderType;

typedef TinyPtrVector<Type *> TypeSeq;
typedef TinyPtrVector<Value *> ValueSeq;

class InstLegalChecker;
class InstPromoter;
/*
class InstExpander;
class InstSoftener;
class InstScalarizer;
class InstElementizer;
*/

class TypeLegalizer : public FunctionPass {
  friend class InstLegalChecker;
  friend class InstPromoter;
  /*
  friend class InstExpander;
  friend class InstSoftener;
  friend class InstScalarizer;
  friend class InstElementizer;
  */

  const DataLayout *DL = nullptr;
  DominatorTree *DT = nullptr;
  ;
  BuilderType *IRB = nullptr;
  ;

  InstLegalChecker *ILC = nullptr;
  ;
  InstPromoter *IPromoter = nullptr;
  ;
  /*
  InstExpander* IExpander = nullptr;;
  InstSoftener* ISoftener = nullptr;;
  InstScalarizer* IScalarizer = nullptr;;
  InstElementizer* IElementizer = nullptr;;
  */

  Module *TheModule = nullptr;
  ;
  Function *TheFunction = nullptr;
  ;

  // Map from illegal type to legalized type(s).
  typedef DenseMap<Type *, TypeSeq> TypeMapTy;
  TypeMapTy TypeMap;

  // Map from illegal value to legalized value(s).
  typedef DenseMap<Value *, ValueSeq> ValueMapTy;
  ValueMapTy ValueMap;

  SmallPtrSet<Instruction *, 8> IllegalInsts;

public:
  static char ID;

  TypeLegalizer();

  bool runOnFunction(Function &F) override;

  static bool isLegalInteger(unsigned width) {
    switch (width) {
    case 8:
    case 16:
    case 32:
    case 64:
      return true;
    default:
      break;
    }
    return false;
  }

private:
  void getAnalysisUsage(AnalysisUsage &AU) const override;

  LLVMContext &getContext() const { return TheModule->getContext(); }
  DominatorTree &getDominatorTree() const { return *DT; }
  Module *getModule() const { return TheModule; }
  Function *getFunction() const { return TheFunction; }

  /*
  bool legalizeArguments(Function& F);
  bool legalizeTerminators(Function& F);
  */
  bool preparePHIs(Function &F);
  bool legalizeInsts(Function &F);
  bool populatePHIs(Function &F);

  void eraseIllegalInsts();

  LegalizeAction getTypeLegalizeAction(Type *Ty) const;

  LegalizeAction getLegalizeAction(Value *V) const;
  LegalizeAction getLegalizeAction(Instruction *I) const;

  std::pair<TypeSeq *, LegalizeAction> getLegalizedTypes(Type *Ty, bool legalizeToScalar = false);

  TypeSeq *getPromotedTypeSeq(Type *Ty, bool legalizeToScalar = false);
  TypeSeq *getSoftenedTypeSeq(Type *Ty);
  /*
  TypeSeq* getExpandedTypeSeq(Type* Ty);
  TypeSeq* getScalarizedTypeSeq(Type* Ty);
  TypeSeq* getElementizedTypeSeq(Type* Ty);
  */

  std::pair<ValueSeq *, LegalizeAction> getLegalizedValues(Value *V, bool isSigned = false);
  void setLegalizedValues(Value *OVal, ArrayRef<Value *> LegalizedVals);
  bool hasLegalizedValues(Value *V) const;

  // TODO: Refactor them into a separate legalizer.
  void promoteConstant(ValueSeq *, TypeSeq *, Constant *C, bool isSigned);
  void softenConstant(ValueSeq *, TypeSeq *, Constant *C);
  /*
  void expandConstant(ValueSeq*, TypeSeq*, Constant* C);
  void scalarizeConstant(ValueSeq*, TypeSeq*, Constant* C);
  void elementizeConstant(ValueSeq*, TypeSeq*, Constant* C);
  */

  // TODO: Refactor them into a separate legalizer.
  /*
  Value* promoteArgument(Argument* Arg, Type* PromotedTy);
  Value* softenArgument(Argument* Arg, Type* SoftenedTy);
  Value* expandArgument(Argument* Arg, Type* ExpandedTy, unsigned Part);
  Value* scalarizeArgument(Argument* Arg, Type* ScalarizedTy, unsigned Part);
  Value* elementizeArgument(Argument* Arg, Type* ElementizedTy,
      unsigned Part);
  */

  Value *getDemotedValue(Value *V);
  Value *getCompactedValue(Value *V);

  // TODO: Refactor them into a separate legalizer.
  /*
  bool promoteRet(ReturnInst* RI);
  bool expandRet(ReturnInst* RI);
  bool softenRet(ReturnInst* RI);
  bool scalarizeRet(ReturnInst* RI);
  bool elementizeRet(ReturnInst* RI);
  */

  // TODO: Refactor them into a separate legalizer.
  bool populatePromotedPHI(PHINode *PN);
  /*
  bool populateExpandedPHI(PHINode* PN);
  bool populateSoftenedPHI(PHINode* PN);
  bool populateScalarizedPHI(PHINode* PN);
  bool populateElementizedPHI(PHINode* PN);
  */

  /// getSizeTypeInBits() - Similar to the same method in DL but assertion test on overflows.
  unsigned getTypeSizeInBits(Type *Ty) const {
    uint64_t FullWidth = DL->getTypeSizeInBits(Ty);
    unsigned Width = static_cast<unsigned>(FullWidth);
    IGC_ASSERT(Width == FullWidth);

    return Width;
  }
  /*
  /// getTypeStoreSize() - Similar to the same method in DL but assertion on overflows.
  unsigned getTypeStoreSize(Type* Ty) const {
      uint64_t FullWidth = DL->getTypeStoreSize(Ty);
      unsigned Width = static_cast<unsigned>(FullWidth);
      IGC_ASSERT(Width == FullWidth);

      return Width;
  }
  */

  /// getTypeStoreSizeInBits() - Similar to the same method in DL but assertion on overflows.
  unsigned getTypeStoreSizeInBits(Type *Ty) const {
    uint64_t FullWidth = DL->getTypeStoreSizeInBits(Ty);
    unsigned Width = static_cast<unsigned>(FullWidth);
    IGC_ASSERT(Width == FullWidth);

    return Width;
  }

  /// getLargestLegalIntTypeSize() - Return the size of the largest legal
  /// integer type with size not bigger than Width bits.
  unsigned getLargestLegalIntTypeSize(unsigned Width) const {
    for (; !isLegalInteger(Width); --Width)
      /*EMPTY*/;
    return Width;
  }

  /// getProfitVectorLength() - Return the profitable vector length for
  /// memory load/store.
  ArrayRef<unsigned> getProfitMemOpVectorLength(Type *EltTy) const {
    // Possible profitable vector length combinations.
    // NOTE: *KEEP* lengths in ascending order.
    static unsigned L12[] = {1, 2};
    static unsigned L1234[] = {1, 2, 3, 4};
    static unsigned L124[] = {1, 2, 4};
    switch (EltTy->getTypeID()) {
    case Type::HalfTyID:
      return {L12};
    case Type::FloatTyID:
      return {L1234};
    case Type::DoubleTyID:
      return {L12};
    case Type::PointerTyID:
      // FIXME: In different addressing mode, the preferred vector length of
      // pointer types is different.
      return {L12};
    case Type::IntegerTyID: {
      IntegerType *IEltTy = cast<IntegerType>(EltTy);
      switch (IEltTy->getBitWidth()) {
      case 8:
        return {L124};
      case 16:
        return {L12};
      case 32:
        return {L1234};
      case 64:
        return {L12};
      default:
        break;
      }
    }
    default:
      break;
    }
    // By default, use scalar load/store only.
    return ArrayRef<unsigned>();
  }

  /// getProfitLoadVectorLength() - Return the profitable vector length for
  /// load.
  ArrayRef<unsigned> getProfitLoadVectorLength(Type *EltTy) const { return getProfitMemOpVectorLength(EltTy); }

  /// getProfitStoreVectorLength() - Return the profitable vector length for
  /// store.
  ArrayRef<unsigned> getProfitStoreVectorLength(Type *EltTy) const { return getProfitMemOpVectorLength(EltTy); }

  /// preferVectorMemOp()
  bool preferVectorMemOp(Type *Ty) const {
    if (!Ty->isVectorTy())
      return false;

    unsigned NumElts = (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements();
    Type *EltTy = cast<VectorType>(Ty)->getElementType();
    const auto &ProfitLengths = getProfitLoadVectorLength(EltTy);

    return std::any_of(ProfitLengths.begin(), ProfitLengths.end(), [&](unsigned PL) { return NumElts == PL; });
  }

  /// preferVectorLoad()
  bool preferVectorLoad(Type *Ty) const { return preferVectorMemOp(Ty); }

  /// preferVectorStore()
  bool preferVectorStore(Type *Ty) const { return preferVectorMemOp(Ty); }

  /// hasLegalRetType()
  bool hasLegalRetType(Instruction *I) const {
    Type *Ty = I->getType();

    if (Ty->isVoidTy())
      return false;

    return getTypeLegalizeAction(Ty) == Legal;
  }

  /// isReservedLegal()
  bool isReservedLegal(Value *V) const {
    Instruction *I = dyn_cast<Instruction>(V);
    // Non-instruction values are always not reserved legal.
    if (!I)
      return false;

    // Loads are legal on certain vector types.
    if (LoadInst *LD = dyn_cast<LoadInst>(I))
      return preferVectorLoad(LD->getType());

    // Stores are legal on certain vector types.
    if (StoreInst *ST = dyn_cast<StoreInst>(I))
      return preferVectorStore(ST->getValueOperand()->getType());

    // ExtractElement from reserved legal insts.
    if (ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(I))
      return isReservedLegal(EEI->getVectorOperand());

    // InsertElement to reserved legal insts.
    if (InsertElementInst *IEI = dyn_cast<InsertElementInst>(I))
      return IEI->hasOneUse() && isReservedLegal(IEI->user_back());

    return false;
  }

  /// getIntrinsic()
  Function *getIntrinsic(GenISAIntrinsic::ID IID, ArrayRef<Type *> Tys) const {
    return GenISAIntrinsic::getDeclaration(getModule(), IID, Tys);
  }

  /// Common helpers by different legalizers.
  ///

  /// getIntNTy() -
  IntegerType *getIntNTy(unsigned N) const { return Type::getIntNTy(getContext(), N); }

  /// getIntN() -
  ConstantInt *getIntN(unsigned N, uint64_t C) const { return ConstantInt::get(getIntNTy(N), C); }

  /// getIntPtrTy() -
  IntegerType *getIntPtrTy(unsigned AddrSpace) const { return DL->getIntPtrType(getContext(), AddrSpace); }

  /*
  /// getIntBitsTy() - Return an integer type with the same bits of the given
  /// type.
  IntegerType* getIntBitsTy(Type* Ty) const {
      return getIntNTy((unsigned int)Ty->getPrimitiveSizeInBits());
  }
  */

  /// getAlignment() - Return the alignment of the memory access being
  /// performed.
  template <typename InstTy> alignment_t getAlignment(InstTy *) const {
    IGC_ASSERT_EXIT_MESSAGE(0, "ALIGNMENT IS CHECKED ON NON MEMORY INSTRUCTION!");
  }

  template <typename InstTy> void dupMemoryAttribute(InstTy *, InstTy *, unsigned) const {
    IGC_ASSERT_EXIT_MESSAGE(0, "ATTRIBUTE IS DUPLICATED ON NON MEMORY INSTRUCTION!");
  }

  /// createBinOpAsGiven() - Create a binary operator with the same attribute
  /// as the specified one but with other operands.
  Value *createBinOpAsGiven(BinaryOperator *I, Value *LHS, Value *RHS, const Twine &Name = "") const {
    // TODO: Check whether it is possible to simplify the case where RHS is
    // constant on specific instructions, e.g. and/or/xor/shl/lshr/ashr and
    // etc.
    Value *Res = IRB->CreateBinOp(I->getOpcode(), LHS, RHS, Name);
    if (BinaryOperator *BO = dyn_cast<BinaryOperator>(Res)) {
      // Copy overflow flags if any.
      if (isa<OverflowingBinaryOperator>(BO)) {
        BO->setHasNoSignedWrap(I->hasNoSignedWrap());
        BO->setHasNoUnsignedWrap(I->hasNoUnsignedWrap());
      }
      // Copy exact flag if any.
      if (isa<PossiblyExactOperator>(BO))
        BO->setIsExact(I->isExact());
      // Copy fast math flags if any.
      if (isa<FPMathOperator>(BO))
        BO->setFastMathFlags(I->getFastMathFlags());
    }
    return Res;
  }
  /*
  /// castToInt() - Cast the specified value into integer type with the same
  /// size.
  Value* castToInt(Value* V) const {
      IntegerType* ITy = getIntBitsTy(V->getType());
      return IRB->CreateBitCast(V, ITy, Twine(V->getName(), ".icast"));
  }
  */

  /// shl() - Left-shift integer values with constant shift amount.
  Value *shl(Value *V, uint64_t ShAmt) const {
    if (ShAmt == 0)
      return V;
    return IRB->CreateShl(V, ShAmt, Twine(V->getName(), ".shl"));
  }

  /// lshr() - Logic right-shift integer values with constant shift amount.
  Value *lshr(Value *V, uint64_t ShAmt) const {
    if (ShAmt == 0)
      return V;
    return IRB->CreateLShr(V, ShAmt, Twine(V->getName(), ".lshr"));
  }

  /// zext() - Zero-extend illegal integer values holding in promoted types.
  Value *zext(Value *V, Type *OrigTy) const {
    IGC_ASSERT(V->getType()->getIntegerBitWidth() > OrigTy->getIntegerBitWidth());

    APInt Mask = APInt::getAllOnes(OrigTy->getIntegerBitWidth());
    Constant *C = getIntN(V->getType()->getIntegerBitWidth(), Mask.getZExtValue());
    return IRB->CreateAnd(V, C, Twine(V->getName(), ".zext"));
  }
  // Variant accepts/returns a pair of values of the same type.
  std::pair<Value *, Value *> zext(Value *LHS, Value *RHS, Type *OrigTy) const {
    IGC_ASSERT(LHS->getType() == RHS->getType());
    IGC_ASSERT(LHS->getType()->getIntegerBitWidth() > OrigTy->getIntegerBitWidth());

    APInt Mask = APInt::getAllOnes(OrigTy->getIntegerBitWidth());
    Constant *C = getIntN(LHS->getType()->getIntegerBitWidth(), Mask.getZExtValue());
    return std::make_pair(IRB->CreateAnd(LHS, C, Twine(LHS->getName(), ".zext")),
                          IRB->CreateAnd(RHS, C, Twine(RHS->getName(), ".zext")));
  }

  /// sext() - Sign-extend illegal integer values holding in promoted types.
  Value *sext(Value *V, Type *OrigTy) const {
    IGC_ASSERT(V->getType()->getIntegerBitWidth() > OrigTy->getIntegerBitWidth());

    Constant *ShAmt =
        getIntN(V->getType()->getIntegerBitWidth(), V->getType()->getIntegerBitWidth() - OrigTy->getIntegerBitWidth());
    return IRB->CreateAShr(IRB->CreateShl(V, ShAmt, Twine(V->getName(), ".lsext")), ShAmt,
                           Twine(V->getName(), ".rsext"));
  }
  // Variant accepts/returns a pair of values of the same type.
  std::pair<Value *, Value *> sext(Value *LHS, Value *RHS, Type *OrigTy) const {
    IGC_ASSERT(LHS->getType() == RHS->getType());
    IGC_ASSERT(LHS->getType()->getIntegerBitWidth() > OrigTy->getIntegerBitWidth());

    Constant *ShAmt = getIntN(LHS->getType()->getIntegerBitWidth(),
                              LHS->getType()->getIntegerBitWidth() - OrigTy->getIntegerBitWidth());
    return std::make_pair(IRB->CreateAShr(IRB->CreateShl(LHS, ShAmt, Twine(LHS->getName(), ".lsext")), ShAmt,
                                          Twine(LHS->getName(), ".rsext")),
                          IRB->CreateAShr(IRB->CreateShl(RHS, ShAmt, Twine(RHS->getName(), ".lsext")), ShAmt,
                                          Twine(RHS->getName(), ".rsext")));
  }
  /*
  /// Expander helper
  /// umin() - Return the minimal of (unsigned) integers and cast it into new
  /// integer type if specified.
  Value* umin(Value* LHS, Value* RHS, Type* NewTy = nullptr) const {
      StringRef Name = LHS->getName();

      Value* Cond =
          IRB->CreateICmpULT(LHS, RHS, Twine(Name, ".umin.cond"));
      Value* Min =
          IRB->CreateSelect(Cond, LHS, RHS, Twine(Name, ".umin"));

      if (!NewTy)
          return Min;
      return IRB->CreateZExtOrTrunc(Min, NewTy, Twine(Name, ".umin.trunc"));
  }
  */

  /// getPointerToElt() - Calculate the pointer to the element specified by
  /// the index, i.e. &Base[Idx], and cast it to the given type if necessary.
  Value *getPointerToElt(Value *BasePtr, unsigned Idx, Type *PtrTy, const Twine &Name = "") const {
    Type *BasePtrTy = BasePtr->getType();
    Value *NewPtr = BasePtr;

    if (Idx != 0) {
      unsigned AS = BasePtrTy->getPointerAddressSpace();
      NewPtr = IRB->CreateInBoundsGEP(IRB->getInt8Ty(), BasePtr, ConstantInt::get(getIntPtrTy(AS), Idx), Name);
    }

    if (BasePtrTy != PtrTy) {
      NewPtr = IRB->CreatePointerCast(NewPtr, PtrTy, Twine(NewPtr->getName(), ".ptrcast"));
    }

    return NewPtr;
  }
  /*
  /// Expander and Scalarizer helper
  /// repack() - Re-pack source values into the new types. It assumes that
  /// they have the equal total bits, e.g. re-pack ((float 123.0), (i8 45))
  /// to (i20, i20), or vice versa.
  void repack(ValueSeq* ValSeq,
      ArrayRef<Type*> Tys, ArrayRef<Value*> Vals,
      const Twine& Name = "") const {
      auto VI = Vals.begin(), VE = Vals.end();

      Value* V = castToInt(*VI);
      unsigned SrcWidth = (unsigned int)V->getType()->getPrimitiveSizeInBits();
      unsigned SrcOff = 0;

      unsigned Part = 0;
      for (auto* Ty : Tys) {
          IntegerType* ITy = getIntBitsTy(Ty);
          Value* Pack = ConstantInt::get(ITy, 0);
          for (unsigned DstOff = 0,
              DstWidth = ITy->getBitWidth(); DstOff != DstWidth; ) {
              if (SrcOff == SrcWidth) {
                  ++VI;
                  IGC_ASSERT(VI != VE);

                  V = castToInt(*VI);
                  SrcWidth = (unsigned int)V->getType()->getPrimitiveSizeInBits();
                  SrcOff = 0;
              }

              StringRef Name = V->getName();

              // Pack |= cast<ITy>(V >> SrcOff) << DstOff;
              Value* Bits =
                  IRB->CreateLShr(V, SrcOff, Twine(Name, ".lshr") + Twine(SrcOff));
              Bits =
                  IRB->CreateZExtOrTrunc(Bits, ITy, Twine(Name, ".cast"));
              Bits =
                  IRB->CreateShl(Bits, DstOff, Twine(Name, ".shl") + Twine(DstOff));
              Pack = IRB->CreateOr(Pack, Bits, Twine(Name, ".or"));

              // Because of that two shifts, only W bits are packed each time.
              unsigned W = std::min(DstWidth - DstOff, SrcWidth - SrcOff);
              SrcOff += W;
              DstOff += W;
          }

          ValSeq->push_back(
              IRB->CreateBitCast(Pack, Ty, Name + Twine(Part)));
      }

      IGC_ASSERT(VI + 1 == VE);
      IGC_ASSERT(SrcOff == SrcWidth);
  }
  */

  /// getSuffix() - return suffix for legalization rewriting.
  static const char *getSuffix(LegalizeAction Act) {
    static const char *Suffixes[] = {".legal", ".promote",
                                     /*
                                     ".ex",
                                     ".sclr",
                                     ".elt",
                                     */
                                     ".soften"};
    return Suffixes[Act];
  }
};

/// Explicitly specialized helper templates.
///

template <> inline alignment_t TypeLegalizer::getAlignment(LoadInst *Ld) const {
  auto Align = IGCLLVM::getAlignmentValue(Ld);
  if (Align == 0)
    Align = DL->getABITypeAlign(Ld->getType()).value();
  return Align;
}

template <> inline alignment_t TypeLegalizer::getAlignment(StoreInst *St) const {
  auto Align = IGCLLVM::getAlignmentValue(St);
  if (Align == 0)
    Align = DL->getABITypeAlign(St->getValueOperand()->getType()).value();
  return Align;
}

template <> inline void TypeLegalizer::dupMemoryAttribute(LoadInst *NewLd, LoadInst *RefLd, unsigned Off) const {
  // Duplicate attributes. TODO: Duplicate necessary metadata!
  auto Align = getAlignment(RefLd);

  NewLd->setVolatile(RefLd->isVolatile());
  NewLd->setAlignment(IGCLLVM::getCorrectAlign(int_cast<alignment_t>(MinAlign(Align, Off))));
  NewLd->setOrdering(RefLd->getOrdering());
  NewLd->setSyncScopeID(RefLd->getSyncScopeID());
}

template <> inline void TypeLegalizer::dupMemoryAttribute(StoreInst *NewSt, StoreInst *RefSt, unsigned Off) const {
  // Duplicate attributes. TODO: Duplicate necessary metadata!
  auto Align = getAlignment(RefSt);

  NewSt->setVolatile(RefSt->isVolatile());
  NewSt->setAlignment(IGCLLVM::getCorrectAlign(int_cast<alignment_t>(MinAlign(Align, Off))));
  NewSt->setOrdering(RefSt->getOrdering());
  NewSt->setSyncScopeID(RefSt->getSyncScopeID());
}

} // namespace Legalizer

} // namespace IGC
