/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

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

#define IF_DEBUG_INFO(X) X
#define IF_DEBUG_INFO_IF(C, X) if (C) { X }
