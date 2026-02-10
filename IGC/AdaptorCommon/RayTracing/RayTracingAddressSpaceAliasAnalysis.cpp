/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass adds custom AddrSpace AA for RT.
///
//===----------------------------------------------------------------------===//

#include "RayTracingAddressSpaceAliasAnalysis.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

namespace IGC {

bool RayTracingAddressSpaceAAResult::checkStateful(const CodeGenContext &Ctx) {
  // Determine if all RT memory regions are enabled stateful. This will
  // determine what we can say about aliasing in some cases.
  auto &rtInfo = Ctx.getModuleMetaData()->rtInfo;
  return rtInfo.RTAsyncStackAddrspace != UINT_MAX && rtInfo.SWHotZoneAddrspace != UINT_MAX &&
         rtInfo.SWStackAddrspace != UINT_MAX && (!Ctx.hasSyncRTCalls() || rtInfo.RTSyncStackAddrspace != UINT_MAX);
}

bool RayTracingAddressSpaceAAResult::isRTAS(unsigned AS, const CodeGenContext &Ctx) {
  auto &rtInfo = Ctx.getModuleMetaData()->rtInfo;
  return isStatefulAddrSpace(AS) && (AS == rtInfo.RTAsyncStackAddrspace || AS == rtInfo.SWHotZoneAddrspace ||
                                     AS == rtInfo.SWStackAddrspace || AS == rtInfo.RTSyncStackAddrspace);
}

bool RayTracingAddressSpaceAAResult::isRTAS(unsigned AS) const { return isRTAS(AS, CGC); }

bool RayTracingAddressSpaceAAResult::noRTASAlias(unsigned AS1, unsigned AS2) const {
  return ((isRTAS(AS1) || isRTAS(AS2)) && AS1 != AS2);
}

IGCLLVM::AliasResultEnum RayTracingAddressSpaceAAResult::alias(const MemoryLocation &LocA, const MemoryLocation &LocB,
                                                               AAQueryInfo &AAQI, const llvm::Instruction *CtxI) {
  PointerType *PtrTy1 = dyn_cast<PointerType>(LocA.Ptr->getType());
  PointerType *PtrTy2 = dyn_cast<PointerType>(LocB.Ptr->getType());

  if (!PtrTy1 || !PtrTy2)
    return IGCLLVM::AliasResultEnum::NoAlias;

  unsigned AS1 = PtrTy1->getAddressSpace();
  unsigned AS2 = PtrTy2->getAddressSpace();
  if (noRTASAlias(AS1, AS2)) {
    return IGCLLVM::AliasResultEnum::NoAlias;
  }

  // Forward the query to the next analysis.
  return BaseT::alias(LocA, LocB, AAQI, CtxI);
}

ModRefInfo RayTracingAddressSpaceAAResult::getModRefInfo(const CallBase *Call, const MemoryLocation &Loc,
                                                         AAQueryInfo &AAQI) {
  auto *PtrTy = dyn_cast<PointerType>(Loc.Ptr->getType());
  if (!PtrTy)
    return ModRefInfo::NoModRef;

  if (isa<StackIDReleaseIntrinsic>(Call)) {
    if (allStateful) {
      uint32_t Addrspace = PtrTy->getPointerAddressSpace();
      return isRTAS(Addrspace) ? ModRefInfo::Mod : ModRefInfo::NoModRef;
    }
  }
  return AAResultBase::getModRefInfo(Call, Loc, AAQI);
}

ModRefInfo RayTracingAddressSpaceAAResult::getModRefInfo(const CallBase *Call1, const CallBase *Call2,
                                                         AAQueryInfo &AAQI) {
  return AAResultBase::getModRefInfo(Call1, Call2, AAQI);
}

void RayTracingAddressSpaceAAWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<CodeGenContextWrapper>();
}

bool RayTracingAddressSpaceAAWrapperPass::doInitialization(Module &M) {
  if (M.size() > 0) {
    auto &F = *M.begin(); // see llvmWrapper/Analysis/TargetLibraryInfo.h
    Result.reset(new RayTracingAddressSpaceAAResult(getAnalysis<TargetLibraryInfoWrapperPass>().getTLI(),
                                                    *getAnalysis<CodeGenContextWrapper>().getCodeGenContext()));
  }
  return false;
}

bool RayTracingAddressSpaceAAWrapperPass::doFinalization(Module &M) {
  Result.reset();
  return false;
}

RayTracingAddressSpaceAAResult &RayTracingAddressSpaceAAWrapperPass::getResult() { return *Result; }
const RayTracingAddressSpaceAAResult &RayTracingAddressSpaceAAWrapperPass::getResult() const { return *Result; }

} // namespace IGC

#define PASS_FLAG "igc-raytracing-address-space-alias-analysis"
#define PASS_DESC "RayTracing Address space alias analysis"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(RayTracingAddressSpaceAAWrapperPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(RayTracingAddressSpaceAAWrapperPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char IGC::RayTracingAddressSpaceAAWrapperPass::ID = 0;

IGC::RayTracingAddressSpaceAAWrapperPass::RayTracingAddressSpaceAAWrapperPass() : ImmutablePass(ID) {
  initializeRayTracingAddressSpaceAAWrapperPassPass(*PassRegistry::getPassRegistry());
  Result = nullptr;
}

ImmutablePass *IGC::createRayTracingAddressSpaceAAWrapperPass() { return new RayTracingAddressSpaceAAWrapperPass(); }

void IGC::addRayTracingAddressSpaceAAResult(Pass &P, Function &, AAResults &AAR) {
  if (auto *WrapperPass = P.getAnalysisIfAvailable<RayTracingAddressSpaceAAWrapperPass>())
    AAR.addAAResult(WrapperPass->getResult());
}
