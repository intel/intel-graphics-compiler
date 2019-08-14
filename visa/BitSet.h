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

#ifndef _BITSET_H_
#define _BITSET_H_

#include "Mem_Manager.h"
#include <cstdlib>
#include <cstring>

// Array-based bitset implementation where each element occupies a single bit.
// Inside each array element, bits are stored and indexed from lsb to msb.
typedef unsigned int BITSET_ARRAY_TYPE;

class BitSet
{
#define BITS_PER_BYTE  8
#define BIT(x)  (((BITSET_ARRAY_TYPE)1 ) << x)
#define NUM_BITS_PER_ELT ( sizeof(BITSET_ARRAY_TYPE) * BITS_PER_BYTE )

public:
    BitSet() : m_BitSetArray(nullptr), m_Size(0) {}
    BitSet(unsigned size, bool defaultValue)
    {
        m_BitSetArray = NULL;
        m_Size = 0;

        create(size);
        if (defaultValue)
        {
            setAll();
        }
    }

    BitSet(const BitSet &other) : m_BitSetArray(nullptr), m_Size(0)
    {
        copy(other);
    }

    BitSet(BitSet && other) noexcept
    {
        m_BitSetArray = other.m_BitSetArray;
        m_Size = other.m_Size;
        other.m_BitSetArray = nullptr;
        other.m_Size = 0;
    }

    ~BitSet() { std::free(m_BitSetArray); }

