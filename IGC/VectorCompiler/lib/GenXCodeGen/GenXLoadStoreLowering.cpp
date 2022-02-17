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
#include "GenXVisa.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/BreakConst.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/GenX/TypeSize.h"
#include "vc/Utils/General/IRBuilder.h"
#include "vc/Utils/General/Types.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/Constants.h"
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
  const GenXSubtarget *ST = nullptr;

private:
  Value *ZExtOrTruncIfNeeded(Value *From, Type *To,
                             Instruction *InsertBefore) const;
  std::pair<Value *, unsigned>
  normalizeDataVecForSVMIntrinsic(Value *From, Type *To,
                                  Instruction *InsertBefore,
                                  IRBuilder<> &Builder) const;
  Instruction *RestoreVectorAfterNormalization(Instruction *From,
                                               Type *To) const;
  Value *NormalizeFuncPtrVec(Value *V, Instruction *InsPoint) const;
  IGCLLVM::FixedVectorType *getLoadVType(const LoadInst &LdI) const;
  Value *vectorizeValueOperandIfNeeded(StoreInst &StI,
                                       IRBuilder<> &Builder) const;
  Instruction *extractFirstElement(Instruction &ProperGather, Type &LdTy,
                                   IRBuilder<> &Builder) const;
  Instruction *createLegacySVMAtomicInst(Value &PointerOp,
                                         ArrayRef<Value *> Args,
                                         GenXIntrinsic::ID IID,
                                         IRBuilder<> &Builder, Module &M) const;

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
  Instruction *createGatherScaled(LoadInst &OrigLoad,
                                  visa::ReservedSurfaceIndex Surface) const;
  Instruction *createSVMScatter(StoreInst &StI) const;
  Instruction *createSVMScatterImpl(StoreInst &StI, Value &NormalizedOldVal,
                                    unsigned ValueEltSz,
                                    IRBuilder<> &Builder) const;
  Instruction *createScatterScaled(StoreInst &OrigStore,
                                   visa::ReservedSurfaceIndex Surface) const;

  Instruction *createLegacySVMAtomicLoad(LoadInst &LdI) const;
  Instruction *createLegacySVMAtomicStore(StoreInst &StI) const;
  Instruction *createLegacySVMAtomicBinOp(AtomicRMWInst &I) const;
  Instruction *createLegacySVMAtomicCmpXchg(AtomicCmpXchgInst &CmpXchgI) const;

public:
  void visitStoreInst(StoreInst &StI) const;
  void visitLoadInst(LoadInst &LdI) const;
  void visitAtomicRMWInst(AtomicRMWInst &Inst) const;
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &Inst) const;
  void visitIntrinsicInst(IntrinsicInst &Intrinsic) const;

public:
  static char ID;
  explicit GenXLoadStoreLowering() : FunctionPass(ID) {}
  StringRef getPassName() const override { return "GenX load store lowering"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
};

