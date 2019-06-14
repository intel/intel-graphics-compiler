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
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "ThreadCombining.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/LLVMUtils.h"

char IGC::ThreadCombining::ID = 0;

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-threadcombining"
#define PASS_DESCRIPTION "Perform analysis and apply optimization to combine number of software threads"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(ThreadCombining, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ThreadCombining, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool ThreadCombining::isBarrier(llvm::Instruction &I) const
{
    if (llvm::GenIntrinsicInst* pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(&I))
    {
        if (pIntrinsic->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_threadgroupbarrier)
        {
            return true;
        }
    }
    return false;
}

bool ThreadCombining::isSLMUsed(llvm::Instruction* I) const
{
    bool ptrType = false;
    uint addrSpace = 0;

    if (llvm::isa<llvm::LoadInst>(I))
    {
        LoadInst* inst = dyn_cast<LoadInst>(I);
        if (inst->getPointerOperand())
        {
            addrSpace = inst->getPointerAddressSpace();
        }
    }
    else if (llvm::isa<llvm::StoreInst>(I))
    {
        StoreInst* inst = dyn_cast<StoreInst>(I);
        if (inst->getPointerOperand())
        {
            addrSpace = inst->getPointerAddressSpace();
        }
    }

    if (addrSpace == ADDRESS_SPACE_LOCAL)
    {
        ptrType = true;
    }
    return ptrType;
}

unsigned int ThreadCombining::GetthreadGroupSize(llvm::Module &M, dim dimension)
{
    unsigned int threadGroupSize = 0;
    llvm::GlobalVariable* pGlobal = nullptr;
    switch (dimension)
    {
    case ThreadGroupSize_X:
        pGlobal = M.getGlobalVariable("ThreadGroupSize_X");
        threadGroupSize = static_cast<unsigned int>(
            (llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue()));
        break;
    case ThreadGroupSize_Y:
        pGlobal = M.getGlobalVariable("ThreadGroupSize_Y");
        threadGroupSize = static_cast<unsigned int>(
            (llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue()));
        break;
    case ThreadGroupSize_Z:
        pGlobal = M.getGlobalVariable("ThreadGroupSize_Z");
        threadGroupSize = static_cast<unsigned int>(
            (llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue()));
        break;
    default:
        assert(0);
        break;
    }
    return threadGroupSize;
}

void ThreadCombining::SetthreadGroupSize(llvm::Module &M, llvm::Constant* size, dim dimension)
{
    llvm::GlobalVariable* pGlobal = nullptr;
    switch (dimension)
    {
    case ThreadGroupSize_X:
        pGlobal = M.getGlobalVariable("ThreadGroupSize_X");
        break;
    case ThreadGroupSize_Y:
        pGlobal = M.getGlobalVariable("ThreadGroupSize_Y");
        break;
    case ThreadGroupSize_Z:
        pGlobal = M.getGlobalVariable("ThreadGroupSize_Z");
        break;
    default:
        assert(0);
        break;
    }
    pGlobal->setInitializer(size);
}

/// \brief Create a kernel that will loop over the modified kernel
/// LoopCount = (threadGroupSize_X / newGroupSizeX) * (threadGroupSize_Y / newGroupSizeY) *(numBarriers + 1)

