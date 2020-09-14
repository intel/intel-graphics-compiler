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

#if defined(__linux__)
#include <dlfcn.h>
#endif

#include "GenXBackendConfig.h"
#include "GenXOCLRuntimeInfo.h"
#include "GenXWATable.h"

#include "llvmWrapper/Target/TargetMachine.h"

#include "vc/GenXCodeGen/GenXTarget.h"
#include "vc/GenXCodeGen/GenXWrapper.h"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "vc/Support/Options.h"
#include "vc/Support/Status.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXSPIRVReaderAdaptor.h"
#include "llvm/GenXIntrinsics/GenXIntrOpts.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/InitializePasses.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Scalar.h"

#include <cctype>
#include <memory>
#include <new>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "Probe/Assertion.h"

using namespace llvm;

static Expected<std::vector<char>> translateSPIRVToIR(ArrayRef<char> Input) {
#if defined(_WIN64)
 //TODO: rename to SPIRVDLL64.dll when binary components are fixed
  constexpr char *SpirvLibName = "SPIRVDLL.dll";
#elif defined(_WIN32)
  constexpr char *SpirvLibName = "SPIRVDLL32.dll";
#else
  constexpr char *SpirvLibName = "libSPIRVDLL.so";
#endif
  constexpr char *SpirvReadVerifyName = "spirv_read_verify_module";
  using SpirvReadVerifyType =
      int(const char *pIn, size_t InSz,
          void (*OutSaver)(const char *pOut, size_t OutSize, void *OutUserData),
          void *OutUserData,
          void (*ErrSaver)(const char *pErrMsg, void *ErrUserData),
          void *ErrUserData);

#if defined(__linux__)
  // Hack to workaround cmoc crashes during loading of SPIRV library
  static auto DeepBindHack = dlopen(SpirvLibName, RTLD_NOW | RTLD_DEEPBIND);
#endif // __linux__

  using DL = sys::DynamicLibrary;
  std::string ErrMsg;
  DL DyLib = DL::getPermanentLibrary(SpirvLibName, &ErrMsg);
  if (!DyLib.isValid())
    return make_error<vc::DynLoadError>(ErrMsg);

  auto *SpirvReadVerifyFunc = reinterpret_cast<SpirvReadVerifyType *>(
      DyLib.getAddressOfSymbol(SpirvReadVerifyName));
  if (!SpirvReadVerifyFunc)
    return make_error<vc::SymbolLookupError>(SpirvLibName, SpirvReadVerifyName);

  auto OutSaver = [](const char *pOut, size_t OutSize, void *OutData) {
    auto *Vec = reinterpret_cast<std::vector<char> *>(OutData);
    Vec->assign(pOut, pOut + OutSize);
  };
  auto ErrSaver = [](const char *pErrMsg, void *ErrData) {
    auto *ErrStr = reinterpret_cast<std::string *>(ErrData);
    *ErrStr = pErrMsg;
  };

  std::vector<char> Result;
  int Status = SpirvReadVerifyFunc(Input.data(), Input.size(), OutSaver,
                                   &Result, ErrSaver, &ErrMsg);
  if (Status != 0)
    return make_error<vc::BadSpirvError>(ErrMsg);

  return {std::move(Result)};
}

static Expected<std::unique_ptr<llvm::Module>> getModule(ArrayRef<char> Input,
                                                         LLVMContext &C) {
  auto ExpIR = translateSPIRVToIR(Input);
  if (!ExpIR)
    return ExpIR.takeError();

  std::vector<char> &IR = ExpIR.get();
  llvm::MemoryBufferRef BufferRef(llvm::StringRef(IR.data(), IR.size()),
                                  "Deserialized SPIRV Module");
  auto ExpModule = llvm::parseBitcodeFile(BufferRef, C);

  if (!ExpModule)
    return llvm::handleExpected(
        std::move(ExpModule),
        []() -> llvm::Error {
          IGC_ASSERT_EXIT_MESSAGE(0, "Should create new error");
        },
        [](const llvm::ErrorInfoBase &E) {
          return make_error<vc::BadBitcodeError>(E.message());
        });

  if (verifyModule(*ExpModule.get()))
    return make_error<vc::InvalidModuleError>();

  return ExpModule;
}

