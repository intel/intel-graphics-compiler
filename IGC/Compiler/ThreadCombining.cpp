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
    if (llvm::isa<llvm::LoadInst>(I) || llvm::isa<llvm::StoreInst>(I))
    {
        if (llvm::isa<llvm::LoadInst>(I))
        {
            LoadInst* inst = dyn_cast<LoadInst>(I);
            if (inst->getPointerOperand())
            {
                addrSpace = inst->getPointerAddressSpace();
            }
        }
        else
        {
            StoreInst* inst = dyn_cast<StoreInst>(I);
            if (inst->getPointerOperand())
            {
                addrSpace = inst->getPointerAddressSpace();
            }
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

void ThreadCombining::createLoopKernel(
    llvm::Module& M,
    unsigned int newSizeX,
    unsigned int newSizeY,
    unsigned int threadGroupSize_X,
    unsigned int threadGroupSize_Y,
    unsigned int divideX,
    unsigned int divideY,
    Function* newFunc,
    llvm::IRBuilder<> builder)
{
    BasicBlock* mainEntry = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* outerLoopEntry1 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* InnerLoop1 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* condCall1 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* AfterCall1 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* outerLoopExit1 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* exit1 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* outerLoopEntry2 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* InnerLoop2 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* condCall2 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* AfterCall2 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* outerLoopExit2 = BasicBlock::Create(M.getContext(), "", m_kernel);
    BasicBlock* exit2 = BasicBlock::Create(M.getContext(), "", m_kernel);

    //In the first block which is main entry block
    //Create alloca s for the 2 loop iterators in the main entry block
    //Create alloca for vector of size = iterIMax * IterJMax for each live register
    //Read the thread ids
    //Initialize the outer loop iterator to 0
    //Branch to outer loop
    builder.SetInsertPoint(mainEntry);
    Value* IterI = builder.CreateAlloca(builder.getInt32Ty());
    Value* IterJ = builder.CreateAlloca(builder.getInt32Ty());

    //Total size is number of iterations in the 2 for loops
    std::map<llvm::Instruction*, Value*> regToAllocaMap;
    unsigned int totalSize = divideX * divideY;
    for (auto it : m_aliveAcrossBarrier)
    {
        llvm::Instruction* aliveInst = dyn_cast<Instruction>(it);
        llvm::VectorType* pVecType = llvm::VectorType::get(aliveInst->getType(), totalSize);
        llvm::Value* inst = builder.CreateAlloca(pVecType);
        regToAllocaMap[aliveInst] = inst;
    }

    Function* ThreadIDFN = GenISAIntrinsic::getDeclaration(&M, GenISAIntrinsic::GenISA_DCL_SystemValue, builder.getFloatTy());
    Value* ThreadIDX = builder.CreateCall(ThreadIDFN, builder.getInt32(THREAD_ID_IN_GROUP_X));
    Value* ThreadIDY = builder.CreateCall(ThreadIDFN, builder.getInt32(THREAD_ID_IN_GROUP_Y));
    ThreadIDX = builder.CreateBitCast(ThreadIDX, builder.getInt32Ty());
    ThreadIDY = builder.CreateBitCast(ThreadIDY, builder.getInt32Ty());
    builder.CreateStore(builder.getInt32(0), IterI);
    builder.CreateBr(outerLoopEntry1);

    //label OuterloopEntry1:
    //In the outer loop, initialize inner loop iterator to 0
    //Branch to inner loop
    builder.SetInsertPoint(outerLoopEntry1);
    builder.CreateStore(builder.getInt32(0), IterJ);
    builder.CreateBr(InnerLoop1);

    //In the inner loop, calculate the modified thread ids
    //label InnerLoop:
    //modifiedThreadIDX = threadIDX + i * new group size X
    //modifiedThreadIDY = threadIDY + j * new group size Y
    //if (condCall1)
    //    (modifiedThreadIDX < originalGroupSizeX) &&  (modifiedThreadIDY < originalGroupSizeY)
    //then 
    //  execute newfunc to execute first section before barrier in orig kernel
    //else (AfterCall1)
    //  j = j+1
    //  if
    //     j < (originalthreadgroupsizeY / modifiedthreadgroupsizeY)
    //  then
    //     goto Innerloop1
    //  else
    //     i = i+1 (OuterLoopEntry1)
    //     if 
    //        i < (originalthreadgroupsizeX / modifiedthreadgroupsizeX)
    //     then 
    //        goto outerLoopEntry1
    //     else
    //        Execute barrier instruction
    //Then re-initialize outer loop iter i  to 0 and 
    //repeat the same for executing second part, section after the barrier

    builder.SetInsertPoint(InnerLoop1);
    Value* X = builder.CreateLoad(IterI);
    Value* Y = builder.CreateLoad(IterJ);
    Value* correctedIDx = builder.CreateAdd(ThreadIDX, builder.CreateMul(X, builder.getInt32(newSizeX)));
    Value* correctedIDy = builder.CreateAdd(ThreadIDY, builder.CreateMul(Y, builder.getInt32(newSizeY)));
    Value* condX = builder.CreateICmp(CmpInst::ICMP_ULT, correctedIDx, builder.getInt32(threadGroupSize_X));
    Value* condY = builder.CreateICmp(CmpInst::ICMP_ULT, correctedIDy, builder.getInt32(threadGroupSize_Y));
    Value* condTh = builder.CreateAnd(condX, condY);
    builder.CreateCondBr(condTh, condCall1, AfterCall1);
    //builder.CreateBr(condCall1);

    builder.SetInsertPoint(condCall1);
    std::vector<llvm::Value*> callArgs;
    callArgs.push_back(correctedIDx);
    callArgs.push_back(correctedIDy);
    callArgs.push_back(builder.getInt1(false));
    for (auto it : m_aliveAcrossBarrier)
    {
        //index = (iterI * (old/New wG threadGroupSize_Y)) + iterJ
        llvm::Instruction* aliveInst = dyn_cast<Instruction>(it);
        Value* rowMul = builder.CreateMul(X, builder.getInt32(divideY));
        Value* index = builder.CreateAdd(rowMul, Y);
        Value* indexes[] = { builder.getInt32(0), index };
        Value* gepPtr = builder.CreateGEP(regToAllocaMap[aliveInst], indexes);
        callArgs.push_back(gepPtr);
    }
    builder.CreateCall(newFunc, callArgs);
    builder.CreateBr(AfterCall1);

    builder.SetInsertPoint(AfterCall1);
    Y = builder.CreateAdd(Y, builder.getInt32(1));
    builder.CreateStore(Y, IterJ);
    Value* cond = builder.CreateICmp(CmpInst::ICMP_ULT, Y, builder.getInt32(divideY));
    builder.CreateCondBr(cond, InnerLoop1, outerLoopExit1);

    builder.SetInsertPoint(outerLoopExit1);
    X = builder.CreateAdd(X, builder.getInt32(1));
    builder.CreateStore(X, IterI);
    cond = builder.CreateICmp(CmpInst::ICMP_ULT, X, builder.getInt32(divideX));
    builder.CreateCondBr(cond, outerLoopEntry1, exit1);

    builder.SetInsertPoint(exit1);
    Function* barrierFn = GenISAIntrinsic::getDeclaration(&M, GenISAIntrinsic::GenISA_threadgroupbarrier);
    builder.CreateCall(barrierFn);
    builder.CreateStore(builder.getInt32(0), IterI);
    builder.CreateBr(outerLoopEntry2);

    builder.SetInsertPoint(outerLoopEntry2);
    builder.CreateStore(builder.getInt32(0), IterJ);
    builder.CreateBr(InnerLoop2);

    builder.SetInsertPoint(InnerLoop2);
    X = builder.CreateLoad(IterI);
    Y = builder.CreateLoad(IterJ);
    correctedIDx = builder.CreateAdd(ThreadIDX, builder.CreateMul(X, builder.getInt32(newSizeX)));
    correctedIDy = builder.CreateAdd(ThreadIDY, builder.CreateMul(Y, builder.getInt32(newSizeY)));
    condX = builder.CreateICmp(CmpInst::ICMP_ULT, correctedIDx, builder.getInt32(threadGroupSize_X));
    condY = builder.CreateICmp(CmpInst::ICMP_ULT, correctedIDy, builder.getInt32(threadGroupSize_Y));
    condTh = builder.CreateAnd(condX, condY);
    builder.CreateCondBr(condTh, condCall2, AfterCall2);
    //builder.CreateBr(condCall2);

    builder.SetInsertPoint(condCall2);
    std::vector<llvm::Value*> callArgs2;
    callArgs2.push_back(correctedIDx);
    callArgs2.push_back(correctedIDy);
    callArgs2.push_back(builder.getInt1(true));
    for (auto it : m_aliveAcrossBarrier)
    {
        //index = (iterI * (old/New WG threadGroupSize_Y)) + iterJ
        llvm::Instruction* aliveInst = dyn_cast<Instruction>(it);
        Value* rowMul = builder.CreateMul(X, builder.getInt32(divideY));
        Value* index = builder.CreateAdd(rowMul, Y);
        Value* indexes[] = { builder.getInt32(0), index };
        Value* gepPtr = builder.CreateGEP(regToAllocaMap[aliveInst], indexes);
        callArgs2.push_back(gepPtr);
    }
    builder.CreateCall(newFunc, callArgs2);
    builder.CreateBr(AfterCall2);

    builder.SetInsertPoint(AfterCall2);
    Y = builder.CreateAdd(Y, builder.getInt32(1));
    builder.CreateStore(Y, IterJ);
    cond = builder.CreateICmp(CmpInst::ICMP_ULT, Y, builder.getInt32(divideY));
    builder.CreateCondBr(cond, InnerLoop2, outerLoopExit2);

    builder.SetInsertPoint(outerLoopExit2);
    X = builder.CreateAdd(X, builder.getInt32(1));
    builder.CreateStore(X, IterI);
    cond = builder.CreateICmp(CmpInst::ICMP_ULT, X, builder.getInt32(divideX));
    builder.CreateCondBr(cond, outerLoopEntry2, exit2);

    builder.SetInsertPoint(exit2);
    builder.CreateRetVoid();
}

bool ThreadCombining::canDoOptimization(Function* m_kernel, llvm::Module& M)
{

    bool slmUsed = false;
    bool barrierPresent = false;
    bool multipleBarier = false;
    DominatorTreeWrapperPass* DT = &getAnalysis<DominatorTreeWrapperPass>(*m_kernel);
    PostDominatorTree* PDT = &getAnalysis<PostDominatorTreeWrapperPass>(*m_kernel).getPostDomTree();
    //Find if there is one or more barriers
    for (auto BI = m_kernel->begin(); BI != m_kernel->end(); ++BI)
    {
        for (auto II = BI->begin(); II != BI->end(); ++II)
        {
            Instruction* inst = &(*II);
            if (isSLMUsed(inst))
            {
                slmUsed = true;
            }
            if (isBarrier(*inst))
            {
                if (barrierPresent)
                {
                    multipleBarier = true;
                    break;
                }
                barrierPresent = true;
                m_barrier = inst;
                continue;
            }

            //find all active live registers across barrier
            //def should be an instruction and shouldnt be dominated by barrier
            //use should be dominated by barrier
            if (m_barrier)
            {
                for (unsigned int i = 0; i < inst->getNumOperands(); ++i)
                {
                    if (Instruction* I = dyn_cast<Instruction>(inst->getOperand(i)))
                    {
                        if (!DT->getDomTree().dominates(m_barrier, I))
                        {
                            bool cannotMove = true;
                            if (!I->mayReadOrWriteMemory())
                            {
                                cannotMove = false;
                                for (unsigned int j = 0; j < I->getNumOperands(); j++)
                                {
                                    if (isa<Instruction>(I->getOperand(j)))
                                    {
                                        cannotMove = true;
                                        break;
                                    }
                                }
                                //optimization to reduce the number of live registers crossing barrier
                                //Move them later in the entry block of new function
                                if (cannotMove == false)
                                {
                                    m_instructionsToMove.insert(I);
                                }
                            }
                            if (cannotMove)
                            {
                                m_aliveAcrossBarrier.insert(I);
                            }
                        }
                    }
                }
            }
        }
    }

    unsigned int threadGroupSize_X = GetthreadGroupSize(M, ThreadGroupSize_X);
    unsigned int threadGroupSize_Y = GetthreadGroupSize(M, ThreadGroupSize_Y);
    unsigned int threadGroupSize_Z = GetthreadGroupSize(M, ThreadGroupSize_Z);

    unsigned int noSoftwareThreads = threadGroupSize_X * threadGroupSize_Y * threadGroupSize_Z;

    //No optimization if there is no barrier - there might not be an impact
    //No optimization if no SLM used - number of dispatchable thread groups is limited by SLM space, only then we have perf issue
    //No optimization if thread group size Z is not equal to 1 - keep for simpler cases
    //No optimization if there are multiple barriers present - keep it simple for now - TODO
    //No optimization if no of software threads are lesser - not needed in such smaller work groups (TODO) - hard coded to 128 now
    //No optimization if barrier is within control flow - to keep it simple for now else gets complex
    //No optimization if new size is greater than orig thread group size along any dimension
    if (threadGroupSize_Z != 1 ||
        !slmUsed || 
        !barrierPresent ||
        multipleBarier ||
        noSoftwareThreads < 128 ||
        !PDT->dominates(m_barrier->getParent(), &m_kernel->getEntryBlock()))
    {
        return false;
    }
    return true;
}

bool ThreadCombining::runOnModule(llvm::Module& M)
{
    llvm::IRBuilder<>  builder(M.getContext());
    CodeGenContext* context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    auto m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    m_kernel = m_pMdUtils->begin_FunctionsInfo()->first;
    DominatorTreeWrapperPass* DT = &getAnalysis<DominatorTreeWrapperPass>(*m_kernel);

    //analyse if optimization can be done
    if (!canDoOptimization(m_kernel, M))
    {
        return false;
    }

    //If all conditions meet then do the optimization

    unsigned int threadGroupSize_X = GetthreadGroupSize(M, ThreadGroupSize_X);
    unsigned int threadGroupSize_Y = GetthreadGroupSize(M, ThreadGroupSize_Y);

    ComputeShaderContext* csCtx = static_cast<ComputeShaderContext*>(context);
    unsigned int newSizeX = threadGroupSize_X;
    unsigned int newSizeY = threadGroupSize_Y;

    //If SIMD32 required then break it down
    if( (newSizeX * newSizeY) > (csCtx->GetHwThreadPerWorkgroup() * 16) )
    {
        newSizeX = threadGroupSize_X / 2;
        newSizeY = threadGroupSize_Y / 2;
    }

    //Override with reg key values if set
    if(IGC_IS_FLAG_ENABLED(EnableForceGroupSize))
    {
        newSizeX = IGC_GET_FLAG_VALUE(ForceGroupSizeX);
        newSizeY = IGC_GET_FLAG_VALUE(ForceGroupSizeY);
    }

    if ((newSizeX >= threadGroupSize_X && newSizeY >= threadGroupSize_Y) ||
        newSizeX == 0 ||
        newSizeY == 0 || 
        threadGroupSize_X % newSizeX != 0 ||
        threadGroupSize_Y % newSizeY != 0)
    {
        //optimization doesn't get applied if thread group sizes are same or bigger
        return false;
    }

    unsigned int divideX = threadGroupSize_X / newSizeX;
    //if (threadGroupSize_X % newSizeX != 0)
    //{
    //    divideX++;
    //}
    unsigned int divideY = threadGroupSize_Y / newSizeY;
    //if (threadGroupSize_Y % newSizeY != 0)
    //{
    //    divideY++;
    //}

    SetthreadGroupSize(M, builder.getInt32(newSizeX), ThreadGroupSize_X);
    SetthreadGroupSize(M, builder.getInt32(newSizeY), ThreadGroupSize_Y);

    //Create a new function with function arguments, New threadIDX, threadIDY, 
    //a bool variable to indicate if it is kernel section before barrier or after 
    //and all the live variables
    std::vector<llvm::Type*> callArgTypes;
    callArgTypes.push_back(builder.getInt32Ty());
    callArgTypes.push_back(builder.getInt32Ty());
    callArgTypes.push_back(builder.getInt1Ty());

    for (auto it : m_aliveAcrossBarrier)
    {
        llvm::Instruction* aliveInst = dyn_cast<Instruction>(it);
        PointerType *PtrTy = PointerType::get(aliveInst->getType(), 0);
        callArgTypes.push_back(PtrTy);
    }

    Function* newFunc =
        Function::Create(FunctionType::get(builder.getVoidTy(), callArgTypes, false),
        llvm::GlobalValue::InternalLinkage,
        "newKernel",
        &M);
    newFunc->addFnAttr(llvm::Attribute::AlwaysInline);
    newFunc->getBasicBlockList().splice(newFunc->begin(), m_kernel->getBasicBlockList());
    
    //Create Loads for all live registers just after the barrier
    //Replace all the uses in this section with that of loads
    builder.SetInsertPoint(m_barrier->getNextNode());
    auto argIter = newFunc->arg_begin();
    argIter++; argIter++; argIter++;
    for (auto it : m_aliveAcrossBarrier)
    {
        Value* loadLiveReg = builder.CreateLoad(&(*argIter++));

        SmallVector<Instruction*, 10> usesToReplace;
        for (auto use_it = it->use_begin();
            use_it != it->use_end();
            use_it++)
        {
            Instruction *useInst = dyn_cast<Instruction>(use_it->getUser());

            //if barrier dominates use then replace all the live registers with this new load register
            if (DT->getDomTree().dominates(m_barrier, useInst))
            {
                usesToReplace.push_back(useInst);
            }
        }
        for (auto it2 : usesToReplace)
        {
            it2->replaceUsesOfWith(it, loadLiveReg);
        }
    }

    //Create stores for all the live registers just before the barrier instruction
    builder.SetInsertPoint(m_barrier);
    argIter = newFunc->arg_begin();
    argIter++; argIter++; argIter++;
    for (auto it : m_aliveAcrossBarrier)
    {
        llvm::Instruction* aliveInst = dyn_cast<Instruction>(it);
        builder.CreateStore(aliveInst, &(*argIter++));
    }

    // split the barrier block into two
    BasicBlock* afterBarrier = BasicBlock::Create(M.getContext(), "", newFunc);
    BasicBlock* barrierBlock = m_barrier->getParent();
    barrierBlock->replaceSuccessorsPhiUsesWith(afterBarrier);
    builder.SetInsertPoint(m_barrier);
    builder.CreateRetVoid();

    afterBarrier->getInstList().splice(
        afterBarrier->begin(),
        barrierBlock->getInstList(),
        m_barrier->getIterator(),
        barrierBlock->getInstList().end());
    m_barrier->eraseFromParent();

    auto it = newFunc->arg_begin();
    Value* IDx = &(*it++);
    Value* IDy = &(*it++);
    Value* afterBarrierSection = &(*it++);

    // split the entry block into two
    BasicBlock* oldEntry = &newFunc->getEntryBlock();
    BasicBlock* newEntry = BasicBlock::Create(M.getContext(), "", newFunc, oldEntry);

    builder.SetInsertPoint(newEntry);
    builder.CreateCondBr(afterBarrierSection, afterBarrier, oldEntry);

    //Move all instructions in m_instructionsToMove to newEntry block
    for (auto iter : m_instructionsToMove)
    {
        Instruction* instToMove = dyn_cast<Instruction>(iter);
        instToMove->moveBefore(&*((newEntry->getInstList().begin())));
    }
    //Move all allocas from oldentry to newEntry block
    {
        std::set<llvm::Instruction*> allocaInstrToMov;
        for (auto BI = oldEntry->begin(); BI != oldEntry->end();)
        {
            Instruction* inst = &(*BI++);
            if(llvm::isa<llvm::AllocaInst>(inst))
            {
                inst->moveBefore(&*((newEntry->getInstList().begin())));
				allocaInstrToMov.insert(inst);
            }
        }
    }
  
    //Replace all the threadIDs in the new kernel with the function arguments
    for (auto BI = newFunc->begin(); BI != newFunc->end(); ++BI)
    {
        for (auto II = BI->begin(); II != BI->end(); ++II)
        {
            Instruction* inst = &(*II);
            if (GenIntrinsicInst* b = dyn_cast<GenIntrinsicInst>(inst))
            {
                if (b->getIntrinsicID() == GenISAIntrinsic::GenISA_DCL_SystemValue)
                {
                    if (cast<ConstantInt>(b->getOperand(0))->getZExtValue() == THREAD_ID_IN_GROUP_X)
                    {
                        builder.SetInsertPoint(&(*newFunc->getEntryBlock().begin()));
                        Value* IDxF = builder.CreateBitCast(IDx, builder.getFloatTy());
                        b->replaceAllUsesWith(IDxF);
                    }
                    if (cast<ConstantInt>(b->getOperand(0))->getZExtValue() == THREAD_ID_IN_GROUP_Y)
                    {
                        builder.SetInsertPoint(&(*newFunc->getEntryBlock().begin()));
                        Value* IDyF = builder.CreateBitCast(IDy, builder.getFloatTy());
                        b->replaceAllUsesWith(IDyF);
                    }
                }
            }
        }
    }

    // Instead of running the origkernel by one logical thread 1 time, hence threadgroupSizeX * threadGroupSizeY logical threads each run sepearetly
    // optimization is to run in one logical thread (threadGroupSize_X / newSizeX)  * (threadGroupSize_Y / newSizeY) times
    // and hence lesser no of logical threads which maps to lesser number of hardware threads

    // for(i = 0 to threadGroupSize_X / newSizeX)
    //    for(j = 0 to threadGroupSize_Y / newSizeY)
    //        Run kernel first part
    // sync instruction
    // for(i = 0 to threadGroupSize_X / newSizeX)
    //    for(j = 0 to threadGroupSize_Y / newSizeY)
    //        Run kernel second part

    createLoopKernel(
        M,
        newSizeX,
        newSizeY,
        threadGroupSize_X,
        threadGroupSize_Y,
        divideX,
        divideY,
        newFunc,
        builder);

    context->m_threadCombiningOptDone = true;

    DumpLLVMIR(context, "comb");
    return true;
}
