/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <string>

#include "RayTracingConstantsEnums.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/Optional.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

struct HitGroupInfo
{
    llvm::Optional<std::string> HitGroupName;
    RTStackFormat::HIT_GROUP_TYPE Type;
    llvm::Optional<std::string> ClosestHit;
    llvm::Optional<std::string> Intersection;
    llvm::Optional<std::string> AnyHit;

    HitGroupInfo(
        const std::string& ClosestHit,
        const std::string& Intersection,
        const std::string& AnyHit,
        RTStackFormat::HIT_GROUP_TYPE HitGroupType,
        const llvm::Optional<std::string>& HitGroupName)
    {
        if (!ClosestHit.empty())
            this->ClosestHit = ClosestHit;
        if (!Intersection.empty())
            this->Intersection = Intersection;
        if (!AnyHit.empty())
            this->AnyHit = AnyHit;

        this->Type = HitGroupType;
        this->HitGroupName = HitGroupName;
    }
};

} // namespace IGC
