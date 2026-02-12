/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGC/common/StringMacros.hpp"
#include "LLVMSPIRVOpts.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/ScaledNumber.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Process.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/ADT/Optional.h"

#include <cstring>
#include <string>
#include <stdexcept>
#include <fstream>
#include <mutex>
#include <numeric>
#include <chrono>

#include "AdaptorCommon/customApi.hpp"
#include "AdaptorOCL/OCL/LoadBuffer.h"
#include "AdaptorOCL/OCL/BuiltinResource.h"
#include "AdaptorOCL/OCL/TB/igc_tb.h"

#include "AdaptorOCL/UnifyIROCL.hpp"
#include "AdaptorOCL/DriverInfoOCL.hpp"

#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "common/debug/Dump.hpp"
#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "common/secure_mem.h"
#include "common/shaderOverride.hpp"
#include "common/ModuleSplitter.h"
#include "common/IGCSPIRVParser.h"

#include "CLElfLib/ElfReader.h"

#if defined(IGC_VC_ENABLED)
#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include "vc/igcdeps/TranslationInterface.h"
#include "vc/Support/StatusCode.h"
#endif // defined(IGC_VC_ENABLED)

#include <iStdLib/MemCopy.h>

#if defined(IGC_SPIRV_ENABLED)
#include "LLVMSPIRVLib.h"
#endif

#ifdef IGC_SPIRV_TOOLS_ENABLED
#include "spirv-tools/libspirv.h"
#endif

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "common/LLVMWarningsPop.hpp"

#include <sstream>
#include <iomanip>
#include "Probe/Assertion.h"
#include "common/StringMacros.hpp"
#include "VISALinkerDriver/VLD.hpp"
#include "VISALinkerDriver/VLD_SPIRVSplitter.hpp"

#include "SPIRVExtensionsSupport.h"

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
#define HANDLE void *

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
#define strtok_s strtok_r
#define _strdup strdup
#define _snprintf snprintf
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

using namespace IGC::IGCMD;
using namespace IGC::Debug;
using namespace IGC;

