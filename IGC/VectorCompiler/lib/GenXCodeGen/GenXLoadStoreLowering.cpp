/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXLoadStoreLowering
/// ---------------------------
///
/// The pass:
/// * replaces all LLVM loads and stores, using correct namespace.
/// * replaces allocas for historical reasons.
/// * removes lifetime builtins as we are not sure how to process those.
///
/// Rules of replacement are really simple:
/// addrspace(1) load goes to svm.gather
/// addrspace(1) store goes to svm.scatter
///
/// Other loads and stores behave like addrspace(1) but in future more complex
/// processing will be added (like scatter.scaled for addrspace(0), etc)
//
//===----------------------------------------------------------------------===//

#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/BreakConst.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/Alignment.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/GenXIntrinsics/GenXMetadata.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Transforms/Utils/Local.h>

#define DEBUG_TYPE "genx-ls-lowering"

using namespace llvm;
using namespace genx;

static cl::opt<bool> EnableLL("enable-ldst-lowering", cl::init(true),
                              cl::Hidden,
                              cl::desc("Enable Load-Store lowering pass"));

namespace {

enum class Atomicity : bool { Atomic = true, NonAtomic = false };

// Define which intrinsics to use: legacy ones (svm.scatter, gather.scaled, ...)
// or LSC ones (lsc.store.stateless, lsc.store.slm, ...).
enum class MessageKind : char {
  Legacy,
  LSC,
};

enum class HWAddrSpace : char {
  A32, // Global memory (SVM), addressed with 32-bit pointers.
  A64, // Global memory (SVM), addressed with 64-bit pointers.
  SLM, // Shared local memory.
};

// load and store lowering pass
class GenXLoadStoreLowering : public FunctionPass,
                              public InstVisitor<GenXLoadStoreLowering> {
  const DataLayout *DL_ = nullptr;

private:
  Value *ZExtOrTruncIfNeeded(Value *From, Type *To,
                             Instruction *InsertBefore) const;
  std::pair<Value *, unsigned> NormalizeVector(Value *From, Type *To,
                                               Instruction *InsertBefore,
                                               IRBuilder<> &Builder) const;
  Instruction *RestoreVectorAfterNormalization(Instruction *From,
                                               Type *To) const;
  Value *NormalizeFuncPtrVec(Value *V, Instruction *InsPoint) const;
  IGCLLVM::FixedVectorType *getLoadVType(const LoadInst &LdI) const;
  Value *splatStoreIfNeeded(StoreInst &StI, IRBuilder<> &Builder) const;
  Instruction *extractFirstElement(Instruction &ProperGather, Type &LdTy,
                                   IRBuilder<> &Builder) const;

private:
  // Creates replacement (series of instructions) for the provided memory
  // instruction \p I. Considers all the required properties of the instruction
  // and the target, and generates the proper intrinsic and some supporting
  // insturctions. A replacement for the provided instruction is returned (smth
  // that can be used in RAUW).
  template <typename MemoryInstT>
  Instruction *createMemoryInstReplacement(MemoryInstT &I) const;

  // Methods to switch through all the possible memory operations and
  // corresponding VC-intrinsics for them. Branching goes in the following
  // order: instruction (load, store, ...) -> atomicity -> message kind
  // (legacy, lsc) -> HW addrspace.
  template <typename MemoryInstT>
  Instruction *switchAtomicity(MemoryInstT &I) const;
  template <Atomicity A, typename MemoryInstT>
  Instruction *switchMessage(MemoryInstT &I) const;
  template <MessageKind MK, Atomicity A, typename MemoryInstT>
  Instruction *switchAddrSpace(MemoryInstT &I) const;

  // Creates a replacement for \p I instruction. The template parameters
  // describe the provided instruction and how it should be lowered.
  template <HWAddrSpace HWAS, MessageKind MK, Atomicity A, typename MemoryInstT>
  Instruction *createIntrinsic(MemoryInstT &I) const;

  Instruction *createSVMGather(LoadInst &LdI) const;
  Instruction *createSVMGatherImpl(LoadInst &LdI, Value &NormalizedOldVal,
                                   unsigned ValueEltSz,
                                   IRBuilder<> &Builder) const;
  Instruction *createSVMScatter(StoreInst &StI) const;
  Instruction *createSVMScatterImpl(StoreInst &StI, Value &NormalizedOldVal,
                                    unsigned ValueEltSz,
                                    IRBuilder<> &Builder) const;

public:
  void visitStoreInst(StoreInst &StI) const;
  void visitLoadInst(LoadInst &LdI) const;
  void visitIntrinsicInst(IntrinsicInst &Intrinsic) const;

public:
  static char ID;
  explicit GenXLoadStoreLowering() : FunctionPass(ID) {}
  StringRef getPassName() const override { return "GenX load store lowering"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
};

} // end namespace

char GenXLoadStoreLowering::ID = 0;
namespace llvm {
void initializeGenXLoadStoreLoweringPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXLoadStoreLowering, "GenXLoadStoreLowering",
                      "GenXLoadStoreLowering", false, false)
INITIALIZE_PASS_END(GenXLoadStoreLowering, "GenXLoadStoreLowering",
                    "GenXLoadStoreLowering", false, false)

FunctionPass *llvm::createGenXLoadStoreLoweringPass() {
  initializeGenXLoadStoreLoweringPass(*PassRegistry::getPassRegistry());
  return new GenXLoadStoreLowering;
}

void GenXLoadStoreLowering::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<GenXBackendConfig>();
  AU.setPreservesCFG();
}

