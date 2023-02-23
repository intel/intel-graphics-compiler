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

// load and store lowering pass
class GenXLoadStoreLowering : public FunctionPass,
                              public InstVisitor<GenXLoadStoreLowering> {
  const DataLayout *DL_ = nullptr;
  const GenXSubtarget *ST = nullptr;

private:
  Value *ZExtOrTruncIfNeeded(Value *From, Type *To,
                             Instruction *InsertBefore) const;
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

  Instruction *createSVMLoadStore(Instruction &I, Value *Ptr,
                                  Value *Data = nullptr) const;
  Instruction *createSVMBlockLoadImpl(IRBuilder<> &Builder, Module *M,
                                      GenXIntrinsic::ID IID,
                                      IGCLLVM::FixedVectorType *Ty,
                                      Value *Addr) const;
  Instruction *createSVMBlockStoreImpl(IRBuilder<> &Builder, Module *M,
                                       Value *Addr, Value *Data) const;
  Instruction *createSVMGatherLoadImpl(IRBuilder<> &Builder, Module *M,
                                       unsigned ESize,
                                       IGCLLVM::FixedVectorType *Ty,
                                       Value *Pred, Value *Addr) const;
  Instruction *createSVMScatterStoreImpl(IRBuilder<> &Builder, Module *M,
                                         unsigned ESize, Value *Pred,
                                         Value *VAddr, Value *Data) const;

  Instruction *createLSCLoadStore(Instruction &I, GenXIntrinsic::ID IID,
                                  Value *BTI, Value *Addr,
                                  Value *Data = nullptr) const;
  Instruction *createLSCLoadImpl(IRBuilder<> &Builder, Module *M,
                                 GenXIntrinsic::ID IID, unsigned ESize,
                                 IGCLLVM::FixedVectorType *Ty, Value *Pred,
                                 Value *BTI, Value *Addr,
                                 Value *Source = nullptr) const;
  Instruction *createLSCStoreImpl(IRBuilder<> &Builder, Module *M,
                                  GenXIntrinsic::ID IID, unsigned ESize,
                                  Value *BTI, Value *Pred, Value *Addr,
                                  Value *Data) const;

  Instruction *createGatherScaled(LoadInst &OrigLoad,
                                  visa::ReservedSurfaceIndex Surface) const;
  Instruction *createScatterScaled(StoreInst &OrigStore,
                                   visa::ReservedSurfaceIndex Surface) const;

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
    Value *Source) const {
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

  LLVM_DEBUG(dbgs() << "Created: " << *Load << "\n");
  return Load;
}

Instruction *GenXLoadStoreLowering::createLSCStoreImpl(
    IRBuilder<> &Builder, Module *M, GenXIntrinsic::ID IID, unsigned ESize,
    Value *Pred, Value *BTI, Value *Addr, Value *Data) const {
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

  if (IsLoad)
    Result = Builder.CreateBitCast(Result, Ty);
  return cast<Instruction>(Result);
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
    auto *TmpRes =
        Builder.CreateBitOrPointerCast(Res, FromVTy->getElementType(), "");
    Res = TmpRes;
  }
  if (auto *ToVTy = dyn_cast<IGCLLVM::FixedVectorType>(ToTy))
    Res = Builder.CreateVectorSplat(ToVTy->getNumElements(), Res,
                                    Res->getName() + ".splat");
  if (FromTySz < ToTySz)
    Res = Builder.CreateZExtOrBitCast(Res, ToTy, "");
  else if (FromTySz > ToTySz)
    Res = Builder.CreateTruncOrBitCast(Res, ToTy, "");
  return Res;
}

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
    vc::fatal(LdI.getContext(), "LDS", "Unsupported type inside replaceLoad",
              LdTy);
  return cast<IGCLLVM::FixedVectorType>(LdTy);
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

Instruction *GenXLoadStoreLowering::createSVMBlockLoadImpl(
    IRBuilder<> &Builder, Module *M, GenXIntrinsic::ID IID,
    IGCLLVM::FixedVectorType *Ty, Value *Addr) const {
  IGC_ASSERT_EXIT(IID == GenXIntrinsic::genx_svm_block_ld ||
                  IID == GenXIntrinsic::genx_svm_block_ld_unaligned);
  IGC_ASSERT_EXIT(Ty);
  IGC_ASSERT_EXIT(Addr);

  auto *Func = GenXIntrinsic::getGenXDeclaration(M, IID, {Ty, Addr->getType()});
  auto *Load = Builder.CreateCall(Func, {Addr});

  LLVM_DEBUG(dbgs() << "Created: " << *Load << "\n");
  return Load;
}

