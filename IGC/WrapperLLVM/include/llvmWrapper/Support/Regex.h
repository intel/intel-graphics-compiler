/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_REGEX_H
#define IGCLLVM_SUPPORT_REGEX_H

#include "llvm/Support/Regex.h"

#include <string>

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR < 10
    inline bool isValid(const llvm::Regex &R)
    {
        std::string Error;
        return R.isValid(Error);
    }
#else
    inline bool isValid(const llvm::Regex &R) { return R.isValid(); }
#endif

} // namespace IGCLLVM

#endif
