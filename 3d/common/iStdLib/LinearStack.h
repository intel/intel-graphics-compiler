/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Array.h"

namespace iSTD
{

/*****************************************************************************\
Struct: IsLinearStackTypeSupported
\*****************************************************************************/
template<typename T> 
struct IsLinearStackTypeSupported                    { enum { value = false }; };

template<> 
struct IsLinearStackTypeSupported<bool>              { enum { value = true }; };

template<> 
struct IsLinearStackTypeSupported<char>              { enum { value = true }; };

template<> 
struct IsLinearStackTypeSupported<unsigned char>     { enum { value = true }; };

template<> 
struct IsLinearStackTypeSupported<int>               { enum { value = true }; };

template<> 
struct IsLinearStackTypeSupported<unsigned int>      { enum { value = true }; };

#ifndef __LP64__ // u/long on linux64 platform is 64-bit type and collides with U/INT64
template<> 
struct IsLinearStackTypeSupported<long>              { enum { value = true }; };

template<> 
struct IsLinearStackTypeSupported<unsigned long>     { enum { value = true }; };
#endif

template<> 
struct IsLinearStackTypeSupported<float>             { enum { value = true }; };

template<> 
struct IsLinearStackTypeSupported<INT64>             { enum { value = true }; };

template<> 
struct IsLinearStackTypeSupported<UINT64>            { enum { value = true }; };

template<typename T> 
struct IsLinearStackTypeSupported<T*>                { enum { value = true }; };

/*****************************************************************************\
Template Parameters
\*****************************************************************************/
#define LinearStackTemplateList   class Type, class CAllocatorType
#define CLinearStackType          CLinearStack<Type,CAllocatorType>

/*****************************************************************************\

Class:
    CLinearStack

Description:
    Implements an array-based stack

\*****************************************************************************/
template<LinearStackTemplateList>
class CLinearStack : public CObject<CAllocatorType>
{
public:

    CLinearStack( const DWORD size );
    virtual ~CLinearStack( void );

    bool    IsEmpty( void ) const;
    DWORD   GetCount( void ) const;

    bool    Push( const Type element );
    Type    Pop( void );
    Type    Top( void ) const;

    void    Reset( void );

    void    DebugPrint( void ) const;

protected:

    CDynamicArray<Type,CAllocatorType> m_ElementArray;
    DWORD   m_Count;
};

/*****************************************************************************\

Function:
    CLinearStack Constructor

Description:
    Initializes internal data

Input:
    const DWORD size - initial size of the stack

Output:
    none

\*****************************************************************************/
template<LinearStackTemplateList>
CLinearStackType::CLinearStack( const DWORD size )
    : CObject<CAllocatorType>(),
    m_ElementArray( size )
{
    m_Count = 0;
}

/*****************************************************************************\

Function:
    CLinearStack Destructor

Description:
    Deletes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinearStackTemplateList>
CLinearStackType::~CLinearStack( void )
{
}

/*****************************************************************************\

Function:
    CLinearStack::IsEmpty

Description:
    Returns if the stack is empty

Input:
    none

Output:
    bool - true or false

\*****************************************************************************/
template<LinearStackTemplateList>
bool CLinearStackType::IsEmpty( void ) const
{
    return m_Count == 0;
}

/*****************************************************************************\

Function:
    CLinearStack::GetCount

Description:
    Returns the number of elements in the stack

Input:
    none

Output:
    DWORD - number of elements

\*****************************************************************************/
template<LinearStackTemplateList>
DWORD CLinearStackType::GetCount( void ) const
{
    return m_Count;
}

/*****************************************************************************\

Function:
    CLinearStack::Push

Description:
    Pushes an element on the stack

Input:
    const Type element

Output:
    bool - success or fail

\*****************************************************************************/
template<LinearStackTemplateList>
bool CLinearStackType::Push( const Type element )
{
    // Add element to the end of list
    return m_ElementArray.SetElement( m_Count++, element );
}

/*****************************************************************************\

Function:
    CLinearStack::Pop

Description:
    Pops an element off the stack

Input:
    none

Output:
    Type - element

\*****************************************************************************/
template<LinearStackTemplateList>
Type CLinearStackType::Pop( void )
{
    Type element = (Type)0;

    if( IsEmpty() )
    {
        ASSERT(0);
    }
    else
    {
        // Get the last element on the list and
        // remove the last element
        element = m_ElementArray.GetElement( --m_Count );
    }

    return element;
}

/*****************************************************************************\

Function:
    CLinearStack::Top

Description:
    Returns the top element of the stack

Input:
    none

Output:
    Type - element

\*****************************************************************************/
template<LinearStackTemplateList>
Type CLinearStackType::Top( void ) const
{
    Type element = (Type)0;

    if( IsEmpty() )
    {
        ASSERT(0);
    }
    else
    {
        // Get the last element on the list
        element = m_ElementArray.GetElement( m_Count-1 );
    }

    return element;
}

/*****************************************************************************\

Function:
    CLinearStack::Reset

Description:
    Resets the stack

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinearStackTemplateList>
void CLinearStackType::Reset( void )
{
    m_Count = 0;
}

/*****************************************************************************\

Function:
    CLinearStack::DebugPrint

Description:
    Prints the stack to std output for debug only

Input:
    none

Output:
    none

\*****************************************************************************/
template<LinearStackTemplateList>
void CLinearStackType::DebugPrint( void ) const
{
#ifdef _DEBUG
    m_ElementArray.DebugPrint();
#endif
}

} // iSTD
