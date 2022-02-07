/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/igc_ocl_translation_ctx.h"
#include "ocl_igc_interface/impl/igc_ocl_device_ctx_impl.h"

#include <memory>
#include <iomanip>

#include "cif/builtins/memory/buffer/impl/buffer_impl.h"
#include "cif/helpers/error.h"
#include "cif/export/pimpl_base.h"

#include "ocl_igc_interface/impl/ocl_translation_output_impl.h"

#include "AdaptorOCL/OCL/TB/igc_tb.h"
#include "common/debug/Debug.hpp"

#include "cif/macros/enable.h"
#include <spirv-tools/libspirv.h>

namespace TC{

// Taken from dllInterfaceCompute
extern bool ProcessElfInput(
  STB_TranslateInputArgs &InputArgs,
  STB_TranslateOutputArgs &OutputArgs,
  IGC::OpenCLProgramContext &Context,
  PLATFORM &platform,
  const TB_DATA_FORMAT& outType,
  float profilingTimerResolution);

extern bool ParseInput(
  llvm::Module*& pKernelModule,
  const STB_TranslateInputArgs* pInputArgs,
  STB_TranslateOutputArgs* pOutputArgs,
  IGC::OpenCLProgramContext &oclContext,
  TB_DATA_FORMAT inputDataFormatTemp);

bool TranslateBuild(
  const STB_TranslateInputArgs* pInputArgs,
  STB_TranslateOutputArgs* pOutputArgs,
  TB_DATA_FORMAT inputDataFormatTemp,
  const IGC::CPlatform &platform,
  float profilingTimerResolution);

bool TranslateBuildSPMD(
  const STB_TranslateInputArgs* pInputArgs,
  STB_TranslateOutputArgs* pOutputArgs,
  TB_DATA_FORMAT inputDataFormatTemp,
  const IGC::CPlatform &platform,
  float profilingTimerResolution,
  const ShaderHash& inputShHash);

bool TranslateBuildVC(
  const STB_TranslateInputArgs* pInputArgs,
  STB_TranslateOutputArgs* pOutputArgs,
  TB_DATA_FORMAT inputDataFormatTemp,
  const IGC::CPlatform &platform,
  float profilingTimerResolution,
  const ShaderHash& inputShHash);

bool ReadSpecConstantsFromSPIRV(
    std::istream &IS,
    std::vector<std::pair<uint32_t, uint32_t>> &OutSCInfo);

void DumpShaderFile(
    const std::string& dstDir,
    const char* pBuffer,
    const UINT bufferSize,
    const QWORD hash,
    const std::string& ext,
    std::string* fileName);

spv_result_t DisassembleSPIRV(
    const char* pBuffer,
    UINT bufferSize,
    spv_text* outSpirvAsm);
}

bool enableSrcLine(void*);

