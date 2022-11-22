/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

//#include "ContextTypes.h"
#include "common/igfxfmid.h"

namespace G6HWC
{

/*****************************************************************************\
Direct state debug functions
\*****************************************************************************/
DWORD   DebugCommand(
            const void* pBuffer,
            const PLATFORM productID,
            std::string &output );

DWORD   DebugMICommand(
            const void* pBuffer,
            const PLATFORM productID,
            std::string &output );

DWORD   DebugGfxCommand(
            const void* pBuffer,
            const PLATFORM productID,
            std::string &output );

/*****************************************************************************\
Indirect state debug functions
\*****************************************************************************/
void    DebugSamplerStateCommand(
            const void* pLinearAddress,
            const PLATFORM productID,
            std::string &output );

void    DebugVmeStateCommand(
            const void* pLinearAddress,
            const PLATFORM productID,
            std::string &output );

void    DebugSamplerIndirectStateCommand(
            const void* pLinearAddress,
            const PLATFORM productID,
            std::string &output );

void    DebugBindingTableStateCommand(
            const void* pLinearAddress,
            const DWORD numBindingTableEntries,
            const PLATFORM productID,
            std::string &output );

void    DebugSurfaceStateCommand(
            const void* pLinearAddress,
            const PLATFORM productID,
            std::string &output );

void    DebugMediaSurfaceStateCommand(
            const void* pLinearAddress,
            const PLATFORM productID,
            std::string &output );

void    DebugInterfaceDescriptorCommand(
            const void* pLinearAddress,
            const PLATFORM productID,
            std::string &output );

void    DebugInterfaceDescriptorDataCommand(
            const void* pLinearAddress,
            const PLATFORM productID,
            std::string &output );

void    DebugKernelProgram(
            const void* pLinearAddress,
            const DWORD size,
            std::string &output );

void    DebugSurfaceStateBufferLengthCommand(
            const void* pLinearAddress,
            const PLATFORM productID,
            std::string &output );

}  // namespace G6HWC
