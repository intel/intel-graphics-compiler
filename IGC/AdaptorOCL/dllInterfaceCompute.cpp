/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGC/common/StringMacros.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/ScaledNumber.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Process.h"
#include "common/LLVMWarningsPop.hpp"

#include <cstring>
#include <string>
#include <stdexcept>
#include <fstream>
#include <mutex>

#include "AdaptorCommon/customApi.hpp"
#include "AdaptorOCL/OCL/LoadBuffer.h"
#include "AdaptorOCL/OCL/BuiltinResource.h"
#include "AdaptorOCL/OCL/TB/igc_tb.h"

#include "AdaptorOCL/UnifyIROCL.hpp"
#include "AdaptorOCL/DriverInfoOCL.hpp"

#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "common/debug/Dump.hpp"
#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "common/secure_mem.h"
#include "common/shaderOverride.hpp"
#include "common/ModuleSplitter.h"

#include "CLElfLib/ElfReader.h"

#if defined(IGC_VC_ENABLED)
#include "common/LLVMWarningsPush.hpp"
#include "vc/igcdeps/TranslationInterface.h"
#include "vc/Support/StatusCode.h"
#include "common/LLVMWarningsPop.hpp"
#endif // defined(IGC_VC_ENABLED)

#include <iStdLib/MemCopy.h>

#if defined(IGC_SPIRV_ENABLED)
#include "common/LLVMWarningsPush.hpp"
#include "AdaptorOCL/SPIRV/SPIRVconsum.h"
#include "common/LLVMWarningsPop.hpp"
#include "AdaptorOCL/SPIRV/libSPIRV/SPIRVModule.h"
#include "AdaptorOCL/SPIRV/libSPIRV/SPIRVValue.h"
#if defined(IGC_SCALAR_USE_KHRONOS_SPIRV_TRANSLATOR)
#include "LLVMSPIRVLib.h"
#endif
#endif

#ifdef IGC_SPIRV_TOOLS_ENABLED
#include "spirv-tools/libspirv.h"
#endif

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Bitcode/BitcodeWriter.h"
#include "common/LLVMWarningsPop.hpp"

#include <sstream>
#include <iomanip>
#include "Probe/Assertion.h"
#include "common/StringMacros.hpp"
#include "VISALinkerDriver/VLD.hpp"

// In case of use GT_SYSTEM_INFO in GlobalData.h from inc/umKmInc/sharedata.h
// We have to do this temporary defines

#ifdef BOOLEAN
#define BOOLEAN_IGC_REPLACED
#pragma push_macro("BOOLEAN")
#undef BOOLEAN
#endif
#define BOOLEAN uint8_t

#ifdef HANDLE
#define HANDLE_IGC_REPLACED
#pragma push_macro("HANDLE")
#undef HANDLE
#endif
#define HANDLE void*

#ifdef VER_H
#define VER_H_IGC_REPLACED
#pragma push_macro("VER_H")
#undef VER_H
#endif
#define VER_H

#include "GlobalData.h"

// We undef BOOLEAN HANDLE and VER_H here
#undef VER_H
#ifdef VER_H_IGC_REPLACED
#pragma pop_macro("VER_H")
#undef VER_H_IGC_REPLACED
#endif

#undef BOOLEAN
#ifdef BOOLEAN_IGC_REPLACED
#pragma pop_macro("BOOLEAN")
#undef BOOLEAN_IGC_REPLACED
#endif

#undef HANDLE
#ifdef HANDLE_IGC_REPLACED
#pragma pop_macro("HANDLE")
#undef HANDLE_IGC_REPLACED
#endif

#if !defined(_WIN32)
#   define strtok_s strtok_r
#   define _strdup strdup
#   define _snprintf snprintf
#endif

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include "common/LLVMWarningsPop.hpp"

#include "IGC/Metrics/IGCMetric.h"

using namespace IGC::IGCMD;
using namespace IGC::Debug;
using namespace IGC;

