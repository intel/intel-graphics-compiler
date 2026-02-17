/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/impl/fcl_ocl_translation_ctx_impl.h"
#include "AdaptorCommon/customApi.hpp"
#include "common/StringMacros.hpp"
#include "common/VCPlatformSelector.hpp"
#include "common/debug/Dump.hpp"
#include "ocl_igc_interface/fcl_ocl_translation_ctx.h"
#include "ocl_igc_interface/impl/platform_impl.h"
#include "3d/common/iStdLib/utility.h"
#include "3d/common/iStdLib/File.h"
#include "OCLFE/igd_fcl_mcl/headers/clang_tb.h"

#include "llvmWrapper/TargetParser/Triple.h"
#include <llvmWrapper/ADT/Optional.h>

#include "IGC/common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/StringSaver.h>
#include <llvm/Support/Host.h>
#include "IGC/common/LLVMWarningsPop.hpp"

#include <optional>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#if defined(__ANDROID__)
#include <sstream>
#endif

#if defined(_DEBUG) || defined(_INTERNAL)
#include <numeric>
#include <array>
#endif // defined( _DEBUG ) || defined( _INTERNAL )
#if defined(IGC_VC_ENABLED)
#include "Frontend.h"
#if defined(_WIN32)
#include <Windows.h>
#include "inc/common/DriverStore.h"
#endif
#endif // defined(IGC_VC_ENABLED)

#include "cif/macros/enable.h"

#include <memory>

