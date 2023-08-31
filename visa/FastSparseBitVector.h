/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvm/Support/MathExtras.h"

#ifndef _FASTSPARSEVECTOR_H_
#define _FASTSPARSEVECTOR_H_

// Following implementation of sparse bitvector is inspired from llvm's
// SparseBitVector. There are some changes made to make it faster.
// llvm's version stores SparseBitVectorElement instances as a linked
// list and caches an iterator for faster lookup. We modified this so
// SparseBitVector instances are stored in an std::vector<unique_ptr>>
// container. std::vector makes lookup faster. We trade speed for
// some extra memory here as some elements of std::vector may be
// unpopulated.

template <unsigned ElementSize = 128> struct SparseBitVectorElement {
public:
  using BitWord = uint64_t;
  using size_type = unsigned;
  enum {
    BITWORD_SIZE = sizeof(BitWord) * CHAR_BIT,
    BITWORDS_PER_ELEMENT = (ElementSize + BITWORD_SIZE - 1) / BITWORD_SIZE,
    BITS_PER_ELEMENT = ElementSize
  };

private:
  // Index of Element in terms of where first bit starts
  unsigned ElementIndex;
  BitWord Bits[BITWORDS_PER_ELEMENT];

  SparseBitVectorElement() {
    ElementIndex = ~0U;
    memset(&Bits[0], 0, sizeof(BitWord) * BITWORDS_PER_ELEMENT);
  }

public:
  explicit SparseBitVectorElement(unsigned Idx) {
    ElementIndex = Idx;
    memset(&Bits[0], 0, sizeof(BitWord) * BITWORDS_PER_ELEMENT);
  }

  // Comparison
  bool operator==(const SparseBitVectorElement &RHS) const {
    if (ElementIndex != RHS.ElementIndex)
      return false;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i)
      if (Bits[i] != RHS.Bits[i])
        return false;
    return true;
  }

  bool operator!=(const SparseBitVectorElement &RHS) const {
    return !(*this == RHS);
  }

  // Return the bits that make up word Idx in our element
  BitWord word(unsigned Idx) const {
    vISA_ASSERT(Idx < BITWORDS_PER_ELEMENT, "Idx OOB");
    return Bits[Idx];
  }

  unsigned index() const { return ElementIndex; }

  bool empty() const {
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i)
      if (Bits[i])
        return false;
    return true;
  }

  void set(unsigned Idx) {
    Bits[Idx / BITWORD_SIZE] |= (BitWord)1 << (Idx % BITWORD_SIZE);
  }

  bool test_and_set(unsigned Idx) {
    bool old = test(Idx);
    if (!old) {
      set(Idx);
      return true;
    }
    return false;
  }

  bool reset(unsigned Idx) {
    auto old = test(Idx);
    Bits[Idx / BITWORD_SIZE] &= ~((BitWord)1 << (Idx % BITWORD_SIZE));
    return old;
  }

  bool test(unsigned Idx) const {
    return Bits[Idx / BITWORD_SIZE] & ((BitWord)1 << (Idx % BITWORD_SIZE));
  }

  size_type count() const {
    unsigned NumBits = 0;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i)
      NumBits += llvm::countPopulation(Bits[i]);
    return NumBits;
  }

  /// find_first - Returns the index of the first set bit.
  int find_first() const {
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i)
      if (Bits[i] != 0)
        return i * BITWORD_SIZE + llvm::countTrailingZeros(Bits[i]);
    vISA_ASSERT(false, "Illegal empty element");
    // Following should be unreachable
    return -1;
  }

  /// find_last - Returns the index of the last set bit
  int find_last() const {
    for (unsigned I = 0; I < BITWORDS_PER_ELEMENT; ++I) {
      unsigned Idx = BITWORDS_PER_ELEMENT - I - 1;
      if (Bits[Idx] != 0)
        return Idx * BITWORD_SIZE + BITWORD_SIZE -
               countLeadingZeros(Bits[Idx]) - 1;
    }
    vISA_ASSERT(false, "Illegal empty element");
    // Following should be unreachable
    return -1;
  }

  /// find_next - Returns the index of the next set bit starting from the
  /// "Curr" bit. Returns -1 if the next set bit is not found.
  int find_next(unsigned Curr) const {
    if (Curr >= BITS_PER_ELEMENT)
      return -1;

    unsigned WordPos = Curr / BITWORD_SIZE;
    unsigned BitPos = Curr % BITWORD_SIZE;
    BitWord Copy = Bits[WordPos];
    vISA_ASSERT(WordPos <= BITWORDS_PER_ELEMENT,
                "Word Position outside of element");

    // Mask off previous bits
    Copy &= (~((BitWord)0)) << (BitWord)BitPos;

    if (Copy != 0)
      return WordPos * BITWORD_SIZE + llvm::countTrailingZeros(Copy);

    // Check subsequent words.
    for (unsigned i = WordPos + 1; i < BITWORDS_PER_ELEMENT; ++i)
      if (Bits[i] != 0)
        return i * BITWORD_SIZE + llvm::countTrailingZeros(Bits[i]);
    return -1;
  }

  // Union (bitwise OR) this element with RHS
  void unionWith(const SparseBitVectorElement &RHS) {
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      Bits[i] |= RHS.Bits[i];
    }
  }

  // Return true if we have any bits in common with RHS
  bool intersects(const SparseBitVectorElement &RHS) const {
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      if (RHS.Bits[i] & Bits[i])
        return true;
    }
    return false;
  }

  // Intersect this Element with RHS and return true if this one changed.
  // BecameZero is set to true if this element became all-zero bits.
  bool intersectWith(const SparseBitVectorElement<ElementSize> &RHS,
                     bool &BecameZero) {
    bool changed = false;
    bool allzero = true;

    BecameZero = false;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      BitWord old = changed ? 0 : Bits[i];

      Bits[i] &= RHS.Bits[i];
      if (Bits[i] != 0)
        allzero = false;

      if (!changed && old != Bits[i])
        changed = true;
    }
    BecameZero = allzero;
    return changed;
  }

  // Intersect this Element with the complement of RHS and return true if this
  // one changed.  BecameZero is set to true if this element became all-zero
  // bits.
  bool intersectWithComplement(const SparseBitVectorElement &RHS,
                               bool &BecameZero) {
    bool changed = false;
    bool allzero = true;

    BecameZero = false;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      BitWord old = changed ? 0 : Bits[i];

      Bits[i] &= ~RHS.Bits[i];
      if (Bits[i] != 0)
        allzero = false;

      if (!changed && old != Bits[i])
        changed = true;
    }
    BecameZero = allzero;
    return changed;
  }

  // Three argument version of intersectWithComplement that intersects
  // RHS1 & ~RHS2 into this element
  void intersectWithComplement(const SparseBitVectorElement &RHS1,
                               const SparseBitVectorElement &RHS2,
                               bool &BecameZero) {
    bool allzero = true;

    BecameZero = false;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      Bits[i] = RHS1.Bits[i] & ~RHS2.Bits[i];
      if (Bits[i] != 0)
        allzero = false;
    }
    BecameZero = allzero;
  }
};

