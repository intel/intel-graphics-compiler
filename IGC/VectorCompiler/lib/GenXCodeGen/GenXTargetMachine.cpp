/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file defines the GenX specific subclass of TargetMachine.
//
/// Non-pass classes
/// ================
///
/// This section documents some GenX backend classes and abstractions that are not
/// in themselves passes, but are used by the passes.
///
/// .. include:: GenXAlignmentInfo.h
///
/// .. include:: GenXSubtarget.h
///
/// Pass documentation
/// ==================
///
/// The GenX backend runs the following passes on LLVM IR:
///
/// .. contents::
///    :local:
///    :depth: 1
///
//
//===----------------------------------------------------------------------===//

#include "GenXTargetMachine.h"

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXDebugInfo.h"
#include "GenXModule.h"

#include "vc/GenXCodeGen/GenXOCLRuntimeInfo.h"
#include "vc/GenXCodeGen/TargetMachine.h"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Support/PassManager.h"

#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvmWrapper/Support/TargetRegistry.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;

static cl::opt<std::string>
    FGDumpsPrefix("vc-fg-dump-prefix", cl::init(""), cl::Hidden,
                  cl::desc("prefix to use for FG dumps"));

static cl::opt<bool> EmitVLoadStore(
    "genx-emit-vldst", cl::init(true), cl::Hidden,
    cl::desc("Emit load/store intrinsic calls for pass-by-ref arguments"));

// There's another copy of DL string in clang/lib/Basic/Targets.cpp
static std::string getDL(bool Is64Bit) {
  return Is64Bit ? "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
                 : "e-p:32:32-p6:32:32-i64:64-n8:16:32";
}

namespace llvm {

//===----------------------------------------------------------------------===//
// This function is required to add GenX passes to opt tool
//===----------------------------------------------------------------------===//
void initializeGenXPasses(PassRegistry &registry) {
  initializeFunctionGroupAnalysisPass(registry);
  initializeGenXAddressCommoningWrapperPass(registry);
  initializeGenXArgIndirectionWrapperPass(registry);
  initializeGenXCategoryWrapperPass(registry);
  initializeGenXCFSimplificationPass(registry);
  initializeGenXCisaBuilderWrapperPass(registry);
  initializeGenXCoalescingWrapperPass(registry);
  initializeGenXDeadVectorRemovalPass(registry);
  initializeGenXDepressurizerWrapperPass(registry);
  initializeGenXEarlySimdCFConformancePass(registry);
  initializeGenXEmulationImportPass(registry);
  initializeGenXEmulatePass(registry);
  initializeGenXExtractVectorizerPass(registry);
  initializeGenXVectorCombinerPass(registry);
  initializeGenXFuncBalingPass(registry);
  initializeGenXGEPLoweringPass(registry);
  initializeGenXGroupBalingWrapperPass(registry);
  initializeGenXIMadPostLegalizationPass(registry);
  initializeGenXLateSimdCFConformanceWrapperPass(registry);
  initializeGenXLegalizationPass(registry);
  initializeGenXLiveRangesWrapperPass(registry);
  // initializeGenXLivenessWrapperPass(registry);
  initializeGenXLivenessWrapperPass(registry);
  initializeGenXLowerAggrCopiesPass(registry);
  initializeGenXLoweringPass(registry);
  initializeGenXModulePass(registry);
  initializeGenXNumberingWrapperPass(registry);
  initializeGenXPacketizePass(registry);
  initializeGenXPatternMatchPass(registry);
  initializeGenXPostLegalizationPass(registry);
  initializeGenXPrologEpilogInsertionPass(registry);
  initializeGenXPromotePredicatePass(registry);
  initializeGenXRawSendRipperPass(registry);
  initializeGenXReduceIntSizePass(registry);
  initializeGenXRegionCollapsingPass(registry);
  initializeGenXRematerializationWrapperPass(registry);
  initializeGenXThreadPrivateMemoryPass(registry);
  initializeGenXTidyControlFlowPass(registry);
  initializeGenXUnbalingWrapperPass(registry);
  initializeGenXVisaRegAllocWrapperPass(registry);
  initializeTransformPrivMemPass(registry);
  initializeGenXBackendConfigPass(registry);
  initializeGenXImportOCLBiFPass(registry);
  initializeGenXSimplifyPass(registry);
  initializeCMABIPass(registry);
  initializeCMLowerVLoadVStorePass(registry);
  initializeGenXLowerJmpTableSwitchPass(registry);
  initializeGenXGlobalValueLoweringPass(registry);
  initializeCMImpParamPass(registry);
  initializeCMKernelArgOffsetPass(registry);
  initializeGenXPrintfResolutionPass(registry);
  initializeGenXPrintfLegalizationPass(registry);
  initializeGenXAggregatePseudoLoweringPass(registry);
  initializeGenXBTIAssignmentPass(registry);
  initializeGenXPromoteStatefulToBindlessPass(registry);
  initializeGenXTranslateSPIRVBuiltinsPass(registry);
  initializeGenXLoadStoreLoweringPass(registry);
  initializeGenXStackUsagePass(registry);
  initializeGenXOCLRuntimeInfoPass(registry);
  initializeGenXStructSplitterPass(registry);
  initializeGenXCloneIndirectFunctionsPass(registry);
  initializeGenXTrampolineInsertionPass(registry);
  initializeGenXPredRegionLoweringPass(registry);
  initializeGenXLinkageCorruptorPass(registry);

  // WRITE HERE MORE PASSES IF IT'S NEEDED;
}

TargetTransformInfo GenXTargetMachine::getTargetTransformInfo(const Function &F) {
  GenXTTIImpl GTTI(F.getParent()->getDataLayout(), *BC);
  return TargetTransformInfo(GTTI);
}

} // namespace llvm

