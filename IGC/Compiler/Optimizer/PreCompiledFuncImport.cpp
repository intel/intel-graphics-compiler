/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Linker/Linker.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvmWrapper/IR/Function.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "AdaptorCommon/AddImplicitArgs.hpp"
#include "Compiler/Optimizer/PreCompiledFuncImport.hpp"
#include "Compiler/Builtins/PreCompiledFuncLibrary.hpp"
#include "Compiler/Builtins/LibraryIntS32DivRemEmu.hpp"
#include "Compiler/Builtins/LibraryIntU32DivRemEmu.hpp"
#include "Compiler/Builtins/LibraryIntS32DivRemEmuSP.hpp"
#include "Compiler/Builtins/LibraryIntU32DivRemEmuSP.hpp"
#include "Compiler/Builtins/LibraryInt64DivRemEmu.hpp"
#include "Compiler/Builtins/LibraryDPEmu.hpp"
#include "Compiler/Builtins/LibraryMiscEmu.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMUtils.h"
#include "AdaptorOCL/OCL/BuiltinResource.h"

#include <vector>
#include <utility>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-precompiled-import"
#define PASS_DESCRIPTION "PreCompiledFuncImport"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PreCompiledFuncImport, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PreCompiledFuncImport, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PreCompiledFuncImport::ID = 0;

PreCompiledFuncImport::PreCompiledFuncImport(CodeGenContext *CGCtx, uint32_t TheEmuKind)
    : ModulePass(ID), m_pCtx(CGCtx), m_enableCallForEmulation(false), m_emuKind(TheEmuKind) {
  initializePreCompiledFuncImportPass(*PassRegistry::getPassRegistry());
  checkAndSetEnableSubroutine();
}

const char *PreCompiledFuncImport::m_Int64DivRemFunctionNames[NUM_FUNCTIONS][NUM_TYPES] = {
    // udiv
    {"__precompiled_udiv", "__precompiled_udiv2", "__precompiled_udiv3", "__precompiled_udiv4", "__precompiled_udiv8",
     "__precompiled_udiv16"},
    // urem
    {"__precompiled_umod", "__precompiled_umod2", "__precompiled_umod3", "__precompiled_umod4", "__precompiled_umod8",
     "__precompiled_umod16"},
    // sdiv
    {"__precompiled_sdiv", "__precompiled_sdiv2", "__precompiled_sdiv3", "__precompiled_sdiv4", "__precompiled_sdiv8",
     "__precompiled_sdiv16"},
    // srem
    {"__precompiled_smod", "__precompiled_smod2", "__precompiled_smod3", "__precompiled_smod4", "__precompiled_smod8",
     "__precompiled_smod16"}};

const char *PreCompiledFuncImport::m_Int64SpDivRemFunctionNames[NUM_FUNCTIONS][NUM_TYPES] = {
    // udiv
    {"__igcbuiltin_u64_udiv_sp", "", "", "", "", ""},
    // urem
    {"__igcbuiltin_u64_urem_sp", "", "", "", "", ""},
    // sdiv
    {"__igcbuiltin_s64_sdiv_sp", "", "", "", "", ""},
    // srem
    {"__igcbuiltin_s64_srem_sp", "", "", "", "", ""}};

const char *PreCompiledFuncImport::m_Int64DpDivRemFunctionNames[NUM_FUNCTIONS][NUM_TYPES] = {
    // udiv
    {"__igcbuiltin_u64_udiv_dp", "", "", "", "", ""},
    // urem
    {"__igcbuiltin_u64_urem_dp", "", "", "", "", ""},
    // sdiv
    {"__igcbuiltin_s64_sdiv_dp", "", "", "", "", ""},
    // srem
    {"__igcbuiltin_s64_srem_dp", "", "", "", "", ""}};

const char *PreCompiledFuncImport::m_Int32EmuFunctionNames[NUM_INT32_EMU_FUNCTIONS] = {
    "precompiled_u32divrem", "precompiled_s32divrem", "precompiled_u32divrem_sp", "precompiled_s32divrem_sp"};

// The entry order must match to FunctionIDs enum!
const PreCompiledFuncInfo PreCompiledFuncImport::m_functionInfos[NUM_FUNCTION_IDS] = {
    // clang-format off
    { "__igcbuiltin_dp_add",              LIBMOD_DP_ADD_SUB },
    { "__igcbuiltin_dp_sub",              LIBMOD_DP_ADD_SUB },
    { "__igcbuiltin_dp_fma",              LIBMOD_DP_FMA_MUL },
    { "__igcbuiltin_dp_mul",              LIBMOD_DP_FMA_MUL },
    { "__igcbuiltin_dp_div",              LIBMOD_DP_DIV },
    { "__igcbuiltin_dp_div_nomadm_ieee",  LIBMOD_DP_DIV_NOMADM },
    { "__igcbuiltin_dp_div_nomadm_fast",  LIBMOD_DP_DIV_NOMADM },
    { "__igcbuiltin_dp_cmp",              LIBMOD_DP_CMP },
    { "__igcbuiltin_dp_to_int32",         LIBMOD_DP_CONV_I32 },
    { "__igcbuiltin_dp_to_uint32",        LIBMOD_DP_CONV_I32 },
    { "__igcbuiltin_int32_to_dp",         LIBMOD_DP_CONV_I32 },
    { "__igcbuiltin_uint32_to_dp",        LIBMOD_DP_CONV_I32 },
    { "__igcbuiltin_dp_to_sp",            LIBMOD_DP_CONV_SP },
    { "__igcbuiltin_sp_to_dp",            LIBMOD_DP_CONV_SP },
    { "__igcbuiltin_dp_sqrt",             LIBMOD_DP_SQRT },
    { "__igcbuiltin_dp_sqrt_nomadm_ieee", LIBMOD_DP_SQRT_NOMADM },
    { "__igcbuiltin_dp_sqrt_nomadm_fast", LIBMOD_DP_SQRT_NOMADM },
    { "__igcbuiltin_sp_div",              LIBMOD_SP_DIV },
    { "__igcbuiltin_dp_to_int64",         LIBMOD_DP_CONV_I64 },
    { "__igcbuiltin_dp_to_uint64",        LIBMOD_DP_CONV_I64 },
    { "__igcbuiltin_int64_to_dp",         LIBMOD_DP_CONV_I64 },
    { "__igcbuiltin_uint64_to_dp",        LIBMOD_DP_CONV_I64 },
    // clang-format on
};

// The entry order must match to LibraryModules enum!
const LibraryModuleInfo PreCompiledFuncImport::m_libModInfos[NUM_LIBMODS] = {
    /* LIBMOD_INT_DIV_REM */ {preCompiledFunctionLibrary, sizeof(preCompiledFunctionLibrary)},
    /* LIBMOD_UINT32_DIV_REM */ {precompiled_u32divrem, sizeof(precompiled_u32divrem)},
    /* LIBMOD_SINT32_DIV_REM,*/ {precompiled_s32divrem, sizeof(precompiled_s32divrem)},
    /* LIBMOD_UINT32_DIV_REM_SP */ {precompiled_u32divrem_sp, sizeof(precompiled_u32divrem_sp)},
    /* LIBMOD_SINT32_DIV_REM_SP,*/ {precompiled_s32divrem_sp, sizeof(precompiled_s32divrem_sp)},
    /* LIBMOD_INT_ADD_SUB */ {igcbuiltin_emu_dp_add_sub, sizeof(igcbuiltin_emu_dp_add_sub)},
    /* LIBMOD_DP_FMA_MUL  */ {igcbuiltin_emu_dp_fma_mul, sizeof(igcbuiltin_emu_dp_fma_mul)},
    /* LIBMOD_DP_DIV      */ {igcbuiltin_emu_dp_div, sizeof(igcbuiltin_emu_dp_div)},
    /* LIBMOD_DP_DIV_NOMADM */ {igcbuiltin_emu_dp_div_nomadm, sizeof(igcbuiltin_emu_dp_div_nomadm)},
    /* LIBMOD_DP_CMP      */ {igcbuiltin_emu_dp_cmp, sizeof(igcbuiltin_emu_dp_cmp)},
    /* LIBMOD_DP_CONV_I32 */ {igcbuiltin_emu_dp_conv_i32, sizeof(igcbuiltin_emu_dp_conv_i32)},
    /* LIBMOD_DP_CONV_SP  */ {igcbuiltin_emu_dp_conv_sp, sizeof(igcbuiltin_emu_dp_conv_sp)},
    /* LIBMOD_DP_SQRT     */ {igcbuiltin_emu_dp_sqrt, sizeof(igcbuiltin_emu_dp_sqrt)},
    /* LIBMOD_DP_SQRT_NOMADM */ {igcbuiltin_emu_dp_sqrt_nomadm, sizeof(igcbuiltin_emu_dp_sqrt_nomadm)},
    /* LIBMOD_SP_DIV      */ {igcbuiltin_emu_sp_div, sizeof(igcbuiltin_emu_sp_div)},
    /* LIBMOD_DP_CONV_I64 */ {igcbuiltin_emu_dp_conv_i64, sizeof(igcbuiltin_emu_dp_conv_i64)},
    /* LIBMOD_INT64_DIV_REM_SP */ {igcbuiltin_emu_i64_divrem_sp, sizeof(igcbuiltin_emu_i64_divrem_sp)},
    /* LIBMOD_INT64_DIV_REM_DP */ {igcbuiltin_emu_i64_divrem_dp, sizeof(igcbuiltin_emu_i64_divrem_dp)},
};

void PreCompiledFuncImport::eraseCallInst(CallInst *CI) {
  // erase CI if it is in the vector by setting it to zero.
  for (int i = 0, sz = (int)m_allNewCallInsts.size(); i < sz; ++i) {
    if (m_allNewCallInsts[i] == CI) {
      m_allNewCallInsts[i] = nullptr;
      return;
    }
  }
}

void PreCompiledFuncImport::handleInstrTypeChange(Instruction *oldInst, Value *newVal) {
  IGC_ASSERT(oldInst->getType()->getScalarType()->isDoubleTy());

  SmallVector<Instruction *, 8> usesOfInst;

  // Mark all uses of oldInst that may be converted from double to i64
  for (auto U : oldInst->users()) {
    if (Instruction *I = dyn_cast<Instruction>(U))
      usesOfInst.push_back(I);
  }

  // Go through the all uses of instruction
  for (auto I : usesOfInst) {
    IRBuilder<> builder(I);
    // If the use of instruction is StoreInst, we need to also convert pointer type from double to i64
    if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
      if (SI->getValueOperand()->getType()->isDoubleTy()) {
        PointerType *newPtrTy = PointerType::get(builder.getInt64Ty(), SI->getPointerAddressSpace());
        Value *newPtr = builder.CreatePointerCast(SI->getPointerOperand(), newPtrTy, "");
        I->replaceUsesOfWith(SI->getPointerOperand(), newPtr);
      } else {
        uint32_t numVecElements =
            (uint32_t)cast<IGCLLVM::FixedVectorType>(SI->getValueOperand()->getType())->getNumElements();
        Type *newVecType = IGCLLVM::FixedVectorType::get(builder.getInt64Ty(), numVecElements);
        PointerType *newVecPtrTy = newVecType->getPointerTo(SI->getPointerAddressSpace());
        Value *newPtr = builder.CreatePointerCast(SI->getPointerOperand(), newVecPtrTy, "");
        I->replaceUsesOfWith(SI->getPointerOperand(), newPtr);
      }
      I->replaceUsesOfWith(oldInst, newVal);
    }
    // For any other instruction we should add additional bitcast from i64 to double
    // because we don't know which of the uses of the instruction has math operations. For math operations (e.g. add,
    // div etc.) we have emulated code, and cannot convert double->i64. For any other (no math operations) we recognize
    // it in preProcessDouble() function. All extra bitcast instruction are deleted by InstructionCombining pass.
    else {
      builder.SetInsertPoint(oldInst);
      if (newVal->getType()->isIntegerTy(64)) {
        Value *newBitCast = builder.CreateBitCast(newVal, builder.getDoubleTy());
        I->replaceUsesOfWith(oldInst, newBitCast);
      } else {
        uint32_t numVecElements = (uint32_t)cast<IGCLLVM::FixedVectorType>(newVal->getType())->getNumElements();
        Type *newVecType = IGCLLVM::FixedVectorType::get(builder.getDoubleTy(), numVecElements);
        Value *newBitCast = builder.CreateBitCast(newVal, newVecType);
        I->replaceUsesOfWith(oldInst, newBitCast);
      }
    }
  }
}

