/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Object.h"
#include "Stack.h"
#include "String.h"

namespace iSTD
{

/*****************************************************************************\
Struct: IsBinaryTreeTypeSupported
\*****************************************************************************/
template<typename T> 
struct IsBinaryTreeTypeSupported                    { enum { value = false }; };

template<> 
struct IsBinaryTreeTypeSupported<bool>              { enum { value = true }; };

template<> 
struct IsBinaryTreeTypeSupported<char>              { enum { value = true }; };

template<> 
struct IsBinaryTreeTypeSupported<unsigned char>     { enum { value = true }; };

template<> 
struct IsBinaryTreeTypeSupported<int>               { enum { value = true }; };

template<> 
struct IsBinaryTreeTypeSupported<unsigned int>      { enum { value = true }; };

#ifndef __LP64__ // u/long on linux64 platform is 64-bit type and collides with U/INT64
template<> 
struct IsBinaryTreeTypeSupported<long>              { enum { value = true }; };

template<> 
struct IsBinaryTreeTypeSupported<unsigned long>     { enum { value = true }; };
#endif

template<> 
struct IsBinaryTreeTypeSupported<float>             { enum { value = true }; };

template<> 
struct IsBinaryTreeTypeSupported<INT64>           { enum { value = true }; };

template<> 
struct IsBinaryTreeTypeSupported<UINT64>  { enum { value = true }; };

template<typename T> 
struct IsBinaryTreeTypeSupported<T*>                { enum { value = true }; };

/*****************************************************************************\
Template Parameters
\*****************************************************************************/
#define BinaryTreeTemplateList  class KeyType, class ElementType, class CAllocatorType
#define CBinaryTreeType         CBinaryTree<KeyType, ElementType, CAllocatorType>

/*****************************************************************************\

Class:
    CBinaryTree

Description:
    Represents an unbalanced binary search tree

\*****************************************************************************/
template<BinaryTreeTemplateList>
class CBinaryTree : public CObject<CAllocatorType>
{
public:

    CBinaryTree( void );
    virtual ~CBinaryTree( void );

    bool    Add( 
                const KeyType &key,
                const ElementType &element );

    bool    Get(
                const KeyType &key,
                ElementType &element ) const;

    bool    GetNext(
                KeyType &key,
                ElementType &element );

    bool    GetMin(
                ElementType &element ) const;

    bool    GetMax(
                ElementType &element ) const;

    bool    GetMin(
                KeyType &key,
                ElementType &element ) const;

    bool    GetMax(
                KeyType &key,
                ElementType &element ) const;

    bool    Remove( 
                const KeyType &key );

    bool    Remove( 
                const KeyType &key,
                ElementType &element );

    bool    RemoveMin(
                ElementType &element );

    bool    RemoveMax(
                ElementType &element );

    bool    RemoveMin(
                KeyType &key,
                ElementType &element );

    bool    RemoveMax(
                KeyType &key,
                ElementType &element );

    void    RemoveAll( void );

    bool    IsEmpty( void ) const;
    DWORD   GetCount( void ) const;

    void    DebugPrint( void ) const;

    C_ASSERT( IsBinaryTreeTypeSupported<ElementType>::value == true );

protected:

    struct SNode
    {
        KeyType     m_Key;
        ElementType m_Element;

#ifdef _DEBUG
        DWORD   m_GetCount;
#endif

        SNode*  m_RightNode;
        SNode*  m_LeftNode;
    };

    bool    AddElement( 
                SNode* &pTree, 
                const KeyType &key,
                const ElementType &element );

    bool    GetElement( 
                SNode* pTree, 
                const KeyType &key,
                ElementType &element ) const;

    bool    GetMinElement( 
                SNode* pTree, 
                KeyType &key,
                ElementType &element ) const;

    bool    GetMaxElement( 
                SNode* pTree, 
                KeyType &key,
                ElementType &element ) const;

    bool    GetNextElement(
                SNode* pTree, 
                KeyType &key,
                ElementType &element );

    bool    RemoveElement( 
                SNode* &pTree, 
                const KeyType &key,
                ElementType &element );

