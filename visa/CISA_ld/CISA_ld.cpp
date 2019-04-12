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

#include "CISA_ld.h"
#include "CISALinker.h"
#include "../Mem_Manager.h"
#include <string.h>
// *** Macros ***

#define ASSERT(assertion, ...)                                          \
    if (!(assertion)) {                                                 \
        fprintf(stderr, "Error: " __VA_ARGS__);                         \
        fprintf(stderr, "!\n");                                         \
        return 1;                                                       \
    }

// *** Functions ***

extern "C" CISA_LD_DECLSPEC int
linkCisaMemObjs(
    const char *kernelName,
    int numCisaObjs, const void *cisaBufs[], unsigned cisaBufSizes[],
    const void* *cisaLinkedBuf, unsigned *cisaLinkedBufSize,
    int numOptions, const char *options[],
    ExternalHeapAllocator *customAllocator)
{
    vISA::Mem_Manager mem(4096);
    CISALinker::ExternalHeapAllocator *heapAllocator =
        (CISALinker::ExternalHeapAllocator *) customAllocator;
    CISALinker::CisaObj *cisaObjs =
        (CISALinker::CisaObj *) mem.alloc(
            sizeof(CISALinker::CisaObj) * numCisaObjs);

    for (int i = 0; i < numCisaObjs; i++) {
        cisaObjs[i].buf = cisaBufs[i];
        cisaObjs[i].size = cisaBufSizes[i];
    }

    CISALinker cisaLinker(numOptions, options, mem, heapAllocator);
    CISALinker::CisaObj cisaLinkedObj;
    int status =
        cisaLinker.LinkCisaMemObjs(
            kernelName, numCisaObjs, cisaObjs, cisaLinkedObj);

    if (status == 0) {
        *cisaLinkedBuf = cisaLinkedObj.buf;
        *cisaLinkedBufSize = cisaLinkedObj.size;
    }
    else {
        *cisaLinkedBuf = NULL;
        *cisaLinkedBufSize = 0;
    }

    return status;
}

extern "C" CISA_LD_DECLSPEC int
linkCisaFileObjs(
    const char *kernelName,
    int numCisaObjs, const char *cisaObjs[],
    const char *cisaLinkedObj,
    int numOptions, const char *options[])
{
    vISA::Mem_Manager mem(4096);
    CISALinker cisaLinker(numOptions, options, mem);
    int status =
        cisaLinker.LinkCisaFileObjs(
            kernelName, numCisaObjs, cisaObjs, cisaLinkedObj);

    return status;
}


#ifdef STANDALONE_MODE
int main(int argc, char *argv[])
{
    const char *cisaExt = ".isa";

    if(argc < 4) {
        fprintf(
            stderr,
            "Usage: cisa_ld cisa_obj_1.isa ... cisa_obj_n.isa cisa_linked.isa "
            "kernel_name.\n");
        return 1;
    }

    const char *knlName = argv[argc - 1];
    ASSERT(
        strstr(knlName, cisaExt) == NULL,
        "The last argument must be the kernel name");
    const char *linkedCisaObjName = argv[argc - 2];
    const char *argExt = strstr(linkedCisaObjName, cisaExt);
    ASSERT(
        argExt != NULL && argExt[4] == '\0',
        "The linked CISA object file must have .isa extension");
    const char* *cisaObjs = (const char**) argv + 1;
    int numCisaObjs = argc - 3;

    if (linkCisaFileObjs(
            knlName,
            numCisaObjs, cisaObjs,
            linkedCisaObjName,
            0, NULL)) {
        return 1;
    }

    return 0;
}
#endif    // STANDALONE_MODE
