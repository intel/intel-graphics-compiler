/*========================== begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef STRUCTURE_ALINGMENT_VERIFICATION
#pragma once
#endif

#include "usc_gen7_types.h"

namespace USC
{

/*****************************************************************************\
\*****************************************************************************/
USC_PARAM()
struct SCompilerOutputVertexShader_Gen8 : public SCompilerOutputVertexShader_Gen7
{
    // ### DW5 3DSTATE_VS ###
    unsigned int   m_SIMD8DispatchEnable;    // SIMD8 Dispatch Enable (DW5, bit 2) 

    // ### DW6 3DSTATE_VS ###
    unsigned int   m_VertexURBEntryOutputReadOffset;  //(DW6, bits 26..21)
    unsigned int   m_VertexURBEntryOutputLength;      //(DW6, bits 20..16)

    PADDING_4_BYTES
};

/*****************************************************************************\
Note: If any additional Gen8 field appears add USC_PARAM() before declaration.
\*****************************************************************************/
struct SCompilerOutputHullShader_Gen8 : public SCompilerOutputHullShader_Gen7
{
    //identical to Gen7
};

/*****************************************************************************\
\*****************************************************************************/
USC_PARAM()
struct SCompilerOutputDomainShader_Gen8 : public SCompilerOutputDomainShader_Gen7
{
    // ### DW7 3DSTATE_DS ###
    unsigned int   m_SIMD8DispatchEnable;               //(DW7, bit 3)

    // ### DW8 3DSTATE_DS ###
    unsigned int   m_VertexURBEntryOutputReadOffset;    //(DW8, bits 26..21)
    unsigned int   m_VertexURBEntryOutputLength;        //(DW8, bits 20..16)

    PADDING_4_BYTES

    // clip cull fields are in Gen7 "Other"
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputGeometryShader_Gen8 : public SCompilerOutputGeometryShader_Gen7
{
    // ### DW1 3DSTATE_GS ###
    void*          m_pKernelProgram;        //Kernel Start Pointer - Rendering Enabled (DW1, bit 31..6)
    unsigned int   m_KernelProgramSize;     //Kernel Program Size

    // ### DW3 3DSTATE_GS ###    
    unsigned int   m_ExpectedVertexCount;   // Expected vertex count per input object  (DW3, bit 5..0)

    // ### DW6 3DSTATE_GS ###    
    unsigned int   m_OutputVertexSize;      //Output Vertex Size - Rendering Enabled   (DW6, bit 28..23)

    // ### DW9 3DSTATE_GS ###
    unsigned int   m_VertexURBEntryOutputReadOffset; //GS Vertex URB Entry Output Read Offset by SBE (DW9, bit 26..21)
    unsigned int   m_VertexURBEntryOutputLength;     //GS Vertex URB Entry Output Length by SBE      (DW9, bit 20..16)

    // ### DW8 3DSTATE_GS ###
    unsigned int   m_StaticOutputVertexCount; //Static Output Vertex Count (DW8, bit 26..16)
    bool           m_StaticOutput;            //StaticOutput               (DW8, bit 30)

    PADDING_1_BYTE
    PADDING_2_BYTES
    PADDING_4_BYTES_x64_ONLY
};

/*****************************************************************************\
\*****************************************************************************/
USC_PARAM()
struct SCompilerOutputPixelShader_Gen8 : public SCompilerOutputPixelShader_Gen7
{
    unsigned int m_IsFrontFaceUsed;
    unsigned int m_DoesNotWriteRT;
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputComputeShader_Gen8 : public SCompilerOutputComputeShader_Gen7
{
    // TODO!!! Any new output required for Gen8?  

};

} // namespace USC
