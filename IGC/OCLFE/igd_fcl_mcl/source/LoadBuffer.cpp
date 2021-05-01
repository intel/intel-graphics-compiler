/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../headers/LoadBuffer.h"
#include "llvm/Config/llvm-config.h"
#include <cstdlib>
#include <stdio.h>
#include <fstream>

using namespace llvm;

#ifdef LLVM_ON_UNIX
#include <dlfcn.h>
#include <inttypes.h>
#include <libgen.h>
#include <link.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

char* llvm::LoadCharBufferFromResource(unsigned int ResNumber,
    const char *pResType, long unsigned int& m_buffSize)
{

    // Symbol Name is <type>_<number>
    char name[73];      // 64 + 9 for prefix
    char size_name[78]; // 64 + 9 for prefix + 5 for suffix
    void *module;
    void *symbol;
    uint32_t size;

    snprintf(name,      sizeof(name),      "IDR_CTH_H_%s_%u",      pResType, ResNumber);
    snprintf(size_name, sizeof(size_name), "IDR_CTH_H_%s_%u_size", pResType, ResNumber);

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
    m_buffSize = size;

    return (char *)symbol;
}
#endif

#ifdef WIN32
#include <Windows.h>

char* llvm::LoadCharBufferFromResource(unsigned int ResNumber,
    const char *pResType, unsigned long& m_buffSize)
{
    HMODULE hMod = NULL;

    char ResName[5] = { '-' };
    _snprintf(ResName, sizeof(ResName), "#%d", ResNumber);

    // Get the handle to the current module
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)llvm::LoadCharBufferFromResource,
        &hMod);
    if (hMod != NULL) {
        // Locate the resource
        HRSRC hRes = FindResourceA(hMod, ResName, pResType);
        if (hRes != NULL) {
            // Load the resource
            HGLOBAL hBytes = LoadResource(hMod, hRes);
            if (hBytes != NULL) {
                // Get the base address to the resource. This call doesn't really lock it
                char *m_buff = (char *)LockResource(hBytes);
                if (m_buff != NULL) {
                    // Get the buffer size
                    m_buffSize = SizeofResource(hMod, hRes);
                    if (m_buffSize != 0) {
                        return m_buff;
                    }
                }
            }
        }
    }
    return NULL;
}

#endif // #ifdef LLVM_ON_WIN32


