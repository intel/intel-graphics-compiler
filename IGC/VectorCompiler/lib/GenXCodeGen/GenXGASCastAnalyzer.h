/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXGASCastWrapper
/// -------------
///
/// GenXGASCastWrapper is an analysis that provides an information about
/// local->generic casts that will be used in GAS resolution.
///
/// The pass processes a callgraph for an each kernel (function group),
/// detect local->generic casts and if find ones mark all functions from FG with
/// HasLocalToGenericCast.
///
/// When the function doesn't have a HasLocalToGenericCast label then all loads
/// stores from generic memory will be resolved to loads/store from global in
/// GAS resolution pass.
//===----------------------------------------------------------------------===//

#ifndef GENXGASCASTANALYZER_H
#define GENXGASCASTANALYZER_H

#include <llvm/Analysis/CallGraph.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>

namespace llvm {

class GenXGASCastWrapper;

namespace genx {
enum {
  HasLocalToGenericCast = 1 << 0,
};
class GASInfo {
public:
  bool canGenericPointToLocal(Function &F) const {
    auto E = FunctionMap.find(&F);
    if (E == FunctionMap.end())
      return true;
    return E->second & HasLocalToGenericCast;
  }

private:
  DenseMap<const Function *, unsigned> FunctionMap;
  friend class llvm::GenXGASCastWrapper;
};
} // end namespace genx

class GenXGASCastWrapper : public ModulePass {
public:
  static char ID;
  GenXGASCastWrapper() : ModulePass(ID) {}
  ~GenXGASCastWrapper() = default;
  StringRef getPassName() const override { return "Cast To GAS Analysis"; }
  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    AU.addRequired<CallGraphWrapperPass>();
  }
  genx::GASInfo &getGASInfo() { return GI; }

private:
  genx::GASInfo GI;
  DenseMap<const Function *, unsigned> CastInfoCache;
  void setInfoForFG(SmallPtrSetImpl<const Function *> &FG, unsigned CastInfo);
  unsigned hasLocalCastsToGeneric(const Function *F);
  void getFunctionGroup(const Function *F, CallGraph &CG,
                        SmallPtrSetImpl<const Function *> &FG,
                        bool &HasIndirectCall) const;
};
} // end namespace llvm

#endif // GENXGASCASTANALYZER_H
