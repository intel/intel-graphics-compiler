/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MergeScalarPhisPass.hpp"
#include "MDFrameWork.h"
#include <cstddef>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char MergeScalarPhisPass::ID = 0;

#define PASS_FLAG "merge-scalar-phis"
#define PASS_DESCRIPTION "Merge scalar phis pass."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN(MergeScalarPhisPass, PASS_FLAG, PASS_DESCRIPTION,
                          PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(MergeScalarPhisPass, PASS_FLAG, PASS_DESCRIPTION,
                        PASS_CFG_ONLY, PASS_ANALYSIS)

// The MergeScalarPhisPass is a function pass designed to optimize the handling
// of scalar PHI nodes. This pass merges scalar PHI nodes into a single vector
// PHI node when all incoming values are ExtractElementInsts with the same
// vector type.

// For example
//
// Before:
// BB0:
//  br i1 %cond1, label %BB1, label %BB3
// BB1:
//  %inc_vec = phi <4 x i32> [ zeroinitializer, %BB0 ], [ %vec1, %BB1 ]
//  %vec1 = call <4 x i32> @update_vec(<4 x i32> %inc_vec)
//  br i1 %cond2, label %BB1, label %BB2
// BB2:
//  %eei_1_1 = extractelement <4 x i32> %vec1, i32 0
//  %eei_1_2 = extractelement <4 x i32> %vec1, i32 1
//  %eei_1_3 = extractelement <4 x i32> %vec1, i32 2
//  %eei_1_4 = extractelement <4 x i32> %vec1, i32 3
//  br label %BB4
// BB3:
//  %vec2 = call <4 x i32> @get_vec()
//  %eei_2_1 = extractelement <4 x i32> %vec2, i32 0
//  %eei_2_2 = extractelement <4 x i32> %vec2, i32 1
//  %eei_2_3 = extractelement <4 x i32> %vec2, i32 2
//  %eei_2_4 = extractelement <4 x i32> %vec2, i32 3
//  br label %BB4
// BB4:
//  %res_scalar1 = phi i32 [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ]
//  %res_scalar2 = phi i32 [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ]
//  %res_scalar3 = phi i32 [ %eei_1_3, %BB2 ], [ %eei_2_3, %BB3 ]
//  %res_scalar4 = phi i32 [ %eei_1_4, %BB2 ], [ %eei_2_4, %BB3 ]
//  call @use_scalars(i32 %res_scalar1, i32 %res_scalar2, i32 %res_scalar3, i32
//  %res_scalar4) ret void
//
// After:
// BB0:
//  br i1 %cond1, label %BB1, label %BB3
// BB1:
//  %inc_vec = phi <4 x i32> [ zeroinitializer, %BB0 ], [ %vec1, %BB1 ]
//  %vec1 = call <4 x i32> @update_vec(<4 x i32> %inc_vec)
//  br i1 %cond2, label %BB1, label %BB2
// BB2:
//  br label %BB4
// BB3:
//  %vec2 = call <4 x i32> @get_vec()
//  br label %BB4
// BB4:
//  %merged_phi = phi <4 x i32> [ %vec1, %BB2 ], [ %vec2, %BB3 ]
//  %res_scalar1 = extractelement <4 x i32> %merged_phi, i32 0
//  %res_scalar2 = extractelement <4 x i32> %merged_phi, i32 1
//  %res_scalar3 = extractelement <4 x i32> %merged_phi, i32 2
//  %res_scalar4 = extractelement <4 x i32> %merged_phi, i32 3
//  call @use_scalars(i32 %res_scalar1, i32 %res_scalar2, i32 %res_scalar3, i32
//  %res_scalar4) ret void

MergeScalarPhisPass::MergeScalarPhisPass() : FunctionPass(ID) {
  initializeMergeScalarPhisPassPass(*PassRegistry::getPassRegistry());
}

void MergeScalarPhisPass::cleanUpIR() {
  for (auto *Phi : PhiNodesToRemove)
    Phi->eraseFromParent();

  for (auto *EEI : ExtrElementsToRemove)
    if (EEI->getNumUses() == 0)
      EEI->eraseFromParent();
}