// This function scans instructions before emulation. It converts double-related
// operations (intrinsics, instructions) into ones that can be emulated. It has:
//   1. Intrinsics
//         1: Replaced some intrinsics of double operands with a known sequence that can be emulated.
//            For example, max() does not have its corresponding emulation function. And it is replaced
//            here with cmp and select.
//         2: Replaced some double intrinsics to i64 intrinsics.
//            We can do it when we don't perform math operations
//            Example: GenISA.WaveShuffleIndex.f64 -> GenISA.WaveShuffleIndex.i64
//   2. instructions
//      Convert something like
//         1:   y = sext i32 x to i64; z = sitofp i64 y to double
//              --> z = sitofp i32 x to double
//         2:   y = zext i32 x to i64; z = uitofp i64 y to double
//              --> z = uitofp i32 x to double
//   3. Convert extract element and insert element with double type:
//         Without this conversion, vISA creates mov with i64 type - for platforms that don't support i64
//         we got an error, so we need to handle it before vISA step. If we don't perform math operations,
//         we can change double -> i64. This relates to only extract element and insert element instruction.
//         1: extract element instruction with double type to extract element instruction with i64 type.
//            This converts from:
//              %0 = load <2 x double>, <2 x double> addrspace(1)* %x, align 16
//              %1 = getelementptr inbounds double, double addrspace(1)* %y, i64 %z
//              %2 = extractelement <2 x double> %0, i32 %idx
//              store double %2, double addrspace(1)* %1, align 8
//            to:
//              %0 = load <2 x double>, <2 x double> addrspace(1)* %x, align 16
//              %1 = getelementptr inbounds double, double addrspace(1)* %y, i64 %z
//              %2 = bitcast <2 x double> %0 to <2 x i64>
//              %3 = extractelement <2 x i64> %2, i32 %idx
//              %4 = bitcast double addrspace(1)* %1 to i64 addrspace(1)*
//              store i64 %3, i64 addrspace(1)* %4, align 8
//         2: insert element instruction with double type to insert element instruction with i64 type.
//            This converts from:
//              %0 = getelementptr inbounds double, double addrspace(1)* %x, i64 %y
//              %1 = load double, double addrspace(1)* %0, align 8
//              %2 = getelementptr inbounds <2 x double>, <2 x double> addrspace(1)* %z, i64 %y
//              %3 = load <2 x double>, <2 x double> addrspace(1)* %2, align 16
//              %4 = insertelement <2 x double> %3, double %1, i32 %idx
//              store <2 x double> %4, <2 x double> addrspace(1)* %2, align 16
//            to:
//              %0 = getelementptr inbounds double, double addrspace(1)* %x, i64 %y
//              %1 = load double, double addrspace(1)* %0, align 8
//              %2 = getelementptr inbounds <2 x double>, <2 x double> addrspace(1) * %z, i64 %y
//              %3 = load <2 x double>, <2 x double> addrspace(1)* %2, align 16
//              %4 = bitcast <2 x double> %3 to <2 x i64>
//              %5 = bitcast double %1 to i64
//              %6 = insertelement <2 x i64> %4, i64 %5, i32 %idx
//              %7 = bitcast <2 x double> addrspace(1)* %2 to <2 x i64> addrspace(1)*
//              store <2 x i64> %6, <2 x i64> addrspace(1)* %7, align 16
bool PreCompiledFuncImport::preProcessDouble() {
  CodeGenContext *pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  bool hasNoNaN = pCtx->getModuleMetaData()->compOpt.FiniteMathOnly;
  SmallVector<Instruction *, 8> toBeDeleted;
  for (auto II = m_pModule->begin(), IE = m_pModule->end(); II != IE; ++II) {
    Function *Func = &(*II);
    for (inst_iterator i = inst_begin(Func), e = inst_end(Func); i != e; ++i) {
      Instruction *Inst = &*i;
      if (UIToFPInst *UFI = dyn_cast<UIToFPInst>(Inst)) {
        Value *oprd = UFI->getOperand(0);
        Type *dstTy = UFI->getType();
        uint32_t srcBits = oprd->getType()->getIntegerBitWidth();
        Instruction *zext = dyn_cast<ZExtInst>(oprd);
        if (zext && srcBits == 64 && dstTy->isDoubleTy() &&
            zext->getOperand(0)->getType()->getIntegerBitWidth() <= 32) {
          Instruction *newinst = CastInst::Create(Instruction::UIToFP, zext->getOperand(0), dstTy, "", UFI);
          UFI->replaceAllUsesWith(newinst);
          toBeDeleted.push_back(UFI);
        }
      } else if (SIToFPInst *SFI = dyn_cast<SIToFPInst>(Inst)) {
        Value *oprd = SFI->getOperand(0);
        Type *dstTy = SFI->getType();
        uint32_t srcBits = oprd->getType()->getIntegerBitWidth();
        Instruction *sext = dyn_cast<SExtInst>(oprd);
        if (sext && srcBits == 64 && dstTy->isDoubleTy() &&
            sext->getOperand(0)->getType()->getIntegerBitWidth() <= 32) {
          Instruction *newinst = CastInst::Create(Instruction::SIToFP, sext->getOperand(0), dstTy, "", SFI);
          SFI->replaceAllUsesWith(newinst);
          toBeDeleted.push_back(SFI);
        }
      } else if (CallInst *CallI = dyn_cast<CallInst>(Inst)) {
        Type *resTy = CallI->getType();
        IntrinsicInst *II = dyn_cast<IntrinsicInst>(CallI);
        GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(CallI);
        if (resTy->isDoubleTy() &&
            ((II && (II->getIntrinsicID() == Intrinsic::maxnum || II->getIntrinsicID() == Intrinsic::minnum)))) {
          // max and min on double operands
          Value *Oprd0 = CallI->getOperand(0);
          Value *Oprd1 = CallI->getOperand(1);
          bool isMax = (II && II->getIntrinsicID() == Intrinsic::maxnum);
          Instruction *res = nullptr;
          if (hasNoNaN) {
            Instruction *cond = FCmpInst::Create(Instruction::FCmp, isMax ? FCmpInst::FCMP_OGE : FCmpInst::FCMP_OLT,
                                                 Oprd0, Oprd1, "", CallI);
            cond->setDebugLoc(CallI->getDebugLoc());

            // Note that select will be splitted in legalization
            res = SelectInst::Create(cond, Oprd0, Oprd1, "", CallI);
            res->setDebugLoc(CallI->getDebugLoc());
          } else {
            // z = max/min(x,y) sementics:
            //     If either operand is NaN, return the other one (both are NaN,
            //     return NaN); otherwise, return normal max/min.
            //     isnan should be created with fcmp_une condition because unordered checks if either operand may be a
            //     NaN while ordered means that neither operand is NaN (and return false). In result of fcmp_une
            //     condition will be a comparison that return true if any of the operands is NaN. Then we return other
            //     operand than NaN.
            //
            // Convert it to:
            //   cond =  x >= y [x < y]  || x&y is unordered   (fcmp_uge|fcmp_ult)
            //   t = cond ? x : y
            //   op0_isnan = (x != x)    (fcmp_une)
            //   res = op0_isnan ? y : t
            //
            Instruction *cond = FCmpInst::Create(Instruction::FCmp, isMax ? FCmpInst::FCMP_UGE : FCmpInst::FCMP_ULT,
                                                 Oprd0, Oprd1, "", CallI);
            cond->setDebugLoc(CallI->getDebugLoc());
            Instruction *sel = SelectInst::Create(cond, Oprd0, Oprd1, "", CallI);
            sel->setDebugLoc(CallI->getDebugLoc());

            Instruction *isnan = FCmpInst::Create(Instruction::FCmp, FCmpInst::FCMP_UNE, Oprd0, Oprd0, "", CallI);
            res = SelectInst::Create(isnan, Oprd1, sel, "", CallI);
            res->setDebugLoc(CallI->getDebugLoc());
          }
          CallI->replaceAllUsesWith(res);
          toBeDeleted.push_back(CallI);
        } else if (resTy->isDoubleTy() && GII && (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_fsat)) {
          // y = fsat(x) :  1. y = 0.0 if x is NaN or x < 0.0;
          //                2. y = 1.0 if x > 1.0;
          //                3. y = y  otherwise.
          Value *Oprd0 = CallI->getOperand(0);
          Constant *FC0 = ConstantFP::get(resTy, 0.0);
          Constant *FC1 = ConstantFP::get(resTy, 1.0);
          Instruction *cond = FCmpInst::Create(Instruction::FCmp, hasNoNaN ? FCmpInst::FCMP_OLT : FCmpInst::FCMP_ULT,
                                               Oprd0, FC0, "", CallI);
          cond->setDebugLoc(CallI->getDebugLoc());
          Instruction *sel = SelectInst::Create(cond, FC0, Oprd0, "", CallI);
          sel->setDebugLoc(CallI->getDebugLoc());

          Instruction *cond1 = FCmpInst::Create(Instruction::FCmp, FCmpInst::FCMP_OGT, Oprd0, FC1, "", CallI);
          cond1->setDebugLoc(CallI->getDebugLoc());
          Instruction *res = SelectInst::Create(cond1, FC1, sel, "", CallI);
          res->setDebugLoc(CallI->getDebugLoc());

          CallI->replaceAllUsesWith(res);
          toBeDeleted.push_back(CallI);
        } else if (resTy->isDoubleTy() && GII &&
                   (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveShuffleIndex ||
                    GII->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveBroadcast)) {
          IRBuilder<> builder(Inst);
          Value *newBitCast = builder.CreateBitCast(CallI->getOperand(0), builder.getInt64Ty());
          Function *newFunc =
              GenISAIntrinsic::getDeclaration(GII->getModule(), GII->getIntrinsicID(), builder.getInt64Ty());
          Value *Args[] = {newBitCast, GII->getOperand(1), GII->getOperand(2)};
          Value *newI64Intrinsic = builder.CreateCall(newFunc, Args);

          handleInstrTypeChange(CallI, newI64Intrinsic);
          toBeDeleted.push_back(CallI);
        }
      } else if (ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(Inst)) {
        if (!EEI->getType()->isDoubleTy()) {
          continue;
        }

        IRBuilder<> builder(Inst);
        uint32_t numVecElements =
            (uint32_t)cast<IGCLLVM::FixedVectorType>(EEI->getVectorOperandType())->getNumElements();
        Type *newVecType = IGCLLVM::FixedVectorType::get(builder.getInt64Ty(), numVecElements);
        Value *newBitCast = builder.CreateBitCast(EEI->getVectorOperand(), newVecType);
        Value *newExtractInst = builder.CreateExtractElement(newBitCast, EEI->getIndexOperand());

        handleInstrTypeChange(EEI, newExtractInst);
        toBeDeleted.push_back(EEI);
      } else if (InsertElementInst *IEI = dyn_cast<InsertElementInst>(Inst)) {
        if (!IEI->getType()->getScalarType()->isDoubleTy()) {
          continue;
        }

        IRBuilder<> builder(Inst);
        uint32_t numVecElements =
            (uint32_t)cast<IGCLLVM::FixedVectorType>(IEI->getOperand(0)->getType())->getNumElements();
        Type *newVecType = IGCLLVM::FixedVectorType::get(builder.getInt64Ty(), numVecElements);
        Value *newVectorBitCast = builder.CreateBitCast(IEI->getOperand(0), newVecType);
        Value *newScalarBitCast = builder.CreateBitCast(IEI->getOperand(1), builder.getInt64Ty());
        Value *newInsertInst = builder.CreateInsertElement(newVectorBitCast, newScalarBitCast, IEI->getOperand(2), "");

        handleInstrTypeChange(IEI, newInsertInst);
        toBeDeleted.push_back(IEI);
      } else if (Inst->getOpcode() == Instruction::FNeg) {
        // check if Inst is double instruction or vector of double instructions
        Type *instType = Inst->getType();
        IGCLLVM::FixedVectorType *instVecType = dyn_cast<IGCLLVM::FixedVectorType>(instType);
        if (instType->isDoubleTy() || (instVecType && instVecType->getElementType()->isDoubleTy())) {
          IGCLLVM::IRBuilder<> builder(Inst);
          Value *fsub = nullptr;

          if (!Inst->getType()->isVectorTy()) {
            fsub = builder.CreateFSub(ConstantFP::get(Inst->getType(), 0.0f), Inst->getOperand(0));
          } else {
            uint32_t vectorSize = cast<IGCLLVM::FixedVectorType>(Inst->getType())->getNumElements();
            fsub = llvm::UndefValue::get(Inst->getType());

            for (uint32_t i = 0; i < vectorSize; ++i) {
              Value *extract = builder.CreateExtractElement(Inst->getOperand(0), i);
              Value *extract_fsub = builder.CreateFSub(ConstantFP::get(extract->getType(), 0.0f), extract);
              fsub = builder.CreateInsertElement(fsub, extract_fsub, i);
            }
          }

          Inst->replaceAllUsesWith(fsub);
          toBeDeleted.push_back(Inst);
        }
      }
    }
  }

  for (int i = 0, e = (int)toBeDeleted.size(); i < e; ++i) {
    Instruction *Inst = toBeDeleted[i];
    Inst->eraseFromParent();
  }

  return (toBeDeleted.size() > 0);
}

inline bool isPrecompiledEmulationFunction(Function *func) {
  return func->getName().contains("precompiled_s32divrem") || func->getName().contains("precompiled_u32divrem") ||
         func->getName().contains("precompiled_s32divrem_sp") || func->getName().contains("precompiled_u32divrem_sp") ||
         func->getName().contains("precompiled_convert_f64_to_f16") ||
         func->getName().contains("__igcbuiltin_sp_div") || func->getName().contains("__igcbuiltin_dp_div_nomadm_ieee");
}