bool GenXLoadStoreLowering::runOnFunction(Function &F) {
  // pass might be switched off
  if (!EnableLL)
    return false;

  LLVM_DEBUG(dbgs() << "GenXLoadStoreLowering started\n");

  auto &M = *F.getParent();
  DL_ = &M.getDataLayout();
  auto &ST = getAnalysis<TargetPassConfig>()
                 .getTM<GenXTargetMachine>()
                 .getGenXSubtarget();
  // auto &BEConf = getAnalysis<GenXBackendConfig>();
  // BEConf.getStatelessPrivateMemSize() will be required

  // pass don't work for CMRT for now, legacy TPM will be used
  if (!ST.isOCLRuntime())
    return false;

  // see visitXXInst members for main logic:
  //   * visitStoreInst
  //   * visitLoadInst
  //   * visitAllocaInst
  //   * visitIntrinsicInst
  visit(F);

  return true;
}

Value *
GenXLoadStoreLowering::ZExtOrTruncIfNeeded(Value *From, Type *ToTy,
                                           Instruction *InsertBefore) const {
  IRBuilder<> Builder(InsertBefore);
  Type *FromTy = From->getType();
  if (FromTy == ToTy)
    return From;

  unsigned FromTySz = DL_->getTypeSizeInBits(FromTy);
  unsigned ToTySz = DL_->getTypeSizeInBits(ToTy);
  Value *Res = From;
  if (auto *FromVTy = dyn_cast<IGCLLVM::FixedVectorType>(FromTy);
      FromVTy && FromVTy->getNumElements() == 1) {
    auto *TmpRes = CastInst::CreateBitOrPointerCast(
        Res, FromVTy->getElementType(), "", InsertBefore);
    Res = TmpRes;
  }
  if (auto *ToVTy = dyn_cast<IGCLLVM::FixedVectorType>(ToTy))
    Res = Builder.CreateVectorSplat(ToVTy->getNumElements(), Res,
                                    Res->getName() + ".splat");
  if (FromTySz < ToTySz)
    Res = CastInst::CreateZExtOrBitCast(Res, ToTy, "", InsertBefore);
  else if (FromTySz > ToTySz)
    Res = CastInst::CreateTruncOrBitCast(Res, ToTy, "", InsertBefore);
  return Res;
}

