/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Object.h"

#ifndef ASSERT
#include "..\..\d3d\ibdw\d3ddebug.h"
#endif

namespace iSTD
{

/*****************************************************************************\
Template Parameters
\*****************************************************************************/
#define FastMaskTemplateList    bool OrderedList
#define CFastMaskSetType        CFastMask<OrderedList>

/*****************************************************************************\

Class:
    FastMask

Description:
    Simple ordered mask with minimal set list traversal

\*****************************************************************************/
template<FastMaskTemplateList>
class CFastMask 
{
public:
    CFastMask( unsigned int inSize );
    virtual ~CFastMask();

    inline bool                 IsValid( void ) const;
    inline bool                 IsSet( unsigned int index ) const;
    inline bool                 IsDirty ( void );
    inline const unsigned int*  GetMask(unsigned int &ioSetKey);
    inline unsigned int         GetSize();
    inline unsigned int         GetSetListSize();
    inline void                 GetUnsortedSetList( unsigned int const** ioList, unsigned int &ioSize );
    inline void                 GetSetList( unsigned int const** ioList, unsigned int &ioSize );
    inline void                 SetBits( unsigned int index, unsigned int count );
    inline void                 SetBit( unsigned int index );
    inline void                 ClearBits( void );
    inline void                 UnSetBit( unsigned int index );
    inline void                 Resize ( unsigned int in_NewSize );

protected:
    inline void                 SortSetList( void );
    inline void                 CollapseSetList( void );

