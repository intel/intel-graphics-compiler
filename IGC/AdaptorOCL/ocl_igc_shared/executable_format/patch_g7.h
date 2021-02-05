/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

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

#pragma once
#include "stdint.h"
#include "patch_shared.h"

namespace iOpenCL
{

/*****************************************************************************\
STRUCT: SKernelBinaryHeaderGen7
\*****************************************************************************/
struct SKernelBinaryHeaderGen7 :
       SKernelBinaryHeaderCommon
{
};

/*****************************************************************************\
STRUCT: SPatchMediaInterfaceDescriptorLoad
\*****************************************************************************/
struct SPatchMediaInterfaceDescriptorLoad :
       SPatchItemHeader
{
    uint32_t   InterfaceDescriptorDataOffset;
};

/*****************************************************************************\
STRUCT: SPatchInterfaceDescriptorData
\*****************************************************************************/
struct SPatchInterfaceDescriptorData :
       SPatchItemHeader
{
    uint32_t   Offset;
    uint32_t   SamplerStateOffset;
    uint32_t   KernelOffset;
    uint32_t   BindingTableOffset;
};

/*****************************************************************************\
STRUCT: SPatchDataParameterStream
\*****************************************************************************/
struct SPatchDataParameterStream :
       SPatchItemHeader
{
    uint32_t   DataParameterStreamSize;
};

} // namespace
