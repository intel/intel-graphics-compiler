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
#include <llvm/ADT/Optional.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

class Spill {
    llvm::WeakTrackingVH Def = nullptr;
    llvm::Instruction* User = nullptr;
public:
    Spill(llvm::Value* Def, llvm::User* U) :
        Def(Def), User(llvm::cast<llvm::Instruction>(U)) {}

    llvm::Value* def() const { return Def; }
    llvm::Instruction* user() const { return User; }
    llvm::BasicBlock* userBlock() const { return User->getParent(); }
};

void rewritePHIs(llvm::Function& F);

void insertSpills(
    CodeGenContext* CGCtx,
    llvm::Function& F,
    const llvm::SmallVector<Spill, 8>& Spills);

void rewriteMaterializableInstructions(const llvm::SmallVector<Spill, 8>& Spills);

enum class RematStage
{
    MID,
    LATE
};

class RematChecker
{
public:
    RematChecker(CodeGenContext& Ctx, RematStage Stage);
    llvm::Optional<std::vector<llvm::Instruction*>>
    canFullyRemat(
        llvm::Instruction* I,
        uint32_t Threshold,
        llvm::ValueToValueMapTy* VM = nullptr) const;
private:
    bool canFullyRemat(
        llvm::Instruction* I,
        std::vector<llvm::Instruction*>& Insts,
        std::unordered_set<llvm::Instruction*>& Visited,
        unsigned StartDepth,
        unsigned Depth,
        llvm::ValueToValueMapTy* VM) const;
    bool materializable(const llvm::Instruction& I) const;
    bool isReadOnly(const llvm::Value* Ptr) const;

    CodeGenContext& Ctx;
    RematStage Stage;
};

} // namespace IGC
