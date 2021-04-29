/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/MemoryBuffer.h>
#include "common/LLVMWarningsPop.hpp"

#include <string>

namespace llvm
{
    MemoryBuffer* LoadBufferFromResource(const char *pResName, const char *pResType);

    /// LoadBufferFromFile - Loads a buffer from a file in disk
    ///
    MemoryBuffer* LoadBufferFromFile( const std::string &FileName );

} // namespace llvm

