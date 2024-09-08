/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXLoadStoreLowering
/// ---------------------------
///
/// The pass:
/// * replaces all LLVM loads and stores, using correct namespace,
/// * replaces all @llvm.masked.gather and @llvm.masked.scatter intrinsics,
/// * replaces all @llvm.vc.internal.atomic intrinsics,
/// * replaces all atomic instructions,
/// * replaces all fence instructions,
/// * removes lifetime builtins as we are not sure how to process those.
///
//===----------------------------------------------------------------------===//

#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "GenXVisa.h"

#include "BiFModule/Headers/spirv_atomics_common.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
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
  A32, // Global memory, addressed with 32-bit pointers.
  A64, // Global memory, addressed with 64-bit pointers.
  SLM, // Shared local memory.
};

enum SPIRVAtomicOp {
  PointerOp = 0,
  ScopeOp = 1,
  SemanticsOp = 2,
  SourceOp = 3
};

constexpr const char AlignMDName[] = "VCAlignment";

// load and store lowering pass
class GenXLoadStoreLowering : public FunctionPass,
                              public InstVisitor<GenXLoadStoreLowering> {
  const DataLayout *DL_ = nullptr;
  const GenXSubtarget *ST = nullptr;
  SmallVector<StringRef, 8> SyncScopeNames;

private:
  using CacheControlsVector = SmallVector<LSC_CACHE_OPT, 4>;
  using CacheControlsRef = ArrayRef<LSC_CACHE_OPT>;

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
  Instruction *switchAddrSpace(MemoryInstT &I, PointerType *PtrTy) const;
  template <MessageKind MK, Atomicity A, typename MemoryInstT>
  Instruction *switchAddrSpace(MemoryInstT &I) const;
  template <MessageKind MK, Atomicity A>
  Instruction *switchAddrSpace(IntrinsicInst &I) const;
  template <MessageKind MK, Atomicity A>
  Instruction *switchAddrSpace(FenceInst &I) const;

  // Creates a replacement for \p I instruction. The template parameters
  // describe the provided instruction and how it should be lowered.
  template <HWAddrSpace HWAS, MessageKind MK, Atomicity A, typename MemoryInstT>
  Instruction *createIntrinsic(MemoryInstT &I) const;
  template <MessageKind MK> Instruction *createIntrinsic(FenceInst &FI) const;

  Instruction *createLegacyLoadStore(Instruction &I, unsigned BTI, Value *Ptr,
                                     Value *Data = nullptr) const;
  Instruction *createLegacyGatherScatter(IntrinsicInst &I, unsigned BTI) const;
  Value *createLegacyGatherScatterQWordImpl(IRBuilder<> &Builder, Module *M,
                                            unsigned BTI, bool IsLoad,
                                            Value *Pred, Value *Addr,
                                            Value *Source,
                                            ConstantInt *Align) const;
  Instruction *createLegacyBlockLoadImpl(IRBuilder<> &Builder, Module *M,
                                         GenXIntrinsic::ID IID, unsigned BTI,
                                         IGCLLVM::FixedVectorType *Ty,
                                         Value *Addr) const;
  Instruction *createLegacyBlockStoreImpl(IRBuilder<> &Builder, Module *M,
                                          unsigned BTI, Value *Addr,
                                          Value *Data) const;
  Instruction *createLegacyGatherLoadImpl(
      IRBuilder<> &Builder, Module *M, unsigned BTI, unsigned ESize,
      IGCLLVM::FixedVectorType *Ty, Value *Pred, Value *Base, Value *Offset,
      Value *Source = nullptr, ConstantInt *Align = nullptr) const;
  Instruction *createLegacyScatterStoreImpl(IRBuilder<> &Builder, Module *M,
                                            unsigned BTI, unsigned ESize,
                                            Value *Pred, Value *Base,
                                            Value *Offset, Value *Data,
                                            ConstantInt *Align = nullptr) const;

  Instruction *createLSCLoadStore(Instruction &I, vc::InternalIntrinsic::ID IID,
                                  Value *Base, Value *Addr,
                                  Value *Data = nullptr) const;
  Instruction *createLSCGatherScatter(IntrinsicInst &I,
                                      vc::InternalIntrinsic::ID LoadIID,
                                      vc::InternalIntrinsic::ID StoreIID,
                                      Value *Base, Type *AddrTy) const;
  Instruction *createLSCLoadImpl(IRBuilder<> &Builder, Module *M,
                                 vc::InternalIntrinsic::ID IID, unsigned ESize,
                                 Value *Pred, Value *Base, Value *Addr,
                                 Value *Source, Constant *CacheOpts,
                                 ConstantInt *Align = nullptr) const;
  Instruction *createLSCStoreImpl(IRBuilder<> &Builder, Module *M,
                                  vc::InternalIntrinsic::ID IID, unsigned ESize,
                                  Value *Pred, Value *Base, Value *Addr,
                                  Value *Data, Constant *CacheOpts,
                                  ConstantInt *Align = nullptr) const;

  Instruction *createLSCAtomicLoad(LoadInst &I, vc::InternalIntrinsic::ID IID,
                                   Type *AddrTy, Value *BTI) const;
  Instruction *createLSCAtomicStore(StoreInst &I, vc::InternalIntrinsic::ID IID,
                                    Type *AddrTy, Value *BTI) const;
  Instruction *createLSCAtomicRMW(AtomicRMWInst &I,
                                  vc::InternalIntrinsic::ID IID, Type *AddrTy,
                                  Value *BTI) const;
  Instruction *createLSCAtomicRMW(IntrinsicInst &I,
                                  vc::InternalIntrinsic::ID IID, Type *AddrTy,
                                  Value *BTI) const;
  Instruction *createLSCAtomicCmpXchg(AtomicCmpXchgInst &I,
                                      vc::InternalIntrinsic::ID IID,
                                      Type *AddrTy, Value *BTI) const;

  Instruction *createLSCAtomicImpl(Instruction &I,
                                   vc::InternalIntrinsic::ID IID,
                                   LSC_OP AtomicOp, Value *BTI, Value *Addr,
                                   Value *Src0, Value *Src1) const;
  Instruction *createLSCStandAloneFence(FenceInst &I) const;
  void createLSCAtomicFenceImpl(Instruction &AtomicI, IRBuilder<> &Builder,
                                bool IsPostFence) const;

  Instruction *createLegacyAtomicLoad(LoadInst &I, unsigned BTI) const;
  Instruction *createLegacyAtomicStore(StoreInst &I, unsigned BTI) const;
  Instruction *createLegacyAtomicRMW(AtomicRMWInst &I, unsigned BTI) const;
  Instruction *createLegacyAtomicCmpXchg(AtomicCmpXchgInst &I,
                                         unsigned BTI) const;
  Instruction *createLegacyAtomicImpl(Instruction &I, GenXIntrinsic::ID IID,
                                      Value *BTI, Value *Addr, Value *Src0,
                                      Value *Src1) const;
  Instruction *createLegacyStandAloneFence(FenceInst &I) const;
  void createLegacyAtomicFenceImpl(Instruction &I, IRBuilder<> &Builder,
                                   bool IsPostFence) const;

  std::pair<unsigned, AtomicOrdering>
  getAddressSpaceAndOrderingOfAtomic(const Instruction &AtomicI) const;

  Value *createExtractDataFromVectorImpl(IRBuilder<> &Builder, Module *M,
                                         IGCLLVM::FixedVectorType *Ty,
                                         Value *Data, unsigned Offset) const;
  Value *createInsertDataIntoVectorImpl(IRBuilder<> &Builder, Module *M,
                                        Value *Target, Value *Data,
                                        unsigned Offset) const;
  Value *createExtendImpl(IRBuilder<> &Builder, Value *Data) const;
  Value *createTruncateImpl(IRBuilder<> &Builder, IGCLLVM::FixedVectorType *Ty,
                            Value *Data) const;

  Type *lowerBoolType(IRBuilder<> &Builder, Type *Ty) const;
  Value *lowerBoolOperand(IRBuilder<> &Builder, Module *M, Value *Data) const;
  Value *extractBoolValue(IRBuilder<> &Builder, Module *M, Value *Data,
                          Type *Ty) const;

  CacheControlsVector getLSCCacheOptsImpl(Instruction *I,
                                          bool IsSLM = false) const;
  Constant *getLSCCacheOpts(Instruction *I, bool IsSLM = false) const;
  LSC_SCOPE getLSCFenceScope(Instruction *I) const;
  static unsigned getLSCBlockElementSizeBits(unsigned DataSizeBytes,
                                             unsigned Align);
  static LSC_DATA_SIZE getLSCElementSize(unsigned Bits);
  static LSC_DATA_ELEMS getLSCElementsPerAddress(unsigned N);

  static Value *makeVector(IRBuilder<> &Builder, Value *Val);

public:
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &Inst) const;
  void visitAtomicRMWInst(AtomicRMWInst &Inst) const;
  void visitIntrinsicInst(IntrinsicInst &Intrinsic) const;
  void visitLoadInst(LoadInst &LdI) const;
  void visitStoreInst(StoreInst &StI) const;
  void visitCallInst(CallInst &CI) const;
  void visitFenceInst(FenceInst &FI) const;

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

static bool isSPIRVAtomic(const Value *Val) {
  unsigned IID = vc::getAnyIntrinsicID(Val);
  switch (IID) {
  default:
    return false;
  case vc::InternalIntrinsic::atomic_fmin:
  case vc::InternalIntrinsic::atomic_fmax:
    return true;
  }
}

static LSC_FENCE_OP getLSCFenceOp(AtomicOrdering Ordering) {
  switch (Ordering) {
  default:
    return LSC_FENCE_OP_NONE;
  case AtomicOrdering::Acquire:
    return LSC_FENCE_OP_INVALIDATE;
  case AtomicOrdering::Release:
    return LSC_FENCE_OP_CLEAN;
  case AtomicOrdering::AcquireRelease:
  case AtomicOrdering::SequentiallyConsistent:
    return LSC_FENCE_OP_EVICT;
  }
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
  M.getContext().getSyncScopeNames(SyncScopeNames);
  IGC_ASSERT(ST);
  // auto &BEConf = getAnalysis<GenXBackendConfig>();
  // BEConf.getStatelessPrivateMemSize() will be required

  // see visitXXInst members for main logic:
  //   * visitAtomicCmpXchgInst
  //   * visitAtomicRMWInst
  //   * visitIntrinsicInst
  //   * visitLoadInst
  //   * visitStoreInst
  //   * visitCallInst
  //   * visitFenceInst
  visit(F);

  return true;
}

void GenXLoadStoreLowering::visitAtomicCmpXchgInst(
    AtomicCmpXchgInst &Inst) const {
  LLVM_DEBUG(dbgs() << "Replacing cmpxchg inst " << Inst << " ===>\n");
  auto *Replacement = createMemoryInstReplacement(Inst);
  Inst.replaceAllUsesWith(Replacement);
  Inst.eraseFromParent();
}

void GenXLoadStoreLowering::visitAtomicRMWInst(AtomicRMWInst &Inst) const {
  LLVM_DEBUG(dbgs() << "Replacing binary atomic inst " << Inst << " ===>\n");
  auto *Replacement = createMemoryInstReplacement(Inst);
  Inst.replaceAllUsesWith(Replacement);
  Inst.eraseFromParent();
}

void GenXLoadStoreLowering::visitIntrinsicInst(IntrinsicInst &Inst) const {
  unsigned ID = vc::getAnyIntrinsicID(&Inst);
  switch (ID) {
  case Intrinsic::masked_gather:
  case Intrinsic::masked_scatter: {
    LLVM_DEBUG(dbgs() << "Replacing intrinsic " << Inst << " ===>\n");
    auto *Replacement = createMemoryInstReplacement(Inst);
    Inst.replaceAllUsesWith(Replacement);
  }
    LLVM_FALLTHROUGH;
  case Intrinsic::lifetime_start:
  case Intrinsic::lifetime_end:
    Inst.eraseFromParent();
    break;
  default:
    break;
  }
}

void GenXLoadStoreLowering::visitLoadInst(LoadInst &LdI) const {
  LLVM_DEBUG(dbgs() << "Replacing load " << LdI << " ===>\n");
  auto *Replacement = createMemoryInstReplacement(LdI);
  LLVM_DEBUG(dbgs() << "Proper gather to replace uses: " << *Replacement
                    << "\n");
  LdI.replaceAllUsesWith(Replacement);
  LdI.eraseFromParent();
}

void GenXLoadStoreLowering::visitStoreInst(StoreInst &StI) const {
  LLVM_DEBUG(dbgs() << "Replacing store " << StI << " ===>\n");
  auto *Replacement = createMemoryInstReplacement(StI);
  LLVM_DEBUG(dbgs() << *Replacement << "\n");
  StI.eraseFromParent();
}

void GenXLoadStoreLowering::visitCallInst(CallInst &CI) const {
  if (!isSPIRVAtomic(&CI))
    return;
  LLVM_DEBUG(dbgs() << "Replacing intrinsic " << CI << " ===>\n");
  auto *Replacement = createMemoryInstReplacement(cast<IntrinsicInst>(CI));
  CI.replaceAllUsesWith(Replacement);
  CI.eraseFromParent();
}

