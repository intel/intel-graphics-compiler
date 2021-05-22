/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Macros.h"
#include "Object.h"
#include "utility.h"
#include "Threading.h"

namespace iSTD
{    

// Macro used for calculation of next stack node (undefined at the bottom of file). 
// Rounding to the higher number is used.
#define CALCULATE_NEXT_NODE_SIZE( prevSize, growthFactor ) \
    ((prevSize * growthFactor + 99) / 100)

/*****************************************************************************\
Template Parameters:
    ElemType        - type of stored element.
    CAllocatorType  - allocator used for stack node allocations.
    BaseSize        - Size of statically allocated buffer. Base for further 
                      growth.
    GrowthFactor    - Defines the size of the next allocated stack node. 
                      E.g. if 100, size will not change, if 200 next node will 
                      be twice as large as the previous one.
\*****************************************************************************/
#define StackTemplateList \
    class ElemType, class CAllocatorType, DWORD BaseSize, DWORD GrowthFactor
#define StackDeclTemplateList \
    class ElemType, class CAllocatorType, DWORD BaseSize = 4, DWORD GrowthFactor = 135
#define CStackType \
    CStack< ElemType, CAllocatorType, BaseSize, GrowthFactor >

/*****************************************************************************\

Class:
    CStack

Description:
    Stack class that can be optimized for number of dynamic allocations. 
    It also supports storing limited number of elements (define with BaseSize 
    template parameter) without any heap allocations.

\*****************************************************************************/
template <StackDeclTemplateList> 
class CStack : public CObject<CAllocatorType>
{
    ISTD_DISALLOW_COPY_AND_ASSIGN( CStack );

    // Make sure we won't get size 0:
    C_ASSERT( CALCULATE_NEXT_NODE_SIZE(BaseSize, GrowthFactor) > 0 );

public:
    CStack();
    ~CStack();

    DWORD   GetCount( void ) const;
    bool    IsEmpty( void ) const;

    bool        Push(ElemType element);
    ElemType    Pop( void );
    ElemType    Top( void ) const;

    bool        Contains( const ElemType& elem ) const;

protected:
    /*************************************************************************\

    Class:
        CStackNode

    Description:
        Nodes are used to store elements on the heap. Nodes are linked with 
        LIFO queue with pointer the the parent (node under current).

    \*************************************************************************/
    class CStackNode : CObject<CAllocatorType>
    {
        ISTD_DISALLOW_COPY_AND_ASSIGN( CStackNode );

    public:
        static bool     Create( 
            DWORD size, 
            CStackNode*& pNewNode );

        static bool     Create( 
            DWORD size, 
            CStackNode* pParentNode, 
            CStackNode*& pNewNode );

        static void     Delete( 
            CStackNode*& pNode );

        bool    IsEmpty( void ) const;
        bool    IsFull( void ) const;

        DWORD   GetMaxSize( void ) const;

        CStackNode* GetParentNode( void ) const;

        void        Push(ElemType element);
        ElemType    Pop( void );
        ElemType    Top( void ) const;

        bool        Contains( const ElemType& elem ) const;

    private:
        CStackNode( DWORD size );
        CStackNode( DWORD size, CStackNode* pParentNode );
        ~CStackNode();

        bool    Initialize( void );

        const DWORD     m_cMaxSize;
        DWORD           m_Count;
        ElemType*       m_pElements;

        CStackNode*     m_pParentNode;
    };

    // Size of statically allocated buffer:
    static const DWORD m_cStaticSize = BaseSize;

    // Current number of elements:
    DWORD m_Count;

    // Top stack node. Null if all elements fits in m_StaticArray.
    CStackNode* m_pTopNode;

    // Static buffer for limited number of elements:
    ElemType m_StaticArray[m_cStaticSize];

