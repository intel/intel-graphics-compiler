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

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/LLVMContext.h>
#include "common/LLVMWarningsPop.hpp"

/*
 * Following definitions are for GlobalData.h only
 * So we will undef them right after header include
 */

#ifndef VER_H
#define VER_H
#endif

/*typedef void *HANDLE;
typedef unsigned char BOOLEAN;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef signed long long INT64;
typedef unsigned long long UINT64;
typedef int INT; */

#if defined(WIN32)
#define DWORD unsigned long
#define INT64 signed long long
#define UINT64 unsigned long long
#define ULONG unsigned long
#define BOOLEAN unsigned char
#define INT int
#define UINT unsigned int
#else
#define DWORD uint32_t
#define INT64 int64_t
#define UINT64 uint64_t
#define ULONG uint32_t
#define BOOLEAN uint8_t
#define INT int32_t
#define UINT uint32_t
#endif

#define HANDLE void*

#include "GlobalData.h"

#ifdef VER_H
#undef VER_H
#endif

#undef DWORD
#undef INT64
#undef UINT64
#undef ULONG
#undef BOOLEAN
#undef INT
#undef UINT
#undef HANDLE

class IGILContext : public llvm::LLVMContext
{
public:
    IGILContext(const SGlobalData *globalData) : m_GlobalData(globalData) {}
    const SGlobalData* GetGlobalData() const { return m_GlobalData; }
private:
    const SGlobalData *m_GlobalData;
};

