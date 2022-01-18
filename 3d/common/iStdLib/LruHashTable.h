/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "HashTable.h"
#include "LRUSet.h"

namespace iSTD
{

namespace HashingFunctions
{
    enum HashingFunctionEnum
    {
        eAbrash = 0,
        HashingFunctionCount,
    };
};

#define LruHashTableTemplateList class KeyType, class ValueType, class CAllocatorType
#define LruHashTableType CLruHashTable<KeyType, ValueType, CAllocatorType>

/*****************************************************************************\
Class: CLruHashTable

Description:
    Maintains a hash table and employs LRUSet objects at each hash index to
    evict older items.
\*****************************************************************************/
template<LruHashTableTemplateList>
class CLruHashTable : public CObject<CAllocatorType>
{
public:
    class CLruHashTableIterator
    {
        friend class CLruHashTable;
    public:
        bool Next(ValueType& out_value);
    private:
        CLruHashTableIterator() {}
        CLruHashTable* hashTablePtr;
        int currentHashIndex;
        typename LRUClassType::CLRUSetIterator* currentSetIterator;
    };
    friend class CLruHashTableIterator;

    CLruHashTable(int in_hashTableSize, int in_lruSetSize, HashingFunctions::HashingFunctionEnum in_hashingFunction = HashingFunctions::eAbrash);
    ~CLruHashTable();
    bool IsItemInHash(const KeyType& in_key);
    bool IsItemInHash(const KeyType& in_key, ValueType& out_value);
    bool TouchItem(const KeyType& in_key, const ValueType& in_value);
    bool TouchItem(const KeyType& touch_key, const ValueType& touch_value, KeyType& evict_key, ValueType& evict_value);
    void DebugPrint(const char* in_filename);
    CLruHashTableIterator* InitAndReturnIterator();

protected:
    int m_hashTableSize, m_lruSetSize;
    DWORD MakeHashValue(const KeyType& in_key);
    int GoodTableSizes(int in_requestedSize);
    LRUClassType* m_hashArray;
    typename LRUClassType::SLRUSetItem* m_linkedListElements;
    HashingFunctions::HashingFunctionEnum m_hashingFunction;
    CLruHashTableIterator m_iterator;
};

/*****************************************************************************\
Function: CLruHashTable()

Description:

Input:
    none

Output:
    none
\*****************************************************************************/
template<LruHashTableTemplateList>
LruHashTableType::CLruHashTable(
    int in_hashTableSize,
    int in_lruSetSize,
    HashingFunctions::HashingFunctionEnum in_hashingFunction)
    : m_hashingFunction(in_hashingFunction)
{
    ASSERT(in_hashTableSize > 0);
    ASSERT(in_lruSetSize > 0);

    m_hashTableSize = GoodTableSizes(in_hashTableSize);
    m_lruSetSize = in_lruSetSize;
    int strideSize = sizeof(LRUClassType::SLRUSetItem) * m_lruSetSize;

    m_hashArray = new LRUClassType[m_hashTableSize];
    while(m_hashArray == NULL)
    {
        //allocation failed, try reducing the size
        int newSize = GoodTableSizes(m_hashTableSize - 1);
        if(newSize == m_hashTableSize) //reached the minimum size, time to fail out
        {
            //at this point there is no hope
            ASSERT(m_hashArray != NULL);
            return;
        }
        m_hashArray = new LRUClassType[m_hashTableSize];
    }

    m_linkedListElements = new typename LRUClassType::SLRUSetItem[m_lruSetSize * m_hashTableSize];
    while(m_linkedListElements == NULL)
    {
        //allocation failed, try reducing the size
        int newSize = in_lruSetSize / 2;
        if(newSize == in_lruSetSize) //reached the minimum size of 1, time to fail out
        {
            //at this point there is no hope
            ASSERT(m_linkedListElements != NULL);
            return;
        }
        m_linkedListElements = new typename LRUClassType::SLRUSetItem[m_lruSetSize * m_hashTableSize];
    }

    BYTE* linkedListElementsPtr = (BYTE*)m_linkedListElements;
    for(int index = 0; index < m_hashTableSize; index++)
    {
        new (&m_hashArray[index]) LRUClassType(m_lruSetSize, (typename LRUClassType::SLRUSetItem*)linkedListElementsPtr);
        linkedListElementsPtr += strideSize;
    }
}

/*****************************************************************************\
Function: ~CLruHashTable()

Description:

Input:
    none

Output:
    none
\*****************************************************************************/
template<LruHashTableTemplateList>
LruHashTableType::~CLruHashTable()
{
    delete[] m_hashArray;
    delete[] m_linkedListElements;
}

/*****************************************************************************\
Function: IsItemInHash()

Description:
    Tests for the existance of an item in the table. This overloaded version is
    used only for testing existance, see the other overloaded version for
    retrieval of values.

Input:
    KeyType in_key - The key value to search for.

Output:
    bool - true or false for the existance of the key in the set.
\*****************************************************************************/
template<LruHashTableTemplateList>
bool LruHashTableType::IsItemInHash(const KeyType& in_key)
{
    DWORD hashIndex = MakeHashValue(in_key);
    hashIndex %= m_hashTableSize;
    return m_hashArray[hashIndex].IsItemInSet(in_key);
}

/*****************************************************************************\
Function: IsItemInHash()

Description:
    Tests for the existance of an item in the table. This overloaded version is
    used for retrieval of values. See the other overloaded version for simple
    tests of existance in the table. If the return value is false (key not found)
    then the out_value is unchanged.

Input:
    KeyType in_key - The key value to search for.

    ValueType& out_value - returns the value associated with the key, if the key
    was found. If the key was not found then this value is not set and the
    return is false.

Output:
    bool - true or false for the existance of the key in the set.
\*****************************************************************************/
template<LruHashTableTemplateList>
bool LruHashTableType::IsItemInHash(
    const KeyType& in_key,
    ValueType& out_value)
{
    DWORD hashIndex = MakeHashValue(in_key);
    hashIndex %= m_hashTableSize;
    return m_hashArray[hashIndex].IsItemInSet(in_key, out_value);
}

/*****************************************************************************\
Function: TouchItem()

Description:
    Attempts to find the key specified. If found the values are compared, if
    the values match then the item is moved to the most-recently-used position
    of that hash index. If not found the item is added to the LRUSet, possibly
    evicting an item if the set is full.

Input:
    KeyTypetouch_key - The key value to search for.

    ValueType touch_value - the associated value stored with the key.

Output:
    bool - true if the item was evicted from the set.
\*****************************************************************************/
template<LruHashTableTemplateList>
bool LruHashTableType::TouchItem(
    const KeyType& touch_key,
    const ValueType& touch_value)
{
    DWORD hashIndex = MakeHashValue(touch_key);
    hashIndex %= m_hashTableSize;
    return m_hashArray[hashIndex].TouchItem(touch_key, touch_value);
}

/*****************************************************************************\
Function: TouchItem()

Description:
    Attempts to find the key specified. If found the values are compared, if
    the values match then the item is moved to the most-recently-used position
    of that hash index. If not found the item is added to the LRUSet, possibly
    evicting an item if the set is full.

Input:
    KeyType touch_key - The key value to search for.

    ValueType touch_value - the associated value stored with the key.

    KeyType evict_key - if return is false, then this value is that of the key
    which was evicted to add the touch_key to the set.

    ValueType evict_value - if return is false, then this value is that of the
    value which was evicted to add the touch_value to the set.

Output:
    bool - true if the item was evicted from the set.
\*****************************************************************************/
template<LruHashTableTemplateList>
bool LruHashTableType::TouchItem(
    const KeyType& touch_key,
    const ValueType& touch_value,
    KeyType& evict_key,
    ValueType& evict_value)
{
    DWORD hashIndex = MakeHashValue(touch_key);
    hashIndex %= m_hashTableSize;
    return m_hashArray[hashIndex].TouchItem(touch_key, touch_value, evict_key, evict_value);
}

/*****************************************************************************\
Function: MakeHashValue()

Description:
    Protected function. Converts a KeyType into a hash value with the remainder
    of the modulo used as the hash index.

Input:
    KeyType in_key - the key value to convert.

Output:
    DWORD - hash value for the key.
\*****************************************************************************/
template<LruHashTableTemplateList>
DWORD LruHashTableType::MakeHashValue(const KeyType& in_key)
{
    DWORD hashCode = 0;

#if defined(_WIN32) && defined(_MSC_VER)
    const __m128i* keyPointer = (__m128i*)&in_key;

    ASSERT( HashingFunctions::HashingFunctionCount == 1 );

    switch(m_hashingFunction)
    {
    case HashingFunctions::eAbrash:
        {
        int fullSections = sizeof(KeyType) / sizeof(__m128i);
        __m128i runningSimdHashCode = _mm_setzero_si128();
        __m128i shiftConstant = _mm_set_epi32(0,0,0,7);

        //iterate over 128-bit blocks generating a hash code
        for(int simdBlock = 0; simdBlock < fullSections; simdBlock++)
        {
            //add the magic number from Abrash's code to the running hash code
            runningSimdHashCode = _mm_add_epi32(runningSimdHashCode,
                                                _mm_set1_epi32(0x83765503));
            //shift left by 7 and add to self
            runningSimdHashCode = _mm_add_epi32(runningSimdHashCode,
                                                _mm_sll_epi32(runningSimdHashCode,
                                                              shiftConstant));
            //load 128 bits of the key, xor into running value. keyPointer is typed DWORD* thus the times 4
            __m128i readin = _mm_loadu_si128((__m128i const*)keyPointer);
            runningSimdHashCode = _mm_xor_si128(runningSimdHashCode,
                                                _mm_loadu_si128(keyPointer));

            //keyPointer is typed DWORD*, +4 moves ahead 128-bits
            keyPointer++;
        }

        //if the value did not evenly divide into 128-bit blocks
        if(sizeof(KeyType) % sizeof(__m128i) != 0)
        {
            //add the magic number from Abrash's code to the running hash code
            runningSimdHashCode = _mm_add_epi32(runningSimdHashCode, _mm_set1_epi32(0x83765503));
            //shift left by 7 and add to self
            runningSimdHashCode = _mm_add_epi32(runningSimdHashCode,
                                                _mm_sll_epi32(runningSimdHashCode,
                                                              shiftConstant));
            //load 128 bits of the key, xor into running value. keyPointer is typed DWORD* thus the times 4
            __m128i partialKey = _mm_loadu_si128(keyPointer);
            //shift out the values which extend past the length of the key
            partialKey = _mm_slli_si128(partialKey, (16 - (sizeof(KeyType) % sizeof(__m128i))));
            runningSimdHashCode = _mm_xor_si128(partialKey, runningSimdHashCode);
        }

        //shuffle and xor field 0 and 1, then 2 and 3
        __m128i shuffledHashCode = _mm_shuffle_epi32(runningSimdHashCode, _MM_SHUFFLE(2,3,0,1));
        runningSimdHashCode = _mm_xor_si128(runningSimdHashCode, shuffledHashCode);

        //shuffle and xor 0 and 2, 1 and 3 have already been worked in.
        shuffledHashCode = _mm_shuffle_epi32(runningSimdHashCode, _MM_SHUFFLE(1,0,3,2));
        runningSimdHashCode = _mm_xor_si128(runningSimdHashCode, shuffledHashCode);

        //write out least significant 32-bits to hashCode.
        hashCode = _mm_cvtsi128_si32(runningSimdHashCode);

        break;
        }// case HashingFunctions::eAbrash:

    default:
        ASSERT(0); //not good
    }

    return hashCode;

#else // _MSC_VER
    ASSERT( IsAligned( &in_key, sizeof(DWORD) ) );
    ASSERT( IsAligned( &hashCode, sizeof(DWORD) ) );

    const DWORD* pKeyValue = (const DWORD*)&in_key;
    hashCode = *pKeyValue++;

    const DWORD count = (DWORD)( sizeof(KeyType) / sizeof(DWORD) );
    for( DWORD i = 1; i < count; ++i )
    {
        const DWORD data = *pKeyValue++;
        hashCode = ( hashCode << 1 ) ^ data;
    }

    return hashCode;
#endif // _MSC_VER
}


/*****************************************************************************\
Function: DebugPrint()

Description:
    Outputs to a file the hash table occupancy, the number of elements stored
    at each hash index. The value at each index cannot be greater than the
    LRUSet size.

Input:
    const char* in_filename - file to write to.

Output:
    none
\*****************************************************************************/
template<LruHashTableTemplateList>
void LruHashTableType::DebugPrint(const char* in_filename)
{
    FILE* fileHandle = fopen(in_filename, "w");

    for(unsigned int index = 0; index < m_hashTableSize; index++)
    {
        fprintf(fileHandle, "%u,", index);
        fprintf(fileHandle, "%d\n", m_hashArray[index].GetOccupancy());
    }

    fclose(fileHandle);
}

/*****************************************************************************\
Function: GoodTableSizes()

Description:
    Revises the hash table size to a better size for the hardware.

Input:
    int in_requestedSize - size requested by the user

Output:
    int - size suggested.
\*****************************************************************************/
template<LruHashTableTemplateList>
int LruHashTableType::GoodTableSizes(int in_requestedSize)
{
    if( in_requestedSize >= 6151 )
    {
        return 6151;
    }
    else if( in_requestedSize >= 3079 )
    {
        return 3079;
    }
    else if( in_requestedSize >= 1543 )
    {
        return 1543;
    }
    else if( in_requestedSize >= 769 )
    {
        return 769;
    }
    else if( in_requestedSize >= 389 )
    {
        return 389;
    }
    else if( in_requestedSize >= 193 )
    {
        return 193;
    }
    else if( in_requestedSize >= 97 )
    {
        return 97;
    }
    else
    {
        return 53;
    }
}

/*****************************************************************************\
Function: Next()

Description:
    Returns the current element of the iterator and moves it to the next item.
    Usage suggestion is as follows (short hand c++):
    iterator = InitAndReturnIterator();
    ValueType value;
    while(iterator.Next(value))
    {
        //do something with value
    }

Input:

Output:
    return bool - true if there is a value to give, if so out_value is set,
    otherwise there are no more elements and out_value was not modified.

    ValueType out_value - is set to the value of the current element for the
    iterator
\*****************************************************************************/
template<LruHashTableTemplateList>
bool LruHashTableType::CLruHashTableIterator::Next(ValueType& out_value)
{
    if(currentHashIndex == hashTablePtr->m_hashTableSize)
    {
        //at the end, no other elements to give. user must use
        //InitAndReturnIterator() again.
        return false;
    }

    if(currentSetIterator->HasNext())
    {
        //the current set still has more elements, return the next one
        out_value = currentSetIterator->Next();
        return true;
    }

    for(currentHashIndex++;
        currentHashIndex < hashTablePtr->m_hashTableSize;
        currentHashIndex++)
    {
        currentSetIterator = hashTablePtr->m_hashArray[currentHashIndex].InitAndReturnIterator();
        if(currentSetIterator->HasNext())
        {
            out_value = currentSetIterator->Next();
            return true;
        }
    }

    //reached the end after searching, no new elements.
    return false;
}

/*****************************************************************************\
Function: GoodTableSizes()

Description:
    Revises the hash table size to a better size for the hardware.

Input:
    int in_requestedSize - size requested by the user

Output:
    int - size suggested.
\*****************************************************************************/
template<LRUTemplateList>
typename LruHashTableType::CLruHashTableIterator* LruHashTableType::InitAndReturnIterator()
{
    //search until the first set iterator is found, if the end is reached then
    //the whole hash table is empty.
    for(m_iterator.currentHashIndex = 0;
        m_iterator.currentHashIndex < m_hashTableSize;
        m_iterator.currentHashIndex++)
    {
        m_iterator.currentSetIterator = m_hashArray[m_iterator.currentHashIndex].InitAndReturnIterator();
        if(m_iterator.currentSetIterator->HasNext())
        {
            break;
        }
    }
    m_iterator.hashTablePtr = this;
    return &m_iterator;
}

} // iSTD
