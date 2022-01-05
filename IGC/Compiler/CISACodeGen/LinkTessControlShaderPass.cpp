/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/LinkTessControlShaderPass.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvmWrapper/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenPublic.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// This pass optimizes tessellation control shaders.
// The pass calculates output control point ID for each logical thread and
// replaces all calls to HSControlPointID in the original shader with the
// calculated value.
// Output control point ID is calculated based on the dispatch type, instance ID
// and simd lane ID. In case of scenario 1) described below all control points
// of a patch are processed in a single logical thread in a loop - control point
// ID is equal to the loop induction variable.
//
// The optimization handles the following scenarios:
// 1) TCS shader without thread barrier instructions (most common)
//    In this case the pass adds a loop to loop through the number of output
//    control points and replaces all occurrences of HSControlPointID with
//    the loop induction variable.
//    In this case, for any patch all output control points are handled in
//    a single thread.
// 2) TCS shader with thread barrier instructions and number of output control
//    points equal to 1 with eight patch dispatch or less than 8 with single
//    patch distpatch. All ouptut control points can be processed in a single
//    physical thread.
//    In this case the pass removes all barrier instructions and falls back to
//    the scenario 1) (with only a single loop iteration).
// 3) TCS shader with thread barrier instructions and number of output control
//    points that require multiple physical thread per patch.
//    In such case the pass only replaces HSControlPointID with the calculated
//    control point ID.
namespace IGC
{
    class LinkTessControlShader : public llvm::ModulePass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        LinkTessControlShader();
        ~LinkTessControlShader() {};

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override {
            return "LinkTessControlShader";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        /// @brief  Main entry point.
        /// @param  F The current function.
        virtual bool runOnModule(llvm::Module& M) override;
    private:
        HullShaderDispatchModes DetermineDispatchMode(
            llvm::Module* mod,
            bool hasBarrier);
        llvm::Function* CreateNewTCSFunction(llvm::Function* pCurrentFunc);
        bool CheckIfBarrierInstructionExists(llvm::Function* pFunc);
        void RemoveBarrierInstructions(llvm::Function* pFunc);
        uint GetNumberOfOutputControlPoints(llvm::Module* module);
    };

    char LinkTessControlShader::ID = 0;
    // Register pass to igc-opt
#define PASS_FLAG "igc-linkTessControlShader"
#define PASS_DESCRIPTION "Perform looping of tessellation function based on control point count"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(LinkTessControlShader, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_END(LinkTessControlShader, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

    LinkTessControlShader::LinkTessControlShader() : llvm::ModulePass(ID)
    {
        initializeLinkTessControlShaderPass(*llvm::PassRegistry::getPassRegistry());
    }

    uint LinkTessControlShader::GetNumberOfOutputControlPoints(llvm::Module* module)
    {
        IGC_ASSERT(module->getGlobalVariable("HSOutputControlPointCount"));
        llvm::GlobalVariable* pOutputControlPointCountVar = module->getGlobalVariable("HSOutputControlPointCount");

        IGC_ASSERT(pOutputControlPointCountVar && pOutputControlPointCountVar->hasInitializer());
        IGC_ASSERT(llvm::isa<llvm::ConstantInt>(pOutputControlPointCountVar->getInitializer()));

        llvm::ConstantInt* pOutputControlPointCountVal =
            llvm::cast<llvm::ConstantInt>(pOutputControlPointCountVar->getInitializer());

        uint outputControlPointCount = int_cast<uint>(pOutputControlPointCountVal->getZExtValue());
        IGC_ASSERT(outputControlPointCount > 0);

        return outputControlPointCount;
    }

