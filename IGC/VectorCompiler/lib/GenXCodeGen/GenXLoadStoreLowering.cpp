/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

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
  A32, // Global memory, addressed with 32-bit pointers.
  A64, // Global memory, addressed with 64-bit pointers.
  SLM, // Shared local memory.
};

constexpr const char AlignMDName[] = "VCAlignment";

// load and store lowering pass
class GenXLoadStoreLowering : public FunctionPass,
                              public InstVisitor<GenXLoadStoreLowering> {
  const DataLayout *DL_ = nullptr;
  const GenXSubtarget *ST = nullptr;

private:
  IGCLLVM::FixedVectorType *getLoadVType(const LoadInst &LdI) const;
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
  Instruction *switchAddrSpace(MemoryInstT &I, PointerType *PtrTy) const;
  template <MessageKind MK, Atomicity A, typename MemoryInstT>
  Instruction *switchAddrSpace(MemoryInstT &I) const;
  template <MessageKind MK, Atomicity A>
  Instruction *switchAddrSpace(IntrinsicInst &I) const;

  // Creates a replacement for \p I instruction. The template parameters
  // describe the provided instruction and how it should be lowered.
  template <HWAddrSpace HWAS, MessageKind MK, Atomicity A, typename MemoryInstT>
  Instruction *createIntrinsic(MemoryInstT &I) const;

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

  Instruction *createLSCLoadStore(Instruction &I, GenXIntrinsic::ID IID,
                                  Value *BTI, Value *Addr,
                                  Value *Data = nullptr) const;
  Instruction *createLSCGatherScatter(IntrinsicInst &I,
                                      GenXIntrinsic::ID LoadIID,
                                      GenXIntrinsic::ID StoreIID, Value *BTI,
                                      Type *AddrTy) const;
  Instruction *createLSCLoadImpl(IRBuilder<> &Builder, Module *M,
                                 GenXIntrinsic::ID IID, unsigned ESize,
                                 IGCLLVM::FixedVectorType *Ty, Value *Pred,
                                 Value *BTI, Value *Addr,
                                 Value *Source = nullptr,
                                 ConstantInt *Align = nullptr) const;
  Instruction *createLSCStoreImpl(IRBuilder<> &Builder, Module *M,
                                  GenXIntrinsic::ID IID, unsigned ESize,
                                  Value *BTI, Value *Pred, Value *Addr,
                                  Value *Data,
                                  ConstantInt *Align = nullptr) const;

  Instruction *createLegacySVMAtomicLoad(LoadInst &LdI) const;
  Instruction *createLegacySVMAtomicStore(StoreInst &StI) const;
  Instruction *createLegacySVMAtomicBinOp(AtomicRMWInst &I) const;
  Instruction *createLegacySVMAtomicCmpXchg(AtomicCmpXchgInst &CmpXchgI) const;

  Value *createExtractDataFromVectorImpl(IRBuilder<> &Builder, Module *M,
                                         IGCLLVM::FixedVectorType *Ty,
                                         Value *Data, unsigned Offset) const;
  Value *createInsertDataIntoVectorImpl(IRBuilder<> &Builder, Module *M,
                                        Value *Target, Value *Data,
                                        unsigned Offset) const;
  Value *createExtendImpl(IRBuilder<> &Builder, Value *Data) const;
  Value *createTruncateImpl(IRBuilder<> &Builder, IGCLLVM::FixedVectorType *Ty,
                            Value *Data) const;

  static unsigned getLSCBlockElementSizeBits(unsigned DataSizeBytes,
                                             unsigned Align);
  static LSC_DATA_SIZE getLSCElementSize(unsigned Bits);
  static LSC_DATA_ELEMS getLSCElementsPerAddress(unsigned N);

public:
  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &Inst) const;
  void visitAtomicRMWInst(AtomicRMWInst &Inst) const;
  void visitIntrinsicInst(IntrinsicInst &Intrinsic) const;
  void visitLoadInst(LoadInst &LdI) const;
  void visitStoreInst(StoreInst &StI) const;

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
  ST = &getAnalysis<TargetPassConfig>()
                 .getTM<GenXTargetMachine>()
                 .getGenXSubtarget();
  IGC_ASSERT(ST);
  // auto &BEConf = getAnalysis<GenXBackendConfig>();
  // BEConf.getStatelessPrivateMemSize() will be required

  // see visitXXInst members for main logic:
  //   * visitAtomicCmpXchgInst
  //   * visitAtomicRMWInst
  //   * visitIntrinsicInst
  //   * visitLoadInst
  //   * visitStoreInst
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

