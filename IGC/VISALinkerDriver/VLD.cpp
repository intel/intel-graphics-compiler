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

  std::vector<StringRef> OutVSAAsmStrings;

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
      OutVSAAsmStrings.push_back(Data);
    }
  }

  return OutVSAAsmStrings;
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


// Translates SPIR-V module that consists of SPMD code that invokes ESIMD code.
// The general flow is:
// 1. Split input SPIR-V module into SPMD and ESIMD parts
// 2. Invoke SPMD and ESIMD backends with appropriate SPIR-V modules
// 3. Extract .visaasm from the output zeBinary
// TODO: 4. Link .visaasm files via vISA interfaces
//
// The function signature corresponds to TC::TranslateBuild interface, so that
// it is easy to pass same arguments to SPMD and VC backends.
//
// Assumptions:
// 1. ZEBinary output format is used.
// TODO: error out if patch token output format is used.
// 2. Input is in SPIR-V format
// TODO: error out if other input format is used.
bool TranslateBuildSPMDAndESIMD(const TC::STB_TranslateInputArgs *pInputArgs,
                                  TC::STB_TranslateOutputArgs *pOutputArgs,
                                  TC::TB_DATA_FORMAT inputDataFormatTemp,
                                  const IGC::CPlatform &IGCPlatform,
                                  float profilingTimerResolution,
                                  const ShaderHash &inputShHash,
                                  std::string& errorMessage) {

    // Split ESIMD and SPMD code.
  auto spmd_esimd_programs_or_err = VLD::SplitSPMDAndESIMD(
      pInputArgs->pInput, pInputArgs->InputSize);

  if (!spmd_esimd_programs_or_err) {
    // Caller releases the error string, so we need to make a copy of the error message here.
        // TODO: pOutputArgs contains field for error string so we can copy it there.
    // Not done now, as it would require copy-paste code that is avaiable in dllinterfacecompute. Needs to be refactored.
    errorMessage = "Error while splitting ESIMD and SPMD code: " +
      llvm::toString(spmd_esimd_programs_or_err.takeError());
    return false;
  }

  if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
    const char* pOutputFolder = IGC::Debug::GetShaderOutputFolder();

    auto dump = [&](auto& program, std::string ext) {
      TC::DumpShaderFile(
          pOutputFolder,
          (const char*)program.data(),
          program.size() * sizeof(uint32_t),
          inputShHash.getAsmHash(), ext);
      spv_text spirvAsm = nullptr;
      if (TC::DisassembleSPIRV(
              (const char*)program.data(),
              program.size() * sizeof(uint32_t),
              &spirvAsm) == SPV_SUCCESS) {
        DumpShaderFile(pOutputFolder, spirvAsm->str, spirvAsm->length,
                       inputShHash.getAsmHash(), ext+"asm");
      }
      spvTextDestroy(spirvAsm);
    };

    dump((*spmd_esimd_programs_or_err).first, ".spmd_split.spv");
    dump((*spmd_esimd_programs_or_err).second, ".esimd_split.spv");
  }

  std::string newOptions{pInputArgs->pOptions};
  IGC_ASSERT(newOptions.find(VLD_compilation_enable_option) !=
             std::string::npos);
  newOptions.erase(newOptions.find(VLD_compilation_enable_option),
                   strnlen(VLD_compilation_enable_option, sizeof(VLD_compilation_enable_option)));

  auto translateToVISA =
      [&](VLD::ProgramStreamType& program,
          const char *newOptions) -> decltype(GetVISAAsmFromZEBinary(0,0)) {
        STB_TranslateInputArgs newArgs = *pInputArgs;

        TC::STB_TranslateOutputArgs outputArgs;
        CIF::SafeZeroOut(outputArgs);

        newArgs.pInput = reinterpret_cast<char*>(program.data());
        newArgs.InputSize = program.size() * sizeof(*program.begin());;
        newArgs.pOptions = newOptions;

        const bool success =
            TranslateBuild(&newArgs, &outputArgs, inputDataFormatTemp,
                           IGCPlatform, profilingTimerResolution);

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

  std::string esimdOptions{ newOptions };
  esimdOptions += " -vc-codegen";

  auto esimdVISA =
      translateToVISA(spmd_esimd_programs_or_err->second, esimdOptions.data());

  if (!esimdVISA) {
    errorMessage = "VLD: Failed to compile ESIMD part with following error: \n" +
                   llvm::toString(esimdVISA.takeError());
      return false;
  }

  auto spmdVISA =
             translateToVISA(spmd_esimd_programs_or_err->first, newOptions.data());

  if (!spmdVISA) {
    errorMessage = "VLD: Failed to compile SPMD part with following error: \n" +
                   llvm::toString(spmdVISA.takeError());
    return false;
  }

  return true;
}
}  // namespace VLD
}  // namespace IGC
