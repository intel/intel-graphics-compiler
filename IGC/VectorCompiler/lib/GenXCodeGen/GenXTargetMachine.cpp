/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

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

#include "vc/GenXCodeGen/GenXFixInvalidFuncName.h"
#include "vc/GenXCodeGen/GenXLowerAggrCopies.h"
#include "vc/GenXCodeGen/GenXOCLRuntimeInfo.h"
#include "vc/GenXCodeGen/GenXReduceIntSize.h"
#include "vc/GenXCodeGen/GenXRegionCollapsing.h"
#include "vc/GenXCodeGen/GenXVerify.h"
#include "vc/GenXCodeGen/TargetMachine.h"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Support/PassManager.h"

#include "llvmWrapper/ADT/Optional.h"
#include "llvmWrapper/Support/TargetRegistry.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/Annotation2Metadata.h"
#include "llvm/Transforms/IPO/ForceFunctionAttrs.h"
#include "llvm/Transforms/IPO/GlobalDCE.h"
#include "llvm/Transforms/IPO/InferFunctionAttrs.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/CorrelatedValuePropagation.h"
#include "llvm/Transforms/Scalar/DCE.h"
#include "llvm/Transforms/Scalar/EarlyCSE.h"
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Scalar/InferAddressSpaces.h"
#include "llvm/Transforms/Scalar/JumpThreading.h"
#include "llvm/Transforms/Scalar/LICM.h"
#include "llvm/Transforms/Scalar/LoopDeletion.h"
#include "llvm/Transforms/Scalar/LoopIdiomRecognize.h"
#include "llvm/Transforms/Scalar/LoopRotation.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/Transforms/Scalar/LowerExpectIntrinsic.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SROA.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

using namespace llvm;

static cl::opt<std::string>
    FGDumpsPrefix("vc-fg-dump-prefix", cl::init(""), cl::Hidden,
                  cl::desc("prefix to use for FG dumps"));

static cl::opt<bool>
    SkipOCLRuntimeInfo("vc-skip-ocl-runtime-info", cl::init(false), cl::Hidden,
                  cl::desc("skip GenXOCLRuntimeInfo in addPassesToEmitFile for llc tests"));

static cl::opt<bool> EmitVLoadStore(
    "genx-emit-vldst", cl::init(true), cl::Hidden,
    cl::desc("Emit load/store intrinsic calls for pass-by-ref arguments"));

static cl::opt<bool> PeelLoopsDpasNullAcc(
    "vc-peel-loops-dpas-null-acc", cl::init(true), cl::Hidden,
    cl::desc("Peel first iteration of a loop with dpas instructions, when dpas "
             "accumulator operand is zero-initialized"));
static cl::opt<unsigned> PeelLoopDpasNullAccMaxBlocks(
    "vc-peel-loops-dpas-null-acc-max-blocks", cl::init(16), cl::Hidden,
    cl::desc("Max number of a loop basic blocks to peel, when the loop has "
             "dpas instructions with zero-initialized accumulator operand"));
static cl::opt<unsigned> PeelLoopDpasNullAccMaxInstr(
    "vc-peel-loops-dpas-null-acc-max-instr", cl::init(192), cl::Hidden,
    cl::desc("Max number of a loop instructions to peel, when the loop has "
             "dpas instructions with zero-initialized accumulator operand"));
static cl::opt<unsigned> PeelLoopDpasNullAccMin(
    "vc-peel-loops-dpas-null-acc-min-dpas-instr", cl::init(4), cl::Hidden,
    cl::desc("Minimal number of dpas instructions with zero-initialized "
             "accumulator operand to peel the loop with first iteration"));

// There's another copy of DL string in clang/lib/Basic/Targets.cpp
static std::string getDL(bool Is64Bit) {
  return Is64Bit ? "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
                 : "e-p:32:32-p3:32:32-p6:32:32-i64:64-n8:16:32:64";
}

