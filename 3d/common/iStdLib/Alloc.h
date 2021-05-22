/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
