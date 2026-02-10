/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GASResolving.h"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Argument.h"

// Generic address space (GAS) pointer resolving is done in two steps:
// 1) Find cast from non-GAS pointer to GAS pointer
// 2) Propagate that non-GAS pointer to all users of that GAS pointer at best
//    effort.

#define PASS_FLAG "igc-gas-resolve"
#define PASS_DESC "Resolve generic address space"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GASResolving, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(GASResolving, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

FunctionPass *IGC::createResolveGASPass() { return new GASResolving(); }

char GASResolving::ID = 0;

bool GASResolving::runOnFunction(Function &F) {
  LLVMContext &Ctx = F.getContext();
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  BuilderType TheBuilder(Ctx);
  GASPropagator ThePropagator(Ctx, &LI);
  IRB = &TheBuilder;
  Propagator = &ThePropagator;
  bool Changed = false;

  Changed |= canonicalizeAddrSpaceCasts(F);
  Changed |= resolveMemoryFromHost(F);

  bool LocalChanged = false;
  do {
    LocalChanged = resolveOnFunction(&F);
    Changed |= LocalChanged;
  } while (LocalChanged);
  return Changed;
}

bool GASResolving::resolveOnFunction(Function *F) const {
  bool Changed = false;

  ReversePostOrderTraversal<Function *> RPOT(F);
  for (auto &BB : RPOT)
    Changed |= resolveOnBasicBlock(BB);

  return Changed;
}

// Transform the following cast
//
//  addrspacecast SrcTy addrspace(S)* to DstTy addrspace(T)*
//
// to
//
//  bitcast SrcTy addrspace(S)* to DstTy addrspace(S)*
//  addrspacecast DstTy addrspace(S)* to DstTy addrspace(T)*
//
// OpaquePointers TODO: This method will be useless once IGC is switched to opaque pointers
bool GASResolving::canonicalizeAddrSpaceCasts(Function &F) const {
  std::vector<AddrSpaceCastInst *> GASAddrSpaceCasts;
  for (auto &I : make_range(inst_begin(F), inst_end(F)))
    if (AddrSpaceCastInst *ASCI = dyn_cast<AddrSpaceCastInst>(&I)) {
      if (IGCLLVM::isPointerTy(ASCI->getType()))
        return false;
      if (ASCI->getDestAddressSpace() == GAS)
        GASAddrSpaceCasts.push_back(ASCI);
    }
  bool changed = false;
  BuilderType::InsertPointGuard Guard(*IRB);
  for (auto ASCI : GASAddrSpaceCasts) {
    Value *Src = ASCI->getPointerOperand();
    Type *SrcType = Src->getType();
    Type *DstElementType = IGCLLVM::getNonOpaquePtrEltTy(ASCI->getType()); // Legacy code: getNonOpaquePtrEltTy

    if (IGCLLVM::getNonOpaquePtrEltTy(SrcType) == DstElementType) // Legacy code: getNonOpaquePtrEltTy
      continue;

    PointerType *TransPtrTy = PointerType::get(DstElementType, SrcType->getPointerAddressSpace());
    IRB->SetInsertPoint(ASCI);
    Src = IRB->CreateBitCast(Src, TransPtrTy);
    ASCI->setOperand(0, Src);
    changed = true;
  }
  return changed;
}

bool GASResolving::resolveOnBasicBlock(BasicBlock *BB) const {
  bool Changed = false;

  for (auto BI = BB->begin(), BE = BB->end(); BI != BE; /* EMPTY */) {
    Instruction *I = &(*BI++);
    AddrSpaceCastInst *CI = dyn_cast<AddrSpaceCastInst>(I);
    // Skip non `addrspacecast` instructions.
    if (!CI)
      continue;
    PointerType *DstPtrTy = cast<PointerType>(CI->getType());
    // Skip non generic address casting.
    if (DstPtrTy->getAddressSpace() != GAS)
      continue;

    Changed = Propagator->propagateToAllUsers(CI);

    // Re-update next instruction once there's change.
    if (Changed)
      BI = std::next(BasicBlock::iterator(CI));
    // Remove this `addrspacecast` once it's no longer used.
    if (CI->use_empty()) {
      CI->eraseFromParent();
      Changed = true;
    }
  }

  return Changed;
}