void PreCompiledFuncImport::removeLLVMModuleFlag(Module *M) {
  // llvm.module.flags has the following for now
  //
  //    !llvm.module.flags = !{!0}
  //    !0 = !{i32 1, !"wchar_size", i32 4}
  //
  // If a user module has wchar_size=2, the linking will fail! Since builtin lib
  // has no use of wchar, we can safely remove this entry from metadata, which
  // will make builtin lib linkable to either wchar_size=2 or wchar_size=4.
  // (It would be better that this metadata wchar_size does not appear in builtin
  //  library in the first place.)
  //
  // "wchar_size" is only one seen so far, and it may have others in the future.
  // Thus, we should check any new flags that are introduced in the future to make
  // sure removing them is safe. In general, builtin libary's compatibility
  // should not be affected by flags!
  //
  // For now, it is safe to remove llvm.module.flags completely.
  NamedMDNode *ModFlags = M->getModuleFlagsMetadata();
  if (ModFlags) {
    ModFlags->eraseFromParent();
  }
}

bool PreCompiledFuncImport::runOnModule(Module &M) {
  m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  m_pModule = &M;
  m_changed = false;

  // When we test it, we need to set emuKind
  if (IGC_GET_FLAG_VALUE(TestIGCPreCompiledFunctions) == 1) {
    m_emuKind = IGC_GET_FLAG_VALUE(ForceEmuKind) ? IGC_GET_FLAG_VALUE(ForceEmuKind) : EmuKind::EMU_DP;
    checkAndSetEnableSubroutine();
  }

  // sanity check
  if (m_emuKind == 0) {
    // Nothing to emulate
    return false;
  }

  m_roundingMode = m_pCtx->m_DriverInfo.DPEmulationRoundingMode();
  m_flushDenorm = (m_pCtx->m_DriverInfo.DPEmulationFlushDenorm()) ? 1 : 0;
  m_flushToZero = (m_pCtx->m_DriverInfo.DPEmulationFlushToZero()) ? 1 : 0;

  for (int i = 0; i < NUM_LIBMODS; ++i) {
    m_libModuleToBeImported[i] = false;
    m_libModuleAlreadyImported[i] = false;
  }
  m_allNewCallInsts.clear();

  SmallSet<Function *, 32> origFunctions;
  for (auto II = M.begin(), IE = M.end(); II != IE; ++II) {
    Function *Func = &(*II);
    if (Func->isDeclaration()) {
      continue;
    }
    origFunctions.insert(Func);
  }

  if ((isDPEmu() || isDPConvEmu()) && preProcessDouble()) {
    m_changed = true;
  }

  unsigned int count = 0;
  while (count < 2) {
    count++;
    visit(M);

    m_CallRemDiv.clear();
    if (m_changed) {
      llvm::Linker ld(M);
      for (int i = 0; i < NUM_LIBMODS; ++i) {
        if (!m_libModuleToBeImported[i]) {
          continue;
        }

        if (m_libModuleAlreadyImported[i])
          continue;

        const char *pLibraryModule = (const char *)m_libModInfos[i].Mod;
        uint32_t libSize = m_libModInfos[i].ModSize;

        // Load the module we want to compile and link it to existing module
        StringRef BitRef(pLibraryModule, libSize);
        llvm::Expected<std::unique_ptr<llvm::Module>> ModuleOrErr =
            llvm::parseBitcodeFile(MemoryBufferRef(BitRef.str(), ""), M.getContext());
        if (llvm::Error EC = ModuleOrErr.takeError()) {
          IGC_ASSERT_MESSAGE(0, "llvm getLazyBitcodeModule - FAILED to parse bitcode");
        }
        std::unique_ptr<llvm::Module> m_pBuiltinModule = std::move(*ModuleOrErr);
        IGC_ASSERT_MESSAGE(m_pBuiltinModule, "llvm version mismatch - could not load llvm module");

        // Set target triple and datalayout to the original module (emulation func
        // works for both 64 & 32 bit applications).
        m_pBuiltinModule->setDataLayout(M.getDataLayout());
        m_pBuiltinModule->setTargetTriple(M.getTargetTriple());
        removeLLVMModuleFlag(m_pBuiltinModule.get());

        // Linking the two modules
        if (ld.linkInModule(std::move(m_pBuiltinModule))) {
          IGC_ASSERT_MESSAGE(0, "Error linking the two modules");
        }
        m_libModuleAlreadyImported[i] = true;
        m_pBuiltinModule = nullptr;
      }
    }
    for (int i = 0; i < NUM_LIBMODS; ++i) {
      m_libModuleToBeImported[i] = false;
    }
  }

  bool hasNewMDEntry = false;
  FuncNeedIA.clear();
  NewFuncWithIA.clear();

  std::vector<std::pair<CallInst *, CallInst *>> replaceInsts;
  auto createIntrinsicCall = [&](CallInst *CI, GenISAIntrinsic::ID GISAIntr) {
    IGCLLVM::IRBuilder<> builder(CI);
    std::vector<Value *> args;
    std::vector<Type *> types;

    types.push_back(CI->getType());

    for (unsigned i = 0; i < IGCLLVM::getNumArgOperands(CI); i++) {
      types.push_back(CI->getArgOperand(i)->getType());
      args.push_back(CI->getArgOperand(i));
    }
    Function *pFunc = GenISAIntrinsic::getDeclaration(&M, GISAIntr, types);
    CallInst *pCall = builder.CreateCall(pFunc, args);
    replaceInsts.emplace_back(CI, pCall);
  };

  for (auto &F : M) {
    if (isPrecompiledEmulationFunction(&F)) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (CallInst *CI = dyn_cast<CallInst>(&I)) {
            if (Function *calledFunc = CI->getCalledFunction()) {
              if (calledFunc->getName().startswith("GenISA_fma_rtz")) {
                createIntrinsicCall(CI, GenISAIntrinsic::GenISA_fma_rtz);
              } else if (calledFunc->getName().startswith("GenISA_fma_rtp")) {
                createIntrinsicCall(CI, GenISAIntrinsic::GenISA_fma_rtp);
              } else if (calledFunc->getName().startswith("GenISA_fma_rtn")) {
                createIntrinsicCall(CI, GenISAIntrinsic::GenISA_fma_rtn);
              } else if (calledFunc->getName().startswith("GenISA_add_rte")) {
                createIntrinsicCall(CI, GenISAIntrinsic::GenISA_add_rte);
              } else if (calledFunc->getName().startswith("GenISA_add_rtz")) {
                createIntrinsicCall(CI, GenISAIntrinsic::GenISA_add_rtz);
              } else if (calledFunc->getName().startswith("GenISA_add_rtn")) {
                createIntrinsicCall(CI, GenISAIntrinsic::GenISA_add_rtn);
              } else if (calledFunc->getName().startswith("GenISA_add_rtp")) {
                createIntrinsicCall(CI, GenISAIntrinsic::GenISA_add_rtp);
              } else if (calledFunc->getName().startswith("GenISA_mul_rtz")) {
                createIntrinsicCall(CI, GenISAIntrinsic::GenISA_mul_rtz);
              } else if (calledFunc->getName().startswith("GenISA_uitof_rtz")) {
                createIntrinsicCall(CI, GenISAIntrinsic::GenISA_uitof_rtz);
              }
            }
          }
        }
      }
    }
  }

  for (const auto &p : replaceInsts) {
    p.first->replaceAllUsesWith(p.second);
    p.first->eraseFromParent();
  }

  // Set new call inst's calling convention to its callee's
  for (int i = 0, sz = (int)m_allNewCallInsts.size(); i < sz; ++i) {
    CallInst *callInst = m_allNewCallInsts[i];
    if (callInst == nullptr) {
      continue;
    }
    if (Function *callee = callInst->getCalledFunction()) {
      callInst->setCallingConv(callee->getCallingConv());
    }
  }

  llvm::SmallVector<ImportedFunction, 32> importedFunctions;
  unsigned totalNumberOfInlinedInst = 0, totalNumberOfPotentiallyInlinedInst = 0;
  int emuFC = (int)IGC_GET_FLAG_VALUE(EmulationFunctionControl);

  // Post processing, set those imported functions as internal linkage.
  for (auto II = M.begin(), IE = M.end(); II != IE;) {
    Function *Func = &(*II);
    ++II;
    if (!Func || Func->isDeclaration()) {
      continue;
    }

    if (!origFunctions.count(Func)) {
      // Make it internal linkage.
      Func->setLinkage(GlobalValue::InternalLinkage);

      // Need to check if it is dead after setting linkage.
      if (Func->isDefTriviallyDead()) {
        eraseFunction(&M, Func);
        continue;
      }

      if (std::find(importedFunctions.begin(), importedFunctions.end(), Func) == importedFunctions.end())
        importedFunctions.push_back(Func);
    } else {
      // Make sure original func isn't inlined accidentally.
      Func->removeFnAttr(llvm::Attribute::AlwaysInline);
    }
  }

  // Sort imported instructions in preferred inlining order.
  std::sort(importedFunctions.begin(), importedFunctions.end(), ImportedFunction::compare);

  // Post processing, set those imported functions as alwaysinline.
  // Also count how many instructions would be added to the shader
  // if inlining occurred.
  for (auto II = importedFunctions.begin(), IE = importedFunctions.end(); II != IE; ++II) {
    Function *Func = II->F;

    // Remove noinline/AlwaysInline attr if present.
    Func->removeFnAttr(llvm::Attribute::NoInline);
    Func->removeFnAttr(llvm::Attribute::AlwaysInline);

    if (m_enableCallForEmulation && emuFC != FLAG_FCALL_DEFAULT && emuFC != FLAG_FCALL_FORCE_INLINE) {
      // Disable inlining completely.
      continue;
    }

    if ((Func->hasOneUse() && !II->isSlowDPEmuFunc()) || emuFC == FLAG_FCALL_FORCE_INLINE) {
      Func->addFnAttr(llvm::Attribute::AlwaysInline);
      continue;
    }

    // Don't want to subroutine small functions
    if (II->funcInstructions <= 5) {
      // Add AlwaysInline attribute to force inlining all calls.
      Func->addFnAttr(llvm::Attribute::AlwaysInline);

      continue;
    }

    // Don't inline original slow DP emu functions, they are only for passing
    // conformance, not for perf.
    if ((isDPEmu() || isDPConvEmu()) && II->isSlowDPEmuFunc())
      continue;

    totalNumberOfPotentiallyInlinedInst += II->totalInstructions;

    // If function fits in threshold, always inline.
    if (totalNumberOfInlinedInst + II->totalInstructions <= (unsigned)IGC_GET_FLAG_VALUE(InlinedEmulationThreshold)) {
      totalNumberOfInlinedInst += II->totalInstructions;
      Func->addFnAttr(llvm::Attribute::AlwaysInline);
    }
  }

  // Check if more functions can fit in threshold if they would be split into inline/noinline copies.
  if (m_enableCallForEmulation && emuFC == FLAG_FCALL_DEFAULT &&
      totalNumberOfInlinedInst < (unsigned)IGC_GET_FLAG_VALUE(InlinedEmulationThreshold)) {
    for (auto II = importedFunctions.begin(); II != importedFunctions.end(); ++II) {
      Function *Func = II->F;

      if (Func->hasFnAttribute(llvm::Attribute::AlwaysInline) ||
          ((isDPEmu() || isDPConvEmu()) && II->isSlowDPEmuFunc()))
        continue;

      unsigned calls =
          ((unsigned)IGC_GET_FLAG_VALUE(InlinedEmulationThreshold) - totalNumberOfInlinedInst) / II->funcInstructions;
      if (calls > 0) {
        // Split function into inline/no-inline copies.
        ImportedFunction copy = createInlinedCopy(*II, calls);
        importedFunctions.push_back(copy);
        totalNumberOfInlinedInst += copy.totalInstructions;
      }
    }
  }

  for (auto II = importedFunctions.begin(), IE = importedFunctions.end(); II != IE; ++II) {
    Function *Func = II->F;

    if (!Func->hasFnAttribute(llvm::Attribute::AlwaysInline)) {
      // Special handling of DP functions: any one that has not been marked as inline
      // at this point, it will be either subroutine or stackcall.
      const bool isDPCallFunc = ((isDPEmu() || isDPConvEmu()) && II->isSlowDPEmuFunc());

      // Use subroutine/stackcall for some DP emulation functions if
      // EmulationFunctionControl is set so, or
      // use subroutines if total number of instructions added when
      // all emulated functions are inlined exceed InlinedEmulationThreshold.
      // If Func is a slow version of DP emu func, perf isn't important.
      if (m_enableCallForEmulation &&
          (totalNumberOfPotentiallyInlinedInst > (unsigned)IGC_GET_FLAG_VALUE(InlinedEmulationThreshold) ||
           isDPCallFunc)) {
        Func->addFnAttr(llvm::Attribute::NoInline);
        if (isDPCallFunc && (emuFC == FLAG_FCALL_FORCE_STACKCALL ||
                             (emuFC == FLAG_FCALL_DEFAULT && m_pCtx->type == ShaderType::OPENCL_SHADER))) {
          Func->addFnAttr("visaStackCall");
        }

        // Create an entry in metadata for this function and add
        // implicit args for private base if needed (note that the
        // entry func should have private base as implicit arg
        // already. It has been added in PrivateMemoryUsageAnalysis.
        // And emulation functions may need implicit args for private
        // memory only, not any other implicit args). Once implicit
        // args are added, function signature and its calls are changed
        // accordingly.
        addMDFuncEntryForEmulationFunc(Func);
        hasNewMDEntry = true;

        if (isDPCallFunc) {
          // To reduce compiling time, disable retry
          m_pCtx->m_retryManager->Disable(true);
        }
      } else {
        // Add AlwaysInline attribute to force inlining all calls.
        Func->addFnAttr(llvm::Attribute::AlwaysInline);
      }
    }
  }

  int sz = (int)FuncNeedIA.size();
  if (sz > 0) {
    createFuncWithIA();
    for (int i = 0; i < sz; ++i) {
      replaceFunc(FuncNeedIA[i], NewFuncWithIA[i]);
    }

    // free FuncsImpArgs
    for (const auto &I : FuncsImpArgs) {
      ImplicitArgs *IA = I.second;
      delete IA;
    }
    FuncsImpArgs.clear();
  }

  if (hasNewMDEntry)
    m_pMdUtils->save(M.getContext());

