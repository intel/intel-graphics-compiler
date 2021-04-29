/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BinaryStream.h"

namespace Util
{

BinaryStream::BinaryStream() : m_membuf( std::ios::in | std::ios::out | std::ios::binary )
{
    // Nothing!
}

BinaryStream::~BinaryStream()
{
    // Nothing!
}

bool BinaryStream::Write( const char* s, std::streamsize n )
{
    bool retValue = false;

    std::streamsize prevSize = Size();

    m_membuf.write( s, n );

    if( ( Size() - prevSize ) == n )
    {
        retValue = true;
    }

    if( m_membuf.fail() )
    {
        retValue = false;
    }

#if defined(_DEBUG)
    GetLinearPointer();
#endif

    return retValue;
}

bool BinaryStream::Write( const BinaryStream& in )
{
    bool retValue = false;

    std::streamsize prevSize = Size();

    m_membuf << in.m_membuf.str();

    if( ( Size() - prevSize ) == in.Size() )
    {
        retValue = true;
    }

    if( m_membuf.fail() )
    {
        retValue = false;
    }

#if defined(_DEBUG)
    GetLinearPointer();
#endif

    return retValue;
}


bool BinaryStream::WriteAt( const char* s, std::streamsize n, std::streamsize loc )
{
    bool retValue = true;

    // Give this function name it seems like this function should enlarge the stream if needed. Discuss.
    if( ( n + loc ) < Size() )
    {
        m_membuf.seekp( loc, std::ios_base::beg );
        Write( s, n );
        m_membuf.seekp( 0, std::ios_base::end );
    }
    else
    {
        retValue = false;
    }

    return retValue;
}

const char* BinaryStream::GetLinearPointer()
{
    m_LinearPointer = m_membuf.str();
    return m_LinearPointer.c_str();
}

bool BinaryStream::Align( std::streamsize alignment )
{
    bool retValue = true;

    std::streamsize currentSize = Size();

    std::streamsize modulo = currentSize % alignment;

    if( modulo )
    {
        std::streamsize offset =  alignment - modulo;

        retValue = AddPadding( offset );
    }

     retValue &= ( Size() % alignment ) == 0;

    return retValue;
}

bool BinaryStream::AddPadding( std::streamsize padding )
{
    bool retValue = true;
    if (padding > 0)
    {
        std::streamsize prevSize = Size();
        // Writes zeros to the width of "padding"
        m_membuf.width(padding);
        m_membuf.fill((char)0x0);
        m_membuf << '\0';

        if ((Size() - prevSize) != padding)
        {
            retValue = false;
        }
        else if (m_membuf.fail())
        {
            retValue = false;
        }
    }
    return retValue;
}

std::streamsize BinaryStream::Size() const
{
    // tellp is a non const function
    // TODO: Is there a better way to do this?
    return const_cast<BinaryStream*>(this)->Size();
}

std::streamsize BinaryStream::Size()
{
    return m_membuf.tellp();
}

}
