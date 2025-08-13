/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _BITSET_H_
#define _BITSET_H_

#include "Mem_Manager.h"
#include <cstdlib>
#include <cstring>
#include <map>

// clang-format on

// Array-based bitset implementation where each element occupies a single bit.
// Inside each array element, bits are stored and indexed from lsb to msb.
typedef unsigned int BITSET_ARRAY_TYPE;
#define BITS_PER_BYTE 8
#define _BIT(x) (((BITSET_ARRAY_TYPE)1) << x)
#define NUM_BITS_PER_ELT (sizeof(BITSET_ARRAY_TYPE) * BITS_PER_BYTE)

class BitSet {
public:
  BitSet() : m_BitSetArray(nullptr), m_Size(0) {}
  BitSet(unsigned size, bool defaultValue) {
    m_BitSetArray = NULL;
    m_Size = 0;

    create(size);
    if (defaultValue) {
      setAll();
    }
  }

  BitSet(const BitSet &other) : m_BitSetArray(nullptr), m_Size(0) {
    copy(other);
  }

  BitSet(BitSet &&other) noexcept {
    m_BitSetArray = other.m_BitSetArray;
    m_Size = other.m_Size;
    other.m_BitSetArray = nullptr;
    other.m_Size = 0;
  }

  ~BitSet() { std::free(m_BitSetArray); }

