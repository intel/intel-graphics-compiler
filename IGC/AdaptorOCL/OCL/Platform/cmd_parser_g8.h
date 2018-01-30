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

//#include "ContextTypes.h"
#include "../inc/common/igfxfmid.h"

namespace G6HWC
{

/*****************************************************************************\
Direct state debug functions
\*****************************************************************************/
DWORD   DebugCommand(
            const void* pBuffer,
            const PLATFORM productID );

DWORD   DebugMICommand(
            const void* pBuffer,
            const PLATFORM productID );

DWORD   DebugGfxCommand(
            const void* pBuffer,
            const PLATFORM productID );

/*****************************************************************************\
Indirect state debug functions
\*****************************************************************************/
void    DebugSamplerStateCommand(
            const void* pLinearAddress,
            const PLATFORM productID );

void    DebugVmeStateCommand(
            const void* pLinearAddress,
            const PLATFORM productID );

void    DebugSamplerIndirectStateCommand(
            const void* pLinearAddress,
            const PLATFORM productID );

void    DebugBindingTableStateCommand(
            const void* pLinearAddress,
            const DWORD numBindingTableEntries,
            const PLATFORM productID );

void    DebugSurfaceStateCommand(
            const void* pLinearAddress,
            const PLATFORM productID );

void    DebugMediaSurfaceStateCommand(
            const void* pLinearAddress,
            const PLATFORM productID );

void    DebugInterfaceDescriptorCommand(
            const void* pLinearAddress,
            const PLATFORM productID );

void    DebugInterfaceDescriptorDataCommand(
            const void* pLinearAddress,
            const PLATFORM productID );
 
void    DebugKernelProgram(
            const void* pLinearAddress,
            const DWORD size );

void    DebugSurfaceStateBufferLengthCommand(
            const void* pLinearAddress,    
            const PLATFORM productID );

}  // namespace G6HWC
