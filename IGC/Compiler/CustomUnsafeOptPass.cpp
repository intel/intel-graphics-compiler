/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== CustomUnsafeOptPass.cpp ==========================

 This file contains CustomUnsafeOptPass and EarlyOutPatterns

 CustomUnsafeOptPass does peephole optimizations which might affect precision.
 This pass combines things like:
    x * 0 = 0               or
    y + x - x = y           or
    fdiv = fmul + inv       or
    fmul+fsub+fcmp = fcmp (if condition allowed)


 EarlyOutPatterns does a few early out cases that adds control flow to
 avoid heavy computation that is not needed.
 For example, if a long/expensive sequence of instruction result is 0 when
 one of the input r0 is 0, we can modify the sequence to
     if(r0==0)
     {
        result = 0;
     }
     else
     {
        do the actual calculation
     }
 The cases are added with existing workload analysis and should be limited to
 the target shader since adding control flow can add overhead to other shaders.

=============================================================================*/

#include "Compiler/CustomUnsafeOptPass.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/Statistic.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include "llvm/IR/DebugInfo.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/IntrinsicInst.h"
#include "llvmWrapper/IR/Function.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-custom-unsafe-opt-pass"
#define PASS_DESCRIPTION "Unsafe Optimizations Pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CustomUnsafeOptPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper);
IGC_INITIALIZE_PASS_END(CustomUnsafeOptPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char CustomUnsafeOptPass::ID = 0;

#define DEBUG_TYPE "CustomUnSafeOptPass"

STATISTIC(Stat_FcmpRemoved, "Number of insts removed in FCmp Opt");
STATISTIC(Stat_FloatRemoved, "Number of insts removed in Float Opt");

static bool allowUnsafeMathOpt(CodeGenContext *ctx) {
  // check compiler options in metadata
  if ((!ctx->m_checkFastFlagPerInstructionInCustomUnsafeOptPass && ctx->getModuleMetaData()->compOpt.FastRelaxedMath) ||
      ctx->getModuleMetaData()->compOpt.UnsafeMathOptimizations) {
    return true;
  }

  if (IGC_IS_FLAG_ENABLED(EnableFastMath)) {
    return true;
  }

  return false;
}

static bool allowUnsafeMathOpt(CodeGenContext *ctx, llvm::BinaryOperator &op) {
  // always allow unsafe opt if instruction has the flag
  if (llvm::isa<llvm::FPMathOperator>(op) && op.getFastMathFlags().isFast()) {
    return true;
  }

  // then check compiler options in metadata
  return allowUnsafeMathOpt(ctx);
}

CustomUnsafeOptPass::CustomUnsafeOptPass()
    : FunctionPass(ID), m_disableReorderingOpt(0), m_ctx(nullptr), m_pMdUtils(nullptr) {
  initializeCustomUnsafeOptPassPass(*PassRegistry::getPassRegistry());
}

bool CustomUnsafeOptPass::runOnFunction(Function &F) {
  if (getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->compOpt.disableCustomUnsafeOpts ||
      IGC_IS_FLAG_ENABLED(DisableCustomUnsafeOpt)) {
    return false;
  }

  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

  m_disableReorderingOpt = false;

  if (m_ctx->type == ShaderType::VERTEX_SHADER) {
    m_disableReorderingOpt = true;
  }
  if (m_ctx->type == ShaderType::COMPUTE_SHADER &&
      m_ctx->getModuleMetaData()->compOpt.FloatDenormMode64 == FLOAT_DENORM_RETAIN) {
    m_disableReorderingOpt = true;
  }

  int iterCount = 0;

  m_isChanged = true;
  // re-run the pass if the shader is changed within the pass.
  // also set a iterCount<=10 to make sure it doesn't run into infinate loop unexpectedly.
  while (m_isChanged && iterCount <= 10) {
    iterCount++;
    m_isChanged = false;
    visit(F);
  }

  // Do reassociate to emit more mad.
  // Skip this optimization for domain shaders due to precision issues observed in those shaders specifically
  if (m_ctx->type != ShaderType::DOMAIN_SHADER) {
    reassociateMulAdd(F);
  }

  eraseCollectedInst();

  return true;
}

void CustomUnsafeOptPass::visitInstruction(Instruction &I) {
  // nothing
}

void CustomUnsafeOptPass::visitPHINode(llvm::PHINode &I) {
  /*
  From
      %zero = phi float [ -0.000000e+00, %if.then ], [ 0.000000e+00, %for.body ]
      %mul = fmul fast float %zero, %smth
  To
      %mul = fmul fast float 0.0, %smth
  */
  if (I.getType()->isFloatingPointTy()) {
    // allow transformation if all PHI users can ignore the sign of zero
    bool isNoSignedZeros = llvm::all_of(I.users(), [](const auto &U) {
      auto *Op = dyn_cast<FPMathOperator>(U);
      return Op && Op->hasNoSignedZeros();
    });

    if (isNoSignedZeros || allowUnsafeMathOpt(m_ctx)) {
      bool isZeroFP = llvm::all_of(I.incoming_values(), [](Use &use) {
        return isa<ConstantFP>(use.get()) && cast<ConstantFP>(use.get())->isZero();
      });

      if (isZeroFP) {
        I.replaceAllUsesWith(ConstantFP::get(I.getType(), 0.0));
        I.eraseFromParent();
        m_isChanged = true;
      }
    }
  }
}

void CustomUnsafeOptPass::visitFPToSIInst(llvm::FPToSIInst &I) {
  /*
  For cases like this, %141 doesn't need rounding because the only possible values are 0.0, 1.0, 2.0, 3.0, and 4.0.
  We can skip %142 and %143

  % 132 = select i1 % 131, float 1.000000e+00, float 0.000000e+00, !dbg !328
  % 134 = select i1 % 133, float 1.000000e+00, float 0.000000e+00, !dbg !328
  % 135 = fadd fast float %134, % 132, !dbg !328
  % 137 = select i1 % 136, float 1.000000e+00, float 0.000000e+00, !dbg !328
  % 138 = fadd fast float %137, % 135, !dbg !328
  % 140 = select i1 % 139, float 1.000000e+00, float 0.000000e+00, !dbg !328
  % 141 = fadd fast float %140, % 138, !dbg !328

  % 142 = fadd fast float %141, 5.000000e-01, !dbg !329
  % 143 = call fast float @llvm.floor.f32(float %142), !dbg !330
  % 144 = fptosi float %143 to i32, !dbg !331
  */
  if (CallInst *floorInst = dyn_cast<CallInst>((&I)->getOperand(0))) {
    if (GetOpCode(floorInst) == llvm_floor) {
      if (BinaryOperator *faddInst = dyn_cast<BinaryOperator>(floorInst->getOperand(0))) {
        if (faddInst->getOpcode() == Instruction::FAdd) {
          ConstantFP *C0_5 = dyn_cast<ConstantFP>(faddInst->getOperand(1));
          if (C0_5 && C0_5->isExactlyValue(0.5)) {
            // check all the sources are from 1.0 and 0.0
            bool allowOpt = true;
            SmallVector<Instruction *, 8> InstList;
            InstList.clear();
            InstList.push_back(cast<Instruction>(faddInst->getOperand(0)));
            while (!InstList.empty() && allowOpt) {
              Instruction *inst = InstList.back();
              InstList.pop_back();
              if (inst->getOpcode() == Instruction::FAdd) {
                Instruction *addSrc0 = dyn_cast<Instruction>(inst->getOperand(0));
                Instruction *addSrc1 = dyn_cast<Instruction>(inst->getOperand(1));
                if (addSrc0 && addSrc1) {
                  InstList.push_back(addSrc0);
                  InstList.push_back(addSrc1);
                } else {
                  allowOpt = false;
                }
              } else if (dyn_cast<SelectInst>(inst)) {
                ConstantFP *c1 = dyn_cast<ConstantFP>(inst->getOperand(1));
                ConstantFP *c2 = dyn_cast<ConstantFP>(inst->getOperand(2));
                if (!c1 || !c2 || (!c1->isZeroValue() && !c1->isExactlyValue(1.0f)) ||
                    (!c2->isZeroValue() && !c2->isExactlyValue(1.0f))) {
                  allowOpt = false;
                }
              } else {
                allowOpt = false;
              }
              if (InstList.size() > 8) {
                allowOpt = false;
              }
            }
            if (allowOpt) {
              floorInst->replaceAllUsesWith(faddInst->getOperand(0));
              collectForErase(*floorInst, 1);
            }
            InstList.clear();
          }
        }
      }
    }
  }
}

bool CustomUnsafeOptPass::possibleForFmadOpt(llvm::Instruction *inst) {
  if (inst->getOpcode() == Instruction::FAdd) {
    if (BinaryOperator *src0 = llvm::dyn_cast<llvm::BinaryOperator>(inst->getOperand(0))) {
      if (src0->getOpcode() == Instruction::FMul) {
        return true;
      }
    }
    if (BinaryOperator *src1 = llvm::dyn_cast<llvm::BinaryOperator>(inst->getOperand(1))) {
      if (src1->getOpcode() == Instruction::FMul) {
        return true;
      }
    }
  }
  return false;
}

bool CustomUnsafeOptPass::visitBinaryOperatorFmulFaddPropagation(BinaryOperator &I) {
  if (m_disableReorderingOpt) {
    return false;
  }

  /*
      Pattern 1:
          From
              %r2.x_1 = fmul float %r2.x_, %r1.y_, !dbg !10
              %r2.y_2 = fmul float %r2.y_, %r1.y_, !dbg !10    -> Src1
              %r2.z_3 = fmul float %r2.z_, %r1.y_, !dbg !10

              %oC0.x_ = fmul float %r2.x_1, 0x3FE3333340000000, !dbg !12
              %oC0.y_ = fmul float %r2.y_2, 0x3FE3333340000000, !dbg !12  -> Base
              %oC0.z_ = fmul float %r2.z_3, 0x3FE3333340000000, !dbg !12

          To
              %r2.x_1 = fmul float 0x3FE3333340000000, %r1.y_, !dbg !10
              %oC0.x_ = fmul float %r2.x_1, %r2.x_, !dbg !12
              %oC0.y_ = fmul float %r2.x_1, %r2.y_, !dbg !12
              %oC0.z_ = fmul float %r2.x_1, %r2.z_, !dbg !12

       Pattern 2:
          From
              %r0.x_ = fmul float %r1.x_, %r2.z_, !dbg !10
              %r0.y_ = fmul float %r1.y_, %r2.z_, !dbg !10                -> Src1
              %r0.z_ = fmul float %r1.z_, %r2.z_, !dbg !10
              %r2.x_1 = fmul float %r2.x_, 0x3FE6666660000000, !dbg !12
              %r2.y_2 = fmul float %r2.y_, 0x3FE6666660000000, !dbg !12   -> Src2
              %r2.z_3 = fmul float %r2.z_, 0x3FE6666660000000, !dbg !12
              %oC0.x_ = fmul float %r0.x_, %r2.x_1, !dbg !14
              %oC0.y_ = fmul float %r0.y_, %r2.y_2, !dbg !14              -> Base
              %oC0.z_ = fmul float %r0.z_, %r2.z_3, !dbg !14

          To
              %r0.x_ = fmul float 0x3FE6666660000000, %r2.z_, !dbg !10
              %r2.x_1 = fmul float %r2.x_, %r0.x_, !dbg !12
              %r2.y_2 = fmul float %r2.y_, %r0.x_, !dbg !12
              %r2.z_3 = fmul float %r2.z_, %r0.x_, !dbg !12
              %oC0.x_ = fmul float %r1.x_, %r2.x_1, !dbg !14
              %oC0.y_ = fmul float %r1.y_, %r2.y_2, !dbg !14
              %oC0.z_ = fmul float %r1.z_, %r2.z_3, !dbg !14
  */
  llvm::Instruction::BinaryOps opcode = I.getOpcode();

  // only fmul or fadd can call into this function
  IGC_ASSERT(opcode == Instruction::FMul || opcode == Instruction::FAdd);

  llvm::Instruction *instBase[4];
  llvm::Instruction *instSrc1[4];
  llvm::Instruction *instSrc2[4];
  int sameSrcIdBase = 0;
  int sameSrcId1 = 0;
  int sameSrcId2 = 0;
  int numOfSet = 0;
  bool matchPattern1 = false;

  for (int i = 0; i < 4; i++) {
    instBase[i] = NULL;
    instSrc1[i] = NULL;
    instSrc2[i] = NULL;
  }

  instBase[0] = llvm::dyn_cast<llvm::Instruction>(&I);
  if (instBase[0] == nullptr || instBase[0]->getOperand(0) == instBase[0]->getOperand(1)) {
    return false;
  }

  instBase[1] = instBase[0]->getNextNonDebugInstruction();

  if (instBase[1] && instBase[1]->getOpcode() != opcode) {
    instBase[1] = instBase[1]->getNextNonDebugInstruction();
  }

  if (instBase[1] == nullptr || instBase[1]->getOpcode() != opcode ||
      instBase[1]->getOperand(0) == instBase[1]->getOperand(1)) {
    return false;
  }

  if (instBase[0]->getOperand(0) == instBase[1]->getOperand(0)) {
    sameSrcIdBase = 0;
    numOfSet = 2;
    matchPattern1 = true;
  } else if (instBase[0]->getOperand(1) == instBase[1]->getOperand(1)) {
    sameSrcIdBase = 1;
    numOfSet = 2;
    matchPattern1 = true;
  }
  for (int i = 2; i < 4; i++) {
    instBase[i] = instBase[i - 1]->getNextNonDebugInstruction();

    if (instBase[i] && instBase[i - 1]->getOpcode() != instBase[i]->getOpcode()) {
      instBase[i] = instBase[i]->getNextNonDebugInstruction();
    }

    if (!instBase[i] || instBase[i]->getOpcode() != opcode ||
        instBase[i]->getOperand(0) == instBase[i]->getOperand(1) || possibleForFmadOpt(instBase[i])) {
      break;
    }
    numOfSet = i + 1;
  }

  if (numOfSet < 2) {
    return false;
  }

  if (matchPattern1) {
    for (int i = 0; i < numOfSet; i++) {
      if (i > 0 && instBase[i]->getOperand(sameSrcIdBase) != instBase[0]->getOperand(sameSrcIdBase)) {
        numOfSet = i;
        break;
      }

      instSrc1[i] = llvm::dyn_cast<llvm::Instruction>(instBase[i]->getOperand(1 - sameSrcIdBase));

      if (!instSrc1[i] || !instSrc1[i]->hasOneUse() || instSrc1[i]->getOpcode() != opcode ||
          possibleForFmadOpt(instSrc1[i])) {
        numOfSet = i;
        break;
      }
    }

    if (numOfSet > 1) {
      if (instSrc1[0]->getOperand(0) == instSrc1[1]->getOperand(0)) {
        sameSrcId1 = 0;
      } else if (instSrc1[0]->getOperand(1) == instSrc1[1]->getOperand(1)) {
        sameSrcId1 = 1;
      } else {
        return false;
      }

      // instructions for the pattern can not overlap with each other
      for (int si = 0; si < numOfSet; si++) {
        for (int sj = 0; sj < numOfSet; sj++) {
          if (instBase[si] == instSrc1[sj]) {
            return false;
          }
        }
      }

      for (int i = 2; i < numOfSet; i++) {
        if (instSrc1[i]->getOperand(sameSrcId1) != instSrc1[0]->getOperand(sameSrcId1)) {
          numOfSet = i;
          break;
        }
      }
    }

    if (numOfSet > 1 && opcode == Instruction::FMul) {
      if (!dyn_cast<ConstantFP>(instSrc1[0]->getOperand(sameSrcId1))) {
        if (llvm::Instruction *tempInstr = llvm::dyn_cast<llvm::Instruction>(instSrc1[0]->getOperand(sameSrcId1))) {
          if (tempInstr->getOpcode() == Instruction::FDiv) {
            ConstantFP *C0 = dyn_cast<ConstantFP>(tempInstr->getOperand(0));
            if (C0 && C0->isExactlyValue(1.0)) {
              numOfSet = 0;
            }
          }

          IGC::EOPCODE intrinsic_name = IGC::GetOpCode(tempInstr);
          if (intrinsic_name == IGC::llvm_exp) {
            numOfSet = 0;
          }
        }
      }
    }

    // start the optimization for pattern 1
    if (numOfSet > 1) {
      Value *tempOp = instBase[0]->getOperand(sameSrcIdBase);
      for (int i = 0; i < numOfSet; i++) {
        instBase[i]->setOperand(1 - sameSrcIdBase, instBase[0]->getOperand(1 - sameSrcIdBase));
      }
      for (int i = 0; i < numOfSet; i++) {
        instBase[i]->setOperand(sameSrcIdBase, instSrc1[i]->getOperand(1 - sameSrcId1));
      }
      instSrc1[0]->setOperand(1 - sameSrcId1, tempOp);
      // move instSrc1[0] to before base
      instSrc1[0]->moveBefore(instBase[0]);

      for (int i = 1; i < numOfSet; ++i) {
        if (instSrc1[i]->use_empty())
          collectForErase(*instSrc1[i]);
      }

      return true;
    }
  } else // check pattern 2
  {
    for (int i = 0; i < numOfSet; i++) {
      if (instBase[i]->getOperand(0) == instBase[i]->getOperand(1) ||
          dyn_cast<ConstantFP>(instBase[i]->getOperand(0)) || dyn_cast<ConstantFP>(instBase[i]->getOperand(1)) ||
          possibleForFmadOpt(instBase[i])) {
        numOfSet = i;
        break;
      }
    }

    if (numOfSet > 1) {
      for (int i = 0; i < numOfSet; i++) {
        instSrc1[i] = llvm::dyn_cast<llvm::Instruction>(instBase[i]->getOperand(0));
        instSrc2[i] = llvm::dyn_cast<llvm::Instruction>(instBase[i]->getOperand(1));
        if (!instSrc1[i] || !instSrc1[i]->hasOneUse() || instSrc1[i]->getOpcode() != opcode || !instSrc2[i] ||
            !instSrc2[i]->hasOneUse() || instSrc2[i]->getOpcode() != opcode || possibleForFmadOpt(instSrc1[i]) ||
            possibleForFmadOpt(instSrc2[i])) {
          numOfSet = i;
          break;
        }
      }
    }

    if (numOfSet > 1) {
      if (instSrc1[0]->getOperand(0) == instSrc1[1]->getOperand(0)) {
        sameSrcId1 = 0;
      } else if (instSrc1[0]->getOperand(1) == instSrc1[1]->getOperand(1)) {
        sameSrcId1 = 1;
      } else {
        return false;
      }

      if (instSrc2[0]->getOperand(0) == instSrc2[1]->getOperand(0)) {
        sameSrcId2 = 0;
      } else if (instSrc2[0]->getOperand(1) == instSrc2[1]->getOperand(1)) {
        sameSrcId2 = 1;
      } else {
        return false;
      }

      // instructions for the pattern can not overlap with each other
      for (int si = 0; si < numOfSet; si++) {
        for (int sj = 0; sj < numOfSet; sj++) {
          for (int sk = 0; sk < numOfSet; sk++) {
            if (instBase[si] == instSrc1[sj] || instBase[si] == instSrc2[sk] || instSrc1[sj] == instSrc2[sk]) {
              return false;
            }
          }
        }
      }

      for (int i = 2; i < numOfSet; i++) {
        if (instSrc1[0]->getOperand(sameSrcId1) != instSrc1[i]->getOperand(sameSrcId1) ||
            instSrc2[0]->getOperand(sameSrcId2) != instSrc2[i]->getOperand(sameSrcId2)) {
          numOfSet = i;
          break;
        }
      }
    }

    // start the optimization for pattern 2
    if (numOfSet > 1) {
      Value *tempOp = instSrc2[0]->getOperand(sameSrcId2);

      for (int i = 0; i < numOfSet; i++) {
        instSrc2[i]->setOperand(sameSrcId2, instBase[0]->getOperand(0));
      }

      for (int i = 0; i < numOfSet; i++) {
        instBase[i]->setOperand(0, instSrc1[i]->getOperand(1 - sameSrcId1));
      }

      instSrc1[0]->setOperand(1 - sameSrcId1, tempOp);

      for (int i = 0; i < numOfSet; i++) {
        instSrc2[i]->moveBefore(instBase[0]);
      }
      instSrc1[0]->moveBefore(instSrc2[0]);

      for (int i = 1; i < numOfSet; ++i) {
        if (instSrc1[i]->use_empty())
          collectForErase(*instSrc1[i]);
      }

      return true;
    }
  }
  return false;
}

bool CustomUnsafeOptPass::removeCommonMultiplier(llvm::Value *I, llvm::Value *commonMultiplier) {
  Value *numerator = NULL;
  Value *denumerator = NULL;
  if (isFDiv(I, numerator, denumerator)) {
    llvm::Instruction *multiplier = llvm::dyn_cast<llvm::Instruction>(numerator);
    if (multiplier && multiplier->getOpcode() == Instruction::FMul && multiplier->getOperand(1) == commonMultiplier) {
      multiplier->setOperand(1, ConstantFP::get(multiplier->getType(), 1.0));
      return true;
    }
  }
  return false;
}

bool CustomUnsafeOptPass::visitBinaryOperatorExtractCommonMultiplier(BinaryOperator &I) {
  bool patternFound = false;

  if (m_disableReorderingOpt || !I.hasOneUse()) {
    return patternFound;
  }

  llvm::Instruction *src0 = llvm::dyn_cast<llvm::Instruction>(I.getOperand(0));

  Value *numerator0 = NULL;
  Value *denumerator0 = NULL;

  if (src0 && src0->hasOneUse() && isFDiv(src0, numerator0, denumerator0)) {
    if (llvm::Instruction *inst = llvm::dyn_cast<llvm::Instruction>(numerator0)) {
      if (inst->getOpcode() == Instruction::FMul) {
        if (llvm::Instruction *commonMultiplier = llvm::dyn_cast<llvm::Instruction>(inst->getOperand(1))) {
          llvm::Instruction *sumComponent = llvm::dyn_cast<llvm::Instruction>(I.getOperand(1));
          llvm::Instruction *currentRoot = &I;
          llvm::Instruction *previousRoot = nullptr;
          while (1) {
            if (sumComponent && sumComponent->hasOneUse()) {
              previousRoot = currentRoot;
              if (removeCommonMultiplier(sumComponent, commonMultiplier)) {
                currentRoot = llvm::cast<Instruction>(*currentRoot->user_begin());
                sumComponent = llvm::dyn_cast<llvm::Instruction>(currentRoot->getOperand(1));
                patternFound = true;
              } else {
                break;
              }
            } else {
              break;
            }
          }
          if (patternFound) {
            Instruction *newResult = copyIRFlags(BinaryOperator::CreateFMul(commonMultiplier, previousRoot, ""), &I);
            newResult->insertAfter(previousRoot);
            previousRoot->replaceAllUsesWith(newResult);
            newResult->setOperand(1, previousRoot);
            inst->setOperand(1, ConstantFP::get(inst->getType(), 1.0));
          }
        }
      }
    }
  }

  return patternFound;
}

// Searches for negation via xor instruction and replaces with subtraction.
//
// Example for float:
//   %22 = bitcast float %a to i32
//   %23 = xor i32 %22, -2147483648
//   %24 = bitcast i32 %23 to float
//
// Will be changed to:
//   %22 = fsub float 0.000000e+00, %a
bool CustomUnsafeOptPass::visitBinaryOperatorXor(llvm::BinaryOperator &I) {
  using namespace llvm::PatternMatch;

  if (!I.hasOneUse()) {
    return false;
  }

  Value *fpValue = nullptr;
  ConstantInt *mask = nullptr;
  if (!match(&I, m_Xor(m_OneUse(m_BitCast(m_Value(fpValue))), m_ConstantInt(mask)))) {
    return false;
  }

  // bitcast before xor, from fp to int
  uint32_t fpBits = fpValue->getType()->getScalarSizeInBits();
  BitCastInst *castF2I = cast<BitCastInst>(I.getOperand(0));
  if (!fpValue->getType()->isFloatingPointTy() || !castF2I->getType()->isIntegerTy(fpBits)) {
    return false;
  }

  // bitcast after xor, back to fp
  BitCastInst *castI2F = dyn_cast<BitCastInst>(*I.user_begin());
  if (!castI2F || castI2F->getType() != fpValue->getType()) {
    return false;
  }

  // mask replaces only one bit representing sign
  uint64_t expectedMask = 1ll << (fpBits - 1);
  if (!mask || mask->getBitWidth() != fpBits || mask->getZExtValue() != expectedMask) {
    return false;
  }

  IRBuilder<> builder(&I);
  auto fsub = builder.CreateFSub(ConstantFP::get(fpValue->getType(), 0.0f), fpValue);

  castI2F->replaceAllUsesWith(fsub);

  collectForErase(*castI2F, 2);

  return true;
}

bool CustomUnsafeOptPass::visitBinaryOperatorToFmad(BinaryOperator &I) {
  if (m_disableReorderingOpt) {
    return false;
  }

  /*
  // take care of the case: C1*(a + C0) = a*C1 + C0*C1
  from
      %38 = fadd float %30, 0x3FAC28F5C0000000
      %39 = fmul float %38, 0x3FEE54EDE0000000
  to
      %38 = fmul float %33, 0x3FEE54EDE0000000
      %39 = fadd float %38, 0x3FAAB12340000000

  fmul+fadd can be replaced with fmad later in matchMad()
  */

  IGC_ASSERT(I.getOpcode() == Instruction::FMul);

  Instruction *mulInst = (Instruction *)(&I);
  Instruction *addInst = dyn_cast<Instruction>(I.getOperand(0));

  if (!addInst || (addInst->getOpcode() != Instruction::FAdd && addInst->getOpcode() != Instruction::FSub)) {
    return false;
  }

  ConstantFP *C0 = dyn_cast<ConstantFP>(addInst->getOperand(1));
  ConstantFP *C1 = dyn_cast<ConstantFP>(mulInst->getOperand(1));

  if (!C0 || !C1 || !addInst->hasOneUse()) {
    return false;
  }

  // If C0/C1 is INF then return false, to avoid the following case for example
  //      INF*(a-1)
  //      Before transformation the above expression would result in INF, but after transformation it will be 0
  if (C0->isInfinity() || C1->isInfinity()) {
    return false;
  }

  Value *op0 = copyIRFlags(BinaryOperator::CreateFMul(addInst->getOperand(0), C1, "", &I), &I);

  APFloat C0Float = C0->getValueAPF();
  C0Float.multiply(C1->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
  Value *op1 = ConstantFP::get(C0->getContext(), C0Float);

  if (addInst->getOpcode() == Instruction::FAdd) {
    I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFAdd(op0, op1, "", &I), &I));
  } else {
    I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFSub(op0, op1, "", &I), &I));
  }

  collectForErase(I, 1);

  m_isChanged = true;

  return true;
}

