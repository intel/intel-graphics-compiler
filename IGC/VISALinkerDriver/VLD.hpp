/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file contains interface for compilation of a SPIR-V module that
// consists of combined SPMD and ESIMD parts.

#include <string>
#include "TranslationBlock.h"

// Forward declarations.
class ShaderHash;
namespace IGC {
    class CPlatform;
}

namespace IGC {
namespace VLD {

bool TranslateBuildSPMDAndESIMD(const TC::STB_TranslateInputArgs* pInputArgs,
                                TC::STB_TranslateOutputArgs* pOutputArgs,
                                TC::TB_DATA_FORMAT inputDataFormatTemp,
                                const IGC::CPlatform& IGCPlatform,
                                float profilingTimerResolution,
                                const ShaderHash& inputShHash,
                                std::string& errorMessage);

}  // namespace VLD
}  // namespace IGC
