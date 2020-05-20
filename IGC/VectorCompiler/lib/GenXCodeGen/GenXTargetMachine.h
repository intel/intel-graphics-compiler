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
// This file declares the TargetMachine that is used by the GenX backend.
//
// Unlike a normal CPU backend, the GenX backend does not use CodeGen (the
// LLVM target independent code generator).
//
//===----------------------------------------------------------------------===//

#ifndef GENXTARGETMACHINE_H
#define GENXTARGETMACHINE_H

#include "llvmWrapper/Target/TargetMachine.h"

#include "GenXIntrinsics.h"
#include "GenXSubtarget.h"
#include "TargetInfo/GenXTargetInfo.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/CodeGen/BasicTTIImpl.h"

namespace llvm {

class raw_pwrite_stream;
class MachineModuleInfo;

class GenXTargetMachine : public IGCLLVM::TargetMachine {
  bool Is64Bit;
  GenXSubtarget Subtarget;

public:
  GenXTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                    StringRef FS, const TargetOptions &Options,
                    Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                    CodeGenOpt::Level OL, bool Is64Bit);

  ~GenXTargetMachine() override;

  bool addPassesToEmitFile(PassManagerBase &PM, raw_pwrite_stream &o, raw_pwrite_stream *pi,
                           CodeGenFileType FileType,
                           bool /*DisableVerify*/ = true,
                           MachineModuleInfo *MMI = nullptr) override;

  void adjustPassManager(PassManagerBuilder &PMBuilder) override;

  virtual const DataLayout *getDataLayout() const { return &DL; }

  virtual const TargetSubtargetInfo *getSubtargetImpl(const Function &) const override {
    return &Subtarget;
  }
  TargetTransformInfo getTargetTransformInfo(const Function &F) override;
};

class GenXTargetMachine32 : public GenXTargetMachine {
  virtual void anchor();

public:
  GenXTargetMachine32(const Target &T, const Triple &TT, StringRef CPU,
                      StringRef FS, const TargetOptions &Options,
                      Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                      CodeGenOpt::Level OL, bool JIT);
};

class GenXTargetMachine64 : public GenXTargetMachine {
  virtual void anchor();

public:
  GenXTargetMachine64(const Target &T, const Triple &TT, StringRef CPU,
                      StringRef FS, const TargetOptions &Options,
                      Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                      CodeGenOpt::Level OL, bool JIT);
};

// This implementation allows us to define our own costs for
// the GenX backend. Did not use BasicTTIImplBase because the overloaded
// constructors have TragetMachine as an argument, so I inherited from
// its parent which has only DL as its arguments
class GenXTTIImpl : public TargetTransformInfoImplCRTPBase<GenXTTIImpl>
{
  typedef TargetTransformInfoImplCRTPBase<GenXTTIImpl> BaseT;
  typedef TargetTransformInfo TTI;
  friend BaseT;
public:
  GenXTTIImpl(const DataLayout& DL) : BaseT(DL) {}

  bool shouldBuildLookupTables() { return false; }
  unsigned getFlatAddressSpace() { return 4; }

  int getUserCost(const User *U, ArrayRef<const Value *> Operands) {
    if (auto EV = dyn_cast<ExtractValueInst>(U)) {
      switch(GenXIntrinsic::getGenXIntrinsicID(EV->getOperand(0))) {
        case GenXIntrinsic::genx_simdcf_goto:
        case GenXIntrinsic::genx_simdcf_join:
          // Do not allow such EVs to be TCC_Free
          return TTI::TCC_Basic;
        default:
          break;
      }
    }

    return BaseT::getUserCost(U, Operands);
  }

  bool isProfitableToHoist(Instruction *I) const {
    // genx_vload and genx_vstore are related to g_store bales
    // and they shouldn't be hoisted from then/else blocks
    // in front of the branch
    auto IntrinsicID = GenXIntrinsic::getGenXIntrinsicID(I);
    return IntrinsicID != GenXIntrinsic::genx_vload &&
           IntrinsicID != GenXIntrinsic::genx_vstore;
  }
};

/// Initialize all GenX passes for opt tool.
void initializeGenXPasses(PassRegistry &);

void initializeFunctionGroupAnalysisPass(PassRegistry &);
void initializeGenXAddressCommoningPass(PassRegistry &);
void initializeGenXArgIndirectionPass(PassRegistry &);
void initializeGenXCategoryPass(PassRegistry &);
void initializeGenXCFSimplificationPass(PassRegistry &);
void initializeGenXCisaBuilderPass(PassRegistry &);
void initializeGenXCoalescingPass(PassRegistry &);
void initializeGenXDeadVectorRemovalPass(PassRegistry &);
void initializeGenXDepressurizerPass(PassRegistry &);
void initializeGenXEarlySimdCFConformancePass(PassRegistry &);
void initializeGenXEmulatePass(PassRegistry &);
void initializeGenXExtractVectorizerPass(PassRegistry &);
void initializeGenXFuncBalingPass(PassRegistry &);
void initializeGenXGEPLoweringPass(PassRegistry &);
void initializeGenXGroupBalingPass(PassRegistry &);
void initializeGenXInstCombineCleanup(PassRegistry &);
void initializeGenXIMadPostLegalizationPass(PassRegistry &);
void initializeGenXLateSimdCFConformancePass(PassRegistry &);
void initializeGenXLayoutBlocksPass(PassRegistry &);
void initializeGenXLegalizationPass(PassRegistry &);
void initializeGenXLiveRangesPass(PassRegistry &);
void initializeGenXLivenessPass(PassRegistry &);
void initializeGenXLowerAggrCopiesPass(PassRegistry &);
void initializeGenXLoweringPass(PassRegistry &);
void initializeGenXModulePass(PassRegistry &);
void initializeGenXNumberingPass(PassRegistry &);
void initializeGenXPatternMatchPass(PassRegistry &);
void initializeGenXPostLegalizationPass(PassRegistry &);
void initializeGenXPostLegalizationPass(PassRegistry &);
void initializeGenXPromotePredicatePass(PassRegistry &);
void initializeGenXRawSendRipperPass(PassRegistry &);
void initializeGenXReduceIntSizePass(PassRegistry &);
void initializeGenXRegionCollapsingPass(PassRegistry &);
void initializeGenXRematerializationPass(PassRegistry &);
void initializeGenXSubtargetPassPass(PassRegistry &);
void initializeGenXThreadPrivateMemoryPass(PassRegistry &);
void initializeGenXUnbalingPass(PassRegistry &);
void initializeGenXVisaRegAllocPass(PassRegistry &);
void initializeTransformPrivMemPass(PassRegistry &);
void initializeGenXFunctionPointersLoweringPass(PassRegistry &);
} // End llvm namespace

#endif
