/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_CALL_GRAPH_H
#define IGCLLVM_CALL_GRAPH_H

#include "Probe/Assertion.h"
#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/AbstractCallSite.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
// Compatibility for CallGraphNode::removeCallEdgeFor(CallBase&), removed in LLVM 22. Drops the call-graph
// edge for the concrete call site CB (removeOneAbstractEdgeTo alone only matches null/abstract edges),
// then drops the abstract edges for any callback functions.
inline void removeCallEdgeFor(llvm::CallGraphNode *Node, llvm::CallGraph &CG, llvm::CallBase &CB) {
#if LLVM_VERSION_MAJOR < 22
  (void)CG;
  Node->removeCallEdgeFor(CB);
#else
  for (auto I = Node->begin(), E = Node->end(); I != E; ++I) {
    if (I->first && *I->first == &CB) {
      Node->removeCallEdge(I);
      llvm::forEachCallbackFunction(
          CB, [&](llvm::Function *Cb) { Node->removeOneAbstractEdgeTo(CG.getOrInsertFunction(Cb)); });
      return;
    }
  }
  IGC_ASSERT_MESSAGE(false, "Cannot find callsite to remove!");
#endif
}

using CallGraphNode = llvm::CallGraphNode;
using CallRecord = llvm::CallGraphNode::CallRecord;

} // namespace IGCLLVM

#endif
