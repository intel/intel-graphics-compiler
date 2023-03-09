/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//
/// GenXGASDynamicResolution
/// ---------------------------
///
/// GenXGASDynamicResolution is a module pass which resolves pointers to generic
/// addrspace (GAS) to pointers to local/global memory.
///
/// The pass:
///   1. The pass attached tag to an every local pointer for the
/// local->generic conversions.
///   2. Resolves loads/stores/masked.gather/masked.scatter from generic memory
/// to loads/store/masked.gather/masked.scatter to a local/global memory
/// using this tag.
///   3. For loads/stores/masked.gather/masked.scatter which pointer can be
/// resolved only into a pointer to a global memory cast generic ptrs to global
/// ones.
///   4. Lowers vc.internal.to.private.explicit, vc.internal.to.local.explicit,
/// vc.internal.to.global.explicit intrinsics.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXGASCastAnalyzer.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"
#include "vc/Utils/General/InstRebuilder.h"
#include "vc/Utils/General/Types.h"

#include "llvmWrapper/IR/Constants.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvmWrapper/Support/Alignment.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>

#define DEBUG_TYPE "GENX_GASDYNAMICRESOLUTION"

using namespace llvm;
using namespace genx;

namespace {

class GenXGASDynamicResolution : public FunctionPass,
                                 public InstVisitor<GenXGASDynamicResolution> {
  bool CanLocalBeGeneric = false;
  bool CanPrivateBeGeneric = false;
  bool CanGlobalBeGeneric = false;
  const unsigned PrivateTag = 1; // tag 001.
  const unsigned LocalTag = 2; // tag 010.
public:
  static char ID;
  explicit GenXGASDynamicResolution() : FunctionPass(ID) {}
  StringRef getPassName() const override {
    return "GenX GAS dynamic resolution";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXGASCastWrapper>();
  }
  bool runOnFunction(Function &F) override;
public:
  void visitAddrSpaceCastInst(AddrSpaceCastInst &CI) const;
  // visitCallInst is overridden since visitIntrinsicInst will not be
  // invoked for vc internal intrinsics.
  void visitCallInst(CallInst &CI) const;
  void visitIntrinsicInst(IntrinsicInst &Intrinsic) const;
  void visitLoadInst(LoadInst &LdI) const;
  void visitStoreInst(StoreInst &StI) const;
private:
  void resolveOnLoadStore(Instruction &I, Value *PtrOp) const;
  Value *lowerGenericCastToPtr(IntrinsicInst &Intrinsic) const;

  // Check if generic points to local/private.
  Value *isLocal(IGCLLVM::IRBuilder<> &IRB, Value *PtrOp, Module *M) const;
  Value *isPrivate(IGCLLVM::IRBuilder<> &IRB, Value *PtrOp, Module *M) const;
};

} // end namespace

static Value *createASCast(IGCLLVM::IRBuilder<> &IRB, Value *PtrOp,
                           unsigned NewAS) {
  auto PtrOrPtrVTy = PtrOp->getType();
  auto NewPtrOrPtrVTy = vc::changeAddrSpace(PtrOrPtrVTy, NewAS);
  return IRB.CreateAddrSpaceCast(PtrOp, NewPtrOrPtrVTy);
}

static Value *getPtrAsVectorOfI32(IGCLLVM::IRBuilder<> &IRB, Value *PtrOp) {
  auto PtrOpTy = PtrOp->getType();
  Type *Ptr2IntTy = nullptr;
  Type *BitCastTy = nullptr;
  if (PtrOpTy->isVectorTy()) {
    auto NElems =
        cast<IGCLLVM::FixedVectorType>(PtrOp->getType())->getNumElements();
    Ptr2IntTy = IGCLLVM::FixedVectorType::get(IRB.getInt64Ty(), NElems);
    BitCastTy = IGCLLVM::FixedVectorType::get(IRB.getInt32Ty(), 2 * NElems);
  } else {
    Ptr2IntTy = IRB.getInt64Ty();
    BitCastTy = IGCLLVM::FixedVectorType::get(IRB.getInt32Ty(), 2);
  }
  auto Ptr2Int = IRB.CreatePtrToInt(PtrOp, Ptr2IntTy);
  return IRB.CreateBitCast(Ptr2Int, BitCastTy);
}

