/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/Support/Alignment.h>
#include <llvmWrapper/Support/MathExtras.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/LdShrink.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

namespace {

// A simple pass to shrink vector load into scalar or narrow vector load
// when only partial elements are used.
class LdShrink : public FunctionPass {
  const DataLayout *DL;

public:
  static char ID;

  LdShrink() : FunctionPass(ID) { initializeLdShrinkPass(*PassRegistry::getPassRegistry()); }

  bool runOnFunction(Function &F) override;

private:
  void getAnalysisUsage(AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

  unsigned getExtractIndexMask(LoadInst *LI) const;
};

char LdShrink::ID = 0;

} // End anonymous namespace

FunctionPass *createLdShrinkPass() { return new LdShrink(); }

#define PASS_FLAG "igc-ldshrink"
#define PASS_DESC "IGC Load Shrink"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LdShrink, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(LdShrink, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

unsigned LdShrink::getExtractIndexMask(LoadInst *LI) const {
  IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(LI->getType());
  // Skip non-vector loads.
  if (!VTy)
    return 0;
  // Skip if there are more than 32 elements.
  if (VTy->getNumElements() > 32)
    return 0;
  // Check whether all users are ExtractElement with constant index.
  // Collect index mask at the same time.
  Type *Ty = VTy->getScalarType();
  // Skip non-BYTE addressable data types. So far, check integer types
  // only.
  if (IntegerType *ITy = dyn_cast<IntegerType>(Ty)) {
    // Unroll isPowerOf2ByteWidth, it was removed in LLVM 12.
    unsigned BitWidth = ITy->getBitWidth();
    if (!((BitWidth > 7) && isPowerOf2_32(BitWidth)))
      return 0;
  }

  unsigned Mask = 0; // Maxmimally 32 elements.

  for (auto UI = LI->user_begin(), UE = LI->user_end(); UI != UE; ++UI) {
    ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(*UI);
    if (!EEI)
      return 0;
    // Skip non-constant index.
    auto Idx = dyn_cast<ConstantInt>(EEI->getIndexOperand());
    if (!Idx)
      return 0;
    IGC_ASSERT_MESSAGE(Idx->getZExtValue() < 32, "Index is out of range!");
    Mask |= (1 << Idx->getZExtValue());
  }

  return Mask;
}

bool LdShrink::runOnFunction(Function &F) {
  DL = &F.getParent()->getDataLayout();
  if (!DL)
    return false;

  bool Changed = false;
  for (auto &BB : F) {
    for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*EMPTY*/) {
      LoadInst *LI = dyn_cast<LoadInst>(BI++);
      // Skip non-load instructions.
      if (!LI)
        continue;
      // Skip non-simple load.
      if (!LI->isSimple())
        continue;
      // Skip for loads that are already doing 32-bit or smaller accesses.
      if (DL->getTypeSizeInBits(LI->getType()) <= 32)
        continue;

      // Replace it with scalar load or narrow vector load.
      unsigned Mask = getExtractIndexMask(LI);
      if (!Mask)
        continue;
      if (!isShiftedMask_32(Mask))
        continue;
      unsigned Offset = IGCLLVM::countr_zero(Mask);
      unsigned Length = IGCLLVM::countr_zero((Mask >> Offset) + 1);
      // TODO: So far skip narrow vector.
      if (Length != 1)
        continue;

      IGCLLVM::IRBuilder<> Builder(LI);

      // Shrink it to scalar load.
      auto Ptr = LI->getPointerOperand();
      Type *Ty = LI->getType();
      Type *ScalarTy = Ty->getScalarType();
      PointerType *PtrTy = cast<PointerType>(Ptr->getType());
      PointerType *ScalarPtrTy = PointerType::get(ScalarTy, PtrTy->getAddressSpace());
      Value *ScalarPtr = Builder.CreatePointerCast(Ptr, ScalarPtrTy);
      if (Offset)
        ScalarPtr = Builder.CreateInBoundsGEP(ScalarTy, ScalarPtr, Builder.getInt32(Offset));

      alignment_t alignment =
          (alignment_t)MinAlign(IGCLLVM::getAlignmentValue(LI), DL->getTypeStoreSize(ScalarTy) * Offset);

      LoadInst *NewLoad = Builder.CreateAlignedLoad(ScalarTy, ScalarPtr, IGCLLVM::getAlign(alignment));
      NewLoad->setDebugLoc(LI->getDebugLoc());
      if (MDNode *mdNode = LI->getMetadata("lsc.cache.ctrl")) {
        NewLoad->setMetadata("lsc.cache.ctrl", mdNode);
      }

      ExtractElementInst *EEI = cast<ExtractElementInst>(*LI->user_begin());
      EEI->replaceAllUsesWith(NewLoad);
    }
  }

  return Changed;
}
