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
