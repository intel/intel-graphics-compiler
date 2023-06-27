/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "llvm/ADT/StringRef.h"
#include <vector>

using namespace llvm;

namespace IGC
{
    class SPIRVParser
    {
    public:
        static std::vector<std::string> getEntryPointNames(const StringRef binary);
    };
}
