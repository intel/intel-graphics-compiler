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

//===- ConstantProp.cpp - Code to perform Simple Constant Propagation -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/*========================== CustomUnsafeOptPass.cpp ==========================

This file contains IGC custom optimizations that are arithmetically safe.
The passes are
    CustomSafeOptPass
    GenSpecificPattern
    IGCConstProp
    IGCIndirectICBPropagaion
    CustomLoopInfo
    GenStrengthReduction
    FlattenSmallSwitch

CustomSafeOptPass does peephole optimizations 
For example, reduce the alloca size so there is a chance to promote indexed temp.

GenSpecificPattern reverts llvm changes back to what is needed. 
For example, llvm changes AND to OR, and GenSpecificPaattern changes it back to 
allow more optimizations

IGCConstProp was originated from llvm copy-prop code with one addition for 
shader-constant replacement. So llvm copyright is added above.

IGCIndirectICBPropagaion reads the immediate constant buffer from meta data and 
use them as immediates instead of using send messages to read from buffer.

CustomLoopInfo returns true if there is any sampleL in a loop for the driver.

GenStrengthReduction performs a fdiv optimization.

FlattenSmallSwitch flatten the if/else or switch structure and use cmp+sel 
instead if the structure is small.

=============================================================================*/

#include "Compiler/CustomSafeOptPass.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"

#include "WrapperLLVM/Utils.h"
#include "llvmWrapper/IR/IRBuilder.h"

#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Analysis/ValueTracking.h>
#include "common/LLVMWarningsPop.hpp"

#include <set>
#include "../inc/common/secure_mem.h"

using namespace llvm;
using namespace IGC;
using namespace GenISAIntrinsic;

// Register pass to igc-opt
#define PASS_FLAG1 "igc-custom-safe-opt"
#define PASS_DESCRIPTION1 "Custom Pass Optimization"
#define PASS_CFG_ONLY1 false
#define PASS_ANALYSIS1 false
IGC_INITIALIZE_PASS_BEGIN(CustomSafeOptPass, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)
IGC_INITIALIZE_PASS_END(CustomSafeOptPass, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)

char CustomSafeOptPass::ID = 0;

CustomSafeOptPass::CustomSafeOptPass() : FunctionPass(ID)
{
    initializeCustomSafeOptPassPass(*PassRegistry::getPassRegistry());
}

#if 0
// In some cases we link LLVM with NDEBUG set with IGC without NDEBUG set, this causes this function to not be missing during linking
// Once we switch to CMAKE this code can be removed
#if (defined(_INTERNAL) && defined(NDEBUG)) && ( !defined( LLVM_ENABLE_THREADS ) || LLVM_ENABLE_THREADS == 0 || ( defined( IGC_CMAKE ) && defined( NDEBUG ) ) || ( !defined( IGC_CMAKE ) && !defined( NDEBUG ) ) )
void AnnotateHappensBefore(const char *file, int line,
                           const volatile void *cv) {}
void AnnotateHappensAfter(const char *file, int line,
                          const volatile void *cv) {}
void AnnotateIgnoreWritesBegin(const char *file, int line) {}
void AnnotateIgnoreWritesEnd(const char *file, int line) {}
#endif
#endif

#define DEBUG_TYPE "CustomSafeOptPass"

STATISTIC(Stat_FcmpRemoved,  "Number of insts removed in FCmp Opt");
STATISTIC(Stat_FloatRemoved,  "Number of insts removed in Float Opt");
STATISTIC(Stat_DiscardRemoved,  "Number of insts removed in Discard Opt");

bool CustomSafeOptPass::runOnFunction(Function &F)
{
    psHasSideEffect = getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->m_instrTypes.psHasSideEffect;
    visit(F);
    return true;
}

void CustomSafeOptPass::visitInstruction(Instruction &I)
{
    // nothing
}


void CustomSafeOptPass::visitAllocaInst(AllocaInst &I)
{
    // reduce the alloca size so there is a chance to promote indexed temp.

    // ex:                                                                  to:
    // dcl_indexable_temp x1[356], 4                                        dcl_indexable_temp x1[2], 4
    // mov x[1][354].xyzw, l(1f, 0f, -1f, 0f)                               mov x[1][0].xyzw, l(1f, 0f, -1f, 0f)
    // mov x[1][355].xyzw, l(1f, 1f, 0f, -1f)                               mov x[1][1].xyzw, l(1f, 1f, 0f, -1f)
    // mov r[1].xy, x[1][r[1].x + 354].xyxx                                 mov r[1].xy, x[1][r[1].x].xyxx

    // in llvm:                                                             to:
    // %outarray_x1 = alloca[356 x float], align 4                          %31 = alloca[2 x float]
    // %outarray_y2 = alloca[356 x float], align 4                          %32 = alloca[2 x float]
    // %27 = getelementptr[356 x float] * %outarray_x1, i32 0, i32 352      %35 = getelementptr[2 x float] * %31, i32 0, i32 0
    // store float 0.000000e+00, float* %27, align 4                        store float 0.000000e+00, float* %35, align 4
    // %28 = getelementptr[356 x float] * %outarray_y2, i32 0, i32 352      %36 = getelementptr[2 x float] * %32, i32 0, i32 0
    // store float 0.000000e+00, float* %28, align 4                        store float 0.000000e+00, float* %36, align 4
    // %43 = add nsw i32 %selRes_s, 354
    // %44 = getelementptr[356 x float] * %outarray_x1, i32 0, i32 %43      %51 = getelementptr[2 x float] * %31, i32 0, i32 %selRes_s
    // %45 = load float* %44, align 4                                       %52 = load float* %51, align 4

    llvm::Type* pType = I.getType()->getPointerElementType();
    if (!pType->isArrayTy() ||
        static_cast<ADDRESS_SPACE>(I.getType()->getAddressSpace()) != ADDRESS_SPACE_PRIVATE)
    {
        return;
    }

    if (!(pType->getArrayElementType()->isFloatingPointTy() ||
        pType->getArrayElementType()->isIntegerTy() ||
        pType->getArrayElementType()->isPointerTy() ))
    {
        return;
    }

    int index_lb = int_cast<int>(pType->getArrayNumElements());
    int index_ub = 0;

    // Find all uses of this alloca
    for (Value::user_iterator it = I.user_begin(), e = I.user_end(); it != e; ++it)
    {
        if (GetElementPtrInst *pGEP = llvm::dyn_cast<GetElementPtrInst>(*it))
        {
            ConstantInt *C0 = dyn_cast<ConstantInt>(pGEP->getOperand(1));
            if (!C0 || !C0->isZero() || pGEP->getNumOperands() != 3)
            {
                return;
            }
            for (Value::user_iterator use_it = pGEP->user_begin(), use_e = pGEP->user_end(); use_it != use_e; ++use_it)
            {
                if (llvm::LoadInst* pLoad = llvm::dyn_cast<llvm::LoadInst>(*use_it))
                {
                }
                else if (llvm::StoreInst* pStore = llvm::dyn_cast<llvm::StoreInst>(*use_it))
                {
                    llvm::Value* pValueOp = pStore->getValueOperand();
                    if (pValueOp == *it)
                    {
                        // GEP instruction is the stored value of the StoreInst (not supported case)
                        return;
                    }
                    if (dyn_cast<ConstantInt>(pGEP->getOperand(2)) && pGEP->getOperand(2)->getType()->isIntegerTy(32))
                    {
                        int currentIndex = int_cast<int>(
                            dyn_cast<ConstantInt>(pGEP->getOperand(2))->getZExtValue());
                        index_lb = (currentIndex < index_lb) ? currentIndex : index_lb;
                        index_ub = (currentIndex > index_ub) ? currentIndex : index_ub;

                    }
                    else
                    {
                        return;
                    }
                }
                else
                {
                    // This is some other instruction. Right now we don't want to handle these
                    return;
                }
            }
        }
        else
        {
            if (!IsBitCastForLifetimeMark(dyn_cast<Value>(*it)))
            {
                return;
            }
        }
    }

    unsigned int newSize = index_ub + 1 - index_lb;
    if (newSize >= pType->getArrayNumElements())
    {
        return;
    }
    // found a case to optimize
    IGCLLVM::IRBuilder<> IRB(&I);
    llvm::ArrayType* allocaArraySize = llvm::ArrayType::get(pType->getArrayElementType(), newSize);
    llvm::Value* newAlloca = IRB.CreateAlloca( allocaArraySize, nullptr);
    llvm::Value* gepArg1;

    for (Value::user_iterator it = I.user_begin(), e = I.user_end(); it != e; ++it)
    {
        if (GetElementPtrInst *pGEP = llvm::dyn_cast<GetElementPtrInst>(*it))
        {
            if (dyn_cast<ConstantInt>(pGEP->getOperand(2)))
            {
                // pGEP->getOperand(2) is constant. Reduce the constant value directly
                int newIndex = int_cast<int>(dyn_cast<ConstantInt>(pGEP->getOperand(2))->getZExtValue())
                    - index_lb;
                gepArg1 = IRB.getInt32(newIndex);
            }
            else
            {
                // pGEP->getOperand(2) is not constant. create a sub instruction to reduce it
                gepArg1 = BinaryOperator::CreateSub(pGEP->getOperand(2), IRB.getInt32(index_lb), "reducedIndex", pGEP);
            }
            llvm::Value* gepArg[] = { pGEP->getOperand(1), gepArg1 };
            llvm::Value* pGEPnew = GetElementPtrInst::Create(nullptr, newAlloca, gepArg, "", pGEP);
            pGEP->replaceAllUsesWith(pGEPnew);
        }
    }
}

void CustomSafeOptPass::visitCallInst(CallInst &C)
{
    // discard optimizations
    if(llvm::GenIntrinsicInst* inst = llvm::dyn_cast<GenIntrinsicInst>(&C))
    {
        GenISAIntrinsic::ID id = inst->getIntrinsicID();
        // try to prune the destination size
        switch (id)
        {
        case GenISAIntrinsic::GenISA_discard:
        {
            Value *srcVal0 = C.getOperand(0);
            if(ConstantInt *CI = dyn_cast<ConstantInt>(srcVal0))
            {
                if(CI->isZero()){ // i1 is false
                    C.eraseFromParent();
                    ++Stat_DiscardRemoved;
                }
                else if(!psHasSideEffect)
                {
                    BasicBlock *blk = C.getParent();
                    BasicBlock *pred = blk->getSinglePredecessor();
                    if(blk && pred)
                    {
                        BranchInst *cbr = dyn_cast<BranchInst>(pred->getTerminator());
                        if(cbr && cbr->isConditional())
                        {
                            if(blk == cbr->getSuccessor(0))
                            {
                                C.setOperand(0, cbr->getCondition());
                                C.removeFromParent();
                                C.insertBefore(cbr);
                            }
                            else if(blk == cbr->getSuccessor(1))
                            {
                                Value *flipCond = llvm::BinaryOperator::CreateNot(cbr->getCondition(), "", cbr);
                                C.setOperand(0, flipCond);
                                C.removeFromParent();
                                C.insertBefore(cbr);
                            }
                        }
                    }
                }
            }
            break;
        }

        case GenISAIntrinsic::GenISA_bfi:
            visitBfi(inst);
            break;

        case GenISAIntrinsic::GenISA_f32tof16_rtz:
        {
            visitf32tof16(inst);
            break;
        }

        case GenISAIntrinsic::GenISA_sampleBptr:
        {
            visitSampleBptr(llvm::cast<llvm::SampleIntrinsic>(inst));
            break;
        }

        case GenISAIntrinsic::GenISA_umulH:
        {
            visitMulH(inst, false);
            break;
        }

        case GenISAIntrinsic::GenISA_imulH:
        {
            visitMulH(inst, true);
            break;
        }

        case GenISAIntrinsic::GenISA_ldptr:
        {
            visitLdptr(inst);
            break;
        }

        default:
            break;
        }
    }
}

//
// pattern match packing of two half float from f32tof16:
//
// % 43 = call float @llvm.genx.GenISA.f32tof16.rtz(float %res_s55.i)
// % 44 = call float @llvm.genx.GenISA.f32tof16.rtz(float %res_s59.i)
// % 47 = bitcast float %44 to i32
// % 49 = bitcast float %43 to i32
// %addres_s68.i = shl i32 % 47, 16
// % mulres_s69.i = add i32 %addres_s68.i, % 49
// % 51 = bitcast i32 %mulres_s69.i to float
// into
// %43 = call half @llvm.genx.GenISA_ftof_rtz(float %res_s55.i)
// %44 = call half @llvm.genx.GenISA_ftof_rtz(float %res_s59.i)
// %45 = insertelement <2 x half>undef, %43, 0
// %46 = insertelement <2 x half>%45, %44, 1
// %51 = bitcast <2 x half> %46 to float
//
// or if the f32tof16 are from fpext half:
//
// % src0_s = fpext half %res_s to float
// % src0_s2 = fpext half %res_s1 to float
// % 2 = call fast float @llvm.genx.GenISA.f32tof16.rtz(float %src0_s)
// % 3 = call fast float @llvm.genx.GenISA.f32tof16.rtz(float %src0_s2)
// % 4 = bitcast float %2 to i32
// % 5 = bitcast float %3 to i32
// % addres_s = shl i32 % 4, 16
// % mulres_s = add i32 %addres_s, % 5
// % 6 = bitcast i32 %mulres_s to float
// into
// % 2 = insertelement <2 x half> undef, half %res_s1, i32 0, !dbg !113
// % 3 = insertelement <2 x half> % 2, half %res_s, i32 1, !dbg !113
// % 4 = bitcast <2 x half> % 3 to float, !dbg !113

void CustomSafeOptPass::visitf32tof16(llvm::CallInst* inst)
{
    if (!inst->hasOneUse())
    {
        return;
    }

    BitCastInst* bitcast = dyn_cast<BitCastInst>(*(inst->user_begin()));
    if (!bitcast || !bitcast->hasOneUse() || !bitcast->getType()->isIntegerTy(32))
    {
        return;
    }
    Instruction* addInst = dyn_cast<BinaryOperator>(*(bitcast->user_begin()));
    if (!addInst || addInst->getOpcode() != Instruction::Add || !addInst->hasOneUse())
    {
        return;
    }
    Instruction* lastValue = addInst;

    if (BitCastInst* finalBitCast = dyn_cast<BitCastInst>(*(addInst->user_begin())))
    {
        lastValue = finalBitCast;
    }

    // check the other half
    Value* otherOpnd = addInst->getOperand(0) == bitcast ? addInst->getOperand(1) : addInst->getOperand(0);
    Instruction* shiftOrMul = dyn_cast<BinaryOperator>(otherOpnd);

    if (!shiftOrMul ||
        (shiftOrMul->getOpcode() != Instruction::Shl && shiftOrMul->getOpcode() != Instruction::Mul))
    {
        return;
    }
    bool isShift = shiftOrMul->getOpcode() == Instruction::Shl;
    ConstantInt* constVal = dyn_cast<ConstantInt>(shiftOrMul->getOperand(1));
    if (!constVal || !constVal->equalsInt(isShift ? 16 : 65536))
    {
        return;
    }
    BitCastInst* bitcast2 = dyn_cast<BitCastInst>(shiftOrMul->getOperand(0));
    if (!bitcast2)
    {
        return;
    }
    llvm::GenIntrinsicInst* valueHi = dyn_cast<GenIntrinsicInst>(bitcast2->getOperand(0));
    if (!valueHi || valueHi->getIntrinsicID() != GenISA_f32tof16_rtz)
    {
        return;
    }

    Value* loVal = nullptr;
    Value* hiVal = nullptr;

    FPExtInst* extInstLo = dyn_cast<FPExtInst>(inst->getOperand(0));
    FPExtInst* extInstHi = dyn_cast<FPExtInst>(valueHi->getOperand(0));

    IRBuilder<> builder(lastValue);
    Type* funcType[] = { Type::getHalfTy(builder.getContext()), Type::getFloatTy(builder.getContext()) };
    Type* halfx2 = VectorType::get(Type::getHalfTy(builder.getContext()), 2);

    if (extInstLo && extInstHi &&
        extInstLo->getOperand(0)->getType()->isHalfTy() &&
        extInstHi->getOperand(0)->getType()->isHalfTy())
    {
        loVal = extInstLo->getOperand(0);
        hiVal = extInstHi->getOperand(0);
    }
    else
    {
        Function* f32tof16_rtz = GenISAIntrinsic::getDeclaration(inst->getParent()->getParent()->getParent(),
            GenISAIntrinsic::GenISA_ftof_rtz, funcType);
        loVal = builder.CreateCall(f32tof16_rtz, inst->getArgOperand(0));
        hiVal = builder.CreateCall(f32tof16_rtz, valueHi->getArgOperand(0));
    }
    Value* vector = builder.CreateInsertElement(UndefValue::get(halfx2), loVal, builder.getInt32(0));
    vector = builder.CreateInsertElement(vector, hiVal, builder.getInt32(1));
    vector = builder.CreateBitCast(vector, lastValue->getType());
    lastValue->replaceAllUsesWith(vector);
    lastValue->eraseFromParent();
}