bool MergeScalarPhisPass::makeChanges() {
  bool Changed = VectorToPhiNodesMap.size() > 0;

  for (const auto &Entry : VectorToPhiNodesMap) {
    Type *VectorType = Entry.first->getType();
    PHINode *FirstPN = Entry.second[0];
    auto NumIncValues = FirstPN->getNumIncomingValues();

    IRBuilder<> Builder(FirstPN);
    auto *NewPhi = cast<PHINode>(
        Builder.CreatePHI(VectorType, NumIncValues, "merged_vector_phi"));

    if (FirstPN->getDebugLoc())
      NewPhi->setDebugLoc(FirstPN->getDebugLoc());

    for (unsigned i = 0; i < NumIncValues; ++i) {
      Value *Incoming = FirstPN->getIncomingValue(i);
      auto *EEI = cast<ExtractElementInst>(Incoming);
      NewPhi->addIncoming(EEI->getVectorOperand(),
                          FirstPN->getIncomingBlock(i));
    }

    BasicBlock *BB = FirstPN->getParent();
    Builder.SetInsertPoint(BB->getFirstNonPHI());
    for (auto *PN : Entry.second) {
      auto *EEI = cast<ExtractElementInst>(PN->getIncomingValue(0));
      auto *CI = cast<ConstantInt>(EEI->getIndexOperand());
      auto *NewEEI = cast<ExtractElementInst>(Builder.CreateExtractElement(
        NewPhi, CI->getZExtValue(), "extract_merged"));
      if (EEI->getDebugLoc())
        NewEEI->setDebugLoc(EEI->getDebugLoc());

      PN->replaceAllUsesWith(NewEEI);
    }
  }

  cleanUpIR();

  return Changed;
}

