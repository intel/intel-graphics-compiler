/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "GASPropagator.h"
#include "llvmWrapper/IR/DerivedTypes.h"

using namespace IGC;

bool GASPropagator::isAddrSpaceResolvable(PHINode *PN, const Loop *L, BasicBlock *BackEdge) const {
  PointerType *PtrTy = dyn_cast<PointerType>(PN->getType());
  if (!PtrTy || PtrTy->getAddressSpace() != ADDRESS_SPACE_GENERIC)
    return false;

  Instruction *Next = dyn_cast<Instruction>(PN->getIncomingValueForBlock(BackEdge));
  if (!Next)
    return false;

  // Walk through use-def chain to figure out whether `Next` is resolvable from
  // `PN`.
  while (Next != PN) {
    // GEP
    if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(Next)) {
      Next = dyn_cast<Instruction>(GEP->getPointerOperand());
      if (!Next)
        return false;
      continue;
    }
    // TODO: Add other operators.
    return false;
  }

  return true;
}

void GASPropagator::populateResolvableLoopPHIs() {
  for (auto &L : LI->getLoopsInPreorder()) {
    populateResolvableLoopPHIsForLoop(L);
  }
}

void GASPropagator::populateResolvableLoopPHIsForLoop(const Loop *L) {
  BasicBlock *H = L->getHeader();

  pred_iterator PI = pred_begin(H), E = pred_end(H);
  if (PI == E)
    return;

  BasicBlock *Incoming = *PI++;
  if (PI == E)
    return;

  BasicBlock *BackEdge = *PI++;
  if (PI != E)
    return;

  if (L->contains(Incoming)) {
    if (L->contains(BackEdge))
      return;
    std::swap(Incoming, BackEdge);
  } else if (!L->contains(BackEdge))
    return;

  for (auto I = H->begin(); isa<PHINode>(I); ++I) {
    PHINode *PN = cast<PHINode>(I);
    if (!isAddrSpaceResolvable(PN, L, BackEdge))
      continue;
    ResolvableLoopPHIs.insert(PN);
  }
}

bool GASPropagator::visitInstruction(Instruction &I) {
  // DO NOTHING.
  LLVM_DEBUG(dbgs() << "PROPAGATE:" << *TheVal << '\n');
  LLVM_DEBUG(dbgs() << "  THROUGH:" << I << '\n');
  return false;
}

bool GASPropagator::visitLoadInst(LoadInst &) {
  TheUse->set(TheVal);
  return true;
}

bool GASPropagator::visitStoreInst(StoreInst &ST) {
  // Only propagate on the `pointer` operand. If that generic pointer is used
  // as value operand and stored in memory, we have to use its value in generic
  // address space.
  if (TheUse->getOperandNo() != ST.getPointerOperandIndex())
    return false;
  TheUse->set(TheVal);
  return true;
}

bool GASPropagator::visitAddrSpaceCastInst(AddrSpaceCastInst &I) {
  PointerType *SrcPtrTy = cast<PointerType>(TheVal->getType());
  PointerType *DstPtrTy = cast<PointerType>(I.getType());
  // Skip if a cast between two different address spaces will be generated.
  if (SrcPtrTy->getAddressSpace() != DstPtrTy->getAddressSpace())
    return false;

  Value *Src = TheVal;
  if (!SrcPtrTy->isOpaqueOrPointeeTypeMatches(DstPtrTy)) {
    BuilderType::InsertPointGuard Guard(IRB);
    IRB.SetInsertPoint(&I);
    Src = IRB.CreateBitCast(Src, DstPtrTy);
  }
  I.replaceAllUsesWith(Src);
  I.eraseFromParent();

  return true;
}