void CustomSafeOptPass::visitBfi(llvm::CallInst* inst)
{
    assert(inst->getType()->isIntegerTy(32));
    ConstantInt* widthV = dyn_cast<ConstantInt>(inst->getOperand(0));
    ConstantInt* offsetV = dyn_cast<ConstantInt>(inst->getOperand(1));
    if(widthV && offsetV)
    {
        // transformation is beneficial if src3 is constant or if the offset is zero
        if(isa<ConstantInt>(inst->getOperand(3)) || offsetV->isZero())
        {
            unsigned int width = static_cast<unsigned int>(widthV->getZExtValue());
            unsigned int offset = static_cast<unsigned int>(offsetV->getZExtValue());
            unsigned int bitMask = ((1 << width) - 1) << offset;
            IRBuilder<> builder(inst);
            // dst = ((src2 << offset) & bitmask) | (src3 & ~bitmask)
            Value* firstTerm = builder.CreateShl(inst->getOperand(2), offsetV);
            firstTerm = builder.CreateAnd(firstTerm, builder.getInt32(bitMask));
            Value* secondTerm = builder.CreateAnd(inst->getOperand(3), builder.getInt32(~bitMask));
            Value* dst = builder.CreateOr(firstTerm, secondTerm);
            inst->replaceAllUsesWith(dst);
            inst->eraseFromParent();
        }
    }
}

void CustomSafeOptPass::visitMulH(CallInst* inst, bool isSigned)
{
    ConstantInt* src0 = dyn_cast<ConstantInt>(inst->getOperand(0));
    ConstantInt* src1 = dyn_cast<ConstantInt>(inst->getOperand(1));
    if (src0 && src1)
    {
        unsigned nbits = inst->getType()->getIntegerBitWidth();
        assert(nbits < 64);

        if (isSigned)
        {
            uint64_t ui0 = src0->getZExtValue();
            uint64_t ui1 = src1->getZExtValue();
            uint64_t r = ((ui0 * ui1) >> nbits);
            inst->replaceAllUsesWith(ConstantInt::get(inst->getType(), r));
        }
        else
        {
            int64_t si0 = src0->getSExtValue();
            int64_t si1 = src1->getSExtValue();
            int64_t r = ((si0 * si1) >> nbits);
            inst->replaceAllUsesWith(ConstantInt::get(inst->getType(), r, true));
        }
        inst->eraseFromParent();
    }
}

// if phi is used in a FPTrunc and the sources all come from fpext we can skip the conversions
void CustomSafeOptPass::visitFPTruncInst(FPTruncInst &I)
{
    if(PHINode* phi = dyn_cast<PHINode>(I.getOperand(0)))
    {
        bool foundPattern = true;
        unsigned int numSrc = phi->getNumIncomingValues();
        SmallVector<Value*, 6> newSources(numSrc);
        for(unsigned int i = 0; i < numSrc; i++)
        {
            FPExtInst* source = dyn_cast<FPExtInst>(phi->getIncomingValue(i));
            if(source && source->getOperand(0)->getType() == I.getType())
            {
                newSources[i] = source->getOperand(0);
            }
            else
            {
                foundPattern = false;
                break;
            }
        }
        if(foundPattern)
        {
            PHINode* newPhi = PHINode::Create(I.getType(), numSrc, "", phi);
            for(unsigned int i = 0; i < numSrc; i++)
            {
                newPhi->addIncoming(newSources[i], phi->getIncomingBlock(i));
            }
            
            I.replaceAllUsesWith(newPhi);
            I.eraseFromParent();
            // if phi has other uses we add a fpext to avoid having two phi
            if(!phi->use_empty())
            {
                IRBuilder<> builder(&(*phi->getParent()->getFirstInsertionPt()));
                Value* extV = builder.CreateFPExt(newPhi, phi->getType());
                phi->replaceAllUsesWith(extV);
            }
        }
    }
    
}

void CustomSafeOptPass::visitFPToUIInst(llvm::FPToUIInst& FPUII)
{
    if (llvm::IntrinsicInst* intrinsicInst = llvm::dyn_cast<llvm::IntrinsicInst>(FPUII.getOperand(0)))
    {
        if (intrinsicInst->getIntrinsicID() == Intrinsic::floor)
        {
            FPUII.setOperand(0, intrinsicInst->getOperand(0));
            if (intrinsicInst->use_empty())
            {
                intrinsicInst->eraseFromParent();
            }
        }
    }
}

/// This remove simplify bitcast across phi and select instruction
/// LLVM doesn't catch those case and it is common in DX10+ as the input language is not typed
/// TODO: support cases where some sources are constant
void CustomSafeOptPass::visitBitCast(BitCastInst &BC)
{
    if(SelectInst* sel = dyn_cast<SelectInst>(BC.getOperand(0)))
    {
        BitCastInst* trueVal = dyn_cast<BitCastInst>(sel->getTrueValue());
        BitCastInst* falseVal = dyn_cast<BitCastInst>(sel->getFalseValue());
        if(trueVal && falseVal)
        {
            Value* trueValOrignalType = trueVal->getOperand(0);
            Value* falseValOrignalType = falseVal->getOperand(0);
            if(trueValOrignalType->getType() == BC.getType() &&
                falseValOrignalType->getType() == BC.getType())
            {
                Value* cond = sel->getCondition();
                Value* newVal = SelectInst::Create(cond, trueValOrignalType, falseValOrignalType, "", sel);
                BC.replaceAllUsesWith(newVal);
                BC.eraseFromParent();
            }
        }
    }
    else if(PHINode* phi = dyn_cast<PHINode>(BC.getOperand(0)))
    {
        if(phi->hasOneUse())
        {
            bool foundPattern = true;
            unsigned int numSrc = phi->getNumIncomingValues();
            SmallVector<Value*, 6> newSources(numSrc);
            for(unsigned int i = 0; i < numSrc; i++)
            {
                BitCastInst* source = dyn_cast<BitCastInst>(phi->getIncomingValue(i));
                if(source && source->getOperand(0)->getType() == BC.getType())
                {
                    newSources[i] = source->getOperand(0);
                }
                else if(Constant* C = dyn_cast<Constant>(phi->getIncomingValue(i)))
                {
                    newSources[i] = ConstantExpr::getCast(Instruction::BitCast, C, BC.getType());
                }
                else
                {
                    foundPattern = false;
                    break;
                }
            }
            if(foundPattern)
            {
                PHINode* newPhi = PHINode::Create(BC.getType(), numSrc, "", phi);
                for(unsigned int i = 0; i < numSrc; i++)
                {
                    newPhi->addIncoming(newSources[i], phi->getIncomingBlock(i));
                }
                BC.replaceAllUsesWith(newPhi);
                BC.eraseFromParent();
            }
        }
    }
}