#if 0
    {
        CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        DumpLLVMIR(pCtx, "PrecompiledLib_after");
    }
#endif

  return m_changed;
}

PreCompiledFuncImport::ImportedFunction::ImportedFunction(Function *F)
    : F(F), type(EmuType::OTHER), funcInstructions(0), totalInstructions(0) {
  // Count number of new instructions added by inlining.
  for (BasicBlock &BB : *F)
    funcInstructions += BB.size();

  updateUses();

  // Get type of imported function.
  StringRef name = F->getName();

  if (name.equals("__igcbuiltin_dp_div_nomadm_ieee") || name.equals("__igcbuiltin_dp_div_nomadm_fast") ||
      name.equals("__igcbuiltin_dp_sqrt_nomadm_ieee") || name.equals("__igcbuiltin_dp_sqrt_nomadm_fast")) {
    type = EmuType::FASTDP;
  } else if (name.equals("__igcbuiltin_dp_add") || name.equals("__igcbuiltin_dp_sub") ||
             name.equals("__igcbuiltin_dp_fma") || name.equals("__igcbuiltin_dp_mul") ||
             name.equals("__igcbuiltin_dp_div") || name.equals("__igcbuiltin_dp_cmp") ||
             name.equals("__igcbuiltin_dp_to_int32") || name.equals("__igcbuiltin_dp_to_uint32") ||
             name.equals("__igcbuiltin_int32_to_dp") || name.equals("__igcbuiltin_uint32_to_dp") ||
             name.equals("__igcbuiltin_dp_to_sp") || name.equals("__igcbuiltin_sp_to_dp") ||
             name.equals("__igcbuiltin_dp_sqrt")) {
    // If true, it is a slow version of DP emu functions. Those functions
    // are the original ones for just passing conformance, not for perf.
    type = EmuType::SLOWDP;
  } else {
    for (int i = 0; i < NUM_FUNCTIONS && type == EmuType::OTHER; ++i) {
      for (int j = 0; j < NUM_TYPES && type == EmuType::OTHER; ++j) {
        if (name.equals(m_Int64SpDivRemFunctionNames[i][j]) || name.equals(m_Int64DpDivRemFunctionNames[i][j])) {
          type = EmuType::INT64;
        }
      }
    }
  }
}

void PreCompiledFuncImport::ImportedFunction::updateUses() { totalInstructions = funcInstructions * F->getNumUses(); }

PreCompiledFuncImport::ImportedFunction PreCompiledFuncImport::ImportedFunction::copy(ImportedFunction &other) {
  ValueToValueMapTy VM;
  Function *copy = CloneFunction(other.F, VM);
  return PreCompiledFuncImport::ImportedFunction(copy, other.type, other.funcInstructions, 0);
}

// Compare two imported functions in order preferred for inlining.
bool PreCompiledFuncImport::ImportedFunction::compare(ImportedFunction &L, ImportedFunction &R) {
  // First sort by preferred type of emulation.
  if (L.type != R.type)
    return L.type < R.type;

  // Then sort by number of inlined instructions.
  return L.totalInstructions < R.totalInstructions;
};

PreCompiledFuncImport::ImportedFunction PreCompiledFuncImport::createInlinedCopy(ImportedFunction &IF, unsigned n) {
  std::vector<CallInst *> toDelete;

  // Make copy that is always inlined.
  ImportedFunction copy = ImportedFunction::copy(IF);
  copy.F->setName(IF.F->getName() + "_always_inline");
  copy.F->addFnAttr(llvm::Attribute::AlwaysInline);

  // Collect first n calls to replace with copy.
  llvm::SmallVector<CallInst *, 8> calls;
  auto it = IF.F->user_begin();
  for (unsigned i = 0; i < n; ++i) {
    CallInst *oldCall = dyn_cast<CallInst>(*(it++));
    IGC_ASSERT(oldCall);
    calls.push_back(oldCall);
  }

  // Replace with always inlined copy.
  for (CallInst *oldCall : calls) {
    std::vector<Value *> args;
    for (unsigned arg = 0; arg < IGCLLVM::getNumArgOperands(oldCall); ++arg)
      args.push_back(oldCall->getArgOperand(arg));

    // Create new call and insert it before old one
    CallInst *newCall = CallInst::Create(copy.F, args, "", oldCall);

    newCall->setCallingConv(copy.F->getCallingConv());
    newCall->setAttributes(oldCall->getAttributes());
    newCall->setDebugLoc(oldCall->getDebugLoc());

    oldCall->replaceAllUsesWith(newCall);
    toDelete.push_back(oldCall);
  }

  for (auto C : toDelete)
    C->eraseFromParent();

  copy.updateUses();
  IF.updateUses();

  return copy;
}

void PreCompiledFuncImport::visitBinaryOperator(BinaryOperator &I) {
  if (I.getOperand(0)->getType()->isIntOrIntVectorTy()) {
    unsigned int integerBitWidth = I.getOperand(0)->getType()->getScalarType()->getIntegerBitWidth();

    if (integerBitWidth == 64) {
      if (isI64DivRem()) {
        switch (I.getOpcode()) {
        case Instruction::UDiv:
          processDivide(I, FUNCTION_UDIV);
          break;
        case Instruction::URem:
          processDivide(I, FUNCTION_UREM);
          break;
        case Instruction::SDiv:
          processDivide(I, FUNCTION_SDIV);
          break;
        case Instruction::SRem:
          processDivide(I, FUNCTION_SREM);
          break;
        default:
          // nothing
          break;
        };
      }
    } else {
      // for any other integer bit width - 8/16 will all get up-cast to 32 bit before emulation
      if (isI32DivRem() || isI32DivRemSP()) {
        switch (I.getOpcode()) {
        case Instruction::UDiv:
        case Instruction::URem: {
          Int32DivRemEmuRemaining = true;
          BinaryOperator *newInst = &I;
          if (integerBitWidth != 32) {
            newInst = upcastTo32Bit(&I);
          }
          if (isI32DivRem()) {
            processInt32Divide(*newInst, FUNCTION_32_UDIVREM);
          } else {
            processInt32Divide(*newInst, FUNCTION_32_UDIVREM_SP);
          }
        } break;
        case Instruction::SDiv:
        case Instruction::SRem: {
          Int32DivRemEmuRemaining = true;
          BinaryOperator *newInst = &I;
          if (integerBitWidth != 32) {
            newInst = upcastTo32Bit(&I);
          }
          if (isI32DivRem()) {
            processInt32Divide(*newInst, FUNCTION_32_SDIVREM);
          } else {
            processInt32Divide(*newInst, FUNCTION_32_SDIVREM_SP);
          }
        } break;
        default:
          // nothing
          break;
        };
      }
    }
  } else if (isDPEmu() && I.getOperand(0)->getType()->isDoubleTy()) {
    switch (I.getOpcode()) {
    case Instruction::FMul:
      processFPBinaryOperator(I, FUNCTION_DP_MUL);
      break;
    case Instruction::FAdd:
      processFPBinaryOperator(I, FUNCTION_DP_ADD);
      break;
    case Instruction::FSub:
      processFPBinaryOperator(I, FUNCTION_DP_SUB);
      break;
    case Instruction::FDiv:
      processFPBinaryOperator(I, FUNCTION_DP_DIV);
      break;

    default:
      // nothing
      break;
    };
  } else if (isDPDivSqrtEmu() && I.getOperand(0)->getType()->isDoubleTy() && I.getOpcode() == Instruction::FDiv) {
    // Partial hardware DP support available, can use faster implementation.
    processFPBinaryOperator(I,
                            I.getFastMathFlags().isFast() ? FUNCTION_DP_DIV_NOMADM_FAST : FUNCTION_DP_DIV_NOMADM_IEEE);
  }
}

//%rem = srem i8 %a, %b
// to
// %rem1 = srem i32 %c, %d
BinaryOperator *PreCompiledFuncImport::upcastTo32Bit(BinaryOperator *I) {
  IGCLLVM::IRBuilder<> IRB(I);

  // original 8/16 bit src0 and src1
  Value *src0 = I->getOperand(0);
  Value *src1 = I->getOperand(1);

  // new 32 bit src0 and src1
  Value *newSrc0 = nullptr;
  Value *newSrc1 = nullptr;

  BinaryOperator *new32BitInst = nullptr;

  switch (I->getOpcode()) {
  case Instruction::UDiv: {
    newSrc0 = IRB.CreateZExt(src0, Type::getInt32Ty(I->getContext()));
    newSrc1 = IRB.CreateZExt(src1, Type::getInt32Ty(I->getContext()));
    // newInst = dyn_cast<BinaryOperator>(IRB.CreateUDiv(newSrc0, newSrc1));
  } break;
  case Instruction::URem: {
    newSrc0 = IRB.CreateZExt(src0, Type::getInt32Ty(I->getContext()));
    newSrc1 = IRB.CreateZExt(src1, Type::getInt32Ty(I->getContext()));
    // newInst = dyn_cast<BinaryOperator>(IRB.CreateURem(newSrc0, newSrc1));
  } break;
  case Instruction::SDiv: {
    newSrc0 = IRB.CreateSExt(src0, Type::getInt32Ty(I->getContext()));
    newSrc1 = IRB.CreateSExt(src1, Type::getInt32Ty(I->getContext()));
    // newInst = dyn_cast<BinaryOperator>(IRB.CreateSDiv(newSrc0, newSrc1));
  } break;
  case Instruction::SRem: {
    newSrc0 = IRB.CreateSExt(src0, Type::getInt32Ty(I->getContext()));
    newSrc1 = IRB.CreateSExt(src1, Type::getInt32Ty(I->getContext()));
    // newInst = dyn_cast<BinaryOperator>(IRB.CreateSRem(newSrc0, newSrc1));
  } break;
  default: {
    IGC_ASSERT_MESSAGE(0, "should not reach here for Any other inst");
    break;
  }
  };
  new32BitInst = dyn_cast<BinaryOperator>(IRB.CreateBinOp(I->getOpcode(), newSrc0, newSrc1, ""));
  IGC_ASSERT(new32BitInst != nullptr);

  Instruction *replaceInst = dyn_cast<Instruction>(IRB.CreateTrunc(new32BitInst, I->getType()));
  IGC_ASSERT(replaceInst != nullptr);

  I->replaceAllUsesWith(replaceInst);
  replaceInst->setDebugLoc(I->getDebugLoc());
  I->eraseFromParent();

  return new32BitInst;
}