namespace TC
{

static std::mutex llvm_mutex;

extern bool ProcessElfInput(
    STB_TranslateInputArgs& InputArgs,
    STB_TranslateOutputArgs& OutputArgs,
    IGC::OpenCLProgramContext& Context,
    PLATFORM& platform,
    const TB_DATA_FORMAT& outType,
    float profilingTimerResolution);

extern bool ParseInput(
    llvm::Module*& pKernelModule,
    const STB_TranslateInputArgs* pInputArgs,
    STB_TranslateOutputArgs* pOutputArgs,
    IGC::OpenCLProgramContext& oclContext,
    TB_DATA_FORMAT inputDataFormatTemp);

bool TranslateBuild(
    const STB_TranslateInputArgs* pInputArgs,
    STB_TranslateOutputArgs* pOutputArgs,
    TB_DATA_FORMAT inputDataFormatTemp,
    const IGC::CPlatform& IGCPlatform,
    float profilingTimerResolution);

bool CIGCTranslationBlock::ProcessElfInput(
    STB_TranslateInputArgs& InputArgs,
    STB_TranslateOutputArgs& OutputArgs,
    IGC::OpenCLProgramContext& Context,
    float ProfilingTimerResolution)
{
    return TC::ProcessElfInput(InputArgs, OutputArgs, Context, m_Platform, m_DataFormatOutput, ProfilingTimerResolution);
}

static void SetOutputMessage(
    const std::string& OutputMessage,
    STB_TranslateOutputArgs& pOutputArgs)
{
    pOutputArgs.ErrorStringSize = OutputMessage.size() + 1;
    pOutputArgs.pErrorString = new char[pOutputArgs.ErrorStringSize];
    memcpy_s(pOutputArgs.pErrorString, pOutputArgs.ErrorStringSize, OutputMessage.c_str(), pOutputArgs.ErrorStringSize);
}

static void SetWarningMessage(
    const std::string& OutputMessage,
    STB_TranslateOutputArgs& pOutputArgs)
{
    SetOutputMessage("warning: " + OutputMessage, pOutputArgs);
}

static void SetErrorMessage(
    const std::string& OutputMessage,
    STB_TranslateOutputArgs& pOutputArgs)
{
    SetOutputMessage("error: " + OutputMessage, pOutputArgs);
}

static bool IsDeviceBinaryFormat(const TB_DATA_FORMAT& format)
{
    return (format == TB_DATA_FORMAT_DEVICE_BINARY)
        || (format == TB_DATA_FORMAT_COHERENT_DEVICE_BINARY)
        || (format == TB_DATA_FORMAT_NON_COHERENT_DEVICE_BINARY);
}

bool CIGCTranslationBlock::Create(
    const STB_CreateArgs* pCreateArgs,
    CIGCTranslationBlock*& pTranslationBlock)
{
    pTranslationBlock = new CIGCTranslationBlock();
    if (!pTranslationBlock)
    {
        return false;
    }

    bool success = pTranslationBlock->Initialize(pCreateArgs);
    if (!success)
    {
        CIGCTranslationBlock::Delete(pTranslationBlock);
    }
    return success;
}

void CIGCTranslationBlock::Delete(
    CIGCTranslationBlock* pTranslationBlock)
{
    delete pTranslationBlock;
}

bool CIGCTranslationBlock::Translate(
    const STB_TranslateInputArgs* pInputArgs,
    STB_TranslateOutputArgs* pOutputArgs)
{
    LoadRegistryKeys();

    // Create a copy of input arguments that can be modified
    STB_TranslateInputArgs InputArgsCopy = *pInputArgs;

    IGC::CPlatform IGCPlatform(m_Platform);

    IGC::SetGTSystemInfo(&m_SysInfo, &IGCPlatform);
    IGC::SetWorkaroundTable(&m_SkuTable, &IGCPlatform);
    IGC::SetCompilerCaps(&m_SkuTable, &IGCPlatform);

    pOutputArgs->pOutput = nullptr;
    pOutputArgs->OutputSize = 0;
    pOutputArgs->pErrorString = nullptr;
    pOutputArgs->ErrorStringSize = 0;
    pOutputArgs->pDebugData = nullptr;
    pOutputArgs->DebugDataSize = 0;

    try
    {
        if (m_DataFormatInput == TB_DATA_FORMAT_ELF)
        {
            // Handle TB_DATA_FORMAT_ELF input as a result of a call to
            // clLinkLibrary(). There are two possible scenarios, link input
            // to form a new library (BC module) or link input to form an
            // executable.

            // First, link input modules together
            USC::SShaderStageBTLayout zeroLayout = USC::g_cZeroShaderStageBTLayout;
            IGC::COCLBTILayout oclLayout(&zeroLayout);
            CDriverInfoOCLNEO driverInfo;
            IGC::OpenCLProgramContext oclContextTemp(oclLayout, IGCPlatform, &InputArgsCopy, driverInfo, nullptr,
                                                     m_DataFormatOutput == TC::TB_DATA_FORMAT_NON_COHERENT_DEVICE_BINARY);
            RegisterComputeErrHandlers(*oclContextTemp.getLLVMContext());
            bool success = ProcessElfInput(InputArgsCopy, *pOutputArgs, oclContextTemp, m_ProfilingTimerResolution);

            return success;
        }

        if ((m_DataFormatInput == TB_DATA_FORMAT_LLVM_TEXT) ||
            (m_DataFormatInput == TB_DATA_FORMAT_SPIR_V) ||
            (m_DataFormatInput == TB_DATA_FORMAT_LLVM_BINARY))
        {
            return TC::TranslateBuild(&InputArgsCopy, pOutputArgs, m_DataFormatInput, IGCPlatform, m_ProfilingTimerResolution);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unsupported input format");
            return false;
        }
    }
    catch (std::exception& e)
    {
        if (pOutputArgs->ErrorStringSize == 0 && pOutputArgs->pErrorString == nullptr)
        {
            SetErrorMessage(std::string("IGC: ") + e.what(), *pOutputArgs);
        }
        return false;
    }
    catch (...)
    {
        if (pOutputArgs->ErrorStringSize == 0 && pOutputArgs->pErrorString == nullptr)
        {
            SetErrorMessage("IGC: Internal Compiler Error", *pOutputArgs);
        }
        return false;
    }

    return false;
}

std::unordered_map<uint32_t, uint64_t> UnpackSpecConstants(
    const uint32_t* pSpecConstantsIds,
    const uint64_t* pSpecConstantsValues,
    uint32_t size)
{
    std::unordered_map<uint32_t, uint64_t> outSpecConstantsMap;
    for (uint32_t i = 0; i < size; i++)
    {
        outSpecConstantsMap[pSpecConstantsIds[i]] = pSpecConstantsValues[i];
    }
    return outSpecConstantsMap;
}

void GenerateCompilerOptionsMD(
    llvm::LLVMContext& C,
    llvm::Module& M,
    llvm::StringRef options)
{
    llvm::SmallVector<llvm::StringRef, 8> flags;
    llvm::StringRef sep(" ");
    options.split(flags, sep, -1, false);

    std::vector<llvm::Metadata*> ValueVec;
    for (auto flag : flags)
    {
        flag = flag.trim();
        flag = flag.rtrim(0);  // make sure no ending 0
        // flags : C string (ended with 0)
        if (!flag.empty() && flag.front() != 0)
            ValueVec.push_back(llvm::MDString::get(C, flag));
    }
    llvm::NamedMDNode* NamedMD = M.getOrInsertNamedMetadata("opencl.compiler.options");
    NamedMD->addOperand(llvm::MDNode::get(C, ValueVec));
}

// Dump shader (binary or text), to output directory.
// Create directory if it doesn't exist.
// Works for all OSes.
// ext - file name suffix (optional) and extension.
void DumpShaderFile(
    const std::string& dstDir,
    const char* pBuffer,
    const UINT bufferSize,
    const QWORD hash,
    const std::string& ext,
    std::string* fileName = nullptr)
{
    if (!pBuffer || bufferSize == 0)
    {
        return;
    }

    std::ostringstream fullPath(dstDir, std::ostringstream::ate);
    fullPath << "OCL_asm"
        << std::hex
        << std::setfill('0')
        << std::setw(sizeof(hash) * CHAR_BIT / 4)
        << hash
        << std::dec
        << std::setfill(' ')
        << ext;

    FILE* pFile = NULL;
    fopen_s(&pFile, fullPath.str().c_str(), "wb");
    if (pFile)
    {
        fwrite(pBuffer, 1, bufferSize, pFile);
        fclose(pFile);
    }

    if (fileName != nullptr)
    {
        *fileName = fullPath.str();
    }
}

std::string getBaseFilename(const std::string& FName)
{
#if defined(_WIN32)
    const char Sep = '\\';  // Windows file separator
#else
    const char Sep = '/';   // Linux file separator
#endif
    size_t i = FName.rfind(Sep);
    return (i == std::string::npos ? FName : FName.substr(i + 1));
}

#if defined(IGC_SPIRV_TOOLS_ENABLED)
spv_result_t DisassembleSPIRV(
    const char* pBuffer,
    UINT bufferSize,
    spv_text* outSpirvAsm)
{
    const spv_target_env target_env = SPV_ENV_UNIVERSAL_1_3;
    spv_context context = spvContextCreate(target_env);
    const uint32_t* const binary = reinterpret_cast<const uint32_t*>(pBuffer);
    const size_t word_count = bufferSize / sizeof(uint32_t);
    const uint32_t options = (SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES | SPV_BINARY_TO_TEXT_OPTION_INDENT | SPV_BINARY_TO_TEXT_OPTION_SHOW_BYTE_OFFSET);
    spv_diagnostic diagnostic = nullptr;

    const spv_result_t result = spvBinaryToText(
        context,
        binary,
        word_count,
        options,
        outSpirvAsm,
        &diagnostic);

    spvContextDestroy(context);
    spvDiagnosticDestroy(diagnostic);
    return result;
}
#endif // defined(IGC_SPIRV_TOOLS_ENABLED)

#if defined(IGC_SPIRV_ENABLED)
// Translate SPIR-V binary to LLVM Module
bool TranslateSPIRVToLLVM(
    const STB_TranslateInputArgs& InputArgs,
    llvm::LLVMContext& Context,
    llvm::StringRef SPIRVBinary,
    llvm::Module*& LLVMModule,
    std::string& stringErrMsg)
{
    bool success = true;
    std::istringstream IS(SPIRVBinary.str());
    std::unordered_map<uint32_t, uint64_t> specIDToSpecValueMap = UnpackSpecConstants(
        InputArgs.pSpecConstantsIds,
        InputArgs.pSpecConstantsValues,
        InputArgs.SpecConstantsSize);

#if defined(IGC_SCALAR_USE_KHRONOS_SPIRV_TRANSLATOR)
    // Set SPIRV-LLVM-Translator translation options
    SPIRV::TranslatorOpts Opts;
    Opts.enableGenArgNameMD();
    Opts.enableAllExtensions();
    Opts.setDesiredBIsRepresentation(SPIRV::BIsRepresentation::SPIRVFriendlyIR);

    // This option has to be enabled since SPIRV-Translator for LLVM13 because of:
    // https://github.com/KhronosGroup/SPIRV-LLVM-Translator/commit/835eb7e. This change
    // has been also backported to SPIRV-Translator for LLVM11.
#if LLVM_VERSION_MAJOR >= 13 || LLVM_VERSION_MAJOR == 11
    Opts.setPreserveOCLKernelArgTypeMetadataThroughString(true);
#endif

    // Unpack specialization constants passed from OCL Runtime (Acquired from
    // clSetProgramSpecializationConstant API call). It is also passed as a
    // translation options.
    if (InputArgs.SpecConstantsSize)
    {
        for (const auto& SC : specIDToSpecValueMap)
            Opts.setSpecConst(SC.first, SC.second);
    }

    // Actual translation from SPIR-V to LLLVM
    success = llvm::readSpirv(Context, Opts, IS, LLVMModule, stringErrMsg);
#else // IGC Legacy SPIRV Translator
    success = igc_spv::ReadSPIRV(Context, IS, LLVMModule, stringErrMsg, &specIDToSpecValueMap);
#endif

    // Handle OpenCL Compiler Options
    if (success)
    {
        GenerateCompilerOptionsMD(
            Context,
            *LLVMModule,
            llvm::StringRef(InputArgs.pOptions, InputArgs.OptionsSize));
    }

    return success;
}
#endif // defined(IGC_SPIRV_ENABLED)

bool ProcessElfInput(
    STB_TranslateInputArgs& InputArgs,
    STB_TranslateOutputArgs& OutputArgs,
    IGC::OpenCLProgramContext& Context,
    PLATFORM& platform,
    const TB_DATA_FORMAT& outType,
    float profilingTimerResolution)
{
    ShaderHash previousHash;
    bool success = true;
    std::string ErrorMsg;

    CLElfLib::CElfReader *pElfReader = CLElfLib::CElfReader::Create(InputArgs.pInput, InputArgs.InputSize);
    CLElfLib::RAIIElf X(pElfReader); // When going out of scope this object calls the Delete() function automatically

    // If input buffer is an ELF file, then process separately
    const CLElfLib::SElf64Header* pHeader = pElfReader->GetElfHeader();
    if (pHeader != NULL)
    {
        // Create an empty module to store the output
        std::unique_ptr<llvm::Module> OutputModule;

#if defined(IGC_SPIRV_ENABLED)
        if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
        {
            // Dumping SPIRV files with temporary hashes
            for (unsigned i = 1; i < pHeader->NumSectionHeaderEntries; i++)
            {
                const CLElfLib::SElf64SectionHeader* pSectionHeader = pElfReader->GetSectionHeader(i);
                IGC_ASSERT(pSectionHeader != NULL);
                if (pSectionHeader->Type != CLElfLib::SH_TYPE_SPIRV)
                {
                    continue;
                }

                char* pSPIRVBitcode = NULL;
                size_t size = 0;
                pElfReader->GetSectionData(i, pSPIRVBitcode, size);
                // The hash created here (from Input) is only temporary and will be replaced
                // if the LLVM translation and linking finishes successfully
                previousHash = ShaderHashOCL(reinterpret_cast<const UINT*>(InputArgs.pInput), InputArgs.InputSize / 4);
                QWORD hash = previousHash.getAsmHash();

                // beyond of general hash, each SPIR-V module needs to have it's own hash
                QWORD spvHash = ShaderHashOCL((const UINT*)pSPIRVBitcode, size / 4).getAsmHash();
                std::ostringstream spvHashSuffix("_", std::ostringstream::ate);
                spvHashSuffix << std::hex << std::setfill('0') << std::setw(sizeof(spvHash) * CHAR_BIT / 4) << spvHash;
                const std::string suffix = spvHashSuffix.str();
                const char* pOutputFolder = IGC::Debug::GetShaderOutputFolder();

                // Remove any already exising SPIR-V dumps from GetSpecConstantsInfo
                // and dump new ones with correct names
                std::string spvHashString = suffix.c_str();
                spvHashString.erase(0, 1);
                std::string prevSpvPath = pOutputFolder;
                prevSpvPath += "OCL_asm" + spvHashString + ".spv";
                llvm::sys::fs::remove(prevSpvPath);
                DumpShaderFile(pOutputFolder, pSPIRVBitcode, size, hash, suffix + ".spv");

#if defined(IGC_SPIRV_TOOLS_ENABLED)
                spv_text spirvAsm = nullptr;
                // Similarly replace any spvasm dump from GetSpecConstantsInfo
                std::string prevSpvAsmPath = pOutputFolder;
                prevSpvAsmPath += "OCL_asm" + spvHashString + ".spvasm";
                llvm::sys::fs::remove(prevSpvAsmPath);
                if (DisassembleSPIRV(pSPIRVBitcode, size, &spirvAsm) == SPV_SUCCESS)
                {
                    DumpShaderFile(pOutputFolder, spirvAsm->str, spirvAsm->length, hash, suffix + ".spvasm");
                }
                spvTextDestroy(spirvAsm);
#endif // defined(IGC_SPIRV_TOOLS_ENABLED)
            }
        }
#endif // defined(IGC_SPIRV_ENABLED)

        // Iterate over all the input modules.
        for (unsigned i = 1; i < pHeader->NumSectionHeaderEntries; i++)
        {
            const CLElfLib::SElf64SectionHeader* pSectionHeader = pElfReader->GetSectionHeader(i);
            IGC_ASSERT(pSectionHeader != NULL);

            char* pData = NULL;
            size_t dataSize = 0;

            if (pSectionHeader->Type == CLElfLib::SH_TYPE_SPIRV_SC_IDS)
            {
                pElfReader->GetSectionData(i, pData, dataSize);
                InputArgs.pSpecConstantsIds = reinterpret_cast<const uint32_t*>(pData);
                InputArgs.SpecConstantsSize = static_cast<uint32_t>(dataSize / sizeof(uint32_t));
            }

            if (pSectionHeader->Type == CLElfLib::SH_TYPE_SPIRV_SC_VALUES)
            {
                pElfReader->GetSectionData(i, pData, dataSize);
                InputArgs.pSpecConstantsValues = reinterpret_cast<const uint64_t*>(pData);
            }

            if ((pSectionHeader->Type == CLElfLib::SH_TYPE_OPENCL_LLVM_BINARY)  ||
                (pSectionHeader->Type == CLElfLib::SH_TYPE_OPENCL_LLVM_ARCHIVE) ||
                (pSectionHeader->Type == CLElfLib::SH_TYPE_SPIRV))
            {
                pElfReader->GetSectionData(i, pData, dataSize);

                // Create input module from the buffer
                llvm::StringRef buf(pData, dataSize);

                std::unique_ptr<llvm::Module> InputModule = nullptr;

                if (pSectionHeader->Type == CLElfLib::SH_TYPE_SPIRV)
                {
                    llvm::Module* pKernelModule = nullptr;
#if defined(IGC_SPIRV_ENABLED)
                    Context.setAsSPIRV();
                    std::string stringErrMsg;
                    bool success = TranslateSPIRVToLLVM(InputArgs, *Context.getLLVMContext(), buf, pKernelModule, stringErrMsg);
#else
                    std::string stringErrMsg{ "SPIRV consumption not enabled for the TARGET." };
                    bool success = false;
#endif
                    // unset specialization constants, to avoid using them by subsequent SPIR-V modules
                    InputArgs.pSpecConstantsIds = nullptr;
                    InputArgs.pSpecConstantsValues = nullptr;
                    InputArgs.SpecConstantsSize = 0;

                    if (success)
                    {
                        InputModule.reset(pKernelModule);
                    }
                }
                else
                {
                    std::unique_ptr<llvm::MemoryBuffer> pInputBuffer =
                    llvm::MemoryBuffer::getMemBuffer(buf, "", false);

                    llvm::Expected<std::unique_ptr<llvm::Module>> errorOrModule =
                    llvm::parseBitcodeFile(pInputBuffer->getMemBufferRef(), *Context.getLLVMContext());
                    if (llvm::Error EC = errorOrModule.takeError())
                    {
                        std::string errMsg;
                        llvm::handleAllErrors(std::move(EC), [&](llvm::ErrorInfoBase &EIB)
                        {
                            llvm::SMDiagnostic(pInputBuffer->getBufferIdentifier(), llvm::SourceMgr::DK_Error,
                                               EIB.message());
                        });
                        IGC_ASSERT_MESSAGE(errMsg.empty(), "parsing bitcode failed");
                    }

                    InputModule = std::move(errorOrModule.get());
                }

                if (InputModule.get() == NULL)
                {
                    success = false;
                    break;
                }

                // Link modules
                if (OutputModule.get() == NULL)
                {
                    InputModule.swap(OutputModule);
                }
                else
                {
                    success = !llvm::Linker::linkModules(*OutputModule, std::move(InputModule));
                }

                if (!success)
                {
                    break;
                }
            }
        }

        if (success == true)
        {
            // Now that the output modules are linked the resulting module needs to be
            // serialized out
            std::string OutputString;
            llvm::raw_string_ostream OStream(OutputString);
            IGCLLVM::WriteBitcodeToFile(OutputModule.get(), OStream);
            OStream.flush();

            // Create a copy of the string to return to the caller. The output type
            // determines how the buffer gets managed
            char* pBufResult = static_cast<char*>(operator new(OutputString.size(), std::nothrow));
            if (pBufResult != NULL)
            {
                if (outType == TB_DATA_FORMAT_LLVM_BINARY)
                {
                    memcpy_s(pBufResult, OutputString.size(), OutputString.c_str(), OutputString.size());

                    // The buffer is returned to the runtime. When the buffer is not
                    // needed anymore the runtime ir responsible to call the module for
                    // destroying it
                    OutputArgs.OutputSize = OutputString.size();
                    OutputArgs.pOutput = pBufResult;

#if defined(IGC_SPIRV_ENABLED)
                    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
                    {
                        // This part renames the previously dumped SPIR-V files
                        // so that the hash in their name matches the one of LLVM files
                        const char* outputDir = IGC::Debug::GetShaderOutputFolder();

                        QWORD prevAsmHash = previousHash.getAsmHash();
                        std::ostringstream oss1(std::ostringstream::ate);
                        oss1 << std::hex
                             << std::setfill('0')
                             << std::setw(sizeof(prevAsmHash) * CHAR_BIT / 4)
                             << prevAsmHash;
                        const std::string prevHashString = oss1.str();

                        QWORD newAsmHash = ShaderHashOCL((const UINT*)OutputArgs.pOutput, OutputArgs.OutputSize / 4).getAsmHash();
                        std::ostringstream oss2(std::ostringstream::ate);
                        oss2 << std::hex
                             << std::setfill('0')
                             << std::setw(sizeof(newAsmHash) * CHAR_BIT / 4)
                             << newAsmHash;
                        const std::string newHashString = oss2.str();

                        llvm::Twine outputPath = outputDir;
                        std::error_code ec;
                        for (llvm::sys::fs::directory_iterator file(outputDir, ec), fileEnd; file != fileEnd && !ec; file.increment(ec))
                        {
                            if (llvm::sys::fs::is_regular_file(file->path()))
                            {
                                std::string name = file->path();
                                // Rename file if it contains the previous hash
                                if (name.find(prevHashString) != std::string::npos)
                                {
                                    name.replace(name.find(prevHashString), newHashString.length(), newHashString);
                                    llvm::sys::fs::rename(file->path(), name);
                                }
                            }
                        }
                    }
#endif // defined(IGC_SPIRV_ENABLED)

                    if (success == true)
                    {
                        // if -dump-opt-llvm is enabled dump the llvm output to the file
                        std::string options = "";
                        if ((InputArgs.pOptions != nullptr) && (InputArgs.OptionsSize > 0))
                        {
                            options.append(InputArgs.pOptions, InputArgs.pOptions + InputArgs.OptionsSize);
                        }
                        size_t dumpOptPosition = options.find("-dump-opt-llvm");
                        if (dumpOptPosition != std::string::npos)
                        {
                            std::string dumpFileName;
                            std::istringstream iss(options.substr(dumpOptPosition));
                            iss >> dumpFileName;
                            size_t equalSignPosition = dumpFileName.find('=');
                            if (equalSignPosition != std::string::npos)
                            {
                                dumpFileName = dumpFileName.substr(equalSignPosition + 1);
                                // dump the buffer
                                FILE* file = fopen(dumpFileName.c_str(), "wb");
                                if (file != NULL)
                                {
                                    fwrite(pBufResult, OutputString.size(), 1, file);
                                    fclose(file);
                                }
                            }
                            else
                            {
                                std::string errorString = "File name not specified with the -dump-opt-llvm option.";
                                SetWarningMessage(errorString, OutputArgs);
                            }
                        }
                    }
                }
                else if (IsDeviceBinaryFormat(outType))
                {
                    InputArgs.pInput = OutputString.data();
                    InputArgs.InputSize = OutputString.size();
                    success = TC::TranslateBuild(
                        &InputArgs,
                        &OutputArgs,
                        TB_DATA_FORMAT_LLVM_BINARY,
                        Context.platform,
                        profilingTimerResolution);
                }
                else
                {
                    IGC_ASSERT_MESSAGE(0, "Unrecognized output format when processing ELF input");
                    success = false;
                }
            }
            else
            {
                success = false;
            }
        }
    }

    return success;
}

bool ParseInput(
    llvm::Module*& pKernelModule,
    const STB_TranslateInputArgs* pInputArgs,
    STB_TranslateOutputArgs* pOutputArgs,
    llvm::LLVMContext& oclContext,
    TB_DATA_FORMAT inputDataFormatTemp)
{
    pKernelModule = nullptr;

    // Parse the module we want to compile
    llvm::SMDiagnostic err;
    // For text IR, we don't need the null terminator
    size_t inputSize = pInputArgs->InputSize;

    if (inputDataFormatTemp == TB_DATA_FORMAT_LLVM_TEXT)
    {
        const char* input_ptr = pInputArgs->pInput; // shortcut
        inputSize = std::find(input_ptr, input_ptr + inputSize, 0) - input_ptr;
    }
    llvm::StringRef strInput = llvm::StringRef(pInputArgs->pInput, inputSize);

    // IGC does not handle legacy ocl binary for now (legacy ocl binary
    // is the binary that contains text LLVM IR (2.7 or 3.0).
    if (strInput.size() > 1 && !(strInput[0] == 'B' && strInput[1] == 'C'))
    {
        bool isLLVM27IR = false, isLLVM30IR = false;

        if (strInput.find("triple = \"GHAL3D") != llvm::StringRef::npos)
        {
            isLLVM27IR = true;
        }
        else if ((strInput.find("triple = \"IGIL") != llvm::StringRef::npos) ||
            (strInput.find("metadata !\"image_access_qualifier\"") != llvm::StringRef::npos))
        {
            isLLVM30IR = true;
        }

        if (isLLVM27IR || isLLVM30IR)
        {
            SetErrorMessage("Old LLVM IR (possibly from legacy binary) :  not supported!", *pOutputArgs);
            return false;
        }
    }

    if (inputDataFormatTemp == TB_DATA_FORMAT_LLVM_BINARY)
    {
        std::unique_ptr<llvm::MemoryBuffer> Buf =
            llvm::MemoryBuffer::getMemBuffer(strInput, "<origin>", false);
        llvm::Expected<std::unique_ptr<llvm::Module>> MOE =
            llvm::parseBitcodeFile(Buf->getMemBufferRef(), oclContext);
        if (llvm::Error E = MOE.takeError())
        {
            llvm::handleAllErrors(std::move(E), [&](llvm::ErrorInfoBase &EIB)
            {
                err = llvm::SMDiagnostic(Buf->getBufferIdentifier(), llvm::SourceMgr::DK_Error,
                                         EIB.message());
            });
        }
        else
        {
            // the MemoryBuffer becomes owned by the module and does not need to be managed
            pKernelModule = MOE->release();
        }
    }
    else if (inputDataFormatTemp == TB_DATA_FORMAT_SPIR_V)
    {
#if defined(IGC_SPIRV_ENABLED)
        // convert SPIR-V binary to LLVM module
        std::string stringErrMsg;
        bool success = TranslateSPIRVToLLVM(*pInputArgs, oclContext, strInput, pKernelModule, stringErrMsg);
#else
        std::string stringErrMsg{"SPIRV consumption not enabled for the TARGET."};
        bool success = false;
#endif
        if (!success)
        {
            SetErrorMessage(stringErrMsg, *pOutputArgs);
            return false;
        }
    }
    else
    {
        // NOTE:
        //  llvm::parseIR routine expects input buffer to be zero-terminated,
        //  otherwise we trigger an assertion fail during parseAssemblyInto (from MemoryBuffer::init)
        //  (see llvm/src/lib/Support/MemoryBuffer.cpp).
        pKernelModule = llvm::parseIR({ std::string(strInput.begin(), strInput.size()), "" },
                                      err, oclContext).release();
    }
    if (pKernelModule == nullptr)
    {
        err.print(nullptr, llvm::errs(), false);
        IGC_ASSERT_MESSAGE(0, "Parsing module failed!");
    }
    if (pKernelModule == nullptr)
    {
        SetErrorMessage("Parsing llvm module failed!", *pOutputArgs);
        return false;
    }

    return true;
}

#if defined(IGC_SPIRV_ENABLED)
bool ReadSpecConstantsFromSPIRV(
    std::istream& IS,
    std::vector<std::pair<uint32_t, uint32_t>>& OutSCInfo)
{
#if defined(IGC_SCALAR_USE_KHRONOS_SPIRV_TRANSLATOR)
    // Parse SPIRV Module and add all decorated specialization constants to OutSCInfo vector
    // as a pair of <spec-const-id, spec-const-size-in-bytes>. It's crucial for OCL Runtime to
    // properly validate clSetProgramSpecializationConstant API call.
    return llvm::getSpecConstInfo(IS, OutSCInfo);
#else // IGC Legacy SPIRV Translator
    using namespace igc_spv;

    std::unique_ptr<SPIRVModule> BM(SPIRVModule::createSPIRVModule());
    IS >> *BM;

    auto SPV = BM->parseSpecConstants();

    for (auto& SC : SPV)
    {
        SPIRVType* type = SC->getType();
        uint32_t spec_size = type->isTypeBool() ? 1 : type->getBitWidth() / 8;

        if (SC->hasDecorate(DecorationSpecId))
        {
            SPIRVWord spec_id = *SC->getDecorate(DecorationSpecId).begin();
            Op OP = SC->getOpCode();
            if (OP == OpSpecConstant ||
                OP == OpSpecConstantFalse ||
                OP == OpSpecConstantTrue)
            {
                OutSCInfo.push_back(std::make_pair(spec_id, spec_size));
            }
            else
            {
                IGC_ASSERT_MESSAGE(0, "Wrong instruction opcode, shouldn't be here!");
                return false;
            }
        }
    }
    return true;
#endif
}
#endif

void overrideOCLProgramBinary(
    OpenCLProgramContext& Ctx,
    char*& binaryOutput,
    int& binarySize)
{
    auto name = DumpName(IGC::Debug::GetShaderOutputName())
        .Hash(Ctx.hash)
        .Type(ShaderType::OPENCL_SHADER)
        .Extension("progbin");

    std::string Path = name.overridePath();

    std::ifstream f(Path, std::ios::binary);
    if (!f.is_open())
        return;

    appendToShaderOverrideLogFile(Path, "OVERRIDDEN: ");

    f.seekg(0, f.end);
    int newBinarySize = (int)f.tellg();
    f.seekg(0, f.beg);

    char* newBinaryOutput = new char[newBinarySize];
    f.read(newBinaryOutput, newBinarySize);

    IGC_ASSERT_MESSAGE(f.good(), "Not fully read!");

    delete[] binaryOutput;
    binaryOutput = newBinaryOutput;
    binarySize = newBinarySize;
}

void dumpOCLProgramBinary(
    OpenCLProgramContext& Ctx,
    const char* binaryOutput,
    size_t binarySize)
{
#if LLVM_VERSION_MAJOR >= 7
    auto name = DumpName(IGC::Debug::GetShaderOutputName())
        .Hash(Ctx.hash)
        .Type(ShaderType::OPENCL_SHADER)
        .Extension("progbin");

    std::error_code EC;
    llvm::raw_fd_ostream f(name.str(), EC);

    if (!EC)
        f.write(binaryOutput, binarySize);
#endif
}

static std::unique_ptr<llvm::MemoryBuffer> GetGenericModuleBuffer()
{
    char Resource[5] = {'-'};
    _snprintf(Resource, sizeof(Resource), "#%d", OCL_BC);
    return std::unique_ptr<llvm::MemoryBuffer>{llvm::LoadBufferFromResource(Resource, "BC")};
}

static void WriteSpecConstantsDump(
    const STB_TranslateInputArgs* pInputArgs,
    QWORD hash)
{
    const char* pOutputFolder = IGC::Debug::GetShaderOutputFolder();
    std::ostringstream outputstr;
    for (unsigned i = 0; i < pInputArgs->SpecConstantsSize; ++i)
    {
        outputstr << pInputArgs->pSpecConstantsIds[i] << ": "
                  << pInputArgs->pSpecConstantsValues[i] << "\n";
    }
    DumpShaderFile(pOutputFolder, outputstr.str().c_str(), outputstr.str().size(),
                   hash, "_specconst.txt");
}

bool TranslateBuildSPMD(
    const STB_TranslateInputArgs* pInputArgs,
    STB_TranslateOutputArgs* pOutputArgs,
    TB_DATA_FORMAT inputDataFormatTemp,
    const IGC::CPlatform& IGCPlatform,
    float profilingTimerResolution,
    const ShaderHash& inputShHash)
{
    // This part of code is a critical-section for threads,
    // due static LLVM object which handles options.
    // Setting mutex to ensure that single thread will enter and setup this flag.
    {
        const std::lock_guard<std::mutex> lock(llvm_mutex);
        // Disable code sinking in instruction combining.
        // This is a workaround for a performance issue caused by code sinking
        // that is being done in LLVM's instcombine pass.
        // This code will be removed once sinking is removed from instcombine.
        auto optionsMap = llvm::cl::getRegisteredOptions();
        llvm::StringRef instCombineFlag = "-instcombine-code-sinking=0";
        auto instCombineSinkingSwitch = optionsMap.find(instCombineFlag.trim("-=0"));
        if (instCombineSinkingSwitch != optionsMap.end())
        {
            if (instCombineSinkingSwitch->getValue()->getNumOccurrences() == 0)
            {
                const char* const args[] = { "igc", instCombineFlag.data() };
                llvm::cl::ParseCommandLineOptions(sizeof(args) / sizeof(args[0]), args);
            }
        }
    }

    if (IGC_IS_FLAG_ENABLED(QualityMetricsEnable))
    {
        IGC::Debug::SetDebugFlag(IGC::Debug::DebugFlag::SHADER_QUALITY_METRICS, true);
    }

    MEM_USAGERESET;

    // Parse the module we want to compile
    llvm::Module* pKernelModule = nullptr;
    LLVMContextWrapper* llvmContext = new LLVMContextWrapper;
    RegisterComputeErrHandlers(*llvmContext);

    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
    {
        std::string iof, of, inputf;  // filenames for internal_options.txt, options.txt, and .spv/.bc
        bool isbc = false;

        const char* pOutputFolder = IGC::Debug::GetShaderOutputFolder();
        QWORD hash = inputShHash.getAsmHash();

        if (inputDataFormatTemp == TB_DATA_FORMAT_LLVM_BINARY)
        {
            isbc = true;
            DumpShaderFile(pOutputFolder, pInputArgs->pInput, pInputArgs->InputSize, hash, ".bc", &inputf);
        }
        else if (inputDataFormatTemp == TB_DATA_FORMAT_SPIR_V)
        {
            DumpShaderFile(pOutputFolder, pInputArgs->pInput, pInputArgs->InputSize, hash, ".spv", &inputf);
#if defined(IGC_SPIRV_TOOLS_ENABLED)
            spv_text spirvAsm = nullptr;
            if (DisassembleSPIRV(pInputArgs->pInput, pInputArgs->InputSize, &spirvAsm) == SPV_SUCCESS)
            {
                DumpShaderFile(pOutputFolder, spirvAsm->str, spirvAsm->length, hash, ".spvasm");
            }
            spvTextDestroy(spirvAsm);
#endif // defined(IGC_SPIRV_TOOLS_ENABLED)
        }

        DumpShaderFile(pOutputFolder, pInputArgs->pInternalOptions, pInputArgs->InternalOptionsSize, hash, "_internal_options.txt", &iof);
        DumpShaderFile(pOutputFolder, pInputArgs->pOptions, pInputArgs->OptionsSize, hash, "_options.txt", &of);

        // dump cmd file that has igcstandalone command to compile this kernel.
        std::ostringstream cmdline;
        cmdline << "igcstandalone -api ocl"
                << std::hex
                << " -device 0x" << IGCPlatform.GetProductFamily()
                << ".0x" << IGCPlatform.GetDeviceId()
                << ".0x" << IGCPlatform.GetRevId()
                << std::dec
                << " -inputcs " << getBaseFilename(inputf);
        if (isbc)
        {
            cmdline << " -bitcode";
        }
        if (of.size() > 0)
        {
            cmdline << " -foptions " << getBaseFilename(of);
        }
        if (iof.size() > 0)
        {
            cmdline << " -finternal_options " << getBaseFilename(iof);
        }

        std::string keyvalues, optionstr;
        GetKeysSetExplicitly(&keyvalues, &optionstr);
        std::ostringstream outputstr;
        outputstr << "IGC keys (some dump keys not shown) and command line to compile:\n\n";
        if (!keyvalues.empty())
        {
            outputstr << keyvalues << "\n\n";
        }
        outputstr << cmdline.str() << "\n";

        if (!optionstr.empty())
        {
            outputstr << "\n\nOr using the following with IGC keys set via -option\n\n";
            outputstr << cmdline.str() << " -option " << optionstr << "\n";
        }
        DumpShaderFile(pOutputFolder, outputstr.str().c_str(), outputstr.str().size(), hash, "_cmd.txt");
    }

    if (!ParseInput(pKernelModule, pInputArgs, pOutputArgs, *llvmContext, inputDataFormatTemp))
    {
        return false;
    }
    CDriverInfoOCLNEO driverInfoOCL;
    IGC::CDriverInfo* driverInfo = &driverInfoOCL;

    USC::SShaderStageBTLayout zeroLayout = USC::g_cZeroShaderStageBTLayout;
    IGC::COCLBTILayout oclLayout(&zeroLayout);
    OpenCLProgramContext oclContext(oclLayout, IGCPlatform, pInputArgs, *driverInfo, llvmContext);

#ifdef __GNUC__
    // Get rid of "the address of 'oclContext' will never be NULL" warning
#pragma GCC diagnostic push
#pragma GCC ignored "-Waddress"
#endif // __GNUC__
    COMPILER_TIME_INIT(&oclContext, m_compilerTimeStats);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

    COMPILER_TIME_START(&oclContext, TIME_TOTAL);
    oclContext.m_ProfilingTimerResolution = profilingTimerResolution;

    if (inputDataFormatTemp == TB_DATA_FORMAT_SPIR_V)
    {
        oclContext.setAsSPIRV();
    }

    if (IGC_IS_FLAG_ENABLED(EnableReadGTPinInput))
    {
        // Set GTPin flags
        oclContext.gtpin_init = pInputArgs->GTPinInput;
    }

    oclContext.setModule(pKernelModule);
    if (oclContext.isSPIRV())
    {
        deserialize(*oclContext.getModuleMetaData(), pKernelModule);
    }

    oclContext.hash = inputShHash;
    oclContext.annotater = nullptr;

    // Set default denorm.
    // Note that those values have been set to FLOAT_DENORM_FLUSH_TO_ZERO
    if (IGFX_GEN8_CORE <= oclContext.platform.GetPlatformFamily())
    {
        oclContext.m_floatDenormMode16 = FLOAT_DENORM_RETAIN;
        oclContext.m_floatDenormMode32 = FLOAT_DENORM_RETAIN;
        oclContext.m_floatDenormMode64 = FLOAT_DENORM_RETAIN;
    }

    unsigned PtrSzInBits = pKernelModule->getDataLayout().getPointerSizeInBits();
    // TODO: Again, this should not happen on each compilation

    bool doSplitModule = oclContext.m_InternalOptions.CompileOneKernelAtTime ||
                         IGC_IS_FLAG_ENABLED(CompileOneAtTime);
    // set retry manager
    bool retry = false;
    oclContext.m_retryManager.Enable();
    do
    {
        llvm::TinyPtrVector<const llvm::Function*> kernelFunctions;
        if (doSplitModule)
        {
            for (const auto& F : pKernelModule->functions())
            {
                if (F.getCallingConv() == llvm::CallingConv::SPIR_KERNEL)
                {
                    kernelFunctions.push_back(&F);
                }
            }

            if (retry)
            {
                fprintf(stderr, "IGC recompiles whole module with different optimization strategy, recompiling all kernels \n");
            }
            IGC_ASSERT_EXIT_MESSAGE(kernelFunctions.empty() == false, "No kernels found!");
            fprintf(stderr, "IGC compiles kernels one by one... (%d total)\n", kernelFunctions.size());
        }

        // for Module splitting feature; if it's inactive, flow is as normal
        do {
            KernelModuleSplitter splitter(oclContext, *pKernelModule);
            if (doSplitModule)
            {
                const llvm::Function* pKernelFunction = kernelFunctions.back();

                fprintf(stderr, "Compiling kernel #%d: %s\n", kernelFunctions.size(), pKernelFunction->getName().data());
                kernelFunctions.pop_back();

                splitter.splitModuleForKernel(pKernelFunction);
                splitter.setSplittedModuleInOCLContext();
            }

            std::unique_ptr<llvm::Module> BuiltinGenericModule = nullptr;
            std::unique_ptr<llvm::Module> BuiltinSizeModule = nullptr;
            std::unique_ptr<llvm::MemoryBuffer> pGenericBuffer = nullptr;
            std::unique_ptr<llvm::MemoryBuffer> pSizeTBuffer = nullptr;
            {
                // IGC has two BIF Modules:
                //            1. kernel Module (pKernelModule)
                //            2. BIF Modules:
                //                 a) generic Module (BuiltinGenericModule)
                //                 b) size Module (BuiltinSizeModule)
                //
                // OCL builtin types, such as clk_event_t/queue_t, etc., are struct (opaque) types. For
                // those types, its original names are themselves; the derived names are ones with
                // '.<digit>' appended to the original names. For example,  clk_event_t is the original
                // name, its derived names are clk_event_t.0, clk_event_t.1, etc.
                //
                // When llvm reads in multiple modules, say, M0, M1, under the same llvmcontext, if both
                // M0 and M1 has the same struct type,  M0 will have the original name and M1 the derived
                // name for that type.  For example, clk_event_t,  M0 will have clk_event_t, while M1 will
                // have clk_event_t.2 (number is arbitary). After linking, those two named types should be
                // mapped to the same type, otherwise, we could have type-mismatch (for example, OCL GAS
                // builtin_functions tests will assertion fail during inlining due to type-mismatch).  Furthermore,
                // when linking M1 into M0 (M0 : dstModule, M1 : srcModule), the final type is the type
                // used in M0.

                // Load the builtin module -  Generic BC
                // Load the builtin module -  Generic BC
                {
                    COMPILER_TIME_START(&oclContext, TIME_OCL_LazyBiFLoading);

                    pGenericBuffer = GetGenericModuleBuffer();

                    if (pGenericBuffer == NULL)
                    {
                        SetErrorMessage("Error loading the Generic builtin resource", *pOutputArgs);
                        return false;
                    }

                    llvm::Expected<std::unique_ptr<llvm::Module>> ModuleOrErr =
                        getLazyBitcodeModule(pGenericBuffer->getMemBufferRef(), *oclContext.getLLVMContext());

                    if (llvm::Error EC = ModuleOrErr.takeError())
                    {
                        std::string error_str = "Error lazily loading bitcode for generic builtins,"
                                                "is bitcode the right version and correctly formed?";
                        SetErrorMessage(error_str, *pOutputArgs);
                        return false;
                    }
                    else
                    {
                        BuiltinGenericModule = std::move(*ModuleOrErr);
                    }

                    if (BuiltinGenericModule == NULL)
                    {
                        SetErrorMessage("Error loading the Generic builtin module from buffer", *pOutputArgs);
                        return false;
                    }
                    COMPILER_TIME_END(&oclContext, TIME_OCL_LazyBiFLoading);
                }

                // Load the builtin module -  pointer depended
                {
                    char ResNumber[5] = { '-' };
                    switch (PtrSzInBits)
                    {
                    case 32:
                        _snprintf_s(ResNumber, sizeof(ResNumber), 5, "#%d", OCL_BC_32);
                        break;
                    case 64:
                        _snprintf_s(ResNumber, sizeof(ResNumber), 5, "#%d", OCL_BC_64);
                        break;
                    default:
                        IGC_ASSERT_MESSAGE(0, "Unknown bitness of compiled module");
                    }

                    // the MemoryBuffer becomes owned by the module and does not need to be managed
                    pSizeTBuffer.reset(llvm::LoadBufferFromResource(ResNumber, "BC"));
                    IGC_ASSERT_MESSAGE(pSizeTBuffer, "Error loading builtin resource");

                    llvm::Expected<std::unique_ptr<llvm::Module>> ModuleOrErr =
                        getLazyBitcodeModule(pSizeTBuffer->getMemBufferRef(), *oclContext.getLLVMContext());
                    if (llvm::Error EC = ModuleOrErr.takeError())
                        IGC_ASSERT_MESSAGE(0, "Error lazily loading bitcode for size_t builtins");
                    else
                        BuiltinSizeModule = std::move(*ModuleOrErr);

                    IGC_ASSERT_MESSAGE(BuiltinSizeModule, "Error loading builtin module from buffer");
                }

                BuiltinGenericModule->setDataLayout(BuiltinSizeModule->getDataLayout());
                BuiltinGenericModule->setTargetTriple(BuiltinSizeModule->getTargetTriple());
            }

            oclContext.getModuleMetaData()->csInfo.forcedSIMDSize |= IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth);

            try
            {
                if (llvm::StringRef(oclContext.getModule()->getTargetTriple()).startswith("spir"))
                {
                    IGC::UnifyIRSPIR(&oclContext, std::move(BuiltinGenericModule), std::move(BuiltinSizeModule));
                }
                else // not SPIR
                {
                    IGC::UnifyIROCL(&oclContext, std::move(BuiltinGenericModule), std::move(BuiltinSizeModule));
                }

                if (oclContext.HasError())
                {
                    if (oclContext.HasWarning())
                    {
                        SetOutputMessage(oclContext.GetErrorAndWarning(), *pOutputArgs);
                    }
                    else
                    {
                        SetOutputMessage(oclContext.GetError(), *pOutputArgs);
                    }
                    return false;
                }

                // Compiler Options information available after unification.
                ModuleMetaData* modMD = oclContext.getModuleMetaData();
                if (modMD->compOpt.DenormsAreZero)
                {
                    oclContext.m_floatDenormMode16 = FLOAT_DENORM_FLUSH_TO_ZERO;
                    oclContext.m_floatDenormMode32 = FLOAT_DENORM_FLUSH_TO_ZERO;
                }
                if (IGC_GET_FLAG_VALUE(ForceFastestSIMD))
                {
                    oclContext.m_retryManager.AdvanceState();
                    oclContext.m_retryManager.SetFirstStateId(oclContext.m_retryManager.GetRetryId());
                }
                // Optimize the IR. This happens once for each program, not per-kernel.
                IGC::OptimizeIR(&oclContext);

                // Now, perform code generation
                IGC::CodeGen(&oclContext);
            }
            catch (std::bad_alloc& e)
            {
                (void)e; // not used now
                SetOutputMessage("IGC: Out Of Memory", *pOutputArgs);
                return false;
            }
            catch (std::exception& e)
            {
                if (pOutputArgs->ErrorStringSize == 0 && pOutputArgs->pErrorString == nullptr)
                {
                    std::string message = "IGC: ";
                    message += oclContext.GetErrorAndWarning();
                    message += '\n';
                    message += e.what();
                    SetErrorMessage(message.c_str(), *pOutputArgs);
                }
                return false;
            }

            retry = (!oclContext.m_retryManager.kernelSet.empty() &&
                     oclContext.m_retryManager.AdvanceState());

            if (retry)
            {
                splitter.retry();
                kernelFunctions.clear();
                oclContext.clear();

                // Create a new LLVMContext
                oclContext.initLLVMContextWrapper();

                IGC::Debug::RegisterComputeErrHandlers(*oclContext.getLLVMContext());

                if (!ParseInput(pKernelModule, pInputArgs, pOutputArgs, *oclContext.getLLVMContext(), inputDataFormatTemp))
                {
                    return false;
                }
                oclContext.setModule(pKernelModule);
            }
        } while (!kernelFunctions.empty());
    } while (retry);

