/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VLD.hpp"
#include "Probe/Assertion.h"
#include "VLD_SPIRVSplitter.hpp"
#include "ocl_igc_interface/impl/igc_ocl_translation_ctx_impl.h"
#include "spirv/unified1/spirv.hpp"
#include <ZEInfoYAML.hpp>

#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/YAMLTraits.h>

#include <algorithm>
#if (defined(__GNUC__) && __GNUC__ >= 9) || (defined(_MSC_VER) && (_MSVC_LANG >= 201703L))
// Temporary WA for VC issue.
#include <filesystem>
#include <fstream>
#endif

namespace TC {
// Declarations for utility functions declared in other libraries that will be linked.
void DumpShaderFile(const std::string &dstDir, const char *pBuffer, const UINT bufferSize, const QWORD hash,
                    const std::string &ext, std::string *fileName = nullptr);
spv_result_t DisassembleSPIRV(const char *pBuffer, UINT bufferSize, spv_text *outSpirvAsm);
} // namespace TC

namespace {
static const std::string ERROR_VLD = "VLD: Failed to compile SPIR-V with following error: \n";

llvm::Expected<object::ELF64LEFile> getElfFile(llvm::StringRef ZeBinary) {
  MemoryBufferRef inputRef(ZeBinary, "zebin");
  auto ElfOrErr = object::ObjectFile::createELFObjectFile(inputRef);
  if (!ElfOrErr)
    return ElfOrErr.takeError();
#if LLVM_VERSION_MAJOR < 12
  auto ElfFilePointer = cast<object::ELF64LEObjectFile>(*ElfOrErr.get()).getELFFile();
  IGC_ASSERT(ElfFilePointer);
  auto ElfFile = *ElfFilePointer;
#else
  auto ElfFile = cast<object::ELF64LEObjectFile>(*ElfOrErr.get()).getELFFile();
#endif

  return ElfFile;
}

llvm::Expected<std::vector<llvm::StringRef>> getZeBinSectionsData(llvm::StringRef ZeBinary,
                                                                  zebin::SHT_ZEBIN SectionType) {
  using namespace llvm;
  std::vector<StringRef> OutVec;

  auto ElfFileOrErr = getElfFile(ZeBinary);
  if (!ElfFileOrErr)
    return ElfFileOrErr.takeError();
  auto ElfFile = ElfFileOrErr.get();

  auto ElfSections = ElfFile.sections();
  if (!ElfSections)
    return ElfSections.takeError();

  for (auto &Sect : *ElfSections) {
    if (Sect.sh_type == SectionType) {
#if LLVM_VERSION_MAJOR < 12
      auto SectionDataOrErr = ElfFile.getSectionContents(&Sect);
#else
      auto SectionDataOrErr = ElfFile.getSectionContents(Sect);
#endif
      if (!SectionDataOrErr)
        return SectionDataOrErr.takeError();
      StringRef Data(reinterpret_cast<const char *>((*SectionDataOrErr).data()), (size_t)Sect.sh_size);
      OutVec.push_back(Data);
    }
  }

  return OutVec;
}

// Extracts .visaasm sections from input zeBinary ELF.
// Returns a vector of strings - one for each section.
llvm::Expected<std::vector<llvm::StringRef>> GetVISAAsmFromZEBinary(llvm::StringRef ZeBinary) {
  return getZeBinSectionsData(ZeBinary, zebin::SHT_ZEBIN_VISAASM);
}
llvm::Expected<zebin::zeInfoContainer> GetZeInfoFromZeBinary(llvm::StringRef ZeBinary) {
  using namespace llvm;
  auto ZeInfoYAMLOrErr = getZeBinSectionsData(ZeBinary, zebin::SHT_ZEBIN_ZEINFO);
  if (!ZeInfoYAMLOrErr) {
    return ZeInfoYAMLOrErr.takeError();
  }

  zebin::zeInfoContainer ZeInfo;

  if (ZeInfoYAMLOrErr->size() != 1) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "ZEBinary expected to contain one .ze_info section!");
  }

  // ZeBinary strings are not null-terminated, so copy it to std::string.
  std::string ZeInfoYAML = (*(ZeInfoYAMLOrErr->begin())).str();
  llvm::yaml::Input yin(ZeInfoYAML.c_str());
  yin >> ZeInfo;
  if (yin.error()) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(), "Failed to parse .ze_info section!");
  }

  return ZeInfo;
}

