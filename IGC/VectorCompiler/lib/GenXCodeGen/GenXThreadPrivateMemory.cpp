/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// This pass lowers alloca instructions to genx.alloca intrinsics and changes
/// pointer from alloca to offset in predefined stack surface
//
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXModule.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "GenXVisa.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/BreakConst.h"
#include "vc/Utils/GenX/KernelInfo.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/ADT/StringRef.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/Local.h"

#include <forward_list>
#include <queue>
#include <unordered_set>
#include <utility>

using namespace llvm;
using namespace genx;

#define DEBUG_TYPE "genx-tpm"

static cl::opt<bool> EnableTPM("enable-legacy-tpm", cl::init(true), cl::Hidden,
                               cl::desc("Enable legacy TPM pass"));
static cl::opt<bool> EnableTPMOCLRT("enable-legacy-tpm-oclrt", cl::init(false),
                                    cl::Hidden,
                                    cl::desc("Enable legacy TPM pass"));
static cl::opt<bool> ForceSVMTPM("force-svm-tpm", cl::init(true), cl::Hidden,
  cl::desc("Force putting thread-private memory to SVM"));

namespace {

struct FunctionInfo {
  std::unordered_set<Value *> Calls;
  llvm::SetVector<Argument *> Args;
  bool Recreate = false;
};

// This actually should've been a FunctionGroupPass,
// but due to the FGPassManager hack we can't run GenXModule twice
// so for now we can't insert module pass that invalidate FGA betw FGPasses
class GenXThreadPrivateMemory : public ModulePass,
                                public InstVisitor<GenXThreadPrivateMemory> {
public:
  GenXThreadPrivateMemory();

  StringRef getPassName() const override { return "GenXThreadPrivateMemory"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    ModulePass::getAnalysisUsage(AU);
    AU.addRequired<TargetPassConfig>();
    AU.addRequired<GenXBackendConfig>();
    AU.setPreservesCFG();
  }

  bool runOnModule(Module &M) override;
  bool runOnFunction(Function &F);

  void visitAllocaInst(AllocaInst &I);
  void visitFunction(Function &F);

private:
  bool replacePhi(PHINode *Phi);
  bool preparePhiForReplacement(PHINode *Phi);
  bool replaceScatterPrivate(CallInst *CI);
  bool replaceGatherPrivate(CallInst *CI);
  bool replacePTI(PtrToIntInst *PTI);
  bool replaceStore(StoreInst *StI);
  bool replaceLoad(LoadInst *LdI);
  bool replaceSelect(SelectInst *Sel);
  bool replaceAddrSpaceCast(AddrSpaceCastInst * AddrCast);
  bool replaceInsertElement(InsertElementInst *Insert);
  bool replaceShuffleVector(ShuffleVectorInst *ShuffleVec);
  Value *lookForPtrReplacement(Value *Ptr) const;
  void addUsers(Value *V, bool ProcessCalls = true);
  void collectEachPossibleTPMUsers();
  void collectAllocaUsers();
  void collectArgUsers(bool ProcessCalls = true);
  bool processUsers();
  void processUnchangedCall(CallInst &CI);
  void addUsersIfNeeded(Value *V, bool ProcessCalls = true);
  Value *NormalizeFuncPtrVec(Value *V, Instruction *InsPoint);
  std::pair<Value *, unsigned> NormalizeVector(Value *From, Type *To,
                                               Instruction *InsertBefore);
  Instruction *RestoreVectorAfterNormalization(Instruction *From, Type *To);
  Value *ZExtOrTruncIfNeeded(Value *From, Type *To, Instruction *InsertBefore);
  Value *lookForTruncOffset(Value *V);
  void switchStack(Module &M);

public:
  static char ID;

private:
  LLVMContext *m_ctx;
  const GenXSubtarget *m_ST;
  const DataLayout *m_DL;
  std::vector<AllocaInst *> m_alloca;
  llvm::SetVector<Argument *> m_args;
  std::unordered_map<AllocaInst *, CallInst *> m_allocaToIntrinsic;
  std::queue<std::pair<Instruction *, bool>> m_AIUsers;
  std::set<Instruction *> m_AlreadyAdded;
  PreDefined_Surface m_stack;
  bool m_useGlobalMem = ForceSVMTPM;
  std::map<Function *, FunctionInfo> m_Calls;
};
} // namespace

// Register pass to igc-opt
namespace llvm {
void initializeGenXThreadPrivateMemoryPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXThreadPrivateMemory, "GenXThreadPrivateMemory",
                      "GenXThreadPrivateMemory", false, false)
INITIALIZE_PASS_END(GenXThreadPrivateMemory, "GenXThreadPrivateMemory",
                    "GenXThreadPrivateMemory", false, false)

char GenXThreadPrivateMemory::ID = 0;

ModulePass *llvm::createGenXThreadPrivateMemoryPass() {
  return new GenXThreadPrivateMemory;
}

GenXThreadPrivateMemory::GenXThreadPrivateMemory() : ModulePass(ID) {
  initializeGenXThreadPrivateMemoryPass(*PassRegistry::getPassRegistry());
}

Value *GenXThreadPrivateMemory::ZExtOrTruncIfNeeded(Value *From, Type *To,
                                                    Instruction *InsertBefore) {
  Type *FromTy = From->getType();
  if (FromTy == To)
    return From;

  unsigned FromTySz = FromTy->getPrimitiveSizeInBits();
  unsigned ToTySz = To->getPrimitiveSizeInBits();
  Value *Res = From;
  if (FromTy->isVectorTy() &&
      cast<IGCLLVM::FixedVectorType>(FromTy)->getNumElements() == 1) {
    auto *TmpRes = CastInst::CreateBitOrPointerCast(
        Res, cast<VectorType>(FromTy)->getElementType(), "", InsertBefore);
    Res = TmpRes;
  }
  if (auto *ToVTy = dyn_cast<IGCLLVM::FixedVectorType>(To)) {
    IRBuilder<> Builder(InsertBefore);
    Res = Builder.CreateVectorSplat(
        ToVTy->getNumElements(),
        Res,
        Res->getName() + ".splat");
  }
  if (FromTySz < ToTySz)
    Res = CastInst::CreateZExtOrBitCast(Res, To, "", InsertBefore);
  else if (FromTySz > ToTySz)
    Res = CastInst::CreateTruncOrBitCast(Res, To, "", InsertBefore);
  return Res;
}

// 32u is max exec_size allowed (see GenXCisaBuilder.cpp:buildIntrinsic
// GetExecSize lambda) For svm.gather/scatter:
//    BlockSize is inferred from vec elem type
//    BlockNum should be TotalMemSize / (ExecSize * BlockSize)
//      where TotalMemSize is a total amount of mem read/written for
//      gather/scatter
// TODO: revise this for non-svm case
static int getNumBlocksForType(Type *Ty, const DataLayout &DL) {
  return DL.getTypeSizeInBits(Ty) /
         (std::min<unsigned>(
              32u, cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements()) *
          DL.getTypeSizeInBits(Ty->getScalarType()));
}

// Wipe all internal ConstantExprs out of V if it's a ConstantVector of function pointers
Value *GenXThreadPrivateMemory::NormalizeFuncPtrVec(Value *V, Instruction *InsPoint) {
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
        auto *F = cast_or_null<Function>(
            getFunctionPointerFunc(I->getVectorOperand()));
        IGC_ASSERT(F);
        return ConstantExpr::getPtrToInt(F, IntegerType::getInt64Ty(*m_ctx));
      });
  auto *NewCV = ConstantVector::get(NewVector);
  IGC_ASSERT(m_DL->getTypeSizeInBits(V->getType()) ==
             m_DL->getTypeSizeInBits(NewCV->getType()));
  return NewCV;
}

