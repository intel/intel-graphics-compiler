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
//
/// GenXLayoutBlocks
/// -------------------
///
/// This pass tidies the control flow in the following way:
///
/// It reorders blocks to increase fallthrough generally, and specifically
/// to ensure that SIMD CF goto and join have the required structure: the
/// "false" successor must be fallthrough and the "true" successor must be
/// forward. (The '"true" successor must be forward' requirement is a vISA
/// requirement, because vISA goto/join does not specify JIP, and the
/// finalizer reconstructs it on this assumption.)
///
/// This pass is invoked in ISPC flow to ensure SIMD CF conformance.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "GENX_LAYOUTBLOCKS"

#include "GenX.h"
#include "GenXBaling.h"
#include "GenXGotoJoin.h"
#include "GenXLiveness.h"
#include "GenXModule.h"
#include "GenXNumbering.h"
#include "GenXSubtarget.h"
#include "GenXUtil.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace genx;

/***********************************************************************
 * GenXLayoutBlocks pass declaration
 */
namespace {
class GenXLayoutBlocks : public FunctionPass {
public:
  static char ID;
  explicit GenXLayoutBlocks() : FunctionPass(ID) {}
  virtual StringRef getPassName() const { return "GenX layout blocks"; }

  void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addPreserved<GenXModule>();
    AU.addPreserved<GenXGroupBaling>();
    AU.addPreserved<GenXLiveness>();
    AU.addPreserved<GenXNumbering>();
    AU.addPreserved<FunctionGroupAnalysis>();
    AU.addRequired<LoopInfoWrapperPass>();
  }

  bool runOnFunction(Function &F);
  // createPrinterPass : get a pass to print the IR, together with the GenX
  // specific analyses
  virtual Pass *createPrinterPass(raw_ostream &O,
                                  const std::string &Banner) const {
    return createGenXPrinterPass(O, Banner);
  }
};
} // end anonymous namespace.

char GenXLayoutBlocks::ID = 0;
namespace llvm {
void initializeGenXLayoutBlocksPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXLayoutBlocks, "GenXLayoutBlocks", "GenXLayoutBlocks",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_END(GenXLayoutBlocks, "GenXLayoutBlocks", "GenXLayoutBlocks",
                    false, false)
namespace llvm {
  FunctionPass *createGenXLayoutBlocksPass() {
    initializeGenXLayoutBlocksPass(*PassRegistry::getPassRegistry());
    return new GenXLayoutBlocks;
  }
} // namespace llvm

/***********************************************************************
 * GenXLayoutBlocks::runOnFunction:
 *    reorder blocks to increase fallthrough,
 *    and specifically to satisfy the requirements of SIMD control flow
 */
bool GenXLayoutBlocks::runOnFunction(Function &F) {
  if (F.empty())
    return false;
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  if (LI.empty())
    LayoutBlocks(F);
  else
    LayoutBlocks(F, LI);
  return true;
}
