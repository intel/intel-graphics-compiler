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
#include <llvm/IR/LegacyPassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include <list>
#include "Stats.hpp"
#include <string.h>

namespace IGC
{
    namespace Debug
    {
        class Dump;
    }
    class CodeGenContext;

    class IGCPassManager : public llvm::legacy::PassManager
    {
    public:
        IGCPassManager(CodeGenContext* ctx, const char* name = "") : m_pContext(ctx), m_name(name)
        {
        }
        void add(llvm::Pass *P);
    private:
        CodeGenContext* const m_pContext;
        const std::string m_name;
        std::list<Debug::Dump> m_irDumps;

        void addPrintPass(llvm::Pass* P, bool isBefore);
        bool isPrintBefore(llvm::Pass* P);
        bool isPrintAfter(llvm::Pass* P);

        // List: comma/semicolon-separate list of names.
        // Return true if N is in the list.
        bool isInList(const llvm::StringRef& N, const llvm::StringRef& List) const;
    };
}

void DumpLLVMIR(IGC::CodeGenContext* pContext, const char* dumpName);