// --- MAGIC LEGACY ---
// If data is a vector of double/int64, bitcast each element to 2 int32.
// If data is a vector of function pointers, strip all internal bitcasts
// and possible extractelems (64->8xi8 cast case) to get a vector of int64s.
// If data is a vector of type < 32bit, extend each element in order to create
// proper send instruction in the finalizer.
//
// returned size almost always size of element of returned data
// except 8/16 case shrink
std::pair<Value *, unsigned>
GenXLoadStoreLowering::NormalizeVector(Value *From, Type *To, Instruction *Inst,
                                       IRBuilder<> &Builder) const {
  Type *I32Ty = Builder.getInt32Ty();
  Type *I64Ty = Builder.getInt64Ty();
  Value *Res = From;
  Type *FromTy = From->getType();
  IGC_ASSERT(isa<VectorType>(FromTy));
  unsigned EltSz =
      DL_->getTypeSizeInBits(FromTy->getScalarType()) / genx::ByteBits;
  IGC_ASSERT(EltSz > 0);
  unsigned NumElts = cast<IGCLLVM::FixedVectorType>(FromTy)->getNumElements();
  if (isFuncPointerVec(From) &&
      DL_->getTypeSizeInBits(From->getType()->getScalarType()) <
          genx::QWordBits) {
    From = NormalizeFuncPtrVec(From, Inst);
    IGC_ASSERT(From);
    To = From->getType();
    IGC_ASSERT(To);
    NumElts = cast<IGCLLVM::FixedVectorType>(To)->getNumElements();
  }
  if (To->getScalarType()->isPointerTy() &&
      To->getScalarType()->getPointerElementType()->isFunctionTy()) {
    To = IGCLLVM::FixedVectorType::get(I64Ty, NumElts);
    Res = CastInst::Create(Instruction::PtrToInt, From, To, "", Inst);
    NumElts *= 2;
    To = IGCLLVM::FixedVectorType::get(I32Ty, NumElts);
    EltSz = I32Ty->getPrimitiveSizeInBits() / genx::ByteBits;
    Res = CastInst::Create(Instruction::BitCast, Res, To, "", Inst);
  } else if (DL_->getTypeSizeInBits(cast<VectorType>(To)->getElementType()) <
             genx::DWordBits) {
    auto EltTy = cast<VectorType>(To)->getElementType();
    auto EltTySz = DL_->getTypeSizeInBits(EltTy);
    // if EltTy not Integer we shoud bitcast it before ZE
    if (!EltTy->isIntegerTy()) {
      auto Ty = IntegerType::get(Inst->getContext(), EltTySz);
      To = IGCLLVM::FixedVectorType::get(Ty, NumElts);
      From = CastInst::Create(Instruction::BitCast, From, To, "", Inst);
    }
    To = IGCLLVM::FixedVectorType::get(I32Ty, NumElts);
    Res = CastInst::CreateZExtOrBitCast(From, To, "", Inst);
  } else if (DL_->getTypeSizeInBits(cast<VectorType>(To)->getElementType()) ==
             genx::QWordBits) {
    if (From->getType()->getScalarType()->isPointerTy()) {
      auto *NewType = IGCLLVM::FixedVectorType::get(I64Ty, NumElts);
      From = CastInst::Create(CastInst::PtrToInt, From, NewType, "", Inst);
      To = IGCLLVM::FixedVectorType::get(I64Ty, NumElts);
      EltSz = I64Ty->getPrimitiveSizeInBits() / genx::ByteBits;
    }
    Res = CastInst::CreateBitOrPointerCast(From, To, "", Inst);
  }

  return std::make_pair(Res, EltSz);
}
// --- MAGIC LEGACY END ---

