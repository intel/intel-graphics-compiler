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

static const char *CommonBIPrefix = "__builtin_spirv_";
static const char *JointMatrixLoadPrefx  = "__builtin_spirv_OpJointMatrixLoadINTEL";
static const char *JointMatrixStorePrefx = "__builtin_spirv_OpJointMatrixStoreINTEL";
static const char *JointMatrixMadPrefx   = "__builtin_spirv_OpJointMatrixMadINTEL";
static const char *JointMatrixSUMadPrefx = "__builtin_spirv_OpJointMatrixSUMadINTEL";
static const char *JointMatrixUSMadPrefx = "__builtin_spirv_OpJointMatrixUSMadINTEL";
static const char *JointMatrixUUMadPrefx = "__builtin_spirv_OpJointMatrixUUMadINTEL";
static const char *JointMatrixFillPrefx  = "__builtin_spirv_OpCompositeConstructJointMatrixINTEL";
static const char *JointMatrixWorkItemLengthPrefx = "__builtin_spirv_OpJointMatrixWorkItemLengthINTEL";
static const char *JointMatrixSliceInsert  = "__builtin_spirv_OpVectorInsertDynamicJointMatrixINTEL";
static const char *JointMatrixSliceExtract = "__builtin_spirv_OpVectorExtractDynamicJointMatrixINTEL";

enum {
    LayoutRowMajor,
    LayoutColumnMajor,
    LayoutPackedA,
    LayoutPackedB,
};

enum {
    MadOpSS,
    MadOpSU,
    MadOpUS,
    MadOpUU,
};

static unsigned getElementBitWidth(unsigned flags) {
    return flags & 0x7fffffff;
}

static bool isElementTypeInteger(unsigned flags) {
    return (flags & (1 << 31)) == 0;
}

static bool isElementTypeFloating(unsigned flags) {
    return !isElementTypeInteger(flags);
}

