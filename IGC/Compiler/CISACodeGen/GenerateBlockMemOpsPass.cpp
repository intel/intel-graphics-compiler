/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "CodeGenPublicEnums.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenerateBlockMemOpsPass.hpp"
#include "IGCIRBuilder.h"

using namespace llvm;
using namespace IGC;

char GenerateBlockMemOpsPass::ID = 0;

#define PASS_FLAG "generate-block-mem-ops"
#define PASS_DESCRIPTION "Generation of block load / block stores instead of regular load / stores."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenerateBlockMemOpsPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(GenerateBlockMemOpsPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

const uint MaxSgSize = 32;

GenerateBlockMemOpsPass::GenerateBlockMemOpsPass() : FunctionPass(ID) {
    initializeGenerateBlockMemOpsPassPass(*PassRegistry::getPassRegistry());
}

bool GenerateBlockMemOpsPass::runOnFunction(Function &F) {
    if (skipFunction(F))
        return false;

    bool Changed = false;
    SmallVector<llvm::Instruction*, 32> LoadStoreToProcess;

    MdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    IGCMD::FunctionInfoMetaDataHandle Info = MdUtils->getFunctionsInfoItem(&F);
    if (Info->getType() != FunctionTypeMD::KernelFunction)
        return false;

    // Check that workgroups have been scalarized along the x-axis.
    if (!checkVectorizationAlongX(&F))
        return false;

    WI = &getAnalysis<WIAnalysis>();

    // Collect all load / store instructions which can be replaced.
    for (auto &B : F)
        for (auto &I : B)
            if (canOptLoadStore(&I))
                LoadStoreToProcess.push_back(&I);

    // Replace load / store instructions with block ones.
    for (auto I : LoadStoreToProcess)
        Changed = changeToBlockInst(I);

    return Changed;
}

bool GenerateBlockMemOpsPass::isAddressAligned(Value *Ptr, const alignment_t &CurrentAlignment, Type *DataType) {
    unsigned ScalarSize = DataType->getScalarSizeInBits();

    // The list of possible alignments should be expanded.
    if (CGCtx->platform.isProductChildOf(IGFX_PVC))
        if ((ScalarSize == 32) && (CurrentAlignment == 4))
            return true;

    return false;
}

// This function checks if Indx is equal to 1 * LocalIdX + UniformPart, assuming LocalIdY and LocalIdZ are uniform values.
bool GenerateBlockMemOpsPass::isIndexContinuous(Value *Indx) {
    Instruction *FirstRoot = dyn_cast<Instruction>(Indx);
    if (!FirstRoot)
        return false;

    bool lx = false;

    // NonUniformInstsMap is used to collect non-uniform part of address arithmetic.
    // The bool value indicates whether the arithmetic chain contained an instruction other than a zext or add instruction.
    DenseMap<Value*, bool> NonUniformInstsMap;
    NonUniformInstsMap.insert({Indx, false});

    // Continue algorithm untill we have non-uniform values in the list.
    while (NonUniformInstsMap.size()) {
        for (const auto& Item : NonUniformInstsMap) {
            Value *CurrentVal = Item.first;
            bool ContainRestrInstr = Item.second;

            Instruction *CurrentInst = dyn_cast<Instruction>(CurrentVal);
            if (!CurrentInst)
                return false;

            for (auto Op = CurrentInst->op_begin(), E = CurrentInst->op_end(); Op != E; Op++) {
                Value *OpVal = cast<Value>(Op);

                if (!WI->isUniform(OpVal)) {
                    if (isa<Instruction>(OpVal)) {

                        if (isa<ZExtInst>(CurrentInst) || (CurrentInst->getOpcode() == Instruction::Add)) {
                            NonUniformInstsMap.insert({OpVal, ContainRestrInstr});
                        } else {
                            // Mark the value as true, since it is used in instructions other than add or zext.
                            NonUniformInstsMap.insert({OpVal, true});
                        }

                    } else {
                        if (isa<Argument>(OpVal)) {

                            if (OpVal == getLocalId(CurrentInst->getFunction(), ImplicitArg::LOCAL_ID_X)) {
                                // If local_id_x has already been encountered during analysis or the chain contains prohibited instructions,
                                // then the index is not continuous.
                                if (lx || ContainRestrInstr)
                                    return false;

                                lx = true;
                            } else if ((OpVal != getLocalId(CurrentInst->getFunction(), ImplicitArg::LOCAL_ID_Y)) &&
                                    (OpVal != getLocalId(CurrentInst->getFunction(), ImplicitArg::LOCAL_ID_Z))) {
                                // We assume that local_id_y and local_id_z are uniform, so it doesn't matter which instructions they were used in.
                                return false;
                            }

                        } else {
                            return false;
                        }
                    }
                }
            }

            NonUniformInstsMap.erase(CurrentInst);
        }
    }

    // If local_id_x was met then index is continuous.
    return lx;
}


