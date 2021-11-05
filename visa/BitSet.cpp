/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BitSet.h"

void BitSet::create(unsigned size)
{
    const unsigned newArraySize = (size + NUM_BITS_PER_ELT - 1) / NUM_BITS_PER_ELT;
    const unsigned oldArraySize = (m_Size + NUM_BITS_PER_ELT - 1) / NUM_BITS_PER_ELT;
    const unsigned numBitsLeft = size % NUM_BITS_PER_ELT;

    if (size == 0)
    {
        free(m_BitSetArray);
        m_Size = 0;
        return;
    }

    if (newArraySize == oldArraySize)
    {
        // same array size, zero out the unused bits if necessary
        m_Size = size;
        if (newArraySize && numBitsLeft != 0)
        {
            m_BitSetArray[ newArraySize - 1 ] &= BIT(numBitsLeft) - 1;
        }
    }
    else
    {
        BITSET_ARRAY_TYPE*  ptr = (BITSET_ARRAY_TYPE*) malloc(newArraySize * sizeof(BITSET_ARRAY_TYPE));

        if (ptr)
        {
            if (m_BitSetArray)
            {
                if (newArraySize > oldArraySize)
                {
                    // copy entire old array over, set uninitialized bits to zero
                    memcpy_s(ptr, newArraySize * sizeof(BITSET_ARRAY_TYPE), m_BitSetArray, oldArraySize * sizeof(BITSET_ARRAY_TYPE));
                    memset(ptr + oldArraySize, 0,
                        (newArraySize - oldArraySize) * sizeof(BITSET_ARRAY_TYPE));
                }
                else
                {
                    // copy old array up to the size of new array, zero out the unused bits
                    memcpy_s(ptr, newArraySize * sizeof(BITSET_ARRAY_TYPE), m_BitSetArray, newArraySize * sizeof(BITSET_ARRAY_TYPE));
                    if (numBitsLeft != 0)
                    {
                        ptr[ newArraySize - 1 ] &= BIT(numBitsLeft) - 1;
                    }
                }
            }
            else
            {
                memset(ptr, 0, newArraySize * sizeof(BITSET_ARRAY_TYPE));
            }

            free(m_BitSetArray);

            m_BitSetArray = ptr;
            m_Size = size;
        }
        else
        {
           assert(0);
        }
    }
}

void BitSet::setAll(void)
{
    if (m_BitSetArray)
    {
        unsigned index = m_Size / NUM_BITS_PER_ELT;
        std::fill_n(m_BitSetArray, index, ~(BITSET_ARRAY_TYPE) 0);

        // do the leftover bits, make sure we don't change the values of the unused bits,
        // so isEmpty() can be implemented faster
        int numBitsLeft = m_Size % NUM_BITS_PER_ELT;
        if (numBitsLeft)
        {
            m_BitSetArray[index] = BIT(numBitsLeft) - 1;
        }
    }
}

void BitSet::invert(void)
{
    if (m_BitSetArray)
    {
        unsigned index;
        for (index = 0; index < m_Size / NUM_BITS_PER_ELT; index++)
        {
            m_BitSetArray[index] = ~m_BitSetArray[index];
        }

        // do the leftover bits
        int numBitsLeft = m_Size % NUM_BITS_PER_ELT;
        if (numBitsLeft)
        {
            m_BitSetArray[index] = ~m_BitSetArray[index] & (BIT(numBitsLeft) - 1);
        }
    }
}

template <typename T>
void vector_and(T *__restrict__ p1, const T *const p2, unsigned n)
{
    for (unsigned i = 0; i < n; ++i)
    {
        p1[i] &= p2[i];
    }
}

template <typename T>
void vector_or(T *__restrict__ p1, const T *const p2, unsigned n)
{
    for (unsigned i = 0; i < n; ++i)
    {
        p1[i] |= p2[i];
    }
}

template <typename T>
void vector_minus(T *__restrict__ p1, const T *const p2, unsigned n)
{
    for (unsigned i = 0; i < n; ++i)
    {
        p1[i] &= ~p2[i];
    }
}

BitSet& BitSet::operator|=(const BitSet& other)
{
    unsigned size = other.m_Size;

    //grow the set to the size of the other set if necessary
    if (m_Size < other.m_Size)
    {
        create(other.m_Size);
        size = m_Size;
    }

    unsigned arraySize = (size + NUM_BITS_PER_ELT - 1) / NUM_BITS_PER_ELT;
    vector_or(m_BitSetArray, other.m_BitSetArray, arraySize);

    return *this;
}

