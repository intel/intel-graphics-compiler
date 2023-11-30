/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

namespace IGC
{
    // These pass is designed to optimize address calculations for built-in block read and write functions.
    // This optimization applies in case address calculation is used only in block read and write functions.
    // Scalarization is achieved inserting llvm.genx.GenISA.WaveShuffleIndex.i32.i32.i32.
    // It broadcasts element from the first element in a subgroup.
    // Here is the example of the optimization.
    // LLVM IR before optimization:
    // entry:
    //    %extr1 = extractelement <8 x i32> %payloadHeader, i32 1
    //    %extr2 = extractelement <8 x i32> %payloadHeader, i32 2
    //    %add1 = add i32 %src1, %extr2
    //    %add2 = add i32 %src2, %extr1
    //    %narrow = add nuw i32 %add1, 4
    //    %zt1 = zext i32 %narrow to i64
    //    %narrow53 = add nuw i32 %add2, 4
    //    %zt2 = zext i32 %narrow53 to i64
    //    %mul1 = mul i64 %zt1, %src3
    //    %add3 = add i64 %mul1, %zt2
    //    %mul2 = mul i64 %add3, %src4
    //    %add4 = add i64 %src5, %mul2
    //    %gep1 = getelementptr inbounds float, float addrspace(1)* %addr, i64 %add4
    //    %btc1 = bitcast float addrspace(1)* %gep1 to i32 addrspace(1)*
    //    call void @llvm.genx.GenISA.simdBlockWrite.p1i32.i32(i32 addrspace(1)* %btc1, i32 %src)
    //
    //
    // LLVM IR after optimization:
    // entry:
    //    %extr1 = extractelement <8 x i32> %payloadHeader, i32 1
    //--> %brd1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32.i32.i32(i32 %extr1, i32 0, i32 0)
    //    %extr2 = extractelement <8 x i32> %payloadHeader, i32 2
    //--> %brd2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32.i32.i32(i32 %extr2, i32 0, i32 0)
    //    %add1 = add i32 %src1, %brd2
    //    %add2 = add i32 %src2, %brd1
    //    %narrow = add nuw i32 %add1, 4
    //    %zt1 = zext i32 %narrow to i64
    //    %narrow53 = add nuw i32 %add2, 4
    //    %zt2 = zext i32 %narrow53 to i64
    //    %mul1 = mul i64 %zt1, %src3
    //    %add3 = add i64 %mul1, %zt2
    //    %mul2 = mul i64 %add3, %src4
    //    %add4 = add i64 %src5, %mul2
    //    %gep1 = getelementptr inbounds float, float addrspace(1)* %addr, i64 %add4
    //    %btc1 = bitcast float addrspace(1)* %gep1 to i32 addrspace(1)*
    //    call void @llvm.genx.GenISA.simdBlockWrite.p1i32.i32(i32 addrspace(1)* %btc1, i32 %src)
    //

    enum class InstType { BlcokMemOp, PreventScalar, CanBeScalar };

    class BlockMemOpAddrScalarizationPass : public llvm::FunctionPass, public llvm::InstVisitor<BlockMemOpAddrScalarizationPass>
    {
    public:
        static char ID;

        BlockMemOpAddrScalarizationPass();

        virtual llvm::StringRef getPassName() const override {
            return "Block Memory Op Addr Scalar";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
            AU.setPreservesAll();
            AU.addRequired<WIAnalysis>();
        }

        virtual bool runOnFunction(llvm::Function &F) override;
        void visitCallInst(llvm::CallInst& C);

    private:
        bool Changed = false;
        WIAnalysis *WI = nullptr;

        bool scalarizeAddrArithmForBlockRdWr(llvm::GenIntrinsicInst *GenI);
        void visitSimdBlockWrite(llvm::GenIntrinsicInst *GenI);
        void visitSimdBlockRead(llvm::GenIntrinsicInst *GenI);
        bool canInstBeScalarized(llvm::Instruction *InstForCheck, llvm::Instruction *Root);
        InstType checkInst(llvm::Instruction *I);
        llvm::Value *insertBroadcast(llvm::Instruction *I);

        // Data structures for address scalarization of block read/write instructions.
        llvm::SmallSet<llvm::Instruction*, 32> InstCanBeScalarized;
        llvm::DenseMap<llvm::Instruction*, llvm::Value*> ExistingBroadcasts;
    };

} // namespace IGC