namespace {
bool isDpasAccumulator(const Value *V) {
  if (!V->hasOneUse())
    return false;

  const auto *User = V->user_back();
  const auto IID = vc::getAnyIntrinsicID(User);

  if (IID != GenXIntrinsic::genx_dpas && IID != GenXIntrinsic::genx_dpas2)
    return false;

  const auto *CI = cast<CallInst>(User);
  return CI->getArgOperand(0) == V;
}
} // namespace

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
  initializeGenXCisaBuilderPass(registry);
  initializeGenXCoalescingWrapperPass(registry);
  initializeGenXGVClobberCheckerPass(registry);
  initializeGenXVerifyPass(registry);
  initializeGenXDeadVectorRemovalPass(registry);
  initializeGenXDepressurizerWrapperPass(registry);
  initializeGenXEarlySimdCFConformancePass(registry);
  initializeGenXEmulatePass(registry);
  initializeGenXExtractVectorizerPass(registry);
  initializeGenXVectorCombinerPass(registry);
  initializeGenXFuncBalingPass(registry);
  initializeGenXFuncLiveElementsPass(registry);
  initializeGenXGEPLoweringPass(registry);
  initializeGenXGroupBalingWrapperPass(registry);
  initializeGenXGroupLiveElementsWrapperPass(registry);
  initializeGenXIMadPostLegalizationPass(registry);
  initializeGenXLateSimdCFConformanceWrapperPass(registry);
  initializeGenXLegalizationPass(registry);
  initializeGenXLiveRangesWrapperPass(registry);
  // initializeGenXLivenessWrapperPass(registry);
  initializeGenXLivenessWrapperPass(registry);
  initializeGenXLowerAggrCopiesPass(registry);
  initializeGenXBFloatLoweringPass(registry);
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
  initializeGenXTidyControlFlowPass(registry);
  initializeGenXUnbalingWrapperPass(registry);
  initializeGenXVisaRegAllocWrapperPass(registry);
  initializeGenXPromoteArrayPass(registry);
  initializeGenXBackendConfigPass(registry);
  initializeGenXImportOCLBiFPass(registry);
  initializeGenXBIFFlagCtrlResolutionPass(registry);
  initializeGenXSimplifyPass(registry);
#if LLVM_VERSION_MAJOR < 16
  initializeCMABILegacyPass(registry);
#else  // LLVM_VERSION_MAJOR < 16
  initializeCMABILegacyPass(registry);
  initializeCMABIPass(registry);
#endif // LLVM_VERSION_MAJOR < 16
  initializeCMLowerVLoadVStorePass(registry);
  initializeGenXLowerJmpTableSwitchPass(registry);
  initializeGenXGlobalValueLoweringPass(registry);
  initializeCMImpParamPass(registry);
  initializeCMKernelArgOffsetPass(registry);
  initializeGenXPrintfPhiClonningPass(registry);
  initializeGenXPrintfResolutionPass(registry);
  initializeGenXPrintfLegalizationPass(registry);
  initializeGenXAggregatePseudoLoweringPass(registry);
  initializeGenXBTIAssignmentPass(registry);
  initializeGenXPromoteStatefulToBindlessPass(registry);
  initializeGenXTranslateSPIRVBuiltinsPass(registry);
  initializeGenXLoadStoreLegalizationPass(registry);
  initializeGenXLoadStoreLoweringPass(registry);
  initializeGenXStackUsagePass(registry);
  initializeGenXOCLRuntimeInfoPass(registry);
  initializeGenXStructSplitterPass(registry);
  initializeGenXCloneIndirectFunctionsPass(registry);
  initializeGenXTrampolineInsertionPass(registry);
  initializeGenXPredRegionLoweringPass(registry);
  initializeGenXPredToSimdCFPass(registry);
  initializeGenXLinkageCorruptorPass(registry);
  initializeGenXInlineAsmLoweringPass(registry);
  initializeGenXDebugLegalizationPass(registry);
  initializeGenXFixInvalidFuncNamePass(registry);
  initializeGenXLegalizeGVLoadUsesPass(registry);
  initializeGenXGASCastWrapperPass(registry);
  initializeGenXGASDynamicResolutionPass(registry);
  initializeGenXInitBiFConstantsPass(registry);
  initializeGenXTranslateIntrinsicsPass(registry);
  initializeGenXFinalizerPass(registry);
  initializeGenXBuiltinFunctionsPass(registry);
  initializeGenXLegacyToLscTranslatorPass(registry);
  initializeGenXSLMResolutionPass(registry);
  initializeGenXTypeLegalizationPass(registry);
  initializeGenXLscAddrCalcFoldingPass(registry);
  initializeGenXDetectPointerArgPass(registry);
  initializeGenXLCECalculationPass(registry);
  initializeGenXFloatControlPass(registry);
  initializeGenXCountIndirectStatelessPass(registry);
  initializeGenXUnfreezePass(registry);
  // WRITE HERE MORE PASSES IF IT'S NEEDED;
}