static vc::ocl::ArgInfo
convertOCLArgInfo(const GenXOCLRuntimeInfo::KernelArgInfo &Info) {
  vc::ocl::ArgInfo Converted;

  using ArgKind = GenXOCLRuntimeInfo::KernelArgInfo::KindType;
  switch (Info.getKind()) {
  case ArgKind::General:
    Converted.Kind = vc::ocl::ArgKind::General;
    break;
  case ArgKind::LocalSize:
    Converted.Kind = vc::ocl::ArgKind::LocalSize;
    break;
  case ArgKind::GroupCount:
    Converted.Kind = vc::ocl::ArgKind::GroupCount;
    break;
  case ArgKind::Buffer:
    Converted.Kind = vc::ocl::ArgKind::Buffer;
    break;
  case ArgKind::SVM:
    Converted.Kind = vc::ocl::ArgKind::SVM;
    break;
  case ArgKind::Sampler:
    Converted.Kind = vc::ocl::ArgKind::Sampler;
    break;
  case ArgKind::Image1D:
    Converted.Kind = vc::ocl::ArgKind::Image1d;
    break;
  case ArgKind::Image2D:
    Converted.Kind = vc::ocl::ArgKind::Image2d;
    break;
  case ArgKind::Image3D:
    Converted.Kind = vc::ocl::ArgKind::Image3d;
    break;
  case ArgKind::PrintBuffer:
    Converted.Kind = vc::ocl::ArgKind::PrintBuffer;
    break;
  case ArgKind::PrivateBase:
    Converted.Kind = vc::ocl::ArgKind::PrivateBase;
    break;
  }

  using ArgAccessKind = GenXOCLRuntimeInfo::KernelArgInfo::AccessKindType;
  switch (Info.getAccessKind()) {
  case ArgAccessKind::None:
    Converted.AccessKind = vc::ocl::ArgAccessKind::None;
    break;
  case ArgAccessKind::ReadOnly:
    Converted.AccessKind = vc::ocl::ArgAccessKind::ReadOnly;
    break;
  case ArgAccessKind::WriteOnly:
    Converted.AccessKind = vc::ocl::ArgAccessKind::WriteOnly;
    break;
  case ArgAccessKind::ReadWrite:
    Converted.AccessKind = vc::ocl::ArgAccessKind::ReadWrite;
    break;
  }

  Converted.Index = Info.getIndex();
  Converted.Offset = Info.getOffset();
  Converted.SizeInBytes = Info.getSizeInBytes();
  Converted.BTI = Info.getBTI();

  return Converted;
}

static void convertOCLKernelInfo(vc::ocl::KernelInfo &Converted,
                                 const GenXOCLRuntimeInfo::KernelInfo &Info) {
  Converted.Name = Info.getName();
  std::transform(Info.arg_begin(), Info.arg_end(),
                 std::back_inserter(Converted.Args),
                 [](const GenXOCLRuntimeInfo::KernelArgInfo &ArgInfo) {
                   return convertOCLArgInfo(ArgInfo);
                 });
  Converted.PrintStrings = Info.getPrintStrings();
  Converted.HasGroupID = Info.usesGroupId();
  Converted.HasBarriers = Info.usesBarriers();
  Converted.SLMSize = Info.getSLMSize();
  Converted.ThreadPrivateMemSize = Info.getTPMSize();
  Converted.StatelessPrivateMemSize = Info.getStatelessPrivMemSize();
  Converted.GRFSizeInBytes = Info.getGRFSizeInBytes();

  if (Info.getRelocationTable().Size > 0) {
    Converted.RelocationTable.Buf = Info.getRelocationTable().Buffer;
    Converted.RelocationTable.Size = Info.getRelocationTable().Size;
    Converted.RelocationTable.NumEntries =
        Info.getRelocationTable().Entries;
  }
  if (Info.getSymbolTable().Size > 0) {
    Converted.SymbolTable.Buf = Info.getSymbolTable().Buffer;
    Converted.SymbolTable.Size = Info.getSymbolTable().Size;
    Converted.SymbolTable.NumEntries = Info.getSymbolTable().Entries;
  }
  Converted.ZEBinInfo.Relocations = Info.ZEBinInfo.Relocations;
  Converted.ZEBinInfo.Symbols.Functions = Info.ZEBinInfo.Symbols.Functions;
  Converted.ZEBinInfo.Symbols.Local = Info.ZEBinInfo.Symbols.Local;
}

