/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Scalar.h>

#include <llvm/Transforms/Utils.h>

#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC
{
    ///////////////////////////////////////////////////////////////////////////
    /// Enforce a single latch for every loop header. This needs to be ran before
    /// LLVM Loop canonicalization pass as LLVM loop simplification pass sometimes
    /// decides to spilt the loop. Spliting the loop may cause functional issues
    /// in case of barriers being used and it may cause extra SIMD divergence causing
    /// performance degradation
    llvm::FunctionPass* createLoopCanonicalization();
    /**
     * Custom loop versioning.
     * Break loop into segments to expose loop invirants.
     *
     * Input loop:
     *   float t = ....;
     *   float nextT = t * CB_Load;
     *   [loop] while (t < loop_range_y)
     *   {
     *        float val0 = max(t, loop_range_x);
     *        float val1 = min(nextT, loop_range_y);
     *        float val = some_alu_func(val1 / val0);
     *        ......
     *        t = nextT;
     *        nextT *= CB_Load;
     *    }
     *
     * Transformed loop:
     *   float t = ....;
     *   float nextT = t * CB_Load;
     *   [branch] if (CB_Load > 1.0 && loop_range_x * CB_Load < loop_range_y)
     *   {
     *       [loop] while (t < loop_range_x)        // loop seg 1
     *       {
     *           float val0 = loop_range_x;
     *           float val1 = nextT;
     *           float val = some_alu_func(val1 / val0);
     *           ......
     *           t = nextT;
     *           nextT *= CB_Load;
     *       }
     *       [loop] while (t < loop_range_y/CB_Load)  // loop seg 2
     *       {
     *           float val0 = t;
     *           float val1 = nextT;
     *           float val = some_alu_func(CB_Load);    // loop invirant
     *           ......
     *           t = nextT;
     *           nextT *= CB_Load;
     *       }
     *       {                                          // loop seg 3
     *           float val0 = t;
     *           float val1 = loop_range_y;
     *           float val = some_alu_func(val1 / val0);
     *           t = nextT;
     *           nextT *= CB_Load;
     *       }
     *   } else {
     *       [loop] while (t < loop_range_y)
     *       {
     *           float val0 = max(t, loop_range_x);
     *           float val1 = min(nextT, loop_range_y);
     *           float val = some_alu_func(val1 / val0);
     *           ......
     *           t = nextT;
     *           nextT *= CB_Load;
     *        }
     *   }
     */
    class CustomLoopVersioning : public llvm::FunctionPass
    {
    public:
        static char ID;

        CustomLoopVersioning();

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequiredID(llvm::LCSSAID);
        }

        bool runOnFunction(llvm::Function& F);
        bool processLoop(llvm::Loop* loop);

        llvm::StringRef getPassName() const
        {
            return "Custom Loop Versioning";
        }

    private:
        CodeGenContext* m_cgCtx = nullptr;
        llvm::LoopInfo* m_LI = nullptr;
        llvm::DominatorTree* m_DT = nullptr;
        llvm::Function* m_function = nullptr;

        // value map from orig loop to loop seg1/seg2/seg3
        llvm::ValueToValueMapTy m_vmapToSeg1;
        llvm::ValueToValueMapTy m_vmapToSeg2;
        llvm::ValueToValueMapTy m_vmapToSeg3;

        bool isCBLoad(llvm::Value* val, unsigned& bufId, unsigned& offset);

        // create phi nodes for after loop BB
        void addPhiNodes(
            const llvm::SmallVectorImpl<llvm::Instruction*>& liveOuts,
            llvm::Loop* loopSeg1, llvm::Loop* loopSeg2,
            llvm::BasicBlock* bbSeg3, llvm::Loop* origLoop);

        bool detectLoop(llvm::Loop* loop,
            llvm::Value*& var_range_x, llvm::Value*& var_range_y,
            llvm::LoadInst*& var_MediumFactor_preHdr,
            llvm::Value*& var_t_preHdr,
            llvm::Value*& var_nextT_preHdr);

        void linkLoops(llvm::Loop* loopSeg1, llvm::Loop* loopSeg2,
            llvm::BasicBlock* afterLoop);

        void rewriteLoopSeg1(llvm::Loop* loop,
            llvm::Value* range_x, llvm::Value* range_y);

        void hoistSeg2Invariant(llvm::Loop* loop,
            llvm::Instruction* fmul, llvm::Value* cbVal);

        void rewriteLoopSeg2(llvm::Loop* loop,
            llvm::Value* range_y, llvm::Value* cbVal);

        void rewriteLoopSeg3(llvm::BasicBlock* bb,
            llvm::Value* range_y);
    };


    llvm::LoopPass* createLoopHoistConstant();

    llvm::LoopPass* createDisableLICMForSpecificLoops();

} // namespace IGC