namespace {

class GenXPassConfig : public TargetPassConfig {
public:
  GenXPassConfig(GenXTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {
    // Cannot add INITIALIZE_PASS with needed dependencies because ID
    // is in parent TargetPassConfig class with its own initialization
    // routine.
    initializeGenXBackendConfigPass(*PassRegistry::getPassRegistry());
  }

  GenXTargetMachine &getGenXTargetMachine() const {
    return getTM<GenXTargetMachine>();
  }

  // PassConfig will always be available: in BE it is created inside
  // addPassesToEmitFile, opt creates it manually before adding other
  // passes. BackendConfig will be either created manually with
  // options structure or default-constructed using cl::opt values.
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXBackendConfig>();
    TargetPassConfig::getAnalysisUsage(AU);
  }

  // Should only be used after GenXPassConfig is added to PassManager.
  // Otherwise getAnalysis won't work.
  const GenXBackendConfig &getBackendConfig() const {
    return getAnalysis<GenXBackendConfig>();
  }
};

} // namespace

static std::unique_ptr<TargetLoweringObjectFile> createTLOF(const Triple &TT) {
  if (TT.isOSWindows())
    return std::make_unique<TargetLoweringObjectFileCOFF>();
  else
    return std::make_unique<TargetLoweringObjectFileELF>();
}

GenXTargetMachine::GenXTargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     Optional<Reloc::Model> RM,
                                     Optional<CodeModel::Model> CM,
                                     CodeGenOpt::Level OL, bool Is64Bit,
                                     std::unique_ptr<GenXBackendConfig> BC)
    : IGCLLVM::LLVMTargetMachine(T, getDL(Is64Bit), TT, CPU, FS, Options,
                                 RM ? RM.getValue() : Reloc::Model::Static,
                                 CM ? CM.getValue() : CodeModel::Model::Small,
                                 OL),
      TLOF(createTLOF(getTargetTriple())), BC(std::move(BC)), Is64Bit(Is64Bit),
      Subtarget(TT, CPU.str(), FS.str()) {}

