/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MergeScalarPhisPass.hpp"
#include "MDFrameWork.h"
#include <cstddef>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/DerivedTypes.h>

using namespace llvm;
using namespace IGC;

char MergeScalarPhisPass::ID = 0;

#define PASS_FLAG "merge-scalar-phis"
#define PASS_DESCRIPTION "Merge scalar phis pass."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN(MergeScalarPhisPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(MergeScalarPhisPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

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

SmallVector<PHINode *, 8> MergeScalarPhisPass::getDuplicates(PHINode *PN, SmallVector<PHINode *, 8> &PhiNodesToErase) {
  BasicBlock *BB = PN->getParent();
  SmallVector<PHINode *, 8> DuplicatePhis;

  // Check for identical PHI nodes in the same basic block
  for (auto I = std::next(BasicBlock::iterator(PN)), E = BB->end(); I != E; ++I) {
    if (!isa<PHINode>(&*I))
      break;

    PHINode *OtherPN = cast<PHINode>(&*I);

    // Skip comparing with itself
    if (PN == OtherPN)
      continue;

    // Skip PHI nodes that are already marked for deletion
    if (std::find(PhiNodesToErase.begin(), PhiNodesToErase.end(), OtherPN) != PhiNodesToErase.end())
      continue;

    // Check if PHI nodes are identical
    if (PN->getNumIncomingValues() == OtherPN->getNumIncomingValues()) {
      bool AreIdentical = true;

      for (unsigned K = 0; K < PN->getNumIncomingValues(); ++K) {
        if (PN->getIncomingValue(K) != OtherPN->getIncomingValue(K) ||
            PN->getIncomingBlock(K) != OtherPN->getIncomingBlock(K)) {
          AreIdentical = false;
          break;
        }
      }

      if (AreIdentical)
        DuplicatePhis.push_back(OtherPN);
    }
  }

  return DuplicatePhis;
}

void MergeScalarPhisPass::cleanUpIR(Function *F) {
  for (auto *Phi : PhiNodesToRemove)
    Phi->eraseFromParent();

  for (auto *EEI : ExtrElementsToRemove)
    if (EEI->getNumUses() == 0)
      EEI->eraseFromParent();

  SmallVector<PHINode *, 8> PhiNodesToErase;
  for (auto &BB : *F) {
    for (auto I = BB.begin(), E = BB.end(); I != E;) {
      if (!isa<PHINode>(I))
        break;

      PHINode *PN = cast<PHINode>(&*I++);
      SmallVector<PHINode *, 8> Duplicates = getDuplicates(PN, PhiNodesToErase);

      for (PHINode *D : Duplicates) {
        D->replaceAllUsesWith(PN);
        PhiNodesToErase.push_back(D);
      }
    }
  }

  for (auto *PN : PhiNodesToErase)
    PN->eraseFromParent();
}

bool MergeScalarPhisPass::makeChanges(Function *F) {
  bool Changed = VectorToPhiNodesMap.size() > 0;

  for (const auto &Entry : VectorToPhiNodesMap) {
    Type *VectorType = Entry.first->getType();
    PHINode *FirstPN = Entry.second[0];
    auto NumIncValues = FirstPN->getNumIncomingValues();

    IRBuilder<> Builder(FirstPN);
    auto *NewPhi = cast<PHINode>(Builder.CreatePHI(VectorType, NumIncValues, "merged_vector_phi"));

    if (FirstPN->getDebugLoc())
      NewPhi->setDebugLoc(FirstPN->getDebugLoc());

    for (unsigned i = 0; i < NumIncValues; ++i) {
      Value *Incoming = FirstPN->getIncomingValue(i);
      if (isa<Constant>(Incoming) && cast<Constant>(Incoming)->isZeroValue()) {
        NewPhi->addIncoming(ConstantAggregateZero::get(VectorType), FirstPN->getIncomingBlock(i));
        continue;
      }
      auto *EEI = cast<ExtractElementInst>(Incoming);
      NewPhi->addIncoming(EEI->getVectorOperand(), FirstPN->getIncomingBlock(i));
    }

    BasicBlock *BB = FirstPN->getParent();
    Builder.SetInsertPoint(BB->getFirstNonPHI());
    for (auto *PN : Entry.second) {
      // Find an incoming ExtractElementInst to get the index value.
      ExtractElementInst *EEI = nullptr;
      for (unsigned i = 0; i < PN->getNumIncomingValues(); ++i) {
        EEI = dyn_cast<ExtractElementInst>(PN->getIncomingValue(i));
        if (EEI)
          break;
      }

      auto *CI = cast<ConstantInt>(EEI->getIndexOperand());
      auto *NewEEI =
          cast<ExtractElementInst>(Builder.CreateExtractElement(NewPhi, CI->getZExtValue(), "extract_merged"));

      if (EEI->getDebugLoc())
        NewEEI->setDebugLoc(EEI->getDebugLoc());

      PN->replaceAllUsesWith(NewEEI);
    }
  }

  cleanUpIR(F);

  return Changed;
}

