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

#include <cstdarg>
#include <cstdio>
#include <Windows.h>
#include <string>

namespace iOpenCL
{

/*****************************************************************************\
Function: DebugMessage
\*****************************************************************************/
void __cdecl DebugMessage( DWORD ulDebugLevel, const char* str, ... )
{
    //if( str && ( ( g_DebugControl.MsgLevel & ulDebugLevel ) != GFXDBG_OFF ) )
    {
        va_list args;
        va_start( args, str );

        const size_t length = _vscprintf( str, args );
        char* temp = new char[length + 1];

        if( temp )
        {
            vsprintf_s( temp, length+1, str, args );
            OutputDebugStringA("INTC CBE: ");
            OutputDebugStringA(temp);
            delete[] temp;
        }

        va_end( args );
    }
}

void __cdecl DebugMessageStr(std::string& output, DWORD ulDebugLevel, const char* str, ...)
{
    //if( str && ( ( g_DebugControl.MsgLevel & ulDebugLevel ) != GFXDBG_OFF ) )
    {
        va_list args;
        va_start(args, str);

        const size_t length = _vscprintf(str, args);
        char* temp = new char[length + 1];

        if (temp)
        {
            vsprintf_s(temp, length + 1, str, args);
#ifdef _DEBUG
            //This prints the output string to the console. We don't want that in release internal mode
            OutputDebugStringA("INTC CBE: ");
            OutputDebugStringA(temp);
#endif
            output += temp;
            delete[] temp;
        }

        va_end(args);
    }
}

}