bool GASResolving::resolveMemoryFromHost(Function &F) const {
  MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

  // skip all non-entry functions
  if (!isEntryFunc(pMdUtils, &F))
    return false;

  // early check in order not to iterate whole function
  if (!checkGenericArguments(F))
    return false;

  SmallVector<StoreInst *, 32> Stores;
  SmallVector<LoadInst *, 32> Loads;
  AliasAnalysis *AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();

  // collect load candidates and in parallel check for unsafe instructions
  // visitor may be a more beautiful way to do this
  for (BasicBlock &B : F) {
    for (Instruction &I : B) {
      if (auto LI = dyn_cast<LoadInst>(&I)) {
        if (isLoadGlobalCandidate(LI)) {
          Loads.push_back(LI);
        }
      } else if (auto CI = dyn_cast<CallInst>(&I)) {
        if (CI->onlyReadsMemory() || CI->onlyAccessesInaccessibleMemory())
          continue;

        // currently recognize only these ones
        // in fact intrinsics should be marked as read-only
        if (auto II = dyn_cast<IntrinsicInst>(CI)) {
          if (II->getIntrinsicID() == Intrinsic::lifetime_start || II->getIntrinsicID() == Intrinsic::lifetime_end)
            continue;
        }

        // if we have an unsafe call in the kernel, abort
        // to improve we can collect arguments of writing calls as memlocations for alias analysis
        return false;
      } else if (isa<PtrToIntInst>(&I)) {
        return false;
      } else if (auto SI = dyn_cast<StoreInst>(&I)) {
        Value *V = SI->getValueOperand();
        if (isa<PointerType>(V->getType())) {
          // this store can potentially write non-global pointer to memory
          Stores.push_back(SI);
        }
      } else if (I.mayWriteToMemory()) {
        // unsupported instruction poisoning memory
        return false;
      }
    }
  }

  if (Loads.empty())
    return false;

  bool Changed = false;
  while (!Loads.empty()) {
    LoadInst *LI = Loads.pop_back_val();

    // check that we don't have aliasing stores for this load
    // we expect to have basic and addrspace AA available at the moment
    // on optimization phase
    bool aliases = false;
    for (auto SI : Stores) {
      if (AA->alias(MemoryLocation::get(SI), MemoryLocation::get(LI))) {
        aliases = true;
        break;
      }
    }
    if (aliases)
      continue;

    convertLoadToGlobal(LI);
    Changed = true;
  }
  return Changed;
}

bool GASResolving::isLoadGlobalCandidate(LoadInst *LI) const {
  // first check that loaded address has generic address space
  // otherwise it is not our candidate
  PointerType *PtrTy = dyn_cast<PointerType>(LI->getType());
  if (!PtrTy || PtrTy->getAddressSpace() != ADDRESS_SPACE_GENERIC)
    return false;

  // next check that it is a load from function argument + offset
  // which is necessary to prove that this address has global addrspace
  Value *LoadBase = LI->getPointerOperand()->stripInBoundsOffsets();
  // WA for gep not_inbounds base, 0, 0 that is not handled in stripoffsets
  LoadBase = LoadBase->stripPointerCasts();
  if (!isa<Argument>(LoadBase))
    return false;

  // don't want to process cases when argument is from local address space
  auto LoadTy = cast<PointerType>(LoadBase->getType());
  if (LoadTy->getAddressSpace() != ADDRESS_SPACE_GLOBAL)
    return false;

  // TODO: skip cases that have been fixed on previous traversals

  return true;
}

void GASResolving::convertLoadToGlobal(LoadInst *LI) const {
  // create two addressspace casts: generic -> global -> generic
  // the next scalar phase of this pass will propagate global to all uses of the load

  PointerType *PtrTy = cast<PointerType>(LI->getType());
  IRB->SetInsertPoint(LI->getNextNode());
  PointerType *GlobalPtrTy = IGCLLVM::get(PtrTy, ADDRESS_SPACE_GLOBAL);
  Value *GlobalAddr = IRB->CreateAddrSpaceCast(LI, GlobalPtrTy);
  Value *GenericCopyAddr = IRB->CreateAddrSpaceCast(GlobalAddr, PtrTy);

  for (auto UI = LI->use_begin(), UE = LI->use_end(); UI != UE; /*EMPTY*/) {
    Use &U = *UI++;
    if (U.getUser() == GlobalAddr)
      continue;
    U.set(GenericCopyAddr);
  }
}

bool GASResolving::checkGenericArguments(Function &F) const {
  // check that we have a pointer to pointer or pointer to struct that has pointer elements
  // and main pointer type is global while underlying pointer type is generic

  auto *FT = F.getFunctionType();
  for (unsigned p = 0; p < FT->getNumParams(); ++p) {
    if (auto Ty = dyn_cast<PointerType>(FT->getParamType(p))) {
      if (Ty->getAddressSpace() != ADDRESS_SPACE_GLOBAL)
        continue;
      auto PteeTy = IGCLLVM::getArgAttrEltTy(F.getArg(p));
      if (PteeTy == nullptr && !IGCLLVM::isPointerTy(Ty))
        PteeTy = IGCLLVM::getNonOpaquePtrEltTy(Ty); // Legacy code: getNonOpaquePtrEltTy
      if (PteeTy == nullptr)
        // go to slow path
        return true;
      if (auto PTy = dyn_cast<PointerType>(PteeTy)) {
        if (PTy->getAddressSpace() == ADDRESS_SPACE_GENERIC)
          return true;
      }
      if (auto STy = dyn_cast<StructType>(PteeTy)) {
        for (unsigned e = 0; e < STy->getNumElements(); ++e) {
          if (auto ETy = dyn_cast<PointerType>(STy->getElementType(e))) {
            if (ETy->getAddressSpace() == ADDRESS_SPACE_GENERIC)
              return true;
          }
        }
      }
    }
  }
  return false;
}
