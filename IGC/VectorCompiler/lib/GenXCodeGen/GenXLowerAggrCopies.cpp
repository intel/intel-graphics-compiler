/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// \file
// Lower aggregate copies, memset, memcpy, memmov intrinsics into loops when
// the size is large or is not a compile-time constant.
//
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/StackProtector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LowerMemIntrinsics.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/Support/Alignment.h"

#include "vc/GenXCodeGen/GenXLowerAggrCopies.h"

#include <tuple>
#include <vector>

#define DEBUG_TYPE "GENX_LOWERAGGRCOPIES"

using namespace llvm;

// 8 * 8 * 16 = 8 instructions each read 8 OWords
static cl::opt<unsigned>
    ExpandLimitOpt("lower-aggr-copies-expand-limit",
                   cl::desc("max memcpy/memset/memmove length (in bytes) that "
                            "is lowered as scalar code"),
                   cl::init(8 * 8 * 16));

namespace {

// actual analysis class, which is a functionpass
struct GenXLowerAggrCopies : public FunctionPass {
  // TODO: more advance analysis
  //       (at least different values for different arch)
  const int ExpandLimit;
  static char ID;

#if LLVM_VERSION_MAJOR < 16
  GenXLowerAggrCopies() : FunctionPass(ID), ExpandLimit(ExpandLimitOpt) {}
#else  // LLVM_VERSION_MAJOR < 16
  const TargetTransformInfo *TTIp = nullptr;
  explicit GenXLowerAggrCopies(const TargetTransformInfo *TTIp = nullptr)
      : TTIp(TTIp), FunctionPass(ID), ExpandLimit(ExpandLimitOpt) {}
#endif // LLVM_VERSION_MAJOR < 16

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addPreserved<StackProtector>();
    AU.addRequired<TargetTransformInfoWrapperPass>();
  }

  bool runOnFunction(Function &F) override;

  static const unsigned MaxAggrCopySize = 1; // 128;

  StringRef getPassName() const override {
    return "Lower aggregate copies/intrinsics into loops";
  }

  template <typename T> void expandMemMov2VecLoadStore(T *MemCall);
};

char GenXLowerAggrCopies::ID = 0;

namespace {
struct SliceInfo {
  int Offset;
  int Width;
  int Align;
};
} // namespace

static std::vector<SliceInfo> getLegalLengths(int TotalLength, int Align) {
  std::vector<SliceInfo> Slices;
  for (int Offset = 0; TotalLength;) {
    int Width = PowerOf2Floor(TotalLength);
    Slices.push_back({Offset, Width, Align});

    Offset += Width;
    TotalLength -= Width;
    if (Width != Align) {
      Align = std::min(Width, Align);
    }
  }
  return std::move(Slices);
}

// Original memset intrinsic fills memory with 8-bit values.
// This function checks whether bigger type can be used (e.g. storing by 32-bit
// values).
// Desired type size is provided with \p CoalescedTySize parametr and given in
// bytes.
static bool memSetCanBeCoalesced(MemSetInst &MemSet, int CoalescedTySize) {
  auto OrigLength = cast<ConstantInt>(MemSet.getLength())->getSExtValue();
  IGC_ASSERT_MESSAGE(MemSet.getValue()->getType()->getScalarSizeInBits() ==
                         genx::ByteBits,
                     "memset is expected to store by bytes");
  IGC_ASSERT_MESSAGE(CoalescedTySize >= 1 && isPowerOf2_32(CoalescedTySize),
                     "wrong argument: invalid CoalescedTySize");
  return OrigLength % CoalescedTySize == 0 &&
         static_cast<int>(MemSet.getDestAlign().valueOrOne().value()) >=
             CoalescedTySize;
}