bool GASPropagator::visitBitCastInst(BitCastInst &I) {
  PointerType *SrcPtrTy = cast<PointerType>(TheVal->getType());
  PointerType *DstPtrTy = cast<PointerType>(I.getType());

  BuilderType::InsertPointGuard Guard(IRB);
  IRB.SetInsertPoint(I.getNextNode());
  Value *Src = TheVal;
  if (!IGCLLVM::isOpaquePointerTy(SrcPtrTy)) {
    // Push `addrspacecast` forward by replacing this `bitcast` on GAS with the
    // one on non-GAS followed by a new `addrspacecast` to GAS.
    Type *DstTy = IGCLLVM::getNonOpaquePtrEltTy(DstPtrTy); // Legacy code: getNonOpaquePtrEltTy
    PointerType *TransPtrTy = PointerType::get(DstTy, SrcPtrTy->getAddressSpace());
    if (IGCLLVM::getNonOpaquePtrEltTy(SrcPtrTy) != DstTy) // Legacy code: getNonOpaquePtrEltTy
      Src = IRB.CreateBitCast(Src, TransPtrTy);
  }
  Value *NewPtr = IRB.CreateAddrSpaceCast(Src, DstPtrTy);
  I.replaceAllUsesWith(NewPtr);
  I.eraseFromParent();
  return true;
}

bool GASPropagator::visitPtrToIntInst(PtrToIntInst &I) {
  // Don't propagate through `ptrtoint` as that conversion is different from
  // various address spaces.
  return false;
}

bool GASPropagator::visitGetElementPtrInst(GetElementPtrInst &I) {
  PointerType *SrcPtrTy = cast<PointerType>(TheVal->getType());
  PointerType *DstPtrTy = cast<PointerType>(I.getType());

  BuilderType::InsertPointGuard Guard(IRB);
  IRB.SetInsertPoint(I.getNextNode());
  // Push `getelementptr` forward by replacing this `bitcast` on GAS with the
  // one on non-GAS followed by a new `addrspacecast` to GAS.
  Type *DstTy = I.getResultElementType();
  PointerType *TransPtrTy = PointerType::get(DstTy, SrcPtrTy->getAddressSpace());
  TheUse->set(TheVal);
  I.mutateType(TransPtrTy);
  Value *NewPtr = IRB.CreateAddrSpaceCast(&I, DstPtrTy);
  for (auto UI = I.use_begin(), UE = I.use_end(); UI != UE; /*EMPTY*/) {
    Use &U = *UI++;
    if (U.getUser() == NewPtr)
      continue;
    U.set(NewPtr);
  }
  return true;
}

bool GASPropagator::visitPHINode(PHINode &PN) {
  Type *NonGASTy = TheVal->getType();
  Type *GASTy = PN.getType();

  unsigned e = PN.getNumIncomingValues();
  SmallVector<Value *, 4> NewIncomingValues(e);

  if (isResolvableLoopPHI(&PN)) {
    // For resolvable loop phi, resolve it based on the
    for (unsigned i = 0; i != e; ++i) {
      Value *V = PN.getIncomingValue(i);
      // For incoming value, use the value being propagated.
      if (V == TheUse->get()) {
        NewIncomingValues[i] = TheVal;
        continue;
      }
      Instruction *I = cast<Instruction>(V);
      // For value generated inside loop, cast them to non-GAS pointers.
      BuilderType::InsertPointGuard Guard(IRB);
      IRB.SetInsertPoint(I->getNextNode());

      NewIncomingValues[i] = IRB.CreateAddrSpaceCast(I, NonGASTy);
    }
  } else {
    // Otherwise check whether all incoming values are casted from the same
    // address space.
    for (unsigned i = 0; i != e; ++i) {
      Value *V = PN.getIncomingValue(i);
      if (V == TheUse->get()) {
        NewIncomingValues[i] = TheVal;
        continue;
      }

      Value *NewVal = nullptr;
      if (isa<ConstantPointerNull>(V)) {
        NewVal = ConstantPointerNull::get(cast<PointerType>(NonGASTy));
      } else if (AddrSpaceCastInst *ASCI = dyn_cast<AddrSpaceCastInst>(V)) {
        if (ASCI->getSrcTy() == NonGASTy)
          NewVal = ASCI->getOperand(0);
        ;
      }

      if (!NewVal)
        return false;

      NewIncomingValues[i] = NewVal;
    }
  }

  // Propagate this phi node.
  PHINode *NewPN = PHINode::Create(NonGASTy, e, "", &PN);
  for (unsigned i = 0; i != e; ++i)
    NewPN->addIncoming(NewIncomingValues[i], PN.getIncomingBlock(i));
  NewPN->takeName(&PN);
  NewPN->setDebugLoc(PN.getDebugLoc());

  BuilderType::InsertPointGuard Guard(IRB);
  IRB.SetInsertPoint(PN.getParent()->getFirstNonPHI());
  Value *NewPtr = IRB.CreateAddrSpaceCast(NewPN, GASTy);
  PN.replaceAllUsesWith(NewPtr);
  PN.eraseFromParent();
  return true;
}