static void convertOCLGTPinInfo(vc::ocl::GTPinInfo &Converted,
                                const GenXOCLRuntimeInfo::GTPinInfo &Info) {
  Converted.GTPinBuffer = Info.getGTPinBuffer();
}

static std::vector<vc::ocl::CompileInfo> convertInternalOCLInfo(
    const std::vector<GenXOCLRuntimeInfo::CompiledKernel> &CompiledKernels) {
  std::vector<vc::ocl::CompileInfo> Converted{CompiledKernels.size()};
  for (unsigned i = 0, e = CompiledKernels.size(); i != e; ++i) {
    auto &Conv = Converted[i];
    auto &Orig = CompiledKernels[i];
    convertOCLKernelInfo(Conv.KernelInfo, Orig.getKernelInfo());
    convertOCLGTPinInfo(Conv.GtpinInfo, Orig.getGTPinInfo());
    Conv.JitInfo = Orig.getJitterInfo();
    Conv.GenBinary = Orig.getGenBinary();
    Conv.DebugInfo = Orig.getDebugInfo();
  }
  return Converted;
}

static Triple overrideTripleWithVC(StringRef TripleStr) {
  Triple T{TripleStr};
  // Normalize triple.
  bool Is32Bit = T.isArch32Bit();
  if (TripleStr.startswith("genx32"))
    Is32Bit = true;
  return Triple{Is32Bit ? "genx32-unknown-unknown" : "genx64-unknown-unknown"};
}

static std::string getSubtargetFeatureString(const vc::CompileOptions &Opts) {

  SubtargetFeatures Features;

  if (!Opts.FeaturesString.empty()) {
    SmallVector<StringRef, 8> AuxFeatures;
    StringRef(Opts.FeaturesString).split(AuxFeatures, ",", -1, false);
    for (const auto& F: AuxFeatures) {
      auto Feature = F.trim();
      bool Enabled = Feature.consume_front("+");
      if (!Enabled) {
        bool Disabled = Feature.consume_front("-");
        IGC_ASSERT(Disabled && "unexpected feature format");
      }
      Features.AddFeature(Feature.str(), Enabled);
    }
  }

  if (Opts.NoVecDecomp)
    Features.AddFeature("disable_vec_decomp");
  if (Opts.Binary == vc::BinaryKind::OpenCL ||
      Opts.Binary == vc::BinaryKind::ZE)
    Features.AddFeature("ocl_runtime");

  return Features.getString();
}

static CodeGenOpt::Level getCodeGenOptLevel(const vc::CompileOptions &Opts) {
  if (Opts.OptLevel == vc::OptimizerLevel::None)
    return CodeGenOpt::None;
  return CodeGenOpt::Default;
}

static Expected<std::unique_ptr<TargetMachine>>
createTargetMachine(const vc::CompileOptions &Opts, Triple &TheTriple) {
  std::string Error;
  const Target *TheTarget = TargetRegistry::lookupTarget(
      TheTriple.getArchName().str(), TheTriple, Error);
  IGC_ASSERT(TheTarget && "vc target was not registered");

  const std::string FeaturesStr = getSubtargetFeatureString(Opts);
  // These ones do not look useful for now. Maybe will be adjusted
  // later to account for fp model.
  const TargetOptions Options;
  CodeGenOpt::Level OptLevel = getCodeGenOptLevel(Opts);
  std::unique_ptr<TargetMachine> TM{
      TheTarget->createTargetMachine(TheTriple.getTriple(), Opts.CPUStr,
                                     FeaturesStr, Options, /*RelocModel=*/None,
                                     /*CodeModel=*/None, OptLevel)};
  if (!TM)
    return make_error<vc::TargetMachineError>();
  return {std::move(TM)};
}

