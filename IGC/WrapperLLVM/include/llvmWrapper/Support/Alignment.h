/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_ALIGNMENT_H
#define IGCLLVM_SUPPORT_ALIGNMENT_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Support/Alignment.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/Instruction.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include <cstdint>
#include <type_traits>

typedef uint64_t alignment_t;

namespace IGCLLVM {
inline alignment_t getAlignmentValue(llvm::Align A) { return (alignment_t)A.value(); }
inline alignment_t getAlignmentValue(llvm::MaybeAlign A) { return A ? (alignment_t)A->value() : 0; }
inline alignment_t getAlignmentValue(uint64_t Val) { return (alignment_t)Val; }
inline llvm::Align getAlign(uint64_t Val) { return llvm::Align{Val}; }

// The transition from unsigned to llvm::Align was not completed with LLVM
// 9->10 switch and some of functions (see IRBuilder methods) were still
// using unsigned type. With LLVM 11 transition this was changed and now
// IRBuilder methods are using llvm::Align. It creates a problem with
// IGCLLVM::getAlign function, because it stopped work properly.
// IGCLLVM::getAlignmentValueIfNeeded is a helper to getAlign function that
// resolves such situations.
inline llvm::Align getAlignmentValueIfNeeded(llvm::Align A) { return A; }

inline llvm::Align getCorrectAlign(alignment_t Val) {
  // llvm::Align does not accept 0 alignment.
  return llvm::assumeAligned(Val);
}

// It is meant for copying alignement.
// getAlign returns different type for different LLVM versions but
// it can be overcome by using auto or direct usage in another LLVM
// interface.
template <typename TValue, std::enable_if_t<std::is_base_of_v<llvm::GlobalObject, TValue>, int> = 0>
llvm::Align getAlign(const TValue &Val) {
  return llvm::Align(Val.getAlignment());
}

template <typename TValue, std::enable_if_t<std::is_base_of_v<llvm::Instruction, TValue>, int> = 0>
llvm::Align getAlign(const TValue &Val) {
  return Val.getAlign();
}

// With multiple LLVM versions, we need a general helper for extracting the
// integer value.
template <typename TValue, std::enable_if_t<std::is_base_of_v<llvm::Instruction, TValue>, int> = 0>
inline alignment_t getAlignmentValue(const TValue *Val) {
  return getAlignmentValue(getAlign(*Val));
}

template <typename TValue, std::enable_if_t<std::is_base_of_v<llvm::Value, TValue>, int> = 0>
llvm::Align getDestAlign(const TValue &Val) {
  return Val.getDestAlign().valueOrOne();
}

template <typename TValue, std::enable_if_t<std::is_base_of_v<llvm::Value, TValue>, int> = 0>
llvm::Align getSourceAlign(const TValue &Val) {
  return Val.getSourceAlign().valueOrOne();
}

inline uint64_t alignTo(uint64_t Size, llvm::Align A) { return llvm::alignTo(Size, A); }

inline llvm::Align getABITypeAlign(const llvm::DataLayout &DL, llvm::Type *Ty) { return DL.getABITypeAlign(Ty); }

inline llvm::Align getPrefTypeAlign(const llvm::DataLayout &DL, llvm::Type *Ty) { return DL.getPrefTypeAlign(Ty); }

} // namespace IGCLLVM

#endif