namespace TC {

static std::mutex llvm_mutex;

void UnlockMutex() { llvm_mutex.unlock(); }

bool ProcessElfInput(STB_TranslateInputArgs &InputArgs, STB_TranslateOutputArgs &OutputArgs,
                     IGC::OpenCLProgramContext &Context, PLATFORM &platform, const TB_DATA_FORMAT &outType,
                     float profilingTimerResolution);

bool TranslateBuild(const STB_TranslateInputArgs *pInputArgs, STB_TranslateOutputArgs *pOutputArgs,
                    TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform &IGCPlatform,
                    float profilingTimerResolution);

bool CIGCTranslationBlock::ProcessElfInput(STB_TranslateInputArgs &InputArgs, STB_TranslateOutputArgs &OutputArgs,
                                           IGC::OpenCLProgramContext &Context, float ProfilingTimerResolution) {
  return TC::ProcessElfInput(InputArgs, OutputArgs, Context, m_Platform, m_DataFormatOutput, ProfilingTimerResolution);
}

static void SetOutputMessage(const std::string &OutputMessage, STB_TranslateOutputArgs &OutputArgs) {
  OutputArgs.ErrorString = OutputMessage;
}

static void SetWarningMessage(const std::string &OutputMessage, STB_TranslateOutputArgs &OutputArgs) {
  SetOutputMessage("warning: " + OutputMessage, OutputArgs);
}

static void SetErrorMessage(const std::string &OutputMessage, STB_TranslateOutputArgs &OutputArgs) {
  SetOutputMessage("error: " + OutputMessage, OutputArgs);
}

static bool IsDeviceBinaryFormat(const TB_DATA_FORMAT &format) {
  return (format == TB_DATA_FORMAT_DEVICE_BINARY) || (format == TB_DATA_FORMAT_COHERENT_DEVICE_BINARY) ||
         (format == TB_DATA_FORMAT_NON_COHERENT_DEVICE_BINARY);
}

bool CIGCTranslationBlock::Create(const STB_CreateArgs *pCreateArgs, CIGCTranslationBlock *&pTranslationBlock) {
  pTranslationBlock = new CIGCTranslationBlock();
  if (!pTranslationBlock) {
    return false;
  }

  bool success = pTranslationBlock->Initialize(pCreateArgs);
  if (!success) {
    CIGCTranslationBlock::Delete(pTranslationBlock);
  }
  return success;
}

void CIGCTranslationBlock::Delete(CIGCTranslationBlock *pTranslationBlock) { delete pTranslationBlock; }

bool CIGCTranslationBlock::Translate(const STB_TranslateInputArgs *pInputArgs, STB_TranslateOutputArgs *pOutputArgs) {
  LoadRegistryKeys();

  // Create a copy of input arguments that can be modified
  STB_TranslateInputArgs InputArgsCopy = *pInputArgs;

  IGC::CPlatform IGCPlatform(m_Platform);

  IGC::SetGTSystemInfo(&m_SysInfo, &IGCPlatform);
  IGC::SetWorkaroundTable(&m_SkuTable, &IGCPlatform);
  IGC::SetCompilerCaps(&m_SkuTable, &IGCPlatform);

  pOutputArgs->Output.clear();
  pOutputArgs->ErrorString.clear();
  pOutputArgs->DebugData.clear();

  try {
    if (m_DataFormatInput == TB_DATA_FORMAT_ELF) {
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

    if ((m_DataFormatInput == TB_DATA_FORMAT_LLVM_TEXT) || (m_DataFormatInput == TB_DATA_FORMAT_SPIR_V) ||
        (m_DataFormatInput == TB_DATA_FORMAT_LLVM_BINARY)) {
      return TC::TranslateBuild(&InputArgsCopy, pOutputArgs, m_DataFormatInput, IGCPlatform,
                                m_ProfilingTimerResolution);
    } else {
      IGC_ASSERT_MESSAGE(0, "Unsupported input format");
      return false;
    }
  } catch (std::exception &e) {
    if (pOutputArgs->ErrorString.empty()) {
      SetErrorMessage(std::string("IGC: ") + e.what(), *pOutputArgs);
    }
    return false;
  } catch (...) {
    if (pOutputArgs->ErrorString.empty()) {
      SetErrorMessage("IGC: Internal Compiler Error", *pOutputArgs);
    }
    return false;
  }

  return false;
}

std::unordered_map<uint32_t, uint64_t> UnpackSpecConstants(const uint32_t *pSpecConstantsIds,
                                                           const uint64_t *pSpecConstantsValues, uint32_t size) {
  std::unordered_map<uint32_t, uint64_t> outSpecConstantsMap;
  for (uint32_t i = 0; i < size; i++) {
    outSpecConstantsMap[pSpecConstantsIds[i]] = pSpecConstantsValues[i];
  }
  return outSpecConstantsMap;
}

void GenerateCompilerOptionsMD(llvm::LLVMContext &C, llvm::Module &M, llvm::StringRef options) {
  llvm::SmallVector<llvm::StringRef, 8> flags;
  llvm::StringRef sep(" ");
  options.split(flags, sep, -1, false);

  std::vector<llvm::Metadata *> ValueVec;
  for (auto flag : flags) {
    flag = flag.trim();
    flag = flag.rtrim(0); // make sure no ending 0
    // flags : C string (ended with 0)
    if (!flag.empty() && flag.front() != 0)
      ValueVec.push_back(llvm::MDString::get(C, flag));
  }
  llvm::NamedMDNode *NamedMD = M.getOrInsertNamedMetadata("opencl.compiler.options");
  NamedMD->addOperand(llvm::MDNode::get(C, ValueVec));
}

// Ensure unnamed global variables are assigned names immediately after translating from SPIRV to LLVM.
// This must occur before removing kernels that do not require recompilation.
// Naming global variables after kernels removal can result in inconsistent naming compared to the first compilation,
// potentially causing crashes in the ProgramScopeConstantAnalysis pass.
void AssignNamesToUnnamedGlobalVariables(llvm::Module &M) {
  for (auto &G : M.globals()) {
    if (!G.hasName()) {
      G.setName("gVar");
    }
  }
}

// Dump shader (binary or text), to output directory.
// Create directory if it doesn't exist.
// Works for all OSes.
// ext - file name suffix (optional) and extension.
void DumpShaderFile(const std::string &dstDir, const char *pBuffer, const UINT bufferSize, const QWORD hash,
                    const std::string &ext, std::string *fullFilePath = nullptr) {
  if (!pBuffer || bufferSize == 0) {
    return;
  }

  std::ostringstream fileName(std::ostringstream::ate);
  fileName << "OCL_asm" << std::hex << std::setfill('0') << std::setw(sizeof(hash) * CHAR_BIT / 4) << hash << std::dec
           << std::setfill(' ') << ext;
  std::string fullFilePathStr = dstDir + fileName.str();

  if (doesRegexMatch(fileName.str(), IGC_GET_REGKEYSTRING(ShaderDumpRegexFilter))) {
    FILE *pFile = NULL;
    fopen_s(&pFile, fullFilePathStr.c_str(), "wb");
    if (pFile) {
      fwrite(pBuffer, 1, bufferSize, pFile);
      fclose(pFile);
    }
  }

  if (fullFilePath != nullptr) {
    *fullFilePath = std::move(fullFilePathStr);
  }
}

std::string getBaseFilename(const std::string &FName) {
#if defined(_WIN32)
  const char Sep = '\\'; // Windows file separator
#else
  const char Sep = '/'; // Linux file separator
#endif
  size_t i = FName.rfind(Sep);
  return (i == std::string::npos ? FName : FName.substr(i + 1));
}

#if defined(IGC_SPIRV_TOOLS_ENABLED)
spv_result_t DisassembleSPIRV(const char *pBuffer, UINT bufferSize, spv_text *outSpirvAsm) {
  const spv_target_env target_env = SPV_ENV_UNIVERSAL_1_3;
  spv_context context = spvContextCreate(target_env);
  const uint32_t *const binary = reinterpret_cast<const uint32_t *>(pBuffer);
  const size_t word_count = bufferSize / sizeof(uint32_t);
  const uint32_t options = (SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES | SPV_BINARY_TO_TEXT_OPTION_INDENT |
                            SPV_BINARY_TO_TEXT_OPTION_SHOW_BYTE_OFFSET);
  spv_diagnostic diagnostic = nullptr;

  const spv_result_t result = spvBinaryToText(context, binary, word_count, options, outSpirvAsm, &diagnostic);

  spvContextDestroy(context);
  spvDiagnosticDestroy(diagnostic);
  return result;
}
#endif // defined(IGC_SPIRV_TOOLS_ENABLED)

#if defined(IGC_SPIRV_ENABLED)
bool CheckForImageUsage(const std::string &SPIRVBinary) {
  std::istringstream repIS(SPIRVBinary);
  std::optional<SPIRV::SPIRVModuleReport> report = IGCLLVM::makeOptional(SPIRV::getSpirvReport(repIS));

  if (!report.has_value())
    return false;

  SPIRV::SPIRVModuleTextReport textReport = SPIRV::formatSpirvReport(report.value());

  auto it = std::find(textReport.Capabilities.begin(), textReport.Capabilities.end(), "ImageBasic");
  return it != textReport.Capabilities.end();
}

void GenerateSPIRVExtensionsMD(llvm::LLVMContext &C, llvm::Module &M, const std::string &SPIRVBinary) {
  std::istringstream repIS(SPIRVBinary);
  std::optional<SPIRV::SPIRVModuleReport> report = IGCLLVM::makeOptional(SPIRV::getSpirvReport(repIS));

  if (!report.has_value())
    return;

  if (report->Extensions.empty())
    return;

  std::vector<llvm::Metadata *> ExtensionsVec;
  for (const auto &E : report->Extensions) {
    ExtensionsVec.push_back(llvm::MDString::get(C, E));
  }

  llvm::NamedMDNode *SPIRVExtensionsMD = M.getOrInsertNamedMetadata("igc.spirv.extensions");
  SPIRVExtensionsMD->addOperand(llvm::MDNode::get(C, ExtensionsVec));
}

IGCLLVM::optional<SPIRV::ExtensionID> ToExtensionID(const std::string &Name) {
  using E = SPIRV::ExtensionID;
  static const std::unordered_map<std::string, E> ExtensionNameToIDMap = {
#define EXT(X) {#X, E::X},
#include "LLVMSPIRVExtensions.inc"
#undef EXT
  };
  if (auto it = ExtensionNameToIDMap.find(Name); it != ExtensionNameToIDMap.end())
    return it->second;
  return IGCLLVM::optional<E>();
}

// Translate SPIR-V binary to LLVM Module
bool TranslateSPIRVToLLVM(const STB_TranslateInputArgs &InputArgs, llvm::LLVMContext &Context,
                          llvm::StringRef SPIRVBinary, llvm::Module *&LLVMModule, std::string &stringErrMsg,
                          const PLATFORM &platform) {
  bool success = true;
  std::istringstream IS(SPIRVBinary.str());
  std::unordered_map<uint32_t, uint64_t> specIDToSpecValueMap =
      UnpackSpecConstants(InputArgs.pSpecConstantsIds, InputArgs.pSpecConstantsValues, InputArgs.SpecConstantsSize);

  // Set SPIRV-LLVM-Translator translation options
  SPIRV::TranslatorOpts Opts;
  Opts.enableGenArgNameMD();
  if (IGC_IS_FLAG_ENABLED(ValidateSPIRVExtensionSupport)) {
    std::vector<IGC::SPIRVExtensionsSupport::SPIRVExtension> SupportedExtensions =
        IGC::SPIRVExtensionsSupport::getSupportedExtensionInfo(platform, true);
    for (const auto &Ext : SupportedExtensions) {
      if (auto id = ToExtensionID(Ext.Name))
        Opts.setAllowedToUseExtension(*id);
    }
  } else {
    Opts.enableAllExtensions();
  }

  Opts.setDesiredBIsRepresentation(SPIRV::BIsRepresentation::SPIRVFriendlyIR);

  // This option has to be enabled since SPIRV-Translator for LLVM13 because of:
  // https://github.com/KhronosGroup/SPIRV-LLVM-Translator/commit/835eb7e. This change
  // has been also backported to SPIRV-Translator for LLVM11.
  Opts.setPreserveOCLKernelArgTypeMetadataThroughString(true);

  // Unpack specialization constants passed from OCL Runtime (Acquired from
  // clSetProgramSpecializationConstant API call). It is also passed as a
  // translation options.
  if (InputArgs.SpecConstantsSize) {
    for (const auto &SC : specIDToSpecValueMap)
      Opts.setSpecConst(SC.first, SC.second);
  }

  if (platform.eProductFamily == IGFX_PVC) {
    if (CheckForImageUsage(SPIRVBinary.str())) {
      stringErrMsg = "For PVC platform images should not be used";
      return false;
    }
  }


  // Actual translation from SPIR-V to LLVM
  success = llvm::readSpirv(Context, Opts, IS, LLVMModule, stringErrMsg);

  if (success) {
    AssignNamesToUnnamedGlobalVariables(*LLVMModule);

    // Handle OpenCL Compiler Options
    GenerateCompilerOptionsMD(Context, *LLVMModule, llvm::StringRef(InputArgs.pOptions, InputArgs.OptionsSize));

    // Parse SPIRV extensions and encode them as 'igc.spirv.extensions' metadata
    GenerateSPIRVExtensionsMD(Context, *LLVMModule, SPIRVBinary.str());

    if (IGC_IS_FLAG_ENABLED(ShaderDumpTranslationOnly))
      LLVMModule->dump();
  }

  return success;
}
#endif // defined(IGC_SPIRV_ENABLED)

bool ProcessElfInput(STB_TranslateInputArgs &InputArgs, STB_TranslateOutputArgs &OutputArgs,
                     IGC::OpenCLProgramContext &Context, PLATFORM &platform, const TB_DATA_FORMAT &outType,
                     float profilingTimerResolution) {
  ShaderHash previousHash;
  bool success = true;
  std::string ErrorMsg;

  CLElfLib::CElfReader *pElfReader = CLElfLib::CElfReader::Create(InputArgs.pInput, InputArgs.InputSize);
  CLElfLib::RAIIElf X(pElfReader); // When going out of scope this object calls the Delete() function automatically

  // If input buffer is an ELF file, then process separately
  const CLElfLib::SElfHeader *pHeader = pElfReader->GetElfHeader();
  if (pHeader != NULL) {
    // Create an empty module to store the output
    std::unique_ptr<llvm::Module> OutputModule;

#if defined(IGC_SPIRV_ENABLED)
    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
      // Dumping SPIRV files with temporary hashes
      for (unsigned i = 1; i < pHeader->NumSectionHeaderEntries; i++) {
        const CLElfLib::SElfSectionHeader *pSectionHeader = pElfReader->GetSectionHeader(i);
        IGC_ASSERT(pSectionHeader != NULL);
        if (pSectionHeader->Type != CLElfLib::SH_TYPE_SPIRV) {
          continue;
        }

        char *pSPIRVBitcode = NULL;
        size_t size = 0;
        pElfReader->GetSectionData(i, pSPIRVBitcode, size);
        // The hash created here (from Input) is only temporary and will be replaced
        // if the LLVM translation and linking finishes successfully
        previousHash = ShaderHashOCL(reinterpret_cast<const UINT *>(InputArgs.pInput), InputArgs.InputSize / 4);
        QWORD hash = previousHash.getAsmHash();

        // beyond of general hash, each SPIR-V module needs to have it's own hash
        QWORD spvHash = ShaderHashOCL((const UINT *)pSPIRVBitcode, size / 4).getAsmHash();
        std::ostringstream spvHashSuffix("_", std::ostringstream::ate);
        spvHashSuffix << std::hex << std::setfill('0') << std::setw(sizeof(spvHash) * CHAR_BIT / 4) << spvHash;
        const std::string suffix = spvHashSuffix.str();
        const char *pOutputFolder = IGC::Debug::GetShaderOutputFolder();

        // Remove any already existing SPIR-V dumps from GetSpecConstantsInfo
        // and dump new ones with correct names
        std::string spvHashString = suffix.c_str();
        spvHashString.erase(0, 1);
        std::string prevSpvPath = pOutputFolder;
        prevSpvPath += "OCL_asm" + spvHashString + ".spv";
        llvm::sys::fs::remove(prevSpvPath);
        DumpShaderFile(pOutputFolder, pSPIRVBitcode, size, hash, suffix + ".spv");

#if defined(IGC_SPIRV_TOOLS_ENABLED)
        if (IGC_IS_FLAG_ENABLED(SpvAsmDumpEnable)) {
          spv_text spirvAsm = nullptr;
          // Similarly replace any spvasm dump from GetSpecConstantsInfo
          std::string prevSpvAsmPath = pOutputFolder;
          prevSpvAsmPath += "OCL_asm" + spvHashString + ".spvasm";
          llvm::sys::fs::remove(prevSpvAsmPath);
          if (DisassembleSPIRV(pSPIRVBitcode, size, &spirvAsm) == SPV_SUCCESS) {
            DumpShaderFile(pOutputFolder, spirvAsm->str, spirvAsm->length, hash, suffix + ".spvasm");
          }
          spvTextDestroy(spirvAsm);
        }

#endif // defined(IGC_SPIRV_TOOLS_ENABLED)
      }
    }
#endif // defined(IGC_SPIRV_ENABLED)

    std::vector<std::unique_ptr<llvm::Module>> LLVMBinariesToLink;
    std::vector<VLD::SPVTranslationPair> SPIRVToLink;

    bool hasSPMD = false;
    bool hasESIMD = false;
    bool hasSPMD_ESIMD = false;

    // Iterate over all the input modules.
    for (unsigned i = 1; i < pHeader->NumSectionHeaderEntries; i++) {
      const CLElfLib::SElfSectionHeader *pSectionHeader = pElfReader->GetSectionHeader(i);
      IGC_ASSERT(pSectionHeader != NULL);

      char *pData = NULL;
      size_t dataSize = 0;

      if (pSectionHeader->Type == CLElfLib::SH_TYPE_SPIRV_SC_IDS) {
        pElfReader->GetSectionData(i, pData, dataSize);
        InputArgs.pSpecConstantsIds = reinterpret_cast<const uint32_t *>(pData);
        InputArgs.SpecConstantsSize = static_cast<uint32_t>(dataSize / sizeof(uint32_t));
      }

      if (pSectionHeader->Type == CLElfLib::SH_TYPE_SPIRV_SC_VALUES) {
        pElfReader->GetSectionData(i, pData, dataSize);
        InputArgs.pSpecConstantsValues = reinterpret_cast<const uint64_t *>(pData);
      }

      if ((pSectionHeader->Type == CLElfLib::SH_TYPE_OPENCL_LLVM_BINARY) ||
          (pSectionHeader->Type == CLElfLib::SH_TYPE_OPENCL_LLVM_ARCHIVE) ||
          (pSectionHeader->Type == CLElfLib::SH_TYPE_SPIRV)) {
        pElfReader->GetSectionData(i, pData, dataSize);

        // Create input module from the buffer
        llvm::StringRef buf(pData, dataSize);

        if (pSectionHeader->Type == CLElfLib::SH_TYPE_SPIRV) {
          auto spvMetadataOrErr = VLD::GetVLDMetadata(buf.data(), buf.size());
          VLD::SPVMetadata spvMetadata;
          if (!spvMetadataOrErr) {
            // Temporary workaround until VLD uses SPIR-V Tools.
            llvm::consumeError(spvMetadataOrErr.takeError());
            spvMetadata.SpirvType = VLD::SPIRVTypeEnum::SPIRV_SPMD;
          } else {
            spvMetadata = *spvMetadataOrErr;
          }

          switch (spvMetadata.SpirvType) {
          case VLD::SPIRVTypeEnum::SPIRV_SPMD:
            hasSPMD = true;
            break;
          case VLD::SPIRVTypeEnum::SPIRV_ESIMD:
            hasESIMD = true;
            break;
          case VLD::SPIRVTypeEnum::SPIRV_SPMD_AND_ESIMD:
            hasSPMD_ESIMD = true;
            break;
          default:
            SetErrorMessage("Unsupported SPIR-V in ELF file!", OutputArgs);
            return false;
          }

          // Copy args, as they hold optional spec constants.
          STB_TranslateInputArgs SpvArgs = InputArgs;
          SpvArgs.pInput = pData;
          SpvArgs.InputSize = dataSize;
          SPIRVToLink.push_back({spvMetadata, SpvArgs});

          // unset specialization constants, to avoid using them by
          // subsequent SPIR-V modules
          InputArgs.pSpecConstantsIds = nullptr;
          InputArgs.pSpecConstantsValues = nullptr;
          InputArgs.SpecConstantsSize = 0;
        } else {
          std::unique_ptr<llvm::MemoryBuffer> pInputBuffer = llvm::MemoryBuffer::getMemBuffer(buf, "", false);

          llvm::Expected<std::unique_ptr<llvm::Module>> errorOrModule =
              llvm::parseBitcodeFile(pInputBuffer->getMemBufferRef(), *Context.getLLVMContext());
          if (auto Err = errorOrModule.takeError()) {
            success = false;
            llvm::handleAllErrors(std::move(Err),
                                  [&](llvm::ErrorInfoBase &EIB) { SetErrorMessage(EIB.message(), OutputArgs); });
          } else {
            LLVMBinariesToLink.push_back(std::move(errorOrModule.get()));
          }
        }

        if (!success) {
          return false;
        }
      }
    }

    bool hasVISALinking = hasSPMD_ESIMD || (hasESIMD && hasSPMD);
    bool hasLLVMBinaries = !LLVMBinariesToLink.empty();
    if ((hasESIMD || hasSPMD_ESIMD) && hasLLVMBinaries) {
      SetErrorMessage("ELF file contained ESIMD SPIR-V and LLVM binaries "
                      "to be linked. This use-case is not supported.",
                      OutputArgs);
      return false;
    }

#if defined(IGC_VC_ENABLED)
    bool hasVCCodegenOpt = false;
    if (InputArgs.pOptions) {
      std::string options(InputArgs.pOptions, InputArgs.OptionsSize);
      hasVCCodegenOpt = (options.find("-vc-codegen") != std::string::npos);
    }
    hasVISALinking &= !hasVCCodegenOpt;
#endif

    if (!hasVISALinking) {
      for (auto &SpvPair : SPIRVToLink) {
        llvm::Module *pKernelModule = nullptr;
#if defined(IGC_SPIRV_ENABLED)
        Context.setAsSPIRV();
        std::string stringErrMsg;
        llvm::StringRef buf(SpvPair.second.pInput, SpvPair.second.InputSize);
        success =
            TranslateSPIRVToLLVM(SpvPair.second, *Context.getLLVMContext(), buf, pKernelModule, stringErrMsg, platform);
        if (!success) {
          SetErrorMessage(stringErrMsg, OutputArgs);
          return false;
        }
        LLVMBinariesToLink.push_back(std::unique_ptr<llvm::Module>(pKernelModule));
#else
        std::string stringErrMsg{"SPIRV consumption not enabled for the TARGET."};
        bool success = false;
#endif
      }
    }

    Context.getLLVMContext()->setDiagnosticHandlerCallBack(
        [](const llvm::DiagnosticInfo &DI, void *Ptr) {
          if (DI.getSeverity() == llvm::DS_Error) {
            auto *S = static_cast<std::string *>(Ptr);
            llvm::raw_string_ostream OS(*S);
            llvm::DiagnosticPrinterRawOStream DP(OS);
            DI.print(DP);
            OS << '\n';
          }
        },
        &ErrorMsg);

    for (auto &InputModule : LLVMBinariesToLink) {
      if (OutputModule.get() == NULL) {
        InputModule.swap(OutputModule);
      } else {
        success = !llvm::Linker::linkModules(*OutputModule, std::move(InputModule));
      }

      if (!success) {
        break;
      }
    }

    if (!success) {
      SetErrorMessage(ErrorMsg.empty() ? "Module linking failed." : ErrorMsg, OutputArgs);
    }

    if (success == true) {
      // Now that the output modules are linked the resulting module needs to be
      // serialized out
      IGC_ASSERT(OutputArgs.Output.empty()); // Make sure we're not overwriting anything here.
      llvm::raw_svector_ostream OStream(OutputArgs.Output);
      if (OutputModule.get()) {
        llvm::WriteBitcodeToFile(*OutputModule.get(), OStream);
      } else {
        // OutputModule can be null only if we use visa linking.
        IGC_ASSERT(hasVISALinking);
      }

      if (outType == TB_DATA_FORMAT_LLVM_BINARY) {
        // Create a copy of the string to return to the caller.
        if (!OutputArgs.Output.empty()) {
#if defined(IGC_SPIRV_ENABLED)
          if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
            // This part renames the previously dumped SPIR-V files
            // so that the hash in their name matches the one of LLVM files
            const char *outputDir = IGC::Debug::GetShaderOutputFolder();

            QWORD prevAsmHash = previousHash.getAsmHash();
            std::ostringstream oss1(std::ostringstream::ate);
            oss1 << std::hex << std::setfill('0') << std::setw(sizeof(prevAsmHash) * CHAR_BIT / 4) << prevAsmHash;
            const std::string prevHashString = oss1.str();

            QWORD newAsmHash =
                ShaderHashOCL((const UINT *)OutputArgs.Output.data(), OutputArgs.Output.size() / 4).getAsmHash();
            std::ostringstream oss2(std::ostringstream::ate);
            oss2 << std::hex << std::setfill('0') << std::setw(sizeof(newAsmHash) * CHAR_BIT / 4) << newAsmHash;
            const std::string newHashString = oss2.str();

            std::error_code ec;
            for (llvm::sys::fs::directory_iterator file(outputDir, ec), fileEnd; file != fileEnd && !ec;
                 file.increment(ec)) {
              if (llvm::sys::fs::is_regular_file(file->path())) {
                std::string name = file->path();
                // Rename file if it contains the previous hash
                if (name.find(prevHashString) != std::string::npos) {
                  name.replace(name.find(prevHashString), newHashString.length(), newHashString);
                  llvm::sys::fs::rename(file->path(), name);
                }
              }
            }
          }
#endif // defined(IGC_SPIRV_ENABLED)

          // if -dump-opt-llvm is enabled dump the llvm output to the file
          std::string options = "";
          if ((InputArgs.pOptions != nullptr) && (InputArgs.OptionsSize > 0)) {
            options.append(InputArgs.pOptions, InputArgs.pOptions + InputArgs.OptionsSize);
          }
          size_t dumpOptPosition = options.find("-dump-opt-llvm");
          if (dumpOptPosition != std::string::npos) {
            std::string dumpFileName;
            std::istringstream iss(options.substr(dumpOptPosition));
            iss >> dumpFileName;
            size_t equalSignPosition = dumpFileName.find('=');
            if (equalSignPosition != std::string::npos) {
              dumpFileName = dumpFileName.substr(equalSignPosition + 1);
              // dump the buffer
              FILE *file = fopen(dumpFileName.c_str(), "wb");
              if (file != NULL) {
                fwrite(OutputArgs.Output.data(), OutputArgs.Output.size(), 1, file);
                fclose(file);
              }
            } else {
              std::string errorString = "File name not specified with the -dump-opt-llvm option.";
              SetWarningMessage(errorString, OutputArgs);
            }
          }
        } else {
          success = false;
        }
      } else if (IsDeviceBinaryFormat(outType)) {
        if (hasVISALinking) {
          ShaderHash hash = ShaderHashOCL(reinterpret_cast<const UINT *>(InputArgs.pInput), InputArgs.InputSize / 4);
          std::string errorMessage;

          // Temporary workaround for invoke_sycl case.
          std::reverse(SPIRVToLink.begin(), SPIRVToLink.end());

          success = VLD::TranslateBuildSPMDAndESIMD(SPIRVToLink, &OutputArgs, TB_DATA_FORMAT_SPIR_V, Context.platform,
                                                    profilingTimerResolution, hash, errorMessage);

          if (!success) {
            SetErrorMessage(errorMessage, OutputArgs);
            return false;
          }
        } else {
          auto OutputIsTheNewInput = std::move(OutputArgs.Output);
          InputArgs.pInput = OutputIsTheNewInput.data();
          InputArgs.InputSize = OutputIsTheNewInput.size();
          OutputArgs.Output = OutBufferType(); // Reinitialize pOutput after moving it to InputArgs as pInput.
          success = TC::TranslateBuild(&InputArgs, &OutputArgs, TB_DATA_FORMAT_LLVM_BINARY, Context.platform,
                                       profilingTimerResolution);
          InputArgs.pInput = nullptr;
          InputArgs.InputSize = 0;
        }
      } else {
        IGC_ASSERT_MESSAGE(0, "Unrecognized output format when processing ELF input");
        success = false;
      }
    }
  }

