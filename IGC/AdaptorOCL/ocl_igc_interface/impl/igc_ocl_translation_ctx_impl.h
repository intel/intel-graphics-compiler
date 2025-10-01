/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/igc_ocl_translation_ctx.h"
#include "ocl_igc_interface/impl/igc_ocl_device_ctx_impl.h"

#include <iomanip>
#include <memory>
#include <setjmp.h>
#include <signal.h>

#include "cif/builtins/memory/buffer/impl/buffer_impl.h"
#include "cif/helpers/error.h"
#include "cif/export/pimpl_base.h"

#include "ocl_igc_interface/impl/ocl_translation_output_impl.h"

#include "AdaptorOCL/OCL/TB/igc_tb.h"
#include "common/debug/Debug.hpp"

#include "cif/macros/enable.h"
#include <spirv-tools/libspirv.h>

#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "OCLAPI/oclapi.h"

namespace TC {

// Taken from dllInterfaceCompute
bool ProcessElfInput(STB_TranslateInputArgs &InputArgs, STB_TranslateOutputArgs &OutputArgs,
                     IGC::OpenCLProgramContext &Context, PLATFORM &platform, const TB_DATA_FORMAT &outType,
                     float profilingTimerResolution);

bool TranslateBuild(const STB_TranslateInputArgs *pInputArgs, STB_TranslateOutputArgs *pOutputArgs,
                    TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform &platform, float profilingTimerResolution);

bool TranslateBuildSPMD(const STB_TranslateInputArgs *pInputArgs, STB_TranslateOutputArgs *pOutputArgs,
                        TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform &platform,
                        float profilingTimerResolution, const ShaderHash &inputShHash);

bool TranslateBuildVC(const STB_TranslateInputArgs *pInputArgs, STB_TranslateOutputArgs *pOutputArgs,
                      TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform &platform,
                      float profilingTimerResolution, const ShaderHash &inputShHash);

OCL_API_CALL void RebuildGlobalAnnotations(IGC::OpenCLProgramContext &oclContext, llvm::Module *pKernelModule);

OCL_API_CALL bool ReadSpecConstantsFromSPIRV(std::istream &IS, std::vector<std::pair<uint32_t, uint32_t>> &OutSCInfo);

OCL_API_CALL void DumpShaderFile(const std::string &dstDir, const char *pBuffer, const UINT bufferSize,
                                 const QWORD hash, const std::string &ext, std::string *fileName);

OCL_API_CALL spv_result_t DisassembleSPIRV(const char *pBuffer, UINT bufferSize, spv_text *outSpirvAsm);

OCL_API_CALL void UnlockMutex();

} // namespace TC

OCL_API_CALL bool enableSrcLine(void *);