// Following class implements logic for sparse bitvector. It stores
// bitset in chunks (ie, instances of SparseBitVectorElement). If
// any bit is set in a chunk, an instance of SparseBitVectorElement
// is created. A SparseBitVectorElement with all bits 0 cannot exist
// in this implementation. Instances of SparseBitVectorElement are
// stored in Elements member below. The vector contains ordered
// storage such that ith element of vector contains chunk with bitset
// starting at i * chunk-bit-size. It is legal to have an empty
// unique_ptr in middle of the vector. It is illegal to have an empty
// unique_ptr at end of the vector. Book keeping is done so that any
// time all bits of last chunk in vector become 0, we pop the vector.
template <unsigned ElementSize = 128> class FastSparseBitVector {
  using ElementVec =
      std::vector<std::unique_ptr<SparseBitVectorElement<ElementSize>>>;
  enum { BITWORD_SIZE = SparseBitVectorElement<ElementSize>::BITWORD_SIZE };

  // Rules of Elements vector;
  // 1. If an element becomes all 0s then the instance must be destroyed,
  //    ie its unique pointer must be reset.
  // 2. If vector is non-empty then last element must not be nullptr.
  ElementVec Elements;

  // Book keeping so that we never have empty chunks at back of vector
  void emptyTrailing() {
    // Remove trailing SparseBitVectorElements
    while (!Elements.empty() && !Elements.back())
      Elements.pop_back();
  }

  // Expensive utility function, currently not invoked from anywhere
  void sanitize() {
    // Release memory for empty SparseBitVectorElements
    for (unsigned int i = 0; i != Elements.size(); ++i) {
      if (Elements[i] && Elements[i]->empty())
        Elements[i].reset();
    }

    emptyTrailing();
  }

 class FastSparseBitVectorIterator {
  private:
    bool AtEnd = false;
    const FastSparseBitVector<ElementSize> *BitVector = nullptr;
    // Current element inside of bitmap
    unsigned int Iter = 0;
    // Current bit number inside of our bitmap
    unsigned BitNumber = 0;
    // Current word number inside of our element
    unsigned WordNumber = 0;
    // Current bits from the element
    typename SparseBitVectorElement<ElementSize>::BitWord Bits;
    // Move our iterator to the first non-zero bit in the bitmap
    void AdvanceToFirstNonZero() {
      if (AtEnd)
        return;
      if (BitVector->empty()) {
        AtEnd = true;
        return;
      }
      Iter = 0;
      while (!BitVector->Elements[Iter])
        ++Iter;
      BitNumber = BitVector->Elements[Iter]->index() * ElementSize;
      unsigned BitPos = BitVector->Elements[Iter]->find_first();
      BitNumber += BitPos;
      WordNumber = (BitNumber % ElementSize) / BITWORD_SIZE;
      Bits = BitVector->Elements[Iter]->word(WordNumber);
      Bits >>= BitPos % BITWORD_SIZE;
    }
    // Move our iterator to the next non-zero bit
    void AdvanceToNextNonZero() {
      if (AtEnd)
        return;
      while (Bits && !(Bits & 1)) {
        Bits >>= 1;
        BitNumber += 1;
      }
      // See if we ran out of Bits in this word
      if (!Bits) {
        int NextSetBitNumber =
            BitVector->Elements[Iter]->find_next(BitNumber % ElementSize);
        // If we ran out of set bits in this element, move to next element.
        if (NextSetBitNumber == -1 || (BitNumber % ElementSize == 0)) {
          if (BitVector->Elements.size() == ++Iter) {
            AtEnd = true;
            return;
          }
          while (!BitVector->Elements[Iter])
            ++Iter;
          WordNumber = 0;

          // Set up for next non-zero word in bitmap
          BitNumber = BitVector->Elements[Iter]->index() * ElementSize;
          NextSetBitNumber = BitVector->Elements[Iter]->find_first();
          BitNumber += NextSetBitNumber;
          WordNumber = (BitNumber % ElementSize) / BITWORD_SIZE;
          Bits = BitVector->Elements[Iter]->word(WordNumber);
          Bits >>= NextSetBitNumber % BITWORD_SIZE;

        } else {
          WordNumber = (NextSetBitNumber % ElementSize) / BITWORD_SIZE;
          Bits = BitVector->Elements[Iter]->word(WordNumber);
          Bits >>= NextSetBitNumber % BITWORD_SIZE;
          BitNumber = BitVector->Elements[Iter]->index() * ElementSize;
          BitNumber += NextSetBitNumber;
        }
      }
    }

  public:
    FastSparseBitVectorIterator() = delete;
    FastSparseBitVectorIterator(
        const FastSparseBitVector<ElementSize> *RHS,
                            bool end = false)
        : BitVector(RHS) {
      if (!RHS || RHS->empty()) {
        AtEnd = true;
        return;
      }
      Iter = 0;
      BitNumber = 0;
      Bits = 0;
      WordNumber = ~0;
      AtEnd = end;
      AdvanceToFirstNonZero();
    }
    // Preincrement
    inline FastSparseBitVectorIterator &operator++() {
      ++BitNumber;
      Bits >>= 1;
      AdvanceToNextNonZero();
      return *this;
    }
    // Postincrement
    inline FastSparseBitVectorIterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }
    // Return the current set bit number
    unsigned operator*() const { return BitNumber; }
    bool operator==(const FastSparseBitVectorIterator &RHS) const {
      // If they are both at the end, ignore the rest of the fields
      if (AtEnd && RHS.AtEnd)
        return true;
      // Otherwise they are the same if they have the same bit number and
      // bitmap
      return AtEnd == RHS.AtEnd && RHS.BitNumber == BitNumber;
    }
    bool operator!=(const FastSparseBitVectorIterator &RHS) const {
      return !(*this == RHS);
    }
  };

