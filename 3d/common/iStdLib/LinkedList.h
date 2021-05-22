/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Object.h"
#include "Threading.h"
#include "MemCopy.h"


#if defined _DEBUG
#   if defined( ISTDLIB_UMD ) && !defined( NO_RTTI )
#       include <typeinfo>
#       define TYPEID_NAME( type )   (typeid(type).name())
#   else // ISTDLIB_UMD
#       define TYPEID_NAME( type )   (#type)
#   endif // ISTDLIB_UMD
#else // _DEBUG
#   define TYPEID_NAME( type )
#endif // _DEBUG

namespace iSTD
{

/*****************************************************************************\
Struct: IsLinkedListTypeSupported
\*****************************************************************************/
template<typename T>
struct IsLinkedListTypeSupported                    { enum { value = false }; };

template<>
struct IsLinkedListTypeSupported<bool>              { enum { value = true }; };

template<>
struct IsLinkedListTypeSupported<char>              { enum { value = true }; };

template<>
struct IsLinkedListTypeSupported<unsigned char>     { enum { value = true }; };

template<>
struct IsLinkedListTypeSupported<int>               { enum { value = true }; };

template<>
struct IsLinkedListTypeSupported<unsigned int>      { enum { value = true }; };

#ifndef __LP64__ // u/long on linux64 platform is 64-bit type and collides with U/INT64
template<>
struct IsLinkedListTypeSupported<long>              { enum { value = true }; };

template<>
struct IsLinkedListTypeSupported<unsigned long>     { enum { value = true }; };
#endif

template<>
struct IsLinkedListTypeSupported<float>             { enum { value = true }; };

template<>
struct IsLinkedListTypeSupported<INT64>             { enum { value = true }; };

template<>
struct IsLinkedListTypeSupported<UINT64>            { enum { value = true }; };

template<typename T>
struct IsLinkedListTypeSupported<T*>                { enum { value = true }; };

/*****************************************************************************\
Struct: AreLinkedListDuplicatesSupported
\*****************************************************************************/
template<typename T>
struct AreLinkedListDuplicatesSupported         { enum { value = true }; };

template<typename T>
struct AreLinkedListDuplicatesSupported<T*>     { enum { value = false }; };

template<typename T>
struct AreLinkedListDuplicatesSupported<T&>     { enum { value = false }; };

/*****************************************************************************\
Template Parameters
\*****************************************************************************/
#define LinkedListTemplateList  class Type, class CAllocatorType
#define CLinkedListType         CLinkedList<Type,CAllocatorType>

/*****************************************************************************\

Class:
    CLinkedList

Description:
    Implements a pointer-based linked-list

\*****************************************************************************/
template<LinkedListTemplateList>
class CLinkedList : public CObject<CAllocatorType>
{
protected:

    class CNode : public CObject<CAllocatorType>
    {
    public:

        CNode( void );
        CNode( const Type &element );
        virtual ~CNode( void );

        void    Insert( CNode* pNext );
        void    Remove( void );

        void        SetElement( const Type &element );
        Type&       GetElement( void );
        const Type& GetElement( void ) const;

        void    SetPrevious( CNode* pNode );
        void    SetNext( CNode* pNode );

        CNode*  GetPrevious( void ) const;
        CNode*  GetNext( void ) const;

    protected:

        Type    m_Element;
        CNode*  m_Next;
        CNode*  m_Previous;
    };

public:

    CLinkedList( void );
    virtual ~CLinkedList( void );

    class CIterator
    {
    public:

        CIterator( void );
        CIterator( CNode* ptr );
        CIterator( const CIterator &iterator );

        CIterator&  operator--( void );
        CIterator&  operator++( void );

        bool operator==( const CIterator &iterator ) const;
        bool operator!=( const CIterator &iterator ) const;

        CIterator &operator=( const CIterator &iterator );
        Type&   operator*( void );

        bool    IsNull( void ) const;
        void    SetNull( void );

        friend class CLinkedList;
        friend class CConstIterator;

    protected:

        CNode*  m_Ptr;
    };

    class CConstIterator
    {
    public:

        CConstIterator( void );
        CConstIterator( const CIterator &iterator );
        CConstIterator( const CNode* ptr );

        CConstIterator& operator--( void );
        CConstIterator& operator++( void );

        bool operator==( const CConstIterator &iterator ) const;
        bool operator!=( const CConstIterator &iterator ) const;

        const Type& operator*( void );

        bool    IsNull( void ) const;
        void    SetNull( void );

        friend class CLinkedList;

    protected:

        const CNode*    m_Ptr;
    };

    bool    IsEmpty( void ) const;
    DWORD   GetCount( void ) const;

    CLinkedListType& operator= ( const CLinkedListType &llist );

    CIterator   Begin( void );
    CIterator   Get( const DWORD index );
    CIterator   End( void );

    CConstIterator  Begin( void ) const;
    CConstIterator  Get( const DWORD index ) const;
    CConstIterator  End( void ) const;

    CIterator Find( const Type &element );
    CConstIterator Find( const Type &element ) const;

    bool    Add( const Type &element );
    bool    Remove( const Type &element );

    bool    Add( const CIterator &location, const Type &element );
    bool    Remove( const CIterator &location );

    void    Clear( void );

    void    Splice( 
                const CIterator &dstLocation, 
                const CIterator &srcLocation,
                CLinkedListType &srcList );

    void    SpliceListPartAtTheEnd(
                CLinkedListType &srcList,
                const CIterator &srcStartLocation,
                const CIterator &srcEndLocation );

    void    Set( const CIterator &location, const Type &element );

    void    DeleteFreePool( void );

    void    DebugPrint( void ) const;

    C_ASSERT( IsLinkedListTypeSupported<Type>::value == true );

protected:

    bool    Remove( CNode* &pNode );

    CNode*  CreateNode( const Type &element );
    void    DeleteNode( CNode* &pNode );

    CNode   m_DummyTail;
    DWORD   m_Count;

    CNode   m_FreePoolDummyTail;
    DWORD   m_FreePoolCount;