// Create backend options for immutable config pass. Override default
// values with provided ones.
static GenXBackendOptions createBackendOptions(const vc::CompileOptions &Opts) {
  GenXBackendOptions BackendOpts;
  if (Opts.StackMemSize)
    BackendOpts.StackSurfaceMaxSize = Opts.StackMemSize.getValue();
  BackendOpts.EnableAsmDumps = Opts.DumpAsm;
  BackendOpts.Dumper = Opts.Dumper.get();
  return BackendOpts;
}

static GenXBackendData createBackendData(const vc::ExternalData &Data) {
  GenXBackendData BackendData{Data.getOCLGenericBIFModule()};
  return BackendData;
}

static void optimizeIR(const vc::CompileOptions &Opts,
                       const vc::ExternalData &ExtData, TargetMachine &TM,
                       Module &M) {
  legacy::PassManager PerModulePasses;
  legacy::FunctionPassManager PerFunctionPasses(&M);

  PerModulePasses.add(
      createTargetTransformInfoWrapperPass(TM.getTargetIRAnalysis()));
  PerModulePasses.add(new GenXBackendConfig{createBackendOptions(Opts),
                                            createBackendData(ExtData)});
  PerFunctionPasses.add(
      createTargetTransformInfoWrapperPass(TM.getTargetIRAnalysis()));

  unsigned OptLevel;
  if (Opts.OptLevel == vc::OptimizerLevel::None)
    OptLevel = 0;
  else
    OptLevel = 2;

  PassManagerBuilder PMBuilder;
  PMBuilder.Inliner = createFunctionInliningPass(2, 2, false);
  PMBuilder.OptLevel = OptLevel;
  PMBuilder.SizeLevel = OptLevel;
  PMBuilder.SLPVectorize = false;
  PMBuilder.LoopVectorize = false;
  PMBuilder.DisableUnrollLoops = false;
  PMBuilder.MergeFunctions = false;
  PMBuilder.PrepareForThinLTO = false;
  PMBuilder.PrepareForLTO = false;
  PMBuilder.RerollLoops = true;

  TM.adjustPassManager(PMBuilder);

  PMBuilder.populateFunctionPassManager(PerFunctionPasses);
  PMBuilder.populateModulePassManager(PerModulePasses);

  // Do we need per function passes at all?
  PerFunctionPasses.doInitialization();
  for (Function &F : M) {
    if (!F.isDeclaration())
      PerFunctionPasses.run(F);
  }
  PerFunctionPasses.doFinalization();

  PerModulePasses.run(M);
}

static void dumpFinalOutput(const vc::CompileOptions &Opts, const Module &M,
                            StringRef IsaBinary) {
  if (Opts.DumpIR && Opts.Dumper)
    Opts.Dumper->dumpModule(M, "final.ll");
  if (Opts.DumpIsa && Opts.Dumper)
    Opts.Dumper->dumpBinary({IsaBinary.data(), IsaBinary.size()}, "final.isa");
}

static void populateCodeGenPassManager(const vc::CompileOptions &Opts,
                                       const vc::ExternalData &ExtData,
                                       TargetMachine &TM, raw_pwrite_stream &OS,
                                       legacy::PassManager &PM) {
  TargetLibraryInfoImpl TLII{TM.getTargetTriple()};
  PM.add(new TargetLibraryInfoWrapperPass(TLII));
  PM.add(new GenXBackendConfig{createBackendOptions(Opts),
                               createBackendData(ExtData)});
  // Non-constant pointer.
  WA_TABLE *WaTable = Opts.WATable.get();
  PM.add(new GenXWATable(WaTable));

  auto FileType = IGCLLVM::TargetMachine::CodeGenFileType::CGFT_AssemblyFile;
  bool AddPasses =
      TM.addPassesToEmitFile(PM, OS, nullptr, FileType, /*NoVerify*/ true);
  IGC_ASSERT(!AddPasses && "Bad filetype for vc-codegen");
}

