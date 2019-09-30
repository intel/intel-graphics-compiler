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
