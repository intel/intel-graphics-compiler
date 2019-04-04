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
#include "VISADefines.h"
#include "JitterDataStruct.h"

/**
    API between CMRT and Jitter:
    JITCompile
    JITCompileWithRelocation
    freeBlock
    getJITVersion
*/

/**
 *
 *  JIT compile a kernel
 *  kernelName:     name of the kernel, must be < 255 chars.
 *  kernelIsa:      common ISA binary of the kernel supplied by CMRT.
 *  kernelIsaSize:  size of the CISA binary in bytes.
 *  genBinary:      pointer to the Gen binary returned by the jitter.  This is
 *                  allocated by the jitter, and the runtime should free it when
 *                  it's done by calling the freeBlock() function.
 *  genBinarySize:  size in bytes of the Gen binary returned by the jitter.
 *  platform:       platform for the Gen binary.  Currently supported values are
 *                  "BDW", "CHV", "SKL", "BXT", "CNL".
 *  majorVersion:   major version of the CISA binary.  Each jitter
 *                  should declare the highest version of CISA it supports,
 *                  and it should support any CISA file below that version
 *  minorVersion:   minor version of the CISA binary
 *  numArgs:        number of arguments for this JIT compilation.
 *  args:           list of arguments to the Jitter.  The complete list of options
*                   can be found in Option.cpp
 *  errorMsg:       If JIT compiling fails, the error message will be returned via
 *                  this parameter.  This buffer is allocated by the runtime.
 *  jitInfo:        A structure containing auxiliary information about the JIT
 *                  compilation that the jitter can pass back to the runtime.
 */
DLL_EXPORT int JITCompile(const char* kernelName,
                          const void* kernelIsa,
                          unsigned int kernelIsaSize,
                          void* &genBinary,
                          unsigned int& genBinarySize,
                          const char* platform,
                          int majorVersion,
                          int minorVersion,
                          int numArgs,
                          const char* args[],
                          char* errorMsg,
                          FINALIZER_INFO* jitInfo);

/**
  * In addition to JITCompile, following API adds support for relocation of immediate operands.
  *
  * numInputRelocEntries:  Number of relocation present in VISA input
  * inputRelocs:           Actual relocation data in VISA
  * numOutputRelocs:       Post compilation, this variable holds number of relocs
  *                        that are still present in native binary. This number
  *                        could be less/more than input relocs, theoretically,
  *                        due to optimizations - loop unrolling (which JIT doesnt do today),
  *                        DCE. For most purposes though, number of input and output relocs
  *                        have a 1:1 correspondence.
  * outputRelocs:          Actual relocation data computed post compilation of VISA to Gen
  *                        binary.
  */

DLL_EXPORT int JITCompileWithRelocation(const char* kernelName,
                                        const void* kernelIsa,
                                        unsigned int kernelIsaSize,
                                        void* &genBinary,
                                        unsigned int& genBinarySize,
                                        const char* platform,
                                        int majorVersion,
                                        int minorVersion,
                                        int numArgs,
                                        const char* args[],
                                        char* errorMsg,
                                        FINALIZER_INFO* jitInfo,
                                        const unsigned int numInputRelocEntries,
                                        const BasicRelocEntry* inputRelocs,
                                        unsigned int& numOutputRelocs,
                                        BasicRelocEntry*& outputRelocs);

/**
 *
 *  Interface for CMRT to free the kernel binary allocated by the Jitter
 */
DLL_EXPORT void freeBlock(void* ptr);

/**
 *
 *  Returns the CISA version #(<major_version>.<minor_version>) for this jitter.
 *  a jitter will support all vISA objects whose major version is the same as the jiter
 *  and whose minor version is <= jitter's minor version.
 *
 */
DLL_EXPORT void getJITVersion(unsigned int& majorV, unsigned int& minorV);