static vc::ocl::CompileOutput runOclCodeGen(const vc::CompileOptions &Opts,
                                            const vc::ExternalData &ExtData,
                                            TargetMachine &TM, Module &M) {
  legacy::PassManager PM;

  SmallString<32> IsaBinary;
  raw_svector_ostream OS(IsaBinary);
  raw_null_ostream NullOS;
  if (Opts.DumpIsa)
    populateCodeGenPassManager(Opts, ExtData, TM, OS, PM);
  else
    populateCodeGenPassManager(Opts, ExtData, TM, NullOS, PM);

  std::vector<GenXOCLRuntimeInfo::CompiledKernel> CompiledKernels;
  PM.add(createGenXOCLInfoExtractorPass(CompiledKernels));

  PM.run(M);
  dumpFinalOutput(Opts, M, IsaBinary);

  vc::ocl::CompileOutput Output;
  Output.Kernels = convertInternalOCLInfo(CompiledKernels);
  Output.PointerSizeInBytes = M.getDataLayout().getPointerSize();
  return Output;
}

static vc::cm::CompileOutput runCmCodeGen(const vc::CompileOptions &Opts,
                                          const vc::ExternalData &ExtData,
                                          TargetMachine &TM, Module &M) {
  legacy::PassManager PM;
  SmallString<32> IsaBinary;
  raw_svector_ostream OS(IsaBinary);
  populateCodeGenPassManager(Opts, ExtData, TM, OS, PM);
  PM.run(M);
  dumpFinalOutput(Opts, M, IsaBinary);
  vc::cm::CompileOutput Output;
  Output.IsaBinary.assign(IsaBinary.begin(), IsaBinary.end());
  return Output;
}

static vc::CompileOutput runCodeGen(const vc::CompileOptions &Opts,
                                    const vc::ExternalData &ExtData,
                                    TargetMachine &TM, Module &M) {
  switch (Opts.Binary) {
  case vc::BinaryKind::CM:
    return runCmCodeGen(Opts, ExtData, TM, M);
  case vc::BinaryKind::OpenCL:
  case vc::BinaryKind::ZE:
    return runOclCodeGen(Opts, ExtData, TM, M);
  }
  IGC_ASSERT_EXIT_MESSAGE(0, "Unknown runtime kind");
}

Expected<vc::CompileOutput> vc::Compile(ArrayRef<char> Input,
                                        const vc::CompileOptions &Opts,
                                        const vc::ExternalData &ExtData) {
  // Environment variable for additional options for debug purposes.
  // This will exit with error if options is incorrect and should not
  // be used to pass meaningful options required for compilation.
#ifndef NDEBUG
  constexpr const char *DebugEnvVarName = "IGC_VCCodeGenDebugOpts";
  cl::ParseEnvironmentOptions("vc-codegen", DebugEnvVarName);
#endif

  if (Opts.DumpIR && Opts.Dumper)
    Opts.Dumper->dumpBinary(Input, "input.spv");

  LLVMContext Context;
  LLVMInitializeGenXTarget();
  LLVMInitializeGenXTargetInfo();
  llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeTarget(Registry);

  auto ExpModule = getModule(Input, Context);
  if (!ExpModule)
    return ExpModule.takeError();
  Module &M = *ExpModule.get();

  if (Opts.DumpIR && Opts.Dumper)
    Opts.Dumper->dumpModule(M, "after_spirv_reader.ll");

  legacy::PassManager PerModulePasses;
  PerModulePasses.add(createGenXSPIRVReaderAdaptorPass());
  PerModulePasses.add(createGenXRestoreIntrAttrPass());
  PerModulePasses.run(M);

  Triple TheTriple = overrideTripleWithVC(M.getTargetTriple());
  M.setTargetTriple(TheTriple.getTriple());

  auto ExpTargetMachine = createTargetMachine(Opts, TheTriple);
  if (!ExpTargetMachine)
    return ExpTargetMachine.takeError();
  TargetMachine &TM = *ExpTargetMachine.get();
  M.setDataLayout(TM.createDataLayout());

  // Save old value and restore at the end.
  bool TimePassesIsEnabledLocal = TimePassesIsEnabled;
  if (Opts.TimePasses)
    TimePassesIsEnabled = true;

  if (Opts.DumpIR && Opts.Dumper)
    Opts.Dumper->dumpModule(M, "after_ir_adaptors.ll");

  optimizeIR(Opts, ExtData, TM, M);

  if (Opts.DumpIR && Opts.Dumper)
    Opts.Dumper->dumpModule(M, "optimized.ll");

  vc::CompileOutput Output = runCodeGen(Opts, ExtData, TM, M);

  // Print timers if any and restore old TimePassesIsEnabled value.
  TimerGroup::printAll(llvm::errs());
  TimePassesIsEnabled = TimePassesIsEnabledLocal;

  return Output;
}

