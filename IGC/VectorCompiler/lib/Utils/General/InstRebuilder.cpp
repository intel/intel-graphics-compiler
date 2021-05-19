/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/InstRebuilder.h"
#include "vc/Utils/General/Types.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/Support/Alignment.h"

#include <llvm/IR/InstVisitor.h>

using namespace llvm;
using namespace vc;

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

  Instruction *visitGetElementPtrInst(GetElementPtrInst &OrigGEP) const {
    return GetElementPtrInst::Create(OrigGEP.getSourceElementType(),
                                     NewOperands.front(),
                                     NewOperands.drop_front());
  }

  Instruction *visitLoadInst(LoadInst &OrigLoad) {
    Value &Ptr = getSingleNewOperand();
    auto *NewLoad =
        new LoadInst{cast<PointerType>(Ptr.getType())->getElementType(),
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
    if (isa<PointerType>(OrigCast.getType()))
      return visitPointerBitCastInst(OrigCast);
    return new BitCastInst{&NewOp, OrigCast.getType()};
  }

  CastInst *visitPointerBitCastInst(BitCastInst &OrigCast) {
    Value &NewOp = getSingleNewOperand();
    auto NewOpAS = cast<PointerType>(NewOp.getType())->getAddressSpace();
    // If the operand changed addrspace the bitcast type should change it too.
    return new BitCastInst{
        &NewOp,
        changeAddrSpace(cast<PointerType>(OrigCast.getType()), NewOpAS)};
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
