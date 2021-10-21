/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

class GenXTargetMachine : public IGCLLVM::LLVMTargetMachine {
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

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  const TargetSubtargetInfo *getSubtargetImpl(const Function &) const override {
    return &Subtarget;
  }
  TargetTransformInfo getTargetTransformInfo(const Function &F) override;

  const GenXSubtarget &getGenXSubtarget() const { return Subtarget; }
};

class GenXTargetMachine32 : public GenXTargetMachine {
public:
  GenXTargetMachine32(const Target &T, const Triple &TT, StringRef CPU,
                      StringRef FS, const TargetOptions &Options,
                      Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                      CodeGenOpt::Level OL, bool JIT);
};

class GenXTargetMachine64 : public GenXTargetMachine {
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

#if LLVM_VERSION_MAJOR >= 13
  InstructionCost
#else
  int
#endif
  getUserCost(const User *U, ArrayRef<const Value *> Operands
#if LLVM_VERSION_MAJOR >= 11
                  ,
                  TTI::TargetCostKind CostKind
#endif
  ) {
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

    return BaseT::getUserCost(U, Operands
#if LLVM_VERSION_MAJOR >= 11
                              ,
                              CostKind
#endif
    );
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
void initializeGenXAddressCommoningWrapperPass(PassRegistry &);
void initializeGenXArgIndirectionWrapperPass(PassRegistry &);
void initializeGenXCategoryWrapperPass(PassRegistry &);
void initializeGenXCFSimplificationPass(PassRegistry &);
void initializeGenXCisaBuilderWrapperPass(PassRegistry &);
void initializeGenXCoalescingWrapperPass(PassRegistry &);
void initializeGenXDeadVectorRemovalPass(PassRegistry &);
void initializeGenXDepressurizerWrapperPass(PassRegistry &);
void initializeGenXEarlySimdCFConformancePass(PassRegistry &);
void initializeGenXEmulationImportPass(PassRegistry &);
void initializeGenXEmulatePass(PassRegistry &);
void initializeGenXExtractVectorizerPass(PassRegistry &);
void initializeGenXFuncBalingPass(PassRegistry &);
void initializeGenXGEPLoweringPass(PassRegistry &);
void initializeGenXGroupBalingWrapperPass(PassRegistry &);
void initializeGenXInstCombineCleanup(PassRegistry &);
void initializeGenXIMadPostLegalizationPass(PassRegistry &);
void initializeGenXLateSimdCFConformanceWrapperPass(PassRegistry &);
void initializeGenXLegalizationPass(PassRegistry &);
void initializeGenXLiveRangesWrapperPass(PassRegistry &);
void initializeGenXLivenessWrapperPass(PassRegistry &);
void initializeGenXLoadStoreLoweringPass(PassRegistry &);
void initializeGenXLowerAggrCopiesPass(PassRegistry &);
void initializeGenXLoweringPass(PassRegistry &);
void initializeGenXModulePass(PassRegistry &);
void initializeGenXNumberingWrapperPass(PassRegistry &);
void initializeGenXPacketizePass(PassRegistry &);
void initializeGenXPatternMatchPass(PassRegistry &);
void initializeGenXPostLegalizationPass(PassRegistry &);
void initializeGenXPostLegalizationPass(PassRegistry &);
void initializeGenXPrologEpilogInsertionPass(PassRegistry &);
void initializeGenXPromotePredicatePass(PassRegistry &);
void initializeGenXRawSendRipperPass(PassRegistry &);
void initializeGenXReduceIntSizePass(PassRegistry &);
void initializeGenXRegionCollapsingPass(PassRegistry &);
void initializeGenXRematerializationWrapperPass(PassRegistry &);
void initializeGenXThreadPrivateMemoryPass(PassRegistry &);
void initializeGenXTidyControlFlowPass(PassRegistry &);
void initializeGenXUnbalingWrapperPass(PassRegistry &);
void initializeGenXVisaRegAllocWrapperPass(PassRegistry &);
void initializeTransformPrivMemPass(PassRegistry &);
void initializeGenXFunctionPointersLoweringPass(PassRegistry &);
void initializeGenXLowerJmpTableSwitchPass(PassRegistry &);
void initializeGenXGlobalValueLoweringPass(PassRegistry &);
void initializeGenXAggregatePseudoLoweringPass(PassRegistry &);
void initializeGenXVectorCombinerPass(PassRegistry &);
void initializeGenXPromoteStatefulToBindlessPass(PassRegistry &);
void initializeGenXStackUsagePass(PassRegistry &);
void initializeCMLowerVLoadVStorePass(PassRegistry &);
void initializeGenXStructSplitterPass(PassRegistry &);
} // End llvm namespace

#endif