static Value *readHigh32BitsOfPtr(IGCLLVM::IRBuilder<> &IRB, Value *PtrOp,
                                  Module *M) {
  auto PtrOpTy = PtrOp->getType();
  auto PtrAsIntVec = getPtrAsVectorOfI32(IRB, PtrOp);

  if (!PtrOpTy->isVectorTy())
    return IRB.CreateExtractElement(PtrAsIntVec, 1);

  auto NElems = cast<IGCLLVM::FixedVectorType>(PtrOpTy)->getNumElements();
  auto ExtractVTy = IGCLLVM::FixedVectorType::get(IRB.getInt32Ty(), NElems);
  auto DataVTy = PtrAsIntVec->getType();

  auto RdRgnFunc = GenXIntrinsic::getGenXDeclaration(
      M, GenXIntrinsic::genx_rdregioni,
      {ExtractVTy, DataVTy, IRB.getInt16Ty()});
  SmallVector<Value *, 6> Args = {
      PtrAsIntVec,                      // vector to read region from
      IRB.getInt32(2),                  // vstride
      IRB.getInt32(1),                  // width
      IRB.getInt32(0),                  // stride
      IRB.getInt16(4),                  // offset in bytes
      UndefValue::get(IRB.getInt32Ty()) // parent width, ignored
  };
  return IRB.CreateCall(RdRgnFunc, Args);
}

static Value *writeHigh32BitsOfPtr(IGCLLVM::IRBuilder<> &IRB, Value *PtrOp,
                                   Value *Val, Module *M) {
  auto PtrOpTy = PtrOp->getType();
  auto PtrAsIntVec = getPtrAsVectorOfI32(IRB, PtrOp);

  if (!PtrOpTy->isVectorTy()) {
    auto TaggedInt = IRB.CreateInsertElement(PtrAsIntVec, Val, 1);
    TaggedInt = IRB.CreateBitCast(TaggedInt, IRB.getInt64Ty());
    return IRB.CreateIntToPtr(TaggedInt, PtrOp->getType());
  }

  auto WrRgnFunc =
      GenXIntrinsic::getAnyDeclaration(M, GenXIntrinsic::genx_wrregioni,
                                       {PtrAsIntVec->getType(), Val->getType(),
                                        IRB.getInt16Ty(), IRB.getInt1Ty()});
  SmallVector<Value *, 8> Args = {
      PtrAsIntVec, // vector to write region to
      Val,
      IRB.getInt32(2),                   // vstride
      IRB.getInt32(1),                   // width
      IRB.getInt32(0),                   // stride
      IRB.getInt16(4),                   // offset in bytes
      UndefValue::get(IRB.getInt32Ty()), // parent width, ignored
      IRB.getTrue()                      // mask
  };
  // Convert result to a correct ptr type.
  auto NElems = cast<IGCLLVM::FixedVectorType>(PtrOpTy)->getNumElements();
  auto BitCastTy = IGCLLVM::FixedVectorType::get(IRB.getInt64Ty(), NElems);
  Value *TaggedIntVec = IRB.CreateCall(WrRgnFunc, Args);
  TaggedIntVec = IRB.CreateBitCast(TaggedIntVec, BitCastTy);
  return IRB.CreateIntToPtr(TaggedIntVec, PtrOp->getType());
}

static void createScatterWithNewAS(IntrinsicInst &OldScatter,
                                   IGCLLVM::IRBuilder<> &IRB, unsigned NewAS,
                                   Value *UpdateMask = nullptr) {
  auto Val = OldScatter.getArgOperand(0);
  auto PtrOp = OldScatter.getArgOperand(1);
  auto Align = OldScatter.getArgOperand(2);
  auto Mask = OldScatter.getArgOperand(3);

  if (UpdateMask)
    Mask = IRB.CreateAnd(UpdateMask, Mask);
  PtrOp = createASCast(IRB, PtrOp, NewAS);

  auto Func = Intrinsic::getDeclaration(OldScatter.getModule(),
                                        Intrinsic::masked_scatter,
                                        {Val->getType(), PtrOp->getType()});
  IRB.CreateCall(Func, {Val, PtrOp, Align, Mask});
}