    bool    RemoveMinElement( 
                SNode* &pTree, 
                KeyType &key,
                ElementType &element );

    bool    RemoveMaxElement( 
                SNode* &pTree, 
                KeyType &key,
                ElementType &element );

    bool    AddTree(
                SNode* &pTree, 
                SNode* pNewTree );

    void    DeleteTree( 
                SNode* &pTree );

    void    DebugPrintTree(
                const SNode* pTree,
                const DWORD depth ) const;

    SNode*  CreateNode(
                const KeyType &key,
                const ElementType &element );

    SNode*  RemoveMinNode(
                SNode* &pTree );

    SNode*  RemoveMaxNode(
                SNode* &pTree );

    void    DeleteNode( 
                SNode* &pNode );

    SNode*  m_pTree;
    DWORD   m_Count;

#ifdef _DEBUG
    DWORD   m_AddCount;
    DWORD   m_RemoveCount;
    DWORD   m_MaxDepth;
#endif

    DECL_DEBUG_MUTEX( m_InstanceNotThreadSafe )
};

/*****************************************************************************\

Function:
    CBinaryTree Constructor

Description:
    Initializes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<BinaryTreeTemplateList>
CBinaryTreeType::CBinaryTree( void )
    : CObject<CAllocatorType>()
{
    m_pTree = NULL;
    m_Count = 0;

#ifdef _DEBUG
    m_AddCount = 0;
    m_RemoveCount = 0;
    m_MaxDepth = 0;
#endif

    INIT_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBinaryTree Destructor

Description:
    Deletes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<BinaryTreeTemplateList>
CBinaryTreeType::~CBinaryTree( void )
{
    DeleteTree( m_pTree );

    DELETE_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBinaryTree::Add

Description:
    Adds an element to the binary tree

Input:
    const KeyType &key - key which determines sorting order of element
    const ElementType &element - element to add to binary tree

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::Add(
    const KeyType &key,
    const ElementType &element )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    const bool success = AddElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::Get

Description:
    Gets the element in the binary tree

Input:
    const KeyType &key - key which determines sorting order of element

Output:
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::Get(
    const KeyType &key,
    ElementType &element ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    const bool success = GetElement( m_pTree, key, element );

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::GetMin

Description:
    Gets the "minimum" element in the binary tree

Input:
    none

Output:
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::GetMin(
    ElementType &element ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    KeyType key;
    const bool success = GetMinElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::GetMax

Description:
    Gets the "maximum" element in the binary tree

Input:
    none

Output:
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::GetMax(
    ElementType &element ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    KeyType key;
    const bool success = GetMaxElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::GetNext

Description:
    Gets the next element in the binary tree

Input:
    none

Output:
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::GetNext(
    KeyType &key,
    ElementType &element )
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    const bool success = GetNextElement( m_pTree, key, element );    

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::GetMin

Description:
    Gets the "minimum" element in the binary tree

Input:
    none

Output:
    KeyType &key - key of the minimum element
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::GetMin(
    KeyType &key,
    ElementType &element ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    const bool success = GetMinElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::GetMax

Description:
    Gets the "maximum" element in the binary tree

Input:
    none

Output:
    KeyType &key - key of the minimum element
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::GetMax(
    KeyType &key,
    ElementType &element ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    const bool success = GetMaxElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::Remove

Description:
    Removes an element from the binary tree

Input:
    const KeyType &key - key which determines sorting order of element

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::Remove(
    const KeyType &key )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    ElementType element;
    const bool success = RemoveElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::Remove

Description:
    Removes an element from the binary tree

Input:
    const KeyType &key - key which determines sorting order of element

Output:
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::Remove(
    const KeyType &key,
    ElementType &element )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    const bool success = RemoveElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::RemoveMin

Description:
    Removes the minimum element from the binary tree

Input:
    none

Output:
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::RemoveMin(
    ElementType &element )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    KeyType key;
    const bool success = RemoveMinElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::RemoveMax

Description:
    Removes the maximum element from the binary tree

Input:
    none

Output:
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::RemoveMax(
    ElementType &element )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    KeyType key;
    const bool success = RemoveMaxElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::RemoveMin

Description:
    Removes the minimum element from the binary tree

Input:
    none

Output:
    KeyType &key - key of the minimum element
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::RemoveMin(
    KeyType &key,
    ElementType &element )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    const bool success = RemoveMinElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::RemoveMax

Description:
    Removes the maximum element from the binary tree

Input:
    none

Output:
    KeyType &key - key of the minimum element
    ElementType &element - element in binary tree
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::RemoveMax(
    KeyType &key,
    ElementType &element )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    const bool success = RemoveMaxElement( m_pTree, key, element );
    ASSERT( success );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CBinaryTree::RemoveAll

Description:
    Removes all elements from the binary tree

Input:
    void

Output:
    void

\*****************************************************************************/
template<BinaryTreeTemplateList>
void CBinaryTreeType::RemoveAll( void )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    DeleteTree( m_pTree );

#ifdef _DEBUG
    m_RemoveCount += m_Count;
#endif

