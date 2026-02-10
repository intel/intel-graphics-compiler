/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/MDFrameWork.h"
#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include <optional>

namespace llvm {
class Value;
class DataLayout;
} // namespace llvm

namespace IGC {

enum class RTMemRegion : uint32_t {
  RTAsyncStack,
  RTSyncStack,
  SWStack,
  SWHotZone,
  RTGlobals,
  LocalArgs,
};

std::optional<RTMemRegion> getRTRegionByAddrspace(const llvm::Value *V, const ModuleMetaData &MMD);

std::optional<RTMemRegion> getRegionOffset(const llvm::Value *Ptr, const ModuleMetaData &moduleMetaData,
                                           const llvm::DataLayout *DL = nullptr, uint64_t *Offset = nullptr,
                                           uint64_t *dereferenceable_value = nullptr);

std::optional<RTMemRegion> getRTRegion(const llvm::Value *Ptr, const ModuleMetaData &MMD);

} // namespace IGC
