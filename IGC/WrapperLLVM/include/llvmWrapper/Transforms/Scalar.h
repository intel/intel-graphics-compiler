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

#ifndef IGCLLVM_TRANSFORMS_SCALAR_H
#define IGCLLVM_TRANSFORMS_SCALAR_H

#include <llvm/Transforms/Scalar.h>

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR == 4
    inline static llvm::Pass* createLoopUnrollPass(
        int OptLevel = 2, int Threshold = -1, int Count = -1,
        int AllowPartial = -1, int Runtime = -1,
        int UpperBound = -1, int AllowPeeling = -1)
    {
        return llvm::createLoopUnrollPass(Threshold, Count, AllowPartial, Runtime, UpperBound);
    }
#elif LLVM_VERSION_MAJOR == 7
    using llvm::createLoopUnrollPass;
#elif LLVM_VERSION_MAJOR == 8
    inline static llvm::Pass* createLoopUnrollPass(
        int OptLevel = 2, int Threshold = -1, int Count = -1,
        int AllowPartial = -1, int Runtime = -1,
        int UpperBound = -1, int AllowPeeling = -1)
    {
        return llvm::createLoopUnrollPass(OptLevel, false, Threshold, Count, AllowPartial, Runtime, UpperBound, AllowPeeling);
    }
#elif LLVM_VERSION_MAJOR == 9 || LLVM_VERSION_MAJOR == 10
    inline static llvm::Pass * createLoopUnrollPass(
        int OptLevel = 2, int Threshold = -1, int Count = -1,
        int AllowPartial = -1, int Runtime = -1,
        int UpperBound = -1, int AllowPeeling = -1)
    {
        return llvm::createLoopUnrollPass(OptLevel, false, false, Threshold, Count, AllowPartial, Runtime, UpperBound, AllowPeeling);
    }
#elif LLVM_VERSION_MAJOR >= 11
    //DO NOT assume same function signature for all incoming llvm versions! Double check to upgrade!
    assert(!"Not supported llvm version!");
#endif
}
#endif