static Expected<opt::InputArgList>
parseOptions(const SmallVectorImpl<const char *> &Argv,
             vc::options::Flags FlagsToInclude) {
  const opt::OptTable &Options = vc::getOptTable();

  const bool IsInternal = FlagsToInclude == vc::options::InternalOption;

  unsigned MissingArgIndex = 0;
  unsigned MissingArgCount = 0;
  opt::InputArgList InputArgs =
      Options.ParseArgs(Argv, MissingArgIndex, MissingArgCount, FlagsToInclude);
  if (MissingArgCount)
    return make_error<vc::OptionError>(Argv[MissingArgIndex], IsInternal);

  // ocloc uncoditionally passes opencl options to internal options.
  // Skip checking of internal options for now.
  if (!IsInternal) {
    if (opt::Arg *A = InputArgs.getLastArg(vc::options::OPT_UNKNOWN,
                                           vc::options::OPT_INPUT)) {
      std::string BadOpt = A->getAsString(InputArgs);
      return make_error<vc::OptionError>(BadOpt, IsInternal);
    }
  }

  return {std::move(InputArgs)};
}

static Expected<opt::InputArgList> parseApiOptions(StringSaver &Saver,
                                                   StringRef ApiOptions) {
  SmallVector<const char *, 8> Argv;
  cl::TokenizeGNUCommandLine(ApiOptions, Saver, Argv);

  const opt::OptTable &Options = vc::getOptTable();
  // This can be rewritten to parse options and then check for
  // OPT_vc_codegen, but it would be better to manually check for
  // this option before any real parsing. If it is missing,
  // then no parsing should be done at all.
  auto HasOption = [&Argv](const std::string &Opt) {
    return std::any_of(Argv.begin(), Argv.end(),
                       [&Opt](const char *ArgStr) { return Opt == ArgStr; });
  };
  const std::string VCCodeGenOptName =
      Options.getOption(vc::options::OPT_vc_codegen).getPrefixedName();
  if (HasOption(VCCodeGenOptName))
    return parseOptions(Argv, vc::options::ApiOption);
  // Deprecated -cmc parsing just for compatibility.
  const std::string IgcmcOptName =
      Options.getOption(vc::options::OPT_igcmc).getPrefixedName();
  if (HasOption(IgcmcOptName))
    return parseOptions(Argv, vc::options::IgcmcApiOption);

  return make_error<vc::NotVCError>();
}

static Expected<opt::InputArgList>
parseInternalOptions(StringSaver &Saver, StringRef InternalOptions) {
  SmallVector<const char *, 8> Argv;
  cl::TokenizeGNUCommandLine(InternalOptions, Saver, Argv);
  return parseOptions(Argv, vc::options::InternalOption);
}

static Error makeOptionError(const opt::Arg &A, const opt::ArgList &Opts,
                             bool IsInternal) {
  const std::string BadOpt = A.getAsString(Opts);
  return make_error<vc::OptionError>(BadOpt, IsInternal);
}

static Error fillApiOptions(const opt::ArgList &ApiOptions,
                            vc::CompileOptions &Opts) {
  if (ApiOptions.hasArg(vc::options::OPT_igcmc))
    Opts.OptLevel = vc::OptimizerLevel::None;
  if (ApiOptions.hasArg(vc::options::OPT_no_vector_decomposition))
    Opts.NoVecDecomp = true;

  if (opt::Arg *A = ApiOptions.getLastArg(vc::options::OPT_optimize)) {
    StringRef Val = A->getValue();
    auto MaybeLevel = StringSwitch<Optional<vc::OptimizerLevel>>(Val)
                          .Case("none", vc::OptimizerLevel::None)
                          .Case("full", vc::OptimizerLevel::Full)
                          .Default(None);
    if (!MaybeLevel)
      return makeOptionError(*A, ApiOptions, /*IsInternal=*/false);
    Opts.OptLevel = MaybeLevel.getValue();
  }

  if (opt::Arg *A = ApiOptions.getLastArg(vc::options::OPT_igcmc_stack_size)) {
    StringRef Val = A->getValue();
    unsigned Result;
    if (Val.getAsInteger(/*Radix=*/0, Result))
      return makeOptionError(*A, ApiOptions, /*IsInternal=*/false);
    Opts.StackMemSize = Result;
  }

  return Error::success();
}

