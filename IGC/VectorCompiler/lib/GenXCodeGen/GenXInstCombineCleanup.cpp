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
/// GenXInstCombineCleanupPass
/// --------------------------
///
/// For switch instructions llvm 7.0 instcombine aggressively shrikns the type
/// of the condition variable. This can introduce types which are unsupported
/// in GenX IR (like i2, i27, etc)
/// The pass tries to detect such switch instructions and modify them to use
/// the original condition instead of a truncated one.
/// The idea is to do it using a standard llvm passes, so we just try to do the
/// opposite to inst combine change and expect irbuilder folding or other passes
/// to change code as it was before.

#define DEBUG_TYPE "GENX_INSTCOMBCLEANUP"

#include "GenX.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace genx;

namespace {

class GenXInstCombineCleanup : public FunctionPass {
public:
  static char ID;

  explicit GenXInstCombineCleanup() : FunctionPass(ID) { }

  StringRef getPassName() const override { return "GenX InstCombineCleanup"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
};

} // end anonymous namespace

char GenXInstCombineCleanup::ID = 0;
namespace llvm { void initializeGenXInstCombineCleanupPass(PassRegistry &); }
INITIALIZE_PASS_BEGIN(GenXInstCombineCleanup, "GenXInstCombineCleanup", "GenXInstCombineCleanup", false, false)
INITIALIZE_PASS_END(GenXInstCombineCleanup, "GenXInstCombineCleanup", "GenXInstCombineCleanup", false, false)

FunctionPass *llvm::createGenXInstCombineCleanup()
{
  initializeGenXInstCombineCleanupPass(*PassRegistry::getPassRegistry());
  return new GenXInstCombineCleanup();
}

void GenXInstCombineCleanup::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.setPreservesCFG();
}

bool typeMustBeChanged(Type *Ty) {
  assert(Ty);
  if (!Ty->isIntegerTy())
    return false;
  unsigned Size = Ty->getPrimitiveSizeInBits();
  // Possible sizes are 1, 8, 16, 32, ... (2 and 4 must be excluded)
  if (isPowerOf2_32(Size) && !(genx::BoolBits < Size && Size < genx::ByteBits))
    return false;
  return true;
}

bool GenXInstCombineCleanup::runOnFunction(Function &F)
{
  bool Modified = false;

#if (LLVM_VERSION_MAJOR <= 7)
  LLVM_DEBUG(dbgs() << "running GenXInstCombineCleanup on " << F.getName() << "\n");

  LLVMContext &Ctx = F.getContext();
  IRBuilder<> Builder(Ctx);

  for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    auto Switch = dyn_cast<SwitchInst>(&*I);
    if (!Switch)
      continue;

    auto Cond = Switch->getCondition();
    Type *CondTy = Cond->getType();
    if (!typeMustBeChanged(CondTy))
      continue;

    unsigned CondSize = CondTy->getPrimitiveSizeInBits();
    assert(CondSize != genx::BoolBits &&
           "CondSize == 1 is not expected here. See typeMustBeChanged");
    // Round up to the next power of 2 skipping i2 and i4 (i3 -> i8, i2 -> i8,
    // etc)
    unsigned Size =
        CondSize < genx::ByteBits ? genx::ByteBits : NextPowerOf2(CondSize);

    Type *NewTy = Type::getIntNTy(Ctx, Size);

    Builder.SetInsertPoint(Switch);
    Value *NewCond =
        Builder.CreateSExt(Cond, NewTy, Switch->getName() + ".condSExt");
    Switch->setCondition(NewCond);

    for (auto Case : Switch->cases()) {
      APInt UpdatedCase = Case.getCaseValue()->getValue().sext(Size);
      Case.setValue(ConstantInt::get(Ctx, UpdatedCase));
    }

    Modified = true;
  }
#endif

  return Modified;
}