void ThreadCombining::CreateLoopKernel(
    llvm::Module& M,
    unsigned int newGroupSizeX,
    unsigned int newGroupSizeY,
    unsigned int threadGroupSize_X,
    unsigned int threadGroupSize_Y,
    Function* newFunc,
    llvm::IRBuilder<> builder)
{
    unsigned int numLoopsX = threadGroupSize_X / newGroupSizeX;
    unsigned int numLoopsY = threadGroupSize_Y / newGroupSizeY;
    unsigned int numBarriers = m_barriers.size();

    BasicBlock* mainEntry = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* BarrierLoopEntry = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* XLoopEntry = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* YLoopEntry = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* innerYLoop = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* exitXLoop = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* barrier = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* exitBarrierLoop = BasicBlock::Create(M.getContext(), "", m_kernel);

    auto barrierIterator = m_LiveRegistersPerBarrier.begin();

    // mainEntry
    builder.SetInsertPoint(mainEntry);
    Value* iterBarriers = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "iterBarriers");
    Value* iterX = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "iterX");
    Value* iterY = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "iterY");

    // This is a map of the live register and the register to store it
    std::map<llvm::Instruction*, Value*> regToAllocaMap;
    unsigned int totalSize = numLoopsX * numLoopsY;

    // Create a vector that stores the live registers per iteration
    // size of the vector will be the total number of iterations
    for (auto& liveInst : m_aliveAcrossBarrier)
    {
        llvm::Instruction* aliveInst = dyn_cast<Instruction>(liveInst);
        llvm::VectorType* pVecType = llvm::VectorType::get(aliveInst->getType(), totalSize);
        llvm::Value* inst = builder.CreateAlloca(pVecType);
        regToAllocaMap[aliveInst] = inst;
    }

    // Read threadIDX and threadIDY
    Function* ThreadIDFN = GenISAIntrinsic::getDeclaration(&M, GenISAIntrinsic::GenISA_DCL_SystemValue, builder.getFloatTy());
    Value* threadIDX = builder.CreateCall(ThreadIDFN, builder.getInt32(THREAD_ID_IN_GROUP_X));
    Value* threadIDY = builder.CreateCall(ThreadIDFN, builder.getInt32(THREAD_ID_IN_GROUP_Y));
    threadIDX = builder.CreateBitCast(threadIDX, builder.getInt32Ty());
    threadIDY = builder.CreateBitCast(threadIDY, builder.getInt32Ty());
    builder.CreateBr(BarrierLoopEntry);

    // BarrierLoopEntry:
    builder.SetInsertPoint(BarrierLoopEntry);
    builder.CreateStore(builder.getInt32(0), iterBarriers);
    builder.CreateBr(XLoopEntry);

    // XLoopEntry:
    builder.SetInsertPoint(XLoopEntry);
    Value* BarrierNum = builder.CreateLoad(iterBarriers);
    BarrierNum = builder.CreateBitCast(BarrierNum, builder.getInt32Ty());
    builder.CreateStore(builder.getInt32(0), iterX);
    builder.CreateBr(YLoopEntry);

    // YLoopEntry:
    builder.SetInsertPoint(YLoopEntry);
    Value* X = builder.CreateLoad(iterX);
    Value* correctedIdX = builder.CreateAdd(threadIDX, builder.CreateMul(X, builder.getInt32(newGroupSizeX)));
    builder.CreateStore(builder.getInt32(0), iterY);
    builder.CreateBr(innerYLoop);

    // innerYLoop:
    // In the inner loop, calculate the modified thread ids
    // label InnerLoop:
    // correctedIdX = threadIDX + iterX * newGroupSizeX
    // correctedIdY = threadIDY + iterY * newGroupSizeY
    // executeKernel(correctedIdX, correctedIdY, barrierNum, liveregisterargs)

    builder.SetInsertPoint(innerYLoop);
    Value* Y = builder.CreateLoad(iterY);
    Value* correctedIdY = builder.CreateAdd(threadIDY, builder.CreateMul(Y, builder.getInt32(newGroupSizeY)));
    std::vector<llvm::Value*> callArgs;
    callArgs.push_back(correctedIdX);
    callArgs.push_back(correctedIdY);
    callArgs.push_back(BarrierNum);

    for (auto& it : m_aliveAcrossBarrier)
    {
        llvm::Instruction* aliveInst = dyn_cast<Instruction>(it);
        Value* rowMul = builder.CreateMul(X, builder.getInt32(numLoopsY));
        Value* index = builder.CreateAdd(rowMul, Y);
        Value* indexes[] = { builder.getInt32(0), index };
        Value* gepPtr = builder.CreateGEP(regToAllocaMap[aliveInst], indexes);
        callArgs.push_back(gepPtr);
    }

    builder.CreateCall(newFunc, callArgs);
    Y = builder.CreateAdd(Y, builder.getInt32(1));
    builder.CreateStore(Y, iterY);
    Value* cond = builder.CreateICmp(CmpInst::ICMP_ULT, Y, builder.getInt32(numLoopsY));
    builder.CreateCondBr(cond, innerYLoop, exitXLoop);

    // exitXloop
    builder.SetInsertPoint(exitXLoop);
    X = builder.CreateAdd(X, builder.getInt32(1));
    builder.CreateStore(X, iterX);
    cond = builder.CreateICmp(CmpInst::ICMP_ULT, X, builder.getInt32(numLoopsX));
    builder.CreateCondBr(cond, YLoopEntry, barrier);

    // barrier
    builder.SetInsertPoint(barrier);
    Function* barrierFn = GenISAIntrinsic::getDeclaration(&M, GenISAIntrinsic::GenISA_threadgroupbarrier);
    builder.CreateCall(barrierFn);

    Value* barriernum = builder.CreateLoad(iterBarriers);
    barriernum = builder.CreateAdd(barriernum, builder.getInt32(1));
    builder.CreateStore(barriernum, iterBarriers);
    Value* conditionBarrier = builder.CreateICmp(CmpInst::ICMP_ULE, barriernum, builder.getInt32(numBarriers));
    builder.CreateCondBr(conditionBarrier, XLoopEntry, exitBarrierLoop);

    // exitBarrierLoop:
    builder.SetInsertPoint(exitBarrierLoop);
    builder.CreateRetVoid();
}