void GenXLoadStoreLowering::visitFenceInst(FenceInst &FI) const {
  LLVM_DEBUG(dbgs() << "Replacing fence " << FI << " ===>\n");
  auto *Replacement = createMemoryInstReplacement(FI);
  if (Replacement)
    LLVM_DEBUG(dbgs() << *Replacement << "\n");
  FI.eraseFromParent();
}

namespace {
struct GatherScatterOperands {
  bool IsLoad;
  Value *Mask;
  Value *Addr;
  Value *Data;
  ConstantInt *Align;
};

GatherScatterOperands getGatherScatterOperands(IntrinsicInst &I) {
  unsigned ID = vc::getAnyIntrinsicID(&I);
  switch (ID) {
  default:
    IGC_ASSERT_MESSAGE(0, "unsupported intrinsic");
    return {false, nullptr, nullptr, nullptr, nullptr};
  case Intrinsic::masked_gather:
    return {true, I.getArgOperand(2), I.getArgOperand(0), I.getArgOperand(3),
            cast<ConstantInt>(I.getArgOperand(1))};
  case Intrinsic::masked_scatter:
    return {false, I.getArgOperand(3), I.getArgOperand(1), I.getArgOperand(0),
            cast<ConstantInt>(I.getArgOperand(2))};
  }
}
} // namespace

unsigned
GenXLoadStoreLowering::getLSCBlockElementSizeBits(unsigned DataSizeBytes,
                                                  unsigned Align) {
  bool IsDWordProfitable = (DataSizeBytes % DWordBytes == 0) &&
                           (DataSizeBytes % QWordBytes != 0) &&
                           (DataSizeBytes <= 64 * DWordBytes);
  if (!IsDWordProfitable && Align >= QWordBytes && DataSizeBytes >= QWordBytes)
    return QWordBits;
  if (Align >= DWordBytes && DataSizeBytes >= DWordBytes)
    return DWordBits;
  return 0;
}

LSC_DATA_SIZE GenXLoadStoreLowering::getLSCElementSize(unsigned Bits) {
  switch (Bits) {
  case QWordBits:
    return LSC_DATA_SIZE_64b;
  case DWordBits:
    return LSC_DATA_SIZE_32b;
  case WordBits:
    return LSC_DATA_SIZE_16c32b;
  case ByteBits:
    return LSC_DATA_SIZE_8c32b;
  default:
    IGC_ASSERT_UNREACHABLE();
    break;
  }
  return LSC_DATA_SIZE_INVALID;
}

LSC_DATA_ELEMS GenXLoadStoreLowering::getLSCElementsPerAddress(unsigned N) {
  switch (N) {
  case 1:
    return LSC_DATA_ELEMS_1;
  case 2:
    return LSC_DATA_ELEMS_2;
  case 3:
    return LSC_DATA_ELEMS_3;
  case 4:
    return LSC_DATA_ELEMS_4;
  case 8:
    return LSC_DATA_ELEMS_8;
  case 16:
    return LSC_DATA_ELEMS_16;
  case 32:
    return LSC_DATA_ELEMS_32;
  case 64:
    return LSC_DATA_ELEMS_64;
  default:
    IGC_ASSERT_UNREACHABLE();
    break;
  }
  return LSC_DATA_ELEMS_INVALID;
}

Value *GenXLoadStoreLowering::createExtractDataFromVectorImpl(
    IRBuilder<> &Builder, Module *M, IGCLLVM::FixedVectorType *Ty, Value *Data,
    unsigned Offset) const {
  auto *DataVTy = cast<IGCLLVM::FixedVectorType>(Data->getType());

  if (Ty == DataVTy)
    return Data;

  auto *DataETy = DataVTy->getElementType();
  auto *TargetETy = Ty->getElementType();

  auto *ExtractVTy = Ty;
  if (DataETy != TargetETy) {
    unsigned TargetNElements = Ty->getNumElements();
    unsigned TargetESize = DL_->getTypeSizeInBits(TargetETy);
    unsigned ESize = DL_->getTypeSizeInBits(DataETy);
    auto ExtractNElements = TargetNElements * TargetESize / ESize;
    ExtractVTy = IGCLLVM::FixedVectorType::get(DataETy, ExtractNElements);
  }

  if (ExtractVTy == DataVTy)
    return Builder.CreateBitCast(Data, Ty);

  auto IID = TargetETy->isFloatingPointTy() ? GenXIntrinsic::genx_rdregionf
                                            : GenXIntrinsic::genx_rdregioni;
  auto *Func = GenXIntrinsic::getGenXDeclaration(
      M, IID, {ExtractVTy, DataVTy, Builder.getInt16Ty()});

  SmallVector<Value *, 6> Args = {
      Data,                // Vector to read region from
      Builder.getInt32(1), // vstride
      Builder.getInt32(1), // width
      Builder.getInt32(0), // stride
      Builder.getInt16(Offset),
      Builder.getInt32(0), // parent width, ignored
  };

  auto *Extract = Builder.CreateCall(Func, Args);
  return Builder.CreateBitCast(Extract, Ty);
}

Value *GenXLoadStoreLowering::createInsertDataIntoVectorImpl(
    IRBuilder<> &Builder, Module *M, Value *Target, Value *Data,
    unsigned Offset) const {
  auto *TargetVTy = cast<IGCLLVM::FixedVectorType>(Target->getType());
  auto *TargetETy = TargetVTy->getElementType();

  auto *DataVTy = cast<IGCLLVM::FixedVectorType>(Data->getType());

  if (TargetVTy == DataVTy)
    return Data;

  auto NElements =
      DL_->getTypeSizeInBits(DataVTy) / DL_->getTypeSizeInBits(TargetETy);
  auto *InsertVTy = IGCLLVM::FixedVectorType::get(TargetETy, NElements);
  auto *Cast = Builder.CreateBitCast(Data, InsertVTy);

  if (InsertVTy == TargetVTy)
    return Cast;

  auto IID = TargetETy->isFloatingPointTy() ? GenXIntrinsic::genx_wrregionf
                                            : GenXIntrinsic::genx_wrregioni;
  auto *Func = GenXIntrinsic::getGenXDeclaration(
      M, IID,
      {TargetVTy, InsertVTy, Builder.getInt16Ty(), Builder.getInt1Ty()});

  SmallVector<Value *, 8> Args = {
      Target,              // vector to write region to
      Cast,                // data to write
      Builder.getInt32(1), // vstride
      Builder.getInt32(1), // width
      Builder.getInt32(0), // stride
      Builder.getInt16(Offset),
      Builder.getInt32(0), // parent width, ignored
      Builder.getTrue(),
  };

  auto *Insert = Builder.CreateCall(Func, Args);
  return Insert;
}

Value *GenXLoadStoreLowering::createExtendImpl(IRBuilder<> &Builder,
                                               Value *Data) const {
  auto *VTy = cast<IGCLLVM::FixedVectorType>(Data->getType());
  auto *ETy = VTy->getElementType();
  auto ESize = DL_->getTypeSizeInBits(ETy);
  auto NElements = VTy->getNumElements();

  if (ESize >= DWordBits)
    return Data;

  auto *CastVTy =
      IGCLLVM::FixedVectorType::get(Builder.getIntNTy(ESize), NElements);
  auto *Cast = Builder.CreateBitCast(Data, CastVTy);

  auto *ExtVTy =
      IGCLLVM::FixedVectorType::get(Builder.getIntNTy(DWordBits), NElements);
  return Builder.CreateZExt(Cast, ExtVTy);
}

Value *GenXLoadStoreLowering::createTruncateImpl(IRBuilder<> &Builder,
                                                 IGCLLVM::FixedVectorType *Ty,
                                                 Value *Data) const {
  auto *DataVTy = cast<IGCLLVM::FixedVectorType>(Data->getType());
  auto *DataETy = DataVTy->getElementType();

  if (DataVTy == Ty)
    return Data;

  IGC_ASSERT(DataETy->isIntegerTy());
  IGC_ASSERT(Ty->getNumElements() == DataVTy->getNumElements());

  auto *ETy = Ty->getElementType();
  auto *TruncETy = Builder.getIntNTy(DL_->getTypeSizeInBits(ETy));
  auto *TruncVTy =
      IGCLLVM::FixedVectorType::get(TruncETy, Ty->getNumElements());

  auto *Trunc = Builder.CreateTrunc(Data, TruncVTy);
  return Builder.CreateBitCast(Trunc, Ty);
}

Type *GenXLoadStoreLowering::lowerBoolType(IRBuilder<> &Builder,
                                           Type *Ty) const {
  if (Ty->isIntegerTy(1))
    return Builder.getInt8Ty();

  auto TargetBits = DL_->getTypeStoreSizeInBits(Ty);

  unsigned TargetElementBits = 0;
  if (TargetBits % QWordBits == 0)
    TargetElementBits = QWordBits;
  else if (TargetBits % DWordBits == 0)
    TargetElementBits = DWordBits;
  else if (TargetBits % WordBits == 0)
    TargetElementBits = WordBits;
  else
    TargetElementBits = ByteBits;

  auto *TargetETy = Builder.getIntNTy(TargetElementBits);
  return IGCLLVM::FixedVectorType::get(TargetETy,
                                       TargetBits / TargetElementBits);
}

Value *GenXLoadStoreLowering::lowerBoolOperand(IRBuilder<> &Builder, Module *M,
                                               Value *Data) const {
  auto *Ty = Data->getType();
  if (Ty->isIntegerTy(1))
    return Builder.CreateZExt(Data, Builder.getInt8Ty());

  auto *VData = makeVector(Builder, Data);

  auto *VTy = cast<IGCLLVM::FixedVectorType>(VData->getType());
  auto *ETy = VTy->getElementType();
  IGC_ASSERT(ETy->isIntegerTy(1));

  unsigned DataBits = VTy->getNumElements();
  unsigned TargetBits = DL_->getTypeStoreSizeInBits(VTy);

  if (TargetBits != DataBits) {
    auto *TargetBoolVTy = IGCLLVM::FixedVectorType::get(ETy, TargetBits);
    auto *VZero = Constant::getNullValue(TargetBoolVTy);
    auto *Func = GenXIntrinsic::getAnyDeclaration(
        M, GenXIntrinsic::genx_wrpredregion, {TargetBoolVTy, VTy});
    VData = Builder.CreateCall(Func, {VZero, VData, Builder.getInt32(0)});
  }

  return Builder.CreateBitCast(VData, lowerBoolType(Builder, VTy));
}

Value *GenXLoadStoreLowering::extractBoolValue(IRBuilder<> &Builder, Module *M,
                                               Value *Data, Type *Ty) const {
  if (Ty->isIntegerTy(1))
    return Builder.CreateTrunc(Data, Ty);

  auto Bits = DL_->getTypeStoreSizeInBits(Ty);
  auto *SrcBoolVTy = IGCLLVM::FixedVectorType::get(Builder.getInt1Ty(), Bits);
  auto *VTy = &vc::getVectorType(*Ty);

  if (VTy != SrcBoolVTy) {
    auto *Func = GenXIntrinsic::getAnyDeclaration(
        M, GenXIntrinsic::genx_rdpredregion, {VTy, SrcBoolVTy});
    auto *Cast = Builder.CreateBitCast(Data, SrcBoolVTy);
    Data = Builder.CreateCall(Func, {Cast, Builder.getInt32(0)});
  }

  return Builder.CreateBitCast(Data, Ty);
}

Value *GenXLoadStoreLowering::makeVector(IRBuilder<> &Builder, Value *Val) {
  auto *Ty = Val->getType();
  if (isa<IGCLLVM::FixedVectorType>(Ty))
    return Val;

  auto *VTy = IGCLLVM::FixedVectorType::get(Ty, 1);
  return Builder.CreateBitCast(Val, VTy);
}