// 32 bit emulation for division/remainder
void PreCompiledFuncImport::processInt32Divide(BinaryOperator &inst, Int32EmulatedFunctions function) {
  StringRef funcName = m_Int32EmuFunctionNames[function];
  Function *func = m_pModule->getFunction(funcName);

  Type *intTy = Type::getInt32Ty(inst.getContext());
  Type *intPtrTy = Type::getInt32PtrTy(inst.getContext());

  // Try to look up the function in the module's symbol
  // table first, else add it.
  if (func == NULL) {
    Type *types[3];

    types[0] = inst.getOperand(0)->getType();
    types[1] = inst.getOperand(1)->getType();
    types[2] = intPtrTy;
    FunctionType *FuncIntrType = FunctionType::get(intTy, // void Return Type
                                                   types, // params
                                                   false);

    func = Function::Create(FuncIntrType, GlobalValue::ExternalLinkage, funcName, m_pModule);
  }
  func->addRetAttr(llvm::Attribute::AlwaysInline);

  // Create a call to emulation function
  Value *args[3];
  args[0] = inst.getOperand(0);
  args[1] = inst.getOperand(1);
  IGCLLVM::IRBuilder<> builder(&*inst.getFunction()->getEntryBlock().getFirstInsertionPt());
  AllocaInst *pRem = builder.CreateAlloca(intTy, nullptr, "Remainder");
  builder.SetInsertPoint(&inst);
  args[2] = pRem;
  CallInst *funcCall = CallInst::Create(func, args, inst.getName(), &inst);
  addCallInst(funcCall);
  funcCall->setDebugLoc(inst.getDebugLoc());

  if (inst.getOpcode() == Instruction::UDiv || inst.getOpcode() == Instruction::URem) {
    if (isI32DivRem()) {
      m_libModuleToBeImported[LIBMOD_UINT32_DIV_REM] = true;
    } else {
      m_libModuleToBeImported[LIBMOD_UINT32_DIV_REM_SP] = true;
    }
  } else if (inst.getOpcode() == Instruction::SDiv || inst.getOpcode() == Instruction::SRem) {
    if (isI32DivRem()) {
      m_libModuleToBeImported[LIBMOD_SINT32_DIV_REM] = true;
    } else {
      m_libModuleToBeImported[LIBMOD_SINT32_DIV_REM_SP] = true;
    }
  }

  // Replace the original int32 udiv/sdiv/urem/srem inst with call to emulation function
  if (inst.getOpcode() == Instruction::UDiv || inst.getOpcode() == Instruction::SDiv) {
    inst.replaceAllUsesWith(funcCall);
    inst.eraseFromParent();
  } else {
    Value *l = builder.CreateLoad(cast<llvm::AllocaInst>(pRem)->getAllocatedType(), pRem, "rem");
    inst.replaceAllUsesWith(l);
    inst.eraseFromParent();
  }
  m_changed = true;
}

// 64 bit emulation for divide
void PreCompiledFuncImport::processDivide(BinaryOperator &inst, EmulatedFunctions function) {
  unsigned int numElements = 1;
  unsigned int elementIndex = 0;

  Type *argumentType = inst.getOperand(0)->getType();

  if (auto argumentVType = dyn_cast<IGCLLVM::FixedVectorType>(argumentType)) {
    numElements = (unsigned)argumentVType->getNumElements();
  }

  switch (numElements) {
  case 1:
    elementIndex = 0;
    break;
  case 2:
    elementIndex = 1;
    break;
  case 3:
    elementIndex = 2;
    break;
  case 4:
    elementIndex = 3;
    break;
  case 8:
    elementIndex = 4;
    break;
  case 16:
    elementIndex = 5;
    break;
  default:
    IGC_ASSERT_MESSAGE(0, "Unexpected vector size");
    return;
  }

  const char *funcName = nullptr;
  LibraryModules module;
  getInt64DivideEmuType(function, elementIndex, funcName, module);

  Function *func = m_pModule->getFunction(funcName);

  // Try to look up the function in the module's symbol
  // table first, else add it.
  if (func == NULL) {
    Type *types[2];

    types[0] = inst.getOperand(0)->getType();
    types[1] = inst.getOperand(1)->getType();

    FunctionType *FuncIntrType = FunctionType::get(inst.getType(), types, false);

    func = Function::Create(FuncIntrType, GlobalValue::ExternalLinkage, funcName, m_pModule);
  }

  Value *args[2];

  args[0] = inst.getOperand(0);
  args[1] = inst.getOperand(1);

  CallInst *funcCall = CallInst::Create(func, args, inst.getName(), &inst);
  addCallInst(funcCall);
  funcCall->setDebugLoc(inst.getDebugLoc());

  inst.replaceAllUsesWith(funcCall);
  inst.eraseFromParent();

  m_libModuleToBeImported[module] = true;
  m_changed = true;
}

// Select correct module and function for int64 div/rem emulation.
void PreCompiledFuncImport::getInt64DivideEmuType(EmulatedFunctions function, unsigned int elementIndex,
                                                  const char *&functionName, LibraryModules &module) {
  // overwrite options
  switch (IGC_GET_FLAG_VALUE(ForceI64DivRemEmu)) {
  case 1:
    functionName = m_Int64DivRemFunctionNames[function][elementIndex];
    module = LIBMOD_INT_DIV_REM;
    return;
  case 2:
    functionName = m_Int64SpDivRemFunctionNames[function][elementIndex];
    module = LIBMOD_INT64_DIV_REM_SP;
    return;
  case 3:
    functionName = m_Int64DpDivRemFunctionNames[function][elementIndex];
    module = LIBMOD_INT64_DIV_REM_DP;
    return;
  default:
    break;
  }

  // look only for native DP support (ignore emulated DP)
  if (m_pCtx->platform.hasNoFP64Inst() || isDPEmu() || isDPConvEmu() || isDPDivSqrtEmu()) {
    // no full DP support, use SP
    functionName = m_Int64SpDivRemFunctionNames[function][elementIndex];
    module = LIBMOD_INT64_DIV_REM_SP;
    return;
  }

  // DP supported
  functionName = m_Int64DpDivRemFunctionNames[function][elementIndex];
  module = LIBMOD_INT64_DIV_REM_DP;
}

void PreCompiledFuncImport::visitFPTruncInst(llvm::FPTruncInst &inst) {
  if ((isRTEFP64toFP16() || isDPEmu() || isDPConvEmu()) && inst.getDestTy()->isHalfTy() &&
      inst.getSrcTy()->isDoubleTy()) {
    if (inst.getDestTy()->isVectorTy()) {
      IGC_ASSERT_MESSAGE(0, "Unexpected vector size");
      return;
    }

    ERoundingMode RM = static_cast<ERoundingMode>(m_pCtx->getModuleMetaData()->compOpt.FloatRoundingMode);
    if (isRTEFP64toFP16() && !isDPEmu() && !isDPConvEmu() && RM != ERoundingMode::ROUND_TO_NEAREST_EVEN) {
      // It is safe to use a sequence of conversions for RTZ, RTN and RTP (fp64 -> fp32 -> fp16)
      return;
    }
    const StringRef funcName = RM == ERoundingMode::ROUND_TO_ZERO       ? "__precompiled_convert_f64_to_f16_rtz"
                               : RM == ERoundingMode::ROUND_TO_NEGATIVE ? "__precompiled_convert_f64_to_f16_rtn"
                               : RM == ERoundingMode::ROUND_TO_POSITIVE ? "__precompiled_convert_f64_to_f16_rtp"
                                                                        : "__precompiled_convert_f64_to_f16_rte";
    Function *func = m_pModule->getFunction(funcName);

    // Try to look up the function in the module's symbol
    // table first, else add it.
    if (func == NULL) {
      FunctionType *FuncIntrType = FunctionType::get(inst.getDestTy(), inst.getSrcTy(), false);

      func = Function::Create(FuncIntrType, GlobalValue::ExternalLinkage, funcName, m_pModule);
    }

    CallInst *funcCall = CallInst::Create(func, inst.getOperand(0));
    addCallInst(funcCall);
    ReplaceInstWithInst(&inst, funcCall);

    m_libModuleToBeImported[LIBMOD_INT_DIV_REM] = true;
    m_changed = true;
    return;
  }

  if ((isDPEmu() || isDPConvEmu()) && inst.getDestTy()->isFloatTy() && inst.getSrcTy()->isDoubleTy()) {
    Function *newFunc = getOrCreateFunction(FUNCTION_DP_TO_SP);
    Value *args[5];

    Type *intTy = Type::getInt32Ty(m_pModule->getContext());
    Function *CurrFunc = inst.getParent()->getParent();
    args[0] = inst.getOperand(0);
    args[1] = ConstantInt::get(intTy, m_roundingMode); // rounding mode
    args[2] = ConstantInt::get(intTy, m_flushToZero);  // flush to zero
    args[3] = ConstantInt::get(intTy, m_flushDenorm);  // flush denorm
    args[4] = createFlagValue(CurrFunc);               // ignored

    CallInst *funcCall = CallInst::Create(newFunc, args, inst.getName(), &inst);
    addCallInst(funcCall);
    funcCall->setDebugLoc(inst.getDebugLoc());

    inst.replaceAllUsesWith(funcCall);
    inst.eraseFromParent();

    m_changed = true;
    return;
  }
}

// Common code to process fadd/fsub/fmul/fdiv whose emu function prototype is
//   double (double, double, int, int, int, int*);
void PreCompiledFuncImport::processFPBinaryOperator(Instruction &I, FunctionIDs FID) {
  ConstantFP *constDouble = nullptr;
  if (FID == FUNCTION_DP_SUB && isa<ConstantFP>(I.getOperand(0))) {
    constDouble = cast<ConstantFP>(I.getOperand(0));
  }

  if (constDouble && constDouble->isZeroValue()) // turn the fsub into an and/xor/and/or operation
  {
    Type *intTy = Type::getInt32Ty(m_pModule->getContext());
    Type *DoubleTy = Type::getDoubleTy(m_pModule->getContext());
    VectorType *vec2Ty = IGCLLVM::FixedVectorType::get(intTy, 2);

    Instruction *twoI32 = CastInst::Create(Instruction::BitCast, I.getOperand(1), vec2Ty, "", &I);
    twoI32->setDebugLoc(I.getDebugLoc());
    Instruction *topI32 = ExtractElementInst::Create(twoI32, ConstantInt::get(intTy, 1), "", &I);
    topI32->setDebugLoc(I.getDebugLoc());

    // Isolate and flip sign bit
    Instruction *Inst2 = BinaryOperator::CreateAnd(topI32, ConstantInt::get(intTy, 0x80000000), "", &I);
    Inst2->setDebugLoc(I.getDebugLoc());
    Instruction *Inst3 = BinaryOperator::CreateXor(Inst2, ConstantInt::get(intTy, 0x80000000), "", &I);
    Inst3->setDebugLoc(I.getDebugLoc());

    // Change sign bit and pack back new value
    Instruction *Inst4 = BinaryOperator::CreateAnd(topI32, ConstantInt::get(intTy, 0x7FFFFFFF), "", &I);
    Inst4->setDebugLoc(I.getDebugLoc());
    Instruction *newTopI32 = BinaryOperator::CreateOr(Inst4, Inst3, "", &I);
    newTopI32->setDebugLoc(I.getDebugLoc());
    Instruction *finalVal = InsertElementInst::Create(twoI32, newTopI32, ConstantInt::get(intTy, 1), "", &I);
    finalVal->setDebugLoc(I.getDebugLoc());
    Instruction *finalValDouble = CastInst::Create(Instruction::BitCast, finalVal, DoubleTy, "", &I);
    finalValDouble->setDebugLoc(I.getDebugLoc());

    I.replaceAllUsesWith(finalValDouble);
    I.eraseFromParent();
    m_changed = true;
  } else {
    Function *newFunc = getOrCreateFunction(FID);

    SmallVector<Value *, 6> args;
    args.push_back(I.getOperand(0));
    args.push_back(I.getOperand(1));

    if (FID != FUNCTION_DP_DIV_NOMADM_IEEE && FID != FUNCTION_DP_DIV_NOMADM_FAST) {
      Type *intTy = Type::getInt32Ty(m_pModule->getContext());
      Function *CurrFunc = I.getParent()->getParent();
      args.push_back(ConstantInt::get(intTy, m_roundingMode)); // rounding mode
      args.push_back(ConstantInt::get(intTy, m_flushToZero));  // flush to zero
      args.push_back(ConstantInt::get(intTy, m_flushDenorm));  // flush denorm
      args.push_back(createFlagValue(CurrFunc));               // FP flag, ignored
    }

    CallInst *funcCall = CallInst::Create(newFunc, args, I.getName(), &I);
    addCallInst(funcCall);
    funcCall->setDebugLoc(I.getDebugLoc());

    I.replaceAllUsesWith(funcCall);
    I.eraseFromParent();
    m_changed = true;
  }
}