// If data is a vector of double/int64, bitcast each element to 2 int32.
// If data is a vector of function pointers, strip all internal bitcasts
// and possible extractelems (64->8xi8 cast case) to get a vector of int64s.
// If data is a vector of type < 32bit, extend each element in order to create
// proper send instruction in the finalizer.
std::pair<Value *, unsigned>
GenXThreadPrivateMemory::NormalizeVector(Value *From, Type *To,
                                         Instruction *Inst) {
  Type *I32Ty = Type::getInt32Ty(Inst->getContext());
  Type *I64Ty = Type::getInt64Ty(Inst->getContext());
  Value *Res = From;
  Type *FromTy = From->getType();
  IGC_ASSERT(isa<VectorType>(FromTy));
  unsigned NumElts = cast<IGCLLVM::FixedVectorType>(FromTy)->getNumElements();
  static_assert(genx::ByteBits);
  unsigned EltSz =
      m_DL->getTypeSizeInBits(FromTy->getScalarType()) / genx::ByteBits;
  IGC_ASSERT(EltSz > 0);
  if (isFuncPointerVec(From) &&
      m_DL->getTypeSizeInBits(From->getType()->getScalarType()) <
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
  } else if (m_DL->getTypeSizeInBits(cast<VectorType>(To)->getElementType()) <
             genx::DWordBits) {
    auto EltTy = cast<VectorType>(To)->getElementType();
    auto EltTySz = m_DL->getTypeSizeInBits(EltTy);
    // if EltTy not Integer we shoud bitcast it before ZE
    if (!EltTy->isIntegerTy()) {
      auto Ty = IntegerType::get(*m_ctx, EltTySz);
      To = IGCLLVM::FixedVectorType::get(Ty, NumElts);
      From = CastInst::Create(Instruction::BitCast, From, To, "", Inst);
    }
    To = IGCLLVM::FixedVectorType::get(I32Ty, NumElts);
    Res = CastInst::CreateZExtOrBitCast(From, To, "", Inst);
  } else if (m_DL->getTypeSizeInBits(cast<VectorType>(To)->getElementType()) ==
             genx::QWordBits) {
    if (From->getType()->getScalarType()->isPointerTy()) {
      auto *NewType = IGCLLVM::FixedVectorType::get(I64Ty, NumElts);
      From = CastInst::Create(CastInst::PtrToInt, From, NewType, "", Inst);
      To = IGCLLVM::FixedVectorType::get(I64Ty, NumElts);
      EltSz = I64Ty->getPrimitiveSizeInBits() / genx::ByteBits;
    }
    if (!m_useGlobalMem) {
      NumElts *= 2;
      EltSz = I32Ty->getPrimitiveSizeInBits() / genx::ByteBits;
      To = IGCLLVM::FixedVectorType::get(I32Ty, NumElts);
    }
    Res = CastInst::CreateBitOrPointerCast(From, To, "", Inst);
  }

  return std::make_pair(Res, EltSz);
}

Instruction *
GenXThreadPrivateMemory::RestoreVectorAfterNormalization(Instruction *From,
                                                         Type *To) {
  if (From->getType() == To)
    return From;
  Instruction *Restored = From;
  unsigned EltSz = m_DL->getTypeSizeInBits(To->getScalarType());
  IGC_ASSERT(EltSz > 0);
  if (To->getScalarType()->isPointerTy() &&
      To->getScalarType()->getPointerElementType()->isFunctionTy()) {
    auto *NewFrom = From;
    if (From->getType()->isVectorTy() &&
        From->getType()->getScalarType()->isIntegerTy(genx::DWordBits)) {
      auto *NewTy = IGCLLVM::FixedVectorType::get(
          Type::getInt64Ty(*m_ctx),
          cast<IGCLLVM::FixedVectorType>(From->getType())->getNumElements() /
              2);
      NewFrom = CastInst::CreateBitOrPointerCast(From, NewTy);
      NewFrom->insertAfter(From);
      From = NewFrom;
    }
    Restored = CastInst::Create(Instruction::IntToPtr, NewFrom, To);
  } else if (EltSz < genx::DWordBits) {
    auto EltTy = cast<VectorType>(To)->getElementType();
    auto EltTySz = m_DL->getTypeSizeInBits(EltTy);
    if (!EltTy->isIntegerTy()) {
      auto Ty = IntegerType::get(*m_ctx, EltTySz);
      auto NumElts = cast<IGCLLVM::FixedVectorType>(To)->getNumElements();
      auto ToTr = IGCLLVM::FixedVectorType::get(Ty, NumElts);
      auto Trunc = CastInst::Create(Instruction::Trunc, From, ToTr, "");
      Trunc->insertAfter(From);
      From = Trunc;
      Restored = CastInst::Create(Instruction::BitCast, Trunc, To, "");
    } else {
      Restored = CastInst::Create(Instruction::Trunc, From, To, "");
    }
  } else if (EltSz == genx::QWordBits &&
             !(m_useGlobalMem && To->getScalarType()->isIntegerTy(64))) {
    if (!From->getType()->getScalarType()->isPointerTy() &&
        To->getScalarType()->isPointerTy()) {
      if (!m_useGlobalMem) {
        IGC_ASSERT(
            From->getType()->getScalarType()->isIntegerTy(genx::DWordBits));
        Type *NewTy = IGCLLVM::FixedVectorType::get(
            Type::getInt64Ty(*m_ctx),
            cast<IGCLLVM::FixedVectorType>(From->getType())->getNumElements() /
                2);
        auto *NewFrom = CastInst::CreateBitOrPointerCast(From, NewTy);
        NewFrom->insertAfter(From);
        From = NewFrom;
      }
      Restored = CastInst::Create(CastInst::IntToPtr, From, To);
    } else
      Restored = CastInst::CreateBitOrPointerCast(From, To);
  }
  if (Restored != From)
    Restored->insertAfter(From);
  return Restored;
}

static Value *DoubleVector(Value *OrigVector, unsigned ShiftVal,
                           Instruction *InsertPoint) {
  IRBuilder<> Builder(InsertPoint);
  Type *I32Ty = Type::getInt32Ty(InsertPoint->getContext());
  auto *OrigVectorTy = cast<IGCLLVM::FixedVectorType>(OrigVector->getType());
  unsigned NumElts = OrigVectorTy->getNumElements() * 2;
  Type *OrigVectorEltTy = OrigVectorTy->getElementType();
  Value *NewElts =
      UndefValue::get(IGCLLVM::FixedVectorType::get(OrigVectorEltTy, NumElts));
  for (unsigned CurEltNum = 0; CurEltNum * 2 < NumElts; ++CurEltNum) {
    Value *OldIdx = ConstantInt::get(I32Ty, CurEltNum);
    Value *NewIdx = ConstantInt::get(I32Ty, CurEltNum * 2);
    Value *EltOld = Builder.CreateExtractElement(OrigVector, OldIdx);
    NewElts = Builder.CreateInsertElement(NewElts, EltOld, NewIdx);
    NewIdx = ConstantInt::get(I32Ty, CurEltNum * 2 + 1);
    if (ShiftVal) {
      Value *TyShift = ConstantInt::get(I32Ty, ShiftVal);
      EltOld = Builder.CreateAdd(EltOld, TyShift);
    }
    NewElts = Builder.CreateInsertElement(NewElts, EltOld, NewIdx);
  }

  return NewElts;
}

static Value *FormEltsOffsetVector(unsigned NumElts, unsigned TySz,
                                   Instruction *InsertBefore) {
  IRBuilder<> Builder(InsertBefore);
  Type *I32Ty = Type::getInt32Ty(InsertBefore->getContext());
  Value *EltsOffset =
      UndefValue::get(IGCLLVM::FixedVectorType::get(I32Ty, NumElts));
  for (unsigned CurElt = 0; CurElt < NumElts; ++CurElt) {
    Value *Idx = ConstantInt::get(I32Ty, CurElt);
    Value *EltOffset = ConstantInt::get(I32Ty, CurElt * TySz);
    EltsOffset = Builder.CreateInsertElement(EltsOffset, EltOffset, Idx);
  }

  return EltsOffset;
}

static Value *FormEltsOffsetVectorForSVM(Value *BaseOffset,
                                         Value *Offsets,
                                         Instruction *InsertBefore) {
  IGC_ASSERT(BaseOffset->getType()->isIntegerTy(64));
  IGC_ASSERT(Offsets->getType()->isVectorTy());

  IRBuilder<> Builder(InsertBefore);
  Type *I64Ty = Type::getInt64Ty(InsertBefore->getContext());
  unsigned NumElts =
      cast<IGCLLVM::FixedVectorType>(Offsets->getType())->getNumElements();
  Value *BaseOffsets = Builder.CreateVectorSplat(NumElts, BaseOffset);
  if (!Offsets->getType()->getScalarType()->isIntegerTy(64))
    Offsets = Builder.CreateZExtOrBitCast(Offsets,
        IGCLLVM::FixedVectorType::get(I64Ty, NumElts));
  return Builder.CreateAdd(BaseOffsets, Offsets);
}

