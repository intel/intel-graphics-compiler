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

#include "GenX.h"
#include "GenXOCLRuntimeInfo.h"
#include "llvm/Pass.h"

using namespace llvm;

namespace llvm {
void initializeGenXOCLInfoExtractorPass(PassRegistry &PR);
}

class GenXOCLInfoExtractor : public ModulePass {
public:
  static char ID;

private:
  std::vector<GenXOCLRuntimeInfo::CompiledKernel> *Dest = nullptr;

public:
  StringRef getPassName() const override { return "GenX OCL Info Extractor"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXOCLRuntimeInfo>();
  }

  GenXOCLInfoExtractor() : ModulePass(ID) {}

  GenXOCLInfoExtractor(std::vector<GenXOCLRuntimeInfo::CompiledKernel> &Dst)
      : ModulePass(ID), Dest(&Dst) {
    initializeGenXOCLInfoExtractorPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) override {
    assert(Dest && "Expected dest to be initialized");
    auto &Info = getAnalysis<GenXOCLRuntimeInfo>();
    *Dest = Info.stealCompiledKernels();
    return false;
  }
};

char GenXOCLInfoExtractor::ID = 0;

INITIALIZE_PASS_BEGIN(GenXOCLInfoExtractor, "GenXOCLInfoExtractor",
                      "GenXOCLInfoExtractor", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXOCLRuntimeInfo)
INITIALIZE_PASS_END(GenXOCLInfoExtractor, "GenXOCLInfoExtractor",
                    "GenXOCLInfoExtractor", false, false)

ModulePass *llvm::createGenXOCLInfoExtractorPass(
    std::vector<GenXOCLRuntimeInfo::CompiledKernel> &Dest) {
  return new GenXOCLInfoExtractor(Dest);
}