void ThreadCombining::FindRegistersAliveAcrossBarriers(llvm::Function* m_kernel, llvm::Module& M)
{
    DominatorTreeWrapperPass* DT = &getAnalysis<DominatorTreeWrapperPass>(*m_kernel);

    for (auto BI = m_kernel->begin(); BI != m_kernel->end(); ++BI)
    {
        for (auto II = BI->begin(); II != BI->end(); II++)
        {
            Instruction* inst = &(*II);
            if (isSLMUsed(inst))
            {
                m_SLMUsed = true;
            }
            if (isBarrier(*inst))
            {
                m_barriers.push_back(inst);
                std::set<llvm::Instruction*> empty;
                m_LiveRegistersPerBarrier.insert(std::pair<llvm::Instruction*, std::set<llvm::Instruction*>>(inst, empty));
            }

            for (auto barrierInst = m_barriers.begin(); barrierInst != m_barriers.end(); ++barrierInst)
            {
                for (unsigned int i = 0; i < inst->getNumOperands(); ++i)
                {
                    if (Instruction* I = dyn_cast<Instruction>(inst->getOperand(i))) // If the last barrier does not dominate the instruction then we need to store and restore
                    {
                        if (!DT->getDomTree().dominates(*barrierInst, I))
                        {
                            // Optimization: check if the live register can be moved to the entry block,
                            // this way we skip the store and restore across barriers
                            bool canMoveInstructionToEntryBlock = false;
                            if (!I->mayReadOrWriteMemory())
                            {
                                canMoveInstructionToEntryBlock = true;

                                for (unsigned int j = 0; j < I->getNumOperands(); j++)
                                {
                                    if (isa<Instruction>(I->getOperand(j)))
                                    {
                                        canMoveInstructionToEntryBlock = false;
                                        break;
                                    }
                                }
                                //optimization to reduce the number of live registers crossing barrier
                                //Move them later in the entry block of new function
                                if (canMoveInstructionToEntryBlock)
                                {
                                    m_instructionsToMove.insert(I);
                                }
                            }
                            if (!canMoveInstructionToEntryBlock)
                            {
                                m_LiveRegistersPerBarrier[*barrierInst].insert(I); // Insert the instruction as one that has to be stored and then restored
                                m_aliveAcrossBarrier.insert(I);
                            }
                        }
                    }
                }
            }
        }
    }
}

