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
#ifndef __IGC_H
#define __IGC_H

#if defined(_WIN32)
    #define IGC_API_CALL_TYPE __stdcall
    #ifdef IGC_EXPORTS
        #define IGC_API_CALL __declspec(dllexport) IGC_API_CALL_TYPE
    #else
        #define IGC_API_CALL __declspec(dllimport) IGC_API_CALL_TYPE
    #endif
#else
    #define IGC_API_CALL_TYPE
    #define IGC_API_CALL __attribute__ ((visibility("default"))) IGC_API_CALL_TYPE
#endif

#if defined(_DEBUG) || defined(_INTERNAL)
# define IGC_DEBUG
#endif

// TODO: add all external declarations so that external projects only need
// to include this file only.

// Definition of integer Registry-key Values
// (may merge this into igc_flags.def's macro)
enum {
    // ForcePixelShaderSIMDMode
    FLAG_PS_SIMD_MODE_DEFAULT = 0,                 // default is SIMD8 compilation + heuristics to determine if we want to compile SIMD32/SIMD16
    FLAG_PS_SIMD_MODE_FORCE_SIMD8 = 1,             // Force SIMD8 compilation
    FLAG_PS_SIMD_MODE_FORCE_SIMD16 = 2,            // Force SIMD16 compilation
    FLAG_PS_SIMD_MODE_FORCE_SIMD32 = 4,            // Force SIMD32 compilation
};

#endif // __IGC_H