static IntrinsicInst *createGatherWithNewAS(IntrinsicInst &OldGather,
                                            IGCLLVM::IRBuilder<> &IRB,
                                            unsigned NewAS, const Twine &Name,
                                            Value *UpdateMask = nullptr) {
  auto PtrOp = OldGather.getArgOperand(0);
  auto Align = OldGather.getArgOperand(1);
  auto Mask = OldGather.getArgOperand(2);
  auto Passthru = OldGather.getArgOperand(3);

  if (UpdateMask)
    Mask = IRB.CreateAnd(UpdateMask, Mask);
  PtrOp = createASCast(IRB, PtrOp, NewAS);

  auto Func =
      Intrinsic::getDeclaration(OldGather.getModule(), Intrinsic::masked_gather,
                                {OldGather.getType(), PtrOp->getType()});
  return cast<IntrinsicInst>(
      IRB.CreateCall(Func, {PtrOp, Align, Mask, Passthru}, Name));
}

char GenXGASDynamicResolution::ID = 0;
namespace llvm {
void initializeGenXGASDynamicResolutionPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXGASDynamicResolution, "GenXGASDynamicResolution",
                      "GenXGASDynamicResolution", false, false)
INITIALIZE_PASS_END(GenXGASDynamicResolution, "GenXGASDynamicResolution",
                    "GenXGASDynamicResolution", false, false)
FunctionPass *llvm::createGenXGASDynamicResolutionPass() {
  initializeGenXGASDynamicResolutionPass(*PassRegistry::getPassRegistry());
  return new GenXGASDynamicResolution;
}

bool GenXGASDynamicResolution::runOnFunction(Function &F) {
  auto &M = *F.getParent();
  // Cannot resolve 32bit pointers.
  if (M.getDataLayout().getPointerSizeInBits(vc::AddrSpace::Generic) != 64)
    return false;

  GASInfo &GI = getAnalysis<GenXGASCastWrapper>().getGASInfo();
  CanPrivateBeGeneric = GI.canGenericPointToPrivate(F);
  CanLocalBeGeneric = GI.canGenericPointToLocal(F);
  CanGlobalBeGeneric = GI.canGenericPointToGlobal(F);

  // Save list of BBs before iterating because visit() can create new BBs.
  SmallVector<BasicBlock*, 8> BBs;
  for (auto bi = F.begin(), be = F.end(); bi != be; ++bi)
    BBs.push_back(&*bi);

  for (auto bi = BBs.begin(), be = BBs.end(); bi != be; ++bi) {
    BasicBlock *BB = *bi;
    for (auto ii = BB->begin(), ie = BB->end(); ii != ie;) {
      Instruction *Inst = &*ii;
      ii++;
      visit(Inst);
      // Parent of the NextInst could change after visit().
      // In that case check for BB->end() is not valid.
      Instruction *NextInst = &*ii;
      Instruction *End = &ii->getParent()->back();
      if(NextInst == End)
        break;
    }
  }
  return true;
}

void GenXGASDynamicResolution::visitLoadInst(LoadInst &LdI) const {
  auto PtrOp = LdI.getPointerOperand();
  auto AS = vc::getAddrSpace(PtrOp->getType());
  if(AS != vc::AddrSpace::Generic)
    return;

  if (!CanLocalBeGeneric) {
    IGCLLVM::IRBuilder<> Builder{&LdI};
    auto GlobalPtrOp = createASCast(Builder, PtrOp, vc::AddrSpace::Global);
    auto NewInst = Builder.CreateAlignedLoad(GlobalPtrOp, IGCLLVM::getAlign(LdI),
                                      LdI.isVolatile(), "globalOrPrivateLoad");
    LdI.replaceAllUsesWith(NewInst);
    LdI.eraseFromParent();
  } else
    resolveOnLoadStore(LdI, PtrOp);
}

