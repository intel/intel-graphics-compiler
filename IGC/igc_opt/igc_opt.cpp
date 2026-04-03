/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

// IGC version of LLVM's opt tool.
// Based on LLVM 14 opt, adapted to work with IGC passes and infrastructure.
// Comments marked "IGC specific:" call out behavior that intentionally
// differs from the original LLVM 14 tool.
//
// Load an .ll/.bc file, perform LLVM/IGC passes on it and print the result.

#include "common/LLVMUtils.h"
#include "CreateIGCContext.h"
#include "InitializeAllIGCPasses.h"
#include "RestoreGenISAIntrinsicDeclarations.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/InitializePasses.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/MetaDataUtilsWrapperInitializer.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/GenTTI.h"
#include "Compiler/CISACodeGen/CheckInstrTypes.hpp"

#include "Probe/Assertion.h"
#include "llvm/Transforms/Utils/Debugify.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LegacyPassNameParser.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#if LLVM_VERSION_MAJOR < 17
#include "llvm/MC/SubtargetFeature.h"
#else // LLVM_VERSION_MAJOR
#include "llvm/TargetParser/SubtargetFeature.h"
#endif // LLVM_VERSION_MAJOR
#include "llvm/Support/Debug.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/ObjCARC.h"
#include "common/LLVMWarningsPop.hpp"

// IGC specific: wrappers for backward LLVM compatibility
#include "llvmWrapper/Support/FileSystem.h"
#include "llvmWrapper/Support/SystemUtils.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "llvmWrapper/ADT/STLExtras.h"
#include "llvmWrapper/Transforms/IPO/LegacyPassManagerBuilder.h"
#include "llvmWrapper/Transforms/IPO/InlineSimple.h"

#include <algorithm>
#include <memory>

using namespace llvm;

// The OptimizationList is automatically populated with registered Passes by the
// PassNameParser.
static cl::list<const PassInfo *, bool, PassNameParser> PassList(cl::desc("Optimizations available:"));

//===----------------------------------------------------------------------===//
// IGC specific: igc_opt-specific command-line options.
//===----------------------------------------------------------------------===//
enum class InstrStatTypesOpt { NONE, LICM, SROA };

static cl::opt<bool> DisableMetaDataUtilsWrapper("disable-metadata-utils-wrapper",
                                                 cl::desc("Do not run the MetaDataUtilsWrapper pass"));

static cl::opt<bool> EnableDebugify("enable-debugify",
                                    cl::desc("Start the pipeline with debugify and end it with check-debugify"));

static cl::opt<InstrStatTypesOpt>
    EnableInstrStatistic(cl::desc("Enable InstrStatistic pass and specify used type"),
                         cl::values(clEnumValN(InstrStatTypesOpt::LICM, "igc-inst-stat-licm", "LICM pass type"),
                                    clEnumValN(InstrStatTypesOpt::SROA, "igc-inst-stat-sroa", "SROA pass type")),
                         cl::init(InstrStatTypesOpt::NONE), cl::Hidden);

static cl::opt<unsigned int> InstrStatisticThreshold("igc-inst-stat-th",
                                                     cl::desc("Set the threshold for InstrStatistic pass"), cl::init(0),
                                                     cl::Hidden);

static cl::list<std::string> RegKeys("regkey", cl::desc("Set IGC register keys."),
                                     cl::value_desc("regkeyname0[=v],regkeyname1[=v]...regkeynameN[=v]"),
                                     cl::CommaSeparated);

static cl::opt<bool> RetainDenormals("retain-denormals", cl::desc("Set retain denormals mode"), cl::init(false),
                                     cl::Hidden);

static cl::opt<bool>
    DisableRestoreGenISAIntrinsics("disable-restore-genisa-intrinsics",
                                   cl::desc("Do not restore canonical GenISA intrinsic declarations after loading IR"),
                                   cl::Hidden);

static cl::opt<bool> UseGenTTI("gen-tti", cl::desc("Use info from GenIntrinsicsTTIImpl"), cl::init(false));