llvm::Expected<bool> ZeBinaryContainsSection(llvm::StringRef ZeBinary, zebin::SHT_ZEBIN SectionType) {
  auto ElfFileOrErr = getElfFile(ZeBinary);
  if (!ElfFileOrErr)
    return ElfFileOrErr.takeError();
  auto ElfSections = (*ElfFileOrErr).sections();
  if (!ElfSections)
    return ElfSections.takeError();

  for (auto &Sect : *ElfSections) {
    if (Sect.sh_type == SectionType) {
      return true;
    }
  }
  return false;
}

llvm::Expected<int> GetSIMDSizeFromZeInfo(const zebin::zeInfoContainer &ZeInfo) {
  std::vector<int> SimdSizes;
  for (auto &Kernel : ZeInfo.kernels) {
    SimdSizes.push_back(Kernel.execution_env.simd_size);
  }
  for (auto &Func : ZeInfo.functions) {
    SimdSizes.push_back(Func.execution_env.simd_size);
  }
  if (SimdSizes.size() == 0) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "Couldn't find any compiled kernel or function SIMD size!");
  }
  if (!std::all_of(SimdSizes.begin(), SimdSizes.end(),
                   [&](auto &SimdSize) { return SimdSize == *SimdSizes.begin(); })) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(), "SIMD sizes in the module are not uniform!");
  }

  return *SimdSizes.begin();
}

void DumpSPIRVFile(const char *programData, size_t programSizeInBytes, const ShaderHash &inputShHash, std::string ext) {
  const char *pOutputFolder = IGC::Debug::GetShaderOutputFolder();

  TC::DumpShaderFile(pOutputFolder, programData, programSizeInBytes, inputShHash.getAsmHash(), ext);
  spv_text spirvAsm = nullptr;
  if (TC::DisassembleSPIRV(programData, programSizeInBytes, &spirvAsm) == SPV_SUCCESS) {
    TC::DumpShaderFile(pOutputFolder, spirvAsm->str, spirvAsm->length, inputShHash.getAsmHash(), ext + "asm");
  }
  spvTextDestroy(spirvAsm);
}

// Given a vector of SPVTranslationPairs, the function moves the pair with
// entry module to the last.
// If entry module has subgroup size forced, it is returned in SimdSize param.
llvm::Expected<std::vector<IGC::VLD::SPVTranslationPair>>
MoveEntryPointModuleToTheEnd(llvm::ArrayRef<IGC::VLD::SPVTranslationPair> InputModules, uint32_t &SimdSize) {
  using namespace IGC::VLD;
  SpvSplitter splitter;
  SPVTranslationPair EntryPointPair;
  bool HasEntryPointModule = false;
  std::vector<SPVTranslationPair> RetPairs;
  for (auto &InputArgsPair : InputModules) {
    if (InputArgsPair.first.HasEntryPoints) {
      EntryPointPair = InputArgsPair;
      if (EntryPointPair.first.SpirvType == SPIRVTypeEnum::SPIRV_SPMD) {
        SimdSize = splitter.GetForcedSubgroupSize();
      }
      if (HasEntryPointModule) {
        return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                       "The list of SPIR-V modules contains more than one module with an "
                                       "entry point!");
      }
      HasEntryPointModule = true;
    } else {
      RetPairs.push_back(InputArgsPair);
    }
  }

  bool IsLibraryCompilation = false;
  auto OptionsOfLastModule = RetPairs[RetPairs.size() - 1].second.pOptions;
  if (OptionsOfLastModule && strstr(OptionsOfLastModule, "-library-compilation")) {
    IsLibraryCompilation = true;
  }

  if (!HasEntryPointModule && !IsLibraryCompilation) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(), "The list of SPIR-V modules did not contain "
                                                                   "any module with an entry point!");
  }

  if (HasEntryPointModule) {
    RetPairs.push_back(EntryPointPair);
  }

  return RetPairs;
}