    unsigned int*               m_Mask;
    unsigned int*               m_SetList;
    unsigned int                m_Key;
    unsigned int                m_SortIdx;
    unsigned int                m_CollapseIdx;
    unsigned int                m_SetListSize;
    unsigned int                m_Capacity;
    bool                        m_SortOnGet;
    bool                        m_CollapseUnsorted;
};

/*****************************************************************************\

Function:
    Constructor

Description:
    Constructs a mask with the incoming size

\*****************************************************************************/
template<FastMaskTemplateList>
CFastMaskSetType::CFastMask( unsigned int inSize )
:   m_Mask(0)
,   m_SetList(0)
,   m_Key(1)
,   m_SortIdx(0)
,   m_CollapseIdx(0)
,   m_SetListSize(0)
,   m_Capacity(inSize)
,   m_SortOnGet(false)
,   m_CollapseUnsorted(false)
{
    ASSERT(inSize > 0);

    if( inSize > 0 )
    {
        m_Mask      = new unsigned int[m_Capacity];
        m_SetList   = new unsigned int[m_Capacity];

        ASSERT(0 != m_Mask);
        ASSERT(0 != m_SetList);

        if( m_Mask && m_SetList )
        {
            // quick run up to next power of 2
            while( (unsigned int)m_Key <= m_Capacity )
            {
                m_Key = m_Key << 1;
            }

            // clear mask
            for( unsigned int i = 0; i < m_Capacity; i++ )
            {
                m_Mask[i] = m_Key;
            } 
        }
        else
        {
            SafeDeleteArray( m_Mask );
            SafeDeleteArray( m_SetList );
        }
    }
}

/*****************************************************************************\

Function:
    Destructor

Description:
    Cleanup memory

\*****************************************************************************/
template<FastMaskTemplateList>
CFastMaskSetType::~CFastMask( void )
{
    SafeDeleteArray(m_Mask);
    SafeDeleteArray(m_SetList);
}

/*****************************************************************************\

Function:
    IsValid

Description:
    Returns whether the object has been constructed properly.

\*****************************************************************************/
template<FastMaskTemplateList>
bool CFastMaskSetType::IsValid( void ) const
{ 
    return ( ( m_Mask != NULL ) && ( m_SetList != NULL ) );
}

/*****************************************************************************\

Function:
    IsSet

Description:
    Returns whether or not a bit has been set in the mask.

\*****************************************************************************/
template<FastMaskTemplateList>
bool CFastMaskSetType::IsSet( unsigned int index ) const
{ 
    ASSERT( index < m_Capacity );
    return ( m_Key != m_Mask[index] );
}

/*****************************************************************************\

Function:
    IsDirty

Description:
    Returns whether or not the mask is dirty

\*****************************************************************************/
template<FastMaskTemplateList>
bool CFastMaskSetType::IsDirty( void ) 
{ 
    if( true == m_CollapseUnsorted )
    {
        CollapseSetList();
    }

    return (m_SetListSize > 0);
}

/*****************************************************************************\

Function:
    GetMask

Description:
    Get the current mask with the set key for use in optimal comparisons for
    multiple bits.

\*****************************************************************************/
template<FastMaskTemplateList>
const unsigned int* CFastMaskSetType::GetMask( unsigned int &ioSetKey )
{
    ioSetKey = m_Key;

    return m_Mask;
}

/*****************************************************************************\

Function:
    GetSize

Description:
    Returns the number of possible indices.

\*****************************************************************************/
template<FastMaskTemplateList>
unsigned int CFastMaskSetType::GetSize( void )
{
    return m_Capacity;
}

/*****************************************************************************\

Function:
    GetSetListSize

Description:
    Returns the number set indices

\*****************************************************************************/
template<FastMaskTemplateList>
unsigned int CFastMaskSetType::GetSetListSize( void )
{
    if( true == m_CollapseUnsorted )
    {
        CollapseSetList();
    }

    return m_SetListSize;
}

/*****************************************************************************\

Function:
    SetBits

Description:
    Sets mask bits from index to count.

\*****************************************************************************/
template<FastMaskTemplateList>
void CFastMaskSetType::SetBits( unsigned int index, unsigned int count )
{
    ASSERT( (index + count) <= m_Capacity );
    for( unsigned int i = index; i < index + count; i++ )
    {
        if( m_Key == m_Mask[i] )
        {
            // when the user sets/un-sets bits often without a Get* then there
            //  exists a possibility of overrunning the setlist.
            if( m_SetListSize >= m_Capacity )
            {
                CollapseSetList();
            }

            ASSERT(m_SetListSize < m_Capacity);
            m_Mask[i]                   = m_SetListSize;      
            m_SetList[m_SetListSize++]  = i;

            if( OrderedList )
            {
                m_SortOnGet = true;
            }
        }
    }
}

/*****************************************************************************\

Function:
    SetBit

Description:
    Sets mask bit for index

\*****************************************************************************/ 
template<FastMaskTemplateList>
void CFastMaskSetType::SetBit( unsigned int index )
{
    ASSERT( index < m_Capacity );
    if( m_Key == m_Mask[index] )
    {
        if( m_SetListSize >= m_Capacity )
        {
            CollapseSetList();
        }

        ASSERT(m_SetListSize < m_Capacity);
        m_Mask[index]               = m_SetListSize;
        m_SetList[m_SetListSize++]  = index;

        if( OrderedList )
        {
            m_SortOnGet = true;
        }
    }
}

/*****************************************************************************\

Function:
    GetUnsortedSetList

Description:
    Gets an unsorted set list as an override if the list is sorted but you don't
    require sorting at this point.

\*****************************************************************************/
template<FastMaskTemplateList>
void CFastMaskSetType::GetUnsortedSetList( unsigned int const** ioList, unsigned int &ioSize )
{
    if( true == m_CollapseUnsorted )
    {
        CollapseSetList();
    }

    *ioList = m_SetList;
    ioSize  = m_SetListSize;
}

/*****************************************************************************\

Function:
    GetSetList

Description:
    Gets the set list for traversal

\*****************************************************************************/
template<FastMaskTemplateList>
void CFastMaskSetType::GetSetList( unsigned int const** ioList, unsigned int &ioSize )
{
    if( OrderedList && ( true == m_SortOnGet ) )
    {
        SortSetList();
    }
    else if( true == m_CollapseUnsorted )
    {
        CollapseSetList();
    }

    *ioList = m_SetList;
    ioSize  = m_SetListSize;
}

/*****************************************************************************\

Function:
    ClearBits

Description:
    Remove the bit from the list and mask if necessary

\*****************************************************************************/
template<FastMaskTemplateList>
void CFastMaskSetType::ClearBits( void )
{
    unsigned int index = 0;

    // walk the set list and remove set elements
    for( unsigned int i = 0; i < m_SetListSize; i++ )
    {
        index = m_SetList[i];

        // the user can un-set bits prior to calling clear and we need to ensure
        //  that we dont try to change those entries as they dont matter and could
        //  corrupt the heap
        if( index != m_Key )
        {
            m_Mask[index] = m_Key;
        }
    }

    m_SetListSize       = 0;
    m_SortIdx           = 0;
    m_CollapseIdx       = 0;
    m_SortOnGet         = false;
    m_CollapseUnsorted  = false;
}

/*****************************************************************************\

Function:
    UnSetBit

Description:
    Remove the bit from the list and mask if necessary

\*****************************************************************************/
template<FastMaskTemplateList>
void CFastMaskSetType::UnSetBit( unsigned int index )
{
    ASSERT( index < m_Capacity );

    // we do not change the setlist size because we will rely
    //  upon the sort to remove those key elements
    if( m_Key != m_Mask[index] )
    {
        // set that we need to collapse this list, required if we query an 
        //  unsorted set from a sorted set list
        m_CollapseUnsorted = true;

        unsigned int setListPos = m_Mask[index];
        if( setListPos < m_CollapseIdx )
        {
            m_CollapseIdx = setListPos; // we need to start our collapse at this index
        }

        // handle ordered list
        if( OrderedList )
        {
            // if we are an ordered list we need to compare the index position
            //  with the current sort index and modify that index if necessary
            if( setListPos < m_SortIdx )
            {
                m_SortIdx = setListPos; // move index down to allow for collapse
            }

            m_SortOnGet = true;
        }

        m_SetList[m_Mask[index]]    = m_Key;
        m_Mask[index]               = m_Key;
    }
}

/*****************************************************************************\

Function:
    Resize

Description:
    Resizes the FastMask.

\*****************************************************************************/
template<FastMaskTemplateList>
void CFastMaskSetType::Resize( unsigned int in_NewSize )
{
    ASSERT(in_NewSize != 0);

    if( in_NewSize == m_Capacity )
        return;

    unsigned int*   old_m_SetList       = m_SetList;
    unsigned int    old_m_SetListSize   = m_SetListSize;
    
    m_SetListSize       = 0; //will be using SetBits() which will correct this value
    m_CollapseIdx       = 0;
    m_SortIdx           = 0;
    m_Capacity          = in_NewSize;
    m_CollapseUnsorted  = false;
    m_SortOnGet         = false;

    SafeDeleteArray(m_Mask);
    m_Mask    = new unsigned int[m_Capacity];
    m_SetList = new unsigned int[m_Capacity];

    ASSERT(0 != m_Mask);
    ASSERT(0 != m_SetList);

    // save off old key
    unsigned int old_m_Key = m_Key;
    m_Key = 1;

    // quick run up to next power of 2
    while( (unsigned int)m_Key <= m_Capacity )
    {
        m_Key = m_Key << 1;
    }

    for( unsigned int i = 0; i < m_Capacity; i++)
    {
        m_Mask[i] = m_Key;
    }

    for( unsigned int i = 0; i < old_m_SetListSize; i++)
    {
        if(old_m_SetList[i] != old_m_Key)
        {
            SetBit(old_m_SetList[i]);
        }
    }

    SafeDeleteArray(old_m_SetList);
}

/*****************************************************************************\

Function:
    SortSetList

Description:
    Sorts the set list, a modified insertion sort algorithm
    http://en.wikipedia.org/wiki/Insertion_sort

\*****************************************************************************/
template<FastMaskTemplateList>
void CFastMaskSetType::SortSetList( )
{
    // perform an insertion sort in place 
    unsigned int    count   = 0;
    unsigned int    keyVal  = 0;
    int             idx;

    for( unsigned int i = m_SortIdx; i < m_SetListSize; i++ )
    {
        keyVal = m_SetList[i];

        // handle un-set bit
        if( keyVal == m_Key )
        {
            count++;    // increment number of un-set keys seen, these will
                        // automatically be sorted to the RHS of the list     
        }
        
        idx = i - 1;

        // find the sort position for this keyVal
        while( idx >= 0 && m_SetList[idx] > keyVal )
        {
            m_SetList[idx+1] = m_SetList[idx];
            idx--;
        }

        m_SetList[idx+1] = keyVal;
    }

    m_SetListSize       -=  count; // subtract off the number of un-set bits on the RHS
    m_SortOnGet         =   false;
    m_CollapseUnsorted  =   false;
    m_SortIdx           =   ( m_SetListSize > 0 ) ? m_SetListSize - 1 : 0;
    m_CollapseIdx       =   m_SortIdx;

    // update the mask with the new positions
    for( unsigned int i = 0; i < m_SetListSize; i++ )
    {
        m_Mask[m_SetList[i]] = i; // new index
    }
}

/*****************************************************************************\

Function:
    CollapseSetList

Description:
    Removes any un-set bits from the set list

\*****************************************************************************/

template<FastMaskTemplateList>
void CFastMaskSetType::CollapseSetList( )
{
    // walk the set list and collapse by skipping over un-set elements
    unsigned int count = m_CollapseIdx;

    for( unsigned int i = m_CollapseIdx; i < m_SetListSize; i++ )
    {
        if( m_Key != m_SetList[i] )
        {
            m_Mask[m_SetList[i]]    = count; // update mask with new position
            m_SetList[count++]      = m_SetList[i];
        }
    }

    m_CollapseUnsorted  = false;
    m_SetListSize       = count;

    if( 0 == m_SetListSize )
    {
        m_SortIdx       = 0;
        m_CollapseIdx   = 0;

        if( OrderedList )
        {
            m_SortOnGet = false; // no need to sort if empty
        }
    } 
    else 
    {
        if( m_CollapseIdx < m_SortIdx )
        {
            m_SortIdx       = m_CollapseIdx;

            if( OrderedList )
            {
                m_SortOnGet = true;
            }
        }

        m_CollapseIdx = m_SetListSize - 1;
    }
}

} // iSTD