namespace IGC {

OCL_API_CALL inline TC::TB_DATA_FORMAT toLegacyFormat(CodeType::CodeType_t format) {
  switch (format) {
  case CodeType::elf:
    return TC::TB_DATA_FORMAT_ELF;
    break;
  case CodeType::llvmBc:
    return TC::TB_DATA_FORMAT_LLVM_BINARY;
    break;
  case CodeType::llvmLl:
    return TC::TB_DATA_FORMAT_LLVM_TEXT;
    break;
  case CodeType::oclC:
    return TC::TB_DATA_FORMAT_OCL_TEXT;
    break;
  case CodeType::oclGenBin:
    return TC::TB_DATA_FORMAT_DEVICE_BINARY;
    break;
  case CodeType::spirV:
    return TC::TB_DATA_FORMAT_SPIR_V;
    break;
  default:
    return TC::TB_DATA_FORMAT_UNKNOWN;
  }
}

struct OCL_API_CALL IGC_State {
protected:
  bool isAlive;

public:
  IGC_State() { isAlive = true; }
  ~IGC_State() { isAlive = false; }
  IGC_State(const IGC_State &) = delete;
  IGC_State &operator=(const IGC_State &) = delete;
  static const bool isDestructed() {
    static const IGC_State obj;
    return !obj.isAlive;
  }
};

CIF_DECLARE_INTERFACE_PIMPL(IgcOclTranslationCtx) : CIF::PimplBase {
  OCL_API_CALL CIF_PIMPL_DECLARE_CONSTRUCTOR(CIF::Version_t version, CIF_PIMPL(IgcOclDeviceCtx) * globalState,
                                             CodeType::CodeType_t inType, CodeType::CodeType_t outType)
      : globalState(CIF::Sanity::ToReferenceOrAbort(globalState)), inType(inType), outType(outType) {}

  OCL_API_CALL static bool SupportsTranslation(CodeType::CodeType_t inType, CodeType::CodeType_t outType) {
    static std::pair<CodeType::CodeType_t, CodeType::CodeType_t> supportedTranslations[] = {
        // clang-format off
                  // from                 // to
                { CodeType::elf,      CodeType::llvmBc },
                { CodeType::elf,      CodeType::oclGenBin },
                { CodeType::llvmLl,   CodeType::oclGenBin },
                { CodeType::llvmBc,   CodeType::oclGenBin },
                { CodeType::spirV,    CodeType::oclGenBin },
        // clang-format on
    };
    for (const auto &st : supportedTranslations) {
      if ((inType == st.first) && (outType == st.second)) {
        return true;
      }
    }
    return false;
  }

  OCL_API_CALL bool GetSpecConstantsInfo(CIF::Builtins::BufferSimple * src,
                                         CIF::Builtins::BufferSimple * outSpecConstantsIds,
                                         CIF::Builtins::BufferSimple * outSpecConstantsSizes) {
    if (IGC_State::isDestructed()) {
      return false;
    }
    bool success = false;
    const char *pInput = src->GetMemory<char>();
    uint32_t inputSize = static_cast<uint32_t>(src->GetSizeRaw());

    if (this->inType == CodeType::spirV) {
      // load registry keys to make sure that flags can be read correctly
      LoadRegistryKeys();
      if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
        // dump SPIRV input if flag is enabled
        const char *pOutputFolder = Debug::GetShaderOutputFolder();
        QWORD hash = Debug::ShaderHashOCL(reinterpret_cast<const UINT *>(pInput), inputSize / 4).getAsmHash();

        TC::DumpShaderFile(pOutputFolder, pInput, inputSize, hash, ".spv", nullptr);
#if defined(IGC_SPIRV_TOOLS_ENABLED)
        if (IGC_IS_FLAG_ENABLED(SpvAsmDumpEnable)) {
          spv_text spirvAsm = nullptr;
          if (TC::DisassembleSPIRV(pInput, inputSize, &spirvAsm) == SPV_SUCCESS) {
            TC::DumpShaderFile(pOutputFolder, spirvAsm->str, spirvAsm->length, hash, ".spvasm", nullptr);
          }
          spvTextDestroy(spirvAsm);
        }
#endif // defined(IGC_SPIRV_TOOLS_ENABLED)
      }
      llvm::StringRef strInput = llvm::StringRef(pInput, inputSize);
      std::istringstream IS(strInput.str());

      // vector of pairs [spec_id, spec_size]
      std::vector<std::pair<uint32_t, uint32_t>> SCInfo;
      success = TC::ReadSpecConstantsFromSPIRV(IS, SCInfo);

      outSpecConstantsIds->Resize(sizeof(uint32_t) * SCInfo.size());
      outSpecConstantsSizes->Resize(sizeof(uint32_t) * SCInfo.size());

      uint32_t *specConstantsIds = outSpecConstantsIds->GetMemoryWriteable<uint32_t>();
      uint32_t *specConstantsSizes = outSpecConstantsSizes->GetMemoryWriteable<uint32_t>();

      for (uint32_t i = 0; i < SCInfo.size(); ++i) {
        specConstantsIds[i] = SCInfo.at(i).first;
        specConstantsSizes[i] = SCInfo.at(i).second;
      }
    } else {
      success = false;
    }

    return success;
  }

