/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/CodeAssumption.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/igc_regkeys.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {
const StringRef OCLBIF_GET_GLOBAL_ID = "_Z13get_global_idj";
const StringRef OCLBIF_GET_LOCAL_ID = "_Z12get_local_idj";
const StringRef OCLBIF_GET_GROUP_ID = "_Z12get_group_idj";
} // namespace

// Register pass to igc-opt
#define PASS_FLAG "igc-codeassumption"
#define PASS_DESCRIPTION "Generate llvm.assume"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CodeAssumption, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(CodeAssumption, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char CodeAssumption::ID = 0;

bool CodeAssumption::runOnModule(Module &M) {
  m_pMDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  // Add code assist uniform analysis.
  uniformHelper(&M);

  if (IGC_GET_FLAG_VALUE(EnableCodeAssumption) > 1) {
    addAssumption(&M);
  }

  return m_changed;
}

void CodeAssumption::uniformHelper(Module *M) {
  ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

  for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I) {
    Function *F = &(*I);

    StringRef FN = F->getName();

    // sub_group_id
    if (!FN.equals("_Z25__spirv_BuiltInSubgroupIdv") && !FN.equals("__builtin_spirv_BuiltInSubgroupId") &&
        !FN.equals("_Z16get_sub_group_idv"))
      continue;
    // find all the callees
    for (auto ui = F->use_begin(), ue = F->use_end(); ui != ue; ++ui) {
      auto CI = dyn_cast<CallInst>(ui->getUser());
      if (!CI)
        continue;
      auto BB = CI->getParent();
      auto KF = BB->getParent();

      if (!IsSGIdUniform(m_pMDUtils, modMD, KF))
        continue;

      // The value must be uniform. Using shuffle with index=0 to
      // enforce it. assuming lane-0 is active
      Type *int32Ty = Type::getInt32Ty(M->getContext());
      Value *args[3];
      args[0] = CI;
      args[1] = ConstantInt::getNullValue(int32Ty);
      args[2] = ConstantInt::get(int32Ty, 0);

      Type *ITys[3] = {args[0]->getType(), int32Ty, int32Ty};
      Function *shuffleIntrin = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_WaveShuffleIndex, ITys);

      Instruction *shuffleCall = CallInst::Create(shuffleIntrin, args, "sgid", CI->getNextNode());

      shuffleCall->setDebugLoc(CI->getDebugLoc());

      CI->replaceAllUsesWith(shuffleCall);
      shuffleCall->setOperand(0, CI);

      m_changed = true;
    }
  }
}

void CodeAssumption::addAssumption(Module *M) {
  // Do it for 64-bit pointer only
  if (M->getDataLayout().getPointerSize() != 8) {
    return;
  }

  for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I) {
    Function *F = &(*I);
    StringRef FN = F->getName();
    if (FN == OCLBIF_GET_GLOBAL_ID || FN == OCLBIF_GET_LOCAL_ID || FN == OCLBIF_GET_GROUP_ID) {
      for (auto U = F->user_begin(), UE = F->user_end(); U != UE; ++U) {
        CallInst *CI = dyn_cast<CallInst>(*U);
        if (!CI || !CI->getType()->isIntegerTy()) {
          // sanity check
          continue;
        }

        BasicBlock::iterator InsertBefore(CI);
        ++InsertBefore;
        IRBuilder<> IRB(CI->getParent(), InsertBefore);

        Constant *Zero = ConstantInt::get(CI->getType(), 0);
        Value *icmp = IRB.CreateICmpSGE(CI, Zero, "assumeCond");
        (void)IRB.CreateAssumption(icmp);

        if (CI->getType()->isIntegerTy(64)) {
          // y = trunc i64 x to i32
          // Assume y is positive as well.
          for (auto UI = CI->user_begin(), UE = CI->user_end(); UI != UE; ++UI) {
            Instruction *userInst = dyn_cast<Instruction>(*UI);
            if (userInst && userInst->getOpcode() == Instruction::Trunc && userInst->getType()->isIntegerTy(32)) {
              BasicBlock::iterator pos(userInst);
              ++pos;
              IRBuilder<> builder(userInst->getParent(), pos);
              Value *tmp = builder.CreateICmpSGE(userInst, ConstantInt::get(userInst->getType(), 0), "assumeCond");
              (void)builder.CreateAssumption(tmp);
            }
          }
        }

        m_changed = true;
      }
    }
  }
}

/// APIs used directly (static functions)