// Original memset intrinsic fills memory with 8-bit values.
// This function checks whether bigger type can be used (e.g. storing by 32-bit
// values).
// New coalesced value and corresponding base address and length are returned
// respectively.
// New instructions may be inserted before the \p MemSet to produce these new
// values.
static std::tuple<Value &, Value &, int>
defineOptimalValueAndLength(MemSetInst &MemSet) {
  auto OrigLength = cast<ConstantInt>(MemSet.getLength())->getSExtValue();
  Value &OrigSetVal = *MemSet.getValue();
  Value &OrigBaseAddr = *MemSet.getRawDest();

  // Because DWord is better than Byte and causes minimal problems.
  // OWord can be better but but it requires more code.
  constexpr int CoalescedTySize = genx::DWordBytes;
  if (!memSetCanBeCoalesced(MemSet, CoalescedTySize))
    return {OrigSetVal, OrigBaseAddr, OrigLength};

  IRBuilder<> IRB{&MemSet};
  auto *PreNewSetVal = IRB.CreateVectorSplat(
      CoalescedTySize, &OrigSetVal, OrigSetVal.getName() + ".pre.coalesce");
  auto *NewSetVal = IRB.CreateBitCast(PreNewSetVal, IRB.getInt32Ty(),
                                      OrigSetVal.getName() + ".coalesce");
  auto DstAS = cast<PointerType>(OrigBaseAddr.getType())->getAddressSpace();
  auto *NewBaseAddr =
      IRB.CreateBitCast(&OrigBaseAddr, IRB.getInt32Ty()->getPointerTo(DstAS),
                        OrigBaseAddr.getName() + ".align");
  return {*NewSetVal, *NewBaseAddr, OrigLength / CoalescedTySize};
}

// Fills memory section/slice defined by \p Slice and \p BaseAddr parameters
// with \p SetVal values. Memory is filled by a vector store instruction.
static void setMemorySliceWithVecStore(SliceInfo Slice, Value &SetVal,
                                       Value &BaseAddr,
                                       Instruction *InsertionPt) {
  IGC_ASSERT_MESSAGE(
      InsertionPt,
      "wrong argument: insertion point must be a valid instruction");
  IGC_ASSERT_MESSAGE(Slice.Offset >= 0 && isPowerOf2_32(Slice.Width),
                     "illegal slice is provided");

  auto *VecTy = IGCLLVM::FixedVectorType::get(SetVal.getType(), Slice.Width);
  IRBuilder<> IRB(InsertionPt);
  Value *WriteOut = IRB.CreateVectorSplat(Slice.Width, &SetVal);
  auto *DstAddr = &BaseAddr;
  if (Slice.Offset != 0)
    DstAddr =
        IRB.CreateGEP(SetVal.getType(), &BaseAddr, IRB.getInt32(Slice.Offset),
                      BaseAddr.getName() + ".addr.offset");
  auto DstAS = cast<PointerType>(DstAddr->getType())->getAddressSpace();
  auto *StoreVecPtr = IRB.CreateBitCast(DstAddr, VecTy->getPointerTo(DstAS));
  auto *Store = IRB.CreateStore(WriteOut, StoreVecPtr);
  Store->setAlignment(IGCLLVM::getAlign(Slice.Align));
  Store->setDebugLoc(InsertionPt->getDebugLoc());
}

bool GenXLowerAggrCopies::runOnFunction(Function &F) {
  SmallVector<MemIntrinsic *, 4> MemCalls;

#if LLVM_VERSION_MAJOR < 16
  const TargetTransformInfo &TTI =
      getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);