// Returns a list of functions that must be called directly:
// functions that are exported in VC modules and imported in SPMD and
// vice-versa.
std::vector<const char *> GetDirectCallFunctions(llvm::ArrayRef<IGC::VLD::SPVTranslationPair> InputModules) {
  set<StringRef> exportedFunctionsSPMD;
  set<StringRef> exportedFunctionsESIMD;
  set<StringRef> importedFunctionsSPMD;
  set<StringRef> importedFunctionsESIMD;

  auto insertToVec = [](const auto &InputVec, auto &OutSPMD, auto &OutESIMD, auto SpirvType) {
    for (auto &FuncName : InputVec) {
      if (SpirvType == IGC::VLD::SPIRVTypeEnum::SPIRV_SPMD) {
        OutSPMD.insert(FuncName);
      } else {
        OutESIMD.insert(FuncName);
      }
    }
  };

  for (auto &InputArgsPair : InputModules) {
    IGC_ASSERT(InputArgsPair.first.SpirvType == IGC::VLD::SPIRVTypeEnum::SPIRV_SPMD ||
               InputArgsPair.first.SpirvType == IGC::VLD::SPIRVTypeEnum::SPIRV_ESIMD);
    insertToVec(InputArgsPair.first.ExportedFunctions, exportedFunctionsSPMD, exportedFunctionsESIMD,
                InputArgsPair.first.SpirvType);
    insertToVec(InputArgsPair.first.ImportedFunctions, importedFunctionsSPMD, importedFunctionsESIMD,
                InputArgsPair.first.SpirvType);
  }

  std::vector<StringRef> DirectCallFunctions;
  set_intersection(exportedFunctionsESIMD.begin(), exportedFunctionsESIMD.end(), importedFunctionsSPMD.begin(),
                   importedFunctionsSPMD.end(), back_inserter(DirectCallFunctions));
  set_intersection(exportedFunctionsSPMD.begin(), exportedFunctionsSPMD.end(), importedFunctionsESIMD.begin(),
                   importedFunctionsESIMD.end(), back_inserter(DirectCallFunctions));
  // Special case: when SPMD+ESIMD module is provided, the invoke_simd callee will not
  // be in the imported SPMD function list, as it is just Exported in the single module.
  // It will be present in both SPMD and ESIMD exports.
  set_intersection(exportedFunctionsSPMD.begin(), exportedFunctionsSPMD.end(), exportedFunctionsESIMD.begin(),
                   exportedFunctionsESIMD.end(), back_inserter(DirectCallFunctions));

  std::vector<const char *> OutVec;
  std::transform(DirectCallFunctions.begin(), DirectCallFunctions.end(), back_inserter(OutVec),
                 [](auto &el) { return el.data(); });
  return OutVec;
}

// Helper function to create a SPVTranslationPair with given TranslateInputArgs
// as base and binary SPV program. The caller needs to be the owner of the
// program and options, as TranslateInputArgs structure keeps a pointer to them.
llvm::Expected<IGC::VLD::SPVTranslationPair> MakeSPVTranslationPair(const TC::STB_TranslateInputArgs *pInputArgs,
                                                                    const IGC::VLD::ProgramStreamType &program,
                                                                    const std::string &options) {

  TC::STB_TranslateInputArgs NewArgs = *pInputArgs;
  NewArgs.pInput = (char *)(program.data());
  NewArgs.InputSize = program.size() * sizeof(*program.begin());
  NewArgs.pOptions = options.data();
  NewArgs.OptionsSize = options.size();
  auto VLDMetadata = IGC::VLD::GetVLDMetadata(NewArgs.pInput, NewArgs.InputSize);
  if (!VLDMetadata) {
    return VLDMetadata.takeError();
  }
  return IGC::VLD::SPVTranslationPair(*VLDMetadata, NewArgs);
}

} // namespace

