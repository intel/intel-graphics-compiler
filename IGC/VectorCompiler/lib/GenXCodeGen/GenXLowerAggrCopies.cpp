/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//
// \file
// Lower aggregate copies, memset, memcpy, memmov intrinsics into loops when
// the size is large or is not a compile-time constant.
//
//===----------------------------------------------------------------------===//

#include "GenXLowerAggrCopies.h"
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
    } else if (MemSetInst *Memset = dyn_cast<MemSetInst>(MemCall)) {
      if (doLinearExpand) {
        llvm::Value *SetVal = Memset->getValue();
        llvm::Value *LenVal = Memset->getLength();
        assert(isa<Constant>(LenVal));
        assert(SetVal->getType()->getScalarSizeInBits() == 8);
        auto Len = (unsigned)cast<ConstantInt>(LenVal)->getZExtValue();
        auto VecTy = VectorType::get(SetVal->getType(), Len);
        Value *WriteOut = UndefValue::get(VecTy);
        IRBuilder<> IRB(Memset);
        for (unsigned i = 0; i < Len; ++i) {
          WriteOut = IRB.CreateInsertElement(WriteOut, SetVal, IRB.getInt32(i));
        }
        auto DstAddr = Memset->getRawDest();
        unsigned dstAS = cast<PointerType>(DstAddr->getType())->getAddressSpace();
        auto StorePtrV =
            IRB.CreateBitCast(DstAddr, VecTy->getPointerTo(dstAS));
        IRB.CreateStore(WriteOut, StorePtrV);
      } else {
        expandMemSetAsLoop(Memset);
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
  assert(isa<Constant>(LenVal));
  auto Len = (unsigned)cast<ConstantInt>(LenVal)->getZExtValue();
  auto DstPtrV = MemCall->getRawDest();
  assert(DstPtrV->getType()->isPointerTy());
  auto I8Ty = cast<PointerType>(DstPtrV->getType())->getElementType();
  assert(I8Ty->isIntegerTy(8));
  auto VecTy = VectorType::get(I8Ty, Len);
  auto SrcAddr = MemCall->getRawSource();
  unsigned srcAS = cast<PointerType>(SrcAddr->getType())->getAddressSpace();
  auto LoadPtrV = IRB.CreateBitCast(SrcAddr, VecTy->getPointerTo(srcAS));
  auto ReadIn = IRB.CreateLoad(LoadPtrV);
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
  return new GenXLowerAggrCopies();
}