GenXTargetMachine::~GenXTargetMachine() = default;

static GenXPassConfig *createGenXPassConfig(GenXTargetMachine &TM,
                                            PassManagerBase &PM) {
  return new GenXPassConfig(TM, PM);
}

TargetPassConfig *GenXTargetMachine::createPassConfig(PassManagerBase &PM) {
  return createGenXPassConfig(*this, PM);
}

GenXTargetMachine32::GenXTargetMachine32(const Target &T, const Triple &TT,
                                         StringRef CPU, StringRef FS,
                                         const TargetOptions &Options,
                                         Optional<Reloc::Model> RM,
                                         Optional<CodeModel::Model> CM,
                                         CodeGenOpt::Level OL, bool JIT,
                                         std::unique_ptr<GenXBackendConfig> BC)
    : GenXTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, false,
                        std::move(BC)) {}

GenXTargetMachine64::GenXTargetMachine64(const Target &T, const Triple &TT,
                                         StringRef CPU, StringRef FS,
                                         const TargetOptions &Options,
                                         Optional<Reloc::Model> RM,
                                         Optional<CodeModel::Model> CM,
                                         CodeGenOpt::Level OL, bool JIT,
                                         std::unique_ptr<GenXBackendConfig> BC)
    : GenXTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, true,
                        std::move(BC)) {}

namespace vc {
std::unique_ptr<llvm::TargetMachine>
createGenXTargetMachine(const Target &T, Triple TT, StringRef CPU,
                        StringRef Features, const TargetOptions &Options,
                        Optional<Reloc::Model> RM,
                        Optional<CodeModel::Model> CM, CodeGenOpt::Level OL,
                        std::unique_ptr<GenXBackendConfig> BC) {
  if (is32BitArch(TT))
    return std::make_unique<GenXTargetMachine32>(T, TT, CPU, Features, Options,
                                                 RM, CM, OL, false /*JIT*/,
                                                 std::move(BC));
  else
    return std::make_unique<GenXTargetMachine64>(T, TT, CPU, Features, Options,
                                                 RM, CM, OL, false /*JIT*/,
                                                 std::move(BC));
}
} // namespace vc

//===----------------------------------------------------------------------===//
//                       External Interface declaration
//===----------------------------------------------------------------------===//
extern "C" void LLVMInitializeGenXTarget() {
  // Register the target.
  RegisterTargetMachine<GenXTargetMachine32> X(getTheGenXTarget32());
  RegisterTargetMachine<GenXTargetMachine64> Y(getTheGenXTarget64());
}

extern "C" void LLVMInitializeGenXPasses() {
  llvm::initializeGenXPasses(*PassRegistry::getPassRegistry());
}