void GenXGASDynamicResolution::visitStoreInst(StoreInst &StI) const {
  auto PtrOp = StI.getPointerOperand();
  auto AS = vc::getAddrSpace(PtrOp->getType());
  if(AS != vc::AddrSpace::Generic)
    return;

  if (!CanLocalBeGeneric) {
    IGCLLVM::IRBuilder<> Builder{&StI};
    auto GlobalPtrOp = createASCast(Builder, PtrOp, vc::AddrSpace::Global);
    Builder.CreateAlignedStore(StI.getValueOperand(), GlobalPtrOp,
                               IGCLLVM::getAlign(StI), StI.isVolatile());
    StI.eraseFromParent();
  } else
    resolveOnLoadStore(StI, PtrOp);
}

void GenXGASDynamicResolution::visitCallInst(CallInst &CI) const {
  const Function *Callee = CI.getCalledFunction();
  if (!Callee)
    return;
  unsigned IntrinsicID = vc::getAnyIntrinsicID(Callee);
  switch (IntrinsicID) {
  case vc::InternalIntrinsic::cast_to_ptr_explicit:
    CI.replaceAllUsesWith(lowerGenericCastToPtr(cast<IntrinsicInst>(CI)));
    CI.eraseFromParent();
    break;
  default:
    break;
  }
}

// Resolve generic ptrs for masked.scatter/masked.gather:
//  1. If no local->generic casts exists then convert all geneirc ptrs to global
//  ones.
//  2. Otherwise, create intrinsic copy with local and global addrspace for its
//  ptrs vec.
void GenXGASDynamicResolution::visitIntrinsicInst(IntrinsicInst &I) const {
  unsigned IntrinsicID = vc::getAnyIntrinsicID(&I);
  switch (IntrinsicID) {
  case Intrinsic::masked_gather:
  case Intrinsic::masked_scatter: {
    auto PtrOp = I.getArgOperand(0);
    if(IntrinsicID == Intrinsic::masked_scatter)
      PtrOp = I.getArgOperand(1);
    unsigned AS = vc::getAddrSpace(PtrOp->getType());
    if(AS != vc::AddrSpace::Generic)
      break;

    IGCLLVM::IRBuilder<> Builder{&I};
    Value *NewInst = nullptr;
    if (!CanLocalBeGeneric) {
      if (IntrinsicID == Intrinsic::masked_gather)
        NewInst = createGatherWithNewAS(I, Builder, vc::AddrSpace::Global,
                                        I.getName());
      else
        createScatterWithNewAS(I, Builder, vc::AddrSpace::Global);
    } else {
      auto LocalMask = isLocal(Builder, PtrOp, I.getModule());
      auto GlobalMask = Builder.CreateNot(LocalMask);
      if (IntrinsicID == Intrinsic::masked_gather) {
        auto Name = I.getName();
        auto LocalGather = createGatherWithNewAS(
            I, Builder, vc::AddrSpace::Local, Name + ".local", LocalMask);
        auto GlobalGather = createGatherWithNewAS(
            I, Builder, vc::AddrSpace::Global, Name + ".global", GlobalMask);
        NewInst = Builder.CreateSelect(LocalMask, LocalGather, GlobalGather);
      } else {
        createScatterWithNewAS(I, Builder, vc::AddrSpace::Local, LocalMask);
        createScatterWithNewAS(I, Builder, vc::AddrSpace::Global, GlobalMask);
      }
    }
    if (IntrinsicID == Intrinsic::masked_gather)
      I.replaceAllUsesWith(NewInst);
    I.eraseFromParent();
    break;
  }
  default:
    break;
  }
}