    DECL_DEBUG_MUTEX( m_InstanceNotThreadSafe )
};

/*****************************************************************************\

Function:
   CStack constructor.

\*****************************************************************************/
template<StackTemplateList>
CStackType::CStack() 
    : m_Count( 0 ), 
      m_pTopNode( NULL ) 
{
    INIT_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
   CStack destructor.

\*****************************************************************************/
template<StackTemplateList>
CStackType::~CStack()
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    // Free stack nodes if there are any:
    while( m_pTopNode )
    {
        CStackNode* pParent = m_pTopNode->GetParentNode();
        CStackNode::Delete( m_pTopNode );
        m_pTopNode = pParent;
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    DELETE_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
   CStack::GetCount

Description:
    Get current number of elements on the stack.

Input:

Output:
    DWORD

\*****************************************************************************/
template<StackTemplateList>
DWORD CStackType::GetCount( void ) const
{
    return m_Count;
}

/*****************************************************************************\

Function:
   CStack::Push

Description:
    Push element on the stack.

Input:
    ElemType element

Output:
    bool    - false for allocation failure

\*****************************************************************************/
template<StackTemplateList>
bool CStackType::Push( ElemType element )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    bool success = true;

    if( m_Count < m_cStaticSize )
    {
        // Add to static array:
        m_StaticArray[m_Count++] = element;
    }
    else
    {
        // Element doesn't fit in static array.
        if( m_pTopNode == NULL )
        {
            // Allocate first stack node:
            const DWORD newSize = 
                CALCULATE_NEXT_NODE_SIZE( m_cStaticSize, GrowthFactor );
            success = CStackNode::Create( newSize, m_pTopNode );
        }
        else if( m_pTopNode->IsFull() )
        {
            // Top stack node full - allocate another:
            const DWORD newSize = 
                CALCULATE_NEXT_NODE_SIZE( m_pTopNode->GetMaxSize(), GrowthFactor );
            success = CStackNode::Create( newSize, m_pTopNode,  m_pTopNode );
        }

        if( success )
        {
            ++m_Count;
            m_pTopNode->Push( element );
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
   CStack::Pop

Description:
    Pop element from the stack.

Input:

Output:
    ElemType    - Top element.

\*****************************************************************************/
template<StackTemplateList>
ElemType CStackType::Pop( void )
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    ASSERT( !IsEmpty() );

    ElemType elem;

    if( m_Count <= m_cStaticSize )
    {
        // Element from static array.
        elem = m_StaticArray[--m_Count];
    }
    else
    {
        // Element from stack node.
        if( m_pTopNode->IsEmpty() )
        {
            // Top node empty - proceed to next one.
            CStackNode* pParentNode = m_pTopNode->GetParentNode();
            CStackNode::Delete( m_pTopNode );
            m_pTopNode = pParentNode;

            ASSERT( m_pTopNode );
        }

        --m_Count;
        elem = m_pTopNode->Pop();
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return elem;
}

/*****************************************************************************\

Function:
   CStack::Top

Description:
    Get the top element without popping it from the stack.

Input:

Output:
    ElemType    - Top element.

\*****************************************************************************/
template<StackTemplateList>
ElemType CStackType::Top( void ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    ASSERT( !IsEmpty() );

    ElemType elem;

    if( m_Count <= m_cStaticSize )
    {
        // Element from static array.
        elem = m_StaticArray[m_Count - 1];
    }
    else
    {
        // Element from stack node.
        if( m_pTopNode->IsEmpty() )
        {
            // Top node empty - proceed to next one.
            ASSERT( m_pTopNode->GetParentNode() );
            elem = m_pTopNode->GetParentNode()->Top();
        }
        else
        {
            elem = m_pTopNode->Top();
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return elem;
}

/*****************************************************************************\

Function:
   CStack::IsEmpty

Description:
    Check if stack is empty.

Input:

Output:
    bool

\*****************************************************************************/
template<StackTemplateList>
bool CStackType::IsEmpty( void ) const
{
    return ( m_Count == 0 );
}

/*****************************************************************************\

Function:
   CStack::Contains

Description:
    Check if given element is already on the stack.

Input:
    elem        - Element to be checked.

Output:
    bool

\*****************************************************************************/
template<StackTemplateList>
bool CStackType::Contains( const ElemType& elem ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    bool found = false;

    if( m_Count >= m_cStaticSize )
    {
        // Search stack nodes:
        CStackNode* pNode = m_pTopNode;
        while( pNode )
        {
            if( pNode->Contains( elem ) )
            {
                // Element found!
                found = true;
                break;
            }
            pNode = pNode->GetParentNode();
        }
    }
    
    if( !found )
    {
        // Search static array:
        DWORD elemNumber = iSTD::Min( m_cStaticSize, m_Count );
        while( elemNumber-- )
        {
            if( m_StaticArray[ elemNumber ] == elem )
            {
                // Element found!
                found = true;
                break;
            }
        }
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return found;
}

/*****************************************************************************\

Function:
   CStack::CStackNode constructor

\*****************************************************************************/
template<StackTemplateList>
CStackType::CStackNode::CStackNode( DWORD size ) 
    : m_cMaxSize( size ), 
      m_Count( 0 ),
      m_pElements( NULL ),
      m_pParentNode( NULL )
{
}

/*****************************************************************************\

Function:
   CStack::CStackNode constructor

\*****************************************************************************/
template<StackTemplateList>
CStackType::CStackNode::CStackNode( DWORD size, CStackNode* pParentNode ) 
    : m_cMaxSize( size ), 
      m_Count( 0 ), 
      m_pElements( NULL ),
      m_pParentNode( pParentNode )
{
}

/*****************************************************************************\

Function:
   CStack::CStackNode destructor

\*****************************************************************************/
template<StackTemplateList>
CStackType::CStackNode::~CStackNode()
{
    CAllocatorType::Deallocate( m_pElements );
}

/*****************************************************************************\

Function:
   CStack::CStackNode::Initialize

Description:
    Allocate element buffer.

Input:

Output:
    bool success

\*****************************************************************************/
template<StackTemplateList>
bool CStackType::CStackNode::Initialize( void )
{
    m_pElements = 
        (ElemType*)CAllocatorType::Allocate( sizeof(ElemType) * m_cMaxSize );
    return ( m_pElements != NULL );
}

/*****************************************************************************\

Function:
   CStack::CStackNode::Create

Description:
    Create new stack node.

Input:
    size

Output:
    pNewNode

\*****************************************************************************/
template<StackTemplateList>
bool CStackType::CStackNode::Create( 
    DWORD size, 
    CStackNode*& pNewNode )
{
    bool success = true;
    pNewNode = new CStackNode( size );

    if( pNewNode )
    {
        pNewNode->Acquire();

        success = pNewNode->Initialize();

        if( success == false )
        {
            CStackNode::Delete( pNewNode );
        }
    }
    else
    {
        ASSERT(0);
        success = false;
    }
    return success;
}

/*****************************************************************************\

Function:
   CStack::CStackNode::Create

Description:
    Create new stack node and attach it to given parent node.

Input:
    size
    pParentNode

Output:
    pNewNode

\*****************************************************************************/
template<StackTemplateList>
bool CStackType::CStackNode::Create( 
    DWORD size, 
    CStackNode* pParentNode, 
    CStackNode*& pNewNode )
{
    bool success = true;
    pNewNode = new CStackNode( size, pParentNode );

    if( pNewNode )
    {
        pNewNode->Acquire();

        success = pNewNode->Initialize();

        if( success == false )
        {
            CStackNode::Delete( pNewNode );
        }
    }
    else
    {
        ASSERT(0);
        success = false;
    }
    return success;
}

/*****************************************************************************\

Function:
   CStack::CStackNode::Delete

Description:
    Delete stack node.

Input:
    pNode

Output:

\*****************************************************************************/
template<StackTemplateList>
void CStackType::CStackNode::Delete( CStackNode*& pNode )
{
    CObject<CAllocatorType>::SafeRelease( pNode );
    pNode = NULL;
}

/*****************************************************************************\

Function:
   CStack::CStackNode::

Description:
    Check if stack node is empty.

Input:

Output:
    bool

\*****************************************************************************/
template<StackTemplateList>
bool CStackType::CStackNode::IsEmpty( void ) const
{
    return ( m_Count == 0 );
}

/*****************************************************************************\

Function:
   CStack::CStackNode::IsFull

Description:
    Check if stack node is full.

Input:

Output:
    bool

\*****************************************************************************/
template<StackTemplateList>
bool CStackType::CStackNode::IsFull( void ) const
{
    ASSERT( m_Count <= m_cMaxSize );
    return ( m_Count == m_cMaxSize );
}

/*****************************************************************************\

Function:
   CStack::CStackNode::GetMaxSize

Description:
    Get capacity of this stack node.

Input:

Output:
    DWORD m_cMaxSize

\*****************************************************************************/
template<StackTemplateList>
DWORD CStackType::CStackNode::GetMaxSize( void ) const
{
    return m_cMaxSize;
}

/*****************************************************************************\

Function:
   CStack::CStackNode::GetParentNode

Description:
    Get parent stack node.

Input:

Output:
    CStackNode* m_pParentNode

\*****************************************************************************/
template<StackTemplateList>
typename CStackType::CStackNode* CStackType::CStackNode::GetParentNode( void ) const
{
    return m_pParentNode;
}

/*****************************************************************************\

Function:
   CStack::CStackNode::Push

Description:
    Push element on the stack node.

Input:
    ElemType element

Output:

\*****************************************************************************/
template<StackTemplateList>
void CStackType::CStackNode::Push(ElemType element)
{
    ASSERT( !IsFull() );
    m_pElements[m_Count++] = element;
}

/*****************************************************************************\

Function:
   CStack::CStackNode::Pop

Description:
    Pop element from the stack node.

Input:

Output:
    ElemType    - Top element.

\*****************************************************************************/
template<StackTemplateList>
ElemType CStackType::CStackNode::Pop( void )
{
    ASSERT( !IsEmpty() );
    return m_pElements[--m_Count];
}

/*****************************************************************************\

Function:
   CStack::CStackNode::Top

Description:
    Get the top element without popping it from the stack node.

Input:

Output:
    ElemType    - Top element.

\*****************************************************************************/
template<StackTemplateList>
ElemType CStackType::CStackNode::Top( void ) const
{
    ASSERT( !IsEmpty() );
    return m_pElements[m_Count - 1];
}

/*****************************************************************************\

Function:
   CStack::CStackNode::Contains

Description:
    Check if given element is already on the stack.

Input:
    elem        - Element to be checked.

Output:
    bool

\*****************************************************************************/
template<StackTemplateList>
bool CStackType::CStackNode::Contains( const ElemType& elem ) const
{
    DWORD elemNumber = m_Count;
    while( elemNumber-- )
    {
        if( m_pElements[elemNumber] == elem )
        {
            return true;
        }
    }

    return false;
}

#undef CALCULATE_NEXT_NODE_SIZE

} // namespace iSTD
