/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_PATTERNMATCH_H
#define IGCLLVM_IR_PATTERNMATCH_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/PatternMatch.h"

namespace llvm {
namespace PatternMatch {
#if LLVM_VERSION_MAJOR < 11

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
