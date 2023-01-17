/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvm/Config/llvm-config.h"
#include "FunctionUpgrader.h"
#include "IGCIRBuilder.h"
#include "Probe/Assertion.h"

using namespace llvm;

void FunctionUpgrader::SetFunctionToUpgrade(llvm::Function* pFunction)
{
    m_pFunction = pFunction;
}

void FunctionUpgrader::Clean()
{
    m_pFunction = nullptr;
    m_pNewArguments.clear();
    m_placeHolders.clear();
}

llvm::Value* FunctionUpgrader::AddArgument(llvm::StringRef argName, llvm::Type* argType)
{
    IGC_ASSERT(nullptr != m_pFunction);

    llvm::IGCIRBuilder<> builder(m_pFunction->getContext());
    if (m_pFunction->begin()->empty())
    {
        builder.SetInsertPoint(&*m_pFunction->begin());
    }
    else
    {
        builder.SetInsertPoint(&*m_pFunction->begin()->begin());
    }
    llvm::AllocaInst* pPlaceHolderArgAlloc = builder.CreateAlloca(argType, 0, "");
    // Create place holder load, which will simulate the argument for now
    llvm::LoadInst* PlaceHolderArg = builder.CreateLoad(pPlaceHolderArgAlloc, argName);

    m_pNewArguments[PlaceHolderArg] = nullptr;
    m_placeHolders.push_back(PlaceHolderArg);

    return PlaceHolderArg;
}

bool FunctionUpgrader::IsUsedPlacedHolder(llvm::Value* PlaceHolderToCheck)
{
    return m_pNewArguments.find((LoadInst*)PlaceHolderToCheck) != m_pNewArguments.end();
}

Argument* FunctionUpgrader::GetArgumentFromRebuild(llvm::Value* pPlaceHolderArg)
{
    return GetArgumentFromRebuild((LoadInst*)pPlaceHolderArg);
}

Argument* FunctionUpgrader::GetArgumentFromRebuild(llvm::LoadInst* pPlaceHolderArg)
{
    if (IsUsedPlacedHolder(pPlaceHolderArg))
    {
        if (m_pNewArguments[pPlaceHolderArg] != nullptr)
        {
            return m_pNewArguments[pPlaceHolderArg];
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "There is no created new argument, did you call RebuildFunction?");
            return nullptr;
        }
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "Didn't found new argument!");
        return nullptr;
    }
}

std::vector<LoadInst*> FunctionUpgrader::GetPlaceholderVec()
{
    return m_placeHolders;
}

uint32_t FunctionUpgrader::GetArgumentsSize()
{
    IGC_ASSERT(nullptr != m_pFunction);

    return m_placeHolders.size() + m_pFunction->arg_size();
}

Function* FunctionUpgrader::RebuildFunction()
{
    Function* pFunctionRebuild = UpgradeFunctionWithNewArgs();

    IGC_ASSERT(nullptr != m_pFunction);
    IGC_ASSERT(nullptr != pFunctionRebuild);

    auto i_arg_old = m_pFunction->arg_begin();
    auto i_arg_new = pFunctionRebuild->arg_begin();

    // Replace all usage in new method from old arguments to new arguments
    while(i_arg_old != m_pFunction->arg_end())
    {
        i_arg_old->replaceAllUsesWith(i_arg_new);
        ++i_arg_old;
        ++i_arg_new;
    }

    // Replace all usage in new method from place holder to real argument
    for (auto it = m_pNewArguments.begin(); it != m_pNewArguments.end(); ++it)
    {
        it->first->replaceAllUsesWith(it->second);
    }

    CleanPlaceHoldersArgs();

    pFunctionRebuild->copyAttributesFrom(m_pFunction);
    pFunctionRebuild->setSubprogram(m_pFunction->getSubprogram());
    m_pFunction->setSubprogram(nullptr);
    pFunctionRebuild->takeName(m_pFunction);

    return pFunctionRebuild;
}

FunctionType* FunctionUpgrader::UpgradeFunctionTypeWithNewArgs()
{
    std::vector<Type*> args;

    // Get from old function all arguments type
    for (auto i = m_pFunction->arg_begin(); i != m_pFunction->arg_end(); ++i)
    {
        args.push_back(i->getType());
    }

    // Append new list arguments for new function
    for (auto it = m_pNewArguments.begin(); it != m_pNewArguments.end(); ++it)
    {
        args.push_back(it->first->getType());
    }

    return FunctionType::get(m_pFunction->getReturnType(), args, m_pFunction->isVarArg());
}

Function* FunctionUpgrader::UpgradeFunctionWithNewArgs()
{
    IGC_ASSERT(nullptr != m_pFunction);

    auto fType = UpgradeFunctionTypeWithNewArgs();
    auto fModule = m_pFunction->getParent();
    auto fName = m_pFunction->getName();
    auto fLinkage = m_pFunction->getLinkage();

    // Create a new function
    Function* pNewFunc =
        Function::Create(
            fType,
            fLinkage,
            fName);

    IGC_ASSERT(nullptr != fModule);

    fModule->getFunctionList().insert(m_pFunction->getIterator(), pNewFunc);

    // rename all args
    auto i_arg_old = m_pFunction->arg_begin();
    auto i_arg_new = pNewFunc->arg_begin();

    // Map old -> new args
    // and setname for new func args from old func args
    for (; i_arg_old != m_pFunction->arg_end(); ++i_arg_old, ++i_arg_new)
    {
        auto arg_it = i_arg_old;
        i_arg_new->takeName(arg_it);
    }
    // setname for new func args which was added as new one
    for (auto it = m_pNewArguments.begin(); it != m_pNewArguments.end(); ++it)
    {
        auto arg_it = i_arg_new;
        //add to map pointer of the new argument
        m_pNewArguments[it->first] = arg_it;

        m_pNewArguments[it->first]->takeName(it->first);

        ++i_arg_new;
    }

    pNewFunc->getBasicBlockList().splice(pNewFunc->begin(), m_pFunction->getBasicBlockList());

    return pNewFunc;
}

void FunctionUpgrader::CleanPlaceHoldersArgs()
{
    for (auto it = m_pNewArguments.begin(); it != m_pNewArguments.end(); ++it)
    {
        // Remove all place holder stuff from new func
        LoadInst* pPlaceHolderArg = it->first;
        AllocaInst* pPlaceHolderArgAlloc = cast<AllocaInst>(pPlaceHolderArg->getPointerOperand());

        pPlaceHolderArg->eraseFromParent();
        pPlaceHolderArgAlloc->eraseFromParent();
    }
}

int FunctionUpgrader::SizeArgFromRebuild()
{
    return m_pNewArguments.size();
}

bool FunctionUpgrader::NeedToRebuild()
{
    return m_pNewArguments.size() > 0;
}
