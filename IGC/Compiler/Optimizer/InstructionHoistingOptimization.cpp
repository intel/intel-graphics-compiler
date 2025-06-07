/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InstructionHoistingOptimization.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "IGC/Compiler/CodeGenPublic.h"
#include "LLVM3DBuilder/BuiltinsFrontend.hpp"
#include "Probe/Assertion.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/PassInfo.h"
#include "llvm/PassRegistry.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
namespace IGC {
// clang-format off
/////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Hoisting instructions (sampler) to earlier place in code after loop
/// unroll The reason on doing this hoisting is because for sampler after LICM,
/// only instructions with loop-invariants can be hoisted, and sampler in the
/// loop is like:
/// 19:                                               ; preds = %281, %0
///    %20 = phi i32[0, %0], [%265, %281]
///    %21 = phi i32[0, %0], [%202, %281]
///    %22 = phi i32[0, %0], [%139, %281]
///    %23 = phi i32[0, %0], [%76,  %281]
///    %24 = phi i32[0, %0], [%283, %281]
///    %25 = phi i32[0, %0], [%284, %281]
///    %26 = phi i32[0, %0], [%285, %281]
///    %27 = add nsw i32 %26, %17
///    %28 = uitofp i32 %27 to float
///    %29 = uitofp i32 %18 to float
///    %30 = fadd fast float %28, 5.000000e-01
///    %31 = fadd fast float %29, 5.000000e-01
///    %32 = call fast <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904v4f32(<4 x float> addrspace(2555904)* %"b0,1", i32 832, i32 4, i1 false)
///    %33 = extractelement <4 x float> %32, i64 0
///    %34 = extractelement <4 x float> %32, i64 1
///    %35 = fmul fast float %30, %33
///    %36 = fmul fast float %34, %31
///    %37 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %36, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
/// In this loop, only ldrawvector can be hoisted, and sampleLptr has operands
/// %36, %36 have input dependency that LICM cannot do hoisting. So, after LICM
/// and loop unroll, only one sampleLptr is hoisted (offset 0 in phi), we can do
/// a custom pass to handle the rest sampleLptr hoist as below:
/// ===> Hoistable instruction case (%71 sampler below):
/// ......
///    %36 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %20, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
///    %37 = extractelement <4 x float> %36, i64 3
/// ......
/// NodeBlock27:                                      ; preds = %NewDefault, %64, %66, %68
///    %70 = phi i32[%69, %68], [%67, %66], [%65, %64], [%63, %NewDefault]
///    %71 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %24, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
///    %72 = extractelement <4 x float> %71, i64 3
/// ......
/// ===> Proposed optimization (the above %71 sampler is hoisted up to be %37 below):
/// ......
///    %36 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %20, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
///    %37 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %24, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
///......
///    %64 = extractelement <4 x float> %36, i64 3
///......
/// NodeBlock27:                                      ; preds = %NewDefault, %91, %93, %95
///    %97 = phi i32[%96, %95], [%94, %93], [%92, %91], [%90, %NewDefault]
///    %98 = extractelement <4 x float> %37, i64 3
///......
// clang-format on
class InstructionHoistingOptimization
    : public llvm::FunctionPass,
      public llvm::InstVisitor<InstructionHoistingOptimization> {
public:
  static char
      ID; ///< ID used by the llvm PassManager (the value is not important)

  InstructionHoistingOptimization();

  ////////////////////////////////////////////////////////////////////////
  virtual bool runOnFunction(llvm::Function &F);

  ////////////////////////////////////////////////////////////////////////
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

  ////////////////////////////////////////////////////////////////////////
  void visitCallInst(llvm::CallInst &CI);

private:
  ////////////////////////////////////////////////////////////////////////
  bool ProcessFunction(llvm::Function &F);

  ////////////////////////////////////////////////////////////////////////
  void InvalidateMembers();

  ////////////////////////////////////////////////////////////////////////
  void CollectHoistableInstructions();

  ////////////////////////////////////////////////////////////////////////
  bool ProcessHoistableInstructions();

  ////////////////////////////////////////////////////////////////////////
  CodeGenContext *m_pCGCtxt = nullptr;
  DominatorTree *m_pDT = nullptr;

  std::vector<llvm::Instruction *> m_SamplerInstructions;
  std::vector<llvm::Instruction *> m_HoistableInstructions;

  llvm::Instruction *m_InsertHoistBack = nullptr;
  llvm::BasicBlock *m_HoistBasicBlock = nullptr;

  inline bool traceOperandInHoistBB(Value *operand, BasicBlock *hoistBB) {
    if (Instruction *inst = dyn_cast<Instruction>(operand)) {
      for (auto &op : inst->operands()) {
        if (isPHINode(op) || !isHoistableInstruction(op, hoistBB)) {
          return false;
        }
      }
    }
    return true;
  }
  inline bool isPHINode(Value *op) { return isa<PHINode>(op); }
  inline bool isHoistableInstruction(Value *op, BasicBlock *hoistBB) {
    if (auto *I = dyn_cast<Instruction>(op)) {
      auto *currBB = I->getParent();
      if (isa<LoadInst>(I) || isa<LdRawIntrinsic>(I) ||
          isa<SampleIntrinsic>(I)) {
        return false;
      }
      if (currBB != hoistBB) {
        if (m_pDT->dominates(hoistBB, currBB)) {
          if (!traceOperandInHoistBB(op, hoistBB)) {
            return false;
          }
          InsertHoistableInstruction(I);
        } else {
          return false;
        }
      } else if (m_InsertHoistBack->comesBefore(I)) {
        return false;
      }
    }
    return true;
  }

  inline void InsertHoistableInstruction(Instruction *hoistInstr) {
    auto isDuplicate = [](std::vector<Instruction *> hoistedInstr,
                          Instruction *newInstr) -> bool {
      auto it = std::find(hoistedInstr.begin(), hoistedInstr.end(), newInstr);
      return it != hoistedInstr.end();
    };

    if (!isDuplicate(m_HoistableInstructions, hoistInstr)) {
      m_HoistableInstructions.push_back(hoistInstr);
    }
  }
};

char InstructionHoistingOptimization::ID = 0;

////////////////////////////////////////////////////////////////////////////
InstructionHoistingOptimization::InstructionHoistingOptimization()
    : llvm::FunctionPass(ID) {
  initializeInstructionHoistingOptimizationPass(
      *PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////
bool InstructionHoistingOptimization::runOnFunction(llvm::Function &F) {
  m_pCGCtxt = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_pDT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  // should be no loop, all loop is unrolled
  if (!getAnalysis<LoopInfoWrapperPass>().getLoopInfo().empty()) {
    return false;
  }

  InvalidateMembers();

  return ProcessFunction(F);
}

////////////////////////////////////////////////////////////////////////
void InstructionHoistingOptimization::visitCallInst(llvm::CallInst &CI) {
  if (llvm::GenIntrinsicInst *pIntr =
          llvm::dyn_cast<llvm::GenIntrinsicInst>(&CI)) {
    if (auto *SI = dyn_cast<SampleIntrinsic>(pIntr)) {
      m_SamplerInstructions.push_back(SI);
    }
  }
}

////////////////////////////////////////////////////////////////////////
void InstructionHoistingOptimization::CollectHoistableInstructions() {
  if (m_SamplerInstructions.empty())
    return;

  // use hash.getAsmHash() to debug specific shader

  // To make the hoisted instruction up as early as possible, check the first
  // BB, and check each sample operand and determined the latest location to as
  // hoist point. For the rest BBs, to simplify the process to sample hoist,
  // only pick the hoist point in first BB with sample, and use it to determine
  // the rest sample hoistable choice.
  for (Instruction *I : m_SamplerInstructions) {
    for (auto &Op : I->operands()) {
      if (Instruction *Inst = dyn_cast<Instruction>(Op)) {
        // pick the first op (inst) as the initial hoist point
        if (!m_InsertHoistBack) {
          m_InsertHoistBack = Inst;
          continue;
        }

        if (m_InsertHoistBack->getParent() == Inst->getParent()) {
          if (m_InsertHoistBack->comesBefore(Inst)) {
            m_InsertHoistBack = Inst;
          }
        }
      }
    }
  }

  if (!m_InsertHoistBack)
    return;

  m_HoistBasicBlock = m_InsertHoistBack->getParent();

  for (auto *pSI : m_SamplerInstructions) {
    bool canInstrHoisted = true;

    for (auto &Op : pSI->operands()) {
      if (isa<PHINode>(Op)) {
        canInstrHoisted = false;
        break;
      }

      if (Instruction *I = dyn_cast<Instruction>(Op)) {
        if (isa<LoadInst>(I) || isa<LdRawIntrinsic>(I) ||
            isa<SampleIntrinsic>(I)) {
          canInstrHoisted = false;
          break;
        }

        if (m_HoistBasicBlock == I->getParent()) {
          // If m_HoistBasicBlock comes before any operand of pSI,
          // this pSI won't be hoisted in this BB
          if (m_InsertHoistBack->comesBefore(I)) {
            canInstrHoisted = false;
            break;
          }
        } else {
          // check if op can be traced back to hoistInBB
          if (traceOperandInHoistBB(Op, m_HoistBasicBlock)) {
            InsertHoistableInstruction(I);
          } else {
            canInstrHoisted = false;
            break;
          }
        }
      }
    }

    if (canInstrHoisted) {
      InsertHoistableInstruction(pSI);
    }
  }
}

////////////////////////////////////////////////////////////////////////
bool InstructionHoistingOptimization::ProcessHoistableInstructions() {
  if (m_HoistableInstructions.empty())
    return false;

  auto *hoist_point = m_InsertHoistBack;

  for (auto *I : m_HoistableInstructions) {
    I->moveAfter(hoist_point);
    hoist_point = I;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////
bool InstructionHoistingOptimization::ProcessFunction(llvm::Function &F) {
  visit(F);

  CollectHoistableInstructions();

  return ProcessHoistableInstructions();
}

////////////////////////////////////////////////////////////////////////
void InstructionHoistingOptimization::getAnalysisUsage(
    llvm::AnalysisUsage &AU) const {
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
}

////////////////////////////////////////////////////////////////////////
void InstructionHoistingOptimization::InvalidateMembers() {
  m_SamplerInstructions.clear();
  m_HoistableInstructions.clear();
}

////////////////////////////////////////////////////////////////////////
llvm::Pass *createInstructionHoistingOptimization() {
  return new InstructionHoistingOptimization();
}

} // namespace IGC

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-instruction-hoisting-optimization"
#define PASS_DESCRIPTION "InstructionHoistingOptimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InstructionHoistingOptimization, PASS_FLAG,
                          PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(InstructionHoistingOptimization, PASS_FLAG,
                        PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
