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
#ifndef __USC_GENNEXT_DEBUG_H__
#define __USC_GENNEXT_DEBUG_H__

#include "usc_config.h"
#if GENNEXT_TR
#include "usc_gen7.h"
#include "usc_gen9.h"

namespace USC
{
/*****************************************************************************\
struct SCompilerOutputPixelShader_GenNexton7
       Used with g_GenNextFeatures.CoarsePixelShadingEnable
\*****************************************************************************/
struct SCompilerOutputPixelShader_GenNexton7 : SCompilerOutputPixelShader_Gen7
{
    // ### DW1  3DSTATE_PS_EXTRA ###
    unsigned int    m_RequiresZwDeltas;                 // (DW1, bit 22)
    unsigned int    m_RequiresRequiredCoarsePixelSize;  // (DW1, bit 21)
    unsigned int    m_KernelIsSampleRate;               // (DW1, bit 6)
    unsigned int    m_KernelIsCoarseRate;               // (DW1, bit 4)
    unsigned int    m_BarycentricParamsArePulled;       // (DW1, bit 3)
};

/*****************************************************************************\
struct SCompilerOutputPixelShader_GenNexton9
       Used with g_GenNextFeatures.CoarsePixelShadingEnable
\*****************************************************************************/
struct SCompilerOutputPixelShader_GenNexton9 : SCompilerOutputPixelShader_Gen9
{
    // ### DW1  3DSTATE_PS_EXTRA ###
    unsigned int    m_RequiresZwDeltas;                 // (DW1, bit 22)
    unsigned int    m_RequiresRequiredCoarsePixelSize;  // (DW1, bit 21)
    unsigned int    m_KernelIsCoarseRate;               // (DW1, bit 4)
};
};
#endif // GENNEXT_TR

#endif // __USC_GENNEXT_DEBUG_H__
