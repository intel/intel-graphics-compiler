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

namespace {

// Extracts .visaasm sections from input zeBinary ELF.
// Returns a vector of strings - one for each section.
llvm::Expected<std::vector<llvm::StringRef>>
GetVISAAsmFromZEBinary(const char *zeBinary, size_t zeBinarySize) {
  using namespace llvm;

  std::vector<StringRef> OutVISAAsmStrings;

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
      OutVISAAsmStrings.push_back(Data);
    }
  }

  return OutVISAAsmStrings;
}
} // namespace

namespace TC {
// Declarations for utility functions declared in other libraries that will be linked.
void DumpShaderFile(const std::string &dstDir, const char *pBuffer,
                    const UINT bufferSize, const QWORD hash,
                    const std::string &ext, std::string *fileName = nullptr);
spv_result_t DisassembleSPIRV(const char* pBuffer, UINT bufferSize,
                              spv_text* outSpirvAsm);
}

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

  IGC_ASSERT(!spmd_esimd_programs_or_err->first.empty() || !spmd_esimd_programs_or_err->second.empty());
  if (spmd_esimd_programs_or_err->first.empty()) {
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
  } else if (spmd_esimd_programs_or_err->second.empty()) {
      // Only SPMD code detected.
      return TranslateBuildSPMD(pInputArgs, pOutputArgs, inputDataFormatTemp,
          IGCPlatform, profilingTimerResolution,
          inputShHash);
  }

  // SPMD+ESIMD code detected.

  if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
    const char* pOutputFolder = IGC::Debug::GetShaderOutputFolder();

    auto dump = [&](const char* programData, auto programSizeInBytes, std::string ext) {
      TC::DumpShaderFile(
          pOutputFolder,
          programData,
          programSizeInBytes,
          inputShHash.getAsmHash(), ext);
      spv_text spirvAsm = nullptr;
      if (TC::DisassembleSPIRV(
              programData,
              programSizeInBytes,
              &spirvAsm) == SPV_SUCCESS) {
        DumpShaderFile(pOutputFolder, spirvAsm->str, spirvAsm->length,
                       inputShHash.getAsmHash(), ext+"asm");
      }
      spvTextDestroy(spirvAsm);
    };
    auto spmdProg = (*spmd_esimd_programs_or_err).first;
    auto esimdProg = (*spmd_esimd_programs_or_err).second;
    dump(pInputArgs->pInput, pInputArgs->InputSize, ".spmd_and_esimd.spv");
    dump((const char*)spmdProg.data(), spmdProg.size() * sizeof(uint32_t), ".spmd_split.spv");
    dump((const char*)esimdProg.data(), esimdProg.size() * sizeof(uint32_t), ".esimd_split.spv");
  }

  auto translateToVISA =
      [&](VLD::ProgramStreamType& program,
          const std::string& newOptions,
          decltype(TranslateBuildSPMD) TranslateFunction
          ) -> decltype(GetVISAAsmFromZEBinary(0,0)) {
        STB_TranslateInputArgs newArgs = *pInputArgs;

        TC::STB_TranslateOutputArgs outputArgs;
        CIF::SafeZeroOut(outputArgs);

        newArgs.pInput = reinterpret_cast<char*>(program.data());
        newArgs.InputSize = program.size() * sizeof(*program.begin());;
        newArgs.pOptions = newOptions.data();
        newArgs.OptionsSize = newOptions.size();

        const bool success =
            TranslateFunction(&newArgs, &outputArgs, inputDataFormatTemp,
                           IGCPlatform, profilingTimerResolution, inputShHash);

        auto outputData = std::unique_ptr<char[]>(outputArgs.pOutput);
        auto errorString = std::unique_ptr<char[]>(outputArgs.pErrorString);
        auto debugData = std::unique_ptr<char[]>(outputArgs.pDebugData);

        if (!success)
          return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                         outputArgs.pErrorString);

        auto spmdVISAAsmVector = GetVISAAsmFromZEBinary(
            outputArgs.pOutput, outputArgs.OutputSize);

        return spmdVISAAsmVector;
  };

#if defined(IGC_VC_ENABLED)
  auto esimdVISA =
      translateToVISA(spmd_esimd_programs_or_err->second, esimdOptions, TranslateBuildVC);

  if (!esimdVISA) {
    errorMessage = "VLD: Failed to compile ESIMD part with following error: \n" +
                   llvm::toString(esimdVISA.takeError());
      return false;
  }
#else // defined(IGC_VC_ENABLED)
    errorMessage = "Could not compile ESIMD part of SPIR-V module, as VC is not included in this build.";
    return false;
#endif // defined(IGC_VC_ENABLED)

  auto spmdVISA =
             translateToVISA(spmd_esimd_programs_or_err->first, newOptions.data(), TranslateBuildSPMD);

  if (!spmdVISA) {
    errorMessage = "VLD: Failed to compile SPMD part with following error: \n" +
                   llvm::toString(spmdVISA.takeError());
    return false;
  }

  return true;
}
}  // namespace VLD
}  // namespace IGC
