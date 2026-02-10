/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass will convert all load and store instructions that have 64-bit
/// elements (whether scalar or vector) to vectors of dwords.  This is meant
/// to be run immediately prior to MemOpt to aid vectorization of values
/// with different types.
///
/// For example:
///
/// %x = load i64, i64 addrspace(1)* %"&x"
/// ===>
/// %7 = bitcast i64 addrspace(1)* %"&x" to <2 x i32> addrspace(1)*
/// 8 = load <2 x i32>, <2 x i32> addrspace(1)* %7
/// %9 = bitcast <2 x i32> %8 to i64
///
//===----------------------------------------------------------------------===//

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CodeGenPublic.h"
#include "AdaptorCommon/RayTracing/RTBuilder.h"
#include "PrepareLoadsStoresUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Support/Alignment.h"
#include <optional>

using namespace llvm;
using namespace IGC;
using IGCLLVM::getAlign;

class PrepareLoadsStoresPass : public FunctionPass {
public:
  PrepareLoadsStoresPass() : FunctionPass(ID) {
    initializePrepareLoadsStoresPassPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override;
  StringRef getPassName() const override { return "PrepareLoadsStoresPass"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
  }

  static char ID;

private:
  std::optional<uint32_t> RTAsyncStackAddrSpace;
  std::optional<uint32_t> RTSyncStackAddrSpace;
  std::optional<uint32_t> SWStackAddrSpace;

  bool shouldSplit(uint32_t AddrSpace) const;
};

char PrepareLoadsStoresPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "prepare-loads-stores"
#define PASS_DESCRIPTION "Swizzles load/store types to be more amenable to MemOpt"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PrepareLoadsStoresPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PrepareLoadsStoresPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool PrepareLoadsStoresPass::shouldSplit(uint32_t AddrSpace) const {
  return (AddrSpace == ADDRESS_SPACE_GLOBAL || AddrSpace == RTAsyncStackAddrSpace ||
          AddrSpace == RTSyncStackAddrSpace || AddrSpace == SWStackAddrSpace);
}

bool PrepareLoadsStoresPass::runOnFunction(Function &F) {
  auto *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  auto &rtInfo = Ctx->getModuleMetaData()->rtInfo;

  if (rtInfo.SWStackSurfaceStateOffset) {
    SWStackAddrSpace = RTBuilder::getSWStackStatefulAddrSpace(*Ctx->getModuleMetaData());
  }
  if (rtInfo.RTSyncStackSurfaceStateOffset) {
    RTSyncStackAddrSpace = RTBuilder::getRTSyncStackStatefulAddrSpace(*Ctx->getModuleMetaData());
  }

  bool Changed = false;
  auto &DL = F.getParent()->getDataLayout();

  IGCIRBuilder<> IRB(F.getContext());
  bool StripVolatile = false;

  for (auto II = inst_begin(&F), EI = inst_end(&F); II != EI; /* empty */) {
    auto *I = &*II++;

    if (auto LI = ALoadInst::get(I); LI.has_value()) {
      auto *PtrTy = LI->getPointerOperandType();
      if (!shouldSplit(PtrTy->getPointerAddressSpace()))
        continue;

      IRB.SetInsertPoint(LI->inst());

      if (auto [NewVal, _] = expand64BitLoad(IRB, DL, LI.value()); NewVal) {
        Changed = true;
        NewVal->takeName(LI->inst());
        LI->inst()->replaceAllUsesWith(NewVal);
        LI->inst()->eraseFromParent();
      }
    } else if (auto SI = AStoreInst::get(I); SI.has_value()) {
      if (StripVolatile && (*SWStackAddrSpace == SI->getPointerAddressSpace()))
        SI->setVolatile(false);

      auto *PtrTy = SI->getPointerOperandType();
      if (!shouldSplit(PtrTy->getPointerAddressSpace()))
        continue;

      IRB.SetInsertPoint(SI->inst());

      if (expand64BitStore(IRB, DL, SI.value())) {
        Changed = true;
        SI->inst()->eraseFromParent();
      }
    }
  }

  return Changed;
}

namespace IGC {

Pass *createPrepareLoadsStoresPass(void) { return new PrepareLoadsStoresPass(); }

} // namespace IGC