    void resize(unsigned size) { create(size); }
    void clear()
    {
        unsigned sizeInBytes = (m_Size + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
        std::memset(m_BitSetArray, 0, sizeInBytes);
    }

    void setAll(void);
    void invert(void);

    bool isEmpty() const
    {
        unsigned arraySize = (m_Size + NUM_BITS_PER_ELT - 1) / NUM_BITS_PER_ELT;
        for (unsigned i = 0; i < arraySize; i++)
        {
            if (m_BitSetArray[i] != 0)
            {
                return false;
            }
        }
        return true;
    }

    bool isAllset() const
    {
        unsigned index;
        unsigned bound = m_Size / NUM_BITS_PER_ELT;

        for (index = 0; index < bound; index++)
        {
            if (~m_BitSetArray[index] != 0)
            {
                return false;
            }
        }

        unsigned numBitsLeft = m_Size % NUM_BITS_PER_ELT;
        for (unsigned bitIndex = 0; bitIndex < numBitsLeft; bitIndex++)
        {
            if ((m_BitSetArray[index] & BIT(bitIndex)) == 0)
            {
                return false;
            }
        }

        return true;
    }


    bool isSet(unsigned index) const
    {
        if (index < m_Size)
        {
            unsigned arrayIndex = index / NUM_BITS_PER_ELT;
            unsigned bitIndex = index % NUM_BITS_PER_ELT;
            return (m_BitSetArray[arrayIndex] & BIT(bitIndex)) != 0;
        }
        return false;
    }

    bool isAllSet(unsigned startIndex, unsigned endIndex) const
    {
        MUST_BE_TRUE(startIndex <= endIndex, "Invalid bitSet Index");
        MUST_BE_TRUE(startIndex < m_Size, "Invalid bitSet Index");
        MUST_BE_TRUE(endIndex < m_Size, "Invalid bitSet Index");
        
        unsigned start = startIndex / NUM_BITS_PER_ELT;
        unsigned end = endIndex / NUM_BITS_PER_ELT;

        if (start == end)
        {
            for (unsigned i = startIndex; i <= endIndex; i++)
            {
                if (!isSet(i))
                {
                    return false;
                }
            }
            return true;
        }
        
        unsigned index;
        unsigned numBitsBefore = startIndex % NUM_BITS_PER_ELT;
        if (numBitsBefore)
        {
            for (unsigned bitIndex = numBitsBefore; bitIndex < NUM_BITS_PER_ELT; bitIndex++)
            {
                if ((m_BitSetArray[start] & BIT(bitIndex)) == 0)
                {
                    return false;
                }
            }
            start++;
        }

        for (index = start; index < end; index++)
        {
            if (~m_BitSetArray[index] != 0)
            {
                return false;
            }
        }

        unsigned numBitsLeft = endIndex % NUM_BITS_PER_ELT;
        for (unsigned bitIndex = 0; bitIndex <= numBitsLeft; bitIndex++)
        {
            if ((m_BitSetArray[index] & BIT(bitIndex)) == 0)
            {
                return false;
            }
        }

        return true;
    }

    bool isEmpty(unsigned startIndex, unsigned endIndex) const
    {
        MUST_BE_TRUE(startIndex <= endIndex, "Invalid bitSet Index");
        MUST_BE_TRUE(startIndex < m_Size, "Invalid bitSet Index");
        MUST_BE_TRUE(endIndex < m_Size, "Invalid bitSet Index");
        
        unsigned start = startIndex / NUM_BITS_PER_ELT;
        unsigned end = endIndex / NUM_BITS_PER_ELT;

        if (start == end)
        {
            for (unsigned i = startIndex; i <= endIndex; i++)
            {
                if (isSet(i))
                {
                    return false;
                }
            }
            return true;
        }

        unsigned index;
        unsigned numBitsBefore = startIndex % NUM_BITS_PER_ELT;
        if (numBitsBefore)
        {
            for (unsigned bitIndex = numBitsBefore; bitIndex < NUM_BITS_PER_ELT; bitIndex++)
            {
                if ((m_BitSetArray[start] & BIT(bitIndex)) != 0)
                {
                    return false;
                }
            }
            start++;
        }

        for (index = start; index < end; index++)
        {
            if (m_BitSetArray[index] != 0)
            {
                return false;
            }
        }

        unsigned numBitsLeft = endIndex % NUM_BITS_PER_ELT;
        for (unsigned bitIndex = 0; bitIndex <= numBitsLeft; bitIndex++)
        {
            if ((m_BitSetArray[index] & BIT(bitIndex)) != 0)
            {
                return false;
            }
        }

        return true;
    }

    BITSET_ARRAY_TYPE getElt(unsigned eltIndex) const
    {
        MUST_BE_TRUE(eltIndex < m_Size, "Invalid bitSet Index");
        return m_BitSetArray[eltIndex];
    }

    void setElt(unsigned eltIndex, BITSET_ARRAY_TYPE value) 
    {
        unsigned bound = (eltIndex + 1) * NUM_BITS_PER_ELT;
        if (bound > m_Size)
        {
            create(bound);
        }
        m_BitSetArray[eltIndex] |= value;
    }

    void resetElt(unsigned eltIndex, BITSET_ARRAY_TYPE value)
    {
        unsigned bound = (eltIndex + 1) * NUM_BITS_PER_ELT;
        if (bound > m_Size)
        {
            create(bound);
        }
        m_BitSetArray[eltIndex] &= ~value;
    }

    void set(unsigned index, bool value)
    {
        // If the index is larger than the size of the BitSet then grow the BitSet
        if (index >= m_Size)
        {
            create(index + 1);
        }

        unsigned arrayIndex = index / NUM_BITS_PER_ELT;
        unsigned bitIndex = index % NUM_BITS_PER_ELT;

        if (value)
        {
            m_BitSetArray[arrayIndex] |= BIT(bitIndex);
        }
        else
        {
            m_BitSetArray[arrayIndex] &= ~BIT(bitIndex);
        }
    }

    void set(unsigned startIndex, unsigned endIndex)
    {
        for (unsigned i = startIndex; i <= endIndex; i++)
        {
            set(i, true);
        }
    }

    unsigned getSize() const { return m_Size; }

    bool operator==(const BitSet &other) const
    {
        if (m_Size == other.m_Size)
        {
            unsigned sizeInBytes = (m_Size + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
            return 0 == std::memcmp(m_BitSetArray, other.m_BitSetArray, sizeInBytes);
        }
        return false;
    }

    bool operator!=(const BitSet &other) const
    {
        if (m_Size == other.m_Size)
        {
            unsigned sizeInBytes = (m_Size + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
            return 0 != std::memcmp(m_BitSetArray, other.m_BitSetArray, sizeInBytes);
        }
        return true;
    }

    BitSet& operator= (const BitSet &other)
    {
        copy(other);
        return *this;
    }

    BitSet& operator=(BitSet&& other) noexcept
    {
        m_BitSetArray = other.m_BitSetArray;
        m_Size = other.m_Size;
        other.m_BitSetArray = nullptr;
        other.m_Size = 0;

        return *this;
    }

    void swap(BitSet &other)
    {
        if (this != &other)
        {
            std::swap(m_Size, other.m_Size);
            std::swap(m_BitSetArray, other.m_BitSetArray);
        }
    }

    BitSet &operator|=(const BitSet &other);
    BitSet &operator&=(const BitSet &other);
    BitSet &operator-=(const BitSet &other);

    void *operator new(size_t sz, vISA::
        Mem_Manager &m) { return m.alloc(sz); }

protected:
    BITSET_ARRAY_TYPE* m_BitSetArray;
    unsigned m_Size;

    void create(unsigned size);
    void copy(const BitSet &other)
    {
        unsigned sizeInBytes = (other.m_Size + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
        if (this != &other)
        {
            if (m_Size == other.m_Size)
            {
                memcpy_s(m_BitSetArray, sizeInBytes, other.m_BitSetArray, sizeInBytes);
            }
            else
            {
                create(other.m_Size);
                memcpy_s(m_BitSetArray, sizeInBytes, other.m_BitSetArray, sizeInBytes);
            }
        }
    }
};

#endif