    HullShaderDispatchModes LinkTessControlShader::DetermineDispatchMode(
        llvm::Module* mod,
        bool hasBarrier)
    {
        const uint outputControlPointCount = GetNumberOfOutputControlPoints(mod);

        IGC::CodeGenContext* pCodeGenContext = nullptr;
        pCodeGenContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

        /* Instance Count
        **         This field determines the number of threads(minus one) spawned per input patch.

        **         If the HS kernel uses a barrier function, software must restrict the Instance Count
        **         to the number of threads that can be simultaneously active within a subslice.
        **         Factors which must be considered includes scratch memory availability.
        **         Value             Description
        **         [0, 15]             representing[1, 16] instances */

        // Use HS single patch if WA exists and input control points >= 29 as there are not enough registers for push constants
        bool useSinglePatch = false;
        if (pCodeGenContext->platform.WaDispatchGRFHWIssueInGSAndHSUnit())
        {
            IGC_ASSERT(!pCodeGenContext->platform.useOnlyEightPatchDispatchHS());
            llvm::GlobalVariable* pGlobal = mod->getGlobalVariable("TessInputControlPointCount");
            if (pGlobal && pGlobal->hasInitializer())
            {
                unsigned int inputControlPointCount = int_cast<unsigned int>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
                if (inputControlPointCount >= 29)
                {
                    useSinglePatch = true;
                }
            }
        }

        HullShaderDispatchModes dispatchMode = HullShaderDispatchModes::SINGLE_PATCH_DISPATCH_MODE;

        if (pCodeGenContext->platform.useOnlyEightPatchDispatchHS() ||
            (pCodeGenContext->platform.supportHSEightPatchDispatch() &&
             !(hasBarrier && outputControlPointCount >= 16) &&
             !useSinglePatch &&
             IGC_IS_FLAG_DISABLED(EnableHSSinglePatchDispatch)))
        {
            dispatchMode = HullShaderDispatchModes::EIGHT_PATCH_DISPATCH_MODE;
        }

        // Set dispatch mode metadata.
        llvm::NamedMDNode* metaData = mod->getOrInsertNamedMetadata("HullShaderDispatchMode");
        Constant* cval = ConstantInt::get(
            Type::getInt32Ty(mod->getContext()),
            dispatchMode);
        MDNode* mdNode = MDNode::get(
            mod->getContext(),
            ConstantAsMetadata::get(cval));
        metaData->addOperand(mdNode);

        return dispatchMode;
    }
    /*
        Function pass to create a new entry function
    */
    llvm::Function* LinkTessControlShader::CreateNewTCSFunction(llvm::Function* pCurrentFunc)
    {
        llvm::IRBuilder<> irBuilder(pCurrentFunc->getContext());
        CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

        std::vector<llvm::Type*> callArgTypes;
        for (auto& argIter : range(pCurrentFunc->arg_begin(), pCurrentFunc->arg_end()))
        {
            callArgTypes.push_back(argIter.getType());
        }
        callArgTypes.push_back(irBuilder.getInt32Ty());

        std::string funcName = "tessControlShaderEntry";

        llvm::Function* pNewFunction = llvm::Function::Create(
            llvm::FunctionType::get(
                irBuilder.getVoidTy(), callArgTypes, false),
            llvm::GlobalValue::PrivateLinkage,
            funcName,
            ctx->getModule());

        pNewFunction->addFnAttr(llvm::Attribute::AlwaysInline);

        // Move over the contents of the original function
        pNewFunction->getBasicBlockList().splice(pNewFunction->begin(), pCurrentFunc->getBasicBlockList());

        llvm::Function* pToBeDeletedFunc = pCurrentFunc;

        for (auto oldArg = pToBeDeletedFunc->arg_begin(),
            oldArgEnd = pToBeDeletedFunc->arg_end(),
            newArg = pNewFunction->arg_begin();
            oldArg != oldArgEnd;
            ++oldArg, ++newArg)
        {
            oldArg->replaceAllUsesWith(&(*newArg));
            newArg->takeName(&(*oldArg));
        }

        // delete the old function signature
        pToBeDeletedFunc->eraseFromParent();

        // now replace all occurences of HSControlPointID with the current
        // loop iteration CPID - pCurrentCPID
        SmallVector<Instruction*, 10> instructionToRemove;

        llvm::Value* pHSControlPointID = llvm::GenISAIntrinsic::getDeclaration(pNewFunction->getParent(),
            GenISAIntrinsic::GenISA_DCL_HSControlPointID);

        unsigned int argIndexInFunc = pNewFunction->arg_size() - 1;
        Function::arg_iterator arg = pNewFunction->arg_begin();
        for (unsigned int i = 0; i < argIndexInFunc; ++i, ++arg);

        for (Value::user_iterator i = pHSControlPointID->user_begin(), e = pHSControlPointID->user_end(); i != e; ++i)
        {
            Instruction* useInst = cast<Instruction>(*i);
            if (useInst->getParent()->getParent() == pNewFunction)
            {
                instructionToRemove.push_back(useInst);
                useInst->replaceAllUsesWith(&(*arg));
            }
        }

        for (auto& inst : instructionToRemove)
        {
            inst->eraseFromParent();
        }
        return pNewFunction;
    }