    oclContext.failOnSpills();

    if (oclContext.HasError())
    {
        if (oclContext.HasWarning())
        {
            SetOutputMessage(oclContext.GetErrorAndWarning(), *pOutputArgs);
        }
        else
        {
            SetOutputMessage(oclContext.GetError(), *pOutputArgs);
        }
        return false;
    }

    if (oclContext.HasWarning())
    {
        SetOutputMessage(oclContext.GetWarning(), *pOutputArgs);
    }

    // Prepare and set program binary
    unsigned int pointerSizeInBytes = (PtrSzInBits == 64) ? 8 : 4;

    // FIXME: zebin currently only support program output itself, will add debug info
    // into it
    int binarySize = 0;
    char* binaryOutput = nullptr;

    oclContext.metrics.FinalizeStats();
    oclContext.metrics.OutputMetrics();

    if (!IGC_IS_FLAG_ENABLED(EnableZEBinary) &&
        !oclContext.getModuleMetaData()->compOpt.EnableZEBinary)
    {
        Util::BinaryStream programBinary;
        // Patch token based binary format
        oclContext.m_programOutput.CreateKernelBinaries();
        oclContext.m_programOutput.GetProgramBinary(programBinary, pointerSizeInBytes);
        binarySize = static_cast<int>(programBinary.Size());
        binaryOutput = new char[binarySize];
        memcpy_s(binaryOutput, binarySize, programBinary.GetLinearPointer(), binarySize);
    }
    else
    {
        // ze binary foramt
        llvm::SmallVector<char, 64> buf;
        llvm::raw_svector_ostream llvm_os(buf);
        const bool excludeIRFromZEBinary = IGC_IS_FLAG_ENABLED(ExcludeIRFromZEBinary) || oclContext.getModuleMetaData()->compOpt.ExcludeIRFromZEBinary;
        const char* spv_data = nullptr;
        uint32_t spv_size = 0;
        if (inputDataFormatTemp == TB_DATA_FORMAT_SPIR_V && !excludeIRFromZEBinary)
        {
            spv_data = pInputArgs->pInput;
            spv_size = pInputArgs->InputSize;
        }

        // IGC metrics
        size_t metricDataSize = oclContext.metrics.getMetricDataSize();
        auto metricData = reinterpret_cast<const char*>(oclContext.metrics.getMetricData());

        oclContext.m_programOutput.GetZEBinary(llvm_os, pointerSizeInBytes,
            spv_data, spv_size, metricData, metricDataSize, pInputArgs->pOptions, pInputArgs->OptionsSize);

        // FIXME: try to avoid memory copy here
        binarySize = buf.size();
        binaryOutput = new char[binarySize];
        memcpy_s(binaryOutput, binarySize, buf.data(), buf.size());
    }

    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
        dumpOCLProgramBinary(oclContext, binaryOutput, binarySize);

