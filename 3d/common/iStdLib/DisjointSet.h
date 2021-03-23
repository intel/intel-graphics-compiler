/*========================== begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "Object.h"
#include "Queue.h"

namespace iSTD
{

/*****************************************************************************\
Struct: IsDisjointSetTypeSupported
\*****************************************************************************/
template<typename T>
struct IsDisjointSetTypeSupported                   { enum { value = false }; };

template<>
struct IsDisjointSetTypeSupported<bool>             { enum { value = true }; };

template<>
struct IsDisjointSetTypeSupported<char>             { enum { value = true }; };

template<>
struct IsDisjointSetTypeSupported<unsigned char>    { enum { value = true }; };

template<>
struct IsDisjointSetTypeSupported<int>              { enum { value = true }; };

template<>
struct IsDisjointSetTypeSupported<unsigned int>     { enum { value = true }; };

#ifndef __LP64__ // u/long on linux64 platform is 64-bit type and collides with U/INT64
template<>
struct IsDisjointSetTypeSupported<long>             { enum { value = true }; };

template<>
struct IsDisjointSetTypeSupported<unsigned long>    { enum { value = true }; };
#endif

template<>
struct IsDisjointSetTypeSupported<INT64>            { enum { value = true }; };

template<>
struct IsDisjointSetTypeSupported<UINT64>           { enum { value = true }; };

template<typename T>
struct IsDisjointSetTypeSupported<T*>               { enum { value = true }; };

/*****************************************************************************\
Template Parameters
\*****************************************************************************/
#define DisjointSetTemplateList class Type, class CAllocatorType
#define CDisjointSetType        CDisjointSet<Type,CAllocatorType>

/*****************************************************************************\

Class:
    CDisjointSet

Description:
    Implements a union find disjoint set

\*****************************************************************************/
template<DisjointSetTemplateList>
class CDisjointSet : public CObject<CAllocatorType>
{
public:

    CDisjointSet( void );
    virtual ~CDisjointSet( void );

    void    Union( CDisjointSetType& ds );

    const Type& Find( void );
    void    Set( const Type& item );

    C_ASSERT( IsDisjointSetTypeSupported<Type>::value == true );

protected:

    CDisjointSetType*   RootRecursive( CDisjointSetType* node );
    CDisjointSetType*   RootNonRecursive( CDisjointSetType* node );

    CDisjointSetType*   m_pParent;

    Type    m_Item;
};

/*****************************************************************************\

Function:
    CDisjointSet Constructor

Description:
    Initializes the disjoint set

Input:

Output:
    none

\*****************************************************************************/
template<DisjointSetTemplateList>
CDisjointSetType::CDisjointSet( void )
    : CObject<CAllocatorType>()
{
    m_pParent = this;
}

/*****************************************************************************\

Function:
    CDisjointSet Destructor

Description:
    Frees all internal dynamic memory

Input:
    none

Output:
    none

\*****************************************************************************/
template<DisjointSetTemplateList>
CDisjointSetType::~CDisjointSet( void )
{
    // Nothing!
}

/*****************************************************************************\

Function:
    CDisjointSet::Union

Description:
    Unions this disjoint set with the passed in disjoint set

Input:
    CDisjointSet& ds - other disjoint set

Output:
    void

\*****************************************************************************/
template<DisjointSetTemplateList>
void CDisjointSetType::Union( CDisjointSetType& ds )
{
    CDisjointSetType*   oldRoot = RootNonRecursive( this );
    CDisjointSetType*   newRoot = RootNonRecursive( &ds );

    if( oldRoot != newRoot )
    {
        oldRoot->m_pParent = newRoot;
    }
}

/*****************************************************************************\

Function:
    CDisjointSet::Find

Description:
    Finds the item in this disjoint set

Input:
    void

Output:
    const Type& - root of this disjoint set

\*****************************************************************************/
template<DisjointSetTemplateList>
const Type& CDisjointSetType::Find( void )
{
    CDisjointSetType*   root = RootNonRecursive( this );

    return root->m_Item;
}

/*****************************************************************************\

Function:
    CDisjointSet::Set

Description:
    Sets the item stored in this disjoint set

Input:
    const Type& - item stored in this disjoint set

Output:
    void

\*****************************************************************************/
template<DisjointSetTemplateList>
void CDisjointSetType::Set( const Type& item )
{
    CDisjointSetType*   root = RootNonRecursive( this );

    root->m_Item = item;
}

/*****************************************************************************\

Function:
    CDisjointSet::RootRecursive

Description:
    Returns the root node of this disjoint set.  Also performs path
    compression using recursion.

Input:
    void

Output:
    CDisjointSetType* root node

\*****************************************************************************/
template<DisjointSetTemplateList>
CDisjointSetType* CDisjointSetType::RootRecursive(
    CDisjointSetType* node )
{
    if( node->m_pParent != node )
    {
        CDisjointSetType*   root = RootRecursive( node->m_pParent );
        node->m_pParent = root;
    }

    return node;
}

/*****************************************************************************\

Function:
    CDisjointSet::RootNonRecursive

Description:
    Returns the root node of this disjoint set.  Also performs path
    compression without using recursion.  The actual path compression isn't
    quite as good as the recursive version but it's pretty good and the
    speed gains from not using recursion might be more important.

Input:
    void

Output:
    CDisjointSetType* root node

\*****************************************************************************/
template<DisjointSetTemplateList>
CDisjointSetType* CDisjointSetType::RootNonRecursive(
    CDisjointSetType* node )
{
    while( node->m_pParent != node )
    {
        node->m_pParent = node->m_pParent->m_pParent;
        node = node->m_pParent;
    }

    return node;
}

} // iSTD
