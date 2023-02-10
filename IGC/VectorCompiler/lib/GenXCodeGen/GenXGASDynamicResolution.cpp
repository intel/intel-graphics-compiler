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
///   2. Resolves loads/stores from generic memory to loads/store to a
///   local/global
/// memory using this tag.
///   3. For loads/stores which pointer can be resolved only into a pointer to a
///   global memory
/// AND masked.gather/masked.scatter instrinsics cast generic ptrs to global
/// ones.
///   4. Lowers vc.internal.to.private.explicit, vc.internal.to.local.explicit,
/// vc.internal.to.global.explicit intrinsics.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXGASCastAnalyzer.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"
#include "vc/Utils/General/IRBuilder.h"
#include "vc/Utils/General/InstRebuilder.h"
#include "vc/Utils/General/Types.h"

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
  Value *isLocal(IGCLLVM::IRBuilder<> &IRB, Value *PtrOp) const;
  Value *isPrivate(IGCLLVM::IRBuilder<> &IRB, Value *PtrOp) const;
};

} // end namespace

static Value *createASCast(IGCLLVM::IRBuilder<> &IRB, Value *PtrOp, int NewAS) {
  auto PtrOrPtrVecTy = PtrOp->getType();
  auto NewPtrOrPtrVecTy = vc::changeAddrSpace(PtrOrPtrVecTy, NewAS);
  return IRB.CreateAddrSpaceCast(PtrOp, NewPtrOrPtrVecTy);
}

static Value *getHigh32BitOfPtr(LLVMContext &C, IGCLLVM::IRBuilder<> &IRB,
                                Value *PtrOp) {
  auto PtrAsI64 = IRB.CreatePtrToInt(PtrOp, IRB.getInt64Ty());
  auto PtrAsV32I64 = IRB.CreateBitCast(
      PtrAsI64, IGCLLVM::FixedVectorType::get(IRB.getInt32Ty(), 2));
  return IRB.CreateExtractElement(PtrAsV32I64, 1);
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

// Resolve ptrs for masked.scatter/masked.gather. Masked intrinsics cannot be
// lowered for addrspace(3) pointers so far. So, convert all generic pointers to
// global ones.
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
    Instruction* NewInst = nullptr;
    auto GlobalPtrOp = createASCast(Builder, PtrOp, vc::AddrSpace::Global);
    if(IntrinsicID == Intrinsic::masked_gather) {
      auto Align = I.getArgOperand(1);
      auto Mask = I.getArgOperand(2);
      auto Passthru = I.getArgOperand(3);
      NewInst =
          vc::cloneInstWithNewOps(I, {GlobalPtrOp, Align, Mask, Passthru});
    } else if (IntrinsicID == Intrinsic::masked_scatter) {
      auto Val = I.getArgOperand(0);
      auto Align = I.getArgOperand(2);
      auto Mask = I.getArgOperand(3);
      NewInst =
          vc::cloneInstWithNewOps(I, {Val, GlobalPtrOp, Align, Mask});
    }
    NewInst->insertBefore(&I);
    NewInst->takeName(&I);
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
  auto PtrOp = CI.getPointerOperand();
  // TODO: Support vector of pointers.
  if (PtrOp->getType()->isVectorTy())
    return;

  // TODO: Set tag for private pointers.
  if (CI.getSrcAddressSpace() == vc::AddrSpace::Local &&
      CI.getDestAddressSpace() == vc::AddrSpace::Generic) {
    IGCLLVM::IRBuilder<> Builder{&CI};
    Type *I32Ty = Builder.getInt32Ty();
    Type *I64Ty = Builder.getInt64Ty();
    auto LocalTagShifted = Builder.getInt32(LocalTag << 29);

    auto PtrAsInt = Builder.CreatePtrToInt(PtrOp, I64Ty);
    PtrAsInt = Builder.CreateBitCast(PtrAsInt,
                                     IGCLLVM::FixedVectorType::get(I32Ty, 2));
    auto TaggedInt = Builder.CreateInsertElement(PtrAsInt, LocalTagShifted, 1);
    TaggedInt = Builder.CreateBitCast(TaggedInt, I64Ty);
    auto TaggedPtr = Builder.CreateIntToPtr(TaggedInt, PtrOp->getType());
    auto NewASCast = Builder.CreateAddrSpaceCast(TaggedPtr, CI.getType());

    NewASCast->takeName(&CI);
    CI.replaceAllUsesWith(NewASCast);
    CI.eraseFromParent();
  }
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
  auto IsLocalTag = isLocal(Builder, PtrOp);
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
    CmpTag = isPrivate(Builder, PtrOp);
  } else if (TargetAS == vc::AddrSpace::Local) {
    if (!CanLocalBeGeneric)
      return PtrNull;
    CmpTag = isLocal(Builder, PtrOp);
  } else if (TargetAS == vc::AddrSpace::Global) {
    if (!CanGlobalBeGeneric)
      return PtrNull;
    auto isPrivateTag =
        CanPrivateBeGeneric ? isPrivate(Builder, PtrOp) : Builder.getFalse();
    auto isLocalTag =
        CanLocalBeGeneric ? isLocal(Builder, PtrOp) : Builder.getFalse();
    auto isPrivateOrLocal = Builder.CreateOr(isPrivateTag, isLocalTag);
    CmpTag = Builder.CreateICmpEQ(isPrivateOrLocal, Builder.getFalse());
  } else
    IGC_ASSERT(0 && "Unimplemented GenericCastToPtr intrinsic");
  auto ASCast = createASCast(Builder, PtrOp, TargetAS);
  return Builder.CreateSelect(CmpTag, ASCast, PtrNull);
}

Value *GenXGASDynamicResolution::isLocal(IGCLLVM::IRBuilder<> &IRB,
                                         Value *PtrOp) const {
  auto High32Bit = getHigh32BitOfPtr(PtrOp->getContext(), IRB, PtrOp);
  auto ShifedTag = IRB.getInt32(LocalTag << 29);
  return IRB.CreateICmpEQ(High32Bit, ShifedTag, "isLocalTag");
}

Value *GenXGASDynamicResolution::isPrivate(IGCLLVM::IRBuilder<> &IRB,
                                           Value *PtrOp) const {
  auto High32Bit = getHigh32BitOfPtr(PtrOp->getContext(), IRB, PtrOp);
  auto ShifedTag = IRB.getInt32(PrivateTag << 29);

  // Clear nonzero [0..29]bits of a high part of a pointer.
  const unsigned TagMask = (7 << 29);
  auto High32BitCl = IRB.CreateAnd(High32Bit, IRB.getInt32(TagMask));
  return IRB.CreateICmpEQ(High32BitCl, ShifedTag, "isPrivateTag");
}