Instruction *GenXLoadStoreLowering::createLSCLoadImpl(
    IRBuilder<> &Builder, Module *M, vc::InternalIntrinsic::ID IID,
    unsigned ESize, Value *Pred, Value *Base, Value *Addr, Value *Source,
    Constant *CacheOpts, ConstantInt *Align) const {
  IGC_ASSERT_EXIT(IID == vc::InternalIntrinsic::lsc_load_bti ||
                  IID == vc::InternalIntrinsic::lsc_load_slm ||
                  IID == vc::InternalIntrinsic::lsc_load_ugm);
  IGC_ASSERT_EXIT(Source);
  IGC_ASSERT_EXIT(Pred);
  IGC_ASSERT_EXIT(Base);
  IGC_ASSERT_EXIT(Addr);
  IGC_ASSERT_EXIT(CacheOpts);

  auto *Ty = cast<IGCLLVM::FixedVectorType>(Source->getType());
  auto *AddrTy = Addr->getType();
  auto IsBlock = !isa<IGCLLVM::FixedVectorType>(AddrTy);
  auto NElements = Ty->getNumElements();

  if (IsBlock)
    IGC_ASSERT_EXIT(ESize == QWordBits || ESize == DWordBits);

  auto ElementSize = getLSCElementSize(ESize);
  auto ElementsPerAddress = getLSCElementsPerAddress(IsBlock ? NElements : 1);
  auto AddrSize = IID == vc::InternalIntrinsic::lsc_load_ugm
                      ? LSC_ADDR_SIZE_64b
                      : LSC_ADDR_SIZE_32b;

  SmallVector<Value *, 10> Args = {
      Pred,
      Builder.getInt8(AddrSize),
      Builder.getInt8(ElementSize),
      Builder.getInt8(ElementsPerAddress),
      CacheOpts,
      Base,
      Addr,
      Builder.getInt16(1), // Address scale
      Builder.getInt32(0), // Address offset
      Source,
  };

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(
      M, IID, {Ty, Pred->getType(), CacheOpts->getType(), Addr->getType()});
  auto *Load = Builder.CreateCall(Func, Args);

  if (Align) {
    auto &Ctx = Load->getContext();
    auto *MD = ConstantAsMetadata::get(Align);
    Load->setMetadata(AlignMDName, MDNode::get(Ctx, MD));
  }

  LLVM_DEBUG(dbgs() << "Created: " << *Load << "\n");
  return Load;
}

Instruction *GenXLoadStoreLowering::createLSCStoreImpl(
    IRBuilder<> &Builder, Module *M, vc::InternalIntrinsic::ID IID,
    unsigned ESize, Value *Pred, Value *Base, Value *Addr, Value *Data,
    Constant *CacheOpts, ConstantInt *Align) const {
  IGC_ASSERT_EXIT(IID == vc::InternalIntrinsic::lsc_store_bti ||
                  IID == vc::InternalIntrinsic::lsc_store_slm ||
                  IID == vc::InternalIntrinsic::lsc_store_ugm);
  IGC_ASSERT_EXIT(Pred);
  IGC_ASSERT_EXIT(Base);
  IGC_ASSERT_EXIT(Addr);
  IGC_ASSERT_EXIT(Data);
  IGC_ASSERT_EXIT(CacheOpts);

  auto *Ty = cast<IGCLLVM::FixedVectorType>(Data->getType());
  auto *AddrTy = Addr->getType();
  auto IsBlock = !isa<IGCLLVM::FixedVectorType>(AddrTy);
  auto NElements = Ty->getNumElements();

  if (IsBlock)
    IGC_ASSERT_EXIT(ESize == QWordBits || ESize == DWordBits);

  auto ElementSize = getLSCElementSize(ESize);
  auto ElementsPerAddress = getLSCElementsPerAddress(IsBlock ? NElements : 1);
  auto AddrSize = IID == vc::InternalIntrinsic::lsc_store_ugm
                      ? LSC_ADDR_SIZE_64b
                      : LSC_ADDR_SIZE_32b;

  SmallVector<Value *, 10> Args = {
      Pred,
      Builder.getInt8(AddrSize),
      Builder.getInt8(ElementSize),
      Builder.getInt8(ElementsPerAddress),
      CacheOpts,
      Base,
      Addr,
      Builder.getInt16(1), // Address scale
      Builder.getInt32(0), // Address offset
      Data,
  };

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(
      M, IID, {Pred->getType(), CacheOpts->getType(), Addr->getType(), Ty});
  auto *Store = Builder.CreateCall(Func, Args);

  if (Align) {
    auto &Ctx = Store->getContext();
    auto *MD = ConstantAsMetadata::get(Align);
    Store->setMetadata(AlignMDName, MDNode::get(Ctx, MD));
  }

  LLVM_DEBUG(dbgs() << "Created: " << *Store << "\n");
  return Store;
}

Instruction *GenXLoadStoreLowering::createLSCLoadStore(
    Instruction &I, vc::InternalIntrinsic::ID IID, Value *Base, Value *Addr,
    Value *Data) const {
  LLVM_DEBUG(dbgs() << "Lowering: " << I << "\n");
  IGC_ASSERT_EXIT(Base);
  IGC_ASSERT_EXIT(Addr);
  IGC_ASSERT_EXIT(isa<LoadInst>(I) || (isa<StoreInst>(I) && Data));

  IRBuilder<> Builder(&I);
  Module *M = I.getModule();
  bool IsLoad = isa<LoadInst>(I);

  auto *Ty = IsLoad ? I.getType() : Data->getType();
  auto *OrigTy = Ty;

  if (OrigTy->getScalarType()->isIntegerTy(1)) {
    if (IsLoad) {
      Ty = lowerBoolType(Builder, Ty);
    } else {
      Data = lowerBoolOperand(Builder, M, Data);
      Ty = Data->getType();
    }
  }

  auto *VTy = &vc::getVectorType(*Ty);

  if (Ty->isPtrOrPtrVectorTy()) {
    auto *IntPtrTy = DL_->getIntPtrType(Ty);
    VTy = &vc::getVectorType(*IntPtrTy);

    if (!IsLoad)
      Data = Builder.CreatePtrToInt(Data, IntPtrTy);
  }

  auto *ETy = VTy->getElementType();

  auto VSize = DL_->getTypeSizeInBits(VTy) / ByteBits;
  auto ESize = DL_->getTypeSizeInBits(ETy) / ByteBits;
  unsigned Rest = VSize;

  if (!IsLoad)
    Data = Builder.CreateBitCast(Data, VTy);

  Value *Result = UndefValue::get(VTy);

  auto Align = IsLoad ? IGCLLVM::getAlignmentValue(&cast<LoadInst>(I))
                      : IGCLLVM::getAlignmentValue(&cast<StoreInst>(I));
  if (Align == 0)
    Align = DL_->getPrefTypeAlignment(Ty);

  bool IsSLM = IID == vc::InternalIntrinsic::lsc_load_slm ||
               IID == vc::InternalIntrinsic::lsc_store_slm;
  auto *CacheOpts = getLSCCacheOpts(&I, IsSLM);

  // Try to generate block messages
  auto BlockESizeBits = getLSCBlockElementSizeBits(VSize, Align);
  if (BlockESizeBits != 0) {
    auto *BlockETy = ESize == BlockESizeBits / ByteBits
                         ? ETy
                         : Builder.getIntNTy(BlockESizeBits);
    auto *Pred = IGCLLVM::ConstantFixedVector::getSplat(1, Builder.getTrue());
    for (auto BlockNElements : {64, 32, 16, 8, 4, 3, 2, 1}) {
      constexpr unsigned MaxRegsPerMessage = 8;
      const auto BlockSize = BlockNElements * BlockESizeBits / ByteBits;
      if (BlockSize > ST->getGRFByteSize() * MaxRegsPerMessage ||
          BlockSize > Rest)
        continue;

      auto *BlockVTy = IGCLLVM::FixedVectorType::get(BlockETy, BlockNElements);

      for (; Rest >= BlockSize; Rest -= BlockSize) {
        auto Offset = VSize - Rest;
        auto *BlockAddr = Addr;
        if (Offset != 0)
          BlockAddr = Builder.CreateAdd(
              Addr, ConstantInt::get(Addr->getType(), Offset));

        if (IsLoad) {
          auto *Passthru = UndefValue::get(BlockVTy);
          auto *Load = createLSCLoadImpl(Builder, M, IID, BlockESizeBits, Pred,
                                         Base, BlockAddr, Passthru, CacheOpts);
          Result =
              createInsertDataIntoVectorImpl(Builder, M, Result, Load, Offset);
        } else {
          auto *Block = createExtractDataFromVectorImpl(Builder, M, BlockVTy,
                                                        Data, Offset);
          Result = createLSCStoreImpl(Builder, M, IID, BlockESizeBits, Pred,
                                      Base, BlockAddr, Block, CacheOpts);
        }
      }
    }
  }

  // Generate a gather/scatter message
  if (Rest != 0) {
    IGC_ASSERT(Rest % ESize == 0);
    auto RestNElements = Rest / ESize;
    auto Offset = VSize - Rest;

    SmallVector<Constant *, 16> Offsets;
    Offsets.reserve(RestNElements);
    std::generate_n(std::back_inserter(Offsets), RestNElements,
                    [AddrTy = Addr->getType(), Offset, ESize]() mutable {
                      auto *C = ConstantInt::get(AddrTy, Offset);
                      Offset += ESize;
                      return C;
                    });
    auto *COffsets = ConstantVector::get(Offsets);
    auto *VAddr = Builder.CreateVectorSplat(RestNElements, Addr);
    if (Offset != 0 || RestNElements > 1)
      VAddr = Builder.CreateAdd(VAddr, COffsets);

    auto *RestVTy = IGCLLVM::FixedVectorType::get(ETy, RestNElements);
    auto *Pred = IGCLLVM::ConstantFixedVector::getSplat(RestNElements,
                                                        Builder.getTrue());

    if (IsLoad) {
      auto *GatherVTy = ESize >= DWordBytes
                            ? RestVTy
                            : IGCLLVM::FixedVectorType::get(
                                  Builder.getIntNTy(DWordBits), RestNElements);
      auto *Passthru = UndefValue::get(GatherVTy);
      auto *Load = createLSCLoadImpl(Builder, M, IID, ESize * ByteBits, Pred,
                                     Base, VAddr, Passthru, CacheOpts);
      auto *Trunc = createTruncateImpl(Builder, RestVTy, Load);
      Result =
          createInsertDataIntoVectorImpl(Builder, M, Result, Trunc, Offset);
    } else {
      auto *Source =
          createExtractDataFromVectorImpl(Builder, M, RestVTy, Data, Offset);
      auto *Extend = createExtendImpl(Builder, Source);
      Result = createLSCStoreImpl(Builder, M, IID, ESize * ByteBits, Pred, Base,
                                  VAddr, Extend, CacheOpts);
    }
  }

  if (IsLoad) {
    if (Ty->isPtrOrPtrVectorTy())
      Result = Builder.CreateIntToPtr(Result, &vc::getVectorType(*Ty));
    Result = Builder.CreateBitCast(Result, Ty);

    if (OrigTy->getScalarType()->isIntegerTy(1))
      Result = extractBoolValue(Builder, M, Result, OrigTy);
  }

  return cast<Instruction>(Result);
}

Instruction *GenXLoadStoreLowering::createLSCGatherScatter(
    IntrinsicInst &I, vc::InternalIntrinsic::ID LoadIID,
    vc::InternalIntrinsic::ID StoreIID, Value *Base, Type *AddrTy) const {
  auto [IsLoad, Mask, Ptr, Data, Align] = getGatherScatterOperands(I);
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto *Ty = Data->getType();
  auto *VTy = cast<IGCLLVM::FixedVectorType>(Ty);
  if (VTy->isPtrOrPtrVectorTy()) {
    auto *IntPtrTy = DL_->getIntPtrType(VTy);
    VTy = &vc::getVectorType(*IntPtrTy);
    Data = Builder.CreatePtrToInt(Data, IntPtrTy);
  }

  auto *ETy = VTy->getElementType();
  auto ESize = DL_->getTypeSizeInBits(ETy);

  auto *Extend = createExtendImpl(Builder, Data);
  auto *Addr = Builder.CreatePtrToInt(
      Ptr, IGCLLVM::FixedVectorType::get(AddrTy, VTy->getNumElements()));

  bool IsSLM = LoadIID == vc::InternalIntrinsic::lsc_load_slm ||
               StoreIID == vc::InternalIntrinsic::lsc_store_slm;
  auto CacheOpts = getLSCCacheOpts(&I, IsSLM);

  if (IsLoad) {
    auto *Load = createLSCLoadImpl(Builder, M, LoadIID, ESize, Mask, Base, Addr,
                                   Extend, CacheOpts, Align);
    auto *Res = createTruncateImpl(Builder, VTy, Load);
    if (Ty->isPtrOrPtrVectorTy())
      Res = Builder.CreateIntToPtr(Res, Ty);
    return cast<Instruction>(Res);
  }

  return createLSCStoreImpl(Builder, M, StoreIID, ESize, Mask, Base, Addr,
                            Extend, CacheOpts, Align);
}

std::pair<unsigned, AtomicOrdering>
GenXLoadStoreLowering::getAddressSpaceAndOrderingOfAtomic(
    const Instruction &AtomicI) const {
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
  if (auto *FI = dyn_cast<FenceInst>(&AtomicI)) {
    auto ScopeID = FI->getSyncScopeID();
    auto AS = StringSwitch<unsigned>(SyncScopeNames[ScopeID])
                  .Case("workitem", vc::AddrSpace::Private)
                  .Case("subgroup", vc::AddrSpace::Private)
                  .Case("workgroup", vc::AddrSpace::Local)
                  .Default(vc::AddrSpace::Global);
    return {AS, FI->getOrdering()};
  }
  if (isSPIRVAtomic(&AtomicI)) {
    auto *II = cast<IntrinsicInst>(&AtomicI);
    unsigned AS = vc::getAddrSpace(
        II->getArgOperand(SPIRVAtomicOp::PointerOp)->getType());
    unsigned Semantics =
        cast<ConstantInt>(II->getArgOperand(SPIRVAtomicOp::SemanticsOp))
            ->getZExtValue();
    switch (Semantics) {
    default:
      LLVM_FALLTHROUGH;
    case SequentiallyConsistent:
      return {AS, AtomicOrdering::SequentiallyConsistent};
    case Relaxed:
      return {AS, AtomicOrdering::Monotonic};
    case Acquire:
      return {AS, AtomicOrdering::Acquire};
    case Release:
      return {AS, AtomicOrdering::Release};
    case AcquireRelease:
      return {AS, AtomicOrdering::AcquireRelease};
    }
  }
  IGC_ASSERT_MESSAGE(false, "Unimplemented atomic inst");
  return {0, AtomicOrdering::Monotonic};
}

