/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "GenIntrinsicEnum.h"
#include "GenIntrinsics.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/ArrayRef.h>
#include "common/LLVMWarningsPop.hpp"

#include <string>

namespace llvm
{
class Type;
class Module;
} // namespace IGC

namespace IGC
{

const char* GetIntrinsicPrefixName();

std::string GetName(llvm::GenISAIntrinsic::ID id, llvm::ArrayRef<llvm::Type*> OverloadedTys);

llvm::GenISAIntrinsic::IntrinsicComments GetIntrinsicComments(llvm::GenISAIntrinsic::ID id);

llvm::Function* GetDeclaration(llvm::Module* pModule, llvm::GenISAIntrinsic::ID id, llvm::ArrayRef<llvm::Type*> overloadedTys);

} // namespace IGC