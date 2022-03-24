/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_CALL_GRAPH_H
#define IGCLLVM_CALL_GRAPH_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/CallGraph.h"
#if LLVM_VERSION_MAJOR < 11
#include "llvm/IR/CallSite.h"
#endif

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR == 10 || LLVM_VERSION_MAJOR == 9
    class CallGraphNode : public llvm::CallGraphNode
    {
      public:
        inline void replaceCallEdge(llvm::CallSite CS, llvm::CallSite NewCS, llvm::CallGraphNode *NewNode)
        {
            llvm::CallGraphNode::replaceCallEdge(*llvm::cast<llvm::CallBase>(CS.getInstruction()),
                                                 *llvm::cast<llvm::CallBase>(NewCS.getInstruction()),
                                                 NewNode);
        }
    };
#else
using CallGraphNode = llvm::CallGraphNode;
#endif

#if LLVM_VERSION_MAJOR < 11
    struct CallRecord final {
        llvm::Optional<llvm::WeakTrackingVH> first{};
        llvm::CallGraphNode *second{nullptr};

        CallRecord() = default;
        CallRecord(llvm::CallGraphNode::CallRecord CR) : first{CR.first}, second{CR.second} {}
    };
#else
    using CallRecord = llvm::CallGraphNode::CallRecord;
#endif
}

#endif