// Check if a loop induction variable is always positive.
// If so, add assumption for that (LLVM value tracking does
// not handle this well, thus we will special-handle this
// case here). The pattern we check is something similar
// to the following:
//
// B0:
//    x0 = 0
//
// B1:
//    x = PHI [x0, B0] [x1, B1]
//    ...
// B1:
//    x1 = x + 1
//
// For this case, we are sure x is positive (overflow is a
// undefined behavior, and thus, do not bother overflow)!
//
bool CodeAssumption::isPositiveIndVar(PHINode *PN, const DataLayout *DL, AssumptionCache *AC) {
  auto getCxtInst = [](Value *I) -> Instruction * {
    if (PHINode *phinode = dyn_cast<PHINode>(I)) {
      // llvm.assume for a PHI is inserted right after all
      // PHI instructions in the same BB. This assumption is
      // always true no matter where the PHI is used. To make
      // this work with llvm value tracking, set Cxt instruction
      // to be the last of this BB.
      return phinode->getParent()->getTerminator();
    } else if (Instruction *Inst = dyn_cast<Instruction>(I)) {
      return Inst;
    }
    return nullptr;
  };

  int nOpnds = PN->getNumOperands();
  if (nOpnds != 2 || !PN->getType()->isIntegerTy(32)) {
    return false;
  }
  Value *NonConstVal = nullptr;
  for (int i = 0; i < nOpnds; ++i) {
    Value *aVal = PN->getOperand(i);
    ConstantInt *IConst = dyn_cast<ConstantInt>(aVal);
    if ((IConst && IConst->getSExtValue() >= 0) || (!IConst && valueIsPositive(aVal, DL, AC, getCxtInst(aVal)))) {
      continue;
    }
    if (NonConstVal) {
      return false;
    }
    NonConstVal = aVal;
  }
  if (!NonConstVal) {
    return true;
  }
  Instruction *Inst = dyn_cast<Instruction>(NonConstVal);
  if (!Inst || Inst->getOpcode() != Instruction::Add) {
    return false;
  }
  ConstantInt *IC = nullptr;
  if (Inst->getOperand(0) == PN) {
    IC = dyn_cast<ConstantInt>(Inst->getOperand(1));
  } else if (Inst->getOperand(1) == PN) {
    IC = dyn_cast<ConstantInt>(Inst->getOperand(0));
  }
  if (IC && IC->getSExtValue() >= 0) {
    return true;
  }
  return false;
}

bool CodeAssumption::addAssumption(Function *F, AssumptionCache *AC) {
  const DataLayout &DL = F->getParent()->getDataLayout();
  DenseMap<Value *, int> assumptionAdded;

  bool assumeAdded = false;
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto BI = F->begin(), BE = F->end(); BI != BE; ++BI) {
      BasicBlock *BB = &(*BI);
      for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II) {
        Instruction *Inst = &(*II);
        PHINode *PN = dyn_cast<PHINode>(Inst);
        if (!PN)
          break;
        if (assumptionAdded.count(PN) == 0 && CodeAssumption::isPositiveIndVar(PN, &DL, AC)) {
          IRBuilder<> IRB(BB->getFirstNonPHI());
          Constant *Zero = ConstantInt::get(PN->getType(), 0);
          Value *icmp = IRB.CreateICmpSGE(PN, Zero, "assumeCond");
          CallInst *assumeInst = IRB.CreateAssumption(icmp);

          // Register assumption
          if (AC) {
            AC->registerAssumption(
                dyn_cast<AssumeInst>(assumeInst)
            );
          }

          assumptionAdded[PN] = 1;
          changed = true;
          assumeAdded = true;
        }
      }
    }
  }
  return assumeAdded;
}

// Return true if SubGroupID is uniform
bool CodeAssumption::IsSGIdUniform(MetaDataUtils *pMDU, ModuleMetaData *modMD, Function *F) {
  if (!isEntryFunc(pMDU, F)) {
    return false;
  }

  FunctionInfoMetaDataHandle funcInfoMD = pMDU->getFunctionsInfoItem(F);
  ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();

  // WO (Walk Order): it is a triple (d0, d1, d2), where each d0/d1/d2 are 0|1|2.
  // This WO indicates that the work-items are dispatched along d0 first, then d1,
  // at last d2. For example, given work group size (8, 2, 1). With WO(0,1,2),
  // the work-items are dispatched in the linear order like the following:
  // (note that each triple is local id triple, assuming SIMD8)
  //   1st thread of simd8: (0, 0, 0) (1, 0, 0), (2, 0, 0), ......, (7, 0, 0)
  //   2nd thread of simd8: (0, 1, 0) (1, 1, 0), (2, 1, 0), ......, (7, 1, 0)
  // With WO(1, 0, 2), work-items are dispatched like:
  //   1st thread of simd8: (0, 0, 0) (0, 1, 0), (1, 0, 0), (1, 1, 0), ......, (3, 1, 0),
  //   2nd thread of simd8: (4, 0, 0) (4, 1, 0), (5, 0, 0), (5, 1, 0), ......, (7, 1, 0)
  //
  int32_t WO_0 = -1, WO_1 = -1, WO_2 = -1;

  auto funcMD = modMD->FuncMD.find(F);
  if (funcMD != modMD->FuncMD.end()) {
    WorkGroupWalkOrderMD workGroupWalkOrder = funcMD->second.workGroupWalkOrder;

    if (workGroupWalkOrder.dim0 || workGroupWalkOrder.dim1 || workGroupWalkOrder.dim2) {
      WO_0 = workGroupWalkOrder.dim0;
      WO_1 = workGroupWalkOrder.dim1;
      WO_2 = workGroupWalkOrder.dim2;

      if (WO_0 == 0 && WO_1 == 1 && WO_2 == 2) {
        // order (0, 1, 2): linear order
        return true;
      }
    }
  }

  if (threadGroupSize->hasValue()) {
    SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
    if (subGroupSize->hasValue()) {
      uint32_t simdSize = (uint32_t)subGroupSize->getSIMDSize();

      uint32_t X = (uint32_t)threadGroupSize->getXDim();
      uint32_t Y = (uint32_t)threadGroupSize->getYDim();
      uint32_t Z = (uint32_t)threadGroupSize->getZDim();

      bool hasWO = (WO_0 >= 0); // WO_1 and WO_2 >=0
      if ((X * Y * Z) <= simdSize) {
        // WG has only 1 thread.
        return true;
      } else if (hasWO && ((Y == 1 && Z == 1) || (X == 1 && Z == 1) || (X == 1 && Y == 1))) {
        return true;
      }
    }
  }
  return false;
}
