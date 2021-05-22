/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <assert.h>

namespace iSTD
{
#define LRUTemplateList class KeyType, class ValueType, class CAllocatorType
#define LRUClassType CLRUSet<KeyType, ValueType, CAllocatorType>

/******************************************************************************
Class: CLRUSet

Description:
    Maintains a linked list of the elements in the set. The structure maintains
    a pointer to the newest item (most recently touched) and the oldest item
    (first to be evicted if needed) items. When traversing the list the
    pointers go from the oldest towards the newest.

    More details can be found in the file description above.
******************************************************************************/
template<LRUTemplateList>
class CLRUSet : public CObject<CAllocatorType>
{
public:
    /**************************************************************************
    Struct: SLRUSetItem

    Description:
        The container for a key-value-pair to be stored in the LRU. This is a
        linked list node.
    **************************************************************************/
    struct SLRUSetItem : public CObject<CAllocatorType>
    {
        SLRUSetItem *pNextItem, *pPreviousItem;
        KeyType key;
        ValueType value;
    };

    class CLRUSetIterator
    {
        friend class CLRUSet;
    public:
        //always use HasNext() before calling Next(). Next has side effects
        //so you only call it once to get the value, the next call to Next()
        //must be preceded by a check of HasNext() otherwise you will
        //eventually dereference NULL.
        bool HasNext() const;
        const ValueType& Next();
    private:
        CLRUSetIterator() {}
        SLRUSetItem* current;
    };

    CLRUSet();
    CLRUSet(short in_setSize, SLRUSetItem* in_itemArrayLocation);
    ~CLRUSet();
    void Initialize(short in_setSize, SLRUSetItem* in_itemArrayLocation);
    bool IsItemInSet(const KeyType& in_key) const;
    bool IsItemInSet(const KeyType& in_key, ValueType& out_value) const;
    bool TouchItem(const KeyType& touch_key, const ValueType& touch_value);
    bool TouchItem(const KeyType& touch_key, const ValueType& touch_value, KeyType& evict_key, ValueType& evict_value);
    short GetOccupancy() const;
    CLRUSetIterator* InitAndReturnIterator();

protected:
    bool EvictAndAdd(const KeyType& newKey, const ValueType& newValue);
    void MakeMRU(SLRUSetItem* touchedItem);

