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


#define PASS_FLAG     "igc-joint-matrix-resolution"
#define PASS_DESC     "Lowering of INTEL Joint Matrix SPIR-V instructions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN(JointMatrixFuncsResolutionPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(JointMatrixFuncsResolutionPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

JointMatrixFuncsResolutionPass::JointMatrixFuncsResolutionPass() : FunctionPass(ID)
{
    initializeJointMatrixFuncsResolutionPassPass(*PassRegistry::getPassRegistry());
}

bool JointMatrixFuncsResolutionPass::runOnFunction(Function& F)
{
    m_Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
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

namespace IGC {
struct JointMatrixTypeDescription {
    unsigned layout = 0;
    unsigned rows = 0;
    unsigned columns = 0;
    unsigned bitWidth = 0;
    bool isFloating = false;
};
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
        (bool isLoad, unsigned operationLayout, const JointMatrixTypeDescription *desc)
{
    /* Treat row major matrices with types not supported by accumulators as
     * PackedA matrices. Both are in row major format. */
    unsigned matrixLayout = desc->layout;
    if (isLoad && matrixLayout == LayoutRowMajor && desc->bitWidth <= 16) {
        matrixLayout = LayoutPackedA;
    }

    std::string name
      = isLoad ? "__builtin_spriv_OpJointMatrixLoadINTEL_" : "__builtin_spriv_OpJointMatrixStoreINTEL_";
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

    switch (operationLayout) {
      case LayoutRowMajor:
        name += "RowMajor_";
        break;
      case LayoutColumnMajor:
        name += "ColumnMajor_";
        break;
      case LayoutPackedB:
        IGC_ASSERT_MESSAGE(matrixLayout == operationLayout, "Unexpected load/store layout.");
        name += "PackedB_";
        break;
      default:
        IGC_ASSERT_MESSAGE(false, "Unexpected load/store layout.");
    }

    /* On PVC due to SIMD16 different SIMD lane contribution is used for matrix A.
     * Therefore different load function is required. */
    if (m_Ctx->platform.hasExecSize16DPAS() && matrixLayout == LayoutPackedA) {
        name += "SG16_";
    }

    name += std::to_string(desc->rows);
    name += "x";
    name += std::to_string(desc->columns);
    name += "_";

    if (desc->bitWidth == 8) {
        name += "i8_";
    } else if (desc->bitWidth == 16) {
        name += "i16_";
    } else if (desc->bitWidth == 32) {
        name += "i32_";
    } else {
        IGC_ASSERT_MESSAGE(false, "Unexpected matrix element size.");
    }

    if (isLoad) {
        name += "v8i8_pi32_i32";
    } else {
        name += "pi64_v8i8";
    }
    return name;
}

static unsigned parseNumber(StringRef name, unsigned *offset) {
#define BUFFER_SIZE 16
    char buffer[BUFFER_SIZE+1];
    unsigned count = 0;
    while (std::isdigit(name[*offset]) && count < BUFFER_SIZE) {
        buffer[count] = name[*offset];
        *offset += 1;
        count += 1;
    }
    buffer[count] = '\0';
    return std::stoi(buffer);
}

/* This function extracts metadata from JointMatrix type names. They use the
 * following convention: intel.joint_matrix_acc_8x8_i32_t */
static void parseMatrixTypeName(const Type *opaqueType, JointMatrixTypeDescription *outDescription) {
    const PointerType *ptrType = cast<PointerType>(opaqueType);
    StringRef name = ptrType->getElementType()->getStructName();

    unsigned offset = 0;
    if (name.startswith("intel.joint_matrix_packedA_")) {
        outDescription->layout = LayoutPackedA;
        offset += sizeof "intel.joint_matrix_packedA_";
    } else if (name.startswith("intel.joint_matrix_packedB_")) {
        outDescription->layout = LayoutPackedB;
        offset += sizeof "intel.joint_matrix_packedB_";
    } else if (name.startswith("intel.joint_matrix_acc_")) {
        outDescription->layout = LayoutRowMajor;
        offset += sizeof "intel.joint_matrix_acc_";
    }
    offset -= 1; /* Go back to the end of prefix. */
    outDescription->rows = parseNumber(name, &offset);

    offset += 1; /* Skip delimiter, 'x'. */
    outDescription->columns = parseNumber(name, &offset);

    offset += 1; /* Skip delimiter, '_' */
    outDescription->isFloating = name[offset] == 'f';

    offset += 1; /* Skip type specifier, [f|i] */
    outDescription->bitWidth = parseNumber(name, &offset);
}

Type *JointMatrixFuncsResolutionPass::ResolveType(const Type *opaqueType, JointMatrixTypeDescription *outDesc)
{
    IGC_ASSERT_EXIT_MESSAGE(opaqueType && opaqueType->isPointerTy(),
        "Unexpected type in matrix function resolution.");

    JointMatrixTypeDescription desc;
    parseMatrixTypeName(opaqueType, &desc);

    if (desc.layout == LayoutRowMajor && desc.bitWidth <= 16) {
        desc.layout = LayoutPackedA;
    }

    if (outDesc != nullptr)
      *outDesc = desc;

    LLVMContext &ctx = opaqueType->getContext();

    if (desc.layout == LayoutPackedA) {
        Type *baseType = Type::getInt32Ty(ctx);
        if (m_Ctx->platform.hasExecSize16DPAS()) {
            baseType = Type::getInt16Ty(ctx);
        }
        return IGCLLVM::FixedVectorType::get(baseType, desc.rows);
    } else if (desc.layout == LayoutPackedB) {
        Type *baseType = Type::getInt32Ty(ctx);
        return IGCLLVM::FixedVectorType::get(baseType, 8);
    } else if (desc.layout == LayoutRowMajor) {
        Type *baseType = Type::getInt32Ty(ctx);
        if (desc.isFloating) {
            baseType = Type::getFloatTy(ctx);
        }
        return IGCLLVM::FixedVectorType::get(baseType, desc.rows);
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

    JointMatrixTypeDescription desc;
    Type *matTy = ResolveType(CI->getType(), &desc);
    /* Cast floating types to integer types of the same size. This allows to
     * have a single set of store builtins for floats and integer */
    Type *retTy = getIntegerEquivalent(matTy);

    Module *M = CI->getParent()->getModule();

    std::string funcName = GetLoadStoreMatrixFuncName(true, loadLayout, &desc);
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

    JointMatrixTypeDescription desc;
    Type *matTy = ResolveType(matrixVal->getType(), &desc);
    /* Cast floating types to integer types of the same size. This allows to
     * have a single set of store builtins for floats and integers */
    matTy = getIntegerEquivalent(matTy);

    Module *M = CI->getParent()->getModule();

    Value *matVal = Resolve(matrixVal);
    if (matVal->getType() != matTy) {
        matVal = BitCastInst::Create(Instruction::BitCast, matVal, matTy, "matrix.store.cast", CI);
    }

    std::string funcName = GetLoadStoreMatrixFuncName(false, storeLayout, &desc);
    FunctionType *funcType =
        FunctionType::get(Type::getVoidTy(M->getContext()),
            { ptrVal->getType(), matTy, strideVal->getType() }, false);
    std::vector<Value *> Args = { ptrVal, matVal, strideVal };

    InstsToErase.insert(CI);

    return CallInst::Create(M->getOrInsertFunction(funcName, funcType), Args, "", CI);
}

static PrecisionType getElementPrecison(const JointMatrixTypeDescription *desc, bool floatOp, bool isUnsigned) {
  const unsigned width = desc->bitWidth;
  if (floatOp && width == 16) {
      /* bf is passed as uint16_t, hf is using halfs */
      return desc->isFloating ? PrecisionType::FP16 : PrecisionType::BF16;
  }
  if (!floatOp && width == 8) {
      return isUnsigned ? PrecisionType::U8 : PrecisionType::S8;
  }
  return PrecisionType::PRECISION_UNUSED;
}

Instruction *JointMatrixFuncsResolutionPass::ResolveMad(CallInst *CI, unsigned OperationType) {
    Value *aMatVal = CI->getArgOperand(0);
    Value *bMatVal = CI->getArgOperand(1);
    Value *cMatVal = CI->getArgOperand(2);

    JointMatrixTypeDescription aDesc;
    Type *aMatTy = ResolveType(aMatVal->getType(), &aDesc);

    JointMatrixTypeDescription bDesc;
    Type *bMatTy = ResolveType(bMatVal->getType(), &bDesc);

    JointMatrixTypeDescription cDesc;
    Type *cMatTy = ResolveType(cMatVal->getType(), &cDesc);

    IGC_ASSERT_MESSAGE(aDesc.layout == LayoutPackedA || aDesc.layout == LayoutRowMajor,
                       "Unexpected layout for matrix A in MAD operation.");
    IGC_ASSERT_MESSAGE(bDesc.layout == LayoutPackedB, "Unexpected layout for matrix A in MAD operation.");
    IGC_ASSERT_MESSAGE(cDesc.layout == LayoutRowMajor, "Unexpected layout for matrix A in MAD operation.");

    const bool floatMad = cDesc.isFloating;

    PrecisionType PA = getElementPrecison(&aDesc, floatMad, isOperandUnsigned(OperationType, 0));
    PrecisionType PB = getElementPrecison(&bDesc, floatMad, isOperandUnsigned(OperationType, 1));

    IGC_ASSERT_MESSAGE(PA != PrecisionType::PRECISION_UNUSED, "Invalid matrix A element type.");
    IGC_ASSERT_MESSAGE(PB != PrecisionType::PRECISION_UNUSED, "Invalid matrix B element type.");

    int SD = aDesc.rows; // systolic depth, 8 or 16
    int RC = bDesc.columns; // repeat count, from 1 to 8

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
    Value *fillValue = CI->getArgOperand(0);
    Type *matTy = ResolveType(CI->getType(), nullptr);

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

    CacheResolvedValue(CI, NewValue);
    return NewValue;
}

void JointMatrixFuncsResolutionPass::CacheResolvedValue(Value *oldValue, Value *newValue) {
    ResolvedValues[oldValue] = newValue;
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

        Type *type = ResolveType(v->getType(), nullptr);
        PHINode *NewPN = PHINode::Create(type, IncomingCount, "matrix.phi.node", PN);
        CacheResolvedValue(v, NewPN);

        for (unsigned i = 0; i < IncomingCount; i++) {
            Value *oldOperand = PN->getIncomingValue(i);
            Value *operand = Resolve(oldOperand);
            NewPN->addIncoming(operand, PN->getIncomingBlock(i));
        }

        InstsToErase.insert(PN);
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
    /* Resolve calls to JointMatrix BIs that haven't been resolved yet. In
     * future when returning and passing matrices by argument is
     * supported also basic block terminators should be used as
     * transformation starting point */
    if (funcName.startswith(CommonBIPrefix) && ResolvedValues.count(&CI) <= 0) {
        ResolveCall(&CI);
    }
}