// Collect all PHI nodes to VectorToPhiNodesMap. Use incoming vector value
// from incoming index 0 as a key for the group of PHI nodes. After collecting
// all PHI nodes, filter out phi nodes using conditions for incoming values with
// indices not equal to 0.
//
// Conditions for PHI nodes to be merged into a single vector PHI node:
// Condition 1: All PHI node incoming values should be ExtractElementInsts (EEIs).
// Condition 2: EEIs vector operands should have FixedVectorType.
// Condition 3: EEIs should have the same vector type.
// Condition 4: EEIs should have the same constant index value.
// Condition 5: All incoming EEIs should be used only once.
// Condition 6: Number of PHI nodes in a group should be equal to the vector
// size.
void MergeScalarPhisPass::collectPhiNodes(Function &F) {
  auto getVectorOperandForPhiNode = [](PHINode *PN,
                                       unsigned IncomingIndex) -> Value * {
    Value *IncVal = PN->getIncomingValue(IncomingIndex);
    auto *EEI = cast<ExtractElementInst>(IncVal);
    return EEI->getVectorOperand();
  };

  auto getEEIFromPhi = [](PHINode *PN,
                          unsigned IncomingIndex) -> ExtractElementInst * {
    Value *IncVal = PN->getIncomingValue(IncomingIndex);
    return dyn_cast<ExtractElementInst>(IncVal);
  };

  clearContainers();

  // Collect PHI nodes that meet the conditions 1 - 5.
  for (auto &BB : F) {
    for (Instruction &I : BB) {
      auto *PN = dyn_cast<PHINode>(&I);

      if (!PN)
        break;

      bool Check = true;
      Type *SavedType = nullptr;
      int IndexValue = 0;

      // Skip PHI nodes with less than 2 incoming values.
      // if (PN->getNumIncomingValues() < 2)
      //   continue;

      for (unsigned i = 0; i < PN->getNumIncomingValues(); ++i) {
        // Check Condition 1
        ExtractElementInst *EEI = getEEIFromPhi(PN, i);
        if (!EEI) {
          Check = false;
          break;
        }

        auto *CI = dyn_cast<ConstantInt>(EEI->getIndexOperand());
        if (!CI) {
          Check = false;
          break;
        }

        auto *CurrType = EEI->getVectorOperand()->getType();
        int CurrIndexValue = CI->getZExtValue();
        if (i == 0) {
          SavedType = CurrType;
          IndexValue = CurrIndexValue;
        }
        if (!isa<IGCLLVM::FixedVectorType>(CurrType)) {
          Check = false;
          break;
        }

        // Check Condition 3
        // Check if all EEI extracts from values with the same type.
        if (SavedType != CurrType) {
          Check = false;
          break;
        }

        // Check Condition 4
        if (IndexValue != CurrIndexValue) {
          Check = false;
          break;
        }

        // Check Condition 5
        if (!EEI->getSingleUndroppableUse()) {
          Check = false;
          break;
        }
      }

      if (Check) {
        // Using vector operand of the first incoming EEI to define a key for
        // the group of PHI nodes.
        Value *FirstEEIVectorOp = getVectorOperandForPhiNode(PN, 0);
        auto CurNumIncValues = PN->getNumIncomingValues();

        // All PHI nodes corresponding to the same vector value should be in one
        // basic block.
        if (VectorToPhiNodesMap.find(FirstEEIVectorOp) != VectorToPhiNodesMap.end()) {
          if (VectorToPhiNodesMap[FirstEEIVectorOp][0]->getParent() !=
              PN->getParent()) {
            continue;
          }

          // All PN in the group should have the same number of incoming values.
          if (VectorToPhiNodesMap[FirstEEIVectorOp][0]->getNumIncomingValues() !=
              CurNumIncValues) {
            continue;
          }
        }

        VectorToPhiNodesMap[FirstEEIVectorOp].push_back(PN);
      }
    }
  }

  // Filter out PHI nodes that do not meet the conditions 6.
  // Filter out some suspicious cases (e.g. when the EEIs for a particular PHI
  // node group and a particular index are not in the same base block).
  for (auto It = VectorToPhiNodesMap.begin(); It != VectorToPhiNodesMap.end();) {
    auto &PhiNodes = It->second;
    auto *FirstPhiNode = PhiNodes[0];

    // Remove groups with less than 2 PHI nodes.
    if (PhiNodes.size() < 2) {
      It = VectorToPhiNodesMap.erase(It);
      continue;
    }

    // Check Condition 6
    Type *VType = getVectorOperandForPhiNode(FirstPhiNode, 0)->getType();
    size_t NumElements =
        cast<VectorType>(VType)->getElementCount().getFixedValue();
    if (NumElements != PhiNodes.size()) {
      It = VectorToPhiNodesMap.erase(It);
      continue;
    }

    bool NeedToBreak = false;
    size_t NumIncomingValues = FirstPhiNode->getNumIncomingValues();

    for (unsigned Index = 0; Index < NumIncomingValues; ++Index) {
      Value *VOp = getVectorOperandForPhiNode(FirstPhiNode, Index);
      BasicBlock *EEIBB = getEEIFromPhi(FirstPhiNode, Index)->getParent();

      for (size_t P = 0; P < PhiNodes.size(); ++P) {
        Value *CurrVOp = getVectorOperandForPhiNode(PhiNodes[P], Index);
        BasicBlock *CurrEEIBB = getEEIFromPhi(PhiNodes[P], Index)->getParent();

        // Check that all incoming values for a specific index in the PHI nodes
        // group were extracted from the same vector.
        if (VOp != CurrVOp) {
          It = VectorToPhiNodesMap.erase(It);
          NeedToBreak = true;
          break;
        }

        // Erase phi group if the incoming EEI basic blocks are not the same.
        if (EEIBB != CurrEEIBB) {
          It = VectorToPhiNodesMap.erase(It);
          NeedToBreak = true;
          break;
        }
      }
    }

    if (!NeedToBreak) {
      ++It;
    }
  }

  for (auto &Entry : VectorToPhiNodesMap) {
    for (auto *PN : Entry.second) {
      PhiNodesToRemove.insert(PN);
      for (unsigned I = 0; I < PN->getNumIncomingValues(); ++I) {
        ExtrElementsToRemove.insert(getEEIFromPhi(PN, I));
      }
    }
  }
}

void MergeScalarPhisPass::clearContainers() {
  VectorToPhiNodesMap.clear();
  ExtrElementsToRemove.clear();
  PhiNodesToRemove.clear();
}

bool MergeScalarPhisPass::runOnFunction(Function &F) {
  bool Changed = false;
  if (skipFunction(F))
    return false;

  collectPhiNodes(F);

  // Optimize the function until optimization patterns can be found.
  while (VectorToPhiNodesMap.size()) {
    Changed |= makeChanges();
    collectPhiNodes(F);
  }

  return Changed;
}