GenXLoadStoreLowering::CacheControlsVector
GenXLoadStoreLowering::getLSCCacheOptsImpl(Instruction *I, bool IsSLM) const {
  bool IsUncached = !IsSLM && I->getMetadata("nontemporal") != nullptr;

  if (!IsSLM && !IsUncached) {
    auto Scope = getLSCFenceScope(I);
    switch (Scope) {
    default:
      break;
    case LSC_SCOPE_GPUS:
    case LSC_SCOPE_SYSACQ:
    case LSC_SCOPE_SYSREL:
      IsUncached |= I->isAtomic();
      break;
    }
  }

  if (IsUncached)
    return CacheControlsVector(ST->getNumCacheLevels(), LSC_CACHING_UNCACHED);

  return CacheControlsVector(ST->getNumCacheLevels(), LSC_CACHING_DEFAULT);
}

Constant *GenXLoadStoreLowering::getLSCCacheOpts(Instruction *I,
                                                 bool IsSLM) const {
  const auto CacheOpts = getLSCCacheOptsImpl(I, IsSLM);
  auto *Int8Ty = IntegerType::get(I->getContext(), 8);

  SmallVector<Constant *, 4> CacheOptValues;
  llvm::transform(CacheOpts, std::back_inserter(CacheOptValues),
                  [Int8Ty](auto Opt) { return ConstantInt::get(Int8Ty, Opt); });

  return ConstantVector::get(CacheOptValues);
}

LSC_SCOPE GenXLoadStoreLowering::getLSCFenceScope(Instruction *I) const {
  SyncScope::ID ScopeID = SyncScope::SingleThread;
  if (auto *LI = dyn_cast<LoadInst>(I))
    ScopeID = LI->getSyncScopeID();
  else if (auto *SI = dyn_cast<StoreInst>(I))
    ScopeID = SI->getSyncScopeID();
  else if (auto *AI = dyn_cast<AtomicRMWInst>(I))
    ScopeID = AI->getSyncScopeID();
  else if (auto *AI = dyn_cast<AtomicCmpXchgInst>(I))
    ScopeID = AI->getSyncScopeID();
  else if (auto *FI = dyn_cast<FenceInst>(I))
    ScopeID = FI->getSyncScopeID();
  else if (isSPIRVAtomic(I)) {
    auto *II = cast<IntrinsicInst>(I);
    unsigned Scope =
        cast<ConstantInt>(II->getArgOperand(SPIRVAtomicOp::ScopeOp))
            ->getZExtValue();
    switch (Scope) {
    default:
      LLVM_FALLTHROUGH;
    case CrossDevice:
      return LSC_SCOPE_GPUS;
    case Device:
      return ST->hasMultiTile() ? LSC_SCOPE_GPU : LSC_SCOPE_TILE;
    case Workgroup:
    case Subgroup:
    case Invocation:
      return LSC_SCOPE_GROUP;
    }
  }

  switch (ScopeID) {
  case SyncScope::SingleThread:
    return LSC_SCOPE_GROUP;
  case SyncScope::System:
    return LSC_SCOPE_SYSACQ;
  }

  return StringSwitch<LSC_SCOPE>(SyncScopeNames[ScopeID])
      .Case("subgroup", LSC_SCOPE_GROUP)
      .Case("workgroup", LSC_SCOPE_GROUP)
      .Case("device", ST->hasMultiTile() ? LSC_SCOPE_GPU : LSC_SCOPE_TILE)
      .Case("all_devices", LSC_SCOPE_GPUS)
      .Default(LSC_SCOPE_GROUP);
}

Instruction *
GenXLoadStoreLowering::createLSCStandAloneFence(FenceInst &I) const {
  IRBuilder<> Builder(&I);
  auto [AS, Ordering] = getAddressSpaceAndOrderingOfAtomic(I);

  if (AS == vc::AddrSpace::Private ||
      AS == vc::AddrSpace::Local && ST->hasLocalMemFenceSupress())
    return nullptr;

  bool IsGlobal = AS == vc::AddrSpace::Global;
  auto FenceOp = IsGlobal ? getLSCFenceOp(Ordering) : LSC_FENCE_OP_NONE;
  auto SubFuncID = IsGlobal ? LSC_UGM : LSC_SLM;
  auto Scope = getLSCFenceScope(&I);

  auto *M = I.getModule();
  auto *Func = GenXIntrinsic::getAnyDeclaration(
      M, GenXIntrinsic::genx_lsc_fence, {Builder.getInt1Ty()});
  return Builder.CreateCall(Func,
                            {Builder.getTrue(), Builder.getInt8(SubFuncID),
                             Builder.getInt8(FenceOp), Builder.getInt8(Scope)});
}

void GenXLoadStoreLowering::createLSCAtomicFenceImpl(Instruction &AtomicI,
                                                     IRBuilder<> &Builder,
                                                     bool IsPostFence) const {
  auto [AS, Ordering] = getAddressSpaceAndOrderingOfAtomic(AtomicI);

  bool IsGlobal = AS != vc::AddrSpace::Local;
  bool EmitFence = IsGlobal || !ST->hasLocalMemFenceSupress();
  if (!EmitFence)
    return;

  EmitFence = Ordering == AtomicOrdering::SequentiallyConsistent ||
              Ordering == AtomicOrdering::AcquireRelease ||
              Ordering == (IsPostFence ? AtomicOrdering::Acquire
                                       : AtomicOrdering::Release);
  if (!EmitFence)
    return;

  auto SubFuncID = IsGlobal ? LSC_UGM : LSC_SLM;
  auto FenceOp = LSC_FENCE_OP_NONE;
  auto Scope = getLSCFenceScope(&AtomicI);

  auto *M = AtomicI.getModule();

  auto *Func = GenXIntrinsic::getAnyDeclaration(
      M, GenXIntrinsic::genx_lsc_fence, {Builder.getInt1Ty()});
  auto *Fence = Builder.CreateCall(
      Func, {Builder.getTrue(), Builder.getInt8(SubFuncID),
             Builder.getInt8(FenceOp), Builder.getInt8(Scope)});
  LLVM_DEBUG(dbgs() << "Created: " << *Fence << "\n");
}

Instruction *GenXLoadStoreLowering::createLSCAtomicImpl(
    Instruction &I, vc::InternalIntrinsic::ID IID, LSC_OP AtomicOp, Value *Base,
    Value *Addr, Value *Src0, Value *Src1) const {
  IGC_ASSERT_EXIT(IID == vc::InternalIntrinsic::lsc_atomic_bti ||
                  IID == vc::InternalIntrinsic::lsc_atomic_slm ||
                  IID == vc::InternalIntrinsic::lsc_atomic_ugm);
  IRBuilder<> Builder(&I);
  auto *M = I.getModule();

  Addr = makeVector(Builder, Addr);
  auto *Pred = makeVector(Builder, Builder.getTrue());

  auto *DataTy = Src0->getType();
  auto *AddrTy = Addr->getType();
  auto *PredTy = Pred->getType();

  auto ElementSize = getLSCElementSize(DataTy->getScalarSizeInBits());
  IGC_ASSERT_EXIT(ElementSize == LSC_DATA_SIZE_16c32b ||
                  ElementSize == LSC_DATA_SIZE_32b ||
                  ElementSize == LSC_DATA_SIZE_64b);

  if (ElementSize == LSC_DATA_SIZE_16c32b) {
    // We must preserve undef operands but
    // zext/sext casts make them zero.
    if (!isa<UndefValue>(Src0)) {
      Src0 = Builder.CreateBitCast(Src0, Builder.getInt16Ty());
      Src0 = Builder.CreateZExt(Src0, Builder.getInt32Ty());
    } else
      Src0 = UndefValue::get(Builder.getInt32Ty());

    if (!isa<UndefValue>(Src1)) {
      Src1 = Builder.CreateBitCast(Src1, Builder.getInt16Ty());
      Src1 = Builder.CreateZExt(Src1, Builder.getInt32Ty());
    } else
      Src1 = UndefValue::get(Builder.getInt32Ty());
  }

  auto AddrSize = IID == vc::InternalIntrinsic::lsc_atomic_ugm
                      ? LSC_ADDR_SIZE_64b
                      : LSC_ADDR_SIZE_32b;
  bool IsSLM = IID == vc::InternalIntrinsic::lsc_atomic_slm;

  auto *CacheOpts = getLSCCacheOpts(&I, IsSLM);
  auto *CacheOptsTy = CacheOpts->getType();

  Src0 = makeVector(Builder, Src0);
  Src1 = makeVector(Builder, Src1);
  auto *DataVTy = cast<IGCLLVM::FixedVectorType>(Src0->getType());

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(
      M, IID, {DataVTy, PredTy, CacheOptsTy, AddrTy});

  SmallVector<Value *, 12> Args = {
      Pred,
      Builder.getInt8(AtomicOp),
      Builder.getInt8(AddrSize),
      Builder.getInt8(ElementSize),
      CacheOpts,
      Base,
      Addr,
      Builder.getInt16(1), // Address scale
      Builder.getInt32(0), // Address offset
      Src0,
      Src1,
      UndefValue::get(DataVTy), // Old value to merge
  };

  createLSCAtomicFenceImpl(I, Builder, false);
  auto *Inst = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "Created: " << *Inst << "\n");
  createLSCAtomicFenceImpl(I, Builder, true);

  auto *Scalar = Builder.CreateBitCast(Inst, DataVTy->getElementType());
  if (ElementSize != LSC_DATA_SIZE_16c32b || I.getType()->isVoidTy())
    return cast<Instruction>(Scalar);

  auto *Trunc = Builder.CreateTrunc(Scalar, Builder.getInt16Ty());
  auto *Cast = Builder.CreateBitCast(Trunc, DataTy);
  return cast<Instruction>(Cast);
}

Instruction *
GenXLoadStoreLowering::createLSCAtomicLoad(LoadInst &I,
                                           vc::InternalIntrinsic::ID IID,
                                           Type *AddrTy, Value *Base) const {
  IGC_ASSERT_EXIT(I.isAtomic());
  IRBuilder<> Builder(&I);

  auto *Ptr = I.getPointerOperand();
  auto *Addr = Builder.CreatePtrToInt(Ptr, AddrTy);

  auto *DataTy = I.getType();
  auto *Undef = UndefValue::get(DataTy);

  return createLSCAtomicImpl(I, IID, LSC_ATOMIC_LOAD, Base, Addr, Undef, Undef);
}

Instruction *
GenXLoadStoreLowering::createLSCAtomicStore(StoreInst &I,
                                            vc::InternalIntrinsic::ID IID,
                                            Type *AddrTy, Value *Base) const {
  IGC_ASSERT_EXIT(I.isAtomic());
  IRBuilder<> Builder(&I);

  auto *Ptr = I.getPointerOperand();
  auto *Addr = Builder.CreatePtrToInt(Ptr, AddrTy);

  auto *Src = I.getValueOperand();
  auto *DataTy = Src->getType();
  auto *Undef = UndefValue::get(DataTy);

  return createLSCAtomicImpl(I, IID, LSC_ATOMIC_STORE, Base, Addr, Src, Undef);
}

