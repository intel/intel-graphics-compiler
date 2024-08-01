/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

/*========================== CustomSafeOptPass.cpp ==========================

This file contains IGC custom optimizations that are arithmetically safe.
The passes are
    CustomSafeOptPass
    GenSpecificPattern
    IGCConstProp
    IGCIndirectICBPropagaion
    CustomLoopInfo
    VectorBitCastOpt
    GenStrengthReduction
    FlattenSmallSwitch
    SplitIndirectEEtoSel

CustomSafeOptPass does peephole optimizations
For example, reduce the alloca size so there is a chance to promote indexed temp.

GenSpecificPattern reverts llvm changes back to what is needed.
For example, llvm changes AND to OR, and GenSpecificPattern changes it back to
allow more optimizations

IGCConstProp was originated from llvm copy-prop code with one addition for
shader-constant replacement. So llvm copyright is added above.

IGCIndirectICBPropagation reads the immediate constant buffer from meta data and
use them as immediates instead of using send messages to read from buffer.

CustomLoopInfo returns true if there is any sampleL in a loop for the driver.

VectorBitCastOpt preprocesses vector bitcasts to be after extractelement
instructions.

GenStrengthReduction performs a fdiv optimization.

FlattenSmallSwitch flatten the if/else or switch structure and use cmp+sel
instead if the structure is small.

SplitIndirectEEtoSel splits extractelements with very small vec to a series of
cmp+sel to avoid expensive VxH mov.

=============================================================================*/

#include "Compiler/CustomSafeOptPass.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/IGCConstantFolder.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include <llvmWrapper/ADT/APInt.h>
#include "llvmWrapper/IR/IntrinsicInst.h"
#include <llvmWrapper/IR/DIBuilder.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include <llvmWrapper/IR/IRBuilder.h>
#include <llvmWrapper/IR/PatternMatch.h>
#include <llvmWrapper/Analysis/TargetLibraryInfo.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/Constants.h>
#include "llvm/IR/DebugInfo.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"
#include <set>
#include "common/secure_mem.h"
#include "Probe/Assertion.h"

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

#define DEBUG_TYPE "CustomSafeOptPass"

STATISTIC(Stat_DiscardRemoved, "Number of insts removed in Discard Opt");

bool CustomSafeOptPass::runOnFunction(Function& F)
{
    pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    psHasSideEffect = getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->m_instrTypes.psHasSideEffect;
    visit(F);
    return true;
}

void CustomSafeOptPass::visitInstruction(Instruction& I)
{
    // nothing
}

//  Searches the following pattern
//      %1 = icmp eq i32 %cmpop1, %cmpop2
//      %2 = xor i1 %1, true
//      ...
//      %3 = select i1 %1, i8 0, i8 1
//
//  And changes it to:
//      %1 = icmp ne i32 %cmpop1, %cmpop2
//      ...
//      %3 = select i1 %1, i8 1, i8 0
//
//  and
//
//  Searches the following pattern
//      %1 = icmp ule i32 %cmpop1, %cmpop2
//      %2 = xor i1 %1, true
//      br i1 %1, label %3, label %4
//
//  And changes it to:
//      %1 = icmp ugt i32 %cmpop1, %cmpop2
//      br i1 %1, label %4, label %3
//
//  This optimization combines statements regardless of the predicate.
//  It will also work if the icmp instruction does not have users, except for the xor, select or branch instruction.
void CustomSafeOptPass::visitXor(Instruction& XorInstr) {
    using namespace llvm::PatternMatch;

    CmpInst::Predicate Pred = CmpInst::Predicate::FCMP_FALSE;
    auto XorPattern = m_c_Xor(m_ICmp(Pred, m_Value(), m_Value()), m_SpecificInt(1));
    if (!match(&XorInstr, XorPattern)) {
        return;
    }

    Value* XorOp0 = XorInstr.getOperand(0);
    Value* XorOp1 = XorInstr.getOperand(1);
    auto ICmpInstr = cast<Instruction>(isa<ICmpInst>(XorOp0) ? XorOp0 : XorOp1);

    llvm::SmallVector<Instruction*, 4> UsersList;

    for (const auto& U : ICmpInstr->uses()) {
        auto user = U.getUser();
        if (isa<BranchInst>(user)) {
            UsersList.push_back(cast<Instruction>(user));
        }
        else if (SelectInst* S = dyn_cast<SelectInst>(user)) {
            constexpr uint32_t condIdx = 0;
            uint32_t idx = U.getOperandNo();
            if (condIdx == idx) {
                UsersList.push_back(cast<Instruction>(S));
            }
            else {
                return;
            }
        }
        else if (user != &XorInstr) {
            return;
        }
    }

    IRBuilder<> builder(ICmpInstr);
    auto NegatedCmpPred = cast<ICmpInst>(ICmpInstr)->getInversePredicate();
    auto NewCmp = cast<ICmpInst>(builder.CreateICmp(NegatedCmpPred, ICmpInstr->getOperand(0), ICmpInstr->getOperand(1)));

    for (auto I : UsersList) {
        if (SelectInst* S = dyn_cast<SelectInst>(I)) {
            S->swapProfMetadata();
            Value* TrueVal = S->getTrueValue();
            Value* FalseVal = S->getFalseValue();
            S->setTrueValue(FalseVal);
            S->setFalseValue(TrueVal);
        }
        else {
            IGC_ASSERT(isa<BranchInst>(I));
            BranchInst* B = cast<BranchInst>(I);
            B->swapSuccessors();
        }
    }

    // DIExpression in debug variable instructions must be extended with additional DWARF opcodes:
    // DW_OP_constu 1, DW_OP_xor, DW_OP_stack_value
    const DebugLoc& DL = XorInstr.getDebugLoc();
    if (Instruction* NewCmpInst = dyn_cast<Instruction>(NewCmp)) {
        NewCmpInst->setDebugLoc(DL);
        auto* Val = static_cast<Value*>(ICmpInstr);
        SmallVector<DbgValueInst*, 1> DbgValues;
        llvm::findDbgValues(DbgValues, Val);
        for (auto DV : DbgValues) {
            DIExpression* OldExpr = DV->getExpression();
            DIExpression* NewExpr = DIExpression::append(
                OldExpr, { dwarf::DW_OP_constu, 1, dwarf::DW_OP_xor, dwarf::DW_OP_stack_value});
            IGCLLVM::setExpression(DV, NewExpr);
        }
    }

    XorInstr.replaceAllUsesWith(NewCmp);
    ICmpInstr->replaceAllUsesWith(NewCmp);
    XorInstr.eraseFromParent();
    ICmpInstr->eraseFromParent();
}

//  Searches for following pattern:
//    %cmp = icmp slt i32 %x, %y
//    %cond.not = xor i1 %cond, true
//    %and.cond = and i1 %cmp, %cond.not
//    br i1 %or.cond, label %bb1, label %bb2
//
//  And changes it to:
//    %0 = icmp sge i32 %x, %y
//    %1 = or i1 %cond, %0
//    br i1 %1, label %bb2, label %bb1
void CustomSafeOptPass::visitAnd(BinaryOperator& I) {
    using namespace llvm::PatternMatch;

    //searching another pattern for And instruction
    if (lower64bto32b(I)) {
        return;
    }

    if (!I.hasOneUse() ||
        !isa<BranchInst>(*I.user_begin()) ||
        !I.getType()->isIntegerTy(1)) {
        return;
    }

    Value* XorArgValue = nullptr;
    CmpInst::Predicate Pred = CmpInst::Predicate::FCMP_FALSE;
    auto AndPattern = m_c_And(m_c_Xor(m_Value(XorArgValue), m_SpecificInt(1)), m_ICmp(Pred, m_Value(), m_Value()));
    if (!match(&I, AndPattern)) return;

    IRBuilder<> builder(&I);
    auto CompareInst = cast<ICmpInst>(isa<ICmpInst>(I.getOperand(0)) ? I.getOperand(0) : I.getOperand(1));
    auto NegatedCompareInst = builder.CreateICmp(CompareInst->getInversePredicate(), CompareInst->getOperand(0), CompareInst->getOperand(1));
    auto OrInst = builder.CreateOr(XorArgValue, NegatedCompareInst);

    auto BrInst = cast<BranchInst>(*I.user_begin());
    BrInst->setCondition(OrInst);
    BrInst->swapSuccessors();

    // after optimization DIExpression in debug variable instructions can be extended with additional DWARF opcodes:
    // DW_OP_constu 1, DW_OP_xor, DW_OP_stack_value
    const DebugLoc& DL = I.getDebugLoc();
    if (Instruction* NewOrInst = dyn_cast<Instruction>(OrInst)) {
        NewOrInst->setDebugLoc(DL);
        auto* Val = static_cast<Value*>(&I);
        SmallVector<DbgValueInst*, 1> DbgValues;
        llvm::findDbgValues(DbgValues, Val);
        for (auto DV : DbgValues) {
            DIExpression* OldExpr = DV->getExpression();
            DIExpression* NewExpr = DIExpression::append(
                OldExpr, { dwarf::DW_OP_constu, 1, dwarf::DW_OP_xor, dwarf::DW_OP_stack_value });
            IGCLLVM::setExpression(DV, NewExpr);
        }
    }
    I.replaceAllUsesWith(OrInst);
    I.eraseFromParent();
}

// Replace sub_group shuffle with index = sub_group_id ^ xor_value,
// where xor_value is a compile-time constant to intrinsic,
// which will produce sequence of movs instead of using indirect access
// This pattern comes from permute_group_by_xor, but can
// also be written manually as
//   uint32_t other_id = sg.get_local_id() ^ XOR_VALUE;
//   r = select_from_group(sg, x, other_id);
void CustomSafeOptPass::visitShuffleIndex(llvm::CallInst* I)
{
    using namespace llvm::PatternMatch;
    /*
    Pattern match
    %simdLaneId16 = call i16 @llvm.genx.GenISA.simdLaneId()
    ...[optional1] = %simdLaneId16
    %xor = xor i16 %[optional1], 1
    ...[optional2] = %xor
    %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %[optional2], i32 0)

    Optional can be any combinations of :
      * %and = and i16 %856, 63
      * %zext = zext i16 %857 to i32
    We ignore any combinations of those, as they don't change the final calculated value,
    and different permutations were observed.
    */

    ConstantInt* enableHelperLanes = dyn_cast<ConstantInt>(I->getOperand(2));
    if (!enableHelperLanes || enableHelperLanes->getZExtValue() != 0) {
        return;
    }

    auto getInstructionIgnoringAndZext = []( Value* V, unsigned Opcode ) -> Instruction* {
            while( auto* VI = dyn_cast<Instruction>( V ) ) {
                if( VI->getOpcode() == Opcode ) {
                    return VI;
                }
                else if( auto* ZI = dyn_cast<ZExtInst>( VI ) ) {
                    // Check if zext is from i16 to i32
                    if( ZI->getSrcTy()->isIntegerTy( 16 ) && ZI->getDestTy()->isIntegerTy( 32 ) ) {
                        V = ZI->getOperand( 0 ); // Skip over zext
                    } else {
                        return nullptr; // Not the zext we are looking for
                    }
                }
                else if( VI->getOpcode() == Instruction::And ) {
                    ConstantInt* andValueConstant = dyn_cast<ConstantInt>( VI->getOperand( 1 ) );
                    // We handle "redundant values", so those which bits enable all of
                    // 32 lanes, so 31, 63 (spotted in nature), 127, 255 etc.
                    if( andValueConstant && (( andValueConstant->getZExtValue() & 31 ) != 31 ) ) {
                        return nullptr;
                    }
                    V = VI->getOperand( 0 ); // Skip over and
                } else {
                    return nullptr; // Not a zext, and, or the specified opcode
                }
            }
            return nullptr; //unreachable
        };

    Instruction* xorInst = getInstructionIgnoringAndZext( I->getOperand( 1 ), Instruction::Xor );
    if( !xorInst )
        return;

    auto xorOperand = xorInst->getOperand( 0 );
    auto xorValueConstant = dyn_cast<ConstantInt> ( xorInst->getOperand( 1 ) );
    if( !xorValueConstant )
        return;

    uint64_t xorValue = xorValueConstant->getZExtValue();
    if( xorValue >= 16 )
    {
        // currently not supported in the emitter
        return;
    }

    auto simdLaneCandidate = getInstructionIgnoringAndZext( xorOperand, Instruction::Call );

    if (!simdLaneCandidate)
        return;

    CallInst* CI = cast<CallInst>( simdLaneCandidate );
    Function* simdIdF = CI->getCalledFunction();
    if( !simdIdF || GenISAIntrinsic::getIntrinsicID( simdIdF ) != GenISAIntrinsic::GenISA_simdLaneId)
        return;

    // since we didn't return earlier, pattern is found

    auto insertShuffleXor = [](IRBuilder<>& builder,
                                    Value* value,
                                    uint32_t xorValue)
    {
        Function* simdShuffleXorFunc = GenISAIntrinsic::getDeclaration(
            builder.GetInsertBlock()->getParent()->getParent(),
            GenISAIntrinsic::GenISA_simdShuffleXor,
            value->getType());

        return builder.CreateCall(simdShuffleXorFunc,
            { value, builder.getInt32(xorValue) }, "simdShuffleXor");
    };

    Value* value = I->getOperand(0);
    IRBuilder<> builder(I);
    Value* result = insertShuffleXor(builder, value, static_cast<uint32_t>(xorValue));
    I->replaceAllUsesWith(result);
    I->eraseFromParent();
}

// here we merge dot-product (no acc) and add (as acc) into one dp4a
// there must be:
// %id213- = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 0, i32 %305, i32 %345)
// %id214- = add i32 %id113-, %id213- <--- case 1: acc is on the 1st operand
// or
// %id214- = add i32 %id213-, %id113- <--- case 2: acc is on the 2nd operand
// after optimizing:
// %id213- = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 %id113-, i32 %305, i32 %345)
void CustomSafeOptPass::mergeDotAddToDp4a(llvm::CallInst* I)
{
    if (IGC_IS_FLAG_ENABLED(DisableDotAddToDp4aMerge))
      return;

    auto isPreviousDp4aInstr = [](GenIntrinsicInst* dp4aInstr, Value* opdVal) {
      // It's the dp4a intrinsic, such as the %id213- above.
      // If so, then the other operand is the acc.
      if (CallInst* valInst = dyn_cast<CallInst>(opdVal)) {
        if (GenIntrinsicInst* valInstr = dyn_cast<GenIntrinsicInst>(valInst)) {
          return (valInstr == dp4aInstr) ? true : false;
        }
      }

      return false;
    };

    // found %id213- = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 0, i32 %305, i32 %345)
    GenIntrinsicInst* instr = dyn_cast<GenIntrinsicInst>(I);

    if (ConstantInt* CI = dyn_cast<ConstantInt>(instr->getOperand(0))) {
      // make sure operand(0) value is (i32 0)
      if (CI->isZero()) {
        // check the followed instruction
        if (Instruction* nextInst = dyn_cast<Instruction>(I)->getNextNode()) {
          // make sure it's add op
          if (nextInst->getOpcode() == Instruction::Add) {
            // the acc value
            Value* accVal = nullptr;

            // If the operand(0) is the previous dp4a, then operand(1) is acc.
            // else vice versa.
            if (isPreviousDp4aInstr(instr, nextInst->getOperand(0))) {
              accVal = nextInst->getOperand(1);
            }
            else if (isPreviousDp4aInstr(instr, nextInst->getOperand(1))) {
              accVal = nextInst->getOperand(0);
            }

            // if a valid acc is found
            if (accVal) {
              for (auto user : instr->users()) {
                // replace i32 0 with %id113-
                instr->setOperand(0, accVal);
                user->replaceAllUsesWith(instr);
                // %id213- = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 %id113-, i32 %305, i32 %345)
                break;
              }
            }
          }
        }
      }
    }
}

// Reform prefetch instructions to maximize width and minimize the number of blocks. This function assumes that
// it operates on the correct form of the intrinsic (width X array_length matches the requirements).
void CustomSafeOptPass::visitLSC2DBlockPrefetch(CallInst* I) {
    const size_t widthInd = 7;
    const size_t numBlocksInd = 9;

    ConstantInt *constWidth = dyn_cast<ConstantInt>(I->getArgOperand(widthInd));
    ConstantInt *constNumBlocks = dyn_cast<ConstantInt>(I->getArgOperand(numBlocksInd));

    if (!constWidth || !constNumBlocks) {
        return;
    }

    uint64_t width = constWidth->getZExtValue();
    uint64_t numBlocks = constNumBlocks->getZExtValue();

    if (numBlocks == 1) {
        return;
    }

    I->setArgOperand(widthInd, ConstantInt::get(Type::getInt32Ty(I->getContext()), width * numBlocks));
    I->setArgOperand(numBlocksInd, ConstantInt::get(Type::getInt32Ty(I->getContext()), 1));
}

// Check if Lower 64b to 32b transformation is applicable for binary operator
// i.e. trunc(a op b) == trunc(a) op trunc(b)
static bool isTruncInvariant(unsigned Opcode) {
    switch (Opcode) {
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
        return true;
    default:
        return false;
    }
}

//  Searches for following pattern:
//    %mul = mul i64 %conv, %conv2
//    %conv3 = and i64 %mul, 0xFFFFFFFF
//
//  And changes it to:
//    %conv3 = trunc i64 %conv to i32
//    %conv4 = trunc i64 %conv2 to i32
//    %mul = mul i32 %conv3, %conv4
//    %conv5 = zext i32 %mul to i64
bool CustomSafeOptPass::lower64bto32b(BinaryOperator& AndInst) {
    using namespace llvm::PatternMatch;

    auto& AndUse = AndInst.getOperandUse(0);
    auto AndPattern = m_c_And(m_c_BinOp(m_Value(), m_Value()), m_SpecificInt(0xFFFFFFFF));

    if (!match(&AndInst, AndPattern) || !AndInst.getType()->isIntegerTy(64) || !AndUse->hasOneUse()) {
        return false;
    }

    auto BinOp = dyn_cast<BinaryOperator>(AndUse);
    if (!BinOp || !isTruncInvariant(BinOp->getOpcode()))
        return false;

    SmallVector<BinaryOperator*, 8> OpsToDelete;
    auto NewBinInst = analyzeTreeForTrunc64bto32b(AndUse, OpsToDelete);
    IRBuilder<> Builder(&AndInst);

    auto Zext = Builder.CreateZExt(NewBinInst, Builder.getInt64Ty());

    AndInst.replaceAllUsesWith(Zext);
    AndInst.eraseFromParent();

    for (BinaryOperator* BinOp : OpsToDelete) {
        BinOp->eraseFromParent();
    }
    return true;
}

Value* CustomSafeOptPass::analyzeTreeForTrunc64bto32b(const Use& OperandUse, SmallVector<BinaryOperator*, 8>& OpsToDelete) {
    Value* Operand = OperandUse.get();
    BinaryOperator* BinInst = dyn_cast<BinaryOperator>(Operand);
    if (BinInst && Operand->hasOneUse() && isTruncInvariant(BinInst->getOpcode())) {
        OpsToDelete.push_back(BinInst);

        Value* NewBinArgValue1 = analyzeTreeForTrunc64bto32b(BinInst->getOperandUse(0), OpsToDelete);
        Value* NewBinArgValue2 = analyzeTreeForTrunc64bto32b(BinInst->getOperandUse(1), OpsToDelete);

        IRBuilder<> Builder(BinInst);
        return Builder.CreateBinOp(BinInst->getOpcode(), NewBinArgValue1, NewBinArgValue2);
    }

    //add trunc for Inst
    IRBuilder<> Builder(cast<Instruction>(OperandUse.getUser()));
    return Builder.CreateTrunc(Operand, Builder.getInt32Ty());
}

/*
Optimizing from
% 377 = call i32 @llvm.genx.GenISA.simdSize()
% .rhs.trunc = trunc i32 % 377 to i8
% .lhs.trunc = trunc i32 % 26 to i8
% 383 = udiv i8 % .lhs.trunc, % .rhs.trunc
to
% 377 = call i32 @llvm.genx.GenISA.simdSize()
% .rhs.trunc = trunc i32 % 377 to i8
% .lhs.trunc = trunc i32 % 26 to i8
% a = shr i8 % rhs.trunc, 4
% b = shr i8 % .lhs.trunc, 3
% 382 = shr i8 % b, % a
or
% 377 = call i32 @llvm.genx.GenISA.simdSize()
% 383 = udiv i32 %382, %377
to
% 377 = call i32 @llvm.genx.GenISA.simdSize()
% a = shr i32 %377, 4
% b = shr i32 %382, 3
% 382 = shr i32 % b, % a
*/
void CustomSafeOptPass::visitUDiv(llvm::BinaryOperator& I)
{
    bool isPatternfound = false;

    if (TruncInst* trunc = dyn_cast<TruncInst>(I.getOperand(1)))
    {
        if (CallInst* Inst = dyn_cast<CallInst>(trunc->getOperand(0)))
        {
            if (GenIntrinsicInst* SimdInst = dyn_cast<GenIntrinsicInst>(Inst))
                if (SimdInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdSize)
                    isPatternfound = true;
        }
    }
    else
    {
        if (CallInst* Inst = dyn_cast<CallInst>(I.getOperand(1)))
        {
            if (GenIntrinsicInst* SimdInst = dyn_cast<GenIntrinsicInst>(Inst))
                if (SimdInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdSize)
                    isPatternfound = true;
        }
    }
    if (isPatternfound)
    {
        IRBuilder<> builder(&I);
        Value* Shift1 = builder.CreateLShr(I.getOperand(1), ConstantInt::get(I.getOperand(1)->getType(), 4));
        Value* Shift2 = builder.CreateLShr(I.getOperand(0), ConstantInt::get(I.getOperand(0)->getType(), 3));
        Value* Shift3 = builder.CreateLShr(Shift2, Shift1);
        I.replaceAllUsesWith(Shift3);
        I.eraseFromParent();
    }
}

