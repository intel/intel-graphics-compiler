/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

typedef void *ExternalHeapAllocator(unsigned size);

extern "C" CISA_LD_DECLSPEC int
linkCisaMemObjs(const char *kernelName, int numCisaObjs, const void *cisaBufs[],
                unsigned cisaBufSizes[], const void **cisaLinkedBuf,
                unsigned *cisaLinkedBufSize, int numOptions,
                const char *options[], ExternalHeapAllocator *customAllocator);

extern "C" CISA_LD_DECLSPEC int
linkCisaFileObjs(const char *kernelName, int numCisaObjs,
                 const char *cisaObjs[], const char *cisaLinkedObj,
                 int numOptions, const char *options[]);

#endif // __CISA_LD_H__