//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//
bool GenXTargetMachine::addPassesToEmitFile(PassManagerBase &PM,
                                            raw_pwrite_stream &o,
                                            raw_pwrite_stream * pi,
                                            CodeGenFileType FileType,
                                            bool DisableVerify,
                                            MachineModuleInfo *) {
  // We can consider the .isa file to be an object file, or an assembly file
  // which may later be converted to GenX code by the Finalizer. If we're
  // asked to produce any other type of file return true to indicate an error.
  if ((FileType != IGCLLVM::TargetMachine::CodeGenFileType::CGFT_ObjectFile) &&
      (FileType != IGCLLVM::TargetMachine::CodeGenFileType::CGFT_AssemblyFile))
    return true;

  GenXPassConfig *PassConfig = createGenXPassConfig(*this, PM);
  vc::addPass(PM, PassConfig);
  const GenXBackendConfig &BackendConfig = PassConfig->getBackendConfig();

  // Install GenX-specific TargetTransformInfo for passes such as
  // LowerAggrCopies and InfoAddressSpace
  vc::addPass(PM, createTargetTransformInfoWrapperPass(getTargetIRAnalysis()));

  if (BackendConfig.isBiFEmulationCompilation())
    vc::addPass(PM, createGenXEmulationModulePreparePass());

  vc::addPass(PM, createSROAPass());
  vc::addPass(PM, createEarlyCSEPass());
  vc::addPass(PM, createLowerExpectIntrinsicPass());
  vc::addPass(PM, createCFGSimplificationPass());
  vc::addPass(PM, createInstructionCombiningPass());

  vc::addPass(PM, createGlobalDCEPass());
  vc::addPass(PM, createGenXLowerAggrCopiesPass());
  vc::addPass(PM, createInferAddressSpacesPass());
  /// .. include:: GenXStructSplitter.cpp
  vc::addPass(PM, createGenXStructSplitterPass());
  /// DeadCodeElimination
  /// -------------------
  /// This is a standard LLVM pass, run at this point in the GenX backend. It
  /// removes code that has been made dead by other passes.
  ///
  vc::addPass(PM, createDeadCodeEliminationPass());
  vc::addPass(PM, createTransformPrivMemPass());
  vc::addPass(PM, createPromoteMemoryToRegisterPass());
  // All passes which modify the LLVM IR are now complete; run the verifier
  // to ensure that the IR is valid.
  if (!DisableVerify)
    vc::addPass(PM, createVerifierPass());
  // Run passes to generate vISA.

  /// BasicAliasAnalysis
  /// ------------------
  /// This is a standard LLVM analysis pass to provide basic AliasAnalysis
  /// support.
  vc::addPass(PM, createBasicAAWrapperPass());
  /// SROA
  /// ----
  /// This is a standard LLVM pass, used at this point in the GenX backend.
  /// Normally all alloca variables have been
  /// removed by now by earlier LLVM passes, unless ``-O0`` was specified.
  /// We run this pass here to cover that case.
  ///
  /// **IR restriction**: alloca, load, store not supported after this pass.
  ///
  vc::addPass(PM, createSROAPass());

  vc::addPass(PM, createGenXLowerJmpTableSwitchPass());
  /// LowerSwitch
  /// -----------
  /// This is a standard LLVM pass to lower a switch instruction to a chain of
  /// conditional branches.
  ///
  /// **IR restriction**: switch not supported after this pass.
  ///
  // TODO: keep some switch instructions and lower them to JMPSWITCH vISA ops.
  vc::addPass(PM, createLowerSwitchPass());
  /// .. include:: GenXCFSimplification.cpp
  vc::addPass(PM, createGenXCFSimplificationPass());
  /// CFGSimplification
  /// -----------------
  /// This is a standard LLVM pass, used at this point in the GenX backend.
  ///
  vc::addPass(PM, createCFGSimplificationPass());
  /// .. include:: GenXInlineAsmLowering.cpp
  vc::addPass(PM, createGenXInlineAsmLoweringPass());
  /// .. include:: GenXReduceIntSize.cpp
  vc::addPass(PM, createGenXReduceIntSizePass());
  /// .. include:: GenXGlobalValueLowering.cpp
  vc::addPass(PM, createGenXGlobalValueLoweringPass());

  /// .. include:: GenXStackUsage.cpp
  vc::addPass(PM, createGenXStackUsagePass());

  // PrologEpilog may emit memory instructions of illegal width.
  vc::addPass(PM, createGenXPrologEpilogInsertionPass());

  /// .. include:: GenXAggregatePseudoLowering.cpp
  /// GenXAggregatePseudoLowering must be run after
  /// GenXPrologEpilogInsertion as the latter may create stack loads and stores
  /// of aggregates.
  vc::addPass(PM, createGenXAggregatePseudoLoweringPass());

  /// .. include:: GenXGEPLowering.cpp
  /// GenXGEPLowering must be run before GenXThreadPrivateMemory and cannot be
  /// run earlier as GenXAggregatePseudoLowering may create GEPs.
  /// TODO: We run GenXGEPLowering twice: before GenXThreadPrivateMemory and
  /// before GenXLowering. It seems that after GenXThreadPrivateMemory removal
  /// we can remove this run of GenXGEPLowering.
  vc::addPass(PM, createGenXGEPLoweringPass());
  /// .. include:: GenXLoadStoreLowering.cpp
  vc::addPass(PM, createGenXLoadStoreLoweringPass());
  vc::addPass(PM, createGenXThreadPrivateMemoryPass());

  /// InstructionCombining
  /// --------------------
  /// This is a standard LLVM pass, used at this point in the GenX backend.
  /// Run instcombine after some lowering passes (e.g. GenXLoadStoreLowering) to
  /// make a cleanup.
  /// Cleans up after bunch of lowering passes such as
  /// GenXPrologEpilogInsertion, GenXAggregatePseudoLowering,
  /// GenXLoadStoreLowering.
  vc::addPass(PM, createInstructionCombiningPass());
  // Run integer reduction again to revert some trunc/ext patterns transformed
  // by instcombine.
  vc::addPass(PM, createGenXReduceIntSizePass());
  /// BreakCriticalEdges
  /// ------------------
  /// In the control flow graph, a critical edge is one from a basic block with
  /// multiple successors (a conditional branch) to a basic block with multiple
  /// predecessors.
  ///
  /// We use this standard LLVM pass to split such edges, to ensure that
  /// constant loader and GenXCoalescing have somewhere to insert a phi copy if
  /// needed.
  ///
  vc::addPass(PM, createBreakCriticalEdgesPass());
  /// .. include:: GenXSimdCFConformance.cpp
  vc::addPass(PM, createGenXEarlySimdCFConformancePass());
  /// .. include:: GenXPromotePredicate.cpp
  vc::addPass(PM, createGenXPromotePredicatePass());

  // Run GEP lowering again to remove possible GEPs after instcombine.
  vc::addPass(PM, createGenXGEPLoweringPass());
  /// .. include:: GenXLowering.cpp
  vc::addPass(PM, createGenXLoweringPass());
  if (!DisableVerify)
    vc::addPass(PM, createVerifierPass());
  /// .. include:: GenXRegionCollapsing.cpp
  vc::addPass(PM, createGenXRegionCollapsingPass());
  /// EarlyCSE
  /// --------
  /// This is a standard LLVM pass, run at this point in the GenX backend.
  /// It commons up common subexpressions, but only in the case that two common
  /// subexpressions are related by one dominating the other.
  ///
  vc::addPass(PM, createEarlyCSEPass());
  /// .. include:: GenXPatternMatch.cpp
  vc::addPass(PM, createGenXPatternMatchPass());
  if (!DisableVerify)
    vc::addPass(PM, createVerifierPass());
  /// .. include:: GenXExtractVectorizer.cpp
  vc::addPass(PM, createGenXExtractVectorizerPass());
  /// .. include:: GenXVectorCombiner.cpp
  vc::addPass(PM, createGenXVectorCombinerPass());
  /// .. include:: GenXRawSendRipper.cpp
  vc::addPass(PM, createGenXRawSendRipperPass());
  /// DeadCodeElimination
  /// -------------------
  /// This is a standard LLVM pass, run at this point in the GenX backend. It
  /// removes code that has been made dead by other passes.
  ///
  vc::addPass(PM, createDeadCodeEliminationPass());
  /// .. include:: GenXBaling.h
  vc::addPass(
      PM, createGenXFuncBalingPass(BalingKind::BK_Legalization, &Subtarget));

  /// .. include:: GenXLegalization.cpp
  vc::addPass(PM, createGenXLegalizationPass());

  // emulation BiF compilation mode stops here.
  if (BackendConfig.isBiFEmulationCompilation())
    return false;

  vc::addPass(PM, createGenXEmulationImportPass());
  /// .. include:: GenXEmulate.cpp
  vc::addPass(PM, createGenXEmulatePass());
  /// .. include:: GenXPromoteStatefulToBindless.cpp
  vc::addPass(PM, createGenXPromoteStatefulToBindlessPass());
  /// .. include:: GenXDeadVectorRemoval.cpp
  vc::addPass(PM, createGenXDeadVectorRemovalPass());
  /// DeadCodeElimination
  /// -------------------
  /// This is a standard LLVM pass, run at this point in the GenX backend. It
  /// removes code that has been made dead by other passes.
  ///
  vc::addPass(PM, createDeadCodeEliminationPass());
  /// .. include:: GenXPostLegalization.cpp
  /// .. include:: GenXConstants.cpp
  /// .. include:: GenXVectorDecomposer.h
  vc::addPass(PM, createGenXPostLegalizationPass());
  if (!DisableVerify)
    vc::addPass(PM, createVerifierPass());
  /// EarlyCSE
  /// --------
  /// This is a standard LLVM pass, run at this point in the GenX backend.
  /// It commons up common subexpressions, but only in the case that two common
  /// subexpressions are related by one dominating the other.
  ///
  vc::addPass(PM, createEarlyCSEPass());
  /// LICM
  /// ----
  /// This is a standard LLVM pass to hoist/sink the loop invariant code after
  /// legalization.
  vc::addPass(PM, createLICMPass());
  /// DeadCodeElimination
  /// -------------------
  /// This is a standard LLVM pass, run at this point in the GenX backend. It
  /// removes code that has been made dead by other passes.
  ///
  vc::addPass(PM, createDeadCodeEliminationPass());
  vc::addPass(PM, createGenXIMadPostLegalizationPass());
  /// GlobalDCE
  /// ---------
  /// This is a standard LLVM pass, run at this point in the GenX backend. It
  /// eliminates unreachable internal globals.
  ///
  vc::addPass(PM, createGlobalDCEPass());
  /// .. include:: GenXModule.h
  vc::addPass(PM, createGenXModulePass());
  /// .. include:: GenXLiveness.h
  vc::addPass(PM, createGenXGroupBalingWrapperPass(BalingKind::BK_Analysis,
                                                   &Subtarget));
  // GenXPredRegionLowering must be run after GenXLegalization and requires
  // updated baling analysis (and preserves it).
  vc::addPass(PM, createGenXPredRegionLoweringPass());
  vc::addPass(PM, createGenXLivenessWrapperPass());
  vc::addPass(PM, createGenXNumberingWrapperPass());
  vc::addPass(PM, createGenXLiveRangesWrapperPass());
  /// .. include:: GenXRematerialization.cpp
  vc::addPass(PM, createGenXRematerializationWrapperPass());
  /// .. include:: GenXCategory.cpp
  vc::addPass(PM, createGenXCategoryWrapperPass());
  /// Late SIMD CF conformance pass
  /// -----------------------------
  /// This is the same pass as GenXSimdCFConformance above, but run in a
  /// slightly different way. See above.
  ///
  /// **IR restriction**: After this pass, the EM values must have EM register
  /// category. The RM values must have RM register category. The !any result of
  /// a goto/join must have NONE register category.
  ///
  vc::addPass(PM, createGenXLateSimdCFConformanceWrapperPass());
  /// CodeGen baling pass
  /// -------------------
  /// This is the same pass as GenXBaling above, but run in a slightly different
  /// way. See above.
  ///
  /// **IR restriction**: Any pass after this needs to be careful when modifying
  /// code, as it also needs to update baling info.
  ///
  vc::addPass(
      PM, createGenXGroupBalingWrapperPass(BalingKind::BK_CodeGen, &Subtarget));

  /// .. include:: GenXNumbering.h
  vc::addPass(PM, createGenXNumberingWrapperPass());
  /// .. include:: GenXLiveRanges.cpp
  vc::addPass(PM, createGenXLiveRangesWrapperPass());
  /// .. include:: GenXUnbaling.cpp
  vc::addPass(PM, createGenXUnbalingWrapperPass());
  /// .. include:: GenXDepressurizer.cpp
  vc::addPass(PM, createGenXDepressurizerWrapperPass());
  /// .. include:: GenXNumbering.h
  vc::addPass(PM, createGenXNumberingWrapperPass());
  /// .. include:: GenXLiveRanges.cpp
  vc::addPass(PM, createGenXLiveRangesWrapperPass());
  /// .. include:: GenXCoalescing.cpp
  vc::addPass(PM, createGenXCoalescingWrapperPass());
  /// .. include:: GenXAddressCommoning.cpp
  vc::addPass(PM, createGenXAddressCommoningWrapperPass());
  /// .. include:: GenXArgIndirection.cpp
  vc::addPass(PM, createGenXArgIndirectionWrapperPass());
  /// .. include:: GenXTidyControlFlow.cpp
  vc::addPass(PM, createGenXTidyControlFlowPass());
  /// .. include:: GenXVisaRegAlloc.h
  auto RegAlloc = createGenXVisaRegAllocWrapperPass();
  vc::addPass(PM, RegAlloc);
  if (BackendConfig.enableRegAllocDump())
    vc::addPass(PM, createGenXModuleAnalysisDumperPass(RegAlloc, FGDumpsPrefix,
                                                       ".regalloc"));
  if (!DisableVerify)
    vc::addPass(PM, createVerifierPass());
  /// .. include:: GenXCisaBuilder.cpp
  vc::addPass(PM, createGenXCisaBuilderWrapperPass());
  vc::addPass(PM, createGenXFinalizerPass(o));
  vc::addPass(PM, createGenXDebugInfoPass());

  // Analysis for collecting information related to OCL runtime. Can
  // be used by external caller by adding extractor pass in the end of
  // compilation pipeline.
  // Explicit construction can be omitted because adding of extractor
  // pass will create runtime info analysis. Leaving it exlicit for
  // clarity.
  if (Subtarget.isOCLRuntime())
    vc::addPass(PM, new GenXOCLRuntimeInfo());

  return false;
}