bool GASPropagator::visitICmp(ICmpInst &I) {
  Type *NonGASTy = TheVal->getType();

  unsigned OpNo = TheUse->getOperandNo();
  Use *TheOtherUse = &I.getOperandUse(1 - OpNo);

  AddrSpaceCastInst *ASCI = dyn_cast<AddrSpaceCastInst>(TheOtherUse->get());
  if (!ASCI || ASCI->getSrcTy() != NonGASTy)
    return false;

  TheUse->set(TheVal);
  TheOtherUse->set(ASCI->getOperand(0));

  return true;
}

bool GASPropagator::visitSelect(SelectInst &I) {
  Type *NonGASTy = TheVal->getType();

  unsigned OpNo = TheUse->getOperandNo();
  Use *TheOtherUse = &I.getOperandUse(3 - OpNo);

  Value *TheOtherVal = nullptr;
  if (isa<ConstantPointerNull>(TheOtherUse->get())) {
    TheOtherVal = ConstantPointerNull::get(cast<PointerType>(NonGASTy));
  } else if (AddrSpaceCastInst *ASCI = dyn_cast<AddrSpaceCastInst>(TheOtherUse->get())) {
    if (ASCI->getSrcTy() == NonGASTy)
      TheOtherVal = ASCI->getPointerOperand();
  }

  if (!TheOtherVal)
    return false;

  // Change select operands to non-GAS
  TheUse->set(TheVal);
  TheOtherUse->set(TheOtherVal);

  // Handle select return type
  BuilderType::InsertPointGuard Guard(IRB);
  IRB.SetInsertPoint(I.getNextNode());

  PointerType *DstPtrTy = cast<PointerType>(I.getType());
  PointerType *NonGASPtrTy = dyn_cast<PointerType>(NonGASTy);

  // Push 'addrspacecast' forward by changing the select return type to non-GAS pointer
  // followed by a new 'addrspacecast' to GAS
  PointerType *TransPtrTy = IGCLLVM::getWithSamePointeeType(DstPtrTy, NonGASPtrTy->getAddressSpace());
  I.mutateType(TransPtrTy);
  Value *NewPtr = IRB.CreateAddrSpaceCast(&I, DstPtrTy);

  for (auto UI = I.use_begin(), UE = I.use_end(); UI != UE;) {
    Use &U = *UI++;
    if (U.getUser() == NewPtr)
      continue;
    U.set(NewPtr);
  }
  return true;
}

static bool handleMemTransferInst(MemTransferInst &I) {
  Value *NewDst = nullptr;
  Type *NewDstTy = nullptr;
  Use *DstUse = &I.getArgOperandUse(0);
  if (auto ASCI = dyn_cast<AddrSpaceCastInst>(DstUse->get())) {
    NewDst = ASCI->getOperand(0);
    NewDstTy = NewDst->getType();
  }

  Value *NewSrc = nullptr;
  Type *NewSrcTy = nullptr;
  Use *SrcUse = &I.getArgOperandUse(1);
  if (auto ASCI = dyn_cast<AddrSpaceCastInst>(SrcUse->get())) {
    NewSrc = ASCI->getOperand(0);
    NewSrcTy = NewSrc->getType();
  }

  // No address space cast on src or dst.
  if (NewDst == nullptr && NewSrc == nullptr)
    return false;

  Type *Tys[] = {NewDstTy ? NewDstTy : I.getArgOperand(0)->getType(),
                 NewSrcTy ? NewSrcTy : I.getArgOperand(1)->getType(), I.getArgOperand(2)->getType()};
  Function *Fn = nullptr;
  IGC_ASSERT(nullptr != I.getParent());
  IGC_ASSERT(nullptr != I.getParent()->getParent());
  Module *M = I.getParent()->getParent()->getParent();
  if (isa<MemCpyInst>(I))
    Fn = Intrinsic::getDeclaration(M, Intrinsic::memcpy, Tys);
  else if (isa<MemMoveInst>(I))
    Fn = Intrinsic::getDeclaration(M, Intrinsic::memmove, Tys);
  else
    IGC_ASSERT_EXIT_MESSAGE(0, "unsupported memory intrinsic");

  I.setCalledFunction(Fn);
  if (nullptr != NewDst) {
    IGC_ASSERT(nullptr != DstUse);
    DstUse->set(NewDst);
  }
  if (nullptr != NewSrc) {
    IGC_ASSERT(nullptr != SrcUse);
    SrcUse->set(NewSrc);
  }
  return true;
}