bool CustomUnsafeOptPass::visitBinaryOperatorFmulToFmad(BinaryOperator &I) {
  bool patternFound = false;

  if (m_disableReorderingOpt || (m_ctx->type == ShaderType::PIXEL_SHADER) ||
      (m_ctx->type == ShaderType::COMPUTE_SHADER)) {
    return patternFound;
  }

  /*
  // take care of the case: x*(1 - a) = x - x*a
  // needed for a OGL lrp pattern match, also enable more cases for the fmad optimization later

  from
      %6 = fsub float 1.000000e+00, %res_s1
      %7 = fmul float %res_s2, %6
  to
      %6 = fmul float %res_s2, %res_s1
      %7 = fsub float %res_s2, %6
  */

  IGC_ASSERT(I.getOpcode() == Instruction::FMul);

  // check for x*(1 +/- a), (1 +/- a)*x, x*(a +/- 1), (a +/- 1)*x
  // also checks if x is a constant. 1 can be any other constant in this case.
  bool enableOpt = false;
  uint xIndex = 0;
  uint immOneIndex = 0;
  llvm::Instruction *FsubOrFaddInst = nullptr;
  for (xIndex = 0; xIndex < 2; xIndex++) {
    FsubOrFaddInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(1 - xIndex));
    if (FsubOrFaddInst && FsubOrFaddInst->hasOneUse() &&
        (FsubOrFaddInst->getOpcode() == Instruction::FSub || FsubOrFaddInst->getOpcode() == Instruction::FAdd)) {
      ConstantFP *Cx = dyn_cast<ConstantFP>(I.getOperand(xIndex));
      ConstantFP *C0 = dyn_cast<ConstantFP>(FsubOrFaddInst->getOperand(0));
      if (C0 && !C0->isZero() && (C0->isExactlyValue(1.f) || Cx)) {
        enableOpt = true;
        immOneIndex = 0;
        break;
      }
      ConstantFP *C1 = dyn_cast<ConstantFP>(FsubOrFaddInst->getOperand(1));
      if (C1 && !C1->isZero() && (C1->isExactlyValue(1.f) || Cx)) {
        enableOpt = true;
        immOneIndex = 1;
        break;
      }
    }
  }

  // start optimization
  if (enableOpt) {
    Value *op1 = nullptr;
    Value *op2 = nullptr;
    ConstantFP *Cx = dyn_cast<ConstantFP>(I.getOperand(xIndex));

    if (immOneIndex == 0) {
      if (Cx) {
        ConstantFP *C0 = dyn_cast<ConstantFP>(FsubOrFaddInst->getOperand(0));
        APFloat CxFloat = Cx->getValueAPF();
        CxFloat.multiply(C0->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        op1 = ConstantFP::get(C0->getContext(), CxFloat);
      } else {
        op1 = I.getOperand(xIndex);
      }
      op2 = copyIRFlags(BinaryOperator::CreateFMul(I.getOperand(xIndex), FsubOrFaddInst->getOperand(1), "", &I), &I);
    } else {
      op1 = copyIRFlags(BinaryOperator::CreateFMul(I.getOperand(xIndex), FsubOrFaddInst->getOperand(0), "", &I), &I);
      if (Cx) {
        ConstantFP *C1 = dyn_cast<ConstantFP>(FsubOrFaddInst->getOperand(1));
        APFloat CxFloat = Cx->getValueAPF();
        CxFloat.multiply(C1->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        op2 = ConstantFP::get(C1->getContext(), CxFloat);
      } else {
        op2 = I.getOperand(xIndex);
      }
    }

    if (FsubOrFaddInst->getOpcode() == Instruction::FSub) {
      I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFSub(op1, op2, "", &I), &I));
    } else {
      I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFAdd(op1, op2, "", &I), &I));
    }

    collectForErase(I, 1);

    m_isChanged = true;
    patternFound = true;
  }
  return patternFound;
}

bool CustomUnsafeOptPass::isFDiv(Value *I, Value *&numerator, Value *&denominator) {
  bool result = false;
  if (llvm::Instruction *div = llvm::dyn_cast<llvm::Instruction>(I)) {
    if (div->getOpcode() == Instruction::FDiv) {
      numerator = div->getOperand(0);
      denominator = div->getOperand(1);
      result = true;
    } else if (div->getOpcode() == Instruction::FMul) {
      if (llvm::Instruction *inv = llvm::dyn_cast<llvm::Instruction>(div->getOperand(1))) {
        if (inv->getOpcode() == Instruction::FDiv && dyn_cast<ConstantFP>(inv->getOperand(0)) &&
            dyn_cast<ConstantFP>(inv->getOperand(0))->isExactlyValue(1.0)) {
          numerator = div->getOperand(0);
          denominator = inv->getOperand(1);
          result = true;
        }
      }
    }
  }
  return result;
}

bool CustomUnsafeOptPass::visitBinaryOperatorDivAddDiv(BinaryOperator &I) {
  // A/B +C/D can be changed to (A * D +C * B)/(B * D).
  if (m_disableReorderingOpt) {
    return false;
  }

  Value *numerator0 = NULL;
  Value *numerator1 = NULL;
  Value *denumerator0 = NULL;
  Value *denumerator1 = NULL;

  if (isFDiv(I.getOperand(0), numerator0, denumerator0) && isFDiv(I.getOperand(1), numerator1, denumerator1) &&
      denumerator0 != denumerator1) {
    Value *mul0 = copyIRFlags(BinaryOperator::CreateFMul(numerator0, denumerator1, "", &I), &I);
    Value *mul1 = copyIRFlags(BinaryOperator::CreateFMul(numerator1, denumerator0, "", &I), &I);
    Value *mul2 = copyIRFlags(BinaryOperator::CreateFMul(denumerator0, denumerator1, "", &I), &I);
    Value *mul2inv = copyIRFlags(BinaryOperator::CreateFDiv(ConstantFP::get(mul2->getType(), 1.0), mul2, "", &I), &I);
    Value *add_mul0_mul1 = copyIRFlags(BinaryOperator::CreateFAdd(mul0, mul1, "", &I), &I);
    I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFMul(add_mul0_mul1, mul2inv, "", &I), &I));

    collectForErase(I, 1);

    return true;
  }
  return false;
}

bool CustomUnsafeOptPass::visitBinaryOperatorDivDivOp(BinaryOperator &I) {
  if (m_disableReorderingOpt) {
    return false;
  }

  llvm::Instruction *prevInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(1));
  bool patternFound = false;

  if (prevInst && prevInst->getOpcode() == Instruction::FDiv) {
    Value *prevInstOp = prevInst->getOperand(0);
    ConstantFP *C0 = dyn_cast<ConstantFP>(prevInstOp);

    if (C0 && C0->isExactlyValue(1.0)) {
      ConstantFP *Iconst = dyn_cast<ConstantFP>(I.getOperand(0));
      if (Iconst && Iconst->isExactlyValue(1.0)) {
        // a = 1 / b
        // c = 1 / a
        //    =>
        // c = b
        I.replaceAllUsesWith(prevInst->getOperand(1));
      } else {
        // a = 1 / b
        // d = c / a
        //    =>
        // d = c * b
        I.replaceAllUsesWith(
            copyIRFlags(BinaryOperator::CreateFMul(I.getOperand(0), prevInst->getOperand(1), "", &I), &I));
      }
      collectForErase(I, 1);
      ++Stat_FloatRemoved;
      patternFound = true;
      m_isChanged = true;
    }
  }

  return patternFound;
}

bool CustomUnsafeOptPass::visitBinaryOperatorAddSubOp(BinaryOperator &I) {
  bool patternFound = false;
  if (m_disableReorderingOpt) {
    return patternFound;
  }

  // a = b + x
  // d = a - x
  //    =>
  // d = b
  IGC_ASSERT(I.getOpcode() == Instruction::FAdd || I.getOpcode() == Instruction::FSub);

  Value *op[2];
  op[0] = I.getOperand(0);
  op[1] = I.getOperand(1);

  for (int i = 0; i < 2; i++) {
    llvm::Instruction *sourceInst = llvm::dyn_cast<llvm::Instruction>(op[i]);
    if (!sourceInst) {
      continue;
    }

    if (I.getOpcode() == Instruction::FAdd && sourceInst->getOpcode() == Instruction::FSub) {
      // a = b - x
      // d = a + x
      //    =>
      // d = b
      if (op[1 - i] == sourceInst->getOperand(1)) {
        I.replaceAllUsesWith(sourceInst->getOperand(0));
        patternFound = true;
        break;
      }
    } else if (I.getOpcode() == Instruction::FSub && sourceInst->getOpcode() == Instruction::FSub) {
      // a = x - b
      // d = x - a
      //    =>
      // d = b
      if (i == 1 && op[0] == sourceInst->getOperand(0)) {
        I.replaceAllUsesWith(sourceInst->getOperand(1));
        patternFound = true;
        break;
      }
    } else if (I.getOpcode() == Instruction::FSub && sourceInst->getOpcode() == Instruction::FAdd) {
      Value *srcOp[2];
      srcOp[0] = sourceInst->getOperand(0);
      srcOp[1] = sourceInst->getOperand(1);
      if (i == 0) {
        for (int srci = 0; srci < 2; srci++) {
          if (op[1 - i] == srcOp[srci]) {
            I.replaceAllUsesWith(srcOp[1 - srci]);
            patternFound = true;
            break;
          }
        }
      } else if (i == 1) {
        for (int srci = 0; srci < 2; srci++) {
          if (op[1 - i] == srcOp[srci]) {
            I.replaceAllUsesWith(copyIRFlags(
                BinaryOperator::CreateFSub(ConstantFP::get(op[0]->getType(), 0), srcOp[1 - srci], "", &I), &I));
            patternFound = true;
            break;
          }
        }
      }
    } else {
      continue;
    }
  }

  if (patternFound) {
    collectForErase(I, 1);
    ++Stat_FloatRemoved;
    m_isChanged = true;
  }

  return patternFound;
}

bool CustomUnsafeOptPass::visitBinaryOperatorPropNegate(BinaryOperator &I) {
  if (m_disableReorderingOpt) {
    return false;
  }

  bool foundPattern = false;

  // a = 0 - b
  // c = a + d
  //    =>
  // c = d - b
  IGC_ASSERT(I.getOpcode() == Instruction::FAdd);

  for (int i = 0; i < 2; i++) {
    llvm::Instruction *prevInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(i));
    if (prevInst && prevInst->getOpcode() == Instruction::FSub) {
      ConstantFP *fp0 = dyn_cast<ConstantFP>(prevInst->getOperand(0));
      if (fp0 && fp0->isZero()) {
        I.replaceAllUsesWith(
            copyIRFlags(BinaryOperator::CreateFSub(I.getOperand(1 - i), prevInst->getOperand(1), "", &I), &I));
        collectForErase(I, 1);
        ++Stat_FloatRemoved;
        m_isChanged = true;
        foundPattern = true;
        break;
      }
    }
  }
  return foundPattern;
}