Value *GenXThreadPrivateMemory::lookForPtrReplacement(Value *Ptr) const {
  Type *PtrTy = Ptr->getType();
  IGC_ASSERT(PtrTy->isPtrOrPtrVectorTy());

  Type *MemTy = IntegerType::get(*m_ctx, (m_useGlobalMem ? 64 : 32));
  if (isa<UndefValue>(Ptr)) {
    if (auto *PtrVecTy = dyn_cast<IGCLLVM::FixedVectorType>(PtrTy))
      return UndefValue::get(
          IGCLLVM::FixedVectorType::get(MemTy, PtrVecTy->getNumElements()));
    return UndefValue::get(MemTy);
  } else if (auto BC = dyn_cast<BitCastInst>(Ptr))
    return lookForPtrReplacement(BC->getOperand(0));
  else if (auto ITP = dyn_cast<IntToPtrInst>(Ptr))
    return ITP->getOperand(0);
  else if (auto AI = dyn_cast<AllocaInst>(Ptr)) {
    auto AllocaIntr = m_allocaToIntrinsic.find(AI);
    IGC_ASSERT(AllocaIntr != m_allocaToIntrinsic.end());
    return AllocaIntr->second;
  } else if (isa<Argument>(Ptr)) {
    if (PtrTy->isPointerTy()) {
      auto *PTI = CastInst::Create(CastInst::PtrToInt, Ptr, MemTy);
      PTI->insertBefore(&cast<Argument>(Ptr)->getParent()->front().front());
      return PTI;
    } else
      return Ptr;
  }
  else if (isa<ExtractElementInst>(Ptr) &&
           lookForPtrReplacement(
               cast<ExtractElementInst>(Ptr)->getVectorOperand())) {
    if (PtrTy->isPointerTy()) {
      auto* PTI = CastInst::Create(Instruction::PtrToInt, Ptr, MemTy);
      PTI->insertAfter(cast<Instruction>(Ptr));
      return PTI;
    }
    else
      return Ptr;
  } else if (auto *SHI = dyn_cast<ShuffleVectorInst>(Ptr)) {
    auto Splat = ShuffleVectorAnalyzer(SHI).getAsSplat();
    if (Splat.Input) {
      return lookForPtrReplacement(Splat.Input);
    }
  } else if (auto *CI = dyn_cast<CallInst>(Ptr)) {
    if (!IGCLLVM::isIndirectCall(*CI) &&
        (vc::getAnyIntrinsicID(CI->getCalledFunction()) ==
             GenXIntrinsic::genx_svm_block_ld ||
         vc::getAnyIntrinsicID(CI->getCalledFunction()) ==
             GenXIntrinsic::genx_svm_gather)) {
      return Ptr;
    }
  } else if (auto *LI = dyn_cast<LoadInst>(Ptr)) {
    // meeting load means we're processing load's user earlier
    // than the load itself, which is possible because we could
    // reach load's user earlier in the du chains thru some other value
    // generate cast for now
    auto *Cast = CastInst::Create(Instruction::PtrToInt, LI, MemTy, "");
    Cast->insertAfter(LI);
    return Cast;
  } else if (auto *ASCast = dyn_cast<AddrSpaceCastInst>(Ptr)) {
    return lookForPtrReplacement(ASCast->getPointerOperand());
  } else if (isa<ConstantPointerNull>(Ptr))
    return ConstantInt::get(MemTy, 0);

  vc::diagnose(Ptr->getContext(), "TPM", Ptr,
               "Cannot find pointer replacement");
  return nullptr; // to suppress warnings
}

static std::pair<Value *, Value *>
castValuesToCommonType(Value *V1, Value *V2, Instruction *InsertBefore) {
  auto *V1T = V1->getType();
  auto *V2T = V2->getType();
  if (V1T == V2T)
    return {V1, V2};

  auto *V1I = dyn_cast<IntegerType>(V1T);
  auto *V2I = dyn_cast<IntegerType>(V2T);
  if (V1I && V2I) {
    IGC_ASSERT(V1I->getBitWidth() != V2I->getBitWidth());
    // Integer here is some pointer representation, thus using zero extension
    if (V1I->getBitWidth() < V2I->getBitWidth())
      V1 = new ZExtInst(V1, V2I, V1->getName() + ".common.ty", InsertBefore);
    else
      V2 = new ZExtInst(V2, V1I, V2->getName() + ".common.ty", InsertBefore);
    return {V1, V2};
  }

  IGC_ASSERT_MESSAGE(0, "Cannot find common type for values");
  return {V1, V2};
}

bool GenXThreadPrivateMemory::replaceAddrSpaceCast(
  AddrSpaceCastInst* AddrCast) {
  auto NewAlloca = lookForPtrReplacement(AddrCast->getPointerOperand());

  auto IntToPtr = IntToPtrInst::Create(
    llvm::Instruction::CastOps::IntToPtr, NewAlloca,
    AddrCast->getPointerOperand()->getType(), "", AddrCast);
  auto NewAddrCast =
    AddrSpaceCastInst::Create(llvm::Instruction::CastOps::AddrSpaceCast,
    IntToPtr, AddrCast->getType(), "", AddrCast);

  AddrCast->replaceAllUsesWith(NewAddrCast);
  AddrCast->eraseFromParent();

  return true;
}

bool GenXThreadPrivateMemory::replaceInsertElement(InsertElementInst *Insert) {
  LLVM_DEBUG(dbgs() << "Replacing insert element inst " << *Insert
                    << " ===>\n");
  auto InsertTy = cast<VectorType>(Insert->getType());
  if (!InsertTy->isPtrOrPtrVectorTy())
    return false;

  Value *Vec = Insert->getOperand(0);
  Value *Elt = Insert->getOperand(1);
  Value *Idx = Insert->getOperand(2);

  Value *NewVec = lookForPtrReplacement(Vec);
  auto NewElt = lookForPtrReplacement(Elt);
  auto NewInsert = InsertElementInst::Create(NewVec, NewElt, Idx,
                                             Insert->getName() + ".tpm");
  NewInsert->insertAfter(Insert);

  auto CastToOldTy =
      CastInst::Create(Instruction::IntToPtr, NewInsert, InsertTy,
                       NewInsert->getName() + ".temp.itp");
  CastToOldTy->insertAfter(NewInsert);
  Insert->replaceAllUsesWith(CastToOldTy);
  Insert->eraseFromParent();

  LLVM_DEBUG(dbgs() << *CastToOldTy << "\n");
  return true;
}

bool GenXThreadPrivateMemory::replaceShuffleVector(
    ShuffleVectorInst *ShuffleVec) {
  LLVM_DEBUG(dbgs() << "Replacing insert element inst " << *ShuffleVec
                    << " ===>\n");
  auto ShuffleTy = cast<VectorType>(ShuffleVec->getType());
  if (!ShuffleTy->isPtrOrPtrVectorTy())
    return false;

  Value *Vec1 = ShuffleVec->getOperand(0);
  Value *Vec2 = ShuffleVec->getOperand(1);

  Value *NewVec1 = lookForPtrReplacement(Vec1);
  Value *NewVec2 = lookForPtrReplacement(Vec2);
  auto NewShuffleVec = new ShuffleVectorInst(
      NewVec1, NewVec2, IGCLLVM::getShuffleMaskForBitcode(ShuffleVec), ShuffleVec->getName() + ".tpm");
  NewShuffleVec->insertAfter(ShuffleVec);

  auto CastToOldTy =
      CastInst::Create(Instruction::IntToPtr, NewShuffleVec, ShuffleTy,
                       NewShuffleVec->getName() + ".temp.itp");
  CastToOldTy->insertAfter(NewShuffleVec);
  ShuffleVec->replaceAllUsesWith(CastToOldTy);
  ShuffleVec->eraseFromParent();

  LLVM_DEBUG(dbgs() << *CastToOldTy << "\n");
  return true;
}

