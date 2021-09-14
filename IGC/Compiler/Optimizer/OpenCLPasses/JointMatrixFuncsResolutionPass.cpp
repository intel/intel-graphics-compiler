/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "JointMatrixFuncsResolutionPass.h"

#include "IGC/common/StringMacros.hpp"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Module.h"
#include "common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

char JointMatrixFuncsResolutionPass::ID = 0;

JointMatrixFuncsResolutionPass::JointMatrixFuncsResolutionPass(OpenCLProgramContext *Context) : FunctionPass(ID)
{
    this->Context = Context;
}

bool JointMatrixFuncsResolutionPass::runOnFunction(Function& F)
{
    ResolvedValues.clear();
    InstsToErase.clear();
    Changed = false;

    visit(F);

    for (Instruction *I : InstsToErase) {
        Value *undef = UndefValue::get(I->getType());
        I->replaceAllUsesWith(undef);
        I->eraseFromParent();
    }
    
    return Changed;
}

static const char *JointMatrixLoadPrefx  = "__builtin_spirv_OpMatrixLoadINTEL";
static const char *JointMatrixStorePrefx = "__builtin_spirv_OpMatrixStoreINTEL";
static const char *JointMatrixMadPrefx   = "__builtin_spirv_OpMatrixMadINTEL";

enum {
    LayoutRowMajor,
    LayoutColumnMajor,
    LayoutPackedA,
    LayoutPackedB,
};

static unsigned getBitWidthFromFlags(unsigned flags) {
    return flags & 0x7fffffff;
}

static bool isElmentTypeInteger(unsigned flags) {
    return (flags & (1 << 31)) == 0;
}

static bool isElmentTypeFloating(unsigned flags) {
    return !isElmentTypeInteger(flags);
}

std::string JointMatrixFuncsResolutionPass::getMADMatrixFuncName
      (uint32_t aTypeFlags, uint32_t bTypeFlags, uint32_t cTypeFlags, unsigned M, unsigned N)
{
    std::string name = "__builtin_IB_sub_group";

    if (isElmentTypeInteger(cTypeFlags)) {
        name += "_idpas_";
        name += "s" + std::to_string(getBitWidthFromFlags(aTypeFlags)) + "_";
        name += "s" + std::to_string(getBitWidthFromFlags(bTypeFlags)) + "_";
    } else {
        name += "_fdpas_";
        /* bf is passed as uint16_t, hf is using halfs */
        name += isElmentTypeFloating(aTypeFlags) ? "hf_" : "bf_";
        name += isElmentTypeFloating(bTypeFlags) ? "hf_" : "bf_";
    }
    name += std::to_string(M) + "_";
    name += std::to_string(N);
    return name;
}

std::string JointMatrixFuncsResolutionPass::GetLoadStoreMatrixFuncName
        (bool load, unsigned loadLayout, unsigned matrixLayout,
         unsigned elemBitWidth, unsigned rows, unsigned cols)
{
    /* Treat row major matrices with types not supported by accumulators as
     * PackedA matrices. Both are in row major format. */
    if (load && matrixLayout == LayoutRowMajor && elemBitWidth <= 16) {
        matrixLayout = LayoutPackedA;
    }

    std::string name
      = load ? "__builtin_spriv_OpMatrixLoadINTEL_" : "__builtin_spriv_OpMatrixStoreINTEL_";
    switch (matrixLayout) {
      case LayoutPackedA:
        name += "PackedA_";
        break;
      case LayoutPackedB:
        name += "PackedB_";
        break;
      case LayoutRowMajor:
      case LayoutColumnMajor:
        name += "Accumulator_";
        break;
      default:
        IGC_ASSERT_MESSAGE(false, "Unexpected matrix layout.");
    }

    switch (loadLayout) {
      case LayoutRowMajor:
        name += "RowMajor_";
        break;
      case LayoutColumnMajor:
        name += "ColumnMajor_";
        break;
      case LayoutPackedB:
        IGC_ASSERT_MESSAGE(matrixLayout == loadLayout, "Unexpected load/store layout.");
        name += "PackedB_";
        break;
      default:
        IGC_ASSERT_MESSAGE(false, "Unexpected load/store layout.");
    }


    name += std::to_string(rows);
    name += "x";
    name += std::to_string(cols);
    name += "_";

    if (elemBitWidth == 8) {
        name += "i8_";
    } else if (elemBitWidth == 16) {
        name += "i16_";
    } else if (elemBitWidth == 32) {
        name += "i32_";
    } else {
        IGC_ASSERT_MESSAGE(false, "Unexpected matrix element size.");
    }

    if (load) {
        name += "v8i8_pi32_i32";
    } else {
        name += "pi64_v8i8";
    }
    return name;
}