Instruction *
GenXLoadStoreLowering::createLSCAtomicRMW(AtomicRMWInst &I,
                                          vc::InternalIntrinsic::ID IID,
                                          Type *AddrTy, Value *Base) const {
  IGC_ASSERT_EXIT(I.isAtomic());
  IRBuilder<> Builder(&I);

  auto *Ptr = I.getPointerOperand();
  auto *Addr = Builder.CreatePtrToInt(Ptr, AddrTy);

  auto *Src = I.getValOperand();
  auto *DataTy = Src->getType();
  auto *Undef = UndefValue::get(DataTy);

  LSC_OP AtomicOp = LSC_ATOMIC_STORE;
  switch (I.getOperation()) {
  case AtomicRMWInst::BinOp::Xchg:
    AtomicOp = LSC_ATOMIC_STORE;
    break;
  case AtomicRMWInst::BinOp::Add: {
    AtomicOp = LSC_ATOMIC_IADD;
    auto *C = dyn_cast<ConstantInt>(Src);
    if (C && C->getSExtValue() == 1) {
      AtomicOp = LSC_ATOMIC_IINC;
      Src = Undef;
    }
    break;
  }
  case AtomicRMWInst::BinOp::Sub: {
    AtomicOp = LSC_ATOMIC_ISUB;
    auto *C = dyn_cast<ConstantInt>(Src);
    if (C && C->getSExtValue() == 1) {
      AtomicOp = LSC_ATOMIC_IDEC;
      Src = Undef;
    }
    break;
  }
  case AtomicRMWInst::BinOp::And:
    AtomicOp = LSC_ATOMIC_AND;
    break;
  case AtomicRMWInst::BinOp::Or:
    AtomicOp = LSC_ATOMIC_OR;
    break;
  case AtomicRMWInst::BinOp::Xor:
    AtomicOp = LSC_ATOMIC_XOR;
    break;
  case AtomicRMWInst::BinOp::Max:
    AtomicOp = LSC_ATOMIC_SMAX;
    break;
  case AtomicRMWInst::BinOp::Min:
    AtomicOp = LSC_ATOMIC_SMIN;
    break;
  case AtomicRMWInst::BinOp::UMax:
    AtomicOp = LSC_ATOMIC_UMAX;
    break;
  case AtomicRMWInst::BinOp::UMin:
    AtomicOp = LSC_ATOMIC_UMIN;
    break;
  case AtomicRMWInst::BinOp::FAdd:
    AtomicOp = LSC_ATOMIC_FADD;
    break;
  case AtomicRMWInst::BinOp::FSub:
    AtomicOp = LSC_ATOMIC_FSUB;
    break;
#if IGC_LLVM_VERSION_MAJOR >= 15
  case AtomicRMWInst::BinOp::FMax:
    AtomicOp = LSC_ATOMIC_FMAX;
    break;
  case AtomicRMWInst::BinOp::FMin:
    AtomicOp = LSC_ATOMIC_FMIN;
    break;
#endif // IGC_LLVM_VERSION_MAJOR >= 15
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Unsupported atomic operation");
    break;
  }

  return createLSCAtomicImpl(I, IID, AtomicOp, Base, Addr, Src, Undef);
}

Instruction *
GenXLoadStoreLowering::createLSCAtomicRMW(IntrinsicInst &I,
                                          vc::InternalIntrinsic::ID IID,
                                          Type *AddrTy, Value *Base) const {
  IGC_ASSERT(isSPIRVAtomic(&I));

  IRBuilder<> Builder(&I);
  auto *Ptr = I.getArgOperand(SPIRVAtomicOp::PointerOp);
  auto *Addr = Builder.CreatePtrToInt(Ptr, AddrTy);

  auto *Src = I.getArgOperand(SPIRVAtomicOp::SourceOp);
  auto *DataTy = Src->getType();
  auto *Undef = UndefValue::get(DataTy);

  auto AtomicOp = LSC_INVALID;
  auto ID = vc::getAnyIntrinsicID(&I);
  switch (ID) {
  case vc::InternalIntrinsic::atomic_fmin:
    AtomicOp = LSC_ATOMIC_FMIN;
    break;
  case vc::InternalIntrinsic::atomic_fmax:
    AtomicOp = LSC_ATOMIC_FMAX;
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Unexpected atomic operation");
    break;
  }

  return createLSCAtomicImpl(I, IID, AtomicOp, Base, Addr, Src, Undef);
}

Instruction *
GenXLoadStoreLowering::createLSCAtomicCmpXchg(AtomicCmpXchgInst &I,
                                              vc::InternalIntrinsic::ID IID,
                                              Type *AddrTy, Value *Base) const {
  IGC_ASSERT_EXIT(I.isAtomic());
  IRBuilder<> Builder(&I);

  auto *Ptr = I.getPointerOperand();
  auto *Addr = Builder.CreatePtrToInt(Ptr, AddrTy);

  auto *CmpVal = I.getCompareOperand();
  auto *NewVal = I.getNewValOperand();

  auto *RetTy = I.getType();
  Value *Res = UndefValue::get(RetTy);

  auto *Atomic =
      createLSCAtomicImpl(I, IID, LSC_ATOMIC_ICAS, Base, Addr, CmpVal, NewVal);
  auto *Cmp = Builder.CreateICmpEQ(Atomic, CmpVal);

  Res = Builder.CreateInsertValue(Res, Atomic, 0);
  Res = Builder.CreateInsertValue(Res, Cmp, 1);
  return cast<Instruction>(Res);
}

Instruction *
GenXLoadStoreLowering::createLegacyStandAloneFence(FenceInst &I) const {
  IRBuilder<> Builder(&I);
  auto [AS, Ordering] = getAddressSpaceAndOrderingOfAtomic(I);

  if (AS == vc::AddrSpace::Private ||
      AS == vc::AddrSpace::Local && ST->hasLocalMemFenceSupress())
    return nullptr;

  bool IsGlobal = AS == vc::AddrSpace::Global;
  bool IsInvalidateL1 = Ordering == AtomicOrdering::SequentiallyConsistent ||
                        Ordering == AtomicOrdering::AcquireRelease ||
                        Ordering == AtomicOrdering::Acquire;
  IsInvalidateL1 &= IsGlobal;

  uint8_t FenceOp = 1;
  if (!IsGlobal)
    FenceOp |= 1 << 5;
  if (IsInvalidateL1)
    FenceOp |= 1 << 6;

  auto *M = I.getModule();
  auto *Func = GenXIntrinsic::getAnyDeclaration(M, GenXIntrinsic::genx_fence);
  return Builder.CreateCall(Func, {Builder.getInt8(FenceOp)});
}

void GenXLoadStoreLowering::createLegacyAtomicFenceImpl(
    Instruction &AtomicI, IRBuilder<> &Builder, bool IsPostFence) const {
  IGC_ASSERT_EXIT(AtomicI.isAtomic());

  auto [AS, Ordering] = getAddressSpaceAndOrderingOfAtomic(AtomicI);

  bool IsGlobal = AS != vc::AddrSpace::Local;
  bool EmitFence = IsGlobal || !ST->hasLocalMemFenceSupress();
  if (!EmitFence)
    return;

  EmitFence = Ordering == AtomicOrdering::SequentiallyConsistent ||
              Ordering == AtomicOrdering::AcquireRelease ||
              Ordering == (IsPostFence ? AtomicOrdering::Acquire
                                       : AtomicOrdering::Release);
  if (!EmitFence)
    return;

  uint8_t FenceOp = 1;
  if (!IsGlobal)
    FenceOp |= 1 << 5;

  auto *M = AtomicI.getModule();
  auto *Func = GenXIntrinsic::getAnyDeclaration(M, GenXIntrinsic::genx_fence);
  auto *Fence = Builder.CreateCall(Func, {Builder.getInt8(FenceOp)});
  LLVM_DEBUG(dbgs() << "Created: " << *Fence << "\n");
}

Instruction *GenXLoadStoreLowering::createLegacyAtomicImpl(
    Instruction &I, GenXIntrinsic::ID IID, Value *BTI, Value *Addr, Value *Src0,
    Value *Src1) const {
  IGC_ASSERT_EXIT(I.isAtomic());
  IRBuilder<> Builder(&I);

  auto *M = I.getModule();

  auto *OrigDataTy = Src0->getType();

  Src0 = makeVector(Builder, Src0);
  Src1 = makeVector(Builder, Src1);
  Addr = makeVector(Builder, Addr);
  auto *Pred = makeVector(Builder, Builder.getTrue());

  auto *DataTy = Src0->getType();
  auto *AddrTy = Addr->getType();
  auto *PredTy = Pred->getType();

  auto ElementSize = DataTy->getScalarSizeInBits();
  IGC_ASSERT_EXIT(ElementSize == DWordBits ||
                  (!BTI && ElementSize == QWordBits));

  Function *AtomicFunc =
      GenXIntrinsic::getAnyDeclaration(M, IID, {DataTy, PredTy, AddrTy});

  SmallVector<Value *, 5> Args = {Pred};
  if (BTI)
    Args.push_back(BTI);
  Args.push_back(Addr);
  if (!isa<UndefValue>(Src0))
    Args.push_back(Src0);
  if (!isa<UndefValue>(Src1))
    Args.push_back(Src1);
  Args.push_back(UndefValue::get(DataTy));

  createLegacyAtomicFenceImpl(I, Builder, false);
  auto *Inst = Builder.CreateCall(AtomicFunc, Args);
  LLVM_DEBUG(dbgs() << "Created: " << *Inst << "\n");
  createLegacyAtomicFenceImpl(I, Builder, true);

  auto *Scalar = Builder.CreateBitCast(Inst, OrigDataTy);
  return cast<Instruction>(Scalar);
}

Instruction *GenXLoadStoreLowering::createLegacyAtomicLoad(LoadInst &I,
                                                           unsigned BTI) const {
  IGC_ASSERT_EXIT(I.isAtomic());
  IRBuilder<> Builder(&I);

  bool IsBTI = BTI <= visa::RSI_Stateless;
  auto *AddrTy = IsBTI ? Builder.getInt32Ty() : Builder.getInt64Ty();
  Value *BTIV = IsBTI ? Builder.getInt32(BTI) : nullptr;

  auto *Ptr = I.getPointerOperand();
  auto *Addr = Builder.CreatePtrToInt(Ptr, AddrTy);

  auto *DataTy = I.getType();
  auto *Src = Constant::getNullValue(DataTy);

  auto IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_or
                   : GenXIntrinsic::genx_svm_atomic_or;
  return createLegacyAtomicImpl(I, IID, BTIV, Addr, Src,
                                UndefValue::get(Src->getType()));
}

Instruction *
GenXLoadStoreLowering::createLegacyAtomicStore(StoreInst &I,
                                               unsigned BTI) const {
  IGC_ASSERT_EXIT(I.isAtomic());
  IRBuilder<> Builder(&I);

  bool IsBTI = BTI <= visa::RSI_Stateless;
  auto *AddrTy = IsBTI ? Builder.getInt32Ty() : Builder.getInt64Ty();
  Value *BTIV = IsBTI ? Builder.getInt32(BTI) : nullptr;

  auto *Ptr = I.getPointerOperand();
  auto *Addr = Builder.CreatePtrToInt(Ptr, AddrTy);

  auto *Src = I.getValueOperand();

  auto IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_xchg
                   : GenXIntrinsic::genx_svm_atomic_xchg;
  return createLegacyAtomicImpl(I, IID, BTIV, Addr, Src,
                                UndefValue::get(Src->getType()));
}

Instruction *GenXLoadStoreLowering::createLegacyAtomicRMW(AtomicRMWInst &I,
                                                          unsigned BTI) const {
  IGC_ASSERT_EXIT(I.isAtomic());
  IRBuilder<> Builder(&I);

  bool IsBTI = BTI <= visa::RSI_Stateless;
  auto *AddrTy = IsBTI ? Builder.getInt32Ty() : Builder.getInt64Ty();
  Value *BTIV = IsBTI ? Builder.getInt32(BTI) : nullptr;

  auto *Ptr = I.getPointerOperand();
  auto *Addr = Builder.CreatePtrToInt(Ptr, AddrTy);

  auto *Src = I.getValOperand();
  auto *DataTy = Src->getType();
  auto *Undef = UndefValue::get(DataTy);

  auto IID = GenXIntrinsic::not_any_intrinsic;

  switch (I.getOperation()) {
  case AtomicRMWInst::BinOp::Xchg:
    IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_xchg
                : GenXIntrinsic::genx_svm_atomic_xchg;
    break;
  case AtomicRMWInst::BinOp::Add: {
    IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_add
                : GenXIntrinsic::genx_svm_atomic_add;
    auto *C = dyn_cast<ConstantInt>(Src);
    if (C && C->getSExtValue() == 1) {
      IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_inc
                  : GenXIntrinsic::genx_svm_atomic_inc;
      Src = Undef;
    }
    break;
  }
  case AtomicRMWInst::BinOp::Sub: {
    IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_sub
                : GenXIntrinsic::genx_svm_atomic_sub;
    auto *C = dyn_cast<ConstantInt>(Src);
    if (C && C->getSExtValue() == 1) {
      IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_dec
                  : GenXIntrinsic::genx_svm_atomic_dec;
      Src = Undef;
    }
    break;
  }
  case AtomicRMWInst::BinOp::And:
    IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_and
                : GenXIntrinsic::genx_svm_atomic_and;
    break;
  case AtomicRMWInst::BinOp::Or:
    IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_or
                : GenXIntrinsic::genx_svm_atomic_or;
    break;
  case AtomicRMWInst::BinOp::Xor:
    IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_xor
                : GenXIntrinsic::genx_svm_atomic_xor;
    break;
  case AtomicRMWInst::BinOp::Max:
    IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_imax
                : GenXIntrinsic::genx_svm_atomic_imax;
    break;
  case AtomicRMWInst::BinOp::Min:
    IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_imin
                : GenXIntrinsic::genx_svm_atomic_imin;
    break;
  case AtomicRMWInst::BinOp::UMax:
    IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_max
                : GenXIntrinsic::genx_svm_atomic_max;
    break;
  case AtomicRMWInst::BinOp::UMin:
    IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_min
                : GenXIntrinsic::genx_svm_atomic_min;
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Unsupported atomic operation");
    break;
  }

  return createLegacyAtomicImpl(I, IID, BTIV, Addr, Src, Undef);
}