bool GenXThreadPrivateMemory::replaceLoad(LoadInst *LdI) {
  LLVM_DEBUG(dbgs() << "Replacing load " << *LdI << " ===>\n");
  IRBuilder<> Builder(LdI);
  Type *LdTy = LdI->getType();
  Type *LdEltTy = LdTy;
  if (isa<VectorType>(LdEltTy))
    LdEltTy = cast<VectorType>(LdEltTy)->getElementType();
  // we can make one-element vectors from primitive types
  else if (LdTy->isIntegerTy() || LdTy->isFloatingPointTy() ||
           LdTy->isPointerTy())
    LdTy = IGCLLVM::FixedVectorType::get(LdTy, 1);
  else {
    vc::diagnose(LdI->getContext(), "TPM", LdTy,
                 "Unsupported type inside replaceLoad");
  }

  unsigned NumEltsToLoad =
      cast<IGCLLVM::FixedVectorType>(LdTy)->getNumElements();
  unsigned ValueEltSz = m_DL->getTypeSizeInBits(LdEltTy) / genx::ByteBits;

  Value *PredVal = ConstantInt::get(Type::getInt1Ty(*m_ctx), 1);
  Value *Pred = Builder.CreateVectorSplat(NumEltsToLoad, PredVal);

  Type *I32Ty = Type::getInt32Ty(*m_ctx);
  Type *I64Ty = Type::getInt64Ty(*m_ctx);
  Value *OldValOfTheDataRead =
      Builder.CreateVectorSplat(NumEltsToLoad, UndefValue::get(LdEltTy));
  std::tie(OldValOfTheDataRead, ValueEltSz) =
      NormalizeVector(OldValOfTheDataRead, LdTy, LdI);
  NumEltsToLoad = cast<IGCLLVM::FixedVectorType>(OldValOfTheDataRead->getType())
                      ->getNumElements();

  Value *PointerOp = LdI->getPointerOperand();
  Value *Offset = lookForPtrReplacement(PointerOp);
  Offset =
      ZExtOrTruncIfNeeded(Offset, m_useGlobalMem ? I64Ty : I32Ty, LdI);
  auto IID = m_useGlobalMem
                 ? llvm::GenXIntrinsic::genx_svm_gather
                 : llvm::GenXIntrinsic::genx_gather_scaled;

  Value *EltsOffset = FormEltsOffsetVector(NumEltsToLoad, ValueEltSz, LdI);

  // always one element for one channel
  Value *logNumBlocks = ConstantInt::get(I32Ty, 0);
  Value *Scale = ConstantInt::get(Type::getInt16Ty(*m_ctx), 0);
  Value *Surface = ConstantInt::get(I32Ty,
                                    visa::getReservedSurfaceIndex(m_stack));
  if (m_useGlobalMem)
    Offset = FormEltsOffsetVectorForSVM(Offset, EltsOffset, LdI);
  Function *F = GenXIntrinsic::getGenXDeclaration(
      LdI->getModule(), IID,
      {OldValOfTheDataRead->getType(),
      Pred->getType(),
       (m_useGlobalMem ? Offset : EltsOffset)->getType()});
  CallInst *Gather =
      m_useGlobalMem
          ? IntrinsicInst::Create(
                F, {Pred, logNumBlocks, Offset, OldValOfTheDataRead},
                LdI->getName())
          : IntrinsicInst::Create(F,
                                  {Pred, logNumBlocks, Scale, Surface, Offset,
                                   EltsOffset, OldValOfTheDataRead},
                                  LdI->getName());
  Gather->insertAfter(LdI);
  Instruction *ProperGather = RestoreVectorAfterNormalization(Gather, LdTy);

  if (!isa<VectorType>(LdI->getType()) &&
      isa<VectorType>(ProperGather->getType())) {
    auto *GatheredTy = cast<IGCLLVM::FixedVectorType>(ProperGather->getType());
    Builder.ClearInsertionPoint();
    Instruction *LdVal = nullptr;
    if (GatheredTy->getNumElements() == 1)
      LdVal = cast<Instruction>(Builder.CreateExtractElement(
          ProperGather, static_cast<uint64_t>(0ul),
          ProperGather->getName() + ".tpm.loadres"));
    else
      LdVal = cast<Instruction>(Builder.CreateBitOrPointerCast(
          ProperGather, LdI->getType(),
          ProperGather->getName() + ".tpm.loadres"));
    LdVal->insertAfter(ProperGather);
    ProperGather = LdVal;
  }

  Gather->setMetadata(vc::InstMD::SVMBlockType,
                      MDNode::get(*m_ctx, llvm::ValueAsMetadata::get(
                                              UndefValue::get(LdEltTy))));

  LLVM_DEBUG(dbgs() << *Gather << "\n");
  LdI->replaceAllUsesWith(ProperGather);
  LdI->eraseFromParent();

  return true;
}

bool GenXThreadPrivateMemory::replaceStore(StoreInst *StI) {
  LLVM_DEBUG(dbgs() << "Replacing store " << *StI << " ===>\n");
  IRBuilder<> Builder(StI);
  Value *ValueOp = StI->getValueOperand();
  Type *ValueOpTy = ValueOp->getType();
  if (ValueOpTy->isIntOrPtrTy() || ValueOpTy->isFloatingPointTy()) {
    ValueOp = Builder.CreateVectorSplat(1, ValueOp);
    ValueOpTy = ValueOp->getType();
  }
  if (!ValueOpTy->isVectorTy()) {
    vc::diagnose(StI->getContext(), "TPM", ValueOpTy,
                 "Unsupported type inside replaceStore");
  }

  unsigned ValueEltSz = 0;
  std::tie(ValueOp, ValueEltSz) = NormalizeVector(ValueOp, ValueOpTy, StI);
  unsigned ValueNumElts =
      cast<IGCLLVM::FixedVectorType>(ValueOp->getType())->getNumElements();

  Value *PointerOp = StI->getPointerOperand();
  Value *Offset = lookForPtrReplacement(PointerOp);
  Type *I32Ty = Type::getInt32Ty(*m_ctx);
  Type *I64Ty = Type::getInt64Ty(*m_ctx);
  Offset =
      ZExtOrTruncIfNeeded(Offset, m_useGlobalMem ? I64Ty : I32Ty, StI);

  auto IID = m_useGlobalMem
                 ? llvm::GenXIntrinsic::genx_svm_scatter
                 : llvm::GenXIntrinsic::genx_scatter_scaled;

  Value *PredVal = ConstantInt::get(Type::getInt1Ty(*m_ctx), 1);
  Value *Pred = Builder.CreateVectorSplat(ValueNumElts, PredVal);
  Value *EltsOffset = FormEltsOffsetVector(ValueNumElts, ValueEltSz, StI);

  if (m_useGlobalMem)
    Offset = FormEltsOffsetVectorForSVM(Offset, EltsOffset, StI);

  Function *F = GenXIntrinsic::getGenXDeclaration(
      StI->getModule(), IID,
      {Pred->getType(),
       (m_useGlobalMem ? Offset : EltsOffset)->getType(),
       ValueOp->getType()});

  // always one element for one channel
  Value *logNumBlocks = ConstantInt::get(I32Ty, 0);
  Value *Scale = ConstantInt::get(Type::getInt16Ty(*m_ctx), 0);
  Value *Surface = ConstantInt::get(I32Ty,
                                    visa::getReservedSurfaceIndex(m_stack));
  auto *Scatter =
      m_useGlobalMem
          ? IntrinsicInst::Create(F, {Pred, logNumBlocks, Offset, ValueOp},
                                  StI->getName())
          : IntrinsicInst::Create(F,
                                  {Pred, logNumBlocks, Scale, Surface, Offset,
                                   EltsOffset, ValueOp},
                                  StI->getName());
  Scatter->insertAfter(StI);
  StI->eraseFromParent();

  Scatter->setMetadata(
      vc::InstMD::SVMBlockType,
      MDNode::get(*m_ctx, llvm::ValueAsMetadata::get(
                              UndefValue::get(ValueOpTy->getScalarType()))));

  LLVM_DEBUG(dbgs() << *Scatter << "\n");

  return true;
}

bool GenXThreadPrivateMemory::replacePTI(PtrToIntInst *PTI) {
  LLVM_DEBUG(dbgs() << "Replacing PTI " << *PTI << " ===> ");
  Value *PointerOp = PTI->getPointerOperand();
  Value *Offset = lookForPtrReplacement(PointerOp);

  if (isa<Argument>(Offset))
    return false;

  Offset = ZExtOrTruncIfNeeded(Offset, PTI->getDestTy(), PTI);
  LLVM_DEBUG(dbgs() << *Offset << "\n");
  PTI->replaceAllUsesWith(Offset);
  PTI->eraseFromParent();

  return true;
}

Value *GenXThreadPrivateMemory::lookForTruncOffset(Value *V) {
  if (auto *I = dyn_cast<TruncInst>(V))
    return I->getOperand(0);
  else {
    // TODO: extend the list of supported instruction types
    if (auto *I = dyn_cast<BinaryOperator>(V)) {
      for (unsigned i = 0; i < I->getNumOperands(); ++i) {
        auto *Op = I->getOperand(i);
        if (Value *Off = lookForTruncOffset(Op); Off != Op) {
          if (I->getType() != Off->getType()) {
            auto *OtherOp = I->getOperand((i + 1) % 2);
            OtherOp = ZExtOrTruncIfNeeded(OtherOp, Off->getType(), I);
            if (i == 0)
              I = BinaryOperator::Create(I->getOpcode(), Off, OtherOp,
                                         I->getName(), I);
            else
              I = BinaryOperator::Create(I->getOpcode(), OtherOp, Off,
                                         I->getName(), I);
          }
          return I;
        }
      }
    }
    return V;
  }
}