Type *JointMatrixFuncsResolutionPass::ResolveType
          (const Type *opaqueType, uint32_t elementTypeFlags, unsigned rows,
           unsigned *outLayout)
{
    IGC_ASSERT_EXIT_MESSAGE(opaqueType && opaqueType->isPointerTy(),
        "Unexpected type in matrix function resolution.");

    const PointerType *ptrType = cast<PointerType>(opaqueType);
    StringRef name = ptrType->getElementType()->getStructName();

    LLVMContext &ctx = opaqueType->getContext();

    if (name.equals("intel.joint_matrix_packedA_t")) {
        *outLayout = LayoutPackedA;
        Type *baseType = Type::getInt32Ty(ctx);
        return IGCLLVM::FixedVectorType::get(baseType, rows);
    } else if (name.equals("intel.joint_matrix_packedB_t")) {
        *outLayout = LayoutPackedB;
        Type *baseType = Type::getInt32Ty(ctx);
        return IGCLLVM::FixedVectorType::get(baseType, 8);
    } else if (name.equals("intel.joint_matrix_acc_t")) {
        *outLayout = LayoutRowMajor;
        Type *baseType = Type::getInt32Ty(ctx);
        if (isElmentTypeFloating(elementTypeFlags)) {
            baseType = Type::getFloatTy(ctx);
        }
        return IGCLLVM::FixedVectorType::get(baseType, rows);
    }

    IGC_ASSERT_EXIT_MESSAGE(false, "Failed to resolve matrix type.");
    return nullptr;
}

static uint64_t constIntValue(const Value *v) {
    return cast<ConstantInt>(v)->getLimitedValue();
}

static Type *getIntegerEquivalent(Type *matTy) {
    /* Already an integer type: */
    if (matTy->isFPOrFPVectorTy() == false) {
        return matTy;
    }

    if (IGCLLVM::FixedVectorType *VT = cast<IGCLLVM::FixedVectorType>(matTy)) {
        unsigned elements = (unsigned) VT->getNumElements();
        unsigned size = VT->getElementType()->getScalarSizeInBits();
        Type *elementType = Type::getIntNTy(matTy->getContext(), size);
        return IGCLLVM::FixedVectorType::get(elementType, elements);
    } else {
        unsigned size = matTy->getScalarSizeInBits();
        return Type::getIntNTy(matTy->getContext(), size);
    }
}