void GenXTargetMachine::adjustPassManager(PassManagerBuilder &PMBuilder) {
  // Lower aggr copies.
  PMBuilder.addExtension(
      PassManagerBuilder::EP_EarlyAsPossible,
      [](const PassManagerBuilder &Builder, PassManagerBase &PM) {
        PM.add(createGenXLowerAggrCopiesPass());
      });

  // Packetize.
  auto AddPacketize = [](const PassManagerBuilder &Builder,
                         PassManagerBase &PM) {
    PM.add(createGenXTranslateSPIRVBuiltinsPass());
    PM.add(createAlwaysInlinerLegacyPass());
    PM.add(createGenXPrintfResolutionPass());
    PM.add(createGenXImportOCLBiFPass());
    PM.add(createGenXPacketizePass());
    PM.add(createAlwaysInlinerLegacyPass());
    PM.add(createGenXPrintfLegalizationPass());
    PM.add(createGlobalDCEPass());
    PM.add(createPromoteMemoryToRegisterPass());
    PM.add(createInferAddressSpacesPass());
    PM.add(createEarlyCSEPass(true));
    PM.add(createCFGSimplificationPass());
    PM.add(createInstructionCombiningPass());
    PM.add(createDeadCodeEliminationPass());
    PM.add(createSROAPass());
    PM.add(createInferAddressSpacesPass());
    PM.add(createEarlyCSEPass(true));
    PM.add(createCFGSimplificationPass());
    PM.add(createInstructionCombiningPass());
    PM.add(createDeadCodeEliminationPass());
    // PM.add(createGlobalDCEPass());
  };
  PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly,
                         AddPacketize);
  PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                         AddPacketize);

  // vldst.
  if (EmitVLoadStore) {
    auto AddLowerLoadStore = [](const PassManagerBuilder &Builder,
                                PassManagerBase &PM) {
      if (Builder.OptLevel > 0) {
        // Inline
        PM.add(createSROAPass());
        PM.add(createEarlyCSEPass());
        PM.add(createJumpThreadingPass());
        PM.add(createCFGSimplificationPass());
        PM.add(createCorrelatedValuePropagationPass());
        PM.add(createGenXReduceIntSizePass());
        PM.add(createInstructionCombiningPass());
        PM.add(createAlwaysInlinerLegacyPass());
        PM.add(createGlobalDCEPass());
        PM.add(createInstructionCombiningPass());
        // Unroll
        PM.add(createCFGSimplificationPass());
        PM.add(createReassociatePass());
        PM.add(createLoopRotatePass());
        PM.add(createLICMPass());
        PM.add(createInstructionCombiningPass());
        PM.add(createIndVarSimplifyPass());
        PM.add(createLoopIdiomPass());
        PM.add(createLoopDeletionPass());
        PM.add(createSimpleLoopUnrollPass());
        PM.add(createInstructionCombiningPass());
        // Simplify region accesses.
        PM.add(createGenXRegionCollapsingPass());
        PM.add(createEarlyCSEPass());
        PM.add(createDeadCodeEliminationPass());
      }
      PM.add(createCMLowerVLoadVStorePass());
    };
    PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly,
                           AddLowerLoadStore);
    PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                           AddLowerLoadStore);
  }

  if (Subtarget.isOCLRuntime()) {
    auto AddIndirect = [](const PassManagerBuilder &Builder,
                          PassManagerBase &PM) {
      PM.add(createGenXCloneIndirectFunctionsPass());
      PM.add(createGenXTrampolineInsertionPass());
    };
    PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly,
                           AddIndirect);
    PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                           AddIndirect);
  }

  // Have to internalize functions before CM implicit parameters as all
  // implicit parameters cannot be supported at once for external functions
  // and some frontends already depend on them working as the linkage is
  // internal.
  // FIXME: move back to CMABI or better remove this pass.
  auto AddLinkageCorruptor = [](const PassManagerBuilder &Builder,
                                PassManagerBase &PM) {
    PM.add(createGenXLinkageCorruptorPass());
  };
  PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly,
                         AddLinkageCorruptor);
  PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                         AddLinkageCorruptor);

  // CM implicit parameters.
  auto AddCMImpParam = [this](const PassManagerBuilder &Builder,
                              PassManagerBase &PM) {
    PM.add(createCMImpParamPass(!Subtarget.isOCLRuntime(),
                                Subtarget.hasThreadPayloadInMemory()));
  };
  PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly,
                         AddCMImpParam);
  PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                         AddCMImpParam);

  // CM ABI.
  auto AddCMABI = [](const PassManagerBuilder &Builder, PassManagerBase &PM) {
    PM.add(createCMABIPass());
  };
  PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly, AddCMABI);
  PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0, AddCMABI);

  // BTI assignment.
  if (Subtarget.isOCLRuntime()) {
    auto AddBTIAssign = [](const PassManagerBuilder &Builder,
                           PassManagerBase &PM) {
      PM.add(createGenXBTIAssignmentPass());
    };
    PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly,
                           AddBTIAssign);
    PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                           AddBTIAssign);
  }

  // CM kernel argument offset.
  auto AddCMKernelArgOffset = [this](const PassManagerBuilder &Builder,
                                     PassManagerBase &PM) {
    PM.add(createCMKernelArgOffsetPass(Subtarget.getGRFByteSize(),
                                       Subtarget.isOCLRuntime()));
  };
  PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly,
                         AddCMKernelArgOffset);
  PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                         AddCMKernelArgOffset);

  auto AddGenXPeephole = [](const PassManagerBuilder &Builder,
                            PassManagerBase &PM) {
    PM.add(createGenXSimplifyPass());
  };
  PMBuilder.addExtension(PassManagerBuilder::EP_Peephole, AddGenXPeephole);
}