  return success;
}

bool ParseInput(llvm::Module *&pKernelModule, const STB_TranslateInputArgs *pInputArgs, std::string &errorString,
                llvm::LLVMContext &oclContext, TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform &IGCPlatform) {
  pKernelModule = nullptr;

  // Parse the module we want to compile
  llvm::SMDiagnostic err;
  // For text IR, we don't need the null terminator
  size_t inputSize = pInputArgs->InputSize;

  if (inputDataFormatTemp == TB_DATA_FORMAT_LLVM_TEXT) {
    const char *input_ptr = pInputArgs->pInput; // shortcut
    inputSize = std::find(input_ptr, input_ptr + inputSize, 0) - input_ptr;
  }
  llvm::StringRef strInput = llvm::StringRef(pInputArgs->pInput, inputSize);

  // IGC does not handle legacy ocl binary for now (legacy ocl binary
  // is the binary that contains text LLVM IR (2.7 or 3.0).
  if (!strInput.startswith("BC")) {
    bool isLLVM27IR = false, isLLVM30IR = false;

    if (strInput.find("triple = \"GHAL3D") != llvm::StringRef::npos) {
      isLLVM27IR = true;
    } else if ((strInput.find("triple = \"IGIL") != llvm::StringRef::npos) ||
               (strInput.find("metadata !\"image_access_qualifier\"") != llvm::StringRef::npos)) {
      isLLVM30IR = true;
    }

    if (isLLVM27IR || isLLVM30IR) {
      errorString = "Old LLVM IR (possibly from legacy binary) :  not supported!";
      return false;
    }
  }

  if (inputDataFormatTemp == TB_DATA_FORMAT_LLVM_BINARY) {
    std::unique_ptr<llvm::MemoryBuffer> Buf = llvm::MemoryBuffer::getMemBuffer(strInput, "<origin>", false);
    llvm::Expected<std::unique_ptr<llvm::Module>> MOE = llvm::parseBitcodeFile(Buf->getMemBufferRef(), oclContext);
    if (llvm::Error E = MOE.takeError()) {
      llvm::handleAllErrors(std::move(E), [&](llvm::ErrorInfoBase &EIB) {
        err = llvm::SMDiagnostic(Buf->getBufferIdentifier(), llvm::SourceMgr::DK_Error, EIB.message());
      });
    } else {
      // the MemoryBuffer becomes owned by the module and does not need to be managed
      pKernelModule = MOE->release();
    }
  } else if (inputDataFormatTemp == TB_DATA_FORMAT_SPIR_V) {
#if defined(IGC_SPIRV_ENABLED)
    // convert SPIR-V binary to LLVM module
    bool success =
        TranslateSPIRVToLLVM(*pInputArgs, oclContext, strInput, pKernelModule, errorString, (PLATFORM &)IGCPlatform);
#else
    std::string stringErrMsg{"SPIRV consumption not enabled for the TARGET."};
    bool success = false;
#endif
    if (!success) {
      return false;
    }
  } else {
    // NOTE:
    //  llvm::parseIR routine expects input buffer to be zero-terminated,
    //  otherwise we trigger an assertion fail during parseAssemblyInto (from MemoryBuffer::init)
    //  (see llvm/src/lib/Support/MemoryBuffer.cpp).
    pKernelModule = llvm::parseIR({std::string(strInput.begin(), strInput.size()), ""}, err, oclContext).release();
  }
  if (pKernelModule == nullptr) {
    err.print(nullptr, llvm::errs(), false);
    IGC_ASSERT_MESSAGE(0, "Parsing module failed!");
    errorString = "Parsing llvm module failed!";
    return false;
  }

  return true;
}