TargetTransformInfo GenXTargetMachine::getTargetTransformInfo(
  const Function &F) LLVM_GET_TTI_API_QUAL
{
  GenXTTIImpl GTTI(F.getParent()->getDataLayout(), *BC, Subtarget);
  return TargetTransformInfo(std::move(GTTI));
}
void GenXTTIImpl::getUnrollingPreferences(
    Loop *L, ScalarEvolution &SE,
    TargetTransformInfo::UnrollingPreferences &UP,
    OptimizationRemarkEmitter *ORE) {
  if (BC.ignoreLoopUnrollThresholdOnPragma() &&
      GetUnrollMetadataForLoop(L, "llvm.loop.unroll.enable"))
    UP.Threshold = UP.PartialThreshold = std::numeric_limits<unsigned>::max();

  if (unsigned VCUnrollThreshold = BC.getLoopUnrollThreshold()) {
    UP.Threshold = VCUnrollThreshold;
    UP.PartialThreshold = VCUnrollThreshold;
    UP.Partial = true;
  }

  if (GetUnrollMetadataForLoop(L, "llvm.loop.unroll.full")) {
    UP.Threshold = std::numeric_limits<unsigned>::max();
    UP.Partial = false;
    UP.Runtime = false;
  }
}

void GenXTTIImpl::getPeelingPreferences(
    Loop *L, ScalarEvolution &SE,
    TargetTransformInfo::PeelingPreferences &PP) const {
  if (!PeelLoopsDpasNullAcc || ST.hasFusedEU())
    return;

  // Don't peel the loop if user explicitly asked to unroll
  if (auto *LoopID = L->getLoopID();
      LoopID && (GetUnrollMetadata(LoopID, "llvm.loop.unroll.enable") ||
                 GetUnrollMetadata(LoopID, "llvm.loop.unroll.full") ||
                 GetUnrollMetadata(LoopID, "llvm.loop.unroll.count")))
    return;

  // Only analyze the inner-most loops
  if (!L->getSubLoops().empty())
    return;

  // Limit loop size to the specific number of basic blocks
  if (L->getNumBlocks() > PeelLoopDpasNullAccMaxBlocks)
    return;

  // Match the following two cases:
  // 1. Dpas accumulator is a phi node with a zero value came from the pre-loop
  // basic block.
  //   %acc = phi <N x Ty> [ zeroinitializer, %entry ], [ %dst, %loop ]
  //   %dst = call @llvm.genx.dpas(%acc, ...)
  // 2. Dpas accumulator is a rdregion taking a piece of a large vector. The
  // large vector is a phi node with a zero value taken from the pre-loop basic
  // block.
  //   %phi = phi <N x ty>
  //   %acc = call @llvm.genx.rdregion(%phi, ...)
  //   %dst = call @llvm.genx.dpas(%acc, ...)
  //   %wrregion = call @llvm.genx.wrregion(%phi, %dst, ...)

  const auto PhiNodes = L->getHeader()->phis();
  const uint64_t NumDpasZeroAcc = std::accumulate(
      std::begin(PhiNodes), std::end(PhiNodes), 0ull,
      [this](uint64_t Acc, const auto &Phi) {
        // Check vector Phi nodes
        const auto *PhiVTy = dyn_cast<IGCLLVM::FixedVectorType>(Phi.getType());
        if (!PhiVTy || Phi.getNumIncomingValues() != 2)
          return Acc;

        // Check if one of the Phi inputs is constant zero
        bool IsZeroAcc = llvm::any_of(Phi.incoming_values(), [](const auto &V) {
          const auto *C = dyn_cast<Constant>(&V);
          return C && C->isZeroValue();
        });
        if (!IsZeroAcc)
          return Acc;

        // Simple case. The only user of the Phi node must be dpas intrinsic.
        if (isDpasAccumulator(&Phi))
          return Acc + 1;

        // If vector decomposition is disabled, we unable to simplify the region
        // access chain. So, it doesn't make sense to peel the loop.
        if (ST.disableVectorDecomposition())
          return Acc;

        if (!Phi.hasNUses(2))
          return Acc;

        // Find the rdregion used as a dpas accumulator
        const auto It = llvm::find_if(Phi.users(), [](const Value *V) {
          return GenXIntrinsic::isRdRegion(V) && isDpasAccumulator(V);
        });
        if (It == Phi.user_end())
          return Acc;

        // Assume that the whole phi node value is used as an accumulator source
        // for multiple similar dpas instructions.
        const auto *DpasVTy = cast<IGCLLVM::FixedVectorType>((*It)->getType());
        return Acc + (PhiVTy->getNumElements() / DpasVTy->getNumElements());
      });

  if (NumDpasZeroAcc < PeelLoopDpasNullAccMin)
    return;

  // Don't peel too large loops
  const auto Blocks = L->getBlocks();
  const auto NumInstr = std::accumulate(
      std::begin(Blocks), std::end(Blocks), 0u,
      [](unsigned Acc, const auto *BB) { return Acc + BB->size(); });
  if (NumInstr > PeelLoopDpasNullAccMaxInstr)
    return;

  PP.PeelCount = 1;
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
                                     IGCLLVM::optional<Reloc::Model> RM,
                                     IGCLLVM::optional<CodeModel::Model> CM,
                                     CodeGenOpt::Level OL, bool Is64Bit,
                                     std::unique_ptr<GenXBackendConfig> BC)
    : IGCLLVM::LLVMTargetMachine(
          T, getDL(Is64Bit), TT, CPU, FS, Options,
          RM ? IGCLLVM::getValue(RM) : Reloc::Model::Static,
          CM ? IGCLLVM::getValue(CM) : CodeModel::Model::Small, OL),
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
                                         IGCLLVM::optional<Reloc::Model> RM,
                                         IGCLLVM::optional<CodeModel::Model> CM,
                                         CodeGenOpt::Level OL, bool JIT,
                                         std::unique_ptr<GenXBackendConfig> BC)
    : GenXTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, false,
                        std::move(BC)) {}

