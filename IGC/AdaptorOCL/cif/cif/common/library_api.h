/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