bool GASPropagator::visitMemCpyInst(MemCpyInst &I) { return handleMemTransferInst(I); }

bool GASPropagator::visitMemMoveInst(MemMoveInst &I) { return handleMemTransferInst(I); }

bool GASPropagator::visitMemSetInst(MemSetInst &I) {
  Use *DstUse = &I.getArgOperandUse(0);
  auto ASCI = dyn_cast<AddrSpaceCastInst>(DstUse->get());
  if (!ASCI)
    return false;

  Value *OrigDst = ASCI->getOperand(0);
  Type *OrigDstTy = OrigDst->getType();

  Type *Tys[] = {OrigDstTy, I.getArgOperand(2)->getType()};
  Function *Fn = Intrinsic::getDeclaration(I.getParent()->getParent()->getParent(), Intrinsic::memset, Tys);

  I.setCalledFunction(Fn);
  DstUse->set(OrigDst);
  return true;
}

bool GASPropagator::visitCallInst(CallInst &I) {
  Function *Callee = I.getCalledFunction();

  if (!Callee)
    return false;

  PointerType *SrcPtrTy = cast<PointerType>(TheVal->getType());
  bool IsGAS2P = Callee->getName().equals("__builtin_IB_memcpy_generic_to_private");
  bool IsP2GAS = Callee->getName().equals("__builtin_IB_memcpy_private_to_generic");
  if (IsGAS2P || IsP2GAS) {
    Type *Tys[4];
    Tys[0] = IsGAS2P ? I.getArgOperand(0)->getType() : SrcPtrTy;
    Tys[1] = IsGAS2P ? SrcPtrTy : I.getArgOperand(1)->getType();
    Tys[2] = I.getArgOperand(2)->getType();
    Tys[3] = I.getArgOperand(3)->getType();
    FunctionType *FTy = FunctionType::get(I.getType(), Tys, false);
    Module *M = I.getParent()->getParent()->getParent();

    auto getBuiltinFn = [&]() {
      switch (SrcPtrTy->getAddressSpace()) {
      case ADDRESS_SPACE_PRIVATE:
        return M->getOrInsertFunction("__builtin_IB_memcpy_private_to_private", FTy);
        break;
      case ADDRESS_SPACE_GLOBAL:
        return M->getOrInsertFunction(
            IsGAS2P ? "__builtin_IB_memcpy_global_to_private" : "__builtin_IB_memcpy_private_to_global", FTy);
        break;
      case ADDRESS_SPACE_CONSTANT:
        return M->getOrInsertFunction(
            IsGAS2P ? "__builtin_IB_memcpy_constant_to_private" : "__builtin_IB_memcpy_private_to_constant", FTy);
        break;
      case ADDRESS_SPACE_LOCAL:
        return M->getOrInsertFunction(
            IsGAS2P ? "__builtin_IB_memcpy_local_to_private" : "__builtin_IB_memcpy_private_to_local", FTy);
        break;
      }
      return (FunctionCallee) nullptr;
    };

    auto NewF = getBuiltinFn();
    if (NewF) {
      I.setCalledFunction(NewF);
      TheUse->set(TheVal);
      return true;
    }
  }

  if (Callee->getName().equals("__builtin_IB_to_local")) {
    Type *DstTy = I.getType();
    Value *NewPtr = Constant::getNullValue(DstTy);
    if (SrcPtrTy->getAddressSpace() == ADDRESS_SPACE_LOCAL) {
      BuilderType::InsertPointGuard Guard(IRB);
      IRB.SetInsertPoint(&I);
      NewPtr = IRB.CreateBitCast(TheVal, DstTy);
    }
    I.replaceAllUsesWith(NewPtr);
    I.eraseFromParent();

    return true;
  }

  if (Callee->getName().equals("__builtin_IB_to_private")) {
    Type *DstTy = I.getType();
    Value *NewPtr = Constant::getNullValue(DstTy);
    if (SrcPtrTy->getAddressSpace() == ADDRESS_SPACE_PRIVATE) {
      BuilderType::InsertPointGuard Guard(IRB);
      IRB.SetInsertPoint(&I);
      NewPtr = IRB.CreateBitCast(TheVal, DstTy);
    }
    I.replaceAllUsesWith(NewPtr);
    I.eraseFromParent();

    return true;
  }

  return false;
}

