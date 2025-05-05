/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LowerGPCallArg.h"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "LLVM3DBuilder/MetadataBuilder.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/PostOrderIterator.h"
#include <llvm/IR/DIBuilder.h>
#include "llvmWrapper/IR/Function.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <optional>

// (1)
// Optimization pass to lower generic pointers in function arguments.
// If all call sites have the same origin address space, address space
// casts with the form of non-generic->generic can safely removed and
// function updated with non-generic pointer argument.
//
// The complete process to lower generic pointer args consists of 5 steps:
//   1) find all functions that are candidates
//   2) update functions and their signatures
//   3) update all call sites
//   4) update functions metadata
//   5) validate that all function calls are properly formed
//
//
// Current limitations/considerations:
// - only arguments of non-extern functions can be lowered
// - no recursive functions support
//
// (2)
//   Once (1) is done. Do further check if there is a cast from local to GAS or
//   a cast from private to GAS. If there is no such cast, GAS inst (such as
//   ld/st, etc, can be converted safely to ld/st on globals.

ModulePass* IGC::createLowerGPCallArg() { return new LowerGPCallArg(); }

char LowerGPCallArg::ID = 0;

#define PASS_FLAG "igc-lower-gp-arg"
#define PASS_DESC "Lower generic pointers in call arguments"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LowerGPCallArg, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(LowerGPCallArg, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

bool LowerGPCallArg::runOnModule(llvm::Module& M)
{
    m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_mdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_module = &M;

    CallGraph& CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
    std::vector<Function*> candidates = findCandidates(CG);
    bool changed = false;
    for (auto F : reverse(candidates))
    {
        GenericPointerArgs genericArgsInfo;
        for (auto& arg : F->args())
        {
            if (arg.use_empty())
                continue;

            Type* argTy = arg.getType();
            if (argTy->isPointerTy() && argTy->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC)
            {
                if (auto originAddrSpace = getOriginAddressSpace(F, arg.getArgNo()))
                    genericArgsInfo.push_back({ arg.getArgNo(), originAddrSpace.value() });
            }
        }

        if (genericArgsInfo.empty())
            continue;

        Function* newFunc = createFuncWithLoweredArgs(F, genericArgsInfo);
        updateFunctionArgs(F, newFunc);
        updateAllUsesWithNewFunction(F, newFunc);
        updateMetadata(F, newFunc);

        F->eraseFromParent();
        changed = true;
    }

    return changed;
}

std::vector<Function*> LowerGPCallArg::findCandidates(CallGraph& CG)
{
    auto skip = [](Function* F)
    {
        // Skip functions with variable number of arguments, e.g. printf.
        if (F->isVarArg())
            return true;

        // Only non-extern functions within the module are optimized
        if (F->hasFnAttribute("referenced-indirectly") || F->isDeclaration()
            || F->isIntrinsic() || F->user_empty())
            return true;

        return false;
    };

    std::vector<Function*> candidates;
    for (auto I : post_order(&CG))
    {
        auto F = I->getFunction();
        if (!F)
            continue;
        if (skip(F))
            continue;

        auto hasGenericArg = [](Argument& arg) {
            Type* argTy = arg.getType();
            return argTy->isPointerTy() && argTy->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC;
        };

        if (std::any_of(F->arg_begin(), F->arg_end(), hasGenericArg))
            candidates.push_back(F);
    }

    return candidates;
}

void LowerGPCallArg::updateMetadata(Function* oldFunc, Function* newFunc) {
    MetadataBuilder mbuilder(m_module);
    auto& FuncMD = m_ctx->getModuleMetaData()->FuncMD;

    auto oldFuncIter = m_mdUtils->findFunctionsInfoItem(oldFunc);
    m_mdUtils->setFunctionsInfoItem(newFunc, oldFuncIter->second);
    m_mdUtils->eraseFunctionsInfoItem(oldFuncIter);
    mbuilder.UpdateShadingRate(oldFunc, newFunc);
    auto loc = FuncMD.find(oldFunc);
    if (loc != FuncMD.end())
    {
        auto funcInfo = loc->second;
        FuncMD.erase(oldFunc);
        FuncMD[newFunc] = std::move(funcInfo);
    }

    m_mdUtils->save(m_module->getContext());
}