bool CustomUnsafeOptPass::visitBinaryOperatorNegateMultiply(BinaryOperator &I) {
  // a = b * c
  // d = 0 - a
  //    =>
  // d = (-b) * c
  IGC_ASSERT(I.getOpcode() == Instruction::FSub);
  bool patternFound = false;
  bool replaced = false;

  if (I.getOperand(1)->hasOneUse()) {
    llvm::Instruction *fmulInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(1));
    if (fmulInst && fmulInst->getOpcode() == Instruction::FMul) {
      // if one of the mul src is imm, apply the negate on that imm
      for (int i = 0; i < 2; i++) {
        if (llvm::Instruction *fmulSrc = llvm::dyn_cast<llvm::Instruction>(fmulInst->getOperand(i))) {
          if (ConstantFP *fp = dyn_cast<ConstantFP>(fmulSrc)) {
            APFloat newConstantFloat = fp->getValueAPF();
            newConstantFloat.changeSign();
            Constant *newConstant = ConstantFP::get(fmulSrc->getContext(), newConstantFloat);
            fmulSrc->setOperand(i, newConstant);
            replaced = true;
            break;
          }
        }
      }

      // otherwise replace mul src0 with the negate
      if (!replaced) {
        BinaryOperator *fsub =
            BinaryOperator::CreateFSub(ConstantFP::get(fmulInst->getType(), 0), fmulInst->getOperand(0), "", fmulInst);
        if (m_ctx->m_checkFastFlagPerInstructionInCustomUnsafeOptPass) {
          fsub = copyIRFlags(fsub, &I);
        }
        fmulInst->setOperand(0, fsub);

        // DIExpression in debug variable instructions must be extended with additional DWARF opcode:
        // DW_OP_neg
        if (auto fsubInstr = dyn_cast<Instruction>(fsub)) {
          Value *fsubOp0 = fsubInstr->getOperand(1);
          if (isa<Instruction>(fsubOp0)) {
            if (Instruction *NewfmulInst = dyn_cast<Instruction>(fmulInst)) {
              const DebugLoc &DL = NewfmulInst->getDebugLoc();
              fsubInstr->setDebugLoc(DL);
              auto *Val = static_cast<Value *>(fmulInst);
              SmallVector<DbgValueInst *, 1> DbgValues;
              llvm::findDbgValues(DbgValues, Val);
              for (auto DV : DbgValues) {
                DIExpression *OldExpr = DV->getExpression();
                DIExpression *NewExpr =
                    DIExpression::append(OldExpr, {dwarf::DW_OP_constu, 0, dwarf::DW_OP_swap, dwarf::DW_OP_minus});
                IGCLLVM::setExpression(DV, NewExpr);
              }
            }
          }
        }
      }
      I.replaceAllUsesWith(fmulInst);
      collectForErase(I);
      ++Stat_FloatRemoved;
      m_isChanged = true;
      patternFound = true;
    }
  }
  return patternFound;
}

bool CustomUnsafeOptPass::visitBinaryOperatorTwoConstants(BinaryOperator &I) {
  bool patternFound = false;

  if (m_disableReorderingOpt) {
    return patternFound;
  }

  // a = b + C0
  // d = a + C1
  //    =>
  // d = b + ( C0 + C1 )

  // this optimization works on fadd, fsub, and fmul

  IGC_ASSERT(dyn_cast<ConstantFP>(I.getOperand(0)) || dyn_cast<ConstantFP>(I.getOperand(1)));

  llvm::Instruction::BinaryOps opcode = I.getOpcode();
  IGC_ASSERT(opcode == Instruction::FAdd || opcode == Instruction::FSub || opcode == Instruction::FMul);

  int regSrcNum = (dyn_cast<ConstantFP>(I.getOperand(0))) ? 1 : 0;
  Value *Iop = I.getOperand(regSrcNum);

  llvm::Instruction *prevInst = llvm::dyn_cast<llvm::Instruction>(Iop);

  if (!prevInst) {
    return patternFound;
  }

  if (prevInst->getOpcode() != Instruction::FMul && prevInst->getOpcode() != Instruction::FAdd &&
      prevInst->getOpcode() != Instruction::FSub) {
    return patternFound;
  }

  // check if prevInst has one constant in the srcs.
  if (dyn_cast<ConstantFP>(prevInst->getOperand(0)) || dyn_cast<ConstantFP>(prevInst->getOperand(1))) {
    if (!prevInst->hasOneUse() && I.getOpcode() == Instruction::FSub) {
      ConstantFP *ConstantZero = dyn_cast<ConstantFP>(I.getOperand(0));
      if (ConstantZero && ConstantZero->isZeroValue()) {
        return patternFound;
      }
    }

    int prevInstRegSrcNum = (dyn_cast<ConstantFP>(prevInst->getOperand(0))) ? 1 : 0;

    Value *prevInstOp = prevInst->getOperand(prevInstRegSrcNum);

    ConstantFP *C0 = dyn_cast<ConstantFP>(prevInst->getOperand(1 - prevInstRegSrcNum));
    ConstantFP *C1 = dyn_cast<ConstantFP>(I.getOperand(1 - regSrcNum));

    IGC_ASSERT(nullptr != C0);
    IGC_ASSERT(nullptr != C1);

    APFloat newConstantFloat(0.0);
    bool orderConstantFirst = false;
    bool changeOpToAdd = false;
    bool changeOpToSub = false;
    if (prevInst->getOpcode() == Instruction::FMul && opcode == Instruction::FMul) {
      newConstantFloat = C0->getValueAPF();
      newConstantFloat.multiply(C1->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
    } else if (prevInst->getOpcode() == Instruction::FAdd && opcode == Instruction::FAdd) {
      newConstantFloat = C0->getValueAPF();
      newConstantFloat.add(C1->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
    } else if (prevInst->getOpcode() == Instruction::FSub && opcode == Instruction::FSub) {
      if (prevInstRegSrcNum == 0 && regSrcNum == 0) {
        newConstantFloat = C0->getValueAPF();
        newConstantFloat.add(C1->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
      } else if (prevInstRegSrcNum == 0 && regSrcNum == 1) {
        newConstantFloat = C0->getValueAPF();
        newConstantFloat.add(C1->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        orderConstantFirst = true;
      } else if (prevInstRegSrcNum == 1 && regSrcNum == 0) {
        newConstantFloat = C0->getValueAPF();
        newConstantFloat.subtract(C1->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        orderConstantFirst = true;
      } else if (prevInstRegSrcNum == 1 && regSrcNum == 1) {
        newConstantFloat = C1->getValueAPF();
        newConstantFloat.subtract(C0->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        changeOpToAdd = true;
      }

    } else if (prevInst->getOpcode() == Instruction::FAdd && opcode == Instruction::FSub) {
      if (regSrcNum == 0) {
        newConstantFloat = C0->getValueAPF();
        newConstantFloat.subtract(C1->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        changeOpToAdd = true;
      } else {
        newConstantFloat = C1->getValueAPF();
        newConstantFloat.subtract(C0->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        orderConstantFirst = true;
      }
    } else if (prevInst->getOpcode() == Instruction::FSub && opcode == Instruction::FAdd) {
      if (prevInstRegSrcNum == 0) {
        newConstantFloat = C1->getValueAPF();
        newConstantFloat.subtract(C0->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        changeOpToAdd = true;
      } else {
        newConstantFloat = C1->getValueAPF();
        newConstantFloat.add(C0->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        changeOpToSub = true;
        orderConstantFirst = true;
      }
    } else {
      return patternFound;
    }

    ++Stat_FloatRemoved;
    Constant *newConstant = ConstantFP::get(C1->getContext(), newConstantFloat);
    if (newConstant->isZeroValue() && !orderConstantFirst) {
      if (opcode == Instruction::FAdd || opcode == Instruction::FSub) {
        I.replaceAllUsesWith(prevInstOp);
        patternFound = true;
        m_isChanged = true;
      } else if (opcode == Instruction::FMul) {
        I.replaceAllUsesWith(ConstantFP::get(Iop->getType(), 0));
        patternFound = true;
        m_isChanged = true;
      }
    } else {
      if (changeOpToAdd) {
        I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFAdd(prevInstOp, newConstant, "", &I), &I));
      } else if (changeOpToSub) {
        if (orderConstantFirst) {
          I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFSub(newConstant, prevInstOp, "", &I), &I));
        } else {
          I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFSub(prevInstOp, newConstant, "", &I), &I));
        }
      } else {
        I.setOperand(orderConstantFirst, prevInstOp);
        I.setOperand(1 - orderConstantFirst, newConstant);
      }
      patternFound = true;
    }

    if (patternFound) {
      if (I.use_empty()) {
        collectForErase(I, 1);
      } else if (prevInst->use_empty()) {
        collectForErase(*prevInst);
      }
    }
  }
  return patternFound;
}

bool CustomUnsafeOptPass::visitBinaryOperatorDivRsq(BinaryOperator &I) {
  if (GenIntrinsicInst *genIntr = dyn_cast<GenIntrinsicInst>(I.getOperand(1))) {
    if (genIntr->getIntrinsicID() == GenISAIntrinsic::GenISA_rsq) {
      if (ConstantFP *fp0 = dyn_cast<ConstantFP>(I.getOperand(0))) {
        llvm::IRBuilder<> builder(I.getContext());
        llvm::CallInst *sqrt_call = llvm::IntrinsicInst::Create(
            llvm::Intrinsic::getDeclaration(m_ctx->getModule(), Intrinsic::sqrt, builder.getFloatTy()),
            genIntr->getOperand(0), "", &I);

        if (fp0->isExactlyValue(1.0)) {
          // 1/rsq -> sqrt
          I.replaceAllUsesWith(sqrt_call);
        } else {
          // a/rsq -> a*sqrt
          I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFMul(I.getOperand(0), sqrt_call, "", &I), &I));
        }
        collectForErase(I, 1);
        return true;
      }
    }
  }
  return false;
}

bool CustomUnsafeOptPass::visitBinaryOperatorAddDiv(BinaryOperator &I) {
  llvm::Instruction *faddInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(0));

  if (faddInst && (faddInst->getOpcode() == Instruction::FAdd || faddInst->getOpcode() == Instruction::FSub) &&
      faddInst->hasOneUse()) {
    for (int i = 0; i < 2; i++) {
      if (faddInst->getOperand(i) == I.getOperand(1)) {
        BinaryOperator *div = BinaryOperator::CreateFDiv(faddInst->getOperand(1 - i), I.getOperand(1), "", faddInst);
        if (m_ctx->m_checkFastFlagPerInstructionInCustomUnsafeOptPass) {
          div = copyIRFlags(div, &I);
        }

        const DebugLoc &DL = faddInst->getDebugLoc();
        if (Instruction *divInst = dyn_cast<Instruction>(div))
          divInst->setDebugLoc(DL);

        Value *one = ConstantFP::get(I.getType(), 1.0);

        if (faddInst->getOpcode() == Instruction::FAdd) {
          if (i == 0) {
            I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFAdd(one, div, "", &I), &I));
          } else {
            I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFAdd(div, one, "", &I), &I));
          }
        } else if (faddInst->getOpcode() == Instruction::FSub) {
          if (i == 0) {
            I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFSub(one, div, "", &I), &I));
          } else {
            I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFSub(div, one, "", &I), &I));
          }
        }
        collectForErase(I, 1);
        return true;
      }
    }
  }
  return false;
}

bool CustomUnsafeOptPass::visitBinaryOperatorFDivFMulCancellation(llvm::BinaryOperator &I) {
  Instruction *fMulInst = (Instruction *)(&I);

  ///    example:
  ///    id192 = id187(op0) /(FDiv) id779(op1)
  ///    id498 = id779(op0) *(FMul) id192(op1) => id498 = id187(FDiv op0)
  /// or id498 = id192(op0) *(FMul) id779(op1) => id498 = id187(FDiv op0)
  if (fMulInst->getOpcode() == BinaryOperator::FMul) {
    Instruction *s0 = dyn_cast<Instruction>(fMulInst->getOperand(0));
    Instruction *s1 = dyn_cast<Instruction>(fMulInst->getOperand(1));

    if (s0 && s0->getOpcode() == BinaryOperator::FDiv) {
      if (s0->getOperand(1) == fMulInst->getOperand(1)) {
        fMulInst->replaceAllUsesWith(s0->getOperand(0));
        return true;
      }
    } else if (s1 && s1->getOpcode() == BinaryOperator::FDiv) {
      if (s1->getOperand(1) == fMulInst->getOperand(0)) {
        fMulInst->replaceAllUsesWith(s1->getOperand(0));
        return true;
      }
    }
  }

  return false;
}

bool CustomUnsafeOptPass::visitExchangeCB(llvm::BinaryOperator &I) {
  // a = b x CB0
  // c = b x CB1
  // e = a + c
  //    =>
  // e = b x ( CB0 + CB1 )

  // CB can be constant buffer load or immediate constant.
  // The goal is to put loop invariant calculations together and hoist it out of a loop.

  Instruction *inst0 = dyn_cast<Instruction>(I.getOperand(0));
  Instruction *inst1 = dyn_cast<Instruction>(I.getOperand(1));

  if (!inst0 || !inst1 || inst0->getOpcode() != Instruction::FMul || inst1->getOpcode() != Instruction::FMul) {
    return false;
  }

  unsigned bufId = 0;
  unsigned cbIndex0 = 0;
  unsigned cbIndex1 = 0;
  unsigned hasCB = 0;
  bool directBuf = false;

  for (int i = 0; i < 2; i++) {
    if (LoadInst *ld0 = dyn_cast<LoadInst>(inst0->getOperand(i))) {
      if (IGC::DecodeAS4GFXResource(ld0->getPointerAddressSpace(), directBuf, bufId) == CONSTANT_BUFFER && directBuf) {
        cbIndex0 = i;
        hasCB++;
      }
    } else if (dyn_cast<Constant>(inst0->getOperand(i))) {
      cbIndex0 = i;
      hasCB++;
    }
  }

  if (hasCB != 1)
    return false;

  hasCB = 0;
  for (int i = 0; i < 2; i++) {
    if (LoadInst *ld1 = dyn_cast<LoadInst>(inst1->getOperand(i))) {
      if (IGC::DecodeAS4GFXResource(ld1->getPointerAddressSpace(), directBuf, bufId) == CONSTANT_BUFFER && directBuf) {
        cbIndex1 = i;
        hasCB++;
      } else if (dyn_cast<Constant>(inst1->getOperand(i))) {
        cbIndex1 = i;
        hasCB++;
      }
    }
  }

  if (hasCB != 1)
    return false;

  if (inst0->getOperand(1 - cbIndex0) != inst1->getOperand(1 - cbIndex1))
    return false;

  // perform the change
  Value *CBsum =
      copyIRFlags(BinaryOperator::CreateFAdd(inst0->getOperand(cbIndex0), inst1->getOperand(cbIndex1), "", &I), &I);
  I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFMul(inst0->getOperand(1 - cbIndex0), CBsum, "", &I), &I));

  collectForErase(I, 1);

  return true;
}

