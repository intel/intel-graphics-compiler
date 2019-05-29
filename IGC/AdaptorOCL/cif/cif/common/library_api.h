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

#if defined(_WIN32)
#define CREATE_CIF_MAIN_CALLING_CONV __cdecl
#if defined(CIF_EXPORT)
#define CREATE_CIF_MAIN_API __declspec(dllexport)
#else
#define CREATE_CIF_MAIN_API __declspec(dllimport)
#endif
#else
#define CREATE_CIF_MAIN_CALLING_CONV
#define CREATE_CIF_MAIN_API __attribute__((visibility("default")))
#endif

#define EXPORTED_FUNC_NAME CIFCreateMain

namespace CIF {
struct CIFMain;
using CreateCIFMainFunc_t = CIF::CIFMain *(CREATE_CIF_MAIN_CALLING_CONV *)();
#define STR2(X) #X
#define STR(X) STR2(X)
static const char * const CreateCIFMainFuncName = STR(EXPORTED_FUNC_NAME);
#undef STR
#undef STR2
}

#ifdef CIF_EXPORT
CIF::CIFMain *CreateCIFMainImpl();
#endif

extern "C" {
// entry point to shared object (dynamic library)
// the only function that gets exported explicitly
#if defined CIF_EXPORT
CREATE_CIF_MAIN_API void *CREATE_CIF_MAIN_CALLING_CONV EXPORTED_FUNC_NAME() {
  return CreateCIFMainImpl();
}
#endif
}

#undef CIF_MAIN_EXPORT_FUNC_NAME

#undef CREATE_CIF_MAIN_API
#undef CREATE_CIF_MAIN_CALLING_CONV