// Add a tag to high part of GAS before Local->Generic conversion.
// For PtrToLocal: [Low32, High32] - only Low32 will be used as
// actual address, so spoil High32 part with a tag.
void GenXGASDynamicResolution::visitAddrSpaceCastInst(
    AddrSpaceCastInst &CI) const {
  // TODO: Set tag for private pointers.
  if (CI.getSrcAddressSpace() != vc::AddrSpace::Local ||
      CI.getDestAddressSpace() != vc::AddrSpace::Generic)
    return;

  IGCLLVM::IRBuilder<> Builder{&CI};
  auto PtrOp = CI.getPointerOperand();
  auto PtrOpTy = PtrOp->getType();
  Constant *ShiftedTag = Builder.getInt32(LocalTag << 29);
  if (PtrOpTy->isVectorTy()) {
    unsigned NElems = cast<IGCLLVM::FixedVectorType>(PtrOpTy)->getNumElements();
    ShiftedTag = IGCLLVM::ConstantFixedVector::getSplat(NElems, ShiftedTag);
  }

  auto TaggedPtr =
      writeHigh32BitsOfPtr(Builder, PtrOp, ShiftedTag, CI.getModule());
  auto NewASCast = Builder.CreateAddrSpaceCast(TaggedPtr, CI.getType());
  NewASCast->takeName(&CI);
  CI.replaceAllUsesWith(NewASCast);
  CI.eraseFromParent();
}

// Resolve GAS in load/store by dynamic information(tag) which is attached to
// the pointer.
void GenXGASDynamicResolution::resolveOnLoadStore(Instruction &I,
                                                  Value *PtrOp) const {
  IGCLLVM::IRBuilder<> Builder{&I};
  auto CurrentBlock = I.getParent();
  auto ConvergeBlock = CurrentBlock->splitBasicBlock(&I);
  Value *LocalLoad = nullptr;
  Value *GlobalLoad = nullptr;
  auto createBlock = [&](const Twine &BlockName, const Twine &LoadName, int AS,
                         Value *&Load) {
    BasicBlock *BB = BasicBlock::Create(
        I.getContext(), BlockName, ConvergeBlock->getParent(), ConvergeBlock);
    Builder.SetInsertPoint(BB);
    auto NewPtrOp = createASCast(Builder, PtrOp, AS);
    if (LoadInst *LI = dyn_cast<LoadInst>(&I))
      Load = Builder.CreateAlignedLoad(NewPtrOp, IGCLLVM::getAlign(*LI),
                                       LI->isVolatile(), LoadName);
    else if (StoreInst *SI = dyn_cast<StoreInst>(&I))
      Builder.CreateAlignedStore(I.getOperand(0), NewPtrOp,
                                 IGCLLVM::getAlign(*SI), SI->isVolatile());
    Builder.CreateBr(ConvergeBlock);
    return BB;
  };
  // Local Branch.
  BasicBlock *LocalBlock =
      createBlock("LocalBlock", "localLoad", vc::AddrSpace::Local, LocalLoad);
  // Global Branch.
  BasicBlock *GlobalBlock = createBlock("GlobalBlock", "globalLoad",
                                        vc::AddrSpace::Global, GlobalLoad);
  Builder.SetInsertPoint(CurrentBlock->getTerminator());

  // Branch to global/local block based on tag.
  auto IsLocalTag = isLocal(Builder, PtrOp, I.getModule());
  Builder.CreateCondBr(IsLocalTag, LocalBlock, GlobalBlock);
  CurrentBlock->getTerminator()->eraseFromParent();

  // Update load uses.
  if (isa<LoadInst>(&I)) {
    IGCLLVM::IRBuilder<> PhiBuilder(&(*ConvergeBlock->begin()));
    PHINode *PHI = PhiBuilder.CreatePHI(I.getType(), 2, I.getName());
    PHI->addIncoming(LocalLoad, LocalBlock);
    PHI->addIncoming(GlobalLoad, GlobalBlock);
    PHI->takeName(&I);
    I.replaceAllUsesWith(PHI);
  }
  I.eraseFromParent();
}

