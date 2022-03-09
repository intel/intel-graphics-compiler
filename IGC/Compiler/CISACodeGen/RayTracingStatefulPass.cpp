/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass will convert all load/store instructions with the address spaces
/// associated with raytracing memory regions to ldraw/storeraw with the
/// resource loaded from the RayDispatchGlobalData and the pointer becoming
/// the offset. For example:
///
/// %SWStackOffset = load i32, i32 addrspace(3080194)* %"&SWStackOffset", align 16
///
/// ==>
///
/// %9 = add i32 %baseSSHOffset, 64
/// %10 = ptrtoint i32 addrspace(3080194)* %"&SWStackOffset" to i32
/// %11 = inttoptr i32 %9 to i8 addrspace(3080194)*
/// %SWStackOffset = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p3080194i8(i8 addrspace(3080194)* %11, i32 %10, i32 16, i1 false)
///
//===----------------------------------------------------------------------===//

#include "RayTracingStatefulPass.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "AdaptorCommon/RayTracing/RTBuilder.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// We run this prior to legalization so we should ensure any loads/stores to
// i1 are upconverted prior to using ldraw*/storeraw* intrinsics.

static LoadInst* legalizeLoad(LoadInst* LI)
{
    if (!LI->getType()->isIntegerTy(1))
        return LI;

    IRBuilder<> IRB(LI);

    PointerType* ptrTy = cast<PointerType>(LI->getPointerOperand()->getType());
    unsigned addressSpace = ptrTy->getAddressSpace();
    PointerType* I8PtrTy = IRB.getInt8PtrTy(addressSpace);
    Value* I8PtrOp = IRB.CreateBitCast(LI->getPointerOperand(), I8PtrTy);

    LoadInst* pNewLoadInst = IGC::cloneLoad(LI, I8PtrOp);
    Value* newVal = IRB.CreateTrunc(pNewLoadInst, LI->getType());
    LI->replaceAllUsesWith(newVal);
    LI->eraseFromParent();

    return pNewLoadInst;
}

static StoreInst* legalizeStore(StoreInst* SI)
{
    if (!SI->getValueOperand()->getType()->isIntegerTy(1))
        return SI;

    IRBuilder<> IRB(SI);

    Value* newVal = IRB.CreateZExt(SI->getValueOperand(), IRB.getInt8Ty());

    PointerType* ptrTy = cast<PointerType>(SI->getPointerOperand()->getType());
    unsigned addressSpace = ptrTy->getAddressSpace();
    PointerType* I8PtrTy = IRB.getInt8PtrTy(addressSpace);
    Value* I8PtrOp = IRB.CreateBitCast(SI->getPointerOperand(), I8PtrTy);

    auto *NewSI = IGC::cloneStore(SI, newVal, I8PtrOp);
    SI->eraseFromParent();
    return NewSI;
}

Value* getBaseSSHOffset(CodeGenContext* Ctx, RTBuilder& RTB) {
    if (Ctx->type == ShaderType::RAYTRACING_SHADER)
    {
        return RTB.getBaseSSHOffset();
    }
    else
    {
        Function* pFunc = GenISAIntrinsic::getDeclaration(
            Ctx->getModule(),
            GenISAIntrinsic::GenISA_RuntimeValue,
            RTB.getInt32Ty());
        return RTB.CreateCall(pFunc, RTB.getInt32(Ctx->getModuleMetaData()->pushInfo.rtSyncSurfPtrOffset));
    }

}

bool RaytracingStatefulPass::runOnFunction(Function& F)
{
    CodeGenContext *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    auto& rtInfo = Ctx->getModuleMetaData()->rtInfo;

    RTBuilder RTB(F.getContext(), *Ctx);

    bool Changed = false;

    for (auto II = inst_begin(&F), IE = inst_end(&F); II != IE; /* empty */)
    {
        Instruction* I = &*II++;
        Value* PointerOp = nullptr;
        if (auto* LI = dyn_cast<LoadInst>(I))
            PointerOp = LI->getPointerOperand();
        else if (auto* SI = dyn_cast<StoreInst>(I))
            PointerOp = SI->getPointerOperand();
        else
            continue;

        uint32_t Addrspace = PointerOp->getType()->getPointerAddressSpace();

        uint32_t BaseOffset = 0;
        if (Addrspace == rtInfo.RTAsyncStackAddrspace)
            BaseOffset = *rtInfo.RTAsyncStackSurfaceStateOffset;
        else if (Addrspace == rtInfo.SWHotZoneAddrspace)
            BaseOffset = *rtInfo.SWHotZoneSurfaceStateOffset;
        else if (Addrspace == rtInfo.SWStackAddrspace)
            BaseOffset = *rtInfo.SWStackSurfaceStateOffset;
        else if (Addrspace == rtInfo.RTSyncStackAddrspace)
            BaseOffset = *rtInfo.RTSyncStackSurfaceStateOffset;
        else
            continue;

        if (DecodeBufferType(Addrspace) == SSH_BINDLESS)
            BaseOffset <<= Ctx->platform.getBSOLocInExtDescriptor();

        RTB.SetInsertPoint(I);
        auto* BaseSSHOffset = getBaseSSHOffset(Ctx, RTB);
        auto* ResourceOffset =
            RTB.CreateAdd(BaseSSHOffset, RTB.getInt32(BaseOffset));

        auto* Offset = RTB.CreatePtrToInt(PointerOp, RTB.getInt32Ty());
        auto* ResourcePtr = RTB.CreateIntToPtr(
            ResourceOffset,
            RTB.getInt8PtrTy(Addrspace));

        if (auto* LI = dyn_cast<LoadInst>(I))
        {
            LI = legalizeLoad(LI);
            auto* LRI = CreateLoadRawIntrinsic(LI, ResourcePtr, Offset);
            LRI->takeName(LI);
            LI->replaceAllUsesWith(LRI);
            LI->eraseFromParent();
        }
        else if (auto* SI = dyn_cast<StoreInst>(I))
        {
            SI = legalizeStore(SI);
            CreateStoreRawIntrinsic(SI, ResourcePtr, Offset);
            SI->eraseFromParent();
        }

        Changed = true;
    }

    return Changed;
}

char RaytracingStatefulPass::ID = 0;
#define PASS_FLAG2 "raytracing-stateful-init"
#define PASS_DESCRIPTION2 "Adds intrinsic for indirect stateful access"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(RaytracingStatefulPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(RaytracingStatefulPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
