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

#include "gtpin_igc_ocl.h"
#if !defined(_WIN32)
#define _strdup strdup
#endif

#include "Probe/Assertion.h"

//
// We need to make the following functions that will be replaced by GT-Pin at runtime
// a little more complex to prevent the compiler from optimizing them (apparently,
// the compiler did perform some link-time optimization on these functions).
//
static volatile bool gtpinIsEnabled = false;

MY_EXTERN_C MY_DLLEXPORT bool MY_CALLINGSTD GTPIN_IGC_OCL_IsEnabled()
{
    return gtpinIsEnabled;
}

MY_EXTERN_C MY_DLLEXPORT void MY_CALLINGSTD GTPIN_IGC_OCL_SetEnabled(const bool x)
{
    gtpinIsEnabled = x;
}

MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_NumberOfSurfaces()
{
    // This function body will be replaced by Pin at runtime if GT-Pin is used.
    if (GTPIN_IGC_OCL_IsEnabled())
    {
        IGC_ASSERT(0);
        return 1;
    }
    else
    {
        return 0;
    }
}

MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_UpdateKernelInfo(const unsigned kernelBinarySize,
                                                                          const int scratchSurfBti,
                                                                          const int pBufBti )
{
    // This function body will be replaced by Pin at runtime if GT-Pin is used.
    if (GTPIN_IGC_OCL_IsEnabled())
    {
        IGC_ASSERT(0);
        return 2;
    }
    else
    {
        return 0;
    }
}

MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_GetSurfaceBTI( const int i )
{
    // This function body will be replaced by Pin at runtime if GT-Pin is used.
    if (GTPIN_IGC_OCL_IsEnabled())
    {
        IGC_ASSERT(0);
        return 3;
    }
    else
    {
        return 0;
    }
}

MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_GetEnqueueInstanceKernelArgNo()
{
    // This function body will be replaced by Pin at runtime if GT-Pin is used.
    if (GTPIN_IGC_OCL_IsEnabled())
    {
        IGC_ASSERT(0);
        return 4;
    }
    else
    {
        return 0;
    }
}

MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_GetSurfaceKernelArgNo( const int i )
{
    // This function body will be replaced by Pin at runtime if GT-Pin is used.
    if (GTPIN_IGC_OCL_IsEnabled())
    {
        IGC_ASSERT(0);
        return 5;
    }
    else
    {
        return 0;
    }
}

MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_CompilerReservedRegs( const char* regNames )
{
    // This function body will be replaced by Pin at runtime if GT-Pin is used.
    if (GTPIN_IGC_OCL_IsEnabled())
    {
        IGC_ASSERT(0);
        return 6;
    }
    else
    {
        return 0;
    }
}

MY_EXTERN_C MY_DLLEXPORT char*  MY_CALLINGSTD GTPIN_IGC_OCL_GetCommandLine()
{
    // This function body will be replaced by Pin at runtime if GT-Pin is used.
    if (GTPIN_IGC_OCL_IsEnabled())
    {
        IGC_ASSERT(0);
        return (char*)7;
    }
    else
    {
        return 0;
    }
}

MY_EXTERN_C MY_DLLEXPORT GTPIN_INVOKE_STRUCT* MY_CALLINGSTD GTPIN_IGC_OCL_GetInvokeStruct()
{
    // This function body will be replaced by Pin at runtime if GT-Pin is used.
    if (GTPIN_IGC_OCL_IsEnabled())
    {
        IGC_ASSERT(0);
        return (GTPIN_INVOKE_STRUCT*)8;
    }
    else
    {
        return 0;
    }
}

MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_FillAllKernelsInfo()
{
    // This function body will be replaced by Pin at runtime if GT-Pin is used.
    if (GTPIN_IGC_OCL_IsEnabled())
    {
        IGC_ASSERT(0);
        return 9;
    }
    else
    {
        return 0;
    }
}


MY_EXTERN_C MY_DLLEXPORT GEN_ISA_TYPE MY_CALLINGSTD GTPIN_IGC_OCL_GetGenIsaFromPlatform(
    PLATFORM platform )
{
    switch ( platform.eRenderCoreFamily )
    {
    case IGFX_GEN7_CORE:
        return GEN_ISA_TYPE_GEN7;
    case IGFX_GEN7_5_CORE:
        return GEN_ISA_TYPE_GEN7p5;
    case IGFX_GEN8_CORE:
        return GEN_ISA_TYPE_GEN8;
    case IGFX_GEN9_CORE:
        return GEN_ISA_TYPE_GEN9;
    case IGFX_GEN10_CORE:
        return GEN_ISA_TYPE_GEN10;
    default:
        IGC_ASSERT(0);
        return GEN_ISA_TYPE_INVALID;
    }
}

MY_EXTERN_C MY_DLLEXPORT int MY_CALLINGSTD GTPIN_IGC_OCL_Instrument(
    const GEN_ISA_TYPE genIsa,
    const char* driverVersionStr,
    const int originalBinarySize,
    void* originalBinaryOutput,
    int &instrumentedBinarySize,
    void*& instrumentedBinaryOutput)
{
    bool success = true;

    GTPIN_INVOKE_STRUCT* gtpinInvokeStruct = GTPIN_IGC_OCL_GetInvokeStruct();
    success = ( gtpinInvokeStruct != NULL );

    INVOKE_GTPIN_PROC funInvokingGTPin = 0;
    if( success )
    {
        funInvokingGTPin = (INVOKE_GTPIN_PROC) gtpinInvokeStruct->_args[ GTPIN_INVOKE_STRUCT_ARG_POS_FUNPTR ];

        const int res = GTPIN_IGC_OCL_FillAllKernelsInfo();
        // use the result of GTPIN_IGC_OCL_FillAllKernelsInfo() to make sure that the compiler won't optimize this call away
        success = ( res == 0 );
    }

    char* driverVersionDup = 0;
    if( success )
    {
        // pass the driver-version string to GT-Pin
        driverVersionDup = (char*)_strdup(driverVersionStr);
        success = ( driverVersionDup != NULL );
    }

    if( success )
    {
        gtpinInvokeStruct->_args[ GTPIN_INVOKE_STRUCT_ARG_POS_DRIVERVERSION ] = driverVersionDup; // driverVersionDup will be freed inside funInvokingGTPin()
        gtpinInvokeStruct->_args[ GTPIN_INVOKE_STRUCT_ARG_POS_OCLDEBUGDATA ] = NULL; // no debug data for now with IGC
        gtpinInvokeStruct->_args[ GTPIN_INVOKE_STRUCT_ARG_POS_OCLDEBUGSIZE ] = reinterpret_cast< void * >(0); // no debug data for now with IGC

        char* gtpinCommandLine = GTPIN_IGC_OCL_GetCommandLine();
        char* resultMsg = 0;

        // Invoke the GT-Pin binary rewriter
        success = funInvokingGTPin(
            genIsa,
            originalBinaryOutput,
            originalBinarySize,
            gtpinCommandLine,
            gtpinInvokeStruct,
            -1, // not used
            -1, // not used
            -1, // not used
            instrumentedBinaryOutput,
            instrumentedBinarySize,
            resultMsg );
    }

    if( success )
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

MY_EXTERN_C MY_DLLEXPORT uint64_t MY_CALLINGSTD GTPIN_IGC_OCL_GetSupportedFeatures()
{
    uint64_t capability = GTPIN_SUPPORTED_CAPABILITY::GTPIN_SUPPORTS_GRF_INFO |
                          GTPIN_SUPPORTED_CAPABILITY::GTPIN_SUPPORTS_RERA;
    return capability;
}
