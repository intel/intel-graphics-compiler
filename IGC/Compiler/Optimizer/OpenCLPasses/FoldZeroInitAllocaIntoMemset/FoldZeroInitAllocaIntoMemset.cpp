/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/FoldZeroInitAllocaIntoMemset/FoldZeroInitAllocaIntoMemset.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

#define DEBUG_TYPE "FoldZeroInitAllocaIntoMemset"

char FoldZeroInitAllocaIntoMemsetLPM::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-fold-zeroinit-alloca-into-memset"
#define PASS_DESCRIPTION "FoldZeroInitAllocaIntoMemset"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(FoldZeroInitAllocaIntoMemsetLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(FoldZeroInitAllocaIntoMemsetLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

FoldZeroInitAllocaIntoMemsetLPM::FoldZeroInitAllocaIntoMemsetLPM() : FunctionPass(ID) {
  initializeFoldZeroInitAllocaIntoMemsetLPMPass(*PassRegistry::getPassRegistry());
}

/*
  The purpose of this pass is to replace such pattern:
    alloca array -> memset whole array with 0 -> bitcast -> copy that array of zeros into other buffer
  with just
    memset buffer with 0s

  Such pattern started appearing after upstream translator changes:
    https://github.com/KhronosGroup/SPIRV-LLVM-Translator/commit/648654da1d135e582dd7aeaec85855d48310109e

  And started causing issues related to exceeding maximum memory

  %2 = alloca [4000000 x i8], align 4, !spirv.Decorations !55
  call void @llvm.memset.p0.i64(ptr align 1 %2, i8 0, i64 4000000, i1 false)

  %14 = bitcast ptr %2 to ptr
  call void @llvm.memcpy.p1.p0.i64(ptr addrspace(1) align 4 %BUFFER, ptr align 4 %14, i64 4000000, i1 false)
*/
bool FoldZeroInitAllocaIntoMemset::runOnFunction(Function &F) {
  std::vector<WorkItem> workItems = findUnnecessaryAllocaInstances(F);

  for (auto entry : workItems) {
    LLVM_DEBUG({
      dbgs() << "___________ START ___________\n";
      dbgs() << "- Alloca Array Size: " << entry.AllocaArraySize << "\n";
      entry.Alloca->dump();
      entry.MemSetCall->dump();
      entry.Bitcast->dump();
      entry.MemCpyCall->dump();
    });

    IRBuilder<> builder(entry.MemCpyCall);
    llvm::Value *fillVal = builder.getInt8(0);
    llvm::Value *sizeVal = builder.getInt64(entry.AllocaArraySize);
    auto newMemSetCall = builder.CreateMemSet(entry.Destination, fillVal, sizeVal, entry.Alignment, entry.IsVolatile);
    (void)newMemSetCall; // avoid unused variable warning in release build

    // Remove backwards: memcpy, bitcast, memset, alloca

    entry.MemCpyCall->eraseFromParent();
    entry.Bitcast->eraseFromParent();
    entry.MemSetCall->eraseFromParent();
    entry.Alloca->eraseFromParent();

    LLVM_DEBUG({
      dbgs() << "- NEW MEM SET:\n";
      newMemSetCall->dump();
      dbgs() << "___________ END   ___________\n";
    });
  }

  return !workItems.empty();
}

