/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "JitterDataStruct.h"
#include "VISADefines.h"

/**
    API between CMRT and Jitter:
    JITCompile
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
 *  args:           list of arguments to the Jitter.  The complete list of
 * options can be found in Option.cpp errorMsg:       If JIT compiling fails,
 * the error message will be returned via this parameter.  This buffer is
 * allocated by the runtime. jitInfo:        A structure containing auxiliary
 * information about the JIT compilation that the jitter can pass back to the
 * runtime.
 */
DLL_EXPORT int JITCompile(const char *kernelName, const void *kernelIsa,
                          unsigned int kernelIsaSize, void *&genBinary,
                          unsigned int &genBinarySize, const char *platform,
                          int majorVersion, int minorVersion, int numArgs,
                          const char *args[], char *errorMsg,
                          vISA::FINALIZER_INFO *jitInfo);

/**
 *
 *  Interface for CMRT to free the kernel binary allocated by the Jitter
 */
DLL_EXPORT void freeBlock(void *ptr);

/**
 *
 *  Returns the CISA version #(<major_version>.<minor_version>) for this jitter.
 *  a jitter will support all vISA objects whose major version is the same as
 * the jiter and whose minor version is <= jitter's minor version.
 *
 */
DLL_EXPORT void getJITVersion(unsigned int &majorV, unsigned int &minorV);
