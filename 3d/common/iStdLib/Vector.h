/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "types.h"
#include "Debug.h"

namespace iSTD
{

/*****************************************************************************\
STRUCT: iSTD::SVector4
\*****************************************************************************/
template<class Type>
struct SVector4
{
    Type    X;
    Type    Y;
    Type    Z;
    Type    W;

    Type& operator[] ( const int i )
    {
        ASSERT( i >= 0 );
        ASSERT( i <= 3 );
        return( ((Type*)this)[i] );
    }

    const Type& operator[] ( const int i ) const
    {
        ASSERT( i >= 0 );
        ASSERT( i <= 3 );
        return( ((Type*)this)[i] );
    }

    bool operator == ( const SVector4& vector ) const
    {
        return ( ( X == vector.X ) &&
                 ( Y == vector.Y ) &&
                 ( Z == vector.Z ) &&
                 ( W == vector.W ) );
    }

    bool operator != ( const SVector4& vector ) const
    {
        return ( ( X != vector.X ) ||
                 ( Y != vector.Y ) ||
                 ( Z != vector.Z ) ||
                 ( W != vector.W ) );
    }
};

// Specialization of iSTD::SVector4 template for Type float :
template<>
struct SVector4<float>
{
    float    X;
    float    Y;
    float    Z;
    float    W;

    float& operator[] ( const int i )
    {
        ASSERT( i >= 0 );
        ASSERT( i <= 3 );
        return( ((float*)this)[i] );
    }

    const float& operator[] ( const int i ) const
    {
        ASSERT( i >= 0 );
        ASSERT( i <= 3 );
        return( ((float*)this)[i] );
    }

    bool operator == ( const SVector4<float>& vector ) const
    {
        return ( ( *(DWORD*)&X == *(DWORD*)&vector.X ) &&
                 ( *(DWORD*)&Y == *(DWORD*)&vector.Y ) &&
                 ( *(DWORD*)&Z == *(DWORD*)&vector.Z ) &&
                 ( *(DWORD*)&W == *(DWORD*)&vector.W ) );
    }

    bool operator != ( const SVector4<float>& vector ) const
    {
        return ( ( *(DWORD*)&X != *(DWORD*)&vector.X ) ||
                 ( *(DWORD*)&Y != *(DWORD*)&vector.Y ) ||
                 ( *(DWORD*)&Z != *(DWORD*)&vector.Z ) ||
                 ( *(DWORD*)&W != *(DWORD*)&vector.W ) );
    }
};

/*****************************************************************************\
STRUCT: SVector3
\*****************************************************************************/
template<class Type>
struct SVector3
{
    Type    X;
    Type    Y;
    Type    Z;

    Type& operator[] ( const int i )
    {
        ASSERT( i >= 0 );
        ASSERT( i <= 2 );
        return( ((Type*)this)[i] );
    }

    const Type& operator[] ( const int i ) const
    {
        ASSERT( i >= 0 );
        ASSERT( i <= 2 );
        return( ((Type*)this)[i] );
    }

    bool operator == ( const SVector3& vector ) const
    {
        return ( ( X == vector.X ) &&
            ( Y == vector.Y ) &&
            ( Z == vector.Z ) );
    }

    bool operator != ( const SVector3& vector ) const
    {
        return ( ( X != vector.X ) ||
            ( Y != vector.Y ) ||
            ( Z != vector.Z ) );
    }
};

// Specialization of SVector3 template for Type float :
template<>
struct SVector3<float>
{
    float    X;
    float    Y;
    float    Z;

    float& operator[] ( const int i )
    {
        ASSERT( i >= 0 );
        ASSERT( i <= 2 );
        return( ((float *)this)[i] );
    }

    const float& operator[] ( const int i ) const
    {
        ASSERT( i >= 0 );
        ASSERT( i <= 2 );
        return( ((float *)this)[i] );
    }

    bool operator == ( const SVector3<float>& vector ) const
    {
        return ( ( *(DWORD*)&X == *(DWORD*)&vector.X ) &&
                 ( *(DWORD*)&Y == *(DWORD*)&vector.Y ) &&
                 ( *(DWORD*)&Z == *(DWORD*)&vector.Z ) );
    }

    bool operator != ( const SVector3& vector ) const
    {
        return ( ( *(DWORD*)&X != *(DWORD*)&vector.X ) ||
                 ( *(DWORD*)&Y != *(DWORD*)&vector.Y ) ||
                 ( *(DWORD*)&Z != *(DWORD*)&vector.Z ) );
    }
};

} // iSTD