void RebuildGlobalAnnotations(IGC::OpenCLProgramContext &oclContext, Module *pKernelModule) {
  auto globalAnnotations = pKernelModule->getGlobalVariable("llvm.global.annotations");
  if (!globalAnnotations)
    return;

  auto requiresRecompilation = [&oclContext](Function *F) {
    return oclContext.m_retryManager->kernelSet.find(F->getName().str()) != oclContext.m_retryManager->kernelSet.end();
  };

  std::vector<Constant *> newGlobalAnnotations;
  auto annotations_array = cast<ConstantArray>(globalAnnotations->getOperand(0));
  for (const auto &op : annotations_array->operands()) {
    auto annotation_struct = cast<ConstantStruct>(op.get());
    auto operand0 = annotation_struct->getOperand(0);

    Function *annotated_function = nullptr;

    // On typed pointers it was: struct -> bitcast -> functionPointer
    // On opaque pointer the bitcast is just "ptr", so it is: struct -> functionPointer
    // Non-opaque pointers path should be removed after transition to opaque pointers.
    if (IGCLLVM::isPointerTy(operand0->getType())) {
      annotated_function = cast<Function>(operand0);
    } else {
      annotated_function = cast<Function>(operand0->getOperand(0));
    }

    IGC_ASSERT_MESSAGE(annotated_function, "Annotated function was not found!");

    if (requiresRecompilation(annotated_function)) {
      newGlobalAnnotations.push_back(annotation_struct);
    }
  }

  // Remove old "llvm.global.annotations" that refers to kernels not requiring recompilation
  globalAnnotations->eraseFromParent();

  if (newGlobalAnnotations.empty()) {
    return;
  }

  // Create new "llvm.global.annotations" that refers only to kernels that need to be recompiled
  Constant *Array = ConstantArray::get(ArrayType::get(newGlobalAnnotations[0]->getType(), newGlobalAnnotations.size()),
                                       newGlobalAnnotations);
  auto *GV = new GlobalVariable(*pKernelModule, Array->getType(), /*IsConstant*/ false, GlobalValue::AppendingLinkage,
                                Array, "llvm.global.annotations");
  GV->setSection("llvm.metadata");
}

