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

#ifndef JITTERDATASTRUCT_
#define JITTERDATASTRUCT_

#include <stdint.h>

typedef struct {
    int id;
    unsigned staticCycle;
    unsigned sendStallCycle;
    unsigned char loopNestLevel;
} VISA_BB_INFO;

typedef struct {
    // Common part
    bool isSpill;
    int numGRFUsed;
    int numAsmCount;

    // spillMemUsed is the scratch size in byte of entire vISA stack for this function/kernel
    // It contains spill size and caller/callee save size.
    unsigned int spillMemUsed = 0;

    // Debug info is callee allocated
    // and populated only if
    // switch is passed to JIT to emit
    // debug info.
    void* genDebugInfo;
    unsigned int genDebugInfoSize;

    // Number of flag spill and fill.
    unsigned numFlagSpillStore;
    unsigned numFlagSpillLoad;

    // whether kernel uses a barrier
    bool usesBarrier;

    unsigned BBNum;
    VISA_BB_INFO* BBInfo;

    // number of spill/fill, weighted by loop
    unsigned int numGRFSpillFill;

    void* freeGRFInfo;
    unsigned int freeGRFInfoSize;
    unsigned char numBytesScratchGtpin;

    uint32_t offsetToSkipPerThreadDataLoad = 0;

    // When two entries prolog is added for setting FFID
    // for compute (GP or GP1), skip this offset to set FFID_GP1.
    // Will set FFID_GP if not skip
    uint32_t offsetToSkipSetFFIDGP = 0;

    bool hasStackcalls = false;
} FINALIZER_INFO;

#endif // JITTERDATASTRUCT_