Function* LowerGPCallArg::createFuncWithLoweredArgs(Function* F, GenericPointerArgs& argsInfo)
{
    FunctionType* pFuncType = F->getFunctionType();
    std::vector<Type*> newParamTypes(pFuncType->param_begin(), pFuncType->param_end());
    for (auto& argInfo : argsInfo)
    {
        PointerType* ptrType = IGCLLVM::getWithSamePointeeType(dyn_cast<PointerType>(newParamTypes[argInfo.argNo]),
            argInfo.addrSpace);
        newParamTypes[argInfo.argNo] = ptrType;
    }

    FunctionType* newFTy = FunctionType::get(F->getReturnType(), newParamTypes, F->isVarArg());
    Function* newFunc = Function::Create(newFTy, F->getLinkage());
    newFunc->copyAttributesFrom(F);
    newFunc->setSubprogram(F->getSubprogram());
    m_module->getFunctionList().insert(F->getIterator(), newFunc);
    newFunc->takeName(F);
    IGCLLVM::splice(newFunc, newFunc->begin(), F);

    return newFunc;
}

std::optional<unsigned> LowerGPCallArg::getOriginAddressSpace(Function* func, unsigned argNo)
{
    std::optional<unsigned> originAddressSpace;

    // Check if all the callers have the same pointer address space
    for (auto U : func->users())
    {
        auto CI = cast<CallInst>(U);
        Value* V = CI->getArgOperand(argNo);

        if (!V->getType()->isPointerTy())
            continue;

        if (AddrSpaceCastInst* ASC = dyn_cast<AddrSpaceCastInst>(V))
        {
            IGC_ASSERT(ASC->getDestAddressSpace() == ADDRESS_SPACE_GENERIC);

            unsigned srcAddrSpace = ASC->getSrcAddressSpace();
            if (originAddressSpace && originAddressSpace.value() != srcAddrSpace)
                return std::nullopt;

            originAddressSpace = srcAddrSpace;
        }
        else
        {
            return std::nullopt;
        }
    }

    return originAddressSpace;
}

// Loops over the argument list transferring uses from old function to new one.
void LowerGPCallArg::updateFunctionArgs(Function* oldFunc, Function* newFunc)
{
    for (const auto& ArgPair : llvm::zip(oldFunc->args(), newFunc->args()))
    {
        Value* oldArg = &std::get<0>(ArgPair);
        Value* newArg = &std::get<1>(ArgPair);

        newArg->takeName(oldArg);

        if (oldArg->getType() == newArg->getType())
        {
            oldArg->replaceAllUsesWith(newArg);
            continue;
        }

        auto* NewArgToGeneric = CastInst::Create(
            Instruction::AddrSpaceCast, newArg, oldArg->getType(), "", newFunc->getEntryBlock().getFirstNonPHI());
        oldArg->replaceAllUsesWith(NewArgToGeneric);

        LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>(*newFunc).getLoopInfo();
        GASPropagator Propagator(newFunc->getContext(), &LI);
        Propagator.propagate(newArg);
    }
}

// This function takes an Old value and a New value. If Old value is referenced in a
// dbg.value or dbg.declare instruction, it replaces that intrinsic and makes new one
// use the New value.
//
// This function is required anytime a pass modifies IR such that RAUW cannot be
// used to directly update uses in metadata node. In case of GAS, RAUW asserts because
// addrspace used in Old/New values are different and this is interpreted as different
// types by LLVM and RAUW on different types is forbidden.
void replaceValueInDbgInfoIntrinsic(llvm::Value* Old, llvm::Value* New, llvm::Module& M)
{
    if (Old->isUsedByMetadata())
    {
        auto localAsMD = ValueAsMetadata::getIfExists(Old);
        auto addrSpaceMD = MetadataAsValue::getIfExists(Old->getContext(), localAsMD);
        if (addrSpaceMD)
        {
            llvm::DIBuilder DIB(M);
            std::vector<llvm::DbgInfoIntrinsic*> DbgInfoInstToDelete;
            for (auto* User : addrSpaceMD->users())
            {
                if (cast<DbgInfoIntrinsic>(User))
                {
                    //User->dump();
                    if (auto DbgV = cast<DbgValueInst>(User))
                    {
                        DIB.insertDbgValueIntrinsic(New,
                            DbgV->getVariable(), DbgV->getExpression(), DbgV->getDebugLoc().get(),
                            cast<llvm::Instruction>(User));
                    }
                    else if (auto DbgD = cast<DbgDeclareInst>(User))
                    {
                        DIB.insertDeclare(New,
                            DbgD->getVariable(), DbgD->getExpression(), DbgD->getDebugLoc().get(),
                            cast<llvm::Instruction>(User));
                    }
                    DbgInfoInstToDelete.push_back(cast<llvm::DbgInfoIntrinsic>(User));
                }
            }

            for (auto DbgInfoInst : DbgInfoInstToDelete)
                DbgInfoInst->eraseFromParent();
        }
    }
}

