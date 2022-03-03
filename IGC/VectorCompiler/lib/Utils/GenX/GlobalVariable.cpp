/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/GlobalVariable.h"
#include "vc/Utils/GenX/PredefinedVariable.h"
#include "vc/Utils/GenX/Printf.h"

#include <llvm/GenXIntrinsics/GenXMetadata.h>

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>

using namespace llvm;

bool vc::isRealGlobalVariable(const GlobalVariable &GV) {
  if (GV.hasAttribute(genx::FunctionMD::GenXVolatile))
    return false;
  if (vc::PredefVar::isPV(GV))
    return false;
  bool IsIndexedString =
      std::any_of(GV.user_begin(), GV.user_end(), [](const User *Usr) {
        return vc::isLegalPrintFormatIndexGEP(*Usr);
      });
  if (IsIndexedString) {
    IGC_ASSERT_MESSAGE(std::all_of(GV.user_begin(), GV.user_end(),
                                   [](const User *Usr) {
                                     return vc::isLegalPrintFormatIndexGEP(
                                         *Usr);
                                   }),
                       "when global is an indexed string, its users can only "
                       "be print format index GEPs");
    return false;
  }
  return true;
}

const GlobalVariable *vc::getUnderlyingGlobalVariable(const Value *V) {
  while (auto *BI = dyn_cast<BitCastInst>(V))
    V = BI->getOperand(0);
  while (auto *CE = dyn_cast_or_null<ConstantExpr>(V)) {
    if (CE->getOpcode() == CastInst::BitCast)
      V = CE->getOperand(0);
    else
      break;
  }
  return dyn_cast_or_null<GlobalVariable>(V);
}

GlobalVariable *vc::getUnderlyingGlobalVariable(Value *V) {
  return const_cast<GlobalVariable *>(
      getUnderlyingGlobalVariable(const_cast<const Value *>(V)));
}

const GlobalVariable *vc::getUnderlyingGlobalVariable(const LoadInst *LI) {
  return getUnderlyingGlobalVariable(LI->getPointerOperand());
}

GlobalVariable *vc::getUnderlyingGlobalVariable(LoadInst *LI) {
  return getUnderlyingGlobalVariable(LI->getPointerOperand());
}
