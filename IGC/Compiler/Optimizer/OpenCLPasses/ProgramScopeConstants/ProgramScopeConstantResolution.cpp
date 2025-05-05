/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "AdaptorCommon/AddImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantResolution.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/MapVector.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include <vector>
#include "Probe/Assertion.h"
#include "BiFModule/Headers/bif_control_common.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-programscope-constant-resolve"
#define PASS_DESCRIPTION "Resolves references to inline constants"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ProgramScopeConstantResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ProgramScopeConstantResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ProgramScopeConstantResolution::ID = 0;

ProgramScopeConstantResolution::ProgramScopeConstantResolution(bool Conservatively)
    : ModulePass(ID), RunCautiously(Conservatively)
{
    initializeProgramScopeConstantResolutionPass(*PassRegistry::getPassRegistry());
}

static bool needRunConservatively(const Module& M) {
    for (auto& F : M) {
        for (auto& BB : F) {
            for (auto& I : BB) {
                const AddrSpaceCastInst* ASCI = dyn_cast<AddrSpaceCastInst>(&I);
                if (!ASCI)
                    continue;
                if (ASCI->getSrcTy()->getPointerAddressSpace() == ADDRESS_SPACE_CONSTANT)
                    return true;
            }
        }
    }
    return false;
}

static bool isLoweredToRelocation(const GlobalVariable *GV)
{
  StringRef name = GV->getName();
  if (name == "__SubDeviceID" || name == BIF_FLAG_CTRL_N_S(MaxHWThreadIDPerSubDevice))
      return true;
  return false;
}

bool ProgramScopeConstantResolution::runOnModule(Module& M)
{
    LLVMContext& C = M.getContext();

    MetaDataUtils* mdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    if (modMD->inlineProgramScopeOffsets.empty())
    {
        // There are no constants, or no constants are used, so we have nothing to do.
        return false;
    }

    if (RunCautiously) {
        if (!needRunConservatively(M))
            return false;
        // RED ALERT! RED ALERT! RED ALERT! Rats found!
        // Per OpenCL C spec, no `constant` object are allowed to be written.
        // Compile should report compile time errors once such kind of usage is
        // found. However, we have tests, which needs passing, rely on a
        // constant buffer to be populated with data in runtime. That violates
        // the OpenCL C spec, either OCL 1.2 or OCL 2.0.
        //
        // We will run constant lowering if we are asked to run cautiously and
        // we found cases where we need to run conservatively, i.e., there are
        // `addrspacecast` from constant address space into other address spaces
        // (private/local/global) where writes are allowed. Once such cases are
        // found, we run constant lowering in the original order before
        // optimization; otherwise, we run post-optimization lowering of
        // constant.
    }

    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I)
    {
        GlobalVariable* pGlobalVar = &(*I);
        PointerType* ptrType = cast<PointerType>(pGlobalVar->getType());
        IGC_ASSERT_MESSAGE(ptrType, "The type of a global variable must be a pointer type");

        // Do not convert future relocations to implict argument.
        if (isLoweredToRelocation(pGlobalVar))
            continue;

        // Pointer's address space should be either constant or global
        const unsigned AS = ptrType->getAddressSpace();

        // local address space variables are also generated as GlobalVariables.
        // Ignore them here.
        if (AS == ADDRESS_SPACE_LOCAL)
        {
            continue;
        }

        if (AS != ADDRESS_SPACE_CONSTANT &&
            AS != ADDRESS_SPACE_GLOBAL &&
            // This is a workaround for clang bug, clang creates string constants with private address sapce!
            AS != ADDRESS_SPACE_PRIVATE)
        {
            IGC_ASSERT_MESSAGE(0, "program scope variable with unexpected address space");
            continue;
        }

        auto bufferOffset = modMD->inlineProgramScopeOffsets.find(pGlobalVar);
        if (bufferOffset == modMD->inlineProgramScopeOffsets.end())
            continue; // This constant is not used, so it didn't get an offset.

        // Get the offset of this constant from the base.
        int64_t offset = bufferOffset->second;
        if (offset == -1)
            continue;

        ConstantInt* pOffset = ConstantInt::get(Type::getInt64Ty(C), offset);
        const ImplicitArg::ArgType argType =
            AS == ADDRESS_SPACE_GLOBAL ? ImplicitArg::GLOBAL_BASE : ImplicitArg::CONSTANT_BASE;

        // Now, go over the users of this constant.
        // First, copy use list, because we will be removing uses.
        std::vector<User*> useVector(pGlobalVar->user_begin(), pGlobalVar->user_end());
        llvm::MapVector<Function*, llvm::MapVector<GlobalVariable*, Value*>> funcToVarSet;

        for (std::vector<User*>::iterator U = useVector.begin(), UE = useVector.end(); U != UE; ++U)
        {
            Instruction* user = dyn_cast<Instruction>(*U);
            if (!user)
            {
                continue;
            }

            Function* userFunc = user->getParent()->getParent();

            // Skip functions called from function marked with stackcall attribute
            if (AddImplicitArgs::hasStackCallInCG(userFunc))
                continue;

            // Skip unused internal functions.
            if (mdUtils->findFunctionsInfoItem(userFunc) == mdUtils->end_FunctionsInfo())
            {
                IGC_ASSERT(userFunc->use_empty() && Function::isDiscardableIfUnused(userFunc->getLinkage()));
                continue;
            }

            ImplicitArgs implicitArgs(*userFunc, mdUtils);

            // Skip if this function does not have the implicit arg
            if (!implicitArgs.isImplicitArgExist(argType))
                continue;

            // Find the implicit argument representing this constant.
            IGC_ASSERT_MESSAGE(userFunc->arg_size() >= implicitArgs.size(), "Function arg size does not match meta data args.");
            unsigned int ImplicitArgsBaseIndex = userFunc->arg_size() - implicitArgs.size();
            unsigned int implicitArgIndex = implicitArgs.getArgIndex(argType);
            unsigned int implicitArgIndexInFunc = ImplicitArgsBaseIndex + implicitArgIndex;
            Function::arg_iterator bufArg = std::next(userFunc->arg_begin(), implicitArgIndexInFunc);

            if (!funcToVarSet[userFunc].count(pGlobalVar))
            {
                Instruction* pEntryPoint = &(*userFunc->getEntryBlock().getFirstInsertionPt());

                // Create a GEP to get to the right offset in the constant buffer
                GetElementPtrInst *gep = GetElementPtrInst::Create(
                    Type::getInt8Ty(C), &*bufArg, pOffset,
                    "off" + pGlobalVar->getName(), pEntryPoint);

                Value* replacement = gep;

                // TODO: Remove when typed pointers are no longer supported.
                if (!ptrType->isOpaque())
                  replacement = CastInst::CreatePointerCast(
                      gep, pGlobalVar->getType(),
                      "cast" + pGlobalVar->getName(), pEntryPoint);

                // Update the map with the fix new value
                funcToVarSet[userFunc][pGlobalVar] = replacement;
            }

            Value* bc = funcToVarSet[userFunc][pGlobalVar];
            IGC_ASSERT_MESSAGE(bc != nullptr, "Program Scope buffer handling is broken!");

            // And actually use the bitcast.
            user->replaceUsesOfWith(pGlobalVar, bc);
        }
    }

    mdUtils->save(C);

    return true;
}