public:
  using iterator = FastSparseBitVectorIterator;

  FastSparseBitVector() : Elements() {}

  FastSparseBitVector(const FastSparseBitVector &RHS) {
    Elements.resize(RHS.Elements.size());
    for (unsigned int i = 0; i != RHS.Elements.size(); ++i) {
      const auto &RHSElem = RHS.Elements[i];
      if (!RHSElem)
        continue;
      Elements[i] =
          std::make_unique<SparseBitVectorElement<ElementSize>>(*RHSElem);
    }
  }

  FastSparseBitVector(FastSparseBitVector &&RHS)
      : Elements(std::move(RHS.Elements)) {}

  // Clear
  void clear() { Elements.clear(); }

  // Assignment
  FastSparseBitVector &operator=(const FastSparseBitVector &RHS) {
    if (this == &RHS)
      return *this;

    clear();

    Elements.resize(RHS.Elements.size());
    for (unsigned int i = 0; i != RHS.Elements.size(); ++i) {
      const auto &RHSElem = RHS.Elements[i];
      if (!RHSElem)
        continue;
      Elements[i] =
          std::make_unique<SparseBitVectorElement<ElementSize>>(*RHSElem);
    }

    return *this;
  }
  FastSparseBitVector &operator=(FastSparseBitVector &&RHS) {
    Elements = std::move(RHS.Elements);
    return *this;
  }

  // Test, Reset, and Set a bit in the bitmap.
  bool test(unsigned Idx) const {
    unsigned ElementIndex = Idx / ElementSize;
    if (Elements.size() <= ElementIndex)
      return false;
    const auto &Elem = Elements[ElementIndex];
    if (Elem)
      return Elem->test(Idx % ElementSize);

    return false;
  }

  void reset(unsigned Idx) {
    unsigned ElementIndex = Idx / ElementSize;
    if (Elements.size() <= ElementIndex)
      return;
    auto &Elem = Elements[ElementIndex];
    // Elem doesn't exist, no need to create new one
    if (!Elem)
      return;

    if (Elem->reset(Idx % ElementSize)) {
      // When the element is zeroed out, delete it
      if (Elem->empty()) {
        Elem.reset();
      }
    }

    if (ElementIndex == Elements.size() - 1)
      emptyTrailing();
  }

  void set(unsigned Idx) {
    unsigned ElementIndex = Idx / ElementSize;
    if (Elements.size() <= ElementIndex)
      Elements.resize(ElementIndex + 1);
    auto &Elem = Elements[ElementIndex];
    if (!Elem) {
      Elem = std::make_unique<SparseBitVectorElement<ElementSize>>(ElementIndex);
    }
    Elem->set(Idx % ElementSize);
  }

  bool test_and_set(unsigned Idx) {
    bool old = test(Idx);
    if (!old) {
      set(Idx);
      return true;
    }
    return false;
  }

  bool operator!=(const FastSparseBitVector &RHS) const {
    return !(*this == RHS);
  }

  bool operator==(const FastSparseBitVector &RHS) const {
    if (RHS.Elements.size() != Elements.size())
      return false;

    for (unsigned int i = 0; i != Elements.size(); ++i) {
      const auto &LHSElem = Elements[i];
      const auto &RHSElem = RHS.Elements[i];

      if (!LHSElem && !RHSElem)
        continue;
      else if (!LHSElem && RHSElem)
        return false;
      else if (!RHSElem && LHSElem)
        return false;
      else if (*LHSElem != *RHSElem)
        return false;
    }

    return true;
  }

  // Union our bitmap with the RHS
  void operator|=(const FastSparseBitVector &RHS) {
    if (this == &RHS)
      return;

    auto maxSize = std::max(Elements.size(), RHS.Elements.size());
    for (unsigned int i = 0; i != maxSize; ++i) {
      if (Elements.size() <= i) {
        if (RHS.Elements[i]) {
          Elements.resize(i + 1);
          Elements[i] = std::make_unique<SparseBitVectorElement<ElementSize>>(
              *RHS.Elements[i]);
        }
        continue;
      }
      if (RHS.Elements.size() <= i)
        break;
      const auto &RHSElem = RHS.Elements[i];
      if (!RHSElem)
        continue;
      auto &LHSElem = Elements[i];
      if (!LHSElem)
        LHSElem =
            std::make_unique<SparseBitVectorElement<ElementSize>>(*RHSElem);
      LHSElem->unionWith(*RHSElem);
    }
  }

  // Intersect our bitmap with the RHS
  void operator&=(const FastSparseBitVector &RHS) {
    if (this == &RHS)
      return;

    bool BecameZero = false;
    for (unsigned int i = 0; i != Elements.size(); ++i) {
      auto &LHSElem = Elements[i];
      if (RHS.Elements.size() <= i) {
        if (LHSElem)
            LHSElem.reset();
        continue;
      }
      const auto &RHSElem = RHS.Elements[i];
      if (!LHSElem || !RHSElem) {
        LHSElem.reset();
        continue;
      }
      LHSElem->intersectWith(*RHSElem, BecameZero);
      if (BecameZero) {
        LHSElem.reset();
      }
    }

    emptyTrailing();
  }

  // Intersect our bitmap with the complement of the RHS
  void intersectWithComplement(const FastSparseBitVector &RHS) {
    if (this == &RHS) {
      clear();
      return;
    }
    // If either our bitmap or RHS is empty, we are done
    if (Elements.empty() || RHS.Elements.empty())
      return;

    for (unsigned int i = 0; i != Elements.size(); ++i) {
      auto &LHSElem = Elements[i];
      if (!LHSElem)
        continue;
      // RHS has no bits allocated at this index,
      // so assume those bits are 0.
      if (RHS.Elements.size() <= i)
        break;
      const auto &RHSElem = RHS.Elements[i];
      // RHS has not allocated a unique_ptr at this index,
      // so assume those bits are 0.
      if (!RHSElem)
        continue;
      // Both, LHS and RHS have valid bits
      bool BecameZero = false;
      LHSElem->intersectWithComplement(*RHSElem, BecameZero);
      if (BecameZero)
        LHSElem.reset();
    }

    emptyTrailing();
  }

   //  Three argument version of intersectWithComplement.
   //  Result of RHS1 & ~RHS2 is stored into this bitmap.
  void intersectWithComplement(const FastSparseBitVector &RHS1,
                               const FastSparseBitVector &RHS2) {
    if (this == &RHS1) {
      intersectWithComplement(RHS2);
      return;
    } else if (this == &RHS2) {
      FastSparseBitVector<ElementSize> RHS2Copy(RHS2);
      intersectWithComplement(RHS1, RHS2Copy);
      return;
    }
    Elements.clear();
    *this = RHS1;
    intersectWithComplement(RHS2);
  }

  // Return true if FastSparseBitVector instance is empty
  bool empty() const {
    if (Elements.empty())
      return true;

    for (const auto &Elem : Elements)
      if (Elem)
        return false;

    return true;
  }

  unsigned count() const {
    unsigned BitCount = 0;
    for (const auto &Elem : Elements) {
      if (!Elem)
        continue;
      BitCount += Elem->count();
    }

    return BitCount;
  }

  iterator begin()const {
    return iterator(this);
  }
   iterator end()const {
    return iterator(this, true);
  }
};

// Convenience functions for infix union, intersection, difference operators

template <unsigned ElementSize>
inline FastSparseBitVector<ElementSize>
operator|(const FastSparseBitVector<ElementSize> &LHS,
          const FastSparseBitVector<ElementSize> &RHS) {
  FastSparseBitVector<ElementSize> Result(LHS);
  Result |= RHS;
  return Result;
}

template <unsigned ElementSize>
inline FastSparseBitVector<ElementSize>
operator&(const FastSparseBitVector<ElementSize> &LHS,
          const FastSparseBitVector<ElementSize> &RHS) {
  FastSparseBitVector<ElementSize> Result(LHS);
  Result &= RHS;
  return Result;
}

template <unsigned ElementSize>
inline FastSparseBitVector<ElementSize>
operator-(const FastSparseBitVector<ElementSize> &LHS,
          const FastSparseBitVector<ElementSize> &RHS) {
  FastSparseBitVector<ElementSize> Result;
  Result.intersectWithComplement(LHS, RHS);
  return Result;
}

typedef FastSparseBitVector<2048> SparseBitVector;

#endif