// Creates ptrtoint instruction that truncates pointer to i32
// Instructions are inserted via the provided \p Builder.
Value *createPtrToInt32(Value &V, IRBuilder<> &IRB) {
  auto *IntPtrTy = IRB.getInt32Ty();
  return IRB.CreatePtrToInt(&V, IntPtrTy, V.getName() + ".p2i32");
}

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
  ST = &getAnalysis<TargetPassConfig>()
                 .getTM<GenXTargetMachine>()
                 .getGenXSubtarget();
  IGC_ASSERT(ST);
  // auto &BEConf = getAnalysis<GenXBackendConfig>();
  // BEConf.getStatelessPrivateMemSize() will be required

  // pass don't work for CMRT for now, legacy TPM will be used
  if (!ST->isOCLRuntime())
    return false;

  // see visitXXInst members for main logic:
  //   * visitStoreInst
  //   * visitLoadInst
  //   * visitAtomicRMWInst
  //   * visitAtomicCmpXchgInst
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
// Modify data operand to match svm.gather/scatter restrictions.
// If data is a vector of double/int64, bitcast each element to 2 int32.
// If data is a vector of function pointers, strip all internal bitcasts
// and possible extractelems (64->8xi8 cast case) to get a vector of int64s.
// If data is a vector of type < 32bit, extend each element in order to create
// proper send instruction in the finalizer.
//
// returned size almost always size of element of returned data
// except 8/16 case shrink
std::pair<Value *, unsigned>
GenXLoadStoreLowering::normalizeDataVecForSVMIntrinsic(
    Value *From, Type *To, Instruction *Inst, IRBuilder<> &Builder) const {
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

// \p TySz must be in bytes.
static Value *FormEltsOffsetVector(unsigned NumElts, unsigned TySz,
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

  Value *EltsOffset = FormEltsOffsetVector(NumEltsToLoad, ValueEltSz, Builder);

  // always one element for one channel
  Value *logNumBlocks = Builder.getInt32(0);
  Offset = FormEltsOffsetVectorForSVM(Offset, EltsOffset, Builder);
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

void GenXLoadStoreLowering::visitAtomicRMWInst(AtomicRMWInst &Inst) const {
  LLVM_DEBUG(dbgs() << "Replacing binary atomic inst " << Inst << " ===>\n");
  auto *Replacement = createMemoryInstReplacement(Inst);
  Inst.replaceAllUsesWith(Replacement);
  Inst.eraseFromParent();
}

void GenXLoadStoreLowering::visitAtomicCmpXchgInst(
    AtomicCmpXchgInst &Inst) const {
  LLVM_DEBUG(dbgs() << "Replacing cmpxchg inst " << Inst << " ===>\n");
  auto *Replacement = createMemoryInstReplacement(Inst);
  Inst.replaceAllUsesWith(Replacement);
  Inst.eraseFromParent();
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
      normalizeDataVecForSVMIntrinsic(OldValOfTheDataRead, LdTy, &LdI, Builder);

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
      vc::InstMD::SVMBlockType,
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

  Value *EltsOffset = FormEltsOffsetVector(ValueNumElts, ValueEltSz, Builder);
  Offset = FormEltsOffsetVectorForSVM(Offset, EltsOffset, Builder);

  Function *F = GenXIntrinsic::getGenXDeclaration(
      StI.getModule(), IID,
      {Pred->getType(), Offset->getType(), NormalizedOldVal.getType()});

  // always one element for one channel
  Value *logNumBlocks = Builder.getInt32(0);
  auto *Scatter = IntrinsicInst::Create(
      F, {Pred, logNumBlocks, Offset, &NormalizedOldVal}, StI.getName());
  return Scatter;
}

// A kind of a transformation that should be applied to memory instruction
// value operand to match its lower representation restrictions (vc-intrinsic
// restrictions) or to memory instruction return value.
// All transformations do not include possible cast to a degenerate vector type
// or from degenerate vector type (most of memory vc-intrinsics do not support
// scalar data operands, so scalar operands should be casted to degenerate
// vectors).
enum class DataTransformation {
  // No modifications to data is required.
  NoModification,
  // Data should be zero extended to match a vc-intrinsic operand, or data from
  // a vc-intrinsic should be truncated to match the original instruction.
  // For example:
  // store i16 %a becomes zext i16 %a to i32 -> scatter.scaled
  // load i16 becomes gather.scaled -> trunc to i16
  ZextOrTrunc,
  // Data should be bitcasted to an integer or a vector of integers type.
  IntBitCast,
};

// Defines how data must be transformed to match scaled memory intrinsics:
//    1) how store value operand must be transformed to be passed to
//       scatter.scaled intrinsic;
//    2) how gather.scaled return value must be trasformed to match the original
//       load return type.
// Arguments:
//    \p OrigTy - data type of the original memory instruction;
//    \p C - LLVM context to create new types;
//    \p DL - data layout.
// Returns:
//    1) intrinsic value operand (for store) or return value (for load) type;
//    2) data element type size (may not match 1st return value vector element
//       type in case of zext/trunc or some other cases, may not match the
//       original type size);
//    3) the kind of transformation that should be applied to the data value.
static std::tuple<IGCLLVM::FixedVectorType *, vc::TypeSizeWrapper,
                  DataTransformation>
transformTypeForScaledIntrinsic(Type &OrigTy, LLVMContext &C,
                                const DataLayout &DL) {
  auto *OrigVecTy = &vc::getVectorType(OrigTy);
  auto *OrigElemTy = OrigVecTy->getElementType();
  unsigned OrigElemCount = OrigVecTy->getNumElements();
  auto OrigElemSize = vc::getTypeSize(OrigElemTy, &DL);
  auto *Int32Ty = IntegerType::get(C, 32);
  IGC_ASSERT_MESSAGE(OrigElemSize >= vc::ByteSize,
                     "bool stores aren't supported");

  // Already legal type.
  if (OrigElemTy->isIntegerTy(vc::DWordBits) || OrigElemTy->isFloatTy())
    return {OrigVecTy, OrigElemSize, DataTransformation::NoModification};

  if (OrigElemSize < vc::DWordSize) {
    IGC_ASSERT_MESSAGE(
        OrigElemTy->isIntegerTy(),
        "only integer types are expected to be less than 32 bits");
    // SCATTER_SCALED will ignore upper bits. GATHER_SCALED will set upper bits
    // with undef values.
    return {IGCLLVM::FixedVectorType::get(Int32Ty, OrigElemCount), OrigElemSize,
            DataTransformation::ZextOrTrunc};
  }

  // For data that is bigger than DWord or DWord sized but not of a legal type
  // cast it to vector of i32.
  IGC_ASSERT_MESSAGE(OrigElemSize.inBits() * OrigElemCount % vc::DWordBits == 0,
                     "the data must be representable as <N x i32>");
  unsigned NewElemCount = OrigElemSize.inBits() * OrigElemCount / vc::DWordBits;
  auto *NewTy = IGCLLVM::FixedVectorType::get(Int32Ty, NewElemCount);
  return {NewTy, vc::DWordSize, DataTransformation::IntBitCast};
}

// Obtains the value that was requested to be loaded by the original memory
// instruction from the \p Gather return value (\p OrigLoadTy type and \p Gather
// type may not match).
// Arguments:
//    \p Gather - gather.scaled intrinsic;
//    \p OrigLoadTy - the type of the original load instruction;
//    \p Action - how \p Gather should be changed to match \p OrigLoadTy type,
//                must be obtained via transformTypeForScaledIntrinsic;
//    \p IRB - IR builder that will be used to construct the code;
//    \p DL - data layout.
// Returns a replacement for the original load instruction. The returned value
// type is equal to \p OrigLoadTy.
static Instruction *restoreLoadValueFromGatherScaled(Instruction &Gather,
                                                     Type &OrigLoadTy,
                                                     DataTransformation Action,
                                                     IRBuilder<> &IRB,
                                                     const DataLayout &DL) {
  Value *Result;
  switch (Action) {
  case DataTransformation::NoModification:
    // May need to fix degenerate vector type.
    // Note: no cast is created when types match.
    Result = IRB.CreateBitCast(&Gather, &OrigLoadTy, Gather.getName() + ".bc");
    break;
  case DataTransformation::ZextOrTrunc: {
    auto *Trunc = IRB.CreateTrunc(&Gather, &vc::getVectorType(OrigLoadTy),
                                  Gather.getName() + ".trunc");
    // May need to fix degenerate vector type.
    // Note: no cast is created when types match.
    Result = IRB.CreateBitCast(Trunc, &OrigLoadTy, Gather.getName() + ".bc");
    break;
  }
  case DataTransformation::IntBitCast:
    Result = vc::castFromIntOrFloat(Gather, OrigLoadTy, IRB, DL);
    break;
  default:
    IGC_ASSERT_MESSAGE(0, "unexpected transformation kind");
  }
  return cast<Instruction>(Result);
}

// Creates a gather.scaled intrinsic replacement for the provided \p OrigLoad
// load instruction. Returns the replacement. The returned value may not be a
// gather.scaled intrinsic, e.g. when replacement is a chain
// svm.gather -> bitcast, in this case bitcast is returned.
// Arguments:
//    \p OrigLoad - original load to be replaced;
//    \p Surface - predefined surface that should be used in the generated
//                 gather.scaled intrinsic, only SLM and Stateless surfaces are
//                 supported/expected, which surface to use should be defined
//                 by outer code depending on \p OrigLoad properties.
Instruction *GenXLoadStoreLowering::createGatherScaled(
    LoadInst &OrigLoad, visa::ReservedSurfaceIndex Surface) const {
  IGC_ASSERT_MESSAGE(Surface == visa::RSI_Stateless || Surface == visa::RSI_Slm,
                     "only Stateless and SLM predefined surfaces are expected");
  IRBuilder<> IRB{&OrigLoad};
  auto [GatherTy, DataSize, Action] = transformTypeForScaledIntrinsic(
      *OrigLoad.getType(), IRB.getContext(), *DL_);
  int NumBlocks = DataSize.inBytes();
  IGC_ASSERT_MESSAGE(NumBlocks == 1 || NumBlocks == 2 || NumBlocks == 4,
                     "number of blocks can only be 1, 2 or 4");
  Constant *Predicate = IGCLLVM::ConstantFixedVector::getSplat(
      GatherTy->getNumElements(), IRB.getTrue());
  Value *GlobalOffset = createPtrToInt32(*OrigLoad.getPointerOperand(), IRB);
  Value *ElementOffsets =
      FormEltsOffsetVector(GatherTy->getNumElements(), NumBlocks, IRB);

  Value *Args[] = {IRB.getInt32(Log2_32(NumBlocks)),
                   IRB.getInt16(0),
                   IRB.getInt32(Surface),
                   GlobalOffset,
                   ElementOffsets,
                   Predicate};
  Function *Decl = vc::getGenXDeclarationForIdFromArgs(
      GatherTy, Args, GenXIntrinsic::genx_gather_masked_scaled2,
      *OrigLoad.getModule());
  auto *Gather = IRB.CreateCall(Decl, Args, OrigLoad.getName() + ".gather");
  auto *LoadReplacement = restoreLoadValueFromGatherScaled(
      *Gather, *OrigLoad.getType(), Action, IRB, *DL_);
  return LoadReplacement;
}

Value *GenXLoadStoreLowering::vectorizeValueOperandIfNeeded(
    StoreInst &StI, IRBuilder<> &Builder) const {
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

// Modifies store value operand \p OrigData to match scatter.scaled
// restrictions.
// Returns data operand and element/block size for scatter.scaled intrinsic.
static std::pair<Value *, vc::TypeSizeWrapper>
normalizeDataVecForScatterScaled(Value &OrigData, IRBuilder<> &Builder,
                                 const DataLayout &DL) {
  auto [NewDataTy, DataSize, Action] = transformTypeForScaledIntrinsic(
      *OrigData.getType(), Builder.getContext(), DL);
  auto *OrigVecTy = &vc::getVectorType(*OrigData.getType());

  Value *NewData;
  switch (Action) {
  case DataTransformation::NoModification:
    NewData =
        Builder.CreateBitCast(&OrigData, OrigVecTy, OrigData.getName() + ".bc");
    break;
  case DataTransformation::ZextOrTrunc: {
    auto *OrigVecData =
        Builder.CreateBitCast(&OrigData, OrigVecTy, OrigData.getName() + ".bc");
    NewData = Builder.CreateZExt(OrigVecData, NewDataTy,
                                 OrigData.getName() + ".zext");
    break;
  }
  case DataTransformation::IntBitCast:
    NewData = vc::castToIntOrFloat(OrigData, *NewDataTy, Builder, DL);
    break;
  default:
    IGC_ASSERT_MESSAGE(0, "unexpected transformation kind");
  }
  return {NewData, DataSize};
}

// Creates a scatter.scaled intrinsic replacement for the provided \p OrigStore
// store instruction. Returns the created scatter.scaled intrinsic. May insert
// some additional instructions besides the scatter.scaled
// Arguments:
//    \p OrigStore - original store to be replaced;
//    \p Surface - predefined surface that should be used in the generated
//                 gather.scaled intrinsic, only SLM and Stateless surfaces are
//                 supported/expected, which surface to use should be defined
//                 by outer code depending on \p OrigLoad properties.
Instruction *GenXLoadStoreLowering::createScatterScaled(
    StoreInst &OrigStore, visa::ReservedSurfaceIndex Surface) const {
  IGC_ASSERT_MESSAGE(Surface == visa::RSI_Stateless || Surface == visa::RSI_Slm,
                     "only Stateless and SLM predefined surfaces are expected");
  IRBuilder<> Builder{&OrigStore};
  auto [ValueOp, DataSize] = normalizeDataVecForScatterScaled(
      *OrigStore.getValueOperand(), Builder, *DL_);
  // For genx.scatter.scaled number of blocks is really the data element size.
  int NumBlocks = DataSize.inBytes();
  IGC_ASSERT_MESSAGE(NumBlocks == 1 || NumBlocks == 2 || NumBlocks == 4,
                     "number of blocks can only be 1, 2 or 4");

  auto *ValueOpTy = cast<IGCLLVM::FixedVectorType>(ValueOp->getType());
  Constant *Predicate = IGCLLVM::ConstantFixedVector::getSplat(
      ValueOpTy->getNumElements(), Builder.getTrue());

  Value *GlobalOffset =
      createPtrToInt32(*OrigStore.getPointerOperand(), Builder);
  Value *ElementOffsets =
      FormEltsOffsetVector(ValueOpTy->getNumElements(), NumBlocks, Builder);

  Value *Args[] = {Predicate,
                   Builder.getInt32(Log2_32(NumBlocks)),
                   Builder.getInt16(0),
                   Builder.getInt32(Surface),
                   GlobalOffset,
                   ElementOffsets,
                   ValueOp};
  Function *Decl = vc::getGenXDeclarationForIdFromArgs(
      Builder.getVoidTy(), Args, GenXIntrinsic::genx_scatter_scaled,
      *OrigStore.getModule());
  return Builder.CreateCall(Decl, Args);
}

static std::pair<unsigned, AtomicOrdering>
getAddressSpaceAndOrderingOfAtomic(const Instruction &AtomicI) {
  IGC_ASSERT(AtomicI.isAtomic());
  if (auto *ARMW = dyn_cast<AtomicRMWInst>(&AtomicI))
    return {ARMW->getPointerAddressSpace(), ARMW->getOrdering()};
  if (auto *CmpXchg = dyn_cast<AtomicCmpXchgInst>(&AtomicI))
    return {CmpXchg->getPointerAddressSpace(), CmpXchg->getSuccessOrdering()};
  if (auto *LI = dyn_cast<LoadInst>(&AtomicI)) {
    IGC_ASSERT(LI->isAtomic());
    unsigned AS = cast<PointerType>(LI->getPointerOperand()->getType())
                      ->getAddressSpace();
    return {AS, LI->getOrdering()};
  }
  if (auto *SI = dyn_cast<StoreInst>(&AtomicI)) {
    IGC_ASSERT(SI->isAtomic());
    unsigned AS = cast<PointerType>(SI->getPointerOperand()->getType())
                      ->getAddressSpace();
    return {AS, SI->getOrdering()};
  }
  IGC_ASSERT_MESSAGE(false, "Unimplemented atomic inst");
  return {0, AtomicOrdering::Monotonic};
}

static void emitFencesForAtomic(Instruction &NewAtomicI,
                                Instruction &OriginalAtomicI,
                                const GenXSubtarget &ST) {
  auto [AS, Ordering] = getAddressSpaceAndOrderingOfAtomic(OriginalAtomicI);

  bool IsGlobal = (AS == vc::AddrSpace::Global);
  IGC_ASSERT_MESSAGE(IsGlobal, "Global address space for atomic expected");
  bool EmitFence = IsGlobal || !ST.hasLocalMemFenceSupress();

  bool PreOpNeedsFence = (Ordering == AtomicOrdering::Release) ||
                         (Ordering == AtomicOrdering::AcquireRelease) ||
                         (Ordering == AtomicOrdering::SequentiallyConsistent);

  bool PostOpNeedsFence = (Ordering == AtomicOrdering::Acquire) ||
                          (Ordering == AtomicOrdering::AcquireRelease) ||
                          (Ordering == AtomicOrdering::SequentiallyConsistent);

  // Create fence message.
  unsigned char FenceFlags = ((!IsGlobal) << 5) | (!EmitFence << 7) | 1;

  auto *FenceDecl = GenXIntrinsic::getGenXDeclaration(
      NewAtomicI.getModule(), GenXIntrinsic::genx_fence);

  if (PreOpNeedsFence) {
    IRBuilder<> Builder{&NewAtomicI};
    Builder.CreateCall(FenceDecl, Builder.getInt8(FenceFlags));
  }
  if (PostOpNeedsFence) {
    IRBuilder<> Builder{NewAtomicI.getNextNode()};
    Builder.CreateCall(FenceDecl, Builder.getInt8(FenceFlags));
  }
}

Instruction *GenXLoadStoreLowering::createLegacySVMAtomicInst(
    Value &PointerOp, ArrayRef<Value *> Args, GenXIntrinsic::ID IID,
    IRBuilder<> &Builder, Module &M) const {
  IGC_ASSERT_MESSAGE(Args.size(), "Expecting non-empty argument list");
  IGC_ASSERT_MESSAGE(llvm::all_of(Args,
                                  [Args](Value *Arg) {
                                    return Args[0]->getType() == Arg->getType();
                                  }),
                     "Expected equal types");
  IGC_ASSERT_MESSAGE(!Args[0]->getType()->isVectorTy(),
                     "Not expecting vector types");

  Value *Offset = vc::createNopPtrToInt(PointerOp, Builder, *DL_);
  Offset =
      Builder.CreateBitCast(Offset, &vc::getVectorType(*Offset->getType()));

  // True predicate always. One-element vector.
  Value *Pred = IGCLLVM::ConstantFixedVector::getSplat(1, Builder.getTrue());

  SmallVector<Value *, 8> InstrinsicArgs{Pred, Offset};
  // Convert arguments to get one-element vectors.
  llvm::transform(
      Args, std::back_inserter(InstrinsicArgs), [&Builder](Value *Arg) {
        return Builder.CreateBitCast(Arg, &vc::getVectorType(*Arg->getType()));
      });

  Type *InstTy = InstrinsicArgs.back()->getType();

  // Old value of the data read.
  InstrinsicArgs.push_back(UndefValue::get(InstTy));

  Function *F =
      vc::getGenXDeclarationForIdFromArgs(InstTy, InstrinsicArgs, IID, M);
  return Builder.CreateCall(F, InstrinsicArgs);
}

Instruction *
GenXLoadStoreLowering::createLegacySVMAtomicLoad(LoadInst &LdI) const {
  IGC_ASSERT_MESSAGE(!LdI.getType()->isPtrOrPtrVectorTy(),
                     "Not exepecting pointer types");
  IGC_ASSERT(LdI.isAtomic());
  IGCLLVM::FixedVectorType *LdTy = getLoadVType(LdI);
  IGC_ASSERT(LdTy->getNumElements() == 1);
  IGC_ASSERT_MESSAGE(!LdTy->getElementType()->isFloatingPointTy(),
                     "Not expecting floating point");
  auto ValueEltSz = vc::getTypeSize(LdTy, DL_);
  IGC_ASSERT_MESSAGE(ValueEltSz.inBytes() == 4 || ValueEltSz.inBytes() == 8,
                     "Expected 32/64-bit atomic");

  Value *PointerOp = LdI.getPointerOperand();
  IGC_ASSERT_MESSAGE(
      cast<PointerType>(PointerOp->getType())->getAddressSpace() ==
          vc::AddrSpace::Global,
      "Global address space expected");

  IRBuilder<> Builder{&LdI};
  // Generate atomic or with zero value for legacy load.
  Instruction *ResCall = createLegacySVMAtomicInst(
      *PointerOp, {Constant::getNullValue(LdI.getType())},
      GenXIntrinsic::genx_svm_atomic_or, Builder, *LdI.getModule());
  emitFencesForAtomic(*ResCall, LdI, *ST);

  return vc::fixDegenerateVector(*ResCall, Builder);
}

Instruction *
GenXLoadStoreLowering::createLegacySVMAtomicStore(StoreInst &StI) const {
  IGC_ASSERT(StI.isAtomic());
  IRBuilder<> Builder{&StI};

  Value *PointerOp = StI.getPointerOperand();
  Value *ValueOp = StI.getValueOperand();
  IGC_ASSERT_MESSAGE(!ValueOp->getType()->isPtrOrPtrVectorTy(),
                     "Not exepecting pointer types");
  IGC_ASSERT_MESSAGE(
      cast<PointerType>(PointerOp->getType())->getAddressSpace() ==
          vc::AddrSpace::Global,
      "Global address space expected");

  auto ValueEltSz = vc::getTypeSize(ValueOp->getType(), DL_);
  IGC_ASSERT_MESSAGE(ValueEltSz.inBytes() == 4 || ValueEltSz.inBytes() == 8,
                     "Expected 32/64-bit atomic");

  // Cast to integer for floating point types.
  if (ValueOp->getType()->getScalarType()->isFloatingPointTy()) {
    auto *IntValTy = Builder.getIntNTy(ValueEltSz.inBits());
    ValueOp = vc::castToIntOrFloat(*ValueOp, *IntValTy, Builder, *DL_);
  }

  Instruction *ResCall = createLegacySVMAtomicInst(
      *PointerOp, {ValueOp}, GenXIntrinsic::genx_svm_atomic_xchg, Builder,
      *StI.getModule());

  emitFencesForAtomic(*ResCall, StI, *ST);

  return ResCall;
}

static GenXIntrinsic::ID
getLegacyGenXIIDForAtomicRMWInst(AtomicRMWInst::BinOp Op, unsigned AS) {
  IGC_ASSERT_MESSAGE(AS == vc::AddrSpace::Global,
                     "Global address space for atomic expected");
  (void)AS;
  switch (Op) {
  default:
    IGC_ASSERT_MESSAGE(false, "unimplemented binary atomic op");
  case AtomicRMWInst::Min:
    return GenXIntrinsic::genx_svm_atomic_imin;
  case AtomicRMWInst::Max:
    return GenXIntrinsic::genx_svm_atomic_imax;
  case AtomicRMWInst::UMin:
    return GenXIntrinsic::genx_svm_atomic_min;
  case AtomicRMWInst::UMax:
    return GenXIntrinsic::genx_svm_atomic_max;
  case AtomicRMWInst::Xchg:
    return GenXIntrinsic::genx_svm_atomic_xchg;
  case AtomicRMWInst::Add:
    return GenXIntrinsic::genx_svm_atomic_add;
  case AtomicRMWInst::Sub:
    return GenXIntrinsic::genx_svm_atomic_sub;
  case AtomicRMWInst::Or:
    return GenXIntrinsic::genx_svm_atomic_or;
  case AtomicRMWInst::Xor:
    return GenXIntrinsic::genx_svm_atomic_xor;
  case AtomicRMWInst::And:
    return GenXIntrinsic::genx_svm_atomic_and;
  }
}

static bool intrinsicNeedsIntegerOperands(GenXIntrinsic::ID IID) {
  return IID != GenXIntrinsic::genx_svm_atomic_fmin &&
         IID != GenXIntrinsic::genx_svm_atomic_fmax &&
         IID != GenXIntrinsic::genx_svm_atomic_fcmpwr;
}

Instruction *
GenXLoadStoreLowering::createLegacySVMAtomicBinOp(AtomicRMWInst &Inst) const {
  IGC_ASSERT_MESSAGE(!Inst.getType()->isPtrOrPtrVectorTy(),
                     "Not exepecting pointer types");
  IRBuilder<> Builder{&Inst};
  Value *PointerOp = Inst.getPointerOperand();
  Value *ValueOp = Inst.getValOperand();

  IGC_ASSERT_MESSAGE(Inst.getPointerAddressSpace() == vc::AddrSpace::Global,
                     "Global address space expected");

  auto ValueEltSz = vc::getTypeSize(ValueOp->getType()->getScalarType(), DL_);
  IGC_ASSERT_MESSAGE(ValueEltSz.inBytes() == 4 || ValueEltSz.inBytes() == 8,
                     "Expected 32/64-bit atomic");

  auto IID = getLegacyGenXIIDForAtomicRMWInst(Inst.getOperation(),
                                              Inst.getPointerAddressSpace());

  bool NeedConversionToInteger =
      ValueOp->getType()->getScalarType()->isFloatingPointTy() &&
      intrinsicNeedsIntegerOperands(IID);

  if (NeedConversionToInteger)
    ValueOp = vc::castToIntOrFloat(
        *ValueOp, *Builder.getIntNTy(ValueEltSz.inBits()), Builder, *DL_);

  Instruction *ResCall = createLegacySVMAtomicInst(*PointerOp, {ValueOp}, IID,
                                                   Builder, *Inst.getModule());

  emitFencesForAtomic(*ResCall, Inst, *ST);

  return cast<Instruction>(
      vc::castFromIntOrFloat(*ResCall, *Inst.getType(), Builder, *DL_));
}

Instruction *GenXLoadStoreLowering::createLegacySVMAtomicCmpXchg(
    AtomicCmpXchgInst &CmpXchgI) const {
  IRBuilder<> Builder{&CmpXchgI};
  Value *PointerOp = CmpXchgI.getPointerOperand();
  Value *CmpOp = CmpXchgI.getCompareOperand();
  Value *NewOp = CmpXchgI.getNewValOperand();
  IGC_ASSERT_MESSAGE(!CmpOp->getType()->isPtrOrPtrVectorTy(),
                     "Not exepecting pointer types");

  IGC_ASSERT_MESSAGE(CmpXchgI.getPointerAddressSpace() == vc::AddrSpace::Global,
                     "Global address space expected");

  auto ValueEltSz = vc::getTypeSize(CmpOp->getType()->getScalarType(), DL_);
  IGC_ASSERT_MESSAGE(ValueEltSz.inBytes() == 4 || ValueEltSz.inBytes() == 8,
                     "Expected 32/64-bit atomic");

  Instruction *ResCall = createLegacySVMAtomicInst(
      *PointerOp, {CmpOp, NewOp}, GenXIntrinsic::genx_svm_atomic_cmpxchg,
      Builder, *CmpXchgI.getModule());

  emitFencesForAtomic(*ResCall, CmpXchgI, *ST);

  Value *ScalarRes = vc::fixDegenerateVector(*ResCall, Builder);

  // Restore original result structure and return it. Second cmpxchg return
  // operand is true if loaded value equals to cmp.
  auto *Res = Builder.CreateInsertValue(UndefValue::get(CmpXchgI.getType()),
                                        ScalarRes, 0);
  auto *CmpRes = Builder.CreateICmpEQ(ScalarRes, CmpXchgI.getCompareOperand());
  Res = Builder.CreateInsertValue(Res, CmpRes, 1);
  return cast<Instruction>(Res);
}

// Creates a svm.scatter intrinsic replacement for the provided \p StI store
// instruction. Returns the constructed svm.scatter. May insert some additional
// instructions besides the svm.scatter.
Instruction *GenXLoadStoreLowering::createSVMScatter(StoreInst &StI) const {
  IRBuilder<> Builder(&StI);
  Value *ValueOp = vectorizeValueOperandIfNeeded(StI, Builder);
  // old Value type to restore into
  Type *ValueOpTy = ValueOp->getType();

  auto [NormalizedOldVal, ValueEltSz] =
      normalizeDataVecForSVMIntrinsic(ValueOp, ValueOpTy, &StI, Builder);

  // creating store after StI, using NormalizedOldVal
  auto *Scatter =
      createSVMScatterImpl(StI, *NormalizedOldVal, ValueEltSz, Builder);
  Scatter->setDebugLoc(StI.getDebugLoc());
  Scatter->insertAfter(&StI);
  Scatter->setMetadata(
      vc::InstMD::SVMBlockType,
      MDNode::get(StI.getContext(), llvm::ValueAsMetadata::get(UndefValue::get(
                                        ValueOpTy->getScalarType()))));
  return Scatter;
}

void GenXLoadStoreLowering::visitIntrinsicInst(IntrinsicInst &Intrinsic) const {
  unsigned ID = vc::getAnyIntrinsicID(&Intrinsic);
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
  auto *Replacement = switchAtomicity(I);
  Replacement->takeName(&I);
  return Replacement;
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

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::Legacy,
                                       Atomicity::NonAtomic, LoadInst>(
    LoadInst &I) const {
  return createGatherScaled(I, visa::RSI_Stateless);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::Legacy,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  return createScatterScaled(I, visa::RSI_Stateless);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::Atomic, LoadInst>(
    LoadInst &I) const {
  return createLegacySVMAtomicLoad(I);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::Atomic, StoreInst>(
    StoreInst &I) const {
  return createLegacySVMAtomicStore(I);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::Atomic, AtomicRMWInst>(
    AtomicRMWInst &I) const {
  return createLegacySVMAtomicBinOp(I);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::Atomic, AtomicCmpXchgInst>(
    AtomicCmpXchgInst &I) const {
  return createLegacySVMAtomicCmpXchg(I);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::Legacy,
                                       Atomicity::NonAtomic, LoadInst>(
    LoadInst &I) const {
  return createGatherScaled(I, visa::RSI_Slm);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::Legacy,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  return createScatterScaled(I, visa::RSI_Slm);
}
