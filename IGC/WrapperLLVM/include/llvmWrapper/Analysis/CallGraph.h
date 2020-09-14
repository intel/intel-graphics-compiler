/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#ifndef IGCLLVM_CALL_GRAPH_H
#define IGCLLVM_CALL_GRAPH_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/CallGraph.h"
#if LLVM_VERSION_MAJOR < 11
#include "llvm/IR/CallSite.h"
#endif

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR == 10
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
}

#endif