Instruction *GenXLoadStoreLowering::createSVMGatherLoadImpl(
    IRBuilder<> &Builder, Module *M, unsigned ESize,
    IGCLLVM::FixedVectorType *Ty, Value *Pred, Value *Addr) const {
  IGC_ASSERT_EXIT(Ty);
  IGC_ASSERT_EXIT(Pred);
  IGC_ASSERT_EXIT(Addr);

  constexpr auto IID = GenXIntrinsic::genx_svm_gather;
  auto BlocksNumLog2 = ESize == WordBytes ? 1 : 0;

  auto NElements = Ty->getNumElements();
  auto *RetVTy = Ty;
  if (ESize < DWordBytes)
    RetVTy = IGCLLVM::FixedVectorType::get(Builder.getInt8Ty(),
                                           NElements * DWordBytes);

  SmallVector<Value *, 4> Args = {
      Pred,
      Builder.getInt32(BlocksNumLog2),
      Addr,
      UndefValue::get(RetVTy),
  };

  auto *Func = GenXIntrinsic::getGenXDeclaration(
      M, IID, {RetVTy, Pred->getType(), Addr->getType()});
  auto *Load = Builder.CreateCall(Func, Args);

  LLVM_DEBUG(dbgs() << "Created: " << *Load << "\n");
  return cast<Instruction>(Builder.CreateBitCast(Load, Ty));
}

Instruction *
GenXLoadStoreLowering::createSVMBlockStoreImpl(IRBuilder<> &Builder, Module *M,
                                               Value *Addr, Value *Data) const {
  constexpr auto IID = GenXIntrinsic::genx_svm_block_st;

  auto *Func = GenXIntrinsic::getGenXDeclaration(
      M, IID, {Addr->getType(), Data->getType()});
  auto *Store = Builder.CreateCall(Func, {Addr, Data});

  LLVM_DEBUG(dbgs() << "Created: " << *Store << "\n");
  return Store;
}

Instruction *GenXLoadStoreLowering::createSVMScatterStoreImpl(
    IRBuilder<> &Builder, Module *M, unsigned ESize, Value *Pred, Value *Addr,
    Value *Data) const {
  IGC_ASSERT_EXIT(Pred);
  IGC_ASSERT_EXIT(Addr);
  IGC_ASSERT_EXIT(Data);

  constexpr auto IID = GenXIntrinsic::genx_svm_scatter;
  auto BlocksNumLog2 = ESize == WordBytes ? 1 : 0;

  auto *Ty = cast<IGCLLVM::FixedVectorType>(Data->getType());
  auto NElements = Ty->getNumElements();
  auto *StoreVTy = Ty;
  auto *StoreData = Data;
  if (ESize < DWordBytes) {
    StoreVTy = IGCLLVM::FixedVectorType::get(Builder.getInt8Ty(),
                                             NElements * DWordBytes);
    StoreData = Builder.CreateBitCast(Data, StoreVTy);
  }

  SmallVector<Value *, 4> Args = {
      Pred,
      Builder.getInt32(BlocksNumLog2),
      Addr,
      StoreData,
  };

  auto *Func = GenXIntrinsic::getGenXDeclaration(
      M, IID, {Pred->getType(), Addr->getType(), StoreVTy});
  auto *Store = Builder.CreateCall(Func, Args);

  LLVM_DEBUG(dbgs() << "Created: " << *Store << "\n");
  return Store;
}

