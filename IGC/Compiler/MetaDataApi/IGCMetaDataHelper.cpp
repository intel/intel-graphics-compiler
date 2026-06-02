/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "common/MDFrameWork.h"
#include <optional>

using namespace IGC;
using namespace IGC::IGCMD;

void IGCMetaDataHelper::addFunction(MetaDataUtils &mdUtils, llvm::Function *pFunc, FunctionTypeMD type) {
  auto finfo = FunctionInfoMetaDataHandle(new FunctionInfoMetaData());
  finfo->setType(type);
  mdUtils.setFunctionsInfoItem(pFunc, finfo);
  mdUtils.save(pFunc->getContext());
}

void IGCMetaDataHelper::moveFunction(MetaDataUtils &mdUtils, ModuleMetaData &MD, llvm::Function *OldFunc,
                                     llvm::Function *NewFunc) {
  auto oldFuncIter = mdUtils.findFunctionsInfoItem(OldFunc);
  if (oldFuncIter != mdUtils.end_FunctionsInfo()) {
    mdUtils.setFunctionsInfoItem(NewFunc, oldFuncIter->second);
    mdUtils.eraseFunctionsInfoItem(oldFuncIter);
  }

  auto &FuncMD = MD.FuncMD;
  auto loc = FuncMD.find(OldFunc);
  if (loc != FuncMD.end()) {
    auto funcInfo = loc->second;
    FuncMD.erase(OldFunc);
    FuncMD[NewFunc] = std::move(funcInfo);
  }
}

void IGCMetaDataHelper::copyFunction(MetaDataUtils &mdUtils, ModuleMetaData &MD, llvm::Function *OldFunc,
                                     llvm::Function *NewFunc) {
  auto oldFuncIter = mdUtils.findFunctionsInfoItem(OldFunc);
  if (oldFuncIter != mdUtils.end_FunctionsInfo()) {
    mdUtils.setFunctionsInfoItem(NewFunc, oldFuncIter->second);
  }

  auto &FuncMD = MD.FuncMD;
  auto loc = FuncMD.find(OldFunc);
  if (loc != FuncMD.end()) {
    if (FuncMD.find(NewFunc) == FuncMD.end()) {
      // Invalidate iterator ahead of time so it doesn't happen during assignment
      FuncMD.reserve(FuncMD.size() + 1);
      loc = FuncMD.find(OldFunc);
    }
    FuncMD[NewFunc] = loc->second;
  }
}

void IGCMetaDataHelper::removeFunction(MetaDataUtils &mdUtils, ModuleMetaData &MD, llvm::Function *Func) {
  auto oldFuncIter = mdUtils.findFunctionsInfoItem(Func);
  if (oldFuncIter != mdUtils.end_FunctionsInfo()) {
    mdUtils.eraseFunctionsInfoItem(oldFuncIter);
  }

  auto &FuncMD = MD.FuncMD;
  auto loc = FuncMD.find(Func);
  if (loc != FuncMD.end()) {
    FuncMD.erase(Func);
  }
}

std::optional<std::array<uint32_t, 3>> IGCMetaDataHelper::getThreadGroupDims(const ModuleMetaData *modMD,
                                                                             llvm::Function *pKernelFunc) {
  auto it = modMD->FuncMD.find(pKernelFunc);
  if (it == modMD->FuncMD.end())
    return std::nullopt;

  const auto &tgs = it->second.threadGroupSize;
  if (!isSpecified(tgs))
    return std::nullopt;

  return std::array<uint32_t, 3>{(uint32_t)tgs.dim0, (uint32_t)tgs.dim1, (uint32_t)tgs.dim2};
}

uint32_t IGCMetaDataHelper::getThreadGroupSize(const ModuleMetaData *modMD, llvm::Function *pKernelFunc) {
  auto Dims = IGCMD::IGCMetaDataHelper::getThreadGroupDims(modMD, pKernelFunc);
  if (!Dims)
    return 0;

  auto &Vals = *Dims;

  return Vals[0] * Vals[1] * Vals[2];
}

uint32_t IGCMetaDataHelper::getThreadGroupSizeHint(const ModuleMetaData *modMD, llvm::Function *pKernelFunc) {
  auto it = modMD->FuncMD.find(pKernelFunc);
  if (it == modMD->FuncMD.end())
    return 0;
  const auto &h = it->second.threadGroupSizeHint;
  return static_cast<uint32_t>(h.dim0) * static_cast<uint32_t>(h.dim1) * static_cast<uint32_t>(h.dim2);
}
