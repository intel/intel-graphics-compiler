/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VLD.hpp"
#include "VLD_SPIRVSplitter.hpp"
#include "Probe/Assertion.h"
#include "spirv/unified1/spirv.hpp"
#include "ocl_igc_interface/impl/igc_ocl_translation_ctx_impl.h"
#include "spirv/unified1/spirv.hpp"

#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Error.h>

#include <algorithm>
#if (defined(__GNUC__) && __GNUC__ >= 9) || (defined(_MSC_VER) && (_MSVC_LANG >= 201703L))
// Temporary WA for VC issue.
#include <filesystem>
#include <fstream>
#endif

namespace TC {
    // Declarations for utility functions declared in other libraries that will be linked.
    void DumpShaderFile(const std::string &dstDir, const char *pBuffer,
        const UINT bufferSize, const QWORD hash,
        const std::string &ext, std::string *fileName = nullptr);
    spv_result_t DisassembleSPIRV(const char* pBuffer, UINT bufferSize,
        spv_text* outSpirvAsm);
}

namespace {

// Extracts .visaasm sections from input zeBinary ELF.
// Returns a vector of strings - one for each section.
llvm::Expected<std::vector<std::string>>
GetVISAAsmFromZEBinary(const char *zeBinary, size_t zeBinarySize) {
  using namespace llvm;

  std::vector<std::string> OutVISAAsmStrings;

  StringRef zeBinaryData(zeBinary, zeBinarySize);
  MemoryBufferRef inputRef(zeBinaryData, "zebin");
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
  auto ElfSections = llvm::cantFail(ElfFile.sections());
  for (auto &sect : ElfSections) {
    if (sect.sh_type == zebin::SHT_ZEBIN_VISAASM) {
#if LLVM_VERSION_MAJOR < 12
      auto SectionDataOrErr = ElfFile.getSectionContents(&sect);
#else
      auto SectionDataOrErr = ElfFile.getSectionContents(sect);
#endif
      if (!SectionDataOrErr)
        return SectionDataOrErr.takeError();
      StringRef Data(reinterpret_cast<const char *>((*SectionDataOrErr).data()),
                     (size_t)sect.sh_size);
      OutVISAAsmStrings.push_back(Data.str());
    }
  }

  return OutVISAAsmStrings;
}


void DumpSPIRVFile(const char *programData, size_t programSizeInBytes,
                   const ShaderHash &inputShHash, std::string ext) {
  const char *pOutputFolder = IGC::Debug::GetShaderOutputFolder();

  TC::DumpShaderFile(pOutputFolder, programData, programSizeInBytes,
                     inputShHash.getAsmHash(), ext);
  spv_text spirvAsm = nullptr;
  if (TC::DisassembleSPIRV(programData, programSizeInBytes, &spirvAsm) ==
      SPV_SUCCESS) {
    TC::DumpShaderFile(pOutputFolder, spirvAsm->str, spirvAsm->length,
                       inputShHash.getAsmHash(), ext + "asm");
  }
  spvTextDestroy(spirvAsm);
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
// TODO: error out if patch token output format is used.
bool TranslateBuildSPMDAndESIMD(const TC::STB_TranslateInputArgs *pInputArgs,
                                  TC::STB_TranslateOutputArgs *pOutputArgs,
                                  TC::TB_DATA_FORMAT inputDataFormatTemp,
                                  const IGC::CPlatform &IGCPlatform,
                                  float profilingTimerResolution,
                                  const ShaderHash &inputShHash,
                                  std::string& errorMessage) {

  IGC_ASSERT(inputDataFormatTemp == TB_DATA_FORMAT_SPIR_V);

    // Split ESIMD and SPMD code.
  auto spmd_esimd_programs_or_err = VLD::SplitSPMDAndESIMD(
      pInputArgs->pInput, pInputArgs->InputSize);

  if (!spmd_esimd_programs_or_err) {
      // The error must be handled. Doing nothing for now.
      handleAllErrors(spmd_esimd_programs_or_err.takeError(),
                      [](const llvm::ErrorInfoBase &EI) {});
      // Workaround: try to compile on SPMD path if splitting failed.
      // This is because not all VC opcodes are merged to SPIR-V Tools.
      return TranslateBuildSPMD(pInputArgs, pOutputArgs, inputDataFormatTemp,
          IGCPlatform, profilingTimerResolution,
          inputShHash);

      // TODO: uncomment once above workaround is removed.
      // Caller releases the error string, so we need to make a copy of the error message here.
      // TODO: pOutputArgs contains field for error string so we can copy it there.
      // Not done now, as it would require copy-paste code that is avaiable in dllinterfacecompute. Needs to be refactored.
      // errorMessage = llvm::toString(spmd_esimd_programs_or_err.takeError());
      // return false;
  }

  std::string newOptions{pInputArgs->pOptions ? pInputArgs->pOptions : ""};
  std::string esimdOptions{ newOptions };
  esimdOptions += " -vc-codegen";

  auto [spmdProg, esimdProg] = spmd_esimd_programs_or_err.get();

  IGC_ASSERT(!spmdProg.empty() || !esimdProg.empty());
  if (spmdProg.empty()) {
#if defined(IGC_VC_ENABLED)
      // Only ESIMD code detected.
      STB_TranslateInputArgs newArgs = *pInputArgs;
      newArgs.pOptions = esimdOptions.data();
      newArgs.OptionsSize = esimdOptions.size();
      return TranslateBuildVC(&newArgs, pOutputArgs, inputDataFormatTemp,
                              IGCPlatform, profilingTimerResolution,
                              inputShHash);
#else // defined(IGC_VC_ENABLED)
      errorMessage = "ESIMD code detected, but VC not enabled in this build.";
      return false;
#endif // defined(IGC_VC_ENABLED)
  } else if (esimdProg.empty()) {
      // Only SPMD code detected.
      return TranslateBuildSPMD(pInputArgs, pOutputArgs, inputDataFormatTemp,
          IGCPlatform, profilingTimerResolution,
          inputShHash);
  }

  // SPMD+ESIMD code detected.

  if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
      DumpSPIRVFile(pInputArgs->pInput, pInputArgs->InputSize, inputShHash, ".spmd_and_esimd.spv");
      DumpSPIRVFile((const char*)spmdProg.data(), spmdProg.size() * sizeof(uint32_t), inputShHash, ".spmd_split.spv");
      DumpSPIRVFile((const char*)esimdProg.data(), esimdProg.size() * sizeof(uint32_t), inputShHash, ".esimd_split.spv");
  }

#if defined(IGC_VC_ENABLED)
  // Compile ESIMD part.
  TC::STB_TranslateOutputArgs outputArgs;
  CIF::SafeZeroOut(outputArgs);
  STB_TranslateInputArgs newArgs = *pInputArgs;
  newArgs.pInput = reinterpret_cast<char *>(esimdProg.data());
  newArgs.InputSize = esimdProg.size() * sizeof(*esimdProg.begin());
  newArgs.pOptions = esimdOptions.data();
  newArgs.OptionsSize = esimdOptions.size();

  std::string esimdInternalOptions{ pInputArgs->pInternalOptions ? pInputArgs->pInternalOptions : "" };
  esimdInternalOptions += " -emit-zebin-visa-sections";
  esimdInternalOptions += " -binary-format=ze";
  newArgs.pInternalOptions = esimdInternalOptions.data();
  newArgs.InternalOptionsSize = esimdInternalOptions.size();

  const bool success =
      TranslateBuildVC(&newArgs, &outputArgs, inputDataFormatTemp, IGCPlatform,
                       profilingTimerResolution, inputShHash);

  auto outputData = std::unique_ptr<char[]>(outputArgs.pOutput);
  auto errorString = std::unique_ptr<char[]>(outputArgs.pErrorString);
  auto debugData = std::unique_ptr<char[]>(outputArgs.pDebugData);

  if (!success) {
    errorMessage = "VLD: Failed to compile ESIMD part with following error: \n";
    errorMessage += outputArgs.pErrorString;
    return false;
  }

  auto esimdVISA = GetVISAAsmFromZEBinary(
      outputArgs.pOutput, outputArgs.OutputSize);

  if (!esimdVISA) {
    errorMessage =
        "VLD: Failed to compile ESIMD part with following error: \n" +
        llvm::toString(esimdVISA.takeError());
    return false;
  }

#if (defined(__GNUC__) && __GNUC__ >= 9) || (defined(_MSC_VER) && (_MSVC_LANG >= 201703L))
  if (esimdVISA->empty()) {
    // Temporary hack. VC backend currently doesn't populate the .visaasm
    // section. Get it from the dumps, if they are enabled.
    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
      const char *pOutputFolder = IGC::Debug::GetShaderOutputFolder();
      if (pOutputFolder && std::strlen(pOutputFolder) == 0) pOutputFolder = ".";

      for (auto &p : std::filesystem::directory_iterator{pOutputFolder}) {
        if (p.is_regular_file() && p.path().extension() == ".visaasm" &&
            llvm::StringRef(p.path().filename().string()).startswith("VC")) {
          std::ifstream file(p.path().string());
          std::stringstream ss;
          ss << file.rdbuf();
          esimdVISA->push_back(ss.str());
        }
      }
    }
  }
#endif // compiler version check