#if defined(IGC_SPIRV_ENABLED)
bool ReadSpecConstantsFromSPIRV(std::istream &IS, std::vector<std::pair<uint32_t, uint32_t>> &OutSCInfo) {
  // Parse SPIRV Module and add all decorated specialization constants to OutSCInfo vector
  // as a pair of <spec-const-id, spec-const-size-in-bytes>. It's crucial for OCL Runtime to
  // properly validate clSetProgramSpecializationConstant API call.
#if LLVM_VERSION_MAJOR < 16
  return llvm::getSpecConstInfo(IS, OutSCInfo);
#else
  auto scInfoVec = std::vector<llvm::SpecConstInfoTy>();
  bool result = llvm::getSpecConstInfo(IS, scInfoVec);

  for (auto &entry : scInfoVec) {
    OutSCInfo.emplace_back(entry.ID, entry.Size);
  }

  return result;
#endif
}
#endif

void overrideOCLProgramBinary(OpenCLProgramContext &Ctx, OutBufferType &binaryOutput) {
  auto name =
      DumpName(IGC::Debug::GetShaderOutputName()).Hash(Ctx.hash).Type(ShaderType::OPENCL_SHADER).Extension("progbin");

  std::string Path = name.overridePath();

  std::ifstream f(Path, std::ios::binary);
  if (!f.is_open())
    return;

  appendToShaderOverrideLogFile(Path, "OVERRIDDEN: ");

  f.seekg(0, f.end);
  size_t newBinarySize = (size_t)f.tellg();
  f.seekg(0, f.beg);

  binaryOutput.resize(newBinarySize);
  f.read(binaryOutput.data(), newBinarySize);

  IGC_ASSERT_MESSAGE(f.good(), "Not fully read!");
}

void dumpOCLProgramBinary(const char *fileName, const char *binaryOutput, size_t binarySize) {
  std::error_code EC;
  llvm::raw_fd_ostream f(fileName, EC);

  if (!EC)
    f.write(binaryOutput, binarySize);
}

void dumpOCLProgramBinary(OpenCLProgramContext &Ctx, const char *binaryOutput, size_t binarySize) {
  auto name =
      DumpName(IGC::Debug::GetShaderOutputName()).Hash(Ctx.hash).Type(ShaderType::OPENCL_SHADER).Extension("progbin");

  if (name.allow()) {
    dumpOCLProgramBinary(name.str().data(), binaryOutput, binarySize);
  }
}

static void WriteSpecConstantsDump(const STB_TranslateInputArgs *pInputArgs, QWORD hash) {
  const char *pOutputFolder = IGC::Debug::GetShaderOutputFolder();
  std::ostringstream outputstr;
  for (unsigned i = 0; i < pInputArgs->SpecConstantsSize; ++i) {
    outputstr << pInputArgs->pSpecConstantsIds[i] << ": " << pInputArgs->pSpecConstantsValues[i] << "\n";
  }
  DumpShaderFile(pOutputFolder, outputstr.str().c_str(), outputstr.str().size(), hash, "_specconst.txt");
}