//===----------------------------------------------------------------------===//
// Legacy command-line options used by igc_opt.
// Most of these options mirror the legacy path in LLVM 14 opt, while some
// older legacy-PM flags are retained for compatibility and newer LLVM options
// that IGC does not support are intentionally omitted.
//===----------------------------------------------------------------------===//
static cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<input bitcode file>"), cl::init("-"),
                                          cl::value_desc("filename"));

static cl::opt<std::string> OutputFilename("o", cl::desc("Override output filename"), cl::value_desc("filename"));

static cl::opt<bool> NoOutput("disable-output", cl::desc("Do not write result bitcode file"), cl::Hidden);

static cl::opt<bool> OutputAssembly("S", cl::desc("Write output as LLVM assembly"));

static cl::opt<bool> NoVerify("disable-verify", cl::desc("Do not verify result module"), cl::Hidden);

static cl::opt<bool> VerifyEach("verify-each", cl::desc("Verify after each transform"));

static cl::opt<bool> StripDebug("strip-debug", cl::desc("Strip debugger symbol info from translation unit"));

#if LLVM_VERSION_MAJOR >= 17
// LLVM 17+ has switched to opaque pointers by default and removed the --opaque-pointers flag. Define a dummy option for
// CLI compatibility with older versions of LLVM. This flag can be removed once IGC drops support for LLVM 16 and older
// and all LIT tests are updated (i.e. have the flag removed).
static cl::opt<bool> OpaquePointers("opaque-pointers", cl::desc("Enable opaque pointers throughout the pipeline"),
                                    cl::init(true));
#endif // LLVM_VERSION_MAJOR

// IGC specific: PassNameParser needs IGC passes registered before parsing so
// igc_opt can accept them by their pass arguments.
void InitIGCOpt() {
  llvm::PassRegistry *const Registry = llvm::PassRegistry::getPassRegistry();
  IGC_ASSERT(nullptr != Registry);
  initializeAllIGCPasses(*Registry);
  initializeAllLLVMWrapperPasses(*Registry);
}

static inline void addPass(legacy::PassManagerBase &PM, Pass *P) {
  PM.add(P);
  if (VerifyEach)
    PM.add(createVerifierPass());
}