static Error fillInternalOptions(const opt::ArgList &InternalOptions,
                                 vc::CompileOptions &Opts) {
  if (InternalOptions.hasArg(vc::options::OPT_dump_isa_binary))
    Opts.DumpIsa = true;
  if (InternalOptions.hasArg(vc::options::OPT_dump_llvm_ir))
    Opts.DumpIR = true;
  if (InternalOptions.hasArg(vc::options::OPT_dump_asm))
    Opts.DumpAsm = true;
  if (InternalOptions.hasArg(vc::options::OPT_ftime_report))
    Opts.TimePasses = true;

  if (opt::Arg *A =
          InternalOptions.getLastArg(vc::options::OPT_binary_format)) {
    StringRef Val = A->getValue();
    auto MaybeBinary = StringSwitch<Optional<vc::BinaryKind>>(Val)
                           .Case("cm", vc::BinaryKind::CM)
                           .Case("ocl", vc::BinaryKind::OpenCL)
                           .Case("ze", vc::BinaryKind::ZE)
                           .Default(None);
    if (!MaybeBinary)
      return makeOptionError(*A, InternalOptions, /*IsInternal=*/true);
    Opts.Binary = MaybeBinary.getValue();
  }

  Opts.FeaturesString = llvm::join(
    InternalOptions.getAllArgValues(vc::options::OPT_target_features), ",");

  if (InternalOptions.hasArg(vc::options::OPT_help)) {
    constexpr const char *Usage = "-options \"-vc-codegen [options]\"";
    constexpr const char *Title = "Vector compiler options";
    constexpr unsigned FlagsToInclude = vc::options::ApiOption;
    constexpr unsigned FlagsToExclude = 0;
    constexpr bool ShowAllAliases = false;
    vc::getOptTable().PrintHelp(llvm::errs(), Usage, Title, FlagsToInclude,
                                FlagsToExclude, ShowAllAliases);
  }
  if (InternalOptions.hasArg(vc::options::OPT_help_internal)) {
    constexpr const char *Usage =
        "-options \"-vc-codegen\" -internal_options \"[options]\"";
    constexpr const char *Title = "Vector compiler internal options";
    constexpr unsigned FlagsToInclude = vc::options::InternalOption;
    constexpr unsigned FlagsToExclude = 0;
    constexpr bool ShowAllAliases = false;
    vc::getOptTable().PrintHelp(llvm::errs(), Usage, Title, FlagsToInclude,
                                FlagsToExclude, ShowAllAliases);
  }

  return Error::success();
}

static Expected<vc::CompileOptions>
fillOptions(const opt::ArgList &ApiOptions,
            const opt::ArgList &InternalOptions) {
  vc::CompileOptions Opts;
  Error Status = fillApiOptions(ApiOptions, Opts);
  if (Status)
    return {std::move(Status)};

  Status = fillInternalOptions(InternalOptions, Opts);
  if (Status)
    return {std::move(Status)};

  return {std::move(Opts)};
}

// Parse global llvm cl options.
// Parsing of cl codegen options should not fail under any circumstances.
static void parseLLVMOptions(const opt::ArgList &Args) {
  // Need to control cl options as vector compiler still uses these ones
  // to control compilation process. This will be addressed later.
  llvm::cl::ResetAllOptionOccurrences();
  BumpPtrAllocator Alloc;
  StringSaver Saver{Alloc};
  SmallVector<const char *, 8> Argv{"vc-codegen"};
  for (const std::string &ArgPart :
       Args.getAllArgValues(vc::options::OPT_llvm_options))
    cl::TokenizeGNUCommandLine(ArgPart, Saver, Argv);
  cl::ParseCommandLineOptions(Argv.size(), Argv.data());
}