    bool LinkTessControlShader::CheckIfBarrierInstructionExists(llvm::Function* pFunc)
    {
        for (llvm::Function::iterator bb = pFunc->begin(), be = pFunc->end();
            bb != be;
            ++bb)
        {
            for (llvm::BasicBlock::iterator ii = bb->begin(), ie = bb->end();
                ii != ie;
                ++ii)
            {
                if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(ii))
                {
                    if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_threadgroupbarrier)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void LinkTessControlShader::RemoveBarrierInstructions(llvm::Function* pFunc)
    {
        std::vector<GenIntrinsicInst*> barriers;
        for (llvm::Function::iterator bb = pFunc->begin(), be = pFunc->end();
            bb != be;
            ++bb)
        {
            for (llvm::BasicBlock::iterator ii = bb->begin(), ie = bb->end();
                ii != ie;
                ++ii)
            {
                if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(ii))
                {
                    if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_threadgroupbarrier)
                    {
                        barriers.push_back(inst);
                    }
                }
            }
        }

        for (GenIntrinsicInst* inst : barriers)
        {
            inst->eraseFromParent();
        }
    }

    bool LinkTessControlShader::runOnModule(llvm::Module& M)
    {
        CodeGenContext* ctx = nullptr;
        ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        MetaDataUtils* pMdUtils = nullptr;
        pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        if (pMdUtils->size_FunctionsInfo() != 1)
        {
            return false;
        }

        Function* pFunc = pMdUtils->begin_FunctionsInfo()->first;
        IGCLLVM::IRBuilder<> builder(pFunc->getContext());
        std::string oldEntryFuncName = pFunc->getName().str();

        llvm::Function* pNewTCSFunction = CreateNewTCSFunction(pFunc);
        bool hasBarrier = CheckIfBarrierInstructionExists(pNewTCSFunction);

        // Determine the dispatch mode
        const HullShaderDispatchModes dispatchMode = DetermineDispatchMode(
            pNewTCSFunction->getParent(),
            hasBarrier);
        IGC_ASSERT(dispatchMode == EIGHT_PATCH_DISPATCH_MODE ||
            dispatchMode == SINGLE_PATCH_DISPATCH_MODE);

        const uint32_t outputControlPointCount = GetNumberOfOutputControlPoints(&M);

        const uint numSimdLanes = numLanes(ctx->platform.getMinDispatchMode());
        // Calculate how many physical threads would have to be spawned to
        // generate data for all output control points.
        const uint controlPointsPerThread =
            (dispatchMode == SINGLE_PATCH_DISPATCH_MODE) ? numSimdLanes : 1;
        const uint threadCount =
            (outputControlPointCount + controlPointsPerThread - 1) / controlPointsPerThread;

        if (threadCount == 1 && hasBarrier)
        {
            // With a single thread, there is no need for thread barriers.
            // This optimization is always required on DG2+. Unified barrier
            // programming requires that InstanceCount be at least 2 if TCS has
            // barrier instructions.

            RemoveBarrierInstructions(pNewTCSFunction);
            hasBarrier = false;
        }

        const bool needsMultiplePhysicalThreadsPerPatch = hasBarrier;

        // This function is the new entry function
        llvm::Function* pNewLoopFunc = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), false),
            llvm::GlobalValue::ExternalLinkage,
            oldEntryFuncName,
            ctx->getModule());

        llvm::BasicBlock* pEntryBlock = llvm::BasicBlock::Create(
            pNewLoopFunc->getContext(),
            oldEntryFuncName,
            pNewLoopFunc);

        builder.SetInsertPoint(pEntryBlock);

        // Lambda to create code for getting patch instance id.
        auto GetPatchInstanceId = [&builder, pNewLoopFunc]()->Value*
        {
            Function* pPatchInstanceIdIntr = GenISAIntrinsic::getDeclaration(
                pNewLoopFunc->getParent(),
                GenISAIntrinsic::GenISA_patchInstanceId);
            return builder.CreateCall(pPatchInstanceIdIntr);
        };

        // Calculate the output control point id value depending on
        // the dispatch mode used.
        llvm::Value* pCPId = builder.getInt32(0);
        if (dispatchMode == SINGLE_PATCH_DISPATCH_MODE)
        {
            Function* pSimdLaneIdIntr = GenISAIntrinsic::getDeclaration(
                pNewLoopFunc->getParent(),
                GenISAIntrinsic::GenISA_simdLaneId);
            pCPId = builder.CreateZExt(
                builder.CreateCall(pSimdLaneIdIntr),
                builder.getInt32Ty());

            if (needsMultiplePhysicalThreadsPerPatch)
            {
                // CPID = patchInstanceID * 8 + SimdLaneId;
                pCPId = builder.CreateAdd(
                    pCPId,
                    builder.CreateMul(
                        GetPatchInstanceId(),
                        builder.getInt32(numSimdLanes)));
            }
        }
        else if (needsMultiplePhysicalThreadsPerPatch)
        {
            IGC_ASSERT(dispatchMode == EIGHT_PATCH_DISPATCH_MODE);
            pCPId = GetPatchInstanceId();
        }

        // We don't need to deal with any loops when we are using multiple hardware threads
        if (!needsMultiplePhysicalThreadsPerPatch)
        {
            // initialize instanceCount to output control point count
            llvm::Value* pInstanceCount = builder.getInt32(outputControlPointCount);

            // initialize loopCounter
            llvm::Value* pLoopCounter = builder.CreateAlloca(builder.getInt32Ty(), 0, "loopCounter");
            llvm::Value* pConstInt = builder.getInt32(0);
            builder.CreateStore(pConstInt, pLoopCounter, false);

            // create loop-entry basic block and setInsertPoint to loop-entry
            //llvm::Function* pParentFunc = builder.GetInsertBlock()->getParent();
            llvm::BasicBlock* pLoopEntryBB = llvm::BasicBlock::Create(pNewLoopFunc->getContext(),
                VALUE_NAME("tcs_loop_entry"),
                pNewLoopFunc);

            llvm::BasicBlock* pLoopConditionTrue = llvm::BasicBlock::Create(pNewLoopFunc->getContext(),
                VALUE_NAME("tcs_loop_condition_true"),
                pNewLoopFunc);

            // Create loop-continue basic block
            llvm::BasicBlock* pLoopContinueBB = llvm::BasicBlock::Create(pNewLoopFunc->getContext(),
                VALUE_NAME("tcs_loop_continue"),
                pNewLoopFunc);

            // create loop exit basic block
            llvm::BasicBlock* pAfterLoopBB = llvm::BasicBlock::Create(pNewLoopFunc->getContext(),
                VALUE_NAME("tcs_after_loop"),
                pNewLoopFunc);

            // setInsertPoint to loopEntryBB
            builder.CreateBr(pLoopEntryBB);
            builder.SetInsertPoint(pLoopEntryBB);

            // Load the loop counter
            llvm::LoadInst* pLoadLoopCounter = builder.CreateLoad(pLoopCounter);
            llvm::Value* pMulLoopCounterRes = nullptr;
            llvm::Value* pCurrentCPID = nullptr;
            llvm::Value* pConditionalRes1 = nullptr;
            uint32_t loopIterationCount = 0;

            switch (dispatchMode)
            {
            case SINGLE_PATCH_DISPATCH_MODE:
                // currentCPID = pCPId + loopCounter x simdsize ( in this case its always simd 8 )
                pMulLoopCounterRes = builder.CreateMul(
                    pLoadLoopCounter,
                    builder.getInt32(numSimdLanes));
                pCurrentCPID = builder.CreateAdd(pCPId, pMulLoopCounterRes);

                // cmp currentCPID to instanceCount so we enable only the required lanes
                pConditionalRes1 = builder.CreateICmpULT(
                    pCurrentCPID,
                    pInstanceCount,
                    VALUE_NAME("tcs_if_ult_cond1"));

                // if true go to startBB else jump to pAfterLoopBB
                builder.CreateCondBr(pConditionalRes1,
                    pLoopConditionTrue,
                    pAfterLoopBB);

                loopIterationCount = ((outputControlPointCount - 1) / 8) + 1;
                break;

            case EIGHT_PATCH_DISPATCH_MODE:
                pCurrentCPID = pLoadLoopCounter;
                loopIterationCount = outputControlPointCount;

                // jump to startBB
                builder.CreateBr(pLoopConditionTrue);
                break;

            default:
                IGC_ASSERT_MESSAGE(0, "should not reach here");
                break;
            }

            // branch to pLoopContinueBB from endBB
            builder.SetInsertPoint(pLoopConditionTrue);

            // Create a call to the TCS function when condition is true to loop the function as many times as the number of control points
            builder.CreateCall(pNewTCSFunction, pCurrentCPID);
            builder.CreateBr(pLoopContinueBB);

            // setInsertPoint to pLoopContinueBB
            builder.SetInsertPoint(pLoopContinueBB);
            // increment loop counter loopCounter = loopCounter + 1
            llvm::Value* pIncrementedLoopCounter = builder.CreateAdd(
                pLoadLoopCounter,
                llvm::ConstantInt::get(builder.getInt32Ty(), 1));
            builder.CreateStore(pIncrementedLoopCounter, pLoopCounter, false);

            // now evaluate loop, if( ( incrementedLoopCounter ) < ( ( maxControlPointCount - 1 )/8) + 1 )
            // then continue loop else go to after loop
            llvm::Value* pNumberOfLoopIterationsRequired = llvm::ConstantInt::get(builder.getInt32Ty(), loopIterationCount);

            llvm::Value* pConditionalRes2 = builder.CreateICmpULT(
                pIncrementedLoopCounter,
                pNumberOfLoopIterationsRequired,
                VALUE_NAME("tcs_if_ult_cond2"));

            // create branch to LoopEntryBB or AfterLoopBB based on result of conditional branch
            builder.CreateCondBr(pConditionalRes2,
                pLoopEntryBB,
                pAfterLoopBB);

            // set insert point to afterloop basic block
            builder.SetInsertPoint(pAfterLoopBB);
        }
        else if (dispatchMode == SINGLE_PATCH_DISPATCH_MODE)
        {
            // In single patch dispatch mode the execution mask is 0xFF. Make
            // that only valid CPIDs execute.

            // Create the main basic block for the shader
            llvm::BasicBlock* pTcsBody = llvm::BasicBlock::Create(pNewLoopFunc->getContext(),
                VALUE_NAME("tcs_body"),
                pNewLoopFunc);
            // and the end block.
            llvm::BasicBlock* pAfterTcsBody = llvm::BasicBlock::Create(pNewLoopFunc->getContext(),
                VALUE_NAME("tcs_end"),
                pNewLoopFunc);

            // Compare current CPID to the number of CPIDs to enable only the required lanes.
            llvm::Value* pIsLaneEnabled = builder.CreateICmpULT(
                pCPId,
                builder.getInt32(outputControlPointCount),
                VALUE_NAME("tcs_if_ult_cond1"));

            builder.CreateCondBr(pIsLaneEnabled,
                pTcsBody,
                pAfterTcsBody);

            builder.SetInsertPoint(pTcsBody);

            // Call TCS function.
            builder.CreateCall(pNewTCSFunction, pCPId);

            builder.CreateBr(pAfterTcsBody);
            builder.SetInsertPoint(pAfterTcsBody);
        }
        else
        {
            // when using multiple hardware threads just call the Control Point function once with the appropriate CPID
            builder.CreateCall(pNewTCSFunction, pCPId);
        }

        // add terminator to the afterloop basic block
        builder.CreateRetVoid();

        pMdUtils->clearFunctionsInfo();
        IGCMetaDataHelper::addFunction(*pMdUtils, pNewLoopFunc);
        return true;
    }

    llvm::Pass* createLinkTessControlShader()
    {
        return new LinkTessControlShader();
    }
}