//===----------------------------------------------------------------------===//
// main for opt
//
int main(int argc, char **argv) {

  sys::PrintStackTraceOnErrorSignal(argv[0]);
  llvm::PrettyStackTraceProgram X(argc, argv);

  // Enable debug stream buffering.
  EnableDebugBuffering = true;

  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.

  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();

  // Initialize passes
  PassRegistry &Registry = *PassRegistry::getPassRegistry();
  initializeCore(Registry);
  initializeScalarOpts(Registry);
#if LLVM_VERSION_MAJOR < 16
  initializeObjCARCOpts(Registry);
#endif // LLVM_VERSION_MAJOR
  initializeVectorization(Registry);
  initializeAnalysis(Registry);
  initializeTransformUtils(Registry);
  initializeInstCombine(Registry);
  initializeTarget(Registry);
  // For codegen passes, only passes that do IR to IR transformation are
  // supported.

  initializeCodeGenPreparePass(Registry);
  initializeAtomicExpandPass(Registry);
#if LLVM_VERSION_MAJOR < 17
  initializeRewriteSymbolsLegacyPassPass(Registry);
#endif // LLVM_VERSION_MAJOR
  initializeWinEHPreparePass(Registry);
  initializeDwarfEHPrepareLegacyPassPass(Registry);
  initializeSjLjEHPreparePass(Registry);

  InitIGCOpt();

  cl::ParseCommandLineOptions(argc, argv, "llvm .bc -> .bc modular optimizer and analysis printer\n");

  // IGC specific: regkeys expose the same debug/compiler toggles used by the
  // full IGC compiler driver.
  for (auto &reg : RegKeys) {

    size_t pos = reg.find('=');

    if (pos == std::string::npos) {
      IGC::Debug::SetCompilerOptionValue(reg.c_str(), true);
      continue;
    }

    std::string key = reg.substr(0, pos);
    debugString valStr = {0};
    strncpy_s(valStr, sizeof(valStr), reg.c_str() + pos + 1, sizeof(valStr) - 1);

    // Check for int value
    char *last = nullptr;
    unsigned int valInt = strtoul(valStr, &last, 0);

    if (last == valStr + strnlen_s(valStr, sizeof(valStr))) {
      IGC::Debug::SetCompilerOptionValue(key.c_str(), valInt);
    } else {
      IGC::Debug::SetCompilerOptionString(key.c_str(), valStr);
    }
  }

  IGC::Debug::SetDebugFlag(IGC::Debug::DebugFlag::DUMP_TO_OUTS, true);

  // IGC specific: igc_opt owns a full CodeGenContext because many IGC passes
  // reach through it for metadata, platform, and shader state.
  std::unique_ptr<IGC::CodeGenContext> ctx(CreateCodeGenContext());
  IGC_ASSERT(nullptr != ctx.get());

  llvm::LLVMContext *const llvmContext = ctx->getLLVMContext();

  SMDiagnostic Err;

  // IGC specific: CodeGenContext takes module ownership, so parseIRFile's
  // unique_ptr is released here.
  llvm::Module *M = parseIRFile(InputFilename, Err, *llvmContext).release();

  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }

  if (!DisableRestoreGenISAIntrinsics)
    restoreGenISAIntrinsicDeclarations(*M);

  // Strip debug info before running the verifier.
  if (StripDebug)
    StripDebugInfo(*M);

  // Immediately run the verifier to catch any problems before starting up the
  // pass pipelines.  Otherwise we can crash on broken code during
  // doInitialization().
  if (!NoVerify && verifyModule(*M, &errs())) {
    errs() << argv[0] << ": " << InputFilename << ": error: input module is broken!\n";
    return 1;
  }

  // Figure out what stream we are supposed to write to...
  std::unique_ptr<llvm::ToolOutputFile> Out;
  if (NoOutput) {
    if (!OutputFilename.empty())
      errs() << "WARNING: The -o (output filename) option is ignored when\n"
                "the --disable-output option is used.\n";
  } else {
    // Default to standard output.
    if (OutputFilename.empty())
      OutputFilename = "-";

    std::error_code EC;
    Out.reset(new llvm::ToolOutputFile(OutputFilename, EC, sys::fs::IGCLLVM_OF_None));
    if (EC) {
      errs() << EC.message() << '\n';
      return 1;
    }
  }

  // IGC specific: VISA emission owns the interesting final output, so suppress
  // LLVM IR emission unless the user already requested --disable-output.
  if (std::any_of(PassList.begin(), PassList.end(),
                  [](const PassInfo *P) { return P->getPassArgument() == "igc-emit-visa"; }) &&
      !NoOutput && NoOutput.getNumOccurrences() == 0)
    NoOutput = true;

  // If the output is set to be emitted to standard out, and standard out is a
  // console, print out a warning message and refuse to do it.  We don't
  // impress anyone by spewing tons of binary goo to a terminal.
  if (!NoOutput && !OutputAssembly)
    if (IGCLLVM::CheckBitcodeOutputToConsole(Out->os(), true))
      NoOutput = true;

  // IGC specific: IGCPassManager carries CodeGenContext-aware state that the
  // original legacy::PassManager does not provide.
  IGC::IGCPassManager Passes(ctx.get(), "igc_opt");

  TargetIRAnalysis TIRA;

  // IGC specific: unresolved triples simply leave igc_opt with default LLVM
  // TTI instead of matching original LLVM 14 opt's hard error.
  // GenTTI exposes IGC-specific cost modeling instead of using
  // the default LLVM TTI implementation.
  if (UseGenTTI) {
    TIRA = TargetIRAnalysis([&](const Function &) {
      GenIntrinsicsTTIImpl GTTI(ctx.get());
      return TargetTransformInfo(std::move(GTTI));
    });
  } else {
    TIRA = TargetIRAnalysis();
  }

  Passes.add(createTargetTransformInfoWrapperPass(std::move(TIRA)));

  if (EnableDebugify)
    Passes.add(createDebugifyModulePass());

  switch (EnableInstrStatistic) {
  case InstrStatTypesOpt::LICM:
    Passes.add(new IGC::InstrStatistic(ctx.get(), IGC::InstrStatTypes::LICM_STAT, IGC::InstrStatStage::BEGIN,
                                       InstrStatisticThreshold));
    break;
  case InstrStatTypesOpt::SROA:
    Passes.add(new IGC::InstrStatistic(ctx.get(), IGC::InstrStatTypes::SROA_PROMOTED, IGC::InstrStatStage::BEGIN,
                                       InstrStatisticThreshold));
    break;
  default:
    break;
  }

  // IGC specific: many IGC passes consume MetaDataUtils directly, so igc_opt
  // recreates the wrapper and deserializes module metadata up front.
  ctx->setModule(M);
  if (!DisableMetaDataUtilsWrapper) {
    IGC::MetaDataUtilsWrapper *mdWrapper =
        new IGC::MetaDataUtilsWrapper(ctx->getMetaDataUtils(), ctx->getModuleMetaData());
    IGC::deserialize(*ctx->getModuleMetaData(), M);
    Passes.add(mdWrapper);
  }
  if (RetainDenormals) {
    IGC::CompOptions &compOpt = ctx->getModuleMetaData()->compOpt;
    compOpt.FloatDenormMode16 = IGC::FLOAT_DENORM_RETAIN;
    compOpt.FloatDenormMode32 = IGC::FLOAT_DENORM_RETAIN;
    compOpt.FloatDenormMode64 = IGC::FLOAT_DENORM_RETAIN;
  }

  // IGC specific: required for any pass using CodeGenContext
  Passes.add(new IGC::CodeGenContextWrapper(ctx.get()));

  // Create a new optimization pass for each one specified on the command line
  for (unsigned i = 0; i < PassList.size(); ++i) {

    const PassInfo *PassInf = PassList[i];
    Pass *P = nullptr;
    IGC_ASSERT(nullptr != PassInf);
      if (PassInf->getNormalCtor())
        P = PassInf->getNormalCtor()();
      else
        errs() << argv[0] << ": cannot create pass: " << PassInf->getPassName() << "\n";
    if (P) {
      addPass(Passes, P);
    }
  }

  switch (EnableInstrStatistic) {
  case InstrStatTypesOpt::LICM:
    Passes.add(new IGC::InstrStatistic(ctx.get(), IGC::InstrStatTypes::LICM_STAT, IGC::InstrStatStage::END,
                                       InstrStatisticThreshold));
    break;
  case InstrStatTypesOpt::SROA:
    Passes.add(new IGC::InstrStatistic(ctx.get(), IGC::InstrStatTypes::SROA_PROMOTED, IGC::InstrStatStage::END,
                                       InstrStatisticThreshold));
    break;
  default:
    break;
  }

  if (EnableDebugify)
    Passes.add(createCheckDebugifyModulePass(false));

  // Check that the module is well formed on completion of optimization
  if (!NoVerify && !VerifyEach)
    Passes.add(createVerifierPass());

  raw_ostream *OS = nullptr;

  // Write bitcode or assembly to the output as the last step...
  if (!NoOutput) {
    IGC_ASSERT(nullptr != Out);
    OS = &Out->os();
    if (OutputAssembly)
      Passes.add(createPrintModulePass(*OS, ""));
    else
      Passes.add(createBitcodeWriterPass(*OS));
  }

  // Before executing passes, print the final values of the LLVM options.
  cl::PrintOptionValues();

  Passes.run(*M);

  if (ctx->HasError())
    errs() << ctx->GetError() << "\n";

  if (ctx->HasWarning())
    outs() << ctx->GetWarning() << "\n";

  // Declare success.
  if (!NoOutput) {
    IGC_ASSERT(nullptr != Out);
    Out->keep();
  }

  return 0;
}
