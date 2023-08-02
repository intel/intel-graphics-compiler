/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "GenIntrinsicEnum.h"


namespace IGC
{

llvm::GenISAIntrinsic::ID LookupIntrinsicId(const char* pName);

} // namespace IGC