GenXTargetMachine64::GenXTargetMachine64(const Target &T, const Triple &TT,
                                         StringRef CPU, StringRef FS,
                                         const TargetOptions &Options,
                                         IGCLLVM::optional<Reloc::Model> RM,
                                         IGCLLVM::optional<CodeModel::Model> CM,
                                         CodeGenOpt::Level OL, bool JIT,
                                         std::unique_ptr<GenXBackendConfig> BC)
    : GenXTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, true,
                        std::move(BC)) {}

namespace vc {
std::unique_ptr<llvm::TargetMachine> createGenXTargetMachine(
    const Target &T, Triple TT, StringRef CPU, StringRef Features,
    const TargetOptions &Options, IGCLLVM::optional<Reloc::Model> RM,
    IGCLLVM::optional<CodeModel::Model> CM, CodeGenOpt::Level OL,
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

  vc::addPass(PM, createGenXUnfreezePass());

  vc::addPass(PM, createGenXInitBiFConstantsPass());
  vc::addPass(PM, createGenXFixInvalidFuncNamePass());

  /// .. include:: GenXRawSendRipper.cpp
  vc::addPass(PM, createGenXRawSendRipperPass());

  // Install GenX-specific TargetTransformInfo for passes such as
  // LowerAggrCopies and InfoAddressSpace
  vc::addPass(PM, createTargetTransformInfoWrapperPass(getTargetIRAnalysis()));

  if (BackendConfig.isBiFCompilation())
    vc::addPass(PM, createGenXBiFPreparePass());

  vc::addPass(PM, createSROAPass());
  vc::addPass(PM, createEarlyCSEPass());
  vc::addPass(PM, createLowerExpectIntrinsicPass());
  vc::addPass(PM, createCFGSimplificationPass(SimplifyCFGOptions().hoistCommonInsts(true)));
  vc::addPass(PM, createInstructionCombiningPass());
  vc::addPass(PM, createCFGSimplificationPass());

  vc::addPass(PM, createGlobalDCEPass());
  vc::addPass(PM, createGenXDebugLegalizationPass());
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
  vc::addPass(PM, createGenXPromoteArrayPass());
  vc::addPass(PM, createPromoteMemoryToRegisterPass());
  /// .. include:: GenXDetectPointerArg.cpp
  vc::addPass(PM, createGenXDetectPointerArgPass());
  /// .. include:: GenXCountIndirectStateless.cpp
  vc::addPass(PM, createGenXCountIndirectStatelessPass());

  // All passes which modify the LLVM IR are now complete; run the verifier
  // to ensure that the IR is valid.
  if (!DisableVerify) {
    vc::addPass(PM, createVerifierPass());
    vc::addPass(PM, createGenXVerifyPass(GenXVerifyStage::PostIrAdaptors));
  }

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

  if (BackendConfig.isCostModelEnabled()) {
    /// LoopSimplify
    /// ------------
    /// This is a standard LLVM pass, run at this point in the GenX backend.
    /// It is required since llvm loop bounds are used for LCE calculations.
    ///
    vc::addPass(PM, createLoopSimplifyPass());
    /// .. include:: GenXLCECalculation.cpp
    vc::addPass(PM, createGenXLCECalculationPass());
  }

  /// .. include:: GenXInlineAsmLowering.cpp
  vc::addPass(PM, createGenXInlineAsmLoweringPass());
  /// .. include:: GenXReduceIntSize.cpp
  vc::addPass(PM, createGenXReduceIntSizePass());
  /// .. include:: GenXGlobalValueLowering.cpp
  vc::addPass(PM, createGenXGlobalValueLoweringPass());
  /// .. include:: GenXSLMResolution.cpp
  vc::addPass(PM, createGenXSLMResolution());

  /// .. include:: GenXStackUsage.cpp
  vc::addPass(PM, createGenXStackUsagePass());

  // PrologEpilog may emit memory instructions of illegal width.
  vc::addPass(PM, createGenXPrologEpilogInsertionPass());

  /// .. include:: GenXAggregatePseudoLowering.cpp
  /// GenXAggregatePseudoLowering must be run after
  /// GenXPrologEpilogInsertion as the latter may create stack loads and stores
  /// of aggregates.
  vc::addPass(PM, createGenXAggregatePseudoLoweringPass());

  //  .. include:: GenXGASCastAnalyzer.cpp
  vc::addPass(PM, createGenXGASCastWrapperPass());
  //  .. include:: GenXGASDynamicResolution.cpp
  vc::addPass(PM, createGenXGASDynamicResolutionPass());
  /// .. include:: GenXLoadStoreLowering.cpp
  vc::addPass(PM, createGenXLoadStoreLoweringPass());
  /// .. include:: GenXLegacyToLscTranslator.cpp
  vc::addPass(PM, createGenXLegacyToLscTranslatorPass());

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

  /// GenXPredToSimdCF
  /// ----------------
  /// This pass is needed to support uinified Simd CF interface.
  /// vc::addPass(PM, createCFGSimplificationPass());
  /// .. include:: GenXPredToSimdCF.cpp
  /// Now pass enable by-default only for BiF compilation
  vc::addPass(PM, createGenXPredToSimdCFPass());
  vc::addPass(PM, createPromoteMemoryToRegisterPass());

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

  /// .. include:: GenXGEPLowering.cpp
  vc::addPass(PM, createGenXGEPLoweringPass());
  /// .. include:: GenXLoadStoreLegalization.cpp
  vc::addPass(PM, createGenXLoadStoreLegalizationPass());

  /// .. include:: GenXBuiltinFunctions.cpp
  // early built-in functions, which allowed to inline
  if (!BackendConfig.isBiFCompilation()) {
    vc::addPass(PM, createGenXBuiltinFunctionsPass(
                        BuiltinFunctionKind::PreLegalization));
    vc::addPass(PM, createAlwaysInlinerLegacyPass());
  }

  /// .. include:: GenXLscAddrCalcFolding.cpp
  vc::addPass(PM, createGenXLscAddrCalcFoldingPass());

  /// .. include:: GenXBFloatLowering.cpp
  vc::addPass(PM, createGenXBFloatLoweringPass());
  /// .. include:: GenXLowering.cpp
  vc::addPass(PM, createGenXLoweringPass());

  if (!DisableVerify) {
    vc::addPass(PM, createVerifierPass());
    vc::addPass(PM, createGenXVerifyPass(GenXVerifyStage::PostGenXLowering));
  }

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
  vc::addPass(PM, createGenXPatternMatchPass(PatternMatchKind::PreLegalization));

  if (!DisableVerify) {
    vc::addPass(PM, createVerifierPass());
    vc::addPass(PM, createGenXVerifyPass(GenXVerifyStage::PostGenXLowering));
  }

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

  /// .. include:: GenXEmulate.cpp
  vc::addPass(PM, createGenXEmulatePass());

  // BiF compilation mode stops here
  if (BackendConfig.isBiFCompilation())
    return false;
  /// .. include:: GenXBuiltinFunctions.cpp
  vc::addPass(PM, createGenXBuiltinFunctionsPass(
                      BuiltinFunctionKind::PostLegalization));

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

  if (!DisableVerify) {
    vc::addPass(PM, createVerifierPass());
    vc::addPass(PM,
                createGenXVerifyPass(GenXVerifyStage::PostGenXLegalization));
  }

  /// EarlyCSE
  /// --------
  /// This is a standard LLVM pass, run at this point in the GenX backend.
  /// It commons up common subexpressions, but only in the case that two common
  /// subexpressions are related by one dominating the other.
  ///
  vc::addPass(PM, createEarlyCSEPass());
  /// .. include:: GenXPatternMatch.cpp
  vc::addPass(PM, createGenXPatternMatchPass(PatternMatchKind::PostLegalization));

  if (!DisableVerify) {
    vc::addPass(PM, createVerifierPass());
    vc::addPass(PM,
                createGenXVerifyPass(GenXVerifyStage::PostGenXLegalization));
  }

  /// LICM
  /// ----
  /// This is a standard LLVM pass to hoist/sink the loop invariant code after
  /// legalization.
  vc::addPass(PM, createLICMPass());
  vc::addPass(PM, createEarlyCSEPass());
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
  vc::addPass(PM, createGenXFloatControlPass());
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

  if (BackendConfig.checkGVClobbering())
    /// .. include:: GenXGVClobberChecker.cpp
    vc::addPass(PM, createGenXGVClobberCheckerPass());

  /// .. include:: GenXVisaRegAlloc.h
  auto *RegAlloc = createGenXVisaRegAllocWrapperPass();
  vc::addPass(PM, RegAlloc);
  if (BackendConfig.enableRegAllocDump())
    vc::addPass(PM, createGenXModuleAnalysisDumperPass(RegAlloc, FGDumpsPrefix,
                                                       ".regalloc"));
  if (!DisableVerify) {
    vc::addPass(PM, createVerifierPass());
    vc::addPass(PM,
                createGenXVerifyPass(GenXVerifyStage::PostGenXLegalization));
  }

  /// .. include:: GenXCisaBuilder.cpp
  vc::addPass(PM, createGenXCisaBuilderPass());
  vc::addPass(PM, createGenXFinalizerPass());
  vc::addPass(PM, createGenXDebugInfoPass());

  // Analysis for collecting information related to OCL runtime. Can
  // be used by external caller by adding extractor pass in the end of
  // compilation pipeline.
  // Explicit construction can be omitted because adding of extractor
  // pass will create runtime info analysis. Leaving it exlicit for
  // clarity.
  if (!SkipOCLRuntimeInfo)
    vc::addPass(PM, new GenXOCLRuntimeInfo());

  return false;
}

#if LLVM_VERSION_MAJOR < 16
void GenXTargetMachine::adjustPassManager(PassManagerBuilder &PMBuilder) {
  // Fix function names.
  PMBuilder.addExtension(
      PassManagerBuilder::EP_EarlyAsPossible,
      [](const PassManagerBuilder &Builder, PassManagerBase &PM) {
        PM.add(createGenXFixInvalidFuncNamePass());
      });

  // Lower aggr copies.
  PMBuilder.addExtension(
      PassManagerBuilder::EP_EarlyAsPossible,
      [](const PassManagerBuilder &Builder, PassManagerBase &PM) {
        PM.add(createGenXLowerAggrCopiesPass());
      });

  // Packetize.
  auto AddPacketize = [](const PassManagerBuilder &Builder,
                         PassManagerBase &PM) {
    PM.add(createGenXLegalizeGVLoadUsesPass());
#ifndef NDEBUG
    PM.add(createGenXVerifyPass(GenXVerifyStage::PostSPIRVReader));
#endif
    PM.add(createGenXTranslateIntrinsicsPass());
    PM.add(createGenXTranslateSPIRVBuiltinsPass());
    PM.add(createAlwaysInlinerLegacyPass());
    PM.add(createAlwaysInlinerLegacyPass());
    PM.add(createGenXPrintfPhiClonningPass());
    PM.add(createInstructionCombiningPass());
    PM.add(createGenXPrintfResolutionPass());
    PM.add(createGenXImportOCLBiFPass());
    PM.add(createGenXBIFFlagCtrlResolutionPass());
    PM.add(createGenXTypeLegalizationPass());
    PM.add(createGenXPacketizePass());
    PM.add(createAlwaysInlinerLegacyPass());
    PM.add(createGenXPrintfLegalizationPass());
    PM.add(createGlobalDCEPass());
    PM.add(createPromoteMemoryToRegisterPass());
    PM.add(createInferAddressSpacesPass());
    PM.add(createEarlyCSEPass(true));
    PM.add(createCFGSimplificationPass(SimplifyCFGOptions().hoistCommonInsts(true)));
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
    auto AddLowerLoadStore = [this](const PassManagerBuilder &Builder,
                                PassManagerBase &PM) {
      GenXPassConfig *PassConfig = createGenXPassConfig(*this, PM);
      vc::addPass(PM, PassConfig);
      const GenXBackendConfig &BackendConfig = PassConfig->getBackendConfig();

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
        if (!(BackendConfig.disableIndvarsOpt())) {
          PM.add(createIndVarSimplifyPass());
        }
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

  auto AddIndirect = [](const PassManagerBuilder &Builder,
                        PassManagerBase &PM) {
    PM.add(createGenXCloneIndirectFunctionsPass());
    PM.add(createGenXTrampolineInsertionPass());
  };
  PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly,
                         AddIndirect);
  PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                         AddIndirect);

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
    PM.add(createCMImpParamPass(Subtarget.hasThreadPayloadInMemory()));
  };
  PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly,
                         AddCMImpParam);
  PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                         AddCMImpParam);

  // CM ABI.
  auto AddCMABI = [](const PassManagerBuilder &Builder, PassManagerBase &PM) {
    PM.add(createCMABILegacyPass());
  };
  PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly, AddCMABI);
  PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0, AddCMABI);

  // BTI assignment.
  auto AddBTIAssign = [this](const PassManagerBuilder &Builder,
                             PassManagerBase &PM) {
    PM.add(createGenXBTIAssignmentPass());
  };
  PMBuilder.addExtension(PassManagerBuilder::EP_ModuleOptimizerEarly,
                         AddBTIAssign);
  PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                         AddBTIAssign);

  // CM kernel argument offset.
  auto AddCMKernelArgOffset = [this](const PassManagerBuilder &Builder,
                                     PassManagerBase &PM) {
    GenXPassConfig *PassConfig = createGenXPassConfig(*this, PM);
    vc::addPass(PM, PassConfig);
    const GenXBackendConfig &BackendConfig = PassConfig->getBackendConfig();

    PM.add(createCMKernelArgOffsetPass(Subtarget.getGRFByteSize(),
                                       BackendConfig.useBindlessImages()));
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
  PMBuilder.addExtension(PassManagerBuilder::EP_EnabledOnOptLevel0,
                         AddGenXPeephole);
}

