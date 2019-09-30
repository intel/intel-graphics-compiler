/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#pragma once
#include "Debug.h"

#ifdef __cplusplus

namespace iSTD
{


/*****************************************************************************\

Function:
    SafeDelete

Description:
    Safe "delete ptr;"

Input:
    Type &ptr - pointer to memory to delete

Output:
    Type &ptr

\*****************************************************************************/
template <class Type>
inline void SafeDelete( Type &ptr )
{
    if( ptr )
    {
#if defined( _DEBUG ) && !defined( NO_EXCEPTION_HANDLING )
#if defined( __GNUC__ )
        try
        {
            delete ptr;
        }
        catch (...)
        {
            ASSERT(0);
        }
#else  // defined( __GNUC__ )
        __try
        {
            delete ptr;
        }
        __except (1)
        {
            ASSERT(0);
        }
#endif // defined( __GNUC__ )
#else  // defined( _DEBUG ) && !defined( NO_EXCEPTION_HANDLING )
        delete ptr;
#endif // defined( _DEBUG ) && !defined( NO_EXCEPTION_HANDLING )
        ptr = 0;
    }
};

/*****************************************************************************\

Function:
    SafeDeleteArray

Description:
    Safe "delete[] ptr;"

Input:
    Type &ptr - pointer to memory to delete

Output:
    Type &ptr

\*****************************************************************************/
template <class Type>
inline void SafeDeleteArray( Type &ptr )
{
    if( ptr )
    {
#if defined( _DEBUG ) && !defined( NO_EXCEPTION_HANDLING )
#if defined( __GNUC__ )
        try
        {
            delete[] ptr;
        }
        catch (int e)
        {
            ASSERT(0);
        }
#else  // defined( __GNUC__ )
        __try
        {
            delete[] ptr;
        }
        __except (1)
        {
            ASSERT(0);
        }
#endif // defined( __GNUC__ )
#else  // defined( _DEBUG ) && !defined( NO_EXCEPTION_HANDLING )
        delete[] ptr;
#endif // defined( _DEBUG ) && !defined( NO_EXCEPTION_HANDLING )
        ptr = 0;
    }
};

} // iSTD

#endif // __cplusplus