Function *PreCompiledFuncImport::getOrCreateFunction(FunctionIDs FID) {
  // Common arguments for DP emulation functions
  //
  // Rounding Modes
  //   #define RN 0
  //   #define RD 1
  //   #define RU 2
  //   #define RZ 3
  //
  // Return value for CMP function
  //   #define CMP_EQ 0
  //   #define CMP_LT 1
  //   #define CMP_GT 2
  //   #define CMP_UNORD 3
  //
  // ftz
  //   0            : not "flush to zero"
  //   1 (non-zero) : flush to zero   (non-zero)
  //
  // daz
  //   0            : retain denorm
  //   1 (non-zero) : denorm as zero.
  //
  // flag bits
  //   #define INEXACT   0x20
  //   #define UNDERFLOW 0x10
  //   #define OVERFLOW  0x8
  //   #define ZERODIV   0x4
  //   #define DENORMAL  0x2
  //   #define INVALID   0x1
  //

  const PreCompiledFuncInfo &finfo = m_functionInfos[FID];
  StringRef funcName = finfo.FuncName;

  Function *newFunc = m_pModule->getFunction(funcName);
  if (newFunc) {
    // Already created, just return it.
    return newFunc;
  }

  SmallVector<Type *, 8> argTypes;
  Type *intTy = Type::getInt32Ty(m_pModule->getContext());
  Type *int64Ty = Type::getInt64Ty(m_pModule->getContext());
  Type *dpTy = Type::getDoubleTy(m_pModule->getContext());
  Type *spTy = Type::getFloatTy(m_pModule->getContext());
  Type *intPtrTy = intTy->getPointerTo(ADDRESS_SPACE_PRIVATE);
  Type *retTy;
  switch (FID) {
  case FUNCTION_DP_FMA:
    // double dp_fma (double xin, double yin, double zin,
    //                int rmode, int ftz, int daz,
    //                int* pflags);
    argTypes.push_back(dpTy);
    // fall-through
  case FUNCTION_DP_MUL:
  case FUNCTION_DP_ADD:
  case FUNCTION_DP_SUB:
  case FUNCTION_DP_DIV:
    // double dp_add/sub/mul/div (
    //     double xin, double yin,
    //     int rmode, int ftz, int daz,
    //     int* pflags)
    argTypes.push_back(dpTy);
    argTypes.push_back(dpTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intPtrTy);

    retTy = dpTy;
    break;

  case FUNCTION_DP_DIV_NOMADM_IEEE:
  case FUNCTION_DP_DIV_NOMADM_FAST:
    // double dp_div_nomadm(double xin, double yin)
    argTypes.push_back(dpTy);
    argTypes.push_back(dpTy);
    retTy = dpTy;
    break;

  case FUNCTION_DP_CMP:
    // int dp_cmp (double xin, double yin, int daz)
    argTypes.push_back(dpTy);
    argTypes.push_back(dpTy);
    argTypes.push_back(intTy);

    retTy = intTy;
    break;

  case FUNCTION_DP_TO_I32:
  case FUNCTION_DP_TO_UI32:
    // [u]int32 dp_to_[u]int32
    //     (double xin, int rmode, int daz, int* pflags)
    argTypes.push_back(dpTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intPtrTy);

    retTy = intTy;
    break;

  case FUNCTION_I32_TO_DP:
  case FUNCTION_UI32_TO_DP:
    // double int32_to_dp (int32 in)
    // double uint32_to_dp (uint32 in)
    argTypes.push_back(intTy);

    retTy = dpTy;
    break;

  case FUNCTION_DP_TO_I64:
  case FUNCTION_DP_TO_UI64:
    // [u]int64 dp_to_[u]int64
    //     (double xin, int rmode, int daz, int* pflags)
    argTypes.push_back(dpTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intPtrTy);

    retTy = int64Ty;
    break;

  case FUNCTION_I64_TO_DP:
  case FUNCTION_UI64_TO_DP:
    // double int64_to_dp (int64 in, int rmode, int* pflags)
    // double uint64_to_dp (uint64 in, int rmode, int* pflags)
    argTypes.push_back(int64Ty);
    argTypes.push_back(intTy);
    argTypes.push_back(intPtrTy);

    retTy = dpTy;
    break;

  case FUNCTION_DP_TO_SP:
    // float dp_to_sp(double xin, int rmode, int ftz, int daz, int* pflags)
    argTypes.push_back(dpTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intPtrTy);

    retTy = spTy;
    break;

  case FUNCTION_SP_TO_DP:
    // double sp_to_dp (float xin, int daz, int* pflags)
    argTypes.push_back(spTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intPtrTy);

    retTy = dpTy;
    break;

  case FUNCTION_DP_SQRT:
    // double dp_sqrt(double xin, int rmode, int ftz, int daz, int* pflags)
    argTypes.push_back(dpTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intPtrTy);

    retTy = dpTy;
    break;

  case FUNCTION_DP_SQRT_NOMADM_IEEE:
  case FUNCTION_DP_SQRT_NOMADM_FAST:
    // double dp_sqrt_nomadm(double xin)
    argTypes.push_back(dpTy);
    retTy = dpTy;
    break;

  case FUNCTION_SP_DIV:
    argTypes.push_back(spTy);
    argTypes.push_back(spTy);
    argTypes.push_back(intTy);
    argTypes.push_back(intTy);
    retTy = spTy;
    break;

  default:
    IGC_ASSERT_UNREACHABLE(); // Undefined FunctionIDs
  }

  FunctionType *funcType = FunctionType::get(retTy, argTypes, false);
  newFunc = Function::Create(funcType, GlobalValue::ExternalLinkage, funcName, m_pModule);

  // keep track of what libraries will be imported.
  m_libModuleToBeImported[finfo.LibModID] = true;
  return newFunc;
}

// Alloca an int and return address Value to that.
Value *PreCompiledFuncImport::createFlagValue(Function *F) {
  // Note: we ignore error condition. For now, just create
  //       one for each function.
  // [todo] remove emulation functions's flag completely.
  auto II = m_DPEmuFlagTemp.find(F);
  if (II == m_DPEmuFlagTemp.end()) {
    LLVMContext &Ctx = F->getContext();
    BasicBlock *EntryBB = &(F->getEntryBlock());
    Instruction *insert_before = &(*EntryBB->getFirstInsertionPt());
    Type *intTy = Type::getInt32Ty(Ctx);
    Value *flagPtrValue = new AllocaInst(intTy, 0, "DPEmuFlag", insert_before);
    m_DPEmuFlagTemp[F] = flagPtrValue;
  }
  return m_DPEmuFlagTemp[F];
}

void PreCompiledFuncImport::visitFPExtInst(llvm::FPExtInst &I) {
  if ((isDPEmu() || isDPConvEmu()) && I.getDestTy()->isDoubleTy() &&
      (I.getSrcTy()->isFloatTy() || I.getSrcTy()->isHalfTy())) {
    Function *newFunc = getOrCreateFunction(FUNCTION_SP_TO_DP);
    Type *intTy = Type::getInt32Ty(m_pModule->getContext());
    Function *CurrFunc = I.getParent()->getParent();
    Value *args[3];
    if (I.getSrcTy()->isHalfTy()) {
      Instruction *newI =
          new FPExtInst(I.getOperand(0), Type::getFloatTy(m_pModule->getContext()), "DPEmufp16tofp32", &I);
      newI->setDebugLoc(I.getDebugLoc());
      args[0] = newI;
    } else {
      args[0] = I.getOperand(0);
    }
    args[1] = ConstantInt::get(intTy, m_flushDenorm); // flush denorm
    args[2] = createFlagValue(CurrFunc);              // FP flags, ignored
    CallInst *funcCall = CallInst::Create(newFunc, args, I.getName(), &I);
    addCallInst(funcCall);
    funcCall->setDebugLoc(I.getDebugLoc());

    I.replaceAllUsesWith(funcCall);
    I.eraseFromParent();
    m_changed = true;
  }
}

void PreCompiledFuncImport::visitCastInst(llvm::CastInst &I) {
  if (!isDPEmu() && !isDPConvEmu()) {
    return;
  }

  // Do not expect vector type for emulation yet.
  if (I.getType()->isVectorTy() || I.getOperand(0)->getType()->isVectorTy()) {
    return;
  }

  FunctionIDs FID;
  Type *intTy = Type::getInt32Ty(m_pModule->getContext());
  Type *dstTy = I.getType();
  Type *srcTy = I.getOperand(0)->getType();
  uint32_t opc = I.getOpcode();
  uint32_t intBits = 0;
  Value *oprd0 = I.getOperand(0);
  switch (opc) {
  case Instruction::FPToSI:
  case Instruction::FPToUI:
    intBits = dstTy->getIntegerBitWidth();
    if (!srcTy->isDoubleTy()) {
      return;
    }

    if (intBits > 32) {
      FID = ((opc == Instruction::FPToSI) ? FUNCTION_DP_TO_I64 : FUNCTION_DP_TO_UI64);
    } else {
      FID = ((opc == Instruction::FPToSI) ? FUNCTION_DP_TO_I32 : FUNCTION_DP_TO_UI32);
    }
    break;

  case Instruction::SIToFP:
  case Instruction::UIToFP:
    intBits = srcTy->getIntegerBitWidth();
    if (!dstTy->isDoubleTy()) {
      return;
    }
    if (intBits != 32 && intBits != 64) {
      // If not 32/64 int, up-cvt to 32/64 first.
      Instruction *newInst;
      Type *int64Ty = Type::getInt64Ty(m_pModule->getContext());
      Type *dTy = intBits < 32 ? intTy : int64Ty;
      if (opc == Instruction::SIToFP) {
        newInst = new SExtInst(oprd0, dTy, "DPEmusext", &I);
      } else {
        newInst = new ZExtInst(oprd0, dTy, "DPEmuzext", &I);
      }
      newInst->setDebugLoc(I.getDebugLoc());
      oprd0 = newInst;
    }

    if (intBits > 32) {
      FID = ((opc == Instruction::SIToFP) ? FUNCTION_I64_TO_DP : FUNCTION_UI64_TO_DP);
    } else {
      FID = ((opc == Instruction::SIToFP) ? FUNCTION_I32_TO_DP : FUNCTION_UI32_TO_DP);
    }
    break;

  default:
    return;
  }

  Function *newFunc = getOrCreateFunction(FID);
  Function *CurrFunc = I.getParent()->getParent();
  SmallVector<Value *, 4> args;
  args.push_back(oprd0);
  if (opc == Instruction::FPToSI || opc == Instruction::FPToUI) {
    args.push_back(ConstantInt::get(intTy, 3));             // rounding mode RZ
    args.push_back(ConstantInt::get(intTy, m_flushDenorm)); // flush denorm
    args.push_back(createFlagValue(CurrFunc));              // FP flags, ignored
  } else if ((opc == Instruction::SIToFP || opc == Instruction::UIToFP) && intBits > 32) {
    // [u]int64_to_dp ([S|U]INT64 ix, int rmode, int* pflags)
    args.push_back(ConstantInt::get(intTy, 0)); // rounding mode RN
    args.push_back(createFlagValue(CurrFunc));  // FP flags, ignored
  }

  CallInst *funcCall = CallInst::Create(newFunc, args, I.getName(), &I);
  addCallInst(funcCall);
  funcCall->setDebugLoc(I.getDebugLoc());

  Instruction *newVal = funcCall;
  if ((intBits != 32 && intBits != 64) && (opc == Instruction::FPToSI || opc == Instruction::FPToUI)) {
    // If not 32/64, down-cvt from 32/64 int.
    newVal = new TruncInst(newVal, I.getType(), "DPEmuTrunc", &I);
    newVal->setDebugLoc(I.getDebugLoc());
  }

  I.replaceAllUsesWith(newVal);
  I.eraseFromParent();
  m_changed = true;
  return;
}

uint32_t PreCompiledFuncImport::getFCmpMask(CmpInst::Predicate Pred) {
  // Return value from dp_cmp
  enum {
    DP_EMU_CMP_EQ = 0,   // bit 0 in mask
    DP_EMU_CMP_LT = 1,   // bit 1 in mask
    DP_EMU_CMP_GT = 2,   // bit 2 in mask
    DP_EMU_CMP_UNORD = 3 // bit 3 in mask
  };
#define SETMASKBIT(n) (0x1 << (n))

  uint32_t mask = 0;
  switch (Pred) {
  case CmpInst::FCMP_OEQ:
  case CmpInst::FCMP_UEQ:
    mask = SETMASKBIT(DP_EMU_CMP_EQ);
    break;
  case CmpInst::FCMP_OLT:
  case CmpInst::FCMP_ULT:
    mask = SETMASKBIT(DP_EMU_CMP_LT);
    break;

  case CmpInst::FCMP_OLE:
  case CmpInst::FCMP_ULE:
    mask = (SETMASKBIT(DP_EMU_CMP_EQ) | SETMASKBIT(DP_EMU_CMP_LT));
    break;

  case CmpInst::FCMP_OGT:
  case CmpInst::FCMP_UGT:
    mask = SETMASKBIT(DP_EMU_CMP_GT);
    break;

  case CmpInst::FCMP_OGE:
  case CmpInst::FCMP_UGE:
    mask = (SETMASKBIT(DP_EMU_CMP_EQ) | SETMASKBIT(DP_EMU_CMP_GT));
    break;

  case CmpInst::FCMP_ONE:
  case CmpInst::FCMP_UNE:
    mask = (SETMASKBIT(DP_EMU_CMP_LT) | SETMASKBIT(DP_EMU_CMP_GT));
    break;

  case CmpInst::FCMP_ORD:
    mask |= (SETMASKBIT(DP_EMU_CMP_EQ) | SETMASKBIT(DP_EMU_CMP_GT) | SETMASKBIT(DP_EMU_CMP_LT));
    break;

  case CmpInst::FCMP_UNO:
    //  All Unordered is set later.
    break;

  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Wrong fcmp flag");
  }

  switch (Pred) {
  case CmpInst::FCMP_UEQ:
  case CmpInst::FCMP_ULT:
  case CmpInst::FCMP_ULE:
  case CmpInst::FCMP_UGT:
  case CmpInst::FCMP_UGE:
  case CmpInst::FCMP_UNE:
  case CmpInst::FCMP_UNO:
    mask |= SETMASKBIT(DP_EMU_CMP_UNORD);
    break;
  default:
    break;
  }
  return mask;
}