// Derive llvm options from different API and internal options.
static opt::DerivedArgList
composeLLVMArgs(const opt::InputArgList &ApiArgs,
                const opt::InputArgList &InternalArgs,
                llvm::StringSaver &Saver) {
  const opt::OptTable &Options = vc::getOptTable();
  const opt::Option LLVMOpt = Options.getOption(vc::options::OPT_llvm_options);

  // Pass through old value.
  opt::DerivedArgList UpdatedArgs{InternalArgs};
  if (const opt::Arg *BaseArg =
          InternalArgs.getLastArg(vc::options::OPT_llvm_options))
    UpdatedArgs.AddSeparateArg(BaseArg, LLVMOpt, BaseArg->getValue());

  // Add visaopts if any.
  for (auto OptID :
       {vc::options::OPT_igcmc_visaopts, vc::options::OPT_Xfinalizer}) {
    if (!ApiArgs.hasArg(OptID))
      continue;

    const std::string FinalizerOpts =
        llvm::join(ApiArgs.getAllArgValues(OptID), " ");
    StringRef WrappedOpts =
        Saver.save(Twine{"-finalizer-opts='"} + FinalizerOpts + "'");
    UpdatedArgs.AddSeparateArg(ApiArgs.getLastArg(OptID), LLVMOpt, WrappedOpts);
  }


  if (opt::Arg *DbgArg = ApiArgs.getLastArg(vc::options::OPT_vc_emit_debug)) {

    UpdatedArgs.AddSeparateArg(DbgArg, LLVMOpt,
                               "-finalizer-opts='-generateDebugInfo'");
    UpdatedArgs.AddSeparateArg(DbgArg, LLVMOpt, "-emit-debug-info");

    // TODO: turn off the debug when debug information is stabilized
    UpdatedArgs.AddSeparateArg(
        nullptr, LLVMOpt,
        "-finalizer-opts='-dumpcommonisa -dumpvisa -output -binary'");
  }

  if (opt::Arg *GTPinReRa = ApiArgs.getLastArg(vc::options::OPT_gtpin_rera)) {
    UpdatedArgs.AddSeparateArg(GTPinReRa, LLVMOpt,
                               "-finalizer-opts='-GTPinReRA'");
  }
  if (opt::Arg *GTPinFreeGRFInfo =
          ApiArgs.getLastArg(vc::options::OPT_gtpin_grf_info)) {
    UpdatedArgs.AddSeparateArg(GTPinFreeGRFInfo, LLVMOpt,
                               "-finalizer-opts='-getfreegrfinfo -rerapostschedule'");
  }
  if (opt::Arg *GTPinScratchAreaSize =
          ApiArgs.getLastArg(vc::options::OPT_gtpin_scratch_area_size)) {
    StringRef ScratchRef =
        Saver.save(GTPinScratchAreaSize->getAsString(ApiArgs));
    auto s = "-finalizer-opts='-GTPinScratchAreaSize " +
             std::string(GTPinScratchAreaSize->getValue()) + "'";
    UpdatedArgs.AddSeparateArg(GTPinScratchAreaSize, LLVMOpt, s);
  }

  return UpdatedArgs;
}

llvm::Expected<vc::CompileOptions>
vc::ParseOptions(llvm::StringRef ApiOptions, llvm::StringRef InternalOptions) {
  llvm::BumpPtrAllocator Alloc;
  llvm::StringSaver Saver{Alloc};
  auto ExpApiArgList = parseApiOptions(Saver, ApiOptions);
  if (!ExpApiArgList)
    return ExpApiArgList.takeError();
  const opt::InputArgList &ApiArgs = ExpApiArgList.get();

  auto ExpInternalArgList = parseInternalOptions(Saver, InternalOptions);
  if (!ExpInternalArgList)
    return ExpInternalArgList.takeError();
  const opt::InputArgList &InternalArgs = ExpInternalArgList.get();

  // Prepare additional llvm options (like finalizer args).
  opt::DerivedArgList LLVMArgs = composeLLVMArgs(ApiArgs, InternalArgs, Saver);

  // This is a temporary solution until we remove all cl options that
  // are accesible by user and affect compilation.
  parseLLVMOptions(LLVMArgs);

  return fillOptions(ApiArgs, InternalArgs);
}
