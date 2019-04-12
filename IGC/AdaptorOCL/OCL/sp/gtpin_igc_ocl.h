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

#include "../../../Compiler/CodeGenPublic.h"

#ifndef GTPIN_IGC_OCL_H
#define GTPIN_IGC_OCL_H

#if defined( _WIN32 ) || defined( _WIN64 )
// Windows
#define MY_EXTERN_C    extern "C"
#define MY_DLLEXPORT   __declspec( dllexport) __declspec( noinline )
#define MY_CALLINGSTD  _fastcall
#define MY_CDECL       _cdecl

#elif defined(ANDROID) || defined (__linux__)

// Linux, Android
#define MY_EXTERN_C    extern "C"
#define MY_DLLEXPORT   __attribute__((visibility("default")))
#define MY_CALLINGSTD
#define MY_CDECL

#else
    
#define MY_EXTERN_C    extern "C"
#define MY_DLLEXPORT   
#define MY_CALLINGSTD
#define MY_CDECL

#endif

#ifndef GTPIN_EXTERNAL_TYPEDEFS_H

// The following definitions must match those defined in gtpin_external_typedefs.h in the GT-Pin source tree

#define GTPIN_EXTERNAL_TYPEDEFS_H

typedef enum 
{
    GEN_ISA_TYPE_INVALID =0,
    GEN_ISA_TYPE_GEN6 = 1,
    GEN_ISA_TYPE_GEN7 = 2,
    GEN_ISA_TYPE_GEN7p5 = 3,
    GEN_ISA_TYPE_GEN8 = 4,
    GEN_ISA_TYPE_GEN9 = 5,
    GEN_ISA_TYPE_GEN10 = 6,
    GEN_ISA_TYPE_GEN11 = 7
}   GEN_ISA_TYPE;

typedef enum 
{
    GEN_GT_TYPE_INVALID = 0,
    GEN_GT_TYPE_GT1 = 1,
    GEN_GT_TYPE_GT2 = 2,
    GEN_GT_TYPE_GT3 = 3,
    GEN_GT_TYPE_GT4 = 4,
    GEN_GT_TYPE_GTVLV = 5,
    GEN_GT_TYPE_GTVLVPLUS = 6
}   GEN_GT_TYPE;

// List of driver versions that have GT-Pin support
#define GTPIN_DRIVERVERSION_OPEN      "intel-open"

typedef enum 
{
    GTPIN_INVOKE_STRUCT_ARG_POS_FUNPTR = 0,
    GTPIN_INVOKE_STRUCT_ARG_POS_GTPROGRAMBINARY = 1,
    GTPIN_INVOKE_STRUCT_ARG_POS_OCLKERNELINFO = 2,
    GTPIN_INVOKE_STRUCT_ARG_POS_DRIVERVERSION = 3,
    GTPIN_INVOKE_STRUCT_ARG_POS_OCLDEBUGDATA=4,
    GTPIN_INVOKE_STRUCT_ARG_POS_OCLDEBUGSIZE=5,
    GTPIN_INVOKE_STRUCT_ARG_POS_LAST
}   GTPIN_INVOKE_STRUCT_ARG_POS;

// This enum will be used as a bitmask so each member should
// have a single bit set to 1.
typedef enum
{
    GTPIN_SUPPORTS_RERA = 0x1,
    GTPIN_SUPPORTS_GRF_INFO = 0x2,
} GTPIN_SUPPORTED_CAPABILITY;

typedef struct GTPIN_INVOKE_STRUCT 
{
    int _numArgs;
    void** _args;
    bool operator==( struct GTPIN_INVOKE_STRUCT& other ); 
}   GTPIN_INVOKE_STRUCT;

typedef bool (MY_CDECL *INVOKE_GTPIN_PROC)(
    GEN_ISA_TYPE isa, 
    void* ptrToOrigGenBinary, 
    int origGenBinarySizeInBytes,
    const char* commandLine, 
    GTPIN_INVOKE_STRUCT* gtpinInvokeStruct,
    const int tbufIdx, 
    const int sbufIdx, 
    const int scacheIdx,
    void* &ptrToInstrumentedBinary, 
    int& instrumentedGenBinarySizeInBytes, 
    char*& resultMsg );

#endif // GTPIN_EXTERNAL_TYPEDEFS_H 

MY_EXTERN_C MY_DLLEXPORT bool MY_CALLINGSTD GTPIN_IGC_OCL_IsEnabled();
MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_NumberOfSurfaces();
MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_UpdateKernelInfo( 
    const unsigned kernelBinarySize,
    const int scratchSurfBti, 
    const int pBufBti );
MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_GetSurfaceBTI( const int i );
MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_GetEnqueueInstanceKernelArgNo();
MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_GetSurfaceKernelArgNo( const int i );
MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_CompilerReservedRegs( const char* regNames );
MY_EXTERN_C MY_DLLEXPORT char*  MY_CALLINGSTD GTPIN_IGC_OCL_GetCommandLine();
MY_EXTERN_C MY_DLLEXPORT GTPIN_INVOKE_STRUCT* MY_CALLINGSTD GTPIN_IGC_OCL_GetInvokeStruct();
MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_FillAllKernelsInfo();
MY_EXTERN_C MY_DLLEXPORT GEN_ISA_TYPE MY_CALLINGSTD GTPIN_IGC_OCL_GetGenIsaFromPlatform( 
    PLATFORM platform );
MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_Instrument(
    const GEN_ISA_TYPE genIsa,
    const char* driverVersionStr,
    const int originalBinarySize,
    void* originalBinaryOutput,
    int &instrumentedBinarySize,
    void* &instrumentedBinaryOutput);
MY_EXTERN_C MY_DLLEXPORT uint64_t MY_CALLINGSTD GTPIN_IGC_OCL_GetSupportedFeatures();

#endif /* GTPIN_IGC_OCL_H */

