/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_REGEX_ITERATOR_H
#define VC_UTILS_GENERAL_REGEX_ITERATOR_H

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Regex.h>

#include <iterator>

#include "Probe/Assertion.h"

namespace vc {

// Iterates over the provided string matches.
// It is a constant input iterator.
// Size template parameter is used only for SmallVector with matches
// parametrization, so it should be equal to the expected number of
// matches.
// Note that matches are stored inside the iterator itself, so it may be not
// that cheap to copy the iterator.
template <unsigned Size> struct RegexIterator {
  using difference_type = unsigned;
  using value_type = llvm::SmallVector<llvm::StringRef, Size>;
  using pointer = const value_type *;
  using reference = const value_type &;
  using iterator_category = std::input_iterator_tag;

private:
  // Regex::match is non-const prior to LLVM-10.
  llvm::Regex *RegExpr = nullptr;
  value_type Match;
  llvm::StringRef Tail;

public:
  RegexIterator() {}
  RegexIterator(llvm::StringRef Str, llvm::Regex &RegExprIn)
      : RegExpr{&RegExprIn} {
    if (!RegExpr->match(Str, &Match)) {
      // Setting to default constructed when nothing is matched.
      RegExpr = nullptr;
      return;
    }
    auto *TailBegin = Match[0].end();
    Tail =
        llvm::StringRef{TailBegin, static_cast<size_t>(Str.end() - TailBegin)};
  }

  RegexIterator(const RegexIterator &) = default;
  RegexIterator(RegexIterator &&) = default;
  RegexIterator &operator=(const RegexIterator &) = default;
  RegexIterator &operator=(RegexIterator &&) = default;

  RegexIterator &operator++() {
    IGC_ASSERT_MESSAGE(RegExpr, "cannot increment sentinel");
    *this = RegexIterator{Tail, *RegExpr};
    return *this;
  }

  reference operator*() const { return Match; }

  pointer operator->() const { return &Match; }

  template <unsigned InnerSize>
  friend bool operator==(const RegexIterator<InnerSize> &LHS,
                         const RegexIterator<InnerSize> &RHS) {
    return LHS.Tail == RHS.Tail && LHS.RegExpr == RHS.RegExpr &&
           LHS.Match == RHS.Match;
  }

  template <unsigned InnerSize>
  friend bool operator!=(const RegexIterator<InnerSize> &LHS,
                         const RegexIterator<InnerSize> &RHS) {
    return !(LHS == RHS);
  }
};

} // namespace vc

#endif // VC_UTILS_GENERAL_REGEX_ITERATOR_H
