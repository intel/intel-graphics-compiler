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

#ifndef IGCLLVM_IR_PATTERNMATCH_H
#define IGCLLVM_IR_PATTERNMATCH_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/PatternMatch.h"

namespace llvm {
namespace PatternMatch {
#if LLVM_VERSION_MAJOR < 8

    template <typename Val_t, typename Idx_t>
    inline ExtractElementClass_match<Val_t, Idx_t>
    m_ExtractElt(const Val_t &Val, const Idx_t &Idx) {
        return m_ExtractElement<Val_t, Idx_t>(Val, Idx);
    }

    template <typename Val_t, typename Elt_t, typename Idx_t>
    inline InsertElementClass_match<Val_t, Elt_t, Idx_t>
    m_InsertElt(const Val_t &Val, const Elt_t &Elt, const Idx_t &Idx) {
        return m_InsertElement<Val_t, Elt_t, Idx_t>(Val, Elt, Idx);
    }

#elif LLVM_VERSION_MAJOR < 11

    template <typename Val_t, typename Elt_t, typename Idx_t>
    inline ThreeOps_match<Val_t, Elt_t, Idx_t, Instruction::InsertElement>
    m_InsertElt(const Val_t &Val, const Elt_t &Elt, const Idx_t &Idx) {
      return m_InsertElement<Val_t, Elt_t, Idx_t>(Val, Elt, Idx);
    }

    template <typename Val_t, typename Idx_t>
    inline TwoOps_match<Val_t, Idx_t, Instruction::ExtractElement>
    m_ExtractElt(const Val_t &Val, const Idx_t &Idx) {
      return m_ExtractElement<Val_t, Idx_t>(Val, Idx);
    }

#endif

}
}

#endif
