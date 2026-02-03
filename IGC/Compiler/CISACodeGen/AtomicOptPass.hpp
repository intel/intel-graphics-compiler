/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"

namespace IGC {
// This pass performs the following optimization:
//
//  It adds a reduce instruction (GenISA.WaveAll) for the subgroup before the atomic float sum.
//  It handles only float emulated uniform atomics.
//
//  Emulated atomic before optimization
//  back:
//     %call1 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %1, i32 addrspace(1)* %1,
//     i32 0, i32 9) %bc1 = bitcast i32 %call1 to float %operation = fadd fast float %bc1, -2.000000e+00 %bc2 = bitcast
//     float %operation to i32 %call2 = call i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32
//     addrspace(1)* %1, i32 addrspace(1)* %1, i32 %call1, i32 %bc2) %cmp = icmp eq i32 %call1, %call2 br i1 %cmp, label
//     %exit, label %back
//  exit:
//      ret void
//
//  Emulated atomic after optimization
//  entry:
//      %2 = call i16 @llvm.genx.GenISA.simdLaneId()
//      %3 = zext i16 %2 to i32
//      %4 = call float @llvm.genx.GenISA.WaveAll.f32(float -2.000000e+00, i8 9, i1 true, i32 0)
//      %5 = icmp eq i32 %3, 0
//      br i1 %5, label %back, label %exit
//  back:
//      %call1 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %1, i32 addrspace(1)* %1,
//      i32 0, i32 9) %bc1 = bitcast i32 %call1 to float %operation = fadd fast float %bc1, %4 %bc2 = bitcast float
//      %operation to i32 %call2 = call i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %1,
//      i32 addrspace(1)* %1, i32 %call1, i32 %bc2) %cmp = icmp eq i32 %call1, %call2 br i1 %cmp, label %exit, label
//      %back
//  exit:
//      ret void

class AtomicOptPass : public llvm::FunctionPass {
public:
  static char ID;

  AtomicOptPass();

  virtual llvm::StringRef getPassName() const override { return "Atomic Optimisation Pass"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    AU.addRequired<WIAnalysis>();
  }

  virtual bool runOnFunction(llvm::Function &F) override;

private:
  llvm::Instruction *createReduce(llvm::Instruction *Pos, llvm::Value *ValueForReduce);
  llvm::Value *getSubgroupLocalIdBI(llvm::Instruction *Pos);
  bool checkFloatAtomicEmulation(llvm::Instruction *Val, size_t &OperandPos);

  bool Changed = false;
  WIAnalysis *Wi = nullptr;
  llvm::Module *M = nullptr;
};

} // namespace IGC