/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

#include "vc/Support/BackendConfig.h"

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
    const GenXSubtarget *ST = nullptr;
    LLVMContext *Ctx = nullptr;
    const GenXBackendConfig *BC = nullptr;

    // Visa option parser contains code that just stores c-strings as
    // pointers without copying. Store all strings here.
    BumpPtrAllocator ArgStorage;

    VISABuilder *CisaBuilder = nullptr;
    void InitCISABuilder();

    VISABuilder *VISAAsmTextReader = nullptr;
    void InitVISAAsmReader();

    bool InlineAsm = false;
    bool CheckForInlineAsm(Module &M) const;

    bool DisableFinalizerOpts = false;
    bool EmitDebugInformation = false;
    bool ImplicitArgsBufferIsUsed = false;
    // represents number of visa instructions in a *kernel*
    std::unordered_map<const Function *, unsigned> VisaCounter;
    // stores vISA mappings for each *function* (including kernel subroutines)
    std::unordered_map<const Function *, genx::di::VisaMapping> VisaMapping;

  private:
    void cleanup() {
      VisaMapping.clear();
      VisaCounter.clear();
      DestroyCISABuilder();
      DestroyVISAAsmReader();
      ArgStorage.Reset();
    }

  public:
    static char ID;

    // Additional info requred to create VISABuilder.
    struct InfoForFinalizer final {
      bool DisableFinalizerOpts = false;
      bool EmitDebugInformation = false;
      bool EmitCrossThreadOffsetRelocation = false;
    };

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

    bool emitDebugInformation() const { return EmitDebugInformation; }
    void updateVisaMapping(const Function *F, const Instruction *Inst,
                           unsigned VisaIndex, StringRef Reason);
    void updateVisaCountMapping(const Function *F, const Instruction *Inst,
                                unsigned VisaIndex, StringRef Reason);
    const genx::di::VisaMapping *getVisaMapping(const Function *F) const;
    // Returns additional info requred to create VISABuilder.
    // Subtarget must be already initialized before calling this method.
    InfoForFinalizer getInfoForFinalizer() const;
  };

  void initializeGenXModulePass(PassRegistry &);

} // end namespace llvm
#endif // ndef GENXMODULE_H