BitSet& BitSet::operator-= (const BitSet &other)
{
    // do not grow the set for subtract
    unsigned size = m_Size < other.m_Size ? m_Size : other.m_Size;
    unsigned arraySize = (size + NUM_BITS_PER_ELT - 1) / NUM_BITS_PER_ELT;
    vector_minus(m_BitSetArray, other.m_BitSetArray, arraySize);
    return *this;
}

BitSet& BitSet::operator&= (const BitSet &other)
{
    // do not grow the set for and
    unsigned size =  m_Size < other.m_Size ? m_Size : other.m_Size;
    unsigned arraySize = (size + NUM_BITS_PER_ELT - 1) / NUM_BITS_PER_ELT;
    vector_and(m_BitSetArray, other.m_BitSetArray, arraySize);

    //zero out the leftover bits if there are any
    unsigned myArraySize = (m_Size + NUM_BITS_PER_ELT - 1) / NUM_BITS_PER_ELT;
    for (unsigned i = arraySize; i < myArraySize; i++)
    {
        m_BitSetArray[ i ] = 0;
    }

    return *this;
}

// Create a bitmask with the N right-most bits set to 1, and all other bits set
// to 0.
static BITSET_ARRAY_TYPE maskTrailingOnes(unsigned n)
{
    assert(n <= NUM_BITS_PER_ELT);
    return n == 0 ? 0 : (BITSET_ARRAY_TYPE(-1) >> (NUM_BITS_PER_ELT - n));
}

// Create a bitmask with the N right-most bits set to 0, and all other bits set
// to 1.
static BITSET_ARRAY_TYPE maskTrailingZeros(unsigned n)
{
    return ~maskTrailingOnes(n);
}

// TODO: Use c++20 bit manipulation utility functions.
static unsigned countTrailingZeros(BITSET_ARRAY_TYPE val)
{
    assert(val != 0);
    unsigned count = 0;
    while ((val & 1) == 0)
    {
        val >>= 1;
        ++count;
    }
    return count;
}

static unsigned countLeadingZeros(BITSET_ARRAY_TYPE val)
{
    assert(val != 0);
    unsigned count = 0;
    while ((val & (1 << (NUM_BITS_PER_ELT - 1))) == 0)
    {
        val <<= 1;
        ++count;
    }
    return count;
}

int BitSet::findFirstIn(unsigned begin, unsigned end) const
{
    assert(begin <= end && end <= m_Size);
    if (begin == end)
        return -1;

    unsigned firstElt = begin / NUM_BITS_PER_ELT;
    unsigned lastElt = (end - 1) / NUM_BITS_PER_ELT;

    for (unsigned i = firstElt; i <= lastElt; ++i)
    {
      auto elt = getElt(i);

      if (i == firstElt)
      {
        unsigned firstBit = begin % NUM_BITS_PER_ELT;
        elt &= maskTrailingZeros(firstBit);
      }

      if (i == lastElt)
      {
        unsigned lastBit = (end - 1) % NUM_BITS_PER_ELT;
        elt &= maskTrailingOnes(lastBit + 1);
      }

      if (elt != 0)
        return i * NUM_BITS_PER_ELT + countTrailingZeros(elt);
    }

    return -1;
}

int BitSet::findLastIn(unsigned begin, unsigned end) const
{
    assert(begin <= end && end <= m_Size);
    if (begin == end)
        return -1;

    unsigned lastElt = (end - 1) / NUM_BITS_PER_ELT;
    unsigned firstElt = begin / NUM_BITS_PER_ELT;

    for (unsigned i = lastElt + 1; i >= firstElt + 1; --i)
    {
      unsigned currentElt = i - 1;
      auto elt = getElt(currentElt);

      if (currentElt == lastElt)
      {
        unsigned lastBit = (end - 1) % NUM_BITS_PER_ELT;
        elt &= maskTrailingOnes(lastBit + 1);
      }

      if (currentElt == firstElt)
      {
        unsigned firstBit = begin % NUM_BITS_PER_ELT;
        elt &= maskTrailingZeros(firstBit);
      }

      if (elt != 0)
        return (currentElt + 1) * NUM_BITS_PER_ELT - countLeadingZeros(elt) - 1;
    }

    return -1;
}
