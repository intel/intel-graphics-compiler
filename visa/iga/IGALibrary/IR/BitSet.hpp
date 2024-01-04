/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_BITSET_HPP
#define _IGA_BITSET_HPP

#include "../asserts.hpp"
#include "common/secure_mem.h"

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

// Align N up to the nearest multiple of A
#define ALIGN_UP_TO(A, N) ((N + (A)-1) - (N + (A)-1) % (A))

namespace iga {

// I so badly wanted to just use std::bitset and then drink beer with my free
// time.  Alas this lacks some of the APIs that I need to efficiently
// perform data flow.  Specifically, I need efficient testAny/testAll
// predicates.
template <typename I = uint32_t> class BitSet {
public:
  static const size_t BITS_PER_WORD = 8 * sizeof(I);

  // allocate and zero bits
  BitSet(size_t numBits)
      : N(numBits),
        wordsSize(ALIGN_UP_TO(BITS_PER_WORD, numBits) / BITS_PER_WORD) {
    words = new I[wordsSize];
    reset();
  }
  BitSet(const BitSet<I> &rhs)
      : N(rhs.N), wordsSize(ALIGN_UP_TO(BITS_PER_WORD, rhs.N) / BITS_PER_WORD)

  {
    words = new I[wordsSize];
    memcpy_s(words, wordsSize * sizeof(I), rhs.words, wordsSize * sizeof(I));
  }

  BitSet(BitSet<I> &&copy) noexcept
      : N(copy.N), wordsSize(copy.wordsSize), words(copy.words) {
    copy.words = nullptr;
    copy.N = copy.wordsSize = 0;
  }

  BitSet<I> &operator=(const BitSet<I> &rhs) {
    if (this == &rhs) {
      return *this;
    }
    if (rhs.N != N) {
      // resize
      if (words)
        delete[] words;
      N = rhs.N;
      wordsSize = rhs.wordsSize;
      words = new I[wordsSize];
    }
    memcpy_s(words, wordsSize * sizeof(I), rhs.words, wordsSize * sizeof(I));
    return *this;
  }

  BitSet<I> &operator=(BitSet<I> &&rhs) {
    if (this == &rhs) {
      return *this;
    }
    if (words) {
      delete[] words;
    }

    N = rhs.N;
    wordsSize = rhs.wordsSize;
    words = rhs.words;

    rhs.words = nullptr;

    return *this;
  }

  ~BitSet() {
    if (words) {
      delete[] words;
      words = nullptr;
    }
  }

  // TODO: remove
  void reset() { clear(); }

  void clear() { memset(words, 0, wordsSize * sizeof(I)); }
  bool set(size_t off, size_t len = 1, bool val = true); // sets/clears range
  bool test(size_t off) const { return testAny(off, 1); }
  bool testAny(size_t off, size_t len) const;
  bool testAll(size_t off, size_t len) const;
  void containsAll(const BitSet<I> &rhs) const;
  bool empty() const { return !testAny(0, N); }

  bool intersects(const BitSet<I> &rhs) const;

  // If the bitSets have intersects in given range, start from off and with
  // legnth len off: bit offset len: range length in bits
  bool intersects(const BitSet<I> &rhs, size_t off, size_t len) const;

  // if the given rhs is completely the same as itself
  bool equal(const BitSet<I> &rhs) const;

  // THERE EXISTS WORD w such that ((w & mask) == eq)
  // bool testAnyEq(size_t off, size_t len, const I &mask, const I &eq) const;
  // FOR ALL ...
  // bool testAllEq(size_t off, size_t len, const I &mask, const I &eq) const;

  //    BitSet& operator ~() const;
  bool andNot(const BitSet<I> &rhs); // set subtraction
  bool add(const BitSet<I> &rhs);    // set union
  bool intersectInto(const BitSet<I> &rhs, BitSet<I> &into)
      const; // set intersection (returns true if result non-empty)

  bool operator[](const size_t &ix) const { return test(ix); }
  bool operator==(const BitSet<I> &bs) const;
  bool operator!=(const BitSet<I> &bs) const { return !(*this == bs); }
  std::string str() const {
    std::stringstream ss;
    str(ss);
    return ss.str();
  }
  void str(std::ostream &os) const;
  size_t cardinality() const;

  static BitSet<I> intersection(const BitSet<I> &bs1, const BitSet<I> &bs2) {
    BitSet<I> bsI(bs1.N);
    bs1.intersectInto(bs2, bsI);
    return bsI;
  }

private:
  size_t N;
  size_t wordsSize;
  I *words;

  static I makeMask(size_t len) {
    IGA_ASSERT(len <= BITS_PER_WORD,
               "BitSet::makeMask: length too large for mask");
    if (len == BITS_PER_WORD) {
      return (I)-1;
    }
    return (1 << len) - 1;
  }
};

template <typename I>
inline bool BitSet<I>::set(size_t off, size_t len, bool val) {
  IGA_ASSERT(off >= 0 && off + len <= N, "BitSet::set: index out of bounds");

  // check the first word (misaligned mask)
  auto w_ix = off / BITS_PER_WORD;
  auto w_off = off % BITS_PER_WORD;
  auto misaligned_len = std::min<size_t>(BITS_PER_WORD - w_off, len);
  auto misaligned_mask = makeMask(misaligned_len) << w_off;
  auto oldWord0 = words[w_ix];
  if (val) {
    words[w_ix] |= misaligned_mask;
  } else {
    words[w_ix] &= ~misaligned_mask;
  }
  bool changed = oldWord0 != words[w_ix];

  // check successive words (all aligned now)
  len -= misaligned_len;
  off += misaligned_len;
  while (len > 0) {
    w_ix++; // next word

    auto aligned_len = len >= 32 ? 32 : len % BITS_PER_WORD; // last will be <32
    auto aligned_mask = makeMask(aligned_len);
    auto oldWord = words[w_ix];
    if (val) {
      words[w_ix] |= aligned_mask;
    } else {
      words[w_ix] &= ~aligned_mask;
    }
    changed |= words[w_ix] != oldWord;

    len -= aligned_len;
    off += aligned_len;
  }

  return changed;
}

template <typename I>
inline bool BitSet<I>::intersects(const BitSet<I> &rhs) const {
  for (size_t i = 0; i < wordsSize; i++) {
    if (words[i] & rhs.words[i]) {
      return true;
    }
  }
  return false;
}

template <typename I>
inline bool BitSet<I>::intersects(const BitSet<I> &rhs, size_t off,
                                  size_t len) const {

  IGA_ASSERT(off >= 0 && off + len <= N,
             "BitSet::intersects: index out of bounds");

  // check the first word (may misaligned)
  auto w_ix = off / BITS_PER_WORD;  // idx of the first word
  auto w_off = off % BITS_PER_WORD; // length from w_ix to off
  size_t misaligned_len = std::min<size_t>(BITS_PER_WORD - w_off, len);
  I misaligned_mask = makeMask(misaligned_len);
  if (((words[w_ix] >> w_off) & misaligned_mask) &
      ((rhs.words[w_ix] >> w_off) & misaligned_mask)) {
    return true;
  }

  // check successive words (all aligned now)
  len -= misaligned_len;
  off += misaligned_len;
  while (len > 0) {
    w_ix++; // next word

    auto aligned_len = len >= 32 ? 32 : len % BITS_PER_WORD; // last will be <32
    auto aligned_mask = makeMask(aligned_len);
    if ((words[w_ix] & aligned_mask) & (rhs.words[w_ix] & aligned_mask)) {
      return true;
    }
    len -= aligned_len;
    off += aligned_len;
  }
  return false;
}

template <typename I> inline bool BitSet<I>::equal(const BitSet<I> &rhs) const {
  bool result = true;
  for (size_t i = 0; i < wordsSize; i++) {
    result = (words[i] == rhs.words[i]) ? true : false;
    if (!result)
      break;
  }
  // padding will remain 0 since both padding are 0's
  // 0 & 0 is 0 so no need special handling on padding
  return result;
}

template <typename I> inline bool BitSet<I>::andNot(const BitSet<I> &rhs) {
  bool changed = false;
  for (size_t i = 0; i < wordsSize; i++) {
    auto old = words[i];
    words[i] &= ~rhs.words[i];
    changed |= (words[i] != old);
  }
  // padding will remain 0 since padding is already 0's
  // ((0x...000 & 0x...FFF) == 0x...000)
  //     this       rhs
  return changed;
}

template <typename I> inline bool BitSet<I>::add(const BitSet<I> &rhs) {
  bool changed = false;
  for (size_t i = 0; i < wordsSize; i++) {
    auto old = words[i];
    words[i] |= rhs.words[i];
    changed |= (words[i] != old);
  }
  // padding will remain 0 since both padding are 0's
  // ((0x...000 & 0x...FFF) == 0x...000)
  //     this       rhs
  return changed;
}

template <typename I>
inline bool BitSet<I>::intersectInto(const BitSet<I> &rhs,
                                     BitSet<I> &into) const {
  bool notEmpty = false;
  for (size_t i = 0; i < wordsSize; i++) {
    into.words[i] = words[i] & rhs.words[i];
    notEmpty |= (into.words[i] != 0);
  }
  // doesn't matter if the padding matches (it'll be correct either way)
  return notEmpty;
}

template <typename I>
inline bool BitSet<I>::testAny(size_t off, size_t len) const {
  IGA_ASSERT(off >= 0 && off + len <= N,
             "BitSet::testAll: index out of bounds");

  // check the first word (misaligned mask)
  auto w_ix = off / BITS_PER_WORD;
  auto w_off = off % BITS_PER_WORD;
  size_t misaligned_len = std::min<size_t>(BITS_PER_WORD - w_off, len);
  I misaligned_mask = makeMask(misaligned_len);
  if ((words[w_ix] >> w_off) & misaligned_mask) {
    return true;
  }

  // check successive words (all aligned now)
  len -= misaligned_len;
  off += misaligned_len;
  while (len > 0) {
    w_ix++; // next word

    auto aligned_len = len >= 32 ? 32 : len % BITS_PER_WORD; // last will be <32
    auto aligned_mask = makeMask(aligned_len);
    if (words[w_ix] & aligned_mask) {
      return true;
    }
    len -= aligned_len;
    off += aligned_len;
  }
  return false;
}

template <typename I>
inline bool BitSet<I>::testAll(size_t off, size_t len) const {
  IGA_ASSERT(off >= 0 && off + len <= N,
             "BitSet::testAll: index out of bounds");

  // check the first word (misaligned mask)
  auto w_ix = off / BITS_PER_WORD;
  auto w_off = off % BITS_PER_WORD;
  auto misaligned_len = std::min<size_t>(BITS_PER_WORD - w_off, len);
  auto misaligned_mask = makeMask(misaligned_len);
  if (((words[w_ix] >> w_off) & misaligned_mask) != misaligned_mask) {
    return false;
  }

  // check successive words (all aligned now)
  len -= misaligned_len;
  off += misaligned_len;
  while (len > 0) {
    w_ix++; // next word

    auto aligned_len = len >= 32 ? 32 : len % BITS_PER_WORD; // last will be <32
    auto aligned_mask = makeMask(aligned_len);
    if ((words[w_ix] & aligned_mask) != aligned_mask) {
      return false;
    }
    len -= aligned_len;
    off += aligned_len;
  }
  return true;
}

template <typename I> bool BitSet<I>::operator==(const BitSet<I> &bs) const {
  // do we have to worry about the ragged padding.
  // e.g. given 32b words if the bitset is 31 bits long, we have to
  // worry about the trailing bit
  if (N != bs.N) { // We should not compare BitSets of different size
    return false;
  }
  return memcmp(&words[0], &bs.words[0], wordsSize * sizeof(I)) == 0;
}

// C++20 gets std::popcount
// until then, provide a couple instances of likely uses and call it a day
static inline size_t bsPopcount(uint32_t i) {
  // this should end up as __popcountsi2
  return std::bitset<32>(i).count();
}
static inline size_t bsPopcount(uint64_t i) {
  if (sizeof(unsigned long) == 8) {
    // __popcountti2
    return std::bitset<64>(i).count();
  } else {
    // __popcountsi2; __popcountsi2
    return bsPopcount((uint32_t)i) + bsPopcount((uint32_t)(i >> 32));
  }
}
static inline size_t bsPopcount(uint16_t i) { return bsPopcount((uint32_t)i); }

template <typename I> inline size_t BitSet<I>::cardinality() const {
  size_t n = 0;
  for (size_t i = 0; i < wordsSize; i++) {
    n += bsPopcount(words[i]);
  }
  return n;
}

template <typename I> inline void BitSet<I>::str(std::ostream &os) const {
  int i = 0;
  bool first = false;
  os << "{";
  while (i < N) {
    if (test(i)) {
      if (first)
        first = false;
      else
        os << ",";
      int start = i + 1;
      while (i < N && test(i)) {
        i++;
      }
      os << start;
      if (i != start + 1) {
        os << "-";
        os << i + start - 1;
      }
    } else {
      i++;
    }
  }
  os << "}";
}

} // namespace iga

#endif // _IGA_BITSET_HPP