std::vector<WorkItem> FoldZeroInitAllocaIntoMemset::findUnnecessaryAllocaInstances(Function &F) {
  /*
    %2 = alloca [4000000 x i8], align 4, !spirv.Decorations !55
    call void @llvm.memset.p0.i64(ptr align 1 %2, i8 0, i64 4000000, i1 false)

    %14 = bitcast ptr %2 to ptr
    call void @llvm.memcpy.p1.p0.i64(ptr addrspace(1) align 4 %ARRAY_3D.ul.GEP.6, ptr align 4 %14, i64 4000000, i1
    false)
  */

  std::vector<WorkItem> workItems{};

  for (auto &I : llvm::instructions(F)) {
    if (auto AI = dyn_cast<llvm::AllocaInst>(&I)) {
      // ------------- Alloca -------------
      auto arrayTy = dyn_cast<llvm::ArrayType>(AI->getAllocatedType());
      if (!arrayTy)
        continue;

      uint64_t sizeOfAllocaArrayTy = arrayTy->getNumElements();

      std::vector<Value *> allocaUserList(AI->user_begin(), AI->user_end());

      // Find allocas with only two users - call to memset and bitcast
      if (allocaUserList.size() != 2)
        continue;

      llvm::CallInst *memSetCall = nullptr;
      llvm::BitCastInst *bitcastInst = nullptr;

      auto firstUser = allocaUserList.at(0);
      auto secondUser = allocaUserList.at(1);
      // order varies
      if (isa_and_nonnull<llvm::CallInst>(firstUser) && isa_and_nonnull<llvm::BitCastInst>(secondUser)) {
        memSetCall = dyn_cast<llvm::CallInst>(firstUser);
        bitcastInst = dyn_cast<llvm::BitCastInst>(secondUser);
      } else if (isa_and_nonnull<llvm::BitCastInst>(firstUser) && isa_and_nonnull<llvm::CallInst>(secondUser)) {
        bitcastInst = dyn_cast<llvm::BitCastInst>(firstUser);
        memSetCall = dyn_cast<llvm::CallInst>(secondUser);
      } else {
        continue;
      }

      // ------------- Memset -------------
      auto calledFunc = memSetCall->getCalledFunction();

      if (!calledFunc || calledFunc->getIntrinsicID() != llvm::Intrinsic::memset)
        continue;

      auto memsetOperand_Value = memSetCall->getArgOperand(1);
      auto memsetOperand_Length = memSetCall->getArgOperand(2);

      if (!memsetOperand_Value || !memsetOperand_Length)
        continue;

      // Check if memset is setting values to 0
      auto memSetOperandVal = dyn_cast<llvm::ConstantInt>(memsetOperand_Value);
      if (!memSetOperandVal || memSetOperandVal->getValue() != 0)
        continue;

      // Check if memset length is equal to alloca array size
      auto memSetOperandLength = dyn_cast<llvm::ConstantInt>(memsetOperand_Length);
      if (!memSetOperandLength || memSetOperandLength->getValue() != sizeOfAllocaArrayTy)
        continue;

      // Get isVolatile value if available
      bool isVolatile = false;
      if (memSetCall->arg_size() > 3) {
        auto memsetOperand_IsVolatile = memSetCall->getArgOperand(3);

        auto memSetIsVolatile = dyn_cast<llvm::ConstantInt>(memsetOperand_IsVolatile);
        if (!memSetIsVolatile)
          continue;

        isVolatile = memSetIsVolatile->getValue() == 1;
      }

      // ------------- Bitcast -------------
      std::vector<Value *> bitcastUserList(bitcastInst->user_begin(), bitcastInst->user_end());
      if (bitcastUserList.size() != 1)
        continue;

      auto bitcastUser = bitcastUserList.at(0);
      if (!isa_and_nonnull<llvm::CallInst>(bitcastUser))
        continue;

      // ------------- MemCpy -------------
      auto memCpyCall = dyn_cast<llvm::CallInst>(bitcastUser);
      auto memCpycalledFunc = memCpyCall->getCalledFunction();
      if (!memCpycalledFunc || memCpycalledFunc->getIntrinsicID() != llvm::Intrinsic::memcpy)
        continue;

      // get destination
      auto memCpyOperand0 = memCpyCall->getArgOperand(0);
      auto memCpyOperandDestination = dyn_cast<llvm::Value>(memCpyOperand0);

      // Check if memCpy length is equal to alloca array size
      auto memCpyOperand2 = memCpyCall->getArgOperand(2);
      auto memCpyOperandLength = dyn_cast<llvm::ConstantInt>(memCpyOperand2);
      if (!memCpyOperandLength || memCpyOperandLength->getValue() != sizeOfAllocaArrayTy)
        continue;

      auto alignment = memCpyCall->getParamAlign(0);

      WorkItem workItem(AI, memCpyOperandDestination, memSetCall, bitcastInst, memCpyCall, alignment,
                        sizeOfAllocaArrayTy, isVolatile);
      workItems.push_back(workItem);
    }
  }

  return workItems;
}

PreservedAnalyses FoldZeroInitAllocaIntoMemsetNPM::run(Module &M, ModuleAnalysisManager &AM) {
  FoldZeroInitAllocaIntoMemset impl;
  bool changed = false;
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    changed |= impl.runOnFunction(F);
  }
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
