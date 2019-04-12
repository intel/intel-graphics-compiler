/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"

class FunctionUpgrader
{
private:
    llvm::Function* m_pFunction;

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