bool GASPropagator::visitDbgDeclareInst(DbgDeclareInst &I) {
  MetadataAsValue *MAV = MetadataAsValue::get(TheVal->getContext(), ValueAsMetadata::get(TheVal));
  I.replaceVariableLocationOp(I.getVariableLocationOp(0), MAV);
  return true;
}

bool GASPropagator::visitDbgValueInst(DbgValueInst &I) {
  MetadataAsValue *MAV = MetadataAsValue::get(TheVal->getContext(), ValueAsMetadata::get(TheVal));
  I.replaceVariableLocationOp(I.getVariableLocationOp(0), MAV);
  return true;
}

bool GASPropagator::propagateToAllUsers(AddrSpaceCastInst *I) {
  // Since %49 is used twice in a phi instruction like the one below:
  // %56 = phi %"class.someclass" addrspace(4)* [ %49, %53 ], [ %49, %742 ]
  // the use iterator was handling such phi instructions twice.
  // This was causing a crash since propagate function might erase instructions.
  SmallPtrSet<Instruction *, 8> InstSet;
  SmallVector<Use *, 8> Uses;
  for (auto UI = I->use_begin(), UE = I->use_end(); UI != UE; ++UI) {
    Use *U = &(*UI);
    Instruction *I = cast<Instruction>(U->getUser());
    if (InstSet.insert(I).second) {
      Uses.push_back(U);
    }
  }

  if (auto *L = LocalAsMetadata::getIfExists(I))
    if (auto *MDV = MetadataAsValue::getIfExists(I->getContext(), L))
      for (auto &Use : MDV->uses())
        Uses.push_back(&Use);

  bool Changed = false;
  // Propagate that source through all users of this cast.
  for (Use *U : Uses) {
    Changed |= propagateToUser(U, I->getOperand(0));
  }
  return Changed;
}

void GASPropagator::propagate(Value *I) {
  PointerType *ptrTy = dyn_cast<PointerType>(I->getType());

  if (!ptrTy)
    return;

  // propagate only non generic pointers
  if (ptrTy->getAddressSpace() == ADDRESS_SPACE_GENERIC)
    return;

  SmallVector<AddrSpaceCastInst *, 8> addrSpaceCastsToResolve;
  for (User *user : I->users())
    if (auto *addrSpaceCast = dyn_cast<AddrSpaceCastInst>(user))
      if (addrSpaceCast->getDestAddressSpace() == ADDRESS_SPACE_GENERIC)
        addrSpaceCastsToResolve.push_back(addrSpaceCast);

  bool propagated = false;
  for (AddrSpaceCastInst *addrSpaceCast : addrSpaceCastsToResolve) {
    propagated |= propagateToAllUsers(addrSpaceCast);

    if (addrSpaceCast->use_empty())
      addrSpaceCast->eraseFromParent();
  }

  if (!propagated)
    return;

  // continue propagation through instructions that may return a pointer
  for (auto user : I->users()) {
    if (Instruction *userInst = dyn_cast<Instruction>(user)) {
      switch (userInst->getOpcode()) {
      case Instruction::PHI:
      case Instruction::GetElementPtr:
      case Instruction::Select:
      case Instruction::BitCast:
        propagate(userInst);
      }
    }
  }
}