bool ThreadCombining::canDoOptimization(Function* m_kernel, llvm::Module& M)
{
    PostDominatorTree* PDT = &getAnalysis<PostDominatorTreeWrapperPass>(*m_kernel).getPostDomTree();

    FindRegistersAliveAcrossBarriers(m_kernel, M);

    // Check if any of the barriers are within control flow
    bool anyBarrierWithinControlFlow = false;
    for (auto& barrier : m_barriers)
    {
        if (!PDT->dominates(barrier->getParent(), &m_kernel->getEntryBlock()))
        {
            anyBarrierWithinControlFlow = true;
        }
    }

    //No optimization if no SLM used - number of dispatchable thread groups is limited by SLM space, only then we have perf issue
    //No optimization if thread group size Z is not equal to 1 - keep for simpler cases
    //No optimization if barrier is within control flow - to keep it simple for now else gets complex
    unsigned int threadGroupSize_X = GetthreadGroupSize(M, ThreadGroupSize_X);
    unsigned int threadGroupSize_Y = GetthreadGroupSize(M, ThreadGroupSize_Y);
    unsigned int threadGroupSize_Z = GetthreadGroupSize(M, ThreadGroupSize_Z);

    if (threadGroupSize_X == 1 ||
        threadGroupSize_Y == 1 ||
        threadGroupSize_Z != 1 ||
        (!m_SLMUsed && IGC_IS_FLAG_DISABLED(EnableThreadCombiningWithNoSLM))||
        anyBarrierWithinControlFlow)
    {
        return false;
    }

    return true;
}

/// Create a New Kernel and do the following
/// -> Copy all instructions from the old kernel to the new Kernel
/// -> Do the following for each barrier
///    -> Add stores to all the registers that are alive across the barrier right
///       before the barrier
///    -> Add loads to all the live registers right after the barrier
///    -> Replace the barrier with a return instruction
/// -> Add a new entry block with a jump table to jump to the basic block to start execution based on the function argument
///    provided by the loop kernel

