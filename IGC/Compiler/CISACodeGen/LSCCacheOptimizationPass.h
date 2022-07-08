/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/**
 * @file  LSCCacheOptimizationPass.h
 * @author  Konstantin Rebrov
 *
 * @brief  This file implements the LSCCacheOptimizationPass
 * This pass performs an optimization upon the Load Store Cache, makes eligible store instructions go into the L1 cache
 * Instead of the L3 cache, which is the default setting.
 * It utilizes the L1 cache for store instructions that are in these four memory regions:
 * RTAsyncStack, SWStack, SWHotZone, RTSyncStack
 *
 * @details  This pass examines all store instructions, and if a store instruction is identified as being eligible,
 * and if the optimization is necessary and possible, the pass performs a Read Write Modify operation on the store instruction.
 *
 * The requirements for a store instruction to go into the L1 cache are:
 *   The address for the store needs to be 16-byte aligned.
 *   The size of the stored data needs to be a multiple of 16 bytes.
 *   .ca on load and store instructions should not be marked as uncached
 * If these requirements are not satisfied then the store instruction is expanded to include the padding around the stored data.
 * This padding is preliminarily loaded into a virtual register to save those values.
 *
 * green blocks represent a dword that we don't want to overwrite (leave it as is)
 *   this is a padding blocks surrounding the (red) original memory location of the store instruction
 *
 * red blocks represent a dword that we actually do want to overwrite, specifically the old value
 *   this is the memory location that we want to store the new value into
 *   it is the region of memory which is addressed to by the original pointer operand of the input store instruction
 *
 * blue blocks represent the new value that we want to overwrite on top of the red dword
 */

#pragma once

#include "common/StringMacros.hpp"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"  // for suppressing LLVM warnings
#include <llvm/ADT/StringRef.h>         // for llvm::StringRef
#include <llvm/IR/LLVMContext.h>        // for llvm:LLVMContext
#include <llvm/IR/Function.h>           // for llvm::Function
#include <llvm/Pass.h>                  // for llvm::FunctionPass
#include <llvm/IR/InstVisitor.h>        // for llvm::InstVisitor
#include <llvm/IR/Instructions.h>       // for llvm::StoreInst
#include "common/LLVMWarningsPop.hpp"   // for suppressing LLVM warnings

class LSCCacheOptimizationPass : public llvm::FunctionPass,
                                 public llvm::InstVisitor<LSCCacheOptimizationPass>
{
public:
    LSCCacheOptimizationPass() : FunctionPass(ID), changed_IR(false), current_function(nullptr), current_context(nullptr) {}

    llvm::StringRef getPassName() const override
    {
        return "LSCCacheOptimizationPass";
    }

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<IGC::CodeGenContextWrapper>();
    }

    /**
     * This method is responsible for running the pass on the LLVM IR Function.
     * It is the main entry point for the LLVM IR analysis and transformations.
     * This method calls helper methods to achieve this task.
     *
     * @param function  The LLVM IR function to run the pass on.
     *
     * @return bool  It returns true if the pass modified the IR in the Function.
     *               It returns false if the pass did not find any eligible code constructs to modify,
     *               and so it didn't modify the IR.
     */
    bool runOnFunction(llvm::Function& function) override;

    /**
     * This method takes the given store instruction, performs a heuristics analysis to determine if it can be optimized,
     * and then performs a RMW operation in order to optimize it.
     * If the caching policy for that store instruction is not "L1 unchached" then this method performs the optimization
     * for the store instruction to go into the L1 cache instead of the L3 cache.
     *
     * My pass will optimize LLVM IR store instruction where the destination pointer is inside of of these 4 memory regions:
     *  RTAsyncStack, SWStack, SWHotZone, RTSyncStack
     * In order for the store instruction to go into the L1 cache
     *  The size of the stored data must be a multiple of 16 bytes AND
     *  The starting memory address of the data must be a multiple of 16 bytes, meaning that it is aligned at a 16 bytes boundary.
     *
     * If the store instruction does not satisfy these criterias then a Read Modify Write operation is performed on the stored data.
     * It does this by expanding the store to include the padding around the actual data,
     * growing the size of the store to be the next largest multiple of 16 bytes in order to satisfy these criterias for L1 caching.
     *
     * The implementation details of the RMW operation depend on the "shape" of the store instruction,
     * which includes the size of the stored data, the offset of the pointer operand from the previous 16 bytes aligned address
     * (the size of the left padding), and the distance from the end of the stored data to the next 16 bytes aligned address
     * (the size of the right padding).
     *
     * This method may modify changed_IR if it successfully performs the transformation.
     *
     * @param storeInst  The store instruction to analyze and optimize.
     *                   Each store instruction is examined individually in isolation.
     */
    void visitStoreInst(llvm::StoreInst& storeInst);
    //
    static char ID;
private:
#if 0
    /**
     * This utility method is only used for testing purposes.
     * Theoretically we may be having a store instruction which straddles across three 16 bytes size chunks.
     * This is the fourth case of data access alignment.
     * However in the shaders that this pass was run on did not have such kinds of store instructions.
     * Finds an "anchor" in the LLVM IR Function to RTStack,
     * and constructs a 48 wide store instruction at an offset into the RTStack.
     * This is supposed to be run before the visit() method, because it creates an input store instruction,
     * which will get processed by the visitStoreInst() method.
     *
     * @param function  The LLVM IR function inside which to look for an "anchor" to RTStack,
     *                  and insert a store instruction at an offset into it.
     *
     * @return bool  It returns true if the operation was successful.
     *               It returns false if it couldn't find an "anchor" to RTStack.
     */
    bool create_48_wide_store(llvm::Function& function);
#endif

    /// Indicates whether the pass changed any IR code.
    /// This variable may be modified by any of the methods to store that status.
    bool changed_IR = false;
    /// A pointer to the current LLVM IR Function which is the input for that pass.
    llvm::Function* current_function = nullptr;
    /// A pointer to the current LLVMContext, which can be used by various IRBuilder's methods for instruction creation.
    llvm::LLVMContext* current_context = nullptr;

    IGC::CodeGenContext* m_CGCtx = nullptr;
};

namespace IGC {
    //===----------------------------------------------------------------------===//
    //
    // This pass optimizes store instructions where the destination pointer is inside one of these 4 memory regions:
    // RTAsynctack, SWStack, SWHotZone, RTSynctack, for increasing hits of the store instructions into the L1 cache vs L3 cache.
    // It performs a RMW operation on the stored data to achieve this optimization depending on custom heuristics.
    //
    inline llvm::FunctionPass* createLSCCacheOptimizationPass()
    {
        return new LSCCacheOptimizationPass();
    }
}  // namespace IGC

