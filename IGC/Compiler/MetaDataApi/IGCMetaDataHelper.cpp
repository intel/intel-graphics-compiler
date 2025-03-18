/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "common/MDFrameWork.h"
#include <optional>

using namespace IGC;
using namespace IGC::IGCMD;

void IGCMetaDataHelper::addFunction(MetaDataUtils& mdUtils, llvm::Function* pFunc, FunctionTypeMD type)
{
    auto finfo = FunctionInfoMetaDataHandle(new FunctionInfoMetaData());
    finfo->setType(type);
    mdUtils.setFunctionsInfoItem(pFunc, finfo);
    mdUtils.save(pFunc->getContext());
}

void IGCMetaDataHelper::moveFunction(
    MetaDataUtils& mdUtils,
    ModuleMetaData& MD,
    llvm::Function* OldFunc, llvm::Function* NewFunc)
{
    auto oldFuncIter = mdUtils.findFunctionsInfoItem(OldFunc);
    if (oldFuncIter != mdUtils.end_FunctionsInfo())
    {
        mdUtils.setFunctionsInfoItem(NewFunc, oldFuncIter->second);
        mdUtils.eraseFunctionsInfoItem(oldFuncIter);
    }

    auto& FuncMD = MD.FuncMD;
    auto loc = FuncMD.find(OldFunc);
    if (loc != FuncMD.end())
    {
        auto funcInfo = loc->second;
        FuncMD.erase(OldFunc);
        FuncMD[NewFunc] = std::move(funcInfo);
    }
}

void IGCMetaDataHelper::copyFunction(
    MetaDataUtils& mdUtils,
    ModuleMetaData& MD,
    llvm::Function* OldFunc, llvm::Function* NewFunc)
{
    auto oldFuncIter = mdUtils.findFunctionsInfoItem(OldFunc);
    if (oldFuncIter != mdUtils.end_FunctionsInfo())
    {
        mdUtils.setFunctionsInfoItem(NewFunc, oldFuncIter->second);
    }

    auto& FuncMD = MD.FuncMD;
    auto loc = FuncMD.find(OldFunc);
    if (loc != FuncMD.end())
    {
        if (FuncMD.find(NewFunc) == FuncMD.end())
        {
            // Invalidate iterator ahead of time so it doesn't happen during assignment
            FuncMD.reserve(FuncMD.size() + 1);
            loc = FuncMD.find(OldFunc);
        }
        FuncMD[NewFunc] = loc->second;
    }
}

void IGCMetaDataHelper::removeFunction(
    MetaDataUtils& mdUtils,
    ModuleMetaData& MD,
    llvm::Function* Func)
{
    auto oldFuncIter = mdUtils.findFunctionsInfoItem(Func);
    if (oldFuncIter != mdUtils.end_FunctionsInfo())
    {
        mdUtils.eraseFunctionsInfoItem(oldFuncIter);
    }

    auto& FuncMD = MD.FuncMD;
    auto loc = FuncMD.find(Func);
    if (loc != FuncMD.end())
    {
        auto funcInfo = loc->second;
        FuncMD.erase(Func);
    }
}

std::optional<std::array<uint32_t, 3>>
IGCMetaDataHelper::getThreadGroupDims(
    MetaDataUtils& mdUtils,
    llvm::Function* pKernelFunc)
{
    auto finfo = mdUtils.findFunctionsInfoItem(pKernelFunc);
    if (finfo == mdUtils.end_FunctionsInfo())
        return std::nullopt;

    auto& FI = finfo->second;

    if (!FI->getThreadGroupSize()->hasValue())
        return std::nullopt;

    auto Dims = FI->getThreadGroupSize();

    std::array<uint32_t, 3> A {
        (uint32_t)Dims->getXDim(),
        (uint32_t)Dims->getYDim(),
        (uint32_t)Dims->getZDim()
    };

    return A;
}

uint32_t IGCMetaDataHelper::getThreadGroupSize(MetaDataUtils& mdUtils, llvm::Function* pKernelFunc)
{
    auto Dims = IGCMD::IGCMetaDataHelper::getThreadGroupDims(mdUtils, pKernelFunc);
    if (!Dims)
        return 0;

    auto& Vals = *Dims;

    return Vals[0] * Vals[1] * Vals[2];
}

uint32_t IGCMetaDataHelper::getThreadGroupSizeHint(MetaDataUtils& mdUtils, llvm::Function* pKernelFunc)
{
    FunctionInfoMetaDataHandle finfo = mdUtils.getFunctionsInfoItem(pKernelFunc);
    uint32_t size = 0;
    if (finfo->getThreadGroupSizeHint()->hasValue())
    {
        size = finfo->getThreadGroupSizeHint()->getXDim() *
            finfo->getThreadGroupSizeHint()->getYDim() *
            finfo->getThreadGroupSizeHint()->getZDim();
    }
    return size;
}