namespace IGC {
namespace VLD {
using namespace TC;

// Translates ESIMD and SPMD code in the module.
// 3 cases are handled:
// 1. only SPMD code is present
// 2. only ESIMD code is present
// 3. ESIMD code is invoked from SPMD code
//
// The general flow is:
// 1. Split input SPIR-V module into SPMD and ESIMD parts
// 2. Invoke SPMD and ESIMD backends with appropriate SPIR-V modules
// 3. If SPMD code invokes ESIMD code, extract .visaasm from the each output zeBinary
// TODO: 4. Link .visaasm files via vISA interfaces
//
// The function signature corresponds to TC::TranslateBuild interface, so that
// it is easy to pass same arguments to SPMD and VC backends.
//
// Assumptions:
// 1. ZEBinary output format is used in SPMD+ESIMD case.
bool TranslateBuildSPMDAndESIMD(const TC::STB_TranslateInputArgs *pInputArgs, TC::STB_TranslateOutputArgs *pOutputArgs,
                                TC::TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform &IGCPlatform,
                                float profilingTimerResolution, const ShaderHash &inputShHash,
                                std::string &errorMessage) {

  IGC_ASSERT(inputDataFormatTemp == TB_DATA_FORMAT_SPIR_V);

#if defined(IGC_VC_ENABLED)
  // Check if -vc-codegen option is present - if so, use VC backend directly
  bool hasVCCodegenOpt = false;
  if (pInputArgs->pOptions) {
    std::string options(pInputArgs->pOptions, pInputArgs->OptionsSize);
    hasVCCodegenOpt = (options.find("-vc-codegen") != std::string::npos);
  }

  if (hasVCCodegenOpt) {
    return TranslateBuildVC(pInputArgs, pOutputArgs, inputDataFormatTemp, IGCPlatform, profilingTimerResolution,
                            inputShHash);
  }
#endif

  // Split ESIMD and SPMD code.
  auto spmd_esimd_programs_or_err = VLD::SplitSPMDAndESIMD(pInputArgs->pInput, pInputArgs->InputSize);

  if (!spmd_esimd_programs_or_err) {
    // The error must be handled. Doing nothing for now.
    handleAllErrors(spmd_esimd_programs_or_err.takeError(), [](const llvm::ErrorInfoBase &EI) {});
    // Workaround: try to compile on SPMD path if splitting failed.
    // This is because not all VC opcodes are merged to SPIR-V Tools.
    return TranslateBuildSPMD(pInputArgs, pOutputArgs, inputDataFormatTemp, IGCPlatform, profilingTimerResolution,
                              inputShHash);

    // TODO: uncomment once above workaround is removed.
    // Caller releases the error string, so we need to make a copy of the error message here.
    // TODO: pOutputArgs contains field for error string so we can copy it there.
    // Not done now, as it would require copy-paste code that is avaiable in dllinterfacecompute. Needs to be
    // refactored. errorMessage = llvm::toString(spmd_esimd_programs_or_err.takeError()); return false;
  }

  std::string newOptions{pInputArgs->pOptions ? pInputArgs->pOptions : ""};
  std::string esimdOptions{newOptions};
  esimdOptions += " -vc-codegen";

  auto [spmdProg, esimdProg] = spmd_esimd_programs_or_err.get();

  IGC_ASSERT(!spmdProg.empty() || !esimdProg.empty());
  if (spmdProg.empty()) {
#if defined(IGC_VC_ENABLED)
    // Only ESIMD code detected.
    STB_TranslateInputArgs newArgs = *pInputArgs;
    newArgs.pOptions = esimdOptions.data();
    newArgs.OptionsSize = esimdOptions.size();
    return TranslateBuildVC(&newArgs, pOutputArgs, inputDataFormatTemp, IGCPlatform, profilingTimerResolution,
                            inputShHash);
#else  // defined(IGC_VC_ENABLED)
    errorMessage = "ESIMD code detected, but VC not enabled in this build.";
    return false;
#endif // defined(IGC_VC_ENABLED)
  } else if (esimdProg.empty()) {
    // Only SPMD code detected.
    return TranslateBuildSPMD(pInputArgs, pOutputArgs, inputDataFormatTemp, IGCPlatform, profilingTimerResolution,
                              inputShHash);
  }

  // SPMD+ESIMD code detected.

  if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
    DumpSPIRVFile(pInputArgs->pInput, pInputArgs->InputSize, inputShHash, ".spmd_and_esimd.spv");
    DumpSPIRVFile((const char *)spmdProg.data(), spmdProg.size() * sizeof(uint32_t), inputShHash, ".spmd_split.spv");
    DumpSPIRVFile((const char *)esimdProg.data(), esimdProg.size() * sizeof(uint32_t), inputShHash, ".esimd_split.spv");
  }