bool GenerateBlockMemOpsPass::checkVectorizationAlongX(Function *F) {
    if (CGCtx->type != ShaderType::OPENCL_SHADER)
        return false;

    IGC::IGCMD::FunctionInfoMetaDataHandle funcInfoMD = MdUtils->getFunctionsInfoItem(F);
    ModuleMetaData *modMD = CGCtx->getModuleMetaData();
    auto funcMD = modMD->FuncMD.find(F);

    if (funcMD == modMD->FuncMD.end())
        return false;

    WorkGroupWalkOrderMD workGroupWalkOrder = funcMD->second.workGroupWalkOrder;
    if (workGroupWalkOrder.dim0 != 0 || workGroupWalkOrder.dim1 != 1 || workGroupWalkOrder.dim2 != 2)
        return false;

    int32_t X = -1;
    IGC::IGCMD::ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();
    if (!threadGroupSize->hasValue())
        return false;

    X = (int32_t)threadGroupSize->getXDim();
    if (!X)
        return false;

    if (X % MaxSgSize == 0)
        return true;

    return false;
}

bool GenerateBlockMemOpsPass::canOptLoadStore(Instruction *I) {
    if (!isa<LoadInst>(I) && !isa<StoreInst>(I))
        return false;

    // Block read and write instructions must be called by all elements in the subgroup.
    if (WI->insideDivergentCF(I))
        return false;

    Value *Ptr = nullptr;
    Value *ValOp = nullptr;
    Type *DataType = nullptr;
    alignment_t CurrentAlignment = 0;

    if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
        Ptr = LI->getPointerOperand();
        CurrentAlignment = IGCLLVM::getAlignmentValue(LI);
        DataType = cast<Value>(LI)->getType();
    } else {
        StoreInst* SI = cast<StoreInst>(I);
        Ptr = SI->getPointerOperand();
        ValOp = SI->getValueOperand();
        CurrentAlignment = IGCLLVM::getAlignmentValue(SI);
        DataType = ValOp->getType();
    }

    if (DataType->isVectorTy())
        return false;

    // Need to check what alignment block load/store requires for the specific architecture.
    if (!isAddressAligned(Ptr, CurrentAlignment, DataType))
        return false;

    GetElementPtrInst *Gep = dyn_cast<GetElementPtrInst>(Ptr);
    // Get the last index from the getelementptr instruction if it is not uniform in the subgroup.
    Value *Idx = checkGep(Gep);

    if (!Idx)
        return false;

    // Check that memory access is continuous in the subgroup.
    if (!isIndexContinuous(Idx))
        return false;

    return true;
}

Value *GenerateBlockMemOpsPass::getLocalId(Function *F, IGC::ImplicitArg::ArgType Id) {
    Instruction *Pos = F->getEntryBlock().getFirstNonPHI();
    IRBuilder<> Builder(Pos);
    ImplicitArgs implicitArgs(*F, MdUtils);
    Value *V = implicitArgs.getImplicitArgValue(*F, Id, MdUtils);
    return V;
}

bool GenerateBlockMemOpsPass::changeToBlockInst(Instruction *I) {
    IRBuilder<> Builder(I);
    Function *BlockOpDecl = nullptr;
    CallInst *BlockOpCall = nullptr;

    if (isa<LoadInst>(I)) {
        Value *Args[1] = {I->getOperand(0)};
        Type *Types[2] = {I->getType(), I->getOperand(0)->getType()};
        BlockOpDecl = GenISAIntrinsic::getDeclaration(I->getModule(),
            GenISAIntrinsic::GenISA_simdBlockRead,
            Types);
        BlockOpCall = Builder.CreateCall(BlockOpDecl, Args);
    } else {
        Value *Args[2] = {I->getOperand(1), I->getOperand(0)};
        Type *Types[2] = {I->getOperand(1)->getType(), I->getOperand(0)->getType()};
        BlockOpDecl = GenISAIntrinsic::getDeclaration(I->getModule(),
            GenISAIntrinsic::GenISA_simdBlockWrite,
            Types);
        BlockOpCall = Builder.CreateCall(BlockOpDecl, Args);
    }

    if (!BlockOpCall)
        return false;

    I->replaceAllUsesWith(BlockOpCall);
    I->eraseFromParent();

    return true;
}

Value *GenerateBlockMemOpsPass::checkGep(GetElementPtrInst *Gep) {
    if (!Gep)
        return nullptr;

    bool IsPtrUniform = false, IsLastIndUniform = false;
    Value *Ptr = Gep->getOperand(0);

    if (WI->isUniform(Ptr))
        IsPtrUniform = true;

    // Make sure that all indexes, not including the last one, are uniform.
    // This is important because the address must be continuous in the subgroup.
    for (auto Idx = Gep->idx_begin(), E = Gep->idx_end() - 1; Idx != E; Idx++)
        if (!WI->isUniform(*Idx))
            return nullptr;

    auto LIndx = Gep->idx_end() - 1;

    if (WI->isUniform(*LIndx))
        IsLastIndUniform = true;

    if (!IsLastIndUniform && IsPtrUniform) {
        return *LIndx;
    } else if (IsLastIndUniform && !IsPtrUniform) {
        if (!isa<GetElementPtrInst>(Ptr))
            return nullptr;

        return checkGep(cast<GetElementPtrInst>(Ptr));
    }

    return nullptr;
}