bool GenXThreadPrivateMemory::replaceGatherPrivate(CallInst *CI) {
  LLVM_DEBUG(dbgs() << "Replacing gather.priv " << *CI << " ===>\n");
  auto IID = m_useGlobalMem ? llvm::GenXIntrinsic::genx_svm_gather
                            : llvm::GenXIntrinsic::genx_gather_scaled;

  Type *OrigDstTy = CI->getType();
  IGC_ASSERT(isa<VectorType>(OrigDstTy));
  Type *NewDstTy = OrigDstTy;
  Value *OldValue = CI->getArgOperand(3);
  unsigned ValueEltSz =
      m_DL->getTypeSizeInBits(NewDstTy->getScalarType()) / genx::ByteBits;

  // Check gather.private invariant.
  IGC_ASSERT(NewDstTy == OldValue->getType());

  // Cast data type to legal.
  // Consider i64 legal for SVM cases
  if (!(m_useGlobalMem && CI->getType()->getScalarType()->isIntegerTy(64)))
    std::tie(OldValue, ValueEltSz) = NormalizeVector(OldValue, NewDstTy, CI);
  NewDstTy = OldValue->getType();
  unsigned ValueNumElts =
      cast<IGCLLVM::FixedVectorType>(NewDstTy)->getNumElements();

  Value *Pred = CI->getArgOperand(0);
  Value *EltsOffset = CI->getArgOperand(2);
  if (!m_useGlobalMem &&
      m_DL->getTypeSizeInBits(cast<VectorType>(OrigDstTy)->getElementType()) ==
          genx::QWordBits) {
    IGC_ASSERT(ValueNumElts ==
               cast<IGCLLVM::FixedVectorType>(EltsOffset->getType())
                       ->getNumElements() *
                   2);
    EltsOffset = DoubleVector(EltsOffset, ValueEltSz, CI);
    Pred = DoubleVector(Pred, 0, CI);
  }

  Type *I32Ty = Type::getInt32Ty(*m_ctx);
  Type *I64Ty = Type::getInt64Ty(*m_ctx);
  Value *PointerOp = CI->getOperand(1);
  Value *Offset = lookForPtrReplacement(PointerOp);
  Offset = ZExtOrTruncIfNeeded(Offset, m_useGlobalMem ? I64Ty : I32Ty, CI);

  if (m_useGlobalMem)
    Offset = FormEltsOffsetVectorForSVM(lookForTruncOffset(Offset), EltsOffset, CI);

  Function *F = GenXIntrinsic::getGenXDeclaration(
      CI->getModule(), IID,
      {NewDstTy, Pred->getType(),
       (m_useGlobalMem ? Offset : EltsOffset)->getType()});

  unsigned NumBlocks =
      (m_useGlobalMem) ? getNumBlocksForType(NewDstTy, *m_DL) : ValueEltSz;
  Value *logNumBlocks = ConstantInt::get(I32Ty, genx::log2(NumBlocks));
  Value *Scale = ConstantInt::get(Type::getInt16Ty(*m_ctx), 0);
  Value *Surface =
      ConstantInt::get(I32Ty, visa::getReservedSurfaceIndex(m_stack));

  CallInst *Gather =
      m_useGlobalMem
          ? IntrinsicInst::Create(F, {Pred, logNumBlocks, Offset, OldValue},
                                  CI->getName())
          : IntrinsicInst::Create(F,
                                  {Pred, logNumBlocks, Scale, Surface, Offset,
                                   EltsOffset, OldValue},
                                  CI->getName());
  Gather->insertAfter(CI);
  LLVM_DEBUG(dbgs() << *Gather << "\n");

  Instruction *ProperGather =
      RestoreVectorAfterNormalization(Gather, OrigDstTy);
  CI->replaceAllUsesWith(ProperGather);
  CI->eraseFromParent();

  return true;
}

bool GenXThreadPrivateMemory::replaceScatterPrivate(CallInst *CI) {
  LLVM_DEBUG(dbgs() << "Replacing scatter.priv " << *CI << " ===>\n");
  auto IID = m_useGlobalMem
                 ? llvm::GenXIntrinsic::genx_svm_scatter
                 : llvm::GenXIntrinsic::genx_scatter_scaled;
  Value *ValueOp = CI->getArgOperand(3);
  Type *OrigValueTy = ValueOp->getType();
  IGC_ASSERT(isa<VectorType>(OrigValueTy));
  unsigned EltSz = 0;
  std::tie(ValueOp, EltSz) = NormalizeVector(ValueOp, ValueOp->getType(), CI);

  Value *Pred = CI->getArgOperand(0);
  Value *EltsOffset = CI->getArgOperand(2);
  if (m_DL->getTypeSizeInBits(
          cast<VectorType>(OrigValueTy)->getElementType()) == genx::QWordBits) {
    // TODO: revisit this for splat and/or non-const value cases,
    // e.g. replace EltSz with  (isSplatValue(EltsOffset) ||
    //                          !isa<Constant>(EltsOffset)) ? 0 : EltSZ
    EltsOffset = DoubleVector(EltsOffset, EltSz, CI);
    Pred = DoubleVector(Pred, 0, CI);
  }

  Value *ScatterPtr = CI->getArgOperand(1);
  Type *I32Ty = Type::getInt32Ty(*m_ctx),
       *I64Ty = Type::getInt64Ty(*m_ctx);
  Value *Offset = lookForPtrReplacement(ScatterPtr);
  Offset = ZExtOrTruncIfNeeded(Offset, m_useGlobalMem ? I64Ty : I32Ty, CI);

  if (m_useGlobalMem) {
    Offset =
        FormEltsOffsetVectorForSVM(lookForTruncOffset(Offset), EltsOffset, CI);
    if (!Offset->getType()->getScalarType()->isIntegerTy(64))
      Offset = CastInst::CreateZExtOrBitCast(
          Offset,
          IGCLLVM::FixedVectorType::get(
              I64Ty, cast<IGCLLVM::FixedVectorType>(EltsOffset->getType())
                         ->getNumElements()),
          "", CI);
  }

  Function *F = GenXIntrinsic::getGenXDeclaration(
      CI->getModule(), IID,
      {Pred->getType(), Offset->getType(), ValueOp->getType()});

  Value *LogNumBlocks = ConstantInt::get(I32Ty, genx::log2(EltSz));
  Value *Scale = ConstantInt::get(Type::getInt16Ty(*m_ctx), 0); // scale is always 0
  Value *Surface = ConstantInt::get(I32Ty,
                                    visa::getReservedSurfaceIndex(m_stack));
  CallInst *ScatterStScaled =
      m_useGlobalMem
          ? IntrinsicInst::Create(F, {Pred, LogNumBlocks, Offset, ValueOp})
          : IntrinsicInst::Create(F, {Pred, LogNumBlocks, Scale, Surface,
                                      Offset, EltsOffset, ValueOp});
  ScatterStScaled->insertAfter(CI);
  LLVM_DEBUG(dbgs() << *ScatterStScaled << "\n");
  CI->replaceAllUsesWith(ScatterStScaled);
  CI->eraseFromParent();

  return true;
}

bool GenXThreadPrivateMemory::replacePhi(PHINode *Phi) {
  SmallVector<Value *, 8> PhiOps;
  for (auto &IncVal : Phi->incoming_values())
    PhiOps.push_back(lookForPtrReplacement(static_cast<Value *>(IncVal.get())));

  IGC_ASSERT(!PhiOps.empty());

  // first we need to synchronize operands of types T and <1 x T> =>
  // make all of them scalar T
  auto NonVecOpIt = std::find_if(PhiOps.begin(), PhiOps.end(), [](Value *V) {
    return !V->getType()->isVectorTy();
  });
  if (NonVecOpIt != PhiOps.end()) {
    auto *NonVecTy = (*NonVecOpIt)->getType();

    auto TypeFixer = [NonVecTy, PhiOps](Value *&V) {
      if (V->getType() == NonVecTy)
        return;
      else if (V->getType()->getScalarType() == NonVecTy->getScalarType() &&
               V->getType()->isVectorTy() != NonVecTy->isVectorTy()) {
        if (V->getType()->isVectorTy()) {
          IGC_ASSERT(
              cast<IGCLLVM::FixedVectorType>(V->getType())->getNumElements() ==
              1);
          auto *VCast = CastInst::Create(CastInst::BitCast, V, NonVecTy->getScalarType());
          VCast->insertAfter(cast<Instruction>(V));
          V = VCast;
        }
      } else {
        IGC_ASSERT_MESSAGE(0, "New phi types mismatch");
      }
    };
    std::for_each(PhiOps.begin(), PhiOps.end(), TypeFixer);
  }

  Type *OffsetTy = PhiOps[0]->getType();
  auto TypeChecker = [OffsetTy](Value *V) { return OffsetTy == V->getType(); };
  IGC_ASSERT(std::all_of(PhiOps.begin(), PhiOps.end(), TypeChecker));

  PHINode *NewPhi = PHINode::Create(OffsetTy, PhiOps.size());
  for (unsigned i = 0; i < PhiOps.size(); ++i)
    NewPhi->addIncoming(PhiOps[i], Phi->getIncomingBlock(i));

  NewPhi->insertAfter(Phi);

  // Create temporary cast instruction to satisfy old phi users. Types must be
  // different due to replacement pointer by integer offset.
  IGC_ASSERT(NewPhi->getType() != Phi->getType());
  CastInst *TempCast = CastInst::CreateBitOrPointerCast(NewPhi, Phi->getType());
  TempCast->insertAfter(NewPhi->getParent()->getFirstNonPHI());

  Phi->replaceAllUsesWith(TempCast);
  Phi->eraseFromParent();

  return true;
}