bool CustomSafeOptPass::isEmulatedAdd(BinaryOperator &I)
{
    if (I.getOpcode() == Instruction::Or)
    {
        if (BinaryOperator *OrOp0 = dyn_cast<BinaryOperator>(I.getOperand(0)))
        {
            if (OrOp0->getOpcode() == Instruction::Shl)
            {
                // Check the SHl. If we have a constant Shift Left val then we can check
                // it to see if it is emulating an add.
                if (ConstantInt *pConstShiftLeft = dyn_cast<ConstantInt>(OrOp0->getOperand(1)))
                {
                    if (ConstantInt *pConstOrVal = dyn_cast<ConstantInt>(I.getOperand(1)))
                    {
                        int const_shift = int_cast<int>(pConstShiftLeft->getZExtValue());
                        int const_or_val = int_cast<int>(pConstOrVal->getZExtValue());
                        if ((1 << const_shift) > const_or_val)
                        {
                            // The value fits in the shl. So this is an emulated add.
                            return true;
                        }
                    }
                }
            }
            else if (OrOp0->getOpcode() == Instruction::Mul)
            {
                // Check to see if the Or is emulating and add.
                // If we have a constant Mul and a constant Or. The Mul constant needs to be divisible by the rounded up 2^n of Or value.
                if (ConstantInt *pConstMul = dyn_cast<ConstantInt>(OrOp0->getOperand(1)))
                {
                    if (ConstantInt *pConstOrVal = dyn_cast<ConstantInt>(I.getOperand(1)))
                    {
                        if (pConstOrVal->isNegative() == false)
                        {
                            DWORD const_or_val = int_cast<DWORD>(pConstOrVal->getZExtValue());
                            DWORD nextPowerOfTwo = iSTD::RoundPower2(const_or_val + 1);
                            if (nextPowerOfTwo && (pConstMul->getZExtValue() % nextPowerOfTwo == 0))
                            {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

void CustomSafeOptPass::visitBinaryOperator(BinaryOperator &I)
{
    // move immediate value in consecutive integer adds to the last added value.
    // this can allow more chance of doing CSE and memopt.
    //    a = b + 8
    //    d = a + c
    //        to
    //    a = b + c
    //    d = a + 8
    
    CodeGenContext* pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    // WA for remaining bug in custom pass
    if(pContext->m_DriverInfo.WADisableCustomPass())
        return;
    if (I.getType()->isIntegerTy())
    {
        if ((I.getOpcode() == Instruction::Add || isEmulatedAdd(I)) &&
            I.hasOneUse())
        {
            ConstantInt *src0imm = dyn_cast<ConstantInt>(I.getOperand(0));
            ConstantInt *src1imm = dyn_cast<ConstantInt>(I.getOperand(1));
            if (src0imm || src1imm)
            {
                llvm::Instruction* nextInst = llvm::dyn_cast<llvm::Instruction>(*(I.user_begin()));
                if (nextInst && nextInst->getOpcode() == Instruction::Add)
                {
                    ConstantInt *secondSrc0imm = dyn_cast<ConstantInt>(nextInst->getOperand(0));
                    ConstantInt *secondSrc1imm = dyn_cast<ConstantInt>(nextInst->getOperand(1));
                    // found 2 add instructions to swap srcs
                    if (!secondSrc0imm && !secondSrc1imm && nextInst->getOperand(0) != nextInst->getOperand(1))
                    {
                        for (int i = 0; i < 2; i++)
                        {
                            if (nextInst->getOperand(i) == &I)
                            {
                                Value * newAdd = BinaryOperator::CreateAdd(src0imm ? I.getOperand(1) : I.getOperand(0), nextInst->getOperand(1 - i), "", nextInst);
                                nextInst->setOperand(0, newAdd);
                                nextInst->setOperand(1, I.getOperand(src0imm ? 0 : 1));
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void IGC::CustomSafeOptPass::visitLdptr(llvm::CallInst* inst)
{
    if (!IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllTextures) &&
        !IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllTypedBuffers) &&
        !IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllNonTemporalTextures))
    {
        return;
    }

    // change
    // % 10 = call fast <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p196608v4f32(i32 %_s1.i, i32 %_s14.i, i32 0, i32 0, <4 x float> addrspace(196608)* null, i32 0, i32 0, i32 0), !dbg !123
    // to
    // % 10 = call fast <4 x float> @llvm.genx.GenISA.typedread.p196608v4f32(<4 x float> addrspace(196608)* null, i32 %_s1.i, i32 %_s14.i, i32 0, i32 0), !dbg !123
    // when the index comes directly from threadid

    Constant *src1 = dyn_cast<Constant>(inst->getOperand(1));
    Constant *src2 = dyn_cast<Constant>(inst->getOperand(2));
    Constant *src3 = dyn_cast<Constant>(inst->getOperand(3));

    // src2 and src3 has to be zero
    if (!src2 || !src3 || !src2->isZeroValue() || !src3->isZeroValue())
    {
        return;
    }

    // if only doing the opt on buffers, make sure src1 is zero too
    if (!IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllTextures) &&
        IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllTypedBuffers) &&
        !IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllNonTemporalTextures))
    {
        if (!src1 || !src1->isZeroValue())
            return;
    }

    // for UseHDCTypedReadForAllNonTemporalTextures, check if the ld uses threadid for index
    if (IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllNonTemporalTextures))
    {
        Instruction* AddInst0 = dyn_cast<Instruction>(inst->getOperand(0));
        Instruction* AddInst1 = dyn_cast<Instruction>(inst->getOperand(1));
        if (!AddInst0 || !AddInst1 ||
            AddInst0->getOpcode() != Instruction::Add ||
            AddInst1->getOpcode() != Instruction::Add)
        {
            return;
        }

        Instruction* ShlInst0 = dyn_cast<Instruction>(AddInst0->getOperand(0));
        Instruction* ShlInst1 = dyn_cast<Instruction>(AddInst1->getOperand(0));
        if (!ShlInst0 || !ShlInst1 ||
            ShlInst0->getOpcode() != Instruction::Shl ||
            ShlInst1->getOpcode() != Instruction::Shl)
        {
            return;
        }

        BitCastInst* bitcastInst0 = dyn_cast<BitCastInst>(ShlInst0->getOperand(0));
        BitCastInst* bitcastInst1 = dyn_cast<BitCastInst>(ShlInst1->getOperand(0));
        if (!bitcastInst0 || !bitcastInst1)
        {
            return;
        }

        GenIntrinsicInst *CI0 = dyn_cast<GenIntrinsicInst>(bitcastInst0->getOperand(0));
        GenIntrinsicInst *CI1 = dyn_cast<GenIntrinsicInst>(bitcastInst1->getOperand(0));
        if (!CI0 || !CI1 ||
            CI0->getIntrinsicID() != GenISAIntrinsic::GenISA_DCL_SystemValue ||
            CI1->getIntrinsicID() != GenISAIntrinsic::GenISA_DCL_SystemValue)
        {
            return;
        }
    }

    // do the transformation
    llvm::IRBuilder<> builder(inst);
    Module *M = inst->getParent()->getParent()->getParent();

    Function* pLdIntrinsic = llvm::GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_typedread,
        inst->getOperand(4)->getType());

    SmallVector<Value*, 5> ld_FunctionArgList(5);
    ld_FunctionArgList[0] = inst->getOperand(4);
    ld_FunctionArgList[1] = builder.CreateAdd(inst->getOperand(0), inst->getOperand(5));
    ld_FunctionArgList[2] = builder.CreateAdd(inst->getOperand(1), inst->getOperand(6));
    ld_FunctionArgList[3] = builder.CreateAdd(inst->getOperand(3), inst->getOperand(7));
    ld_FunctionArgList[4] = inst->getOperand(2);  // lod=zero

    llvm::CallInst* pNewCallInst = builder.CreateCall(
        pLdIntrinsic, ld_FunctionArgList);

    inst->replaceAllUsesWith(pNewCallInst);
}

void IGC::CustomSafeOptPass::visitSampleBptr(llvm::SampleIntrinsic* sampleInst)
{
    // sampleB with bias_value==0 -> sample
    llvm::ConstantFP* constBias = llvm::dyn_cast<llvm::ConstantFP>(sampleInst->getOperand(0));
    if(constBias && constBias->isZero())
    {
        // Copy args skipping bias operand:
        llvm::SmallVector<llvm::Value*, 10> args;
        for(unsigned int i = 1; i < sampleInst->getNumArgOperands(); i++)
        {
            args.push_back(sampleInst->getArgOperand(i));
        }

        // Copy overloaded types unchanged:
        llvm::SmallVector<llvm::Type*, 4> overloadedTys;
        overloadedTys.push_back(sampleInst->getCalledFunction()->getReturnType());
        overloadedTys.push_back(sampleInst->getOperand(0)->getType());
        overloadedTys.push_back(sampleInst->getTextureValue()->getType());
        overloadedTys.push_back(sampleInst->getSamplerValue()->getType());

        llvm::Function* sampleIntr = llvm::GenISAIntrinsic::getDeclaration(
            sampleInst->getParent()->getParent()->getParent(),
            GenISAIntrinsic::GenISA_sampleptr,
            overloadedTys);

        llvm::Value *newSample = llvm::CallInst::Create(sampleIntr, args, "", sampleInst);
        sampleInst->replaceAllUsesWith(newSample);
    }
}

void CustomSafeOptPass::visitExtractElementInst(ExtractElementInst &I)
{
    // convert:
    // %1 = lshr i32 %0, 16,
    // %2 = bitcast i32 %1 to <2 x half>
    // %3 = extractelement <2 x half> %2, i32 0
    // to ->
    // %2 = bitcast i32 %0 to <2 x half>
    // %3 = extractelement <2 x half> %2, i32 1
    BitCastInst* bitCast = dyn_cast<BitCastInst>(I.getVectorOperand());
    ConstantInt* cstIndex = dyn_cast<ConstantInt>(I.getIndexOperand());
    if(bitCast && cstIndex)
    {
        // skip intermediate bitcast
        while(isa<BitCastInst>(bitCast->getOperand(0)))
        {
            bitCast = cast<BitCastInst>(bitCast->getOperand(0));
        }
        if(BinaryOperator* binOp = dyn_cast<BinaryOperator>(bitCast->getOperand(0)))
        {
            unsigned int bitShift = 0;
            bool rightShift = false;
            if(binOp->getOpcode() == Instruction::LShr)
            {
                if(ConstantInt* cstShift = dyn_cast<ConstantInt>(binOp->getOperand(1)))
                {
                    bitShift = (unsigned int)cstShift->getZExtValue();
                    rightShift = true;
                }
            }
            else if(binOp->getOpcode() == Instruction::Shl)
            {
                if(ConstantInt* cstShift = dyn_cast<ConstantInt>(binOp->getOperand(1)))
                {
                    bitShift = (unsigned int)cstShift->getZExtValue();
                }
            }
            if(bitShift != 0)
            {
                Type* vecType = I.getVectorOperand()->getType();
                unsigned int eltSize = vecType->getVectorElementType()->getPrimitiveSizeInBits();
                if(bitShift % eltSize == 0)
                {
                    int elOffset = (int)(bitShift / eltSize);
                    elOffset = rightShift ? elOffset : -elOffset;
                    unsigned int newIndex = (unsigned int)((int)cstIndex->getZExtValue() + elOffset);
                    if(newIndex < vecType->getVectorNumElements())
                    {
                        IRBuilder<> builder(&I);
                        Value* newBitCast = builder.CreateBitCast(binOp->getOperand(0), vecType);
                        Value* newScalar = builder.CreateExtractElement(newBitCast, newIndex);
                        I.replaceAllUsesWith(newScalar);
                        I.eraseFromParent();
                    }
                }
            }
        }
    }
}

// Register pass to igc-opt
#define PASS_FLAG2 "igc-gen-specific-pattern"
#define PASS_DESCRIPTION2 "LastPatternMatch Pass"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(GenSpecificPattern, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_END(GenSpecificPattern, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

char GenSpecificPattern::ID = 0;

GenSpecificPattern::GenSpecificPattern() : FunctionPass(ID)
{
    initializeGenSpecificPatternPass(*PassRegistry::getPassRegistry());
}

bool GenSpecificPattern::runOnFunction(Function &F)
{
    visit(F);
    return true;
}

// Lower SDiv to better code sequence if possible
void GenSpecificPattern::visitSDiv(llvm::BinaryOperator& I)
{
    if(ConstantInt* divisor = dyn_cast<ConstantInt>(I.getOperand(1)))
    {
        // signed division of power of 2 can be transformed to asr
        // For negative values we need to make sure we round correctly
        int log2Div = divisor->getValue().exactLogBase2();
        if(log2Div > 0)
        {
            unsigned int intWidth = divisor->getBitWidth();
            IRBuilder<> builder(&I);
            Value * signedBitOnly = I.getOperand(0);
            if(log2Div > 1)
            {
                signedBitOnly = builder.CreateAShr(signedBitOnly, builder.getIntN(intWidth, intWidth - 1));
            }
            Value* offset = builder.CreateLShr(signedBitOnly, builder.getIntN(intWidth, intWidth - log2Div));
            Value* offsetedValue = builder.CreateAdd(I.getOperand(0), offset);
            Value* newValue = builder.CreateAShr(offsetedValue, builder.getIntN(intWidth, log2Div));
            I.replaceAllUsesWith(newValue);
            I.eraseFromParent();
        }
    }
}

/*
Optimizes bit reversing pattern:

%and = shl i32 %0, 1
%shl = and i32 %and, 0xAAAAAAAA
%and2 = lshr i32 %0, 1
%shr = and i32 %and2, 0x55555555
%or = or i32 %shl, %shr
%and3 = shl i32 %or, 2
%shl4 = and i32 %and3, 0xCCCCCCCC
%and5 = lshr i32 %or, 2
%shr6 = and i32 %and5, 0x33333333
%or7 = or i32 %shl4, %shr6
%and8 = shl i32 %or7, 4
%shl9 = and i32 %and8, 0xF0F0F0F0
%and10 = lshr i32 %or7, 4
%shr11 = and i32 %and10, 0x0F0F0F0F
%or12 = or i32 %shl9, %shr11
%and13 = shl i32 %or12, 8
%shl14 = and i32 %and13, 0xFF00FF00
%and15 = lshr i32 %or12, 8
%shr16 = and i32 %and15, 0x00FF00FF
%or17 = or i32 %shl14, %shr16
%shl19 = shl i32 %or17, 16
%shr21 = lshr i32 %or17, 16
%or22 = or i32 %shl19, %shr21

into:

%or22 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %0)

And similarly for patterns reversing 16 and 64 bit type values.
*/
template <typename MaskType>
void GenSpecificPattern::matchReverse(BinaryOperator &I)
{
    using namespace llvm::PatternMatch;
    assert(I.getType()->isIntegerTy());
    Value *nextOrShl = nullptr, *nextOrShr = nullptr;
    uint64_t currentShiftShl = 0, currentShiftShr = 0;
    uint64_t currentMaskShl = 0, currentMaskShr = 0;
    auto patternBfrevFirst =
        m_Or(
            m_Shl(m_Value(nextOrShl), m_ConstantInt(currentShiftShl)),
            m_LShr(m_Value(nextOrShr), m_ConstantInt(currentShiftShr)));

    auto patternBfrev =
        m_Or(
            m_And(
                m_Shl(m_Value(nextOrShl), m_ConstantInt(currentShiftShl)),
                m_ConstantInt(currentMaskShl)),
            m_And(
                m_LShr(m_Value(nextOrShr), m_ConstantInt(currentShiftShr)),
                m_ConstantInt(currentMaskShr)));

    unsigned int bitWidth = std::numeric_limits<MaskType>::digits;
    assert(bitWidth == 16 || bitWidth == 32 || bitWidth == 64);

    unsigned int currentShift = bitWidth / 2;
    // First mask is a value with all upper half bits present.
    MaskType mask = std::numeric_limits<MaskType>::max() << currentShift;

    bool isBfrevMatchFound = false;
    nextOrShl = &I;
    if (match(nextOrShl, patternBfrevFirst) &&
        nextOrShl == nextOrShr &&
        currentShiftShl == currentShift &&
        currentShiftShr == currentShift)
    {
        // NextOrShl is assigned to next one by match().
        currentShift /= 2;
        // Constructing next mask to match.
        mask ^= mask >> currentShift;
    }

    while (currentShift > 0)
    {
        if (match(nextOrShl, patternBfrev) &&
            nextOrShl == nextOrShr &&
            currentShiftShl == currentShift &&
            currentShiftShr == currentShift &&
            currentMaskShl == mask &&
            currentMaskShr == (MaskType)~mask)
        {
            // NextOrShl is assigned to next one by match().
            if (currentShift == 1)
            {
                isBfrevMatchFound = true;
                break;
            }

            currentShift /= 2;
            // Constructing next mask to match.
            mask ^= mask >> currentShift;
        }
        else
        {
            break;
        }
    }

    if (isBfrevMatchFound)
    {
        llvm::IRBuilder<> builder(&I);
        Function *bfrevFunc = GenISAIntrinsic::getDeclaration(
            I.getParent()->getParent()->getParent(), GenISAIntrinsic::GenISA_bfrev, builder.getInt32Ty());
        if (bitWidth == 16)
        {
            Value* zext = builder.CreateZExt(nextOrShl, builder.getInt32Ty());
            Value* bfrev = builder.CreateCall(bfrevFunc, zext);
            Value* lshr = builder.CreateLShr(bfrev, 16);
            Value* trunc = builder.CreateTrunc(lshr, I.getType());
            I.replaceAllUsesWith(trunc);
        }
        else if (bitWidth == 32)
        {
            Value* bfrev = builder.CreateCall(bfrevFunc, nextOrShl);
            I.replaceAllUsesWith(bfrev);
        }
        else
        { // bitWidth == 64
            Value* int32Source = builder.CreateBitCast(nextOrShl, llvm::VectorType::get(builder.getInt32Ty(), 2));
            Value* extractElement0 = builder.CreateExtractElement(int32Source, builder.getInt32(0));
            Value* extractElement1 = builder.CreateExtractElement(int32Source, builder.getInt32(1));
            Value* bfrevLow = builder.CreateCall(bfrevFunc, extractElement0);
            Value* bfrevHigh = builder.CreateCall(bfrevFunc, extractElement1);
            Value* bfrev64Result = llvm::UndefValue::get(int32Source->getType());
            bfrev64Result = builder.CreateInsertElement(bfrev64Result, bfrevHigh, builder.getInt32(0));
            bfrev64Result = builder.CreateInsertElement(bfrev64Result, bfrevLow, builder.getInt32(1));
            Value* bfrevBitcast = builder.CreateBitCast(bfrev64Result, I.getType());
            I.replaceAllUsesWith(bfrevBitcast);
        }
    }
}

void GenSpecificPattern::visitBinaryOperator(BinaryOperator &I)
{
    if (I.getOpcode() == Instruction::Or)
    {
        using namespace llvm::PatternMatch;

        if (I.getType()->isIntegerTy())
        {
            unsigned int bitWidth = cast<IntegerType>(I.getType())->getBitWidth();
            switch (bitWidth)
            {
            case 16:
                matchReverse<unsigned short>(I);
                break;
            case 32:
                matchReverse<unsigned int>(I);
                break;
            case 64:
                matchReverse<unsigned long long>(I);
                break;
            }
        }

        /*
        llvm changes ADD to OR when possible, and this optimization changes it back and allow 2 ADDs to merge.
        This can avoid scattered read for constant buffer when the index is calculated by shl + or + add.

        ex:
        from
        %22 = shl i32 %14, 2
        %23 = or i32 %22, 3
        %24 = add i32 %23, 16
        to
        %22 = shl i32 %14, 2
        %23 = add i32 %22, 19
        */
        Value *AndOp1 = nullptr, *EltOp1 = nullptr;
        auto pattern1 = m_Or(
            m_And(m_Value(AndOp1), m_SpecificInt(0xFFFFFFFF)),
            m_Shl(m_Value(EltOp1), m_SpecificInt(32)));

    #if LLVM_VERSION_MAJOR >= 7
        Value *AndOp2 = nullptr, *EltOp2 = nullptr, *VecOp = nullptr;
        auto pattern2 = m_Or(
            m_And(m_Value(AndOp2), m_SpecificInt(0xFFFFFFFF)),
            m_BitCast(m_InsertElement(m_Value(VecOp),m_Value(EltOp2),m_SpecificInt(1))));
    #endif // LLVM_VERSION_MAJOR >= 7
        // Transforms pattern1 or pattern2 to a bitcast,extract,insert,insert,bitcast
        auto transformPattern = [=](BinaryOperator &I, Value* Op1, Value* Op2)
        {
            llvm::IRBuilder<> builder(&I);
            VectorType* vec2 = VectorType::get(builder.getInt32Ty(), 2);
            Value* vec_undef = UndefValue::get(vec2);
            Value* BC = builder.CreateBitCast(Op1, vec2);
            Value* EE = builder.CreateExtractElement(BC, builder.getInt32(0));
            Value* vec = builder.CreateInsertElement(vec_undef, EE, builder.getInt32(0));
            vec = builder.CreateInsertElement(vec, Op2, builder.getInt32(1));
            vec = builder.CreateBitCast(vec, builder.getInt64Ty());
            I.replaceAllUsesWith(vec);
            I.eraseFromParent();
        };

        if (match(&I, pattern1) && AndOp1->getType()->isIntegerTy(64))
        {
            transformPattern(I, AndOp1, EltOp1);
        }
    #if LLVM_VERSION_MAJOR >= 7
        else if (match(&I, pattern2) && AndOp2->getType()->isIntegerTy(64))
        {
            ConstantVector* cVec = dyn_cast<ConstantVector>(VecOp);
            VectorType * vector_type = dyn_cast<VectorType>(VecOp->getType());
            if (cVec && vector_type &&
                isa<ConstantInt>(cVec->getOperand(0)) &&
                cast<ConstantInt>(cVec->getOperand(0))->isZero() &&
                vector_type->getElementType()->isIntegerTy(32) &&
                vector_type->getNumElements() == 2)
            {
                transformPattern(I, AndOp2, EltOp2);
            }
        }
    #endif // LLVM_VERSION_MAJOR >= 7
        else
        {

            /*
            from
                % 22 = shl i32 % 14, 2
                % 23 = or i32 % 22, 3
            to
                % 22 = shl i32 % 14, 2
                % 23 = add i32 % 22, 3
            */
            ConstantInt *OrConstant = dyn_cast<ConstantInt>(I.getOperand(1));
            if (OrConstant)
            {
                llvm::Instruction* ShlInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(0));
                if (ShlInst && ShlInst->getOpcode() == Instruction::Shl)
                {
                    ConstantInt *ShlConstant = dyn_cast<ConstantInt>(ShlInst->getOperand(1));
                    if (ShlConstant)
                    {
                        // if the constant bit width is larger than 64, we cannot store ShlIntValue and OrIntValue rawdata as uint64_t.
                        // will need a fix then
                        assert(ShlConstant->getBitWidth() <= 64);
                        assert(OrConstant->getBitWidth() <= 64);

                        uint64_t ShlIntValue = *(ShlConstant->getValue()).getRawData();
                        uint64_t OrIntValue = *(OrConstant->getValue()).getRawData();

                        if (OrIntValue < pow(2, ShlIntValue))
                        {
                            Value * newAdd = BinaryOperator::CreateAdd(I.getOperand(0), I.getOperand(1), "", &I);
                            I.replaceAllUsesWith(newAdd);
                        }
                    }
                }
            }
        }
    }
    else if (I.getOpcode() == Instruction::Add)
    {
        /*
        from
            %23 = add i32 %22, 3
            %24 = add i32 %23, 16
        to
            %24 = add i32 %22, 19
        */
        for (int ImmSrcId1 = 0; ImmSrcId1 < 2; ImmSrcId1++)
        {
            ConstantInt *IConstant = dyn_cast<ConstantInt>(I.getOperand(ImmSrcId1));
            if (IConstant)
            {
                llvm::Instruction* AddInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(1 - ImmSrcId1));
                if (AddInst && AddInst->getOpcode() == Instruction::Add)
                {
                    for (int ImmSrcId2 = 0; ImmSrcId2 < 2; ImmSrcId2++)
                    {
                        ConstantInt *AddConstant = dyn_cast<ConstantInt>(AddInst->getOperand(ImmSrcId2));
                        if (AddConstant)
                        {
                            llvm::APInt CombineAddValue = AddConstant->getValue() + IConstant->getValue();
                            I.setOperand(0, AddInst->getOperand(1 - ImmSrcId2));
                            I.setOperand(1, ConstantInt::get(I.getType(), CombineAddValue));
                        }
                    }
                }
            }
        }
    }
    else if (I.getOpcode() == Instruction::Shl)
    {
        auto op0 = dyn_cast<ZExtInst>(I.getOperand(0));
        auto offset = llvm::dyn_cast<llvm::ConstantInt>(I.getOperand(1));
        if (op0 &&
            op0->getType()->isIntegerTy(64) &&
            op0->getOperand(0)->getType()->isIntegerTy(32) &&
            offset &&
            offset->getZExtValue() == 32)
        {
            llvm::IRBuilder<> builder(&I);
            auto vec2 = VectorType::get(builder.getInt32Ty(), 2);
            Value* vec = UndefValue::get(vec2);
            vec = builder.CreateInsertElement(vec, builder.getInt32(0), builder.getInt32(0));
            vec = builder.CreateInsertElement(vec, op0->getOperand(0), builder.getInt32(1));
            vec = builder.CreateBitCast(vec, builder.getInt64Ty());
            I.replaceAllUsesWith(vec);
            I.eraseFromParent();
        }
    }
}

void GenSpecificPattern::visitCmpInst(CmpInst &I)
{
    using namespace llvm::PatternMatch;
    CmpInst::Predicate Pred = CmpInst::Predicate::BAD_ICMP_PREDICATE;
    Value* Val1 = nullptr;
    uint64_t const_int1 = 0, const_int2 = 0;
    auto cmp_pattern = m_Cmp(Pred,
        m_And(m_Value(Val1), m_ConstantInt(const_int1)), m_ConstantInt(const_int2));

    if (match(&I, cmp_pattern) &&
        (const_int1 << 32) == 0 &&
        (const_int2 << 32) == 0 &&
        Val1->getType()->isIntegerTy(64))
    {
        llvm::IRBuilder<> builder(&I);
        VectorType* vec2 = VectorType::get(builder.getInt32Ty(), 2);
        Value* BC = builder.CreateBitCast(Val1, vec2);
        Value* EE = builder.CreateExtractElement(BC, builder.getInt32(1));
        Value* AI = builder.CreateAnd(EE, builder.getInt32(const_int1 >> 32));
        Value* new_Val = builder.CreateICmp(Pred, AI, builder.getInt32(const_int2 >> 32));
        I.replaceAllUsesWith(new_Val);
        I.eraseFromParent();
    }
    else 
    {
        CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        if (pCtx->m_DriverInfo.IgnoreNan())
        {
            if (I.getPredicate() == CmpInst::FCMP_ORD)
            {
                I.replaceAllUsesWith(ConstantInt::getTrue(I.getType()));
            }
        }
    }
}

void GenSpecificPattern::visitSelectInst(SelectInst &I)
{
    /*
    from
        %7 = fcmp olt float %5, %6
        %8 = select i1 %7, float 1.000000e+00, float 0.000000e+00
        %9 = bitcast float %8 to i32
        %10 = icmp eq i32 %9, 0
        %.01 = select i1 %10, float %4, float %11
        br i1 %10, label %endif, label %then
    to
        %7 = fcmp olt float %5, %6
        %.01 = select i1 %7, float %11, float %4
        br i1 %7, label %then, label %endif
    */
    {
        bool swapNodesFromSel = false;
        bool isSelWithConstants = false;
        ConstantFP *Cfp1 = dyn_cast<ConstantFP>(I.getOperand(1));
        ConstantFP *Cfp2 = dyn_cast<ConstantFP>(I.getOperand(2));
        if (Cfp1 && Cfp1->getValueAPF().isFiniteNonZero() &&
            Cfp2 && Cfp2->isZero())
        {
            isSelWithConstants = true;
        }
        if (Cfp1 && Cfp1->isZero() &&
            Cfp2 && Cfp2->getValueAPF().isFiniteNonZero())
        {
            isSelWithConstants = true;
            swapNodesFromSel = true;
        }
        if (isSelWithConstants &&
            dyn_cast<FCmpInst>(I.getOperand(0)))
        {
            for (auto bitCastI : I.users())
            {
                if (BitCastInst* bitcastInst = dyn_cast<BitCastInst>(bitCastI))
                {
                    for (auto cmpI : bitcastInst->users())
                    {
                        ICmpInst* iCmpInst = dyn_cast<ICmpInst>(cmpI);
                        if (iCmpInst &&
                            iCmpInst->isEquality())
                        {
                            ConstantInt *icmpC = dyn_cast<ConstantInt>(iCmpInst->getOperand(1));
                            if (!icmpC || !icmpC->isZero())
                            {
                                continue;
                            }

                            bool swapNodes = swapNodesFromSel;
                            if (iCmpInst->getPredicate() == CmpInst::Predicate::ICMP_EQ)
                            {
                                swapNodes = (swapNodes != true);
                            }

                            SmallVector<Instruction*, 4> matchedBrSelInsts;
                            for (auto brOrSelI : iCmpInst->users())
                            {
                                BranchInst* brInst = dyn_cast<BranchInst>(brOrSelI);
                                if (brInst &&
                                    brInst->isConditional())
                                {
                                    //match
                                    matchedBrSelInsts.push_back(brInst);
                                    if (swapNodes)
                                    {
                                        brInst->swapSuccessors();
                                    }
                                }

                                if (SelectInst* selInst = dyn_cast<SelectInst>(brOrSelI))
                                {
                                    //match
                                    matchedBrSelInsts.push_back(selInst);
                                    if (swapNodes)
                                    {
                                        Value* selTrue = selInst->getTrueValue();
                                        Value* selFalse = selInst->getFalseValue();
                                        selInst->setTrueValue(selFalse);
                                        selInst->setFalseValue(selTrue);
                                        selInst->swapProfMetadata();
                                    }
                                }
                            }
                            for (Instruction* inst : matchedBrSelInsts)
                            {
                                inst->setOperand(0, I.getOperand(0));
                            }
                        }
                    }
                }
            }
        }
    }
    /*
    from
        %res_s42 = icmp eq i32 %src1_s41, 0
        %src1_s81 = select i1 %res_s42, i32 15, i32 0
    to
        %res_s42 = icmp eq i32 %src1_s41, 0
        %17 = sext i1 %res_s42 to i32
        %18 = and i32 15, %17

               or

    from
        %res_s73 = fcmp oge float %res_s61, %42
        %res_s187 = select i1 %res_s73, float 1.000000e+00, float 0.000000e+00
    to
        %res_s73 = fcmp oge float %res_s61, %42
        %46 = sext i1 %res_s73 to i32
        %47 = and i32 %46, 1065353216
        %48 = bitcast i32 %47 to float
    */

    assert( I.getOpcode() == Instruction::Select );

    bool skipOpt = false;

    ConstantInt *Cint = dyn_cast<ConstantInt>( I.getOperand(2) );
    if( Cint && Cint->isZero() )
    {
        llvm::Instruction* cmpInst = llvm::dyn_cast<llvm::Instruction>( I.getOperand(0) );
        if( cmpInst &&
            cmpInst->getOpcode() == Instruction::ICmp &&
            I.getOperand(1) != cmpInst->getOperand(0) )
        {
            // disable the cases for csel or where we can optimize the instructions to such as add.ge.* later in vISA
            ConstantInt *C = dyn_cast<ConstantInt>( cmpInst->getOperand(1) );
            if( C && C->isZero() )
            {
                skipOpt = true;
            }

            if( !skipOpt )
            {
                // temporary disable the case where cmp is used in multiple sel, and not all of them have src2=0
                // we should remove this if we can allow both flag and grf dst for the cmp to be used.
                for(auto selI = cmpInst->user_begin(), E = cmpInst->user_end(); selI!=E; ++selI)
                {
                    if(llvm::SelectInst* selInst = llvm::dyn_cast<llvm::SelectInst>(*selI))
                    {
                        ConstantInt *C = dyn_cast<ConstantInt>( selInst->getOperand(2) );
                        if( !(C && C->isZero()) )
                        {
                            skipOpt = true;
                            break;
                        }
                    }
                }
            }

            if( !skipOpt )
            {
                Value * newValueSext = CastInst::CreateSExtOrBitCast( I.getOperand(0), I.getType(), "", &I );
                Value * newValueAnd = BinaryOperator::CreateAnd( I.getOperand(1), newValueSext, "", &I );
                I.replaceAllUsesWith( newValueAnd );
            }
        }
    }
    else
    {
        ConstantFP *Cfp = dyn_cast<ConstantFP>( I.getOperand(2) );
        if( Cfp && Cfp->isZero() )
        {
            llvm::Instruction* cmpInst = llvm::dyn_cast<llvm::Instruction>( I.getOperand(0) );
            if( cmpInst &&
                cmpInst->getOpcode() == Instruction::FCmp &&
                I.getOperand(1) != cmpInst->getOperand(0) )
            {
                // disable the cases for csel or where we can optimize the instructions to such as add.ge.* later in vISA
                ConstantFP *C = dyn_cast<ConstantFP>( cmpInst->getOperand(1) );
                if( C && C->isZero() )
                {
                    skipOpt = true;
                }

                if( !skipOpt )
                {
                    for(auto selI = cmpInst->user_begin(), E = cmpInst->user_end(); selI!=E; ++selI)
                    {
                        if(llvm::SelectInst* selInst = llvm::dyn_cast<llvm::SelectInst>(*selI))
                        {
                            // temporary disable the case where cmp is used in multiple sel, and not all of them have src2=0
                            // we should remove this if we can allow both flag and grf dst for the cmp to be used.
                            ConstantFP *C2 = dyn_cast<ConstantFP>( selInst->getOperand(2) );
                            if( !(C2 && C2->isZero()) )
                            {
                                skipOpt = true;
                                break;
                            }

                            // if it is cmp-sel(1.0 / 0.0)-mul, we could better patten match it later in codeGen.
                            ConstantFP *C1 = dyn_cast<ConstantFP>(selInst->getOperand(1));
                            if (C1 && C2 && selInst->hasOneUse())
                            {
                                if ((C2->isZero() && C1->isExactlyValue(1.f)) || (C1->isZero() && C2->isExactlyValue(1.f)))
                                {
                                    Instruction *mulInst = dyn_cast<Instruction>(*selInst->user_begin());
                                    if (mulInst && mulInst->getOpcode() == Instruction::FMul)
                                    {
                                        skipOpt = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                if( !skipOpt )
                {
                    ConstantFP *C1 = dyn_cast<ConstantFP>( I.getOperand(1) );
                    if (C1)
                    {
                        if (C1->getType()->isHalfTy())
                        {
                            Value * newValueSext = CastInst::CreateSExtOrBitCast(I.getOperand(0), Type::getInt16Ty(I.getContext()), "", &I);
                            Value * newConstant = ConstantInt::get(I.getContext(), C1->getValueAPF().bitcastToAPInt());
                            Value * newValueAnd = BinaryOperator::CreateAnd(newValueSext, newConstant, "", &I);
                            Value * newValueCastFP = CastInst::CreateZExtOrBitCast(newValueAnd, Type::getHalfTy(I.getContext()), "", &I);
                            I.replaceAllUsesWith(newValueCastFP);
                        }
                        else if (C1->getType()->isFloatTy())
                        {
                            Value * newValueSext = CastInst::CreateSExtOrBitCast(I.getOperand(0), Type::getInt32Ty(I.getContext()), "", &I);
                            Value * newConstant = ConstantInt::get(I.getContext(), C1->getValueAPF().bitcastToAPInt());
                            Value * newValueAnd = BinaryOperator::CreateAnd(newValueSext, newConstant, "", &I);
                            Value * newValueCastFP = CastInst::CreateZExtOrBitCast(newValueAnd, Type::getFloatTy(I.getContext()), "", &I);
                            I.replaceAllUsesWith(newValueCastFP);
                        }
                    }
                    else
                    {
                        if (I.getOperand(1)->getType()->isHalfTy())
                        {
                            Value * newValueSext = CastInst::CreateSExtOrBitCast(I.getOperand(0), Type::getInt16Ty(I.getContext()), "", &I);
                            Value * newValueBitcast = CastInst::CreateZExtOrBitCast(I.getOperand(1), Type::getInt16Ty(I.getContext()), "", &I);
                            Value * newValueAnd = BinaryOperator::CreateAnd(newValueSext, newValueBitcast, "", &I);
                            Value * newValueCastFP = CastInst::CreateZExtOrBitCast(newValueAnd, Type::getHalfTy(I.getContext()), "", &I); \
                            I.replaceAllUsesWith(newValueCastFP);
                        }
                        else if (I.getOperand(1)->getType()->isFloatTy())
                        {
                            Value * newValueSext = CastInst::CreateSExtOrBitCast(I.getOperand(0), Type::getInt32Ty(I.getContext()), "", &I);
                            Value * newValueBitcast = CastInst::CreateZExtOrBitCast(I.getOperand(1), Type::getInt32Ty(I.getContext()), "", &I);
                            Value * newValueAnd = BinaryOperator::CreateAnd(newValueSext, newValueBitcast, "", &I);
                            Value * newValueCastFP = CastInst::CreateZExtOrBitCast(newValueAnd, Type::getFloatTy(I.getContext()), "", &I); \
                            I.replaceAllUsesWith(newValueCastFP);
                        }
                    }
                }
            }
        }
    }

    /*
    from
        %230 = sdiv i32 %214, %scale
        %276 = trunc i32 %230 to i8
        %277 = icmp slt i32 %230, 255
        %278 = select i1 %277, i8 %276, i8 -1
    to
        %230 = sdiv i32 %214, %scale
        %277 = icmp slt i32 %230, 255
        %278 = select i1 %277, i32 %230, i32 255
        %279 = trunc i32 %278 to i8

        This tranform allows for min/max instructions to be
        picked up in the IsMinOrMax instruction in PatternMatchPass.cpp
    */
    if (auto *compInst = dyn_cast<ICmpInst>(I.getOperand(0)))
    {
        auto selOp1 = I.getOperand(1);
        auto selOp2 = I.getOperand(2);
        auto cmpOp0 = compInst->getOperand(0);
        auto cmpOp1 = compInst->getOperand(1);
        auto trunc1 = dyn_cast<TruncInst>(selOp1);
        auto trunc2 = dyn_cast<TruncInst>(selOp2);
        auto icmpType = compInst->getOperand(0)->getType();

        if (selOp1->getType()->isIntegerTy() &&
            icmpType->isIntegerTy() &&
            selOp1->getType()->getIntegerBitWidth() < icmpType->getIntegerBitWidth())
        {
            Value * newSelOp1 = NULL;
            Value * newSelOp2 = NULL;
            if (trunc1 &&
                (trunc1->getOperand(0) == cmpOp0 ||
                 trunc1->getOperand(0) == cmpOp1))
            {
                newSelOp1 = (trunc1->getOperand(0) == cmpOp0) ? cmpOp0 : cmpOp1;
            }

            if (trunc2 &&
                (trunc2->getOperand(0) == cmpOp0 ||
                 trunc2->getOperand(0) == cmpOp1))
            {
                newSelOp2 = (trunc2->getOperand(0) == cmpOp0) ? cmpOp0 : cmpOp1;
            }

            if (isa<llvm::ConstantInt>(selOp1) &&
                isa<llvm::ConstantInt>(cmpOp0) &&
                (cast<llvm::ConstantInt>(selOp1)->getZExtValue() ==
                 cast<llvm::ConstantInt>(cmpOp0)->getZExtValue()))
            {
                assert(newSelOp1 == NULL);
                newSelOp1 = cmpOp0;
            }

            if (isa<llvm::ConstantInt>(selOp1) &&
                isa<llvm::ConstantInt>(cmpOp1) &&
                (cast<llvm::ConstantInt>(selOp1)->getZExtValue() ==
                 cast<llvm::ConstantInt>(cmpOp1)->getZExtValue()))
            {
                assert(newSelOp1 == NULL);
                newSelOp1 = cmpOp1;
            }

            if (isa<llvm::ConstantInt>(selOp2) &&
                isa<llvm::ConstantInt>(cmpOp0) &&
                (cast<llvm::ConstantInt>(selOp2)->getZExtValue() ==
                 cast<llvm::ConstantInt>(cmpOp0)->getZExtValue()))
            {
                assert(newSelOp2 == NULL);
                newSelOp2 = cmpOp0;
            }

            if (isa<llvm::ConstantInt>(selOp2) &&
                isa<llvm::ConstantInt>(cmpOp1) &&
                (cast<llvm::ConstantInt>(selOp2)->getZExtValue() ==
                 cast<llvm::ConstantInt>(cmpOp1)->getZExtValue()))
            {
                assert(newSelOp2 == NULL);
                newSelOp2 = cmpOp1;
            }

            if (newSelOp1 && newSelOp2)
            {
                auto newSelInst = SelectInst::Create(I.getCondition(), newSelOp1, newSelOp2, "", &I);
                auto newTruncInst = TruncInst::CreateTruncOrBitCast(newSelInst, selOp1->getType(), "", &I);
                I.replaceAllUsesWith(newTruncInst);
                I.eraseFromParent();
            }
        }

    }

}

void GenSpecificPattern::visitCastInst(CastInst &I)
{
    Instruction* srcVal = nullptr;
    if(isa<SIToFPInst>(&I))
    {
        srcVal = dyn_cast<FPToSIInst>(I.getOperand(0));
    }
    if(srcVal && srcVal->getOperand(0)->getType() == I.getType())
    {
        if((srcVal = dyn_cast<Instruction>(srcVal->getOperand(0))))
        {
            // need fast math to know that we can ignore Nan
            if(srcVal->isFast())
            {
                IRBuilder<> builder(&I);
                Function* func = Intrinsic::getDeclaration(
                    I.getParent()->getParent()->getParent(),
                    Intrinsic::trunc,
                    I.getType());
                Value* newVal = builder.CreateCall(func, srcVal);
                I.replaceAllUsesWith(newVal);
                I.eraseFromParent();
            }
        }
    }
}

void GenSpecificPattern::visitZExtInst(ZExtInst &ZEI)
{
    CmpInst *Cmp = dyn_cast<CmpInst>(ZEI.getOperand(0));
    if (!Cmp)
        return;

    IRBuilder<> Builder(&ZEI);

    Value *S = Builder.CreateSExt(Cmp, ZEI.getType());
    Value *N = Builder.CreateNeg(S);
    ZEI.replaceAllUsesWith(N);
    ZEI.eraseFromParent();
}

void GenSpecificPattern::visitIntToPtr(llvm::IntToPtrInst& I)
{
    if(ZExtInst* zext = dyn_cast<ZExtInst>(I.getOperand(0)))
    {
        IRBuilder<> builder(&I);
        Value* newV = builder.CreateIntToPtr(zext->getOperand(0), I.getType());
        I.replaceAllUsesWith(newV);
        I.eraseFromParent();
    }
}

void GenSpecificPattern::visitTruncInst(llvm::TruncInst &I)
{
    /*
    from
    %22 = lshr i64 %a, 52
    %23 = trunc i64  %22 to i32
    to
    %22 = extractelement <2 x i32> %a, 1
    %23 = lshr i32 %22, 20 //52-32
    */

    using namespace llvm::PatternMatch;
    Value *LHS = nullptr;
    ConstantInt *CI;
    if (match(&I, m_Trunc(m_LShr(m_Value(LHS), m_ConstantInt(CI)))) &&
        I.getType()->isIntegerTy(32) &&
        LHS->getType()->isIntegerTy(64) &&
        CI->getZExtValue() >= 32)
    {
        auto new_shift_size = (unsigned)CI->getZExtValue() - 32;
        llvm::IRBuilder<> builder(&I);
        VectorType* vec2 = VectorType::get(builder.getInt32Ty(), 2);
        Value* new_Val = builder.CreateBitCast(LHS, vec2);
        new_Val = builder.CreateExtractElement(new_Val, builder.getInt32(1));
        if (new_shift_size > 0)
        {
            new_Val = builder.CreateLShr(new_Val, builder.getInt32(new_shift_size));
        }
        I.replaceAllUsesWith(new_Val);
        I.eraseFromParent();
    }
}

// Register pass to igc-opt
#define PASS_FLAG3 "igc-const-prop"
#define PASS_DESCRIPTION3 "Custom Const-prop Pass"
#define PASS_CFG_ONLY3 false
#define PASS_ANALYSIS3 false
IGC_INITIALIZE_PASS_BEGIN(IGCConstProp, PASS_FLAG3, PASS_DESCRIPTION3, PASS_CFG_ONLY3, PASS_ANALYSIS3)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(IGCConstProp, PASS_FLAG3, PASS_DESCRIPTION3, PASS_CFG_ONLY3, PASS_ANALYSIS3)

char IGCConstProp::ID = 0;

IGCConstProp::IGCConstProp(bool enableMathConstProp,
                           bool enableSimplifyGEP) :
    FunctionPass(ID),
    m_enableMathConstProp(enableMathConstProp),
    m_enableSimplifyGEP(enableSimplifyGEP),
    m_TD(nullptr), m_TLI(nullptr)
{
    initializeIGCConstPropPass(*PassRegistry::getPassRegistry());
}

static Constant* GetConstantValue(Type* type, char* rawData)
{
    unsigned int size_in_bytes = type->getPrimitiveSizeInBits() / 8;
    uint64_t returnConstant = 0;
    memcpy_s(&returnConstant, size_in_bytes, rawData, size_in_bytes);
    if(type->isIntegerTy())
    {
        return ConstantInt::get(type, returnConstant);
    }
    else if(type->isFloatingPointTy())
    {
        return  ConstantFP::get(type->getContext(),
            APFloat(type->getFltSemantics(), APInt(type->getPrimitiveSizeInBits(), returnConstant)));
    }
    return nullptr;
}

bool IGCConstProp::EvalConstantAddress(Value* address, unsigned int &offset, Value* ptrSrc)
{
    if((ptrSrc == nullptr && isa<ConstantPointerNull>(address)) ||
       (ptrSrc == address))
    {
        offset = 0;
        return true;
    }
    else if (Instruction* ptrExpr = dyn_cast<Instruction>(address))
    {
        if(ptrExpr->getOpcode() == Instruction::BitCast ||
           ptrExpr->getOpcode() == Instruction::AddrSpaceCast)
        {
            return EvalConstantAddress(ptrExpr->getOperand(0), offset, ptrSrc);
        }
        if(ptrExpr->getOpcode() == Instruction::IntToPtr)
        {
            Value *eltIdxVal = ptrExpr->getOperand(0);
            ConstantInt *eltIdx = dyn_cast<ConstantInt>(eltIdxVal);
            if(!eltIdx)
                return false;
            offset = int_cast<unsigned>(eltIdx->getZExtValue());
            return true;
        }
        else if(ptrExpr->getOpcode() == Instruction::GetElementPtr)
        {
            offset = 0;
            if(!EvalConstantAddress(ptrExpr->getOperand(0), offset, ptrSrc))
            {
                return false;
            }
            Type *Ty = ptrExpr->getType();
            gep_type_iterator GTI = gep_type_begin(ptrExpr);
            for(auto OI = ptrExpr->op_begin() + 1, E = ptrExpr->op_end(); OI != E; ++OI, ++GTI) {
                Value *Idx = *OI;
                if(StructType *StTy = GTI.getStructTypeOrNull()) {
                    unsigned Field = int_cast<unsigned>(cast<ConstantInt>(Idx)->getZExtValue());
                    if(Field) {
                        offset += int_cast<unsigned int>(m_TD->getStructLayout(StTy)->getElementOffset(Field));
                    }
                    Ty = StTy->getElementType(Field);
                }
                else {
                    Ty = GTI.getIndexedType();
                    if(const ConstantInt *CI = dyn_cast<ConstantInt>(Idx)) {
                        offset += int_cast<unsigned int>(
                            m_TD->getTypeAllocSize(Ty) * CI->getSExtValue());
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool GetStatelessBufferInfo(Value* pointer, unsigned &bufferId,
    BufferType &bufferTy, Value* &bufferSrcPtr)
{
    // If the buffer info is not encoded in the address space, we can still find it by
    // tracing the pointer to where it's created.
    Value* src = IGC::TracePointerSource(pointer);
    BufferAccessType accType;
    if(src && IGC::GetResourcePointerInfo(src, bufferId, bufferTy, accType))
    {
        bufferSrcPtr = src;
        return true;
    }
    return false;
}

Constant* IGCConstProp::ReplaceFromDynConstants(unsigned bufId, unsigned eltId, unsigned int size_in_bytes, Type* type)
{
    ConstantAddress cl;
    char * pConstVal;
    cl.bufId = bufId;
    cl.eltId = eltId;
    cl.size = size_in_bytes;

    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData *modMD = ctx->getModuleMetaData();

    auto it = modMD->inlineDynConstants.find(cl);
    if ((it != modMD->inlineDynConstants.end()) && (it->first.size == cl.size))
    {
        // For constant buffer accesses of size <= 32bit.
        if(size_in_bytes <= 4)
        {
            // This constant is
            //          found in the Dynamic inline constants list, and
            //          sizes match (find only looking for bufId and eltId, so need to look for size explicitly)
            if (!(type->isVectorTy()))
            {
                // Handling for base types (Integer/FloatingPoint)
                pConstVal = (char *) (&(it->second));
                return GetConstantValue(type, pConstVal);
            }
        }
    }
    return nullptr;
}

Constant *IGCConstProp::replaceShaderConstant(LoadInst *inst)
{
    unsigned as = inst->getPointerAddressSpace();
    bool directBuf = false;
    unsigned bufId = 0;
    unsigned int size_in_bytes = 0;
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData *modMD = ctx->getModuleMetaData();

    BufferType bufType;
    Value* pointerSrc = nullptr;

    if (as == ADDRESS_SPACE_CONSTANT)
    {
        if (!GetStatelessBufferInfo(inst->getPointerOperand(), bufId, bufType, pointerSrc))
        {
            return nullptr;
        }
        directBuf = true;
    }
    else
    {
        bufType = IGC::DecodeAS4GFXResource(as, directBuf, bufId);
    }
    
    if(bufType == CONSTANT_BUFFER && 
        directBuf && modMD) 
    {
        Value *ptrVal = inst->getPointerOperand();
        unsigned eltId = 0;
        size_in_bytes = inst->getType()->getPrimitiveSizeInBits() / 8;
        if(!EvalConstantAddress(ptrVal, eltId, pointerSrc))
        {
            return nullptr;
        }

        if(size_in_bytes)
        {
            if (modMD->immConstant.data.size() &&
                (bufId == modMD->pushInfo.inlineConstantBufferSlot))
            {
                char *offset = &(modMD->immConstant.data[0]);
                if (inst->getType()->isVectorTy())
                {
                    Type *srcEltTy = inst->getType()->getVectorElementType();
                    uint32_t srcNElts = inst->getType()->getVectorNumElements();
                    uint32_t eltSize_in_bytes = srcEltTy->getPrimitiveSizeInBits() / 8;
                    IRBuilder<> builder(inst);
                    Value *vectorValue = UndefValue::get(inst->getType());
                    for (uint i = 0; i < srcNElts; i++)
                    {
                        vectorValue = builder.CreateInsertElement(
                            vectorValue,
                            GetConstantValue(srcEltTy, offset + eltId + (i*eltSize_in_bytes)),
                            builder.getInt32(i));
                    }
                    return dyn_cast<Constant>(vectorValue);
                }
                else
                {
                    return GetConstantValue(inst->getType(), offset + eltId);
                }
            }
            else if ((!IGC_IS_FLAG_ENABLED(DisableDynamicConstantFolding)) && (modMD->inlineDynConstants.size() > 0))
            {
                return ReplaceFromDynConstants(bufId, eltId, size_in_bytes, inst->getType());
            }
        }
    }
    return nullptr;
}

Constant *IGCConstProp::ConstantFoldCallInstruction(CallInst *inst)
{
    Constant *C = nullptr;
    if (inst)
    {
        llvm::Type * type = inst->getType();
        // used for GenISA_sqrt and GenISA_rsq
        ConstantFP *C0 = dyn_cast<ConstantFP>(inst->getOperand(0));
        EOPCODE igcop = GetOpCode(inst);

        // special case of gen-intrinsic
        switch (igcop)
        {
        case llvm_sqrt:
            if (C0)
            {
                auto APF = C0->getValueAPF();
                double C0value = type->isFloatTy() ? APF.convertToFloat() :
                    APF.convertToDouble();
                if (C0value > 0.0)
                {
                    C = ConstantFP::get(type, sqrt(C0value));
                }
            }
            break;
        case llvm_rsq:
            if (C0)
            {
                auto APF = C0->getValueAPF();
                double C0value = type->isFloatTy() ? APF.convertToFloat() :
                    APF.convertToDouble();
                if (C0value > 0.0)
                {
                    C = ConstantFP::get(type, 1. / sqrt(C0value));
                }
            }
            break;
        case llvm_max:
        {
            ConstantFP *CFP0 = dyn_cast<ConstantFP>(inst->getOperand(0));
            ConstantFP *CFP1 = dyn_cast<ConstantFP>(inst->getOperand(1));
            if (CFP0 && CFP1)
            {
                const APFloat &A = CFP0->getValueAPF();
                const APFloat &B = CFP1->getValueAPF();
                C = ConstantFP::get(inst->getContext(), maxnum(A, B));
            }
        }
        break;
        case llvm_min:
        {
            ConstantFP *CFP0 = dyn_cast<ConstantFP>(inst->getOperand(0));
            ConstantFP *CFP1 = dyn_cast<ConstantFP>(inst->getOperand(1));
            if (CFP0 && CFP1)
            {
                const APFloat &A = CFP0->getValueAPF();
                const APFloat &B = CFP1->getValueAPF();
                C = ConstantFP::get(inst->getContext(), minnum(A, B));
            }
        }
        break;
        case llvm_fsat:
        {
            ConstantFP *CFP0 = dyn_cast<ConstantFP>(inst->getOperand(0));
            if (CFP0)
            {
                const APFloat &A = CFP0->getValueAPF();
                const APFloat &zero = cast<ConstantFP>(ConstantFP::get(type, 0.))->getValueAPF();
                const APFloat &One = cast<ConstantFP>(ConstantFP::get(type, 1.))->getValueAPF();
                C = ConstantFP::get(inst->getContext(), minnum(One, maxnum(zero, A)));
            }
        }
        default:
            break;
        }

        if (m_enableMathConstProp && type->isFloatTy())
        {
            float C0value = 0;
            float C1value = 0;
            ConstantFP *C0 = dyn_cast<ConstantFP>(inst->getOperand(0));
            ConstantFP *C1 = nullptr;
            if (C0)
            {
                C0value = C0->getValueAPF().convertToFloat();

                switch (igcop)
                {
                case llvm_cos:
                    C = ConstantFP::get(type, cosf(C0value));
                    break;
                case llvm_sin:
                    C = ConstantFP::get(type, sinf(C0value));
                    break;
                case llvm_log:
                    // skip floating-point exception and keep the original instructions
                    if (C0value > 0.0f)
                    {
                        C = ConstantFP::get(type, log10f(C0value) / log10f(2.0f));
                    }
                    break;
                case llvm_exp:
                    C = ConstantFP::get(type, powf(2.0f, C0value));
                    break;
                case llvm_pow:
                    C1 = dyn_cast<ConstantFP>(inst->getOperand(1));
                    if (C1)
                    {
                        C1value = C1->getValueAPF().convertToFloat();
                        C = ConstantFP::get(type, powf(C0value, C1value));
                    }
                    break;
                case llvm_sqrt:
                    // Don't handle negative values
                    if (C0value > 0.0f)
                    {
                        C = ConstantFP::get(type, sqrtf(C0value));
                    }
                    break;
                case llvm_floor:
                    C = ConstantFP::get(type, floorf(C0value));
                    break;
                case llvm_ceil:
                    C = ConstantFP::get(type, ceilf(C0value));
                    break;
                default:
                    break;
                }
            }
        }
    }
    return C;
}

// constant fold the following code for any index:
//
// %95 = extractelement <4 x i16> <i16 3, i16 16, i16 21, i16 39>, i32 %94
// %96 = icmp eq i16 %95, 0
//
Constant *IGCConstProp::ConstantFoldCmpInst(CmpInst *CI)
{
    // Only handle scalar result.
    if (CI->getType()->isVectorTy())
        return nullptr;

    Value *LHS = CI->getOperand(0);
    Value *RHS = CI->getOperand(1);
    if (!isa<Constant>(RHS) && CI->isCommutative())
        std::swap(LHS, RHS);
    if (!isa<ConstantInt>(RHS) && !isa<ConstantFP>(RHS))
        return nullptr;

    auto EEI = dyn_cast<ExtractElementInst>(LHS);
    if (EEI && isa<Constant>(EEI->getVectorOperand()))
    {
        bool AllTrue = true, AllFalse = true;
        auto VecOpnd = cast<Constant>(EEI->getVectorOperand());
        unsigned N = VecOpnd->getType()->getVectorNumElements();
        for (unsigned i = 0; i < N; ++i)
        {
            Constant *Opnd = VecOpnd->getAggregateElement(i);
            assert(Opnd && "null entry");
            if (isa<UndefValue>(Opnd))
                continue;
            Constant *Result = ConstantFoldCompareInstOperands(
                CI->getPredicate(), Opnd, cast<Constant>(RHS), CI->getFunction()->getParent()->getDataLayout());
            if (!Result->isAllOnesValue())
                AllTrue = false;
            if (!Result->isNullValue())
                AllFalse = false;
        }

        if (AllTrue)
        {
            return ConstantInt::getTrue(CI->getType());
        }
        else if (AllFalse)
        {
            return ConstantInt::getFalse(CI->getType());
        }
    }

    return nullptr;
}

// constant fold the following code for any index:
//
// %93 = insertelement  <4 x float> <float 1.0, float 1.0, float 1.0, float 1.0>, float %v7.w_, i32 0
// %95 = extractelement <4 x float> %93, i32 1
//
// constant fold the selection of the same value in a vector component, e.g.:
// %Temp - 119.i.i.v.v = select i1 %Temp - 118.i.i, <2 x i32> <i32 0, i32 17>, <2 x i32> <i32 4, i32 17>
// %scalar9 = extractelement <2 x i32> %Temp - 119.i.i.v.v, i32 1
//
Constant *IGCConstProp::ConstantFoldExtractElement(ExtractElementInst *EEI)
{

    Constant *EltIdx = dyn_cast<Constant>(EEI->getIndexOperand());
    if (EltIdx)
    {
        if (InsertElementInst *IEI = dyn_cast<InsertElementInst>(EEI->getVectorOperand()))
    {
        Constant *InsertIdx = dyn_cast<Constant>(IEI->getOperand(2));
            // try to find the constant from a chain of InsertElement
        while(IEI && InsertIdx)
        {
            if (InsertIdx == EltIdx)
            {
                Constant *EltVal = dyn_cast<Constant>(IEI->getOperand(1));
                return EltVal;
            }
            else
            {
                Value *Vec = IEI->getOperand(0);
                if (isa<ConstantDataVector>(Vec))
                {
                    ConstantDataVector *CVec = cast<ConstantDataVector>(Vec);
                    return CVec->getAggregateElement(EltIdx);
                }
                else if (isa<InsertElementInst>(Vec))
                {
                    IEI = cast<InsertElementInst>(Vec);
                    InsertIdx = dyn_cast<Constant>(IEI->getOperand(2));
                }
                else
                {
                    break;
                }
            }
        }
    }
        else if (SelectInst *sel = dyn_cast<SelectInst>(EEI->getVectorOperand()))
        {
            Value *vec0 = sel->getOperand(1);
            Value *vec1 = sel->getOperand(2);

            assert(vec0->getType() == vec1->getType());

            if (isa<ConstantDataVector>(vec0) && isa<ConstantDataVector>(vec1))
            {
                ConstantDataVector *cvec0 = cast<ConstantDataVector>(vec0);
                ConstantDataVector *cvec1 = cast<ConstantDataVector>(vec1);
                Constant* cval0 = cvec0->getAggregateElement(EltIdx);
                Constant* cval1 = cvec1->getAggregateElement(EltIdx);
                
                if (cval0 == cval1)
                {
                    return cval0;
                }
            }
        }
    }
    return nullptr;
}

//  simplifyAdd() push any constants to the top of a sequence of Add instructions,
//  which makes CSE/GVN to do a better job.  For example,
//      (((A + 8) + B) + C) + 15
//  will become
//      ((A + B) + C) + 23
//  Note that the order of non-constant values remain unchanged throughout this
//  transformation.
//  (This was added to remove redundant loads. If the
//  the future LLVM does better job on this (reassociation), we should use LLVM's
//  instead.)
bool IGCConstProp::simplifyAdd(BinaryOperator *BO)
{
    // Only handle Add
    if (BO->getOpcode() != Instruction::Add)
    {
        return false;
    }

    Value *LHS = BO->getOperand(0);
    Value *RHS = BO->getOperand(1);
    bool changed = false;
    if (BinaryOperator *LBO = dyn_cast<BinaryOperator>(LHS))
    {
        if (simplifyAdd(LBO))
        {
            changed = true;
        }
    }
    if (BinaryOperator *RBO = dyn_cast<BinaryOperator>(RHS))
    {
        if (simplifyAdd(RBO))
        {
            changed = true;
        }
    }

    // Refresh LHS and RHS
    LHS = BO->getOperand(0);
    RHS = BO->getOperand(1);
    BinaryOperator *LHSbo = dyn_cast<BinaryOperator>(LHS);
    BinaryOperator *RHSbo = dyn_cast<BinaryOperator>(RHS);
    bool isLHSAdd = LHSbo && LHSbo->getOpcode() == Instruction::Add;
    bool isRHSAdd = RHSbo && RHSbo->getOpcode() == Instruction::Add;
    IRBuilder<> Builder(BO);
    if (isLHSAdd && isRHSAdd)
    {
        Value *A = LHSbo->getOperand(0);
        Value *B = LHSbo->getOperand(1);
        Value *C = RHSbo->getOperand(0);
        Value *D = RHSbo->getOperand(1);

        ConstantInt *C0 = dyn_cast<ConstantInt>(B);
        ConstantInt *C1 = dyn_cast<ConstantInt>(D);

        if (C0 || C1)
        {
            Value *R = nullptr;
            if (C0 && C1)
            {
                Value *newC = ConstantFoldBinaryOpOperands(Instruction::Add,
                    C0, C1, *m_TD);
                R = Builder.CreateAdd(A, C);
                R = Builder.CreateAdd(R, newC);
            }
            else if (C0)
            {
                R = Builder.CreateAdd(A, RHS);
                R = Builder.CreateAdd(R, B);
            }
            else
            {   // C1 is not nullptr
                R = Builder.CreateAdd(LHS, C);
                R = Builder.CreateAdd(R, D);
            }
            BO->replaceAllUsesWith(R);
            return true;
        }
    }
    else if (isLHSAdd)
    {
        Value *A = LHSbo->getOperand(0);
        Value *B = LHSbo->getOperand(1);
        Value *C = RHS;

        ConstantInt *C0 = dyn_cast<ConstantInt>(B);
        ConstantInt *C1 = dyn_cast<ConstantInt>(C);

        if (C0 && C1)
        {
            Value *newC = ConstantFoldBinaryOpOperands(Instruction::Add,
                C0, C1, *m_TD);
            Value *R = Builder.CreateAdd(A, newC);
            BO->replaceAllUsesWith(R);
            return true;
        }
        if (C0)
        {
            Value *R = Builder.CreateAdd(A, C);
            R = Builder.CreateAdd(R, B);
            BO->replaceAllUsesWith(R);
            return true;
        }
    }
    else if (isRHSAdd)
    {
        Value *A = LHS;
        Value *B = RHSbo->getOperand(0);
        Value *C = RHSbo->getOperand(1);

        ConstantInt *C0 = dyn_cast<ConstantInt>(A);
        ConstantInt *C1 = dyn_cast<ConstantInt>(C);

        if (C0 && C1)
        {
            Constant *Ops[] = { C0, C1 };
            Value *newC = ConstantFoldBinaryOpOperands(Instruction::Add,
                C0, C1, *m_TD);
            Value *R = Builder.CreateAdd(B, newC);
            BO->replaceAllUsesWith(R);
            return true;
        }
        if (C0)
        {
            Value *R = Builder.CreateAdd(RHS, A);
            BO->replaceAllUsesWith(R);
            return true;
        }
        if (C1)
        {
            Value *R = Builder.CreateAdd(A, B);
            R = Builder.CreateAdd(R, C);
            BO->replaceAllUsesWith(R);
            return true;
        }
    }
    else
    {
        if (ConstantInt *CLHS = dyn_cast<ConstantInt>(LHS))
        {
            if (ConstantInt *CRHS = dyn_cast<ConstantInt>(RHS))
            {
                Constant *Ops[] = { CLHS, CRHS };
                Value *newC = ConstantFoldBinaryOpOperands(Instruction::Add,
                    CLHS, CRHS, *m_TD);
                BO->replaceAllUsesWith(newC);
                return true;
            }

            // Constant is kept as RHS
            Value *R = Builder.CreateAdd(RHS, LHS);
            BO->replaceAllUsesWith(R);
            return true;
        }
    }
    return changed;
}

bool IGCConstProp::simplifyGEP(GetElementPtrInst *GEP)
{
    bool changed = false;
    for (int i = 0; i < (int)GEP->getNumIndices(); ++i)
    {
        Value *Index = GEP->getOperand(i + 1);
        BinaryOperator *BO = dyn_cast<BinaryOperator>(Index);
        if (!BO || BO->getOpcode() != Instruction::Add)
        {
            continue;
        }
        if (simplifyAdd(BO))
        {
            changed = true;
        }
    }
    return changed;
}

/**
* the following code is essentially a copy of llvm copy-prop code with one little
* addition for shader-constant replacement.
*
* we don't have to do this if llvm version uses a virtual function in place of calling
* ConstantFoldInstruction.
*/
bool IGCConstProp::runOnFunction(Function &F)
{
    module = F.getParent();
    // Initialize the worklist to all of the instructions ready to process...
    llvm::SetVector<Instruction*> WorkList;
    for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i)
    {
        WorkList.insert(&*i);
    }
    bool Changed = false;
    m_TD = &F.getParent()->getDataLayout();
    m_TLI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
    while (!WorkList.empty())
    {
        Instruction *I = *WorkList.rbegin();
        WorkList.remove(I);    // Get an element from the worklist...
        if (I->use_empty())                  // Don't muck with dead instructions...
        {
            continue;
        }
        Constant *C = nullptr;
        C = ConstantFoldInstruction(I, *m_TD, m_TLI);

        if (!C && isa<CallInst>(I))
        {
            C = ConstantFoldCallInstruction(cast<CallInst>(I));
        }

        // replace shader-constant load with the known value
        if (!C && isa<LoadInst>(I))
        {
            C = replaceShaderConstant(cast<LoadInst>(I));
        }
        if (!C && isa<CmpInst>(I))
        {
            C = ConstantFoldCmpInst(cast<CmpInst>(I));
        }
        if (!C && isa<ExtractElementInst>(I))
        {
            C = ConstantFoldExtractElement(cast<ExtractElementInst>(I));
        }
        if (C)
        {
            // Add all of the users of this instruction to the worklist, they might
            // be constant propagatable now...
            for (Value::user_iterator UI = I->user_begin(), UE = I->user_end();
                UI != UE; ++UI)
            {
                WorkList.insert(cast<Instruction>(*UI));
            }

            // Replace all of the uses of a variable with uses of the constant.
            I->replaceAllUsesWith(C);

            if ( 0 /* isa<ConstantPointerNull>(C)*/) // disable optimization generating invalid IR until it gets re-written
            {
                // if we are changing function calls/ genisa intrinsics, then we need 
                // to fix the function declarations to account for the change in pointer address type
                for (Value::user_iterator UI = C->user_begin(), UE = C->user_end();
                    UI != UE; ++UI)
                {
                    if (GenIntrinsicInst *genIntr = dyn_cast<GenIntrinsicInst>(*UI))
                    {
                                    GenISAIntrinsic::ID ID = genIntr->getIntrinsicID();
                        if (ID == GenISAIntrinsic::GenISA_storerawvector_indexed)
                        {
                            llvm::Type* tys[2];
                            tys[0] = genIntr->getOperand(0)->getType();
                            tys[1] = genIntr->getOperand(2)->getType();
                            GenISAIntrinsic::getDeclaration(F.getParent(),
                                llvm::GenISAIntrinsic::GenISA_storerawvector_indexed,
                                tys);
                        }
                        else if (ID == GenISAIntrinsic::GenISA_storeraw_indexed)
                        {
                            llvm::Type* types[2] = {
                                genIntr->getOperand(0)->getType(),
                                genIntr->getOperand(1)->getType() };

                            GenISAIntrinsic::getDeclaration(F.getParent(),
                                llvm::GenISAIntrinsic::GenISA_storeraw_indexed,
                                types);
                        }
                        else if (ID == GenISAIntrinsic::GenISA_ldrawvector_indexed || ID == GenISAIntrinsic::GenISA_ldraw_indexed)
                        {
                            llvm::Type* tys[2];
                            tys[0] = genIntr->getType();
                            tys[1] = genIntr->getOperand(0)->getType();
                            GenISAIntrinsic::getDeclaration(F.getParent(),
                                ID,
                                tys);
                        }
                    }
                }
            }

            // Remove the dead instruction.
            I->eraseFromParent();

            // We made a change to the function...
            Changed = true;

            // I is erased, continue to the next one.
            continue;
        }

        if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(I))
        {
            if (m_enableSimplifyGEP && simplifyGEP(GEP))
            {
                Changed = true;
            }
        }
    }
    return Changed;
}


namespace {

    class IGCIndirectICBPropagaion : public FunctionPass
    {
    public:
        static char ID;
        IGCIndirectICBPropagaion() : FunctionPass(ID)
        {
            initializeIGCIndirectICBPropagaionPass(*PassRegistry::getPassRegistry());
        }
        virtual llvm::StringRef getPassName() const { return "Indirect ICB Propagaion"; }
        virtual bool runOnFunction(Function &F);
        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
        }
    private:
        bool isICBOffseted(llvm::LoadInst* inst, uint offset);
    };

} // namespace

char IGCIndirectICBPropagaion::ID = 0;
FunctionPass *IGC::createIGCIndirectICBPropagaionPass() { return new IGCIndirectICBPropagaion(); }

bool IGCIndirectICBPropagaion::runOnFunction(Function &F)
{
    CodeGenContext *ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData *modMD = ctx->getModuleMetaData();

    //MaxImmConstantSizePushed = 256 by default. For float values, it will contains 64 numbers, and stored in 8 GRF
    if (modMD && 
        modMD->immConstant.data.size() &&
        modMD->immConstant.data.size() <= IGC_GET_FLAG_VALUE(MaxImmConstantSizePushed))
    {
        uint maxImmConstantSizePushed = modMD->immConstant.data.size();
        char *offset = &(modMD->immConstant.data[0]);
        IRBuilder<> m_builder(F.getContext());

        for (auto &BB : F)
        {
            for (auto BI = BB.begin(), BE = BB.end(); BI != BE;)
            {
                if (llvm::LoadInst* inst = llvm::dyn_cast<llvm::LoadInst>(&(*BI++)))
                {
                    unsigned as = inst->getPointerAddressSpace();
                    bool directBuf;
                    unsigned bufId;
                    BufferType bufType = IGC::DecodeAS4GFXResource(as, directBuf, bufId);
                    bool bICBNoOffset =
                        (IGC::INVALID_CONSTANT_BUFFER_INVALID_ADDR == modMD->pushInfo.inlineConstantBufferOffset && bufType == CONSTANT_BUFFER && directBuf && bufId == modMD->pushInfo.inlineConstantBufferSlot);
                    bool bICBOffseted =
                        (IGC::INVALID_CONSTANT_BUFFER_INVALID_ADDR != modMD->pushInfo.inlineConstantBufferOffset && ADDRESS_SPACE_CONSTANT == as && isICBOffseted(inst, modMD->pushInfo.inlineConstantBufferOffset));
                    if (bICBNoOffset || bICBOffseted)
                    {
                        Value *ptrVal = inst->getPointerOperand();
                        Value *eltPtr = nullptr;
                        Value *eltIdx = nullptr;
                        if (IntToPtrInst *i2p = dyn_cast<IntToPtrInst>(ptrVal))
                        {
                            eltPtr = i2p->getOperand(0);
                        }
                        else if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(ptrVal))
                        {
                            if (gep->getNumOperands() != 3)
                            {
                                continue;
                            }

                            Type* eleType = gep->getPointerOperandType()->getPointerElementType();
                            if (!eleType->isArrayTy() ||
                                !(eleType->getArrayElementType()->isFloatTy() || eleType->getArrayElementType()->isIntegerTy(32)))
                            {
                                continue;
                            }
                            
                            eltIdx = gep->getOperand(2);
                        }
                        else
                        {
                            continue;
                        }

                        m_builder.SetInsertPoint(inst);

                        unsigned int size_in_bytes = inst->getType()->getPrimitiveSizeInBits() / 8;
                        if (size_in_bytes)
                        {
                            Value* ICBbuffer = UndefValue::get(VectorType::get(inst->getType(), maxImmConstantSizePushed / size_in_bytes));
                            if (inst->getType()->isFloatTy())
                            {
                                float returnConstant = 0;
                                for (unsigned int i = 0; i < maxImmConstantSizePushed; i += size_in_bytes)
                                {
                                    memcpy_s(&returnConstant, size_in_bytes, offset + i, size_in_bytes);
                                    Value *fp = ConstantFP::get(inst->getType(), returnConstant);
                                    ICBbuffer = m_builder.CreateInsertElement(ICBbuffer, fp, m_builder.getInt32(i / size_in_bytes));
                                }

                                if (eltPtr)
                                {
                                    eltIdx = m_builder.CreateLShr(eltPtr, m_builder.getInt32(2));
                                }
                                Value *ICBvalue = m_builder.CreateExtractElement(ICBbuffer, eltIdx);
                                inst->replaceAllUsesWith(ICBvalue);
                            }
                            else if (inst->getType()->isIntegerTy(32))
                            {
                                int returnConstant = 0;
                                for (unsigned int i = 0; i < maxImmConstantSizePushed; i += size_in_bytes)
                                {
                                    memcpy_s(&returnConstant, size_in_bytes, offset + i, size_in_bytes);
                                    Value *fp = ConstantInt::get(inst->getType(), returnConstant);
                                    ICBbuffer = m_builder.CreateInsertElement(ICBbuffer, fp, m_builder.getInt32(i / size_in_bytes));
                                }
                                if (eltPtr)
                                {
                                    eltIdx = m_builder.CreateLShr(eltPtr, m_builder.getInt32(2));
                                }
                                Value *ICBvalue = m_builder.CreateExtractElement(ICBbuffer, eltIdx);
                                inst->replaceAllUsesWith(ICBvalue);
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool IGCIndirectICBPropagaion::isICBOffseted(llvm::LoadInst* inst, uint offset) {
    Value *ptrVal = inst->getPointerOperand();
    std::vector<Value*> srcInstList;
    IGC::TracePointerSource(ptrVal, false, true, srcInstList);
    if (srcInstList.size())
    {
        CallInst* inst = dyn_cast<CallInst>(srcInstList.back());
        GenIntrinsicInst* genIntr = inst ? dyn_cast<GenIntrinsicInst>(inst) : nullptr;
        if (!genIntr || (genIntr->getIntrinsicID() != GenISAIntrinsic::GenISA_RuntimeValue))
            return false;

        llvm::ConstantInt* ci = dyn_cast<llvm::ConstantInt>(inst->getOperand(0));
        return ci && (uint)ci->getZExtValue() == offset;
    }

    return false;
}

IGC_INITIALIZE_PASS_BEGIN(IGCIndirectICBPropagaion, "IGCIndirectICBPropagaion",
    "IGCIndirectICBPropagaion", false, false)
IGC_INITIALIZE_PASS_END(IGCIndirectICBPropagaion, "IGCIndirectICBPropagaion",
    "IGCIndirectICBPropagaion", false, false)

namespace {
    class NanHandling : public FunctionPass, public llvm::InstVisitor<NanHandling>
    {
    public:
        static char ID;
        NanHandling() : FunctionPass(ID)
        {
            initializeNanHandlingPass(*PassRegistry::getPassRegistry());
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const
        {
            AU.setPreservesCFG();
            AU.addRequired<LoopInfoWrapperPass>();
        }

        virtual llvm::StringRef getPassName() const { return "NAN handling"; }
        virtual bool runOnFunction(llvm::Function &F);
        void visitBranchInst(llvm::BranchInst &I);
        void loopNanCases(Function &F);

    private:
        int longestPathInstCount(llvm::BasicBlock *BB, int &depth);
        void swapBranch(llvm::Instruction *inst, llvm::BranchInst &BI);
        SmallVector<llvm::BranchInst*, 10> visitedInst;
    };
} // namespace

char NanHandling::ID = 0;
FunctionPass *IGC::createNanHandlingPass() { return new NanHandling(); }

bool NanHandling::runOnFunction(Function &F)
{
    loopNanCases(F);
    visit(F);
    return true;
}

void NanHandling::loopNanCases(Function &F)
{
    // take care of loop cases
    visitedInst.clear();
    llvm::LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    if (LI && !LI->empty())
    {
        FastMathFlags FMF;
        FMF.clear();
        for (LoopInfo::iterator I = LI->begin(), E = LI->end(); I != E; ++I)
        {
            Loop *loop = *I;
            BranchInst* br = cast<BranchInst>(loop->getLoopLatch()->getTerminator());
            BasicBlock* header = loop->getHeader();
            if (br && br->isConditional() && header)
            {
                visitedInst.push_back(br);
                if (FCmpInst *brCmpInst = dyn_cast<FCmpInst>(br->getCondition()))
                {
                    FPMathOperator *FPO = dyn_cast<FPMathOperator>(brCmpInst);
                    if (!FPO || !FPO->isFast())
                    {
                        continue;
                    }
                    if (br->getSuccessor(1) == header)
                    {
                        swapBranch(brCmpInst, *br);
                    }
                }
                else if (BinaryOperator *andOrInst = dyn_cast<BinaryOperator>(br->getCondition()))
                {
                    if (andOrInst->getOpcode() != BinaryOperator::And &&
                        andOrInst->getOpcode() != BinaryOperator::Or)
                    {
                        continue;
                    }
                    FCmpInst *brCmpInst0 = dyn_cast<FCmpInst>(andOrInst->getOperand(0));
                    FCmpInst *brCmpInst1 = dyn_cast<FCmpInst>(andOrInst->getOperand(1));
                    if (!brCmpInst0 || !brCmpInst1)
                    {
                        continue;
                    }
                    if (br->getSuccessor(1) == header)
                    {
                        brCmpInst0->copyFastMathFlags(FMF);
                        brCmpInst1->copyFastMathFlags(FMF);
                    }
                }
            }
        }
    }
}

int NanHandling::longestPathInstCount(llvm::BasicBlock *BB, int &depth)
{
#define MAX_SEARCH_DEPTH 10

    depth++;
    if (!BB || depth>MAX_SEARCH_DEPTH)
        return 0;

    int sumSuccInstCount = 0;
    for (succ_iterator SI = succ_begin(BB), E = succ_end(BB); SI != E; ++SI)
    {
        sumSuccInstCount += longestPathInstCount(*SI, depth);
    }
    return (int)(BB->getInstList().size()) + sumSuccInstCount;
}

void NanHandling::swapBranch(llvm::Instruction *inst, llvm::BranchInst &BI)
{
    if (FCmpInst *brCondition = dyn_cast<FCmpInst>(inst))
    {
        if (inst->hasOneUse())
        {
            brCondition->setPredicate(FCmpInst::getInversePredicate(brCondition->getPredicate()));
            BI.swapSuccessors();
        }
    }
    else
    {
        // inst not expected
        assert(0);
    }
}

void NanHandling::visitBranchInst(llvm::BranchInst &I)
{
    if (!I.isConditional())
        return;

    // if this branch is part of a loop, it is taken care of already in loopNanCases
    for (auto iter = visitedInst.begin(); iter != visitedInst.end(); iter++)
    {
        if (&I == *iter)
            return;
    }

    FCmpInst *brCmpInst = dyn_cast<FCmpInst>(I.getCondition());
    FCmpInst *src0 = nullptr;
    FCmpInst *src1 = nullptr;

    // if the branching is based on a cmp instruction
    if (brCmpInst)
    {
        FPMathOperator *FPO = dyn_cast<FPMathOperator>(brCmpInst);
        if (!FPO || !FPO->isFast())
            return;

        if (!brCmpInst->hasOneUse())
            return;
    }
    // if the branching is based on a and/or from multiple conditions.
    else if (BinaryOperator *andOrInst = dyn_cast<BinaryOperator>(I.getCondition()))
    {
        if (andOrInst->getOpcode() != BinaryOperator::And && andOrInst->getOpcode() != BinaryOperator::Or)
            return;

        src0 = dyn_cast<FCmpInst>(andOrInst->getOperand(0));
        src1 = dyn_cast<FCmpInst>(andOrInst->getOperand(1));

        if (!src0 || !src1)
            return;
    }
    else
    {
        return;
    }

    // Calculate the maximum instruction count when going down one branch.
    // Make the false case (including NaN) goes to the shorter path.
    int depth = 0;
    int trueBranchSize = longestPathInstCount(I.getSuccessor(0), depth);
    depth = 0;
    int falseBranchSize = longestPathInstCount(I.getSuccessor(1), depth);

    if (falseBranchSize - trueBranchSize > (int)(IGC_GET_FLAG_VALUE(SetBranchSwapThreshold)))
    {
        if (brCmpInst)
        {
            // swap the condition and the successor blocks
            swapBranch(brCmpInst, I);
        }
        else
        {
            FastMathFlags FMF;
            FMF.clear();
            src0->copyFastMathFlags(FMF);
            src1->copyFastMathFlags(FMF);
        }
        return;
    }
}
IGC_INITIALIZE_PASS_BEGIN(NanHandling, "NanHandling", "NanHandling", false, false)
IGC_INITIALIZE_PASS_END(NanHandling, "NanHandling", "NanHandling", false, false)

namespace {

class GenStrengthReduction : public FunctionPass
{
public:
    static char ID;
    GenStrengthReduction() : FunctionPass(ID)
    {
        initializeGenStrengthReductionPass(*PassRegistry::getPassRegistry());
    }
    virtual llvm::StringRef getPassName() const { return "Gen strength reduction"; }
    virtual bool runOnFunction(Function &F);

private:
    bool processInst(Instruction *Inst);
    // Transform (extract-element (bitcast %vector) ...) to
    // (bitcast (extract-element %vector) ...) in order to help coalescing in DeSSA.
    bool optimizeVectorBitCast(Function &F) const;
};

} // namespace

char GenStrengthReduction::ID = 0;
FunctionPass *IGC::createGenStrengthReductionPass() { return new GenStrengthReduction(); }

bool GenStrengthReduction::runOnFunction(Function &F)
{
    bool Changed = false;
    for (auto &BB : F)
    {
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE;)
        {
            Instruction *Inst = &(*BI++);
            if (isInstructionTriviallyDead(Inst))
            {
                Inst->eraseFromParent();
                Changed = true;
                continue;
            }
            Changed |= processInst(Inst);
        }
    }

    Changed |= optimizeVectorBitCast(F);

    return Changed;
}

// Check if this is a fdiv that allows reciprocal, and its divident is not known
// to be 1.0.
static bool isCandidateFDiv(Instruction *Inst)
{
    // Only floating points, and no vectors.
    if (!Inst->getType()->isFloatingPointTy() || Inst->use_empty())
        return false;

    auto Op = dyn_cast<FPMathOperator>(Inst);
    if (Op && Op->getOpcode() == Instruction::FDiv && Op->hasAllowReciprocal())
    {
        Value *Src0 = Op->getOperand(0);
        if (auto CFP = dyn_cast<ConstantFP>(Src0))
            return !CFP->isExactlyValue(1.0);
        return true;
    }
    return false;
}

bool GenStrengthReduction::processInst(Instruction *Inst)
{

    unsigned opc = Inst->getOpcode();
    auto Op = dyn_cast<FPMathOperator>(Inst);
    if (opc == Instruction::Select)
    {
        Value *oprd1 = Inst->getOperand(1);
        Value *oprd2 = Inst->getOperand(2);
        ConstantFP *CF1 = dyn_cast<ConstantFP>(oprd1);
        ConstantFP *CF2 = dyn_cast<ConstantFP>(oprd2);
        if (oprd1 == oprd2 ||
            (CF1 && CF2 && CF1->isExactlyValue(CF2->getValueAPF())))
        {
            Inst->replaceAllUsesWith(oprd1);
            Inst->eraseFromParent();
            return true;
        }
    }
    if (Op && 
        Op->hasNoNaNs() && 
        Op->hasNoInfs() &&
        Op->hasNoSignedZeros())
    {
        switch (opc)
        {
        case Instruction::FDiv :
          {
            Value *Oprd0 = Inst->getOperand(0);
            if (ConstantFP *CF = dyn_cast<ConstantFP>(Oprd0))
            {
                if (CF->isZero())
                {
                    Inst->replaceAllUsesWith(Oprd0);
                    Inst->eraseFromParent();
                    return true;
                }
            }
            break;
          }
        case Instruction::FMul :
          {
            for (int i=0; i < 2; ++i)
            {
                ConstantFP  *CF = dyn_cast<ConstantFP>(Inst->getOperand(i));
                if (CF && CF->isZero())
                {
                    Inst->replaceAllUsesWith(CF);
                    Inst->eraseFromParent();
                    return true;
                }
            }
            break;
          }
        case Instruction::FAdd :
          {
            for (int i = 0; i < 2; ++i)
            {
                ConstantFP  *CF = dyn_cast<ConstantFP>(Inst->getOperand(i));
                if (CF && CF->isZero())
                {
                    Value *otherOprd = Inst->getOperand(1-i);
                    Inst->replaceAllUsesWith(otherOprd);
                    Inst->eraseFromParent();
                    return true;
                }
            }
            break;
          }
        }
    }

    // fdiv -> inv + mul. On gen, fdiv seems always slower
    // than inv + mul. Do it if fdiv's fastMathFlag allows it.
    //
    // Rewrite
    // %1 = fdiv arcp float %x, %z
    // into
    // %1 = fdiv arcp float 1.0, %z
    // %2 = fmul arcp float %x, %1
    if (isCandidateFDiv(Inst))
    {
        Value *Src1 = Inst->getOperand(1);
        if (isa<Constant>(Src1))
        {
            // should not happen (but do see "fdiv  x / 0.0f"). Skip.
            return false;
        }

        Value *Src0 = ConstantFP::get(Inst->getType(), 1.0);
        Instruction *Inv = nullptr;

        // Check if there is any other (x / Src1). If so, commonize 1/Src1.
        for (auto UI = Src1->user_begin(), UE = Src1->user_end();
             UI != UE; ++UI)
        {
            Value *Val = *UI;
            Instruction *I = dyn_cast<Instruction>(Val);
            if (I && I != Inst && I->getOpcode() == Instruction::FDiv &&
                I->getOperand(1) == Src1 && isCandidateFDiv(I))
            {
                // special case
                if (ConstantFP *CF = dyn_cast<ConstantFP>(I->getOperand(0)))
                {
                    if (CF->isZero())
                    {
                        // Skip this one.
                        continue;
                    }
                }

                // Found another 1/Src1. Insert Inv right after the def of Src1
                // or in the entry BB if Src1 is an argument.
                if (!Inv)
                {
                    Instruction *insertBefore = dyn_cast<Instruction>(Src1);
                    if (insertBefore)
                    {
                        if (isa<PHINode>(insertBefore))
                        {
                            BasicBlock *BB = insertBefore->getParent();
                            insertBefore = &(*BB->getFirstInsertionPt());
                        }
                        else
                        {
                            // Src1 is an instruction
                            BasicBlock::iterator iter(insertBefore);
                            ++iter;
                            insertBefore = &(*iter);
                        }
                    }
                    else
                    {
                        // Src1 is an argument and insert at the begin of entry BB
                        BasicBlock &entryBB = Inst->getParent()->getParent()->getEntryBlock();
                        insertBefore = &(*entryBB.getFirstInsertionPt());
                    }
                    Inv = BinaryOperator::CreateFDiv(Src0, Src1, "", insertBefore);
                    Inv->setFastMathFlags(Inst->getFastMathFlags());
                }

                Instruction *Mul = BinaryOperator::CreateFMul(I->getOperand(0), Inv, "", I);
                Mul->setFastMathFlags(Inst->getFastMathFlags());
                I->replaceAllUsesWith(Mul);
                // Don't erase it as doing so would invalidate iterator in this func's caller
                // Instead, erase it in the caller.
                // I->eraseFromParent();
            }
        }

        if (!Inv)
        {
            // Only a single use of 1 / Src1. Create Inv right before the use.
            Inv = BinaryOperator::CreateFDiv(Src0, Src1, "", Inst);
            Inv->setFastMathFlags(Inst->getFastMathFlags());
        }

        auto Mul = BinaryOperator::CreateFMul(Inst->getOperand(0), Inv, "", Inst);
        Mul->setFastMathFlags(Inst->getFastMathFlags());
        Inst->replaceAllUsesWith(Mul);
        Inst->eraseFromParent();
        return true;
    }

    return false;
}

bool GenStrengthReduction::optimizeVectorBitCast(Function &F) const {
    IRBuilder<> Builder(F.getContext());

    bool Changed = false;
    for (auto &BB : F) {
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*EMPTY*/) {
            BitCastInst *BC = dyn_cast<BitCastInst>(&*BI++);
            if (!BC) continue;
            // Skip non-element-wise bitcast.
            VectorType *DstVTy = dyn_cast<VectorType>(BC->getType());
            VectorType *SrcVTy = dyn_cast<VectorType>(BC->getOperand(0)->getType());
            if (!DstVTy || !SrcVTy || DstVTy->getNumElements() != SrcVTy->getNumElements())
                continue;
            // Skip if it's not used only all extract-element.
            bool ExactOnly = true;
            for (auto User : BC->users()) {
                if (auto EEI = dyn_cast<ExtractElementInst>(User)) continue;
                ExactOnly = false;
                break;
            }
            if (!ExactOnly)
                continue;
            // Autobots, transform and roll out!
            Value *Src = BC->getOperand(0);
            Type *DstEltTy = DstVTy->getElementType();
            for (auto UI = BC->user_begin(), UE = BC->user_end(); UI != UE;
                 /*EMPTY*/) {
                auto EEI = cast<ExtractElementInst>(*UI++);
                Builder.SetInsertPoint(EEI);
                auto NewVal = Builder.CreateExtractElement(Src, EEI->getIndexOperand());
                NewVal = Builder.CreateBitCast(NewVal, DstEltTy);
                EEI->replaceAllUsesWith(NewVal);
                EEI->eraseFromParent();
            }
            BI = BC->eraseFromParent();
            Changed = true;
        }
    }

    return Changed;
}

IGC_INITIALIZE_PASS_BEGIN(GenStrengthReduction, "GenStrengthReduction",
                          "GenStrengthReduction", false, false)
IGC_INITIALIZE_PASS_END(GenStrengthReduction, "GenStrengthReduction",
                        "GenStrengthReduction", false, false)


/*========================== FlattenSmallSwitch ==============================

This class flatten small switch. For example,

before optimization:
    switch i32 %115, label %else229 [
    i32 1, label %then214
    i32 2, label %then222
    ]

    then214:                                          ; preds = %then153
    %150 = fdiv float 1.000000e+00, %res_s208
    %151 = fmul float %147, %150
    br label %ifcont237

    then222:                                          ; preds = %then153
    %152 = fsub float 1.000000e+00, %141
    br label %ifcont237

    else229:                                          ; preds = %then153
    %res_s230 = icmp eq i32 %115, 3
    %. = select i1 %res_s230, float 1.000000e+00, float 0.000000e+00
    br label %ifcont237

    ifcont237:                                        ; preds = %else229, %then222, %then214
    %"r[9][0].x.0" = phi float [ %151, %then214 ], [ %152, %then222 ], [ %., %else229 ]

after optimization:
    %res_s230 = icmp eq i32 %115, 3
    %. = select i1 %res_s230, float 1.000000e+00, float 0.000000e+00
    %150 = fdiv float 1.000000e+00, %res_s208
    %151 = fmul float %147, %150
    %152 = icmp eq i32 %115, 1
    %153 = select i1 %152, float %151, float %.
    %154 = fsub float 1.000000e+00, %141
    %155 = icmp eq i32 %115, 2
    %156 = select i1 %155, float %154, float %153

=============================================================================*/
namespace {
class FlattenSmallSwitch : public FunctionPass
{
public:
    static char ID;
    FlattenSmallSwitch() : FunctionPass(ID)
    {
        initializeFlattenSmallSwitchPass(*PassRegistry::getPassRegistry());
    }
    virtual llvm::StringRef getPassName() const { return "Flatten Small Switch"; }
    virtual bool runOnFunction(Function &F);
    bool processSwitchInst(SwitchInst *SI);
};

} // namespace

char FlattenSmallSwitch::ID = 0;
FunctionPass *IGC::createFlattenSmallSwitchPass() { return new FlattenSmallSwitch(); }

bool FlattenSmallSwitch::processSwitchInst(SwitchInst *SI) 
{
    const unsigned maxSwitchCases = 3;  // only apply to switch with 3 cases or less
    const unsigned maxCaseInsts = 3;    // only apply optimization when each case has 3 instructions or less.

    BasicBlock* Default = SI->getDefaultDest();
    Value *Val = SI->getCondition();  // The value we are switching on...
    IRBuilder<> builder(SI);

    if (SI->getNumCases() > maxSwitchCases || SI->getNumCases() == 0)
    {
        return false;
    }

    // Dest will be the block that the control flow from the switch merges to.
    // Currently, there are two options:
    // 1. The Dest block is the default block from the switch
    // 2. The Dest block is jumped to by all of the switch cases (and the default)
    BasicBlock *Dest = nullptr;
    {
        const auto *CaseSucc = 
#if LLVM_VERSION_MAJOR == 4
            SI->case_begin().getCaseSuccessor();
#elif LLVM_VERSION_MAJOR >= 7
            SI->case_begin()->getCaseSuccessor();
#endif
        auto *BI = dyn_cast<BranchInst>(CaseSucc->getTerminator());

        if (BI == nullptr)
            return false;

        if (BI->isConditional())
            return false;

        // We know the first case jumps to this block.  Now let's
        // see below whether all the cases jump to this same block.
        Dest = BI->getSuccessor(0);
    }

    // Does BB unconditionally branch to MergeBlock?
    auto branchPattern = [](const BasicBlock *BB, const BasicBlock *MergeBlock)
    {
        auto *br = dyn_cast<BranchInst>(BB->getTerminator());

        if (br == nullptr)
            return false;

        if (br->isConditional())
            return false;

        if (br->getSuccessor(0) != MergeBlock)
            return false;

        return true;
    };

    // We can speculatively execute a basic block if it
    // is small, unconditionally branches to Dest, and doesn't
    // have high latency or unsafe to speculate instructions.
    auto canSpeculateBlock = [&](BasicBlock *BB)
    {
        if (BB->size() > maxCaseInsts)
            return false;

        if (!branchPattern(BB, Dest))
            return false;

        for (auto &I : *BB)
        {
            auto *inst = &I;

            if (isa<BranchInst>(inst))
                continue;

            // if there is any high-latency instruction in the switch,
            // don't flatten it
            if (isSampleInstruction(inst)  ||
                isGather4Instruction(inst) ||
                isInfoInstruction(inst)    ||
                isLdInstruction(inst)      ||
                // If the instruction can't be speculated (e.g., phi node),
                // punt.
                !isSafeToSpeculativelyExecute(inst))
            {
                return false;
            }
        }

        return true;
    };

    for (auto &I : SI->cases())
    {
        BasicBlock *CaseDest = I.getCaseSuccessor();

        if (!canSpeculateBlock(CaseDest))
            return false;
    }

    // Is the default case of the switch the block
    // where all other cases meet?
    const bool DefaultMergeBlock = (Dest == Default);

    // If we merge to the default block, there is no block
    // we jump to beforehand so there is nothing to
    // speculate.
    if (!DefaultMergeBlock && !canSpeculateBlock(Default))
        return false;

    // Get all PHI nodes that needs to be replaced
    SmallVector<PHINode*, 4> PhiNodes;
    for (auto &I : *Dest)
    {
        auto *Phi = dyn_cast<PHINode>(&I);

        if (!Phi)
            break;

        if (Phi->getNumIncomingValues() != SI->getNumCases() + 1)
            return false;

        PhiNodes.push_back(Phi);
    }

    if (PhiNodes.empty())
        return false;

    // Move all instructions except the last (i.e., the branch)
    // from BB to the InsertPoint.
    auto splice = [](BasicBlock *BB, Instruction *InsertPoint)
    {
        Instruction* preIter = nullptr;
        for (auto &iter : *BB)
        {
            if (preIter)
            {
                preIter->moveBefore(InsertPoint);
            }
            preIter = cast<Instruction>(&iter);
        }
    };

    // move default block out
    if (!DefaultMergeBlock)
        splice(Default, SI);

    // move case blocks out
    for (auto &I : SI->cases())
    {
        BasicBlock *CaseDest = I.getCaseSuccessor();
        splice(CaseDest, SI);
    }

    // replaces PHI with select
    for (auto *Phi : PhiNodes)
    {
        Value *vTemp = Phi->getIncomingValueForBlock(
            DefaultMergeBlock ? SI->getParent() : Default);

        for (auto &I : SI->cases())
        {
            BasicBlock *CaseDest   = I.getCaseSuccessor();
            ConstantInt *CaseValue = I.getCaseValue();

            Value *selTrueValue = Phi->getIncomingValueForBlock(CaseDest);
            builder.SetInsertPoint(SI);
            Value *cmp = builder.CreateICmp(CmpInst::Predicate::ICMP_EQ, Val, CaseValue);
            Value *sel = builder.CreateSelect(cmp, selTrueValue, vTemp);
            vTemp = sel;
        }

        Phi->replaceAllUsesWith(vTemp);
    }

    // connect the original block and the phi node block with a pass through branch
    builder.CreateBr(Dest);

    // Remove the switch.
    BasicBlock *SelectBB = SI->getParent();
    for (unsigned i = 0, e = SI->getNumSuccessors(); i < e; ++i) 
    {
        BasicBlock *Succ = SI->getSuccessor(i);
        if (Succ == Dest)
        {
            continue;
        }
        Succ->removePredecessor(SelectBB);
    }
    SI->eraseFromParent();

    return true;
}

bool FlattenSmallSwitch::runOnFunction(Function &F)
{
    bool Changed = false;
    for (Function::iterator I = F.begin(), E = F.end(); I != E; ) 
    {
        BasicBlock *Cur = &*I++; // Advance over block so we don't traverse new blocks
        if (SwitchInst *SI = dyn_cast<SwitchInst>(Cur->getTerminator()))
        {
            Changed |= processSwitchInst(SI);
        }
    }
    return Changed;
}

IGC_INITIALIZE_PASS_BEGIN(FlattenSmallSwitch, "flattenSmallSwitch", "flattenSmallSwitch", false, false)
IGC_INITIALIZE_PASS_END(FlattenSmallSwitch, "flattenSmallSwitch", "flattenSmallSwitch", false, false)

////////////////////////////////////////////////////////////////////////
// LogicalAndToBranch trying to find logical AND like below:
//    res = simpleCond0 && complexCond1
// and convert it to:
//    if simpleCond0
//        res = complexCond1
//    else
//        res = false
namespace {
class LogicalAndToBranch : public FunctionPass
{
public:
    static char ID;
    const int NUM_INST_THRESHOLD = 32;
    LogicalAndToBranch();

    StringRef getPassName() const override { return "LogicalAndToBranch"; }

    bool runOnFunction(Function& F) override;

protected:
    SmallPtrSet<Instruction*, 8> m_sched;

    // schedule instruction up before insertPos
    bool scheduleUp(BasicBlock* bb, Value* V, Instruction* &insertPos);

    // check if it's safe to convert instructions between cond0 & cond1,
    // moveInsts are the values referened out of (cond0, cond1), we need to
    // move them before cond0
    bool isSafeToConvert(Instruction* cond0, Instruction* cond1,
        smallvector<Instruction*, 8>& moveInsts);

    void convertAndToBranch(Instruction* opAnd,
        Instruction* cond0, Instruction* cond1, BasicBlock* &newBB);
};

}

IGC_INITIALIZE_PASS_BEGIN(LogicalAndToBranch, "logicalAndToBranch", "logicalAndToBranch", false, false)
IGC_INITIALIZE_PASS_END(LogicalAndToBranch, "logicalAndToBranch", "logicalAndToBranch", false, false)

char LogicalAndToBranch::ID = 0;
FunctionPass *IGC::createLogicalAndToBranchPass() { return new LogicalAndToBranch(); }

LogicalAndToBranch::LogicalAndToBranch() : FunctionPass(ID)
{
    initializeLogicalAndToBranchPass(*PassRegistry::getPassRegistry());
}

bool LogicalAndToBranch::scheduleUp(BasicBlock* bb, Value* V,
    Instruction* &insertPos)
{
    Instruction *inst = dyn_cast<Instruction>(V);
    if (!inst)
        return false;

    if (inst->getParent() != bb || isa<PHINode>(inst))
        return false;

    if (m_sched.count(inst))
    {
        if (insertPos && !isInstPrecede(inst, insertPos))
            insertPos = inst;
        return false;
    }

    bool changed = false;

    for (auto OI = inst->op_begin(), OE = inst->op_end(); OI != OE; ++OI)
    {
        changed |= scheduleUp(bb, OI->get(), insertPos);
    }
    m_sched.insert(inst);

    if (insertPos && isInstPrecede(inst, insertPos))
        return changed;

    if (insertPos) {
        inst->removeFromParent();
        inst->insertBefore(insertPos);
    }

    return true;
}

// split original basic block from:
//   original BB:
//     %cond0 =
//     ...
//     %cond1 =
//     %andRes = and i1 %cond0, %cond1
//     ...
// to:
//    original BB:
//      %cond0 =
//      if %cond0, if.then, if.else
//
//    if.then:
//      ...
//      %cond1 =
//      br if.end
//
//    if.else:
//      br if.end
//
//    if.end:
//       %andRes = phi [%cond1, if.then], [false, if.else]
//       ...
void LogicalAndToBranch::convertAndToBranch(Instruction* opAnd,
    Instruction* cond0, Instruction* cond1, BasicBlock* &newBB)
{
    BasicBlock* bb = opAnd->getParent();
    BasicBlock *bbThen, *bbElse, *bbEnd;

    bbThen = bb->splitBasicBlock(cond0->getNextNode(), "if.then");
    bbElse = bbThen->splitBasicBlock(opAnd, "if.else");
    bbEnd = bbElse->splitBasicBlock(opAnd, "if.end");

    bb->getTerminator()->eraseFromParent();
    BranchInst* br = BranchInst::Create(bbThen, bbElse, cond0, bb);

    bbThen->getTerminator()->eraseFromParent();
    br = BranchInst::Create(bbEnd, bbThen);

    PHINode* phi = PHINode::Create(opAnd->getType(), 2, "", opAnd);
    phi->addIncoming(cond1, bbThen);
    phi->addIncoming(ConstantInt::getFalse(opAnd->getType()), bbElse);
    opAnd->replaceAllUsesWith(phi);
    opAnd->eraseFromParent();

    newBB = bbEnd;
}

bool LogicalAndToBranch::isSafeToConvert(
    Instruction* cond0, Instruction* cond1,
    smallvector<Instruction*, 8>& moveInsts)
{
    BasicBlock::iterator is0(cond0);
    BasicBlock::iterator is1(cond1);

    bool isSafe = true;
    SmallPtrSet<Value*, 32> iset;

    iset.insert(cond1);
    for (auto i = ++is0; i != is1; ++i)
    {
        if ((*i).mayHaveSideEffects())
        {
            isSafe = false;
            break;
        }
        iset.insert(&(*i));
    }

    if (!isSafe)
    {
        return false;
    }

    is0 = cond0->getIterator();
    // check if the values in between are used elsewhere
    for (auto i = ++is0; i != is1; ++i)
    {
        Instruction* inst = &*i;
        for (auto ui : inst->users())
        {
            if (iset.count(ui) == 0)
            {
                moveInsts.push_back(inst);
                break;
            }
        }
    }
    return isSafe;
}

bool LogicalAndToBranch::runOnFunction(Function& F)
{
    bool changed = false;
    if (IGC_IS_FLAG_DISABLED(EnableLogicalAndToBranch))
    {
        return changed;
    }

    for (auto BI = F.begin(), BE = F.end(); BI != BE; )
    {
        // advance iterator before handling current BB
        BasicBlock *bb = &*BI++;

        for (auto II = bb->begin(), IE = bb->end(); II != IE; )
        {
            Instruction* inst = &(*II++);

            // search for "and i1"
            if (inst->getOpcode() == BinaryOperator::And &&
                inst->getType()->isIntegerTy(1))
            {
                Instruction* s0 = dyn_cast<Instruction>(inst->getOperand(0));
                Instruction* s1 = dyn_cast<Instruction>(inst->getOperand(1));
                if (s0 && s1 &&
                    !isa<PHINode>(s0) && !isa<PHINode>(s1) &&
                    s0->getNumUses() == 1 && s1->getNumUses() == 1 &&
                    s0->getParent() == bb && s1->getParent() == bb)
                {
                    if (isInstPrecede(s1, s0))
                    {
                        std::swap(s0, s1);
                    }
                    BasicBlock::iterator is0(s0);
                    BasicBlock::iterator is1(s1);

                    if (std::distance(is0, is1) < NUM_INST_THRESHOLD)
                    {
                        continue;
                    }

                    smallvector<Instruction*, 8> moveInsts;
                    if (isSafeToConvert(s0, s1, moveInsts))
                    {
                        // if values defined between s0 & s1 are referenced
                        // outside of (s0, s1), they need to be moved before
                        // s0 to keep SSA form.
                        for (auto inst : moveInsts)
                            scheduleUp(bb, inst, s0);
                        m_sched.clear();

                        // IE need to be updated since original BB is splited
                        convertAndToBranch(inst, s0, s1, bb);
                        IE = bb->end();
                        changed = true;
                    }
                }
            }
        }
    }

    return changed;
}

