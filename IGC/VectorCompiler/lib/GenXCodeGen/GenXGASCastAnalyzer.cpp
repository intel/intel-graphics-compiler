/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXGASCastAnalyzer.h"
#include "GenX.h"

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

bool GenXGASCastWrapper::runOnModule(Module &M) {
  CastInfoCache.clear();
  GI.FunctionMap.clear();

  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  for (auto &F : M.functions()) {
    if (F.getCallingConv() != CallingConv::SPIR_KERNEL)
      continue;
    bool HasIndirectCall = false;
    SmallPtrSet<const Function *, 16> FG;
    getFunctionGroup(&F, CG, FG, HasIndirectCall);
    FG.insert(&F);
    if (HasIndirectCall) {
      setInfoForFG(FG, HasLocalToGenericCast);
      continue;
    }
    unsigned Info = 0;
    for (auto F : FG) {
      Info |= hasLocalCastsToGeneric(F);
      // Early exit if local->generic casts have been found.
      if (Info == HasLocalToGenericCast)
        break;
    }
    setInfoForFG(FG, Info);
  }
  return false;
}

void GenXGASCastWrapper::getFunctionGroup(const Function *F, CallGraph &CG,
                                          SmallPtrSetImpl<const Function *> &FG,
                                          bool &HasIndirectCall) const {
  HasIndirectCall = false;
  SmallVector<const Function *, 16> WorkList;
  WorkList.push_back(F);
  while (!WorkList.empty()) {
    const Function *F = WorkList.back();
    WorkList.pop_back();
    CallGraphNode &N = *CG[F];
    for (IGCLLVM::CallRecord CE : N) {
      // Skipping reference edges.
      if (!CE.first)
        continue;
      Function *Child = CE.second->getFunction();
      if (!Child) {
        // Indirect call or inline asm.
        if (CallBase *CB = dyn_cast_or_null<CallBase>(CE.first.getValue())) {
          if (CB->isIndirectCall()) {
            // Continue gathering all functions accessible from kernel "F"
            // to find out what functions need additional control flow for GAS
            // accesses.
            HasIndirectCall = true;
          }
        }
        continue;
      }
      if (Child->isDeclaration())
        continue;
      bool NotVisited = FG.insert(Child).second;
      if (NotVisited) {
        WorkList.push_back(Child);
      }
    }
  }
}

void GenXGASCastWrapper::setInfoForFG(SmallPtrSetImpl<const Function *> &FG,
                                      unsigned CastInfo) {
  for (auto F : FG) {
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

unsigned GenXGASCastWrapper::hasLocalCastsToGeneric(const Function *F) {
  if (CastInfoCache.count(F))
    return CastInfoCache[F];
  unsigned CastInfo = 0;
  for (auto &I : instructions(F)) {
    if (CastInfo == HasLocalToGenericCast)
      break;
    if (const AddrSpaceCastInst *CI = dyn_cast<AddrSpaceCastInst>(&I)) {
      unsigned DestAS = CI->getDestAddressSpace();
      unsigned SrcAS = CI->getSrcAddressSpace();
      if (DestAS == vc::AddrSpace::Generic && SrcAS == vc::AddrSpace::Local)
        CastInfo |= HasLocalToGenericCast;
    }
  }
  CastInfoCache[F] = CastInfo;
  return CastInfo;
}