Instruction *
GenXLoadStoreLowering::createLegacyAtomicCmpXchg(AtomicCmpXchgInst &I,
                                                 unsigned BTI) const {
  IGC_ASSERT_EXIT(I.isAtomic());
  IRBuilder<> Builder(&I);

  bool IsBTI = BTI <= visa::RSI_Stateless;
  auto *AddrTy = IsBTI ? Builder.getInt32Ty() : Builder.getInt64Ty();
  Value *BTIV = IsBTI ? Builder.getInt32(BTI) : nullptr;

  auto *Ptr = I.getPointerOperand();
  auto *Addr = Builder.CreatePtrToInt(Ptr, AddrTy);

  auto *CmpVal = I.getCompareOperand();
  auto *NewVal = I.getNewValOperand();

  auto *RetTy = I.getType();
  Value *Res = UndefValue::get(RetTy);

  auto IID = IsBTI ? GenXIntrinsic::genx_dword_atomic_cmpxchg
                   : GenXIntrinsic::genx_svm_atomic_cmpxchg;
  auto *Atomic = createLegacyAtomicImpl(I, IID, BTIV, Addr, NewVal, CmpVal);
  auto *Cmp = Builder.CreateICmpEQ(Atomic, CmpVal);

  Res = Builder.CreateInsertValue(Res, Atomic, 0);
  Res = Builder.CreateInsertValue(Res, Cmp, 1);
  return cast<Instruction>(Res);
}

Instruction *GenXLoadStoreLowering::createLegacyBlockLoadImpl(
    IRBuilder<> &Builder, Module *M, GenXIntrinsic::ID IID, unsigned BTI,
    IGCLLVM::FixedVectorType *Ty, Value *Addr) const {
  IGC_ASSERT_EXIT(Ty);
  IGC_ASSERT_EXIT(Addr);

  Function *Func = nullptr;
  SmallVector<Value *, 3> Args;

  if (BTI > visa::RSI_Stateless) {
    IGC_ASSERT_EXIT(IID == GenXIntrinsic::genx_svm_block_ld ||
                    IID == GenXIntrinsic::genx_svm_block_ld_unaligned);
    Func = GenXIntrinsic::getGenXDeclaration(M, IID, {Ty, Addr->getType()});
  } else {
    IGC_ASSERT_EXIT(IID == GenXIntrinsic::genx_oword_ld ||
                    IID == GenXIntrinsic::genx_oword_ld_unaligned);
    Func = GenXIntrinsic::getGenXDeclaration(M, IID, {Ty});
    Args.push_back(Builder.getInt32(0));   // is_modified flag
    Args.push_back(Builder.getInt32(BTI)); // buffer index
  }

  Args.push_back(Addr);

  auto *Load = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "Created: " << *Load << "\n");
  return Load;
}

Instruction *GenXLoadStoreLowering::createLegacyGatherLoadImpl(
    IRBuilder<> &Builder, Module *M, unsigned BTI, unsigned ESize,
    IGCLLVM::FixedVectorType *Ty, Value *Pred, Value *Base, Value *Offset,
    Value *Source, ConstantInt *Align) const {
  IGC_ASSERT_EXIT(Ty);
  IGC_ASSERT_EXIT(Pred);
  IGC_ASSERT_EXIT(Base);
  IGC_ASSERT_EXIT(Offset);

  GenXIntrinsic::ID IID = GenXIntrinsic::not_genx_intrinsic;

  auto NElements = Ty->getNumElements();
  auto *AddrTy = cast<IGCLLVM::FixedVectorType>(Offset->getType());
  auto *RetVTy = Ty;

  SmallVector<Value *, 7> Args = {Pred};

  bool IsBTI = BTI <= visa::RSI_Stateless;
  if (IsBTI) {
    IID = GenXIntrinsic::genx_gather_scaled;
    Args.push_back(Builder.getInt32(
        genx::log2(ESize))); // Log2(NumBlocks), each block is 1 byte
    Args.push_back(Builder.getInt16(0)); // scale
    Args.push_back(Builder.getInt32(BTI));
    Args.push_back(Base);
    Args.push_back(Offset);
  } else {
    IID = GenXIntrinsic::genx_svm_gather;

    if (ESize < DWordBytes)
      RetVTy = IGCLLVM::FixedVectorType::get(Builder.getInt8Ty(),
                                             NElements * DWordBytes);

    // Log2(NumBlocks), block can be 1, 4 or 8 bytes
    Args.push_back(Builder.getInt32(ESize == WordBytes ? 1 : 0));

    // Global offset is not supported, so emitting add instruction
    auto *Addr = Offset;
    auto *BaseConst = dyn_cast<ConstantInt>(Base);
    if (!BaseConst || !BaseConst->isNullValue()) {
      auto *BaseSplat =
          Builder.CreateVectorSplat(AddrTy->getNumElements(), Base);
      Addr = Builder.CreateAdd(BaseSplat, Offset);
    }

    Args.push_back(Addr);
  }

  Args.push_back(Source ? Builder.CreateBitCast(Source, RetVTy)
                        : UndefValue::get(RetVTy));

  auto *Func = GenXIntrinsic::getGenXDeclaration(
      M, IID, {RetVTy, Pred->getType(), AddrTy});
  auto *Load = Builder.CreateCall(Func, Args);

  if (Align) {
    auto &Ctx = Load->getContext();
    auto *MD = ConstantAsMetadata::get(Align);
    Load->setMetadata(AlignMDName, MDNode::get(Ctx, MD));
  }

  LLVM_DEBUG(dbgs() << "Created: " << *Load << "\n");
  return cast<Instruction>(Builder.CreateBitCast(Load, Ty));
}

Value *GenXLoadStoreLowering::createLegacyGatherScatterQWordImpl(
    IRBuilder<> &Builder, Module *M, unsigned BTI, bool IsLoad, Value *Pred,
    Value *Addr, Value *Source, ConstantInt *Align) const {
  IGC_ASSERT(M);
  IGC_ASSERT(Pred);
  IGC_ASSERT(Addr);
  IGC_ASSERT(Source);
  IGC_ASSERT(Align);
  IGC_ASSERT(BTI <= visa::RSI_Stateless);

  auto *VTy = cast<IGCLLVM::FixedVectorType>(Source->getType());
  unsigned NElements = VTy->getNumElements();

  auto *CastVTy =
      IGCLLVM::FixedVectorType::get(Builder.getInt32Ty(), NElements * 2);
  auto *Cast = Builder.CreateBitCast(Source, CastVTy);

  // gather4/scatter4 address values must be dword-aligned
  unsigned Alignment = Align->getValue().getZExtValue();
  if (Alignment < DWordBytes) {
    auto *ExtractVTy =
        IGCLLVM::FixedVectorType::get(Builder.getInt32Ty(), NElements);
    auto *RdRgnFunc = GenXIntrinsic::getAnyDeclaration(
        M, GenXIntrinsic::genx_rdregioni,
        {ExtractVTy, CastVTy, Builder.getInt16Ty()});

    SmallVector<Value *, 6> Args = {
        Cast,
        Builder.getInt32(2),                  // vstride
        Builder.getInt32(1),                  // width
        Builder.getInt32(0),                  // stride
        Builder.getInt16(0),                  // offset
        UndefValue::get(Builder.getInt32Ty()) // parent width, ignored
    };
    auto *Low = Builder.CreateCall(RdRgnFunc, Args);
    Args[4] = Builder.getInt16(DWordBytes); // offset for high qword parts
    auto *High = Builder.CreateCall(RdRgnFunc, Args);

    if (IsLoad) {
      auto *LoadLow = createLegacyGatherLoadImpl(
          Builder, M, BTI, DWordBytes, ExtractVTy, Pred, Builder.getInt32(0),
          Addr, Low, Align);
      auto *LoadHigh = createLegacyGatherLoadImpl(
          Builder, M, BTI, DWordBytes, ExtractVTy, Pred,
          Builder.getInt32(DWordBytes), Addr, High, Align);

      auto *WrRgnFunc = GenXIntrinsic::getAnyDeclaration(
          M, GenXIntrinsic::genx_wrregioni,
          {CastVTy, ExtractVTy, Builder.getInt16Ty(), Builder.getInt1Ty()});
      SmallVector<Value *, 8> Args = {
          UndefValue::get(CastVTy), // vector to insert to
          LoadLow,
          Builder.getInt32(2),                   // vstride
          Builder.getInt32(1),                   // width
          Builder.getInt32(0),                   // stride
          Builder.getInt16(0),                   // offset for low qword parts
          UndefValue::get(Builder.getInt32Ty()), // parent width, ignored
          Builder.getTrue(),
      };
      auto *InsertLow = Builder.CreateCall(WrRgnFunc, Args);
      Args[0] = InsertLow;
      Args[1] = LoadHigh;
      Args[5] = Builder.getInt16(DWordBytes); // offset for high qword parts
      auto *InsertHigh = Builder.CreateCall(WrRgnFunc, Args);
      return Builder.CreateBitCast(InsertHigh, VTy);
    }

    createLegacyScatterStoreImpl(Builder, M, BTI, DWordBytes, Pred,
                                 Builder.getInt32(0), Addr, Low, Align);
    return createLegacyScatterStoreImpl(Builder, M, BTI, DWordBytes, Pred,
                                        Builder.getInt32(DWordBytes), Addr,
                                        High, Align);
  }

  auto *RdRgnFunc = GenXIntrinsic::getAnyDeclaration(
      M, GenXIntrinsic::genx_rdregioni,
      {CastVTy, CastVTy, Builder.getInt16Ty()});
  auto *MemFunc = IsLoad ? GenXIntrinsic::getAnyDeclaration(
                               M, GenXIntrinsic::genx_gather4_scaled,
                               {CastVTy, Pred->getType(), Addr->getType()})
                         : GenXIntrinsic::getAnyDeclaration(
                               M, GenXIntrinsic::genx_scatter4_scaled,
                               {Pred->getType(), Addr->getType(), CastVTy});

  SmallVector<Value *, 6> ConvArgs = {
      Cast,
      Builder.getInt32(1),                  // vstride
      Builder.getInt32(NElements),          // width
      Builder.getInt32(2),                  // stride
      Builder.getInt16(0),                  // offset
      UndefValue::get(Builder.getInt32Ty()) // parent width, ignored
  };
  auto *Convert = Builder.CreateCall(RdRgnFunc, ConvArgs);

  SmallVector<Value *, 7> MemArgs = {
      Pred,                     // mask
      Builder.getInt32(0b1100), // channel mask: RG
      Builder.getInt16(0),      // scale
      Builder.getInt32(BTI),    // surface index
      Builder.getInt32(0),      // global offset
      Addr,
      Convert,
  };
  auto *MemOp = Builder.CreateCall(MemFunc, MemArgs);
  auto *MD = ConstantAsMetadata::get(Align);
  MemOp->setMetadata(AlignMDName, MDNode::get(MemOp->getContext(), MD));

  LLVM_DEBUG(dbgs() << "Created: " << *MemOp << "\n");
  if (!IsLoad)
    return MemOp;

  SmallVector<Value *, 6> BackConvArgs = {
      MemOp,
      Builder.getInt32(1),                  // vstride
      Builder.getInt32(2),                  // width
      Builder.getInt32(NElements),          // stride
      Builder.getInt16(0),                  // offset
      UndefValue::get(Builder.getInt32Ty()) // parent width, ignored
  };
  auto *BackConv = Builder.CreateCall(RdRgnFunc, BackConvArgs);
  return Builder.CreateBitCast(BackConv, VTy);
}

Instruction *GenXLoadStoreLowering::createLegacyBlockStoreImpl(
    IRBuilder<> &Builder, Module *M, unsigned BTI, Value *Addr,
    Value *Data) const {
  Function *Func = nullptr;
  auto *DataTy = Data->getType();

  SmallVector<Value *, 3> Args;

  bool IsBTI = BTI <= visa::RSI_Stateless;
  if (IsBTI) {
    Func = GenXIntrinsic::getGenXDeclaration(M, GenXIntrinsic::genx_oword_st,
                                             {DataTy});
    Args.push_back(Builder.getInt32(BTI));
  } else {
    Func = GenXIntrinsic::getGenXDeclaration(
        M, GenXIntrinsic::genx_svm_block_st, {Addr->getType(), DataTy});
  }

  Args.push_back(Addr);
  Args.push_back(Data);

  auto *Store = Builder.CreateCall(Func, Args);

  LLVM_DEBUG(dbgs() << "Created: " << *Store << "\n");
  return Store;
}