namespace {

using HashType = unsigned long long;

struct FeShaderDumpData {
  struct DumpDescriptor {
    llvm::StringRef OutputName;
    llvm::StringRef Contents;
  };
  FeShaderDumpData(PLATFORM *Platform, const char *Selected, int Stepping, CIF::Builtins::BufferSimple *src,
                   CIF::Builtins::BufferSimple *options, CIF::Builtins::BufferSimple *internalOptions,
                   CIF::Builtins::BufferSimple *tracingOptions);
  std::vector<DumpDescriptor> getDumps() const;

private:
  std::string PlatformInformation;
  CIF::Builtins::BufferSimple *InputSource = nullptr;
  CIF::Builtins::BufferSimple *Options = nullptr;
  CIF::Builtins::BufferSimple *InternalOptions = nullptr;
  CIF::Builtins::BufferSimple *TracingOptions = nullptr;
};

[[maybe_unused]] FeShaderDumpData::FeShaderDumpData(PLATFORM *Platform, const char *Selected, int Stepping,
                                                    CIF::Builtins::BufferSimple *Src, CIF::Builtins::BufferSimple *Opts,
                                                    CIF::Builtins::BufferSimple *InternalOpts,
                                                    CIF::Builtins::BufferSimple *TracingOpts) {
  std::ostringstream Os;
  if (Platform) {
    auto Core = Platform->eDisplayCoreFamily;
    auto RenderCore = Platform->eRenderCoreFamily;
    auto Product = Platform->eProductFamily;
    auto RevId = Platform->usRevId;

    Os << "NEO passed: DisplayCore = " << Core << ", RenderCore = " << RenderCore << ", Product = " << Product
       << ", Revision = " << RevId << "\n";
    Os << "IGC translated into: " << Selected << ", " << Stepping << "\n";
  } else {
    Os << "Nothing came from NEO\n";
  }
  PlatformInformation = Os.str();

  InputSource = Src;
  Options = Opts;
  InternalOptions = InternalOpts;
  TracingOptions = TracingOpts;
}

std::vector<FeShaderDumpData::DumpDescriptor> FeShaderDumpData::getDumps() const {
  auto AsStringRef = [](CIF::Builtins::BufferSimple *Src) {
    if (!Src)
      return llvm::StringRef();
    size_t Size = Src->GetSize<char>();
    const char *Buff = Src->GetMemory<char>();
    return llvm::StringRef(Buff, Size);
  };
  return {{"platform", PlatformInformation.c_str()},
          {"source_code", AsStringRef(InputSource)},
          {"options", AsStringRef(Options)},
          {"internal_options", AsStringRef(InternalOptions)},
          {"tracing_options", AsStringRef(TracingOptions)}};
}

std::optional<FeShaderDumpData> getShaderDumpData(PLATFORM *Platform, const char *Selected, int Stepping,
                                                  CIF::Builtins::BufferSimple *Src,
                                                  CIF::Builtins::BufferSimple *Options,
                                                  CIF::Builtins::BufferSimple *InternalOptions,
                                                  CIF::Builtins::BufferSimple *TracingOptions) {
#if defined(_DEBUG) || defined(_INTERNAL)
  if (!FCL_IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
    return {};

  return FeShaderDumpData(Platform, Selected, Stepping, Src, Options, InternalOptions, TracingOptions);
#else
  return {};
#endif
}

void emitFCLDump(llvm::StringRef FilenamePrefix, FeShaderDumpData::DumpDescriptor DataToDump) {
#if defined(_DEBUG) || defined(_INTERNAL)
  // do factual dump, this part can be replaced to DumpShaderFile
  const char *DstDir = FCL::GetShaderOutputFolder();
  std::ostringstream FullPath(DstDir, std::ostringstream::ate);
  FullPath << FilenamePrefix.str() << "_" << DataToDump.OutputName.str() << ".txt";
  // NOTE: for now, we expect only text data here
  std::ofstream OutF(FullPath.str(), std::ofstream::out);
  // if we can create dump file at all...
  if (OutF)
    OutF.write(DataToDump.Contents.data(), DataToDump.Contents.size());
#endif
}

std::string makeShaderDumpPrefixWithHash(llvm::StringRef Prefix, HashType Hash) {
  std::ostringstream OS;
  OS << Prefix.str() << std::hex << std::setfill('0') << std::setw(sizeof(Hash) * CHAR_BIT / 4) << Hash;
  return OS.str();
}

void DoShaderDumpPreFE(const FeShaderDumpData &DumpInfo) {
  auto Dumps = DumpInfo.getDumps();
  auto Hash = std::accumulate(Dumps.begin(), Dumps.end(), 0ull, [](const auto &HashVal, const auto &DI) {
    const auto *BuffStart = DI.Contents.data();
    auto BuffSize = DI.Contents.size();
    return HashVal ^ iSTD::HashFromBuffer(BuffStart, BuffSize);
  });
  auto DumpNamePrefix = makeShaderDumpPrefixWithHash("VC_fe_", Hash);
  for (const auto &DumpDescriptor : Dumps)
    emitFCLDump(DumpNamePrefix, DumpDescriptor);
};

void DoShaderDumpPostFE(const FeShaderDumpData &DumpInfo, HashType Hash) {
  auto DumpNamePrefix = makeShaderDumpPrefixWithHash("VC_PostFe_", Hash);
  for (const auto &DumpDescriptor : DumpInfo.getDumps())
    emitFCLDump(DumpNamePrefix, DumpDescriptor);
};

} // namespace

namespace IGC {

OclTranslationOutputBase *CIF_GET_INTERFACE_CLASS(FclOclTranslationCtx, 1)::TranslateImpl(
    CIF::Version_t outVersion, CIF::Builtins::BufferSimple *src, CIF::Builtins::BufferSimple *options,
    CIF::Builtins::BufferSimple *internalOptions, CIF::Builtins::BufferSimple *tracingOptions,
    uint32_t tracingOptionsCount) {

  return CIF_GET_PIMPL()->Translate(outVersion, src, options, internalOptions, tracingOptions, tracingOptionsCount, 0);
}

void CIF_GET_INTERFACE_CLASS(FclOclTranslationCtx, 2)::GetFclOptions(CIF::Builtins::BufferSimple *opts) {
  CIF_GET_PIMPL()->GetFclOptions(opts);
}

void CIF_GET_INTERFACE_CLASS(FclOclTranslationCtx, 2)::GetFclInternalOptions(CIF::Builtins::BufferSimple *opts) {
  CIF_GET_PIMPL()->GetFclInternalOptions(opts);
}

OclTranslationOutputBase *CIF_GET_INTERFACE_CLASS(FclOclTranslationCtx, 3)::TranslateImpl(
    CIF::Version_t outVersion, CIF::Builtins::BufferSimple *src, CIF::Builtins::BufferSimple *options,
    CIF::Builtins::BufferSimple *internalOptions, CIF::Builtins::BufferSimple *tracingOptions,
    uint32_t tracingOptionsCount, uint64_t srcHash) {

  return CIF_GET_PIMPL()->Translate(outVersion, src, options, internalOptions, tracingOptions, tracingOptionsCount,
                                    srcHash);
}

std::optional<std::vector<char>> readBinaryFile(const std::string &fileName) {
  std::ifstream file(fileName, std::ios_base::binary);
  if (!file.good()) {
    return std::nullopt;
  }
  size_t length;
  file.seekg(0, file.end);
  length = static_cast<size_t>(file.tellg());
  file.seekg(0, file.beg);
  std::vector<char> binary(length);
  file.read(binary.data(), length);
  return binary;
}

#if defined(IGC_VC_ENABLED)

using InvocationInfo = IGC::AdaptorCM::Frontend::IDriverInvocation;
using PathT = llvm::SmallVector<char, 1024>;

using CMStringVect = IGC::AdaptorCM::Frontend::StringVect_t;
static std::vector<char> makeVcOptPayload(uint64_t IR_size, const std::string &VCApiOpts,
                                          const CMStringVect &VCLLVMOptions, const std::string &TargetFeaturesStr) {
  std::string InternalVcOptions;
  if (!VCLLVMOptions.empty()) {
    InternalVcOptions += " -llvm-options='" + llvm::join(VCLLVMOptions, " ") + "'";
  }
  if (!TargetFeaturesStr.empty()) {
    InternalVcOptions.append(" -target-features=").append(TargetFeaturesStr);
  }
  // Payload format:
  // |-vc-payload|api opts|internal opts|i64(IR size)|i64(Payload size)|-vc-payload|
  // NOTE: <api/internal opts> are c-strings.
  //
  // Should be in sync with:
  //      Source/IGC/AdaptorOCL/dllInterfaceCompute.cpp
  const std::string PayloadMarker = "-vc-payload";
  std::vector<char> VcPayload;
  VcPayload.insert(VcPayload.end(), PayloadMarker.begin(), PayloadMarker.end());
  VcPayload.insert(VcPayload.end(), VCApiOpts.begin(), VCApiOpts.end());
  VcPayload.push_back(0); // zero-terminate api options
  VcPayload.insert(VcPayload.end(), InternalVcOptions.begin(), InternalVcOptions.end());
  VcPayload.push_back(0); // zero-terminate internal options
  uint64_t payloadSize = VcPayload.size() +
                         /* as suffix */ PayloadMarker.size() +
                         /* for sanity-checks */ +sizeof(IR_size) +
                         /* include the size itself */ +sizeof(uint64_t);
  VcPayload.insert(VcPayload.end(), reinterpret_cast<const char *>(&IR_size),
                   reinterpret_cast<const char *>(&IR_size + 1));
  VcPayload.insert(VcPayload.end(), reinterpret_cast<const char *>(&payloadSize),
                   reinterpret_cast<const char *>(&payloadSize + 1));
  VcPayload.insert(VcPayload.end(), PayloadMarker.begin(), PayloadMarker.end());
  return VcPayload;
}

static void finalizeFEOutput(const IGC::AdaptorCM::Frontend::IOutputArgs &FEOutput, const std::string &VCApiOptions,
                             const CMStringVect &VCLLVMOptions, const std::string &TargetFeatures,
                             OclTranslationOutputBase &Output, const std::optional<FeShaderDumpData> &ShaderDumpInfo) {
  auto &OutputInterface = *Output.GetImpl();
  const auto &ErrLog = FEOutput.getLog();
  if (FEOutput.getStatus() == Intel::CM::ClangFE::IOutputArgs::ErrT::COMPILE_PROGRAM_FAILURE) {
    if (ErrLog.empty())
      OutputInterface.SetError(TranslationErrorType::Internal, "unknown error during cm source compilation");
    else
      OutputInterface.SetError(TranslationErrorType::UnhandledInput, ErrLog.c_str());
    return;
  }

  if (!ErrLog.empty())
    OutputInterface.AddWarning(ErrLog);

  const auto &IR = FEOutput.getIR();
  // This is where the tricky part starts
  // Right now we have no way to pass auxiliary options to vc-codegen backend
  // So we introduce a temporary hack to incorporate the options
  // into the underlying buffer which contains SPIRV IR
  std::vector<char> Payload = makeVcOptPayload(IR.size(), VCApiOptions, VCLLVMOptions, TargetFeatures);

  std::vector<char> FinalOutput;
  FinalOutput.reserve(IR.size() + Payload.size());
  FinalOutput.insert(FinalOutput.end(), IR.begin(), IR.end());
  FinalOutput.insert(FinalOutput.end(), Payload.begin(), Payload.end());

  if (!OutputInterface.SetSuccessfulAndCloneOutput(FinalOutput.data(), FinalOutput.size())) {
    OutputInterface.SetError(TranslationErrorType::Internal, "OOM (cm FE)");
    return;
  }

  if (ShaderDumpInfo) {
    auto HashValue = iSTD::HashFromBuffer(FinalOutput.data(), FinalOutput.size());
    DoShaderDumpPostFE(*ShaderDumpInfo, HashValue);
  }
}

static std::optional<std::string> MakeTemporaryCMSource(CIF::Builtins::BufferSimple *Src, std::string tmpFilename,
                                                        OclTranslationOutputBase &outI);

static bool processCmSrcOptions(llvm::SmallVectorImpl<const char *> &userArgs, std::string optname,
                                std::string &inputFile) {

  auto toErase = std::find_if(userArgs.begin(), userArgs.end(),
                              [&optname](const auto &Item) { return std::strcmp(Item, optname.c_str()) == 0; });
  if (toErase != userArgs.end()) {
    auto itFilename = toErase + 1;
    if (itFilename != userArgs.end() && strcmp(*itFilename, "--") != 0) {
      inputFile = *itFilename;
      userArgs.erase(toErase, toErase + 2);
      return true;
    }
  }

  optname += "=";
  toErase = std::find_if(userArgs.begin(), userArgs.end(), [&optname](const auto &Item) {
    llvm::StringRef S = Item;
    return S.startswith(optname);
  });
  if (toErase != userArgs.end()) {
    inputFile = *toErase;
    inputFile = inputFile.substr(optname.length());
    userArgs.erase(toErase);
    return true;
  }

  return false;
}

static std::vector<const char *> processFeOptions(llvm::sys::DynamicLibrary &LibInfo, CIF::Builtins::BufferSimple *Src,
                                                  OclTranslationOutputBase &outI, CIF::Builtins::BufferSimple *options,
                                                  llvm::StringSaver &stringSaver, const char *platform,
                                                  unsigned stepping, bool &isMemFile) {

  llvm::SmallVector<const char *, 20> userArgs;

  std::string userArgsStr{options->GetMemory<char>(), options->GetSize<char>()};
  if (llvm::Triple(llvm::sys::getProcessTriple()).isOSWindows())
    llvm::cl::TokenizeWindowsCommandLine(userArgsStr, stringSaver, userArgs);
  else
    llvm::cl::TokenizeGNUCommandLine(userArgsStr, stringSaver, userArgs);

  auto toErase =
      std::remove_if(userArgs.begin(), userArgs.end(), [](const auto &Item) { return std::strcmp(Item, "-cmc") == 0; });
  userArgs.erase(toErase, userArgs.end());

  // this was old hack before FE can pass platform, now we prefer to use argument
  // but if it is null, it may be still useful, so let it be for a while
  auto cmfeDefaultArchOpt = IGCLLVM::makeOptional(llvm::sys::Process::GetEnv("IGC_CMFE_DEFAULT_ARCH"));
  const std::string &cmfeDefaultArch = cmfeDefaultArchOpt ? cmfeDefaultArchOpt.value() : "";

  std::string inputFile = "src.cm";
  isMemFile = processCmSrcOptions(userArgs, "-cm-src", inputFile) || processCmSrcOptions(userArgs, "-s", inputFile);
  if (!isMemFile) {
    auto OptSrc = MakeTemporaryCMSource(Src, inputFile, outI);
    if (!OptSrc)
      return {};
    inputFile = OptSrc.value();
  }

  std::vector<const char *> result = {
      "-emit-spirv",
  };

  // Pass the runtime-specified architecture.
  // User still can override it if -march is passed in user arguments
  if (platform)
    result.push_back(stringSaver.save("-march=" + std::string(platform)).data());
  else
    result.push_back(stringSaver.save("-march=" + cmfeDefaultArch).data());

  result.push_back(stringSaver.save(inputFile).data());
  result.insert(result.end(), userArgs.begin(), userArgs.end());

  auto ExtraCMOpts = IGCLLVM::makeOptional(llvm::sys::Process::GetEnv("IGC_ExtraCMOptions"));
  if (ExtraCMOpts) {
    llvm::SmallVector<const char *, 8> Argv;
    llvm::cl::TokenizeGNUCommandLine(ExtraCMOpts.value(), stringSaver, Argv);
    result.insert(result.end(), Argv.begin(), Argv.end());
  }

  return result;
}

// TODO: most probably we should remove this function once FrontendWrapper
// is capable to handle in-memory objects properly
static std::optional<std::string> MakeTemporaryCMSource(CIF::Builtins::BufferSimple *Src, std::string tmpFilename,
                                                        OclTranslationOutputBase &outI) {

  auto &outputInterface = *outI.GetImpl();

  assert(Src);

  PathT workDir;

  // TODO: resulting temporary file can collide with the existing one
  auto Err = llvm::sys::fs::getPotentiallyUniqueTempFileName("tmp_cm_to_spv", "", workDir);
  if (Err) {
    outputInterface.SetError(TranslationErrorType::Internal, "could not determine temporary directory name");
    return {};
  }
  if (llvm::sys::fs::create_directories(workDir)) {
    outputInterface.SetError(TranslationErrorType::Internal, "could not create temporary directory");
    return {};
  }

  size_t sizeToDump = Src->GetSize<char>();
  // TODO: consider removing this legacy code.
  // It looks like that we are trying to handle some kind of weird user input,
  // instead of reporting an error.
  while ((sizeToDump > 0) && (Src->GetMemory<char>()[sizeToDump - 1] == 0)) {
    --sizeToDump;
  }
  if (sizeToDump == 0) {
    outputInterface.SetError(TranslationErrorType::Internal, "effective input is zero");
    return {};
  }

  // Dump Src
  PathT srcPath{workDir.begin(), workDir.end()};
  llvm::sys::path::append(srcPath, tmpFilename);

  std::string strPath(srcPath.begin(), srcPath.end());
  std::ofstream TmpSrc(strPath, std::ios::out);
  TmpSrc.write(Src->GetMemory<char>(), sizeToDump);
  TmpSrc.close();
  if (!TmpSrc) {
    outputInterface.SetError(TranslationErrorType::Internal, "could not create temporary temporary file");
    return {};
  }
  return strPath;
}

// Retrieve directory where FEWrapper is located.
// Return driver store path on windows and empty path on linux.
static std::string getCMFEWrapperDir() {
#if defined(_WIN32)
  // Expand libname to full driver path on windows.
  char DriverPath[MAX_PATH] = {};
  GetDependencyPath(DriverPath, "");
  return DriverPath;
#else
  return "";
#endif
}

#endif // defined(IGC_VC_ENABLED)

OclTranslationOutputBase *CIF_PIMPL(FclOclTranslationCtx)::TranslateCM(CIF::Version_t outVersion,
                                                                       CIF::Builtins::BufferSimple *src,
                                                                       CIF::Builtins::BufferSimple *options,
                                                                       CIF::Builtins::BufferSimple *internalOptions,
                                                                       CIF::Builtins::BufferSimple *tracingOptions,
                                                                       uint32_t tracingOptionsCount) {

  // Output
  auto *outputInterface = CIF::InterfaceCreator<OclTranslationOutput>::CreateInterfaceVer(outVersion, outType);
  if (outputInterface == nullptr)
    return nullptr;

  PLATFORM *platformDescr = nullptr;
  const char *platformStr = nullptr;
  unsigned stepping = 0U;

  if (globalState.GetPlatformImpl()) {
    // NEO supports platform interface
    auto *PlatformImpl = globalState.GetPlatformImpl();
    platformDescr = &PlatformImpl->p;
    stepping = PlatformImpl->p.usRevId;
    platformStr = cmc::getPlatformStr(PlatformImpl->p, /* inout */ stepping);
  }

  // we need output shader dumps before reporting fatal error for
  // unsupported/empty platform
  auto ShaderDumpData = getShaderDumpData(platformDescr, platformStr ? platformStr : "(empty)", stepping, src, options,
                                          internalOptions, tracingOptions);
  if (ShaderDumpData)
    DoShaderDumpPreFE(*ShaderDumpData);

  if (platformStr == nullptr) {
    // llvm::report_fatal_error("TranslateCM: unsupported platform");
    // this is temporary to unblock customers
    platformStr = "SKL";
  }

  OclTranslationOutputBase &Out = *outputInterface;

#if defined(IGC_VC_ENABLED)
  auto ErrFn = [&Out](const std::string &Err) { Out.GetImpl()->SetError(TranslationErrorType::Internal, Err.c_str()); };

  auto MaybeFE = IGC::AdaptorCM::Frontend::makeFEWrapper(ErrFn, getCMFEWrapperDir());

  if (!MaybeFE)
    return outputInterface;

  llvm::BumpPtrAllocator A;
  llvm::StringSaver Saver(A);
  auto &FE = MaybeFE.value();
  bool isMemFile = false;
  auto FeArgs = processFeOptions(FE.LibInfo(), src, Out, options, Saver, platformStr, stepping, isMemFile);
  if (FeArgs.empty())
    return outputInterface; // proper error message is already set

  auto Drv = FE.buildDriverInvocation(FeArgs.size(), FeArgs.data());
  if (!Drv) {
    ErrFn("Null driver invocation in CMFE");
    return outputInterface;
  }
  if (Drv->getOutputType() != InvocationInfo::OutputTypeT::SPIRV) {
    ErrFn("CM frontend: unsupported output request");
    return outputInterface;
  }

  IGC::AdaptorCM::Frontend::InputArgs InputArgs;
  InputArgs.CompilationOpts = Drv->getFEArgs();
  // if real file is used, then don't fill InputArgs.InputText
  // with content of source to avoid compilation from
  // both memory and real temporary files
  if (isMemFile)
    InputArgs.InputText = src->GetMemory<char>();

  // CM frontend option parser is not thread-safe, so guard the frontend call
  // with a mutex.
  auto FEOutput = [&]() {
    static std::mutex M;
    const std::lock_guard<std::mutex> Lock(M);
    return FE.translate(InputArgs);
  }();

  if (!FEOutput) {
    ErrFn("Null output in CMFE");
    return outputInterface;
  }

  const auto &TargetFeatures = Drv->getTargetFeaturesStr();
  const auto &[VCLLVMOpts, VCFinalizerOpts] =
      IGC::AdaptorCM::Frontend::convertBackendArgsToVcAndFinalizerOpts(Drv->getBEArgs());
  auto VCApiOpts = FE.getVCApiOptions(&*Drv);
  if (!VCFinalizerOpts.empty())
    VCApiOpts += " -Xfinalizer '" + llvm::join(VCFinalizerOpts, " ") + "'";

  finalizeFEOutput(*FEOutput, VCApiOpts, VCLLVMOpts, TargetFeatures, Out, ShaderDumpData);

  FclInternalOpts = std::string(" -llvm-options='" + llvm::join(VCLLVMOpts, " ") + "'");
  FclOpts = VCApiOpts;

#else
  Out.GetImpl()->SetError(TranslationErrorType::Internal, "CM compilation is not supported in this configuration");
#endif // defined(IGC_VC_ENABLED)

  return outputInterface;
}

} // namespace IGC

#include "cif/macros/disable.h"