Instruction *JointMatrixFuncsResolutionPass::ResolveLoad(CallInst *CI)
{
    Value *ptrVal        = CI->getArgOperand(0);
    Value *strideVal     = CI->getArgOperand(1);
    unsigned loadLayout  = (unsigned) constIntValue(CI->getArgOperand(2));
    uint32_t elementType = (uint32_t) constIntValue(CI->getArgOperand(3));
    unsigned rows        = (unsigned) constIntValue(CI->getArgOperand(4));
    unsigned columns     = (unsigned) constIntValue(CI->getArgOperand(5));

    unsigned matrixLayout = LayoutRowMajor;
    Type *matTy = ResolveType(CI->getType(), elementType, rows, &matrixLayout);
    /* Cast floating types to integer types of the same size. This allows to
     * have a single set of store builtins for floats and integer */
    Type *retTy = getIntegerEquivalent(matTy);

    Module *M = CI->getParent()->getModule();

    unsigned elemBitWidth = getBitWidthFromFlags(elementType);
    std::string funcName = GetLoadStoreMatrixFuncName(true, loadLayout, matrixLayout, elemBitWidth, rows, columns);
    FunctionType *funcType = FunctionType::get(retTy, { ptrVal->getType(), strideVal->getType() }, false);
    std::vector<Value *> Args = { ptrVal, strideVal };

    InstsToErase.insert(CI);

    Instruction *newCall = CallInst::Create(M->getOrInsertFunction(funcName, funcType), Args, "matrix", CI);
    if (retTy != matTy) {
        newCall = BitCastInst::Create(Instruction::BitCast, newCall, matTy,"matrix.load.cast", CI); 
    }
    return newCall;
}

Instruction *JointMatrixFuncsResolutionPass::ResolveStore(CallInst *CI)
{
    Value *ptrVal        = CI->getArgOperand(0);
    Value *matrixVal     = CI->getArgOperand(1);
    Value *strideVal     = CI->getArgOperand(2);
    unsigned storeLayout = (unsigned) constIntValue(CI->getArgOperand(3));
    uint32_t elementType = (uint32_t) constIntValue(CI->getArgOperand(4));
    unsigned rows        = (unsigned) constIntValue(CI->getArgOperand(5));
    unsigned columns     = (unsigned) constIntValue(CI->getArgOperand(6));

    unsigned matrixLayout = LayoutRowMajor;
    Type *matTy = ResolveType(matrixVal->getType(), elementType, rows, &matrixLayout);
    /* Cast floating types to integer types of the same size. This allows to
     * have a single set of store builtins for floats and integers */
    matTy = getIntegerEquivalent(matTy);

    Module *M = CI->getParent()->getModule();

    Value *matVal = Resolve(matrixVal);
    if (matVal->getType() != matTy) {
        matVal = BitCastInst::Create(Instruction::BitCast, matVal, matTy, "matrix.store.cast", CI);
    }

    unsigned elemBitWidth = getBitWidthFromFlags(elementType);
    std::string funcName = GetLoadStoreMatrixFuncName(false, storeLayout, matrixLayout, elemBitWidth, rows, columns);
    FunctionType *funcType =
        FunctionType::get(Type::getVoidTy(M->getContext()),
            { ptrVal->getType(), matTy, strideVal->getType() }, false);
    std::vector<Value *> Args = { ptrVal, matVal, strideVal };

    InstsToErase.insert(CI);

    return CallInst::Create(M->getOrInsertFunction(funcName, funcType), Args, "", CI);
}