void LowerGPCallArg::updateAllUsesWithNewFunction(Function* oldFunc, Function* newFunc)
{
    IGC_ASSERT(!oldFunc->use_empty());

    // Keep track of old calls and addrspacecast to be deleted later
    std::vector<CallInst*> callsToDelete;
    std::vector<AddrSpaceCastInst*> ASCToDelete;
    std::vector<Use*> UsesToReplace;

    for (auto U = oldFunc->user_begin(), E = oldFunc->user_end(); U != E; ++U)
    {
        CallInst* cInst = dyn_cast<CallInst>(*U);
        auto BC = dyn_cast<BitCastInst>(*U);
        if (BC && BC->hasOneUse())
            cInst = dyn_cast<CallInst>(BC->user_back());

        if (!cInst)
        {
            IGC_ASSERT_MESSAGE(0, "Unknown function usage");
            return;
        }

        if (cInst->getCalledFunction() != oldFunc)
        {
            for (Use& cArg : cInst->args())
                if (cArg == oldFunc)
                    UsesToReplace.push_back(&cArg);
            continue;
        }

        // Prepare args for new call
        std::vector<Value*> newCallArgs;

        auto AI = newFunc->arg_begin();
        for (unsigned int i = 0; i < IGCLLVM::getNumArgOperands(cInst); ++i, ++AI)
        {
            Value* callArg = cInst->getOperand(i);
            Value* funcArg = AI;
            if (callArg->getType() != funcArg->getType())
            {
                IGC_ASSERT(callArg->getType()->isPointerTy() &&
                    funcArg->getType()->isPointerTy());

                PointerType* callArgTy = dyn_cast<PointerType>(callArg->getType());
                PointerType* funcArgTy = dyn_cast<PointerType>(funcArg->getType());
                IGC_ASSERT(
                    callArgTy->getAddressSpace() == ADDRESS_SPACE_GENERIC &&
                    funcArgTy->getAddressSpace() != ADDRESS_SPACE_GENERIC);
                // If call site address space is generic and function arg is non-generic,
                // the addrspacecast is removed and non-generic address space lowered
                // to the function call.
                AddrSpaceCastInst* addrSpaceCastInst = dyn_cast<AddrSpaceCastInst>(callArg);
                if (addrSpaceCastInst)
                {
                    callArg = addrSpaceCastInst->getOperand(0);
                    if (addrSpaceCastInst->hasOneUse())
                    {
                        // when addrspacecast is used in a metadata node, replacing it
                        // requires reconstruction of the node. we cannot used standard
                        // llvm APIs to replace uses as they require that type be
                        // preserved, which is not in this case.
                        replaceValueInDbgInfoIntrinsic(addrSpaceCastInst, addrSpaceCastInst->getPointerOperand(),
                            *newFunc->getParent());
                        ASCToDelete.push_back(addrSpaceCastInst);
                    }
                }
            }
            newCallArgs.push_back(callArg);
        }

        // Create new call and insert it before old one
        CallInst* inst = CallInst::Create(newFunc, newCallArgs,
            newFunc->getReturnType()->isVoidTy() ? "" : newFunc->getName(),
            cInst);

        inst->setCallingConv(newFunc->getCallingConv());
        inst->setDebugLoc(cInst->getDebugLoc());
        cInst->replaceAllUsesWith(inst);
        callsToDelete.push_back(cInst);
    }

    // Delete old calls
    for (auto i : callsToDelete)
    {
        i->eraseFromParent();
    }

    // Delete addrspacecasts that are no longer needed
    for (auto i : ASCToDelete)
    {
        IGC_ASSERT(i->user_empty());
        i->eraseFromParent();
    }

    // Replace call arguments
    for (auto i : UsesToReplace)
    {
        i->set(newFunc);
    }
}
