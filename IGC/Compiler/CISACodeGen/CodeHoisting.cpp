/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
/*
    This pass try's to hoist the URBWrite instruction in the shader code generated
    There are 2 reasons we need to do this
    1) Since we emit all the outputs only at the end of the program, this causes the live ranges for the registers holding the output data to be really long
    2) Since live ranges are long, the shaders can spill due to registers being used up to hold the output data

    By hoisting the URBWrite instruction we try to free up registers sooner, so spilling can be avoided
*/

#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/Stats.hpp"
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Dominators.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/ADT/PostOrderIterator.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/CodeHoisting.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"


using namespace llvm;
using namespace IGC::Debug;

namespace IGC {

    char CodeHoisting::ID = 0;
#define PASS_FLAG "CodeHoisting"
#define PASS_DESCRIPTION "Code Hoisting"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(CodeHoisting, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_END(CodeHoisting, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        CodeHoisting::CodeHoisting() : llvm::FunctionPass(ID)
    {
        initializeCodeHoistingPass(*PassRegistry::getPassRegistry());
    }

    bool CodeHoisting::runOnFunction(Function& F)
    {
        bool everMadeChange = false;
        CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
        PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
        LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

        basicBlockReadInstructionMap.clear();

        gatherLastURBReadInEachBB(F);

        // Next Process all basic blocks in post-dominator-tree post-order
        // to identify location for the URBWrites to sink to
        for (po_iterator<DomTreeNode*> domIter = po_begin(PDT->getRootNode()),
            domEnd = po_end(PDT->getRootNode()); domIter != domEnd; ++domIter)
        {
            auto bb = domIter->getBlock();
            if (bb != nullptr)
            {
                hoistURBWriteInBB(*(bb));
            }
        }

        // MapVector preserves the insertion order during iteration
        MapVector<llvm::Instruction*, llvm::Instruction*>::iterator it;
        for (auto it : instMovDataMap)
        {
            if (it.second != nullptr)
            {
                if (llvm::dyn_cast<llvm::PHINode>((it.second)->getNextNode()))
                {
                    // if we are inserting after a PHINode,
                    // make sure we insert at the end of all PHINodes in that basic block
                    llvm::Instruction* currInst = (it.second)->getNextNode();
                    while (llvm::dyn_cast<llvm::PHINode>(currInst))
                    {
                        currInst = currInst->getNextNode();
                    }

                    (it.first)->moveBefore(currInst);
                }
                else
                {
                    (it.first)->moveBefore((it.second)->getNextNode());
                }
                everMadeChange = true;
            }
        }
        DumpLLVMIR(ctx, "CodeHoisting");

        instMovDataMap.clear();

        return everMadeChange;
    }

    Instruction* CodeHoisting::searchBackForAliasedURBRead(
        Instruction* urbWrite)
    {
        BasicBlock::iterator I = BasicBlock::iterator(urbWrite);
        BasicBlock::iterator IB = urbWrite->getParent()->begin();
        if (I == IB)
        {
            // if URBWrite is the first inst
            return nullptr;
        }

        Value* woffV = urbWrite->getOperand(0);
        if (ConstantInt * woffCI = dyn_cast<ConstantInt>(woffV))
        {
            unsigned woff = int_cast<unsigned>(woffCI->getZExtValue());
            do {
                --I;
                GenIntrinsicInst* intrin = dyn_cast<GenIntrinsicInst>(I);
                if (intrin && intrin->isGenIntrinsic(GenISAIntrinsic::GenISA_URBRead))
                {
                    Value* roffV = intrin->getOperand(1);
                    if (ConstantInt * roffCI = dyn_cast<ConstantInt>(roffV))
                    {
                        unsigned roff = int_cast<unsigned>(roffCI->getZExtValue());
                        if (roff == woff || roff == woff + 1)
                        {
                            return intrin;
                        }
                    }
                    else
                    {
                        return intrin;
                    }
                }
            } while (I != IB);
            return nullptr;
        }
        else
        {
            do {
                --I;
                GenIntrinsicInst* intrin = dyn_cast<GenIntrinsicInst>(I);
                if (intrin && intrin->isGenIntrinsic(GenISAIntrinsic::GenISA_URBRead))
                {
                    return intrin;
                }
            } while (I != IB);
            return nullptr;
        }
    }