void ThreadCombining::CreateNewKernel(llvm::Module& M,
                                      llvm::IRBuilder<> builder,
                                      llvm::Function* newFunc)
{

    DominatorTreeWrapperPass* DT = &getAnalysis<DominatorTreeWrapperPass>(*m_kernel);

    // Move all instructions from the the old kernel to the new function
    newFunc->getBasicBlockList().splice(newFunc->begin(), m_kernel->getBasicBlockList(), m_kernel->begin(), m_kernel->end());

    // Check if there is at least one barrier
    if (!m_barriers.empty())
    {
        // On every invocation of the kernel we pass a barrier number that indexes a vector of Basic block
        // address to jump to. This vector contains the address of the entry block and the address of every
        // BB following a barrier instruction

        std::vector<llvm::BasicBlock* > gotoAddresses;
        gotoAddresses.push_back(&newFunc->getEntryBlock());

        auto firstBarrier = m_barriers.begin();
        builder.SetInsertPoint(*firstBarrier);
        auto argIter = newFunc->arg_begin();
        argIter++; argIter++; argIter++; // argIter now points to the first live register in m_aliveAcrossBarrier

        // Add stores for all registers that are live across the first barrier right before the
        // first barrier
        for (auto& aliveInst : m_aliveAcrossBarrier)
        {
            if (DT->getDomTree().dominates(aliveInst, *firstBarrier))
            {
                builder.CreateStore(aliveInst, &(*argIter));
            }
            argIter++;
        }

        auto lastBarrier = --(m_barriers.end());

        // Enter this loop when there are two or more barriers
        // In this loop, stores and loads of live registers are added
        // before and after the barrier instructions.
        for (auto it = firstBarrier; it != lastBarrier; ++it)
        {
            auto barIter = it;
            llvm::Instruction* currentBarrier = *barIter;
            llvm::Instruction* nextBarrier = *(++barIter);
            // Store all the live registers right before the next barrier
            builder.SetInsertPoint(nextBarrier);
            auto argIter = newFunc->arg_begin();
            argIter++; argIter++; argIter++;

            for (auto& aliveInst : m_aliveAcrossBarrier)
            {
                 // m_LiveRegistersPerBarrier stores for each barrier, all the registers that are alive across that
                // barrier. We check if a register is alive across the nextBarrier and store all of those values in
                // a vector that is passed back to the calling function so that we can retrieve it when we execute
                // the code after nextBarrier
                if (m_LiveRegistersPerBarrier[nextBarrier].find(aliveInst) != m_LiveRegistersPerBarrier[nextBarrier].end())
                {
                    builder.CreateStore(aliveInst, &(*argIter));
                }
                argIter++;
            }

            // Add loads of all the live registers right after the currentBarrier instruction
            // change the uses of that register to the new value
            llvm::Instruction* pointToInsert = currentBarrier->getNextNode();
            builder.SetInsertPoint(pointToInsert);
            argIter = newFunc->arg_begin();
            argIter++; argIter++; argIter++;

            for (auto& it : m_aliveAcrossBarrier)
            {
                llvm::Instruction* inst = dyn_cast<llvm::Instruction>(it);
                // Add loads for all registers that are alive across the CurrentBarrier and replace all the uses of that
                // register between the current and next barrier with the new load
                if (m_LiveRegistersPerBarrier[currentBarrier].find(inst) != m_LiveRegistersPerBarrier[currentBarrier].end())
                {
                    Value* loadLiveReg = builder.CreateLoad(&(*argIter));
                    SmallVector<Instruction*, 10> usesToReplace;
                    for (auto use_it = it->use_begin();
                        use_it != it->use_end();
                        use_it++)
                    {
                        Instruction *useInst = dyn_cast<Instruction>(use_it->getUser());

                        // Check if the use instruction lies between the current barrier and the next
                        if (DT->getDomTree().dominates(currentBarrier, useInst) &&
                            !DT->getDomTree().dominates(nextBarrier, useInst))
                        {
                            usesToReplace.push_back(useInst);
                        }
                    }

                    for (auto it2 : usesToReplace)
                    {
                        it2->replaceUsesOfWith(inst, loadLiveReg);
                    }
                }
                argIter++; // if the register is not live across the currentBarrier, don't add the load, proceed to next register
            }
        }

        // add loads after the last barrier instruction in the kernel
        builder.SetInsertPoint((*lastBarrier)->getNextNode());
        argIter = newFunc->arg_begin();
        argIter++; argIter++; argIter++;

        for (auto& it2 : m_aliveAcrossBarrier)
        {
            llvm::Instruction* inst = dyn_cast<llvm::Instruction>(it2);

            if (m_LiveRegistersPerBarrier[*lastBarrier].find(inst) != m_LiveRegistersPerBarrier[*lastBarrier].end())
            {
                Value* loadLiveReg = builder.CreateLoad(&(*argIter));
                SmallVector<Instruction*, 10> usesToReplace;
                for (auto use_it = it2->use_begin();
                    use_it != it2->use_end();
                    use_it++)
                {
                    Instruction *useInst = dyn_cast<Instruction>(use_it->getUser());

                    if (DT->getDomTree().dominates(*lastBarrier, useInst))
                    {
                        usesToReplace.push_back(useInst);
                    }
                }

                for (auto it2 : usesToReplace)
                {
                    it2->replaceUsesOfWith(inst, loadLiveReg);
                }
            }
            argIter++;
        }

        // Replace all the barrier instructions with return instructions
        for (auto& barrier : m_barriers)
        {
            BasicBlock* oldBasicBlock = barrier->getParent();
            BasicBlock* NewBasicBlock = oldBasicBlock->splitBasicBlock(barrier);
            gotoAddresses.push_back(NewBasicBlock);
            IGCLLVM::TerminatorInst* oldTermInst = oldBasicBlock->getTerminator();
            IGCLLVM::TerminatorInst* newTermInst = ReturnInst::Create(M.getContext());
            llvm::ReplaceInstWithInst(oldTermInst, newTermInst);
        }

        // Remove barrier instructions from the new kernel
        for (auto barrier : m_barriers)
        {
            barrier->eraseFromParent();
        }

        // Create a Jump table to branch to the required BB based on the barriernum
        // argument
        BasicBlock* oldEntry = &newFunc->getEntryBlock();
        BasicBlock* newEntry = BasicBlock::Create(M.getContext(), "new_entry", newFunc, oldEntry);

        builder.SetInsertPoint(newEntry);
        argIter = newFunc->arg_begin();
        argIter++; argIter++;
        unsigned int numBarriers = m_barriers.size();
        llvm::Value* barriernum = &(*argIter);
        Value* barrierVal = builder.CreateBitCast(barriernum, builder.getInt32Ty());
        llvm::SwitchInst* pSwitchInst = builder.CreateSwitch(barrierVal, gotoAddresses[0], numBarriers);

        for (unsigned int i = 0; i <= numBarriers; i++)
        {
            llvm::ConstantInt* caseClause = builder.getInt32(i);
            pSwitchInst->addCase(caseClause, gotoAddresses[i]);
        }
    }

    // Move the live registers marked as safe to move to the new entry block
    BasicBlock* newEntry = &newFunc->getEntryBlock();

    for (auto& instruction : m_instructionsToMove)
    {
        Instruction* instructionToMove = dyn_cast<Instruction>(instruction);
        instructionToMove->moveBefore(&*((newEntry->getInstList().begin())));
    }

    // Replace all the threadIDs in the new kernel with the modified id from the function arguments
    auto it = newFunc->arg_begin();
    Value* IDx = &(*it++);
    Value* IDy = &(*it++);

    for (auto& BI : *newFunc)
    {
        for (auto& inst : BI)
        {
            if (GenIntrinsicInst* b = dyn_cast<GenIntrinsicInst>(&inst))
            {
                if (b->getIntrinsicID() == GenISAIntrinsic::GenISA_DCL_SystemValue)
                {
                    if (cast<ConstantInt>(b->getOperand(0))->getZExtValue() == THREAD_ID_IN_GROUP_X)
                    {
                        builder.SetInsertPoint(&(*newFunc->getEntryBlock().begin()));
                        Value* IDxF = builder.CreateBitCast(IDx, b->getType());
                        b->replaceAllUsesWith(IDxF);
                    }
                    if (cast<ConstantInt>(b->getOperand(0))->getZExtValue() == THREAD_ID_IN_GROUP_Y)
                    {
                        builder.SetInsertPoint(&(*newFunc->getEntryBlock().begin()));
                        Value* IDyF = builder.CreateBitCast(IDy, b->getType());
                        b->replaceAllUsesWith(IDyF);
                    }
                }
            }
        }
    }
}