Instruction *GenXLoadStoreLowering::createLegacyScatterStoreImpl(
    IRBuilder<> &Builder, Module *M, unsigned BTI, unsigned ESize, Value *Pred,
    Value *Base, Value *Offset, Value *Data, ConstantInt *Align) const {
  IGC_ASSERT_EXIT(Pred);
  IGC_ASSERT_EXIT(Base);
  IGC_ASSERT_EXIT(Offset);
  IGC_ASSERT_EXIT(Data);

  GenXIntrinsic::ID IID = GenXIntrinsic::not_genx_intrinsic;

  auto *AddrTy = cast<IGCLLVM::FixedVectorType>(Offset->getType());
  auto *StoreVTy = cast<IGCLLVM::FixedVectorType>(Data->getType());
  auto NElements = StoreVTy->getNumElements();

  SmallVector<Value *, 7> Args = {Pred};

  bool IsBTI = BTI <= visa::RSI_Stateless;
  if (IsBTI) {
    IID = GenXIntrinsic::genx_scatter_scaled;
    Args.push_back(Builder.getInt32(
        genx::log2(ESize))); // Log2(NumBlocks), each block is 1 byte
    Args.push_back(Builder.getInt16(0)); // scale
    Args.push_back(Builder.getInt32(BTI));
    Args.push_back(Base);
    Args.push_back(Offset);
  } else {
    IID = GenXIntrinsic::genx_svm_scatter;

    if (ESize < DWordBytes) {
      StoreVTy = IGCLLVM::FixedVectorType::get(Builder.getInt8Ty(),
                                               NElements * DWordBytes);
      Data = Builder.CreateBitCast(Data, StoreVTy);
    }

    // Log2(NumBlocks), block can be 1, 4 or 8 bytes
    Args.push_back(Builder.getInt32(ESize == WordBytes ? 1 : 0));

    // Global offset is not supported, so emitting add instruction
    auto *Addr = Offset;
    auto *BaseConst = dyn_cast<ConstantInt>(Base);
    if (!BaseConst || !BaseConst->isNullValue()) {
      auto *BaseSplat =
          Builder.CreateVectorSplat(AddrTy->getNumElements(), Base);
      Addr = Builder.CreateAdd(BaseSplat, Offset);
    }

    Args.push_back(Addr);
  }

  Args.push_back(Data);

  auto *Func = GenXIntrinsic::getGenXDeclaration(
      M, IID, {Pred->getType(), AddrTy, StoreVTy});
  auto *Store = Builder.CreateCall(Func, Args);

  if (Align) {
    auto &Ctx = Store->getContext();
    auto *MD = ConstantAsMetadata::get(Align);
    Store->setMetadata(AlignMDName, MDNode::get(Ctx, MD));
  }

  LLVM_DEBUG(dbgs() << "Created: " << *Store << "\n");
  return Store;
}

Instruction *GenXLoadStoreLowering::createLegacyLoadStore(Instruction &I,
                                                          unsigned BTI,
                                                          Value *Ptr,
                                                          Value *Data) const {
  LLVM_DEBUG(dbgs() << "Lowering: " << I << "\n");
  IGC_ASSERT(isa<LoadInst>(I) || (isa<StoreInst>(I) && Data));

  IRBuilder<> Builder(&I);
  auto *M = I.getModule();
  bool IsLoad = isa<LoadInst>(I);

  auto *Ty = IsLoad ? I.getType() : Data->getType();
  auto *OrigTy = Ty;

  if (OrigTy->getScalarType()->isIntegerTy(1)) {
    if (IsLoad) {
      Ty = lowerBoolType(Builder, Ty);
    } else {
      Data = lowerBoolOperand(Builder, M, Data);
      Ty = Data->getType();
    }
  }

  auto *VTy = &vc::getVectorType(*Ty);

  if (Ty->isPtrOrPtrVectorTy()) {
    auto *IntPtrTy = DL_->getIntPtrType(Ty);
    VTy = &vc::getVectorType(*IntPtrTy);

    if (!IsLoad)
      Data = Builder.CreatePtrToInt(Data, IntPtrTy);
  }

  auto *ETy = VTy->getElementType();

  auto VSize = DL_->getTypeSizeInBits(VTy) / ByteBits;
  auto ESize = DL_->getTypeSizeInBits(ETy) / ByteBits;
  unsigned Rest = VSize;

  auto Align = IsLoad ? IGCLLVM::getAlignmentValue(&cast<LoadInst>(I))
                      : IGCLLVM::getAlignmentValue(&cast<StoreInst>(I));
  if (Align == 0)
    Align = DL_->getPrefTypeAlignment(VTy);

  if (!IsLoad)
    Data = Builder.CreateBitCast(Data, VTy);

  bool IsBTI = BTI <= visa::RSI_Stateless;
  auto *AddrTy = IsBTI ? Builder.getInt32Ty() : Builder.getInt64Ty();

  Value *Addr = Builder.CreatePtrToInt(Ptr, AddrTy);
  Value *Result = UndefValue::get(VTy);

  // Some target platforms do not support SLM oword block messages
  bool IsSLM = BTI == visa::RSI_Slm;
  bool IsBlockAllowed = IsSLM ? ST->hasSLMOWord() : true;

  bool IsOWordAligned = Align >= OWordBytes;
  bool IsDWordAligned = Align >= DWordBytes;

  // Try to generate OWord block load/store
  if (IsBlockAllowed &&
      (IsOWordAligned || (IsLoad && IsDWordAligned && !IsSLM))) {
    auto LoadIID = IsOWordAligned
                       ? (IsBTI ? GenXIntrinsic::genx_oword_ld
                                : GenXIntrinsic::genx_svm_block_ld)
                       : (IsBTI ? GenXIntrinsic::genx_oword_ld_unaligned
                                : GenXIntrinsic::genx_svm_block_ld_unaligned);
    unsigned Shift = IsOWordAligned && IsBTI ? Log2_32(OWordBytes) : 0;
    auto *Base = Shift == 0 ? Addr : Builder.CreateLShr(Addr, Shift);

    for (auto OWords : {8, 4, 2, 1}) {
      auto BlockBytes = OWords * OWordBytes;
      if (BlockBytes > Rest)
        continue;

      auto BlockNElements = BlockBytes / ESize;
      auto *BlockVTy = IGCLLVM::FixedVectorType::get(ETy, BlockNElements);

      for (; Rest >= BlockBytes; Rest -= BlockBytes) {
        auto Offset = VSize - Rest;
        auto *BlockAddr = Base;
        if (Offset != 0) {
          auto MemOffset = Shift == 0 ? Offset : Offset >> Shift;
          BlockAddr =
              Builder.CreateAdd(Base, ConstantInt::get(AddrTy, MemOffset));
        }

        if (IsLoad) {
          auto *Load = createLegacyBlockLoadImpl(Builder, M, LoadIID, BTI,
                                                 BlockVTy, BlockAddr);
          Result =
              createInsertDataIntoVectorImpl(Builder, M, Result, Load, Offset);
        } else {
          auto *BlockData = createExtractDataFromVectorImpl(
              Builder, M, BlockVTy, Data, Offset);
          Result =
              createLegacyBlockStoreImpl(Builder, M, BTI, BlockAddr, BlockData);
        }
      }
    }
  }

  // Generate a gather/scatter message
  if (Rest != 0) {
    IGC_ASSERT(Rest % ESize == 0);
    auto RestNElements = Rest / ESize;

    bool SplitQWords = IsBTI && ESize == QWordBytes;
    unsigned SizeFactor = SplitQWords ? 2 : 1;

    auto *RestVTy = IGCLLVM::FixedVectorType::get(ETy, RestNElements);
    auto *MessageVTy = RestVTy;

    if (ESize < DWordBytes || SplitQWords)
      MessageVTy = IGCLLVM::FixedVectorType::get(Builder.getIntNTy(DWordBits),
                                                 RestNElements * SizeFactor);

    unsigned Offset = VSize - Rest;

    SmallVector<Constant *, 16> Offsets;
    Offsets.reserve(RestNElements * SizeFactor);
    std::generate_n(
        std::back_inserter(Offsets), RestNElements * SizeFactor,
        [AddrTy = Addr->getType(), Offset, ESize, SizeFactor]() mutable {
          auto *C = ConstantInt::get(AddrTy, Offset);
          Offset += ESize / SizeFactor;
          return C;
        });

    auto *COffsets = ConstantVector::get(Offsets);
    auto *Pred = IGCLLVM::ConstantFixedVector::getSplat(
        RestNElements * SizeFactor, Builder.getTrue());

    if (IsLoad) {
      auto *Load =
          createLegacyGatherLoadImpl(Builder, M, BTI, ESize / SizeFactor,
                                     MessageVTy, Pred, Addr, COffsets);
      auto *Cast = SplitQWords ? Builder.CreateBitCast(Load, RestVTy)
                               : createTruncateImpl(Builder, RestVTy, Load);
      Result = createInsertDataIntoVectorImpl(Builder, M, Result, Cast, Offset);
    } else {
      auto *Source =
          createExtractDataFromVectorImpl(Builder, M, RestVTy, Data, Offset);
      auto *Extend = createExtendImpl(Builder, Source);
      auto *Cast = Builder.CreateBitCast(Extend, MessageVTy);
      Result = createLegacyScatterStoreImpl(Builder, M, BTI, ESize / SizeFactor,
                                            Pred, Addr, COffsets, Cast);
    }
  }

  if (IsLoad) {
    if (Ty->isPtrOrPtrVectorTy())
      Result = Builder.CreateIntToPtr(Result, &vc::getVectorType(*Ty));
    Result = Builder.CreateBitCast(Result, Ty);

    if (OrigTy->getScalarType()->isIntegerTy(1))
      Result = extractBoolValue(Builder, M, Result, OrigTy);
  }

  return cast<Instruction>(Result);
}

Instruction *
GenXLoadStoreLowering::createLegacyGatherScatter(IntrinsicInst &I,
                                                 unsigned BTI) const {
  auto [IsLoad, Mask, Ptr, Data, Align] = getGatherScatterOperands(I);
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto *Ty = IsLoad ? I.getType() : Data->getType();
  auto *VTy = &vc::getVectorType(*Ty);

  if (Ty->isPtrOrPtrVectorTy()) {
    auto *IntPtrTy = DL_->getIntPtrType(Ty);
    VTy = &vc::getVectorType(*IntPtrTy);
    Data = Builder.CreatePtrToInt(Data, IntPtrTy);
  }

  auto *ETy = VTy->getElementType();
  auto ESize = DL_->getTypeSizeInBits(ETy);

  auto *Extend = createExtendImpl(Builder, Data);
  auto *ExtendTy = cast<IGCLLVM::FixedVectorType>(Extend->getType());

  bool IsBTI = BTI <= visa::RSI_Stateless;
  bool SplitQWords = IsBTI && ESize == QWordBits;

  auto *AddrTy = IsBTI ? Builder.getInt32Ty() : Builder.getInt64Ty();
  auto *Addr = Builder.CreatePtrToInt(
      Ptr, IGCLLVM::FixedVectorType::get(AddrTy, VTy->getNumElements()));
  auto *Base = ConstantInt::get(AddrTy, 0);

  if (SplitQWords) {
    auto *Res = createLegacyGatherScatterQWordImpl(Builder, M, BTI, IsLoad,
                                                   Mask, Addr, Data, Align);
    if (IsLoad && Ty->isPtrOrPtrVectorTy())
      Res = Builder.CreateIntToPtr(Res, Ty);
    return cast<Instruction>(Res);
  }

  if (IsLoad) {
    auto *Load =
        createLegacyGatherLoadImpl(Builder, M, BTI, ESize / ByteBits, ExtendTy,
                                   Mask, Base, Addr, Extend, Align);
    auto *Res = createTruncateImpl(Builder, VTy, Load);
    if (Ty->isPtrOrPtrVectorTy())
      Res = Builder.CreateIntToPtr(Res, Ty);
    return cast<Instruction>(Res);
  }

  return createLegacyScatterStoreImpl(Builder, M, BTI, ESize / ByteBits, Mask,
                                      Base, Addr, Extend, Align);
}

template <typename MemoryInstT>
Instruction *
GenXLoadStoreLowering::createMemoryInstReplacement(MemoryInstT &I) const {
  auto *Replacement = switchAtomicity(I);
  if (Replacement)
    Replacement->takeName(&I);
  return Replacement;
}

template <typename MemoryInstT>
Instruction *GenXLoadStoreLowering::switchAtomicity(MemoryInstT &I) const {
  if (I.isAtomic())
    return switchMessage<Atomicity::Atomic>(I);
  return switchMessage<Atomicity::NonAtomic>(I);
}

template <>
Instruction *
GenXLoadStoreLowering::switchAtomicity<IntrinsicInst>(IntrinsicInst &I) const {
  if (isSPIRVAtomic(&I))
    return switchMessage<Atomicity::Atomic>(I);
  return switchMessage<Atomicity::NonAtomic>(I);
}

template <Atomicity A, typename MemoryInstT>
Instruction *GenXLoadStoreLowering::switchMessage(MemoryInstT &I) const {
  if (ST->hasLSCMessages())
    return switchAddrSpace<MessageKind::LSC, A>(I);
  return switchAddrSpace<MessageKind::Legacy, A>(I);
}