    if (IGC_IS_FLAG_ENABLED(ShaderOverride))
        overrideOCLProgramBinary(oclContext, binaryOutput, binarySize);

    pOutputArgs->OutputSize = binarySize;
    pOutputArgs->pOutput = binaryOutput;

    // Prepare and set program debug data
    size_t debugDataSize = 0;
    oclContext.m_programOutput.GetProgramDebugDataSize(debugDataSize);
    if (debugDataSize > 0)
    {
        char* debugDataOutput = new char[debugDataSize];
        oclContext.m_programOutput.GetProgramDebugData(debugDataOutput, debugDataSize);

        pOutputArgs->DebugDataSize = debugDataSize;
        pOutputArgs->pDebugData = debugDataOutput;
    }

    COMPILER_TIME_END(&oclContext, TIME_TOTAL);

    COMPILER_TIME_PRINT(&oclContext, ShaderType::OPENCL_SHADER, oclContext.hash);

    COMPILER_TIME_DEL(&oclContext, m_compilerTimeStats);

    return true;
}

#if defined(IGC_VC_ENABLED)
bool TranslateBuildVC(
    const STB_TranslateInputArgs* pInputArgs,
    STB_TranslateOutputArgs* pOutputArgs,
    TB_DATA_FORMAT inputDataFormatTemp,
    const IGC::CPlatform& IGCPlatform,
    float profilingTimerResolution,
    const ShaderHash& inputShHash)
{
    IGC_ASSERT(pInputArgs->pOptions &&
              (strstr(pInputArgs->pOptions, "-vc-codegen") ||
               strstr(pInputArgs->pOptions, "-cmc")));

    // Currently, VC compiler effectively uses global variables to store
    // some configuration information. This may lead to problems
    // during multi-threaded compilations. The mutex below serializes
    // the whole compilation process.
    // This is a temporary measure till a proper re-design is done.
    const std::lock_guard<std::mutex> lock(llvm_mutex);

    std::error_code status =
        vc::translateBuild(pInputArgs, pOutputArgs, inputDataFormatTemp,
                           IGCPlatform, profilingTimerResolution);
    return !status;
}
#endif // defined(IGC_VC_ENABLED)