namespace IGC {

inline TC::TB_DATA_FORMAT toLegacyFormat(CodeType::CodeType_t format){
    switch(format){
        case CodeType::elf : return TC::TB_DATA_FORMAT_ELF; break;
        case CodeType::llvmBc : return TC::TB_DATA_FORMAT_LLVM_BINARY; break;
        case CodeType::llvmLl : return TC::TB_DATA_FORMAT_LLVM_TEXT; break;
        case CodeType::oclC : return TC::TB_DATA_FORMAT_OCL_TEXT; break;
        case CodeType::oclGenBin : return TC::TB_DATA_FORMAT_DEVICE_BINARY; break;
        case CodeType::spirV : return TC::TB_DATA_FORMAT_SPIR_V; break;
        default:
            return TC::TB_DATA_FORMAT_UNKNOWN;
    }
}

struct IGC_State {
protected:
    bool isAlive;
public:
    IGC_State() {
        isAlive = true;
    }
    ~IGC_State() {
        isAlive = false;
    }
    static const bool isDestructed() {
        static const IGC_State obj;
        return !obj.isAlive;
    }
};

CIF_DECLARE_INTERFACE_PIMPL(IgcOclTranslationCtx) : CIF::PimplBase
{
    CIF_PIMPL_DECLARE_CONSTRUCTOR(CIF::Version_t version, CIF_PIMPL(IgcOclDeviceCtx) *globalState,
                                  CodeType::CodeType_t inType, CodeType::CodeType_t outType)
        : globalState(CIF::Sanity::ToReferenceOrAbort(globalState)), inType(inType), outType(outType)
    {
    }

    static bool SupportsTranslation(CodeType::CodeType_t inType, CodeType::CodeType_t outType){
        static std::pair<CodeType::CodeType_t, CodeType::CodeType_t> supportedTranslations[] =
            {
                  // from                 // to
                { CodeType::elf,      CodeType::llvmBc },
                { CodeType::elf,      CodeType::oclGenBin },
                { CodeType::llvmLl,   CodeType::oclGenBin },
                { CodeType::llvmBc,   CodeType::oclGenBin },
                { CodeType::spirV,    CodeType::oclGenBin },
            };
        for(const auto & st : supportedTranslations){
            if((inType == st.first) && (outType == st.second)){
                return true;
            }
        }
        return false;
    }

    bool GetSpecConstantsInfo(CIF::Builtins::BufferSimple *src,
                              CIF::Builtins::BufferSimple *outSpecConstantsIds,
                              CIF::Builtins::BufferSimple *outSpecConstantsSizes)
    {
        if (IGC_State::isDestructed()) {
            return false;
        }
        bool success = false;
        const char* pInput = src->GetMemory<char>();
        uint32_t inputSize = static_cast<uint32_t>(src->GetSizeRaw());

        if(this->inType == CodeType::spirV){
            // load registry keys to make sure that flags can be read correctly
            LoadRegistryKeys();
            if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
            {
                // dump SPIRV input if flag is enabled
                const char* pOutputFolder = Debug::GetShaderOutputFolder();
                QWORD hash = Debug::ShaderHashOCL(reinterpret_cast<const UINT*>(pInput), inputSize / 4).getAsmHash();

                TC::DumpShaderFile(pOutputFolder, pInput, inputSize, hash, ".spv", nullptr);
#if defined(IGC_SPIRV_TOOLS_ENABLED)
                spv_text spirvAsm = nullptr;
                if (TC::DisassembleSPIRV(pInput, inputSize, &spirvAsm) == SPV_SUCCESS)
                {
                    TC::DumpShaderFile(pOutputFolder, spirvAsm->str, spirvAsm->length, hash, ".spvasm", nullptr);
                }
                spvTextDestroy(spirvAsm);
#endif // defined(IGC_SPIRV_TOOLS_ENABLED)
            }
            llvm::StringRef strInput = llvm::StringRef(pInput, inputSize);
            std::istringstream IS(strInput.str());

            // vector of pairs [spec_id, spec_size]
            std::vector<std::pair<uint32_t, uint32_t>> SCInfo;
            success = TC::ReadSpecConstantsFromSPIRV(IS, SCInfo);

            outSpecConstantsIds->Resize(sizeof(uint32_t) * SCInfo.size());
            outSpecConstantsSizes->Resize(sizeof(uint32_t) * SCInfo.size());

            uint32_t* specConstantsIds = outSpecConstantsIds->GetMemoryWriteable<uint32_t>();
            uint32_t* specConstantsSizes = outSpecConstantsSizes->GetMemoryWriteable<uint32_t>();

            for(uint32_t i = 0; i < SCInfo.size(); ++i){
                specConstantsIds[i] = SCInfo.at(i).first;
                specConstantsSizes[i] = SCInfo.at(i).second;
            }
        }
        else{
            success = false;
        }

        return success;
    }

    OclTranslationOutputBase *Translate(CIF::Version_t outVersion,
                                        CIF::Builtins::BufferSimple *src,
                                        CIF::Builtins::BufferSimple *specConstantsIds,
                                        CIF::Builtins::BufferSimple *specConstantsValues,
                                        CIF::Builtins::BufferSimple *options,
                                        CIF::Builtins::BufferSimple *internalOptions,
                                        CIF::Builtins::BufferSimple *tracingOptions,
                                        uint32_t tracingOptionsCount,
                                        void *gtPinInput) const{
        // Create interface for return data
        auto outputInterface = CIF::RAII::UPtr(CIF::InterfaceCreator<OclTranslationOutput>::CreateInterfaceVer(outVersion, this->outType));
        if(outputInterface == nullptr){
            return nullptr; // OOM
        }
        if (IGC_State::isDestructed()) {
            outputInterface->GetImpl()->SetError(TranslationErrorType::UnhandledInput, "IGC is destructed");
            return outputInterface.release();
        }

        TC::STB_TranslateInputArgs inputArgs;
        if(src != nullptr){
            if (gtPinInput)
            {
                bool srcLine = enableSrcLine(gtPinInput);
                if (srcLine)
                {
                    const char* arg = " -gline-tables-only";
                    src->PushBackRawBytes(arg, strlen(arg));
                }
            }
            inputArgs.pInput = src->GetMemoryWriteable<char>();
            inputArgs.InputSize = static_cast<uint32_t>(src->GetSizeRaw());
        }
        if(options != nullptr){
            inputArgs.pOptions = options->GetMemory<char>();
            inputArgs.OptionsSize = static_cast<uint32_t>(options->GetSizeRaw());
        }
        if(internalOptions != nullptr){
            inputArgs.pInternalOptions =  internalOptions->GetMemory<char>();
            inputArgs.InternalOptionsSize = static_cast<uint32_t>(internalOptions->GetSizeRaw());
        }
        if(tracingOptions != nullptr){
            inputArgs.pTracingOptions = tracingOptions->GetMemoryRawWriteable();
        }
        inputArgs.TracingOptionsCount = tracingOptionsCount;
        if(specConstantsIds != nullptr && specConstantsValues != nullptr){
            inputArgs.pSpecConstantsIds = specConstantsIds->GetMemory<uint32_t>();
            inputArgs.SpecConstantsSize = static_cast<uint32_t>(specConstantsIds->GetSizeRaw() / sizeof(uint32_t));
            inputArgs.pSpecConstantsValues = specConstantsValues->GetMemory<uint64_t>();
        }
        inputArgs.GTPinInput = gtPinInput;

        CIF::Sanity::NotNullOrAbort(this->globalState.GetPlatformImpl());
        auto platform = this->globalState.GetPlatformImpl()->p;

        USC::SShaderStageBTLayout zeroLayout = USC::g_cZeroShaderStageBTLayout;
        IGC::COCLBTILayout oclLayout(&zeroLayout);

        TC::STB_TranslateOutputArgs output;
        CIF::SafeZeroOut(output);

        std::string RegKeysFlagsFromOptions;
        if (inputArgs.pOptions != nullptr)
        {
            const std::string optionsWithFlags = inputArgs.pOptions;
            std::size_t found = optionsWithFlags.find("-igc_opts");
            if (found != std::string::npos)
            {
                std::size_t foundFirstSingleQuote = optionsWithFlags.find('\'', found);
                std::size_t foundSecondSingleQuote = optionsWithFlags.find('\'', foundFirstSingleQuote + 1);
                if (foundFirstSingleQuote != std::string::npos && foundSecondSingleQuote)
                {
                    RegKeysFlagsFromOptions = optionsWithFlags.substr(foundFirstSingleQuote + 1, (foundSecondSingleQuote - foundFirstSingleQuote - 1));
                    RegKeysFlagsFromOptions = RegKeysFlagsFromOptions + ',';
                }
            }
        }
        bool RegFlagNameError = 0;
        LoadRegistryKeys(RegKeysFlagsFromOptions, &RegFlagNameError);
        if(RegFlagNameError) outputInterface->GetImpl()->SetError(TranslationErrorType::Unused, "Invalid registry flag name in -igc_opts, at least one flag has been ignored");

        IGC::CPlatform igcPlatform = this->globalState.GetIgcCPlatform();

        // extra ocl options set from regkey
        const char *extraOptions = IGC_GET_REGKEYSTRING(ExtraOCLOptions);
        std::string combinedOptions;
        if (extraOptions[0] != '\0')
        {
            if (inputArgs.pOptions != nullptr)
            {
                combinedOptions = std::string(inputArgs.pOptions) + ' ';
            }
            combinedOptions += extraOptions;
            inputArgs.pOptions = combinedOptions.c_str();
            inputArgs.OptionsSize = combinedOptions.size();
        }

        // extra ocl internal options set from regkey
        const char *extraInternlOptions = IGC_GET_REGKEYSTRING(ExtraOCLInternalOptions);
        std::string combinedInternalOptions;
        if (extraInternlOptions[0] != '\0')
        {
            if (inputArgs.pInternalOptions != nullptr)
            {
                combinedInternalOptions = std::string(inputArgs.pInternalOptions) + ' ';
            }
            combinedInternalOptions += extraInternlOptions;
            inputArgs.pInternalOptions = combinedInternalOptions.c_str();
            inputArgs.InternalOptionsSize = combinedInternalOptions.size();
        }

        bool success = false;
        if (this->inType == CodeType::elf)
        {
            // Handle TB_DATA_FORMAT_ELF input as a result of a call to
            // clLinkLibrary(). There are two possible scenarios, link input
            // to form a new library (BC module) or link input to form an
            // executable.

            // First, link input modules together
            CDriverInfo dummyDriverInfo;
            IGC::OpenCLProgramContext oclContextTemp(oclLayout, igcPlatform, &inputArgs, dummyDriverInfo, nullptr, false);
            IGC::Debug::RegisterComputeErrHandlers(*oclContextTemp.getLLVMContext());
            success = TC::ProcessElfInput(
                inputArgs, output, oclContextTemp, platform,
                toLegacyFormat(this->outType),
                this->globalState.MiscOptions.ProfilingTimerResolution);
        }else
        {
            if ((this->inType == CodeType::llvmLl) ||
                (this->inType == CodeType::spirV) ||
                (this->inType == CodeType::llvmBc))
            {
                TC::TB_DATA_FORMAT inFormatLegacy = toLegacyFormat(this->inType);
                success = TC::TranslateBuild(
                    &inputArgs,
                    &output,
                    inFormatLegacy,
                    igcPlatform,
                    this->globalState.MiscOptions.ProfilingTimerResolution);
            }
            else
            {
                outputInterface->GetImpl()->SetError(TranslationErrorType::UnhandledInput, "Unhandled inType");
                success = false;
            }
        }

        auto outputData = std::unique_ptr<char[]>(output.pOutput);
        auto errorString = std::unique_ptr<char[]>(output.pErrorString);
        auto debugData = std::unique_ptr<char[]>(output.pDebugData);

        bool dataCopiedSuccessfuly = true;
        if(success){
            dataCopiedSuccessfuly &= outputInterface->GetImpl()->AddWarning(output.pErrorString, output.ErrorStringSize);
            dataCopiedSuccessfuly &= outputInterface->GetImpl()->CloneDebugData(output.pDebugData, output.DebugDataSize);
            dataCopiedSuccessfuly &= outputInterface->GetImpl()->SetSuccessfulAndCloneOutput(output.pOutput, output.OutputSize);
        }else{
            dataCopiedSuccessfuly &= outputInterface->GetImpl()->SetError(TranslationErrorType::FailedCompilation, output.pErrorString);
        }

        if(dataCopiedSuccessfuly == false){
            return nullptr; // OOM
        }

        return outputInterface.release();
    }

protected:
    CIF_PIMPL(IgcOclDeviceCtx) &globalState;
    CodeType::CodeType_t inType;
    CodeType::CodeType_t outType;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(IgcOclTranslationCtx);

}

#include "cif/macros/disable.h"
