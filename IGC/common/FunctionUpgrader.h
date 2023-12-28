/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"

class FunctionUpgrader
{
private:
    llvm::Function* m_pFunction = nullptr;

    // map place holder argument with the real argument
    llvm::MapVector<llvm::LoadInst*, llvm::Argument*> m_pNewArguments;
    std::vector<llvm::LoadInst*> m_placeHolders;

    llvm::FunctionType* UpgradeFunctionTypeWithNewArgs();

    llvm::Function* UpgradeFunctionWithNewArgs();

    void CleanPlaceHoldersArgs();

public:
    void SetFunctionToUpgrade(llvm::Function* pFunction);
    void Clean();

    int SizeArgFromRebuild();

    bool IsUsedPlacedHolder(llvm::Value* PlaceHolderToCheck);

    llvm::Argument* GetArgumentFromRebuild(llvm::Value* pPlaceHolderArg);
    llvm::Argument* GetArgumentFromRebuild(llvm::LoadInst* pPlaceHolderArg);
    std::vector<llvm::LoadInst*> GetPlaceholderVec();

    llvm::Value* AddArgument(llvm::StringRef argName, llvm::Type* argType);

    uint32_t GetArgumentsSize();
    llvm::Function* RebuildFunction();

    bool NeedToRebuild();
};
