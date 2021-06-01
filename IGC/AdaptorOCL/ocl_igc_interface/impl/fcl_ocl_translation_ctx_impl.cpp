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

#pragma warning(disable:4141)
#pragma warning(disable:4146)
#pragma warning(disable:4242)
#include <llvm/ADT/Triple.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/StringSaver.h>
#include <llvm/Support/Host.h>
#pragma warning(default:4242)
#pragma warning(default:4146)
#pragma warning(default:4141)

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#if defined( _DEBUG ) || defined( _INTERNAL )
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

namespace IGC {

OclTranslationOutputBase *
CIF_GET_INTERFACE_CLASS(FclOclTranslationCtx, 1)::TranslateImpl(
        CIF::Version_t outVersion,
        CIF::Builtins::BufferSimple *src,
        CIF::Builtins::BufferSimple *options,
        CIF::Builtins::BufferSimple *internalOptions,
        CIF::Builtins::BufferSimple *tracingOptions,
        uint32_t tracingOptionsCount) {

    return CIF_GET_PIMPL()->Translate(outVersion, src, options, internalOptions,
                                      tracingOptions, tracingOptionsCount);
}

}

namespace IGC {

llvm::Optional<std::vector<char>> readBinaryFile(const std::string& fileName) {
    std::ifstream file(fileName, std::ios_base::binary);
    if (!file.good()) {
        return llvm::Optional<std::vector<char>>::create(nullptr);
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

std::vector<char> makeVcOptPayload(uint64_t IR_size,
                                   const std::string& TargetFeaturesStr,
                                   const std::vector<std::string>& VcOpts) {
    std::string InternalVcOptions;
    if (!VcOpts.empty()) {
        InternalVcOptions += " -llvm-options='" + llvm::join(VcOpts, " ") + "'";
    }
    if (!TargetFeaturesStr.empty()) {
        InternalVcOptions.append(" -target-features=").append(TargetFeaturesStr);
    }
    // Payload format:
    // |-vc-codegen|c-str llvm-opts|i64(IR size)|i64(Payload size)|-vc-codegen|
    // Should be in sync with:
    //  Source/IGC/AdaptorOCL/dllInterfaceCompute.cpp
    const std::string CodegenMarker = "-vc-codegen";
    std::vector<char> VcPayload;
    VcPayload.insert(VcPayload.end(), CodegenMarker.begin(), CodegenMarker.end());
    VcPayload.insert(VcPayload.end(), InternalVcOptions.begin(), InternalVcOptions.end());
    VcPayload.push_back(0); // zero-terminate options
    uint64_t payloadSize = VcPayload.size() +
                            /* as suffix */ CodegenMarker.size() +
                            /* for sanity-checks */ + sizeof(IR_size) +
                            /* include the size itself */ + sizeof(uint64_t);
    VcPayload.insert(VcPayload.end(),
                     reinterpret_cast<char*>(&IR_size),
                     reinterpret_cast<char*>(&IR_size + 1));
    VcPayload.insert(VcPayload.end(),
                     reinterpret_cast<char*>(&payloadSize),
                     reinterpret_cast<char*>(&payloadSize + 1));
    VcPayload.insert(VcPayload.end(), CodegenMarker.begin(), CodegenMarker.end());
    return VcPayload;
}

void finalizeFEOutput(const IGC::AdaptorCM::Frontend::IOutputArgs& FEOutput,
                      const InvocationInfo& Invocation,
                      OclTranslationOutputBase& Output)
{
    auto& OutputInterface = *Output.GetImpl();
    const auto& ErrLog = FEOutput.getLog();
    if (FEOutput.getStatus() ==
        Intel::CM::ClangFE::IOutputArgs::ErrT::COMPILE_PROGRAM_FAILURE)
    {
        if (ErrLog.empty())
            OutputInterface.SetError(
                TranslationErrorType::Internal,
                "unknown error during cm source compilation");
        else
            OutputInterface.SetError(TranslationErrorType::UnhandledInput,
                                     ErrLog.c_str());
        return;
    }

    if (!ErrLog.empty())
        OutputInterface.AddWarning(ErrLog);

    const auto& IR = FEOutput.getIR();
    // This is where the tricky part starts
    // Right now we have no way to pass auxiliary options to vc-codegen backend
    // So we introduce a temporary hack to incorporate the options
    // into the underlying buffer which contains SPIRV IR
    const auto& VcOpts =
        IGC::AdaptorCM::Frontend::convertBackendArgsToVcOpts(Invocation.getBEArgs());
    const auto& TargetFeatures = Invocation.getTargetFeaturesStr();

    std::vector<char> Payload = makeVcOptPayload(IR.size(), TargetFeatures, VcOpts);

    std::vector<char> FinalOutput;
    FinalOutput.reserve(IR.size() + Payload.size());
    FinalOutput.insert(FinalOutput.end(), IR.begin(), IR.end());
    FinalOutput.insert(FinalOutput.end(), Payload.begin(), Payload.end());

    if (!OutputInterface.SetSuccessfulAndCloneOutput(FinalOutput.data(),
                                                     FinalOutput.size()))
    {
        OutputInterface.SetError(TranslationErrorType::Internal, "OOM (cm FE)");
        return;
    }
}

static std::vector<const char*>
    processFeOptions(llvm::sys::DynamicLibrary &LibInfo,
                     const std::string& inputFile,
                     CIF::Builtins::BufferSimple* options,
                     llvm::StringSaver& stringSaver,
                     const char *platform,
                     unsigned stepping) {

    llvm::SmallVector<const char*, 20> userArgs;

    std::string userArgsStr { options->GetMemory<char>(), options->GetSize<char>() };
    if (llvm::Triple(llvm::sys::getProcessTriple()).isOSWindows())
        llvm::cl::TokenizeWindowsCommandLine(userArgsStr, stringSaver, userArgs);
    else
        llvm::cl::TokenizeGNUCommandLine(userArgsStr, stringSaver, userArgs);

    auto toErase = std::remove_if(userArgs.begin(), userArgs.end(),
        [](const auto& Item) { return std::strcmp(Item, "-cmc") == 0; });
    userArgs.erase(toErase, userArgs.end());

    // this was old hack before FE can pass platform, now we prefer to use argument
    // but if it is null, it may be still useful, so let it be for a while
    auto cmfeDefaultArchOpt = llvm::sys::Process::GetEnv("IGC_CMFE_DEFAULT_ARCH");
    const std::string& cmfeDefaultArch =
        cmfeDefaultArchOpt ? cmfeDefaultArchOpt.getValue() : "SKL";

    std::vector<const char*> result = {
        "-emit-spirv",
        "-fcmocl",
        stringSaver.save(inputFile).data()
    };
    auto ItArchScanEnd = std::find_if(userArgs.begin(), userArgs.end(),
        [](const auto& Arg) { return std::strcmp(Arg, "--") == 0; });

    // if user specified exact arch we are in trouble
    // but then user knows what to do
    auto CmArchPresent = std::any_of(userArgs.begin(), ItArchScanEnd,
        [](const auto& Arg) {
          llvm::StringRef S = Arg;
          return S.startswith("-march=") || S.startswith("-mcpu=");
        });
    if (!CmArchPresent) {
      // Pass the runtime-specified architecture if user hasn't specified one
      if (platform)
        result.push_back(stringSaver.save("-march=" + std::string(platform)).data());
      else
        result.push_back(stringSaver.save("-march=" + cmfeDefaultArch).data());
    }

    result.insert(result.end(), userArgs.begin(), userArgs.end());

    auto ExtraCMOpts = llvm::sys::Process::GetEnv("IGC_ExtraCMOptions");
    if (ExtraCMOpts) {
        llvm::SmallVector<const char *, 8> Argv;
        llvm::cl::TokenizeGNUCommandLine(ExtraCMOpts.getValue(), stringSaver, Argv);
        result.insert(result.end(), Argv.begin(), Argv.end());
    }

    return result;
}

// TODO: most probably we should remove this function once FrontendWrapper
// is capable to handle in-memory objects properly
static llvm::Optional<std::string> MakeTemporaryCMSource(
    CIF::Builtins::BufferSimple* Src,
    OclTranslationOutputBase& outI) {

    auto& outputInterface = *outI.GetImpl();

    assert(Src);

    PathT workDir;

    // TODO: resulting temporary file can collide with the existing one
    auto Err = llvm::sys::fs::getPotentiallyUniqueTempFileName("tmp_cm_to_spv",
                                                               "", workDir);
    if (Err) {
        outputInterface.SetError(TranslationErrorType::Internal,
                                 "could not determine temporary directory name");
        return {};
    }
    if (llvm::sys::fs::create_directories(workDir)) {
        outputInterface.SetError(TranslationErrorType::Internal,
                                 "could not create temporary directory");
        return {};
    }

    size_t sizeToDump = Src->GetSize<char>();
    // TODO: consider removing this legacy code.
    // It looks like that we are trying to handle some kind of weird user input,
    // instead of reporting an error.
    while( (sizeToDump > 0) && (Src->GetMemory<char>()[sizeToDump - 1] == 0)) {
        --sizeToDump;
    }
    if(sizeToDump == 0){
        outputInterface.SetError(TranslationErrorType::Internal,
                                 "effective input is zero");
        return {};
    }

    // Dump Src
    PathT srcPath{ workDir.begin(), workDir.end() };
    llvm::sys::path::append(srcPath, "src.cm");

    std::string strPath(srcPath.begin(), srcPath.end());
    std::ofstream TmpSrc(strPath, std::ios::out);
    TmpSrc.write(Src->GetMemory<char>(), sizeToDump);
    TmpSrc.close();
    if (!TmpSrc) {
        outputInterface.SetError(TranslationErrorType::Internal,
                                 "could not create temporary temporary file");
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

// This essentialy duplicates existing DumpShaderFile in dllInterfaceCompute
// but now I see no way to reuse it. To be converged later
static void DumpInputs(PLATFORM *Platform, const char *Selected,
                       int Stepping, CIF::Builtins::BufferSimple* src,
                       CIF::Builtins::BufferSimple* options,
                       CIF::Builtins::BufferSimple* internalOptions,
                       CIF::Builtins::BufferSimple* tracingOptions) {
#if defined( _DEBUG ) || defined( _INTERNAL )
  // check for shader dumps enabled
  if (!FCL_IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
    return;

  auto AsStringRef = [](CIF::Builtins::BufferSimple* Src) {
     if (!Src)
       return llvm::StringRef();
     size_t Size = Src->GetSize<char>();
     const char* Buff = Src->GetMemory<char>();
     return llvm::StringRef(Buff, Size);
  };
  // dump if yes
  std::ostringstream Os;
  if (Platform) {
    auto Core = Platform->eDisplayCoreFamily;
    auto RenderCore = Platform->eRenderCoreFamily;
    auto Product = Platform->eProductFamily;
    auto RevId = Platform->usRevId;

    Os << "NEO passed: DisplayCore = " << Core
       << ", RenderCore = " << RenderCore << ", Product = " << Product
       << ", Revision = " << RevId << "\n";
    Os << "IGC translated into: " << Selected << ", " << Stepping << "\n";
  } else {
    Os << "Nothing came from NEO\n";
  }
  auto PlatformInfo = Os.str();

  struct DumpDescriptor {
    llvm::StringRef OutputName;
    llvm::StringRef Contents;
  };
  std::array<DumpDescriptor, 5> Dumps = {{
    { "platform", PlatformInfo.c_str() },
    { "options", AsStringRef(options) },
    { "internal_options", AsStringRef(internalOptions) },
    { "tracing_options", AsStringRef(tracingOptions) },
    { "source_code", AsStringRef(src) }
  }};
  using HashType = unsigned long long;
  auto Hash = std::accumulate(Dumps.begin(), Dumps.end(), 0ull,
      [](const auto& HashVal, const auto& DI) {
          const auto *BuffStart = DI.Contents.data();
          auto BuffSize = DI.Contents.size();
          return HashVal ^ iSTD::HashFromBuffer(BuffStart, BuffSize);
      });

  std::for_each(Dumps.begin(), Dumps.end(),
      [&Hash](const auto &DI) {
          // do factual dump, this part can be replaced to DumpShaderFile
          const char *DstDir = FCL::GetShaderOutputFolder();
          std::ostringstream FullPath(DstDir, std::ostringstream::ate);
          FullPath << "VC_fe_"  << std::hex << std::setfill('0')
              << std::setw(sizeof(Hash) * CHAR_BIT / 4) << Hash << std::dec
              << std::setfill(' ') << "_" << DI.OutputName.str() << ".txt";
          // NOTE: for now, we expect only text data here
          std::ofstream OutF(FullPath.str(), std::ofstream::out);
          // if we can create dump file at all...
          if (OutF)
            OutF.write(DI.Contents.data(), DI.Contents.size());
      });

#endif // defined( _DEBUG ) || defined( _INTERNAL )
}

OclTranslationOutputBase* CIF_PIMPL(FclOclTranslationCtx)::TranslateCM(
    CIF::Version_t outVersion,
    CIF::Builtins::BufferSimple* src,
    CIF::Builtins::BufferSimple* options,
    CIF::Builtins::BufferSimple* internalOptions,
    CIF::Builtins::BufferSimple* tracingOptions,
    uint32_t tracingOptionsCount) {

    // Output
    auto* outputInterface =
        CIF::InterfaceCreator<OclTranslationOutput>::CreateInterfaceVer(
            outVersion, outType);
    if (outputInterface == nullptr)
        return nullptr;

    PLATFORM *platformDescr = nullptr;
    const char *platformStr = nullptr;
    unsigned stepping       = 0U;

    if(globalState.GetPlatformImpl()){
      // NEO supports platform interface
      auto *PlatformImpl = globalState.GetPlatformImpl();
      platformDescr = &PlatformImpl->p;
      stepping = PlatformImpl->p.usRevId;
      platformStr = cmc::getPlatformStr(PlatformImpl->p, /* inout */ stepping);
    }

    DumpInputs(platformDescr, platformStr, stepping,
               src, options, internalOptions, tracingOptions);

    OclTranslationOutputBase& Out = *outputInterface;

#if defined(IGC_VC_ENABLED)
    auto ErrFn = [&Out](const std::string& Err) {
        Out.GetImpl()->SetError(TranslationErrorType::Internal, Err.c_str());
    };
    auto MaybeFE =
        IGC::AdaptorCM::Frontend::makeFEWrapper(ErrFn, getCMFEWrapperDir());
    if (!MaybeFE)
        return outputInterface;

    auto OptSrc = MakeTemporaryCMSource(src, Out);
    if (!OptSrc)
        return outputInterface; // proper error message is already set

    llvm::BumpPtrAllocator A;
    llvm::StringSaver Saver(A);
    auto& FE = MaybeFE.getValue();
    auto FeArgs = processFeOptions(FE.LibInfo(), OptSrc.getValue(),
                                   options, Saver, platformStr, stepping);

    auto Drv = FE.buildDriverInvocation(FeArgs.size(), FeArgs.data());
    if (!Drv)
    {
        ErrFn("Null driver invocation in CMFE");
        return outputInterface;
    }
    if (Drv->getOutputType() != InvocationInfo::OutputTypeT::SPIRV)
    {
        ErrFn("CM frontend: unsupported output request");
        return outputInterface;
    }

    IGC::AdaptorCM::Frontend::InputArgs InputArgs;
    InputArgs.CompilationOpts = Drv->getFEArgs();
    auto FEOutput = FE.translate(InputArgs);
    if (!FEOutput)
    {
        ErrFn("Null output in CMFE");
        return outputInterface;
    }

    finalizeFEOutput(*FEOutput, *Drv, Out);

#else
    Out.GetImpl()->SetError(TranslationErrorType::Internal,
                            "CM compilation is not supported in this configuration");
#endif // defined(IGC_VC_ENABLED)

    return outputInterface;
}

}

#include "cif/macros/disable.h"