    /// Determine whether it is safe to elevate the specified machine
    /// instruction out of its current block into a predecessor
    bool CodeHoisting::isSafeToHoistURBWriteInstruction(
        Instruction* urbWrite,
        Instruction*& tgtInst)
    {
        // tgtInst contains the location to move the URBWrite instruction to
        const int sourceStartOffset = 0, sourceEndOffset = 10;
        SmallVector<Instruction*, 10> argDefs;

        // gather all arg def instructions
        // URBWrite(offset, mask, d0, d1, d2, d3, d4, d5, d6, d7)
        for (int i = sourceStartOffset; i < sourceEndOffset; i++)
        {
            Instruction* argDef =
                dyn_cast<Instruction>(urbWrite->getOperand(i));
            if (!argDef)
            {
                continue;
            }
            argDefs.push_back(argDef);
        }

        if (argDefs.size() == 0)
        {
            return false;
        }

        // URBWrite basic block
        BasicBlock* uwBB = urbWrite->getParent();
        bool localHoist = false;
        tgtInst = searchBackForAliasedURBRead(urbWrite);
        if (tgtInst)
        {
            localHoist = true;
        }

        // iterate all args to see if we can do cross BB move quickly
        if (!localHoist)
        {
            for (auto argDef : argDefs)
            {
                if (uwBB == argDef->getParent())
                {
                    localHoist = true;
                    break;
                }
            }
        }

        if (localHoist)
        {
            // search for latest position to move URBWrite to, it's either
            // the last local arg def, or last aliased URBRead.
            for (auto argDef : argDefs)
            {
                if (uwBB == argDef->getParent())
                {
                    if (tgtInst == nullptr)
                    {
                        tgtInst = argDef;
                    }
                    else
                        if (isInstPrecede(tgtInst, argDef))
                        {
                            tgtInst = argDef;
                        }
                }
            }
            assert(tgtInst != nullptr);
            return true;
        }

        // no URBRead in URBWrite BB & all args coming from other BBs
        // we can move to nearest common post dominator, or the beginning
        // of URBWrite BB.
        bool moveToHead = false;

        for (auto argDef : argDefs)
        {
            BasicBlock* defBB = argDef->getParent();
            if (PDT->dominates(uwBB, defBB))
            {
                // if in different loop
                Loop* defLoop = LI->getLoopFor(defBB);
                Loop* uwLoop = LI->getLoopFor(uwBB);
                if (defLoop != uwLoop)
                {
                    moveToHead = true;
                    break;
                }

                // if there are URBRead between arg def & URBWrite
                // TODO: add alias checking
                for (auto I : basicBlockReadInstructionMap)
                {
                    BasicBlock* urBB = I.second->getParent();
                    if (PDT->dominates(uwBB, urBB) &&
                        PDT->dominates(urBB, defBB))
                    {
                        moveToHead = true;
                        break;
                    }
                }

                if (tgtInst == nullptr)
                {
                    tgtInst = argDef;
                }
                else
                    if (tgtInst->getParent() != defBB)
                    {
                        // candidate position not the same BB as current arg def
                        // find the common post dominator
                        BasicBlock* cmnDom = PDT->findNearestCommonDominator(
                            tgtInst->getParent(), defBB);
                        assert(cmnDom == tgtInst->getParent() ||
                            cmnDom == defBB);

                        if (cmnDom == defBB)
                        {
                            tgtInst = argDef;
                        }
                    }
                    else
                    {
                        // candidate position is the same BB as current arg def
                        if (isInstPrecede(tgtInst, argDef))
                        {
                            tgtInst = argDef;
                        }
                    }
            }
            else
            {
                moveToHead = true;
                break;
            }
        }

        if (moveToHead)
        {
            tgtInst = &(*uwBB->getFirstInsertionPt());
        }

        assert(tgtInst != nullptr);
        return true;
    }

    void CodeHoisting::hoistURBWriteInBB(BasicBlock& blk)
    {
        if (blk.empty())
            return;

        // Walk the basic block bottom-up
        BasicBlock::iterator I = blk.end();
        --I;
        bool processedBegin = false;
        do {
            Instruction* inst = &(*I);

            // Predecrement I (if it's not begin) so that it isn't invalidated by sinking.
            processedBegin = (I == blk.begin());
            if (!processedBegin)
                --I;

            auto intrinsicInst = dyn_cast<GenIntrinsicInst>(inst);
            if (intrinsicInst != nullptr &&
                isURBWriteIntrinsic(intrinsicInst))
            {
                // we found a urb write instruction to try and hoist
                // tgtInst contains the location to move the URBWrite instruction to
                Instruction* tgtInst = nullptr;

                // if it is possible to hoist the URB write instruction, add it to the instrMovDataMap to be moved later
                if (isSafeToHoistURBWriteInstruction(inst, tgtInst))
                {
                    // tgtInst contains the location to move the URBWrite instruction to
                    instMovDataMap[inst] = tgtInst;
                }
            }
            // If we just processed the first instruction in the block, we're done.
        } while (!processedBegin);
    }

    void CodeHoisting::gatherLastURBReadInEachBB(llvm::Function& F)
    {
        for (auto BI = F.begin(), BE = F.end(); BI != BE; ++BI)
        {
            for (BasicBlock::reverse_iterator II = BI->rbegin(), IE = BI->rend();
                II != IE; ++II)
            {
                GenIntrinsicInst* intrin = dyn_cast<GenIntrinsicInst>(&*II);
                if (intrin && intrin->isGenIntrinsic(GenISAIntrinsic::GenISA_URBRead))
                {
                    basicBlockReadInstructionMap[&(*BI)] = intrin;
                    break;
                }
            }
        }
    }

} // namespace IGC