void PreCompiledFuncImport::visitFCmpInst(FCmpInst &I) {
  if ((!isDPEmu() && !isDPConvEmu()) || !I.getOperand(0)->getType()->isDoubleTy()) {
    return;
  }

  CmpInst::Predicate pred = I.getPredicate();
  if (pred == CmpInst::FCMP_FALSE || pred == CmpInst::FCMP_TRUE) {
    return;
  }

  Function *newFunc = getOrCreateFunction(FUNCTION_DP_CMP);
  Type *intTy = Type::getInt32Ty(m_pModule->getContext());
  Value *args[3];
  args[0] = I.getOperand(0);
  args[1] = I.getOperand(1);
  args[2] = ConstantInt::get(intTy, m_flushDenorm); // flush denorm.

  CallInst *funcCall = CallInst::Create(newFunc, args, I.getName(), &I);
  funcCall->setDebugLoc(I.getDebugLoc());
  addCallInst(funcCall);

  //
  // 'mask' indicates that if any of bits is set, the condition is true.
  // 'bitVal' is the bit that is set by this emulation function, which is
  // calculated by  1 << funcCall.
  //   Given the fcmp
  //       i1 cond = fcmp (Pred) x,  y
  //   it is translated into
  //       c1 = dp_cmp(x, y, 1)
  //       c2 = 1 << c1;
  //       if ( (c2 & mask(Pred)) != 0) cond = 1 else cond = 0;
  //
  uint32_t mask = getFCmpMask(pred);
  Constant *maskVal = ConstantInt::get(intTy, mask);
  Constant *COne = ConstantInt::get(intTy, 1);
  Constant *CZero = ConstantInt::get(intTy, 0);
  Instruction *bitVal = BinaryOperator::CreateShl(COne, funcCall, "", &I);
  Instruction *anyBitSet = BinaryOperator::CreateAnd(maskVal, bitVal, "", &I);
  Instruction *intcmp = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_NE, anyBitSet, CZero, "DPEmuCmp", &I);

  I.replaceAllUsesWith(intcmp);
  I.eraseFromParent();
  m_changed = true;
}

void PreCompiledFuncImport::visitCallInst(llvm::CallInst &I) {
  if (IGC_IS_FLAG_ENABLED(EnableTestIGCBuiltin)) {
    // This is to test if an emulated function is the same
    // as the hardware instruction. It requires the platform
    // that has hw instructions, such ask SKL etc.
    // Currently, it is available for OCL only.  For example,
    // the following ocl kernel will test if double-to-int32
    // is the same as its emulated function.
    //
    // extern int __igcbuildin_dp_to_int32(double, int, int, int*);
    //
    // void kernel test_db2si32(global int* dst, global double *src)
    // {
    //    int flag = 0;
    //    int ix = get_global_id(0);
    //    double din = src[ix];
    //    int hwRes = (int)din;
    //    int emuRes = __igcbuildin_dp_to_int32(din, 0, 0, &flag);
    //    int r0 = 0, r1 = 0;
    //    if (hwRes != emuRes)
    //    {
    //        r0 = hwRes;
    //        r1 = emuRes;
    //    }
    //    dst[2 * ix] = r0;
    //    dst[2 * ix + 1] = r1;
    // }
    //
    // With the host application invoking this kernel, it will check if the emulated
    // function matches the hardware instruction.
    //
    // Note that when using this functionality, ForceDPEmulation must be 0 AND it must
    // be on the platform that supports the hardware instrutions for those emulated
    // operations.
    //
    Function *func = I.getCalledFunction();
    if (func) {
      StringRef fName = func->getName();
      for (int FID = 0; FID < NUM_FUNCTION_IDS; ++FID) {
        const PreCompiledFuncInfo &finfo = m_functionInfos[FID];
        if (fName.equals(finfo.FuncName)) {
          m_libModuleToBeImported[finfo.LibModID] = true;
          m_changed = true;

          // Make sure to keep this call so that its calling convention
          // will be adjusted later.
          addCallInst(&I);
          break;
        }
      }
    }
  }

  Type *resTy = I.getType();
  Type *intTy = Type::getInt32Ty(m_pModule->getContext());
  IntrinsicInst *II = dyn_cast<IntrinsicInst>(&I);
  GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(&I);
  if (isSPDiv() && resTy->isFloatTy() && GII && GII->getIntrinsicID() == GenISAIntrinsic::GenISA_IEEE_Divide) {
    Function *newFunc = getOrCreateFunction(FUNCTION_SP_DIV);
    Value *args[4];
    args[0] = I.getOperand(0);
    args[1] = I.getOperand(1);

    auto pMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    int ftz = (pMD->compOpt.FloatDenormMode32 == FLOAT_DENORM_FLUSH_TO_ZERO) ? 1 : 0;
    int daz = (pMD->compOpt.DenormsAreZero) ? 1 : 0;
    args[2] = ConstantInt::get(Type::getInt32Ty(I.getContext()), ftz);
    args[3] = ConstantInt::get(Type::getInt32Ty(I.getContext()), daz);

    CallInst *newVal = CallInst::Create(newFunc, args, I.getName(), &I);
    addCallInst(newVal);
    newVal->setDebugLoc(I.getDebugLoc());

    I.replaceAllUsesWith(newVal);
    I.eraseFromParent();

    m_changed = true;
    return;
  }

  /*In TGLLP (or any platform that does not support int "/" and "%"),
  we have to emulate both integer divide and integer modulo.
  For that we use emulation routines that do both operations in the same time:
  OCL:
    int c = a/b;
    int d = a%b;
  LLVM before opt:
    %Remainder3 = alloca i32
    %Remainder = alloca i32
    %div2 = call i32 @precompiled_s32divrem_sp(i32 %a, i32 %b, i32* %Remainder)
    store i32 %div2, i32 addrspace(1)* %c, align 4
    %rem4 = call i32 @precompiled_s32divrem_sp(i32 %a, i32 %b, i32* %Remainder3)
    %rem5 = load i32, i32* %Remainder3
    store i32 %rem5, i32 addrspace(1)* %d, align 4
    ret void
  LLVM after opt:
    %Remainder = alloca i32
    %div2 = call i32 @precompiled_s32divrem_sp(i32 %a, i32 %b, i32* %Remainder)
    store i32 %div2, i32 addrspace(1)* %c, align 4
    %rem5 = load i32, i32* %Remainder
    store i32 %rem5, i32 addrspace(1)* %d, align 4
    ret void

  As a result, we reduce 2x necessary work
  */
  Function *fn = I.getCalledFunction();
  if (fn && (fn->getName() == "precompiled_s32divrem_sp" || fn->getName() == "precompiled_u32divrem_sp")) {
    for (CallInst *InstCompare : m_CallRemDiv) {
      if (I.getArgOperand(0) == InstCompare->getArgOperand(0) && I.getArgOperand(1) == InstCompare->getArgOperand(1) &&
          I.getParent() == InstCompare->getParent()) {
        Value *remValue = I.getArgOperand(2);
        Value *remValueCompare = InstCompare->getArgOperand(2);
        remValue->replaceAllUsesWith(remValueCompare);

        if (auto *AI = dyn_cast<AllocaInst>(remValue))
          AI->eraseFromParent();
        InstCompare->setDebugLoc(I.getDebugLoc());

        eraseCallInst(&I);
        I.replaceAllUsesWith(InstCompare);
        I.eraseFromParent();
        m_changed = true;
        return;
      }
    }
    m_CallRemDiv.push_back(&I);
  }

  if ((isDPEmu() || isDPDivSqrtEmu()) && resTy->isDoubleTy() && (II && II->getIntrinsicID() == Intrinsic::sqrt)) {
    FunctionIDs sqrtType = FUNCTION_DP_SQRT;
    if (!isDPEmu() && isDPDivSqrtEmu()) {
      // Partial hardware DP support available, can use faster implementation.
      sqrtType = I.getFastMathFlags().approxFunc() ? FUNCTION_DP_SQRT_NOMADM_FAST : FUNCTION_DP_SQRT_NOMADM_IEEE;
    }

    Function *newFunc = getOrCreateFunction(sqrtType);
    SmallVector<Value *, 5> args;
    args.push_back(I.getOperand(0));

    if (sqrtType == FUNCTION_DP_SQRT) {
      Function *CurrFunc = I.getParent()->getParent();
      args.push_back(ConstantInt::get(intTy, m_roundingMode)); // rounding mode
      args.push_back(ConstantInt::get(intTy, m_flushToZero));  // flush to zero
      args.push_back(ConstantInt::get(intTy, m_flushDenorm));  // flush denorm
      args.push_back(createFlagValue(CurrFunc));               // FP Flag, ignored
    }

    CallInst *newVal = CallInst::Create(newFunc, args, I.getName(), &I);

    addCallInst(newVal);
    newVal->setDebugLoc(I.getDebugLoc());

    I.replaceAllUsesWith(newVal);
    I.eraseFromParent();
    m_changed = true;
    return;
  }

  // llvm.fma.f64
  if (isDPEmu() && resTy->isDoubleTy() && II && II->getIntrinsicID() == Intrinsic::fma) {
    Function *newFunc = getOrCreateFunction(FUNCTION_DP_FMA);
    Function *CurrFunc = I.getParent()->getParent();
    Value *args[7];
    args[0] = I.getOperand(0);
    args[1] = I.getOperand(1);
    args[2] = I.getOperand(2);
    args[3] = ConstantInt::get(intTy, m_roundingMode); // rounding mode
    args[4] = ConstantInt::get(intTy, m_flushToZero);  // flush to zero
    args[5] = ConstantInt::get(intTy, m_flushDenorm);  // flush denorm
    args[6] = createFlagValue(CurrFunc);               // FP Flag, ignored

    CallInst *newVal = CallInst::Create(newFunc, args, I.getName(), &I);
    addCallInst(newVal);
    newVal->setDebugLoc(I.getDebugLoc());

    I.replaceAllUsesWith(newVal);
    I.eraseFromParent();
    m_changed = true;
    return;
  }

  // llvm.fma.rtn.f64
  // llvm.fma.rtp.f64
  // llvm.fma.rtz.f64
  if (isDPEmu() && resTy->isDoubleTy() && GII &&
      (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_fma_rtn ||
       GII->getIntrinsicID() == GenISAIntrinsic::GenISA_fma_rtp ||
       GII->getIntrinsicID() == GenISAIntrinsic::GenISA_fma_rtz)) {
    Function *newFunc = getOrCreateFunction(FUNCTION_DP_FMA);
    Function *CurrFunc = I.getParent()->getParent();

    // Recognize which rounding mode is used, by default is round to nearest
    unsigned roundingMode = EmuRoundingMode::ROUND_TO_NEAREST_EVEN;
    if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_fma_rtn) {
      roundingMode = EmuRoundingMode::ROUND_TO_NEGATIVE;
    } else if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_fma_rtp) {
      roundingMode = EmuRoundingMode::ROUND_TO_POSITIVE;
    } else if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_fma_rtz) {
      roundingMode = EmuRoundingMode::ROUND_TO_ZERO;
    }

    Value *args[7];
    args[0] = I.getOperand(0);
    args[1] = I.getOperand(1);
    args[2] = I.getOperand(2);
    args[3] = ConstantInt::get(intTy, roundingMode);  // rounding mode
    args[4] = ConstantInt::get(intTy, m_flushToZero); // flush to zero
    args[5] = ConstantInt::get(intTy, m_flushDenorm); // flush denorm
    args[6] = createFlagValue(CurrFunc);              // FP Flag, ignored

    CallInst *newVal = CallInst::Create(newFunc, args, I.getName(), &I);
    addCallInst(newVal);
    newVal->setDebugLoc(I.getDebugLoc());

    I.replaceAllUsesWith(newVal);
    I.eraseFromParent();
    m_changed = true;
    return;
  }

  if ((isRTEFP64toFP16() || isDPEmu() || isDPConvEmu()) && GII &&
      (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_ftof_rte ||
       GII->getIntrinsicID() == GenISAIntrinsic::GenISA_ftof_rtz ||
       GII->getIntrinsicID() == GenISAIntrinsic::GenISA_ftof_rtn ||
       GII->getIntrinsicID() == GenISAIntrinsic::GenISA_ftof_rtp) &&
      resTy->isHalfTy() && GII->getOperand(0)->getType()->isDoubleTy()) {
    if (isRTEFP64toFP16() && !isDPEmu() && !isDPConvEmu() &&
        GII->getIntrinsicID() != GenISAIntrinsic::GenISA_ftof_rte) {
      // It is safe to use a sequence of conversions for RTZ, RTN and RTP (fp64 -> fp32 -> fp16)
      return;
    }
    const StringRef funcName =
        GII->getIntrinsicID() == GenISAIntrinsic::GenISA_ftof_rte   ? "__precompiled_convert_f64_to_f16_rte"
        : GII->getIntrinsicID() == GenISAIntrinsic::GenISA_ftof_rtz ? "__precompiled_convert_f64_to_f16_rtz"
        : GII->getIntrinsicID() == GenISAIntrinsic::GenISA_ftof_rtn ? "__precompiled_convert_f64_to_f16_rtn"
                                                                    : "__precompiled_convert_f64_to_f16_rtp";
    Function *func = m_pModule->getFunction(funcName);

    // Try to look up the function in the module's symbol
    // table first, else add it.
    if (func == NULL) {
      FunctionType *FuncIntrType = FunctionType::get(resTy, I.getOperand(0)->getType(), false);

      func = Function::Create(FuncIntrType, GlobalValue::ExternalLinkage, funcName, m_pModule);
    }

    CallInst *funcCall = CallInst::Create(func, GII->getOperand(0));
    ReplaceInstWithInst(GII, funcCall);
    addCallInst(funcCall);

    m_libModuleToBeImported[LIBMOD_INT_DIV_REM] = true;
    m_changed = true;
    return;
  }

  // llvm.fabs.f64
  if (isDPEmu() && resTy->isDoubleTy() && II && II->getIntrinsicID() == Intrinsic::fabs) {
    // bit 63 is sign bit, set it to zero. Don't use int64.
    VectorType *vec2Ty = IGCLLVM::FixedVectorType::get(intTy, 2);
    Instruction *twoI32 = CastInst::Create(Instruction::BitCast, I.getOperand(0), vec2Ty, "", &I);
    twoI32->setDebugLoc(I.getDebugLoc());
    Instruction *topI32 = ExtractElementInst::Create(twoI32, ConstantInt::get(intTy, 1), "", &I);
    topI32->setDebugLoc(I.getDebugLoc());
    Instruction *newTopI32 = BinaryOperator::CreateAnd(topI32, ConstantInt::get(intTy, 0x7fffffff), "", &I);
    newTopI32->setDebugLoc(I.getDebugLoc());
    Instruction *val = InsertElementInst::Create(twoI32, newTopI32, ConstantInt::get(intTy, 1), "", &I);
    val->setDebugLoc(I.getDebugLoc());
    Instruction *fabsVal = CastInst::Create(Instruction::BitCast, val, resTy, "DPEmuFabs", &I);
    fabsVal->setDebugLoc(I.getDebugLoc());

    I.replaceAllUsesWith(fabsVal);
    I.eraseFromParent();
    m_changed = true;
    return;
  }
  return;
}

