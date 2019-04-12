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

#ifndef __CISA_LD_H__
#define __CISA_LD_H__

#ifdef STANDALONE_MODE
    #define CISA_LD_DECLSPEC
#else
    #ifdef DLL_MODE
    #define CISA_LD_DECLSPEC __declspec(dllexport)
    #else
    #define CISA_LD_DECLSPEC __declspec(dllimport)
    #endif
#endif

typedef void* ExternalHeapAllocator(unsigned size);

extern "C" CISA_LD_DECLSPEC int
linkCisaMemObjs(
    const char *kernelName,
    int numCisaObjs, const void *cisaBufs[], unsigned cisaBufSizes[],
    const void* *cisaLinkedBuf, unsigned *cisaLinkedBufSize,
    int numOptions, const char *options[],
    ExternalHeapAllocator *customAllocator);

extern "C" CISA_LD_DECLSPEC int
linkCisaFileObjs(
    const char *kernelName,
    int numCisaObjs, const char *cisaObjs[],
    const char *cisaLinkedObj,
    int numOptions, const char *options[]);

#endif // __CISA_LD_H__
