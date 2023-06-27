/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file contains interface for compilation of a SPIR-V module that
// consists of combined SPMD and ESIMD parts.

#include <string>
#include <llvm/ADT/ArrayRef.h>
#include "TranslationBlock.h"

#pragma once

// Forward declarations.
class ShaderHash;
namespace IGC {
class CPlatform;
}

namespace IGC {
namespace VLD {

  enum class SPIRVTypeEnum {
    SPIRV_SPMD,
    SPIRV_ESIMD,
    SPIRV_SPMD_AND_ESIMD,
    SPIRV_TYPE_UNKNOWN
  };

  struct SPVMetadata {
    SPIRVTypeEnum SpirvType = SPIRVTypeEnum::SPIRV_TYPE_UNKNOWN;
    std::vector<std::string> ExportedFunctions;
    std::vector<std::string> ImportedFunctions;
    bool HasEntryPoints = false;
    int ForcedSubgroupSize = 0;
  };

  using SPVTranslationPair = std::pair<SPVMetadata, TC::STB_TranslateInputArgs>;

// This function detects if binary passed in pInputArgs is SPMD, ESIMD or
// SPMD+ESIMD
bool TranslateBuildSPMDAndESIMD(
    const TC::STB_TranslateInputArgs* pInputArgs,
    TC::STB_TranslateOutputArgs* pOutputArgs,
    TC::TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform& IGCPlatform,
    float profilingTimerResolution, const ShaderHash& inputShHash,
    std::string& errorMessage);

// This function takes SPMD and ESIMD modules explicitly.
bool TranslateBuildSPMDAndESIMD(
    llvm::ArrayRef<SPVTranslationPair> inputModules,
    TC::STB_TranslateOutputArgs* pOutputArgs,
    TC::TB_DATA_FORMAT inputDataFormatTemp, const IGC::CPlatform& IGCPlatform,
    float profilingTimerResolution, const ShaderHash& inputShHash,
    std::string& errorMessage);

}  // namespace VLD
}  // namespace IGC