bool PreCompiledFuncImport::usePrivateMemory(Function *F) {
  // Assume alloca is in entry BB
  for (const Instruction &I : F->getEntryBlock()) {
    if (isa<AllocaInst>(&I))
      return true;
  }
  return false;
}

void PreCompiledFuncImport::addMDFuncEntryForEmulationFunc(Function *F) {
  FunctionInfoMetaDataHandle FH = FunctionInfoMetaDataHandle(new FunctionInfoMetaData());
  FH->setType(FunctionTypeMD::UserFunction);
  for (auto arg = F->arg_begin(); arg != F->arg_end(); ++arg) {
    ArgInfoMetaDataHandle AH = ArgInfoMetaDataHandle(new ArgInfoMetaData());
    AH->setExplicitArgNum(arg->getArgNo());
    FH->addArgInfoListItem(AH);
  }
  m_pMdUtils->setFunctionsInfoItem(F, FH);

  if (!usePrivateMemory(F) || AddImplicitArgs::hasStackCallInCG(F))
    return;

  // Add private base
  ArgInfoMetaDataHandle arg_R0 = ArgInfoMetaDataHandle(new ArgInfoMetaData());
  arg_R0->setArgId(ImplicitArg::R0);
  FH->addImplicitArgInfoListItem(arg_R0);
  ArgInfoMetaDataHandle arg_PBase = ArgInfoMetaDataHandle(new ArgInfoMetaData());
  arg_PBase->setArgId(ImplicitArg::PRIVATE_BASE);
  FH->addImplicitArgInfoListItem(arg_PBase);

  FuncNeedIA.push_back(F);
}

FunctionType *PreCompiledFuncImport::getNewFuncType(Function *pFunc, const ImplicitArgs *pImplicitArgs) {
  // Add all explicit parameters
  FunctionType *pFuncType = pFunc->getFunctionType();
  std::vector<Type *> newParamTypes(pFuncType->param_begin(), pFuncType->param_end());

  // Add implicit arguments parameter types
  for (unsigned int i = 0; i < pImplicitArgs->size(); ++i) {
    newParamTypes.push_back((*pImplicitArgs)[i].getLLVMType(pFunc->getContext()));
  }

  // Create new function type with explicit and implicit parameter types
  return FunctionType::get(pFunc->getReturnType(), newParamTypes, pFunc->isVarArg());
}

// Once the new function is created, the old function's metadata is
// re-assigned to the new one. And thus, the old function has no
// metadata entry anymore.
void PreCompiledFuncImport::createFuncWithIA() {
  NewFuncWithIA.clear();
  for (int i = 0, sz = (int)FuncNeedIA.size(); i < sz; ++i) {
    Function *pFunc = FuncNeedIA[i];
    ImplicitArgs implicitArgs(*pFunc, m_pMdUtils);
    FunctionType *pNewFTy = getNewFuncType(pFunc, &implicitArgs);
    Function *pNewFunc = Function::Create(pNewFTy, pFunc->getLinkage());
    pNewFunc->copyAttributesFrom(pFunc);
    pNewFunc->setSubprogram(pFunc->getSubprogram());
    pFunc->setSubprogram(nullptr);
    m_pModule->getFunctionList().insert(pFunc->getIterator(), pNewFunc);
    pNewFunc->takeName(pFunc);

    // Since we have now created the new function, splice the body of the old
    // function right into the new function, leaving the old body of the function empty.
    IGCLLVM::splice(pNewFunc, pNewFunc->begin(), pFunc);

    // Loop over the argument list, transferring uses of the old arguments over to
    // the new arguments
    // updateNewFuncArgs(pFunc, pNewFunc, &implicitArgs);
    Function::arg_iterator currArg = pNewFunc->arg_begin();
    for (Function::arg_iterator I = pFunc->arg_begin(), E = pFunc->arg_end(); I != E; ++I, ++currArg) {
      llvm::Value *newArg = &(*currArg);
      // Move the name and users over to the new version.
      I->replaceAllUsesWith(newArg);
      currArg->takeName(&(*I));
    }

    // Set name for implicit args
    int iArgIx = 0;
    for (Function::arg_iterator AE = pNewFunc->arg_end(); currArg != AE; ++currArg, ++iArgIx) {
      currArg->setName(implicitArgs[iArgIx].getName());
    }

    // Assign metadata for old_func to new_func
    IGCMD::IGCMetaDataHelper::moveFunction(*m_pMdUtils, *m_pCtx->getModuleMetaData(), pFunc, pNewFunc);

    // Map old func to new func
    NewFuncWithIA.push_back(pNewFunc);
  }
}

void PreCompiledFuncImport::replaceFunc(Function *old_func, Function *new_func) {
  FunctionInfoMetaDataHandle newFH = m_pMdUtils->getFunctionsInfoItem(new_func);
  std::vector<Instruction *> list_delete;
  for (auto U = old_func->user_begin(), UE = old_func->user_end(); U != UE; ++U) {
    std::vector<Value *> new_args;
    CallInst *cInst = dyn_cast<CallInst>(*U);
    if (!cInst) {
      m_pCtx->EmitError(" undefined reference to `jmp()' ", *U);
      return;
    }
    Function *parent_func = cInst->getParent()->getParent();
    size_t numArgOperands = IGCLLVM::getNumArgOperands(cInst);

    // let's prepare argument list on new call function
    llvm::Function::arg_iterator new_arg_iter = new_func->arg_begin();
    llvm::Function::arg_iterator new_arg_end = new_func->arg_end();

    IGC_ASSERT(new_func->arg_size() >= numArgOperands);

    // basic arguments
    for (unsigned int i = 0; i < numArgOperands; ++i, ++new_arg_iter) {
      llvm::Value *arg = cInst->getOperand(i);
      new_args.push_back(arg);
    }

    // implicit arguments
    ImplicitArgs *parentIA = getImplicitArgs(parent_func);
    int cImpCount = 0;
    while (new_arg_iter != new_arg_end) {
      ArgInfoMetaDataHandle argInfo = newFH->getImplicitArgInfoListItem(cImpCount);
      ImplicitArg::ArgType argId = (ImplicitArg::ArgType)argInfo->getArgId();
      Argument *iArgVal = parentIA->getImplicitArg(*parent_func, argId);

      new_args.push_back(iArgVal);
      ++new_arg_iter;
      ++cImpCount;
    }

    // insert new call instruction before old one
    CallInst *inst;
    if (new_func->getReturnType()->isVoidTy()) {
      inst = CallInst::Create(new_func, new_args, "", cInst);
    } else {
      inst = CallInst::Create(new_func, new_args, new_func->getName(), cInst);
    }
    inst->setCallingConv(new_func->getCallingConv());
    inst->setDebugLoc(cInst->getDebugLoc());
    cInst->replaceAllUsesWith(inst);
    list_delete.push_back(cInst);
  }

  for (auto i : list_delete) {
    i->eraseFromParent();
  }

  IGC_ASSERT_MESSAGE(old_func->use_empty(), "old_func should have no use at this point!");
  // Now, after changing funciton signature,
  // and validate there are no calls to the old function we can erase it.
  //
  // Note that old_func's metadata has been deleted already
  old_func->eraseFromParent();
}

ImplicitArgs *PreCompiledFuncImport::getImplicitArgs(Function *F) {
  if (FuncsImpArgs.count(F) == 0) {
    ImplicitArgs *IA = new ImplicitArgs(*F, m_pMdUtils);
    FuncsImpArgs[F] = IA;
  }
  return FuncsImpArgs[F];
}

void PreCompiledFuncImport::checkAndSetEnableSubroutine() {
  // Skip if subroutine/stackcall is already on.
  if (m_pCtx->m_enableSubroutine) {
    m_enableCallForEmulation = true;
    return;
  }

  int emuFC = (int)IGC_GET_FLAG_VALUE(EmulationFunctionControl);
  if (emuFC == FLAG_FCALL_FORCE_SUBROUTINE || emuFC == FLAG_FCALL_FORCE_STACKCALL) {
    m_enableCallForEmulation = true;
    m_pCtx->m_enableSubroutine = true;
    return;
  }


  bool SPDiv = isSPDiv();
  bool DPEmu = isDPEmu();
  bool DPDivSqrtEmu = isDPDivSqrtEmu();
  bool DPConvEmu = isDPConvEmu();
  bool I64DivRem = isI64DivRem();

  Module *M = m_pCtx->getModule();
  for (auto FI = M->begin(), FE = M->end(); FI != FE; ++FI) {
    Function *F = &*FI;
    for (auto II = inst_begin(F), IE = inst_end(F); II != IE; ++II) {
      Instruction *I = &*II;
      switch (I->getOpcode()) {
      default:
        break;
      case Instruction::FDiv:
        if (DPDivSqrtEmu && I->getOperand(0)->getType()->isDoubleTy()) {
          m_enableCallForEmulation = true;
          m_pCtx->m_hasDPDivSqrtEmu = true;
        }
        break;
      case Instruction::FMul:
      case Instruction::FAdd:
      case Instruction::FSub:
        if (DPEmu && I->getOperand(0)->getType()->isDoubleTy()) {
          m_enableCallForEmulation = true;
        }
        break;
      case Instruction::FCmp:
      case Instruction::FPToSI:
      case Instruction::FPToUI:
      case Instruction::FPTrunc:
        if ((DPEmu || DPConvEmu) && I->getOperand(0)->getType()->isDoubleTy()) {
          m_enableCallForEmulation = true;
        }
        break;
      case Instruction::SIToFP:
      case Instruction::UIToFP:
      case Instruction::FPExt:
        if ((DPEmu || DPConvEmu) && I->getType()->isDoubleTy()) {
          m_enableCallForEmulation = true;
        }
        break;
      case Instruction::UDiv:
      case Instruction::URem:
      case Instruction::SDiv:
      case Instruction::SRem:
        if (I64DivRem && I->getOperand(0)->getType()->isIntegerTy(64)) {
          m_enableCallForEmulation = true;
        }
        break;
      }

      GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(I);
      IntrinsicInst *IInst = dyn_cast<IntrinsicInst>(I);
      // Handle intrinsic calls
      if ((DPEmu || DPDivSqrtEmu) && I->getType()->isDoubleTy() && IInst &&
          IInst->getIntrinsicID() == Intrinsic::sqrt) {
        m_enableCallForEmulation = true;
        m_pCtx->m_hasDPDivSqrtEmu = true;
      } else if (DPEmu && I->getType()->isDoubleTy() && IInst && IInst->getIntrinsicID() == Intrinsic::fma) {
        m_enableCallForEmulation = true;
      } else if (SPDiv && I->getType()->isFloatTy() && GII &&
                 GII->getIntrinsicID() == GenISAIntrinsic::GenISA_IEEE_Divide) {
        m_enableCallForEmulation = true;
      }

      if (m_enableCallForEmulation) {
        m_pCtx->m_enableSubroutine = true;
        return;
      }
    }
  }
}
