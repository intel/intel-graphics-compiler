/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "RTStackFormat.h"
#include "iStdLib/utility.h"
#include <array>
#include "Probe/Assertion.h"

namespace RTStackFormat {

// Compute the size of the hardware portion of the RTStack.
uint32_t getRTStackHeaderSize(uint32_t MaxBVHLevels)
{
    // TODO: The compiler currently hardcodes its offsets to assume 2 BVH
    // levels.  When we move to read the BVH levels from the
    // RayDispatchGlobalData we can remove this check.
    IGC_ASSERT_MESSAGE((MaxBVHLevels > 0), "Unsupported level!");

#define STYLE(X) static_assert(sizeof(RTStack2<Xe>) == sizeof(RTStack2<X>));
#include "RayTracingMemoryStyle.h"
#undef STYLE

    static constexpr std::array<uint32_t, 9> Sizes =
    {
        0,
        sizeof(RTStack<Xe, 1>),
        sizeof(RTStack<Xe, 2>),
        sizeof(RTStack<Xe, 3>),
        sizeof(RTStack<Xe, 4>),
        sizeof(RTStack<Xe, 5>),
        sizeof(RTStack<Xe, 6>),
        sizeof(RTStack<Xe, 7>),
        sizeof(RTStack<Xe, 8>),
    };

    // stack size per ray is in encoded in 64 byte chunks.  Align up so the UMD
    // can evenly divide it.
    // In addition, we currently align up to 128 bytes so that different RTStacks
    // won't cross an LSC sector.
    static_assert(IGC::RTStackAlign % IGC::RayDispatchGlobalData::StackChunkSize == 0, "no?");
    return IGC::Align(Sizes[MaxBVHLevels], IGC::RTStackAlign);
}

} // namespace RTStackFormat
