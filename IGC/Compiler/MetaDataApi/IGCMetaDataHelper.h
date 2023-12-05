/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/MDFrameWork.h"
#include "common/igc_regkeys.hpp"

namespace IGC::IGCMD {
    class IGCMetaDataHelper
    {
    public:
        static void addFunction(MetaDataUtils& mdUtils, llvm::Function* pFunc, IGC::FunctionTypeMD type = IGC::FunctionTypeMD::KernelFunction);
        static void moveFunction(MetaDataUtils& mdUtils, ModuleMetaData& MD, llvm::Function* OldFunc, llvm::Function* NewFunc);
        static void removeFunction(MetaDataUtils& mdUtils, ModuleMetaData& MD, llvm::Function* Func);

        // In OCL, thread group size (hint) is given by kernel attributes reqd_work_group_size and work_group_size_hint.
        // Return thread group size (hint) if present; return 0 otherwise.
        static llvm::Optional<std::array<uint32_t, 3>> getThreadGroupDims(MetaDataUtils& mdUtils, llvm::Function* pKernelFunc);
        static uint32_t getThreadGroupSize(MetaDataUtils& mdUtils, llvm::Function* pKernelFunc);
        static uint32_t getThreadGroupSizeHint(MetaDataUtils& mdUtils, llvm::Function* pKernelFunc);
    };
}
