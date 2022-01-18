/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#if defined(_WIN32)
#include <Windows.h>
#endif
#include <string>
#include "IGC/common/igc_regkeys.hpp"
#include "sp_debug.h"

namespace iOpenCL
{

#if __ARM_ARCH
void DebugMessageStr(std::string& output, unsigned int ulDebugLevel, const char* fmt, ...)
#else
void __cdecl DebugMessageStr(std::string& output, unsigned int ulDebugLevel, const char* fmt, ...)
#endif
{
    if(IGC_IS_FLAG_ENABLED(ShaderDumpEnable) && IGC_IS_FLAG_ENABLED(EnableCosDump))
    //if( str && ( ( g_DebugControl.MsgLevel & ulDebugLevel ) != GFXDBG_OFF ) )
    {
        va_list args;
        va_start(args, fmt);

#if defined(ICBE_LHDM) || defined(_WIN32)
        if (IGC_IS_FLAG_ENABLED(DumpPatchTokens))
        {
          const size_t length = _vscprintf(fmt, args);
          char* temp = new char[length + 1];

          if (temp)
          {
            vsprintf_s(temp, length + 1, fmt, args);
            //This prints the output string to the console. We don't want that in release internal mode
            OutputDebugStringA("INTC CBE: ");
            OutputDebugStringA(temp);
            output += temp;
            delete[] temp;
          }
        }
#else
        if (IGC_IS_FLAG_ENABLED(DumpPatchTokens))
        {
            va_list argcopy;
            va_copy(argcopy, args);

            const size_t length = vsnprintf(NULL, 0, fmt, argcopy);

            char* temp = new char[length + 1];

            if (temp)
            {
                vsnprintf(temp, length + 1, fmt, args);
                output += temp;
                delete[] temp;
            }
        }
#endif

        va_end(args);
    }
}

}