  OCL_API_CALL OclTranslationOutputBase *Translate(
      CIF::Version_t outVersion, CIF::Builtins::BufferSimple * src, CIF::Builtins::BufferSimple * specConstantsIds,
      CIF::Builtins::BufferSimple * specConstantsValues, CIF::Builtins::BufferSimple * options,
      CIF::Builtins::BufferSimple * internalOptions, CIF::Builtins::BufferSimple * tracingOptions,
      uint32_t tracingOptionsCount, void *gtPinInput) const {
    // Create interface for return data
    auto outputInterface =
        CIF::RAII::UPtr(CIF::InterfaceCreator<OclTranslationOutput>::CreateInterfaceVer(outVersion, this->outType));
    if (outputInterface == nullptr) {
      return nullptr; // OOM
    }
    if (IGC_State::isDestructed()) {
      outputInterface->GetImpl()->SetError(TranslationErrorType::UnhandledInput, "IGC is destructed");
      return outputInterface.release();
    }

    TC::STB_TranslateInputArgs inputArgs;
    if (src != nullptr) {
      if (gtPinInput) {
        bool srcLine = enableSrcLine(gtPinInput);
        if (srcLine) {
          const char *arg = " -gline-tables-only";
          src->PushBackRawBytes(arg, strlen(arg));
        }
      }
      inputArgs.pInput = src->GetMemoryWriteable<char>();
      inputArgs.InputSize = static_cast<uint32_t>(src->GetSizeRaw());
    }
    if (options != nullptr) {
      inputArgs.pOptions = options->GetMemory<char>();
      inputArgs.OptionsSize = static_cast<uint32_t>(options->GetSizeRaw());
    }
    if (internalOptions != nullptr) {
      inputArgs.pInternalOptions = internalOptions->GetMemory<char>();
      inputArgs.InternalOptionsSize = static_cast<uint32_t>(internalOptions->GetSizeRaw());
    }
    if (tracingOptions != nullptr) {
      inputArgs.pTracingOptions = tracingOptions->GetMemoryRawWriteable();
    }
    inputArgs.TracingOptionsCount = tracingOptionsCount;
    if (specConstantsIds != nullptr && specConstantsValues != nullptr) {
      inputArgs.pSpecConstantsIds = specConstantsIds->GetMemory<uint32_t>();
      inputArgs.SpecConstantsSize = static_cast<uint32_t>(specConstantsIds->GetSizeRaw() / sizeof(uint32_t));
      inputArgs.pSpecConstantsValues = specConstantsValues->GetMemory<uint64_t>();
    }
    inputArgs.GTPinInput = gtPinInput;

    CIF::Sanity::NotNullOrAbort(this->globalState.GetPlatformImpl());
    auto platform = this->globalState.GetPlatformImpl()->p;

    USC::SShaderStageBTLayout zeroLayout = USC::g_cZeroShaderStageBTLayout;
    IGC::COCLBTILayout oclLayout(&zeroLayout);

    std::string RegKeysFlagsFromOptions;
    if (inputArgs.pOptions != nullptr) {
      std::string_view optionsWithFlags = inputArgs.pOptions;

      // check if there are more instances of '-igc_opts'
      while (!optionsWithFlags.empty()) {
        std::size_t found = optionsWithFlags.find("-igc_opts");
        if (found == std::string::npos)
          break;

        std::size_t foundFirstSingleQuote = optionsWithFlags.find('\'', found);
        std::size_t foundSecondSingleQuote = optionsWithFlags.find('\'', foundFirstSingleQuote + 1);
        if (foundFirstSingleQuote == std::string::npos || foundSecondSingleQuote == std::string::npos) {
          outputInterface->GetImpl()->SetError(TranslationErrorType::Unused, "Missing single quotes for -igc_opts");
          return outputInterface.release();
        }
        RegKeysFlagsFromOptions +=
            optionsWithFlags.substr(foundFirstSingleQuote + 1, (foundSecondSingleQuote - foundFirstSingleQuote - 1));
        RegKeysFlagsFromOptions = RegKeysFlagsFromOptions + ',';
        optionsWithFlags = optionsWithFlags.substr(foundSecondSingleQuote + 1, optionsWithFlags.length());
      }
    }
    bool RegFlagNameError = 0;
    LoadRegistryKeys(RegKeysFlagsFromOptions, &RegFlagNameError);
    if (RegFlagNameError)
      outputInterface->GetImpl()->SetError(
          TranslationErrorType::Unused, "Invalid registry flag name in -igc_opts, at least one flag has been ignored");

    IGC::CPlatform igcPlatform = this->globalState.GetIgcCPlatform();

    // extra ocl options set from regkey
    const char *extraOptions = IGC_GET_REGKEYSTRING(ExtraOCLOptions);
    std::string combinedOptions;
    if (extraOptions[0] != '\0') {
      if (inputArgs.pOptions != nullptr) {
        combinedOptions = std::string(inputArgs.pOptions) + ' ';
      }
      combinedOptions += extraOptions;
      inputArgs.pOptions = combinedOptions.c_str();
      inputArgs.OptionsSize = combinedOptions.size();
    }

    // extra ocl internal options set from regkey
    const char *extraInternlOptions = IGC_GET_REGKEYSTRING(ExtraOCLInternalOptions);
    std::string combinedInternalOptions;
    if (extraInternlOptions[0] != '\0') {
      if (inputArgs.pInternalOptions != nullptr) {
        combinedInternalOptions = std::string(inputArgs.pInternalOptions) + ' ';
      }
      combinedInternalOptions += extraInternlOptions;
      inputArgs.pInternalOptions = combinedInternalOptions.c_str();
      inputArgs.InternalOptionsSize = combinedInternalOptions.size();
    }

    TC::STB_TranslateOutputArgs output{};

    bool success = false;
    try {
      if (this->inType == CodeType::elf) {
        // Handle TB_DATA_FORMAT_ELF input as a result of a call to
        // clLinkLibrary(). There are two possible scenarios, link input
        // to form a new library (BC module) or link input to form an
        // executable.

        // First, link input modules together
        CDriverInfo dummyDriverInfo;
        IGC::OpenCLProgramContext oclContextTemp(oclLayout, igcPlatform, &inputArgs, dummyDriverInfo, nullptr, false);
        IGC::Debug::RegisterComputeErrHandlers(*oclContextTemp.getLLVMContext());
        success = TC::ProcessElfInput(inputArgs, output, oclContextTemp, platform, toLegacyFormat(this->outType),
                                      this->globalState.MiscOptions.ProfilingTimerResolution);
      } else {
        if ((this->inType == CodeType::llvmLl) || (this->inType == CodeType::spirV) ||
            (this->inType == CodeType::llvmBc)) {
          TC::TB_DATA_FORMAT inFormatLegacy = toLegacyFormat(this->inType);
          success = TC::TranslateBuild(&inputArgs, &output, inFormatLegacy, igcPlatform,
                                       this->globalState.MiscOptions.ProfilingTimerResolution);
        } else {
          outputInterface->GetImpl()->SetError(TranslationErrorType::UnhandledInput, "Unhandled inType");
          success = false;
        }
      }
    } catch (std::exception &e) {
      success = false;
      if (output.ErrorString.empty()) {
        std::string msg = "IGC: ";
        msg += e.what();
        outputInterface->GetImpl()->SetError(TranslationErrorType::FailedCompilation, msg.c_str());
      }
    } catch (...) {
      success = false;
      if (output.ErrorString.empty()) {
        outputInterface->GetImpl()->SetError(TranslationErrorType::FailedCompilation, "IGC: Internal Compiler Error");
      }
    }

    bool dataCopiedSuccessfuly = true;
    if (success) {
      dataCopiedSuccessfuly &= outputInterface->GetImpl()->AddWarning(output.ErrorString);
      dataCopiedSuccessfuly &=
          outputInterface->GetImpl()->CloneDebugData(output.DebugData.data(), output.DebugData.size());
      dataCopiedSuccessfuly &=
          outputInterface->GetImpl()->SetSuccessfulAndCloneOutput(output.Output.data(), output.Output.size());
    } else {
      dataCopiedSuccessfuly &=
          outputInterface->GetImpl()->SetError(TranslationErrorType::FailedCompilation, output.ErrorString.c_str());
    }

    if (dataCopiedSuccessfuly == false) {
      return nullptr; // OOM
    }

    return outputInterface.release();
  }

