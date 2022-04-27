/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_LLD_COMMON_DRIVER_H
#define IGCLLVM_LLD_COMMON_DRIVER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/raw_ostream.h"

#include "lld/Common/Driver.h"

namespace IGCLLD {
    namespace elf {
        inline bool link(llvm::ArrayRef<const char *> Args, bool CanExitEarly,
                         llvm::raw_ostream &stdoutOS, llvm::raw_ostream &stderrOS) {
#if LLVM_VERSION_MAJOR >= 14
            return lld::elf::link(Args, stdoutOS, stderrOS, CanExitEarly, false);
#elif LLVM_VERSION_MAJOR >= 10
            return lld::elf::link(Args, CanExitEarly, stdoutOS, stderrOS);
#else
            (void)stdoutOS;
             return lld::elf::link(Args, CanExitEarly, stderrOS);
#endif
        }
    } // namespace elf
} // namespace IGCLLD

#endif // IGCLLVM_LLD_COMMON_DRIVER_H