// --- MAGIC LEGACY ---
Instruction *
GenXLoadStoreLowering::RestoreVectorAfterNormalization(Instruction *From,
                                                       Type *To) const {
  IGC_ASSERT(From);
  if (From->getType() == To)
    return From;
  Instruction *Restored = From;
  unsigned EltSz = DL_->getTypeSizeInBits(To->getScalarType());
  IGC_ASSERT(EltSz > 0);
  if (To->getScalarType()->isPointerTy() &&
      To->getScalarType()->getPointerElementType()->isFunctionTy()) {
    auto *NewFrom = From;
    if (From->getType()->isVectorTy() &&
        From->getType()->getScalarType()->isIntegerTy(genx::DWordBits)) {
      auto HalfElts =
          cast<IGCLLVM::FixedVectorType>(From->getType())->getNumElements() / 2;
      IGC_ASSERT(HalfElts > 0);
      auto *NewTy = IGCLLVM::FixedVectorType::get(
          Type::getInt64Ty(From->getContext()), HalfElts);
      NewFrom = CastInst::CreateBitOrPointerCast(From, NewTy);
      NewFrom->setDebugLoc(From->getDebugLoc());
      NewFrom->insertAfter(From);
      From = NewFrom;
    }
    Restored = CastInst::Create(Instruction::IntToPtr, NewFrom, To);
  } else if (EltSz < genx::DWordBits) {
    auto EltTy = cast<VectorType>(To)->getElementType();
    auto EltTySz = DL_->getTypeSizeInBits(EltTy);
    if (!EltTy->isIntegerTy()) {
      auto Ty = IntegerType::get(From->getContext(), EltTySz);
      auto NumElts = cast<IGCLLVM::FixedVectorType>(To)->getNumElements();
      auto ToTr = IGCLLVM::FixedVectorType::get(Ty, NumElts);
      auto Trunc = CastInst::Create(Instruction::Trunc, From, ToTr, "");
      Trunc->setDebugLoc(From->getDebugLoc());
      Trunc->insertAfter(From);
      From = Trunc;
      Restored = CastInst::Create(Instruction::BitCast, Trunc, To, "");
    } else {
      Restored = CastInst::Create(Instruction::Trunc, From, To, "");
    }
  } else if (EltSz == genx::QWordBits &&
             !To->getScalarType()->isIntegerTy(64)) {
    if (!From->getType()->getScalarType()->isPointerTy() &&
        To->getScalarType()->isPointerTy()) {
      Restored = CastInst::Create(CastInst::IntToPtr, From, To);
    } else
      Restored = CastInst::CreateBitOrPointerCast(From, To);
  }
  if (Restored != From) {
    Restored->setDebugLoc(From->getDebugLoc());
    Restored->insertAfter(From);
  }
  return Restored;
}
// --- MAGIC LEGACY END ---

static Value *FormEltsOffsetVector(unsigned NumElts, unsigned TySz,
                                   Instruction *InsertBefore,
                                   IRBuilder<> &Builder) {
  Value *EltsOffset = UndefValue::get(
      IGCLLVM::FixedVectorType::get(Builder.getInt32Ty(), NumElts));
  for (unsigned CurElt = 0; CurElt < NumElts; ++CurElt) {
    Value *Idx = Builder.getInt32(CurElt);
    Value *EltOffset = Builder.getInt32(CurElt * TySz);
    EltsOffset = Builder.CreateInsertElement(EltsOffset, EltOffset, Idx);
  }

  return EltsOffset;
}

static Value *FormEltsOffsetVectorForSVM(Value *BaseOffset, Value *Offsets,
                                         Instruction *InsertBefore,
                                         IRBuilder<> &Builder) {
  IGC_ASSERT(BaseOffset->getType()->isIntegerTy(64));
  IGC_ASSERT(Offsets->getType()->isVectorTy());

  Type *I64Ty = Builder.getInt64Ty();
  unsigned NumElts =
      cast<IGCLLVM::FixedVectorType>(Offsets->getType())->getNumElements();
  Value *BaseOffsets = Builder.CreateVectorSplat(NumElts, BaseOffset);
  if (!Offsets->getType()->getScalarType()->isIntegerTy(64))
    Offsets = Builder.CreateZExtOrBitCast(
        Offsets, IGCLLVM::FixedVectorType::get(I64Ty, NumElts));
  return Builder.CreateAdd(BaseOffsets, Offsets);
}

// Wipe all internal ConstantExprs out of V if it's a ConstantVector of function
// pointers
Value *GenXLoadStoreLowering::NormalizeFuncPtrVec(Value *V,
                                                  Instruction *InsPoint) const {
  V = vc::breakConstantVector(cast<ConstantVector>(V), InsPoint, InsPoint,
                              vc::LegalizationStage::NotLegalized);
  auto *Inst = dyn_cast<InsertElementInst>(V);
  if (!Inst)
    return V;
  std::vector<ExtractElementInst *> Worklist;
  for (; Inst; Inst = dyn_cast<InsertElementInst>(Inst->getOperand(0))) {
    if (auto *EEInst = dyn_cast<ExtractElementInst>(Inst->getOperand(1)))
      if (auto *Idx = dyn_cast<Constant>(EEInst->getIndexOperand());
          Idx && Idx->isZeroValue())
        Worklist.push_back(EEInst);
  }

  std::vector<Constant *> NewVector;
  std::transform(
      Worklist.rbegin(), Worklist.rend(), std::back_inserter(NewVector),
      [this](ExtractElementInst *I) {
        IGC_ASSERT(I->getType()->getScalarType()->isIntegerTy(genx::ByteBits));
        auto *F = cast<Function>(getFunctionPointerFunc(I->getVectorOperand()));
        return ConstantExpr::getPtrToInt(
            F, IntegerType::getInt64Ty(I->getContext()));
      });
  auto *NewCV = ConstantVector::get(NewVector);
  IGC_ASSERT(DL_->getTypeSizeInBits(V->getType()) ==
             DL_->getTypeSizeInBits(NewCV->getType()));
  return NewCV;
}