    m_Count = 0;

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CBinaryTree::IsEmpty

Description:
    Determines if the binary tree is empty

Input:
    void

Output:
    bool

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::IsEmpty( void ) const
{
    const bool isEmpty = ( m_pTree == NULL );
    return isEmpty;
}

/*****************************************************************************\

Function:
    CBinaryTree::GetCount

Description:
    Returns the number of nodes in the tree

Input:
    void

Output:
    DWORD

\*****************************************************************************/
template<BinaryTreeTemplateList>
DWORD CBinaryTreeType::GetCount( void ) const
{
    const DWORD count = m_Count;
    return count;
}

/*****************************************************************************\

Function:
    CBinaryTree::DebugPrint

Description:
    Prints the tree to std output for debug only

Input:
    void

Output:
    void

\*****************************************************************************/
template<BinaryTreeTemplateList>
void CBinaryTreeType::DebugPrint( void ) const
{
#ifdef _DEBUG
    if( m_pTree )
    {
        DPF( GFXDBG_STDLIB, "%s\n", __FUNCTION__ );
        DPF( GFXDBG_STDLIB, "\tAddress = %p\n", this );
        DPF( GFXDBG_STDLIB, "\tCount = %d\n", m_Count );
        DPF( GFXDBG_STDLIB, "\tAddCount = %d\n", m_AddCount );
        DPF( GFXDBG_STDLIB, "\tRemoveCount = %d\n", m_RemoveCount );
        DPF( GFXDBG_STDLIB, "\tMaxDepth = %d\n", m_MaxDepth );

        DebugPrintTree( m_pTree, 0 );
    }
#endif
}

/*****************************************************************************\

Function:
    CBinaryTree::AddElement

Description:
    Adds an element to the tree

Input:
    SNode* &pTree - pointer to tree to add element
    const KeyType &key - key which determines sorting order of element
    const ElementType &element - element to add

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::AddElement( 
    SNode* &pTree, 
    const KeyType &key,
    const ElementType &element )
{
    SNode** ppNode = &pTree;
    SNode* pNode = *ppNode;

#ifdef _DEBUG
    DWORD depth = 1;
#endif

    while( pNode )
    {
        // Two elements should not have the same key
        if( pNode->m_Key == key )
        {
            if( pNode->m_Element == element )
            {
                // the identical node already exists in the tree
                return true;
            }
            else
            {
                ASSERT(0);
                return false;
            }
        }
        // Add element to left sub-tree
        else if( key < pNode->m_Key )
        {
            ppNode = &(pNode->m_LeftNode);

#ifdef _DEBUG
            ++depth;
#endif
        }
        // Add element to right sub-tree
        else
        {
            ppNode = &(pNode->m_RightNode);

#ifdef _DEBUG
            ++depth;
#endif
        }

        pNode = *ppNode;
    }

    pNode = CreateNode( key, element );

    if( pNode )
    {
        *ppNode = pNode;

#ifdef _DEBUG
        ++m_AddCount;
        m_MaxDepth = iSTD::Max<DWORD>( m_MaxDepth, depth );
#endif
        return true;
    }
    else
    {
        return false;
    }
}

/*****************************************************************************\

Function:
    CBinaryTree::GetElement

Description:
    Gets the element from a tree

Input:
    SNode* pTree - pointer to tree to search
    const KeyType &key - key which determines sorting order of element

Output:
    ElementType &element - element found
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::GetElement( 
    SNode* pTree, 
    const KeyType &key,
    ElementType &element ) const
{
    SNode* pNode = pTree;

    while( pNode )
    {
        // Do keys match?
        if( pNode->m_Key == key )
        {
            // Element found
            element = pNode->m_Element;

#ifdef _DEBUG
            pNode->m_GetCount++;
#endif
            return true;
        }
        // Search left sub-tree
        else if( key < pNode->m_Key )
        {
            pNode = pNode->m_LeftNode;
        }
        // Search right sub-tree
        else
        {
            pNode = pNode->m_RightNode;
        }
    }

    // Could not find the element
    return false;
}

/*****************************************************************************\

Function:
    CBinaryTree::GetMinElement

Description:
    Gets the "minimum" element from a sub-tree

Input:
    SNode* pTree - pointer to sub-tree to search

Output:
    KeyType &key - key of element
    ElementType &element - element found
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::GetMinElement( 
    SNode* pTree, 
    KeyType &key,
    ElementType &element ) const
{
    SNode* pNode = pTree;

    while( pNode )
    {
        if( pNode->m_LeftNode )
        {
            pNode = pNode->m_LeftNode;
        }
        else
        {
            key = pNode->m_Key;
            element = pNode->m_Element;

#ifdef _DEBUG
            pNode->m_GetCount++;
#endif
            return true;
        }
    }

    return false;
}

/*****************************************************************************\

Function:
    CBinaryTree::GetMaxElement

Description:
    Gets the "maximum" element from a sub-tree

Input:
    SNode* pTree - pointer to sub-tree to search

Output:
    KeyType &key - key of element
    ElementType &element - element found
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::GetMaxElement( 
    SNode* pTree, 
    KeyType &key,
    ElementType &element ) const
{
    SNode* pNode = pTree;

    while( pNode )
    {
        if( pNode->m_RightNode )
        {
            pNode = pNode->m_RightNode;
        }
        else
        {
            key = pNode->m_Key;
            element = pNode->m_Element;

#ifdef _DEBUG
            pNode->m_GetCount++;
#endif
            return true;
        }
    }

    return false;
}

/*****************************************************************************\

Function:
    CBinaryTree::GetNextElement

Description:
    Gets the next element from a sub-tree

Input:
    SNode* pTree - pointer to sub-tree to search

Output:
    KeyType &key - key of element
    ElementType &element - element found
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::GetNextElement(
    SNode* pTree, 
    KeyType &key,
    ElementType &element )
{
    static const DWORD prev_count = 1024;
    DWORD prev_pos = 0;
    SNode* pNode = pTree;
    SNode* prev[ prev_count ];

    prev[ prev_pos ] = 0;
    
    while( pNode )
    {
        // Do keys match?
        if( pNode->m_Key == key )
        {
            // Element found
            if( pNode->m_RightNode )
            {
                // there is right child - return it or leftmost node of it's subtree
                pNode = pNode->m_RightNode;
                while( pNode->m_LeftNode ) 
                {
                    pNode = pNode->m_LeftNode;
                }
                key = pNode->m_Key;
                element = pNode->m_Element;
                return true;
            }
            else
            {
                // there is no right child
                // must traverse up as long as node is right child of it's parent
                // ...and parent is not NULL
                while(    prev[ prev_pos ]!=0 
                       && prev[ prev_pos ]->m_RightNode == pNode )
                {
                    pNode = prev[ prev_pos ];
                    --prev_pos;
                }
                if( prev[ prev_pos ]==0 )
                {
                    // node is root - there is no 'next'
                    return false;
                }
                else
                {   // node is not root - it has to be left child of it's parent
                    key = prev[ prev_pos ]->m_Key;
                    element = prev[ prev_pos ]->m_Element;
                    return true;
                }
            }
        }
        // Search left sub-tree
        else if( key < pNode->m_Key )
        {
            ++prev_pos;
            ASSERT( prev_pos < prev_count );
            prev[ prev_pos ] = pNode;
            pNode = pNode->m_LeftNode;
        }
        // Search right sub-tree
        else
        {
            ++prev_pos;
            ASSERT( prev_pos < prev_count );
            prev[ prev_pos ] = pNode;
            pNode = pNode->m_RightNode;
        }
    }

    // Could not find the element
    return false;
}


/*****************************************************************************\

Function:
    CBinaryTree::RemoveElement

Description:
    Deletes the element from a sub-tree

Input:
    SNode* &pTree - pointer to sub-tree to search
    const KeyType &key - key which determines sorting order of element

Output:
    ElementType &element - element found
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::RemoveElement( 
    SNode* &pTree, 
    const KeyType &key,
    ElementType &element )
{
    SNode** ppNode = &pTree;
    SNode* pNode = *ppNode;

    while( pNode )
    {
        // Do keys match?
        if( pNode->m_Key == key )
        {
            // Cache the pointer to the node containing the element
            SNode* pRemoveNode = pNode;

            if( pRemoveNode->m_LeftNode &&
                pRemoveNode->m_RightNode )
            {
                pNode = RemoveMaxNode( pRemoveNode->m_LeftNode );

                ASSERT( pNode );
                if( pNode )
                {
                    ASSERT( pNode->m_LeftNode == NULL );
                    pNode->m_LeftNode = pRemoveNode->m_LeftNode;

                    ASSERT( ( pNode->m_LeftNode )
                        ? !( pNode->m_Key < pNode->m_LeftNode->m_Key )
                        : 1 );

                    ASSERT( pNode->m_RightNode == NULL );
                    pNode->m_RightNode = pRemoveNode->m_RightNode;

                    ASSERT( ( pNode->m_RightNode )
                        ? ( pNode->m_Key < pNode->m_RightNode->m_Key )
                        : 1 );
                }

                pRemoveNode->m_LeftNode = NULL;
                pRemoveNode->m_RightNode = NULL;
            }
            else if( pRemoveNode->m_LeftNode )
            {
                pNode = pRemoveNode->m_LeftNode;
                pRemoveNode->m_LeftNode = NULL;
            }
            else if( pRemoveNode->m_RightNode )
            {
                pNode = pRemoveNode->m_RightNode;
                pRemoveNode->m_RightNode = NULL;
            }
            else
            {
                pNode = NULL;
            }

            // Return the element
            element = pRemoveNode->m_Element;

            // Delete the node
            DeleteNode( pRemoveNode );
            *ppNode = pNode;

#ifdef _DEBUG
            ++m_RemoveCount;
#endif
            return true;
        }
        // Search left sub-tree
        else if( key < pNode->m_Key )
        {
            ppNode = &(pNode->m_LeftNode);
        }
        // Search right sub-tree
        else
        {
            ppNode = &(pNode->m_RightNode);
        }

        pNode = *ppNode;
    }

    return false;
}

/*****************************************************************************\

Function:
    CBinaryTree::RemoveMinElement

Description:
    Deletes the minimum element from a sub-tree

Input:
    SNode* &pTree - pointer to sub-tree to search

Output:
    KeyType &key - key of element
    ElementType &element - element found
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::RemoveMinElement( 
    SNode* &pTree, 
    KeyType &key,
    ElementType &element )
{
    SNode** ppNode = &pTree;
    SNode* pNode = *ppNode;

    while( pNode )
    {
        if( pNode->m_LeftNode )
        {
            ppNode = &(pNode->m_LeftNode);
            pNode = *ppNode;
        }
        else
        {
            // Cache the pointer to the node containing the element
            SNode* pRemoveNode = pNode;
            pNode = pRemoveNode->m_RightNode;
            pRemoveNode->m_RightNode = NULL;

            // Return the key and element
            key = pRemoveNode->m_Key;
            element = pRemoveNode->m_Element;

            // Delete the node
            DeleteNode( pRemoveNode );
            *ppNode = pNode;

#ifdef _DEBUG
            ++m_RemoveCount;
#endif
            return true;
        }
    }

    return false;
}

/*****************************************************************************\

Function:
    CBinaryTree::RemoveMaxElement

Description:
    Deletes the maximum element from a sub-tree

Input:
    SNode* &pTree - pointer to sub-tree to search

Output:
    KeyType &key - key of element
    ElementType &element - element found
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::RemoveMaxElement( 
    SNode* &pTree, 
    KeyType &key,
    ElementType &element )
{
    SNode** ppNode = &pTree;
    SNode* pNode = *ppNode;

    while( pNode )
    {
        if( pNode->m_RightNode )
        {
            ppNode = &(pNode->m_RightNode);
            pNode = *ppNode;
        }
        else
        {
            // Cache the pointer to the node containing the element
            SNode* pRemoveNode = pNode;
            pNode = pRemoveNode->m_LeftNode;
            pRemoveNode->m_LeftNode = NULL;

            // Return the key and element
            key = pRemoveNode->m_Key;
            element = pRemoveNode->m_Element;

            // Delete the node
            DeleteNode( pRemoveNode );
            *ppNode = pNode;

#ifdef _DEBUG
            ++m_RemoveCount;
#endif
            return true;
        }
    }

    return false;
}

/*****************************************************************************\

Function:
    CBinaryTree::AddTree

Description:
    Adds a sub-tree

Input:
    SNode* &pTree - pointer to sub-tree to add to
    SNode* &pNewTree - pointer to sub-tree to add

Output:
    bool

\*****************************************************************************/
template<BinaryTreeTemplateList>
bool CBinaryTreeType::AddTree( 
    SNode* &pTree, 
    SNode* pNewTree )
{
    SNode** ppNode = &pTree;
    SNode* pNode = *ppNode;

    while( pNode )
    {
        // Two elements should not have the same key
        if( pNode->m_Key == pNewTree->m_Key )
        {
            ASSERT( pNode->m_Element == pNewTree->m_Element );
            return false;
        }
        // Add element to left sub-tree
        else if( pNewTree->m_Key < pNode->m_Key )
        {
            ppNode = &(pNode->m_LeftNode);
        }
        // Add element to right sub-tree
        else
        {
            ppNode = &(pNode->m_RightNode);
        }

        pNode = *ppNode;
    }

    *ppNode = pNewTree;
    return true;
}

/*****************************************************************************\

Function:
    CBinaryTree::DeleteTree

Description:
    Recursive function to delete a sub-tree

Input:
    SNode* &pTree - pointer to sub-tree to delete

Output:
    void

\*****************************************************************************/
template<BinaryTreeTemplateList>
void CBinaryTreeType::DeleteTree( SNode* &pTree )
{
    while( pTree )
    {
        SNode* pNode = RemoveMinNode( pTree );
        DeleteNode( pNode );
    }
}

/*****************************************************************************\

Function:
    CBinaryTree::DebugPrintTree

Description:
    Recursive function to debug a sub-tree

Input:
    const SNode* tree - pointer to sub-tree to debug
    const DWORD depth - current node depth

Output:
    void

\*****************************************************************************/
template<BinaryTreeTemplateList>
void CBinaryTreeType::DebugPrintTree(
    const SNode* pTree,
    const DWORD depth ) const
{
#ifdef _DEBUG
    if( pTree )
    {
        // Debug right sub-tree
        if( pTree->m_RightNode )
        {
            DebugPrintTree( pTree->m_RightNode, depth + 1 );
        }

        iSTD::CString<CAllocatorType> strIndent;
        strIndent = "\t";

        DWORD i = 0;
        for( i = 0; i < depth; ++i )
        {
            strIndent += " ";
        }

        iSTD::CString<CAllocatorType> strKeyData;
        strKeyData = "{ ";

        //DWORD* pKeyData = (DWORD*)&(pTree->m_Key);
        //for( i = 0; i < sizeof(KeyType) / sizeof(DWORD); ++i )
        //{
        //    strKeyData.AppendFormatted( "0x%08x ", pKeyData[i] );
        //}

        strKeyData += "}";

        iSTD::CString<CAllocatorType> strElementData;
        strElementData = "{ ";

        //DWORD* pElementData = (DWORD*)&(pTree->m_Element);
        //for( i = 0; i < sizeof(ElementType) / sizeof(DWORD); ++i )
        //{
        //    strElementData.AppendFormatted( "0x%08x ", pElementData[i] );
        //}

        strElementData += "}";

        DPF( GFXDBG_STDLIB, "%sBinaryTreeNode[%u].Key = %s\n",
            (const char*)strIndent,
            depth,
            (const char*)strKeyData );

        DPF( GFXDBG_STDLIB, "%sBinaryTreeNode[%u].Element = %s\n",
            (const char*)strIndent,
            depth,
            (const char*)strElementData );

        DPF( GFXDBG_STDLIB, "%sBinaryTreeNode[%u].GetCount = %d\n",
            (const char*)strIndent,
            depth,
            pTree->m_GetCount );

        // Debug left sub-tree
        if( pTree->m_LeftNode )
        {
            DebugPrintTree( pTree->m_LeftNode, depth + 1 );
        }
    }
#endif
}

/*****************************************************************************\

Function:
    CBinaryTree::CreateNode

Description:
    Creates a node

Input:
    const KeyType &key - key which determines sorting order of element
    const ElementType &element - element in binary tree

Output:
    SNode* - node created

\*****************************************************************************/
template<BinaryTreeTemplateList>
typename CBinaryTreeType::SNode* CBinaryTreeType::CreateNode(
    const KeyType &key,
    const ElementType &element )
{
    // Create node to add element here
    SNode* pNode = (SNode*)CAllocatorType::Allocate( sizeof(SNode) );
    ASSERT( pNode );
    if( pNode )
    {
        pNode->m_Key = key;
        pNode->m_Element = element;
        pNode->m_RightNode = NULL;
        pNode->m_LeftNode = NULL;

#ifdef _DEBUG
        pNode->m_GetCount = 0;
#endif
        ++m_Count;
    }

    return pNode;
}

/*****************************************************************************\

Function:
    CBinaryTree::RemoveMinNode

Description:
    Removes the min node from the tree

Input:
    SNode* &pTree - tree to remove min node from

Output:
    SNode* - node removed

\*****************************************************************************/
template<BinaryTreeTemplateList>
typename CBinaryTreeType::SNode* CBinaryTreeType::RemoveMinNode(
    SNode* &pTree )
{
    SNode** ppNode = &pTree;
    SNode* pNode = *ppNode;

    while( pNode )
    {
        if( pNode->m_LeftNode )
        {
            ppNode = &(pNode->m_LeftNode);
            pNode = *ppNode;
        }
        else
        {
            // Cache the pointer to the node containing the element
            SNode* pRemoveNode = pNode;
            pNode = pRemoveNode->m_RightNode;
            pRemoveNode->m_RightNode = NULL;
            *ppNode = pNode;
            return pRemoveNode;
        }
    }

    return NULL;
}

/*****************************************************************************\

Function:
    CBinaryTree::RemoveMaxNode

Description:
    Removes the max node from the tree

Input:
    SNode* &pTree - tree to remove max node from

Output:
    SNode* - node removed

\*****************************************************************************/
template<BinaryTreeTemplateList>
typename CBinaryTreeType::SNode* CBinaryTreeType::RemoveMaxNode(
    SNode* &pTree )
{
    SNode** ppNode = &pTree;
    SNode* pNode = *ppNode;

    while( pNode )
    {
        if( pNode->m_RightNode )
        {
            ppNode = &(pNode->m_RightNode);
            pNode = *ppNode;
        }
        else
        {
            // Cache the pointer to the node containing the element
            SNode* pRemoveNode = pNode;
            pNode = pRemoveNode->m_LeftNode;
            pRemoveNode->m_LeftNode = NULL;
            *ppNode = pNode;
            return pRemoveNode;
        }
    }

    return NULL;
}

/*****************************************************************************\

Function:
    CBinaryTree::DeleteNode

Description:
    Deletes a node

Input:
    SNode* node - node to delete

Output:
    void

\*****************************************************************************/
template<BinaryTreeTemplateList>
void CBinaryTreeType::DeleteNode( SNode* &pNode )
{
    CAllocatorType::Deallocate( pNode );
    --m_Count;
}

} // iSTD
