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

class GASInfo {
public:
  enum {
    HasPrivateToGeneric = 1 << 0,
    HasLocalToGeneric = 1 << 1,
    HasGlobalToGeneric = 1 << 2
  };

public:
  bool canGenericPointToPrivate(Function &F) const {
    auto E = FunctionMap.find(&F);
    if (E == FunctionMap.end())
      return true;
    return E->second & HasPrivateToGeneric;
  }
  bool canGenericPointToLocal(Function &F) const {
    auto E = FunctionMap.find(&F);
    if (E == FunctionMap.end())
      return true;
    return E->second & HasLocalToGeneric;
  }
  bool canGenericPointToGlobal(Function &F) const {
    auto E = FunctionMap.find(&F);
    if (E == FunctionMap.end())
      return true;
    return E->second & HasGlobalToGeneric;
  }

private:
  DenseMap<const Function *, unsigned> FunctionMap;
  friend class llvm::GenXGASCastWrapper;
};

class GenXGASCastWrapper : public ModulePass {
public:
  static char ID;
  explicit GenXGASCastWrapper() : ModulePass(ID) {}
  StringRef getPassName() const override { return "Cast To GAS Analysis"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;

public:
  GASInfo &getGASInfo() { return GI; }

private:
  void setInfoForFG(SmallPtrSetImpl<const Function *> &FG, unsigned CastInfo);
  void traverseCallGraph(const Function *F, CallGraph &CG,
                         SmallPtrSetImpl<const Function *> &FG,
                         bool &HasIndirectCall) const;
  unsigned hasCastsToGeneric(const Function &F);

private:
  DenseMap<const Function *, unsigned> CastInfoCache;
  GASInfo GI;
};
} // end namespace llvm

#endif // GENXGASCASTANALYZER_H
