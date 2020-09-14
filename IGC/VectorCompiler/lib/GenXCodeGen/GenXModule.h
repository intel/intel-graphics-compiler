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
/// GenXModule
/// ----------
///
/// GenXModule is a module pass whose purpose is to store information
/// about the module being written, such as the built kernels and functions.
///
/// A vISA kernel or function can call a *subroutine*, which can
/// then call further subroutines. All called subroutines are considered part of
/// the kernel or function, which means that a subroutine used by two different
/// kernels needs to have a copy in each. The two copies may be treated
/// differently by the backend passes, so there does actually need to be two
/// copies of the subroutine in the LLVM IR in the backend, one called by each
/// kernel.
///
/// The GenXModule pass performs any necessary copying of subroutines, and
/// populates FunctionGroupAnalysis such that each kernel and its subroutines
/// make one FunctionGroup.
///
/// Subsequent passes are mostly FunctionGroupPasses, so they process one
/// FunctionGroup at a time.
///
/// GenXModule is also an analysis, preserved through subsequent passes to
/// GenXFinalizer at the end, that is used to store each written vISA kernel.
///
/// **IR restriction**: After this pass, the lead function in a FunctionGroup is
/// a kernel (or function in the vISA sense), and other functions in the same
/// FunctionGroup are its subroutines.  A (non-intrinsic) call must be to a
/// function in the same FunctionGroup, and not the lead function.
///
//===----------------------------------------------------------------------===//
#ifndef GENXMODULE_H
#define GENXMODULE_H

#include "GenX.h"
#include "GenXBaling.h"
#include "GenXDebugInfo.h"

#include "llvm/ADT/Twine.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"

#include <inc/common/sku_wa.h>

#include <map>
#include <string>
#include <vector>
#include "Probe/Assertion.h"

class VISABuilder;
class VISAKernel;

namespace llvm {
  class GenXSubtarget;

  //--------------------------------------------------------------------
  // GenXModule pass. Stores the information from various parts of the
  // GenX writing process
  class GenXModule : public ModulePass {
    const GenXSubtarget *ST;
    LLVMContext *Ctx = nullptr;
    WA_TABLE *WaTable = nullptr;

    // Visa option parser contains code that just stores c-strings as
    // pointers without copying. Store all strings here.
    BumpPtrAllocator ArgStorage;
    bool AsmDumpsEnabled = false;

    VISABuilder *CisaBuilder = nullptr;
    void InitCISABuilder();

    VISABuilder *VISAAsmTextReader = nullptr;
    void InitVISAAsmReader();

    bool InlineAsm = false;
    bool CheckForInlineAsm(Module &M) const;

    std::map<const Function *, genx::VisaDebugInfo> VisaDebugMap;

  private:
    void cleanup() {
      DestroyCISABuilder();
      DestroyVISAAsmReader();
      ArgStorage.Reset();
    }

  public:
    static char ID;

    explicit GenXModule() : ModulePass(ID) {}
    ~GenXModule() { cleanup(); }

    StringRef getPassName() const override { return "GenX module"; }
    void getAnalysisUsage(AnalysisUsage &AU) const override;
    bool runOnModule(Module &M) override;
    void releaseMemory() override { cleanup(); }

    const GenXSubtarget *getSubtarget() const { return ST; }
    bool HasInlineAsm() const { return InlineAsm; }
    VISABuilder *GetCisaBuilder();
    VISABuilder *GetVISAAsmReader();
    void DestroyCISABuilder();
    void DestroyVISAAsmReader();
    LLVMContext &getContext();

    void updateVisaDebugInfo(const Function *F, const Instruction *Inst);
    const genx::VisaDebugInfo *getVisaDebugInfo(const Function *F) const;
  };

  void initializeGenXModulePass(PassRegistry &);

} // end namespace llvm
#endif // ndef GENXMODULE_H
