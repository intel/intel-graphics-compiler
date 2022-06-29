/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/InstRebuilder.h"
#include "vc/Utils/General/Types.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/Support/Alignment.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IntrinsicInst.h>

#include <algorithm>
#include <iterator>

using namespace llvm;
using namespace vc;

// Define intrinsic return type based on its arguments types.
static Type *getIntrinsicRetTypeBasedOnArgs(Intrinsic::ID IID,
                                            ArrayRef<Type *> ArgTys,
                                            LLVMContext &C) {
  switch (IID) {
  case Intrinsic::masked_gather:
    // "Pass through" operand.
    return ArgTys[3];
  case Intrinsic::masked_scatter:
    return Type::getVoidTy(C);
  default:
    IGC_ASSERT_MESSAGE(0, "unsupported intrinsic");
    return nullptr;
  }
}

// Emits list of overloaded types for the provided intrinsic. Takes all the
// types that may take part in overloading: return type, types of arguments.
static std::vector<Type *>
getIntrinsicOverloadedTypes(Intrinsic::ID IID, Type *RetTy,
                            ArrayRef<Type *> ArgTys) {
  IGC_ASSERT_MESSAGE(RetTy, "wrong argument");
  // FIXME: generalize for any ID using IntrinsicInfoTable.
  switch (IID) {
  case Intrinsic::masked_gather:
    // Loaded value type and pointer operand type.
    return {RetTy, ArgTys[0]};
  case Intrinsic::masked_scatter:
    // Value and pointer types.
    return {ArgTys[0], ArgTys[1]};
  default:
    IGC_ASSERT_MESSAGE(0, "unsupported intrinsic");
    return {};
  }
}

namespace {
class cloneInstWithNewOpsImpl
    : public InstVisitor<cloneInstWithNewOpsImpl, Instruction *> {
  ArrayRef<Value *> NewOperands;

public:
  cloneInstWithNewOpsImpl(ArrayRef<Value *> NewOperandsIn)
      : NewOperands{NewOperandsIn} {}

  Instruction *visitInstruction(Instruction &I) const {
    IGC_ASSERT_MESSAGE(0, "yet unsupported instruction");
    return nullptr;
  }

  Instruction *visitBinaryOperator(BinaryOperator &OrigBO) {
    IGC_ASSERT_MESSAGE(NewOperands.size() == 2, "binary operator has 2 operands");
    return BinaryOperator::Create(OrigBO.getOpcode(), NewOperands[0],
                                  NewOperands[1]);
  }

  Instruction *visitGetElementPtrInst(GetElementPtrInst &OrigGEP) const {
    return GetElementPtrInst::Create(OrigGEP.getSourceElementType(),
                                     NewOperands.front(),
                                     NewOperands.drop_front());
  }

  Instruction *visitTrunc(TruncInst &Trunc) {
    Value &NewOp = getSingleNewOperand();
    Type *NewOutType = vc::getNewTypeForCast(
        Trunc.getType(), Trunc.getOperand(0)->getType(), NewOp.getType());
    return new TruncInst{&NewOp, NewOutType};
  }

  Instruction *visitLoadInst(LoadInst &OrigLoad) {
    Value &Ptr = getSingleNewOperand();
    auto *NewLoad =
        new LoadInst{cast<PointerType>(Ptr.getType())->getPointerElementType(),
                     &Ptr,
                     "",
                     OrigLoad.isVolatile(),
                     IGCLLVM::getAlign(OrigLoad),
                     OrigLoad.getOrdering(),
                     OrigLoad.getSyncScopeID()};
    return NewLoad;
  }

  StoreInst *visitStoreInst(StoreInst &OrigStore) {
    IGC_ASSERT_MESSAGE(NewOperands.size() == 2, "store has 2 operands");
    return new StoreInst{NewOperands[0],          NewOperands[1],
                         OrigStore.isVolatile(),  IGCLLVM::getAlign(OrigStore),
                         OrigStore.getOrdering(), OrigStore.getSyncScopeID()};
  }

  // Rebuilds bitcast \p OrigInst so it now has \p NewOp as operand and result
  // type addrspace corresponds with this operand.
  CastInst *visitBitCastInst(BitCastInst &OrigCast) {
    Value &NewOp = getSingleNewOperand();
    Type *NewOutType = vc::getNewTypeForCast(
        OrigCast.getType(), OrigCast.getOperand(0)->getType(), NewOp.getType());
    return new BitCastInst{&NewOp, NewOutType};
  }

  CastInst *visitAddrSpaceCastInst(AddrSpaceCastInst &OrigCast) {
    Value &NewOp = getSingleNewOperand();
    auto *NewOpTy = cast<PointerType>(NewOp.getType());
    auto *CastTy = cast<PointerType>(OrigCast.getType());
    if (NewOpTy->getAddressSpace() == CastTy->getAddressSpace())
      return nullptr;
    return new AddrSpaceCastInst{&NewOp, CastTy};
  }

  SelectInst *visitSelectInst(SelectInst &OrigSelect) {
    IGC_ASSERT_MESSAGE(NewOperands.size() == 3, "select has 3 operands");
    return SelectInst::Create(NewOperands[0], NewOperands[1], NewOperands[2]);
  }

  IntrinsicInst *visitIntrinsicInst(IntrinsicInst &OrigIntrinsic) {
    auto IID = OrigIntrinsic.getIntrinsicID();
    if (IID != Intrinsic::masked_gather && IID != Intrinsic::masked_scatter) {
      IGC_ASSERT_MESSAGE(0, "yet unsupported instruction");
      return nullptr;
    }
    std::vector<Type *> ArgTys;
    std::transform(NewOperands.begin(), NewOperands.end(),
                   std::back_inserter(ArgTys),
                   [](Value *Operand) { return Operand->getType(); });
    auto *RetTy =
        getIntrinsicRetTypeBasedOnArgs(IID, ArgTys, OrigIntrinsic.getContext());
    auto OverloadedTys = getIntrinsicOverloadedTypes(IID, RetTy, ArgTys);
    auto *Decl = Intrinsic::getDeclaration(OrigIntrinsic.getModule(), IID,
                                           OverloadedTys);
    return cast<IntrinsicInst>(CallInst::Create(Decl, NewOperands));
  }

private:
  Value &getSingleNewOperand() {
    IGC_ASSERT_MESSAGE(
        NewOperands.size() == 1,
        "it should've been called only for instructions with a single operand");
    return *NewOperands.front();
  }
};
} // anonymous namespace

// Creates new instruction with all the properties taken from the \p OrigInst
// except for operands that are taken from \p NewOps.
// nullptr is returned when clonning is imposible.
Instruction *vc::cloneInstWithNewOps(Instruction &OrigInst,
                                     ArrayRef<Value *> NewOps) {
  Instruction *NewInst = cloneInstWithNewOpsImpl{NewOps}.visit(OrigInst);
  if (NewInst) {
    NewInst->copyIRFlags(&OrigInst);
    NewInst->copyMetadata(OrigInst);
  }
  return NewInst;
}
