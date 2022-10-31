/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_ENUM_BITSET
#define _IGA_ENUM_BITSET

#include "asserts.hpp"
#include <cstdint>

namespace iga {

#define ENUM_BITSET_VALUE(T, I) (1 << (static_cast<I>(T)))
#define ENUM_BITSET_EMPTY_VALUE 0

// This class represnts a bitset, typically enum values.
// The element type is T; the underlying integer represenation type is I.
// Distinct enumerates must have ordinal values between 0 and the number
// of bits in the underlying word representing the set.  I.e. we shift
// by the enum ordinal to access an element.  Hence, enumerations with large
// values will fail (checked in Debug mode).
template <typename T, typename I = uint32_t> struct EnumBitset {
  I bits;

  // the default constructor clears the set
  EnumBitset() : bits(ENUM_BITSET_EMPTY_VALUE) {}
  // conversion from a set of bits
  EnumBitset(I _bits) : bits(_bits) {}

  // checks membership of an element in the set
  // delegates to & on the bits
  bool contains(T t) const { return (bits & ENUM_BITSET_VALUE(t, I)) != 0; }

  // removes an element from the set
  // uses ~ on the bits.
  void remove(T t) { bits = (bits & (~ENUM_BITSET_VALUE(t, I))); }

  // queries if the set is empty
  bool empty() const { return bits == ENUM_BITSET_EMPTY_VALUE; }

  // clears the set
  void clear() { bits = ENUM_BITSET_EMPTY_VALUE; }

  // adds an element to the set
  // returns bool indicating if the set changed as a result of the add
  bool add(const T t) {
    IGA_ASSERT(static_cast<I>(t) >= 0 && static_cast<I>(t) < 8 * sizeof(bits),
               "bit index out of range");

    I oldBits = bits;
    bits |= ENUM_BITSET_VALUE(t, I);

    return bits != oldBits;
  }

  // unions in a full bitset into the current bitset
  // returns bool indicating if the set changed as a result of the add
  bool add(const EnumBitset<T, I> &bs) {
    I oldBits = bits;
    bits |= bs.bits;
    return bits != oldBits;
  }
}; // struct EnumBitset

// see ForAllSetBits() below
template <typename T, typename I = uint32_t> struct EnumBitsetIterator {
  EnumBitsetIterator(const EnumBitset<T, I> &_bits, int _currIndex)
      : bits(_bits), currIndex(_currIndex) {}

  T operator*() const {
    IGA_ASSERT(currIndex < 8 * sizeof(I), "iterator at end");
    return static_cast<T>(currIndex);
  }
  bool operator==(const EnumBitsetIterator<T, I> &rhs) const {
    return currIndex == rhs.currIndex;
  }
  bool operator!=(const EnumBitsetIterator<T, I> &rhs) const {
    return !(*this == rhs);
  }
  EnumBitsetIterator<T, I> operator++(int) { // post-increment
    auto v = *this;
    currIndex++;
    advance();
    return v;
  }
  EnumBitsetIterator<T, I> &operator++() { // pre increment
    currIndex++;
    advance();
    return *this;
  }

private:
  const EnumBitset<T, I> &bits;
  int currIndex;

  // advance to next set element starting from currIndex
  void advance() {
    while (currIndex < 8 * sizeof(I) &&
           !bits.contains(static_cast<T>(currIndex))) {
      currIndex++;
    }
  }
};
template <typename T, typename I = uint32_t> struct EnumBitsetWalker {
  const EnumBitset<T, I> &bits;
  EnumBitsetIterator<T, I> endItr;

  EnumBitsetWalker(const EnumBitset<T, I> &_bits)
      : bits(_bits), endItr(_bits, 8 * sizeof(I)) {}

  EnumBitsetIterator<T, I> begin() { return EnumBitsetIterator<T, I>(bits, 0); }
  EnumBitsetIterator<T, I> &end() { return endItr; }
};

//
// Allows one to iterate all the elements of an EnumBitSet in a for
// each loop.
//
// for (Enum thing : ForAllSetBits(bs)) {
//   ...
// }
template <typename T, typename I = uint32_t>
EnumBitsetWalker<T, I> ForAllSetBits(const EnumBitset<T, I> &bs) {
  return EnumBitsetWalker<T, I>(bs);
}

} // namespace iga

#endif // _IGA_ENUM_BITSET
