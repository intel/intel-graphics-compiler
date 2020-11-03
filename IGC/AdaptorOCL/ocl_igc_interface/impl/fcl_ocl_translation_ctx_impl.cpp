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

#include "ocl_igc_interface/fcl_ocl_translation_ctx.h"
#include "ocl_igc_interface/impl/fcl_ocl_translation_ctx_impl.h"
#include "ocl_igc_interface/impl/platform_impl.h"

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
#include <llvm/Support/StringSaver.h>
#include <llvm/Support/Host.h>
#pragma warning(default:4242)
#pragma warning(default:4146)
#pragma warning(default:4141)

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#if !defined(WDDM_LINUX) && (!defined(IGC_VC_DISABLED) || !IGC_VC_DISABLED)
#include "Frontend.h"
#endif // !defined(WDDM_LINUX) && (!defined(IGC_VC_DISABLED) || !IGC_VC_DISABLED)

#include "cif/macros/enable.h"

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

#if !defined(WDDM_LINUX) && (!defined(IGC_VC_DISABLED) || !IGC_VC_DISABLED)

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
    processFeOptions(const std::string& inputFile,
                     CIF::Builtins::BufferSimple* options,
                     llvm::StringSaver& stringSaver) {

    llvm::SmallVector<const char*, 20> userArgs;

    std::string userArgsStr { options->GetMemory<char>(), options->GetSize<char>() };
    if (llvm::Triple(llvm::sys::getProcessTriple()).isOSWindows())
        llvm::cl::TokenizeWindowsCommandLine(userArgsStr, stringSaver, userArgs);
    else
        llvm::cl::TokenizeGNUCommandLine(userArgsStr, stringSaver, userArgs);

    auto toErase = std::remove_if(userArgs.begin(), userArgs.end(),
        [](const auto& Item) { return std::strcmp(Item, "-cmc") == 0; });
    userArgs.erase(toErase, userArgs.end());

    // WARNING-WARNING-WARNING*****DIRTY HACK*****WARNING-WARNING-WARNING
    // The concept of a "default architecuture" for CM Frontend is a temporary
    // workaround.
    // The problem  is that in order to process CM code CM Frontend must know
    // the target architecture.
    // Currentl,y OCL runtime does not expose the target architecture
    // in a format which CM Frontend understands. The information about
    // the target architecture is available only when backend is invoked
    // (through PLATFORM data structure).
    // The expectation is that we should modify the behavior of OCL runtime
    // to propagate the necessary arguments for CMFE
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
    auto CmArchPresent = std::any_of(userArgs.begin(), ItArchScanEnd,
        [](const auto& Arg) {
          llvm::StringRef S = Arg;
          return S.startswith("-march=") || S.startswith("-mcpu=");
        });
    if (!CmArchPresent) {
      // Pass the default architecture if user hasn't specified one
      result.push_back(stringSaver.save("-march=" + cmfeDefaultArch).data());
    }

    auto auxIncludes = llvm::sys::Process::GetEnv("CM_INCLUDE_DIR");
    if (auxIncludes) {
        result.push_back(stringSaver.save("-isystem").data());
        result.push_back(stringSaver.save(auxIncludes.getValue()).data());
    }
    result.insert(result.end(), userArgs.begin(), userArgs.end());
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

#endif // !defined(WDDM_LINUX) && (!defined(IGC_VC_DISABLED) || !IGC_VC_DISABLED)

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

    PRODUCT_FAMILY product = IGFX_UNKNOWN;
    uint32_t stepping = 0U;

    if(globalState.GetPlatformImpl()){
        product = globalState.GetPlatformImpl()->p.eProductFamily;
        stepping = globalState.GetPlatformImpl()->p.usRevId;
    }

    (void)product;
    (void)stepping;

    OclTranslationOutputBase& Out = *outputInterface;

#if !defined(WDDM_LINUX) && (!defined(IGC_VC_DISABLED) || !IGC_VC_DISABLED)
    auto ErrFn = [&Out](const std::string& Err) {
        Out.GetImpl()->SetError(TranslationErrorType::Internal, Err.c_str());
    };
    auto MaybeFE = IGC::AdaptorCM::Frontend::makeFEWrapper(ErrFn);
    if (!MaybeFE)
        return outputInterface;

    auto OptSrc = MakeTemporaryCMSource(src, Out);
    if (!OptSrc)
        return outputInterface; // proper error message is already set

    llvm::BumpPtrAllocator A;
    llvm::StringSaver Saver(A);
    auto FeArgs = processFeOptions(OptSrc.getValue(), options, Saver);

    auto& FE = MaybeFE.getValue();
    auto Drv = FE.buildDriverInvocation(FeArgs);
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
#endif // !defined(WDDM_LINUX) && (!defined(IGC_VC_DISABLED) || !IGC_VC_DISABLED)

    return outputInterface;
}

}

#include "cif/macros/disable.h"
