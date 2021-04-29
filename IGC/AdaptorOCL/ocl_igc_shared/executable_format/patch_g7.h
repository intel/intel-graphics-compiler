/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

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