    SLRUSetItem *m_pNewestItem, *m_pOldestItem;
    SLRUSetItem *m_aItemArray;
    short m_count, m_setSize;
    CLRUSetIterator m_iterator;
};

/*****************************************************************************\
Function: CLRUSet()

Description:
    Initializes values. The set depends on the count value to indicate that the
    unitialized values in m_aItemArray are not actual elements.

Input:
    none

Output:
    none
\*****************************************************************************/
template<LRUTemplateList>
LRUClassType::CLRUSet()
    : CObject<CAllocatorType>(),
    m_setSize(1),
    m_count(0)
{
    m_pOldestItem = NULL;
    m_pNewestItem = NULL;
}

/*****************************************************************************\
Function: CLRUSet()

Description:
    Initializes values. The set depends on the count value to indicate that the
    unitialized values in m_aItemArray are not actual elements.

Input:
    short in_setSize - number of elements in the set.

    SLRUSetItem *in_itemArrayLocation - pointer to the space for the linked
    list nodes.

Output:
    none
\*****************************************************************************/
template<LRUTemplateList>
LRUClassType::CLRUSet(
    short in_setSize,
    SLRUSetItem *in_itemArrayLocation)
    : m_count(0),
    m_setSize(in_setSize),
    m_aItemArray(in_itemArrayLocation)
{
    if(m_setSize < 1)
    {
        m_setSize = 1;
    }
    m_pOldestItem = NULL;
    m_pNewestItem = NULL;
}

/*****************************************************************************\
Function: ~CLRUSet()

Description:
    No items are allocated during runtime.

Input:
    none

Output:
    none
\*****************************************************************************/
template<LRUTemplateList>
LRUClassType::~CLRUSet()
{
    //no work required
}

/*****************************************************************************\
Function: Initialize()

Description:
    Sets the value of the size for the set.

Input:
    short in_setSize - number of elements in the set.

    SLRUSetItem *in_itemArrayLocation - pointer to the space for the linked
    list nodes.

Output:
    none
\*****************************************************************************/
template<LRUTemplateList>
void LRUClassType::Initialize(
    short in_setSize,
    SLRUSetItem *in_itemArrayLocation)
{
    CLRUSet(in_setSize, in_itemArrayLocation);
}

/*****************************************************************************\
Function: IsItemInSet()

Description:
    Tests for the existance of an item in the set. This overloaded version is
    used only for testing existance, see the other overloaded version for
    retrieval of values.

Input:
    KeyType in_key - The key value to search for.

Output:
    bool - true or false for the existance of the key in the set.
\*****************************************************************************/
template<LRUTemplateList>
bool LRUClassType::IsItemInSet(const KeyType& in_key) const
{
    SLRUSetItem* currentItem = m_pNewestItem;
    while(NULL != currentItem)
    {
        if(currentItem->key == in_key)
            return true;
        currentItem = currentItem->pPreviousItem;
    }
    return false;
}

/*****************************************************************************\
Function: IsItemInSet()

Description:
    Tests for the existance of an item in the set. This overloaded version is
    used for retrieval of values. See the other overloaded version for simple
    tests of existance in the set. If the return value is false (key not found)
    then the out_value is unchanged.

Input:
    KeyType in_key - The key value to search for.

    ValueType& out_value - returns the value associated with the key, if the key
    was found. If the key was not found then this value is not set and the
    return is false.

Output:
    bool - true or false for the existance of the key in the set.
\*****************************************************************************/
template<LRUTemplateList>
bool LRUClassType::IsItemInSet(
    const KeyType& in_key,
    ValueType& out_value) const
{
    SLRUSetItem* currentItem = m_pNewestItem;
    while(NULL != currentItem)
    {
        if(currentItem->key == in_key)
        {
            out_value = currentItem->value;
            return true;
        }
        currentItem = currentItem->pPreviousItem;
    }
    return false;
}

/*****************************************************************************\
Function: TouchItem()

Description:
    Moves the specified item to the most-recently-used position. If the item is
    in the set then the items are re-ordered to make MRU, if the item is not in
    the set then the least-recently-used item in the set is evicted and the new
    item is added to the set.

Input:
    KeyType touch_key - The key value to search for.

    ValueType touch_value - the associated value stored with the key.

Output:
    bool - true/false if an item was evicted to make room
\*****************************************************************************/
template<LRUTemplateList>
bool LRUClassType::TouchItem(
    const KeyType& touch_key,
    const ValueType& touch_value)
{
    SLRUSetItem* currentItem = m_pNewestItem;
    while(currentItem)
    {
        //found the key in the list, no evict required
        if(currentItem->key == touch_key)
        {
            MakeMRU(currentItem);
            return false;
        }
        currentItem = currentItem->pPreviousItem;
    }

    //couldn't find the key in the list, evict the LRU and add the new key+value
    bool wasItemEvicted = EvictAndAdd(touch_key, touch_value);
    return wasItemEvicted;
}

/*****************************************************************************\
Function: TouchItem()

Description:
    Moves the specified item to the most-recently-used position. If the item is
    in the set then the items are re-ordered to make MRU, if the item is not in
    the set then the least-recently-used item in the set is evicted and the new
    item is added to the set. If an item is not evicted then the value of
    evict_key and evict_value are not changed.

Input:
    KeyType touch_key - The key value to search for.

    ValueType touch_value - the associated value stored with the key.

    KeyType& evict_key - if return is true, then this value is that of the key
    which was evicted to add the touch_key to the set.

    ValueType& evict_value - if return is true, then this value is that of the
    value which was evicted to add the touch_value to the set.

Output:
    bool - true/false if an item was evicted to make room
\*****************************************************************************/
template<LRUTemplateList>
bool LRUClassType::TouchItem(
    const KeyType& touch_key,
    const ValueType& touch_value,
    KeyType& evict_key,
    ValueType& evict_value)
{
    SLRUSetItem* currentItem = m_pNewestItem;
    while(currentItem)
    {
        //found the key in the list, no evict required
        if(currentItem->key == touch_key)
        {
            MakeMRU(currentItem);
            return false;
        }
        currentItem = currentItem->pPreviousItem;
    }

    //couldn't find the key in the list, evict the LRU and add the new key+value
    //and return the evicted item. if m_count is not up to m_setSize yet this
    //means an uninitialized value is going to be evicted, don't want to tell
    //the user about it.
    if(m_count == m_setSize)
    {
        evict_key = m_pOldestItem->key;
        evict_value = m_pOldestItem->value;
    }
    bool wasItemEvicted = EvictAndAdd(touch_key, touch_value);
    return wasItemEvicted;
}

/*****************************************************************************\
Function: EvictAndAdd()

Description:
    Protected function. Used after determining that an item touched is not in
    the set, causes the eviction of the least-recently-used item and adds the
    new item into the set at the most-recently-used position. In the case when
    the set is not full the new item is added without an eviction. This function
    has no intelligence for searching, it is unsafe to not search for the
    element to be added first.

Input:
    KeyType newKey - the key value to add.

    ValueType newValue - the associated value to add.

Output:
    none
\*****************************************************************************/
template<LRUTemplateList>
bool LRUClassType::EvictAndAdd(
    const KeyType& newKey,
    const ValueType& newValue)
{
    //this is the first item added to the set, need to initialize the newest
    //and oldest item pointers.
    if(m_count == 0)
    {
        SLRUSetItem* newItem = &m_aItemArray[0];
        newItem->key = newKey;
        newItem->value = newValue;
        newItem->pNextItem = NULL;
        newItem->pPreviousItem = NULL;
        m_pNewestItem = newItem;
        m_pOldestItem = newItem;
        m_count++;
        return false;
    }
    //if the number of items stored is less than the capacity then don't evict
    //an element, instead insert it and push out the m_pNewestItem.
    else if(m_count < m_setSize)
    {
        SLRUSetItem* newItem = &m_aItemArray[m_count];
        newItem->key = newKey;
        newItem->value = newValue;
        newItem->pNextItem = NULL;
        newItem->pPreviousItem = m_pNewestItem;
        m_pNewestItem->pNextItem = newItem;
        m_pNewestItem = newItem;
        m_count++;
        return false;
    }
    else //normal operation, evict the oldest and add the new item.
    {
        //this will become the LRU after this code finishes evicting the oldest
        SLRUSetItem* newOldest = m_pOldestItem->pNextItem;
        //the new item being inserted into the set, it will be the MRU
        SLRUSetItem* newNewest = m_pOldestItem;
        //is currently the MRU, will be the second most recently used when finished
        SLRUSetItem* oldNewest = m_pNewestItem;

        //at this point the oldest is going to be evicted but in it's place the
        //newest will be stored.
        newNewest->key = newKey;
        newNewest->value = newValue;
        newNewest->pNextItem = NULL;
        newNewest->pPreviousItem = oldNewest;

        //the second MRU points to the new first MRU
        oldNewest->pNextItem = newNewest;
        //the new LRU becomes the tail and NULLs it's previous pointer
        newOldest->pPreviousItem = NULL;

        m_pNewestItem = newNewest;
        m_pOldestItem = newOldest;

        return true;
    }
}

/*****************************************************************************\
Function: MakeMRU()

Description:
    Protected function. Used after determining that an item touched is in the
    set, causes the touched item to be moved into the most-recently used
    position.

Input:
    SLRUSetItem* touchedItem: The linked list element to be moved to the MRU
    position.

Output:
    none
\*****************************************************************************/
template<LRUTemplateList>
void LRUClassType::MakeMRU(SLRUSetItem* touchedItem)
{
    //touched item is MRU
    if(m_pNewestItem == touchedItem)
        return;

    //if there is a previous item then this is not the oldest item, hook
    //the previous item to the next
    if(touchedItem->pPreviousItem != NULL)
    {
        touchedItem->pPreviousItem->pNextItem = touchedItem->pNextItem;
        touchedItem->pNextItem->pPreviousItem = touchedItem->pPreviousItem;
    }
    else
    {
        m_pOldestItem = touchedItem->pNextItem;
        m_pOldestItem->pPreviousItem = NULL;
    }

    //always place the most recently used at "end" of the linked list
    m_pNewestItem->pNextItem = touchedItem;
    touchedItem->pNextItem = NULL;
    touchedItem->pPreviousItem = m_pNewestItem;
    m_pNewestItem = touchedItem;
}

/*****************************************************************************\
Function: GetOccupancy()

Description:
    Returns the number of keys in the set

Input:
    none

Output:
    short - The number of keys in the set.
\*****************************************************************************/
template<LRUTemplateList>
short LRUClassType::GetOccupancy() const
{
    return m_count;
}

/*****************************************************************************\
Function: InitAndReturnIterator()

Description:
    Returns a pointer to the singleton iterator for the class. Do not attempt
    to free the memory for this pointer.

Input:
    none

Output:
    return CLRUSetIterator* - a pointer to the iterator, do not free this
    pointer.
\*****************************************************************************/
template<LRUTemplateList>
typename LRUClassType::CLRUSetIterator* LRUClassType::InitAndReturnIterator()
{
    m_iterator.current = m_pNewestItem;
    return &m_iterator;
}

/*****************************************************************************\
Function: HasNext()

Description:
    Indicates if it is safe to call Next() on the iterator. when HasNext() is
    false there are no more items to iterate.

Input:
    none

Output:
    bool - true if it is safe to use Next(), false if the iterator is finished.
\*****************************************************************************/
template<LRUTemplateList>
bool LRUClassType::CLRUSetIterator::HasNext() const
{
    return (NULL != current) ? true : false;
}

/*****************************************************************************\
Function: Next()

Description:
    Returns the value stored at the current iterator position. This function is
    unsafe if Next() returns NULL.

Input:
    none

Output:
    ValueType - the value stored at the current iterator position.
\*****************************************************************************/
template<LRUTemplateList>
const ValueType& LRUClassType::CLRUSetIterator::Next()
{
    ValueType& val = current->value;
    current = current->pNextItem;
    return val;
}

}; //namespace iSTD
