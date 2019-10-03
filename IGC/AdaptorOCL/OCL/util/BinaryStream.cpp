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

    while( padding-- )
    {
        // Always pad with 0x0 to make external tools that parse
        // OpenCL program binaries easier to maintain
        retValue &= Write( (char)0x0 );
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
