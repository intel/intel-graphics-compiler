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

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#pragma once
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
    posix_memalign(&kernel, alignBytes, nBytes);
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