bool TranslateBuildSPMD(const STB_TranslateInputArgs *pInputArgs, STB_TranslateOutputArgs *pOutputArgs,
                        TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform &IGCPlatform,
                        float profilingTimerResolution, const ShaderHash &inputShHash) {
  // This part of code is a critical-section for threads,
  // due static LLVM object which handles options.
  // Setting mutex to ensure that single thread will enter and setup this flag.
  {
    const std::lock_guard<std::mutex> lock(llvm_mutex);
    std::vector<const char *> args;
    args.push_back("igc");
    auto optionsMap = llvm::cl::getRegisteredOptions();

    // The default value (8) for max of trip count upper bound that is considered
    // in unrolling is not enough for some important compute workloads, so we set it to 16.
    // When UnrollMaxUpperBound parameter will be available to set in UnrollingPreferences
    // this code will be removed.
    llvm::StringRef unrollMaxUpperBoundFlag = "-unroll-max-upperbound=16";
    auto unrollMaxUpperBoundSwitch = optionsMap.find(unrollMaxUpperBoundFlag.trim("-=16"));
    if (unrollMaxUpperBoundSwitch != optionsMap.end()) {
      if (unrollMaxUpperBoundSwitch->getValue()->getNumOccurrences() == 0) {
        args.push_back(unrollMaxUpperBoundFlag.data());
      }
    }

    // Disable code sinking in instruction combining.
    // This is a workaround for a performance issue caused by code sinking
    // that is being done in LLVM's instcombine pass.
    // This code will be removed once sinking is removed from instcombine.
    llvm::StringRef instCombineFlag = "-instcombine-code-sinking=0";
    auto instCombineSinkingSwitch = optionsMap.find(instCombineFlag.trim("-=0"));
    if (instCombineSinkingSwitch != optionsMap.end()) {
      if (instCombineSinkingSwitch->getValue()->getNumOccurrences() == 0) {
        args.push_back(instCombineFlag.data());
      }
    }

    // With the default (250) maximum number of accesses allowed for memory
    // promotion when using MemorySSA we lack the performance for some
    // applications. Setting the number of accesses for memory promotion
    // cap to 500 solves this issue.
    llvm::StringRef licmMSSAPromotionFlag = "-licm-mssa-max-acc-promotion=500";
    auto licmMSSAPromotionSwitch = optionsMap.find(licmMSSAPromotionFlag.trim("-=500"));
    if (licmMSSAPromotionSwitch != optionsMap.end()) {
      if (licmMSSAPromotionSwitch->getValue()->getNumOccurrences() == 0) {
        args.push_back(licmMSSAPromotionFlag.data());
      }
    }

    // Avoid stack overflow in AliasAnalysis for expansive loop unrolling cases.
    llvm::StringRef aaQueryDepthFlag = "-basic-aa-max-query-depth=192";
    auto aaQueryDepthSwitch = optionsMap.find(aaQueryDepthFlag.trim("-=192"));
    if (aaQueryDepthSwitch != optionsMap.end()) {
      if (aaQueryDepthSwitch->getValue()->getNumOccurrences() == 0) {
        args.push_back(aaQueryDepthFlag.data());
      }
    }

    llvm::StringRef dsePartialOverwriteTrackingFlag = "-enable-dse-partial-overwrite-tracking=1";
    auto dsePartialOverwriteTrackingSwitch = optionsMap.find(dsePartialOverwriteTrackingFlag.trim("-=1"));
    if (dsePartialOverwriteTrackingSwitch != optionsMap.end()) {
      if (dsePartialOverwriteTrackingSwitch->getValue()->getNumOccurrences() == 0) {
        args.push_back(dsePartialOverwriteTrackingFlag.data());
      }
    }

    llvm::StringRef dseMSSAStepLimitFlag = "-dse-memoryssa-walklimit=150";
    auto dseMSSAStepLimitSwitch = optionsMap.find(dseMSSAStepLimitFlag.trim("-=150"));
    if (dseMSSAStepLimitSwitch != optionsMap.end()) {
      if (dseMSSAStepLimitSwitch->getValue()->getNumOccurrences() == 0) {
        args.push_back(dseMSSAStepLimitFlag.data());
      }
    }

    // From pass IndVarSimplify we are only interested in optimization done by -replexitval.
    // Disable other features that can have a negative impact on performance.
    std::array<llvm::StringRef, 4> indVarSimplifyFlags = {"-indvars-post-increment-ranges=0", "-disable-lftr=1",
                                                          "-indvars-widen-indvars=0", "-verify-indvars=0"};
    for (const auto indVarSimplifyFlag : indVarSimplifyFlags) {
      auto indVarSimplifySwitch = optionsMap.find(indVarSimplifyFlag.drop_front(1).split("=").first);
      if (indVarSimplifySwitch != optionsMap.end()) {
        if (indVarSimplifySwitch->getValue()->getNumOccurrences() == 0) {
          args.push_back(indVarSimplifyFlag.data());
        }
      }
    }

    if (std::size(args) > 1) {
      llvm::cl::ParseCommandLineOptions(std::size(args), &args[0]);
    }
  }

  if (IGC_IS_FLAG_ENABLED(QualityMetricsEnable)) {
    IGC::Debug::SetDebugFlag(IGC::Debug::DebugFlag::SHADER_QUALITY_METRICS, true);
  }

  MEM_USAGERESET;

  // Parse the module we want to compile
  llvm::Module *pKernelModule = nullptr;
  LLVMContextWrapper *llvmContext = new LLVMContextWrapper;
  RegisterComputeErrHandlers(*llvmContext);
  RegisterErrHandlers();

  if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
    std::string iof, of, inputf; // filenames for internal_options.txt, options.txt, and .spv/.bc
    bool isbc = false;

    const char *pOutputFolder = IGC::Debug::GetShaderOutputFolder();
    QWORD hash = inputShHash.getAsmHash();

    if (inputDataFormatTemp == TB_DATA_FORMAT_LLVM_BINARY) {
      isbc = true;
      DumpShaderFile(pOutputFolder, pInputArgs->pInput, pInputArgs->InputSize, hash, ".bc", &inputf);
    } else if (inputDataFormatTemp == TB_DATA_FORMAT_SPIR_V) {
      DumpShaderFile(pOutputFolder, pInputArgs->pInput, pInputArgs->InputSize, hash, ".spv", &inputf);
#if defined(IGC_SPIRV_TOOLS_ENABLED)
      if (IGC_IS_FLAG_ENABLED(SpvAsmDumpEnable)) {
        spv_text spirvAsm = nullptr;
        if (DisassembleSPIRV(pInputArgs->pInput, pInputArgs->InputSize, &spirvAsm) == SPV_SUCCESS) {
          DumpShaderFile(pOutputFolder, spirvAsm->str, spirvAsm->length, hash, ".spvasm");
        }
        spvTextDestroy(spirvAsm);
      }
#endif // defined(IGC_SPIRV_TOOLS_ENABLED)
    }

    DumpShaderFile(pOutputFolder, pInputArgs->pInternalOptions, pInputArgs->InternalOptionsSize, hash,
                   "_internal_options.txt", &iof);
    DumpShaderFile(pOutputFolder, pInputArgs->pOptions, pInputArgs->OptionsSize, hash, "_options.txt", &of);

    // dump cmd file that has igcstandalone command to compile this kernel.
    std::ostringstream cmdline;
    cmdline << "IGCStandalone -api ocl" << std::hex << " -device 0x" << IGCPlatform.GetProductFamily() << ".0x"
            << IGCPlatform.GetDeviceId() << ".0x" << IGCPlatform.GetRevId() << " -gmd_render 0x"
            << GFX_GET_GMD_RELEASE_VERSION_RENDER(IGCPlatform.getPlatformInfo()) << ".0x"
            << GFX_GET_GMD_REV_ID_RENDER(IGCPlatform.getPlatformInfo()) << std::dec << " -inputcs "
            << getBaseFilename(inputf);
    if (isbc) {
      cmdline << " -bitcode";
    }
    if (of.size() > 0) {
      cmdline << " -foptions " << getBaseFilename(of);
    }
    if (iof.size() > 0) {
      cmdline << " -finternal_options " << getBaseFilename(iof);
    }

    std::string keyvalues, optionstr;
    GetKeysSetExplicitly(&keyvalues, &optionstr);
    std::ostringstream outputstr;
    outputstr << "IGC keys (some dump keys not shown) and command line to compile:\n\n";
    if (!keyvalues.empty()) {
      outputstr << keyvalues << "\n\n";
    }
    outputstr << cmdline.str() << "\n";

    if (!optionstr.empty()) {
      outputstr << "\n\nOr using the following with IGC keys set via -option\n\n";
      outputstr << cmdline.str() << " -option " << optionstr << "\n";
    }
    DumpShaderFile(pOutputFolder, outputstr.str().c_str(), outputstr.str().size(), hash, "_cmd.txt");
  }

  if (!ParseInput(pKernelModule, pInputArgs, pOutputArgs->ErrorString, *llvmContext, inputDataFormatTemp,
                  IGCPlatform)) {
    return false;
  }
  CDriverInfoOCLNEO driverInfoOCL;
  IGC::CDriverInfo *driverInfo = &driverInfoOCL;

  USC::SShaderStageBTLayout zeroLayout = USC::g_cZeroShaderStageBTLayout;
  IGC::COCLBTILayout oclLayout(&zeroLayout);
  OpenCLProgramContext oclContext(oclLayout, IGCPlatform, pInputArgs, *driverInfo, llvmContext);

  SIMDMode MinDispatchMode = oclContext.platform.getMinDispatchMode();
  if (IGC_IS_FLAG_SET(ForceOCLSIMDWidth) && IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth) < numLanes(MinDispatchMode)) {
    std::string errorMsg = "SIMD size of " + std::to_string(IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth)) +
                           " has been forced when SIMD size of at least " + std::to_string(numLanes(MinDispatchMode)) +
                           " is required on this platform";
    oclContext.EmitError(errorMsg.c_str(), nullptr);
    SetOutputMessage(oclContext.GetError(), *pOutputArgs);
    return false;
  }

  if (oclContext.m_InternalOptions.Efficient64b) {
    // IGC depends on FtrEfficient64BitAddressing to determine whether 64bit
    // addressing is supported. To allow testing this feature through ocloc tests
    // and Neo ULTS, we override the SKU table here to imitate the behavior when
    // Efficient64Bit is enabled in KMD.
    auto &skuTable = const_cast<SKU_FEATURE_TABLE &>(IGCPlatform.getSkuTable());
    skuTable.FtrEfficient64BitAddressing = 1;
  }

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

  if (inputDataFormatTemp == TB_DATA_FORMAT_SPIR_V) {
    oclContext.setAsSPIRV();
  }

  if (IGC_IS_FLAG_ENABLED(EnableReadGTPinInput)) {
    // Set GTPin flags
    oclContext.gtpin_init = pInputArgs->GTPinInput;
  }

  oclContext.hash = inputShHash;
  // FIXME: pKernelModule can become a dangling pointer in case of ShaderOverride.
  oclContext.setModule(pKernelModule);
  if (oclContext.isSPIRV()) {
    deserialize(*oclContext.getModuleMetaData(), pKernelModule);
  }

  oclContext.annotater = nullptr;

  // Set default denorm.
  // Note that those values have been set to FLOAT_DENORM_FLUSH_TO_ZERO
  CompOptions *compOpt = &oclContext.getModuleMetaData()->compOpt;
  if (IGFX_GEN8_CORE <= oclContext.platform.GetPlatformFamily()) {
    compOpt->FloatDenormMode16 = FLOAT_DENORM_RETAIN;
    compOpt->FloatDenormMode32 = FLOAT_DENORM_RETAIN;
    compOpt->FloatDenormMode64 = FLOAT_DENORM_RETAIN;
  }
  if (oclContext.platform.hasBFTFDenormMode()) {
    compOpt->FloatDenormModeBFTF = FLOAT_DENORM_RETAIN;
  }

  // TODO: Again, this should not happen on each compilation

  bool doSplitModule = oclContext.m_InternalOptions.CompileOneKernelAtTime || IGC_IS_FLAG_ENABLED(CompileOneAtTime);
  // set retry manager
  bool retry = false;
  oclContext.m_retryManager->Enable(ShaderType::OPENCL_SHADER);
  do {
    llvm::TinyPtrVector<const llvm::Function *> kernelFunctions;
    if (doSplitModule) {
      for (const auto &F : pKernelModule->functions()) {
        if (F.getCallingConv() == llvm::CallingConv::SPIR_KERNEL) {
          kernelFunctions.push_back(&F);
        }
      }

      if (retry) {
        fprintf(stderr, "IGC recompiles whole module with different optimization strategy, recompiling all kernels \n");
      }
      IGC_ASSERT_EXIT_MESSAGE(kernelFunctions.empty() == false, "No kernels found!");
      fprintf(stderr, "IGC compiles kernels one by one... (%d total)\n", kernelFunctions.size());
    }

    // for Module splitting feature; if it's inactive, flow is as normal
    do {
      KernelModuleSplitter splitter(oclContext, *pKernelModule);
      if (doSplitModule) {
        const llvm::Function *pKernelFunction = kernelFunctions.back();

        fprintf(stderr, "Compiling kernel #%d: %s\n", kernelFunctions.size(), pKernelFunction->getName().data());
        kernelFunctions.pop_back();

        splitter.splitModuleForKernel(pKernelFunction);
        splitter.setSplittedModuleInOCLContext();
      }

      oclContext.getModuleMetaData()->csInfo.forcedSIMDSize |= IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth);

      try {
        if (llvm::StringRef(oclContext.getModule()->getTargetTriple()).startswith("spir")) {
          IGC::UnifyIRSPIR(&oclContext);
        } else // not SPIR
        {
          IGC::UnifyIROCL(&oclContext);
        }

        if (oclContext.HasError()) {
          if (oclContext.HasWarning()) {
            SetOutputMessage(oclContext.GetErrorAndWarning(), *pOutputArgs);
          } else {
            SetOutputMessage(oclContext.GetError(), *pOutputArgs);
          }
          return false;
        }

        // Compiler Options information available after unification.
        ModuleMetaData *modMD = oclContext.getModuleMetaData();
        if (modMD->compOpt.DenormsAreZero) {
          modMD->compOpt.FloatDenormMode16 = FLOAT_DENORM_FLUSH_TO_ZERO;
          modMD->compOpt.FloatDenormMode32 = FLOAT_DENORM_FLUSH_TO_ZERO;
        }
        if (modMD->compOpt.BFTFDenormsAreZero) {
          modMD->compOpt.FloatDenormModeBFTF = FLOAT_DENORM_FLUSH_TO_ZERO;
        }
        if (IGC_GET_FLAG_VALUE(ForceFastestSIMD)) {
          oclContext.m_retryManager->AdvanceState();
          oclContext.m_retryManager->SetFirstStateId(oclContext.m_retryManager->GetRetryId());
        }
        // Optimize the IR. This happens once for each program, not per-kernel.
        IGC::OptimizeIR(&oclContext);

        // Now, perform code generation
        IGC::CodeGen(&oclContext);
      } catch (std::bad_alloc &e) {
        (void)e; // not used now
        SetOutputMessage("IGC: Out Of Memory", *pOutputArgs);
        return false;
      } catch (std::exception &e) {
        if (pOutputArgs->ErrorString.empty()) {
          std::string message = "IGC: ";
          message += oclContext.GetErrorAndWarning();
          message += '\n';
          message += e.what();
          SetErrorMessage(message.c_str(), *pOutputArgs);
        }
        return false;
      }

      retry = (!oclContext.m_retryManager->kernelSet.empty() && oclContext.m_retryManager->AdvanceState());

      if (retry) {
        splitter.retry();
        kernelFunctions.clear();
        oclContext.clearBeforeRetry();
        oclContext.clear();

        // Create a new LLVMContext
        oclContext.initLLVMContextWrapper();

        IGC::Debug::RegisterComputeErrHandlers(*oclContext.getLLVMContext());

        if (!ParseInput(pKernelModule, pInputArgs, pOutputArgs->ErrorString, *oclContext.getLLVMContext(),
                        inputDataFormatTemp, IGCPlatform)) {
          return false;
        }
        oclContext.setModule(pKernelModule);

        // Remove annotations for kernels that do not require recompilation
        RebuildGlobalAnnotations(oclContext, pKernelModule);

        // Set default denorm since metadata was cleared.
        // Note that those values have been set to FLOAT_DENORM_FLUSH_TO_ZERO
        compOpt = &oclContext.getModuleMetaData()->compOpt;
        if (IGFX_GEN8_CORE <= oclContext.platform.GetPlatformFamily()) {
          compOpt->FloatDenormMode16 = FLOAT_DENORM_RETAIN;
          compOpt->FloatDenormMode32 = FLOAT_DENORM_RETAIN;
          compOpt->FloatDenormMode64 = FLOAT_DENORM_RETAIN;
        }
        if (oclContext.platform.hasBFTFDenormMode()) {
          compOpt->FloatDenormModeBFTF = FLOAT_DENORM_RETAIN;
        }

        for (auto it = pKernelModule->getFunctionList().begin(), ie = pKernelModule->getFunctionList().end();
             it != ie;) {
          Function *pFunc = &*(it++);
          // Only retry compilation on kernels that need it
          if (pFunc->getCallingConv() == llvm::CallingConv::SPIR_KERNEL &&
              oclContext.m_retryManager->kernelSet.find(pFunc->getName().str()) ==
                  oclContext.m_retryManager->kernelSet.end()) {
            pFunc->eraseFromParent();
            // TODO: Consider running a proper cleanup of
            // !opencl.kernels metadata entries here instead of
            // deferring 'null' entries to the "retried"
            // unification phase.
          }
        }
      }
    } while (!kernelFunctions.empty());
  } while (retry);

  oclContext.failOnSpills();

  if (oclContext.HasError()) {
    if (oclContext.HasWarning()) {
      SetOutputMessage(oclContext.GetErrorAndWarning(), *pOutputArgs);
    } else {
      SetOutputMessage(oclContext.GetError(), *pOutputArgs);
    }
    return false;
  }

  if (oclContext.HasWarning()) {
    SetOutputMessage(oclContext.GetWarning(), *pOutputArgs);
  }

  // FIXME: zebin currently only support program output itself, will add debug info
  // into it
  llvm::raw_svector_ostream llvm_os(pOutputArgs->Output);
  const bool excludeIRFromZEBinary =
      IGC_IS_FLAG_ENABLED(ExcludeIRFromZEBinary) || oclContext.getModuleMetaData()->compOpt.ExcludeIRFromZEBinary;
  const char *spv_data = nullptr;
  uint32_t spv_size = 0;
  if (inputDataFormatTemp == TB_DATA_FORMAT_SPIR_V && !excludeIRFromZEBinary) {
    spv_data = pInputArgs->pInput;
    spv_size = pInputArgs->InputSize;
  }

  // IGC metrics are empty
  auto metricData = "n\a";
  size_t metricDataSize = sizeof(metricData);

  unsigned PtrSzInBits = oclContext.getModule()->getDataLayout().getPointerSizeInBits();
  unsigned int pointerSizeInBytes = (PtrSzInBits == 64) ? 8 : 4;
  oclContext.m_programOutput.GetZEBinary(llvm_os, pointerSizeInBytes, spv_data, spv_size, metricData, metricDataSize,
                                         pInputArgs->pOptions, pInputArgs->OptionsSize);

  if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
    dumpOCLProgramBinary(oclContext, pOutputArgs->Output.data(), pOutputArgs->Output.size());

  if (const char *progbinCustomFN = IGC_GET_REGKEYSTRING(ProgbinDumpFileName))
    dumpOCLProgramBinary(progbinCustomFN, pOutputArgs->Output.data(), pOutputArgs->Output.size());

  if (IGC_IS_FLAG_ENABLED(ShaderOverride))
    overrideOCLProgramBinary(oclContext, pOutputArgs->Output);

  COMPILER_TIME_END(&oclContext, TIME_TOTAL);

  COMPILER_TIME_PER_PASS_PRINT(&oclContext, ShaderType::OPENCL_SHADER, oclContext.hash);
  COMPILER_TIME_PRINT(&oclContext, ShaderType::OPENCL_SHADER, oclContext.hash);

  COMPILER_TIME_DEL(&oclContext, m_compilerTimeStats);

  return true;
}