// we need vector type that emulates what real load type
// will be for gather/scatter
IGCLLVM::FixedVectorType *
GenXLoadStoreLowering::getLoadVType(const LoadInst &LdI) const {
  Type *LdTy = LdI.getType();
  if (LdTy->isIntOrPtrTy() || LdTy->isFloatingPointTy())
    LdTy = IGCLLVM::FixedVectorType::get(LdTy, 1);

  if (!LdTy->isVectorTy())
    vc::diagnose(LdI.getContext(), "LDS", LdTy,
                 "Unsupported type inside replaceLoad");
  return cast<IGCLLVM::FixedVectorType>(LdTy);
}

// Creates svm.gather intrinsic and returns it.
Instruction *GenXLoadStoreLowering::createSVMGatherImpl(
    LoadInst &LdI, Value &NormalizedOldVal, unsigned ValueEltSz,
    IRBuilder<> &Builder) const {
  Type *I64Ty = Builder.getInt64Ty();
  LLVM_DEBUG(dbgs() << "Creating load from: " << LdI << "\n");
  IGCLLVM::FixedVectorType *LdTy = getLoadVType(LdI);
  unsigned NumEltsToLoad = LdTy->getNumElements();
  Value *PredVal = Builder.getTrue();
  Value *Pred = Builder.CreateVectorSplat(NumEltsToLoad, PredVal);

  Value *PointerOp = LdI.getPointerOperand();
  Value *Offset =
      CastInst::Create(Instruction::PtrToInt, PointerOp, I64Ty, "", &LdI);

  ZExtOrTruncIfNeeded(Offset, I64Ty, &LdI);

  auto IID = llvm::GenXIntrinsic::genx_svm_gather;

  Value *EltsOffset =
      FormEltsOffsetVector(NumEltsToLoad, ValueEltSz, &LdI, Builder);

  // always one element for one channel
  Value *logNumBlocks = Builder.getInt32(0);
  Offset = FormEltsOffsetVectorForSVM(Offset, EltsOffset, &LdI, Builder);
  Function *F = GenXIntrinsic::getGenXDeclaration(
      LdI.getModule(), IID,
      {NormalizedOldVal.getType(), Pred->getType(), Offset->getType()});
  CallInst *Gather = IntrinsicInst::Create(
      F, {Pred, logNumBlocks, Offset, &NormalizedOldVal}, LdI.getName());

  LLVM_DEBUG(dbgs() << "Created: " << *Gather << "\n");
  return Gather;
}

Instruction *GenXLoadStoreLowering::extractFirstElement(
    Instruction &ProperGather, Type &LdTy, IRBuilder<> &Builder) const {
  auto *GatheredTy = cast<IGCLLVM::FixedVectorType>(ProperGather.getType());
  Builder.ClearInsertionPoint();
  Instruction *LdVal = nullptr;
  if (GatheredTy->getNumElements() == 1)
    LdVal = cast<Instruction>(
        Builder.CreateExtractElement(&ProperGather, static_cast<uint64_t>(0ul),
                                     ProperGather.getName() + ".tpm.loadres"));
  else
    LdVal = cast<Instruction>(Builder.CreateBitOrPointerCast(
        &ProperGather, &LdTy, ProperGather.getName() + ".tpm.loadres"));
  LdVal->setDebugLoc(ProperGather.getDebugLoc());
  LdVal->insertAfter(&ProperGather);
  return LdVal;
}

