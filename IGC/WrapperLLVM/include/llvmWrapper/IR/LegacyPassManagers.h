/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#ifndef IGCLLVM_LEGACY_PASS_MANAGERS_H
#define IGCLLVM_LEGACY_PASS_MANAGERS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/LegacyPassManagers.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR == 4
    using llvm::PMDataManager;
#elif LLVM_VERSION_MAJOR >= 7
    class PMDataManager : public llvm::PMDataManager
    {
    public:
      inline unsigned initSizeRemarkInfo(llvm::Module &M,
                                  llvm::StringMap<std::pair<unsigned, unsigned>> &FunctionToInstrCount)
        {
#if LLVM_VERSION_MAJOR > 7
            return llvm::PMDataManager::initSizeRemarkInfo(M, FunctionToInstrCount);
#else
            return llvm::PMDataManager::initSizeRemarkInfo(M);
#endif
        }

      inline void emitInstrCountChangedRemark(llvm::Pass *P, llvm::Module &M, int64_t Delta, unsigned CountBefore,
                                       llvm::StringMap<std::pair<unsigned, unsigned>> &FunctionToInstrCount)
        {
#if LLVM_VERSION_MAJOR > 7
            llvm::PMDataManager::emitInstrCountChangedRemark(P, M, Delta, CountBefore, FunctionToInstrCount);
#else
            llvm::PMDataManager::emitInstrCountChangedRemark(P, M, CountBefore);
#endif
        }
    };
#endif
}

#endif
