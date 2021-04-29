/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace iOpenCL
{
    struct SProgramBinaryHeader;
    struct SKernelBinaryHeaderGen7;

void    DebugProgramBinaryHeader(
            const iOpenCL::SProgramBinaryHeader* pHeader,
            std::string& output );

void    DebugKernelBinaryHeader_Gen7(
            const iOpenCL::SKernelBinaryHeaderGen7* pHeader,
            std::string& output );

void    DebugPatchList(
            const void* pPatchList,
            const DWORD size,
            std::string& output );

} // namespace