#else // LLVM_VERSION_MAJOR < 16

void GenXTargetMachine::registerPassBuilderCallbacks(PassBuilder &PB) {

  PB.registerAnalysisRegistrationCallback([=](ModuleAnalysisManager &MAM) {
    MAM.registerPass([&] { return CMABIAnalysisPass(); });
    MAM.registerPass([&] { return GenXBackendConfigPass(); });
  });

  PB.registerPipelineStartEPCallback([this](ModulePassManager &PM,
                                            OptimizationLevel Level) {
    // Fix function names.
    PM.addPass(createModuleToFunctionPassAdaptor(GenXFixInvalidFuncNamePass()));

    // Lower aggr copies.
    PM.addPass(createModuleToFunctionPassAdaptor(GenXLowerAggrCopiesPass()));

    // Standard set of passes called in llvm-14
    PM.addPass(createModuleToFunctionPassAdaptor(LowerExpectIntrinsicPass()));
    PM.addPass(createModuleToFunctionPassAdaptor(SimplifyCFGPass()));
    PM.addPass(
        createModuleToFunctionPassAdaptor(SROAPass(SROAOptions::ModifyCFG)));
    PM.addPass(createModuleToFunctionPassAdaptor(EarlyCSEPass(true)));
    PM.addPass(Annotation2MetadataPass());
    PM.addPass(ForceFunctionAttrsPass());
    PM.addPass(InferFunctionAttrsPass());

    // Packetize.
    PM.addPass(GenXLegalizeGVLoadUsesPass());
#ifndef NDEBUG
    // PM.addPass(GenXVerifyPass(GenXVerifyStage::PostSPIRVReader));
#endif
    PM.addPass(
        createModuleToFunctionPassAdaptor(GenXTranslateIntrinsicsPass()));
    PM.addPass(GenXTranslateSPIRVBuiltinsPass(BC->getResult()));
    PM.addPass(AlwaysInlinerPass());
    PM.addPass(AlwaysInlinerPass());
    PM.addPass(GenXPrintfPhiClonningPass());
    PM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
    PM.addPass(GenXPrintfResolutionPass(this));
    PM.addPass(GenXImportOCLBiFPass());
    PM.addPass(GenXBIFFlagCtrlResolutionPass());
    PM.addPass(createModuleToFunctionPassAdaptor(GenXTypeLegalizationPass()));
    PM.addPass(GenXPacketizePass());
    PM.addPass(AlwaysInlinerPass());
    PM.addPass(GenXPrintfLegalizationPass());
    PM.addPass(GlobalDCEPass());
    PM.addPass(createModuleToFunctionPassAdaptor(PromotePass()));

    PM.addPass(createModuleToFunctionPassAdaptor(InferAddressSpacesPass()));
    PM.addPass(createModuleToFunctionPassAdaptor(EarlyCSEPass(true)));
    PM.addPass(createModuleToFunctionPassAdaptor(
        SimplifyCFGPass(SimplifyCFGOptions().hoistCommonInsts(true))));
    PM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
    PM.addPass(createModuleToFunctionPassAdaptor(DCEPass()));
    PM.addPass(
        createModuleToFunctionPassAdaptor(SROAPass(SROAOptions::ModifyCFG)));
    PM.addPass(createModuleToFunctionPassAdaptor(InferAddressSpacesPass()));
    PM.addPass(createModuleToFunctionPassAdaptor(EarlyCSEPass(true)));
    PM.addPass(createModuleToFunctionPassAdaptor(SimplifyCFGPass()));
    PM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
    PM.addPass(createModuleToFunctionPassAdaptor(DCEPass()));

    // vldst.
    if (EmitVLoadStore) {
      // this->OptLevel > 0
      // Inline
      PM.addPass(
          createModuleToFunctionPassAdaptor(SROAPass(SROAOptions::ModifyCFG)));
      PM.addPass(createModuleToFunctionPassAdaptor(EarlyCSEPass(true)));
      PM.addPass(createModuleToFunctionPassAdaptor(JumpThreadingPass()));
      PM.addPass(createModuleToFunctionPassAdaptor(SimplifyCFGPass()));
      PM.addPass(
          createModuleToFunctionPassAdaptor(CorrelatedValuePropagationPass()));
      PM.addPass(createModuleToFunctionPassAdaptor(GenXReduceIntSizePass()));
      PM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
      PM.addPass(AlwaysInlinerPass());
      PM.addPass(GlobalDCEPass());
      PM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));

      // Unroll
      PM.addPass(createModuleToFunctionPassAdaptor(SimplifyCFGPass()));
      PM.addPass(createModuleToFunctionPassAdaptor(ReassociatePass()));
      PM.addPass(createModuleToFunctionPassAdaptor(
          createFunctionToLoopPassAdaptor(LoopRotatePass())));
      // TODO: Check LICM-options
      PM.addPass(createModuleToFunctionPassAdaptor(
          createFunctionToLoopPassAdaptor(LICMPass(100, 250, false),
                                          /*UseMemorySSA=*/true,
                                          /*UseBlockFrequencyInfo=*/true)));
      PM.addPass(createModuleToFunctionPassAdaptor(EarlyCSEPass(true)));

      PM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));

      if (!BC->disableIndvarsOpt()) {
        PM.addPass(createModuleToFunctionPassAdaptor(
            createFunctionToLoopPassAdaptor(IndVarSimplifyPass())));
      }
      PM.addPass(createModuleToFunctionPassAdaptor(
          createFunctionToLoopPassAdaptor(LoopIdiomRecognizePass())));

      PM.addPass(createModuleToFunctionPassAdaptor(
          createFunctionToLoopPassAdaptor(LoopDeletionPass())));
      // TODO: pass 'simple' options to LoopUnrollPass
      PM.addPass(createModuleToFunctionPassAdaptor(LoopUnrollPass()));
      PM.addPass(createModuleToFunctionPassAdaptor(InstCombinePass()));
      // Simplify region accesses.
      PM.addPass(
          createModuleToFunctionPassAdaptor(GenXRegionCollapsingPass(this)));
      PM.addPass(createModuleToFunctionPassAdaptor(EarlyCSEPass(true)));
      PM.addPass(createModuleToFunctionPassAdaptor(DCEPass()));
      // }
      PM.addPass(createModuleToFunctionPassAdaptor(CMLowerVLoadVStorePass()));
    }

    // AddIndirect
    PM.addPass(GenXCloneIndirectFunctionsPass(BC->getResult()));
    PM.addPass(GenXTrampolineInsertionPass(BC->getResult()));

    // AddLinkageCorruptor
    PM.addPass(GenXLinkageCorruptorPass(BC->getResult()));

    // AddCMImpParam
    PM.addPass(CMImpParamPass(Subtarget.hasThreadPayloadInMemory()));

    // CM ABI.
    PM.addPass(CMABIPass());

    // BTI assignment.
    PM.addPass(GenXBTIAssignmentPass(BC->getResult()));

    PM.addPass(CMKernelArgOffsetPass(Subtarget.getGRFByteSize(),
                                     BC->useBindlessImages()));
  });

  // AddGenXPeephole
  PB.registerPeepholeEPCallback(
      [this](FunctionPassManager &PM, OptimizationLevel Level) {
        PM.addPass(GenXSimplifyPass());
      });
}

#endif // LLVM_VERSION_MAJOR < 16