#endif

  // Collect all aggregate loads and mem* calls.
  for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI) {
    for (BasicBlock::iterator II = BI->begin(), IE = BI->end(); II != IE;
         ++II) {
      if (MemIntrinsic *IntrCall = dyn_cast<MemIntrinsic>(II)) {
        // Convert intrinsic calls with variable size or with constant size
        // larger than the MaxAggrCopySize threshold.
        if (ConstantInt *LenCI = dyn_cast<ConstantInt>(IntrCall->getLength())) {
          if (LenCI->getZExtValue() >= MaxAggrCopySize) {
            MemCalls.push_back(IntrCall);
          }
        } else {
          MemCalls.push_back(IntrCall);
        }
      }
    }
  }

  if (MemCalls.size() == 0) {
    return false;
  }

  // Transform mem* intrinsic calls.
  for (MemIntrinsic *MemCall : MemCalls) {
    bool doLinearExpand =
        !MemCall->isVolatile() && isa<ConstantInt>(MemCall->getLength()) &&
        cast<ConstantInt>(MemCall->getLength())->getSExtValue() <= ExpandLimit;
    if (MemCpyInst *Memcpy = dyn_cast<MemCpyInst>(MemCall)) {
      if (doLinearExpand) {
        expandMemMov2VecLoadStore(Memcpy);
      } else {
#if LLVM_VERSION_MAJOR >= 16
        if (!TTIp) {
            const TargetTransformInfo &TTI = getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);
#endif
        expandMemCpyAsLoop(Memcpy, TTI);
#if LLVM_VERSION_MAJOR >= 16
        }
        else expandMemCpyAsLoop(Memcpy, *TTIp);
#endif
      }
    } else if (MemMoveInst *Memmove = dyn_cast<MemMoveInst>(MemCall)) {
      if (doLinearExpand) {
        expandMemMov2VecLoadStore(Memmove);
      } else {
        expandMemMoveAsLoop(Memmove);
      }
    } else if (MemSetInst *MemSet = dyn_cast<MemSetInst>(MemCall)) {
      if (doLinearExpand) {
        auto &&[SetVal, BaseAddr, Len] = defineOptimalValueAndLength(*MemSet);
        auto Align = MemSet->getDestAlign();
        std::vector<SliceInfo> LegalLengths =
            getLegalLengths(Len, Align.valueOrOne().value());
        for (SliceInfo Slice : LegalLengths)
          setMemorySliceWithVecStore(Slice, SetVal, BaseAddr, MemSet);
      } else {
        expandMemSetAsLoop(MemSet);
      }
    }
    MemCall->eraseFromParent();
  }

  return true;
}

template <typename T>
void GenXLowerAggrCopies::expandMemMov2VecLoadStore(T *MemCall) {
  IRBuilder<> IRB(MemCall);

  auto *LenVal = MemCall->getLength();
  IGC_ASSERT(isa<Constant>(LenVal));
  unsigned Len = cast<ConstantInt>(LenVal)->getZExtValue();

  auto *DstAddr = MemCall->getRawDest();

  auto *VecTy = IGCLLVM::FixedVectorType::get(IRB.getInt8Ty(), Len);

  auto *SrcAddr = MemCall->getRawSource();
  unsigned SrcAddrSpace =
      cast<PointerType>(SrcAddr->getType())->getAddressSpace();

  auto *LoadPtrV = IRB.CreateBitCast(SrcAddr, VecTy->getPointerTo(SrcAddrSpace));
  auto *Load = IRB.CreateLoad(VecTy, LoadPtrV);
  Load->setAlignment(IGCLLVM::getSourceAlign(*MemCall));

  unsigned DstAddrSpace = cast<PointerType>(DstAddr->getType())->getAddressSpace();
  auto *StorePtrV = IRB.CreateBitCast(DstAddr, VecTy->getPointerTo(DstAddrSpace));
  auto *Store = IRB.CreateStore(Load, StorePtrV);
  Store->setAlignment(IGCLLVM::getDestAlign(*MemCall));
}

} // namespace

namespace llvm {
void initializeGenXLowerAggrCopiesPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(
    GenXLowerAggrCopies, "genx-lower-aggr-copies",
    "Lower aggregate copies, and llvm.mem* intrinsics into loops", false, false)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(
    GenXLowerAggrCopies, "genx-lower-aggr-copies",
    "Lower aggregate copies, and llvm.mem* intrinsics into loops", false, false)

#if LLVM_VERSION_MAJOR >= 16
llvm::PreservedAnalyses
GenXLowerAggrCopiesPass::run(Function &F, FunctionAnalysisManager &AM) {
  auto &TT = AM.getResult<TargetIRAnalysis>(F);
  GenXLowerAggrCopies GenXLowAggrCop(&TT);
  if (GenXLowAggrCop.runOnFunction(F))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}
#endif

FunctionPass *llvm::createGenXLowerAggrCopiesPass() {
  initializeGenXLowerAggrCopiesPass(*PassRegistry::getPassRegistry());
  return new GenXLowerAggrCopies();
}