void CustomUnsafeOptPass::visitBinaryOperator(BinaryOperator &I) {
  if (I.use_empty()) {
    return;
  }

  if (allowUnsafeMathOpt(m_ctx, I)) {
    if (!m_ctx->platform.supportLRPInstruction()) {
      // Optimize mix operation if detected.
      // Mix is computed as: x*(1 - a) + y*a
      matchMixOperation(I);
    }

    Value *op0 = I.getOperand(0);
    Value *op1 = I.getOperand(1);
    if (op0->getType()->isFPOrFPVectorTy() && op1->getType()->isFPOrFPVectorTy()) {
      ConstantFP *fp0 = dyn_cast<ConstantFP>(op0);
      ConstantFP *fp1 = dyn_cast<ConstantFP>(op1);
      Type *opType = op0->getType();

      switch (I.getOpcode()) {
      case Instruction::FSub:
        if (op0 == op1) {
          // X - X => 0
          I.replaceAllUsesWith(ConstantFP::get(opType, 0));
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else if (fp1 && fp1->isZero()) {
          // X - 0 => X
          I.replaceAllUsesWith(op0);
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else if (fp0 && fp0->isZero()) {
          m_isChanged |= visitBinaryOperatorNegateMultiply(I);
        } else {
          bool patternFound = false;
          if (fp0 || fp1) {
            // a = b + C0
            // d = a + C1
            //    =>
            // d = b + ( C0 + C1 )
            patternFound = visitBinaryOperatorTwoConstants(I);
          } else {
            // a = b + x
            // d = a - x
            //    =>
            // d = b
            patternFound = visitBinaryOperatorAddSubOp(I);
          }
          m_isChanged |= patternFound;
        }

        break;

      case Instruction::FAdd:
        if (fp0 && fp0->isZero()) {
          // 0 + X => X
          I.replaceAllUsesWith(op1);
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else if (fp1 && fp1->isZero()) {
          // X + 0 => X
          I.replaceAllUsesWith(op0);
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else {
          bool patternFound = false;
          if (fp0 || fp1) {
            // a = b + C0
            // d = a + C1
            //    =>
            // d = b + ( C0 + C1 )
            patternFound = visitBinaryOperatorTwoConstants(I);
          }

          if (op0 != op1) {
            // a = b - x
            // d = a + x
            //    =>
            // d = b
            if (!patternFound) {
              patternFound = visitBinaryOperatorAddSubOp(I);
            }

            // a = 0 - b
            // c = a + d
            //    =>
            // c = d - b
            if (!patternFound) {
              patternFound = visitBinaryOperatorPropNegate(I);
            }
          }

          // fmul/fadd propagation
          if (!patternFound) {
            patternFound = visitBinaryOperatorFmulFaddPropagation(I);
          }

          // A/B +C/D can be changed to (A * D +C * B)/(B * D).
          if (!patternFound && IGC_IS_FLAG_ENABLED(EnableSumFractions)) {
            patternFound = visitBinaryOperatorDivAddDiv(I);
          }

          if (!patternFound && IGC_IS_FLAG_ENABLED(EnableExtractCommonMultiplier)) {
            patternFound = visitBinaryOperatorExtractCommonMultiplier(I);
          }

          if (!patternFound) {
            patternFound = visitExchangeCB(I);
          }
          m_isChanged |= patternFound;
        }
        break;

      case Instruction::FMul:
        if ((fp0 && fp0->isZero()) || (fp1 && fp1->isZero())) {
          // X * 0 => 0
          // 0 * X => 0
          I.replaceAllUsesWith(ConstantFP::get(opType, 0));
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else if (fp0 && fp0->isExactlyValue(1.0)) {
          // 1 * X => X
          I.replaceAllUsesWith(op1);
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else if (fp1 && fp1->isExactlyValue(1.0)) {
          // X * 1 => X
          I.replaceAllUsesWith(op0);
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        }
        // X * -1 => -X
        else if (fp1 && fp1->isExactlyValue(-1.0)) {
          I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFSub(ConstantFP::get(opType, 0), op0, "", &I), &I));
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else if (fp0 && fp0->isExactlyValue(-1.0)) {
          I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFSub(ConstantFP::get(opType, 0), op1, "", &I), &I));
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else {
          bool patternFound = false;

          if (fp0 || fp1) {
            // a = b * C0
            // d = a * C1
            //    =>
            // d = b * ( C0 * C1 )
            patternFound = visitBinaryOperatorTwoConstants(I);
          }

          // fmul/fadd propagation
          if (!patternFound) {
            patternFound = visitBinaryOperatorFmulFaddPropagation(I);
          }

          // x*(1 - a) = x - x*a
          if (!patternFound) {
            patternFound = visitBinaryOperatorFmulToFmad(I);
          }

          // C1*(a + C0) = a*C1 + C0*C1
          if (!patternFound) {
            patternFound = visitBinaryOperatorToFmad(I);
          }

          m_isChanged |= patternFound;
        }
        break;

      case Instruction::FDiv:
        if (fp0 && fp0->isZero()) {
          // 0 / X => 0
          I.replaceAllUsesWith(ConstantFP::get(opType, 0));
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else if (fp1 && fp1->isExactlyValue(1.0)) {
          // X / 1 => X
          I.replaceAllUsesWith(op0);
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else if (op0 == op1) {
          // X / X = 1
          I.replaceAllUsesWith(ConstantFP::get(opType, 1.0));
          collectForErase(I);
          ++Stat_FloatRemoved;
          m_isChanged = true;
        } else {
          // a = 1 / b
          // c = 1 / a
          //    =>
          // c = b
          //     or
          // a = 1 / b
          // d = c / a
          //    =>
          // d = b * c
          bool patternFound = false;
          patternFound = visitBinaryOperatorDivDivOp(I);

          // 1/rsq -> rsq or a/rsq -> a * sqrt
          if (!patternFound) {
            patternFound = visitBinaryOperatorDivRsq(I);
          }

          // skip for double type.
          if (opType->getTypeID() == llvm::Type::FloatTyID || opType->getTypeID() == llvm::Type::HalfTyID) {
            // add r6.x, -r6.y, |r6.x|
            // div_sat r6.x, r6.x, r6.y
            //     To
            // div r6.y, l(1.000000, 1.000000, 1.000000, 1.000000), r6.y
            // mad_sat r6.x, | r6.x | , r6.y, l(-1.000000)
            if (!patternFound) {
              patternFound = visitBinaryOperatorAddDiv(I);
            }

            // FDIV to FMUL+INV
            if (!patternFound && !m_ctx->getCompilerOption().DisableFDivToFMulInvOpt) {
              if (!(fp0 && fp0->isExactlyValue(1.0))) {
                if (m_ctx->getModuleMetaData()->compOpt.FastRelaxedMath || I.hasAllowReciprocal()) {
                  Value *invOp = copyIRFlags(BinaryOperator::CreateFDiv(ConstantFP::get(opType, 1.0), op1, "", &I), &I);
                  I.replaceAllUsesWith(copyIRFlags(BinaryOperator::CreateFMul(op0, invOp, "", &I), &I));
                  collectForErase(I);
                  patternFound = true;
                }
              }
            }
          }
          m_isChanged |= patternFound;
        }
        break;

      default:
        break;
      }
    } else if (I.getOpcode() == Instruction::Xor) {
      m_isChanged |= visitBinaryOperatorXor(I);
    }
  } else if (m_ctx->getModuleMetaData()->compOpt.FastRelaxedMath) {
    if (I.getOpcode() == Instruction::Xor) {
      m_isChanged |= visitBinaryOperatorXor(I);
    }
  } else if (I.getType()->isFloatingPointTy()) {
    // c = a / b
    // d = b * c => d = a
    // or
    // d = c * b => d = a
    if (visitBinaryOperatorFDivFMulCancellation(I)) {
      collectForErase(I);
      m_isChanged = true;
    }
  }
}

// Optimize mix operation if detected.
// Mix is computed as x*(1 - a) + y*a
// Replace it with a*(y - x) + x to save one instruction ('add' ISA, 'sub' in IR).
// This pattern also optimizes a similar operation:
// x*(a - 1) + y*a which can be replaced with a(x + y) - x
void CustomUnsafeOptPass::matchMixOperation(llvm::BinaryOperator &I) {
  // Pattern Mix check step 1: find a FSub instruction with a constant value of 1.
  if (I.getOpcode() == BinaryOperator::FSub) {
    unsigned int fSubOpIdx = 0;
    while (fSubOpIdx < 2 && !llvm::isa<llvm::ConstantFP>(I.getOperand(fSubOpIdx))) {
      fSubOpIdx++;
    }
    if ((fSubOpIdx == 1) || ((fSubOpIdx == 0) && !llvm::isa<llvm::ConstantFP>(I.getOperand(1)))) {
      llvm::ConstantFP *fSubOpConst = llvm::cast<llvm::ConstantFP>(I.getOperand(fSubOpIdx));
      const APFloat &APF = fSubOpConst->getValueAPF();
      bool isInf = APF.isInfinity();
      bool isNaN = APF.isNaN();
      double val = 0.0;
      if (!isInf && !isNaN) {
        if (&APF.getSemantics() == &APFloat::IEEEdouble()) {
          val = APF.convertToDouble();
        } else if (&APF.getSemantics() == &APFloat::IEEEsingle()) {
          val = (double)APF.convertToFloat();
        }
      }
      if (val == 1.0) {
        bool doNotOptimize = false;
        bool matchFound = false;
        SmallVector<std::pair<Instruction *, Instruction *>, 3> fMulInsts;
        SmallVector<Instruction *, 3> fAddInsts;

        // Pattern Mix check step 2: there should be only FMul users of this FSub instruction
        for (User *subU : I.users()) {
          matchFound = false;
          Instruction *fMul = dyn_cast_or_null<Instruction>(subU);
          if (fMul && fMul->getOpcode() == BinaryOperator::FMul) {
            // Pattern Mix check step 3: there should be only fAdd users for such an FMul instruction
            for (User *mulU : fMul->users()) {
              Instruction *fAdd = dyn_cast_or_null<Instruction>(mulU);
              if (fAdd && fAdd->getOpcode() == BinaryOperator::FAdd) {
                // Pattern Mix check step 4: fAdd should be a user of two FMul instructions
                unsigned int opIdx = 0;
                while (opIdx < 2 && fMul != fAdd->getOperand(opIdx)) {
                  opIdx++;
                }

                if (opIdx < 2) {
                  opIdx = 1 - opIdx; // 0 -> 1 or 1 -> 0
                  Instruction *fMul2nd = dyn_cast_or_null<Instruction>(fAdd->getOperand(opIdx));

                  // Pattern Mix check step 5: Second fMul should be a user of the same,
                  // other than a value of 1.0, operand as fSub instruction
                  if (fMul2nd && fMul2nd->getOpcode() == BinaryOperator::FMul) {
                    unsigned int fSubNon1OpIdx = 1 - fSubOpIdx; // 0 -> 1 or 1 -> 0
                    unsigned int fMul2OpIdx = 0;
                    while (fMul2OpIdx < 2 && fMul2nd->getOperand(fMul2OpIdx) != I.getOperand(fSubNon1OpIdx)) {
                      fMul2OpIdx++;
                    }

                    if (fMul2OpIdx < 2) {
                      fMulInsts.push_back(std::make_pair(fMul, fMul2nd));
                      fAddInsts.push_back(fAdd);
                      matchFound = true; // Pattern Mix (partially) detected.
                    }
                  }
                }
              }
            }
          }

          if (!matchFound) {
            doNotOptimize = true; // To optimize both FMul instructions and FAdd must be found
          }
        }

        if (!doNotOptimize && !fMulInsts.empty()) {
          // Pattern Mix fully detected. Replace sequence of detected instructions with new ones.
          IGC_ASSERT_MESSAGE(fMulInsts.size() == fAddInsts.size(), "Incorrect pattern match data");
          // If Pattern Mix with 1-a in the first instruction was detected then create
          // this sequence of new instructions: FSub, FMul, FAdd.
          // But if Pattern Mix with a-1 in the first instruction was detected then create
          // this sequence of new instructions: FAdd, FMul, FSub.
          Instruction::BinaryOps newFirstInstType = (fSubOpIdx == 0) ? Instruction::FSub : Instruction::FAdd;
          Instruction::BinaryOps newLastInstType = (fSubOpIdx == 0) ? Instruction::FAdd : Instruction::FSub;

          fSubOpIdx = 1 - fSubOpIdx; // 0 -> 1 or 1 -> 0, i.e. get another FSub operand
          Value *r = I.getOperand(fSubOpIdx);

          while (!fMulInsts.empty()) {
            std::pair<Instruction *, Instruction *> fMulPair = fMulInsts.back();
            fMulInsts.pop_back();

            Instruction *fAdd = fAddInsts.back();
            fAddInsts.pop_back();

            unsigned int fMul2OpToFirstInstIdx = (r == fMulPair.second->getOperand(0)) ? 1 : 0;
            Value *newFirstInstOp = fMulPair.second->getOperand(fMul2OpToFirstInstIdx);
            Value *fSubVal = cast<Value>(&I);
            unsigned int fMul1OpToTakeIdx = (fSubVal == fMulPair.first->getOperand(0)) ? 1 : 0;

            Instruction *newFirstInst = BinaryOperator::Create(newFirstInstType, newFirstInstOp,
                                                               fMulPair.first->getOperand(fMul1OpToTakeIdx), "", fAdd);
            newFirstInst->copyFastMathFlags(fMulPair.first);
            DILocation *DL1st = I.getDebugLoc();
            if (DL1st) {
              newFirstInst->setDebugLoc(DL1st);
            }

            Instruction *newFMul = BinaryOperator::CreateFMul(
                fMulPair.second->getOperand((fMul2OpToFirstInstIdx + 1) % 2), newFirstInst, "", fAdd);
            newFMul->copyFastMathFlags(fMulPair.second);
            DILocation *DL2nd = fMulPair.second->getDebugLoc();
            if (DL2nd) {
              newFMul->setDebugLoc(DL2nd);
            }

            Instruction *newLastInst = BinaryOperator::Create(newLastInstType, newFMul,
                                                              fMulPair.first->getOperand(fMul1OpToTakeIdx), "", fAdd);
            newLastInst->copyFastMathFlags(fAdd);
            DILocation *DL3rd = fAdd->getDebugLoc();
            if (DL3rd) {
              newLastInst->setDebugLoc(DL3rd);
            }

            fAdd->replaceAllUsesWith(newLastInst);
            m_isChanged = true;
          }
        }
      }
    }
  }
}

bool CustomUnsafeOptPass::visitFCmpInstFCmpFAddOp(FCmpInst &FC) {
  //  %3 = fadd float %2, 0x40015C29
  //  %4 = fcmp uge float %3, 0.0
  //         =>
  //  %4 = fcmp uge float %2, -(0x40015C29)
  // only do the optimization if we add has only one use since it could prevent us to do
  // other optimization otherwise

  Value *fcmpOp1 = FC.getOperand(1);
  ConstantFP *fcmpConstant = dyn_cast<ConstantFP>(fcmpOp1);
  if (fcmpConstant) {
    llvm::Instruction *faddInst = llvm::dyn_cast<llvm::Instruction>(FC.getOperand(0));
    if (faddInst && (faddInst->getOpcode() == Instruction::FAdd || faddInst->getOpcode() == Instruction::FSub) &&
        faddInst->hasOneUse()) {
      Value *faddOp1 = faddInst->getOperand(1);
      ConstantFP *faddConstant = dyn_cast<ConstantFP>(faddOp1);
      if (faddConstant) {
        APFloat newConstantFloat(0.0);
        if (faddInst->getOpcode() == Instruction::FAdd) {
          newConstantFloat = fcmpConstant->getValueAPF();
          newConstantFloat.subtract(faddConstant->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        } else {
          newConstantFloat = fcmpConstant->getValueAPF();
          newConstantFloat.add(faddConstant->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
        }

        ConstantFP *newConstant = ConstantFP::get(fcmpConstant->getContext(), newConstantFloat);
        FC.setOperand(0, faddInst->getOperand(0));
        FC.setOperand(1, newConstant);
        if (faddInst->use_empty())
          collectForErase(*faddInst);
        ++Stat_FcmpRemoved;
        return true;
      }
    }
  }
  return false;
}

bool CustomUnsafeOptPass::visitFMulFCmpOp(FCmpInst &FC) {
  // pattern match fmul+fsub+fcmp into fcmp
  bool patternFound = false;
  llvm::Instruction *prevInst[2];
  prevInst[0] = llvm::dyn_cast<llvm::Instruction>(FC.getOperand(0));
  prevInst[1] = llvm::dyn_cast<llvm::Instruction>(FC.getOperand(1));

  if (!prevInst[0] || !prevInst[1]) {
    return false;
  }

  for (int i = 0; i < 2; i++) {
    if (prevInst[i]->getOpcode() != Instruction::FSub || prevInst[1 - i]->getOpcode() != Instruction::FMul) {
      continue;
    }
    ConstantFP *fpc = dyn_cast<ConstantFP>(prevInst[i]->getOperand(0));
    if (!fpc || !fpc->isZero() || prevInst[i]->getOperand(1) != prevInst[1 - i]) {
      continue;
    }
    // Found the following template:
    //   op_<1-i>: %mul = fmul float %x, %y
    //   op_<i>  : %sub = fsub float 0.000000e+00, %mul
    //             %cmp = fcmp <cmpOp> float %op_0, %op_1

    if (prevInst[1 - i]->getOperand(0) == prevInst[1 - i]->getOperand(1)) {
      // %x == %y --> %mul =  (x^2) always >=0
      //              %sub = -(x^2) always <=0

      if ((FC.getPredicate() == FCmpInst::FCMP_OLT && i == 0) || (FC.getPredicate() == FCmpInst::FCMP_OGT && i == 1)) {
        // Optimize:
        //   %cmp = fcmp olt float %sub, %mul       -> cmp.lt -(x^2), (x^2)
        // or:
        //   %cmp = fcmp ogt float %mul, %sub       -> cmp.gt (x^2), -(x^2)
        // into:
        //   %cmp = fcmp one float %x, 0          -> cmp.ne (x^2), 0
        FC.setPredicate(FCmpInst::FCMP_ONE);
        FC.setOperand(0, prevInst[1 - i]->getOperand(0));
        FC.setOperand(1, ConstantFP::get(Type::getFloatTy(FC.getContext()), 0));
        patternFound = true;
        break;
      }
    } else if (ConstantFP *fmulConstant = dyn_cast<ConstantFP>(prevInst[1 - i]->getOperand(1))) {
      if (fmulConstant->isZeroValue()) {
        continue;
      }
      // Optimize:
      //   %mul = fmul float %x, 5.000000e-01     ->   (x * constant)
      //   %sub = fsub float 0.000000e+00, %mul   ->  -(x * constant)
      //   %cmp = fcmp <cmpOp> float %mul, %sub
      // into:
      //   %cmp = fcmp <cmpOp> float %x, 0  [if (constant>0)]
      // or:
      //   %cmp = fcmp <cmpOp> float 0, %x  [if (constant<0)]

      if (fmulConstant->isNegative()) {
        if (i == 0) {
          // handling case:
          //     %mul = fmul float %x, -5.000000e-01     ->   (x * constant)
          //     %sub = fsub float 0.000000e+00, %mul   ->  -(x * constant)
          //     %cmp = fcmp <cmpOp> float %sub, %mul
          //  into:
          //     %cmp = fcmp <cmpOp> float %x, 0  [since (constant<0)]
          FC.setOperand(1, ConstantFP::get(Type::getFloatTy(FC.getContext()), 0));
          FC.setOperand(0, prevInst[1 - i]->getOperand(0));
          patternFound = true;
        } else {
          // handling case:
          //     %mul = fmul float %x, -5.000000e-01     ->   (x * constant)
          //     %sub = fsub float 0.000000e+00, %mul   ->  -(x * constant)
          //     %cmp = fcmp <cmpOp> float %mul, %sub
          //  into:
          //     %cmp = fcmp <cmpOp> float 0, %x  [since (constant<0)]
          FC.setOperand(0, ConstantFP::get(Type::getFloatTy(FC.getContext()), 0));
          FC.setOperand(1, prevInst[1 - i]->getOperand(0));
          patternFound = true;
        }
      } else {
        // handling case:
        //     %mul = fmul float %x, 5.000000e-01     ->   (x * constant)
        //     %sub = fsub float 0.000000e+00, %mul   ->  -(x * constant)
        //     %cmp = fcmp <cmpOp> float %sub, %mul
        if (i == 0) {
          FC.setOperand(0, ConstantFP::get(Type::getFloatTy(FC.getContext()), 0));
          FC.setOperand(1, prevInst[1 - i]->getOperand(0));
          patternFound = true;
        } else {
          // handling case:
          //     %mul = fmul float %x, 5.000000e-01     ->   (x * constant)
          //     %sub = fsub float 0.000000e+00, %mul   ->  -(x * constant)
          //     %cmp = fcmp <cmpOp> float %mul, %sub

          FC.setOperand(0, prevInst[1 - i]->getOperand(0));
          FC.setOperand(1, ConstantFP::get(Type::getFloatTy(FC.getContext()), 0));
          patternFound = true;
        }
      }
      break;
    }
  }

  if (patternFound) {
    llvm::Instruction *inst = prevInst[0]->getOpcode() == Instruction::FSub ? prevInst[0] : prevInst[1];
    if (inst->use_empty())
      collectForErase(*inst, 1);
  }

  return patternFound;
}

bool CustomUnsafeOptPass::visitFCmpInstFCmpSelOp(FCmpInst &FC) {
  //  %17 = fcmp ole float %16, 0.000000e+00
  //  %18 = select i1 %17, float 0.000000e+00, float 1.000000e+00
  //  %19 = fsub float -0.000000e+00, %18
  //  %20 = fcmp ueq float %18, %19
  //         =>
  //  %20 = fcmp ole float %16, 0.000000e+00
  llvm::Instruction *fSubInst = llvm::dyn_cast<llvm::Instruction>(FC.getOperand(1));
  if (fSubInst && fSubInst->getOpcode() == Instruction::FSub) {
    ConstantFP *fSubConstant = dyn_cast<ConstantFP>(fSubInst->getOperand(0));

    llvm::Instruction *selectInst = llvm::dyn_cast<llvm::Instruction>(FC.getOperand(0));

    if (selectInst && selectInst->getOpcode() == Instruction::Select &&
        selectInst == llvm::dyn_cast<llvm::Instruction>(fSubInst->getOperand(1)) && fSubConstant &&
        fSubConstant->isZero()) {
      ConstantFP *selectConstant1 = dyn_cast<ConstantFP>(selectInst->getOperand(1));
      ConstantFP *selectConstant2 = dyn_cast<ConstantFP>(selectInst->getOperand(2));

      llvm::Instruction *fCmpInst = llvm::dyn_cast<llvm::Instruction>(selectInst->getOperand(0));

      if (fCmpInst && fCmpInst->getOpcode() == Instruction::FCmp && selectConstant1 && selectConstant2 &&
          selectConstant1->isZero() && !selectConstant2->isZero()) {
        FC.setOperand(0, fCmpInst->getOperand(0));
        FC.setOperand(1, fCmpInst->getOperand(1));
        if (FC.getPredicate() == FCmpInst::FCMP_UNE) {
          FC.setPredicate(dyn_cast<FCmpInst>(fCmpInst)->getInversePredicate());
        } else {
          FC.setPredicate(dyn_cast<FCmpInst>(fCmpInst)->getPredicate());
        }
        Stat_FcmpRemoved += 3;

        if (fSubInst->use_empty())
          collectForErase(*fSubInst, 2);

        return true;
      }
    }
  }
  return false;
}

void CustomUnsafeOptPass::visitFCmpInst(FCmpInst &FC) {
  bool patternFound = false;
  if (FC.use_empty()) {
    return;
  }
  if (FC.getPredicate() == CmpInst::FCMP_UNO) {
    if (m_ctx->getCompilerOption().NoNaNs) {
      FC.replaceAllUsesWith(ConstantInt::getFalse(FC.getType()));
      collectForErase(FC);
      ++Stat_FcmpRemoved;
      patternFound = true;
    }
  } else if (FC.getPredicate() == CmpInst::FCMP_ORD) {
    if (m_ctx->getCompilerOption().NoNaNs) {
      FC.replaceAllUsesWith(ConstantInt::getTrue(FC.getType()));
      collectForErase(FC);
      ++Stat_FcmpRemoved;
      patternFound = true;
    }
  } else {
    patternFound = visitFCmpInstFCmpFAddOp(FC);
    if (!patternFound && (FC.getPredicate() == FCmpInst::FCMP_UEQ || FC.getPredicate() == FCmpInst::FCMP_UNE)) {
      patternFound = visitFCmpInstFCmpSelOp(FC);
    }

    if (!patternFound) {
      patternFound = visitFMulFCmpOp(FC);
    }
  }
  m_isChanged |= patternFound;
}

void CustomUnsafeOptPass::visitSelectInst(SelectInst &I) {
  if (llvm::FCmpInst *cmpInst = llvm::dyn_cast<llvm::FCmpInst>(I.getOperand(0))) {
    if (dyn_cast<FCmpInst>(cmpInst)->getPredicate() == FCmpInst::FCMP_OEQ) {
      if (ConstantFP *cmpConstant = dyn_cast<ConstantFP>(cmpInst->getOperand(1))) {
        if (cmpConstant->isZeroValue()) {
          /*
          %16 = fmul float %15, %0
          %17 = fadd float %16, %14
          %24 = fcmp oeq float %0, 0.000000e+00
          %25 = select i1 % 24, float %14, float %17
              to
          %25 = %17

          */
          bool foundPattern = false;
          llvm::Instruction *addInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(2)); // %17
          if (addInst && addInst->getOpcode() == Instruction::FAdd) {
            for (uint j = 0; j < 2; j++) {
              llvm::Instruction *mulInst = llvm::dyn_cast<llvm::Instruction>(addInst->getOperand(j)); // %16
              if (mulInst && mulInst->getOpcode() == Instruction::FMul &&
                  addInst->getOperand(1 - j) == I.getOperand(1)) {
                for (uint k = 0; k < 2; k++) {
                  if (mulInst->getOperand(k) == cmpInst->getOperand(0)) {
                    I.replaceAllUsesWith(I.getOperand(2));
                    collectForErase(I, 1);
                    foundPattern = true;
                    break;
                  }
                }
                if (foundPattern)
                  break;
              }
            }
          }
        } else if (cmpConstant->isExactlyValue(1.f)) {
          /*
          %21 = fsub float %8, %14
          %22 = fmul float %21, %0
          %23 = fadd float %22, %14
          %24 = fcmp oeq float %0, 1.000000e+00
          %27 = select i1 %24, float %8, float %23
              to
          %27 = %23

          */
          bool foundPattern = false;
          llvm::Instruction *addInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(2)); // %23
          if (addInst && addInst->getOpcode() == Instruction::FAdd) {
            for (uint j = 0; j < 2; j++) {
              llvm::Instruction *mulInst = llvm::dyn_cast<llvm::Instruction>(addInst->getOperand(j)); // %22
              if (mulInst && mulInst->getOpcode() == Instruction::FMul) {
                for (uint k = 0; k < 2; k++) {
                  llvm::Instruction *subInst = llvm::dyn_cast<llvm::Instruction>(mulInst->getOperand(k)); // %21
                  if (subInst && subInst->getOpcode() == Instruction::FSub &&
                      subInst->getOperand(0) == I.getOperand(1) &&
                      subInst->getOperand(1) == addInst->getOperand(1 - j) &&
                      mulInst->getOperand(1 - k) == cmpInst->getOperand(0)) {
                    I.replaceAllUsesWith(I.getOperand(2));
                    collectForErase(I, 1);
                    foundPattern = true;
                    break;
                  }
                }
                if (foundPattern)
                  break;
              }
            }
          }
        }
      }
    }
  }
}

void CustomUnsafeOptPass::strengthReducePowOrExpLog(IntrinsicInst *intrin, Value *base, Value *exponent, bool isPow) {
  IRBuilder<> irb(intrin);
  irb.setFastMathFlags(intrin->getFastMathFlags());
  if (exponent == ConstantFP::get(exponent->getType(), 0.5)) {
    // pow(x, 0.5) -> sqrt(x)
    llvm::Function *sqrtIntr = llvm::Intrinsic::getDeclaration(m_ctx->getModule(), Intrinsic::sqrt, base->getType());
    llvm::CallInst *sqrt = irb.CreateCall(sqrtIntr, base);
    intrin->replaceAllUsesWith(sqrt);
    collectForErase(*intrin);
  } else if (exponent == ConstantFP::get(exponent->getType(), 1.0)) {
    intrin->replaceAllUsesWith(base);
    collectForErase(*intrin);
  } else if (exponent == ConstantFP::get(exponent->getType(), 2.0)) {
    // pow(x, 2.0) -> x * x
    Value *x2 = irb.CreateFMul(base, base);
    intrin->replaceAllUsesWith(x2);
    collectForErase(*intrin);
  } else if (exponent == ConstantFP::get(exponent->getType(), 3.0)) {
    // pow(x, 3.0) -> x * x * x
    Value *x2 = irb.CreateFMul(base, base);
    Value *x3 = irb.CreateFMul(x2, base);
    intrin->replaceAllUsesWith(x3);
    collectForErase(*intrin);
  } else if (exponent == ConstantFP::get(exponent->getType(), 4.0)) {
    // pow(x, 4.0) -> (x*x) * (x*x)
    Value *x2 = irb.CreateFMul(base, base);
    Value *x4 = irb.CreateFMul(x2, x2);
    intrin->replaceAllUsesWith(x4);
    collectForErase(*intrin);
  } else if (exponent == ConstantFP::get(exponent->getType(), 5.0)) {
    // pow(x, 5.0) -> (x*x) * (x*x) * x
    Value *x2 = irb.CreateFMul(base, base);
    Value *x4 = irb.CreateFMul(x2, x2);
    Value *x5 = irb.CreateFMul(x4, base);
    intrin->replaceAllUsesWith(x5);
    collectForErase(*intrin);
  } else if (exponent == ConstantFP::get(exponent->getType(), 6.0)) {
    // pow(x, 6.0) -> (x*x) * (x*x) * (x*x)
    Value *x2 = irb.CreateFMul(base, base);
    Value *x4 = irb.CreateFMul(x2, x2);
    Value *x6 = irb.CreateFMul(x4, x2);
    intrin->replaceAllUsesWith(x6);
    collectForErase(*intrin);
  } else if (exponent == ConstantFP::get(exponent->getType(), 8.0)) {
    // pow(x, 8.0) -> ((x*x) * (x*x)) * ((x*x) * (x*x))
    Value *x2 = irb.CreateFMul(base, base);
    Value *x4 = irb.CreateFMul(x2, x2);
    Value *x8 = irb.CreateFMul(x4, x4);
    intrin->replaceAllUsesWith(x8);
    collectForErase(*intrin);
  } else if (isPow && IGC_IS_FLAG_ENABLED(EnablePowToLogMulExp)) {
    // pow(x, y) -> exp2(log2(x) * y)
    Function *logf = Intrinsic::getDeclaration(m_ctx->getModule(), Intrinsic::log2, base->getType());
    Function *expf = Intrinsic::getDeclaration(m_ctx->getModule(), Intrinsic::exp2, base->getType());
    CallInst *logv = irb.CreateCall(logf, base);
    Value *mulv = irb.CreateFMul(logv, exponent);
    CallInst *expv = irb.CreateCall(expf, mulv);
    intrin->replaceAllUsesWith(expv);
    collectForErase(*intrin);
  }
}

void CustomUnsafeOptPass::visitIntrinsicInst(IntrinsicInst &I) {
  const Intrinsic::ID ID = I.getIntrinsicID();
  auto modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

  if (ID == Intrinsic::pow) {
    if (!modMD->compOpt.disableReducePow && IGC_IS_FLAG_DISABLED(DisableReducePow)) {
      strengthReducePowOrExpLog(&I, I.getOperand(0), I.getOperand(1), true /* isPow */);
    }
  } else if (ID == Intrinsic::exp2) {
    BinaryOperator *mul = dyn_cast<BinaryOperator>(I.getOperand((0)));
    if (mul && mul->getOpcode() == Instruction::FMul) {
      for (uint j = 0; j < 2; j++) {
        IntrinsicInst *log = dyn_cast<IntrinsicInst>(mul->getOperand(j));
        if (log && log->getIntrinsicID() == Intrinsic::log2) {
          strengthReducePowOrExpLog(&I, log->getOperand(0), mul->getOperand(1 - j), false /* isPow */);
        }
      }
    }
  } else if (ID == Intrinsic::sqrt) {
    // y*y = x if y = sqrt(x).
    if (IGC_IS_FLAG_DISABLED(DisableSqrtOpt) && !modMD->compOpt.disableSqrtOpt) {
      int replacedUses = 0;
      for (auto iter = I.user_begin(); iter != I.user_end(); iter++) {
        if (Instruction *mul = dyn_cast<Instruction>(*iter)) {
          if (mul->getOpcode() == Instruction::FMul && mul->getOperand(0) == mul->getOperand(1)) {
            mul->replaceAllUsesWith(I.getOperand(0));
            collectForErase(*mul);
            ++replacedUses;
          }
        }
      }
      if (I.hasNUses(replacedUses))
        collectForErase(I);
    }

  }
  // This optimization simplifies FMA expressions with zero arguments.
  else if (ID == Intrinsic::fma && I.isFast()) {
    //  Searches the following pattern
    //      %0 = call fast float @llvm.fma.f32(float %x, float %y, float 0.000000e+00)
    //
    //  And changes it to:
    //      %0 = fmul fast float %y, %x
    if (ConstantFP *C = dyn_cast<ConstantFP>(I.getArgOperand(2))) {
      if (C->isZero()) {
        // change to mul
        IRBuilder<> irb(&I);
        Value *v = irb.CreateFMulFMF(I.getArgOperand(1), I.getArgOperand(0), &I);
        I.replaceAllUsesWith(v);
        collectForErase(I);
      }
    }
    //  Searches the following pattern
    //      %0 = call fast float @llvm.fma.f32(float %x, float 0.000000e+00, float %y)
    //      %1 = fmul fast float %0, 5.000000
    //
    //  And changes it to:
    //      %0 = fmul fast float %y, 5.000000
    //
    //  and
    //
    //  Searches the following pattern
    //      %0 = call fast float @llvm.fma.f32(float 0.000000e+00, float %x, float %y)
    //      %1 = fmul fast float %0, 5.000000
    //
    //  And changes it to:
    //      %0 = fmul fast float %y, 5.000000
    else {
      ConstantFP *A = dyn_cast<ConstantFP>(I.getArgOperand(0));
      ConstantFP *B = dyn_cast<ConstantFP>(I.getArgOperand(1));

      if ((A != nullptr && A->isZero()) || (B != nullptr && B->isZero())) {
        // replace
        I.replaceAllUsesWith(I.getArgOperand(2));
        collectForErase(I);
      }
    }
  }
}

// Search for reassociation candidate.
static bool searchFAdd(Instruction *DefI, Instruction *UseI, unsigned &level) {
  // Could search further, however we need to rewrite
  // instructions along the path. So limit this two
  // levels, which should cover common cases.
  if (level >= 2)
    return false;
  if (DefI->getParent() != UseI->getParent() || !DefI->hasOneUse() || UseI->user_empty())
    return false;
  if (UseI->getOpcode() != Instruction::FAdd && UseI->getOpcode() != Instruction::FSub)
    return false;

  // Swap operands such DefI is always the LHS in UseI.
  Value *Op = UseI->getOperand(1);
  bool IsFAdd = UseI->getOpcode() == Instruction::FAdd;
  if (DefI == Op) {
    if (IsFAdd) {
      cast<BinaryOperator>(UseI)->swapOperands();
      Op = UseI->getOperand(1);
    } else {
      return false;
    }
  }

  // The rhs is not mul, so could be folded into a mad.
  auto RHS = dyn_cast<Instruction>(Op);
  if (RHS && RHS->getOpcode() != Instruction::FMul)
    return true;

  // For simplicity, only allow the last level to be fsub.
  if (!IsFAdd)
    return false;

  return searchFAdd(UseI, UseI->user_back(), ++level);
}

// Match and re-associate arithmetic computation to emit more
// mad instructions. E.g.
//
// a * b + c * d +/- e -> MUL, MAD, ADD
//
// After reassociation, this becomes
//
// a * b +/- e + c * d -> MAD, MAD
//
void CustomUnsafeOptPass::reassociateMulAdd(Function &F) {
  if (m_disableReorderingOpt) {
    return;
  }

  auto modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  if (!modMD->compOpt.MadEnable && !m_ctx->m_DriverInfo.RespectPerInstructionContractFlag()) {
    return;
  }

  using namespace PatternMatch;

  for (auto &BB : F) {
    for (auto I = BB.begin(); I != BB.end(); /*Empty*/) {
      Instruction *Inst = &*I++;
      BinaryOperator *BinOp = dyn_cast<BinaryOperator>(Inst);
      if (!BinOp || !allowUnsafeMathOpt(m_ctx, *BinOp))
        continue;
      Value *A, *B, *C, *D;
      // Match Exp = A * B + C * D with a single use so that
      // it is benefical to fold one FSub/FAdd with A * B.
      if (match(Inst, m_OneUse(m_FAdd(m_FMul(m_Value(A), m_Value(B)), m_FMul(m_Value(C), m_Value(D)))))) {
        Instruction *L0 = Inst->user_back();
        unsigned level = 0;
        if (searchFAdd(Inst, L0, level)) {
          Value *T0 = Inst->getOperand(0);
          Value *T1 = Inst->getOperand(1);

          // rewrite the expression tree
          if (level == 0) {
            // t0 = A * B
            // t1 = C * D
            // t2 = t0 + t1  // Inst
            // t3 = t2 - E   // L0
            //
            // as
            //
            // t0 = A * B
            // t1 = C * D
            // t2 = t0 + t1   // Inst
            // ...
            // t2n = t0 - E   // new Inst
            // t3n = t2n + t1 // new L0
            // t3  = t2 - E   // L0
            IRBuilder<> Builder(L0);
            Builder.setFastMathFlags(L0->getFastMathFlags());
            Value *E = L0->getOperand(1);
            auto OpKind = BinaryOperator::BinaryOps(L0->getOpcode());
            Value *NewInst = Builder.CreateBinOp(OpKind, T0, E, Inst->getName());
            Value *NewL0 = Builder.CreateFAdd(NewInst, T1, L0->getName());
            L0->replaceAllUsesWith(NewL0);
            collectForErase(*L0, 1);
            m_isChanged = true;
          } else if (level == 1) {
            // t0 = A * B
            // t1 = C * D
            // t2 = E * F
            // t3 = t0 + t1 // Inst
            // t4 = t3 + t2 // L0
            // t5 = t4 - G  // L1
            //
            // as
            //
            // t0 = A * B
            // t1 = C * D
            // t2 = E * F
            // t3 = t0 + t1  // Inst
            // t4 = t3 + t2  // L0
            // ...
            // t3n = t0 - G   // NewInst
            // t4n = t3n + t2 // NewL0
            // t5n = t4n + t1 // NewL1
            // t5  = t4 - G   // L1
            Instruction *L1 = L0->user_back();
            IRBuilder<> Builder(L1);
            Builder.setFastMathFlags(L1->getFastMathFlags());
            Value *T2 = L0->getOperand(1);
            Value *G = L1->getOperand(1);

            auto OpKind = BinaryOperator::BinaryOps(L1->getOpcode());
            Value *NewInst = Builder.CreateBinOp(OpKind, T0, G, Inst->getName());
            Value *NewL0 = Builder.CreateFAdd(NewInst, T2, L0->getName());
            Value *NewL1 = Builder.CreateFAdd(NewL0, T1, L1->getName());
            L1->replaceAllUsesWith(NewL1);
            collectForErase(*L1, 2);
            m_isChanged = true;
          }
        }
      }
    }
  }
}

// Collects instructions that were optimized out and are safe to erase.
// Instructions will be erased at the end of the function pass. This is
// required if optimized instruction can't be removed during optimization
// (e.g. is next instruction from currently visited).
// Optional argument controls how many instructions back should be
// considered for erase.
inline void CustomUnsafeOptPass::collectForErase(llvm::Instruction &I, unsigned int operandsDepth) {
  m_instToErase.insert(&I);

  if (operandsDepth > 0) {
    for (auto it = I.op_begin(); it != I.op_end(); ++it) {
      Instruction *op = dyn_cast<Instruction>(*it);
      if (op && op->hasOneUse()) {
        collectForErase(*op, operandsDepth - 1);
      }
    }
  }
}

void CustomUnsafeOptPass::eraseCollectedInst() {
  if (m_instToErase.empty())
    return;

  for (auto i : m_instToErase) {
    i->eraseFromParent();
  }
  m_instToErase.clear();
}

// This pass looks for potential patterns where, if some value evaluates
// to zero, then a long chain of computation will be zero as well and
// we can just skip it (a so called 'early out').  For example:
//
// before:
//
// a = some value; // might be zero
// result = a * expensive_operation();
//
// after:
//
// a = some value;
// if (a == 0)
//   result = 0;
// else
//   result = a * expensive_operation();
//
// Currently this is used to target d3d workloads
//
class EarlyOutPatterns : public FunctionPass {
public:
  static char ID;

  EarlyOutPatterns() : FunctionPass(ID), m_ctx(nullptr), m_ShaderLength(0) {}
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const { AU.addRequired<CodeGenContextWrapper>(); }

  virtual bool runOnFunction(Function &F);
  virtual bool processBlock(BasicBlock *BB);

  virtual llvm::StringRef getPassName() const { return "EarlyOutPatterns"; }

private:
  static bool canOptimizeSampleInst(SmallVector<Instruction *, 4> &Channels, GenIntrinsicInst *GII);
  static bool canOptimizeDotProduct(SmallVector<Instruction *, 4> &Values, Instruction *I);
  static bool canOptimizeNdotL(SmallVector<Instruction *, 4> &Values, FCmpInst *FC);
  static bool canOptimizeDirectOutput(SmallVector<Instruction *, 4> &Values, GenIntrinsicInst *GII, Value *&SI,
                                      unsigned int ShaderLength);
  static bool canOptimizeMulMaxMatch(SmallVector<Instruction *, 4> &Values, Instruction *I);
  static bool canOptimizeSelectOutput(SelectInst *SI);
  static bool DotProductMatch(const Instruction *I);
  static bool DotProductSourceMatch(const Instruction *I);
  static BasicBlock *tryFoldAndSplit(ArrayRef<Instruction *> Values, Instruction *Root, const unsigned FoldThreshold,
                                     const unsigned FoldThresholdMultiChannel, const unsigned RatioNeeded);
  static bool trackAddSources(BinaryOperator *addInst);
  static DenseSet<const Value *> tryAndFoldValues(ArrayRef<Instruction *> Values);
  static BasicBlock *SplitBasicBlock(Instruction *inst, const DenseSet<const Value *> &FoldedVals);
  static bool FoldsToZero(const Instruction *inst, const Value *use, const DenseSet<const Value *> &FoldedVals);
  static bool EarlyOutBenefit(const Instruction *earlyOutInst, const DenseSet<const Value *> &FoldedVals,
                              const unsigned int ratioNeeded);
  static void foldFromAdd(SmallVector<Instruction *, 4> &Values, Instruction *&NewInsertPoint);
  static Instruction *moveToDef(Instruction *Def, ArrayRef<Instruction *> Users);
  static bool
  isSplitProfitable(const Instruction *Root, ArrayRef<Instruction *> Values, const DenseSet<const Value *> &FoldedVals,
                    // Number of instructions which needs to be folded in order for the optimization to be worth it
                    const unsigned FoldThreshold,
                    // For cases where we need to AND several channels we have a higher threshold
                    const unsigned FoldThresholdMultiChannel, const unsigned RatioNeeded);
  static BasicBlock *cmpAndSplitAtPoint(Instruction *Root, ArrayRef<Instruction *> Values,
                                        const DenseSet<const Value *> &FoldedVals);
  static unsigned int shortPathToOutput(Value *inst, unsigned int limit);
  static Instruction *FindLastFoldedValue(const DenseSet<const Value *> &FoldedVals, BasicBlock *currentBB);

  CodeGenContext *m_ctx;
  unsigned int m_ShaderLength;
};

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

#define PASS_FLAG "igc-early-out-patterns-pass"
#define PASS_DESCRIPTION "Early out to avoid heavy computation"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(EarlyOutPatterns, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(EarlyOutPatterns, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char EarlyOutPatterns::ID = 0;

FunctionPass *IGC::CreateEarlyOutPatternsPass() { return new EarlyOutPatterns(); }

bool EarlyOutPatterns::runOnFunction(Function &F) {
  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (IGC_IS_FLAG_ENABLED(DisableEarlyOutPatterns) || m_ctx->getModuleMetaData()->compOpt.DisableEarlyOut ||
      m_ctx->m_DriverInfo.WaNOSNotResolved()) {
    return false;
  }

  m_ShaderLength = 0;
  for (auto BI = F.begin(), BE = F.end(); BI != BE;) {
    BasicBlock *currentBB = &(*BI);
    ++BI;
    m_ShaderLength += currentBB->size();
  }

  bool changed = false;
  for (auto BI = F.begin(), BE = F.end(); BI != BE;) {
    BasicBlock *currentBB = &(*BI);
    ++BI;
    changed |= processBlock(currentBB);
  }
  return changed;
}

// Calculates whether the given 'use' evaluates to zero given that 'inst' is known to
// evaluate to zero.
bool EarlyOutPatterns::FoldsToZero(const Instruction *inst, const Value *use,
                                   const DenseSet<const Value *> &FoldedVals) {
  auto isZero = [](const Value *V) { return isa<ConstantFP>(V) && cast<ConstantFP>(V)->isZero(); };

  auto geZero = [](const Value *V) {
    if (auto *CFP = dyn_cast<ConstantFP>(V)) {
      auto &APF = CFP->getValueAPF();
      if (CFP->getType()->isDoubleTy())
        return APF.convertToDouble() >= 0.0;
      else if (CFP->getType()->isFloatTy())
        return APF.convertToFloat() >= 0.0f;
    }

    return false;
  };

  if (auto *binInst = dyn_cast<BinaryOperator>(use)) {
    switch (binInst->getOpcode()) {
    case Instruction::FMul:
    case Instruction::And:
      return true;
      // watch out for the zero in the denominator
    case Instruction::FDiv:
      return inst != binInst->getOperand(1);
    case Instruction::FSub:
      return isZero(binInst->getOperand(0)) || isZero(binInst->getOperand(1));
    default:
      return false;
    }
  } else if (dyn_cast<BitCastInst>(use)) {
    return true;
  } else if (auto *SI = dyn_cast<SelectInst>(use)) {
    // Assuming %x is 0, if the other operand is also
    // 0 the result of the select must be 0 as well.

    // select i1 %p, float 0.0, float %x
    bool zero0 = isZero(SI->getTrueValue());
    // select i1 %p, float %x, float 0.0
    bool zero1 = isZero(SI->getFalseValue());

    if (zero0 || zero1)
      return true;

    // If we have previously visited this select with a
    // folded value, check the map and allow the
    // select to be folded.
    auto *OtherOp = (inst == SI->getTrueValue()) ? SI->getFalseValue() : SI->getTrueValue();

    return FoldedVals.count(OtherOp) != 0;
  } else if (auto *CI = dyn_cast<CallInst>(use)) {
    // if x == 0
    switch (GetOpCode(CI)) {
      // max(0, x) or min(x, 0) == 0
    case llvm_max:
      return isZero(CI->getArgOperand(0)) || isZero(CI->getArgOperand(1));
    case llvm_min:
      return geZero(CI->getArgOperand(0)) || geZero(CI->getArgOperand(1));

      // Useful in matching dp3_sat
    case llvm_fsat:
      return true;
    default:
      return false;
    }
  } else if (auto *inst = dyn_cast<Instruction>(use)) {
    if (inst->getOpcode() == Instruction::SExt) {
      return true;
    }
  }

  return false;
}

// Count the number of instruction in the new block created if the ratio of instruction duplicated
// by instruction skipped is greater than the threshold return false
bool EarlyOutPatterns::EarlyOutBenefit(const Instruction *earlyOutInst, const DenseSet<const Value *> &FoldedVals,
                                       const unsigned int ratioNeeded) {
  auto *BB = earlyOutInst->getParent();

  unsigned int numberOfInstruction = 0;
  unsigned int numberOfInstructionDuplicated = 0;

  DenseSet<const Value *> instDuplicated;
  instDuplicated.insert(BB->getTerminator());

  for (auto it = BB->rbegin(); &(*it) != earlyOutInst; ++it) {
    numberOfInstruction++;
    const Instruction *inst = &(*it);

    if (FoldedVals.count(inst) != 0)
      continue;

    bool instNeeded = false;

    // We can't throw away side effects
    if (inst->mayWriteToMemory()) {
      instNeeded = true;
    } else {
      for (auto *UI : inst->users()) {
        if (auto *useInst = dyn_cast<Instruction>(UI)) {
          // We must also keep the instruction if its use has
          // escaped into another BB or, transitively, because
          // its user must be kept.
          if (useInst->getParent() != BB || instDuplicated.count(useInst) != 0) {
            instNeeded = true;
            break;
          }
        }
      }
    }

    if (instNeeded) {
      bool noOp = false;
      if (inst->getOpcode() == Instruction::FAdd) {
        // x + 0 = x, should be folded so don't add it
        // to the count.
        if (FoldedVals.count(inst->getOperand(0)) != 0 || FoldedVals.count(inst->getOperand(1)) != 0) {
          noOp = true;
        }
      }
      if (!noOp) {
        numberOfInstructionDuplicated++;
      }
      instDuplicated.insert(inst);
    }
  }

  return numberOfInstructionDuplicated * ratioNeeded <= numberOfInstruction;
}

void EarlyOutPatterns::foldFromAdd(SmallVector<Instruction *, 4> &Values, Instruction *&NewInsertPoint) {
  // if the sample has only one channel
  if (Values.size() == 1) {
    // if it has one use which is a add then try to fold from the add
    if (Values[0]->hasOneUse()) {
      BinaryOperator *bin = dyn_cast<BinaryOperator>(*Values[0]->user_begin());
      if (bin && bin->getOpcode() == Instruction::FAdd) {
        if (trackAddSources(bin)) {
          // try to fold the result of the add instead of the sample_C
          Values[0] = bin;
          NewInsertPoint = bin;
        }
      }
    }
  }
}

Instruction *EarlyOutPatterns::moveToDef(Instruction *Def, ArrayRef<Instruction *> Users) {
  Instruction *insertPoint = Def;

  for (auto it : Users) {
    // has a bitcast between extractelement and sample* instruction.
    // need to move the bitcast too.
    if (it->getOperand(0) != Def) {
      auto bitcast = cast<Instruction>(it->getOperand(0));
      bitcast->moveBefore(insertPoint->getNextNode());
      insertPoint = bitcast;
    }
  }
  for (auto it : Users) {
    // move all the users right after the def instruction for simplicity
    it->moveBefore(insertPoint->getNextNode());
    insertPoint = it;
  }

  return insertPoint;
}

bool EarlyOutPatterns::isSplitProfitable(const Instruction *Root, ArrayRef<Instruction *> Values,
                                         const DenseSet<const Value *> &FoldedVals, const unsigned FoldThreshold,
                                         const unsigned FoldThresholdMultiChannel, const unsigned RatioNeeded) {
  const unsigned NumInstFolded = FoldedVals.size();

  const bool SplitProfitable = (NumInstFolded > FoldThreshold) &&
                               // Check if we folded, we need a higher threshold if we have to check more channels
                               (Values.size() == 1 || NumInstFolded > FoldThresholdMultiChannel) &&
                               EarlyOutBenefit(Root, FoldedVals, RatioNeeded);

  return SplitProfitable;
}

// Once a candidate position 'Root' has been determined to be
// a profitable splitting point, generate the == 0 comparison
// and split the basic block at that point.
BasicBlock *EarlyOutPatterns::cmpAndSplitAtPoint(Instruction *Root, ArrayRef<Instruction *> Values,
                                                 const DenseSet<const Value *> &FoldedVals) {
  IRBuilder<> builder(Root->getNextNode());
  Instruction *splitCondition = nullptr;

  if (Values[0]->getType()->isIntOrIntVectorTy()) {
    splitCondition =
        cast<Instruction>(builder.CreateICmp(ICmpInst::ICMP_EQ, Values[0], ConstantInt::get(Values[0]->getType(), 0)));
  } else {
    splitCondition = cast<Instruction>(builder.CreateFCmpOEQ(Values[0], ConstantFP::get(Values[0]->getType(), 0.0)));
  }

  for (unsigned int i = 1; i < Values.size(); i++) {
    if (Values[i]->getType()->isIntOrIntVectorTy()) {
      Value *cmp = builder.CreateICmp(ICmpInst::ICMP_EQ, Values[i], ConstantInt::get(Values[i]->getType(), 0));
      splitCondition = cast<Instruction>(builder.CreateAnd(splitCondition, cmp));
    } else {
      Value *cmp = builder.CreateFCmpOEQ(Values[i], ConstantFP::get(Values[i]->getType(), 0.0));
      splitCondition = cast<Instruction>(builder.CreateAnd(splitCondition, cmp));
    }
  }

  auto *BB = SplitBasicBlock(splitCondition, FoldedVals);
  return BB;
}

BasicBlock *EarlyOutPatterns::tryFoldAndSplit(ArrayRef<Instruction *> Values, Instruction *Root,
                                              const unsigned FoldThreshold, const unsigned FoldThresholdMultiChannel,
                                              const unsigned RatioNeeded) {
  if (Values.empty())
    return nullptr;

  auto FoldedVals = tryAndFoldValues(Values);

  const bool SplitProfitable =
      isSplitProfitable(Root, Values, FoldedVals, FoldThreshold, FoldThresholdMultiChannel, RatioNeeded);

  return SplitProfitable ? cmpAndSplitAtPoint(Root, Values, FoldedVals) : nullptr;
}

bool EarlyOutPatterns::canOptimizeDotProduct(SmallVector<Instruction *, 4> &Values, Instruction *I) {
  Values.push_back(I);
  return true;
}

// Matches the llvm instruction pattern we generate after decomposing
// a dot product.
bool EarlyOutPatterns::DotProductMatch(const Instruction *I) {
  if (I->getOpcode() != Instruction::FAdd)
    return false;

  using namespace PatternMatch;

  Value *X1 = nullptr;
  Value *Y1 = nullptr;
  Value *Z1 = nullptr;
  Value *X2 = nullptr;
  Value *Y2 = nullptr;
  Value *Z2 = nullptr;

  // dp3
  if (match(I, m_FAdd(m_FMul(m_Value(Z1), m_Value(Z2)),
                      m_FAdd(m_FMul(m_Value(X1), m_Value(X2)), m_FMul(m_Value(Y1), m_Value(Y2)))))) {
    return true;
  }
  if (match(I, m_FAdd(m_FAdd(m_FMul(m_Value(X1), m_Value(X2)), m_FMul(m_Value(Y1), m_Value(Y2))),
                      m_FMul(m_Value(Z1), m_Value(Z2))))) {
    return true;
  }
  return false;
}

// Does is a dot product pattern the source of this instruction?
bool EarlyOutPatterns::DotProductSourceMatch(const Instruction *I) {
  if (auto *Src = dyn_cast<Instruction>(I->getOperand(0)))
    return DotProductMatch(Src);

  return false;
}

bool EarlyOutPatterns::canOptimizeMulMaxMatch(SmallVector<Instruction *, 4> &Values, Instruction *I) {
  // %a = fmul fast float %b, %b

  //   some other instructions that can be skipped

  // %mul_xa = fmul fast float %x, %a
  // %mul_ya = fmul fast float %y, %a
  // %mul_za = fmul fast float %z, %a
  // %result_a = call fast float @llvm.maxnum.f32(float% mul_xa, float 0.000000e+00)
  // %result_b = call fast float @llvm.maxnum.f32(float% mul_ya, float 0.000000e+00)
  // %result_c = call fast float @llvm.maxnum.f32(float% mul_za, float 0.000000e+00)

  if (I->getOpcode() != Instruction::FMul || I->getOperand(0) != I->getOperand(1))
    return false;

  for (auto iter = I->user_begin(); iter != I->user_end(); iter++) {
    if (Instruction *mulInst = dyn_cast<Instruction>(*iter)) {
      if (mulInst->getOpcode() != Instruction::FMul || !mulInst->hasOneUse()) {
        return false;
      }
      CallInst *maxInst = dyn_cast<CallInst>(*mulInst->user_begin());
      if (maxInst && GetOpCode(maxInst) == llvm_max) {
        ConstantFP *c = dyn_cast<ConstantFP>(maxInst->getOperand(1));
        if (c && c->isZero()) {
          continue;
        }
      }
      return false;
    }
  }
  return true;
}

bool EarlyOutPatterns::canOptimizeSelectOutput(SelectInst *SI) {
  if (FCmpInst *FC = dyn_cast<FCmpInst>(SI->getCondition())) {
    ConstantFP *src1 = dyn_cast<ConstantFP>(FC->getOperand(1));
    if (FC->getPredicate() != FCmpInst::FCMP_OGT || !FC->hasOneUse() || !src1 || !src1->isZero()) {
      return false;
    }
    return true;
  }
  return false;
}

bool EarlyOutPatterns::canOptimizeNdotL(SmallVector<Instruction *, 4> &Values, FCmpInst *FC) {
  // this function checks the lighting pattern -
  //     in short, the shader has a dot(N, L), multiply it with color, and max with 0.
  // if so, we might benefit from checking the dot(N, L) > 0 before calculating the color

  // LLVM example:
  // %319 = from dp3
  // %res_s231 = fcmp fast ogt float %319, 0.000000e+00    -> function input parameter FC
  // %selResi32_s232 = sext i1 %res_s231 to i32
  // %res_s246 = and i32 % 339, %selResi32_s232
  // % res_s247 = and i32 % 340, %selResi32_s232
  // % res_s248 = and i32 % 341, %selResi32_s232
  // % 342 = bitcast i32 %res_s246 to float
  // % 343 = bitcast i32 %res_s247 to float
  // % 344 = bitcast i32 %res_s248 to float
  // % res_s249 = fmul fast float %res_s224, % 342
  // % res_s250 = fmul fast float %res_s225, % 343
  // % res_s251 = fmul fast float %res_s226, % 344
  // % 345 = call fast float @llvm.maxnum.f32(float %res_s249, float 0.000000e+00)
  // % 346 = call fast float @llvm.maxnum.f32(float %res_s250, float 0.000000e+00)
  // % 347 = call fast float @llvm.maxnum.f32(float %res_s251, float 0.000000e+00)

  // ========================================================================================

  // check if FC is comparing with 0
  // %res_s231 = fcmp fast ogt float %319, 0.000000e+00, !dbg !278
  ConstantFP *src1 = dyn_cast<ConstantFP>(FC->getOperand(1));
  if (FC->getPredicate() != FCmpInst::FCMP_OGT || !FC->hasOneUse() || !src1 || !src1->isZero()) {
    return false;
  }

  // check if FC is from a dp3
  Instruction *src0 = dyn_cast<Instruction>(FC->getOperand(0));
  if (!src0 || !DotProductMatch(src0)) {
    return false;
  }

  // check if FC is followed by and+mul+max

  // sext is needed between "fcmp" and "and".
  // also the result should have 3 uses - x,y,z component of the light ray.
  Instruction *sextInst = dyn_cast<Instruction>(*FC->user_begin());
  if (!sextInst || sextInst->getOpcode() != Instruction::SExt || !sextInst->hasNUses(3)) {
    return false;
  }

  for (auto iter = sextInst->user_begin(); iter != sextInst->user_end(); iter++) {
    //  %res_s246 = and i32 %339, %selResi32_s232
    BinaryOperator *andInst = dyn_cast<BinaryOperator>(*iter);
    if (!andInst || andInst->getOpcode() != BinaryOperator::And || !andInst->hasOneUse()) {
      return false;
    }

    // % 342 = bitcast i32 %res_s246 to float
    BitCastInst *bitCastInst = dyn_cast<BitCastInst>(*andInst->user_begin());
    if (!bitCastInst || !bitCastInst->hasOneUse()) {
      return false;
    }

    Instruction *tempInst = dyn_cast<Instruction>(*bitCastInst->user_begin());
    while (tempInst && tempInst->hasOneUse()) {
      // % 345 = call fast float @llvm.maxnum.f32(float %res_s249, float 0.000000e+00)
      CallInst *CI = dyn_cast<CallInst>(tempInst);
      if (CI && GetOpCode(CI) == llvm_max) {
        ConstantFP *maxSrc1 = dyn_cast<ConstantFP>(CI->getOperand(1));
        if (maxSrc1 && maxSrc1->isZero()) {
          // found max with 0. do the optimization
          break;
        } else {
          return false;
        }
      } else if (tempInst->getOpcode() == BinaryOperator::FMul) {
        // if it is a fmul, keep going down to see if we can find a max
        tempInst = dyn_cast<Instruction>(*tempInst->user_begin());
        continue;
      }

      // not max, not mul, skip the optimization
      return false;
    }
  }

  // find all instructions contribute to FC and safely move them up to skip as many instructions as possible after early
  // out
  DenseSet<llvm::Instruction *> Scheduled;
  Scheduled.clear();
  BasicBlock *BB = FC->getParent();
  Instruction *InsertPos = &*BB->getFirstInsertionPt();
  safeScheduleUp(BB, cast<Value>(FC), InsertPos, std::move(Scheduled));

  return true;
}

bool EarlyOutPatterns::canOptimizeSampleInst(SmallVector<Instruction *, 4> &Channels, GenIntrinsicInst *GII) {
  auto ID = GII->getIntrinsicID();

  // -- Pattern, we are looking for sample instructions followed
  // by a large number of instructions which can be folded
  if (ID == GenISAIntrinsic::GenISA_sampleLptr || ID == GenISAIntrinsic::GenISA_sampleLCptr ||
      ID == GenISAIntrinsic::GenISA_sampleptr) {
    bool canOptimize = false;
    for (auto I : GII->users()) {
      // check patterns:
      // % 281 = call fast <4 x float> @llvm.genx.GenISA.sampleptr....
      // % bc1 = bitcast <4 x float> % 281 to <4 x i32>, !dbg !187
      // % 287 = extractelement <4 x i32> %bc1, i32 1, !dbg !187
      //                or
      // % 280 = call fast <4 x float> @llvm.genx.GenISA.sampleptr....
      // % 285 = extractelement <4 x float> % 280, i32 0, !dbg !182
      if (auto *bitCast = dyn_cast<BitCastInst>(I)) {
        if (bitCast->hasOneUse()) {
          I = *bitCast->user_begin();
        }
      }
      if (auto *extract = dyn_cast<ExtractElementInst>(I)) {
        if (GII->getParent() == extract->getParent() && isa<ConstantInt>(extract->getIndexOperand())) {
          Channels.push_back(extract);
          canOptimize = true;
          continue;
        }
      }
      canOptimize = false;
      break;
    }

    if (ID == GenISAIntrinsic::GenISA_sampleLCptr) {
      if (Channels.size() != 1 || !cast<ConstantInt>(Channels[0]->getOperand(1))->isZero()) {
        // for now allow multiple channels for everything except SampleLCptr
        // to reduce the scope
        canOptimize = false;
      }
    }

    // limit the number of channles to check to 3 for now
    if (Channels.size() > 3) {
      canOptimize = false;
    }

    return canOptimize;
  }

  return false;
}

enum class CS_PATTERNS {
#define EARLY_OUT_CS_PATTERN(Name, Val) Name = Val,
#include "igc_regkeys_enums_defs.h"
  EARLY_OUT_CS_PATTERNS
#undef EARLY_OUT_CS_PATTERN
#undef EARLY_OUT_CS_PATTERNS
};

enum class PS_PATTERNS {
#define EARLY_OUT_PS_PATTERN(Name, Val) Name = Val,
#include "igc_regkeys_enums_defs.h"
  EARLY_OUT_PS_PATTERNS
#undef EARLY_OUT_PS_PATTERN
#undef EARLY_OUT_PS_PATTERNS
};

bool EarlyOutPatterns::processBlock(BasicBlock *BB) {
  bool Changed = false;
  bool BBSplit = true;
  bool SamplePatternEnable = false;
  bool DPMaxPatternEnable = false;
  bool DPFSatPatternEnable = false;
  bool NdotLPatternEnable = false;
  bool DirectOutputPatternEnable = false;
  bool MulMaxMatchEnable = false;
  bool SelectFcmpPatternEnable = false;

  // Each pattern below is given a bit to toggle on/off
  // to isolate the performance for each individual pattern.
  if (m_ctx->type == ShaderType::COMPUTE_SHADER) {
    auto PatEnable = [](CS_PATTERNS Pat) { return (IGC_GET_FLAG_VALUE(EarlyOutPatternSelectCS) & (uint32_t)Pat) != 0; };

    SamplePatternEnable = PatEnable(CS_PATTERNS::SamplePatternEnable);
    DPMaxPatternEnable = PatEnable(CS_PATTERNS::DPMaxPatternEnable);
    DPFSatPatternEnable = PatEnable(CS_PATTERNS::DPFSatPatternEnable);
    NdotLPatternEnable = PatEnable(CS_PATTERNS::NdotLPatternEnable);
    SelectFcmpPatternEnable = PatEnable(CS_PATTERNS::SelectFcmpPatternEnable);
  } else if (m_ctx->type == ShaderType::PIXEL_SHADER) {
    auto PatEnable = [](PS_PATTERNS Pat) { return (IGC_GET_FLAG_VALUE(EarlyOutPatternSelectPS) & (uint32_t)Pat) != 0; };

    SamplePatternEnable = PatEnable(PS_PATTERNS::SamplePatternEnable);
    DPMaxPatternEnable = PatEnable(PS_PATTERNS::DPMaxPatternEnable);
    DPFSatPatternEnable = PatEnable(PS_PATTERNS::DPFSatPatternEnable);
    NdotLPatternEnable = PatEnable(PS_PATTERNS::NdotLPatternEnable);
    DirectOutputPatternEnable = PatEnable(PS_PATTERNS::DirectOutputPatternEnable);
    MulMaxMatchEnable = PatEnable(PS_PATTERNS::MulMaxMatchEnable);
  }

  while (BBSplit) {
    BBSplit = false;
    Instruction *nextII = nullptr;
    for (auto iter = BB->begin(); iter != BB->end(); iter++) {
      if (nextII && iter != nextII->getIterator()) {
        iter = nextII->getIterator();
      }
      Instruction &II = *iter;
      nextII = II.getNextNode();
      SmallVector<Instruction *, 4> Values;
      bool OptCandidate = false;
      Instruction *Root = &II;

      unsigned FoldThreshold = 5;
      unsigned FoldThresholdMultiChannel = 10;
      unsigned RatioNeeded = 10;

      if (auto *SI = dyn_cast<SampleIntrinsic>(&II)) {
        OptCandidate = SamplePatternEnable && canOptimizeSampleInst(Values, SI);

        if (!OptCandidate)
          continue;

        Root = moveToDef(SI, Values);
        foldFromAdd(Values, Root);
      } else if (GetOpCode(&II) == llvm_max) {
        auto *CI = cast<CallInst>(&II);
        OptCandidate = DPMaxPatternEnable && DotProductSourceMatch(CI) && canOptimizeDotProduct(Values, &II);
        // Lower the ratio threshold for this case
        FoldThreshold = 9;
        RatioNeeded = 3;
      } else if (auto *GII = dyn_cast<GenIntrinsicInst>(&II)) {
        Value *SI = nullptr;
        int outputCount = 0;
        switch (GII->getIntrinsicID()) {
        case GenISAIntrinsic::GenISA_fsat:
          OptCandidate = DPFSatPatternEnable && DotProductSourceMatch(GII) && canOptimizeDotProduct(Values, &II);
          break;
        case GenISAIntrinsic::GenISA_OUTPUT:
        case GenISAIntrinsic::GenISA_OUTPUTPS:
          for (auto iter = GII->getParent()->begin(); iter != GII->getParent()->end(); iter++) {
            GenIntrinsicInst *outI = dyn_cast<GenIntrinsicInst>(iter);
            if (outI && outI->getIntrinsicID() == GetOutputPSIntrinsic(m_ctx->platform)) {
              outputCount++;
            }
          }
          // only handle cases with one output
          if (outputCount != 1)
            continue;

          OptCandidate = DirectOutputPatternEnable && canOptimizeDirectOutput(Values, GII, SI, m_ShaderLength);

          if (!OptCandidate)
            continue;

          Root = moveToDef(cast<Instruction>(SI), Values);

          FoldThreshold = 1;
          FoldThresholdMultiChannel = 1;
          RatioNeeded = 1;
          break;
        default:
          break;
        }
      } else if (auto *FC = dyn_cast<FCmpInst>(&II)) {
        OptCandidate = NdotLPatternEnable && canOptimizeNdotL(Values, FC) && canOptimizeDotProduct(Values, &II);
      } else if (II.getOpcode() == Instruction::FMul) {
        OptCandidate = MulMaxMatchEnable && canOptimizeMulMaxMatch(Values, &II);
        if (OptCandidate) {
          Values.push_back(&II);
        }
      } else if (auto *SI = dyn_cast<SelectInst>(&II)) {
        OptCandidate = SelectFcmpPatternEnable && canOptimizeSelectOutput(SI);
        if (OptCandidate)
          Values.push_back(&II);
      }
      if (OptCandidate) {
        BasicBlock *BB1 = tryFoldAndSplit(Values, Root, FoldThreshold, FoldThresholdMultiChannel, RatioNeeded);
        BBSplit = (BB1 != nullptr);
        if (BBSplit) {
          BB = BB1;
          break;
        }
      }
    }

    Changed |= BBSplit;
  }

  return Changed;
}

unsigned int EarlyOutPatterns::shortPathToOutput(Value *Val, unsigned int limit) {
  if (limit == 0) {
    return 0;
  }
  limit--;

  if (Instruction *inst = dyn_cast<Instruction>(Val)) {
    unsigned int maxDepth = 0;
    for (unsigned int i = 0; i < inst->getNumOperands(); i++) {
      maxDepth = MAX(maxDepth, shortPathToOutput(inst->getOperand(i), limit));
    }
    return 1 + maxDepth;
  } else {
    return 0;
  }
}

bool EarlyOutPatterns::canOptimizeDirectOutput(SmallVector<Instruction *, 4> &Values, GenIntrinsicInst *GII, Value *&SI,
                                               unsigned int ShaderLength) {
#define MAX_FMUL_VEC_SIZE 8
#define PATH_TO_OUTPUT_LIMIT 5

  // Find the case where most calculation goes to .w channel, and very few instructions are used to calculate .xyz.
  if (dyn_cast<Instruction>(GII->getOperand(3))) {
    unsigned int findex = 0;
    smallvector<llvm::Value *, MAX_FMUL_VEC_SIZE> fmulVec;
    fmulVec.push_back(GII->getOperand(3));

    // look for cases with output comes from sample and up to 3 levels of fmul
    // Also the sample srcs are either constant or inputVec.
    // We can then add an earlyOut condition to check the sample output.
    // If the sample result = 0, skip all the instructions contribute to the other src of mul.
    while (findex < fmulVec.size() && findex < MAX_FMUL_VEC_SIZE) {
      if (ExtractElementInst *eeInst = dyn_cast<ExtractElementInst>(fmulVec[findex])) {
        if (SampleIntrinsic *genIntr = dyn_cast<SampleIntrinsic>(eeInst->getOperand(0))) {
          for (unsigned int i = 0; i < genIntr->getNumOperands(); i++) {
            if (dyn_cast<Constant>(genIntr->getOperand(i)))
              continue;
            else if (GenIntrinsicInst *intrinsic = dyn_cast<GenIntrinsicInst>(genIntr->getOperand(i))) {
              if (intrinsic->getIntrinsicID() == GenISAIntrinsic::GenISA_DCL_inputVec) {
                continue;
              }
            }
            return false;
          }
          Values.push_back(eeInst);
          SI = genIntr;
          break;
        }
      } else if (BinaryOperator *fmulInst = dyn_cast<BinaryOperator>(fmulVec[findex])) {
        if (fmulInst->getOpcode() == Instruction::FMul) {
          fmulVec.push_back(fmulInst->getOperand(0));
          fmulVec.push_back(fmulInst->getOperand(1));
        }
      }
      findex++;
    }
  }

  if (SI == nullptr)
    return false;

  // if the .xyz path are all short, and the shader is not too short (10+ times the xyz path)
  if (shortPathToOutput(GII->getOperand(0), PATH_TO_OUTPUT_LIMIT) < PATH_TO_OUTPUT_LIMIT &&
      shortPathToOutput(GII->getOperand(1), PATH_TO_OUTPUT_LIMIT) < PATH_TO_OUTPUT_LIMIT &&
      shortPathToOutput(GII->getOperand(2), PATH_TO_OUTPUT_LIMIT) < PATH_TO_OUTPUT_LIMIT &&
      ShaderLength > PATH_TO_OUTPUT_LIMIT * 10) {
    return true;
  }
  return false;
}

bool EarlyOutPatterns::trackAddSources(BinaryOperator *addInst) {
  for (unsigned int i = 0; i < 2; i++) {
    Value *source = addInst->getOperand(i);
    if (BinaryOperator *binSrc = dyn_cast<BinaryOperator>(source)) {
      if (binSrc->getOpcode() == Instruction::FAdd) {
        if (trackAddSources(binSrc)) {
          continue;
        }
      }
    } else if (ExtractElementInst *ext = dyn_cast<ExtractElementInst>(source)) {
      if (ConstantInt *index = dyn_cast<ConstantInt>(ext->getIndexOperand())) {
        if (index->isZero()) {
          if (GenIntrinsicInst *genIntr = dyn_cast<GenIntrinsicInst>(ext->getVectorOperand())) {
            GenISAIntrinsic::ID ID = genIntr->getIntrinsicID();
            if (ID == GenISAIntrinsic::GenISA_sampleLCptr || ID == GenISAIntrinsic::GenISA_sampleptr) {
              continue;
            }
          }
        }
      }
    }
    // if any source is not a sample_LC or a add coming from sampleLC we shouldn't fold the value
    return false;
  }
  return true;
}

// Recursively walk the users of 'Values' and, under the assumption the the 'Values' are
// == 0, determine which other instructions could be folded away to 0 as well.
DenseSet<const Value *> EarlyOutPatterns::tryAndFoldValues(ArrayRef<Instruction *> Values) {
  std::function<void(const Instruction *, DenseSet<const Value *> &)> fold =
      [&fold](const Instruction *inst, DenseSet<const Value *> &FoldedVals) {
        for (auto UI : inst->users()) {
          if (auto *useInst = dyn_cast<Instruction>(UI)) {
            if (useInst->getParent() == inst->getParent()) {
              if (FoldsToZero(inst, useInst, FoldedVals)) {
                FoldedVals.insert(useInst);
                fold(useInst, FoldedVals);
              }
            }
          }
        }
      };

  // try to fold assuming all the channels are 0.f
  // right now only fold with 0, to replace with a map in case we want to
  // support more values
  DenseSet<const Value *> FoldedVals;
  for (auto I : Values) {
    fold(I, FoldedVals);
  }

  return FoldedVals;
}

Instruction *EarlyOutPatterns::FindLastFoldedValue(const DenseSet<const Value *> &FoldedVals, BasicBlock *currentBB) {
  // traverse the block backwards and find the last folded value
  for (BasicBlock::reverse_iterator re = currentBB->rbegin(), ri = currentBB->rend(); re != ri; ++re) {
    if (FoldedVals.count(&(*re))) {
      re--;
      return &*re;
    }
  }
  // default to return the last instruction in the block
  return &*(currentBB->rbegin());
}

// return the new block where the code after inst was moved
BasicBlock *EarlyOutPatterns::SplitBasicBlock(Instruction *inst, const DenseSet<const Value *> &FoldedVals) {
  IRBuilder<> builder(inst->getContext());
  BasicBlock *currentBB = inst->getParent();
  Instruction *lastFoldedInst = FindLastFoldedValue(FoldedVals, currentBB);
  BasicBlock *elseBlock = currentBB->splitBasicBlock(inst->getNextNode(), "EO_else");
  currentBB->getTerminator()->eraseFromParent();
  BasicBlock *endifBlock = elseBlock->splitBasicBlock(lastFoldedInst->getIterator(), "EO_endif");
  currentBB->replaceSuccessorsPhiUsesWith(endifBlock);

  // split the blocks
  ValueToValueMapTy VMap;

  BasicBlock *ifBlock = CloneBasicBlock(elseBlock, VMap);
  ifBlock->setName(VALUE_NAME("EO_IF"));
  IGCLLVM::insertBasicBlock(currentBB->getParent(), endifBlock->getIterator(), ifBlock);

  for (auto &Inst : *ifBlock)
    RemapInstruction(&Inst, VMap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);

  // create phi instruction
  for (auto II = elseBlock->begin(), IE = elseBlock->end(); II != IE; ++II) {
    PHINode *newPhi = nullptr;
    for (auto UI = II->user_begin(), UE = II->user_end(); UI != UE; ++UI) {
      if (Instruction *useInst = dyn_cast<Instruction>(*UI)) {
        if (useInst->getParent() != elseBlock) {
          newPhi = PHINode::Create(II->getType(), 2, "", &(*endifBlock->begin()));
          II->replaceUsesOutsideBlock(newPhi, elseBlock);
          newPhi->addIncoming(&(*II), elseBlock);
          newPhi->addIncoming(VMap[&(*II)], ifBlock);
          break;
        }
      }
    }
  }
  // replace the folded values with 0
  for (auto it : FoldedVals) {
    if (it->getType()->isIntOrIntVectorTy()) {
      VMap[it]->replaceAllUsesWith(ConstantInt::get(it->getType(), 0));
    } else {
      VMap[it]->replaceAllUsesWith(ConstantFP::get(it->getType(), 0.0));
    }
  }
  // branching
  builder.SetInsertPoint(currentBB);
  builder.CreateCondBr(inst, ifBlock, elseBlock);
  return elseBlock;
}

///////////////////////////////////////////////////////////////////////
// Lower llvm.fma intrinsic to mul/add for exposing more optimizations
// if the shader allows fp unsafe optimizations.
///////////////////////////////////////////////////////////////////////

class LowerFma : public FunctionPass {
public:
  static char ID;

  LowerFma() : m_CheckAllowContractFlag(false), FunctionPass(ID) {}

  LowerFma(bool checkAllowContractFlag) : m_CheckAllowContractFlag(checkAllowContractFlag), FunctionPass(ID) {}

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const { AU.setPreservesCFG(); }

  StringRef getPassName() const { return "LowerFma"; }

  bool runOnFunction(Function &F);

private:
  // Lower fma only if it has `AllowContract` fast math flag
  bool m_CheckAllowContractFlag;
};

char LowerFma::ID = 0;

FunctionPass *IGC::CreateLowerFmaPass(bool checkAllowContractFlag) { return new LowerFma(checkAllowContractFlag); }

bool LowerFma::runOnFunction(Function &F) {
  bool changed = false;
  for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++) {
    for (auto II = BI->begin(); II != BI->end();) {
      IntrinsicInst *fmad = dyn_cast<IntrinsicInst>(II++);
      if (fmad && fmad->getIntrinsicID() == Intrinsic::fma && (!m_CheckAllowContractFlag || fmad->hasAllowContract())) {
        changed = true;
        IRBuilder<> irb(fmad);
        irb.setFastMathFlags(fmad->getFastMathFlags());
        Value *v = irb.CreateFMul(fmad->getArgOperand(0), fmad->getArgOperand(1));
        v = irb.CreateFAdd(v, fmad->getArgOperand(2));
        fmad->replaceAllUsesWith(v);
        fmad->eraseFromParent();
      }
    }
  }
  return changed;
}

///////////////////////////////////////////////////////////////////////
// Hoist fp mul out of loop.
///////////////////////////////////////////////////////////////////////

class HoistFMulInLoopPass : public FunctionPass {
public:
  static char ID;

  HoistFMulInLoopPass();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<LoopInfoWrapperPass>();
  }

  StringRef getPassName() const { return "HoistFMulInLoopPass"; }

  bool runOnFunction(Function &F);

protected:
  CodeGenContext *m_ctx;
  LoopInfo *m_LI;
  llvm::BumpPtrAllocator Allocator;

  struct MulNode {
    llvm::Value *value = nullptr;
    struct MulNode *left = nullptr;
    struct MulNode *right = nullptr;
    llvm::Value *replace = nullptr;

    smallvector<llvm::Value *, 4> invariants;
    bool hasInvariant = false;
    bool visited = false;
    FastMathFlags FMF;
  };

  struct RootNode {
    llvm::Instruction *fsum;
    llvm::PHINode *phi;
    llvm::Value *phiNonZeroValue;
    unsigned phiNonZeroValueIdx;
    MulNode *root;
    RootNode(llvm::Instruction *_fsum, llvm::PHINode *_phi, llvm::Value *nonZero, unsigned idx, MulNode *_root)
        : fsum(_fsum), phi(_phi), phiNonZeroValue(nonZero), phiNonZeroValueIdx(idx), root(_root) {}
  };
  typedef llvm::DenseMap<llvm::Value *, MulNode *> MulToNodeMapTy;
  typedef smallvector<RootNode, 8> MulNodeVecTy;

  MulNode *addToNodeTree(llvm::Value *value, MulToNodeMapTy &nodeMap) {
    MulNode *node;
    auto it = nodeMap.find(value);

    if (it != nodeMap.end()) {
      node = (*it).second;
    } else {
      node = new (Allocator) MulNode();
      node->value = value;
      nodeMap[value] = node;
    }
    return node;
  }

  MulNode *visitFMul(Loop *loop, BinaryOperator *fmul, MulToNodeMapTy &nodeMap);

  bool isLeafNode(MulNode *node) { return node->left == nullptr && node->right == nullptr; }

  bool appendInvariants(MulNode *node, MulNode *child) {
    bool visited = false;
    if (child && child->visited) {
      visited = true;
      if (child->invariants.size()) {
        node->invariants.insert(node->invariants.end(), child->invariants.begin(), child->invariants.end());
      }
    }
    return visited;
  }

  bool isInvariantLeaf(MulNode *node) { return (node && node->hasInvariant && isLeafNode(node)); }

  void combineNode(MulNode *node, MulToNodeMapTy &nodeMap, bool isRoot = false);

  void combineInvariant(MulNodeVecTy &roots, MulToNodeMapTy &nodeMap) {
    for (auto &NI : roots) {
      combineNode(NI.root, nodeMap, true);
    }
  }

  void updateMulNode(MulNode *node, Instruction *use) {
    if (!isLeafNode(node)) {
      Instruction *inst;
      if (node->replace)
        inst = cast<Instruction>(node->replace);
      else
        inst = cast<Instruction>(node->value);
      updateMulNode(node->left, inst);
      updateMulNode(node->right, inst);
    }

    if (node->replace)
      use->replaceUsesOfWith(node->replace, node->value);
  }

  // get rid of multiply with loop invariants
  void updateMul(MulNodeVecTy &roots) {
    for (auto &NI : roots) {
      if (NI.root->hasInvariant || (isLeafNode(NI.root) && NI.root->replace)) {
        updateMulNode(NI.root, NI.fsum);
      }
    }
  }

  //
  // hoist multiply in loop with given pattern, from:
  //   sum = 0;
  //   loop {
  //     sum += x * ... * y * loopinvirant;
  //   }
  // to:
  //   sum = 0;
  //   loop {
  //     sum += x * ... * y;
  //   }
  //   sum = sum * loopinvirant
  bool hoistMulInLoops();

  llvm::BinaryOperator *dynCastFMul(llvm::Value *v) {
    llvm::BinaryOperator *bi = llvm::dyn_cast<llvm::BinaryOperator>(v);
    if (bi && bi->getOpcode() == llvm::Instruction::FMul) {
      return bi;
    }
    return nullptr;
  }

  bool hoistMulInLoop(llvm::Loop *loop, bool replacePhi = true);

  void updateOutLoopSumUse(llvm::Loop *loop, llvm::Instruction *fsum, const smallvector<llvm::Value *, 4> &invariants,
                           llvm::Value *nonZero, const FastMathFlags &FMF);
};

char HoistFMulInLoopPass::ID = 0;

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

#define PASS_FLAG "igc-hoist-fmul-in-loop-pass"
#define PASS_DESCRIPTION "Hoist FMul in Loop Pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HoistFMulInLoopPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper);
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass);
IGC_INITIALIZE_PASS_END(HoistFMulInLoopPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

FunctionPass *IGC::CreateHoistFMulInLoopPass() { return new HoistFMulInLoopPass(); }

HoistFMulInLoopPass::HoistFMulInLoopPass() : FunctionPass(ID) {
  initializeHoistFMulInLoopPassPass(*PassRegistry::getPassRegistry());
}

bool HoistFMulInLoopPass::runOnFunction(Function &F) {
  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  if (!IGC_IS_FLAG_ENABLED(EnableHoistMulInLoop) || m_ctx->type == ShaderType::VERTEX_SHADER) {
    return false;
  }

  bool changed = false;

  m_LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  changed = hoistMulInLoops();

  Allocator.Reset();

  return changed;
}

HoistFMulInLoopPass::MulNode *HoistFMulInLoopPass::visitFMul(Loop *loop, BinaryOperator *fmul,
                                                             MulToNodeMapTy &nodeMap) {
  MulNode *ret;
  auto MI = nodeMap.find(fmul);
  if (MI != nodeMap.end()) {
    ret = (*MI).second;
    return ret;
  }
  ret = addToNodeTree(fmul, nodeMap);
  ret->FMF = fmul->getFastMathFlags();

  for (int i = 0; i < 2; i++) {
    MulNode **pp;
    pp = i == 0 ? &ret->left : &ret->right;

    Value *src = fmul->getOperand(i);
    BinaryOperator *bop = dynCastFMul(src);
    if (bop && loop->contains(bop)) {
      *pp = visitFMul(loop, bop, nodeMap);
    } else {
      MulNode *leaf = addToNodeTree(src, nodeMap);
      leaf->FMF = fmul->getFastMathFlags();
      leaf->hasInvariant = loop->isLoopInvariant(src);
      *pp = leaf;
    }
  }
  ret->hasInvariant = ret->left->hasInvariant | ret->right->hasInvariant;
  return ret;
}

//
// There is a special case:
//     %82 = fmul %81, %inv
//   %83 = fmul %82, %inv
//   %85 = fmul %83, %84
//
//   %86 = fmul %78, %85
//   %87 = fmul %79, %85
//   %88 = fmul %80, %85
//   %89 = fmul %84, %82
//
//   %90 = fadd %86, ...
//   %91 = fadd %87, ...
//   %92 = fadd %88, ...
//   %93 = fadd %89, ...
//
//   %131 = fcmp olt %93, 0x3F...
// Here %82 is %81 * %inv, and %83 is %82 * %inv * %inv, and %90/%91/%92
// are products from %83, while %93 is from %82 and used for comparison.
// So we can optimize %90/%91/%92, while %82 need to be leave unmodified.
//
// The expression tree we built for %85 is:
/*        %85
          / \
        %83  \
        / \   \
      %81  ---%inv
   And we will try to recursively propogate %81 to %83 & %85, and remove
   %83 from the tree:
     ==>      %85              ==>      %85->%81
              / \                     [%inv, %inv]
             /   \
        %83->%81  \
        [%inv]     \
                  %inv
*/
void HoistFMulInLoopPass::combineNode(MulNode *node, MulToNodeMapTy &nodeMap, bool isRoot) {
  if (node->visited)
    return;

  if (isLeafNode(node)) {
    node->visited = true;
    if (node->hasInvariant)
      node->invariants.push_back(node->value);
    return;
  }

  if (node->left)
    combineNode(node->left, nodeMap);
  if (node->right)
    combineNode(node->right, nodeMap);

  appendInvariants(node, node->left);
  appendInvariants(node, node->right);

  if (isInvariantLeaf(node->left) && isInvariantLeaf(node->right)) {
    // both sources are invariants, hoist this node to invariants
    node->left = node->right = nullptr;
  } else if (isInvariantLeaf(node->left)) {
    // replace the mul reference with non-invariant source
    node->replace = node->value;
    node->value = node->right->value;

    if (isLeafNode(node->right)) {
      node->left = node->right = nullptr;
      node->hasInvariant = false;
    }
  } else if (isInvariantLeaf(node->right)) {
    // replace the mul reference with non-invariant source
    node->replace = node->value;
    node->value = node->left->value;

    if (isLeafNode(node->left)) {
      node->left = node->right = nullptr;
      node->hasInvariant = false;
    }
  }

  // check whether the product is used by other places
  if (!isRoot && !isLeafNode(node) && !node->replace && node->invariants.size()) {
    bool skipValue = false;
    Value *value;
    value = node->value;
    for (auto *UI : value->users()) {
      Instruction *ui = dyn_cast<Instruction>(UI);
      if (ui && (nodeMap.find(ui) == nodeMap.end() || ui->getOpcode() != Instruction::FMul)) {
        // the value is used by other places or used not as multiply,
        // we cannot change it since the results won't be correct
        skipValue = true;
        break;
      }
    }

    if (skipValue) {
      node->left = node->right = nullptr;
      node->hasInvariant = false;
      node->invariants.clear();
    }
  }

  node->visited = true;
}

// update references of sum result outside of the loop, need to multiply
// the sum result by loop invariant factors
void HoistFMulInLoopPass::updateOutLoopSumUse(Loop *loop, Instruction *fsum,
                                              const smallvector<llvm::Value *, 4> &invariants, Value *nonZero,
                                              const FastMathFlags &FMF) {
  SmallPtrSet<BasicBlock *, 4> bbSet;

  for (auto &UI : fsum->uses()) {
    Instruction *inst = cast<Instruction>(UI.getUser());
    if (loop->contains(inst))
      continue;

    if (bbSet.count(inst->getParent()))
      continue;
    bbSet.insert(inst->getParent());

    IRBuilder<> irb(inst);
    Instruction *insertPoint;
    bool isLCSSA = false;

    if (PHINode *phiUse = dyn_cast<PHINode>(inst)) {
      if (phiUse->getNumOperands() == 1) {
        // handle lcssa
        insertPoint = &*inst->getParent()->getFirstInsertionPt();
        isLCSSA = true;
      } else {
        // Use of the result is a phi node, we need to create the
        // mul in incoming edge block. So we need to break critical
        // edge in advance.
        BasicBlock *bb = phiUse->getIncomingBlock(UI);
        insertPoint = bb->getTerminator();
      }
    } else {
      insertPoint = inst;
    }
    irb.SetInsertPoint(insertPoint);
    irb.setFastMathFlags(FMF);

    Value *mulSrc = nullptr;

    // multiply all the factors together
    for (auto VI : invariants) {
      if (!mulSrc)
        mulSrc = VI;
      else
        mulSrc = irb.CreateFMul(mulSrc, VI, VALUE_NAME("hoist"));
    }

    if (mulSrc) {
      if (isLCSSA) {
        Value *v = irb.CreateFMul(inst, mulSrc, VALUE_NAME("hoist"));
        Value *w = v;
        if (nonZero)
          w = irb.CreateFAdd(v, nonZero, VALUE_NAME("hoist"));

        for (auto UI = inst->uses().begin(); UI != inst->uses().end();) {
          auto *usr = dyn_cast<Instruction>((*UI).getUser());
          auto &use = (*UI);
          ++UI;
          if (usr && usr != v) {
            use.set(w);
          }
        }
      } else {
        Value *v = irb.CreateFMul(fsum, mulSrc, VALUE_NAME("hoist"));
        if (nonZero)
          v = irb.CreateFAdd(v, nonZero, VALUE_NAME("hoist"));
        inst->replaceUsesOfWith(fsum, v);
      }
    }
  }
}

bool HoistFMulInLoopPass::hoistMulInLoop(Loop *loop, bool replacePhi) {
  BasicBlock *header = loop->getHeader();
  BasicBlock *body = loop->getLoopLatch();

  MulToNodeMapTy nodeMap;
  MulNodeVecTy roots;

  for (auto &II : *header) {
    PHINode *phi = dyn_cast<PHINode>(&II);
    if (!phi) {
      break;
    }

    if (!phi->getType()->isFloatTy() && !phi->getType()->isDoubleTy() && !phi->getType()->isHalfTy()) {
      continue;
    }

    // look for pattern inside loop:
    //   sum += x * y * ... * loop_invariant
    BinaryOperator *fsum;
    BinaryOperator *fmul = nullptr;
    Value *phiNonZeroValue = nullptr;
    unsigned phiNonZeroValueIdx = 0;

    if (replacePhi && phi->getNumIncomingValues() == 2) {
      // handle the cases where sum is not initialized to 0 before loop,
      // we need to add the value before the loop with sum results
      if (phi->getIncomingBlock(0) == body) {
        phiNonZeroValueIdx = 1;
      } else {
        phiNonZeroValueIdx = 0;
      }

      bool skip = false;
      BasicBlock *bb = phi->getIncomingBlock(phiNonZeroValueIdx);
      Loop *lp = loop;
      while (lp) {
        if (lp->contains(bb)) {
          skip = true;
          break;
        }
        lp = lp->getParentLoop();
      }

      if (!skip) {
        Value *v = phi->getIncomingValue(phiNonZeroValueIdx);
        ConstantFP *ci = dyn_cast<ConstantFP>(v);
        if (!ci || !ci->isZero()) {
          phiNonZeroValue = v;
          Constant *f0 = ConstantFP::get(v->getType(), 0.0);
          phi->setIncomingValue(phiNonZeroValueIdx, f0);
        }
      }
    }

    for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
      Value *v = phi->getIncomingValue(i);
      if (phi->getIncomingBlock(i) != body) {
        ConstantFP *ci = dyn_cast<ConstantFP>(v);
        // all incoming of phi need to be 0 except loop back edge
        if (!ci || !ci->isZero()) {
          fmul = nullptr;
          break;
        }
      } else {
        fsum = dyn_cast<BinaryOperator>(v);
        if (!fsum || !allowUnsafeMathOpt(m_ctx, *fsum) || fsum->getOpcode() != Instruction::FAdd) {
          break;
        }

        BinaryOperator *addsrc = nullptr;
        if (fsum->getOperand(0) == phi) {
          addsrc = dyn_cast<BinaryOperator>(fsum->getOperand(1));
        } else if (fsum->getOperand(1) == phi) {
          addsrc = dyn_cast<BinaryOperator>(fsum->getOperand(0));
        } else {
          break;
        }

        // skip if other add/phi result referece inside loop
        for (auto *UI : fsum->users()) {
          Instruction *ui = dyn_cast<Instruction>(UI);
          if (ui && loop->contains(ui) && UI != phi) {
            addsrc = nullptr;
            break;
          }
        }
        for (auto *UI : phi->users()) {
          Instruction *ui = dyn_cast<Instruction>(UI);
          if (ui && loop->contains(ui) && UI != fsum) {
            addsrc = nullptr;
            break;
          }
        }

        if (addsrc && addsrc->getOpcode() == Instruction::FMul && allowUnsafeMathOpt(m_ctx, *addsrc)) {
          fmul = addsrc;
        }
      }
    }

    if (fmul && loop->contains(fmul) && fmul->hasOneUse()) {
      MulNode *root = visitFMul(loop, fmul, nodeMap);
      if (root->hasInvariant) {
        roots.emplace_back(fsum, phi, phiNonZeroValue, phiNonZeroValueIdx, root);
      } else {
        if (phiNonZeroValue)
          phi->setIncomingValue(phiNonZeroValueIdx, phiNonZeroValue);
      }
    } else {
      if (phiNonZeroValue)
        phi->setIncomingValue(phiNonZeroValueIdx, phiNonZeroValue);
    }
  }

  if (roots.size() == 0) {
    // cannot find candidates
    return false;
  }

  combineInvariant(roots, nodeMap);

  updateMul(roots);

  for (auto &NI : roots) {
    // if we found values cannot be hoisted later, we need to
    // restore the phi if it's changed
    if (NI.root->invariants.size() == 0 && NI.phiNonZeroValue) {
      NI.phi->setIncomingValue(NI.phiNonZeroValueIdx, NI.phiNonZeroValue);
    }
  }

  for (auto &NI : roots) {
    if (NI.root->invariants.size() > 0 || (isLeafNode(NI.root) && NI.root->replace)) {
      updateOutLoopSumUse(loop, NI.phi, NI.root->invariants, NI.phiNonZeroValue, NI.root->FMF);
      updateOutLoopSumUse(loop, NI.fsum, NI.root->invariants, NI.phiNonZeroValue, NI.root->FMF);
    }
  }

  return true;
}

//
// hoist multiply in loop with given pattern, from:
//   sum = 0;
//   loop {
//     sum += x * ... * y * loopinvirant;
//   }
// to:
//   sum = 0;
//   loop {
//     sum += x * ... * y;
//   }
//   sum = sum * loopinvirant
bool HoistFMulInLoopPass::hoistMulInLoops() {
  bool changed = false;
  for (auto &LI : *m_LI) {
    Loop *L = &(*LI);

    changed |= hoistMulInLoop(L);

    for (auto &SL : L->getSubLoops()) {
      changed |= hoistMulInLoop(SL);
    }
  }
  return changed;
}