void CustomSafeOptPass::visitAllocaInst(AllocaInst& I)
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

    llvm::Type* pType = I.getAllocatedType();
    if (!pType->isArrayTy() ||
        static_cast<ADDRESS_SPACE>(I.getType()->getAddressSpace()) != ADDRESS_SPACE_PRIVATE)
    {
        return;
    }

    if (!(pType->getArrayElementType()->isFloatingPointTy() ||
        pType->getArrayElementType()->isIntegerTy() ||
        pType->getArrayElementType()->isPointerTy()))
    {
        return;
    }

    int index_lb = int_cast<int>(pType->getArrayNumElements());
    int index_ub = 0;

    // Find all uses of this alloca
    for (Value::user_iterator it = I.user_begin(), e = I.user_end(); it != e; ++it)
    {
        if (GetElementPtrInst * pGEP = llvm::dyn_cast<GetElementPtrInst>(*it))
        {
            ConstantInt* C0 = dyn_cast<ConstantInt>(pGEP->getOperand(1));
            if (!C0 || !C0->isZero() || pGEP->getNumOperands() != 3)
            {
                return;
            }
            for (Value::user_iterator use_it = pGEP->user_begin(), use_e = pGEP->user_end(); use_it != use_e; ++use_it)
            {
                if (llvm::dyn_cast<llvm::LoadInst>(*use_it))
                {
                }
                else if (llvm::StoreInst * pStore = llvm::dyn_cast<llvm::StoreInst>(*use_it))
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
    if (newSize >= pType->getArrayNumElements() || newSize == 0)
        return;
    // found a case to optimize
    IGCLLVM::IRBuilder<> IRB(&I);
    llvm::ArrayType* allocaArraySize = llvm::ArrayType::get(pType->getArrayElementType(), newSize);
    llvm::AllocaInst* newAlloca = IRB.CreateAlloca(allocaArraySize, nullptr);
    llvm::Value* gepArg1 = nullptr;

    llvm::Function* userFunc = I.getParent()->getParent();
    llvm::Module& M = *userFunc->getParent();
    llvm::DIBuilder DIB(M);

    // A debug line info is moved so the alloca has corresponding dbg.declare call
    // with DIExpression DW_OP_LLVM_fragment specifying fragment.
    TinyPtrVector<DbgVariableIntrinsic*>  Dbgs = llvm::FindDbgAddrUses(&I);
    unsigned typeSize = (unsigned)pType->getArrayElementType()->getPrimitiveSizeInBits();
    if (!Dbgs.empty()) {
        const DebugLoc DL = I.getDebugLoc();
        for (auto DDI : Dbgs) {
            DILocalVariable* Var = DDI->getVariable();
            DIExpression* OldExpr = DDI->getExpression();
            auto ExprFragment = OldExpr->getFragmentInfo();
            auto* NewExpr = OldExpr;
            if (auto E = DIExpression::createFragmentExpression(OldExpr, (unsigned)index_lb* typeSize, (unsigned)newSize* typeSize)) {
                NewExpr = *E;
                if (Instruction* NewAllocaInst = dyn_cast<Instruction>(newAlloca)) {
                    if (Var)
                        DIB.insertDeclare(newAlloca, Var, NewExpr, DL, NewAllocaInst);
                }
            }
        }
    }

    for (Value::user_iterator it = I.user_begin(), e = I.user_end(); it != e; ++it) {
        if (GetElementPtrInst * pGEP = llvm::dyn_cast<GetElementPtrInst>(*it)) {
            if (dyn_cast<ConstantInt>(pGEP->getOperand(2))) {
                // pGEP->getOperand(2) is constant. Reduce the constant value directly
                int newIndex = int_cast<int>(dyn_cast<ConstantInt>(pGEP->getOperand(2))->getZExtValue())
                    - index_lb;
                gepArg1 = IRB.getInt32(newIndex);
            }
            else {
                // pGEP->getOperand(2) is not constant. create a sub instruction to reduce it
                gepArg1 = BinaryOperator::CreateSub(pGEP->getOperand(2), IRB.getInt32(index_lb), "reducedIndex", pGEP);
            }
            llvm::Value* gepArg[] = { pGEP->getOperand(1), gepArg1 };
            Type *BaseTy = newAlloca->getAllocatedType();
            llvm::Value* pGEPnew = GetElementPtrInst::Create(BaseTy, newAlloca, gepArg, "", pGEP);

            pGEP->replaceAllUsesWith(pGEPnew);
        }
    }
}

void CustomSafeOptPass::visitLoadInst(LoadInst& load)
{
    // Optimize indirect access to private arrays. Handle cases where
    // array index is a select between two immediate constant values.
    // After the optimization there is fair chance the alloca will be
    // promoted to registers.
    //
    // E.g. change the following:
    // %PrivareArray = alloca[4 x <3 x float>], align 16
    // %IndirectIndex = select i1 %SomeCondition, i32 1, i32 2
    // %IndirectAccessPtr= getelementptr[4 x <3 x float>], [4 x <3 x float>] * %PrivareArray, i32 0, i32 %IndirectIndex
    // %LoadedValue = load <3 x float>, <3 x float>* %IndirectAccess, align 16

    // %PrivareArray = alloca[4 x <3 x float>], align 16
    // %DirectAccessPtr1 = getelementptr[4 x <3 x float>], [4 x <3 x float>] * %PrivareArray, i32 0, i32 1
    // %DirectAccessPtr2 = getelementptr[4 x <3 x float>], [4 x <3 x float>] * %PrivareArray, i32 0, i32 2
    // %LoadedValue1 = load <3 x float>, <3 x float>* %DirectAccessPtr1, align 16
    // %LoadedValue2 = load <3 x float>, <3 x float>* %DirectAccessPtr2, align 16
    // %LoadedValue = select i1 %SomeCondition, <3 x float> %LoadedValue1, <3 x float> %LoadedValue2

    Value* ptr = load.getPointerOperand();
    if (ptr->getType()->getPointerAddressSpace() != 0)
    {
        // only private arrays are handled
        return;
    }
    if (GetElementPtrInst * gep = dyn_cast<GetElementPtrInst>(ptr))
    {
        bool found = false;
        uint selIdx = 0;
        // Check if this gep is a good candidate for optimization.
        // The instruction has to have exactly one non-constant index value.
        // The index value has to be a select instruction with immediate
        // constant values.
        for (uint i = 0; i < gep->getNumIndices(); ++i)
        {
            Value* gepIdx = gep->getOperand(i + 1);
            if (!isa<ConstantInt>(gepIdx))
            {
                SelectInst* sel = dyn_cast<SelectInst>(gepIdx);
                if (!found &&
                    sel &&
                    isa<ConstantInt>(sel->getOperand(1)) &&
                    isa<ConstantInt>(sel->getOperand(2)))
                {
                    found = true;
                    selIdx = i;
                }
                else
                {
                    found = false; // optimize cases with only a single non-constant index.
                    break;
                }
            }
        }
        if (found)
        {
            SelectInst* sel = cast<SelectInst>(gep->getOperand(selIdx + 1));
            SmallVector<Value*, 8> indices;
            indices.append(gep->idx_begin(), gep->idx_end());
            indices[selIdx] = sel->getOperand(1);
            Type *BaseTy = gep->getSourceElementType();
            GetElementPtrInst* gep1 = GetElementPtrInst::Create(BaseTy, gep->getPointerOperand(), indices, gep->getName(), gep);
            gep1->setDebugLoc(gep->getDebugLoc());
            indices[selIdx] = sel->getOperand(2);
            GetElementPtrInst* gep2 = GetElementPtrInst::Create(BaseTy, gep->getPointerOperand(), indices, gep->getName(), gep);
            gep2->setDebugLoc(gep->getDebugLoc());
            LoadInst* load1 = cast<LoadInst>(load.clone());
            load1->insertBefore(&load);
            load1->setOperand(0, gep1);
            LoadInst* load2 = cast<LoadInst>(load.clone());
            load2->insertBefore(&load);
            load2->setOperand(0, gep2);
            SelectInst* result = SelectInst::Create(sel->getCondition(), load1, load2, load.getName(), &load);
            result->setDebugLoc(load.getDebugLoc());
            load.replaceAllUsesWith(result);
            load.eraseFromParent();
            if (gep->use_empty())
            {
                gep->eraseFromParent();
            }
            if (sel->use_empty())
            {
                sel->eraseFromParent();
            }
        }
    }
}

void CustomSafeOptPass::visitCallInst(CallInst& C)
{
    // discard optimizations
    if (llvm::GenIntrinsicInst * inst = llvm::dyn_cast<GenIntrinsicInst>(&C))
    {
        GenISAIntrinsic::ID id = inst->getIntrinsicID();
        // try to prune the destination size
        switch (id)
        {

        case GenISAIntrinsic::GenISA_bfi:
        {
            visitBfi(inst);
            break;
        }

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
            visitLdptr(llvm::cast<llvm::SamplerLoadIntrinsic>(inst));
            break;
        }

        case GenISAIntrinsic::GenISA_ldrawvector_indexed:
        {
            visitLdRawVec(inst);
            break;
        }

        case GenISAIntrinsic::GenISA_WaveShuffleIndex:
        {
            visitShuffleIndex(inst);
            break;
        }

        case GenISAIntrinsic::GenISA_dp4a_ss:
        case GenISAIntrinsic::GenISA_dp4a_uu:
        {
            mergeDotAddToDp4a(&C);
            break;
        }

        case GenISAIntrinsic::GenISA_LSC2DBlockPrefetch:
        {
            visitLSC2DBlockPrefetch(inst);
            break;
        }

        default:
            break;
        }
    }
}

void CustomSafeOptPass::visitSelectInst(SelectInst& I)
{
    // select i1 %x, 1, %y ->  or i1 %x, %y
    // select i1 %x, %y, 0 -> and i1 %x, %y

    using namespace llvm::PatternMatch;
    llvm::IRBuilder<> Builder(&I);

    if (I.getType()->isIntegerTy(1) && match(&I, m_Select(m_Value(), m_SpecificInt(1), m_Value())))
    {
        auto NewVal = Builder.CreateOr(I.getOperand(0), I.getOperand(2));
        Instruction *OrInst = dyn_cast<Instruction>(NewVal);
        if (OrInst)
        {
            OrInst->setDebugLoc(I.getDebugLoc());
        }
        I.replaceAllUsesWith(NewVal);
    }

    if (I.getType()->isIntegerTy(1) && match(&I, m_Select(m_Value(), m_Value(), m_SpecificInt(0))))
    {
        auto NewVal = Builder.CreateAnd(I.getOperand(0), I.getOperand(1));
        Instruction *AndInst = dyn_cast<Instruction>(NewVal);
        if (AndInst)
        {
            AndInst->setDebugLoc(I.getDebugLoc());
        }
        I.replaceAllUsesWith(NewVal);
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

    if (BitCastInst * finalBitCast = dyn_cast<BitCastInst>(*(addInst->user_begin())))
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
    Type* halfx2 = IGCLLVM::FixedVectorType::get(Type::getHalfTy(builder.getContext()), 2);

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
    IGC_ASSERT(inst->getType()->isIntegerTy(32));
    ConstantInt* widthV = dyn_cast<ConstantInt>(inst->getOperand(0));
    ConstantInt* offsetV = dyn_cast<ConstantInt>(inst->getOperand(1));
    if (widthV && offsetV)
    {
        // transformation is beneficial if src3 is constant or if the offset is zero
        if (isa<ConstantInt>(inst->getOperand(3)) || offsetV->isZero())
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
    else if (widthV && widthV->isZeroValue())
    {
        inst->replaceAllUsesWith(inst->getOperand(3));
        inst->eraseFromParent();
    }
}

void CustomSafeOptPass::visitMulH(CallInst* inst, bool isSigned)
{
    ConstantInt* src0 = dyn_cast<ConstantInt>(inst->getOperand(0));
    ConstantInt* src1 = dyn_cast<ConstantInt>(inst->getOperand(1));
    if (src0 && src1)
    {
        unsigned nbits = inst->getType()->getIntegerBitWidth();
        IGC_ASSERT(nbits < 64);

        if (isSigned)
        {
            int64_t si0 = src0->getSExtValue();
            int64_t si1 = src1->getSExtValue();
            int64_t r = ((si0 * si1) >> nbits);
            inst->replaceAllUsesWith(ConstantInt::get(inst->getType(), r, true));
        }
        else
        {
            uint64_t ui0 = src0->getZExtValue();
            uint64_t ui1 = src1->getZExtValue();
            uint64_t r = ((ui0 * ui1) >> nbits);
            inst->replaceAllUsesWith(ConstantInt::get(inst->getType(), r));
        }
        inst->eraseFromParent();
    }
}

// if phi is used in a FPTrunc and the sources all come from fpext we can skip the conversions
void CustomSafeOptPass::visitFPTruncInst(FPTruncInst& I)
{
    if (PHINode * phi = dyn_cast<PHINode>(I.getOperand(0)))
    {
        bool foundPattern = true;
        unsigned int numSrc = phi->getNumIncomingValues();
        SmallVector<Value*, 6> newSources(numSrc);
        for (unsigned int i = 0; i < numSrc; i++)
        {
            FPExtInst* source = dyn_cast<FPExtInst>(phi->getIncomingValue(i));
            if (source && source->getOperand(0)->getType() == I.getType())
            {
                newSources[i] = source->getOperand(0);
            }
            else
            {
                foundPattern = false;
                break;
            }
        }
        if (foundPattern)
        {
            PHINode* newPhi = PHINode::Create(I.getType(), numSrc, "", phi);
            for (unsigned int i = 0; i < numSrc; i++)
            {
                newPhi->addIncoming(newSources[i], phi->getIncomingBlock(i));
            }
            // Debug info location must be preserved in a new phi instruction
            const DebugLoc& DL = I.getDebugLoc();
            if (Instruction* newPhiInst = dyn_cast<Instruction>(newPhi))
                newPhiInst->setDebugLoc(DL);
            I.replaceAllUsesWith(newPhi);
            I.eraseFromParent();
            // if phi has other uses we add a fpext to avoid having two phi
            if (!phi->use_empty())
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
    if (llvm::IntrinsicInst * intrinsicInst = llvm::dyn_cast<llvm::IntrinsicInst>(FPUII.getOperand(0)))
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
void CustomSafeOptPass::visitBitCast(BitCastInst& BC)
{
    if (SelectInst * sel = dyn_cast<SelectInst>(BC.getOperand(0)))
    {
        BitCastInst* trueVal = dyn_cast<BitCastInst>(sel->getTrueValue());
        BitCastInst* falseVal = dyn_cast<BitCastInst>(sel->getFalseValue());
        if (trueVal && falseVal)
        {
            Value* trueValOrignalType = trueVal->getOperand(0);
            Value* falseValOrignalType = falseVal->getOperand(0);
            if (trueValOrignalType->getType() == BC.getType() &&
                falseValOrignalType->getType() == BC.getType())
            {
                Value* cond = sel->getCondition();
                Value* newVal = SelectInst::Create(cond, trueValOrignalType, falseValOrignalType, "", sel);

                const DebugLoc& DL = BC.getDebugLoc();
                if (Instruction* newValInst = dyn_cast<Instruction>(newVal))
                    newValInst->setDebugLoc(DL);

                BC.replaceAllUsesWith(newVal);
                BC.eraseFromParent();
            }
        }
    }
    else if (PHINode * phi = dyn_cast<PHINode>(BC.getOperand(0)))
    {
        if (phi->hasOneUse())
        {
            bool foundPattern = true;
            unsigned int numSrc = phi->getNumIncomingValues();
            SmallVector<Value*, 6> newSources(numSrc);
            for (unsigned int i = 0; i < numSrc; i++)
            {
                BitCastInst* source = dyn_cast<BitCastInst>(phi->getIncomingValue(i));
                if (source && source->getOperand(0)->getType() == BC.getType())
                {
                    newSources[i] = source->getOperand(0);
                }
                else if (Constant * C = dyn_cast<Constant>(phi->getIncomingValue(i)))
                {
                    newSources[i] = ConstantExpr::getCast(Instruction::BitCast, C, BC.getType());
                }
                else
                {
                    foundPattern = false;
                    break;
                }
            }
            if (foundPattern)
            {
                PHINode* newPhi = PHINode::Create(BC.getType(), numSrc, "", phi);
                for (unsigned int i = 0; i < numSrc; i++)
                {
                    newPhi->addIncoming(newSources[i], phi->getIncomingBlock(i));
                }
                const DebugLoc& DL = BC.getDebugLoc();
                // Debug info location must be preserved on a resulting select and phi
                if (Instruction* newPhiInst = dyn_cast<Instruction>(newPhi))
                    newPhiInst->setDebugLoc(DL);
                BC.replaceAllUsesWith(newPhi);
                BC.eraseFromParent();
            }
        }
    }
}

/*
Search for DP4A pattern, e.g.:

%14 = load i32, i32 addrspace(1)* %13, align 4 <---- c variable (accumulator)
// Instructions to be matched:
%conv.i.i4 = sext i8 %scalar35 to i32
%conv.i7.i = sext i8 %scalar39 to i32
%mul.i = mul nsw i32 %conv.i.i4, %conv.i7.i
%conv.i6.i = sext i8 %scalar36 to i32
%conv.i5.i = sext i8 %scalar40 to i32
%mul4.i = mul nsw i32 %conv.i6.i, %conv.i5.i
%add.i = add nsw i32 %mul.i, %mul4.i
%conv.i4.i = sext i8 %scalar37 to i32
%conv.i3.i = sext i8 %scalar41 to i32
%mul7.i = mul nsw i32 %conv.i4.i, %conv.i3.i
%add8.i = add nsw i32 %add.i, %mul7.i
%conv.i2.i = sext i8 %scalar38 to i32
%conv.i1.i = sext i8 %scalar42 to i32
%mul11.i = mul nsw i32 %conv.i2.i, %conv.i1.i
%add12.i = add nsw i32 %add8.i, %mul11.i
%add13.i = add nsw i32 %14, %add12.i
// end matched instructions
store i32 %add13.i, i32 addrspace(1)* %18, align 4

=>

%14 = load i32, i32 addrspace(1)* %13, align 4
%15 = insertelement <4 x i8> undef, i8 %scalar35, i64 0
%16 = insertelement <4 x i8> undef, i8 %scalar39, i64 0
%17 = insertelement <4 x i8> %15, i8 %scalar36, i64 1
%18 = insertelement <4 x i8> %16, i8 %scalar40, i64 1
%19 = insertelement <4 x i8> %17, i8 %scalar37, i64 2
%20 = insertelement <4 x i8> %18, i8 %scalar41, i64 2
%21 = insertelement <4 x i8> %19, i8 %scalar38, i64 3
%22 = insertelement <4 x i8> %20, i8 %scalar42, i64 3
%23 = bitcast <4 x i8> %21 to i32
%24 = bitcast <4 x i8> %22 to i32
%25 = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 %14, i32 %23, i32 %24)
...
store i32 %25, i32 addrspace(1)* %29, align 4
*/
void CustomSafeOptPass::matchDp4a(BinaryOperator &I) {
    using namespace llvm::PatternMatch;

    if (I.getOpcode() != Instruction::Add) return;
    CodeGenContext* Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (!Ctx->platform.hasHWDp4AddSupport()) return;

    static constexpr int NUM_DP4A_COMPONENTS = 4;

    // Holds sext/zext instructions.
    std::array<Instruction *, NUM_DP4A_COMPONENTS> ExtA{}, ExtB{};
    // Holds i8 values that were multiplied.
    std::array<Value *, NUM_DP4A_COMPONENTS> ArrA{}, ArrB{};
    // Holds the accumulator.
    Value* AccVal = nullptr;
    IRBuilder<> Builder(&I);

    // Enum to check if given branch is signed or unsigned, e.g.
    // comes from SExt or ZExt value.
    enum class OriginSignedness { originSigned, originUnsigned, originUnknown };

    // Note: the returned pattern from this lambda doesn't use m_ZExtOrSExt pattern on purpose -
    // There is no way to bind to both instruction and it's arguments, so we bind to instruction and
    // check the opcode later.
    auto getMulPat = [&](int i) { return m_c_Mul(m_Instruction(ExtA[i]), m_Instruction(ExtB[i])); };
    auto getAdd4Pat = [&](const auto& pat0, const auto& pat1, const auto& pat2, const auto& pat3) {
      return m_c_Add(m_c_Add(m_c_Add(pat0, pat1), pat2), pat3);
    };
    auto getAdd5Pat = [&](const auto& pat0, const auto& pat1, const auto& pat2, const auto& pat3, const auto& pat4) {
      return m_c_Add(getAdd4Pat(pat0, pat1, pat2, pat3), pat4);
    };
    auto PatternAccAtBeginning = getAdd5Pat(m_Value(AccVal), getMulPat(0), getMulPat(1), getMulPat(2), getMulPat(3));
    auto PatternAccAtEnd = getAdd5Pat(getMulPat(0), getMulPat(1), getMulPat(2), getMulPat(3), m_Value(AccVal));
    auto PatternNoAcc = getAdd4Pat(getMulPat(0), getMulPat(1), getMulPat(2), getMulPat(3));

    if (!match(&I, PatternAccAtEnd) && !match(&I, PatternAccAtBeginning)) {
      if (match(&I, PatternNoAcc)) {
        AccVal = Builder.getInt32(0);
      } else {
        return;
      }
    }

    // Check if values in A and B all have the same extension type (sext/zext)
    // and that they come from i8 type. A and B extension types can be
    // different.
    auto checkIfValuesComeFromCharType = [](auto &range,
                                            OriginSignedness &retSign) {
      IGC_ASSERT_MESSAGE((range.begin() != range.end()),
                         "Cannot check empty collection.");

      auto shr24Pat = m_Shr(m_Value(), m_SpecificInt(24));
      auto and255Pat = m_And(m_Value(), m_SpecificInt(255));

      IGC_ASSERT_MESSAGE(range.size() == NUM_DP4A_COMPONENTS,
                         "Range too big in dp4a pattern match!");
      std::array<OriginSignedness, NUM_DP4A_COMPONENTS> signs;
      int counter = 0;
      for (auto I : range) {
        if (!I->getType()->isIntegerTy(32)) {
          return false;
        }

        switch (I->getOpcode()) {
        case Instruction::SExt:
        case Instruction::ZExt:
          if (!I->getOperand(0)->getType()->isIntegerTy(8)) {
            return false;
          }
          signs[counter] = (I->getOpcode() == Instruction::SExt)
                               ? OriginSignedness::originSigned
                               : OriginSignedness::originUnsigned;
          break;
        case Instruction::AShr:
        case Instruction::LShr:
          if (!match(I, shr24Pat)) {
            return false;
          }
          signs[counter] = (I->getOpcode() == Instruction::AShr)
                               ? OriginSignedness::originSigned
                               : OriginSignedness::originUnsigned;
          break;
        case Instruction::And:
          if (!match(I, and255Pat)) {
            return false;
          }
          signs[counter] = OriginSignedness::originUnsigned;
          break;
        default:
          return false;
        }

        counter++;
      }

      // check if all have the same sign.
      retSign = signs[0];
      return std::all_of(
          signs.begin(), signs.end(),
          [&signs](OriginSignedness v) { return v == signs[0]; });
    };

    OriginSignedness ABranch = OriginSignedness::originUnknown, BBranch = OriginSignedness::originUnknown;
    bool canMatch = AccVal->getType()->isIntegerTy(32) && checkIfValuesComeFromCharType(ExtA, ABranch) && checkIfValuesComeFromCharType(ExtB, BBranch);
    if (!canMatch) return;

    GenISAIntrinsic::ID IntrinsicID{};

    static_assert(ExtA.size() == ArrA.size() && ExtB.size() == ArrB.size() && ExtA.size() == NUM_DP4A_COMPONENTS, "array sizes must match!");

    auto getInt8Origin = [&](Instruction *I) {
      if (I->getOpcode() == Instruction::SExt ||
          I->getOpcode() == Instruction::ZExt) {
        return I->getOperand(0);
      }
      Builder.SetInsertPoint(I->getNextNode());
      return Builder.CreateTrunc(I, Builder.getInt8Ty());
    };

    std::transform(ExtA.begin(), ExtA.end(), ArrA.begin(), getInt8Origin);
    std::transform(ExtB.begin(), ExtB.end(), ArrB.begin(), getInt8Origin);

    switch (ABranch) {
    case OriginSignedness::originSigned:
      IntrinsicID = (BBranch == OriginSignedness::originSigned)
                        ? GenISAIntrinsic::GenISA_dp4a_ss
                        : GenISAIntrinsic::GenISA_dp4a_su;
      break;
    case OriginSignedness::originUnsigned:
      IntrinsicID = (BBranch == OriginSignedness::originSigned)
                        ? GenISAIntrinsic::GenISA_dp4a_us
                        : GenISAIntrinsic::GenISA_dp4a_uu;
      break;
    default:
      IGC_ASSERT(0);
    }

    // Additional optimisation: check if the values come from an ExtractElement instruction.
    // If this is the case, reorder the elements in the array to match the ExtractElement pattern.
    // This way we avoid potential shufflevector instructions which cause additional mov instructions in final asm.
    // Note: indices in ExtractElement do not have to be in range [0-3], they can be greater. We just want to have them ordered ascending.
    auto extractElementOrderOpt = [&](std::array<Value*, NUM_DP4A_COMPONENTS> & Arr) {
      bool CanOptOrder = true;
      llvm::SmallPtrSet<Value*, NUM_DP4A_COMPONENTS> OriginValues;
      std::map<int64_t, Value*> IndexMap;
      for (int i = 0; i < NUM_DP4A_COMPONENTS; ++i) {
        ConstantInt* IndexVal = nullptr;
        Value* OriginVal = nullptr;
        auto P = m_ExtractElt(m_Value(OriginVal), m_ConstantInt(IndexVal));
        if (!match(Arr[i], P)) {
          CanOptOrder = false;
          break;
        }
        OriginValues.insert(OriginVal);
        IndexMap.insert({ IndexVal->getSExtValue(), Arr[i] });
      }

      if (CanOptOrder && OriginValues.size() == 1 && IndexMap.size() == NUM_DP4A_COMPONENTS) {
        int i = 0;
        for(auto &El : IndexMap) {
          Arr[i++] = El.second;
        }
      }
    };
    extractElementOrderOpt(ArrA);
    extractElementOrderOpt(ArrB);

    Builder.SetInsertPoint(I.getNextNode());

    Value* VectorA = UndefValue::get(IGCLLVM::FixedVectorType::get(Builder.getInt8Ty(), NUM_DP4A_COMPONENTS));
    Value* VectorB = UndefValue::get(IGCLLVM::FixedVectorType::get(Builder.getInt8Ty(), NUM_DP4A_COMPONENTS));
    for (int i = 0; i < NUM_DP4A_COMPONENTS; ++i) {
      VectorA = Builder.CreateInsertElement(VectorA, ArrA[i], i);
      VectorB = Builder.CreateInsertElement(VectorB, ArrB[i], i);
    }
    Value* ValA = Builder.CreateBitCast(VectorA, Builder.getInt32Ty());
    Value* ValB = Builder.CreateBitCast(VectorB, Builder.getInt32Ty());

    Function* Dp4aFun = GenISAIntrinsic::getDeclaration(I.getModule(), IntrinsicID, Builder.getInt32Ty());
    Value* Res = Builder.CreateCall(Dp4aFun, { AccVal, ValA, ValB });
    I.replaceAllUsesWith(Res);
}

void CustomSafeOptPass::hoistDp3(BinaryOperator& I)
{
    if (I.getOpcode() != Instruction::BinaryOps::FAdd)
    {
        return;
    }

    using namespace PatternMatch;

    Value* X1 = nullptr;
    Value* Y1 = nullptr;
    Value* Z1 = nullptr;
    Value* X2 = nullptr;
    Value* Y2 = nullptr;
    Value* Z2 = nullptr;

    // dp3
    bool isdp3 = match(cast<Instruction>(&I),
                        m_FAdd(
                            m_FMul(m_Value(Z1), m_Value(Z2)),
                            m_FAdd(
                                m_FMul(m_Value(X1), m_Value(X2)),
                                m_FMul(m_Value(Y1), m_Value(Y2)))));

    // need to try matching for when fadd and fmul are switched
    if (!isdp3)
    {
        isdp3 = match(cast<Instruction>(&I),
                    m_FAdd(
                        m_FAdd(
                            m_FMul(m_Value(X1), m_Value(X2)),
                            m_FMul(m_Value(Y1), m_Value(Y2))),
                        m_FMul(m_Value(Z1), m_Value(Z2))));
    }

    if (isdp3)
    {
        // quick check to verify that all source instructions exist in the same basic block.
        // that way we don't need to iterate through the basic block if we don't need to.
        BasicBlock* currentBB = I.getParent();
        unsigned numSourcesAreInst = 0;
        if (auto I = dyn_cast<Instruction>(X1))
        {
            if (I->getParent() == currentBB)
                numSourcesAreInst++;
            else
                return;
        }
        if (auto I = dyn_cast<Instruction>(X2))
        {
            if (I->getParent() == currentBB)
                numSourcesAreInst++;
            else
                return;
        }
        if (auto I = dyn_cast<Instruction>(Y1))
        {
            if (I->getParent() == currentBB)
                numSourcesAreInst++;
            else
                return;
        }
        if (auto I = dyn_cast<Instruction>(Y2))
        {
            if (I->getParent() == currentBB)
                numSourcesAreInst++;
            else
                return;
        }
        if (auto I = dyn_cast<Instruction>(Z1))
        {
            if (I->getParent() == currentBB)
                numSourcesAreInst++;
            else
                return;
        }
        if (auto I = dyn_cast<Instruction>(Z2))
        {
            if (I->getParent() == currentBB)
                numSourcesAreInst++;
            else
                return;
        }

        // find the last source as it appears in the shader.
        // needed for hoist location
        Value* lastSource = nullptr;
        unsigned char i = 0;
        for (BasicBlock::iterator inst = I.getParent()->begin(); inst != I.getParent()->end(); inst++)
        {
            // break if we found the last source
            // no need to iterate rest of BB
            if (i == numSourcesAreInst)
                break;
            if (&(*inst) == X1)
            {
                lastSource = X1;
                i++;
            }
            else if (&(*inst) == Y1)
            {
                lastSource = Y1;
                i++;
            }
            else if (&(*inst) == Z1)
            {
                lastSource = Z1;
                i++;
            }
            else if (&(*inst) == X2)
            {
                lastSource = X2;
                i++;
            }
            else if (&(*inst) == Y2)
            {
                lastSource = Y2;
                i++;
            }
            else if (&(*inst) == Z2)
            {
                lastSource = Z2;
                i++;
            }
        }

        // Obtain each instruction of dp3
        Instruction* fmul_x = nullptr;
        Instruction* fmul_y = nullptr;
        Instruction* fadd_xy = nullptr;
        Instruction* fmul_z = nullptr;

        Instruction* t1 = cast<Instruction>(I.getOperand(0));
        Instruction* t2 = cast<Instruction>(I.getOperand(1));

        if (t1->getOperand(0) == Z1 || t1->getOperand(0) == Z2)
        {
            fmul_z = t1;
            fadd_xy = t2;
        }
        else
        {
            fmul_z = t2;
            fadd_xy = t1;
        }

        t1 = cast<Instruction>(fadd_xy->getOperand(0));
        t2 = cast<Instruction>(fadd_xy->getOperand(1));

        if (t1->getOperand(0) == X1 || t1->getOperand(0) == X2)
        {
            fmul_x = t1;
            fmul_y = t2;
        }
        else
        {
            fmul_x = t2;
            fmul_y = t1;
        }

        fmul_x->moveAfter(cast<Instruction>(lastSource));
        fmul_y->moveAfter(fmul_x);
        fadd_xy->moveAfter(fmul_y);
        fmul_z->moveAfter(fadd_xy);
        I.moveAfter(fmul_z);
    }
}

bool CustomSafeOptPass::isEmulatedAdd(BinaryOperator& I)
{
    if (I.getOpcode() == Instruction::Or)
    {
        if (BinaryOperator * OrOp0 = dyn_cast<BinaryOperator>(I.getOperand(0)))
        {
            if (OrOp0->getOpcode() == Instruction::Shl)
            {
                // Check the SHl. If we have a constant Shift Left val then we can check
                // it to see if it is emulating an add.
                if (ConstantInt * pConstShiftLeft = dyn_cast<ConstantInt>(OrOp0->getOperand(1)))
                {
                    if (ConstantInt * pConstOrVal = dyn_cast<ConstantInt>(I.getOperand(1)))
                    {
                        int const_shift = int_cast<int>(pConstShiftLeft->getZExtValue());
                        int const_or_val = int_cast<int>(pConstOrVal->getSExtValue());
                        if ((1 << const_shift) > abs(const_or_val))
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
                if (ConstantInt * pConstMul = dyn_cast<ConstantInt>(OrOp0->getOperand(1)))
                {
                    if (ConstantInt * pConstOrVal = dyn_cast<ConstantInt>(I.getOperand(1)))
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

// Attempt to create new float instruction if both operands are from FPTruncInst instructions.
// Example with fadd:
//  %Temp-31.prec.i = fptrunc float %34 to half
//  %Temp-30.prec.i = fptrunc float %33 to half
//  %41 = fadd fast half %Temp-31.prec.i, %Temp-30.prec.i
//  %Temp-32.i = fpext half %41 to float
//
//  This fadd is used as a float, and doesn't need the operands to be cased to half.
//  We can remove the extra casts in this case.
//  This becomes:
//  %41 = fadd fast float %34, %33
// Can also do matches with fadd/fmul that will later become an mad instruction.
// mad example:
//  %.prec70.i = fptrunc float %273 to half
//  %.prec78.i = fptrunc float %276 to half
//  %279 = fmul fast half %233, %.prec70.i
//  %282 = fadd fast half %279, %.prec78.i
//  %.prec84.i = fpext half %282 to float
// This becomes:
//  %279 = fpext half %233 to float
//  %280 = fmul fast float %273, %279
//  %281 = fadd fast float %280, %276
void CustomSafeOptPass::removeHftoFCast(Instruction& I)
{
    if (!I.getType()->isFloatingPointTy())
        return;

    // Check if the only user is a FPExtInst
    if (!I.hasOneUse())
        return;

    // Check if this instruction is used in a single FPExtInst
    FPExtInst* castInst = NULL;
    User* U = *I.user_begin();
    if (FPExtInst* inst = dyn_cast<FPExtInst>(U))
    {
        if (inst->getType()->isFloatTy())
        {
            castInst = inst;
        }
    }
    if (!castInst)
      return;

    // Check for fmad pattern
    if (I.getOpcode() == Instruction::FAdd)
    {
        Value* src0 = nullptr, * src1 = nullptr, * src2 = nullptr;

        // CodeGenPatternMatch::MatchMad matches the first fmul.
        Instruction* fmulInst = nullptr;
        for (uint i = 0; i < 2; i++)
        {
            fmulInst = dyn_cast<Instruction>(I.getOperand(i));
            if (fmulInst && fmulInst->getOpcode() == Instruction::FMul)
            {
                src0 = fmulInst->getOperand(0);
                src1 = fmulInst->getOperand(1);
                src2 = I.getOperand(1 - i);
                break;
            }
            else
            {
                // Prevent other non-fmul instructions from getting used
                fmulInst = nullptr;
            }
        }
        if (fmulInst)
        {
            // Used to get the new float operands for the new instructions
            auto getFloatValue = [](Value* operand, Instruction* I, Type* type)
            {
                if (FPTruncInst* inst = dyn_cast<FPTruncInst>(operand))
                {
                    // Use the float input of the FPTrunc
                    if (inst->getOperand(0)->getType()->isFloatTy())
                    {
                        return inst->getOperand(0);
                    }
                    else
                    {
                        return (Value*)NULL;
                    }
                }
                else if (Instruction* inst = dyn_cast<Instruction>(operand))
                {
                    // Cast the result of this operand to a float
                    return dyn_cast<Value>(new FPExtInst(inst, type, "", I));
                }
                return (Value*)NULL;
            };

            int convertCount = 0;
            if (dyn_cast<FPTruncInst>(src0))
                convertCount++;
            if (dyn_cast<FPTruncInst>(src1))
                convertCount++;
            if (dyn_cast<FPTruncInst>(src2))
                convertCount++;
            if (convertCount >= 2)
            {
                // Conversion for the hf values
                auto floatTy = castInst->getType();
                src0 = getFloatValue(src0, fmulInst, floatTy);
                src1 = getFloatValue(src1, fmulInst, floatTy);
                src2 = getFloatValue(src2, &I, floatTy);

                if (!src0 || !src1 || !src2)
                    return;

                // Create new float fmul and fadd instructions
                Value* newFmul = BinaryOperator::Create(Instruction::FMul, src0, src1, "", &I);
                Value* newFadd = BinaryOperator::Create(Instruction::FAdd, newFmul, src2, "", &I);

                // Copy fast math flags
                Instruction* fmulInst = dyn_cast<Instruction>(newFmul);
                Instruction* faddInst = dyn_cast<Instruction>(newFadd);
                fmulInst->copyFastMathFlags(fmulInst);
                faddInst->copyFastMathFlags(&I);
                faddInst->setDebugLoc(castInst->getDebugLoc());

                castInst->replaceAllUsesWith(faddInst);
                return;
            }
        }
    }

    // Check if operands come from a Float to HF Cast
    Value *S1 = NULL, *S2 = NULL;
    if (FPTruncInst* inst = dyn_cast<FPTruncInst>(I.getOperand(0)))
    {
        if (!inst->getType()->isHalfTy())
          return;
        S1 = inst->getOperand(0);
    }
    if (FPTruncInst* inst = dyn_cast<FPTruncInst>(I.getOperand(1)))
    {
        if (!inst->getType()->isHalfTy())
          return;
        S2 = inst->getOperand(0);
    }
    if (!S1 || !S2)
    {
        return;
    }

    Value* newInst = NULL;
    if (BinaryOperator* bo = dyn_cast<BinaryOperator>(&I))
    {
        newInst = BinaryOperator::Create(bo->getOpcode(), S1, S2, "", &I);
        Instruction* inst = dyn_cast<Instruction>(newInst);
        inst->copyFastMathFlags(&I);
        inst->setDebugLoc(castInst->getDebugLoc());
        castInst->replaceAllUsesWith(inst);
    }
}

void CustomSafeOptPass::visitBinaryOperator(BinaryOperator& I)
{
    matchDp4a(I);

    CodeGenContext* pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    // move immediate value in consecutive integer adds to the last added value.
    // this can allow more chance of doing CSE and memopt.
    //    a = b + 8
    //    d = a + c
    //        to
    //    a = b + c
    //    d = a + 8

    // Before WA if() as it's validated behavior.
    if (I.getType()->isIntegerTy() && I.getOpcode() == Instruction::Or)
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

    if (I.getType()->isIntegerTy())
    {
        if ((I.getOpcode() == Instruction::Add || isEmulatedAdd(I)) &&
            I.hasOneUse())
        {
            ConstantInt* src0imm = dyn_cast<ConstantInt>(I.getOperand(0));
            ConstantInt* src1imm = dyn_cast<ConstantInt>(I.getOperand(1));
            if (src0imm || src1imm)
            {
                llvm::Instruction* nextInst = llvm::dyn_cast<llvm::Instruction>(*(I.user_begin()));
                if (nextInst && nextInst->getOpcode() == Instruction::Add)
                {
                    // do not apply if any add instruction has NSW flag since we can't save it
                    if ((isa<OverflowingBinaryOperator>(I) && I.hasNoSignedWrap()) || nextInst->hasNoSignedWrap())
                        return;
                    ConstantInt* secondSrc0imm = dyn_cast<ConstantInt>(nextInst->getOperand(0));
                    ConstantInt* secondSrc1imm = dyn_cast<ConstantInt>(nextInst->getOperand(1));
                    // found 2 add instructions to swap srcs
                    if (!secondSrc0imm && !secondSrc1imm && nextInst->getOperand(0) != nextInst->getOperand(1))
                    {
                        for (int i = 0; i < 2; i++)
                        {
                            if (nextInst->getOperand(i) == &I)
                            {
                                Value* newAdd = BinaryOperator::CreateAdd(src0imm ? I.getOperand(1) : I.getOperand(0), nextInst->getOperand(1 - i), "", nextInst);

                                // Conservatively clear the NSW NUW flags, since they may not be
                                // preserved by the reassociation.
                                bool IsNUW = isa<OverflowingBinaryOperator>(I) && I.hasNoUnsignedWrap() && nextInst->hasNoUnsignedWrap();
                                cast<BinaryOperator>(newAdd)->setHasNoUnsignedWrap(IsNUW);
                                nextInst->setHasNoUnsignedWrap(IsNUW);
                                nextInst->setHasNoSignedWrap(false);

                                nextInst->setOperand(0, newAdd);
                                nextInst->setOperand(1, I.getOperand(src0imm ? 0 : 1));
                                break;
                            }
                        }
                    }
                }
            }
        }
    } else if (I.getType()->isFloatingPointTy()) {
        removeHftoFCast(I);
    }

    if (IGC_IS_FLAG_ENABLED(ForceHoistDp3) || (!pContext->m_retryManager.IsFirstTry() && IGC_IS_FLAG_ENABLED(EnableHoistDp3)))
    {
        hoistDp3(I);
    }
}

void IGC::CustomSafeOptPass::visitLdptr(llvm::SamplerLoadIntrinsic* inst)
{
    if (!IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllTextures) &&
        !IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllTypedBuffers))
    {
        return;
    }

    // Sampler feedback (GenISA_evaluateSampler) supports GenISA_ldptr and not GenISA_typedread
    llvm::Value* pairedTexture = inst->getPairedTextureValue();
    if (pairedTexture != nullptr && !isa<UndefValue>(pairedTexture))
    {
        return;
    }
    // change
    // % 10 = call fast <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p196608v4f32(i32 %_s1.i, i32 %_s14.i, i32 0, i32 0, <4 x float> addrspace(196608)* null, i32 0, i32 0, i32 0), !dbg !123
    // to
    // % 10 = call fast <4 x float> @llvm.genx.GenISA.typedread.p196608v4f32(<4 x float> addrspace(196608)* null, i32 %_s1.i, i32 %_s14.i, i32 0, i32 0), !dbg !123
    // when the index comes directly from threadid

    Constant* src1 = dyn_cast<Constant>(inst->getOperand(1));
    Constant* src2 = dyn_cast<Constant>(inst->getOperand(2));
    Constant* src3 = dyn_cast<Constant>(inst->getOperand(3));

    // src2 and src3 has to be zero
    if (!src2 || !src3 || !src2->isZeroValue() || !src3->isZeroValue())
    {
        return;
    }

    // if only doing the opt on buffers, make sure src1 is zero too
    if (!IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllTextures) &&
        IGC_IS_FLAG_ENABLED(UseHDCTypedReadForAllTypedBuffers))
    {
        if (!src1 || !src1->isZeroValue())
            return;
    }

    // do the transformation
    llvm::IRBuilder<> builder(inst);
    Module* M = inst->getParent()->getParent()->getParent();

    Function* pLdIntrinsic = llvm::GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_typedread,
        inst->getOperand(4)->getType());

    SmallVector<Value*, 5> ld_FunctionArgList(5);
    ld_FunctionArgList[0] = inst->getTextureValue();
    ld_FunctionArgList[1] = builder.CreateAdd(inst->getOperand(0), inst->getOperand(inst->getTextureIndex() + 1));
    ld_FunctionArgList[2] = builder.CreateAdd(inst->getOperand(1), inst->getOperand(inst->getTextureIndex() + 2));
    ld_FunctionArgList[3] = builder.CreateAdd(inst->getOperand(3), inst->getOperand(inst->getTextureIndex() + 3));
    ld_FunctionArgList[4] = inst->getOperand(2);  // lod=zero

    llvm::CallInst* pNewCallInst = builder.CreateCall(
        pLdIntrinsic, ld_FunctionArgList);

    // as typedread returns float4 by default, bitcast it
    // to int4 if necessary
    // FIXME: is it better to make typedRead return ty a anyvector?
    if (inst->getType() != pNewCallInst->getType())
    {
        IGC_ASSERT_MESSAGE(inst->getType()->isVectorTy(), "expect int4 here");
        IGC_ASSERT_MESSAGE(cast<IGCLLVM::FixedVectorType>(inst->getType())
                               ->getElementType()
                               ->isIntegerTy(32),
                           "expect int4 here");
        IGC_ASSERT_MESSAGE(
            cast<IGCLLVM::FixedVectorType>(inst->getType())->getNumElements() ==
                4,
            "expect int4 here");
        auto bitCastInst = builder.CreateBitCast(pNewCallInst, inst->getType());
        inst->replaceAllUsesWith(bitCastInst);
    }
    else
    {
        inst->replaceAllUsesWith(pNewCallInst);
    }
}


void IGC::CustomSafeOptPass::visitLdRawVec(llvm::CallInst* inst)
{
    //Try to optimize and remove vector ld raw and change to scalar ld raw

    //%a = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p1441792f32(
    //.....float addrspace(1441792) * %243, i32 %offset, i32 4, i1 false), !dbg !216
    //%b = extractelement <4 x float> % 245, i32 0, !dbg !216

    //into

    //%new_offset = add i32 %offset, 0, !dbg !216
    //%b = call float @llvm.genx.GenISA.ldraw.indexed.f32.p1441792f32.i32.i32.i1(
    //.....float addrspace(1441792) * %251, i32 %new_offset, i32 4, i1 false)

    if (inst->hasOneUse() &&
        isa<ExtractElementInst>(inst->user_back()))
    {
        auto EE = cast<ExtractElementInst>(inst->user_back());
        if (auto constIndex = dyn_cast<ConstantInt>(EE->getIndexOperand()))
        {
            llvm::IRBuilder<> builder(inst);

            llvm::SmallVector<llvm::Type*, 2> ovldtypes{
                EE->getType(), //float type
                inst->getOperand(0)->getType(),
            };

            // For new_offset we need to take into acount the index of the Extract
            // and convert it to bytes and add it to the existing offset
            const llvm::DataLayout& dataLayout = inst->getModule()->getDataLayout();
            const uint32_t numBytesPerElement = int_cast<uint32_t>(dataLayout.getTypeAllocSize(EE->getType()));
            auto new_offset = constIndex->getZExtValue() * numBytesPerElement;

            llvm::SmallVector<llvm::Value*, 4> new_args{
                inst->getOperand(0),
                builder.CreateAdd(inst->getOperand(1),builder.getInt32((unsigned)new_offset)),
                inst->getOperand(2),
                inst->getOperand(3)
            };

            Function* pLdraw_indexed_intrinsic = llvm::GenISAIntrinsic::getDeclaration(
                inst->getModule(),
                GenISAIntrinsic::GenISA_ldraw_indexed,
                ovldtypes);

            llvm::Value* ldraw_indexed = builder.CreateCall(pLdraw_indexed_intrinsic, new_args, "");
            EE->replaceAllUsesWith(ldraw_indexed);
        }
    }
}

void IGC::CustomSafeOptPass::visitSampleBptr(llvm::SampleIntrinsic* sampleInst)
{
    // sampleB with bias_value==0 -> sample
    llvm::ConstantFP* constBias = llvm::dyn_cast<llvm::ConstantFP>(sampleInst->getOperand(0));
    if (constBias && constBias->isZero())
    {
        // Copy args skipping bias operand:
        llvm::SmallVector<llvm::Value*, 10> args;
        for (unsigned int i = 1; i < IGCLLVM::getNumArgOperands(sampleInst); i++)
        {
            args.push_back(sampleInst->getArgOperand(i));
        }

        // Copy overloaded types unchanged:
        llvm::SmallVector<llvm::Type*, 4> overloadedTys;
        overloadedTys.push_back(sampleInst->getCalledFunction()->getReturnType());
        overloadedTys.push_back(sampleInst->getOperand(0)->getType());
        overloadedTys.push_back(sampleInst->getPairedTextureValue()->getType());
        overloadedTys.push_back(sampleInst->getTextureValue()->getType());
        overloadedTys.push_back(sampleInst->getSamplerValue()->getType());

        llvm::Function* sampleIntr = llvm::GenISAIntrinsic::getDeclaration(
            sampleInst->getParent()->getParent()->getParent(),
            GenISAIntrinsic::GenISA_sampleptr,
            overloadedTys);

        llvm::Instruction* newSample = llvm::CallInst::Create(sampleIntr, args, "", sampleInst);
        newSample->setDebugLoc(sampleInst->getDebugLoc());
        sampleInst->replaceAllUsesWith(newSample);
    }
}

bool CustomSafeOptPass::isIdentityMatrix(ExtractElementInst& I)
{
    bool found = false;
    auto extractType = cast<IGCLLVM::FixedVectorType>(I.getVectorOperandType());
    auto extractTypeVecSize = (uint32_t)extractType->getNumElements();
    if (extractTypeVecSize == 20 ||
        extractTypeVecSize == 16)
    {
        if (Constant * C = dyn_cast<Constant>(I.getVectorOperand()))
        {
            found = true;

            // found = true if the extractelement is like this:
            // %189 = extractelement <20 x float>
            //    <float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00,
            //     float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00,
            //     float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00,
            //     float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00,
            //     float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00>, i32 %188
            for (unsigned int i = 0; i < extractTypeVecSize; i++)
            {
                if (i == 0 || i == 5 || i == 10 || i == 15)
                {
                    ConstantFP* fpC = dyn_cast<ConstantFP>(C->getAggregateElement(i));
                    ConstantInt* intC = dyn_cast<ConstantInt>(C->getAggregateElement(i));
                    if((fpC && !fpC->isExactlyValue(1.f)) || (intC && !intC->isAllOnesValue()))
                    {
                        found = false;
                        break;
                    }
                }
                else if (!C->getAggregateElement(i)->isZeroValue())
                {
                    found = false;
                    break;
                }
            }
        }
    }
    return found;
}

void CustomSafeOptPass::dp4WithIdentityMatrix(ExtractElementInst& I)
{
    /*
    convert dp4 with identity matrix icb ( ex: "dp4 r[6].x, cb[2][8].xyzw, icb[ r[4].w].xyzw") from
        %189 = shl nuw nsw i32 %188, 2, !dbg !326
        %190 = extractelement <20 x float> <float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00>, i32 %189, !dbg !326
        %191 = or i32 %189, 1, !dbg !326
        %192 = extractelement <20 x float> <float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00>, i32 %191, !dbg !326
        %193 = or i32 %189, 2, !dbg !326
        %194 = extractelement <20 x float> <float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00>, i32 %193, !dbg !326
        %195 = or i32 %189, 3, !dbg !326
        %196 = extractelement <20 x float> <float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00>, i32 %195, !dbg !326
        %s01_s.chan0141 = fmul fast float %181, %190, !dbg !326
        %197 = fmul fast float %182, %192, !dbg !326
        %198 = fadd fast float %197, %s01_s.chan0141, !dbg !326
        %199 = fmul fast float %183, %194, !dbg !326
        %200 = fadd fast float %199, %198, !dbg !326
        %201 = fmul fast float %184, %196, !dbg !326
        %202 = fadd fast float %201, %200, !dbg !326
    to
        %533 = icmp eq i32 %532, 0, !dbg !434
        %534 = icmp eq i32 %532, 1, !dbg !434
        %535 = icmp eq i32 %532, 2, !dbg !434
        %536 = icmp eq i32 %532, 3, !dbg !434
        %537 = select i1 %533, float %525, float 0.000000e+00, !dbg !434
        %538 = select i1 %534, float %526, float %537, !dbg !434
        %539 = select i1 %535, float %527, float %538, !dbg !434
        %540 = select i1 %536, float %528, float %539, !dbg !434
    */

    // check %190 = extractelement <20 x float> <float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00 ...
    if (!I.hasOneUse() || !isIdentityMatrix(I))
        return;

    Instruction* offset[4] = {nullptr, nullptr, nullptr, nullptr};
    ExtractElementInst* eeInst[4] = { &I, nullptr, nullptr, nullptr };

    // check %189 = shl nuw nsw i32 %188, 2, !dbg !326
    // put it in offset[0]
    offset[0] = dyn_cast<BinaryOperator>(I.getOperand(1));
    if (!offset[0] ||
        offset[0]->getOpcode() != Instruction::Shl ||
        offset[0]->getOperand(1) != ConstantInt::get(offset[0]->getOperand(1)->getType(), 2))
    {
        return;
    }

    // check %191 = or i32 %189, 1, !dbg !326
    //       %193 = or i32 % 189, 2, !dbg !326
    //       %195 = or i32 % 189, 3, !dbg !326
    // put them in offset[1], offset[2], offset[3]
    for (auto iter = offset[0]->user_begin(); iter != offset[0]->user_end(); iter++)
    {
        // skip checking for the %190 = extractelement <20 x float> <float 1.000000e+00, ....
        if (*iter == &I)
            continue;

        if (BinaryOperator * orInst = dyn_cast<BinaryOperator>(*iter))
        {
            if (orInst->getOpcode() == BinaryOperator::Or && orInst->hasOneUse())
            {
                if (ConstantInt * orSrc1 = dyn_cast<ConstantInt>(orInst->getOperand(1)))
                {
                    if (orSrc1->getZExtValue() < 4)
                    {
                        offset[orSrc1->getZExtValue()] = orInst;
                    }
                }
            }
        }
    }

    for (int i = 0; i < 4; i++)
    {
        if (offset[i] == nullptr)
            return;
    }

    // check %192 = extractelement <20 x float> ...
    //       %194 = extractelement <20 x float> ...
    //       %196 = extractelement <20 x float> ...
    // put them in eeInst[i]
    for (int i = 1; i < 4; i++)
    {
        eeInst[i] = dyn_cast<ExtractElementInst>(*offset[i]->user_begin());
        if (!eeInst[i] || !isIdentityMatrix(*eeInst[i]))
        {
            return;
        }
    }

    // check dp4 and put them in mulI[] and addI[]
    Instruction* mulI[4] = { nullptr, nullptr, nullptr, nullptr };
    for (int i = 0; i < 4; i++)
    {
        mulI[i] = dyn_cast<Instruction>(*eeInst[i]->user_begin());
        if (mulI[i] == nullptr || !mulI[i]->hasOneUse())
        {
            return;
        }
    }
    int inputInSrcIndex = 0;
    if (mulI[0]->getOperand(0) == eeInst[0])
        inputInSrcIndex = 1;

    // the 1st and 2nd mul are the srcs for add
    if (*mulI[0]->user_begin() != *mulI[1]->user_begin())
    {
        return;
    }
    Instruction* addI[3] = { nullptr, nullptr, nullptr };
    addI[0] = dyn_cast<Instruction>(*mulI[0]->user_begin());
    addI[1] = dyn_cast<Instruction>(*mulI[2]->user_begin());
    addI[2] = dyn_cast<Instruction>(*mulI[3]->user_begin());

    if( addI[0] == nullptr ||
        addI[1] == nullptr ||
        addI[2] == nullptr ||
        !addI[0]->hasOneUse() ||
        !addI[1]->hasOneUse() ||
        *addI[0]->user_begin() != *mulI[2]->user_begin() ||
        *addI[1]->user_begin() != *mulI[3]->user_begin())
    {
        return;
    }

    // start the conversion
    IRBuilder<> builder(addI[2]);

    Value* cond0 = builder.CreateICmp(ICmpInst::ICMP_EQ, offset[0]->getOperand(0), ConstantInt::get(offset[0]->getOperand(0)->getType(), 0));
    Value* cond1 = builder.CreateICmp(ICmpInst::ICMP_EQ, offset[0]->getOperand(0), ConstantInt::get(offset[0]->getOperand(0)->getType(), 1));
    Value* cond2 = builder.CreateICmp(ICmpInst::ICMP_EQ, offset[0]->getOperand(0), ConstantInt::get(offset[0]->getOperand(0)->getType(), 2));
    Value* cond3 = builder.CreateICmp(ICmpInst::ICMP_EQ, offset[0]->getOperand(0), ConstantInt::get(offset[0]->getOperand(0)->getType(), 3));

    Value* zero = ConstantFP::get(Type::getFloatTy(I.getContext()), 0);
    Value* sel0 = builder.CreateSelect(cond0, mulI[0]->getOperand(inputInSrcIndex), zero);
    Value* sel1 = builder.CreateSelect(cond1, mulI[1]->getOperand(inputInSrcIndex), sel0);
    Value* sel2 = builder.CreateSelect(cond2, mulI[2]->getOperand(inputInSrcIndex), sel1);
    Value* sel3 = builder.CreateSelect(cond3, mulI[3]->getOperand(inputInSrcIndex), sel2);

    addI[2]->replaceAllUsesWith(sel3);
}

void CustomSafeOptPass::visitExtractElementInst(ExtractElementInst& I)
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
    if (bitCast && cstIndex)
    {
        // skip intermediate bitcast
        while (isa<BitCastInst>(bitCast->getOperand(0)))
        {
            bitCast = cast<BitCastInst>(bitCast->getOperand(0));
        }
        if (BinaryOperator * binOp = dyn_cast<BinaryOperator>(bitCast->getOperand(0)))
        {
            unsigned int bitShift = 0;
            bool rightShift = false;
            if (binOp->getOpcode() == Instruction::LShr)
            {
                if (ConstantInt * cstShift = dyn_cast<ConstantInt>(binOp->getOperand(1)))
                {
                    bitShift = (unsigned int)cstShift->getZExtValue();
                    rightShift = true;
                }
            }
            else if (binOp->getOpcode() == Instruction::Shl)
            {
                if (ConstantInt * cstShift = dyn_cast<ConstantInt>(binOp->getOperand(1)))
                {
                    bitShift = (unsigned int)cstShift->getZExtValue();
                }
            }
            if (bitShift != 0)
            {
                Type* vecType = I.getVectorOperand()->getType();
                unsigned int eltSize = (unsigned int)cast<VectorType>(vecType)->getElementType()->getPrimitiveSizeInBits();
                if (bitShift % eltSize == 0)
                {
                    int elOffset = (int)(bitShift / eltSize);
                    elOffset = rightShift ? elOffset : -elOffset;
                    unsigned int newIndex = (unsigned int)((int)cstIndex->getZExtValue() + elOffset);
                    if (newIndex < cast<IGCLLVM::FixedVectorType>(vecType)->getNumElements())
                    {
                        IRBuilder<> builder(&I);
                        Value* newBitCast = builder.CreateBitCast(binOp->getOperand(0), vecType);
                        Value* newScalar = builder.CreateExtractElement(newBitCast, newIndex);
                        I.replaceAllUsesWith(newScalar);
                        I.eraseFromParent();
                        return;
                    }
                }
            }
        }
    }

    dp4WithIdentityMatrix(I);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This pass removes dead local memory loads and stores. If we remove all such loads and stores, we also
// remove all local memory fences together with barriers that follow.
//
IGC_INITIALIZE_PASS_BEGIN(TrivialLocalMemoryOpsElimination, "TrivialLocalMemoryOpsElimination", "TrivialLocalMemoryOpsElimination", false, false)
IGC_INITIALIZE_PASS_END(TrivialLocalMemoryOpsElimination, "TrivialLocalMemoryOpsElimination", "TrivialLocalMemoryOpsElimination", false, false)

char TrivialLocalMemoryOpsElimination::ID = 0;

TrivialLocalMemoryOpsElimination::TrivialLocalMemoryOpsElimination() : FunctionPass(ID)
{
    initializeTrivialLocalMemoryOpsEliminationPass(*PassRegistry::getPassRegistry());
}

bool TrivialLocalMemoryOpsElimination::runOnFunction(Function& F)
{
    bool change = false;

    IGCMD::MetaDataUtils* pMdUtil = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (!isEntryFunc(pMdUtil, &F))
    {
        // Skip if it is non-entry function.  For example, a subroutine
        //   foo ( local int* p) { ...... store v, p; ......}
        // in which no localMemoptimization will be performed.
        return change;
    }

    visit(F);
    if (!abortPass && (m_LocalLoadsToRemove.empty() ^ m_LocalStoresToRemove.empty()))
    {
        for (StoreInst* Inst : m_LocalStoresToRemove)
        {
            Inst->eraseFromParent();
            change = true;
        }

        for (LoadInst* Inst : m_LocalLoadsToRemove)
        {
            if (Inst->use_empty())
            {
                Inst->eraseFromParent();
                change = true;
            }
        }

        for (CallInst* Inst : m_LocalFencesBariersToRemove)
        {
            Inst->eraseFromParent();
            change = true;
        }
    }
    m_LocalStoresToRemove.clear();
    m_LocalLoadsToRemove.clear();
    m_LocalFencesBariersToRemove.clear();

    return change;
}

/*
OCL instruction barrier(CLK_LOCAL_MEM_FENCE); is translate to two instructions
call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 true)
call void @llvm.genx.GenISA.threadgroupbarrier()

if we remove call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 true)
we must remove next instruction if it is call void @llvm.genx.GenISA.threadgroupbarrier()
*/
void TrivialLocalMemoryOpsElimination::findNextThreadGroupBarrierInst(Instruction& I)
{
    auto nextInst = I.getNextNonDebugInstruction();
    if (isa<GenIntrinsicInst>(nextInst))
    {
        GenIntrinsicInst* II = cast<GenIntrinsicInst>(nextInst);
        if (II->getIntrinsicID() == GenISAIntrinsic::GenISA_threadgroupbarrier)
        {
            m_LocalFencesBariersToRemove.push_back(dyn_cast<CallInst>(nextInst));
        }
    }
}

void TrivialLocalMemoryOpsElimination::visitLoadInst(LoadInst& I)
{
    if (I.getPointerAddressSpace() == ADDRESS_SPACE_LOCAL)
    {
        m_LocalLoadsToRemove.push_back(&I);
    }
    else if (I.getPointerAddressSpace() == ADDRESS_SPACE_GENERIC)
    {
        abortPass = true;
    }
}

void TrivialLocalMemoryOpsElimination::visitStoreInst(StoreInst& I)
{
    if (I.getPointerAddressSpace() == ADDRESS_SPACE_LOCAL)
    {
        if (auto *GV = dyn_cast<GlobalVariable>(I.getPointerOperand()->stripPointerCasts()))
        {
            // Device sanitizer instrumentation pass inserts a new local memory
            // variable and inserts store to the variable in a kernel. The
            // variable is loaded later in no-inline functions. For this case,
            // do not eliminate the store.
            if (GV->getName().startswith("__Asan"))
            {
                return;
            }
        }
        m_LocalStoresToRemove.push_back(&I);
    }
    else if (I.getPointerAddressSpace() == ADDRESS_SPACE_GENERIC)
    {
        abortPass = true;
    }
}

bool TrivialLocalMemoryOpsElimination::isLocalBarrier(CallInst& I)
{
    //check arguments in call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 true) if match to
    // (i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 true) it is local barrier
    std::vector<bool> argumentsOfMemoryBarrier;

    for (auto arg = I.arg_begin(); arg != I.arg_end(); ++arg)
    {
        ConstantInt* ci = dyn_cast<ConstantInt>(arg);
        if (ci) {
            argumentsOfMemoryBarrier.push_back(ci->getValue().getBoolValue());
        }
        else {
            // argument is not a constant, so we can't tell.
            return false;
        }
    }

    return argumentsOfMemoryBarrier == m_argumentsOfLocalMemoryBarrier;
}

// If any call instruction use pointer to local memory abort pass execution
void TrivialLocalMemoryOpsElimination::anyCallInstUseLocalMemory(CallInst& I)
{
    Function* fn = I.getCalledFunction();

    if (fn != NULL)
    {
        for (auto arg = fn->arg_begin(); arg != fn->arg_end(); ++arg)
        {
            if (arg->getType()->isPointerTy())
            {
                if (arg->getType()->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL || arg->getType()->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC) abortPass = true;
            }
        }
    }
}

void TrivialLocalMemoryOpsElimination::visitCallInst(CallInst& I)
{
    // detect only: llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 true)
    // (note: the first and last arguments are true)
    // and add them with immediately following barriers to m_LocalFencesBariersToRemove
    anyCallInstUseLocalMemory(I);

    if (isa<GenIntrinsicInst>(I))
    {
        GenIntrinsicInst* II = cast<GenIntrinsicInst>(&I);
        if (II->getIntrinsicID() == GenISAIntrinsic::GenISA_memoryfence)
        {
            if (isLocalBarrier(I))
            {
                m_LocalFencesBariersToRemove.push_back(&I);
                findNextThreadGroupBarrierInst(I);
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

// m_Add is able to match add and equivalent sub instructions.
// This predicate is useful for patterns where only add should be matched:
bool isActualAddInstr(const Instruction* inst) { return inst && inst->getOpcode() == Instruction::Add; }

GenSpecificPattern::GenSpecificPattern() : FunctionPass(ID)
{
    initializeGenSpecificPatternPass(*PassRegistry::getPassRegistry());
}

bool GenSpecificPattern::runOnFunction(Function& F)
{
    // TODO: Consider a worklist-based backwards iteration instead for this
    // pass.
    // In its current form, we struggle with replacing any of the visited
    // instruction's users, which limits the depth of an instruction chain
    // that we may want to rebuild.
    visit(F);
    return true;
}

// Lower SDiv to better code sequence if possible
void GenSpecificPattern::visitSDiv(llvm::BinaryOperator& I)
{
    if (ConstantInt * divisor = dyn_cast<ConstantInt>(I.getOperand(1)))
    {
        // signed division of power of 2 can be transformed to asr
        // For negative values we need to make sure we round correctly
        int log2Div = divisor->getValue().exactLogBase2();
        if (log2Div > 0)
        {
            unsigned int intWidth = divisor->getBitWidth();
            IRBuilder<> builder(&I);
            Value* signedBitOnly = I.getOperand(0);
            if (log2Div > 1)
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

// Replace: 'mul i64 %x, 2^n'
// with:    'shl i64 %x, n'
// We can handle -2^n multiplier similarly by negating the shl
void GenSpecificPattern::visitMul(llvm::BinaryOperator& I)
{
    Value* ValOp = nullptr;
    const APInt* ConstOp = nullptr;
    using namespace llvm::PatternMatch;
    if (match(&I, m_c_Mul(m_Value(ValOp), m_APInt(ConstOp))))
    {
        IRBuilder<> builder(&I);
        if (ConstOp->isPowerOf2())
        {
            I.replaceAllUsesWith(
                builder.CreateShl(ValOp, (uint64_t)ConstOp->exactLogBase2()));
            I.eraseFromParent();
            return;
        }
        else if (!IGCLLVM::isNegatedPowerOf2(*ConstOp))
            return;

        APInt ConstOpAbs = ConstOp->abs();
        Value* Shl = builder.CreateShl(
            ValOp, (uint64_t)ConstOpAbs.exactLogBase2());
        for (User* UI : I.users())
        {
            // We're going a little further and making sure that we merge the
            // shift result's negation with any subsequent adds:
            //   '%shift = shl i64 %x, n'
            //   '%res' = sub %var, %shift
            // preventing the additional negation in between the shift and the
            // `var` addition. This kind of a peephole optimization may not be
            // available later down the pipeline.
            // TODO: Consider moving this logic to add/sub visitors once the
            // whole GenSpecificPattern iteration logic is guaranteed to allow
            // deferred instructions.
            Value* Addend = nullptr;
            if (match(UI, m_c_Add(m_Specific(&I), m_Value(Addend))))
            {
                // Propagate the 'shl' <- 'sub' results instead. No way to
                // erase the original adds within this pass just yet, as we'd
                // be invalidating the InstVisitor iteration, but subsequent
                // DCE instances will handle the orphaned instructions anyway.
                builder.SetInsertPoint(cast<Instruction>(UI));
                UI->replaceAllUsesWith(builder.CreateSub(Addend, Shl));
            }
        }
        // Make sure to reset the insertion point for the shl negation after
        // the possible transformations above
        builder.SetInsertPoint(&I);
        I.replaceAllUsesWith(builder.CreateNeg(Shl));
        I.eraseFromParent();
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
void CustomSafeOptPass::matchReverse(BinaryOperator& I)
{
    using namespace llvm::PatternMatch;
    IGC_ASSERT(I.getType()->isIntegerTy());
    Value* nextOrShl = nullptr, * nextOrShr = nullptr;
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
    IGC_ASSERT(bitWidth == 16 || bitWidth == 32 || bitWidth == 64);

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
        Function* bfrevFunc = GenISAIntrinsic::getDeclaration(
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
            Value* int32Source = builder.CreateBitCast(nextOrShl, IGCLLVM::FixedVectorType::get(builder.getInt32Ty(), 2));
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

/* Transforms pattern1 or pattern2 to a bitcast,extract,insert,insert,bitcast

    From:
        %5 = zext i32 %a to i64 <--- optional
        %6 = shl i64 %5, 32
        or
        %6 = and i64 %5, 0xFFFFFFFF00000000
    To:
        %BC = bitcast i64 %5 to <2 x i32> <---- not needed when %5 is zext
        %6 = extractelement <2 x i32> %BC, i32 0/1 <---- not needed when %5 is zext
        %7 = insertelement <2 x i32> %vec, i32 0, i32 0
        %8 = insertelement <2 x i32> %vec, %6, i32 1
        %9 = bitcast <2 x i32> %8 to i64
 */
void GenSpecificPattern::createBitcastExtractInsertPattern(BinaryOperator& I, Value* OpLow, Value* OpHi, unsigned extractNum1, unsigned extractNum2)
{
    if (IGC_IS_FLAG_DISABLED(EnableBitcastExtractInsertPattern)) {
        return;
    }

    llvm::IRBuilder<> builder(&I);
    auto vec2 = IGCLLVM::FixedVectorType::get(builder.getInt32Ty(), 2);
    Value* vec = UndefValue::get(vec2);
    Value* elemLow = nullptr;
    Value* elemHi = nullptr;

    auto zeroextorNot = [&](Value* Op, unsigned num)
    {
        Value* elem = nullptr;
        if (auto ZextInst = dyn_cast<ZExtInst>(Op))
        {
            if (ZextInst->getDestTy() == builder.getInt64Ty() && ZextInst->getSrcTy() == builder.getInt32Ty())
            {
                elem = ZextInst->getOperand(0);
            }
        }
        else if (auto IEIInst = dyn_cast<InsertElementInst>(Op))
        {
            auto opType = IEIInst->getType();
            if (opType->isVectorTy() && cast<VectorType>(opType)->getElementType()->isIntegerTy(32) && cast<IGCLLVM::FixedVectorType>(opType)->getNumElements() == 2)
            {
                elem = IEIInst->getOperand(1);
            }
        }
        else
        {
            Value* BC = builder.CreateBitCast(Op, vec2);
            elem = builder.CreateExtractElement(BC, builder.getInt32(num));
        }
        return elem;
    };

    elemLow = (OpLow == nullptr) ? builder.getInt32(0) : zeroextorNot(OpLow, extractNum1);
    elemHi = (OpHi == nullptr) ? builder.getInt32(0) : zeroextorNot(OpHi, extractNum2);

    if (elemHi == nullptr || elemLow == nullptr)
        return;

    vec = builder.CreateInsertElement(vec, elemLow, builder.getInt32(0));
    vec = builder.CreateInsertElement(vec, elemHi, builder.getInt32(1));
    vec = builder.CreateBitCast(vec, builder.getInt64Ty());
    I.replaceAllUsesWith(vec);
    I.eraseFromParent();
}

void GenSpecificPattern::createAddcIntrinsicPattern(Instruction& ZEI, Value* val1, Value* val2, Value* val3, Instruction& addInst)
{
    IRBuilder<> Builder(&addInst);

    SmallVector<Value*, 2> packed_res_params;
    packed_res_params.push_back(val1);
    packed_res_params.push_back(val2);
    Type* types[] = { IGCLLVM::FixedVectorType::get(val3->getType(), 2), val3->getType() };
    Function* addcIntrinsic = llvm::GenISAIntrinsic::getDeclaration(ZEI.getModule(), GenISAIntrinsic::GenISA_uaddc, types);
    Value* newVal = Builder.CreateCall(addcIntrinsic, packed_res_params);
    CallInst* packed_res_call = cast<CallInst>(newVal);

    Value* result = Builder.CreateExtractElement(packed_res_call, Builder.getInt32(0));
    Value* carryFlag = Builder.CreateExtractElement(packed_res_call, Builder.getInt32(1));

    ZEI.replaceAllUsesWith(carryFlag);
    addInst.replaceAllUsesWith(result);
}

void GenSpecificPattern::visitAdd(BinaryOperator& I) {
    /*
    from
        %23 = add i32 %22, 3
        %24 = add i32 %23, 16
    to
        %24 = add i32 %22, 19
    */

    llvm::IRBuilder<> builder(&I);
    for (int ImmSrcId1 = 0; ImmSrcId1 < 2; ImmSrcId1++)
    {
        ConstantInt* IConstant = dyn_cast<ConstantInt>(I.getOperand(ImmSrcId1));
        if (IConstant)
        {
            llvm::Instruction* AddInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(1 - ImmSrcId1));
            if (AddInst && AddInst->getOpcode() == Instruction::Add)
            {
                for (int ImmSrcId2 = 0; ImmSrcId2 < 2; ImmSrcId2++)
                {
                    ConstantInt* AddConstant = dyn_cast<ConstantInt>(AddInst->getOperand(ImmSrcId2));
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

void GenSpecificPattern::visitAnd(BinaryOperator& I)
{
    /*  This `and` is basically fabs() done on high part of int representation.
        For float instructions minus operand can end as SrcMod, but since we cast it
        from double to int it will end as additional mov, and we can ignore this m_FNeg
        anyway.

        From :
            %sub = fsub double -0.000000e+00, %res.039
            %25 = bitcast double %sub to i64
            %26 = bitcast i64 %25 to <2 x i32> // or directly double to <2xi32>
            %27 = extractelement <2 x i32> %26, i32 1
            %and31.i = and i32 %27, 2147483647

        To:
            %25 = bitcast double %res.039 to <2 x i32>
            %27 = extractelement <2 x i32> %26, i32 1
            %and31.i = and i32 %27, 2147483647

        Or on Int64 without extract:
        From:
            %sub = fsub double -0.000000e+00, %res.039
            %astype.i112.i.i = bitcast double %sub to i64
            %and107.i.i = and i64 %astype.i112.i.i, 9223372032559808512 // 0x7FFFFFFF00000000
        To:
            %bit_cast = bitcast double %res.039 to i64
            %and107.i.i = and i64 %bit_cast, 9223372032559808512 // 0x7FFFFFFF00000000

    */

    /*  Get src of either 2 bitcast chain: double -> i64, i64 -> 2xi32
        or from single direct: double -> 2xi32
    */

    auto getValidBitcastSrc = [](Instruction* op) -> llvm::Value*
    {
        if (!(isa<BitCastInst>(op)))
            return nullptr;

        BitCastInst* opBC = cast<BitCastInst>(op);

        auto opType = opBC->getType();
        if (!(opType->isVectorTy() && cast<VectorType>(opType)->getElementType()->isIntegerTy(32) && cast<IGCLLVM::FixedVectorType>(opType)->getNumElements() == 2))
            return nullptr;

        if (opBC->getSrcTy()->isDoubleTy())
            return opBC->getOperand(0); // double -> 2xi32

        BitCastInst* bitCastSrc = dyn_cast<BitCastInst>(opBC->getOperand(0));

        if (bitCastSrc && bitCastSrc->getDestTy()->isIntegerTy(64) && bitCastSrc->getSrcTy()->isDoubleTy())
            return bitCastSrc->getOperand(0); // double -> i64, i64 -> 2xi32

        return nullptr;
    };

    llvm::IRBuilder<> builder(&I);
    using namespace llvm::PatternMatch;
    Value* src_of_FNeg = nullptr;
    Instruction* inst = nullptr;

    auto fabs_on_int_pattern1 = m_And(m_ExtractElt(m_Instruction(inst), m_SpecificInt(1)), m_SpecificInt(0x7FFFFFFF));
    auto fabs_on_int_pattern2 = m_And(m_Instruction(inst), m_SpecificInt(0x7FFFFFFF00000000));
    auto fneg_pattern = m_FNeg(m_Value(src_of_FNeg));

    /*
    From:
        %5 = zext i32 %a to i64 <--- optional
        %6 = and i64 %5, 0xFFFFFFFF00000000 (-4294967296)
    To:
        %BC = bitcast i64 %5 to <2 x i32> <---- not needed when %5 is zext
        %6 = extractelement <2 x i32> %BC, i32 1 <---- not needed when %5 is zext
        %7 = insertelement <2 x i32> %vec, i32 0, i32 0
        %8 = insertelement <2 x i32> %vec, %6, i32 1
        %9 = bitcast <2 x i32> %8 to i64
    */

    auto pattern1 = m_And(m_Instruction(inst), m_SpecificInt(0xFFFFFFFF00000000));

    if (match(&I, fabs_on_int_pattern1))
    {
        Value* src = getValidBitcastSrc(inst);
        if (src && match(src, fneg_pattern) && src_of_FNeg && src_of_FNeg->getType()->isDoubleTy())
        {
            VectorType* vec2 = IGCLLVM::FixedVectorType::get(builder.getInt32Ty(), 2);
            Value* BC = builder.CreateBitCast(src_of_FNeg, vec2);
            Value* EE = builder.CreateExtractElement(BC, builder.getInt32(1));
            Value* AI = builder.CreateAnd(EE, builder.getInt32(0x7FFFFFFF));
            I.replaceAllUsesWith(AI);
            I.eraseFromParent();
        }
    }
    else if (match(&I, fabs_on_int_pattern2))
    {
        BitCastInst* bitcast = dyn_cast<BitCastInst>(inst);
        bool bitcastValid = bitcast && bitcast->getDestTy()->isIntegerTy(64) && bitcast->getSrcTy()->isDoubleTy();

        if (bitcastValid && match(bitcast->getOperand(0), fneg_pattern) && src_of_FNeg->getType()->isDoubleTy())
        {
            Value* BC = builder.CreateBitCast(src_of_FNeg, I.getType());
            Value* AI = builder.CreateAnd(BC, builder.getInt64(0x7FFFFFFF00000000));
            I.replaceAllUsesWith(AI);
            I.eraseFromParent();
        }
    }
    else if (match(&I, pattern1) && I.getType()->isIntegerTy(64))
    {
        createBitcastExtractInsertPattern(I, nullptr, I.getOperand(0), 0, 1);
    }
    else
    {

        Instruction* AndSrc = nullptr;
        ConstantInt* CI = nullptr;

        /*
        From:
          %28 = and i32 %24, 255
          %29 = lshr i32 %24, 8
          %30 = and i32 %29, 255
          %31 = lshr i32 %24, 16
          %32 = and i32 %31, 255
        To:
          %temp = bitcast i32 %24 to <4 x i8>
          %ee1 = extractelement <4 x i8> %temp, i32 0
          %ee2 = extractelement <4 x i8> %temp, i32 1
          %ee3 = extractelement <4 x i8> %temp, i32 2
          %28 = zext i8 %ee1 to i32
          %30 = zext i8 %ee2 to i32
          %32 = zext i8 %ee3 to i32
        */
        auto pattern_And_0xFF = m_And(m_Instruction(AndSrc), m_SpecificInt(0xFF));

        if (match(&I, pattern_And_0xFF) && I.getType()->isIntegerTy(32) && AndSrc && AndSrc->getType()->isIntegerTy(32))
        {
            Instruction* LhsSrc = nullptr;
            auto LShr_Pattern = m_LShr(m_Instruction(LhsSrc), m_ConstantInt(CI));
            bool LShrMatch = match(AndSrc, LShr_Pattern) && LhsSrc && LhsSrc->getType()->isIntegerTy(32) && CI && (CI->getZExtValue() % 8 == 0);

            // in case there's no shr, it will be 0
            uint32_t newIndex = 0;

            if (LShrMatch) // extract inner
            {
                AndSrc = LhsSrc;
                newIndex = (uint32_t)CI->getZExtValue() / 8;
                VectorType* vec4 = VectorType::get(builder.getInt8Ty(), 4, false);
                Value* BC = builder.CreateBitCast(AndSrc, vec4);
                Value* EE = builder.CreateExtractElement(BC, builder.getInt32(newIndex));
                Value* Zext = builder.CreateZExt(EE, builder.getInt32Ty());
                I.replaceAllUsesWith(Zext);
                I.eraseFromParent();
            }
        }
    }
}

void GenSpecificPattern::visitAShr(BinaryOperator& I)
{
    /*
        From:
        %129 = i32...
        %Temp = shl i32 %129, 16
        %132 = ashr exact i32 %Temp, 16
        %133 = ashr i32 %129, 16
        To:
        %129 = i32...
        %temp = bitcast i32 %129 to <2 x i16>
        %ee1 = extractelement <2 x i16> %temp, i32 0
        %ee2 = extractelement <2 x i16> %temp, i32 1
        %132 = sext i8 %ee1 to i32
        %133 = sext i8 %ee2 to i32
        Which will end up as regioning instead of 2 isntr.
    */

    llvm::IRBuilder<> builder(&I);
    using namespace llvm::PatternMatch;

    Instruction* AShrSrc = nullptr;
    auto pattern_1 = m_AShr(m_Instruction(AShrSrc), m_SpecificInt(16));

    if (match(&I, pattern_1) && I.getType()->isIntegerTy(32) && AShrSrc && AShrSrc->getType()->isIntegerTy(32))
    {
        Instruction* ShlSrc = nullptr;

        auto Shl_Pattern = m_Shl(m_Instruction(ShlSrc), m_SpecificInt(16));
        bool submatch = match(AShrSrc, Shl_Pattern) && ShlSrc && ShlSrc->getType()->isIntegerTy(32);

        // in case there's no shr, we take upper half
        uint32_t newIndex = 1;

        // if there was Shl, we take lower half
        if (submatch)
        {
            AShrSrc = ShlSrc;
            newIndex = 0;
        }
        VectorType* vec2 = VectorType::get(builder.getInt16Ty(), 2, false);
        Value* BC = builder.CreateBitCast(AShrSrc, vec2);
        Value* EE = builder.CreateExtractElement(BC, builder.getInt32(newIndex));
        Value* Sext = builder.CreateSExt(EE, builder.getInt32Ty());
        I.replaceAllUsesWith(Sext);
        I.eraseFromParent();
    }
}

void GenSpecificPattern::visitShl(BinaryOperator& I)
{
    /*
      From:
          %5 = zext i32 %a to i64 <--- optional
          %6 = shl i64 %5, 32

      To:
          %BC = bitcast i64 %5 to <2 x i32> <---- not needed when %5 is zext
          %6 = extractelement <2 x i32> %BC, i32 0 <---- not needed when %5 is zext
          %7 = insertelement <2 x i32> %vec, i32 0, i32 0
          %8 = insertelement <2 x i32> %vec, %6, i32 1
          %9 = bitcast <2 x i32> %8 to i64
    */

    using namespace llvm::PatternMatch;
    Instruction* inst = nullptr;

    auto pattern1 = m_Shl(m_Instruction(inst), m_SpecificInt(32));

    if (match(&I, pattern1) && I.getType()->isIntegerTy(64))
    {
        createBitcastExtractInsertPattern(I, nullptr, I.getOperand(0), 0, 0);
    }
}

void GenSpecificPattern::visitOr(BinaryOperator& I)
{
    llvm::IRBuilder<> builder(&I);
    using namespace llvm::PatternMatch;

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
    Value* AndOp1 = nullptr, * EltOp1 = nullptr;
    auto pattern1 = m_Or(
        m_And(m_Value(AndOp1), m_SpecificInt(0xFFFFFFFF)),
        m_Shl(m_Value(EltOp1), m_SpecificInt(32)));
    Value* AndOp2 = nullptr, * EltOp2 = nullptr, * VecOp = nullptr;
    auto pattern2 = m_Or(
        m_And(m_Value(AndOp2), m_SpecificInt(0xFFFFFFFF)),
        m_BitCast(m_InsertElt(m_Value(VecOp), m_Value(EltOp2), m_SpecificInt(1))));
    if (match(&I, pattern1) && AndOp1 && AndOp1->getType()->isIntegerTy(64))
    {
        createBitcastExtractInsertPattern(I, AndOp1, EltOp1, 0, 0);
    }
    else if (match(&I, pattern2) && AndOp2 && AndOp2->getType()->isIntegerTy(64))
    {
        ConstantVector* cVec = dyn_cast<ConstantVector>(VecOp);
        if (cVec)
        {
            IGCLLVM::FixedVectorType* vector_type = dyn_cast<IGCLLVM::FixedVectorType>(VecOp->getType());
            if (vector_type &&
                isa<ConstantInt>(cVec->getOperand(0)) &&
                cast<ConstantInt>(cVec->getOperand(0))->isZero() &&
                vector_type->getElementType()->isIntegerTy(32) &&
                vector_type->getNumElements() == 2)
            {
                auto InsertOp = cast<BitCastInst>(I.getOperand(1))->getOperand(0);
                createBitcastExtractInsertPattern(I, AndOp2, InsertOp, 0, 0);
            }
        }
    }
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
        ConstantInt* OrConstant = dyn_cast<ConstantInt>(I.getOperand(1));
        if (OrConstant)
        {
            llvm::Instruction* ShlInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(0));
            if (ShlInst && ShlInst->getOpcode() == Instruction::Shl)
            {
                ConstantInt* ShlConstant = dyn_cast<ConstantInt>(ShlInst->getOperand(1));
                if (ShlConstant)
                {
                    // if the constant bit width is larger than 64, we cannot store ShlIntValue and OrIntValue rawdata as uint64_t.
                    // will need a fix then
                    IGC_ASSERT(ShlConstant->getBitWidth() <= 64);
                    IGC_ASSERT(OrConstant->getBitWidth() <= 64);

                    uint64_t ShlIntValue = *(ShlConstant->getValue()).getRawData();
                    uint64_t OrIntValue = *(OrConstant->getValue()).getRawData();

                    if (OrIntValue < pow(2, ShlIntValue))
                    {
                        Value* newAdd = builder.CreateAdd(I.getOperand(0), I.getOperand(1));
                        I.replaceAllUsesWith(newAdd);
                    }
                }
            }
        }
    }
}

void GenSpecificPattern::visitCmpInst(CmpInst& I)
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
        VectorType* vec2 = IGCLLVM::FixedVectorType::get(builder.getInt32Ty(), 2);
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
        if (pCtx->getCompilerOption().NoNaNs)
        {
            if (I.getPredicate() == CmpInst::FCMP_ORD)
            {
                I.replaceAllUsesWith(ConstantInt::getTrue(I.getType()));
            }
        }
    }
}

void GenSpecificPattern::visitSelectInst(SelectInst& I)
{
    /*
    from
        %res = select i1 %cond, i1 true, i1 %cmp
    to
        %res = or i1 %cond, %cmp

               or

    from
        %2 = select i1 %0, i1 %1, i1 false
    to
        %2 = and i1 %0, %1

               or

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

    IGC_ASSERT(I.getOpcode() == Instruction::Select);

    llvm::IRBuilder<> builder(&I);
    bool skipOpt = false;

    ConstantInt* C1 = dyn_cast<ConstantInt>(I.getOperand(1));
    if (C1 && C1->isOne())
    {
        if (I.getType()->isIntegerTy(1))
        {
            Value* newValueOr = builder.CreateOr(I.getOperand(0), I.getOperand(2));
            I.replaceAllUsesWith(newValueOr);
        }
    }

    ConstantInt* Cint = dyn_cast<ConstantInt>(I.getOperand(2));
    if (Cint && Cint->isZero())
    {
        if (I.getType()->isIntegerTy(1))
        {
            Value* newValueAnd = builder.CreateAnd(I.getOperand(0), I.getOperand(1));
            I.replaceAllUsesWith(newValueAnd);
        }
        else
        {
            llvm::Instruction* cmpInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(0));
            if (cmpInst &&
                cmpInst->getOpcode() == Instruction::ICmp &&
                I.getOperand(1) != cmpInst->getOperand(0))
            {
                // disable the cases for csel or where we can optimize the instructions to such as add.ge.* later in vISA
                ConstantInt* C = dyn_cast<ConstantInt>(cmpInst->getOperand(1));
                if (C && C->isZero())
                {
                    skipOpt = true;
                }

                if (!skipOpt)
                {
                    // temporary disable the case where cmp is used in multiple sel, and not all of them have src2=0
                    // we should remove this if we can allow both flag and grf dst for the cmp to be used.
                    for (auto selI = cmpInst->user_begin(), E = cmpInst->user_end(); selI != E; ++selI)
                    {
                        if (llvm::SelectInst * selInst = llvm::dyn_cast<llvm::SelectInst>(*selI))
                        {
                            ConstantInt* C = dyn_cast<ConstantInt>(selInst->getOperand(2));
                            if (!(C && C->isZero()))
                            {
                                skipOpt = true;
                                break;
                            }
                        }
                    }
                }

                if (!skipOpt)
                {
                    llvm::IRBuilder<> builder(&I);
                    Value* newValueSext = builder.CreateSExtOrBitCast(I.getOperand(0), I.getType());
                    Value* newValueAnd = builder.CreateAnd(I.getOperand(1), newValueSext);
                    I.replaceAllUsesWith(newValueAnd);
                }
            }
        }
    }
    else
    {
        ConstantFP* Cfp = dyn_cast<ConstantFP>(I.getOperand(2));
        if (Cfp && Cfp->isZero())
        {
            llvm::Instruction* cmpInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(0));
            if (cmpInst &&
                cmpInst->getOpcode() == Instruction::FCmp &&
                I.getOperand(1) != cmpInst->getOperand(0))
            {
                // disable the cases for csel or where we can optimize the instructions to such as add.ge.* later in vISA
                ConstantFP* C = dyn_cast<ConstantFP>(cmpInst->getOperand(1));
                if (C && C->isZero())
                {
                    skipOpt = true;
                }

                if (!skipOpt)
                {
                    for (auto selI = cmpInst->user_begin(), E = cmpInst->user_end(); selI != E; ++selI)
                    {
                        if (llvm::SelectInst * selInst = llvm::dyn_cast<llvm::SelectInst>(*selI))
                        {
                            // temporary disable the case where cmp is used in multiple sel, and not all of them have src2=0
                            // we should remove this if we can allow both flag and grf dst for the cmp to be used.
                            ConstantFP* C2 = dyn_cast<ConstantFP>(selInst->getOperand(2));
                            if (!(C2 && C2->isZero()))
                            {
                                skipOpt = true;
                                break;
                            }

                            // if it is cmp-sel(1.0 / 0.0)-mul, we could better patten match it later in codeGen.
                            ConstantFP* C1 = dyn_cast<ConstantFP>(selInst->getOperand(1));
                            if (C1 && C2 && selInst->hasOneUse())
                            {
                                if ((C2->isZero() && C1->isExactlyValue(1.f)) || (C1->isZero() && C2->isExactlyValue(1.f)))
                                {
                                    Instruction* mulInst = dyn_cast<Instruction>(*selInst->user_begin());
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

                if (!skipOpt)
                {
                    ConstantFP* C1 = dyn_cast<ConstantFP>(I.getOperand(1));
                    if (C1)
                    {
                        if (C1->getType()->isHalfTy())
                        {
                            Value* newValueSext = builder.CreateSExtOrBitCast(I.getOperand(0), Type::getInt16Ty(I.getContext()));
                            Value* newConstant = ConstantInt::get(I.getContext(), C1->getValueAPF().bitcastToAPInt());
                            Value* newValueAnd = builder.CreateAnd(newValueSext, newConstant);
                            Value* newValueCastFP = builder.CreateZExtOrBitCast(newValueAnd, Type::getHalfTy(I.getContext()));
                            I.replaceAllUsesWith(newValueCastFP);
                        }
                        else if (C1->getType()->isFloatTy())
                        {
                            Value* newValueSext = builder.CreateSExtOrBitCast(I.getOperand(0), Type::getInt32Ty(I.getContext()));
                            Value* newConstant = ConstantInt::get(I.getContext(), C1->getValueAPF().bitcastToAPInt());
                            Value* newValueAnd = builder.CreateAnd(newValueSext, newConstant);
                            Value* newValueCastFP = builder.CreateZExtOrBitCast(newValueAnd, Type::getFloatTy(I.getContext()));
                            I.replaceAllUsesWith(newValueCastFP);
                        }
                    }
                    else
                    {
                        if (I.getOperand(1)->getType()->isHalfTy())
                        {
                            Value* newValueSext = builder.CreateSExtOrBitCast(I.getOperand(0), Type::getInt16Ty(I.getContext()));
                            Value* newValueBitcast = builder.CreateZExtOrBitCast(I.getOperand(1), Type::getInt16Ty(I.getContext()));
                            Value* newValueAnd = builder.CreateAnd(newValueSext, newValueBitcast);
                            Value* newValueCastFP = builder.CreateZExtOrBitCast(newValueAnd, Type::getHalfTy(I.getContext()));
                            I.replaceAllUsesWith(newValueCastFP);
                        }
                        else if (I.getOperand(1)->getType()->isFloatTy())
                        {
                            Value* newValueSext = builder.CreateSExtOrBitCast(I.getOperand(0), Type::getInt32Ty(I.getContext()));
                            Value* newValueBitcast = builder.CreateZExtOrBitCast(I.getOperand(1), Type::getInt32Ty(I.getContext()));
                            Value* newValueAnd = builder.CreateAnd(newValueSext, newValueBitcast);
                            Value* newValueCastFP = builder.CreateZExtOrBitCast(newValueAnd, Type::getFloatTy(I.getContext()));
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
    if (auto * compInst = dyn_cast<ICmpInst>(I.getOperand(0)))
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
            Value* newSelOp1 = NULL;
            Value* newSelOp2 = NULL;
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
                IGC_ASSERT(newSelOp1 == NULL);
                newSelOp1 = cmpOp0;
            }

            if (isa<llvm::ConstantInt>(selOp1) &&
                isa<llvm::ConstantInt>(cmpOp1) &&
                (cast<llvm::ConstantInt>(selOp1)->getZExtValue() ==
                    cast<llvm::ConstantInt>(cmpOp1)->getZExtValue()))
            {
                IGC_ASSERT(newSelOp1 == NULL);
                newSelOp1 = cmpOp1;
            }

            if (isa<llvm::ConstantInt>(selOp2) &&
                isa<llvm::ConstantInt>(cmpOp0) &&
                (cast<llvm::ConstantInt>(selOp2)->getZExtValue() ==
                    cast<llvm::ConstantInt>(cmpOp0)->getZExtValue()))
            {
                IGC_ASSERT(newSelOp2 == NULL);
                newSelOp2 = cmpOp0;
            }

            if (isa<llvm::ConstantInt>(selOp2) &&
                isa<llvm::ConstantInt>(cmpOp1) &&
                (cast<llvm::ConstantInt>(selOp2)->getZExtValue() ==
                    cast<llvm::ConstantInt>(cmpOp1)->getZExtValue()))
            {
                IGC_ASSERT(newSelOp2 == NULL);
                newSelOp2 = cmpOp1;
            }

            if (newSelOp1 && newSelOp2)
            {
                auto newSelInst = builder.CreateSelect(I.getCondition(), newSelOp1, newSelOp2);
                auto newTruncInst = builder.CreateTruncOrBitCast(newSelInst, selOp1->getType());
                I.replaceAllUsesWith(newTruncInst);
                I.eraseFromParent();
            }
        }

    }
}

void GenSpecificPattern::visitCastInst(CastInst& I)
{
    Instruction* srcVal = nullptr;
    // Intrinsic::trunc call is handled by 'rndz' hardware instruction which
    // does not support double precision floating point type
    if (I.getType()->isDoubleTy())
    {
        return;
    }
    if (isa<SIToFPInst>(&I))
    {
        srcVal = dyn_cast<FPToSIInst>(I.getOperand(0));
    }
    if (srcVal && srcVal->getOperand(0)->getType() == I.getType())
    {
        if ((srcVal = dyn_cast<Instruction>(srcVal->getOperand(0))))
        {
            // need fast math to know that we can ignore Nan
            if (isa<FPMathOperator>(srcVal) && srcVal->isFast())
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

/*
from:
    %HighBits.Vec = insertelement <2 x i32> <i32 0, i32 undef>, i32 %HighBits.32, i32 1
    %HighBits.64 = bitcast <2 x i32> %HighBits.Vec to i64
    %LowBits.64 = zext i32 %LowBits.32 to i64
    %LowPlusHighBits = or i64 %HighBits.64, %LowBits.64
    %19 = bitcast i64 %LowPlusHighBits to double
to:
    %17 = insertelement <2 x i32> undef, i32 %LowBits.32, i32 0
    %18 = insertelement <2 x i32> %17, i32 %HighBits.32, i32 1
    %19 = bitcast <2 x i32> %18 to double
*/

void GenSpecificPattern::visitBitCastInst(BitCastInst& I)
{
    if (I.getType()->isDoubleTy() && I.getOperand(0)->getType()->isIntegerTy(64))
    {
        BinaryOperator* binOperator = nullptr;
        if ((binOperator = dyn_cast<BinaryOperator>(I.getOperand(0))) && binOperator->getOpcode() == Instruction::Or)
        {
            if (isa<BitCastInst>(binOperator->getOperand(0)) && isa<ZExtInst>(binOperator->getOperand(1)))
            {
                BitCastInst* bitCastInst = cast<BitCastInst>(binOperator->getOperand(0));
                ZExtInst* zExtInst = cast<ZExtInst>(binOperator->getOperand(1));

                if (zExtInst->getOperand(0)->getType()->isIntegerTy(32) &&
                    isa<InsertElementInst>(bitCastInst->getOperand(0)) &&
                    bitCastInst->getOperand(0)->getType()->isVectorTy() &&
                    cast<IGCLLVM::FixedVectorType>(bitCastInst->getOperand(0)->getType())->getElementType()->isIntegerTy(32) &&
                    cast<IGCLLVM::FixedVectorType>(bitCastInst->getOperand(0)->getType())->getNumElements() == 2)
                {
                    InsertElementInst* insertElementInst = cast<InsertElementInst>(bitCastInst->getOperand(0));

                    if (isa<Constant>(insertElementInst->getOperand(0)) &&
                        cast<Constant>(insertElementInst->getOperand(0))->getAggregateElement((unsigned int)0)->isZeroValue() &&
                        cast<ConstantInt>(insertElementInst->getOperand(2))->getZExtValue() == 1)
                    {
                        IRBuilder<> builder(&I);
                        Value* vectorValue = UndefValue::get(bitCastInst->getOperand(0)->getType());
                        vectorValue = builder.CreateInsertElement(vectorValue, zExtInst->getOperand(0), builder.getInt32(0));
                        vectorValue = builder.CreateInsertElement(vectorValue, insertElementInst->getOperand(1), builder.getInt32(1));
                        Value* newBitCast = builder.CreateBitCast(vectorValue, builder.getDoubleTy());
                        I.replaceAllUsesWith(newBitCast);
                        I.eraseFromParent();
                    }
                }
            }
        }
    }
}

/*
    Matches a pattern where pointer to load instruction is fetched by other load instruction.
    On targets that do not support 64 bit operations, Emu64OpsPass will insert pair_to_ptr intrinsic
    between the loads and InstructionCombining will not optimize this case.

    This function changes following pattern:
    %3 = load <2 x i32>, <2 x i32> addrspace(1)* %2, align 64
    %4 = extractelement <2 x i32> %3, i32 0
    %5 = extractelement <2 x i32> %3, i32 1
    %6 = call %union._XReq addrspace(1)* addrspace(1)* addrspace(1)* addrspace(1)* addrspace(1)* addrspace(1)* addrspace(1)* addrspace(1)* @llvm.genx.GenISA.pair.to.ptr.p1p1p1p1p1p1p1p1union._XReq(i32 %4, i32 %5)
    %7 = bitcast %union._XReq addrspace(1)* addrspace(1)* addrspace(1)* addrspace(1)* addrspace(1)* addrspace(1)* addrspace(1)* addrspace(1)* %6 to i64 addrspace(1)*
    %8 = bitcast i64 addrspace(1)* %7 to <2 x i32> addrspace(1)*
    %9 = load <2 x i32>, <2 x i32> addrspace(1)* %8, align 64

    to:
    %3 = bitcast <2 x i32> addrspace(1)* %2 to <2 x i32> addrspace(1)* addrspace(1)*
    %4 = load <2 x i32> addrspace(1)*, <2 x i32> addrspace(1)* addrspace(1)* %3, align 64
    ... dead code
    %11 = load <2 x i32>, <2 x i32> addrspace(1)* %4, align 64
*/
void GenSpecificPattern::visitLoadInst(LoadInst &LI) {
    Value* PO = LI.getPointerOperand();
    std::vector<Value*> OneUseValues = { PO };
    while (isa<BitCastInst>(PO)) {
        PO = cast<BitCastInst>(PO)->getOperand(0);
        OneUseValues.push_back(PO);
    }

    bool IsPairToPtrInst = (isa<GenIntrinsicInst>(PO) &&
        cast<GenIntrinsicInst>(PO)->getIntrinsicID() ==
        GenISAIntrinsic::GenISA_pair_to_ptr);

    if (!IsPairToPtrInst)
        return;

    // check if this pointer comes from a load.
    auto CallInst = cast<GenIntrinsicInst>(PO);
    auto Op0 = dyn_cast<ExtractElementInst>(CallInst->getArgOperand(0));
    auto Op1 = dyn_cast<ExtractElementInst>(CallInst->getArgOperand(1));
    bool PointerComesFromALoad = (Op0 && Op1 && isa<ConstantInt>(Op0->getIndexOperand()) &&
        isa<ConstantInt>(Op1->getIndexOperand()) &&
        cast<ConstantInt>(Op0->getIndexOperand())->getZExtValue() == 0 &&
        cast<ConstantInt>(Op1->getIndexOperand())->getZExtValue() == 1 &&
        isa<LoadInst>(Op0->getVectorOperand()) &&
        isa<LoadInst>(Op1->getVectorOperand()) &&
        Op0->getVectorOperand() == Op1->getVectorOperand());

    if (!PointerComesFromALoad)
        return;

    OneUseValues.insert(OneUseValues.end(), { Op0, Op1 });

    if (!std::all_of(OneUseValues.begin(), OneUseValues.end(), [](auto v) { return v->hasOneUse(); }))
        return;

    auto VectorLoadInst = cast<LoadInst>(Op0->getVectorOperand());
    if (!VectorLoadInst->hasNUses(2))
        return;

    auto PointerOperand = VectorLoadInst->getPointerOperand();
    PointerType* newLoadPointerType = PointerType::get(
        LI.getPointerOperand()->getType(), PointerOperand->getType()->getPointerAddressSpace());
    IRBuilder<> builder(VectorLoadInst);
    auto CastedPointer =
        builder.CreateBitCast(PointerOperand, newLoadPointerType);
    auto NewLoadInst = IGC::cloneLoad(VectorLoadInst, LI.getPointerOperand()->getType(), CastedPointer);

    LI.setOperand(0, NewLoadInst);
}

void GenSpecificPattern::visitZExtInst(ZExtInst& ZEI)
{
    CmpInst* Cmp = dyn_cast<CmpInst>(ZEI.getOperand(0));
    if (!Cmp)
        return;

    using namespace llvm::PatternMatch;
    CmpInst::Predicate pred = CmpInst::Predicate::FCMP_FALSE;
    Instruction* I1 = nullptr;
    Instruction* I2 = nullptr;
    Instruction* I3 = nullptr;
    ConstantInt* C1 = nullptr, *C2 = nullptr;

    /*
    * from
    *   %add = add i32 %0, 1
    *   %cmp = icmp eq i32 %0, -1
    *   %conv = zext i1 %cmp to i32
    * to
    *   %call = call <2 x i32> @llvm.genx.GenISA.uaddc.v2i32.i32(i32 %0, i32 1)
    *   %zext.0 = extractelement <2 x i32> %call, i32 0     --> result
    *   %zext.1 = extractelement <2 x i32> %call, i32 1     --> carry
    */
    auto addcPattern1 = m_Cmp(pred, m_Instruction(I1), m_ConstantInt(C2));
    auto addcPattern2 = m_Add(m_Instruction(I2), m_ConstantInt(C1));

    /*
    * from
    *   %add.1 = add i32 %1, %conv
    *   %cmp.1 = icmp ult i32 %add.1, %1
    *   %conv.1 = zext i1 %cmp.1 to i32
    * to
    *   %call.1 = call <2 x i32> @llvm.genx.GenISA.uaddc.v2i32.i32(i32 %1, i32 %zext.1)
    *   %zext.2 = extractelement <2 x i32> %call.1, i32 0     --> result
    *   %zext.3 = extractelement <2 x i32> %call.1, i32 1     --> carry
    */
    auto addcPattern3 = m_Cmp(pred, m_Add(m_Instruction(I1), m_Instruction(I2)), m_Instruction(I3));

    if (match(Cmp, addcPattern1) && pred == CmpInst::Predicate::ICMP_EQ && C2->isMinusOne())
    {
        IGC_ASSERT(I1);
        for (auto U : I1->users())
        {
            Instruction* inst = dyn_cast<Instruction>(U);
            if (isActualAddInstr(inst) &&
                match(inst, addcPattern2) &&
                I1 == I2 &&
                ZEI.getType() == I2->getType() &&
                I2->getType()->isIntegerTy(32))
            {
                createAddcIntrinsicPattern(ZEI, I1, C1, I2, *inst);
                return;
            }
        }
    }
    else if (match(Cmp, addcPattern3) &&
        pred == CmpInst::Predicate::ICMP_ULT &&
        (I1 == I3 || I2 == I3) &&
        ZEI.getType() == I3->getType() &&
        I3->getType()->isIntegerTy(32))
    {
        Instruction* inst = dyn_cast<Instruction>(Cmp->getOperand(0));
        if (isActualAddInstr(inst))
        {
            createAddcIntrinsicPattern(ZEI, I1, I2, I3, *inst);
            return;
        }
    }

    IRBuilder<> Builder(&ZEI);

    Value* S = Builder.CreateSExt(Cmp, ZEI.getType());
    IGC_ASSERT(S);
    Value* N = Builder.CreateNeg(S);
    IGC_ASSERT(N);
    ZEI.replaceAllUsesWith(N);
    ZEI.eraseFromParent();
}

void GenSpecificPattern::visitIntToPtr(llvm::IntToPtrInst& I)
{
    if (ZExtInst * zext = dyn_cast<ZExtInst>(I.getOperand(0)))
    {
        IRBuilder<> builder(&I);
        Value* newV = builder.CreateIntToPtr(zext->getOperand(0), I.getType());
        I.replaceAllUsesWith(newV);
        I.eraseFromParent();
    }
}

void GenSpecificPattern::visitTruncInst(llvm::TruncInst& I)
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
    Value* LHS = nullptr;
    ConstantInt* CI = nullptr;
    if (match(&I, m_Trunc(m_LShr(m_Value(LHS), m_ConstantInt(CI)))) &&
        I.getType()->isIntegerTy(32) &&
        LHS->getType()->isIntegerTy(64) &&
        CI->getZExtValue() >= 32)
    {
        auto new_shift_size = (unsigned)CI->getZExtValue() - 32;
        llvm::IRBuilder<> builder(&I);
        VectorType* vec2 = IGCLLVM::FixedVectorType::get(builder.getInt32Ty(), 2);
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

#if LLVM_VERSION_MAJOR >= 10
void GenSpecificPattern::visitFNeg(llvm::UnaryOperator& I)
{
    // from
    // %neg = fneg double %1
    // to
    // %neg = fsub double -0.000000e+00, %1
    // vISA have parser which looks for such operation pattern "0 - x"
    // and adds source modifier for this region/value.

    IRBuilder<> builder(&I);
    Value* fsub = nullptr;

    if (IGCLLVM::isBFloatTy(I.getType())) {
        return;
    }

    if (!I.getType()->isVectorTy())
    {
        fsub = builder.CreateFSub(ConstantFP::get(I.getType(), 0.0f), I.getOperand(0));
    }
    else
    {
        uint32_t vectorSize = cast<IGCLLVM::FixedVectorType>(I.getType())->getNumElements();
        fsub = llvm::UndefValue::get(I.getType());

        for (uint32_t i = 0; i < vectorSize; ++i)
        {
            Value* extract = builder.CreateExtractElement(I.getOperand(0), i);
            Value* extract_fsub = builder.CreateFSub(ConstantFP::get(extract->getType(), 0.0f), extract);
            fsub = builder.CreateInsertElement(fsub, extract_fsub, i);
        }
    }

    I.replaceAllUsesWith(fsub);
}
#endif

static cl::opt<bool> overrideEnableSimplifyGEP(
    "override-enable-simplify-gep", cl::init(false), cl::Hidden,
    cl::desc("Override enableSimplifyGEP passed on init"));

// Register pass to igc-opt
#define PASS_FLAG3 "igc-const-prop"
#define PASS_DESCRIPTION3 "Custom Const-prop Pass"
#define PASS_CFG_ONLY3 false
#define PASS_ANALYSIS3 false
IGC_INITIALIZE_PASS_BEGIN(IGCConstProp, PASS_FLAG3, PASS_DESCRIPTION3, PASS_CFG_ONLY3, PASS_ANALYSIS3)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(IGCConstProp, PASS_FLAG3, PASS_DESCRIPTION3, PASS_CFG_ONLY3, PASS_ANALYSIS3)

char IGCConstProp::ID = 0;

IGCConstProp::IGCConstProp(
    bool enableSimplifyGEP) :
    FunctionPass(ID),
    m_enableSimplifyGEP(enableSimplifyGEP),
    m_TD(nullptr), m_TLI(nullptr)
{
    initializeIGCConstPropPass(*PassRegistry::getPassRegistry());
}

static Constant* GetConstantValue(Type* type, char* rawData)
{
    unsigned int size_in_bytes = (unsigned int)type->getPrimitiveSizeInBits() / 8;
    uint64_t returnConstant = 0;
    memcpy_s(&returnConstant, size_in_bytes, rawData, size_in_bytes);
    if (type->isIntegerTy())
    {
        return ConstantInt::get(type, returnConstant);
    }
    else if (type->isFloatingPointTy())
    {
        return  ConstantFP::get(type->getContext(),
            APFloat(type->getFltSemantics(), APInt((unsigned int)type->getPrimitiveSizeInBits(), returnConstant)));
    }
    return nullptr;
}


Constant* IGCConstProp::replaceShaderConstant(Instruction* inst)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData* modMD = ctx->getModuleMetaData();
    ConstantAddress cl;
    unsigned int& bufIdOrGRFOffset = cl.bufId;
    unsigned int& eltId = cl.eltId;
    unsigned int& size_in_bytes = cl.size;
    bool directBuf = false;
    bool statelessBuf = false;
    bool bindlessBuf = false;
    bool rootconstantBuf = false;
    unsigned int tableOffset = 0;
    if (getConstantAddress(*inst, cl, ctx, directBuf, statelessBuf, bindlessBuf, rootconstantBuf, tableOffset))
    {
        if (size_in_bytes)
        {
            if (modMD->immConstant.data.size() &&
                ((statelessBuf && (bufIdOrGRFOffset == modMD->pushInfo.inlineConstantBufferGRFOffset))||
                (directBuf && (bufIdOrGRFOffset == modMD->pushInfo.inlineConstantBufferSlot))))
            {
                char* offset = &(modMD->immConstant.data[0]);
                if (eltId >= modMD->immConstant.data.size())
                {
                    // OOB access to immediate constant buffer should return 0
                    return Constant::getNullValue(inst->getType());
                }
                char* pEltValue = nullptr;        // Pointer to element value
                if (inst->getType()->isVectorTy())
                {
                    Type* srcEltTy = cast<VectorType>(inst->getType())->getElementType();
                    uint32_t srcNElts = (uint32_t)cast<IGCLLVM::FixedVectorType>(inst->getType())->getNumElements();
                    uint32_t eltSize_in_bytes = (unsigned int)srcEltTy->getPrimitiveSizeInBits() / 8;
                    IRBuilder<> builder(inst);
                    Value* vectorValue = UndefValue::get(inst->getType());
                    for (uint i = 0; i < srcNElts; i++)
                    {
                        pEltValue = offset + eltId + (i * eltSize_in_bytes);
                        vectorValue = builder.CreateInsertElement(vectorValue,
                                                                  GetConstantValue(srcEltTy, pEltValue),
                                                                  builder.getInt32(i));
                    }
                    return dyn_cast<Constant>(vectorValue);
                }
                else
                {
                    pEltValue = offset + eltId;
                    return GetConstantValue(inst->getType(), pEltValue);
                }
            }
        }
    }
    return nullptr;
}

Constant* IGCConstProp::ConstantFoldCallInstruction(CallInst* inst)
{
    IGCConstantFolder constantFolder;
    Constant* C = nullptr;
    if (inst)
    {
        Constant* C0 = dyn_cast<Constant>(inst->getOperand(0));
        EOPCODE igcop = GetOpCode(inst);

        switch (igcop)
        {
        case llvm_gradientXfine:
        {
            if (C0)
            {
                C = constantFolder.CreateGradientXFine(C0);
            }
        }
        break;
        case llvm_gradientYfine:
        {
            if (C0)
            {
                C = constantFolder.CreateGradientYFine(C0);
            }
        }
        break;
        case llvm_gradientX:
        {
            if (C0)
            {
                C = constantFolder.CreateGradientX(C0);
            }
        }
        break;
        case llvm_gradientY:
        {
            if (C0)
            {
                C = constantFolder.CreateGradientY(C0);
            }
        }
        break;
        case llvm_rsq:
        {
            if (C0)
            {
                C = constantFolder.CreateRsq(C0);
            }
        }
        break;
        case llvm_roundne:
        {
            if (C0)
            {
                C = constantFolder.CreateRoundNE(C0);
            }
        }
        break;
        case llvm_fsat:
        {
            if (C0)
            {
                C = constantFolder.CreateFSat(C0);
            }
        }
        break;
        case llvm_fptrunc_rte:
        {
            if (C0)
            {
                C = constantFolder.CreateFPTrunc(C0, inst->getType(), llvm::APFloatBase::rmNearestTiesToEven);
            }
        }
        break;
        case llvm_fptrunc_rtz:
        {
            if (C0)
            {
                C = constantFolder.CreateFPTrunc(C0, inst->getType(), llvm::APFloatBase::rmTowardZero);
            }
        }
        break;
        case llvm_fptrunc_rtp:
        {
            if (C0)
            {
                C = constantFolder.CreateFPTrunc(C0, inst->getType(), llvm::APFloatBase::rmTowardPositive);
            }
        }
        break;
        case llvm_fptrunc_rtn:
        {
            if (C0)
            {
                C = constantFolder.CreateFPTrunc(C0, inst->getType(), llvm::APFloatBase::rmTowardNegative);
            }
        }
        break;
        case llvm_f32tof16_rtz:
        {
            if (C0)
            {
                C = constantFolder.CreateFPTrunc(C0, Type::getHalfTy(inst->getContext()), llvm::APFloatBase::rmTowardZero);
                C = constantFolder.CreateBitCast(C, Type::getInt16Ty(inst->getContext()));
                C = constantFolder.CreateZExtOrBitCast(C, Type::getInt32Ty(inst->getContext()));
                C = constantFolder.CreateBitCast(C, inst->getType());
            }
        }
        break;
        case llvm_fadd_rtz:
        {
            Constant* C1 = dyn_cast<Constant>(inst->getOperand(1));
            if (C0 && C1)
            {
                C = constantFolder.CreateFAdd(C0, C1, llvm::APFloatBase::rmTowardZero);
            }
        }
        break;
        case llvm_fmul_rtz:
        {
            Constant* C1 = dyn_cast<Constant>(inst->getOperand(1));
            if (C0 && C1)
            {
                C = constantFolder.CreateFMul(C0, C1, llvm::APFloatBase::rmTowardZero);
            }
        }
        break;
        case llvm_ubfe:
        {
            Constant* C1 = dyn_cast<Constant>(inst->getOperand(1));
            Constant* C2 = dyn_cast<Constant>(inst->getOperand(2));
            if (C0 && C0->isZeroValue())
            {
                C = llvm::ConstantInt::get(inst->getType(), 0);
            }
            else if (C0 && C1 && C2)
            {
                C = constantFolder.CreateUbfe(C0, C1, C2);
            }
        }
        break;
        case llvm_ibfe:
        {
            Constant* C1 = dyn_cast<Constant>(inst->getOperand(1));
            Constant* C2 = dyn_cast<Constant>(inst->getOperand(2));
            if (C0 && C0->isZeroValue())
            {
                C = llvm::ConstantInt::get(inst->getType(), 0);
            }
            else if (C0 && C1 && C2)
            {
                C = constantFolder.CreateIbfe(C0, C1, C2);
            }
        }
        break;
        case llvm_canonicalize:
        {
            // If the instruction should be emitted anyway, then remove the condition.
            // Please, be aware of the fact that clients can understand the term canonical FP value in other way.
            if (C0)
            {
                CodeGenContext* pCodeGenContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
                bool flushVal = pCodeGenContext->m_floatDenormMode16 == ::IGC::FLOAT_DENORM_FLUSH_TO_ZERO && inst->getType()->isHalfTy();
                flushVal = flushVal || (pCodeGenContext->m_floatDenormMode32 == ::IGC::FLOAT_DENORM_FLUSH_TO_ZERO && inst->getType()->isFloatTy());
                flushVal = flushVal || (pCodeGenContext->m_floatDenormMode64 == ::IGC::FLOAT_DENORM_FLUSH_TO_ZERO && inst->getType()->isDoubleTy());
                C = constantFolder.CreateCanonicalize(C0, flushVal);
            }
        }
        break;
        case llvm_fbh:
        {
            if (C0)
            {
                C = constantFolder.CreateFirstBitHi(C0);
            }
        }
        break;
        case llvm_fbh_shi:
        {
            if (C0)
            {
                C = constantFolder.CreateFirstBitShi(C0);
            }
        }
        break;
        case llvm_fbl:
        {
            if (C0)
            {
                C = constantFolder.CreateFirstBitLo(C0);
            }
        }
        break;
        case llvm_bfrev:
        {
            if (C0)
            {
                C = constantFolder.CreateBfrev(C0);
            }
        }
        break;
        case llvm_bfi:
        {
            Constant* C1 = dyn_cast<Constant>(inst->getOperand(1));
            Constant* C2 = dyn_cast<Constant>(inst->getOperand(2));
            Constant* C3 = dyn_cast<Constant>(inst->getOperand(3));
            if (C0 && C0->isZeroValue() && C3)
            {
                C = C3;
            }
            else if (C0 && C1 && C2 && C3)
            {
                C = constantFolder.CreateBfi(C0, C1, C2, C3);
            }
        }
        break;
        default:
            break;
        }
    }
    return C;
}

// constant fold the following code for any index:
//
// %95 = extractelement <4 x i16> <i16 3, i16 16, i16 21, i16 39>, i32 %94
// %96 = icmp eq i16 %95, 0
//
Constant* IGCConstProp::ConstantFoldCmpInst(CmpInst* CI)
{
    // Only handle scalar result.
    if (CI->getType()->isVectorTy())
        return nullptr;

    Value* LHS = CI->getOperand(0);
    Value* RHS = CI->getOperand(1);
    if (!isa<Constant>(RHS) && CI->isCommutative())
        std::swap(LHS, RHS);
    if (!isa<ConstantInt>(RHS) && !isa<ConstantFP>(RHS))
        return nullptr;

    auto EEI = dyn_cast<ExtractElementInst>(LHS);
    if (EEI && isa<Constant>(EEI->getVectorOperand()))
    {
        bool AllTrue = true, AllFalse = true;
        auto VecOpnd = cast<Constant>(EEI->getVectorOperand());
        unsigned N = (unsigned)cast<IGCLLVM::FixedVectorType>(VecOpnd->getType())->getNumElements();
        for (unsigned i = 0; i < N; ++i)
        {
            Constant* const Opnd = VecOpnd->getAggregateElement(i);
            IGC_ASSERT_MESSAGE(nullptr != Opnd, "null entry");

            if (isa<UndefValue>(Opnd))
                continue;
            Constant* Result = ConstantFoldCompareInstOperands(
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
Constant* IGCConstProp::ConstantFoldExtractElement(ExtractElementInst* EEI)
{

    Constant* EltIdx = dyn_cast<Constant>(EEI->getIndexOperand());
    if (EltIdx)
    {
        if (InsertElementInst * IEI = dyn_cast<InsertElementInst>(EEI->getVectorOperand()))
        {
            Constant* InsertIdx = dyn_cast<Constant>(IEI->getOperand(2));
            // try to find the constant from a chain of InsertElement
            while (IEI && InsertIdx)
            {
                if (InsertIdx == EltIdx)
                {
                    Constant* EltVal = dyn_cast<Constant>(IEI->getOperand(1));
                    return EltVal;
                }
                else
                {
                    Value* Vec = IEI->getOperand(0);
                    if (isa<ConstantDataVector>(Vec))
                    {
                        ConstantDataVector* CVec = cast<ConstantDataVector>(Vec);
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
        else if (SelectInst * sel = dyn_cast<SelectInst>(EEI->getVectorOperand()))
        {
            Value* vec0 = sel->getOperand(1);
            Value* vec1 = sel->getOperand(2);

            IGC_ASSERT(vec0->getType() == vec1->getType());

            if (isa<ConstantDataVector>(vec0) && isa<ConstantDataVector>(vec1))
            {
                ConstantDataVector* cvec0 = cast<ConstantDataVector>(vec0);
                ConstantDataVector* cvec1 = cast<ConstantDataVector>(vec1);
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
bool IGCConstProp::simplifyAdd(BinaryOperator* BO)
{
    // Only handle Add
    if (BO->getOpcode() != Instruction::Add)
    {
        return false;
    }

    Value* LHS = BO->getOperand(0);
    Value* RHS = BO->getOperand(1);
    bool changed = false;
    if (BinaryOperator * LBO = dyn_cast<BinaryOperator>(LHS))
    {
        if (simplifyAdd(LBO))
        {
            changed = true;
        }
    }
    if (BinaryOperator * RBO = dyn_cast<BinaryOperator>(RHS))
    {
        if (simplifyAdd(RBO))
        {
            changed = true;
        }
    }

    // Refresh LHS and RHS
    LHS = BO->getOperand(0);
    RHS = BO->getOperand(1);
    BinaryOperator* LHSbo = dyn_cast<BinaryOperator>(LHS);
    BinaryOperator* RHSbo = dyn_cast<BinaryOperator>(RHS);
    bool isLHSAdd = LHSbo && LHSbo->getOpcode() == Instruction::Add;
    bool isRHSAdd = RHSbo && RHSbo->getOpcode() == Instruction::Add;
    IRBuilder<> Builder(BO);
    if (isLHSAdd && isRHSAdd)
    {
        Value* A = LHSbo->getOperand(0);
        Value* B = LHSbo->getOperand(1);
        Value* C = RHSbo->getOperand(0);
        Value* D = RHSbo->getOperand(1);

        ConstantInt* C0 = dyn_cast<ConstantInt>(B);
        ConstantInt* C1 = dyn_cast<ConstantInt>(D);

        if (C0 || C1)
        {
            Value* R = nullptr;
            if (C0 && C1)
            {
                Value* newC = ConstantFoldBinaryOpOperands(Instruction::Add,
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
        Value* A = LHSbo->getOperand(0);
        Value* B = LHSbo->getOperand(1);
        Value* C = RHS;

        ConstantInt* C0 = dyn_cast<ConstantInt>(B);
        ConstantInt* C1 = dyn_cast<ConstantInt>(C);

        if (C0 && C1)
        {
            Value* newC = ConstantFoldBinaryOpOperands(Instruction::Add,
                C0, C1, *m_TD);
            Value* R = Builder.CreateAdd(A, newC);
            BO->replaceAllUsesWith(R);
            return true;
        }
        if (C0)
        {
            Value* R = Builder.CreateAdd(A, C);
            R = Builder.CreateAdd(R, B);
            BO->replaceAllUsesWith(R);
            return true;
        }
    }
    else if (isRHSAdd)
    {
        Value* A = LHS;
        Value* B = RHSbo->getOperand(0);
        Value* C = RHSbo->getOperand(1);

        ConstantInt* C0 = dyn_cast<ConstantInt>(A);
        ConstantInt* C1 = dyn_cast<ConstantInt>(C);

        if (C0 && C1)
        {
            Value* newC = ConstantFoldBinaryOpOperands(Instruction::Add,
                C0, C1, *m_TD);
            Value* R = Builder.CreateAdd(B, newC);
            BO->replaceAllUsesWith(R);
            return true;
        }
        if (C0)
        {
            Value* R = Builder.CreateAdd(RHS, A);
            BO->replaceAllUsesWith(R);
            return true;
        }
        if (C1)
        {
            Value* R = Builder.CreateAdd(A, B);
            R = Builder.CreateAdd(R, C);
            BO->replaceAllUsesWith(R);
            return true;
        }
    }
    else
    {
        if (ConstantInt * CLHS = dyn_cast<ConstantInt>(LHS))
        {
            if (ConstantInt * CRHS = dyn_cast<ConstantInt>(RHS))
            {
                Value* newC = ConstantFoldBinaryOpOperands(Instruction::Add,
                    CLHS, CRHS, *m_TD);
                BO->replaceAllUsesWith(newC);
                return true;
            }

            // Constant is kept as RHS
            Value* R = Builder.CreateAdd(RHS, LHS);
            BO->replaceAllUsesWith(R);
            return true;
        }
    }
    return changed;
}

bool IGCConstProp::simplifyGEP(GetElementPtrInst* GEP)
{
    bool changed = false;
    for (int i = 0; i < (int)GEP->getNumIndices(); ++i)
    {
        Value* Index = GEP->getOperand(i + 1);
        BinaryOperator* BO = dyn_cast<BinaryOperator>(Index);
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
bool IGCConstProp::runOnFunction(Function& F)
{
    module = F.getParent();
    // Initialize the worklist to all of the instructions ready to process...
    llvm::SetVector<Instruction*> WorkList;
    bool Changed = false;
    bool NotClosed;
    do // Fold constants until closure
    {
        NotClosed = false;
        for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i)
        {
            WorkList.insert(&*i);
        }
        m_TD = &F.getParent()->getDataLayout();
        m_TLI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
        while (!WorkList.empty())
        {
            Instruction* I = WorkList.pop_back_val(); // Get an element from the worklist...
            if (I->use_empty())                  // Don't muck with dead instructions...
            {
                continue;
            }
            Constant* C = nullptr;
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

                // Remove the dead instruction.
                I->eraseFromParent();

                // We made a change to the function...
                Changed = NotClosed = true;

                // I is erased, continue to the next one.
                continue;
            }

            if (GetElementPtrInst * GEP = dyn_cast<GetElementPtrInst>(I))
            {
                if ((m_enableSimplifyGEP || overrideEnableSimplifyGEP) && simplifyGEP(GEP))
                {
                    Changed = NotClosed = true;
                }
            }
        }
    } while (NotClosed);
    return Changed;
}

static bool isICBOffseted(llvm::LoadInst* inst, uint offset, uint& offsetIntoMergedBuffer)
{
    Value* ptrVal = inst->getPointerOperand();
    std::vector<Value*> srcInstList;
    IGC::TracePointerSource(ptrVal, false, true, true, srcInstList);
    if (srcInstList.size())
    {
        CallInst* inst = dyn_cast<CallInst>(srcInstList.back());
        GenIntrinsicInst* genIntr = inst ? dyn_cast<GenIntrinsicInst>(inst) : nullptr;
        if (!genIntr || (genIntr->getIntrinsicID() != GenISAIntrinsic::GenISA_RuntimeValue))
            return false;

        // ICB may contain multiple ICBs merged into one
        // find getelementptr after GenISA_RuntimeValue to find offset to needed ICB in merged ICB
        if (srcInstList.size() >= 2)
        {
            GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(srcInstList[srcInstList.size() - 2]);

            if (gep &&
                gep->getNumOperands() == 2 &&
                gep->getOperand(0) == genIntr &&
                genIntr->getType() == PointerType::get(Type::getInt8Ty(inst->getContext()), ADDRESS_SPACE_CONSTANT))
            {
                llvm::ConstantInt* ci = dyn_cast<llvm::ConstantInt>(gep->getOperand(1));

                if (ci)
                    offsetIntoMergedBuffer = (uint)ci->getZExtValue();
            }
        }

        llvm::ConstantInt* ci = dyn_cast<llvm::ConstantInt>(inst->getOperand(0));
        return ci && (uint)ci->getZExtValue() == offset;
    }

    return false;
}

namespace {

    class ClampICBOOBAccess : public FunctionPass
    {
    public:
        static char ID;
        ClampICBOOBAccess() : FunctionPass(ID)
        {
            initializeClampICBOOBAccessPass(*PassRegistry::getPassRegistry());
        }
        virtual llvm::StringRef getPassName() const { return "Clamp ICB OOB Access"; }
        virtual bool runOnFunction(Function& F);
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
        }
    };

} // namespace

char ClampICBOOBAccess::ID = 0;
FunctionPass* IGC::createClampICBOOBAccess() { return new ClampICBOOBAccess(); }

IGC_INITIALIZE_PASS_BEGIN(ClampICBOOBAccess, "ClampICBOOBAccess", "ClampICBOOBAccess", false, false)
IGC_INITIALIZE_PASS_END(ClampICBOOBAccess, "ClampICBOOBAccess", "ClampICBOOBAccess", false, false)

bool ClampICBOOBAccess::runOnFunction(Function& F)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData* modMD = ctx->getModuleMetaData();
    IRBuilder<> m_builder(F.getContext());
    bool changed = false;

    for (auto& BB : F)
    {
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE;)
        {
            if (llvm::LoadInst* inst = llvm::dyn_cast<llvm::LoadInst>(&(*BI++)))
            {
                unsigned offsetIntoMergedBuffer = 0;

                if (isICBOffseted(inst, modMD->pushInfo.inlineConstantBufferOffset, offsetIntoMergedBuffer))
                {
                    Value* ptrVal = inst->getPointerOperand();

                    if (GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(ptrVal))
                    {
                        if (gep->getNumOperands() != 3)
                        {
                            continue;
                        }

                        Type* eleType = gep->getSourceElementType();
                        if (!eleType->isArrayTy() ||
                            !(eleType->getArrayElementType()->isFloatTy() || eleType->getArrayElementType()->isIntegerTy(32)))
                        {
                            continue;
                        }

                        // Want to compare index into ICB to ensure index doesn't go past the size of the ICB
                        // If it does clamp to some index where a zero is stored
                        m_builder.SetInsertPoint(gep);
                        uint64_t arrSize = gep->getSourceElementType()->getArrayNumElements();
                        Value* eltIdx = gep->getOperand(2);
                        Value* isOOB = m_builder.CreateICmp(ICmpInst::ICMP_UGE, eltIdx, llvm::ConstantInt::get(eltIdx->getType(), arrSize));
                        unsigned zeroIndex = modMD->immConstant.zeroIdxs[offsetIntoMergedBuffer];
                        Value* eltIdxClampped = m_builder.CreateSelect(isOOB, llvm::ConstantInt::get(eltIdx->getType(), zeroIndex), eltIdx);

                        gep->replaceUsesOfWith(eltIdx, eltIdxClampped);

                        changed = true;
                    }
                }
            }
        }
    }

    return changed;
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
        virtual llvm::StringRef getPassName() const { return "Indirect ICB Propagation"; }
        virtual bool runOnFunction(Function& F);
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
        }
    };

} // namespace

char IGCIndirectICBPropagaion::ID = 0;
FunctionPass* IGC::createIGCIndirectICBPropagaionPass() { return new IGCIndirectICBPropagaion(); }

bool IGCIndirectICBPropagaion::runOnFunction(Function& F)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData* modMD = ctx->getModuleMetaData();

    //MaxImmConstantSizePushed = 256 by default. For float values, it will contains 64 numbers, and stored in 8 GRF
    if (modMD &&
        modMD->immConstant.data.size() &&
        modMD->immConstant.data.size() <= IGC_GET_FLAG_VALUE(MaxImmConstantSizePushed))
    {
        IRBuilder<> m_builder(F.getContext());

        for (auto& BB : F)
        {
            for (auto BI = BB.begin(), BE = BB.end(); BI != BE;)
            {
                if (llvm::LoadInst * inst = llvm::dyn_cast<llvm::LoadInst>(&(*BI++)))
                {
                    unsigned as = inst->getPointerAddressSpace();
                    bool directBuf = false;
                    unsigned bufId = 0;
                    unsigned offsetIntoMergedBuffer = 0;
                    BufferType bufType = IGC::DecodeAS4GFXResource(as, directBuf, bufId);
                    bool bICBNoOffset =
                        (IGC::INVALID_CONSTANT_BUFFER_INVALID_ADDR == modMD->pushInfo.inlineConstantBufferOffset && bufType == CONSTANT_BUFFER && directBuf && bufId == modMD->pushInfo.inlineConstantBufferSlot);
                    bool bICBOffseted =
                        (IGC::INVALID_CONSTANT_BUFFER_INVALID_ADDR != modMD->pushInfo.inlineConstantBufferOffset && ADDRESS_SPACE_CONSTANT == as && isICBOffseted(inst, modMD->pushInfo.inlineConstantBufferOffset, offsetIntoMergedBuffer));
                    if (bICBNoOffset || bICBOffseted)
                    {
                        Value* ptrVal = inst->getPointerOperand();
                        Value* eltPtr = nullptr;
                        Value* eltIdx = nullptr;
                        if (IntToPtrInst * i2p = dyn_cast<IntToPtrInst>(ptrVal))
                        {
                            eltPtr = i2p->getOperand(0);
                        }
                        else if (GetElementPtrInst * gep = dyn_cast<GetElementPtrInst>(ptrVal))
                        {
                            if (gep->getNumOperands() != 3)
                            {
                                continue;
                            }

                            Type* eleType = gep->getSourceElementType();
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

                        unsigned int size_in_bytes = (unsigned int)inst->getType()->getPrimitiveSizeInBits() / 8;
                        if (size_in_bytes)
                        {
                            uint maxImmConstantSizePushed = !modMD->immConstant.sizes.empty() ? modMD->immConstant.sizes[offsetIntoMergedBuffer] : modMD->immConstant.data.size();
                            char* offset = &(modMD->immConstant.data[0]) + offsetIntoMergedBuffer;
                            Value* ICBbuffer = UndefValue::get(IGCLLVM::FixedVectorType::get(inst->getType(), maxImmConstantSizePushed / size_in_bytes));
                            if (inst->getType()->isFloatTy())
                            {
                                float returnConstant = 0;
                                for (unsigned int i = 0; i < maxImmConstantSizePushed; i += size_in_bytes)
                                {
                                    memcpy_s(&returnConstant, size_in_bytes, offset + i, size_in_bytes);
                                    Value* fp = ConstantFP::get(inst->getType(), returnConstant);
                                    ICBbuffer = m_builder.CreateInsertElement(ICBbuffer, fp, m_builder.getInt32(i / size_in_bytes));
                                }

                                if (eltPtr)
                                {
                                    eltIdx = m_builder.CreateLShr(eltPtr, m_builder.getInt32(2));
                                }
                                Value* ICBvalue = m_builder.CreateExtractElement(ICBbuffer, eltIdx);
                                inst->replaceAllUsesWith(ICBvalue);
                            }
                            else if (inst->getType()->isIntegerTy(32))
                            {
                                int returnConstant = 0;
                                for (unsigned int i = 0; i < maxImmConstantSizePushed; i += size_in_bytes)
                                {
                                    memcpy_s(&returnConstant, size_in_bytes, offset + i, size_in_bytes);
                                    Value* fp = ConstantInt::get(inst->getType(), returnConstant);
                                    ICBbuffer = m_builder.CreateInsertElement(ICBbuffer, fp, m_builder.getInt32(i / size_in_bytes));
                                }
                                if (eltPtr)
                                {
                                    eltIdx = m_builder.CreateLShr(eltPtr, m_builder.getInt32(2));
                                }
                                Value* ICBvalue = m_builder.CreateExtractElement(ICBbuffer, eltIdx);
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
        virtual bool runOnFunction(llvm::Function& F);
        void visitBranchInst(llvm::BranchInst& I);
        void loopNanCases(Function& F);

    private:
        int longestPathInstCount(llvm::BasicBlock* BB, int& depth);
        void swapBranch(llvm::Instruction* inst, llvm::BranchInst& BI);
        SmallVector<llvm::BranchInst*, 10> visitedInst;
    };
} // namespace

char NanHandling::ID = 0;
FunctionPass* IGC::createNanHandlingPass() { return new NanHandling(); }

bool NanHandling::runOnFunction(Function& F)
{
    loopNanCases(F);
    visit(F);
    return true;
}

void NanHandling::loopNanCases(Function& F)
{
    // take care of loop cases
    visitedInst.clear();
    llvm::LoopInfo* LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    if (LI && !LI->empty())
    {
        FastMathFlags FMF;
        FMF.clear();
        for (Loop* loop : *LI)
        {
            BranchInst* br = cast<BranchInst>(loop->getLoopLatch()->getTerminator());
            BasicBlock* header = loop->getHeader();
            if (br && br->isConditional() && header)
            {
                visitedInst.push_back(br);
                if (FCmpInst * brCmpInst = dyn_cast<FCmpInst>(br->getCondition()))
                {
                    FPMathOperator* FPO = dyn_cast<FPMathOperator>(brCmpInst);
                    if (!FPO || !FPO->isFast())
                    {
                        continue;
                    }
                    if (br->getSuccessor(1) == header)
                    {
                        swapBranch(brCmpInst, *br);
                    }
                }
                else if (BinaryOperator * andOrInst = dyn_cast<BinaryOperator>(br->getCondition()))
                {
                    if (andOrInst->getOpcode() != BinaryOperator::And &&
                        andOrInst->getOpcode() != BinaryOperator::Or)
                    {
                        continue;
                    }
                    FCmpInst* brCmpInst0 = dyn_cast<FCmpInst>(andOrInst->getOperand(0));
                    FCmpInst* brCmpInst1 = dyn_cast<FCmpInst>(andOrInst->getOperand(1));
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

int NanHandling::longestPathInstCount(llvm::BasicBlock* BB, int& depth)
{
#define MAX_SEARCH_DEPTH 10

    depth++;
    if (!BB || depth > MAX_SEARCH_DEPTH)
        return 0;

    int sumSuccInstCount = 0;
    for (succ_iterator SI = succ_begin(BB), E = succ_end(BB); SI != E; ++SI)
    {
        sumSuccInstCount += longestPathInstCount(*SI, depth);
    }
    return (int)(BB->getInstList().size()) + sumSuccInstCount;
}

void NanHandling::swapBranch(llvm::Instruction* inst, llvm::BranchInst& BI)
{
    if (FCmpInst * brCondition = dyn_cast<FCmpInst>(inst))
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
        IGC_ASSERT(0);
    }
}

void NanHandling::visitBranchInst(llvm::BranchInst& I)
{
    if (!I.isConditional())
        return;

    // if this branch is part of a loop, it is taken care of already in loopNanCases
    if (std::find(visitedInst.begin(), visitedInst.end(), &I) != visitedInst.end())
        return;

    FCmpInst* brCmpInst = dyn_cast<FCmpInst>(I.getCondition());
    FCmpInst* src0 = nullptr;
    FCmpInst* src1 = nullptr;

    // if the branching is based on a cmp instruction
    if (brCmpInst)
    {
        FPMathOperator* FPO = dyn_cast<FPMathOperator>(brCmpInst);
        if (!FPO || !FPO->isFast())
            return;

        if (!brCmpInst->hasOneUse())
            return;
    }
    // if the branching is based on a and/or from multiple conditions.
    else if (BinaryOperator * andOrInst = dyn_cast<BinaryOperator>(I.getCondition()))
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

namespace IGC
{

    class VectorBitCastOpt: public FunctionPass
    {
    public:
        static char ID;

        VectorBitCastOpt() : FunctionPass(ID)
        {
            initializeVectorBitCastOptPass(*PassRegistry::getPassRegistry());
        };
        ~VectorBitCastOpt() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "VectorBitCastOptPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

    private:
        // Transform (extractelement (bitcast %vector) ...) to
        // (bitcast (extractelement %vector) ...) in order to help coalescing
        // in DeSSA and enable memory operations simplification
        // in VectorPreProcess.
        bool optimizeVectorBitCast(Function& F) const;
    };

    bool VectorBitCastOpt::runOnFunction(Function& F)
    {
        bool Changed = optimizeVectorBitCast(F);
        return Changed;
    }

    bool VectorBitCastOpt::optimizeVectorBitCast(Function& F) const {
        IRBuilder<> Builder(F.getContext());

        bool Changed = false;
        for (auto& BB : F) {
            for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*EMPTY*/) {
                BitCastInst* BC = dyn_cast<BitCastInst>(&*BI++);
                if (!BC) continue;
                // Skip non-element-wise bitcast.
                IGCLLVM::FixedVectorType* DstVTy = dyn_cast<IGCLLVM::FixedVectorType>(BC->getType());
                IGCLLVM::FixedVectorType* SrcVTy = dyn_cast<IGCLLVM::FixedVectorType>(BC->getOperand(0)->getType());
                if (!DstVTy || !SrcVTy || DstVTy->getNumElements() != SrcVTy->getNumElements())
                    continue;
                // Skip if it's not used only all extractelement.
                bool ExactOnly = true;
                for (auto User : BC->users()) {
                    if (isa<ExtractElementInst>(User)) continue;
                    ExactOnly = false;
                    break;
                }
                if (!ExactOnly)
                    continue;
                // Autobots, transform and roll out!
                Value* Src = BC->getOperand(0);
                Type* DstEltTy = DstVTy->getElementType();
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

    char VectorBitCastOpt::ID = 0;
    FunctionPass* createVectorBitCastOptPass() { return new VectorBitCastOpt(); }

} // namespace IGC

#define VECTOR_BITCAST_OPT_PASS_FLAG "igc-vector-bitcast-opt"
#define VECTOR_BITCAST_OPT_PASS_DESCRIPTION "Preprocess vector bitcasts to be after extractelement instructions."
#define VECTOR_BITCAST_OPT_PASS_CFG_ONLY true
#define VECTOR_BITCAST_OPT_PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(VectorBitCastOpt, VECTOR_BITCAST_OPT_PASS_FLAG, VECTOR_BITCAST_OPT_PASS_DESCRIPTION, VECTOR_BITCAST_OPT_PASS_CFG_ONLY, VECTOR_BITCAST_OPT_PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(VectorBitCastOpt, VECTOR_BITCAST_OPT_PASS_FLAG, VECTOR_BITCAST_OPT_PASS_DESCRIPTION, VECTOR_BITCAST_OPT_PASS_CFG_ONLY, VECTOR_BITCAST_OPT_PASS_ANALYSIS)

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
        virtual bool runOnFunction(Function& F);

    private:
        bool processInst(Instruction* Inst);
    };

} // namespace

char GenStrengthReduction::ID = 0;
FunctionPass* IGC::createGenStrengthReductionPass() { return new GenStrengthReduction(); }

bool GenStrengthReduction::runOnFunction(Function& F)
{
    bool Changed = false;
    for (auto& BB : F)
    {
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE;)
        {
            Instruction* Inst = &(*BI++);
            if (isInstructionTriviallyDead(Inst))
            {
                Inst->eraseFromParent();
                Changed = true;
                continue;
            }
            Changed |= processInst(Inst);
        }
    }
    return Changed;
}

// Check if this is a fdiv that allows reciprocal, and its divident is not known
// to be 1.0.
static bool isCandidateFDiv(Instruction* Inst)
{
    // Only floating points, and no vectors.
    if (!Inst->getType()->isFloatingPointTy() || Inst->use_empty())
        return false;

    auto Op = dyn_cast<FPMathOperator>(Inst);
    if (Op && Op->getOpcode() == Instruction::FDiv && Op->hasAllowReciprocal())
    {
        Value* Src0 = Op->getOperand(0);
        if (auto CFP = dyn_cast<ConstantFP>(Src0))
            return !CFP->isExactlyValue(1.0);
        return true;
    }
    return false;
}

bool GenStrengthReduction::processInst(Instruction* Inst)
{

    unsigned opc = Inst->getOpcode();
    auto Op = dyn_cast<FPMathOperator>(Inst);
    if (opc == Instruction::Select)
    {
        Value* oprd1 = Inst->getOperand(1);
        Value* oprd2 = Inst->getOperand(2);
        ConstantFP* CF1 = dyn_cast<ConstantFP>(oprd1);
        ConstantFP* CF2 = dyn_cast<ConstantFP>(oprd2);
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
        case Instruction::FDiv:
        {
            Value* Oprd0 = Inst->getOperand(0);
            if (ConstantFP * CF = dyn_cast<ConstantFP>(Oprd0))
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
        case Instruction::FMul:
        {
            for (int i = 0; i < 2; ++i)
            {
                ConstantFP* CF = dyn_cast<ConstantFP>(Inst->getOperand(i));
                if (CF && CF->isZero())
                {
                    Inst->replaceAllUsesWith(CF);
                    Inst->eraseFromParent();
                    return true;
                }
            }
            break;
        }
        case Instruction::FAdd:
        {
            for (int i = 0; i < 2; ++i)
            {
                ConstantFP* CF = dyn_cast<ConstantFP>(Inst->getOperand(i));
                if (CF && CF->isZero())
                {
                    Value* otherOprd = Inst->getOperand(1 - i);
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
        Value* Src1 = Inst->getOperand(1);
        if (isa<Constant>(Src1))
        {
            // should not happen (but do see "fdiv  x / 0.0f"). Skip.
            return false;
        }

        Value* Src0 = ConstantFP::get(Inst->getType(), 1.0);
        Instruction* Inv = nullptr;

        // Check if there is any other (x / Src1). If so, commonize 1/Src1.
        for (auto UI = Src1->user_begin(), UE = Src1->user_end();
            UI != UE; ++UI)
        {
            Value* Val = *UI;
            Instruction* I = dyn_cast<Instruction>(Val);
            if (I && I != Inst && I->getOpcode() == Instruction::FDiv &&
                I->getOperand(1) == Src1 && isCandidateFDiv(I))
            {
                // special case
                if (ConstantFP * CF = dyn_cast<ConstantFP>(I->getOperand(0)))
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
                    Instruction* insertBefore = dyn_cast<Instruction>(Src1);
                    if (insertBefore)
                    {
                        if (isa<PHINode>(insertBefore))
                        {
                            BasicBlock* BB = insertBefore->getParent();
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
                        BasicBlock& entryBB = Inst->getParent()->getParent()->getEntryBlock();
                        insertBefore = &(*entryBB.getFirstInsertionPt());
                    }
                    Inv = BinaryOperator::CreateFDiv(Src0, Src1, "", insertBefore);
                    Inv->setFastMathFlags(Inst->getFastMathFlags());
                    Inv->setDebugLoc(Inst->getDebugLoc());
                }

                Instruction* Mul = BinaryOperator::CreateFMul(I->getOperand(0), Inv, "", I);
                Mul->setFastMathFlags(Inst->getFastMathFlags());
                Mul->setDebugLoc(I->getDebugLoc());
                I->replaceAllUsesWith(Mul);
                // Don't erase it as doing so would invalidate iterator in this func's caller
                // Instead, erase it in the caller.
                // I->eraseFromParent();
            }
        }

        if (!Inv)
        {
            if (Inst->getType()->isDoubleTy())
                return false;

            // Only a single use of 1 / Src1. Create Inv right before the use.
            Inv = BinaryOperator::CreateFDiv(Src0, Src1, "", Inst);
            Inv->setFastMathFlags(Inst->getFastMathFlags());
            Inv->setDebugLoc(Inst->getDebugLoc());
        }

        bool bUseFSub = false;
        if (ConstantFP* CF = dyn_cast<ConstantFP>(Inst->getOperand(0)))
        {
            // Rewrite
            // %1 = fdiv float -1.0, %x
            // into
            // %1 = fdiv float 1.0, %x
            // %2 = fsub float 0, %1
            if (CF->isExactlyValue(-1))
                bUseFSub = true;
        }

        Instruction *NewInst = nullptr;
        if (bUseFSub)
        {
            NewInst = BinaryOperator::CreateFSub(
                ConstantFP::get(Inst->getType(), 0), Inv, "", Inst);
        }
        else
        {
            NewInst = BinaryOperator::CreateFMul(Inst->getOperand(0), Inv, "", Inst);
        }
        NewInst->setFastMathFlags(Inst->getFastMathFlags());
        NewInst->setDebugLoc(Inst->getDebugLoc());
        Inst->replaceAllUsesWith(NewInst);
        Inst->eraseFromParent();
        return true;
    }

    return false;
}

IGC_INITIALIZE_PASS_BEGIN(GenStrengthReduction, "GenStrengthReduction",
    "GenStrengthReduction", false, false)
    IGC_INITIALIZE_PASS_END(GenStrengthReduction, "GenStrengthReduction",
        "GenStrengthReduction", false, false)

/*========================== FlattenSmallSwitch ==============================

This class flattens small switch. For example,

before optimization:
    then153:
    switch i32 %115, label %else229 [
    i32 1, label %then214
    i32 2, label %then222
    i32 3, label %then222 ; duplicate blocks are fine
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
        virtual bool runOnFunction(Function& F);
        bool processSwitchInst(SwitchInst* SI);
    };
} // namespace

char FlattenSmallSwitch::ID = 0;
FunctionPass* IGC::createFlattenSmallSwitchPass() { return new FlattenSmallSwitch(); }

bool FlattenSmallSwitch::processSwitchInst(SwitchInst* SI)
{
    const unsigned maxSwitchCases = 3;  // only apply to switch with 3 cases or less
    const unsigned maxCaseInsts = 3;    // only apply optimization when each case has 3 instructions or less.

    if (SI->getNumCases() > maxSwitchCases || SI->getNumCases() == 0)
    {
        return false;
    }

    // Dest will be the block that the control flow from the switch merges to.
    // Currently, there are two options:
    // 1. The Dest block is the default block from the switch
    // 2. The Dest block is jumped to by all of the switch cases (and the default)
    BasicBlock* Dest = nullptr;
    {
        const auto* CaseSucc =
            SI->case_begin()->getCaseSuccessor();
        auto* BI = dyn_cast<BranchInst>(CaseSucc->getTerminator());

        if (BI == nullptr)
            return false;

        if (BI->isConditional())
            return false;

        // We know the first case jumps to this block.  Now let's
        // see below whether all the cases jump to this same block.
        Dest = BI->getSuccessor(0);
    }

    // Does BB unconditionally branch to MergeBlock?
    auto branchPattern = [](const BasicBlock* BB, const BasicBlock* MergeBlock)
    {
        auto* br = dyn_cast<BranchInst>(BB->getTerminator());

        if (br == nullptr)
            return false;

        if (br->isConditional())
            return false;

        if (br->getSuccessor(0) != MergeBlock)
            return false;

        if (!BB->getUniquePredecessor())
            return false;

        return true;
    };

    // We can speculatively execute a basic block if it
    // is small, unconditionally branches to Dest, and doesn't
    // have high latency or unsafe to speculate instructions.
    auto canSpeculateBlock = [&](const BasicBlock* BB)
    {
        if (BB->size() > maxCaseInsts)
            return false;

        if (!branchPattern(BB, Dest))
            return false;

        for (auto& I : *BB)
        {
            auto* inst = &I;

            if (isa<BranchInst>(inst))
                continue;

            // if there is any high-latency instruction in the switch,
            // don't flatten it
            if (isSampleInstruction(inst) ||
                isGather4Instruction(inst) ||
                isInfoInstruction(inst) ||
                isLdInstruction(inst) ||
                // If the instruction can't be speculated (e.g., phi node),
                // punt.
                !isSafeToSpeculativelyExecute(inst))
            {
                return false;
            }
        }

        return true;
    };

    // Are all Phi incomming blocks from SI switch?
    auto checkPhiPredecessorBlocks = [](const SwitchInst* SI, const PHINode* Phi, bool DefaultMergeBlock)
    {
        for (auto* BB : Phi->blocks())
        {
            if (BB == SI->getDefaultDest())
                continue;
            if (std::any_of(SI->case_begin(), SI->case_end(), [BB](auto &Case) { return Case.getCaseSuccessor() == BB; }))
                continue;
            return false;
        }
        return true;
    };

    for (auto& I : SI->cases())
    {
        BasicBlock* CaseDest = I.getCaseSuccessor();

        if (!canSpeculateBlock(CaseDest))
            return false;
    }

    // Is the default case of the switch the block
    // where all other cases meet?
    BasicBlock* Default = SI->getDefaultDest();
    const bool DefaultMergeBlock = (Dest == Default);

    // If we merge to the default block, there is no block
    // we jump to beforehand so there is nothing to
    // speculate.
    if (!DefaultMergeBlock && !canSpeculateBlock(Default))
        return false;

    // Get all PHI nodes that needs to be replaced
    SmallVector<PHINode*, 4> PhiNodes;
    for (auto& I : *Dest)
    {
        auto* Phi = dyn_cast<PHINode>(&I);

        if (!Phi)
            break;

        if (!checkPhiPredecessorBlocks(SI, Phi, DefaultMergeBlock))
            return false;

        PhiNodes.push_back(Phi);
    }

    if (PhiNodes.empty())
        return false;

    Value* Val = SI->getCondition();  // The value we are switching on...
    IRBuilder<> builder(SI);

    // Move all instructions except the last (i.e., the branch)
    // from BB to the InsertPoint.
    auto splice = [](BasicBlock* BB, Instruction* InsertPoint)
    {
        for (auto II = BB->begin(), IE = BB->end(); II != IE; /* empty */)
        {
            auto* I = &*II++;
            if (I->isTerminator())
                return;

            I->moveBefore(InsertPoint);
        }
    };

    // move default block out
    if (!DefaultMergeBlock)
        splice(Default, SI);

    // move case blocks out
    for (auto& I : SI->cases())
    {
        BasicBlock* CaseDest = I.getCaseSuccessor();
        splice(CaseDest, SI);
    }

    // replaces PHI with select
    for (auto* Phi : PhiNodes)
    {
        Value* vTemp = Phi->getIncomingValueForBlock(
            DefaultMergeBlock ? SI->getParent() : Default);

        for (auto& I : SI->cases())
        {
            BasicBlock* CaseDest = I.getCaseSuccessor();
            ConstantInt* CaseValue = I.getCaseValue();

            Value* selTrueValue = Phi->getIncomingValueForBlock(CaseDest);
            builder.SetInsertPoint(SI);
            Value* cmp = builder.CreateICmp(CmpInst::Predicate::ICMP_EQ, Val, CaseValue);
            builder.SetCurrentDebugLocation(Phi->getDebugLoc());
            Value* sel = builder.CreateSelect(cmp, selTrueValue, vTemp);
            vTemp = sel;
        }

        Phi->replaceAllUsesWith(vTemp);
        Phi->eraseFromParent();
    }

    // connect the original block and the phi node block with a pass through branch
    builder.CreateBr(Dest);

    SmallPtrSet<BasicBlock*, 4> Succs;

    // Remove the switch.
    BasicBlock* SelectBB = SI->getParent();
    for (unsigned i = 0, e = SI->getNumSuccessors(); i < e; ++i)
    {
        BasicBlock* Succ = SI->getSuccessor(i);
        if (Succ == Dest)
        {
            continue;
        }

        if (Succs.insert(Succ).second)
            Succ->removePredecessor(SelectBB);
    }
    SI->eraseFromParent();

    return true;
}

bool FlattenSmallSwitch::runOnFunction(Function& F)
{
    bool Changed = false;
    for (Function::iterator I = F.begin(), E = F.end(); I != E; )
    {
        BasicBlock* Cur = &*I++; // Advance over block so we don't traverse new blocks
        if (SwitchInst * SI = dyn_cast<SwitchInst>(Cur->getTerminator()))
        {
            Changed |= processSwitchInst(SI);
        }
    }
    return Changed;
}

IGC_INITIALIZE_PASS_BEGIN(FCmpPaternMatch, "FCmpPaternMatch", "FCmpPaternMatch", false, false)
IGC_INITIALIZE_PASS_END(FCmpPaternMatch, "FCmpPaternMatch", "FCmpPaternMatch", false, false)

char FCmpPaternMatch::ID = 0;

FCmpPaternMatch::FCmpPaternMatch() : FunctionPass(ID)
{
    initializeFCmpPaternMatchPass(*PassRegistry::getPassRegistry());
}

bool FCmpPaternMatch::runOnFunction(Function& F)
{
    bool change = true;
    visit(F);
    return change;
}

void FCmpPaternMatch::visitSelectInst(SelectInst& I)
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
        ConstantFP* Cfp1 = dyn_cast<ConstantFP>(I.getOperand(1));
        ConstantFP* Cfp2 = dyn_cast<ConstantFP>(I.getOperand(2));
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
                if (BitCastInst * bitcastInst = dyn_cast<BitCastInst>(bitCastI))
                {
                    for (auto cmpI : bitcastInst->users())
                    {
                        ICmpInst* iCmpInst = dyn_cast<ICmpInst>(cmpI);
                        if (iCmpInst &&
                            iCmpInst->isEquality())
                        {
                            ConstantInt* icmpC = dyn_cast<ConstantInt>(iCmpInst->getOperand(1));
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

                                if (SelectInst * selInst = dyn_cast<SelectInst>(brOrSelI))
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
}

IGC_INITIALIZE_PASS_BEGIN(FlattenSmallSwitch, "flattenSmallSwitch", "flattenSmallSwitch", false, false)
IGC_INITIALIZE_PASS_END(FlattenSmallSwitch, "flattenSmallSwitch", "flattenSmallSwitch", false, false)



/*======================== SplitIndirectEEtoSel =============================

This class changes extract element for small vectors to series of cmp+sel to avoid VxH mov.
before:
  %268 = mul nuw i32 %res.i2.i, 3
  %269 = extractelement <12 x float> %234, i32 %268
  %270 = add i32 %268, 1
  %271 = extractelement <12 x float> %234, i32 %270
  %272 = add i32 %268, 2
  %273 = extractelement <12 x float> %234, i32 %272
  %274 = extractelement <12 x float> %198, i32 %268
  %275 = extractelement <12 x float> %198, i32 %270
  %276 = extractelement <12 x float> %198, i32 %272

after:
  %250 = icmp eq i32 %res.i2.i, i16 1
  %251 = select i1 %250, float %206, float %200
  %252 = select i1 %250, float %208, float %202
  %253 = select i1 %250, float %210, float %204
  %254 = select i1 %250, float %48, float %32
  %255 = select i1 %250, float %49, float %33
  %256 = select i1 %250, float %50, float %34
  %257 = icmp eq i32 %res.i2.i, i16 2
  %258 = select i1 %257, float %214, float %251
  %259 = select i1 %257, float %215, float %252
  %260 = select i1 %257, float %216, float %253
  %261 = select i1 %257, float %64, float %254
  %262 = select i1 %257, float %65, float %255
  %263 = select i1 %257, float %66, float %256

  It is a bit similar to SimplifyConstant::isCmpSelProfitable for OCL, but not restricted to api.
  And to GenSimplification::visitExtractElement() but not restricted to vec of 2, and later.
  TODO: for known vectors check how many unique items there are.
===========================================================================*/
namespace {
    class SplitIndirectEEtoSel : public FunctionPass, public llvm::InstVisitor<SplitIndirectEEtoSel>
    {
    public:
        static char ID;
        SplitIndirectEEtoSel() : FunctionPass(ID)
        {
            initializeSplitIndirectEEtoSelPass(*PassRegistry::getPassRegistry());
        }
        virtual llvm::StringRef getPassName() const { return "Split Indirect EE to ICmp Plus Sel"; }
        virtual bool runOnFunction(Function& F);
        void visitExtractElementInst(llvm::ExtractElementInst& I);
    private:
        bool isProfitableToSplit(uint64_t num, int64_t mul, int64_t add);
        bool didSomething;
    };
} // namespace


char SplitIndirectEEtoSel::ID = 0;
FunctionPass* IGC::createSplitIndirectEEtoSelPass() { return new SplitIndirectEEtoSel(); }

bool SplitIndirectEEtoSel::runOnFunction(Function& F)
{
    didSomething = false;
    visit(F);
    return didSomething;
}

bool SplitIndirectEEtoSel::isProfitableToSplit(uint64_t num, int64_t mul, int64_t add)
{
    /* Assumption:
       Pass is profitable when: (X * cmp + Y * sel) < (ExecSize * mov VxH).
    */

    const int64_t assumedVXHCost = IGC_GET_FLAG_VALUE(SplitIndirectEEtoSelThreshold);
    int64_t possibleCost = 0;

    /* for: extractelement <4 x float> , %index
       cost is (4 - 1)  * (icmp + sel) = 6;
    */
    possibleCost = ((int64_t)num -1) * 2;
    if (possibleCost < assumedVXHCost)
        return true;

    /* for: extractelement <12 x float> , (mul %real_index, 3)
       cost is ((12/3) - 1) * (icmp + sel) = 6;
    */

    if (mul > 0) // not tested negative options
    {
        int64_t differentOptions = 1 + ((int64_t)num - 1) / mul; // ceil(num/mul)
        possibleCost = (differentOptions - 1) * 2;

        if (possibleCost < assumedVXHCost)
            return true;
    }

    return false;
}

void SplitIndirectEEtoSel::visitExtractElementInst(llvm::ExtractElementInst& I)
{
    using namespace llvm::PatternMatch;

    IGCLLVM::FixedVectorType* vecTy = cast<IGCLLVM::FixedVectorType>(I.getVectorOperandType());
    uint64_t num = vecTy->getNumElements();
    Type* eleType = vecTy->getElementType();

    Value* vec = I.getVectorOperand();
    Value* index = I.getIndexOperand();

    // ignore constant index
    if (dyn_cast<ConstantInt>(index))
    {
        return;
    }

    // ignore others for now (did not yet evaluate perf. impact)
    if (!(eleType->isIntegerTy(32) || eleType->isFloatTy()))
    {
        return;
    }

    // used to calculate offsets
    int64_t add = 0;
    int64_t mul = 1;

    /* strip mul/add from index calculation and remember it for later:
       %268 = mul nuw i32 %res.i2.i, 3
       %270 = add i32 %268, 1
       %271 = extractelement <12 x float> %234, i32 %270
    */
    Value* Val1 = nullptr;
    Value* Val2 = nullptr;
    ConstantInt* ci_add = nullptr;
    ConstantInt* ci_mul = nullptr;

    auto pat_add = m_Add(m_Value(Val2), m_ConstantInt(ci_add));
    auto pat_mul = m_Mul(m_Value(Val1), m_ConstantInt(ci_mul));
    // Some code shows `shl+or` instead of mul+add.
    auto pat_or = m_Or(m_Value(Val2), m_ConstantInt(ci_add));
    auto pat_shl = m_Shl(m_Value(Val1), m_ConstantInt(ci_mul));

    if (match(index, pat_mul) || (match(index, pat_add) && match(Val2, pat_mul)))
    {
        mul = ci_mul ? ci_mul->getSExtValue() : 1;
    }
    else if (match(index, pat_shl) || (match(index, pat_or) && match(Val2, pat_shl)))
    {
        mul = ci_mul ? (1LL << ci_mul->getSExtValue()) : 1LL;
    }
    // Instruction::hasPoisonGeneratingFlags() could be used instead
    // after llvm9 support is dropped
    auto hasNoOverflow = [](Value* value) {
        if (OverflowingBinaryOperator *OBO = dyn_cast<OverflowingBinaryOperator>(value))
           return OBO->hasNoUnsignedWrap() || OBO->hasNoSignedWrap();
        return true;
    };

    // If pattern matched check that corresponding index calculation has nsw or nuw
    if (Val1)
    {
       // Transformation could still be profitable,
       // but index and it's multiplier shouldn't be modified
       if (!hasNoOverflow(index) || (Val2 && !hasNoOverflow(Val2)))
       {
          mul = 1;
       }
       else
       {
          add = ci_add ? ci_add->getSExtValue() : 0;
          index = Val1;
       }
    }

    if (!isProfitableToSplit(num, mul, add))
        return;

    Value* vTemp = llvm::UndefValue::get(eleType);
    IRBuilder<> builder(I.getNextNode());

    // returns true if we can skip this icmp, such as:
    // icmp eq (add (mul %index, 3), 2), 1
    // icmp eq (mul %index, 3), 1
    auto canSafelySkipThis = [&](int64_t add, int64_t mul, int64_t & newIndex) {
        if (mul)
        {
            newIndex -= add;
            if ((newIndex % mul) != 0)
                return true;
            newIndex = newIndex / mul;
        }
        return false;
    };

    // Generate combinations
    for (uint64_t elemIndex = 0; elemIndex < num; elemIndex++)
    {
        int64_t cmpIndex = elemIndex;

        if (canSafelySkipThis(add, mul, cmpIndex))
            continue;

        // Those 2 might be different, when cmp will get altered by it's operands, but EE index stays the same
        ConstantInt* cmpIndexCI = llvm::ConstantInt::get(dyn_cast<IntegerType>(index->getType()), (uint64_t)cmpIndex);
        ConstantInt* eeiIndexCI = llvm::ConstantInt::get(builder.getInt32Ty(), (uint64_t)elemIndex);

        Value* cmp = builder.CreateICmp(CmpInst::Predicate::ICMP_EQ, index, cmpIndexCI);
        Value* subcaseEE = builder.CreateExtractElement(vec, eeiIndexCI);
        Value* sel = builder.CreateSelect(cmp, subcaseEE, vTemp);
        vTemp = sel;
        didSomething = true;
    }

    // In theory there's no situation where we don't do something up to this point.
    if (didSomething)
    {
        I.replaceAllUsesWith(vTemp);
    }
}


IGC_INITIALIZE_PASS_BEGIN(SplitIndirectEEtoSel, "SplitIndirectEEtoSel", "SplitIndirectEEtoSel", false, false)
IGC_INITIALIZE_PASS_END(SplitIndirectEEtoSel, "SplitIndirectEEtoSel", "SplitIndirectEEtoSel", false, false)

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
        bool scheduleUp(BasicBlock* bb, Value* V, Instruction*& insertPos);

        // check if it's safe to convert instructions between cond0 & cond1,
        // moveInsts are the values referened out of (cond0, cond1), we need to
        // move them before cond0
        bool isSafeToConvert(Instruction* cond0, Instruction* cond1,
            smallvector<Instruction*, 8> & moveInsts);

        void convertAndToBranch(Instruction* opAnd,
            Instruction* cond0, Instruction* cond1, BasicBlock*& newBB);
    };
}

IGC_INITIALIZE_PASS_BEGIN(LogicalAndToBranch, "logicalAndToBranch", "logicalAndToBranch", false, false)
IGC_INITIALIZE_PASS_END(LogicalAndToBranch, "logicalAndToBranch", "logicalAndToBranch", false, false)

char LogicalAndToBranch::ID = 0;
FunctionPass* IGC::createLogicalAndToBranchPass() { return new LogicalAndToBranch(); }

LogicalAndToBranch::LogicalAndToBranch() : FunctionPass(ID)
{
    initializeLogicalAndToBranchPass(*PassRegistry::getPassRegistry());
}

bool LogicalAndToBranch::scheduleUp(BasicBlock* bb, Value* V,
    Instruction*& insertPos)
{
    Instruction* inst = dyn_cast<Instruction>(V);
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
    Instruction* cond0, Instruction* cond1, BasicBlock*& newBB)
{

    BasicBlock* bb = opAnd->getParent();
    BasicBlock* bbThen, * bbElse, * bbEnd;

    Instruction* splitBefore = cond0->getNextNonDebugInstruction();
    bbThen = bb->splitBasicBlock(splitBefore->getIterator(), "if.then");
    bbElse = bbThen->splitBasicBlock(opAnd, "if.else");
    bbEnd = bbElse->splitBasicBlock(opAnd, "if.end");

    bb->getTerminator()->eraseFromParent();
    BranchInst* br = BranchInst::Create(bbThen, bbElse, cond0, bb);
    br->setDebugLoc(splitBefore->getDebugLoc());

    bbThen->getTerminator()->eraseFromParent();
    br = BranchInst::Create(bbEnd, bbThen);
    br->setDebugLoc(opAnd->getDebugLoc());


    PHINode* phi = PHINode::Create(opAnd->getType(), 2, "", opAnd);
    phi->addIncoming(cond1, bbThen);
    phi->addIncoming(ConstantInt::getFalse(opAnd->getType()), bbElse);
    phi->setDebugLoc(opAnd->getDebugLoc());

    opAnd->replaceAllUsesWith(phi);
    opAnd->eraseFromParent();

    newBB = bbEnd;
}

bool LogicalAndToBranch::isSafeToConvert(
    Instruction* cond0, Instruction* cond1,
    smallvector<Instruction*, 8> & moveInsts)
{
    BasicBlock::iterator is0(cond0);
    BasicBlock::iterator is1(cond1);

    bool isSafe = true;
    SmallPtrSet<Value*, 32> iset;

    iset.insert(cond1);
    for (auto i = ++is0; i != is1; ++i)
    {
        if (i->mayHaveSideEffects())
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
        BasicBlock* bb = &*BI++;

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
                    s0->hasOneUse() && s1->hasOneUse() &&
                    s0->getParent() == bb && s1->getParent() == bb)
                {
                    if (isInstPrecede(s1, s0))
                    {
                        std::swap(s0, s1);
                    }
                    BasicBlock::iterator is0(s0);
                    BasicBlock::iterator is1(s1);
                    auto distance = std::count_if(is0, is1,
                                                 [](const auto& i){return !isDbgIntrinsic(&i);});

                    if (distance < NUM_INST_THRESHOLD)
                    {
                        continue;
                    }

                    smallvector<Instruction*, 8> moveInsts;
                    if (isSafeToConvert(s0, inst, moveInsts))
                    {
                        // if values defined between s0 & inst(branch) are referenced
                        // outside of (s0, inst), they need to be moved before
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

// clean PHINode does the following:
//   given the following:
//     a = phi (x, b0), (x, b1)
//   this pass will replace 'a' with 'x', and as result, phi is removed.
//
// Special note:
//   LCSSA PHINode has a single incoming value. Make sure it is not removed
//   as WIA uses lcssa phi as a seperator between a uniform value inside loop
//   and non-uniform value outside a loop.  For example:
//      B0:
//             i = 0;
//      Loop:
//             i_0 = phi (0, B0)  (t, Bn)
//             .... <use i_0>
//             if (divergent cond)
//      Bi:
//                goto out;
//      Bn:
//             t = i_0 + 1;
//             if (t < N) goto Loop;
//             goto output;
//      out:
//             i_1 = phi (i_0, Bi)    <-- lcssa phi node
//      ....
//      output:
//   Here, i_0 is uniform within the loop,  but it is not outside loop as each WI will
//   exit with different i, thus i_1 is non-uniform. (Note that removing lcssa might be
//   bad in performance, but it should not cause any functional issue.)
//
// This is needed to avoid generating the following code for which vISA cannot generate
// the correct code:
//   i = 0;             // i is uniform
//   Loop:
//         x = i + 1      // x is uniform
//     B0  if (divergent-condition)
//            <code1>
//     B1  else
//            z = array[i]
//     B2  endif
//         i = phi (x, B0), (x, B1)
//         ......
//     if (i < n) goto Loop
//
// Generated code (visa) (phi becomes mov in its incoming BBs).
//
//   i = 0;             // i is uniform
//   Loop:
//         (W) x = i + 1      // x is uniform, NoMask
//     B0  if (divergent-condition)
//            <code1>
//         (W) i = x         // noMask
//     B1  else
//            z = array[i]
//         (W) i = x         // noMask
//     B2  endif
//         ......
//         if (i < n) goto Loop
//
// In the 1st iteration, 'z' should be array[0].  Assume 'if' is divergent, thus both B0 and B1
// blocks will be executed. As result, the value of 'i' after B0 will be x, which is 1. And 'z'
// will take array[1], which is wrong (correct one is array[0]).
//
// This case happens if phi is uniform, which means all phis' incoming values are identical
// and uniform (current hehavior of WIAnalysis). Note that the identical values means this phi
// is no longer needed.  Once such a phi is removed,  we will never generate code like one shown
// above and thus, no wrong code will be generated from visa.
//
// This pass will be invoked at place close to the Emit pass, where WIAnalysis will be invoked,
// so that IR between this pass and WIAnalysis stays the same, at least no new PHINodes like this
// will be generated.
//
namespace {
    class CleanPHINode : public FunctionPass
    {
    public:
        static char ID;
        CleanPHINode();

        StringRef getPassName() const override { return "CleanPhINode"; }

        bool runOnFunction(Function& F) override;
    };
}

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-cleanphinode"
#define PASS_DESCRIPTION "Clean up PHINode"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CleanPHINode, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(CleanPHINode,   PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char CleanPHINode::ID = 0;
FunctionPass* IGC::createCleanPHINodePass()
{
    return new CleanPHINode();
}

CleanPHINode::CleanPHINode() : FunctionPass(ID)
{
    initializeCleanPHINodePass(*PassRegistry::getPassRegistry());
}

bool CleanPHINode::runOnFunction(Function& F)
{
    bool changed = false;
    for (BasicBlock& BB : F)
    {
        auto II = BB.begin();
        auto IE = BB.end();
        while (II != IE)
        {
            auto currII = II;
            ++II;
            PHINode* PHI = dyn_cast<PHINode>(currII);
            if (PHI == nullptr)
            {
                // proceed to the next BB
                break;
            }
            if (PHI->getNumIncomingValues() == 1)
            {
                // Keep LCSSA PHI as uniform analysis needs it.
                continue;
            }

            if (PHI->getNumIncomingValues() > 0) // sanity
            {
                Value* sameVal = PHI->getIncomingValue(0);
                bool isAllSame = true;
                for (unsigned i = 1, sz = PHI->getNumIncomingValues(); i < sz; ++i)
                {
                    if (sameVal != PHI->getIncomingValue(i))
                    {
                        isAllSame = false;
                        break;
                    }
                }
                if (isAllSame)
                {
                    PHI->replaceAllUsesWith(sameVal);
                    PHI->eraseFromParent();
                    changed = true;
                }
            }
        }
    }
    return changed;
}

namespace {
    class InsertBranchOpt : public FunctionPass
    {
    public:
        static char ID;
        InsertBranchOpt();

        StringRef getPassName() const override { return "InsertBranchOpt"; }

        bool runOnFunction(Function& F) override;
        void atomicSpiltOpt(Function& F, int mode);
        void ThreeWayLoadSpiltOpt(Function& F);
        void findOptCases(SelectInst* I);
        bool HasSrcFromEE(Instruction* I, uint selNum, Instruction*& loadInst);
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
        }
    private:
        struct loadGroup {
            CmpInst* cmpInst0, * cmpInst1;
            SelectInst* selInst[2][4];
            Instruction* toMovInst0[4], * toMovInst1[4], * toMovInst2[4];
            PHINode* newPhi[3][4];
            GenIntrinsicInst* dclInst0, * dclInst1;
            uint num;
        };

        std::vector<loadGroup>SplitGroup;
        CodeGenContext* pContext = nullptr;
        void CreateBranchBlock(Function& F, Value* cmpI, loadGroup& lg, Instruction* inst[], int groupID);
    };
}

namespace {
    class MergeMemFromBranchOpt : public FunctionPass, public llvm::InstVisitor<MergeMemFromBranchOpt>
    {
    public:
        static char ID;
        MergeMemFromBranchOpt();

        StringRef getPassName() const override { return "MergeMemFromBranchOpt"; }

        bool runOnFunction(Function& F) override;
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }

        void visitCallInst(llvm::CallInst& C);
        void visitTypedWrite(llvm::CallInst* inst);
    private:
        bool changed = false;
    };
}

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-MergeMemFromBranchOpt"
#define PASS_DESCRIPTION "Merge Mem from Branch Opt"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MergeMemFromBranchOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(MergeMemFromBranchOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

/*
This optimization merge the UAV writes from if/else branch to the common successor to avoid partial writes
ex: Change
    Label-1337.i:                                     ; preds = %Label-1339.i, %Label-1780.i
      <skip instructions>
      %494 = inttoptr i32 %0 to %__2D_DIM_Resource addrspace(2490368)*
      call void @llvm.genx.GenISA.typedwrite.p2490368__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490368)* %494, i32 %17, i32 %18, i32 0, i32 0, float %491, float %492, float %493, float %Temp-2360.i163)
      br label %Output
    Label-1313.i:                                     ; preds = %Label-1577.i
      %495 = inttoptr i32 %0 to %__2D_DIM_Resource addrspace(2490368)*
      call void @llvm.genx.GenISA.typedwrite.p2490368__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490368)* %495, i32 %17, i32 %18, i32 0, i32 0, float %scalar39, float %scalar40, float %scalar41, float %scalar42)
      br label %Output
    Output:                                           ; preds = %Label-1313.i, %Label-1337.i
      ret void
                      to
    Output:                                           ; preds = %Label-1313.i, %Label-1337.i
      %494 = phi float [ %scalar39, %Label-1313.i ], [ %491, %Label-1337.i ]
      %495 = phi float [ %scalar40, %Label-1313.i ], [ %492, %Label-1337.i ]
      %496 = phi float [ %scalar41, %Label-1313.i ], [ %493, %Label-1337.i ]
      %497 = phi float [ %scalar42, %Label-1313.i ], [ %Temp-2360.i163, %Label-1337.i ]
      %498 = inttoptr i32 %0 to %__2D_DIM_Resource addrspace(2490368)*
      call void @llvm.genx.GenISA.typedwrite.p2490368__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490368)* %498, i32 %17, i32 %18, i32 0, i32 0, float %494, float %495, float %496, float %497)
      ret void
    }
*/

char MergeMemFromBranchOpt::ID = 0;
FunctionPass* IGC::createMergeMemFromBranchOptPass()
{
    return new MergeMemFromBranchOpt();
}

MergeMemFromBranchOpt::MergeMemFromBranchOpt() : FunctionPass(ID), changed(false)
{
    initializeMergeMemFromBranchOptPass(*PassRegistry::getPassRegistry());
}

void MergeMemFromBranchOpt::visitTypedWrite(llvm::CallInst* inst)
{
    std::vector<CallInst*> callSet;
    // last instruction in the BB
    if (dyn_cast<BranchInst>(inst->getNextNode()))
    {
        BasicBlock* BB = inst->getParent();
        // the basicblock with inst has single successor BB
        if (BasicBlock* succBB = BB->getSingleSuccessor())
        {
            callSet.push_back(inst);
            // find the other BB with the same successor
            for (auto* BB2 : predecessors(succBB))
            {
                if (BB2 != BB && BB2->getSingleSuccessor())
                {
                    Instruction* br = BB2->getTerminator();
                    if (br && br->getPrevNode())
                    {
                        if (llvm::GenIntrinsicInst* giInst = llvm::dyn_cast<GenIntrinsicInst>(br->getPrevNode()))
                        {
                            if (giInst->getIntrinsicID() == GenISAIntrinsic::GenISA_typedwrite)
                            {
                                IntToPtrInst* itp0 = dyn_cast<IntToPtrInst>(inst->getOperand(0));
                                IntToPtrInst* itp1 = dyn_cast<IntToPtrInst>(giInst->getOperand(0));

                                if (itp0 && itp1 &&
                                    (itp0->getType()->getPointerAddressSpace() ==
                                        itp1->getType()->getPointerAddressSpace()) &&
                                    (itp0->getOperand(0) == itp1->getOperand(0)) &&
                                    (giInst->getOperand(1) == inst->getOperand(1)) &&
                                    (giInst->getOperand(2) == inst->getOperand(2)) &&
                                    (giInst->getOperand(3) == inst->getOperand(3)) &&
                                    (giInst->getOperand(4) == inst->getOperand(4)))
                                {
                                    callSet.push_back(giInst);
                                }
                            }
                        }
                    }
                }
            }

            // if there are more predecessor than the ones with urb partial write,
            // skip the optimization as it might increase the number of times it needs to do the urb write.
            if (!succBB->hasNPredecessors(callSet.size()) || callSet.size() < 2)
                return;

            // start the conversion
            changed = 1;

            // create phi nodes for the data used by typedwrite
            PHINode* p8 = PHINode::Create(inst->getOperand(8)->getType(), callSet.size(), "", &(succBB->front()));
            PHINode* p7 = PHINode::Create(inst->getOperand(7)->getType(), callSet.size(), "", &(succBB->front()));
            PHINode* p6 = PHINode::Create(inst->getOperand(6)->getType(), callSet.size(), "", &(succBB->front()));
            PHINode* p5 = PHINode::Create(inst->getOperand(5)->getType(), callSet.size(), "", &(succBB->front()));
            PHINode* itp = PHINode::Create(inst->getOperand(0)->getType(), callSet.size(), "", &(succBB->front()));
            for (unsigned int i = 0; i < callSet.size(); i++)
            {
                itp->addIncoming(callSet[i]->getOperand(0), callSet[i]->getParent());
                p5->addIncoming(callSet[i]->getOperand(5), callSet[i]->getParent());
                p6->addIncoming(callSet[i]->getOperand(6), callSet[i]->getParent());
                p7->addIncoming(callSet[i]->getOperand(7), callSet[i]->getParent());
                p8->addIncoming(callSet[i]->getOperand(8), callSet[i]->getParent());
            }

            // move the first typedwrite instruction into succBB
            inst->removeFromParent();
            inst->insertBefore(&*succBB->getFirstInsertionPt());

            // set the inst srcs to results from phi nodes above
            inst->setOperand(0, itp);
            inst->setOperand(5, p5);
            inst->setOperand(6, p6);
            inst->setOperand(7, p7);
            inst->setOperand(8, p8);

            // erase the other typedwrite instructions
            for (unsigned int i = 1; i < callSet.size(); i++)
            {
                callSet[i]->eraseFromParent();
            }
        }
    }
}

void MergeMemFromBranchOpt::visitCallInst(CallInst& C)
{
    if (llvm::GenIntrinsicInst* inst = llvm::dyn_cast<GenIntrinsicInst>(&C))
    {
        if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_typedwrite)
        {
            visitTypedWrite(inst);
        }
    }
}

bool MergeMemFromBranchOpt::runOnFunction(Function& F)
{
    visit(F);
    return changed;
}


namespace {
    class SinkLoadOpt : public FunctionPass, public llvm::InstVisitor<SinkLoadOpt>
    {
    public:
        static char ID;
        SinkLoadOpt();

        bool runOnFunction(Function& F) override;

        virtual StringRef getPassName() const override
        {
            return IGCOpts::SinkLoadOptPass;
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
        }

        bool sinkLoadInstruction(Function& F);
    private:
        bool changed = false;
    };
}

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-SinkLoadOpt"
#define PASS_DESCRIPTION "load sinking Opt"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SinkLoadOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(SinkLoadOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char SinkLoadOpt::ID = 0;
FunctionPass* IGC::createSinkLoadOptPass()
{
    return new SinkLoadOpt();
}

SinkLoadOpt::SinkLoadOpt() : FunctionPass(ID), changed(false)
{
    initializeSinkLoadOptPass(*PassRegistry::getPassRegistry());
}

bool SinkLoadOpt::sinkLoadInstruction(Function& F)
{
    std::vector<std::pair<Instruction*, Instruction*>> moveInst;
    for (auto& BB : F)
    {
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE; ++BI)
        {
            GenIntrinsicInst* CI = dyn_cast<GenIntrinsicInst>(BI);
            if (CI &&
                (CI->getIntrinsicID() == GenISAIntrinsic::GenISA_ldraw_indexed ||
                 CI->getIntrinsicID() == GenISAIntrinsic::GenISA_ldrawvector_indexed))
            {
                uint32_t Addrspace = CI->getArgOperand(0)->getType()->getPointerAddressSpace();
                BufferType BufTy = GetBufferType(Addrspace);
                BufferAccessType Access = getDefaultAccessType(BufTy);
                if (Access == BufferAccessType::ACCESS_READ)
                {
                    if (CI->hasOneUse())
                    {
                        // if the load only has one use, move it to right before the use.
                        Instruction* useInst = dyn_cast<Instruction>(*(CI->user_begin()));
                        if (useInst && !isa<PHINode>(useInst) && CI->getNextNode() != useInst)
                        {
                            moveInst.push_back(std::pair(cast<Instruction>(CI), useInst));
                        }
                    }
                    else
                    {
                        // if all the use are extractelement, and they are all in the same BB.
                        // move the load and extractelement into the beginning of this BB.
                        std::vector<ExtractElementInst*> ee;
                        std::vector<Instruction*> eeUser;
                        bool stop = 0;
                        BasicBlock* useBlock = CI->getParent();
                        for (auto* iter : CI->users())
                        {
                            if (ExtractElementInst* eeTemp = dyn_cast<ExtractElementInst>(iter))
                            {
                                for (auto* eeUseIter : eeTemp->users())
                                {
                                    Instruction* eeUseInst = dyn_cast<Instruction>(eeUseIter);
                                    if (!eeUseInst || eeUseInst->getParent() != useBlock || isa<PHINode>(eeUseIter))
                                    {
                                        stop = 1;
                                        break;
                                    }
                                    eeUser.push_back(eeUseInst);
                                }
                                if (!stop)
                                {
                                    ee.push_back(eeTemp);
                                }
                            }
                            else
                            {
                                stop = 1;
                                break;
                            }
                        }
                        if (stop || ee.size() == 0)
                            continue;

                        // find the first used to move the load/ee down
                        Instruction* currentInst = CI->getNextNode();
                        Instruction* insertLocation = nullptr;
                        while (currentInst && !currentInst->isTerminator() && currentInst->getParent() == useBlock)
                        {
                            for (unsigned int i = 0; i < eeUser.size(); i++)
                            {
                                if (currentInst == eeUser[i])
                                {
                                    insertLocation = currentInst;
                                    break;
                                }
                            }
                            if (insertLocation)
                                break;
                            currentInst = currentInst->getNextNode();
                        }

                        if (!insertLocation)
                            continue;

                        // store the instructions to be moved
                        for (unsigned int i = 0; i < ee.size(); i++)
                        {
                            moveInst.push_back(std::pair(cast<Instruction>(ee[i]), insertLocation));
                        }
                        moveInst.push_back(std::pair(cast<Instruction>(CI), insertLocation));
                    }
                }
            }
        }
    }

    if (moveInst.size())
    {
        for (auto &mi : reverse(moveInst))
        {
            mi.first->moveBefore(mi.second);
        }
        return true;
    }
    return false;
}

bool SinkLoadOpt::runOnFunction(Function& F)
{
    auto ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (isOptDisabledForFunction(ctx->getModuleMetaData(), getPassName(), &F))
        return false;

    bool changed = sinkLoadInstruction(F);

    return changed;
}

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-InsertBranchOpt"
#define PASS_DESCRIPTION "Insert Branch Opt"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InsertBranchOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(InsertBranchOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char InsertBranchOpt::ID = 0;
FunctionPass* IGC::createInsertBranchOptPass()
{
    return new InsertBranchOpt();
}

InsertBranchOpt::InsertBranchOpt() : FunctionPass(ID)
{
    initializeInsertBranchOptPass(*PassRegistry::getPassRegistry());
}

bool InsertBranchOpt::HasSrcFromEE(Instruction* I, uint selNum, Instruction*& loadInst)
{
    std::vector<Instruction*> traceBackInst;
    int itemID = 0;
    uint maxSearchQueueSize = 10;
    traceBackInst.push_back(I);
    while (itemID != traceBackInst.size())
    {
        Instruction* tbInst = traceBackInst[itemID];
        if (ExtractElementInst* eetemp = dyn_cast<ExtractElementInst>(tbInst))
        {
            Instruction* load = dyn_cast<Instruction>(eetemp->getOperand(0));
            if (BitCastInst* BCinst = dyn_cast<BitCastInst>(load))
            {
                load = dyn_cast<Instruction>(BCinst->getOperand(0));
            }
            // load in the same BB as the extractElement
            if (load->getParent() == I->getParent() &&
                !load->hasNUsesOrMore(1 + selNum) &&
                eetemp->hasOneUse())
            {
                loadInst = load;
                return true;
            }
        }
        else
        {
            for (uint srci = 0; srci < tbInst->getNumOperands(); srci++)
            {
                Instruction* temp = dyn_cast<Instruction>(tbInst->getOperand(srci));
                if (temp && traceBackInst.size() < maxSearchQueueSize && temp->getParent() == I->getParent())
                {
                    traceBackInst.push_back(temp);
                }
            }
        }
        itemID++;
    }
    return false;
}

void InsertBranchOpt::findOptCases(SelectInst* I)
{
    loadGroup lg;
    memset(&lg, 0, sizeof(loadGroup));
    lg.selInst[1][0] = dyn_cast<SelectInst>(I);
    if (lg.selInst[1][0])
    {
        // only start when this is the first select in the sequence
        // skip over bitcast
        Instruction* tempNode = lg.selInst[1][0]->getPrevNode();
        if (!tempNode)
            return;

        if (dyn_cast<BitCastInst>(tempNode))
            tempNode = tempNode->getPrevNode();

        if (dyn_cast<SelectInst>(tempNode))
            return;

        // selInst[0][0] src 1 needs to come from another select inst
        lg.selInst[0][0] = dyn_cast<SelectInst>(lg.selInst[1][0]->getOperand(1));
        if (!lg.selInst[0][0])
            return;

        // record the select condition
        lg.cmpInst0 = dyn_cast<CmpInst>(lg.selInst[0][0]->getOperand(0));
        lg.cmpInst1 = dyn_cast<CmpInst>(lg.selInst[1][0]->getOperand(0));

        if (!lg.cmpInst0 || !lg.cmpInst1)
            return;

        // cmpInst comes from inputVec and constant
        lg.dclInst0 = dyn_cast<GenIntrinsicInst>(lg.cmpInst0->getOperand(0));
        lg.dclInst1 = dyn_cast<GenIntrinsicInst>(lg.cmpInst1->getOperand(0));
        if (!lg.dclInst0 || !lg.dclInst1)
            return;

        if (lg.dclInst0->getIntrinsicID() != GenISAIntrinsic::GenISA_DCL_inputVec ||
            lg.dclInst1->getIntrinsicID() != GenISAIntrinsic::GenISA_DCL_inputVec ||
            !dyn_cast<Constant>(lg.cmpInst0->getOperand(1)) ||
            !dyn_cast<Constant>(lg.cmpInst1->getOperand(1)))
            return;

        // find the other select instructions in the sequence with the same select condition
        uint numSel = 1;
        for (int i = 1; i < 4; i++)
        {
            // selInst[1][*] src 1 need to come from another select inst
            // selInst[1][*] needs to have the same cmp (sel condition)
            // selInst[0][*] needs to have the same cmp (sel condition)
            tempNode = dyn_cast<BitCastInst>(lg.selInst[1][i - 1]->getNextNode());
            if (tempNode)
            {
                // skip over bitcast and find the next instruction
                lg.selInst[1][i] = dyn_cast<SelectInst>(tempNode->getNextNode());
            }
            else
            {
                lg.selInst[1][i] = dyn_cast<SelectInst>(lg.selInst[1][i - 1]->getNextNode());
            }

            if (!lg.selInst[1][i] || lg.selInst[1][i]->getOperand(0) != lg.cmpInst1)
                break;

            lg.selInst[0][i] = dyn_cast<SelectInst>(lg.selInst[1][i]->getOperand(1));
            if (!lg.selInst[0][i] || lg.selInst[0][i]->getOperand(0) != lg.cmpInst0)
                break;

            numSel++;
        }

        // only apply opt on 3 or 4 channel cases
        if (numSel != 3 && numSel != 4)
            return;

        for (uint i = 0; i < numSel; i++)
        {
            Instruction* temp0 = dyn_cast<Instruction>(lg.selInst[0][i]->getOperand(1));
            Instruction* temp1 = dyn_cast<Instruction>(lg.selInst[0][i]->getOperand(2));
            Instruction* temp2 = dyn_cast<Instruction>(lg.selInst[1][i]->getOperand(2));
            Instruction* load0 = nullptr;
            Instruction* load1 = nullptr;
            Instruction* load2 = nullptr;
            if (temp0 && temp1 && temp2 &&
                HasSrcFromEE(temp0, numSel, load0) && HasSrcFromEE(temp1, numSel, load1) && HasSrcFromEE(temp2, numSel, load2))
            {
                // three loads can't be the same. We won't be able to break each group into branches if the loads are the same
                if (load0 != load1 && load0 != load2 && load1 != load2)
                {
                    lg.toMovInst0[i] = temp0;
                    lg.toMovInst1[i] = temp1;
                    lg.toMovInst2[i] = temp2;
                }
                else
                    return;
            }
            else
                return;
        }

        lg.num = numSel;
        SplitGroup.push_back(lg);
    }
}

void InsertBranchOpt::CreateBranchBlock(Function& F, Value* cmpI, loadGroup& lg, Instruction* inst[], int groupID)
{
    Instruction* termatorInst = SplitBlockAndInsertIfThen(cmpI, inst[0], false);

    for (uint i = 0; i < lg.num; i++)
    {
        inst[i]->moveBefore(termatorInst);
    }

    Value* zeroF = ConstantFP::get(Type::getFloatTy(F.getContext()), 0.);
    Value* zeroI = ConstantInt::get(Type::getInt32Ty(F.getContext()), 0);

    // set up the new phi nodes
    for (int i = (int)lg.num - 1; i >= 0; i--)
    {
        lg.newPhi[groupID][i] = PHINode::Create(inst[i]->getType(), 2, "", &(termatorInst->getSuccessor(0)->front()));
    }

    for (uint i = 0; i < lg.num; i++)
    {
        inst[i]->replaceUsesOutsideBlock(lg.newPhi[groupID][i], inst[i]->getParent());
        lg.newPhi[groupID][i]->addIncoming(inst[i], inst[i]->getParent());
        if (groupID == 0)
            lg.newPhi[groupID][i]->addIncoming(inst[i]->getType()->isFloatingPointTy() ? zeroF : zeroI, termatorInst->getParent()->getSinglePredecessor());
        else
            lg.newPhi[groupID][i]->addIncoming(lg.newPhi[groupID - 1][i], termatorInst->getParent()->getSinglePredecessor());
    }

    // replace select with the phi node
    if (groupID > 0)
    {
        for (uint i = 0; i < lg.num; i++)
        {
            lg.selInst[groupID - 1][i]->replaceAllUsesWith(lg.newPhi[groupID][i]);
        }
    }
}

void InsertBranchOpt::ThreeWayLoadSpiltOpt(Function& F)
{
    // For a pattern with 3 sample / load and only use one of them based on some
    // condition, change it to use branches to only do the one sample / load that matters.

    // For example:
    // load1
    // load2
    // load3
    // temp1 = select cond1, load1, load2
    // temp2 = select cond2, temp1, load3
    //      to
    // if(cond1)
    //      load1
    // if(!cond1 && cond2)
    //      load2
    // if(!cond1 && !cond2)
    //      load3

    // find the cases to optimize and store info in SplitGroup
    for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
    {
        for (auto II = BI->begin(); II != BI->end(); II++)
        {
            if (SelectInst* inst = dyn_cast<SelectInst>(II))
            {
                findOptCases(inst);
            }
        }
    }

    // start making changes to the shader
    // add branching for the load/sample instructions
    std::vector<CmpInst*> movedCmpInst;
    for (std::vector<loadGroup>::iterator iter = SplitGroup.begin(); iter != SplitGroup.end(); iter++)
    {
        loadGroup lg = *iter;
        // move the cmp insts to the before the sequence
        // if there are multiple sets to optimization and share the same cmp inst, only move the 1st set since
        // it will be before all the other sets already
        bool cmpInst0Moved = false;
        bool cmpInst1Moved = false;
        for (auto mi = movedCmpInst.begin(); mi != movedCmpInst.end(); mi++)
        {
            if (*mi == lg.cmpInst0)
            {
                cmpInst0Moved = true;
            }
            else if (*mi == lg.cmpInst1)
            {
                cmpInst1Moved = true;
            }
        }

        if (!cmpInst0Moved)
        {
            lg.cmpInst0->moveAfter(lg.dclInst0);
            movedCmpInst.push_back(lg.cmpInst0);
        }
        if (!cmpInst1Moved)
        {
            lg.cmpInst1->moveAfter(lg.dclInst1);
            movedCmpInst.push_back(lg.cmpInst1);
        }

        // create branch for cmp0
        CreateBranchBlock(F, lg.cmpInst0, lg, lg.toMovInst0, 0);

        // create branch for !cmp0 && cmp1
        IRBuilder<> builder(dyn_cast<Instruction>(lg.toMovInst1[0]));
        Value* notCmp0 = builder.CreateNot(lg.cmpInst0);
        Value* notCmp0_Cmp1 = builder.CreateAnd(notCmp0, lg.cmpInst1);
        CreateBranchBlock(F, notCmp0_Cmp1, lg, lg.toMovInst1, 1);

        // create branch for !cmp0 && !cmp1
        builder.SetInsertPoint(lg.toMovInst2[0]);
        Value* notCmp1 = builder.CreateNot(lg.cmpInst1);
        Value* notCmp0_notCmp1 = builder.CreateAnd(notCmp0, notCmp1);
        CreateBranchBlock(F, notCmp0_notCmp1, lg, lg.toMovInst2, 2);
    }
}

void InsertBranchOpt::atomicSpiltOpt(Function& F, int mode)
{
    // Allow both modes to be applied
    bool defaultMode = ( ( mode & 1 ) == 1 );
    bool alternateUmaxMode = ( ( mode & 2 ) == 2 );

    auto createReadFromAtomic = []( IRBuilder<>& builder, Instruction* inst, bool isTyped )
        {
            Constant* zero = ConstantInt::get( inst->getType(), 0 );
            Instruction* NewInst = nullptr;
            if( isTyped )
            {
                Function* pLdIntrinsic = llvm::GenISAIntrinsic::getDeclaration(
                    inst->getModule(),
                    GenISAIntrinsic::GenISA_typedread,
                    inst->getOperand( 0 )->getType() );

                SmallVector<Value*, 5> ld_FunctionArgList( 5 );
                ld_FunctionArgList[ 0 ] = inst->getOperand( 0 );
                ld_FunctionArgList[ 1 ] = inst->getOperand( 1 );
                ld_FunctionArgList[ 2 ] = inst->getOperand( 2 );
                ld_FunctionArgList[ 3 ] = zero;
                ld_FunctionArgList[ 4 ] = zero;
                NewInst = builder.CreateCall( pLdIntrinsic, ld_FunctionArgList );
            }
            else
            {
                std::vector<Type*> types;
                std::vector<Value*> ld_FunctionArgList;

                Value* resourcePtr = inst->getOperand( 0 );
                types.push_back( IGCLLVM::FixedVectorType::get( builder.getFloatTy(), 4 ) );
                types.push_back( resourcePtr->getType() );//Paired resource
                types.push_back( resourcePtr->getType() );//Resource


                Function* pLdIntrinsic = GenISAIntrinsic::getDeclaration(
                    inst->getModule(),
                    GenISAIntrinsic::GenISA_ldptr,
                    types );

                ld_FunctionArgList.push_back( inst->getOperand( 1 ) );    //coordinates x
                ld_FunctionArgList.push_back( zero );                   //coordinates y
                ld_FunctionArgList.push_back( zero );                   //coordinates z
                ld_FunctionArgList.push_back( zero );                   //lod
                ld_FunctionArgList.push_back( llvm::UndefValue::get( inst->getOperand( 0 )->getType() ) );
                ld_FunctionArgList.push_back( inst->getOperand( 0 ) );    //src buffer
                ld_FunctionArgList.push_back( zero );                   //immediate offset u
                ld_FunctionArgList.push_back( zero );                   //immediate offset v
                ld_FunctionArgList.push_back( zero );                   //immediate offset w
                NewInst = builder.CreateCall( pLdIntrinsic, ld_FunctionArgList );
            }

            Instruction* ExtractE = cast<Instruction>( builder.CreateExtractElement( NewInst, (uint64_t)0 ) );
            Instruction* castI = cast<Instruction>( builder.CreateBitCast( ExtractE, inst->getType() ) );
            return castI;
        };

    auto splitBBAndName = []( Instruction* condInst , Instruction* inst, Instruction** ThenTerm, Instruction** ElseTerm, BasicBlock* &MergeBlock)
        {
            if( ElseTerm )
            {
                SplitBlockAndInsertIfThenElse( condInst, inst, ThenTerm, ElseTerm );
                BasicBlock* ElseBlock = (*ElseTerm)->getParent();
                ElseBlock->setName( "atomic.if.false" );
            }
            else
            {
                (*ThenTerm) = SplitBlockAndInsertIfThen( condInst, inst, false );
            }
            BasicBlock* ThenBlock = (*ThenTerm)->getParent();
            MergeBlock = inst->getParent();

            ThenBlock->setName( "atomic.if.true" );
            MergeBlock->setName( "atomic.if.end" );
        };

    std::vector<std::pair<GenIntrinsicInst*, IGC::AtomicOp>> atomicSplit;
    for( auto BI = F.begin(), BE = F.end(); BI != BE; BI++ )
    {
        for (auto II = BI->begin(); II != BI->end(); II++)
        {
            if (GenIntrinsicInst* inst = dyn_cast<GenIntrinsicInst>(II))
            {
                Instruction* src = nullptr;
                ConstantInt* op = nullptr;
                if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_intatomictyped)
                {
                    src = dyn_cast<Instruction>(inst->getOperand(4));
                    op = dyn_cast<ConstantInt>(inst->getOperand(5));
                }
                else if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_intatomicraw)
                {
                    src = dyn_cast<Instruction>(inst->getOperand(2));
                    op = dyn_cast<ConstantInt>(inst->getOperand(3));
                }
                if (!src || !op)
                    continue;

                AtomicOp atomicOp = static_cast<AtomicOp>(op->getZExtValue());

                if( ( ( defaultMode ) && ( atomicOp == AtomicOp::EATOMIC_IADD || atomicOp == AtomicOp::EATOMIC_SUB ) )
                    ||
                    atomicOp == AtomicOp::EATOMIC_UMAX )
                {
                    atomicSplit.push_back( std::make_pair( inst, atomicOp ) );
                }
            }
        }
    }

    for ( auto& [inst, op] : atomicSplit)
    {
        bool isTyped = false;
        int srcID = 2;
        if (cast<GenIntrinsicInst>(inst)->getIntrinsicID() == GenISAIntrinsic::GenISA_intatomictyped)
        {
            isTyped = true;
            srcID = 4;
        }

        IRBuilder<> builder(inst);
        Instruction* src = dyn_cast<Instruction>(inst->getOperand(srcID));
        Instruction* readI = nullptr;
        Instruction* ThenTerm = nullptr;
        Instruction* ElseTerm = nullptr;
        BasicBlock* MergeBlock = nullptr;


        if( op != AtomicOp::EATOMIC_UMAX || !alternateUmaxMode )
        {
            // Create an if-then-else structure.
            // if (cond!=0)
            //    use the original atomic add inst
            // else
            //    use typedread or load
            Instruction* condInst = dyn_cast<Instruction>(builder.CreateICmp(ICmpInst::ICMP_NE, src, builder.getInt32(0)));
            splitBBAndName( condInst, inst, &ThenTerm, &ElseTerm, MergeBlock );
            inst->moveBefore( ThenTerm );

            builder.SetInsertPoint( ElseTerm );
            readI = createReadFromAtomic( builder, inst, isTyped);
        }
        else // ( op == AtomicOp::EATOMIC_UMAX && alternateUmaxMode )
        {
            // Create an if-then structure.
            // x = typedread or load
            // if (x < src)
            //    use the original atomic umax inst src
            readI = createReadFromAtomic( builder, inst, isTyped );
            Instruction* condInst = dyn_cast<Instruction>( builder.CreateICmp( ICmpInst::ICMP_UGT, src, readI ) );

            splitBBAndName( condInst, inst, &ThenTerm, nullptr, MergeBlock );
            inst->moveBefore( ThenTerm );
        }
        if( inst->getNumUses() )
        {
            PHINode* newPhi = PHINode::Create(inst->getType(), 2, "", &MergeBlock->front());
            inst->replaceUsesOutsideBlock(newPhi, inst->getParent());
            newPhi->addIncoming( inst, inst->getParent() );
            newPhi->addIncoming( readI, readI->getParent() );
        }
    }
}

void typedWriteZeroStoreCheck(Function& F)
{
    for (auto BI = F.begin(), BE = F.end(); BI != BE; ++BI)
    {
        for (auto II = BI->begin(); II != BI->end(); ++II)
        {
            Instruction* faddInst[4], * fmulInst[4], * eeInst[4];
            std::vector<PHINode*>phiNodes;
            //Obtain typedWrite
            if (GenIntrinsicInst* tyWrite = dyn_cast<GenIntrinsicInst>(&*II))
            {
                if (tyWrite->getIntrinsicID() == GenISAIntrinsic::GenISA_typedwrite)
                {
                    bool checksAreGreen = true;

                    //Many chances for the conditions to not be met, and so there are early outs in place
                    for (int i = 0; i < 4; i++)
                    {
                        //Check if typedwrite values come from fadd
                        faddInst[i] = dyn_cast<Instruction>(tyWrite->getOperand(i + 5));
                        if (!faddInst[i] || faddInst[i]->getOpcode() != Instruction::FAdd)
                        {
                            checksAreGreen = false;
                            break;
                        }

                        //Check if fAdd has a value from an fmul
                        fmulInst[i] = dyn_cast<Instruction>(faddInst[i]->getOperand(0));
                        if (!fmulInst[i] || fmulInst[i]->getOpcode() != Instruction::FMul)
                        {
                            checksAreGreen = false;
                            break;
                        }

                        //Confirm whether fadd is using a value from an extract elem inst
                        eeInst[i] = dyn_cast<ExtractElementInst>(faddInst[i]->getOperand(1));
                        if (!eeInst[i] || !isa<ExtractElementInst>(faddInst[i]->getOperand(1)))
                        {
                            checksAreGreen = false;
                            break;
                        }

                        //Check if extract elem comes from a typedRead
                        GenIntrinsicInst* tyRead = dyn_cast<GenIntrinsicInst>(eeInst[i]->getOperand(0));
                        if (!tyRead || tyRead->getIntrinsicID() != GenISAIntrinsic::GenISA_typedread)
                        {
                            checksAreGreen = false;
                            break;
                        }
                    }

                    //if all checks are green, can set up tyW and tyr
                    if (!checksAreGreen)
                    {
                        continue;
                    }

                    //Check phi node to find target block and if phi node can have 0
                    BasicBlock* fromBB = nullptr;
                    bool allPhiContainsZeroAndOnlyOneNonZero = false;
                    bool breakOut = false;

                    for (Instruction* fmul : fmulInst)
                    {
                        if (PHINode* p = dyn_cast<PHINode>(fmul->getOperand(1)))
                        {
                            bool currentNodeContainsZero = false;
                            bool onlyOneNonZeroFound = false;
                            if (p->getNumIncomingValues() <= 4)
                            {
                                for (unsigned int predIndex = 0; predIndex < p->getNumIncomingValues(); ++predIndex)
                                {
                                    if (ConstantFP* fp = dyn_cast<ConstantFP>(p->getIncomingValue(predIndex)))
                                    {
                                        if (!fp->isZero())
                                        {
                                            //If there are multiple non zero values, break immediately
                                            if (onlyOneNonZeroFound)
                                            {
                                                onlyOneNonZeroFound = false;
                                                break;
                                            }

                                            fromBB = p->getIncomingBlock(predIndex);

                                            if (!onlyOneNonZeroFound)
                                            {
                                                onlyOneNonZeroFound = true;
                                            }

                                            if (currentNodeContainsZero && onlyOneNonZeroFound)
                                            {
                                                continue;
                                            }
                                            else
                                            {
                                                currentNodeContainsZero = false;
                                            }
                                        }
                                        else if (fp->isZero())
                                        {
                                            currentNodeContainsZero = true;
                                        }

                                        if (currentNodeContainsZero && onlyOneNonZeroFound)
                                        {
                                            allPhiContainsZeroAndOnlyOneNonZero = true;
                                        }
                                        else
                                        {
                                            allPhiContainsZeroAndOnlyOneNonZero = false;
                                        }
                                    }
                                    else if (Value* val = dyn_cast<Value>(p->getIncomingValue(predIndex)))
                                    {
                                        //If one value has already been found, then this would be the second time
                                        if (onlyOneNonZeroFound)
                                        {
                                            onlyOneNonZeroFound = false;
                                            break;
                                        }

                                        fromBB = p->getIncomingBlock(predIndex);

                                        if (!onlyOneNonZeroFound)
                                        {
                                            onlyOneNonZeroFound = true;
                                        }

                                        if (currentNodeContainsZero && onlyOneNonZeroFound)
                                        {
                                            continue;
                                        }
                                        else
                                        {
                                            currentNodeContainsZero = false;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                allPhiContainsZeroAndOnlyOneNonZero = false;
                            }

                            //Break out and return if one of the phi nodes can't have 0 or has more than one nonzero
                            if (allPhiContainsZeroAndOnlyOneNonZero)
                            {
                                phiNodes.push_back(p);
                            }
                            else
                            {
                                breakOut = true;
                                break;
                            }
                        }
                        else
                        {
                            breakOut = true;
                            break;
                        }
                    }

                    Instruction* tyW = cast<GenIntrinsicInst>(tyWrite);
                    Instruction* tyR = cast<GenIntrinsicInst>(eeInst[0]->getOperand(0));

                    int paramCnt = 5;
                    for (int j = 0; j < paramCnt; j++)
                    {
                        if (tyR->getOperand(j) != tyW->getOperand(j))
                        {
                            breakOut = true;
                        }
                    }

                    if (breakOut)
                    {
                        break;
                    }
                    II++;

                    //accounting for IntToPtrInsts used for the typedread/write
                    Instruction* u = nullptr;
                    Instruction* addIntoOpt = nullptr;

                    if (IntToPtrInst* intoOpt = dyn_cast<IntToPtrInst>(tyR->getOperand(0)))
                    {
                        Instruction* prev = cast<Instruction>(intoOpt->getOperand(0));
                        if (GenIntrinsicInst* runTimeVal = dyn_cast<GenIntrinsicInst>(prev->getOperand(0)))
                        {
                           if (runTimeVal->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue)
                           {
                              //If all of these check out, then clone the IntOprPtr instr
                              u = intoOpt->clone();
                              addIntoOpt = prev->clone();
                           }
                        }
                    }

                    IGC_ASSERT( fromBB != nullptr );
                    if (fromBB != nullptr)
                    {
                        if (u != nullptr && addIntoOpt != nullptr)
                        {
                            addIntoOpt->insertBefore(fromBB->getTerminator());
                            u->insertBefore(fromBB->getTerminator());
                            u->setOperand(0, addIntoOpt);
                        }
                        tyR->setOperand(0, u);
                        tyR->removeFromParent();
                        tyR->insertBefore(fromBB->getTerminator());

                        //How to check for redundant inst?
                        for (int k = 0; k < 3; k++)
                        {
                            eeInst[k]->removeFromParent();
                            eeInst[k]->insertBefore(fromBB->getTerminator());
                        }

                        //ensure that the fmul instructions are given the values that the phi nodes had

                        for (int i = 0; i < 3; i++)
                        {
                            PHINode* ph = dyn_cast<PHINode>(fmulInst[i]->getOperand(1));
                            Value* val = ph->getIncomingValue(0);
                            fmulInst[i]->setOperand(1, val);
                            fmulInst[i]->removeFromParent();
                            fmulInst[i]->insertBefore(fromBB->getTerminator());
                            faddInst[i]->removeFromParent();
                            faddInst[i]->insertBefore(fromBB->getTerminator());
                        }

                        tyW->setOperand(0, u);
                        tyW->removeFromParent();
                        tyW->insertBefore(fromBB->getTerminator());
                    }
                }
            }
        }
    }
}

bool InsertBranchOpt::runOnFunction(Function& F)
{
    pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    int mode = IGC_IS_FLAG_ENABLED( EnableAtomicBranch ) ? IGC_GET_FLAG_VALUE( EnableAtomicBranch ) : pContext->getModuleMetaData()->csInfo.atomicBranch;
    if( mode )
    {
        atomicSpiltOpt( F, mode );
    }

    if (IGC_IS_FLAG_ENABLED(EnableThreeWayLoadSpiltOpt) || pContext->getModuleMetaData()->enableThreeWayLoadSpiltOpt)
    {
        ThreeWayLoadSpiltOpt(F);
    }

    typedWriteZeroStoreCheck(F);

    return false;
}