Value *GenXGASDynamicResolution::lowerGenericCastToPtr(IntrinsicInst &I) const {
  IGCLLVM::IRBuilder<> Builder{&I};
  auto M = I.getModule();
  auto TargetTy = I.getType();
  auto TargetAS = vc::getAddrSpace(TargetTy);
  auto PtrNull = Constant::getNullValue(TargetTy);
  auto PtrOp = I.getArgOperand(0);

  // Two cases:
  // 1: Generic pointer's AS matches with instrinsic's target AS
  //    So we create the address space cast.
  // 2: Generic pointer's AS does not match with instrinsic's target AS
  //    So the instrinsic call returns NULL.
  Value *CmpTag = nullptr;
  if (TargetAS == vc::AddrSpace::Private) {
    if (!CanPrivateBeGeneric)
      return PtrNull;
    CmpTag = isPrivate(Builder, PtrOp, M);
  } else if (TargetAS == vc::AddrSpace::Local) {
    if (!CanLocalBeGeneric)
      return PtrNull;
    CmpTag = isLocal(Builder, PtrOp, M);
  } else if (TargetAS == vc::AddrSpace::Global) {
    if (!CanGlobalBeGeneric)
      return PtrNull;
    auto isPrivateTag =
        CanPrivateBeGeneric ? isPrivate(Builder, PtrOp, M) : Builder.getFalse();
    auto isLocalTag =
        CanLocalBeGeneric ? isLocal(Builder, PtrOp, M) : Builder.getFalse();
    auto isPrivateOrLocal = Builder.CreateOr(isPrivateTag, isLocalTag);
    CmpTag = Builder.CreateICmpEQ(isPrivateOrLocal, Builder.getFalse());
  } else
    IGC_ASSERT(0 && "Unimplemented GenericCastToPtr intrinsic");
  auto ASCast = createASCast(Builder, PtrOp, TargetAS);
  return Builder.CreateSelect(CmpTag, ASCast, PtrNull);
}

// Check whether a ptr/ptrs from ptrvec contain a local tag in the high bits.
// Returning value is true/false for a ptr, mask for a ptrvec.
Value *GenXGASDynamicResolution::isLocal(IGCLLVM::IRBuilder<> &IRB,
                                         Value *PtrOp, Module *M) const {
  Constant *ShiftedTag = IRB.getInt32(LocalTag << 29);
  auto PtrOpTy = PtrOp->getType();
  if (PtrOpTy->isVectorTy()) {
    auto NElems = cast<IGCLLVM::FixedVectorType>(PtrOpTy)->getNumElements();
    ShiftedTag = IGCLLVM::ConstantFixedVector::getSplat(NElems, ShiftedTag);
  }
  auto High32Bits = readHigh32BitsOfPtr(IRB, PtrOp, M);
  return IRB.CreateICmpEQ(High32Bits, ShiftedTag, "isLocalTag");
}

// Check whether a ptr/ptrs from ptrvec contain a private tag in the high bits.
// Returning value is true/false for a ptr, mask for a ptrvec.
Value *GenXGASDynamicResolution::isPrivate(IGCLLVM::IRBuilder<> &IRB,
                                           Value *PtrOp, Module *M) const {
  Constant *TagMask = IRB.getInt32(7 << 29);
  Constant *ShiftedTag = IRB.getInt32(PrivateTag << 29);
  auto PtrOpTy = PtrOp->getType();
  if (PtrOpTy->isVectorTy()) {
    auto NElems = cast<IGCLLVM::FixedVectorType>(PtrOpTy)->getNumElements();
    ShiftedTag = IGCLLVM::ConstantFixedVector::getSplat(NElems, ShiftedTag);
    TagMask = IGCLLVM::ConstantFixedVector::getSplat(NElems, TagMask);
  }

  auto High32Bits = readHigh32BitsOfPtr(IRB, PtrOp, M);
  // Clear nonzero [0..29]bits of a high part of a pointer.
  auto High32BitsClr = IRB.CreateAnd(High32Bits, TagMask);
  return IRB.CreateICmpEQ(High32BitsClr, ShiftedTag, "isPrivateTag");
}
