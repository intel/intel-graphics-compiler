/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// This is shared with the UMD. This holds all compilation options that the UMD
// can set based upon AIL or other criteria.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "../../IGC/AdaptorCommon/RayTracing/ADT/Optional.h"

namespace IGC {

template <template<typename> typename T>
struct RTCompileOptionsT
{
    T<uint32_t> TileXDim1D;
    T<uint32_t> TileYDim1D;
    T<uint32_t> TileXDim2D;
    T<uint32_t> TileYDim2D;
    T<uint32_t> RematThreshold;
    T<bool>     HoistRemat;
};

using RTCompileOptions = RTCompileOptionsT<Interface::Optional>;

} // namespace IGC