Instruction *GenXLoadStoreLowering::createLSCLoadImpl(
    IRBuilder<> &Builder, Module *M, GenXIntrinsic::ID IID, unsigned ESize,
    IGCLLVM::FixedVectorType *Ty, Value *Pred, Value *BTI, Value *Addr,
    Value *Source, ConstantInt *Align) const {
  IGC_ASSERT_EXIT(
      IID == GenXIntrinsic::genx_lsc_load_stateless ||
      IID == GenXIntrinsic::genx_lsc_load_slm ||
      IID == GenXIntrinsic::genx_lsc_load_bti ||
      (Source && (IID == GenXIntrinsic::genx_lsc_load_merge_stateless ||
                  IID == GenXIntrinsic::genx_lsc_load_merge_slm ||
                  IID == GenXIntrinsic::genx_lsc_load_merge_bti)));
  IGC_ASSERT_EXIT(Ty);
  IGC_ASSERT_EXIT(Pred);
  IGC_ASSERT_EXIT(BTI);
  IGC_ASSERT_EXIT(Addr);

  auto *AddrTy = Addr->getType();
  auto IsBlock = !isa<IGCLLVM::FixedVectorType>(AddrTy);
  auto NElements = Ty->getNumElements();

  if (IsBlock)
    IGC_ASSERT_EXIT(ESize == QWordBits || ESize == DWordBits);

  auto ElementSize = getLSCElementSize(ESize);
  auto ElementsPerAddress = getLSCElementsPerAddress(IsBlock ? NElements : 1);
  auto Transpose = IsBlock && NElements > 1 ? LSC_DATA_ORDER_TRANSPOSE
                                            : LSC_DATA_ORDER_NONTRANSPOSE;

  SmallVector<Value *, 13> Args = {
      Pred,
      Builder.getInt8(LSC_LOAD),           // Subopcode
      Builder.getInt8(0),                  // L1 hint (default)
      Builder.getInt8(0),                  // L3 hint (default)
      Builder.getInt16(1),                 // Address scale
      Builder.getInt32(0),                 // Address offset
      Builder.getInt8(ElementSize),        // Element size
      Builder.getInt8(ElementsPerAddress), // Elements per address
      Builder.getInt8(Transpose), // Transposed (block) or gather operation
      Builder.getInt8(0),         // Channel mask, ignored
      Addr,
      BTI,
  };

  if (Source)
    Args.push_back(Source);

  auto *Func = GenXIntrinsic::getGenXDeclaration(
      M, IID, {Ty, Pred->getType(), Addr->getType()});
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
    IRBuilder<> &Builder, Module *M, GenXIntrinsic::ID IID, unsigned ESize,
    Value *Pred, Value *BTI, Value *Addr, Value *Data,
    ConstantInt *Align) const {
  IGC_ASSERT_EXIT(IID == GenXIntrinsic::genx_lsc_store_stateless ||
                  IID == GenXIntrinsic::genx_lsc_store_bti ||
                  IID == GenXIntrinsic::genx_lsc_store_slm);
  IGC_ASSERT_EXIT(Pred);
  IGC_ASSERT_EXIT(BTI);
  IGC_ASSERT_EXIT(Addr);
  IGC_ASSERT_EXIT(Data);

  auto *Ty = cast<IGCLLVM::FixedVectorType>(Data->getType());
  auto *AddrTy = Addr->getType();
  auto IsBlock = !isa<IGCLLVM::FixedVectorType>(AddrTy);
  auto NElements = Ty->getNumElements();

  if (IsBlock)
    IGC_ASSERT_EXIT(ESize == QWordBits || ESize == DWordBits);

  auto ElementSize = getLSCElementSize(ESize);
  auto ElementsPerAddress = getLSCElementsPerAddress(IsBlock ? NElements : 1);
  auto Transpose = IsBlock && NElements > 1 ? LSC_DATA_ORDER_TRANSPOSE
                                            : LSC_DATA_ORDER_NONTRANSPOSE;

  SmallVector<Value *, 13> Args = {
      Pred,
      Builder.getInt8(LSC_STORE),          // Subopcode
      Builder.getInt8(0),                  // L1 hint (default)
      Builder.getInt8(0),                  // L3 hint (default)
      Builder.getInt16(1),                 // Address scale
      Builder.getInt32(0),                 // Address offset
      Builder.getInt8(ElementSize),        // Element size
      Builder.getInt8(ElementsPerAddress), // Elements per address
      Builder.getInt8(Transpose), // Transposed (block) or scatter operation
      Builder.getInt8(0),         // Channel mask, ignored
      Addr,
      Data,
      BTI,
  };

  auto *Func = GenXIntrinsic::getGenXDeclaration(
      M, IID, {Pred->getType(), Addr->getType(), Ty});
  auto *Store = Builder.CreateCall(Func, Args);

  if (Align) {
    auto &Ctx = Store->getContext();
    auto *MD = ConstantAsMetadata::get(Align);
    Store->setMetadata(AlignMDName, MDNode::get(Ctx, MD));
  }

  LLVM_DEBUG(dbgs() << "Created: " << *Store << "\n");
  return Store;
}

