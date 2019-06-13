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
#ifndef _IGA_BITSET_HPP
#define _IGA_BITSET_HPP

#include "../asserts.hpp"

#include <cstdint>
#include <ostream>
#include <cstring>
#include <algorithm>

// Align N up to the nearest multiple of 32
#define ALIGN_UP_TO(A, N) \
    ((N + (A) - 1) - (N + (A) - 1) % (A))


namespace iga
{

// I so badly wanted to just use std::bitset and then drink beer with my free
// time.  Alas this lacks some of the APIs that I need to efficiently
// perform data flow.  Specifically, I need efficient testAny/testAll
// predicates.
template <typename I = uint32_t>
class BitSet {
public:
    static const size_t BITS_PER_WORD = 8 * sizeof(I);

    BitSet(size_t bit_size) {
        N = bit_size;
        wordsSize = ALIGN_UP_TO(BITS_PER_WORD, N) / BITS_PER_WORD;
        // allocate bits
        words = new I[wordsSize];
        reset();
    }
    ~BitSet() {
        delete[] words;
    }

    void reset() { memset(words, 0, wordsSize * sizeof(I)); }
    bool set(size_t off, size_t len = 1, bool val = true); // sets/clears range
    bool test(size_t off) const { return testAny(off,1); }
    bool testAny(size_t off, size_t len) const;
    bool testAll(size_t off, size_t len) const;
    bool empty() const {return !testAny(0, N);}

    inline bool intersects(const BitSet<I> &rhs) const;

    // If the bitSets have intersects in given range, start from off and with legnth len
    // off: bit offset
    // len: range length in bits
    inline bool intersects(const BitSet<I> &rhs, size_t off, size_t len) const;

    // if the given rhs is completely the same as itself
    bool equal(const BitSet<I> &rhs) const;

    // THERE EXISTS WORD w such that ((w & mask) == eq)
    // bool testAnyEq(size_t off, size_t len, const I &mask, const I &eq) const;
    // FOR ALL ...
    // bool testAllEq(size_t off, size_t len, const I &mask, const I &eq) const;

//    BitSet& operator ~() const;
    bool andNot(const BitSet<I> &rhs); // set subtraction
    bool add(const BitSet<I> &rhs); // set union
    bool intersectInto(const BitSet<I> &rhs, BitSet<I> &into) const; // set intersection (returns true if result non-empty)

    bool operator [](const size_t &ix) const { return test(ix); }
    bool operator ==(const BitSet &bs) const;
    bool operator !=(const BitSet &bs) const { return !(*this == bs); }
    //    std::string str() const;
//    std::string str(std::ostream &os) const;
private:
    size_t N;
    size_t wordsSize;
    I* words;

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
inline bool BitSet<I>::set(size_t off, size_t len, bool val)
{
    IGA_ASSERT(off >= 0 && off + len <= N,
        "BitSet::testAll: index out of bounds");

    // check the first word (misaligned mask)
    auto w_ix = off / BITS_PER_WORD;
    auto w_off = off % BITS_PER_WORD;
    auto misaligned_len = std::min<size_t>(BITS_PER_WORD - w_off, len);
    auto misaligned_mask = makeMask(misaligned_len) << w_off;
    auto oldWord = words[w_ix];
    if (val) {
        words[w_ix] |= misaligned_mask;
    } else {
        words[w_ix] &= ~misaligned_mask;
    }
    bool changed = oldWord != words[w_ix];

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
    for (size_t i = 0; i < N / BITS_PER_WORD; i++) {
        if (words[i] & rhs.words[i]) {
            return true;
        }
    }
    return false;
}

template <typename I>
inline bool BitSet<I>::intersects(
    const BitSet<I> &rhs, size_t off, size_t len) const {

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
        if ((words[w_ix] & aligned_mask) &
            (rhs.words[w_ix] & aligned_mask)) {
            return true;
        }
        len -= aligned_len;
        off += aligned_len;
    }
    return false;
}

template <typename I>
inline bool BitSet<I>::equal(const BitSet<I> &rhs) const {
    bool result = true;
    for (size_t i = 0; i < N / BITS_PER_WORD; i++) {
        result = (words[i] == rhs.words[i]) ? true : false;
        if (!result)
            break;
    }
    // padding will remain 0 since both padding are 0's
    // 0 & 0 is 0 so no need special handling on padding
    return result;
}

template <typename I>
inline bool BitSet<I>::andNot(const BitSet<I> &rhs)
{
    bool changed = false;
    for (size_t i = 0; i < N/BITS_PER_WORD; i++) {
        auto old = words[i];
        words[i] &= ~rhs.words[i];
        changed |= (words[i] != old);
    }
    // padding will remain 0 since padding is already 0's
    // ((0x...000 & 0x...FFF) == 0x...000)
    //     this       rhs
    return changed;
}

template <typename I>
inline bool BitSet<I>::add(const BitSet<I> &rhs)
{
    bool changed = false;
    for (size_t i = 0; i < N/BITS_PER_WORD; i++) {
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
inline bool BitSet<I>::intersectInto(
    const BitSet<I> &rhs, BitSet<I> &into) const
{
    bool notEmpty = true;
    for (size_t i = 0; i < N/BITS_PER_WORD; i++) {
        into.words[i] = words[i] & rhs.words[i];
        notEmpty |= (words[i] != 0);
    }
    // doesn't matter if the padding matches (it'll be correct either way)
    return notEmpty;
}

template <typename I>
inline bool BitSet<I>::testAny(size_t off, size_t len) const
{
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
inline bool BitSet<I>::testAll(size_t off, size_t len) const
{
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

template <typename I>
bool BitSet<I>::operator==(const BitSet<I> &bs) const
{
    // do we have to worry about the ragged padding.
    // e.g. given 32b words if the bitset is 31 bits long, we have to
    // worry about the trailing bit
    return memcmp(&words[0], &bs.words[0], sizeof(words)) == 0;
}

} // namespace iga

#endif // _IGA_BITSET_HPP