bool TranslateBuild(
    const STB_TranslateInputArgs* pInputArgs,
    STB_TranslateOutputArgs* pOutputArgs,
    TB_DATA_FORMAT inputDataFormatTemp,
    const IGC::CPlatform& IGCPlatform,
    float profilingTimerResolution)
{
    ShaderHash inputShHash = ShaderHashOCL(reinterpret_cast<const UINT*>(pInputArgs->pInput),
                                           pInputArgs->InputSize / 4);

    // on wrong spec constants, vc::translateBuild may fail
    // so lets dump those early
    if (pInputArgs->SpecConstantsSize > 0 && IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
    {
        WriteSpecConstantsDump(pInputArgs, inputShHash.getAsmHash());
    }

#if defined(IGC_VC_ENABLED)
    // if VC option was specified, go to VC compilation directly.
    if (pInputArgs->pOptions && (strstr(pInputArgs->pOptions, "-vc-codegen") ||
                                 strstr(pInputArgs->pOptions, "-cmc")))
    {
        return TranslateBuildVC(pInputArgs, pOutputArgs, inputDataFormatTemp,
                                IGCPlatform, profilingTimerResolution,
                                inputShHash);
    }
#endif // defined(IGC_VC_ENABLED)

    if (inputDataFormatTemp != TB_DATA_FORMAT_SPIR_V)
    {
        return TranslateBuildSPMD(pInputArgs, pOutputArgs, inputDataFormatTemp,
                                  IGCPlatform, profilingTimerResolution,
                                  inputShHash);
    }

    // Recognize if SPIR-V module contains SPMD,ESIMD or SPMD+ESIMD code and compile it.
    std::string errorMessage;
    bool ret = VLD::TranslateBuildSPMDAndESIMD(
        pInputArgs, pOutputArgs, inputDataFormatTemp, IGCPlatform,
        profilingTimerResolution, inputShHash, errorMessage);
    if (!ret && !errorMessage.empty())
    {
        SetErrorMessage(errorMessage, *pOutputArgs);
    }
    return ret;
}

bool CIGCTranslationBlock::FreeAllocations(STB_TranslateOutputArgs* pOutputArgs)
{
    IGC_ASSERT(pOutputArgs);
    delete [] pOutputArgs->pOutput;
    return true;
}

bool CIGCTranslationBlock::Initialize(const STB_CreateArgs* pCreateArgs)
{
    const SGlobalData* pCreateArgsGlobalData =
                  static_cast<const SGlobalData*>(pCreateArgs->pCreateData);

    // IGC maintains its own WA table - ignore the version in the global arguments.
    m_Platform = *pCreateArgsGlobalData->pPlatform;
    m_SkuTable = *pCreateArgsGlobalData->pSkuTable;
    m_SysInfo  = *pCreateArgsGlobalData->pSysInfo;

    m_DataFormatInput  = pCreateArgs->TranslationCode.Type.Input;
    m_DataFormatOutput = pCreateArgs->TranslationCode.Type.Output;

    m_ProfilingTimerResolution = pCreateArgsGlobalData->ProfilingTimerResolution;

    bool validTBChain = false;

    validTBChain |=
        (m_DataFormatInput == TB_DATA_FORMAT_ELF) &&
        (m_DataFormatOutput == TB_DATA_FORMAT_LLVM_BINARY);

    validTBChain |=
        (m_DataFormatInput == TB_DATA_FORMAT_LLVM_TEXT) &&
        IsDeviceBinaryFormat(m_DataFormatOutput);

    validTBChain |=
        (m_DataFormatInput == TB_DATA_FORMAT_LLVM_BINARY) &&
        IsDeviceBinaryFormat(m_DataFormatOutput);

    validTBChain |=
        (m_DataFormatInput == TB_DATA_FORMAT_SPIR_V) &&
        IsDeviceBinaryFormat(m_DataFormatOutput);

    IGC_ASSERT_MESSAGE(validTBChain, "Invalid TB Chain");

    return validTBChain;
}

static constexpr STB_TranslationCode g_cICBETranslationCodes[] =
{
    { { TB_DATA_FORMAT_ELF,           TB_DATA_FORMAT_LLVM_BINARY   } },
    { { TB_DATA_FORMAT_LLVM_TEXT,     TB_DATA_FORMAT_DEVICE_BINARY } },
    { { TB_DATA_FORMAT_LLVM_BINARY,   TB_DATA_FORMAT_DEVICE_BINARY } },
    { { TB_DATA_FORMAT_SPIR_V,        TB_DATA_FORMAT_DEVICE_BINARY } },

    { { TB_DATA_FORMAT_LLVM_TEXT,     TB_DATA_FORMAT_COHERENT_DEVICE_BINARY } },
    { { TB_DATA_FORMAT_LLVM_BINARY,   TB_DATA_FORMAT_COHERENT_DEVICE_BINARY } },
    { { TB_DATA_FORMAT_SPIR_V,        TB_DATA_FORMAT_COHERENT_DEVICE_BINARY } },
    { { TB_DATA_FORMAT_LLVM_TEXT,     TB_DATA_FORMAT_NON_COHERENT_DEVICE_BINARY } },
    { { TB_DATA_FORMAT_LLVM_BINARY,   TB_DATA_FORMAT_NON_COHERENT_DEVICE_BINARY } },
    { { TB_DATA_FORMAT_SPIR_V,        TB_DATA_FORMAT_NON_COHERENT_DEVICE_BINARY } }
};

TRANSLATION_BLOCK_API void Register(STB_RegisterArgs* pRegisterArgs)
{
    pRegisterArgs->Version = TC::STB_VERSION;
    pRegisterArgs->NumTranslationCodes =
        sizeof(g_cICBETranslationCodes) /
        sizeof(g_cICBETranslationCodes[0]);

    if (pRegisterArgs->pTranslationCodes)
    {
        iSTD::MemCopy<sizeof(g_cICBETranslationCodes)>(
            pRegisterArgs->pTranslationCodes,
            g_cICBETranslationCodes);
    }
}

TRANSLATION_BLOCK_API CTranslationBlock* Create(STB_CreateArgs* pCreateArgs)
{
    CIGCTranslationBlock* pIGCTranslationBlock = nullptr;

    CIGCTranslationBlock::Create(
        pCreateArgs,
        pIGCTranslationBlock);

    return pIGCTranslationBlock;
}

TRANSLATION_BLOCK_API void Delete(CTranslationBlock* pTranslationBlock)
{
    CIGCTranslationBlock*  pIGCTranslationBlock =
        static_cast<CIGCTranslationBlock*>(pTranslationBlock);

    CIGCTranslationBlock::Delete(pIGCTranslationBlock);
}

} // namespace TC
