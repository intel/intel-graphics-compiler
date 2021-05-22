/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace iSTD
{

/*****************************************************************************\

Class:
    CPointer

Description:

\*****************************************************************************/
template<class Type>
class CPointer
{
public:

    CPointer( void );
    virtual ~CPointer( void );

    CPointer<Type>& operator= ( Type* rValue );
    CPointer<Type>& operator= ( CPointer<Type> rValue );

    Type& operator[] ( long index );
    Type& operator* ( void );
    Type* operator-> ( void );

protected:

    Type*   m_Pointer;
};

/*****************************************************************************\

Function:

Description:

Input:

Output:

\*****************************************************************************/
template<class Type>
CPointer<Type>::CPointer( void )
    : Object()
{
    m_Pointer = NULL;
}

/*****************************************************************************\

Function:

Description:

Input:

Output:

\*****************************************************************************/
template<class Type>
CPointer<Type>::~CPointer( void )
{
    m_Pointer = NULL;
}

/*****************************************************************************\

Function:

Description:

Input:

Output:

\*****************************************************************************/
template<class Type>
CPointer<Type>& CPointer<Type>::operator= ( Type* rValue )
{
    m_Pointer = rValue;
    return *this;
}

/*****************************************************************************\

Function:

Description:

Input:

Output:

\*****************************************************************************/
template<class Type>
CPointer<Type>& CPointer<Type>::operator= ( CPointer<Type> rValue )
{
    m_Pointer = rValue.m_Pointer;
    return *this;
}

/*****************************************************************************\

Function:

Description:

Input:

Output:

\*****************************************************************************/
template<class Type>
Type& CPointer<Type>::operator[] ( long index )
{
    // TODO: exception handling
    // TODO: bounds checking
    return m_Pointer[ index ];
}

/*****************************************************************************\

Function:

Description:

Input:

Output:

\*****************************************************************************/
template<class Type>
Type& CPointer<Type>::operator* ( void )
{
    // TODO: exception handling
    return *m_Pointer;
}

/*****************************************************************************\

Function:

Description:

Input:

Output:

\*****************************************************************************/
template<class Type>
Type* CObjectPointer<Type>::operator-> ( void )
{
    return m_Pointer;
}

} // iSTD
