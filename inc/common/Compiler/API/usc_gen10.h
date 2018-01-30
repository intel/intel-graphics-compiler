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
#include "usc_gen9.h"

namespace USC
{

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputVertexShader_Gen10 : public SCompilerOutputVertexShader_Gen9
{
    // identical to Gen9
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputHullShader_Gen10 : public SCompilerOutputHullShader_Gen9
{
    // identical to Gen9
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputDomainShader_Gen10 : public SCompilerOutputDomainShader_Gen9
{
    // identical to Gen9
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputGeometryShader_Gen10 : public SCompilerOutputGeometryShader_Gen9
{
    // identical to Gen9
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputPixelShader_Gen10 : public SCompilerOutputPixelShader_Gen9
{
    // ### DW1  3DSTATE_PS_EXTRA ###
    unsigned int    m_RequiresRequiredCoarsePixelSize;  // (DW1, bit 21)
    unsigned int    m_KernelIsCoarseRate;               // (DW1, bit 4)
};

/*****************************************************************************\
STRUCT: SCompilerInputComputeShader_Gen9
\*****************************************************************************/
struct SCompilerInputComputeShader_Gen10 : public SCompilerInputComputeShader_Gen9
{
    // identical to Gen9
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputComputeShader_Gen10 : public SCompilerOutputComputeShader_Gen9
{
    // identical to Gen9
};

} // namespace USC
