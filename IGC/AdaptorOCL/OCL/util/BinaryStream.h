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

#include <sstream>

namespace Util
{

class BinaryStream
{
public:
    BinaryStream();
    ~BinaryStream();

    bool Write( const char* s, std::streamsize n );

    bool Write( const BinaryStream& in );

    template< class T >
    bool Write( const T& in );

    bool WriteAt( const char* s, std::streamsize n, std::streamsize loc );

    template< class T >
    bool WriteAt( const T& in, std::streamsize loc ) { return WriteAt( (const char*)&in, sizeof(T), loc ); }

    bool Align( std::streamsize alignment );
    bool AddPadding( std::streamsize padding );

    const char* GetLinearPointer();

    std::streamsize Size() const;
    std::streamsize Size();

private:
    std::stringstream m_membuf;

    std::string m_LinearPointer;
};

template< class T >
bool BinaryStream::Write(const T& in )
{
    return Write( (const char*)&in, sizeof(T) );
}

}