#if defined(IGC_VC_ENABLED)
bool TranslateBuildVC(const STB_TranslateInputArgs *pInputArgs, STB_TranslateOutputArgs *pOutputArgs,
                      TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform &IGCPlatform,
                      float profilingTimerResolution, const ShaderHash &inputShHash) {
  IGC_ASSERT(pInputArgs->pOptions &&
             (strstr(pInputArgs->pOptions, "-vc-codegen") || strstr(pInputArgs->pOptions, "-cmc")));

  // Currently, VC compiler effectively uses global variables to store
  // some configuration information. This may lead to problems
  // during multi-threaded compilations. The mutex below serializes
  // the whole compilation process.
  // This is a temporary measure till a proper re-design is done.
  const std::lock_guard<std::mutex> lock(llvm_mutex);

  std::error_code status =
      vc::translateBuild(pInputArgs, pOutputArgs, inputDataFormatTemp, IGCPlatform, profilingTimerResolution);
  return !status;
}
#endif // defined(IGC_VC_ENABLED)

bool TranslateBuild(const STB_TranslateInputArgs *pInputArgs, STB_TranslateOutputArgs *pOutputArgs,
                    TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform &IGCPlatform,
                    float profilingTimerResolution) {
  ShaderHash inputShHash;
  if (IGC_IS_FLAG_ENABLED(EnableKernelNamesBasedHash)) {
    // Create the hash based on kernel names.
    // This takes the names and concatenates them into a string
    // which is then used to calculate the hash
    StringRef inputBin(pInputArgs->pInput, pInputArgs->InputSize);
    const std::vector<std::string> &entryPoints = IGC::SPIRVParser::getEntryPointNames(inputBin);

    std::string entryPointsString = std::accumulate(entryPoints.begin(), entryPoints.end(), std::string(""));

    // 3 is the highest possible remainder of division by 4. Resizing the string by +3 ensures
    // that all characters in the string are contained in the memory read by ShaderHashOCL().
    // This is just easier to do than resizing the string to a length that's divisible by 4.
    size_t entryPointsStringSize = entryPointsString.length() + 3;
    entryPointsString.resize(entryPointsStringSize);

    inputShHash = ShaderHashOCL(reinterpret_cast<const UINT *>(&entryPointsString[0]), entryPointsStringSize / 4);
  } else
    inputShHash = ShaderHashOCL(reinterpret_cast<const UINT *>(pInputArgs->pInput), pInputArgs->InputSize / 4);

  // set g_CurrentShaderHash in igc_regkeys.cpp
  SetCurrentDebugHash(inputShHash);
  // on wrong spec constants, vc::translateBuild may fail
  // so lets dump those early
  if (pInputArgs->SpecConstantsSize > 0 && IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
    WriteSpecConstantsDump(pInputArgs, inputShHash.getAsmHash());
  }

#if defined(IGC_VC_ENABLED)
  // if VC option was specified, go to VC compilation directly.
  if (pInputArgs->pOptions && (strstr(pInputArgs->pOptions, "-vc-codegen") || strstr(pInputArgs->pOptions, "-cmc"))) {
    return TranslateBuildVC(pInputArgs, pOutputArgs, inputDataFormatTemp, IGCPlatform, profilingTimerResolution,
                            inputShHash);
  }
#endif // defined(IGC_VC_ENABLED)

  if (inputDataFormatTemp != TB_DATA_FORMAT_SPIR_V) {
    return TranslateBuildSPMD(pInputArgs, pOutputArgs, inputDataFormatTemp, IGCPlatform, profilingTimerResolution,
                              inputShHash);
  }

  // Recognize if SPIR-V module contains SPMD,ESIMD or SPMD+ESIMD code and compile it.
  std::string errorMessage;
  bool ret = VLD::TranslateBuildSPMDAndESIMD(pInputArgs, pOutputArgs, inputDataFormatTemp, IGCPlatform,
                                             profilingTimerResolution, inputShHash, errorMessage);
  if (!ret && !errorMessage.empty()) {
    SetErrorMessage(errorMessage, *pOutputArgs);
  }
  return ret;
}