  void resize(unsigned size) { create(size); }
  void clear() {
    unsigned sizeInBytes = (m_Size + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    std::memset(m_BitSetArray, 0, sizeInBytes);
  }

  void setAll(void);
  void invert(void);

  bool isEmpty() const {
    unsigned arraySize = (m_Size + NUM_BITS_PER_ELT - 1) / NUM_BITS_PER_ELT;
    for (unsigned i = 0; i < arraySize; i++) {
      if (m_BitSetArray[i] != 0) {
        return false;
      }
    }
    return true;
  }

  bool isAllset() const {
    unsigned index;
    unsigned bound = m_Size / NUM_BITS_PER_ELT;

    for (index = 0; index < bound; index++) {
      if (~m_BitSetArray[index] != 0) {
        return false;
      }
    }

    unsigned numBitsLeft = m_Size % NUM_BITS_PER_ELT;
    for (unsigned bitIndex = 0; bitIndex < numBitsLeft; bitIndex++) {
      if ((m_BitSetArray[index] & _BIT(bitIndex)) == 0) {
        return false;
      }
    }

    return true;
  }

  bool isSet(unsigned index) const {
    if (index < m_Size) {
      unsigned arrayIndex = index / NUM_BITS_PER_ELT;
      unsigned bitIndex = index % NUM_BITS_PER_ELT;
      return (m_BitSetArray[arrayIndex] & _BIT(bitIndex)) != 0;
    }
    return false;
  }

  bool isAllSet(unsigned startIndex, unsigned endIndex) const {
    vISA_ASSERT(startIndex <= endIndex, "Invalid bitSet Index");
    vISA_ASSERT(startIndex < m_Size, "Invalid bitSet Index");
    vISA_ASSERT(endIndex < m_Size, "Invalid bitSet Index");

    unsigned start = startIndex / NUM_BITS_PER_ELT;
    unsigned end = endIndex / NUM_BITS_PER_ELT;

    if (start == end) {
      for (unsigned i = startIndex; i <= endIndex; i++) {
        if (!isSet(i)) {
          return false;
        }
      }
      return true;
    }

    unsigned index;
    unsigned numBitsBefore = startIndex % NUM_BITS_PER_ELT;
    if (numBitsBefore) {
      for (unsigned bitIndex = numBitsBefore; bitIndex < NUM_BITS_PER_ELT;
           bitIndex++) {
        if ((m_BitSetArray[start] & _BIT(bitIndex)) == 0) {
          return false;
        }
      }
      start++;
    }

    for (index = start; index < end; index++) {
      if (~m_BitSetArray[index] != 0) {
        return false;
      }
    }

    unsigned numBitsLeft = endIndex % NUM_BITS_PER_ELT;
    for (unsigned bitIndex = 0; bitIndex <= numBitsLeft; bitIndex++) {
      if ((m_BitSetArray[index] & _BIT(bitIndex)) == 0) {
        return false;
      }
    }

    return true;
  }

  bool isEmpty(unsigned startIndex, unsigned endIndex) const {
    vISA_ASSERT(startIndex <= endIndex, "Invalid bitSet Index");
    vISA_ASSERT(startIndex < m_Size, "Invalid bitSet Index");
    vISA_ASSERT(endIndex < m_Size, "Invalid bitSet Index");

    unsigned start = startIndex / NUM_BITS_PER_ELT;
    unsigned end = endIndex / NUM_BITS_PER_ELT;

    if (start == end) {
      for (unsigned i = startIndex; i <= endIndex; i++) {
        if (isSet(i)) {
          return false;
        }
      }
      return true;
    }

    unsigned index;
    unsigned numBitsBefore = startIndex % NUM_BITS_PER_ELT;
    if (numBitsBefore) {
      for (unsigned bitIndex = numBitsBefore; bitIndex < NUM_BITS_PER_ELT;
           bitIndex++) {
        if ((m_BitSetArray[start] & _BIT(bitIndex)) != 0) {
          return false;
        }
      }
      start++;
    }

    for (index = start; index < end; index++) {
      if (m_BitSetArray[index] != 0) {
        return false;
      }
    }

    unsigned numBitsLeft = endIndex % NUM_BITS_PER_ELT;
    for (unsigned bitIndex = 0; bitIndex <= numBitsLeft; bitIndex++) {
      if ((m_BitSetArray[index] & _BIT(bitIndex)) != 0) {
        return false;
      }
    }

    return true;
  }

  unsigned count() const {
    unsigned count = 0;
    unsigned arraySize = (m_Size + NUM_BITS_PER_ELT - 1) / NUM_BITS_PER_ELT;

    for (unsigned i = 0; i < arraySize; i++) {
      BITSET_ARRAY_TYPE value = m_BitSetArray[i];
      while (value) {
        ++count;
        value = value & (value - 1);
      }
    }
    return count;
  }

  BITSET_ARRAY_TYPE getElt(unsigned eltIndex) const {
    vISA_ASSERT(eltIndex < m_Size, "Invalid bitSet Index");
    return m_BitSetArray[eltIndex];
  }

  void setElt(unsigned eltIndex, BITSET_ARRAY_TYPE value) {
    unsigned bound = (eltIndex + 1) * NUM_BITS_PER_ELT;
    if (bound > m_Size) {
      create(bound);
    }
    m_BitSetArray[eltIndex] |= value;
  }

  void resetElt(unsigned eltIndex, BITSET_ARRAY_TYPE value) {
    unsigned bound = (eltIndex + 1) * NUM_BITS_PER_ELT;
    if (bound > m_Size) {
      create(bound);
    }
    m_BitSetArray[eltIndex] &= ~value;
  }

  void set(unsigned index, bool value) {
    // If the index is larger than the size of the BitSet then grow the BitSet
    if (index >= m_Size) {
      create(index + 1);
    }

    unsigned arrayIndex = index / NUM_BITS_PER_ELT;
    unsigned bitIndex = index % NUM_BITS_PER_ELT;

    if (value) {
      m_BitSetArray[arrayIndex] |= _BIT(bitIndex);
    } else {
      m_BitSetArray[arrayIndex] &= ~_BIT(bitIndex);
    }
  }

  void set(unsigned startIndex, unsigned endIndex) {
    for (unsigned i = startIndex; i <= endIndex; i++) {
      set(i, true);
    }
  }

  unsigned getSize() const { return m_Size; }

  bool operator==(const BitSet &other) const {
    if (m_Size == other.m_Size) {
      if (m_Size == 0) {
          return true;
      }
      unsigned sizeInBytes = (m_Size + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
      return 0 == std::memcmp(m_BitSetArray, other.m_BitSetArray, sizeInBytes);
    }
    return false;
  }

  bool operator!=(const BitSet &other) const {
    if (m_Size == other.m_Size) {
      if (m_Size == 0) {
          return false;
      }
      unsigned sizeInBytes = (m_Size + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
      return 0 != std::memcmp(m_BitSetArray, other.m_BitSetArray, sizeInBytes);
    }
    return true;
  }

  BitSet &operator=(const BitSet &other) {
    copy(other);
    return *this;
  }

  BitSet &operator=(BitSet &&other) noexcept {
    if (this == &other) {
          return *this;
    }
    if (m_BitSetArray) {
      std::free(m_BitSetArray);
    }
    m_BitSetArray = other.m_BitSetArray;
    m_Size = other.m_Size;
    other.m_BitSetArray = nullptr;
    other.m_Size = 0;

    return *this;
  }

  void swap(BitSet &other) {
    if (this != &other) {
      std::swap(m_Size, other.m_Size);
      std::swap(m_BitSetArray, other.m_BitSetArray);
    }
  }

  BitSet &operator|=(const BitSet &other);
  BitSet &operator&=(const BitSet &other);
  BitSet &operator-=(const BitSet &other);

  void *operator new(size_t sz, vISA::Mem_Manager &m) { return m.alloc(sz); }

  // Return the index of the first set bit in the range [begin, end).
  // Return -1 if all bits in the range are unset.
  int findFirstIn(unsigned begin, unsigned end) const;
  // Return the index of the last set bit in the range [begin, end).
  // Return -1 if all bits in the range are unset.
  int findLastIn(unsigned begin, unsigned end) const;

protected:
  BITSET_ARRAY_TYPE *m_BitSetArray;
  unsigned m_Size;

  void create(unsigned size);
  void copy(const BitSet &other) {
    unsigned sizeInBytes = (other.m_Size + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    if (this != &other) {
      if (m_Size == other.m_Size) {
        memcpy_s(m_BitSetArray, sizeInBytes, other.m_BitSetArray, sizeInBytes);
      } else {
        create(other.m_Size);
        memcpy_s(m_BitSetArray, sizeInBytes, other.m_BitSetArray, sizeInBytes);
      }
    }
  }
};

template <unsigned Size> class FixedBitSet {
  static const unsigned WordBitSize = NUM_BITS_PER_ELT;
  static const unsigned NumWords = (Size + WordBitSize - 1) / WordBitSize;
  BITSET_ARRAY_TYPE Bits[NumWords];

protected:
  // Helper function to map a bit index into its segment index.
  std::pair<unsigned, unsigned> bitToWordPair(unsigned Bit) const {
    return std::make_pair(Bit / WordBitSize, Bit % WordBitSize);
  }

public:
  FixedBitSet() {
    for (unsigned i = 0; i < NumWords; ++i)
      Bits[i] = 0;
  }

  bool isSet(unsigned Bit) const {
    unsigned Word, BitInWord;
    std::tie(Word, BitInWord) = bitToWordPair(Bit);
    return (Bits[Word] & _BIT(BitInWord)) != 0;
  }

  bool isEmpty() const {
    for (unsigned i = 0; i < NumWords; ++i)
      if (Bits[i])
        return false;
    return true;
  }

  BITSET_ARRAY_TYPE getElt(unsigned Elt) const {
    vISA_ASSERT(Elt < NumWords, "Invalid FixedBitSet Element Index");
    return Bits[Elt];
  }

  void set(unsigned Bit, bool Val) {
    unsigned Word, BitInWord;
    std::tie(Word, BitInWord) = bitToWordPair(Bit);
    if (Val)
      Bits[Word] |= _BIT(BitInWord);
    else
      Bits[Word] &= ~_BIT(BitInWord);
  }

  bool operator!=(const FixedBitSet &Other) const {
    for (unsigned i = 0; i < NumWords; ++i)
      if (Bits[i] != Other.Bits[i])
        return true;
    return false;
  }

  FixedBitSet &operator&=(const FixedBitSet &Other) {
    for (unsigned i = 0; i < NumWords; ++i)
      Bits[i] &= Other.Bits[i];
    return *this;
  }

  FixedBitSet &operator|=(const FixedBitSet &Other) {
    for (unsigned i = 0; i < NumWords; ++i)
      Bits[i] |= Other.Bits[i];
    return *this;
  }

  FixedBitSet &operator-=(const FixedBitSet &Other) {
    for (unsigned i = 0; i < NumWords; ++i)
      Bits[i] &= ~Other.Bits[i];
    return *this;
  }
};

// SparseBitSet is an implementation of a bit set where most bits are zeros. It
// reduces the memory and lookup overhead by storing elements with
// corresponding ones.
class SparseBitSet {
  // SparseBitSet is a collection of segments, i.e. a BitSet with fixed size,
  // says 64, 128 or 256 bits. That collection is organized as a
  // self-balanced tree to speed up the lookup and insertion.
  static const unsigned SegmentBitSize = 2048;
  static const unsigned SegmentEltSize = SegmentBitSize / NUM_BITS_PER_ELT;
  // `std::map` is used as the container to prevent reinventing the wheel as
  // `std::map` is usually implemented as red-black trees, one kind of
  // self-balanced binary search trees
  std::map<unsigned, FixedBitSet<SegmentBitSize>> Segments;

  unsigned MaxBits;

protected:
  // Helper function to map a bit index into its segment index.
  std::pair<unsigned, unsigned> bitToSegPair(unsigned Bit) const {
    return std::make_pair(Bit / SegmentBitSize, Bit % SegmentBitSize);
  }

  // Helper function to map a elt index into its segment index.
  std::pair<unsigned, unsigned> eltToSegPair(unsigned Elt) const {
    return std::make_pair(Elt / SegmentEltSize, Elt % SegmentEltSize);
  }

  // Helper function to round a size of bits up to the size of segments
  // required.
  unsigned roundUpToSegments(unsigned Bits) const {
    return (Bits + SegmentBitSize - 1) / SegmentBitSize;
  }

public:
  SparseBitSet(unsigned Bits = 0) : MaxBits(Bits) {}
  SparseBitSet(const SparseBitSet &Other)
      : Segments(Other.Segments), MaxBits(Other.MaxBits) {}
  SparseBitSet(const SparseBitSet &&Other)
      : Segments(std::move(Other.Segments)), MaxBits(Other.MaxBits) {}

  ~SparseBitSet() = default;

  unsigned getSize() const { return MaxBits; }

  void clear() { Segments.clear(); }
  void resize(unsigned Bits) {
    unsigned Segs = roundUpToSegments(Bits);
    if (Segs < roundUpToSegments(MaxBits)) {
      for (auto I = Segments.begin(), E = Segments.end(); I != E; /*EMPTY*/) {
        if (I->first < Segs) {
          // Skip segments in range.
          ++I;
          continue;
        }
        // Erase segments beyond.
        I = Segments.erase(I);
      }
    }
    MaxBits = Bits;
  }

  class SparseBitSetIterator {
    SparseBitSet *Set;
    std::map<unsigned, FixedBitSet<SegmentBitSize>>::const_iterator MI;
    std::map<unsigned, FixedBitSet<SegmentBitSize>>::const_iterator ME;
    BITSET_ARRAY_TYPE CachedWord;
    unsigned Elt; // The elt number in that segment.
    unsigned Bit; // The bit number in that element.

  protected:
    bool isAtEnd() const { return MI == ME; }
    // Advance to the next bit set in the cached word.
    int advanceToNextBit(int Bit) {
      if ((Bit + 1) < NUM_BITS_PER_ELT) {
        unsigned TrailingMask = (~0U) << (Bit + 1);
        unsigned Word = CachedWord & TrailingMask;
        if (Word) {
#if defined(_MSC_VER)
          unsigned long trailing_zeros;
          _BitScanForward(&trailing_zeros, (unsigned long)Word);
          return trailing_zeros;
#else
          return __builtin_ctz(Word);
#endif
        }
      }
      return -1;
    }

  public:
    SparseBitSetIterator() = default;
    SparseBitSetIterator(SparseBitSet *B, bool End = false) : Set(B) {
      ME = Set->Segments.end();
      MI = End ? ME : Set->Segments.begin();
      if (!End && !isAtEnd()) {
        while (MI != ME) {
          Bit = NUM_BITS_PER_ELT;
          Elt = 0;
          for (; Elt < SegmentEltSize; ++Elt) {
            CachedWord = MI->second.getElt(Elt);
            if (CachedWord) {
              int NextBit = advanceToNextBit(-1);
              vISA_ASSERT(0 <= NextBit && NextBit < NUM_BITS_PER_ELT,
                          "Non-zero word has no bit set or out of range bit!");
              Bit = NextBit;
              break;
            }
          }
          if (Bit == NUM_BITS_PER_ELT) { // empty segment
            MI = Set->Segments.erase(MI);
          } else {
            break;
          }
        }
      }
    }

    unsigned operator*() const {
      return (MI->first * SegmentBitSize) + (Elt * NUM_BITS_PER_ELT) + Bit;
    }

    bool operator==(const SparseBitSetIterator &Other) {
      if (isAtEnd() && Other.isAtEnd())
        return true;
      if (MI != Other.MI)
        return false;
      return (Elt == Other.Elt) && (Bit == Other.Bit);
    }

    bool operator!=(const SparseBitSetIterator &Other) {
      return !(*this == Other);
    }

    SparseBitSetIterator &operator++() {
      if (isAtEnd())
        return *this;
      // Advance to the next bit set.
      int NextBit = advanceToNextBit(Bit);
      if (NextBit > 0) {
        Bit = NextBit;
        return *this;
      }
      // Advance to the next element and/or segment.
      ++Elt;
      do {
        bool startFromZero = (Elt == 0);
        Bit = NUM_BITS_PER_ELT;
        for (; Elt < SegmentEltSize; ++Elt) {
          CachedWord = MI->second.getElt(Elt);
          if (CachedWord) {
            int NextBit = advanceToNextBit(-1);
            vISA_ASSERT(0 <= NextBit && NextBit < NUM_BITS_PER_ELT,
                         "Non-zero word has no bit set or out of range bit!");
            Bit = NextBit;
            return *this;
          }
        }
        // Advance to the next segment.
        if (startFromZero && Bit == NUM_BITS_PER_ELT) {
          MI = Set->Segments.erase(MI);
        } else {
          ++MI;
        }
        Elt = 0;
      } while (!isAtEnd());
      return *this;
    }

    SparseBitSetIterator operator++(int) {
      SparseBitSetIterator Tmp = *this;
      ++(*this);
      return Tmp;
    }
  };

  using iterator = SparseBitSetIterator;

  iterator begin() { return iterator(this); }
  iterator end() { return iterator(this, true); }

  bool isSet(unsigned Bit) const {
    if (Bit >= MaxBits)
      return false;
    unsigned Seg, BitInSeg;
    std::tie(Seg, BitInSeg) = bitToSegPair(Bit);
    auto I = Segments.find(Seg);
    if (I == Segments.end())
      return false;
    return I->second.isSet(BitInSeg);
  }

  void set(unsigned Bit, bool Val) {
    // Ignore if just to clear the bit beyond.
    if (Bit >= MaxBits && !Val)
      return;
    MaxBits = std::max(MaxBits, Bit + 1);
    unsigned Seg, BitInSeg;
    std::tie(Seg, BitInSeg) = bitToSegPair(Bit);
    auto I = Segments.find(Seg);
    if (I == Segments.end()) {
      // Ignore if just to clear the bit not present.
      if (!Val)
        return;
      I = Segments.emplace(Seg, FixedBitSet<SegmentBitSize>()).first;
    }
    I->second.set(BitInSeg, Val);
  }

  // TODO: Based on the current usage, `getElt` is an interface to retrieve
  // more bits to save the overhead of set access. For a sparse bitset, a
  // more convenient approach would be the use of find_first and find_next.
  BITSET_ARRAY_TYPE getElt(unsigned Elt) const {
    unsigned Seg, EltInSeg;
    std::tie(Seg, EltInSeg) = eltToSegPair(Elt);
    auto I = Segments.find(Seg);
    if (I == Segments.end())
      return 0;
    return I->second.getElt(EltInSeg);
  }

  SparseBitSet &operator=(const SparseBitSet &Other) {
    if (this == &Other)
      return *this;
    Segments = Other.Segments;
    MaxBits = Other.MaxBits;
    return *this;
  }
  SparseBitSet &operator=(SparseBitSet &&Other) {
    Segments = std::move(Other.Segments);
    MaxBits = Other.MaxBits;
    return *this;
  }

  SparseBitSet &operator&=(const SparseBitSet &Other) {
    auto I = Segments.begin(), E = Segments.end();
    // Skip when this is empty.
    if (I == E)
      return *this;
    // Scan this and other simultaneously.
    auto OI = Other.Segments.begin(), OE = Other.Segments.end();
    while (I != E) {
      if (OI == OE || OI->first > I->first) {
        // Erase unmatching segments from this directly.
        I = Segments.erase(I);
        continue;
      }
      if (OI->first == I->first) {
        // Apply `and` on the matching segment.
        I->second &= OI->second;
        if (I->second.isEmpty())
          I = Segments.erase(I);
        else
          ++I;
        ++OI;
        continue;
      }
      // Advance other cursor.
      while (OI != OE && OI->first < I->first)
        ++OI;
    }
    // Erase all remaining segments.
    while (I != E)
      I = Segments.erase(I);
    MaxBits = std::min(MaxBits, Other.MaxBits);
    return *this;
  }

  SparseBitSet &operator|=(const SparseBitSet &Other) {
    auto OI = Other.Segments.begin(), OE = Other.Segments.end();
    // Skip when the other is empty.
    if (OI == OE)
      return *this;
    auto I = Segments.begin(), E = Segments.end();
    // Scan this and other simultaneously.
    while (OI != OE) {
      if (I == E || I->first > OI->first) {
        // Copy unmatching segments from other directly.
        Segments.emplace(OI->first, OI->second);
        ++OI;
        continue;
      }
      if (I->first == OI->first) {
        // Apply `or` on the matching segment.
        I->second |= OI->second;
        ++OI;
        ++I;
        continue;
      }
      // Advance this cursor.
      while (I != E && I->first < OI->first)
        ++I;
    }
    MaxBits = std::max(MaxBits, Other.MaxBits);
    return *this;
  }

  SparseBitSet &operator-=(const SparseBitSet &Other) {
    auto OI = Other.Segments.begin(), OE = Other.Segments.end();
    auto I = Segments.begin(), E = Segments.end();
    // Skip when either this or other is empty.
    if (OI == OE || I == E)
      return *this;
    // Scan two sparse bitsets simultaneously.
    while (I != E && OI != OE) {
      if (OI->first == I->first) {
        // Apply 'sub' on the matching segment.
        I->second -= OI->second;
        if (I->second.isEmpty())
          I = Segments.erase(I);
        else
          ++I;
        ++OI;
        continue;
      }
      // Advance this cursor.
      while (I != E && I->first < OI->first)
        ++I;
      if (I == E)
        break;
      // Advance other cursor.
      while (OI != OE && OI->first < I->first)
        ++OI;
      if (OI == OE)
        break;
    }
    return *this;
  }

  bool operator!=(const SparseBitSet &Other) const {
    // Two sets are obviously not equal if they have different sizes.
    if (Segments.size() != Other.Segments.size())
      return true;
    auto I = Segments.begin(), E = Segments.end();
    auto OI = Other.Segments.begin(), OE = Other.Segments.end();
    // Scan two sparse bitsets simultaneously.
    for (; I != E && OI != OE; ++I, ++OI) {
      // Not equal if there are unmatching segments.
      if (I->first != OI->first)
        return true;
      // Check matching segments.
      if (I->second != OI->second)
        return true;
    }
    // Not equal if either one has remaining segments.
    return I != E || OI != OE;
  }

  class SparseBitSetAndIterator {
    const SparseBitSet *LHS, *RHS;
    std::map<unsigned, FixedBitSet<SegmentBitSize>>::const_iterator LI, RI;
    std::map<unsigned, FixedBitSet<SegmentBitSize>>::const_iterator LE, RE;
    BITSET_ARRAY_TYPE CachedWord; // Cached result from the matching elements.
    unsigned Elt, Bit;

  protected:
    bool isAtEnd() const { return LI == LE || RI == RE; }
    // Advance to the next bit set in the cached word.
    int advanceToNextBit(int Bit) {
      if ((Bit + 1) < NUM_BITS_PER_ELT) {
        unsigned TrailingMask = (~0U) << (Bit + 1);
        unsigned Word = CachedWord & TrailingMask;
        if (Word) {
#if defined(_MSC_VER)
          unsigned long trailing_zeros;
          _BitScanForward(&trailing_zeros, (unsigned long)Word);
          return trailing_zeros;
#else
          return __builtin_ctz(Word);
#endif
        }
      }
      return -1;
    }

  public:
    SparseBitSetAndIterator() = default;
    SparseBitSetAndIterator(const SparseBitSet *L, const SparseBitSet *R,
                            bool End = false)
        : LHS(L), RHS(R) {
      LE = LHS->Segments.end();
      RE = RHS->Segments.end();
      LI = End ? LE : LHS->Segments.begin();
      RI = End ? RE : RHS->Segments.begin();
      if (!End) {
        while (!isAtEnd()) {
          if (LI->first == RI->first) {
            Bit = NUM_BITS_PER_ELT;
            Elt = 0;
            for (; Elt < SegmentEltSize; ++Elt) {
              unsigned LW = LI->second.getElt(Elt);
              unsigned RW = RI->second.getElt(Elt);
              CachedWord = LW & RW;
              if (CachedWord) {
                int NextBit = advanceToNextBit(-1);
                vISA_ASSERT(
                    0 <= NextBit && NextBit < NUM_BITS_PER_ELT,
                    "Non-zero word has no bit set or out of range bit!");
                Bit = NextBit;
                break;
              }
            }
            if (Bit < NUM_BITS_PER_ELT)
              break;
            // Matching segments have no intersection.
            ++LI;
            ++RI;
          }
          // Advance LHS to match RHS.
          if (RI != RE)
              for (; LI != LE && LI->first < RI->first; ++LI)
                  ;
          // Advance RHS to match LHS.
          if (LI != LE)
              for (; RI != RE && RI->first < LI->first; ++RI)
                ;
        }
      }
    }

    unsigned operator*() const {
      // This operation is only valid when there are matching segments.
      // '&' is a no-op for matching segments but it causes invalid
      // memory references if either LI or RI is at end.
      return ((LI->first & RI->first) * SegmentBitSize) +
             (Elt * NUM_BITS_PER_ELT) + Bit;
    }

    bool operator==(const SparseBitSetAndIterator &Other) const {
      if (isAtEnd() && Other.isAtEnd())
        return true;
      if (LI != Other.LI || RI != Other.RI)
        return false;
      return (Elt == Other.Elt) && (Bit == Other.Bit);
    }

    bool operator!=(const SparseBitSetAndIterator &Other) const {
      return !(*this == Other);
    }

    SparseBitSetAndIterator &operator++() {
      if (isAtEnd())
        return *this;
      // Advance to the next bit set.
      int NextBit = advanceToNextBit(Bit);
      if (NextBit > 0) {
        Bit = NextBit;
        return *this;
      }
      // Advance to the next element and/or segment.
      Bit = NUM_BITS_PER_ELT;
      ++Elt;
      do {
        if (LI->first == RI->first) {
          for (; Elt < SegmentEltSize; ++Elt) {
            unsigned LW = LI->second.getElt(Elt);
            unsigned RW = RI->second.getElt(Elt);
            CachedWord = LW & RW;
            if (CachedWord) {
              int NextBit = advanceToNextBit(-1);
              vISA_ASSERT(0 <= NextBit && NextBit < NUM_BITS_PER_ELT,
                           "Non-zero word has no bit set or out of range bit!");
              Bit = NextBit;
              return *this;
            }
          }
          // Matching segments have no intersection.
          ++LI;
          ++RI;
        }
        Elt = 0;
        // Advance LHS to match RHS if the later is not at the end.
        if (RI != RE)
          for (; LI != LE && LI->first < RI->first; ++LI)
            ;
        // Advance RHS to match LHS if the later is not at the end.
        if (LI != LE)
          for (; RI != RE && RI->first < LI->first; ++RI)
            ;
      } while (!isAtEnd());
      return *this;
    }

    SparseBitSetAndIterator operator++(int) {
      SparseBitSetAndIterator Tmp = *this;
      ++(*this);
      return Tmp;
    }
  };

  using and_iterator = SparseBitSetAndIterator;

  and_iterator and_begin(const SparseBitSet &Other) const {
    return and_iterator(this, &Other);
  }
  and_iterator and_end(const SparseBitSet &Other) const {
    return and_iterator(this, &Other, true);
  }
};

#endif