Instruction *JointMatrixFuncsResolutionPass::ResolveMad(CallInst *CI)
{
    /* Matrix A: */
    Value *aMatVal        = CI->getArgOperand(0);
    uint32_t aMatElemType = (uint32_t) constIntValue(CI->getArgOperand(1));
    unsigned aMatRows     = (unsigned) constIntValue(CI->getArgOperand(2));
    //uint64_t aMatColumns  = constIntValue(CI->getArgOperand(3));
    /* Matrix B: */
    Value *bMatVal        = CI->getArgOperand(4);
    uint32_t bMatElemType = (uint32_t) constIntValue(CI->getArgOperand(5));
    unsigned bMatRows     = (unsigned) constIntValue(CI->getArgOperand(6));
    unsigned bMatColumns  = (unsigned) constIntValue(CI->getArgOperand(7));
    /* Matrix C: */
    Value *cMatVal        = CI->getArgOperand(8);
    uint32_t cMatElemType = (uint32_t) constIntValue(CI->getArgOperand(9));
    unsigned cMatRows     = (unsigned) constIntValue(CI->getArgOperand(10));
    //uint64_t cMatColumns  = constIntValue(CI->getArgOperand(11));

    unsigned aMatLayout = LayoutRowMajor;
    Type *aMatTy = ResolveType(aMatVal->getType(), aMatElemType, aMatRows, &aMatLayout);

    unsigned bMatLayout = LayoutRowMajor;
    Type *bMatTy = ResolveType(bMatVal->getType(), bMatElemType, bMatRows, &bMatLayout);

    unsigned cMatLayout = LayoutRowMajor;
    Type *cMatTy = ResolveType(cMatVal->getType(), cMatElemType, cMatRows, &cMatLayout);

    unsigned M = aMatRows;
    unsigned N = bMatColumns;
    const bool isIntegerMAD = isElmentTypeInteger(cMatElemType);
    std::string funcName = getMADMatrixFuncName(aMatElemType, bMatElemType, cMatElemType, M, N);
    FunctionType *funcType = FunctionType::get(cMatTy, { cMatTy, aMatTy, bMatTy }, false);
  
    Value *cMat = Resolve(cMatVal);
    Value *aMat = Resolve(aMatVal);
    Value *bMat = Resolve(bMatVal);

    std::vector<Value *> Args = { cMat, aMat, bMat };

    Module *Mod = CI->getParent()->getModule();

    InstsToErase.insert(CI);

    return CallInst::Create(Mod->getOrInsertFunction(funcName, funcType), Args, "matrix", CI);
}

Value *JointMatrixFuncsResolutionPass::Resolve(Value *v)
{
    if (ResolvedValues.count(v) > 0) {
        return ResolvedValues[v];
    }

    if (CallInst *CI = dyn_cast<CallInst>(v)) {
        Function* func = CI->getCalledFunction();
        IGC_ASSERT_MESSAGE(func, "Unexpected missing function.");

        Instruction *NewInst = nullptr;
        StringRef funcName = func->getName();
        if (funcName.startswith(JointMatrixLoadPrefx)) {
            NewInst = ResolveLoad(CI);
        } else if (funcName.startswith(JointMatrixStorePrefx)) {
            NewInst = ResolveStore(CI);
        } else if (funcName.startswith(JointMatrixMadPrefx)) {
            NewInst = ResolveMad(CI);
        }
        return NewInst;
    } else if (PHINode *PN = dyn_cast<PHINode>(v)) {
        unsigned IncomingCount = PN->getNumIncomingValues();
        ResolvedValues[v] = v;
        Value *First = Resolve(PN->getIncomingValue(0));

        PHINode *NewPN = PHINode::Create(First->getType(), IncomingCount, "matrix.phi.node", PN);
        ResolvedValues[v] = NewPN;

        NewPN->addIncoming(First, PN->getIncomingBlock(0));
        for (unsigned i = 1; i < IncomingCount; i++) {
            Value *oldOperand = PN->getIncomingValue(i);
            Value *operand = Resolve(oldOperand);
            NewPN->addIncoming(operand, PN->getIncomingBlock(i));
        }
        
        PN->replaceAllUsesWith(UndefValue::get(PN->getType()));
        PN->eraseFromParent();

        return NewPN;
    }
    
    IGC_ASSERT_MESSAGE(false, "Resolve failure.");
    return nullptr;
}

void JointMatrixFuncsResolutionPass::visitCallInst(CallInst& CI)
{
    Function* func = CI.getCalledFunction();
    if (!func)
        return;

    StringRef funcName = func->getName();
    /* Look for store functions and then recursively trace values that are used
     * by them. In future when returning and passing matrices by argument is
     * supported this also basic block terminators should be used as
     * transformation starting point */
    if (funcName.startswith(JointMatrixStorePrefx)) {
        ResolveStore(&CI);
    }
}
