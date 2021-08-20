/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"
#include "IGC/common/igc_debug.h"
#include "common/debug/DebugMacros.hpp"
#include <string>

namespace llvm
{
    class AssemblyAnnotationWriter;
    class Value;
}


namespace IGC
{
    namespace Debug
    {
        void Banner(llvm::raw_ostream & OS, std::string const& message);

        /// Stream that writes to both std::cout and OutputDebugString
        llvm::raw_ostream& ods();

        void Warning(
            const char*           pExpr,
            unsigned int          line,
            const char*           pFileName,
            std::string    const& message );

        void RegisterErrHandlers();
        void ReleaseErrHandlers();

        void RegisterComputeErrHandlers(llvm::LLVMContext &C);

        extern void DumpLock();
        extern void DumpUnlock();
    }

    int getPointerSize(llvm::Module &M);
}