bool MergeScalarPhisPass::isIncomingValueZero(PHINode *pPN, unsigned IncomingIndex) {
  Value *pIncVal = pPN->getIncomingValue(IncomingIndex);
  return isa<Constant>(pIncVal) && cast<Constant>(pIncVal)->isZeroValue();
}

Value *MergeScalarPhisPass::getVectorOperandForPhiNode(PHINode *PN, unsigned IncomingIndex) {
  Value *IncVal = PN->getIncomingValue(IncomingIndex);
  auto *EEI = dyn_cast<ExtractElementInst>(IncVal);
  return EEI ? EEI->getVectorOperand() : nullptr;
}

ExtractElementInst *MergeScalarPhisPass::getEEIFromPhi(PHINode *PN, unsigned IncomingIndex) {
  Value *IncVal = PN->getIncomingValue(IncomingIndex);
  return dyn_cast<ExtractElementInst>(IncVal);
}

// Collect all PHI nodes to VectorToPhiNodesMap. Use incoming vector value
// from incoming index 0 as a key for the group of PHI nodes. After collecting
// all PHI nodes, filter out phi nodes using conditions for incoming values with
// indices not equal to 0.
//
// Conditions for PHI nodes to be merged into a single vector PHI node:
// Condition 1: At least one PHI node incoming value should be ExtractElementInsts (EEIs), other can be zeros.
// Condition 2: EEIs vector operands should have FixedVectorType.
// Condition 3: EEIs should have the same vector type.
// Condition 4: EEIs should have the same constant index value.
// Condition 6: Number of PHI nodes in a group should be equal to the vector size.
void MergeScalarPhisPass::collectPhiNodes(Function &F) {
  auto getFirstVectorIncomingValForPhiNode = [](PHINode *PN) -> Value * {
    for (unsigned i = 0; i < PN->getNumIncomingValues(); ++i) {
      Value *IncVal = PN->getIncomingValue(i);
      if (isa<Constant>(IncVal) && cast<Constant>(IncVal)->isZeroValue())
        continue;

      auto *EEI = cast<ExtractElementInst>(IncVal);
      return EEI->getVectorOperand();
    }
    return nullptr;
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
      for (unsigned i = 0; i < PN->getNumIncomingValues(); ++i) {
        if (isIncomingValueZero(PN, i))
          continue;

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
        if (!SavedType) {
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
      }

      if (Check) {
        // Using vector operand of the first incoming EEI to define a key for
        // the group of PHI nodes.
        Value *FirstVectorOp = getFirstVectorIncomingValForPhiNode(PN);
        unsigned CurNumIncValues = PN->getNumIncomingValues();

        // All PHI nodes corresponding to the same vector value should be in one
        // basic block.
        if (VectorToPhiNodesMap.find(FirstVectorOp) != VectorToPhiNodesMap.end()) {
          if (VectorToPhiNodesMap[FirstVectorOp][0]->getParent() != PN->getParent())
            continue;

          // All PN in the group should have the same number of incoming values.
          if (VectorToPhiNodesMap[FirstVectorOp][0]->getNumIncomingValues() != CurNumIncValues)
            continue;
        }
        VectorToPhiNodesMap[FirstVectorOp].push_back(PN);
      }
    }
  }

  filterOutUnexpectedIncomingConstants();

  // Filter out PHI nodes that do not meet the conditions 6.
  // Filter out some suspicious cases (e.g. when the EEIs for a particular PHI
  // node group and a particular index are not in the same base block).
  filterOutUnvectorizedPhis();

  for (auto &Entry : VectorToPhiNodesMap) {
    for (auto *PN : Entry.second) {
      PhiNodesToRemove.insert(PN);
      for (unsigned I = 0; I < PN->getNumIncomingValues(); ++I) {
        ExtractElementInst *EEI = getEEIFromPhi(PN, I);
        // Skip zeros incoming values.
        if (EEI && EEI->hasOneUser()) {
          ExtrElementsToRemove.insert(EEI);
        }
      }
    }
  }
}

