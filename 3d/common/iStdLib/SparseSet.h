/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Object.h"
#include "MemCopy.h"

namespace iSTD
{

/*****************************************************************************\
Template Parameters
\*****************************************************************************/
#define SparseSetTemplateList   class CAllocatorType
#define CSparseSetType          CSparseSet<CAllocatorType>

/*****************************************************************************\

Class:
    CSparseSet

Description:

\*****************************************************************************/
template<SparseSetTemplateList>
class CSparseSet : public CObject<CAllocatorType>
{
public:

    CSparseSet( void );
    CSparseSet( DWORD size );
    CSparseSet( const CSparseSetType& other );

    virtual ~CSparseSet( void );

    void    Resize( DWORD size );

    void    Clear( void );
    void    SetAll( void );
    void    Invert( void );

    bool    IsEmpty() const;
    bool    IsSet( DWORD index ) const;
    bool    Intersects( const CSparseSetType& other ) const;

    void    Set( DWORD index );
    void    Set( const CSparseSetType& other );
    void    UnSet( DWORD index );
    void    UnSet( const CSparseSetType& other );

    bool    UnsafeIsSet( DWORD index ) const;
    void    UnsafeSet( DWORD index );

    DWORD   GetSize( void ) const;

    bool    operator==( const CSparseSetType& other ) const;
    bool    operator!=( const CSparseSetType& other ) const;

    CSparseSetType& operator=  ( const CSparseSetType& other );
    CSparseSetType& operator|= ( const CSparseSetType& other );
    CSparseSetType& operator&= ( const CSparseSetType& other );

    DWORD   GetNumMembers( void ) const;
    DWORD   GetMember( DWORD memberNum ) const;

protected:

    DWORD*  m_Array;
    DWORD*  m_Members;
    DWORD   m_Size;
    DWORD   m_NumMembers;

