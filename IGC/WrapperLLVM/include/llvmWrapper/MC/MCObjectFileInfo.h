/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_MC_MCOBJECTFILEINFO_H
#define IGCLLVM_MC_MCOBJECTFILEINFO_H

#include "llvm/Config/llvm-config.h"
#include "llvm/MC/MCObjectFileInfo.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR <= 12
    using llvm::MCObjectFileInfo;
#else
    class MCObjectFileInfo : public llvm::MCObjectFileInfo
    {
    public:
        inline void InitMCObjectFileInfo(const llvm::Triple &TT, bool PIC, llvm::MCContext &ctx,
                            bool LargeCodeModel = false)
        {
            this->initMCObjectFileInfo(ctx, PIC, LargeCodeModel);
        }
    };
#endif
}

#endif
