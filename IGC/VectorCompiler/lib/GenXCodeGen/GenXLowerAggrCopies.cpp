/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// \file
// Lower aggregate copies, memset, memcpy, memmov intrinsics into loops when
// the size is large or is not a compile-time constant.
//
//===----------------------------------------------------------------------===//

#include "GenXLowerAggrCopies.h"
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

  GenXLowerAggrCopies() : FunctionPass(ID), ExpandLimit(ExpandLimitOpt) {}

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
};
} // namespace

static std::vector<SliceInfo> getLegalLengths(int TotalLength) {
  std::vector<SliceInfo> Slices;
  for (int Offset = 0; TotalLength;) {
    int Width = PowerOf2Floor(TotalLength);
    Slices.push_back({Offset, Width});
    Offset += Width;
    TotalLength -= Width;
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
         static_cast<int>(MemSet.getDestAlignment()) >= CoalescedTySize;
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
  IGC_ASSERT_MESSAGE(SetVal.getType() ==
                         BaseAddr.getType()->getPointerElementType(),
                     "value and pointer types must correspond");

  auto *VecTy = IGCLLVM::FixedVectorType::get(SetVal.getType(), Slice.Width);
  IRBuilder<> IRB(InsertionPt);
  Value *WriteOut = IRB.CreateVectorSplat(Slice.Width, &SetVal);
  auto *DstAddr = &BaseAddr;
  if (Slice.Offset != 0)
    DstAddr = IRB.CreateGEP(BaseAddr.getType()->getPointerElementType(),
                            &BaseAddr, IRB.getInt32(Slice.Offset),
                            BaseAddr.getName() + ".addr.offset");
  auto DstAS = cast<PointerType>(DstAddr->getType())->getAddressSpace();
  auto *StoreVecPtr = IRB.CreateBitCast(DstAddr, VecTy->getPointerTo(DstAS));
  IRB.CreateStore(WriteOut, StoreVecPtr);
}

bool GenXLowerAggrCopies::runOnFunction(Function &F) {
  SmallVector<MemIntrinsic *, 4> MemCalls;

  const TargetTransformInfo &TTI =
      getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);

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
    bool doLinearExpand = !MemCall->isVolatile() && isa<ConstantInt>(MemCall->getLength()) &&
      cast<ConstantInt>(MemCall->getLength())->getSExtValue() <= ExpandLimit;
    if (MemCpyInst *Memcpy = dyn_cast<MemCpyInst>(MemCall)) {
      if (doLinearExpand) {
        expandMemMov2VecLoadStore(Memcpy);
      } else {
        expandMemCpyAsLoop(Memcpy, TTI);
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
        std::vector<SliceInfo> LegalLengths = getLegalLengths(Len);
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
  llvm::Value *LenVal = MemCall->getLength();
  IGC_ASSERT(isa<Constant>(LenVal));
  auto Len = (unsigned)cast<ConstantInt>(LenVal)->getZExtValue();
  auto DstPtrV = MemCall->getRawDest();
  IGC_ASSERT(DstPtrV->getType()->isPointerTy());
  auto I8Ty = cast<PointerType>(DstPtrV->getType())->getPointerElementType();
  IGC_ASSERT(I8Ty->isIntegerTy(8));
  auto VecTy = IGCLLVM::FixedVectorType::get(I8Ty, Len);
  auto SrcAddr = MemCall->getRawSource();
  unsigned srcAS = cast<PointerType>(SrcAddr->getType())->getAddressSpace();
  auto LoadPtrV = IRB.CreateBitCast(SrcAddr, VecTy->getPointerTo(srcAS));
  Type *Ty = LoadPtrV->getType()->getPointerElementType();
  auto ReadIn = IRB.CreateLoad(Ty, LoadPtrV);
  auto DstAddr = MemCall->getRawDest();
  unsigned dstAS = cast<PointerType>(DstAddr->getType())->getAddressSpace();
  auto StorePtrV = IRB.CreateBitCast(DstAddr, VecTy->getPointerTo(dstAS));
  IRB.CreateStore(ReadIn, StorePtrV);
}

} // namespace

namespace llvm {
void initializeGenXLowerAggrCopiesPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXLowerAggrCopies, "genx-lower-aggr-copies",
                "Lower aggregate copies, and llvm.mem* intrinsics into loops",
                false, false)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(GenXLowerAggrCopies, "genx-lower-aggr-copies",
                "Lower aggregate copies, and llvm.mem* intrinsics into loops",
                false, false)

FunctionPass *llvm::createGenXLowerAggrCopiesPass() {
  initializeGenXLowerAggrCopiesPass(*PassRegistry::getPassRegistry());
  return new GenXLowerAggrCopies();
}
