/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_CALL_GRAPH_H
#define IGCLLVM_CALL_GRAPH_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/CallGraph.h"

namespace IGCLLVM {
using CallGraphNode = llvm::CallGraphNode;

using CallRecord = llvm::CallGraphNode::CallRecord;
} // namespace IGCLLVM

#endif
