/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef LIB_GENXCODEGEN_DEBUG_INFO_H
#define LIB_GENXCODEGEN_DEBUG_INFO_H

#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Error.h>

#include "Probe/Assertion.h"

#include <unordered_map>
#include <map>

class VISAKernel;
class VISABuilder;

namespace llvm {

class Function;
class Instruction;
class FunctionGroup;
class GenXModule;

namespace genx {
namespace di {

struct VisaMapping {
  unsigned visaCounter = 0;

  struct Mapping {
    unsigned VisaIdx = 0;
    const Instruction *Inst = nullptr;
  };
  std::vector<Mapping> V2I;
};
} // namespace di
} // namespace genx

//--------------------------------------------------------------------
// Builds and holds the debug information for the current module
class GenXDebugInfo : public ModulePass {

  // NOTE: the term "program" is used to avoid a potential confusion
  // since the term "kernel" may introduce some ambiguity.
  // Here a "program" represents a kind of wrapper over a standalone vISA
  // object (which currently is produced by function groups and
  // visa-external functions) that finally gets compiled into a stand-alone
  // gen entity (binary gen kernel) with some auxiliary information
  struct ProgramInfo {
    struct FunctionInfo {
      const genx::di::VisaMapping &VisaMapping;
      VISAKernel &CompiledKernel;
      Function &F;
    };

    std::vector<FunctionInfo> FIs;
  };

  std::vector<llvm::DISubprogram*> DISubprogramNodes;

  using ElfBin = std::vector<char>;
  using DbgInfoStorage = std::unordered_map<const Function *, ElfBin>;
  DbgInfoStorage ElfOutputs;

  void cleanup();
  void processKernel(const ProgramInfo &PD);
  void processFunctionGroup(GenXModule &GM, VISABuilder &VB,
                            const FunctionGroup &FG);

public:
  static char ID;

  explicit GenXDebugInfo() : ModulePass(ID) {}
  ~GenXDebugInfo() { cleanup(); }

  StringRef getPassName() const override { return "GenX Debug Info"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
  void releaseMemory() override { cleanup(); }

  const DbgInfoStorage &getModuleDebug() const { return ElfOutputs; }
};

void initializeGenXDebugInfoPass(PassRegistry &);

} // namespace llvm

#endif // LIB_GENXCODEGEN_DEBUG_INFO_H
