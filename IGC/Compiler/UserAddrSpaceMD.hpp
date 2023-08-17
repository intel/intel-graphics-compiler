/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

///=======================================================================================
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Instruction.h"
#include "common/LLVMWarningsPop.hpp"
#include <visa_igc_common_header.h>

namespace IGC
{
    typedef uint user_addrspace;

    class UserAddrSpaceMD
    {
    private:
        user_addrspace user_addrspace_priv;
        user_addrspace user_addrspace_global;
        user_addrspace user_addrspace_local;
        user_addrspace user_addrspace_generic;
        user_addrspace user_addrspace_raystack;
        llvm::LLVMContext* ctx;
        llvm::MDNode* dummyNode;

    public:
        UserAddrSpaceMD() {};
        UserAddrSpaceMD(llvm::LLVMContext* ctx);

        void Set(llvm::Instruction* inst, LSC_DOC_ADDR_SPACE type, llvm::MDNode* node = nullptr);
        LSC_DOC_ADDR_SPACE Get(llvm::Instruction* inst);
        bool Has(llvm::Instruction* inst, LSC_DOC_ADDR_SPACE type);
    };
} // namespace IGC