    void    Create( DWORD size );
    void    Copy( const CSparseSetType& other );
    void    Delete( void );
};

/*****************************************************************************\

Function:
    CSparseSet Constructor

Description:
    Initializes the set.  The set is initially empty.

Input:

Output:
    none

\*****************************************************************************/
template<SparseSetTemplateList>
CSparseSetType::CSparseSet( void )
    : CObject<CAllocatorType>()
{
    m_Array = 0;
    m_Members = 0;
    m_Size = 0;
    m_NumMembers = 0;
}

/*****************************************************************************\

Function:
    CSparseSet Constructor

Description:
    Initializes the set.  The set is initially empty.

Input:
    DWORD size - initial size of the set

Output:
    none

\*****************************************************************************/
template<SparseSetTemplateList>
CSparseSetType::CSparseSet( DWORD size )
    : CObject<CAllocatorType>()
{
    m_Array = 0;
    m_Members = 0;
    m_Size = 0;
    m_NumMembers = 0;

    Create( size );
}

/*****************************************************************************\

Function:
    CSparseSet Copy Constructor

Description:
    Initializes the set.

Input:
    const CSparseSetType& other - other set to copy

Output:
    none

\*****************************************************************************/
template<SparseSetTemplateList>
CSparseSetType::CSparseSet( const CSparseSetType& other )
    : CObject<CAllocatorType>()
{
    m_Array = 0;
    m_Members = 0;
    m_Size = 0;
    m_NumMembers = 0;

    Copy( other );
}

/*****************************************************************************\

Function:
    CSparseSet Destructor

Description:
    Frees all internal dynamic memory

Input:
    none

Output:
    none

\*****************************************************************************/
template<SparseSetTemplateList>
CSparseSetType::~CSparseSet( void )
{
    Delete();
}

/*****************************************************************************\

Function:
    CSparseSet::Resize

Description:
    Resizes the set.  Note that this is a destructive operation, i.e. the set
    contents are not preserved on a resize.  This behavior is different than
    the behavior for BitSets.

Input:
    DWORD size - new size of the set

Output:
    none

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::Resize( DWORD size )
{
    Create( size );
}

/*****************************************************************************\

Function:
    CSparseSet::Clear

Description:
    Unsets all bits in the sparse set

Input:
    none

Output:
    none

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::Clear( void )
{
    m_NumMembers = 0;
}

/*****************************************************************************\

Function:
    CSparseSet::SetAll

Description:
    Sets all the bits in this sparse set.  This is not a particularly fast
    operation.

Input:
    void

Output:
    void

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::SetAll( void )
{
    DWORD   index = 0;
    for( index = 0; index < GetSize(); index++ )
    {
        Set( index );
    }
}

/*****************************************************************************\

Function:
    CSparseSet::Invert

Description:
    Computes the inverse of this sparse set.  This is not a particularly fast
    operation either.

Input:
    void

Output:
    void

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::Invert( void )
{
    DWORD   index = 0;
    for( index = 0; index < GetSize(); index++ )
    {
        if( IsSet( index ) )
        {
            UnSet( index );
        }
        else
        {
            Set( index );
        }
    }
}

/*****************************************************************************\

Function:
    CSparseSet::IsEmpty

Description:
    Determines if any bits are on in the bit set.

Input:
    none

Output:
    bool

\*****************************************************************************/
template<SparseSetTemplateList>
bool CSparseSetType::IsEmpty( void ) const
{
    if( GetNumMembers() == 0 )
    {
        return true;
    }

    return false;
}

/*****************************************************************************\

Function:
    CSparseSet::IsSet

Description:
    Returns true if the specified index exists in the set, false otherwise.

Input:
    DWORD index - index to check

Output:
    bool

\*****************************************************************************/
template<SparseSetTemplateList>
bool CSparseSetType::IsSet( DWORD index ) const
{
    bool    isSet = false;

    if( index < GetSize() )
    {
        DWORD   memberNum = m_Array[ index ];
        if( memberNum < GetNumMembers() )
        {
            DWORD   member = GetMember( memberNum );
            isSet = ( member == index );
        }
    }

    return isSet;
}

/*****************************************************************************\

Function:
    CSparseSet::UnsafeIsSet

Description:
    Returns true if the specified index exists in the set, false otherwise.

    Note that function does not check if index is in range! 

Input:
    DWORD index - index to check

Output:
    bool

\*****************************************************************************/
template<SparseSetTemplateList>
bool CSparseSetType::UnsafeIsSet( DWORD index ) const
{
    bool    isSet = false;

    DWORD   memberNum = m_Array[ index ];
    if( memberNum < GetNumMembers() )
    {
        DWORD   member = GetMember( memberNum );
        isSet = ( member == index );
    }

    return isSet;
}

/*****************************************************************************\

Function:
    CSparseSet::Intersects

Description:
    Returns true if any members exist in both this set and the passed-in
    set.  This is a shortcut for ( Set1 & Set2 ).IsEmpty().

Input:
    const CSparseSetType& other - Other set

Output:
    bool

\*****************************************************************************/
template<SparseSetTemplateList>
bool CSparseSetType::Intersects( const CSparseSetType& other ) const
{
    const CSparseSetType*   smallSet = &other;
    const CSparseSetType*   bigSet = this;

    if( smallSet->GetNumMembers() > bigSet->GetNumMembers() )
    {
        const CSparseSetType *tmp = smallSet;
        smallSet = bigSet;
        bigSet = tmp;
    }

    DWORD   memberNum = smallSet->GetNumMembers();
    DWORD   index = 0;
    while( memberNum-- )
    {
        index = smallSet->GetMember( memberNum );
        if( bigSet->IsSet( index ) )
        {
            return true;
        }
    }

    return false;
}

/*****************************************************************************\

Function:
    CSparseSet::Set

Description:
    Sets the bit at the given index.

Input:
    DWORD index - index of the bit to set

Output:
    void

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::Set( DWORD index )
{
    // If the index is larger than the size of this set then grow the set.
    if( index >= GetSize() )
    {
        Create( index + 1 );
    }

    if( index < GetSize() )
    {
        if( IsSet( index ) == false )
        {
            m_Array[ index ] = m_NumMembers;
            m_Members[ m_NumMembers ] = index;
            m_NumMembers++;
        }
    }
    else
    {
        ASSERT(0);
    }

    ASSERT( IsSet( index ) );
}

/*****************************************************************************\

Function:
    CSparseSet::UnsafeSet

Description:
    Sets the bit at the given index.

    Note that function does not check if index is in range! 

Input:
    DWORD index - index of the bit to set

Output:
    void

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::UnsafeSet( DWORD index )
{
    ASSERT( index < GetSize() );

    if( UnsafeIsSet( index ) == false )
    {
        m_Array[ index ] = m_NumMembers;
        m_Members[ m_NumMembers ] = index;
        m_NumMembers++;
    }

    ASSERT( IsSet( index) );
}

/*****************************************************************************\

Function:
    CSparseSet::Set

Description:
    Sets all of the given bits.

Input:
    CSparseSetType other - bits to set.

Output:
    void

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::Set( const CSparseSetType& other )
{
    DWORD   memberNum = other.GetNumMembers();
    DWORD   index = 0;
    while( memberNum-- )
    {
        index = other.GetMember( memberNum );
        Set( index );
    }
}

/*****************************************************************************\

Function:
    CSparseSet::UnSet

Description:
    Un-Sets the bit at the given index.

Input:
    DWORD index - index of the bit to un-set

Output:
    void

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::UnSet( DWORD index )
{
    // If the index is larger than the size of this set then grow the set.
    if( index >= GetSize() )
    {
        Create( index + 1 );
    }

    if( index < GetSize() )
    {
        if( IsSet( index ) )
        {
            m_NumMembers--;

            DWORD   movedIndex = m_Members[ m_NumMembers ];
            DWORD   movedMember = m_Array[ index ];

            m_Array[ movedIndex ] = movedMember;
            m_Members[ movedMember ] = movedIndex;
        }
    }
    else
    {
        ASSERT(0);
    }

    ASSERT( IsSet( index) == false );
}

/*****************************************************************************\

Function:
    CSparseSet::UnSet

Description:
    Un-Sets all of the given bits.

Input:
    CSparseSetType other - bits to un-set.

Output:
    void

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::UnSet( const CSparseSetType& other )
{
    DWORD   memberNum = other.GetNumMembers();
    DWORD   index = 0;
    while( memberNum-- )
    {
        index = other.GetMember( memberNum );
        UnSet( index );
    }
}

/*****************************************************************************\

Function:
    CSparseSet::GetSize

Description:
    Returns the maximum number of elements in the sparse set.

Input:
    void

Output:
    DWORD length

\*****************************************************************************/
template<SparseSetTemplateList>
DWORD CSparseSetType::GetSize( void ) const
{
    return m_Size;
}

/*****************************************************************************\

Function:
    CSparseSet::operator ==

Description:
    Tests this sparse set and another sparse set for equality.

Input:
    CSparseSetType& other - other sparse set

Output:
    bool

\*****************************************************************************/
template<SparseSetTemplateList>
bool CSparseSetType::operator==( const CSparseSetType& other ) const
{
    if( ( GetSize() == other.GetSize() ) &&
        ( GetNumMembers() == other.GetNumMembers() ) )
    {
        DWORD   memberNum = other.GetNumMembers();
        DWORD   index = 0;
        while( memberNum-- )
        {
            index = other.GetMember( memberNum );
            if( IsSet( index ) == false )
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

/*****************************************************************************\

Function:
    CSparseSet::operator !=

Description:
    Tests this sparse set and another sparse set for inequality.

Input:
    CSparseSetType& other - other sparse set

Output:
    bool

\*****************************************************************************/
template<SparseSetTemplateList>
bool CSparseSetType::operator!=( const CSparseSetType& other ) const
{
    if( ( GetSize() == other.GetSize() ) &&
        ( GetNumMembers() == other.GetNumMembers() ) )
    {
        DWORD   memberNum = other.GetNumMembers();
        DWORD   index = 0;
        while( memberNum-- )
        {
            index = other.GetMember( memberNum );
            if( IsSet( index ) == false )
            {
                return true;
            }
        }

        return false;
    }

    return true;
}

/*****************************************************************************\

Function:
    CSparseSet::operator =

Description:
    Equal operator to copy a sparse set.

Input:
    CSparseSetType& other - sparse set to copy

Output:
    *this

\*****************************************************************************/
template<SparseSetTemplateList>
CSparseSetType& CSparseSetType::operator= ( const CSparseSetType &other )
{
    Copy( other );

    return *this;
}

/*****************************************************************************\

Function:
    CSparseSet::operator |=

Description:
    Computes the union of this sparse set with another sparse set.

Input:
    CSparseSetType& other - other sparse set

Output:
    *this

\*****************************************************************************/
template<SparseSetTemplateList>
CSparseSetType& CSparseSetType::operator|= ( const CSparseSetType &other )
{
    DWORD   memberNum = other.GetNumMembers();
    DWORD   index = 0;
    while( memberNum-- )
    {
        index = other.GetMember( memberNum );
        Set( index );
    }

    return *this;
}

/*****************************************************************************\

Function:
    CSparseSet::operator &=

Description:
    Computes the intersection of this sparse set with another sparse set.

Input:
    CSparseSetType& other - other sparse set

Output:
    *this

\*****************************************************************************/
template<SparseSetTemplateList>
CSparseSetType& CSparseSetType::operator&= ( const CSparseSetType &other )
{
    DWORD   memberNum = GetNumMembers();
    DWORD   index = 0;

    while( memberNum-- )
    {
        index = GetMember( memberNum );
        if( other.IsSet( index ) == false )
        {
            UnSet( index );
        }
    }

    return *this;
}

/*****************************************************************************\

Function:
    CSparseSet::GetNumMembers

Description:
    Returns the number of members in this sparse set.

Input:
    void

Output:
    DWORD number of members in this sparse set

\*****************************************************************************/
template<SparseSetTemplateList>
DWORD CSparseSetType::GetNumMembers( void ) const
{
    return m_NumMembers;
}

/*****************************************************************************\

Function:
    CSparseSet::GetMember

Description:
    Returns a member of this sparse set, given a member number.

Input:
    void

Output:
    DWORD member of this sparse set

\*****************************************************************************/
template<SparseSetTemplateList>
DWORD CSparseSetType::GetMember( DWORD memberNum ) const
{
    ASSERT( memberNum < GetNumMembers() );    
    return m_Members[ memberNum ];
}

/*****************************************************************************\

Function:
    CSparseSet::Create

Description:
    Creates the internal sparse set structure of the specified size

Input:
    DWORD size - number of elements

Output:
    void

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::Create( DWORD size )
{
    const DWORD newArraySize = size;
    const DWORD oldArraySize = GetSize();

    if( newArraySize == oldArraySize )
    {
        // Nothing to do.
    }
    else if( newArraySize )
    {
        DWORD*  newArray = (DWORD*)CAllocatorType::Allocate(
            sizeof(DWORD) * newArraySize );
        DWORD*  newMembers = (DWORD*)CAllocatorType::Allocate(
            sizeof(DWORD) * newArraySize );

        if( newArray && newMembers )
        {
            if( m_Array && ( newArraySize > oldArraySize ) )
            {
                MemCopy( newArray,
                    m_Array,
                    oldArraySize * sizeof(DWORD) );
                SafeMemSet( newArray + oldArraySize,
                    0,
                    ( newArraySize - oldArraySize) * sizeof(DWORD) );
            }
            else
            {
                SafeMemSet( newArray,
                    0,
                    newArraySize * sizeof(DWORD) );
            }

            if( m_Members && ( newArraySize > oldArraySize ) )
            {
                MemCopy( newMembers,
                    m_Members,
                    oldArraySize * sizeof(DWORD) );
                SafeMemSet( newMembers + oldArraySize,
                    0,
                    ( newArraySize - oldArraySize) * sizeof(DWORD) );
            }
            else
            {
                SafeMemSet( newMembers,
                    0,
                    newArraySize * sizeof(DWORD) );
            }

            Delete();

            m_Array = newArray;
            m_Members = newMembers;
            m_Size = size;
        }
        else
        {
            CAllocatorType::Deallocate( newArray );
            newArray = 0;

            CAllocatorType::Deallocate( newMembers );
            newMembers = 0;
        }
    }
    else
    {
        Delete();
    }
}

/*****************************************************************************\

Function:
    CSparseSet::Copy

Description:
    Copies information from one sparse set to this sparse set.

Input:
    void

Output:
    void

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::Copy( const CSparseSetType& other )
{
    if( GetSize() == other.GetSize() )
    {
        if( this != &other )
        {
            MemCopy( m_Array,
                       other.m_Array,
                       GetSize() * sizeof(DWORD) );
            MemCopy( m_Members,
                       other.m_Members,
                       GetSize() * sizeof(DWORD) );

            m_NumMembers = other.GetNumMembers();
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        Create( other.GetSize() );
        if( GetSize() == other.GetSize() )
        {
            MemCopy( m_Array,
                       other.m_Array,
                       GetSize() * sizeof(DWORD) );
            MemCopy( m_Members,
                       other.m_Members,
                       GetSize() * sizeof(DWORD) );

            m_NumMembers = other.GetNumMembers();
        }
        else
        {
            ASSERT( 0 );
        }
    }
}

/*****************************************************************************\

Function:
    CSparseSet::Delete

Description:
    Deletes the internal sparse set structure

Input:
    void

Output:
    void

\*****************************************************************************/
template<SparseSetTemplateList>
void CSparseSetType::Delete( void )
{
    CAllocatorType::Deallocate( m_Array );
    m_Array = 0;

    CAllocatorType::Deallocate( m_Members );
    m_Members = 0;

    m_Size = 0;
    m_NumMembers = 0;
}

} // iSTD
