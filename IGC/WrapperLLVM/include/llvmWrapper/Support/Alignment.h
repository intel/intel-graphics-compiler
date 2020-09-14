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

#ifndef IGCLLVM_SUPPORT_ALIGNMENT_H
#define IGCLLVM_SUPPORT_ALIGNMENT_H

#include <cstdint>
#include "llvm/Config/llvm-config.h"
#if LLVM_VERSION_MAJOR >= 10
#include "llvm/Support/Alignment.h"
using namespace llvm;
#endif

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR < 10
    inline uint64_t getAlignmentValue(uint64_t Val) { return Val; }
    inline unsigned getAlign(uint64_t Val) { return (unsigned)Val; }
#else
    inline uint64_t getAlignmentValue(llvm::Align A) { return A.value(); }
    inline uint64_t getAlignmentValue(uint64_t Val) { return Val; }
    inline llvm::Align getAlign(uint64_t Val) { return llvm::Align{Val}; }
#endif

    // The transition from unsigned to llvm::Align was not completed with LLVM
    // 9->10 switch and some of functions (see IRBuilder methods) were still
    // using unsigned type. With LLVM 11 transition this was changed and now
    // IRBuilder methods are using llvm::Align. It creates a problem with
    // IGCLLVM::getAlign function, because it stopped work properly.
    // IGCLLVM::getAlignmentValueIfNeeded is a helper to getAlign function that
    // resolves such situations.
#if LLVM_VERSION_MAJOR < 10
    inline uint64_t getAlignmentValueIfNeeded(uint64_t A) { return A; }
#elif LLVM_VERSION_MAJOR == 10
    inline uint64_t getAlignmentValueIfNeeded(llvm::Align A) {
      return A.value();
    }
#else
    inline llvm::Align getAlignmentValueIfNeeded(llvm::Align A) { return A; }
#endif

    template<typename T =
#if LLVM_VERSION_MAJOR < 10
        uint32_t
#elif LLVM_VERSION_MAJOR == 10
        llvm::MaybeAlign
#elif LLVM_VERSION_MAJOR >= 11
        llvm::Align
#endif
    >
    inline T getCorrectAlign(uint32_t Val)
    {
        return T{ Val };
    }

} // namespace IGCLLVM

#endif