  if (esimdVISA->empty()) {
      errorMessage = "VLD: ZeBinary did not contain any .visaasm sections for "
          "ESIMD kernel!";
      return false;
  }

  auto visaStrings = std::make_unique<const char* []>(esimdVISA.get().size());
  int i = 0;
  for (auto& s : esimdVISA.get()) {
    visaStrings[i++] = s.c_str();
  }

  // Compile SPMD part with ESIMD visaasm attached.
  STB_TranslateInputArgs newArgsSPMD = *pInputArgs;

  newArgsSPMD.pInput = reinterpret_cast<char*>(spmd_esimd_programs_or_err->first.data());
  newArgsSPMD.InputSize = spmd_esimd_programs_or_err->first.size() * sizeof(*spmd_esimd_programs_or_err->first.begin());
  newArgsSPMD.pVISAAsmToLinkArray = visaStrings.get();
  newArgsSPMD.NumVISAAsmsToLink = esimdVISA.get().size();

  return TranslateBuildSPMD(&newArgsSPMD, pOutputArgs, inputDataFormatTemp,
      IGCPlatform, profilingTimerResolution,
      inputShHash);

#else // defined(IGC_VC_ENABLED)
    errorMessage = "Could not compile ESIMD part of SPIR-V module, as VC is not included in this build.";
    return false;
#endif // defined(IGC_VC_ENABLED)

}
}  // namespace VLD
}  // namespace IGC
