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

#include "AdaptorOCL/OCL/LoadBuffer.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/MemoryBuffer.h"
#include "common/LLVMWarningsPop.hpp"

#include <cstdlib>

using namespace llvm;
#ifdef LLVM_ON_UNIX
#include <dlfcn.h>
#include <stdio.h>

MemoryBuffer *llvm::LoadBufferFromResource(const char *pResName,
        const char *pResType)
{
    // Symbol Name is <type>_<number>
    char name[73];      // 64 + 9 for prefix
    char size_name[78]; // 64 + 9 for prefix + 5 for suffix
    void *module;
    void *symbol;
    uint32_t size;

    snprintf(name,      sizeof(name),      "_igc_bif_%s_%s",      pResType, &pResName[1]);
    snprintf(size_name, sizeof(size_name), "_igc_bif_%s_%s_size", pResType, &pResName[1]);

    module = RTLD_DEFAULT;

    symbol = dlsym(module, size_name);
    if (!symbol)
    {
        return NULL;
    }
    size = *(uint32_t *)symbol;

    symbol = dlsym(module, name);
    if (!symbol)
    {
        return NULL;
    }

    // Create a copy of the buffer for the caller. This copy is managed
    return MemoryBuffer::getMemBufferCopy(StringRef((char *)symbol, size)).release();
}

#endif

#ifdef WIN32
#include <Windows.h>
// Windows.h defines MemoryFence as _mm_mfence, but this conflicts with llvm::sys::MemoryFence
#undef MemoryFence

HRSRC FindResourceCompat(HMODULE hMod, const char * pResName, const char * pResType)
{
    std::wstring resNameW;
    std::wstring resTypeW;
    if (pResName != nullptr)
    {
        resNameW.assign(pResName, pResName + strlen(pResName));
    }
    if (pResType != nullptr)
    {
        resTypeW.assign(pResType, pResType + strlen(pResType));
    }

    return FindResourceExW(hMod,
                           pResType ? resTypeW.c_str() : nullptr,
                           pResName ? resNameW.c_str() : nullptr,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
}

MemoryBuffer *llvm::LoadBufferFromResource(const char *pResName,
  const char *pResType)
{
    HMODULE hMod = NULL;

    // Get the handle to the current module
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)llvm::LoadBufferFromResource,
        &hMod);
    if (hMod == NULL)
    {
        return NULL;
    }

    // Locate the resource
    HRSRC hRes = FindResourceCompat(hMod, pResName, pResType);

    if (hRes == NULL)
    {
        return NULL;
    }

    // Load the resource
    HGLOBAL hBytes = LoadResource(hMod, hRes);
    if (hBytes == NULL)
    {
        return NULL;
    }

    // Get the base address to the resource. This call doesn't really lock it
    const char *pData = (char *)LockResource(hBytes);
    if (pData == NULL)
    {
        return NULL;
    }

    // Get the buffer size
    size_t dResSize = SizeofResource(hMod, hRes);
    if (dResSize == 0)
    {
        return NULL;
    }

    // this memory never needs to be freeded since it is not dynamically allocated
    return MemoryBuffer::getMemBuffer(StringRef(pData, dResSize), "", false).release();
}
#endif // LLVM_ON_WIN32

MemoryBuffer* llvm::LoadBufferFromFile( const std::string &FileName )
{
    std::string FullFileName(FileName);

    // If buffer is not on the current working directory try to load it from
    // a predetermined path
    char *pEnv = getenv("INTEL_OPENCL_GPU_DIR");
    FullFileName.insert(0, "Inc\\");
    if (pEnv) {
        FullFileName.insert(0, "\\");
        FullFileName.insert(0, pEnv);
    }

    ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
        MemoryBuffer::getFileOrSTDIN(FullFileName);

    return FileOrErr.get().release();
}

