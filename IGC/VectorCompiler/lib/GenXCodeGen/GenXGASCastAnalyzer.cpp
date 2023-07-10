/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXGASCastAnalyzer.h"
#include "GenX.h"

#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/General/Types.h"

#include "llvmWrapper/Analysis/CallGraph.h"

#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>

#define DEBUG_TYPE "GENX_GASCASTANALYZER"

using namespace llvm;
using namespace genx;

char GenXGASCastWrapper::ID = 0;
namespace llvm {
void initializeGenXGASCastWrapperPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXGASCastWrapper, "GenXGASCastWrapper",
                      "GenXGASCastWrapper", false, true)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_END(GenXGASCastWrapper, "GenXGASCastWrapper",
                    "GenXGASCastWrapper", false, true)

ModulePass *llvm::createGenXGASCastWrapperPass() {
  initializeGenXGASCastWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXGASCastWrapper;
}

static unsigned getInfoForAllCasts() {
  return GASInfo::HasPrivateToGeneric | GASInfo::HasLocalToGeneric |
         GASInfo::HasGlobalToGeneric;
}

static bool isInfoFull(unsigned Info) { return (Info == getInfoForAllCasts()); }

void GenXGASCastWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<CallGraphWrapperPass>();
}

bool GenXGASCastWrapper::runOnModule(Module &M) {
  CastInfoCache.clear();
  GI.FunctionMap.clear();

  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  for (auto &F : M.functions()) {
    if (!vc::isKernel(&F))
      continue;

    bool HasIndirectCall = false;
    SmallPtrSet<const Function *, 16> FG;
    traverseCallGraph(&F, CG, FG, HasIndirectCall);
    FG.insert(&F);

    // It's not possible to determine if a function called indirectly
    // contains any addrspacecasts. So, we assume that all 3 casts
    // are possible.
    if (HasIndirectCall) {
      setInfoForFG(FG, getInfoForAllCasts());
      continue;
    }

    unsigned CastInfo = 0;
    for (auto *F : FG) {
      CastInfo |= hasCastsToGeneric(*F);
      // Early exit if all 3 possible casts are found.
      if (isInfoFull(CastInfo))
        break;
    }
    setInfoForFG(FG, CastInfo);
  }
  return false;
}

void GenXGASCastWrapper::traverseCallGraph(
    const Function *F, CallGraph &CG, SmallPtrSetImpl<const Function *> &FG,
    bool &HasIndirectCall) const {
  HasIndirectCall = false;
  SmallVector<const Function *, 16> WorkList;
  WorkList.push_back(F);
  while (!WorkList.empty()) {
    const auto *F = WorkList.pop_back_val();
    CallGraphNode &N = *CG[F];
    for (IGCLLVM::CallRecord CE : N) {
      // Skipping reference edges.
      if (!CE.first)
        continue;
      Function *Child = CE.second->getFunction();
      if (!Child) {
        // Indirect call or inline asm.
        if (CallBase *CB = dyn_cast_or_null<CallBase>(CE.first.getValue()))
          if (CB->isIndirectCall())
            HasIndirectCall = true;
        continue;
      }
      if (Child->isDeclaration())
        continue;
      bool NotVisited = FG.insert(Child).second;
      if (NotVisited)
        WorkList.push_back(Child);
    }
  }
}

void GenXGASCastWrapper::setInfoForFG(SmallPtrSetImpl<const Function *> &FG,
                                      unsigned CastInfo) {
  for (auto *F : FG) {
    auto E = GI.FunctionMap.find(F);
    if (E != GI.FunctionMap.end()) {
      // If a function already exists in the map, it means that it is called by
      // more than one kernel. Existing element represents a result of an
      // analysis processed on another kernel call graph. Below code makes an OR
      // operation on previous and current analysis results to take both into
      // account.
      GI.FunctionMap[F] = E->second | CastInfo;
      continue;
    }
    GI.FunctionMap[F] = CastInfo;
  }
}

unsigned GenXGASCastWrapper::hasCastsToGeneric(const Function &F) {
  if (CastInfoCache.count(&F))
    return CastInfoCache[&F];
  unsigned CastInfo = 0;
  for (auto &I : instructions(&F)) {
    if (isInfoFull(CastInfo))
      break;
    if (const AddrSpaceCastInst *CI = dyn_cast<AddrSpaceCastInst>(&I)) {
      unsigned DstAS = CI->getDestAddressSpace();
      unsigned SrcAS = CI->getSrcAddressSpace();
      if (DstAS != vc::AddrSpace::Generic)
        continue;
      if (SrcAS == vc::AddrSpace::Private)
        CastInfo |= GASInfo::HasPrivateToGeneric;
      else if (SrcAS == vc::AddrSpace::Local)
        CastInfo |= GASInfo::HasLocalToGeneric;
      else if (SrcAS == vc::AddrSpace::Global)
        CastInfo |= GASInfo::HasGlobalToGeneric;
    }
  }
  CastInfoCache[&F] = CastInfo;
  return CastInfo;
}