void GenXLoadStoreLowering::visitLoadInst(LoadInst &LdI) const {
  LLVM_DEBUG(dbgs() << "Replacing load " << LdI << " ===>\n");
  auto *Replacement = createMemoryInstReplacement(LdI);
  LLVM_DEBUG(dbgs() << "Proper gather to replace uses: " << *Replacement
                    << "\n");
  LdI.replaceAllUsesWith(Replacement);
  LdI.eraseFromParent();
}

// Creates a svm.gather intrinsic replacement for the provided \p LdI load
// instruction. Returns the replacement. The returned value may not be a
// svm.gather intrinsic, e.g. when replacement is a chain svm.gather -> bitcast,
// in this case bitcast is returned.
Instruction *GenXLoadStoreLowering::createSVMGather(LoadInst &LdI) const {
  IRBuilder<> Builder(&LdI);

  IGCLLVM::FixedVectorType *LdTy = getLoadVType(LdI);
  auto *LdEltTy = LdTy->getElementType();
  unsigned NumEltsToLoad = LdTy->getNumElements();

  Value *OldValOfTheDataRead =
      Builder.CreateVectorSplat(NumEltsToLoad, UndefValue::get(LdEltTy));

  // normalize (see restore below)
  auto [NormalizedOldVal, ValueEltSz] =
      NormalizeVector(OldValOfTheDataRead, LdTy, &LdI, Builder);

  auto *Gather =
      createSVMGatherImpl(LdI, *NormalizedOldVal, ValueEltSz, Builder);
  Gather->setDebugLoc(LdI.getDebugLoc());
  Gather->insertAfter(&LdI);

  // SVMBlockType metadata and ProperGather insertion below
  // is a part of obscure 8/16 gather support
  // see comment in InternalMetadata.h

  // restore (see normalize above)
  Instruction *ProperGather = RestoreVectorAfterNormalization(Gather, LdTy);

  // if LdI is not vector, extract first element from ProperGather type
  if (!isa<VectorType>(LdI.getType()) &&
      isa<VectorType>(ProperGather->getType()))
    ProperGather = extractFirstElement(*ProperGather, *LdTy, Builder);

  Gather->setMetadata(
      InstMD::SVMBlockType,
      MDNode::get(LdI.getContext(),
                  llvm::ValueAsMetadata::get(UndefValue::get(LdEltTy))));

  LLVM_DEBUG(dbgs() << "Replaced with: " << *Gather << "\n");
  IGC_ASSERT(LdI.getType() == ProperGather->getType());
  return ProperGather;
}

// Creates svm.scatter intrinsic and returns it.
Instruction *GenXLoadStoreLowering::createSVMScatterImpl(
    StoreInst &StI, Value &NormalizedOldVal, unsigned ValueEltSz,
    IRBuilder<> &Builder) const {
  Value *PointerOp = StI.getPointerOperand();
  Type *I64Ty = Builder.getInt64Ty();
  Value *PredVal = Builder.getTrue();
  Value *Offset =
      CastInst::Create(Instruction::PtrToInt, PointerOp, I64Ty, "", &StI);

  unsigned ValueNumElts =
      cast<IGCLLVM::FixedVectorType>(NormalizedOldVal.getType())
          ->getNumElements();
  Value *Pred = Builder.CreateVectorSplat(ValueNumElts, PredVal);

  auto IID = llvm::GenXIntrinsic::genx_svm_scatter;

  Value *EltsOffset =
      FormEltsOffsetVector(ValueNumElts, ValueEltSz, &StI, Builder);
  Offset = FormEltsOffsetVectorForSVM(Offset, EltsOffset, &StI, Builder);

  Function *F = GenXIntrinsic::getGenXDeclaration(
      StI.getModule(), IID,
      {Pred->getType(), Offset->getType(), NormalizedOldVal.getType()});

  // always one element for one channel
  Value *logNumBlocks = Builder.getInt32(0);
  auto *Scatter = IntrinsicInst::Create(
      F, {Pred, logNumBlocks, Offset, &NormalizedOldVal}, StI.getName());
  return Scatter;
}

Value *GenXLoadStoreLowering::splatStoreIfNeeded(StoreInst &StI,
                                                 IRBuilder<> &Builder) const {
  Value *ValueOp = StI.getValueOperand();
  Type *ValueOpTy = ValueOp->getType();
  if (ValueOpTy->isIntOrPtrTy() || ValueOpTy->isFloatingPointTy())
    ValueOp = Builder.CreateVectorSplat(1, ValueOp);
  ValueOpTy = ValueOp->getType();

  if (!ValueOpTy->isVectorTy()) {
    vc::diagnose(StI.getContext(), "LDS", ValueOpTy,
                 "Unsupported type inside replaceStore");
  }

  return ValueOp;
}