    DECL_DEBUG_MUTEX( m_InstanceNotThreadSafe )
};

/*****************************************************************************\

Function:
    CNode Constructor

Description:
    Initializes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::CNode::CNode( void )
    : CObject<CAllocatorType>()
{
    SafeMemSet( &m_Element, 0, sizeof(Type) );

    m_Next = this;
    m_Previous = this;
}

/*****************************************************************************\

Function:
    CNode Constructor

Description:
    Initializes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::CNode::CNode( const Type &element )
    : CObject<CAllocatorType>()
{
    m_Element = element;
    m_Next = this;
    m_Previous = this;
}

/*****************************************************************************\

Function:
    CNode Destructor

Description:
    Deletes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::CNode::~CNode( void )
{
    ASSERT( m_Next == this );
    ASSERT( m_Previous == this );
}

/*****************************************************************************\

Function:
    CNode::Insert

Description:
    Inserts the node before the given node

Input:
    const CNode* pNext

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::CNode::Insert( CNode* pNext )
{
    m_Next = pNext;
    m_Previous = m_Next->m_Previous;

    m_Previous->m_Next = this;
    m_Next->m_Previous = this;
}

/*****************************************************************************\

Function:
    CNode::Remove

Description:
    Removes the node from the linked list

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::CNode::Remove( void )
{
    m_Previous->m_Next = m_Next;
    m_Next->m_Previous = m_Previous;

    m_Previous = this;
    m_Next = this;

    SafeMemSet( &m_Element, 0, sizeof(Type) );
}

/*****************************************************************************\

Function:
    CNode::SetElement

Description:
    Sets the element of the node from the linked list

Input:
    const Type &element

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::CNode::SetElement( const Type &element )
{
    m_Element = element;
}

/*****************************************************************************\

Function:
    CNode::GetElement

Description:
    Gets the element of the node from the linked list

Input:
    none

Output:
    Type&

\*****************************************************************************/
template<LinkedListTemplateList>
Type& CLinkedListType::CNode::GetElement( void )
{
    return m_Element;
}

/*****************************************************************************\

Function:
    CNode::GetElement

Description:
    Gets the element of the node from the linked list

Input:
    none

Output:
    const Type&

\*****************************************************************************/
template<LinkedListTemplateList>
const Type& CLinkedListType::CNode::GetElement( void ) const
{
    return m_Element;
}

/*****************************************************************************\

Function:
    CNode::SetPrevious

Description:
    Sets the previous pointer of the node from the linked list

Input:
    CNode* pNode

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::CNode::SetPrevious( CNode* pNode )
{
    m_Previous = pNode;
}

/*****************************************************************************\

Function:
    CNode::SetNext

Description:
    Sets the next pointer of the node from the linked list

Input:
    CNode* pNode

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::CNode::SetNext( CNode* pNode )
{
    m_Next = pNode;
}

/*****************************************************************************\

Function:
    CNode::GetPrevious

Description:
    Gets the previous pointer of the node from the linked list

Input:
    none

Output:
    CNode*

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CNode* CLinkedListType::CNode::GetPrevious( void ) const
{
    return m_Previous;
}

/*****************************************************************************\

Function:
    CNode::GetNext

Description:
    Gets the next pointer of the node from the linked list

Input:
    none

Output:
    CNode*

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CNode* CLinkedListType::CNode::GetNext( void ) const
{
    return m_Next;
}

/*****************************************************************************\

Function:
    CLinkedList Constructor

Description:
    Initializes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::CLinkedList( void )
    : CObject<CAllocatorType>()
{
    m_Count = 0;
    m_FreePoolCount = 0;
    m_DummyTail.SetNext( &m_DummyTail );
    m_DummyTail.SetPrevious( &m_DummyTail );

    INIT_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CLinkedList Destructor

Description:
    Deletes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::~CLinkedList( void )
{
    ASSERT( IsEmpty() );

    // Delete all objects on the linked-list
    Clear();

    DeleteFreePool();

    DELETE_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    Default CLinkedList::CIterator Constructor

Description:
    Creates a NULL CIterator.

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::CIterator::CIterator( void )
{
    m_Ptr = NULL;
}

/*****************************************************************************\

Function:
    CLinkedList::CIterator Constructor

Description:
    Constructs a CIterator to the specified linked list node pointer.

Input:
    CNode* - pointer to create an iterator to.

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::CIterator::CIterator( CNode* ptr )
{
    ASSERT( ptr );
    m_Ptr = ptr;
}

/*****************************************************************************\

Function:
    CLinkedList::CIterator Copying Constructor

Description:
    Constructs a CIterator being copy of other iterator.

Input:
    const CIterator & - reference to iterator to copy from.

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::CIterator::CIterator( const CIterator &iterator )
{
    m_Ptr = iterator.m_Ptr;
}


/*****************************************************************************\

Function:
    CLinkedList::CIterator::operator--

Description:
    Iterates backwards through the linked list via predecrement.

Input:
    none

Output:
    CLinkedList CIterator& - iterator to the previous node in the list.

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CIterator&
CLinkedListType::CIterator::operator--( void )
{
    m_Ptr = m_Ptr->GetPrevious();
    ASSERT( m_Ptr );
    return *this;
}

/*****************************************************************************\

Function:
    CLinkedList::CIterator::operator++

Description:
    Iterates forwards through the linked list via preincrement.

Input:
    none

Output:
    CLinkedList CIterator& - iterator to the next node in the list.

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CIterator&
CLinkedListType::CIterator::operator++( void )
{
    m_Ptr = m_Ptr->GetNext();
    ASSERT( m_Ptr );
    return *this;
}

/*****************************************************************************\

Function:
    CLinkedList::CIterator::operator==

Description:

Input:
    const CIterator& - pointer to the iterator to compare to.

Output:
    bool - true if this iterator and the passed-in iterator point to the same
           linked list node.

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::CIterator::operator==(
    const CIterator& iterator ) const
{
    return m_Ptr == iterator.m_Ptr;
}

/*****************************************************************************\

Function:
    CLinkedList::CIterator::operator!=

Description:

Input:
    const CIterator& - pointer to the iterator to compare to.

Output:
    bool - true if this iterator and the passed-in iterator point to different
           linked list nodes.

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::CIterator::operator!=(
    const CIterator& iterator ) const
{
    return m_Ptr != iterator.m_Ptr;
}

/*****************************************************************************\

Function:
    CLinkedList::CIterator::operator=

Description:

Input:
    const CIterator& - reference to the iterator to copy from.

Output:
    const CIterator & - reference to self

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CIterator &CLinkedListType::CIterator::operator=(
    const CIterator &iterator )
{
    m_Ptr = iterator.m_Ptr;
    return *this;
}
/*****************************************************************************\

Function:
    CLinkedList::CIterator::operator*

Description:
    Returns a reference to the item that this iterator points to.

Input:
    none

Output:
    Type& - reference to the item that this iterator points to.

\*****************************************************************************/
template<LinkedListTemplateList>
Type& CLinkedListType::CIterator::operator*( void )
{
    return m_Ptr->GetElement();
}

/*****************************************************************************\

Function:
    CLinkedList::CIterator::IsNull

Description:
    Determines if the iterator has been assigned

Input:
    none

Output:
    bool

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::CIterator::IsNull( void ) const
{
    return ( m_Ptr == NULL );
}

/*****************************************************************************\

Function:
    CLinkedList::CIterator::SetNull

Description:
    Sets the iterator pointer to null

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::CIterator::SetNull( void )
{
    m_Ptr = NULL;
}

/*****************************************************************************\

Function:
    Default CLinkedList::CConstIterator Constructor

Description:
    Creates a NULL CConstIterator.

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::CConstIterator::CConstIterator( void )
{
    m_Ptr = NULL;
}

/*****************************************************************************\

Function:
    CLinkedList::CConstIterator Constructor

Description:
    Constructs a CConstIterator to the specified const linked list node pointer.

Input:
    const CNode* - pointer to create an iterator to.

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::CConstIterator::CConstIterator( const CNode* ptr )
{
    ASSERT( ptr );
    m_Ptr = ptr;
}

/*****************************************************************************\

Function:
    CLinkedList::CConstIterator Constructor

Description:
    Constructs a CConstIterator from a CIterator.

Input:
    const CIterator& - CIterator to convert to a CConstIterator.

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType::CConstIterator::CConstIterator( const CIterator& iterator )
{
    ASSERT( iterator.m_Ptr );
    m_Ptr = iterator.m_Ptr;
}

/*****************************************************************************\

Function:
    CLinkedList::CConstIterator::operator--

Description:
    Iterates backwards through the linked list via predecrement.

Input:
    none

Output:
    CLinkedList CConstIterator& - iterator to the previous node in the list.

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CConstIterator&
CLinkedListType::CConstIterator::operator--( void )
{
    m_Ptr = m_Ptr->GetPrevious();
    ASSERT( m_Ptr );
    return *this;
}

/*****************************************************************************\

Function:
    CLinkedList::CConstIterator::operator++

Description:
    Iterates forwards through the linked list via preincrement.

Input:
    none

Output:
    CLinkedList CConstIterator& - iterator to the next node in the list.

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CConstIterator&
CLinkedListType::CConstIterator::operator++( void )
{
    m_Ptr = m_Ptr->GetNext();
    ASSERT( m_Ptr );
    return *this;
}

/*****************************************************************************\

Function:
    CLinkedList::CConstIterator::operator==

Description:

Input:
    const CConstIterator& - pointer to the iterator to compare to.

Output:
    bool - true if this iterator and the passed-in iterator point to the same
           linked list node.

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::CConstIterator::operator==(
    const CConstIterator& iterator ) const
{
    return m_Ptr == iterator.m_Ptr;
}

/*****************************************************************************\

Function:
    CLinkedList::CConstIterator::operator!=

Description:

Input:
    const CConstIterator& - pointer to the iterator to compare to.

Output:
    bool - true if this iterator and the passed-in iterator point to different
           linked list nodes.

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::CConstIterator::operator!=(
    const CConstIterator& o ) const
{
    return m_Ptr != o.m_Ptr;
}

/*****************************************************************************\

Function:
    CLinkedList::CConstIterator::operator*

Description:
    Returns a const reference to the item that this iterator points to.

Input:
    none

Output:
    const Type& - reference to the item that this iterator points to.

\*****************************************************************************/
template<LinkedListTemplateList>
const Type& CLinkedListType::CConstIterator::operator*( void )
{
    return m_Ptr->GetElement();
}

/*****************************************************************************\

Function:
    CLinkedList::CConstIterator::IsNull

Description:
    Determines if the iterator has been assigned

Input:
    none

Output:
    bool

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::CConstIterator::IsNull( void ) const
{
    return ( m_Ptr == NULL );
}

/*****************************************************************************\

Function:
    CLinkedList::CConstIterator::SetNull

Description:
    Sets the iterator pointer to null

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::CConstIterator::SetNull( void )
{
    m_Ptr = NULL;
}

/*****************************************************************************\

Function:
    CLinkedList::IsEmpty

Description:
    Returns if the linked-list is empty

Input:
    none

Output:
    bool - true or false

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::IsEmpty( void ) const
{
    return ( m_DummyTail.GetNext() == &m_DummyTail );
}

/*****************************************************************************\

Function:
    CLinkedList::GetCount

Description:
    Returns the number of elements in the linked-list

Input:
    none

Output:
    DWORD - number of elements

\*****************************************************************************/
template<LinkedListTemplateList>
DWORD CLinkedListType::GetCount( void ) const
{
    return m_Count;
}

/*****************************************************************************\

Function:
    CLinkedList::operator=

Description:
    Copies a linked-list to this linked-list

Input:
    CLinkedListType &llist - linked-list to copy

Output:
    CLinkedListType& - *this

\*****************************************************************************/
template<LinkedListTemplateList>
CLinkedListType& CLinkedListType::operator= ( const CLinkedListType &llist )
{
    // Copy from back-to-front so we wind up with the same order of elements
    const CNode* pDummyTail = &llist.m_DummyTail;
    const CNode* pNode = pDummyTail->GetPrevious();

    while( pNode != pDummyTail )
    {
        Add( pNode->GetElement() );
        pNode = pNode->GetPrevious();
    }

    return *this;
}

/*****************************************************************************\

Function:
    CLinkedList::Begin

Description:
    Returns an iterator to the first node in the linked list.

Input:
    none

Output:
    CLinkedListType::CIterator

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CIterator
CLinkedListType::Begin( void )
{
    return CIterator( m_DummyTail.GetNext() );
}

/*****************************************************************************\

Function:
    CLinkedList::Get

Description:
    Returns an iterator to the nth node in the linked list.

Input:
    const DWORD index - index of node requesting

Output:
    CLinkedListType::CIterator

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CIterator
CLinkedListType::Get( const DWORD index )
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    CIterator iterator;

    if( index <= ( m_Count - 1 ) )
    {
        if( index <= ( m_Count / 2 ) )
        {
            iterator = Begin();

            for( DWORD i = 0; i < index; i++ )
            {
                ++iterator;
            }
        }
        else
        {
            iterator = End();

            for( DWORD i = m_Count; i > index; i-- )
            {
                --iterator;
            }
        }
    }
    else
    {
        iterator = End();
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return iterator;
}

/*****************************************************************************\

Function:
    CLinkedList::End

Description:
    Returns an iterator to the last (dummy) node in the linked list.

Input:
    none

Output:
    CLinkedListType::CIterator

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CIterator
CLinkedListType::End( void )
{
    return CIterator( &m_DummyTail );
}

/*****************************************************************************\

Function:
    CLinkedList::Begin

Description:
    Returns a const iterator to the first node in the linked list.

Input:
    none

Output:
    CLinkedListType::CConstIterator

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CConstIterator
CLinkedListType::Begin( void ) const
{
    return CConstIterator( m_DummyTail.GetNext() );
}

/*****************************************************************************\

Function:
    CLinkedList::Get

Description:
    Returns a const iterator to the nth node in the linked list.

Input:
    const DWORD index - index of node requesting

Output:
    CLinkedListType::CConstIterator

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CConstIterator
CLinkedListType::Get( const DWORD index ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    CConstIterator iterator;

    if( index <= ( m_Count - 1 ) )
    {
        if( index <= ( m_Count / 2 ) )
        {
            iterator = Begin();

            for( DWORD i = 0; i < index; i++ )
            {
                ++iterator;
            }
        }
        else
        {
            iterator = End();

            for( DWORD i = m_Count; i > index; i-- )
            {
                --iterator;
            }
        }
    }
    else
    {
        iterator = End();
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return iterator;
}

/*****************************************************************************\

Function:
    CLinkedList::End

Description:
    Returns a const iterator to the last (dummy) node in the linked list.

Input:
    none

Output:
    CLinkedListType::CConstIterator

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CConstIterator
CLinkedListType::End( void ) const
{
    return CConstIterator( &m_DummyTail );
}

/*****************************************************************************\

Function:
    CLinkedList::Find

Description:
    Finds the first instance of the specified element in the linked-list

Input:
    Type element - element to find

Output:
    CIterator - iterator to the node containing the element

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CIterator
CLinkedListType::Find( const Type &element )
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    CNode* pDummyTail = &m_DummyTail;
    CNode* pNode = m_DummyTail.GetNext();

    while( pNode != pDummyTail )
    {
        // If this node contains the element, then
        // return an iterator to this node
        if( pNode->GetElement() == element )
        {
            RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
            return CIterator( pNode );
        }

        // Get the next node
        pNode = pNode->GetNext();
    }

    // Node not found
    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return End();
}

/*****************************************************************************\

Function:
    CLinkedList::Find

Description:
    Finds the first instance of the specified element in the linked-list

Input:
    Type element - element to find

Output:
    CConstIterator - iterator to the node containing the element

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CConstIterator
CLinkedListType::Find( const Type &element ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    const CNode* pDummyTail = &m_DummyTail;
    const CNode* pNode = m_DummyTail.GetNext();

    while( pNode != pDummyTail )
    {
        // If this node contains the element, then
        // return an iterator to this node
        if( pNode->GetElement() == element )
        {
            RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
            return CConstIterator( pNode );
        }

        // Get the next node
        pNode = pNode->GetNext();
    }

    // Node not found
    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return End();
}

/*****************************************************************************\

Function:
    CLinkedList::Add

Description:
    Adds the specified element to the beginning of the linked-list

Input:
    const Type& element - element to add to the linked-list

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::Add( const Type &element )
{
    const bool success = Add( Begin(), element );
    return success;
}

/*****************************************************************************\

Function:
    CLinkedList::Remove

Description:
    Removes the first instance of the specified element from the linked-list

Input:
    Type element - element to remove

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::Remove( const Type &element )
{
    // Find the specified element in the list
    CIterator iterator = Find( element );
    const bool success = Remove( iterator );
    return success;
}

/*****************************************************************************\

Function:
    CLinkedList::Add

Description:
    Adds the specified element to the linked-list at the location specified by
    the passed-in iterator.  The item is added before the passed-in iterator.

Input:
    const CIterator& location - place in the list to add the element
    const Type& element - element to add to the linked-list

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::Add(
    const CIterator &location,
    const Type &element )
{
    // If necessary, make sure we do not add an element twice
    ASSERT( ( AreLinkedListDuplicatesSupported<Type>::value == true ) ||
            ( Find( element ) == End() ) );

    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    bool success = false;

    // Create a new object for the element
    CNode* pNode = CreateNode( element );

    if( pNode )
    {
        pNode->Insert( location.m_Ptr );

        m_Count++;

        // Element successfully added
        success = true;
    }
    else
    {
        ASSERT( 0 );

        // Element not added
        success = false;
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CLinkedList::Remove

Description:
    Removes the node specified by the passed-in iterator.

Input:
    const CIterator& - location to remove

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::Remove(
    const CIterator &location )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    bool success = false;

    if( location == End() )
    {
        // Element not removed
        success = false;
    }
    else
    {
        CNode* pNode = location.m_Ptr;

        success = Remove( pNode );
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CLinkedList::Clear

Description:
    clears the list

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::Clear( void )
{
    // Delete all objects on the linked-list
    while( !IsEmpty() )
    {
        Remove( Begin() );
    }
}

/*****************************************************************************\

Function:
    CLinkedList::Splice

Description:
    Splices some or all of a source list into this list.  Upon return the
    source list will be smaller than it was before (assuming the source
    iterator is not the list end).

    This would be a constant time operation, but we need to maintain the
    list of counts in each list.  As such, it is a linear time operation, 
    albeit a fast linear operation.

Input:
    const CIterator& dstLocation - location to put the source list's nodes;
                                   the source lists's nodes will be spliced 
                                   before this node
    const CIterator& srcLocation - the first node in the source list that will
                                   be spliced into this list
    CLinkedListType& srcList - source list to splice into this list

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::Splice(
    const CIterator &dstLocation,
    const CIterator &srcLocation,
    CLinkedListType &srcList )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    if( this != &srcList )
    {
        if( !srcList.IsEmpty() )
        {
            // Figure out how many nodes we're going to splice.
            CIterator countIterator = srcLocation;
            DWORD   count = 0;
            while( countIterator != srcList.End() )
            {
                count++;
                ++countIterator;
            }

            // Update the counts.

            m_Count += count;
            srcList.m_Count -= count;

            // Actually do the splicing.

            CNode* pDstNode = dstLocation.m_Ptr;

            CNode* pSrcStartNode = srcLocation.m_Ptr;
            CNode* pSrcEndNode = srcList.m_DummyTail.GetPrevious();

            pSrcStartNode->GetPrevious()->SetNext( &srcList.m_DummyTail );
            srcList.m_DummyTail.SetPrevious( pSrcStartNode->GetPrevious() );

            pDstNode->GetPrevious()->SetNext( pSrcStartNode );
            pSrcStartNode->SetPrevious( pDstNode->GetPrevious() );

            pSrcEndNode->SetNext( pDstNode );
            pDstNode->SetPrevious( pSrcEndNode );
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CLinkedList::SpliceListPartAtTheEnd

Description:
    Splices some or all (interval) of a source list at the end of
    this list. 
    
    Upon return the source list will be smaller than it was before
    (assuming the source iterator is not the list end).

    This would be a constant time operation, but we need to maintain the
    list of counts in each list.  As such, it is a linear time operation, 
    albeit a fast linear operation.

Input:
    CLinkedListType& srcList - source list to splice into this list.

    const CIterator &srcStartLocation - the first node in the source list that will
        be spliced into this list.

    const CIterator &srcEndLocation - the first node AFTER the last 
        in the source list that will be spliced into this list 
        (this element will not be moved).


Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::SpliceListPartAtTheEnd(
    CLinkedListType &srcList,
    const CIterator &srcStartLocation,
    const CIterator &srcEndLocation )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    if( this != &srcList )
    {
        if( !srcList.IsEmpty() && 
            ( srcStartLocation.m_Ptr != srcEndLocation.m_Ptr ) )
        {
            // Figure out how many nodes we're going to splice.
            CIterator countIterator = srcStartLocation;
            DWORD   count = 0;
            while( countIterator != srcEndLocation )
            {
                count++;
                ++countIterator;
            }

            // Update the counts.

            m_Count += count;
            srcList.m_Count -= count;

            // Actually do the splicing.

            // last before End.
            CNode* pDstNode = m_DummyTail.GetPrevious();

            CNode* pSrcStartNode = srcStartLocation.m_Ptr;
            CNode* pSrcEndNode = srcEndLocation.m_Ptr->GetPrevious();

            // Seal the gap after removed part
            pSrcStartNode->GetPrevious()->SetNext( srcEndLocation.m_Ptr );
            srcEndLocation.m_Ptr->SetPrevious( pSrcStartNode->GetPrevious() );

            // splice in.
            pDstNode->SetNext( pSrcStartNode );
            pSrcStartNode->SetPrevious( pDstNode );

            pSrcEndNode->SetNext( &m_DummyTail );
            m_DummyTail.SetPrevious( pSrcEndNode );
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}


/*****************************************************************************\

Function:
    CLinkedList::Set

Description:
    Sets the contents of the node pointed to by location with element
    provided

Input:
    const CIterator &location - location to set the element
    const Type &element - the element to set

Output:
    bool

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::Set(
    const CIterator &location,
    const Type &element )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    if( location.m_Ptr )
    {
        location.m_Ptr->SetElement( element );
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CLinkedList::DeleteFreePool

Description:
    Deletes all memory on the free pool

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::DeleteFreePool( void )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    // Delete all objects on the free pool list
    while( m_FreePoolDummyTail.GetNext() != &m_FreePoolDummyTail )
    {
        CNode* pNode = m_FreePoolDummyTail.GetNext();
        pNode->Remove();
        SafeDelete( pNode );
    }

    m_FreePoolCount = 0;

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CLinkedList::DebugPrint

Description:
    Prints the linked-list to std output for debug only

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::DebugPrint( void ) const
{
#ifdef _DEBUG
    DPF( GFXDBG_STDLIB, "%s\n", __FUNCTION__ );
    DPF( GFXDBG_STDLIB, "\tAddress = %p\n", this );
    DPF( GFXDBG_STDLIB, "\tType = %s\n", TYPEID_NAME(Type) );

    const CNode* pDummyTail = &m_DummyTail;
    CNode* pNode = m_DummyTail.GetNext();

    while( pNode != pDummyTail )
    {
        DWORD index = 0;
        DPF( GFXDBG_STDLIB, "\t\tLinkedListNode[%u] = 0x%08x\n",
            index,
            *(DWORD*)&pNode->GetElement() );
        index++;

        // Get the next node
        pNode = pNode->GetNext();
    }
#endif
}

/*****************************************************************************\

Function:
    CLinkedList::Remove

Description:
    Removes the specified node from the linked-list

Input:
    CNode* pNode - object to remove

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
bool CLinkedListType::Remove( CNode* &pNode )
{
    bool success = false;

    if( pNode )
    {
        // Remove the node from the linked-list
        pNode->Remove();

        // Delete the node
        DeleteNode( pNode );
        m_Count--;

        success = true;
    }

    return success;
}

/*****************************************************************************\

Function:
    CLinkedList::CreateNode

Description:
    Creates a node

Input:
    const Type &element

Output:
    CNode*

\*****************************************************************************/
template<LinkedListTemplateList>
typename CLinkedListType::CNode* CLinkedListType::CreateNode( const Type &element )
{
    CNode* pNode = NULL;

    if( m_FreePoolDummyTail.GetNext() != &m_FreePoolDummyTail )
    {
        pNode = m_FreePoolDummyTail.GetNext();
        pNode->Remove();
        --m_FreePoolCount;

        pNode->SetElement( element );
    }
    else
    {
        pNode = new CNode( element );
    }

    return pNode;
}

/*****************************************************************************\

Function:
    CLinkedList::DeleteNode

Description:
    Deletes a node

Input:
    CNode* pNode

Output:
    none

\*****************************************************************************/
template<LinkedListTemplateList>
void CLinkedListType::DeleteNode( CNode* &pNode )
{
    const DWORD cMaxFreePoolCount = 32;
    if( m_FreePoolCount <= cMaxFreePoolCount )
    {
        pNode->Insert( m_FreePoolDummyTail.GetNext() );
        ++m_FreePoolCount;
    }
    else
    {
        SafeDelete( pNode );
    }
}

} // iSTD