bool CIGCTranslationBlock::Initialize(const STB_CreateArgs *pCreateArgs) {
  const SGlobalData *pCreateArgsGlobalData = static_cast<const SGlobalData *>(pCreateArgs->pCreateData);

  // IGC maintains its own WA table - ignore the version in the global arguments.
  m_Platform = *pCreateArgsGlobalData->pPlatform;
  m_SkuTable = *pCreateArgsGlobalData->pSkuTable;
  m_SysInfo = *pCreateArgsGlobalData->pSysInfo;

  m_DataFormatInput = pCreateArgs->TranslationCode.Type.Input;
  m_DataFormatOutput = pCreateArgs->TranslationCode.Type.Output;

  m_ProfilingTimerResolution = pCreateArgsGlobalData->ProfilingTimerResolution;

  bool validTBChain = false;

  validTBChain |= (m_DataFormatInput == TB_DATA_FORMAT_ELF) && (m_DataFormatOutput == TB_DATA_FORMAT_LLVM_BINARY);

  validTBChain |= (m_DataFormatInput == TB_DATA_FORMAT_LLVM_TEXT) && IsDeviceBinaryFormat(m_DataFormatOutput);

  validTBChain |= (m_DataFormatInput == TB_DATA_FORMAT_LLVM_BINARY) && IsDeviceBinaryFormat(m_DataFormatOutput);

  validTBChain |= (m_DataFormatInput == TB_DATA_FORMAT_SPIR_V) && IsDeviceBinaryFormat(m_DataFormatOutput);

  IGC_ASSERT_MESSAGE(validTBChain, "Invalid TB Chain");

  return validTBChain;
}

static constexpr STB_TranslationCode g_cICBETranslationCodes[] = {
    // clang-format off
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
    // clang-format on
};

TRANSLATION_BLOCK_API void Register(STB_RegisterArgs *pRegisterArgs) {
  pRegisterArgs->Version = TC::STB_VERSION;
  pRegisterArgs->NumTranslationCodes = std::size(g_cICBETranslationCodes);

  if (pRegisterArgs->pTranslationCodes) {
    iSTD::MemCopy<sizeof(g_cICBETranslationCodes)>(pRegisterArgs->pTranslationCodes, g_cICBETranslationCodes);
  }
}

TRANSLATION_BLOCK_API CTranslationBlock *Create(STB_CreateArgs *pCreateArgs) {
  CIGCTranslationBlock *pIGCTranslationBlock = nullptr;

  CIGCTranslationBlock::Create(pCreateArgs, pIGCTranslationBlock);

  return pIGCTranslationBlock;
}

TRANSLATION_BLOCK_API void Delete(CTranslationBlock *pTranslationBlock) {
  CIGCTranslationBlock *pIGCTranslationBlock = static_cast<CIGCTranslationBlock *>(pTranslationBlock);

  CIGCTranslationBlock::Delete(pIGCTranslationBlock);
}

} // namespace TC