void GenXLoadStoreLowering::visitStoreInst(StoreInst &StI) const {
  LLVM_DEBUG(dbgs() << "Replacing store " << StI << " ===>\n");
  auto *Replacement = createMemoryInstReplacement(StI);
  LLVM_DEBUG(dbgs() << *Replacement << "\n");
  StI.eraseFromParent();
}

// Creates a svm.scatter intrinsic replacement for the provided \p StI store
// instruction. Returns the constructed svm.scatter. May insert some additional
// instructions besides the svm.scatter.
Instruction *GenXLoadStoreLowering::createSVMScatter(StoreInst &StI) const {
  IRBuilder<> Builder(&StI);
  Value *ValueOp = splatStoreIfNeeded(StI, Builder);
  // old Value type to restore into
  Type *ValueOpTy = ValueOp->getType();

  auto [NormalizedOldVal, ValueEltSz] =
      NormalizeVector(ValueOp, ValueOpTy, &StI, Builder);

  // creating store after StI, using NormalizedOldVal
  auto *Scatter =
      createSVMScatterImpl(StI, *NormalizedOldVal, ValueEltSz, Builder);
  Scatter->setDebugLoc(StI.getDebugLoc());
  Scatter->insertAfter(&StI);
  Scatter->setMetadata(
      InstMD::SVMBlockType,
      MDNode::get(StI.getContext(), llvm::ValueAsMetadata::get(UndefValue::get(
                                        ValueOpTy->getScalarType()))));
  return Scatter;
}

void GenXLoadStoreLowering::visitIntrinsicInst(IntrinsicInst &Intrinsic) const {
  unsigned ID = GenXIntrinsic::getAnyIntrinsicID(&Intrinsic);
  switch (ID) {
  case Intrinsic::lifetime_start:
  case Intrinsic::lifetime_end:
    Intrinsic.eraseFromParent();
    break;
  }
}

template <typename MemoryInstT>
Instruction *
GenXLoadStoreLowering::createMemoryInstReplacement(MemoryInstT &I) const {
  return switchAtomicity(I);
}

template <typename MemoryInstT>
Instruction *GenXLoadStoreLowering::switchAtomicity(MemoryInstT &I) const {
  if (I.isAtomic())
    return switchMessage<Atomicity::Atomic>(I);
  return switchMessage<Atomicity::NonAtomic>(I);
}

template <Atomicity A, typename MemoryInstT>
Instruction *GenXLoadStoreLowering::switchMessage(MemoryInstT &I) const {
  // FIXME: support LSC messages.
  return switchAddrSpace<MessageKind::Legacy, A>(I);
}

template <MessageKind MK, Atomicity A, typename MemoryInstT>
Instruction *GenXLoadStoreLowering::switchAddrSpace(MemoryInstT &I) const {
  auto *PtrTy = cast<PointerType>(I.getPointerOperand()->getType());
  auto AS = PtrTy->getAddressSpace();
  if (AS == vc::AddrSpace::Local)
    return createIntrinsic<HWAddrSpace::SLM, MK, A>(I);
  // All other address spaces are placed in global memory (SVM).
  unsigned PtrSize = DL_->getPointerTypeSizeInBits(PtrTy);
  if (PtrSize == 32)
    return createIntrinsic<HWAddrSpace::A32, MK, A>(I);
  IGC_ASSERT_MESSAGE(PtrSize == 64, "only 32 and 64 bit pointers are expected");
  return createIntrinsic<HWAddrSpace::A64, MK, A>(I);
}

template <HWAddrSpace HWAS, MessageKind MK, Atomicity A, typename MemoryInstT>
Instruction *GenXLoadStoreLowering::createIntrinsic(MemoryInstT &I) const {
  IGC_ASSERT_MESSAGE(0, "unsupported kind of memory operation");
  return &I;
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::NonAtomic, LoadInst>(
    LoadInst &I) const {
  return createSVMGather(I);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  return createSVMScatter(I);
}