Instruction *GenXLoadStoreLowering::createSVMLoadStore(Instruction &I,
                                                       Value *Ptr,
                                                       Value *Data) const {
  LLVM_DEBUG(dbgs() << "Lowering: " << I << "\n");
  IGC_ASSERT(isa<LoadInst>(I) || (isa<StoreInst>(I) && Data));

  IRBuilder<> Builder(&I);
  auto *M = I.getModule();
  bool IsLoad = isa<LoadInst>(I);

  auto *Ty = IsLoad ? I.getType() : Data->getType();
  auto *VTy = &vc::getVectorType(*Ty);
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

  Value *Addr = Builder.CreatePtrToInt(Ptr, Builder.getInt64Ty());
  Value *Result = UndefValue::get(VTy);

  // Try to generate OWord block load/store
  if (Align >= OWordBytes || (IsLoad && Align >= DWordBytes)) {
    auto IID = Align >= OWordBytes ? GenXIntrinsic::genx_svm_block_ld
                                   : GenXIntrinsic::genx_svm_block_ld_unaligned;
    for (auto OWords : {8, 4, 2, 1}) {
      auto BlockBytes = OWords * OWordBytes;
      if (BlockBytes > Rest)
        continue;

      auto BlockNElements = BlockBytes / ESize;
      auto *BlockVTy = IGCLLVM::FixedVectorType::get(ETy, BlockNElements);

      for (; Rest >= BlockBytes; Rest -= BlockBytes) {
        auto Offset = VSize - Rest;
        auto *BlockAddr =
            Offset == 0 ? Addr
                        : Builder.CreateAdd(Addr, Builder.getInt64(Offset));

        if (IsLoad) {
          auto *Load =
              createSVMBlockLoadImpl(Builder, M, IID, BlockVTy, BlockAddr);
          Result =
              createInsertDataIntoVectorImpl(Builder, M, Result, Load, Offset);
        } else {
          auto *BlockData = createExtractDataFromVectorImpl(
              Builder, M, BlockVTy, Data, Offset);
          Result = createSVMBlockStoreImpl(Builder, M, BlockAddr, BlockData);
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
      auto *Load =
          createSVMGatherLoadImpl(Builder, M, ESize, GatherVTy, Pred, VAddr);
      auto *Trunc = createTruncateImpl(Builder, RestVTy, Load);
      Result =
          createInsertDataIntoVectorImpl(Builder, M, Result, Trunc, Offset);
    } else {
      auto *Source =
          createExtractDataFromVectorImpl(Builder, M, RestVTy, Data, Offset);
      auto *Extend = createExtendImpl(Builder, Source);
      Result =
          createSVMScatterStoreImpl(Builder, M, ESize, Pred, VAddr, Extend);
    }
  }

  if (IsLoad)
    Result = Builder.CreateBitCast(Result, Ty);
  return cast<Instruction>(Result);
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
  Value *Result = nullptr;
  switch (Action) {
  case DataTransformation::NoModification:
    // May need to fix degenerate vector type.
    // Note: no cast is created when types match.
    Result = IRB.CreateBitCast(&Gather, &OrigLoadTy, Gather.getName() + ".bc");
    break;
  case DataTransformation::ZextOrTrunc: {
    auto *TruncTy = &vc::getVectorType(OrigLoadTy);
    if (auto *ETy = TruncTy->getElementType(); ETy->isFloatingPointTy()) {
      auto BitWidth = DL.getTypeSizeInBits(ETy);
      auto NElements = TruncTy->getNumElements();
      TruncTy =
          IGCLLVM::FixedVectorType::get(IRB.getIntNTy(BitWidth), NElements);
    }
    auto *Trunc =
        IRB.CreateTrunc(&Gather, TruncTy, Gather.getName() + ".trunc");

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
    vc::fatal(StI.getContext(), "LDS", "Unsupported type inside replaceStore",
              ValueOpTy);
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
    auto *CastTy = OrigVecTy;
    if (auto *ETy = OrigVecTy->getElementType(); ETy->isFloatingPointTy()) {
      auto BitWidth = DL.getTypeSizeInBits(ETy);
      auto NElements = OrigVecTy->getNumElements();
      CastTy =
          IGCLLVM::FixedVectorType::get(Builder.getIntNTy(BitWidth), NElements);
    }
    auto *OrigVecData =
        Builder.CreateBitCast(&OrigData, CastTy, OrigData.getName() + ".bc");
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
  if (ST->hasLSCMessages() && !I.isAtomic())
    return switchAddrSpace<MessageKind::LSC, A>(I);
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
  return createSVMLoadStore(I, I.getPointerOperand());
}

template <>
Instruction *
GenXLoadStoreLowering::createIntrinsic<HWAddrSpace::A64, MessageKind::Legacy,
                                       Atomicity::NonAtomic, StoreInst>(
    StoreInst &I) const {
  return createSVMLoadStore(I, I.getPointerOperand(), I.getValueOperand());
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
