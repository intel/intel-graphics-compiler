/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdlib.h>
#include <stdint.h>

namespace IGC
{
inline void * aligned_malloc(size_t nBytes, size_t alignBytes)
{
    void * kernel = nullptr;
#if   defined(_WIN32)
    kernel = _aligned_malloc(nBytes, alignBytes);
#elif defined(ANDROID)
    kernel = memalign(alignBytes, nBytes);
#else // !defined(_WIN32) && !defined(ANDROID)
    if (posix_memalign(&kernel, alignBytes, nBytes))
    {
        kernel = nullptr;
    }
#endif
    return kernel;
}

inline void aligned_free(void *ptr)
{
#if   defined(_WIN32)
    _aligned_free(ptr);
#else // !defined(_WIN32)
    free(ptr);
#endif
}

}

#endif //ALLOCATOR_H