  OCL_API_CALL OclTranslationOutputBase *GetErrorOutput(CIF::Version_t outVersion, unsigned int code) const {
    auto outputInterface =
        CIF::RAII::UPtr(CIF::InterfaceCreator<OclTranslationOutput>::CreateInterfaceVer(outVersion, this->outType));
    if (outputInterface == nullptr) {
      return nullptr; // OOM
    }
    const char *outputMessage;
    switch (code) {
#if defined(WIN32)
    case EXCEPTION_ACCESS_VIOLATION:
      outputMessage = "IGC: Internal Compiler Error: Access violation";
      break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
      outputMessage = "IGC: Internal Compiler Error: Datatype misalignement";
      break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      outputMessage = "IGC: Internal Compiler Error: Divide by zero";
      break;
    case EXCEPTION_STACK_OVERFLOW:
      outputMessage = "IGC: Internal Compiler Error: Stack overflow";
      break;
#else
    case SIGABRT:
      outputMessage = "IGC: Internal Compiler Error: Abnormal termination";
      break;
    case SIGFPE:
      outputMessage = "IGC: Internal Compiler Error: Floating point exception";
      break;
    case SIGILL:
      outputMessage = "IGC: Internal Compiler Error: Invalid instruction";
      break;
    case SIGINT:
      outputMessage = "IGC: Internal Compiler Error: Interrupt request sent to the program";
      break;
    case SIGSEGV:
      outputMessage = "IGC: Internal Compiler Error: Segmentation violation";
      break;
    case SIGTERM:
      outputMessage = "IGC: Internal Compiler Error: Termination request sent to the program";
      break;
#endif
    default:
      outputMessage = "IGC: Internal Compiler Error: Signal caught";
    }
    if (outputInterface->GetImpl()->SetError(TranslationErrorType::Internal, outputMessage)) {
      return outputInterface.release();
    }
    return nullptr;
  }

protected:
  CIF_PIMPL(IgcOclDeviceCtx) & globalState;
  CodeType::CodeType_t inType;
  CodeType::CodeType_t outType;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(IgcOclTranslationCtx);

} // namespace IGC

