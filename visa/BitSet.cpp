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

#include "BitSet.h"

void BitSet::create( unsigned size )
{
    const unsigned newArraySize = ( size + NUM_BITS_PER_ELT - 1 ) / NUM_BITS_PER_ELT;
    const unsigned oldArraySize = ( m_Size + NUM_BITS_PER_ELT - 1 ) / NUM_BITS_PER_ELT;
    const unsigned numBitsLeft = size % NUM_BITS_PER_ELT;

    if( size == 0 )
    {
        free( m_BitSetArray );
        m_Size = 0;
        return;
    }

    if( newArraySize == oldArraySize )
    {
        // same array size, zero out the unused bits if necessary
        m_Size = size;
        if( newArraySize && numBitsLeft != 0 )
        {
            m_BitSetArray[ newArraySize - 1 ] &= BIT(numBitsLeft) - 1;
        }
    }
    else
    {
        BITSET_ARRAY_TYPE*  ptr = (BITSET_ARRAY_TYPE*) malloc( newArraySize * sizeof(BITSET_ARRAY_TYPE) );

        if( ptr )
        {
            if( m_BitSetArray )
            {
                if( newArraySize > oldArraySize )
                {
                    // copy entire old array over, set uninitialized bits to zero
                    memcpy_s(ptr, newArraySize * sizeof(BITSET_ARRAY_TYPE), m_BitSetArray, oldArraySize * sizeof(BITSET_ARRAY_TYPE));
                    memset( ptr + oldArraySize, 0,
                        (newArraySize - oldArraySize) * sizeof(BITSET_ARRAY_TYPE) );
                }
                else
                {
                    // copy old array up to the size of new array, zero out the unused bits
                    memcpy_s(ptr, newArraySize * sizeof(BITSET_ARRAY_TYPE), m_BitSetArray, newArraySize * sizeof(BITSET_ARRAY_TYPE));
                    if( numBitsLeft != 0 )
                    {
                        ptr[ newArraySize - 1 ] &= BIT(numBitsLeft) - 1;
                    }
                }
            }
            else
            {
                memset( ptr, 0, newArraySize * sizeof(BITSET_ARRAY_TYPE) );
            }

            free( m_BitSetArray );

            m_BitSetArray = ptr;
            m_Size = size;
        }
        else
        {
           assert(0);
        }
    }
}

void BitSet::setAll( void )
{
    if( m_BitSetArray )
    {
        unsigned index;
        for( index = 0; index < m_Size / NUM_BITS_PER_ELT; index++ )
        {
            m_BitSetArray[index] = ~((BITSET_ARRAY_TYPE)0);
        }

        // do the leftover bits, make sure we don't change the values of the unused bits,
        // so isEmpty() can be implemented faster
        int numBitsLeft = m_Size % NUM_BITS_PER_ELT;
        if( numBitsLeft )
        {
            m_BitSetArray[index] = BIT(numBitsLeft) - 1;
        }
    }
}

void BitSet::invert( void )
{
    if( m_BitSetArray )
    {
        unsigned index;
        for( index = 0; index < m_Size / NUM_BITS_PER_ELT; index++ )
        {
            m_BitSetArray[index] = ~m_BitSetArray[index];
        }

        // do the leftover bits
        int numBitsLeft = m_Size % NUM_BITS_PER_ELT;
        if( numBitsLeft )
        {
            m_BitSetArray[index] = ~m_BitSetArray[index] & ( BIT(numBitsLeft) - 1);
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

BitSet& BitSet::operator|=( const BitSet& other )
{
    unsigned size = other.m_Size;

    //grow the set to the size of the other set if necessary
    if( m_Size < other.m_Size )
    {
        create( other.m_Size );
        size = m_Size;
    }

    unsigned arraySize = ( size + NUM_BITS_PER_ELT - 1 ) / NUM_BITS_PER_ELT;
    vector_or(m_BitSetArray, other.m_BitSetArray, arraySize);

    return *this;
}

BitSet& BitSet::operator-= ( const BitSet &other )
{
    // do not grow the set for subtract
    unsigned size = m_Size < other.m_Size ? m_Size : other.m_Size;
    unsigned arraySize = ( size + NUM_BITS_PER_ELT - 1 ) / NUM_BITS_PER_ELT;
    vector_minus(m_BitSetArray, other.m_BitSetArray, arraySize);
    return *this;
}

BitSet& BitSet::operator&= ( const BitSet &other )
{
    // do not grow the set for and
    unsigned size =  m_Size < other.m_Size ? m_Size : other.m_Size;
    unsigned arraySize = ( size + NUM_BITS_PER_ELT - 1 ) / NUM_BITS_PER_ELT;
    vector_and(m_BitSetArray, other.m_BitSetArray, arraySize);

    //zero out the leftover bits if there are any
    unsigned myArraySize = ( m_Size + NUM_BITS_PER_ELT - 1 ) / NUM_BITS_PER_ELT;
    for( unsigned i = arraySize; i < myArraySize; i++ )
    {
        m_BitSetArray[ i ] = 0;
    }

    return *this;
}
