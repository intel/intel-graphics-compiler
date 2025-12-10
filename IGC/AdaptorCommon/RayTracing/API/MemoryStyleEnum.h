/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace IGC
{
    enum class RTMemoryStyle
    {
        // This should be in sync with RayTracingMemoryStyle.h!
        Xe,
        Xe3,
        Xe3PEff64,
    };

    enum class RayDispatchInlinedDataStyle
    {
        // This should be in sync with RayTracingMemoryStyle.h!
        Xe,
        Eff64,
    };
} // namespace IGC