template <MessageKind MK, Atomicity A, typename MemoryInstT>
Instruction *GenXLoadStoreLowering::switchAddrSpace(MemoryInstT &I,
                                                    PointerType *PtrTy) const {
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

template <MessageKind MK, Atomicity A, typename MemoryInstT>
Instruction *GenXLoadStoreLowering::switchAddrSpace(MemoryInstT &I) const {
  auto *PtrTy = cast<PointerType>(I.getPointerOperand()->getType());
  return switchAddrSpace<MK, A>(I, PtrTy);
}

template <MessageKind MK, Atomicity A>
Instruction *GenXLoadStoreLowering::switchAddrSpace(IntrinsicInst &I) const {
  unsigned ID = vc::getAnyIntrinsicID(&I);
  unsigned PointerOperandNum = 0;
  switch (ID) {
  default:
    IGC_ASSERT_MESSAGE(0, "unsupported intrinsic");
    return &I;
  case Intrinsic::masked_gather:
    PointerOperandNum = 0;
    break;
  case Intrinsic::masked_scatter:
    PointerOperandNum = 1;
    break;
  case vc::InternalIntrinsic::atomic_fmin:
  case vc::InternalIntrinsic::atomic_fmax:
    PointerOperandNum = SPIRVAtomicOp::PointerOp;
    break;
  }

  auto *Ptr = I.getArgOperand(PointerOperandNum);
  auto *PtrTy = Ptr->getType();
  if (auto *PtrVTy = dyn_cast<IGCLLVM::FixedVectorType>(PtrTy))
    PtrTy = PtrVTy->getElementType();
  return switchAddrSpace<MK, A>(I, cast<PointerType>(PtrTy));
}

template <MessageKind MK, Atomicity A>
Instruction *GenXLoadStoreLowering::switchAddrSpace(FenceInst &I) const {
  return createIntrinsic<MK>(I);
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
  return createLegacyLoadStore(I, -1, I.getPointerOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::Legacy,
                                       Atomicity::NonAtomic, LoadInst>(
    LoadInst &I) const {
  return createLegacyLoadStore(I, visa::RSI_Stateless, I.getPointerOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::Legacy,
                                       Atomicity::NonAtomic, LoadInst>(
    LoadInst &I) const {
  return createLegacyLoadStore(I, visa::RSI_Slm, I.getPointerOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  return createLegacyLoadStore(I, -1, I.getPointerOperand(),
                               I.getValueOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::Legacy,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  return createLegacyLoadStore(I, visa::RSI_Stateless, I.getPointerOperand(),
                               I.getValueOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::Legacy,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  return createLegacyLoadStore(I, visa::RSI_Slm, I.getPointerOperand(),
                               I.getValueOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::Atomic, LoadInst>(
    LoadInst &I) const {
  return createLegacyAtomicLoad(I, -1);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::Legacy,
                                       Atomicity::Atomic, LoadInst>(
    LoadInst &I) const {
  return createLegacyAtomicLoad(I, visa::RSI_Stateless);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::Legacy,
                                       Atomicity::Atomic, LoadInst>(
    LoadInst &I) const {
  return createLegacyAtomicLoad(I, visa::RSI_Slm);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::Atomic, StoreInst>(
    StoreInst &I) const {
  return createLegacyAtomicStore(I, -1);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::Legacy,
                                       Atomicity::Atomic, StoreInst>(
    StoreInst &I) const {
  return createLegacyAtomicStore(I, visa::RSI_Stateless);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::Legacy,
                                       Atomicity::Atomic, StoreInst>(
    StoreInst &I) const {
  return createLegacyAtomicStore(I, visa::RSI_Slm);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::Atomic, AtomicRMWInst>(
    AtomicRMWInst &I) const {
  return createLegacyAtomicRMW(I, -1);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::Legacy,
                                       Atomicity::Atomic, AtomicRMWInst>(
    AtomicRMWInst &I) const {
  return createLegacyAtomicRMW(I, visa::RSI_Stateless);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::Legacy,
                                       Atomicity::Atomic, AtomicRMWInst>(
    AtomicRMWInst &I) const {
  return createLegacyAtomicRMW(I, visa::RSI_Slm);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::Atomic, AtomicCmpXchgInst>(
    AtomicCmpXchgInst &I) const {
  return createLegacyAtomicCmpXchg(I, -1);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::Legacy,
                                       Atomicity::Atomic, AtomicCmpXchgInst>(
    AtomicCmpXchgInst &I) const {
  return createLegacyAtomicCmpXchg(I, visa::RSI_Stateless);
}
template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::Legacy,
                                       Atomicity::Atomic, AtomicCmpXchgInst>(
    AtomicCmpXchgInst &I) const {
  return createLegacyAtomicCmpXchg(I, visa::RSI_Slm);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::NonAtomic, IntrinsicInst>(
    IntrinsicInst &I) const {
  return createLegacyGatherScatter(I, -1);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::Legacy,
                                       Atomicity::NonAtomic, IntrinsicInst>(
    IntrinsicInst &I) const {
  return createLegacyGatherScatter(I, visa::RSI_Stateless);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::Legacy,
                                       Atomicity::NonAtomic, IntrinsicInst>(
    IntrinsicInst &I) const {
  return createLegacyGatherScatter(I, visa::RSI_Slm);
}

template <>
Instruction *GenXLoadStoreLowering::createIntrinsic<MessageKind::Legacy>(
    FenceInst &I) const {
  return createLegacyStandAloneFence(I);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::LSC,
                                       Atomicity::NonAtomic, LoadInst>(
    LoadInst &I) const {
  IRBuilder<> Builder(&I);
  Type *AddrTy = Builder.getInt64Ty();
  return createLSCLoadStore(
      I, vc::InternalIntrinsic::lsc_load_ugm, Builder.getInt64(0),
      Builder.CreatePtrToInt(I.getPointerOperand(), AddrTy));
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::LSC,
                                       Atomicity::NonAtomic, LoadInst>(
    LoadInst &I) const {
  IRBuilder<> Builder(&I);
  Type *AddrTy = Builder.getInt32Ty();
  return createLSCLoadStore(
      I, vc::InternalIntrinsic::lsc_load_bti,
      Builder.getInt32(visa::RSI_Stateless),
      Builder.CreatePtrToInt(I.getPointerOperand(), AddrTy));
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::LSC,
                                       Atomicity::NonAtomic, LoadInst>(
    LoadInst &I) const {
  IRBuilder<> Builder(&I);
  Type *AddrTy = Builder.getInt32Ty();
  return createLSCLoadStore(
      I, vc::InternalIntrinsic::lsc_load_slm, Builder.getInt32(0),
      Builder.CreatePtrToInt(I.getPointerOperand(), AddrTy));
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::LSC,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  IRBuilder<> Builder(&I);
  Type *AddrTy = Builder.getInt64Ty();
  return createLSCLoadStore(
      I, vc::InternalIntrinsic::lsc_store_ugm, Builder.getInt64(0),
      Builder.CreatePtrToInt(I.getPointerOperand(), AddrTy),
      I.getValueOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::LSC,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  IRBuilder<> Builder(&I);
  Type *AddrTy = Builder.getInt32Ty();
  return createLSCLoadStore(
      I, vc::InternalIntrinsic::lsc_store_bti,
      Builder.getInt32(visa::RSI_Stateless),
      Builder.CreatePtrToInt(I.getPointerOperand(), AddrTy),
      I.getValueOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::LSC,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  IRBuilder<> Builder(&I);
  Type *AddrTy = Builder.getInt32Ty();
  return createLSCLoadStore(
      I, vc::InternalIntrinsic::lsc_store_slm, Builder.getInt32(0),
      Builder.CreatePtrToInt(I.getPointerOperand(), AddrTy),
      I.getValueOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::LSC,
                                       Atomicity::NonAtomic, IntrinsicInst>(
    IntrinsicInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt64(0);
  auto *AddrTy = Builder.getInt64Ty();
  return createLSCGatherScatter(I, vc::InternalIntrinsic::lsc_load_ugm,
                                vc::InternalIntrinsic::lsc_store_ugm, Base,
                                AddrTy);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::LSC,
                                       Atomicity::NonAtomic, IntrinsicInst>(
    IntrinsicInst &I) const {
  IRBuilder<> Builder(&I);
  auto *BTI = Builder.getInt32(visa::RSI_Stateless);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCGatherScatter(I, vc::InternalIntrinsic::lsc_load_bti,
                                vc::InternalIntrinsic::lsc_store_bti, BTI,
                                AddrTy);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::LSC,
                                       Atomicity::NonAtomic, IntrinsicInst>(
    IntrinsicInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt32(0);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCGatherScatter(I, vc::InternalIntrinsic::lsc_load_slm,
                                vc::InternalIntrinsic::lsc_store_slm, Base,
                                AddrTy);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::LSC,
                                       Atomicity::Atomic, IntrinsicInst>(
    IntrinsicInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt64(0);
  auto *AddrTy = Builder.getInt64Ty();
  return createLSCAtomicRMW(I, vc::InternalIntrinsic::lsc_atomic_ugm, AddrTy,
                            Base);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::LSC,
                                       Atomicity::Atomic, IntrinsicInst>(
    IntrinsicInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt32(0);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCAtomicRMW(I, vc::InternalIntrinsic::lsc_atomic_slm, AddrTy,
                            Base);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::LSC,
                                       Atomicity::Atomic, LoadInst>(
    LoadInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt64(0);
  auto *AddrTy = Builder.getInt64Ty();
  return createLSCAtomicLoad(I, vc::InternalIntrinsic::lsc_atomic_ugm, AddrTy,
                             Base);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::LSC,
                                       Atomicity::Atomic, LoadInst>(
    LoadInst &I) const {
  IRBuilder<> Builder(&I);
  auto *BTI = Builder.getInt32(visa::RSI_Stateless);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCAtomicLoad(I, vc::InternalIntrinsic::lsc_atomic_bti, AddrTy,
                             BTI);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::LSC,
                                       Atomicity::Atomic, LoadInst>(
    LoadInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt32(0);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCAtomicLoad(I, vc::InternalIntrinsic::lsc_atomic_slm, AddrTy,
                             Base);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::LSC,
                                       Atomicity::Atomic, StoreInst>(
    StoreInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt64(0);
  auto *AddrTy = Builder.getInt64Ty();
  return createLSCAtomicStore(I, vc::InternalIntrinsic::lsc_atomic_ugm, AddrTy,
                              Base);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::LSC,
                                       Atomicity::Atomic, StoreInst>(
    StoreInst &I) const {
  IRBuilder<> Builder(&I);
  auto *BTI = Builder.getInt32(visa::RSI_Stateless);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCAtomicStore(I, vc::InternalIntrinsic::lsc_atomic_bti, AddrTy,
                              BTI);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::LSC,
                                       Atomicity::Atomic, StoreInst>(
    StoreInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt32(0);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCAtomicStore(I, vc::InternalIntrinsic::lsc_atomic_slm, AddrTy,
                              Base);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::LSC,
                                       Atomicity::Atomic, AtomicRMWInst>(
    AtomicRMWInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt64(0);
  auto *AddrTy = Builder.getInt64Ty();
  return createLSCAtomicRMW(I, vc::InternalIntrinsic::lsc_atomic_ugm, AddrTy,
                            Base);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::LSC,
                                       Atomicity::Atomic, AtomicRMWInst>(
    AtomicRMWInst &I) const {
  IRBuilder<> Builder(&I);
  auto *BTI = Builder.getInt32(visa::RSI_Stateless);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCAtomicRMW(I, vc::InternalIntrinsic::lsc_atomic_bti, AddrTy,
                            BTI);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::LSC,
                                       Atomicity::Atomic, AtomicRMWInst>(
    AtomicRMWInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt32(0);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCAtomicRMW(I, vc::InternalIntrinsic::lsc_atomic_slm, AddrTy,
                            Base);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::LSC,
                                       Atomicity::Atomic, AtomicCmpXchgInst>(
    AtomicCmpXchgInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt64(0);
  auto *AddrTy = Builder.getInt64Ty();
  return createLSCAtomicCmpXchg(I, vc::InternalIntrinsic::lsc_atomic_ugm,
                                AddrTy, Base);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::LSC,
                                       Atomicity::Atomic, AtomicCmpXchgInst>(
    AtomicCmpXchgInst &I) const {
  IRBuilder<> Builder(&I);
  auto *BTI = Builder.getInt32(visa::RSI_Stateless);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCAtomicCmpXchg(I, vc::InternalIntrinsic::lsc_atomic_bti,
                                AddrTy, BTI);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::LSC,
                                       Atomicity::Atomic, AtomicCmpXchgInst>(
    AtomicCmpXchgInst &I) const {
  IRBuilder<> Builder(&I);
  auto *Base = Builder.getInt32(0);
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCAtomicCmpXchg(I, vc::InternalIntrinsic::lsc_atomic_slm,
                                AddrTy, Base);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<MessageKind::LSC>(FenceInst &I) const {
  IRBuilder<> Builder(&I);
  return createLSCStandAloneFence(I);
}