static bool isOperandUnsigned(unsigned OperationType, unsigned OperandId) {
    switch (OperationType) {
        default:
        case MadOpSS: return false;
        case MadOpUU: return true;
        case MadOpSU: return OperandId != 0;
        case MadOpUS: return OperandId == 0;
    }
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
      = load ? "__builtin_spriv_OpJointMatrixLoadINTEL_" : "__builtin_spriv_OpJointMatrixStoreINTEL_";
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

    /* On PVC due to SIMD16 different SIMD lane contribution is used for matrix A.
     * Therefore different load function is required. */
    if (Context->platform.hasExecSize16DPAS() && matrixLayout == LayoutPackedA) {
        name += "SG16_";
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
        if (Context->platform.hasExecSize16DPAS()) {
            baseType = Type::getInt16Ty(ctx);
        }
        return IGCLLVM::FixedVectorType::get(baseType, rows);
    } else if (name.equals("intel.joint_matrix_packedB_t")) {
        *outLayout = LayoutPackedB;
        Type *baseType = Type::getInt32Ty(ctx);
        return IGCLLVM::FixedVectorType::get(baseType, 8);
    } else if (name.equals("intel.joint_matrix_acc_t")) {
        *outLayout = LayoutRowMajor;
        Type *baseType = Type::getInt32Ty(ctx);
        if (isElementTypeFloating(elementTypeFlags)) {
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

    if (IGCLLVM::FixedVectorType *VT = dyn_cast<IGCLLVM::FixedVectorType>(matTy)) {
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

    unsigned elemBitWidth = getElementBitWidth(elementType);
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

    unsigned elemBitWidth = getElementBitWidth(elementType);
    std::string funcName = GetLoadStoreMatrixFuncName(false, storeLayout, matrixLayout, elemBitWidth, rows, columns);
    FunctionType *funcType =
        FunctionType::get(Type::getVoidTy(M->getContext()),
            { ptrVal->getType(), matTy, strideVal->getType() }, false);
    std::vector<Value *> Args = { ptrVal, matVal, strideVal };

    InstsToErase.insert(CI);

    return CallInst::Create(M->getOrInsertFunction(funcName, funcType), Args, "", CI);
}

static PrecisionType getElementPrecison(unsigned elemType, bool floatOp, bool isUnsigned) {
  const unsigned width = getElementBitWidth(elemType);
  if (floatOp && width == 16) {
      /* bf is passed as uint16_t, hf is using halfs */
      return isElementTypeInteger(elemType) ? PrecisionType::BF16 : PrecisionType::FP16;
  }
  if (!floatOp && width == 8) {
      return isUnsigned ? PrecisionType::S8 : PrecisionType::U8;
  }
  return PrecisionType::PRECISION_UNUSED;
}

Instruction *JointMatrixFuncsResolutionPass::ResolveMad(CallInst *CI, unsigned OperationType) {
    /* Matrix A: */
    Value *aMatVal        = CI->getArgOperand(0);
    uint32_t aMatElemType = (uint32_t) constIntValue(CI->getArgOperand(1));
    unsigned aMatRows     = (unsigned) constIntValue(CI->getArgOperand(2));
    /* Matrix B: */
    Value *bMatVal        = CI->getArgOperand(4);
    uint32_t bMatElemType = (uint32_t) constIntValue(CI->getArgOperand(5));
    unsigned bMatRows     = (unsigned) constIntValue(CI->getArgOperand(6));
    unsigned bMatColumns  = (unsigned) constIntValue(CI->getArgOperand(7));
    /* Matrix C: */
    Value *cMatVal        = CI->getArgOperand(8);
    uint32_t cMatElemType = (uint32_t) constIntValue(CI->getArgOperand(9));
    unsigned cMatRows     = (unsigned) constIntValue(CI->getArgOperand(10));

    unsigned aMatLayout = LayoutRowMajor;
    Type *aMatTy = ResolveType(aMatVal->getType(), aMatElemType, aMatRows, &aMatLayout);

    unsigned bMatLayout = LayoutRowMajor;
    Type *bMatTy = ResolveType(bMatVal->getType(), bMatElemType, bMatRows, &bMatLayout);

    unsigned cMatLayout = LayoutRowMajor;
    Type *cMatTy = ResolveType(cMatVal->getType(), cMatElemType, cMatRows, &cMatLayout);

    IGC_ASSERT_MESSAGE(aMatLayout == LayoutPackedA || aMatLayout == LayoutRowMajor,
                       "Unexpected layout for matrix A in MAD operation.");
    IGC_ASSERT_MESSAGE(bMatLayout == LayoutPackedB, "Unexpected layout for matrix A in MAD operation.");
    IGC_ASSERT_MESSAGE(cMatLayout == LayoutRowMajor, "Unexpected layout for matrix A in MAD operation.");

    const bool floatMad = isElementTypeFloating(cMatElemType);

    PrecisionType PA = getElementPrecison(aMatElemType, floatMad, isOperandUnsigned(OperationType, 0));
    PrecisionType PB = getElementPrecison(bMatElemType, floatMad, isOperandUnsigned(OperationType, 1));

    IGC_ASSERT_MESSAGE(PA != PrecisionType::PRECISION_UNUSED, "Invalid matrix A element type.");
    IGC_ASSERT_MESSAGE(PB != PrecisionType::PRECISION_UNUSED, "Invalid matrix B element type.");

    int SD = aMatRows; // systolic depth, 8 or 16
    int RC = bMatColumns; // repeat count, from 1 to 8

    IGC_ASSERT_MESSAGE(SD == 8 || SD == 16, "Unexpected systolic depth in MAD operaion.");
    IGC_ASSERT_MESSAGE(RC >= 1 && RC <= 8,  "Unexpected repeat count in MAD operaion.");

    bool IsDpasw = false; // is wide

    LLVMContext& Ctx = CI->getContext();
    Type* intTy = Type::getInt32Ty(Ctx);
    Type* boolTy = Type::getInt1Ty(Ctx);

    Value* args[8];
    args[0] = Resolve(cMatVal);
    args[1] = Resolve(aMatVal);
    args[2] = Resolve(bMatVal);
    args[3] = ConstantInt::get(intTy, PA);
    args[4] = ConstantInt::get(intTy, PB);
    args[5] = ConstantInt::get(intTy, SD);
    args[6] = ConstantInt::get(intTy, RC);
    args[7] = ConstantInt::get(boolTy, IsDpasw);

    Type* ITys[4] = { cMatTy, cMatTy, aMatTy, bMatTy };

    Module *Mod = CI->getParent()->getModule();
    GenISAIntrinsic::ID iid = GenISAIntrinsic::GenISA_sub_group_dpas;
    Function *dpasFunc = GenISAIntrinsic::getDeclaration(Mod, iid, ITys);
    Instruction *dpasCall = CallInst::Create(dpasFunc, args, VALUE_NAME("dpas"), CI);

    InstsToErase.insert(CI);

    return dpasCall;
}

static int getSliceSize(Type *matrixType) {
    IGCLLVM::FixedVectorType *ty = dyn_cast<IGCLLVM::FixedVectorType>(matrixType);
    IGC_ASSERT_MESSAGE(ty, "Unexpected type when calculating slice size.");
    return (int)ty->getNumElements();
}

Value *JointMatrixFuncsResolutionPass::ResolveFill(CallInst *CI) {
    Value *fillValue     = CI->getArgOperand(0);
    uint32_t elementType = (uint32_t) constIntValue(CI->getArgOperand(1));
    unsigned rows        = (unsigned) constIntValue(CI->getArgOperand(2));

    unsigned matrixLayout = LayoutRowMajor;
    Type *matTy = ResolveType(CI->getType(), elementType, rows, &matrixLayout);

    IRBuilder builder(CI);
    const int sliceSize = getSliceSize(matTy);
    Value *slice = UndefValue::get(matTy);
    for (int i = 0; i < sliceSize; i++) {
        slice = builder.CreateInsertElement(slice, fillValue, i);
    }

    InstsToErase.insert(CI);
    return slice;
}

Value *JointMatrixFuncsResolutionPass::ResolveWILength(CallInst *CI) {
    Value *matrix = Resolve(CI->getArgOperand(0));

    const int sliceSize = getSliceSize(matrix->getType());
    Value *lenght = ConstantInt::get(CI->getType(), sliceSize, "matrix.slice.size");

    CI->replaceAllUsesWith(lenght);
    InstsToErase.insert(CI);
    return lenght;
}

Value *JointMatrixFuncsResolutionPass::ResolveSliceInsert(CallInst *CI) {
    Value *matrix = Resolve(CI->getArgOperand(0));
    Value *component = CI->getArgOperand(1);
    Value *index = CI->getArgOperand(2);

    IRBuilder builder(CI);
    Value *slice = builder.CreateInsertElement(matrix, component, index);

    InstsToErase.insert(CI);
    return slice;
}

Value *JointMatrixFuncsResolutionPass::ResolveSliceExtract(CallInst *CI) {
    Value *matrix = Resolve(CI->getArgOperand(0));
    Value *index = CI->getArgOperand(1);

    IRBuilder builder(CI);
    Value *element = builder.CreateExtractElement(matrix, index, "matrix.element");

    CI->replaceAllUsesWith(element);
    InstsToErase.insert(CI);
    return element;
}

Value *JointMatrixFuncsResolutionPass::ResolveCall(CallInst *CI) {
    Function* func = CI->getCalledFunction();
    if (!func)
        return nullptr;

    IGC_ASSERT_MESSAGE(func, "Unexpected missing function.");

    Value *NewValue = nullptr;
    StringRef funcName = func->getName();
    if (funcName.startswith(JointMatrixLoadPrefx)) {
        NewValue = ResolveLoad(CI);
    } else if (funcName.startswith(JointMatrixStorePrefx)) {
        NewValue = ResolveStore(CI);
    } else if (funcName.startswith(JointMatrixMadPrefx)) {
        NewValue = ResolveMad(CI, MadOpSS);
    } else if (funcName.startswith(JointMatrixSUMadPrefx)) {
        NewValue = ResolveMad(CI, MadOpSU);
    } else if (funcName.startswith(JointMatrixUSMadPrefx)) {
        NewValue = ResolveMad(CI, MadOpUS);
    } else if (funcName.startswith(JointMatrixUUMadPrefx)) {
        NewValue = ResolveMad(CI, MadOpUU);
    } else if (funcName.startswith(JointMatrixFillPrefx)) {
        NewValue = ResolveFill(CI);
    } else if (funcName.startswith(JointMatrixWorkItemLengthPrefx)) {
        NewValue = ResolveWILength(CI);
    } else if (funcName.startswith(JointMatrixSliceInsert)) {
        NewValue = ResolveSliceInsert(CI);
    } else if (funcName.startswith(JointMatrixSliceExtract)) {
        NewValue = ResolveSliceExtract(CI);
    }
    return NewValue;
}

Value *JointMatrixFuncsResolutionPass::Resolve(Value *v)
{
    if (ResolvedValues.count(v) > 0) {
        return ResolvedValues[v];
    }

    if (CallInst *CI = dyn_cast<CallInst>(v)) {
        return ResolveCall(CI);
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
    if (funcName.startswith(CommonBIPrefix)) {
        ResolveCall(&CI);
    }
}