// |--%1 = PHI(%2, ...)
// |         ^
// |         |
// |         |
// |  %2 = PHI(%1, ...)
// |---------^
//
// In this situation, it's difficult to find the origin of the pointer. PtrToInt
// and IntToPtr break the process of searching (see lookForPtrReplacement) and
// it helps to 'emulate' phi in TPM
bool GenXThreadPrivateMemory::preparePhiForReplacement(PHINode *Phi) {
  if (!isa<PointerType>(Phi->getType()))
    return false;

  Type *I64Ty = Type::getInt64Ty(Phi->getContext());
  StringRef Name = Phi->getName();
  Instruction *TempPtrToInt = CastInst::Create(
      Instruction::PtrToInt, Phi, I64Ty, Name + ".tpm.temp.pti",
      Phi->getParent()->getFirstNonPHI());
  Instruction *TempIntToPtr =
      CastInst::Create(Instruction::IntToPtr, TempPtrToInt, Phi->getType(),
                       Name + ".tpm.temp.itp");
  TempIntToPtr->insertAfter(TempPtrToInt);
  Phi->replaceAllUsesWith(TempIntToPtr);

  // Replacement here was incorrect
  TempPtrToInt->replaceUsesOfWith(TempIntToPtr, Phi);

  return true;
}

bool GenXThreadPrivateMemory::replaceSelect(SelectInst *Sel) {
  Value *Cond = Sel->getCondition();
  Value *TrueValue = lookForPtrReplacement(Sel->getTrueValue());
  Value *FalseValue = lookForPtrReplacement(Sel->getFalseValue());

  std::tie(TrueValue, FalseValue) =
      castValuesToCommonType(TrueValue, FalseValue, Sel);

  SelectInst *NewSel = SelectInst::Create(Cond, TrueValue, FalseValue);
  NewSel->insertAfter(Sel);
  NewSel->setDebugLoc(Sel->getDebugLoc());

  CastInst *TempCast = CastInst::CreateBitOrPointerCast(NewSel, Sel->getType());
  TempCast->insertAfter(NewSel);
  TempCast->setDebugLoc(Sel->getDebugLoc());

  Sel->replaceAllUsesWith(TempCast);
  Sel->eraseFromParent();

  return true;
}

static Value *GetUndefVec(Type *Ty, unsigned NumElts) {
  return UndefValue::get(IGCLLVM::FixedVectorType::get(Ty, NumElts));
}

static std::pair<Value *, Value *> GetUndefPair(Type *Ty, unsigned NumElts) {
  return std::make_pair(GetUndefVec(Ty, NumElts), GetUndefVec(Ty, NumElts));
}

static Value *FillVecWithSeqVals(Value *Vec, unsigned Start,
                                 Instruction *InsertBefore) {
  IRBuilder<> Builder(InsertBefore);
  Builder.SetInsertPoint(InsertBefore);

  Type *I32Ty = Type::getInt32Ty(InsertBefore->getContext());
  unsigned NumElts =
      cast<IGCLLVM::FixedVectorType>(Vec->getType())->getNumElements();
  for (unsigned i = 0; i < NumElts; ++i) {
    Value *Idx = ConstantInt::get(I32Ty, i);
    Value *Val = ConstantInt::get(I32Ty, i + Start);
    Vec = Builder.CreateInsertElement(Vec, Val, Idx);
  }
  return Vec;
}

static std::pair<Value *, Value *>
SplitVec(Value *Vec, unsigned NumElts, Instruction *InsertBefore,
         std::pair<Value *, Value *> Splitters) {
  IRBuilder<> Builder(InsertBefore);
  Builder.SetInsertPoint(InsertBefore);

  Type *EltTy = cast<VectorType>(Vec->getType())->getElementType();
  Value *First = Builder.CreateShuffleVector(Vec, GetUndefVec(EltTy, NumElts),
                                             Splitters.first);
  Value *Second = Builder.CreateShuffleVector(Vec, GetUndefVec(EltTy, NumElts),
                                              Splitters.second);
  return std::make_pair(First, Second);
}

static void EraseUsers(Instruction *Inst) {
  std::forward_list<User *> Users(Inst->user_begin(), Inst->user_end());
  for (auto U : Users) {
    IGC_ASSERT_MESSAGE(
        !isa<StoreInst>(U) &&
            !(isa<CallInst>(U) &&
              (GenXIntrinsic::getGenXIntrinsicID(cast<CallInst>(U)) ==
                   GenXIntrinsic::genx_svm_scatter ||
               GenXIntrinsic::getGenXIntrinsicID(cast<CallInst>(U)) ==
                   GenXIntrinsic::genx_scatter_scaled ||
               GenXIntrinsic::getGenXIntrinsicID(cast<CallInst>(U)) ==
                   GenXIntrinsic::genx_svm_block_st)),
        "Should not erase stores");
    Instruction *PotentiallyDeadInst = cast<Instruction>(U);
    EraseUsers(PotentiallyDeadInst);
    IGC_ASSERT_MESSAGE(U->use_empty(),
                       "Cannot recursively remove users of a replaced alloca");
    PotentiallyDeadInst->eraseFromParent();
  }
}

class SVMChecker {
  static constexpr unsigned LoadsThreshold = 1;

  std::map<Value *, unsigned> Visited;

public:
  // pre-transformation analysis to determine
  // which kind of mem should we place TPM at
  unsigned checkSVMNecessary(Value *V) {
    if (Visited.count(V) > 0)
      return Visited.at(V);
    // do not handle ConstExprs for now
    if (!isa<Instruction>(V) && !isa<Argument>(V))
      return 0;
    unsigned LoadsMet = 0;
    if (isa<LoadInst>(V)) {
      ++LoadsMet;
    } else if (auto *CI = dyn_cast<CallInst>(V)) {
      auto IID = vc::getAnyIntrinsicID(CI);
      if (IID == GenXIntrinsic::genx_gather_private ||
          IID == GenXIntrinsic::genx_scatter_private ||
          // TODO: make this analysis interprocedural
          IID == GenXIntrinsic::not_any_intrinsic) {
        // do not process users of priv mem intrinsics
        // or calls to other functions
        return 0;
      } else if (IID == GenXIntrinsic::genx_svm_gather ||
                 IID == GenXIntrinsic::genx_svm_scatter) {
        // Switch to SVM immediately once we meet some previously
        // generated genx.svm intrinsics communicating with private memory
        // TODO: handling svm.block_ld/st requires support from replace* and
        // split* methods as well
        return LoadsThreshold + 1;
      }
    } else if (isa<PHINode>(V) || isa<ICmpInst>(V)) {
      // do not go thru phi as cycles may appear and
      // it doesn't seem necessary for the analysis now
      return 0;
    }
    unsigned Result = 0;
    for (auto *U : V->users())
      Result = std::max(Result, checkSVMNecessary(U));
    Visited.insert(std::make_pair(V, Result + LoadsMet));
    return Result + LoadsMet;
  }

  bool operator()(Value *V) { return checkSVMNecessary(V) > LoadsThreshold; }
};

void GenXThreadPrivateMemory::switchStack(Module &M) {
  LLVM_DEBUG(dbgs() << "Switching TPM to SVM\n");
  if (!m_ST->isOCLRuntime())
    vc::diagnose(M.getContext(), "TPM",
                 "CMRT not supported for stack switching to SVM");
  m_useGlobalMem = true;
}