Instruction *GenXLoadStoreLowering::createLSCLoadStore(Instruction &I,
                                                       GenXIntrinsic::ID IID,
                                                       Value *BTI, Value *Addr,
                                                       Value *Data) const {
  LLVM_DEBUG(dbgs() << "Lowering: " << I << "\n");
  IGC_ASSERT_EXIT(BTI);
  IGC_ASSERT_EXIT(Addr);
  IGC_ASSERT_EXIT(isa<LoadInst>(I) || (isa<StoreInst>(I) && Data));

  IRBuilder<> Builder(&I);
  Module *M = I.getModule();
  bool IsLoad = isa<LoadInst>(I);

  auto *Ty = IsLoad ? I.getType() : Data->getType();
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
          auto *Load = createLSCLoadImpl(Builder, M, IID, BlockESizeBits,
                                         BlockVTy, Pred, BTI, BlockAddr);
          Result =
              createInsertDataIntoVectorImpl(Builder, M, Result, Load, Offset);
        } else {
          auto *Block = createExtractDataFromVectorImpl(Builder, M, BlockVTy,
                                                        Data, Offset);
          Result = createLSCStoreImpl(Builder, M, IID, BlockESizeBits, Pred,
                                      BTI, BlockAddr, Block);
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
      auto *Load = createLSCLoadImpl(Builder, M, IID, ESize * ByteBits,
                                     GatherVTy, Pred, BTI, VAddr);
      auto *Trunc = createTruncateImpl(Builder, RestVTy, Load);
      Result =
          createInsertDataIntoVectorImpl(Builder, M, Result, Trunc, Offset);
    } else {
      auto *Source =
          createExtractDataFromVectorImpl(Builder, M, RestVTy, Data, Offset);
      auto *Extend = createExtendImpl(Builder, Source);
      Result = createLSCStoreImpl(Builder, M, IID, ESize * ByteBits, Pred, BTI,
                                  VAddr, Extend);
    }
  }

  if (IsLoad) {
    if (Ty->isPtrOrPtrVectorTy())
      Result = Builder.CreateIntToPtr(Result, &vc::getVectorType(*Ty));
    Result = Builder.CreateBitCast(Result, Ty);
  }

  return cast<Instruction>(Result);
}

Instruction *GenXLoadStoreLowering::createLSCGatherScatter(
    IntrinsicInst &I, GenXIntrinsic::ID LoadIID, GenXIntrinsic::ID StoreIID,
    Value *BTI, Type *AddrTy) const {
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
  auto *ExtendTy = cast<IGCLLVM::FixedVectorType>(Extend->getType());

  auto *Addr = Builder.CreatePtrToInt(
      Ptr, IGCLLVM::FixedVectorType::get(AddrTy, VTy->getNumElements()));

  if (IsLoad) {
    auto *Load = createLSCLoadImpl(Builder, M, LoadIID, ESize, ExtendTy, Mask,
                                   BTI, Addr, Extend, Align);
    auto *Res = createTruncateImpl(Builder, VTy, Load);
    if (Ty->isPtrOrPtrVectorTy())
      Res = Builder.CreateIntToPtr(Res, Ty);
    return cast<Instruction>(Res);
  }

  return createLSCStoreImpl(Builder, M, StoreIID, ESize, Mask, BTI, Addr,
                            Extend, Align);
}