// Check that if at least one phi node incoming value is zero for a
// specific index, then all other incoming values for that index should
// be zeros as well.
void MergeScalarPhisPass::filterOutUnexpectedIncomingConstants() {
  for (auto It = VectorToPhiNodesMap.begin(); It != VectorToPhiNodesMap.end();) {
    auto &PhiNodes = It->second;
    auto *FirstPhiNode = PhiNodes[0];

    // Remove groups with less than 2 PHI nodes.
    if (PhiNodes.size() < 2) {
      It = VectorToPhiNodesMap.erase(It);
      continue;
    }

    bool NeedToBreak = false;
    size_t NumIncomingValues = FirstPhiNode->getNumIncomingValues();
    for (unsigned Index = 0; Index < NumIncomingValues; ++Index) {
      bool ExpectZeroVal = false;
      for (size_t P = 0; P < PhiNodes.size(); ++P) {
        if (isIncomingValueZero(PhiNodes[P], Index)) {
          if (!P) {
            ExpectZeroVal = true;
            continue;
          }

          if (!ExpectZeroVal) {
            It = VectorToPhiNodesMap.erase(It);
            NeedToBreak = true;
            break;
          }
        } else {
          if (ExpectZeroVal) {
            It = VectorToPhiNodesMap.erase(It);
            NeedToBreak = true;
            break;
          }
        }
      }

      if (NeedToBreak)
        break;
    }

    if (!NeedToBreak)
      ++It;
  }
}

void MergeScalarPhisPass::filterOutUnvectorizedPhis() {
  for (auto It = VectorToPhiNodesMap.begin(); It != VectorToPhiNodesMap.end();) {
    Type *VType = It->first->getType();
    auto &PhiNodes = It->second;
    auto *FirstPhiNode = PhiNodes[0];

    // Check Condition 6: Number of PHI nodes in a group should be equal to the vector size.
    size_t NumElements = cast<VectorType>(VType)->getElementCount().getFixedValue();
    if (NumElements != PhiNodes.size()) {
      It = VectorToPhiNodesMap.erase(It);
      continue;
    }

    bool NeedToBreak = false;
    size_t NumIncomingValues = FirstPhiNode->getNumIncomingValues();
    for (unsigned Index = 0; Index < NumIncomingValues; ++Index) {
      Value *VOp = getVectorOperandForPhiNode(FirstPhiNode, Index);
      ExtractElementInst *EEI = getEEIFromPhi(FirstPhiNode, Index);

      if (!VOp || !EEI)
        continue;

      BasicBlock *EEIBB = EEI->getParent();
      for (size_t P = 0; P < PhiNodes.size(); ++P) {
        Value *CurrVOp = getVectorOperandForPhiNode(PhiNodes[P], Index);
        ExtractElementInst *CurrEEI = getEEIFromPhi(PhiNodes[P], Index);

        if (!CurrVOp || !CurrEEI) {
          It = VectorToPhiNodesMap.erase(It);
          NeedToBreak = true;
          break;
        }

        BasicBlock *CurrEEIBB = CurrEEI->getParent();

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

      if (NeedToBreak)
        break;
    }

    if (!NeedToBreak)
      ++It;
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
    Changed |= makeChanges(&F);
    collectPhiNodes(F);
  }

  return Changed;
}