void GenXThreadPrivateMemory::addUsers(Value *V, bool ProcessCalls) {
  IGC_ASSERT(isa<Instruction>(V) || isa<Argument>(V));
  if (ProcessCalls) {
    if (auto *CI = dyn_cast<CallInst>(V)) {
      auto IID = vc::getAnyIntrinsicID(CI);
      switch (IID) {
      case GenXIntrinsic::genx_svm_gather:
      case GenXIntrinsic::genx_svm_scatter:
      case GenXIntrinsic::genx_svm_block_ld:
      case GenXIntrinsic::genx_svm_block_st:
      case GenXIntrinsic::genx_gather_private:
      case GenXIntrinsic::genx_scatter_private:
        ProcessCalls = false;
        break;
      default:
        break;
      }
    } else if (isa<LoadInst>(V) || isa<StoreInst>(V))
      ProcessCalls = false;
  }
  for (const auto &U : V->uses()) {
    Instruction *ToAdd = cast<Instruction>(U.getUser());
    if (auto *CI = dyn_cast<CallInst>(ToAdd)) {
      Function *Callee = nullptr;
      if ((Callee = CI->getCalledFunction()) &&
          vc::getAnyIntrinsicID(Callee) ==
              GenXIntrinsic::not_any_intrinsic &&
          V->getType()->isPointerTy() && ProcessCalls) {
        m_Calls[CI->getCalledFunction()].Calls.insert(CI);
        auto *ArgToTrack =
            IGCLLVM::getArg(*Callee, IGCLLVM::getArgOperandNo(*CI, &U));
        LLVM_DEBUG(dbgs() << "Adding arg " << *ArgToTrack << " of "
                          << Callee->getName() << "\nMet in " << *CI << "\n");
        m_Calls[CI->getCalledFunction()].Args.insert(ArgToTrack);
      }
    }
    auto Found = m_AlreadyAdded.find(ToAdd);
    if (Found == m_AlreadyAdded.end()) {
      m_AlreadyAdded.insert(ToAdd);
      m_AIUsers.push({ToAdd, ProcessCalls});
    }
  }
}

void GenXThreadPrivateMemory::collectEachPossibleTPMUsers() {
  collectAllocaUsers();
  collectArgUsers();
}

void GenXThreadPrivateMemory::collectAllocaUsers() {
  IGC_ASSERT(m_AIUsers.empty());
  // At first collect every alloca user
  for (auto Alloca : m_alloca) {
    Instruction *I = dyn_cast<Instruction>(Alloca);
    IGC_ASSERT(I);
    addUsers(I);
  }
}

void GenXThreadPrivateMemory::collectArgUsers(bool ProcessCalls) {
  // IGC_ASSERT(m_AIUsers.empty());
  // Then collect all pointer args - they may be used
  // in loads/stores we need to lower to svm intrinsics
  // m_args already contatins only args that require processing
  for (auto &Arg : m_args)
    addUsers(Arg, ProcessCalls);
}

void GenXThreadPrivateMemory::addUsersIfNeeded(Value *V, bool ProcessCalls) {
  bool isGatherScatterPrivate = false;
  if (IntrinsicInst *CI = dyn_cast<IntrinsicInst>(V)) {
    unsigned ID = vc::getAnyIntrinsicID(CI);
    switch (ID) {
    case GenXIntrinsic::genx_gather_private:
    case GenXIntrinsic::genx_scatter_private:
    case Intrinsic::lifetime_start:
    case Intrinsic::lifetime_end:
      isGatherScatterPrivate = true;
      break;
    default:
      break;
    }
  }
  if (!isa<LoadInst>(V) && !isa<StoreInst>(V) &&
      V->getType()->getScalarType()->isIntegerTy(1))
    return;
  if (m_useGlobalMem ||
      (!isa<LoadInst>(V) && !isa<StoreInst>(V) && !isGatherScatterPrivate))
    addUsers(V, ProcessCalls);
}

bool GenXThreadPrivateMemory::runOnModule(Module &M) {
  if (!EnableTPM)
    return false;
  m_ST = &getAnalysis<TargetPassConfig>()
              .getTM<GenXTargetMachine>()
              .getGenXSubtarget();
  m_DL = &M.getDataLayout();

  // switching off for OCLRT only
  if (m_ST->isOCLRuntime() && !EnableTPMOCLRT)
    return false;

  if (!m_ST->isOCLRuntime())
    m_useGlobalMem = false;
  CallGraph CG(M);
  for (auto &F : M)
    visit(F);
  if (m_useGlobalMem ||
      (m_ST->isOCLRuntime() && std::find_if(m_alloca.begin(), m_alloca.end(),
                                            SVMChecker()) != m_alloca.end()))
    switchStack(M);
  bool Result = false;

  std::unordered_set<Function *> Visited;
  std::queue<Function *> Worklist;
  std::for_each(M.begin(), M.end(), [&Worklist](Function &F) {
    if (vc::isKernel(&F))
      Worklist.push(&F);
  });

  while (!Worklist.empty()) {
    auto *F = Worklist.front();
    Worklist.pop();
    if (Visited.count(F) > 0)
      continue;
    Visited.insert(F);
    Result |= runOnFunction(*F);
    for (auto &N : *CG[F]) {
      if (N.second->getFunction() &&
          vc::getAnyIntrinsicID(N.second->getFunction()) ==
              GenXIntrinsic::not_any_intrinsic) {
        Worklist.push(N.second->getFunction());
        LLVM_DEBUG(dbgs() << "Adding func "
                          << N.second->getFunction()->getName()
                          << " to worklist\n");
      }
    }
    if (m_Calls[F].Recreate) {
      LLVM_DEBUG(dbgs() << "Recreating func " << F->getName() << ": "
                        << *F->getType() << " -> ");
      // rewrite func arg types
      auto FTy = F->getFunctionType();
      SmallVector<Type *, 4> ArgTys;
      auto *NewArgTy = IntegerType::get(*m_ctx, m_useGlobalMem ? 64 : 32);
      for (unsigned i = 0, e = FTy->getNumParams(); i != e; ++i) {
        if (m_Calls[F].Args.count(IGCLLVM::getArg(*F, i)) > 0)
          ArgTys.push_back(NewArgTy);
        else
          ArgTys.push_back(FTy->getParamType(i));
      }
      FTy = FunctionType::get(FTy->getReturnType(), ArgTys, false);
      // Create the new function.
      auto *NewFunc = Function::Create(FTy, F->getLinkage(), "");
      LLVM_DEBUG(dbgs() << *FTy << "\n");
      NewFunc->takeName(F);
      NewFunc->copyAttributesFrom(F);
      F->getParent()->getFunctionList().insert(F->getIterator(), NewFunc);
      for (auto &NewArg : NewFunc->args()) {
        auto *OldArg = IGCLLVM::getArg(*F, NewArg.getArgNo());
        while (!OldArg->user_empty()) {
          auto *U = OldArg->user_back();
          if (m_Calls[F].Args.count(OldArg) > 0 && isa<CastInst>(U)) {
            // we should process a type-changed arg
            IGC_ASSERT(NewArg.getType() == NewArgTy);
            if (isa<PtrToIntInst>(U)) {
              IGC_ASSERT(U->getType()->getScalarType() == NewArgTy);
              U->replaceAllUsesWith(&NewArg);
              cast<Instruction>(U)->eraseFromParent();
            } else if (auto *BC = dyn_cast<BitCastInst>(U)) {
              auto *NewBC = CastInst::CreateBitOrPointerCast(
                  &NewArg, BC->getType(), "", BC);
              BC->replaceAllUsesWith(NewBC);
              BC->eraseFromParent();
            } else
              IGC_ASSERT_MESSAGE(0, "Unsupported cast of a new func arg");
          } else
            cast<Instruction>(U)->replaceUsesOfWith(OldArg, &NewArg);
        }
        NewArg.takeName(OldArg);
      }
      NewFunc->getBasicBlockList().splice(NewFunc->begin(),
                                          F->getBasicBlockList());
      while (!F->use_empty()) {
        auto *U = F->user_back();
        IGC_ASSERT(checkFunctionCall(U, F));
        auto *OrigCall = cast<CallInst>(U);

        std::vector<Value *> Args;
        std::copy(OrigCall->arg_begin(), OrigCall->arg_end(),
                  std::back_inserter(Args));
        CallInst *NewCall = CallInst::Create(NewFunc, Args, "", OrigCall);
        IGC_ASSERT(nullptr != NewCall);
        NewCall->setCallingConv(OrigCall->getCallingConv());
        // TODO:
        // check if this necessary, as we can't assign old mem-bound
        // attrs to new i64 args
        // NewCall->setAttributes(OrigCall->getAttributes());
        if (OrigCall->isTailCall())
          NewCall->setTailCall();
        NewCall->setDebugLoc(OrigCall->getDebugLoc());
        NewCall->takeName(OrigCall);
        OrigCall->replaceAllUsesWith(NewCall);
        OrigCall->eraseFromParent();
      }
      F->eraseFromParent();
    }
  }
  return Result;
}

