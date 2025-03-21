/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <string>
#include <optional>

#include "ConstantsEnums.h"

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

struct HitGroupInfo
{
    std::string HitGroupName;
    RTStackFormat::HIT_GROUP_TYPE Type;
    std::string ClosestHit;
    std::string Intersection;
    std::string AnyHit;
};

} // namespace IGC