// we need vector type that emulates what real load type
// will be for gather/scatter
IGCLLVM::FixedVectorType *
GenXLoadStoreLowering::getLoadVType(const LoadInst &LdI) const {
  Type *LdTy = LdI.getType();
  if (LdTy->isIntOrPtrTy() || LdTy->isFloatingPointTy())
    LdTy = IGCLLVM::FixedVectorType::get(LdTy, 1);

  if (!LdTy->isVectorTy())
    vc::fatal(LdI.getContext(), "LDS", "Unsupported type inside replaceLoad",
              LdTy);
  return cast<IGCLLVM::FixedVectorType>(LdTy);
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
  bool IsBlockAllowed = BTI == visa::RSI_Slm ? ST->hasSLMOWord() : true;

  bool IsOWordAligned = Align >= OWordBytes;
  bool IsDWordAligned = Align >= DWordBytes;

  // Try to generate OWord block load/store
  if (IsBlockAllowed && (IsOWordAligned || (IsLoad && IsDWordAligned))) {
    auto LoadIID = IsOWordAligned
                       ? (IsBTI ? GenXIntrinsic::genx_oword_ld
                                : GenXIntrinsic::genx_svm_block_ld)
                       : (IsBTI ? GenXIntrinsic::genx_oword_ld_unaligned
                                : GenXIntrinsic::genx_svm_block_ld_unaligned);

    for (auto OWords : {8, 4, 2, 1}) {
      auto BlockBytes = OWords * OWordBytes;
      if (BlockBytes > Rest)
        continue;

      auto BlockNElements = BlockBytes / ESize;
      auto *BlockVTy = IGCLLVM::FixedVectorType::get(ETy, BlockNElements);

      for (; Rest >= BlockBytes; Rest -= BlockBytes) {
        auto Offset = VSize - Rest;
        auto *BlockAddr =
            Offset == 0
                ? Addr
                : Builder.CreateAdd(Addr, ConstantInt::get(AddrTy, Offset));

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
  if (ST->hasLSCMessages() && !I.isAtomic())
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
  }

  auto *Ptr = I.getArgOperand(PointerOperandNum);
  auto *PtrVTy = cast<IGCLLVM::FixedVectorType>(Ptr->getType());
  auto *PtrTy = cast<PointerType>(PtrVTy->getElementType());
  return switchAddrSpace<MK, A>(I, PtrTy);
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
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  return createLegacyLoadStore(I, -1, I.getPointerOperand(),
                               I.getValueOperand());
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
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A32, MessageKind::Legacy,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  return createLegacyLoadStore(I, visa::RSI_Stateless, I.getPointerOperand(),
                               I.getValueOperand());
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
  return createLegacyLoadStore(I, visa::RSI_Slm, I.getPointerOperand());
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
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::LSC,
                                       Atomicity::NonAtomic, LoadInst>(
    LoadInst &I) const {
  IRBuilder<> Builder(&I);
  Type *AddrTy = Builder.getInt64Ty();
  return createLSCLoadStore(
      I, GenXIntrinsic::genx_lsc_load_stateless, Builder.getInt32(0),
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
      I, GenXIntrinsic::genx_lsc_load_bti,
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
      I, GenXIntrinsic::genx_lsc_load_slm, Builder.getInt32(0),
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
      I, GenXIntrinsic::genx_lsc_store_stateless, Builder.getInt32(0),
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
      I, GenXIntrinsic::genx_lsc_store_bti,
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
      I, GenXIntrinsic::genx_lsc_store_slm, Builder.getInt32(0),
      Builder.CreatePtrToInt(I.getPointerOperand(), AddrTy),
      I.getValueOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::LSC,
                                       Atomicity::NonAtomic, IntrinsicInst>(
    IntrinsicInst &I) const {
  IRBuilder<> Builder(&I);
  auto *BTI = Builder.getInt32(0); // ignored
  auto *AddrTy = Builder.getInt64Ty();
  return createLSCGatherScatter(I, GenXIntrinsic::genx_lsc_load_merge_stateless,
                                GenXIntrinsic::genx_lsc_store_stateless, BTI,
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
  return createLSCGatherScatter(I, GenXIntrinsic::genx_lsc_load_merge_bti,
                                GenXIntrinsic::genx_lsc_store_bti, BTI, AddrTy);
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::SLM, MessageKind::LSC,
                                       Atomicity::NonAtomic, IntrinsicInst>(
    IntrinsicInst &I) const {
  IRBuilder<> Builder(&I);
  auto *BTI = Builder.getInt32(0); // ignored
  auto *AddrTy = Builder.getInt32Ty();
  return createLSCGatherScatter(I, GenXIntrinsic::genx_lsc_load_merge_slm,
                                GenXIntrinsic::genx_lsc_store_slm, BTI, AddrTy);
}