bool GenXThreadPrivateMemory::processUsers() {
  bool Changed = false;
  while (!m_AIUsers.empty()) {
    Instruction *I = m_AIUsers.front().first;
    bool ProcessCalls = m_AIUsers.front().second;
    LLVM_DEBUG(dbgs() << "Processing inst: " << *I << "\n");
    m_AIUsers.pop();

    addUsersIfNeeded(I, ProcessCalls);
    bool ChangeRequired = false;
    if (auto *LdI = dyn_cast<LoadInst>(I)) {
      Changed |= replaceLoad(LdI);
      ChangeRequired = true;
    } else if (auto *StI = dyn_cast<StoreInst>(I)) {
      Changed |= replaceStore(StI);
      ChangeRequired = true;
    } else if (auto *PTI = dyn_cast<PtrToIntInst>(I)) {
      Changed |= replacePTI(PTI);
      ChangeRequired = true;
    } else if (auto *AddrCast = dyn_cast<AddrSpaceCastInst>(I)) {
      Changed |= replaceAddrSpaceCast(AddrCast);
      ChangeRequired = true;
    } else if (isa<IntToPtrInst>(I) || isa<BitCastInst>(I)) {
      // resolve all IntToPtr users and remove it.
      if (I->use_empty()) {
        I->eraseFromParent();
        Changed = true;
      }
    } else if (auto *CI = dyn_cast<CallInst>(I)) {
      unsigned ID = vc::getAnyIntrinsicID(CI);
      if (ID == GenXIntrinsic::genx_gather_private) {
        Changed |= replaceGatherPrivate(CI);
        ChangeRequired = true;
      } else if (ID == GenXIntrinsic::genx_scatter_private) {
        Changed |= replaceScatterPrivate(CI);
        ChangeRequired = true;
      } else if (ID == Intrinsic::lifetime_start ||
                 ID == Intrinsic::lifetime_end) {
        CI->eraseFromParent();
        Changed = true;
      } else if (ID == GenXIntrinsic::not_any_intrinsic) {
        if (m_Calls[CI->getCalledFunction()].Calls.count(CI) > 0) {
          for (unsigned i = 0; i < CI->getNumArgOperands(); i++) {
            if (m_Calls[CI->getCalledFunction()].Args.count(
                    IGCLLVM::getArg(*CI->getCalledFunction(), i)) > 0)
              CI->replaceUsesOfWith(
                  CI->getArgOperand(i),
                  lookForPtrReplacement(CI->getArgOperand(i)));
          }
          Changed = true;
        } else {
          processUnchangedCall(*CI);
          Changed = true;
        }
      }
    } else if (PHINode *Phi = dyn_cast<PHINode>(I)) {
      if (isa<PointerType>(Phi->getType())) {
        Changed |= replacePhi(Phi);
        ChangeRequired = true;
      }
    } else if (SelectInst *Sel = dyn_cast<SelectInst>(I)) {
      if (isa<PointerType>(Sel->getType())) {
        Changed |= replaceSelect(Sel);
        ChangeRequired = true;
      }
    }
    if (m_AIUsers.empty()) {
      if (!Changed && ChangeRequired) {
        vc::diagnose(I->getContext(), "TPM", I,
                     "Thread private memory: cannot resolve all alloca uses");
      }
      Changed = false;
      collectEachPossibleTPMUsers();
    }
  }
  return Changed;
}

// TPM tries to rewrite function declaration. It not always can do this.
// This method fixes call site for such functions. Otherwise TPM may delete
// call as PotentiallyDeadInst.
void GenXThreadPrivateMemory::processUnchangedCall(CallInst &CI) {
  auto &&PointerArgs = make_filter_range(CI.args(), [](const Use &U) {
    return U.get()->getType()->isPointerTy();
  });
  for (Use &Arg : PointerArgs) {
    Value *Replacement = lookForPtrReplacement(Arg.get());
    // FIXME: Currently TPM presumes that it can replace all pointers with
    //        integers. Case where pointer call arg has no replace is possible,
    //        but lookForPtrReplacement won't return nullptr in this case it
    //        will report an error.
    if (!Replacement)
      continue;
    Value *BackToPtr =
        new IntToPtrInst{Replacement, Arg.get()->getType(),
                         Replacement->getName() + ".cast.back", &CI};
    Arg = BackToPtr;
  }
}

bool GenXThreadPrivateMemory::runOnFunction(Function &F) {
  // skip function which is not a kernel or stackfunc
  // typically it's an emulation-related func (__cm_intrinsic_impl_*)
  if (vc::getAnyIntrinsicID(&F) != GenXIntrinsic::not_any_intrinsic)
    return false;
  LLVM_DEBUG(dbgs() << "Running TPM on " << F.getName() << "\n");
  m_stack = m_ST->stackSurface();

  m_ctx = &F.getContext();
  m_alloca.clear();
  m_args.clear();
  m_allocaToIntrinsic.clear();
  m_AIUsers = {};
  m_AlreadyAdded.clear();

  visit(F);

  for (auto Alloca : m_alloca) {
    Type *AllocaTy = Alloca->getAllocatedType();

    auto IID = llvm::GenXIntrinsic::genx_alloca;
    Function *IntrDecl = GenXIntrinsic::getGenXDeclaration(
        Alloca->getModule(), IID,
        {IntegerType::get(*m_ctx,
                          (m_useGlobalMem ? genx::QWordBits : genx::DWordBits)),
         AllocaTy});
    CallInst *AllocaIntr =
        IntrinsicInst::Create(IntrDecl, {Constant::getNullValue(AllocaTy)});
    AllocaIntr->insertAfter(Alloca);
    m_allocaToIntrinsic[Alloca] = AllocaIntr;
  }

  // Firstly, we resolve dependencies in PHI nodes (see comments in
  // preparePhiForReplacement).
  LLVM_DEBUG(dbgs() << "Processing phis\n");
  collectEachPossibleTPMUsers();
  bool Changed = false;
  while (!m_AIUsers.empty()) {
    Instruction *I = m_AIUsers.front().first;
    m_AIUsers.pop();

    addUsersIfNeeded(I, false);

    if (PHINode *Phi = dyn_cast<PHINode>(I))
      Changed |= preparePhiForReplacement(Phi);
  }
  LLVM_DEBUG(dbgs() << "Finished processing phis\n");

  // Main loop where instructions are replaced one by one.
  m_AlreadyAdded.clear();
  LLVM_DEBUG(dbgs() << "Processing allocas\n");
  collectAllocaUsers();
  Changed |= processUsers();

  LLVM_DEBUG(dbgs() << "Processing args\n");
  collectArgUsers(!vc::isKernel(&F));
  Changed |= processUsers();

  for (auto Alloca : m_alloca) {
    EraseUsers(Alloca);
    IGC_ASSERT_MESSAGE(Alloca->use_empty(),
                       "uses of replaced alloca aren't empty");
    Alloca->eraseFromParent();
  }

  return !m_allocaToIntrinsic.empty();
}

void GenXThreadPrivateMemory::visitAllocaInst(AllocaInst &I) {
  m_alloca.push_back(&I);
}

void GenXThreadPrivateMemory::visitFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "Visiting func " << F.getName() << "\n");
  if (GenXIntrinsic::isAnyNonTrivialIntrinsic(&F))
    return;
  // here we don't use vc::KernelMetadata as it's only able to
  // deal with kernels while we want to look at functions as well
  NamedMDNode *Named =
      F.getParent()->getNamedMetadata(genx::FunctionMD::GenXKernels);

  if (!Named)
    return;

  auto NodeIt =
      std::find_if(Named->op_begin(), Named->op_end(), [&F](MDNode *N) {
        return N->getNumOperands() > KernelMDOp::ArgTypeDescs &&
               vc::getValueAsMetadata(N->getOperand(KernelMDOp::FunctionRef)) ==
                   &F;
      });
  if (NodeIt == Named->op_end()) {
    // not a kernel
    // check if all call uses of F were met in allocas du chains
    if (!m_Calls[&F].Calls.empty()) {
      if (std::all_of(F.user_begin(), F.user_end(), [&F, this](User *U) {
            return checkFunctionCall(U, &F) && m_Calls[&F].Calls.count(U) > 0;
          })) {
        m_args = m_Calls[&F].Args;
        LLVM_DEBUG(dbgs() << "Confirmed " << m_args.size() << " args of "
                          << F.getName() << "\n");
        m_Calls[&F].Recreate = true;
      } else {
        vc::diagnose(F.getContext(), "TPM",
                     F.getName() +
                         " args are used in TPM, but rewriting impossible",
                     DS_Warning);
      }
    }
    return;
  }
  auto *Node = *NodeIt;
  if (Node->getNumOperands() <= KernelMDOp::ArgTypeDescs)
    return;

  MDNode *ArgDescNode =
      cast<MDNode>(Node->getOperand(KernelMDOp::ArgTypeDescs));

  for (auto &Arg : F.args()) {
    if (ArgDescNode->getNumOperands() <= Arg.getArgNo())
      continue;
    StringRef SvmMD =
        cast<MDString>(ArgDescNode->getOperand(Arg.getArgNo()))->getString();
    if (!IGCLLVM::contains_insensitive(SvmMD, "svmptr_t"))
      continue;
    if (!m_useGlobalMem)
      switchStack(*F.getParent());
    LLVM_DEBUG(dbgs() << "Adding svm arg " << Arg << " of " << F.getName()
                      << "\n");
    m_args.insert(&Arg);
  }
}
