/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_ITERATOR_H
#define VC_UTILS_GENERAL_ITERATOR_H

#include <llvm/ADT/iterator_range.h>

namespace vc {

// A flat iterator to go through Container(OutItTy) and nested
// Contaiter(InnItTy).
// eg. std::vector<std::vector<int>>.
template <typename OutItTy, typename InnItTy> struct FlatIterator {
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;
  using value_type = typename std::iterator_traits<InnItTy>::value_type;
  using pointer = typename std::iterator_traits<InnItTy>::pointer;
  using reference = typename std::iterator_traits<InnItTy>::reference;

  FlatIterator() = default;
  FlatIterator(OutItTy OuterItIn, InnItTy InnerItIn, OutItTy EndIt)
      : OuterIt{OuterItIn}, InnerIt{InnerItIn}, LastValidOuterIt{EndIt} {
    skipEmpty();
  }

  reference operator*() const { return *InnerIt; }
  pointer operator->() const { return InnerIt.operator->(); }

  // Prefix increment.
  FlatIterator &operator++() {
    if (InnerIt != OuterIt->end())
      InnerIt = std::next(InnerIt);
    if (InnerIt == OuterIt->end())
      // Skips all empty 'rows' and procceds to the next valid item or to the
      // end.
      skipEmpty();
    return *this;
  }

  // Postfix increment.
  FlatIterator operator++(int) {
    FlatIterator Temp = *this;
    ++(*this);
    return Temp;
  }

  friend bool operator==(const FlatIterator &LHS, const FlatIterator &RHS) {
    return LHS.InnerIt == RHS.InnerIt && LHS.OuterIt == RHS.OuterIt;
  }
  friend bool operator!=(const FlatIterator &LHS, const FlatIterator &RHS) {
    return !(LHS == RHS);
  }

private:
  OutItTy LastValidOuterIt{};
  OutItTy OuterIt{};
  InnItTy InnerIt{};

  // An example of iterator setup in flat_begin(), flat_end() cases
  // for std::array<std::vector<int>, 3>:
  //  OuterIt - flat_begin()
  //  v      .- InnerIt - flat_begin()
  // |~| -> |*|*|*|*|*|*|*|*|*|*|*|*|*|*|*|
  // |~| -> |*|*|*|*|*|*|*|*|*|*|*|*|
  // |~| -> |*|*|*|*|*|*|*|*|*|*|*|*|*|*|*|*|end()|
  //  ^                                       '- InnerIt - flat_end()
  //  LastValidOuterIt - always points here and OuterIt points for flat_end().

  // Skips empty nested containers.
  // Sets pointer to the first valid item in nested container or sets pointer to
  // the end of the last nested container.
  // {{}, {}, {}, {10, 5}, {}}
  //  ^-start.     ^- after skipEmpty() will point here.
  void skipEmpty() {
    while (InnerIt == OuterIt->end() && OuterIt != LastValidOuterIt) {
      ++OuterIt;
      InnerIt = OuterIt->begin();
    }
  }
};

template <typename Container> struct FlatIteratorTraits {
  using OuterItTy = std::conditional_t<std::is_const_v<Container>,
                                       typename Container::const_iterator,
                                       typename Container::iterator>;
  using InnerItTy =
      std::conditional_t<std::is_const_v<Container>,
                         typename Container::value_type::const_iterator,
                         typename Container::value_type::iterator>;
};

// Creates an iterator that points to the first valid element in the nested
// container. OuterIt points to the first non-empty nested container. InnerIt
// points to the first element of the first non-empty nested container.
// flat_begin() == flat_end() if there is no any elements in the nested
// containers.
template <typename Container> auto flat_begin(Container &&C) {
  using NoRefContainer = std::remove_reference_t<Container>;
  using MaybeConstOuterItTy =
      typename FlatIteratorTraits<NoRefContainer>::OuterItTy;
  using MaybeConstInnerItTy =
      typename FlatIteratorTraits<NoRefContainer>::InnerItTy;
  if (C.empty())
    return FlatIterator<MaybeConstOuterItTy, MaybeConstInnerItTy>{};
  return FlatIterator{C.begin(), C.begin()->begin(), std::prev(C.end())};
}

// Creates an iterator that points to the end of the nested container.
// OuterIt points to the last nested container.
// InnerIt points to the end() of the last nested container.
template <typename Container> auto flat_end(Container &&C) {
  using NoRefContainer = std::remove_reference_t<Container>;
  using MaybeConstOuterItTy =
      typename FlatIteratorTraits<NoRefContainer>::OuterItTy;
  using MaybeConstInnerItTy =
      typename FlatIteratorTraits<NoRefContainer>::InnerItTy;
  if (C.empty())
    return FlatIterator<MaybeConstOuterItTy, MaybeConstInnerItTy>{};
  MaybeConstOuterItTy LastIt = std::prev(C.end());
  return FlatIterator{LastIt, LastIt->end(), LastIt};
}

// Creates range for nested containers.
template <typename Container> auto make_flat_range(Container &&C) {
  return llvm::make_range(flat_begin(std::forward<Container>(C)),
                          flat_end(std::forward<Container>(C)));
}

} // namespace vc

#endif // VC_UTILS_GENERAL_ITERATOR_H