#include "cif/macros/disable.h"

#if defined(NDEBUG)
#if defined(WIN32)
OCL_API_CALL int ex_filter(unsigned int code, struct _EXCEPTION_POINTERS *ep);

#define EX_GUARD_BEGIN __try {

#define EX_GUARD_END                                                                                                   \
  }                                                                                                                    \
  __except (ex_filter(GetExceptionCode(), GetExceptionInformation())) {                                                \
    TC::UnlockMutex();                                                                                                 \
    res = CIF_GET_PIMPL()->GetErrorOutput(outVersion, GetExceptionCode());                                             \
  }

#else
OCL_API_CALL void signalHandler(int sig, siginfo_t *info, void *ucontext);

#include "igc_signal_guard.h"

#define EX_GUARD_BEGIN                                                                                                 \
  do {                                                                                                                 \
    SET_SIG_HANDLER(SIGABRT)                                                                                           \
    SET_SIG_HANDLER(SIGFPE)                                                                                            \
    SET_SIG_HANDLER(SIGILL)                                                                                            \
    SET_SIG_HANDLER(SIGINT)                                                                                            \
    SET_SIG_HANDLER(SIGSEGV)                                                                                           \
    SET_SIG_HANDLER(SIGTERM)                                                                                           \
    int sig = setjmp(sig_jmp_buf);                                                                                     \
    if (sig == 0) {

#define EX_GUARD_END                                                                                                   \
  }                                                                                                                    \
  else {                                                                                                               \
    TC::UnlockMutex();                                                                                                 \
    res = CIF_GET_PIMPL()->GetErrorOutput(outVersion, sig);                                                            \
  }                                                                                                                    \
  REMOVE_SIG_HANDLER(SIGABRT)                                                                                          \
  REMOVE_SIG_HANDLER(SIGFPE)                                                                                           \
  REMOVE_SIG_HANDLER(SIGILL)                                                                                           \
  REMOVE_SIG_HANDLER(SIGINT)                                                                                           \
  REMOVE_SIG_HANDLER(SIGSEGV)                                                                                          \
  REMOVE_SIG_HANDLER(SIGTERM)                                                                                          \
  }                                                                                                                    \
  while (0)                                                                                                            \
    ;

#endif
#else
#define EX_GUARD_BEGIN
#define EX_GUARD_END
#endif
