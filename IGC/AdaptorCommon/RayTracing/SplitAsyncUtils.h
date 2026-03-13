/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===- CoroFrame.cpp - Builds and manipulates coroutine frame -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvm/Support/raw_ostream.h>
#include <optional>

namespace IGC {

class Spill {
  llvm::WeakTrackingVH Def = nullptr;
  llvm::Instruction *User = nullptr;

public:
  Spill(llvm::Value *Def, llvm::User *U) : Def(Def), User(llvm::cast<llvm::Instruction>(U)) {}

  llvm::Value *def() const { return Def; }
  llvm::Instruction *user() const { return User; }
  llvm::BasicBlock *userBlock() const { return User->getParent(); }
};

void rewritePHIs(llvm::Function &F);

void insertSpills(CodeGenContext *CGCtx, llvm::Function &F, const llvm::SmallVector<Spill, 8> &Spills);

void rewriteMaterializableInstructions(const llvm::SmallVector<Spill, 8> &Spills);

enum class RematStage { MID, LATE };

// Rejection reason for greedy remat heuristic:
enum class RejectionReason {
  LOAD,           // non-constant load
  LDRAW,          // non-read-only ldraw
  COND_PHI,       // value requires resolving of a conditional PHI
  LOOP_PHI,       // value fed by a loop carried PHI
  GEN_INTRINSIC,  // GenIntrinsic non-materializable or not supported
  LLVM_INTRINSIC, // llvm intrinsic non-materializable
  ALLOCA,         // alloca pointer, will be resolved after remat
  OTHER,          // other, non-accounted reason
  EXHAUSTED,      // search depth exhausted, analysis not completed
  ACCEPTED        // value accepted for a full re-materialization
};

class RematChecker {
public:
  /// Map type for tracking greedy remat rejection reasons.
  using RejectionReasonMapType = llvm::DenseMap<const llvm::Instruction *, RejectionReason>;

  /// Constructor used by late-remat pass.
  RematChecker(CodeGenContext &Ctx, RematStage Stage);

  /// Constructor used by remat pass (adds dominance tree and loop info parameters).
  RematChecker(CodeGenContext &Ctx, RematStage Stage, llvm::DominatorTree *DT, llvm::LoopInfo *LI);

#if defined(_DEBUG) || defined(_INTERNAL)
  /// A debug mode constructor used by remat pass, with dominator tree, loop info, and diag stream.
  /// note: There is no special debug constructor for late-remat pass.
  RematChecker(CodeGenContext &Ctx, RematStage Stage, llvm::DominatorTree *DT, llvm::LoopInfo *LI,
               llvm::raw_ostream *Stream);
#endif

  std::optional<std::vector<llvm::Instruction *>> canFullyRemat(llvm::Instruction *I, uint32_t Threshold,
                                                                llvm::ValueToValueMapTy *VM = nullptr) const;
  bool materializable(const llvm::Instruction &I) const;
  bool isFreeOperand(const llvm::Value *Op) const;

#if defined(_DEBUG) || defined(_INTERNAL)
  std::unique_ptr<RejectionReasonMapType> CaptureRejectionReasonMap() { return std::move(m_UniqueReasonMap); }
#endif

private:
  bool canFullyRemat(llvm::Instruction *I, std::vector<llvm::Instruction *> &Insts,
                     std::unordered_set<llvm::Instruction *> &Visited, unsigned StartDepth, unsigned Depth,
                     llvm::ValueToValueMapTy *VM) const;
  bool isReadOnly(const llvm::Value *Ptr) const;

  CodeGenContext &Ctx;
  RematStage Stage;
#if defined(_DEBUG) || defined(_INTERNAL)
  llvm::raw_ostream *m_pStream;
  std::unique_ptr<RejectionReasonMapType> m_UniqueReasonMap;
  mutable llvm::Instruction *m_RootInstruction = nullptr;
#endif

  llvm::DominatorTree *DT = nullptr;
  llvm::LoopInfo *LI = nullptr;
};

} // namespace IGC
