/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "GenIntrinsicEnum.h"
#include <llvm/ADT/StringRef.h>

namespace IGC {

llvm::GenISAIntrinsic::ID LookupIntrinsicId(llvm::StringRef GenISAprefix, llvm::StringRef name);

} // namespace IGC