  auto SpmdTPOrErr = MakeSPVTranslationPair(pInputArgs, spmdProg, newOptions);
  auto EsimdTPOrErr = MakeSPVTranslationPair(pInputArgs, esimdProg, esimdOptions);
  if (!SpmdTPOrErr) {
    errorMessage = ERROR_VLD + llvm::toString(SpmdTPOrErr.takeError());
    return false;
  }
  if (!EsimdTPOrErr) {
    errorMessage = ERROR_VLD + llvm::toString(EsimdTPOrErr.takeError());
    return false;
  }

  std::array<SPVTranslationPair, 2> SpvArr{*EsimdTPOrErr, *SpmdTPOrErr};

  return TranslateBuildSPMDAndESIMD(SpvArr, pOutputArgs, inputDataFormatTemp, IGCPlatform, profilingTimerResolution,
                                    inputShHash, errorMessage);
}

bool TranslateBuildSPMDAndESIMD(llvm::ArrayRef<SPVTranslationPair> InputModules,
                                TC::STB_TranslateOutputArgs *pOutputArgs, TC::TB_DATA_FORMAT inputDataFormatTemp,
                                const IGC::CPlatform &IGCPlatform, float profilingTimerResolution,
                                const ShaderHash &inputShHash, std::string &errorMessage) {
#if defined(IGC_VC_ENABLED)

  std::vector<std::string> OwnerStrings;
  std::vector<const char *> VisaCStrings;
  uint32_t SimdSize = 0;

  std::vector<SPVTranslationPair> SplitInputModules;
  std::vector<ProgramStreamType> OwnerSplitBinaries;

  auto SetVLDErrorMessage = [&errorMessage](llvm::Error Err) {
    errorMessage = ERROR_VLD + llvm::toString(std::move(Err));
  };

  // Split any SPMD+ESIMD modules.
  for (auto &InputModule : InputModules) {
    if (InputModule.first.SpirvType == VLD::SPIRVTypeEnum::SPIRV_SPMD_AND_ESIMD) {
      auto SPMPAndESIMDOrErr = VLD::SplitSPMDAndESIMD(InputModule.second.pInput, InputModule.second.InputSize);
      if (!SPMPAndESIMDOrErr) {
        SetVLDErrorMessage(SPMPAndESIMDOrErr.takeError());
        return false;
      }
      OwnerSplitBinaries.push_back(std::move(SPMPAndESIMDOrErr.get().first));

      std::string newOptions{InputModule.second.pOptions ? InputModule.second.pOptions : ""};
      std::string esimdOptions{newOptions};
      esimdOptions += " -vc-codegen";

      OwnerStrings.push_back(std::move(newOptions));

      auto SpmdTPOrErr = MakeSPVTranslationPair(&InputModule.second, OwnerSplitBinaries.back(), OwnerStrings.back());
      if (!SpmdTPOrErr) {
        SetVLDErrorMessage(SpmdTPOrErr.takeError());
      }

      OwnerSplitBinaries.push_back(std::move(SPMPAndESIMDOrErr.get().second));
      OwnerStrings.push_back(std::move(esimdOptions));
      auto EsimdTPOrErr = MakeSPVTranslationPair(&InputModule.second, OwnerSplitBinaries.back(), OwnerStrings.back());
      if (!EsimdTPOrErr) {
        SetVLDErrorMessage(EsimdTPOrErr.takeError());
      }

      SplitInputModules.push_back(*EsimdTPOrErr);
      SplitInputModules.push_back(*SpmdTPOrErr);

    } else {
      SplitInputModules.push_back(InputModule);
    }
  }

  auto DirectCallFunctions = GetDirectCallFunctions(SplitInputModules);

  // Module with entry points should be compiled as the last one.
  // We currently support the use-case, where only one of the SPIR-V modules contain entry points.
  auto NewInputModulesOrErr = MoveEntryPointModuleToTheEnd(SplitInputModules, SimdSize);
  if (!NewInputModulesOrErr) {
    SetVLDErrorMessage(NewInputModulesOrErr.takeError());
    return false;
  }

  auto &NewInputModules = NewInputModulesOrErr.get();

  for (auto &InputArgsPair : NewInputModules) {
    bool IsLast = &InputArgsPair == &NewInputModules.back();

    auto &InputArgs = InputArgsPair.second;

    TC::STB_TranslateOutputArgs NewOutputArgs;
    CIF::SafeZeroOut(NewOutputArgs);
    auto outputData = std::unique_ptr<char[]>(NewOutputArgs.pOutput);
    auto errorString = std::unique_ptr<char[]>(NewOutputArgs.pErrorString);
    auto debugData = std::unique_ptr<char[]>(NewOutputArgs.pDebugData);

    STB_TranslateInputArgs NewInputArgs = InputArgs;
    std::string NewInternalOptions{InputArgs.pInternalOptions ? InputArgs.pInternalOptions : ""};
    std::string NewOptions{InputArgs.pOptions ? InputArgs.pOptions : ""};
    std::string NewEsimdOptions{std::move(NewOptions)};

    switch (InputArgsPair.first.SpirvType) {
    case VLD::SPIRVTypeEnum::SPIRV_SPMD:
      if (!IsLast)
        NewInternalOptions += " -ze-emit-visa-only -ze-emit-zebin-visa-sections";
      break;
    case VLD::SPIRVTypeEnum::SPIRV_ESIMD:
      if (!IsLast)
        NewInternalOptions += " -emit-visa-only -emit-zebin-visa-sections";
      NewInternalOptions += " -binary-format=ze";
      if (SimdSize != 0) {
        NewInternalOptions += " -vc-interop-subgroup-size ";
        NewInternalOptions += std::to_string(SimdSize);
      }
      break;
    case VLD::SPIRVTypeEnum::SPIRV_SPMD_AND_ESIMD:
      IGC_ASSERT("SPMD+ESIMD module assumed to be split before this code.");
      errorMessage = "VisaLinkerDriver logic error!";
      return false;
    default:
      errorMessage = "Unsupported SPIR-V flavour detected!";
      return false;
    }

    if (InputArgsPair.second.pVISAAsmToLinkArray) {
      for (uint32_t i = 0; i < InputArgsPair.second.NumVISAAsmsToLink; ++i) {
        VisaCStrings.push_back(InputArgsPair.second.pVISAAsmToLinkArray[i]);
      }
    }

    NewInputArgs.pVISAAsmToLinkArray = (!VisaCStrings.empty()) ? VisaCStrings.data() : nullptr;
    NewInputArgs.NumVISAAsmsToLink = VisaCStrings.size();
    NewInputArgs.pDirectCallFunctions = DirectCallFunctions.data();
    NewInputArgs.NumDirectCallFunctions = DirectCallFunctions.size();
    NewInputArgs.pInternalOptions = NewInternalOptions.data();
    NewInputArgs.InternalOptionsSize = NewInternalOptions.size();
    bool success = false;
    if (InputArgsPair.first.SpirvType == VLD::SPIRVTypeEnum::SPIRV_SPMD) {
      success = TranslateBuildSPMD(&NewInputArgs, &NewOutputArgs, inputDataFormatTemp, IGCPlatform,
                                   profilingTimerResolution, inputShHash);
    } else if (InputArgsPair.first.SpirvType == VLD::SPIRVTypeEnum::SPIRV_ESIMD) {
      NewEsimdOptions += " -vc-codegen";
      NewInputArgs.pOptions = NewEsimdOptions.data();
      NewInputArgs.OptionsSize = NewEsimdOptions.size();
      success = TranslateBuildVC(&NewInputArgs, &NewOutputArgs, inputDataFormatTemp, IGCPlatform,
                                 profilingTimerResolution, inputShHash);
    } else {
      IGC_ASSERT("SPMD+ESIMD module assumed to be split before this code.");
      errorMessage = "VisaLinkerDriver logic error!";
      return false;
    }

    if (!success) {
      if (errorMessage.empty() && NewOutputArgs.pErrorString) {
        errorMessage = "VLD: Failed to compile SPIR-V with following error: \n";
        errorMessage += NewOutputArgs.pErrorString;
      }
      return false;
    }

    // If this is the last SPIR-V to compile, stop here. The rest of the code
    // handles extracting information for further compilations.
    if (IsLast) {
      *pOutputArgs = NewOutputArgs;
      break;
    }

    llvm::StringRef ZeBinary(NewOutputArgs.pOutput, NewOutputArgs.OutputSize);

    // Check if the output has ZEINFO section. If not, it can mean that the
    // module didn't contain any exported functions.
    auto HasZeInfoSectionOrErr = ZeBinaryContainsSection(ZeBinary, zebin::SHT_ZEBIN_ZEINFO);
    if (!HasZeInfoSectionOrErr) {
      SetVLDErrorMessage(HasZeInfoSectionOrErr.takeError());
      return false;
    }
    if (!*HasZeInfoSectionOrErr) {
      continue;
    }

    auto ZeInfoOrErr = GetZeInfoFromZeBinary(ZeBinary);
    if (!ZeInfoOrErr) {
      SetVLDErrorMessage(ZeInfoOrErr.takeError());
      return false;
    }

    // Set SimdSize based on first SPMD module, as ESIMD always returns 1.
    if (InputArgsPair.first.SpirvType == SPIRVTypeEnum::SPIRV_SPMD) {

      auto SimdSizeOrErr = GetSIMDSizeFromZeInfo(*ZeInfoOrErr);
      if (!SimdSizeOrErr) {
        SetVLDErrorMessage(SimdSizeOrErr.takeError());
        return false;
      }
      if (SimdSize != 0 && SimdSize != *SimdSizeOrErr) {
        errorMessage = ERROR_VLD + "Compilation of SPIR-V modules resulted in different SIMD sizes!";
        return false;
      }
      if (SimdSize == 0)
        SimdSize = *SimdSizeOrErr;
    }

    for (auto &F : ZeInfoOrErr->functions) {
      OwnerStrings.push_back(std::move(F.name));
      DirectCallFunctions.push_back(OwnerStrings.back().c_str());
    }

    auto VISAAsm = GetVISAAsmFromZEBinary(ZeBinary);

    if (!VISAAsm) {
      SetVLDErrorMessage(VISAAsm.takeError());
      return false;
    }

    if (VISAAsm->empty()) {
      errorMessage = ERROR_VLD + "ZeBinary did not contain any .visaasm sections!";
      return false;
    }

    // ZeBinary contains non-null terminated strings, add the null via
    // std::string ownership.
    for (auto &s : *VISAAsm) {
      OwnerStrings.push_back(s.str());
      VisaCStrings.push_back(OwnerStrings.back().c_str());
    }
  }

  return true;

#else  // defined(IGC_VC_ENABLED)
  errorMessage = "Could not compile ESIMD part of SPIR-V module, as VC is not included in this build.";
  return false;
#endif // defined(IGC_VC_ENABLED)
}
} // namespace VLD
} // namespace IGC