bool ThreadCombining::runOnModule(llvm::Module& M)
{
    llvm::IRBuilder<>  builder(M.getContext());
    CodeGenContext* context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ComputeShaderContext* csCtx = static_cast<ComputeShaderContext*>(context);
    auto m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    m_kernel = m_pMdUtils->begin_FunctionsInfo()->first;

    if (!canDoOptimization(m_kernel, M))
    {
        return false;
    }

    unsigned int threadGroupSize_X = GetthreadGroupSize(M, ThreadGroupSize_X);
    unsigned int threadGroupSize_Y = GetthreadGroupSize(M, ThreadGroupSize_Y);

    // This is a heuristic from experiments (same as the value in ShaderCodeGen),
    // if number of temp registers is greater than this number then, likely
    // that SIMD16 will spill and we should chose SIMD 8
    static const int SIMD16_NUM_TEMPREG_THRESHOLD = 92;

    // This value tells us what is the minimum acceptable threadgroup size
    // to make sure that we are not too aggressive with thread combining.
    // Current Heurstic is to have no less than 8 H/W threads per WG.
    unsigned int minTGSizeHeuristic = 0;

    SIMDMode simdMode = csCtx->GetLeastSIMDModeAllowed();
    // If SIMD8 is legal then, heuristics are SIMD8 selection if spill is
    // expected, otherwise based on simd16 selection
    if (simdMode == SIMDMode::SIMD8)
    {
        if (csCtx->m_tempCount > SIMD16_NUM_TEMPREG_THRESHOLD)
        {
            simdMode = SIMDMode::SIMD8;
            minTGSizeHeuristic = 64; // => 8 h/w threads per WG
        }
        else
        {
            simdMode = SIMDMode::SIMD16;
            minTGSizeHeuristic = 128; // => 8 h/w threads per WG
        }
    }

    float currentThreadOccupancy = csCtx->GetThreadOccupancy(simdMode);
    unsigned x = (threadGroupSize_X % 2 == 0) ? threadGroupSize_X / 2 : threadGroupSize_X;
    unsigned y = (threadGroupSize_Y % 2 == 0) ? threadGroupSize_Y / 2 : threadGroupSize_Y;
    float newThreadOccupancy = GetThreadOccupancyPerSubslice(simdMode, x*y, GetHwThreadsPerWG(csCtx->platform), csCtx->m_slmSize, csCtx->GetSlmSizePerSubslice());

    unsigned int newSizeX = threadGroupSize_X;
    unsigned int newSizeY = threadGroupSize_Y;
    // Heuristic for Threadcombining based on EU Occupancy, if EU occupancy increases with the new
    // size then combine threads, otherwise skip it
    if (IGC_IS_FLAG_ENABLED(EnableForceGroupSize))
    {
        newSizeX = IGC_GET_FLAG_VALUE(ForceGroupSizeX);
        newSizeY = IGC_GET_FLAG_VALUE(ForceGroupSizeY);
    }
    else if (x*y >= minTGSizeHeuristic && newThreadOccupancy > currentThreadOccupancy)
    {
        newSizeX = x;
        newSizeY = y;
        currentThreadOccupancy = newThreadOccupancy;
        x = (x % 2 == 0) ? x / 2 : x;
        y = (y % 2 == 0) ? y / 2 : y;
        newThreadOccupancy = GetThreadOccupancyPerSubslice(simdMode, x*y, GetHwThreadsPerWG(csCtx->platform), csCtx->m_slmSize, csCtx->GetSlmSizePerSubslice());
        if (x*y >= minTGSizeHeuristic && newThreadOccupancy > currentThreadOccupancy)
        {
            newSizeX = x;
            newSizeY = y;
        }
    }
    else
    {
        return false;
    }

    assert(newSizeX <= threadGroupSize_X);
    assert(newSizeY <= threadGroupSize_Y);

    SetthreadGroupSize(M, builder.getInt32(newSizeX), ThreadGroupSize_X);
    SetthreadGroupSize(M, builder.getInt32(newSizeY), ThreadGroupSize_Y);

    // Create a new function with function arguments, New threadIDX, threadIDY,
    // a bool variable to indicate if it is kernel section before last barrier or after
    // last barrier and all the live variables

    std::vector<llvm::Type*> callArgTypes;
    callArgTypes.push_back(builder.getInt32Ty()); // ThreadX
    callArgTypes.push_back(builder.getInt32Ty()); // ThreadY
    callArgTypes.push_back(builder.getInt32Ty()); // BarrierNum

    // The function takes as argument all the registers that are alive across barriers
    for (auto& it : m_aliveAcrossBarrier)
    {
        llvm::Instruction* aliveInst = dyn_cast<Instruction>(it);
        PointerType *PtrTy = PointerType::get(aliveInst->getType(), 0);
        callArgTypes.push_back(PtrTy);
    }

    // Create a new function from the original kernel
    llvm::Function* newFunc =
        Function::Create(FunctionType::get(builder.getVoidTy(), callArgTypes, false),
            llvm::GlobalValue::InternalLinkage,
            "newKernel",
            &M);
    newFunc->addFnAttr(llvm::Attribute::AlwaysInline);
    // Fills in the instructions
    CreateNewKernel(M, builder, newFunc);

    // Instead of running the origkernel by one logical thread 1 time, hence threadgroupSizeX * threadGroupSizeY logical threads each run sepearetly
    // optimization is to run in one logical thread (threadGroupSize_X / newSizeX)  * (threadGroupSize_Y / newSizeY) times
    // and hence lesser no of logical threads which maps to lesser number of hardware threads

    // for(i = 0 to numBarriers)
    //  for(j = 0 to threadGroupSize_X / newSizeX)
    //   for(k = 0 to threadGroupSize_Y / newSizeY)
    //        Run portion of kernel after Barrier[i]
    //  sync instruction

    CreateLoopKernel(
        M,
        newSizeX,
        newSizeY,
        threadGroupSize_X,
        threadGroupSize_Y,
        newFunc,
        builder);

    context->m_threadCombiningOptDone = true;

    return true;
}
