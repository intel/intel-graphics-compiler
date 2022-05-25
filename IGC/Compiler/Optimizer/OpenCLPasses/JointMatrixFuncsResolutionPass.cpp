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
#include <llvm/IR/IRBuilder.h>

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
    PlaceholderInstructions.clear();
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
    /* Treat row major matrices with types not supported by accumulators as
     * PackedA matrices. Both are in row major format. */
    if (desc.layout == LayoutRowMajor && desc.bitWidth <= 16) {
        desc.layout = LayoutPackedA;
    }

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

    int SD = 8; // systolic depth, only 8 supported currently
    int RC = aDesc.rows; // repeat count, from 1 to 8

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

static int getResolvedVectorSize(Type *matrixType) {
    IGCLLVM::FixedVectorType *ty = dyn_cast<IGCLLVM::FixedVectorType>(matrixType);
    IGC_ASSERT_MESSAGE(ty, "Unexpected type when calculating slice size.");
    return (int)ty->getNumElements();
}

static Type *getResolvedVectorElementType(Type *matrixType) {
    IGCLLVM::FixedVectorType *ty = dyn_cast<IGCLLVM::FixedVectorType>(matrixType);
    IGC_ASSERT_MESSAGE(ty, "Unexpected type when calculating slice size.");
    return ty->getElementType();
}

static int getSliceSize(const JointMatrixTypeDescription *desc) {
    if (desc->layout == LayoutRowMajor) {
        return desc->rows;
    }
    if (desc->layout == LayoutPackedA) {
        return desc->rows * (32 / desc->bitWidth);
    }
    if (desc->layout == LayoutPackedB) {
        return 8  * (32 / desc->bitWidth);
    }
    IGC_ASSERT_MESSAGE(true, "Unexpected matrix layout.");
    return 1;
}

template <class BuilderT>
static Value *packFillValue
        (BuilderT *Builder, Value *V, IntegerType *SourceType, IntegerType *TargetType) {
    /* TODO: fixup for DPCPP bug */
    if (V->getType()->isPointerTy()) {
        PointerType *PT = dyn_cast<PointerType>(V->getType());
        V = Builder->CreateBitCast(V, PointerType::get(SourceType, PT->getAddressSpace()));
        V = Builder->CreateLoad(SourceType, V);
    }

    IntegerType *currentType = dyn_cast<IntegerType>(V->getType());
    if (currentType == nullptr) {
        unsigned size = V->getType()->getScalarSizeInBits();
        V = Builder->CreateBitCast(V, Type::getIntNTy(Builder->getContext(), size));
        currentType = dyn_cast<IntegerType>(V->getType());
    }

    uint64_t sourceBitWidth = currentType->getBitWidth();
    uint64_t packFactor = TargetType->getBitWidth() / sourceBitWidth;

    if (ConstantInt *Constant = dyn_cast<ConstantInt>(V)) {
        uint64_t value = Constant->getLimitedValue();
        if (value == 0) {
            return ConstantInt::get(TargetType, 0, "matrix.fill.zero");
        }

        uint64_t packedValue = 0;
        for (unsigned i = 0; i < packFactor; i++) {
            packedValue |= value << (sourceBitWidth * i);
        }
        return ConstantInt::get(TargetType, packedValue, "matrix.fill.packedconst");
    }

    Value *extendedValue = Builder->CreateZExt(V, TargetType);
    Value *acc = extendedValue;//ConstantInt::get(TargetType, 0, "matrix.fill.acc");
    for (unsigned i = 1; i < packFactor; i++) {
        Value *shl = Builder->CreateShl(extendedValue, sourceBitWidth * i);
        acc = Builder->CreateOr(shl, acc);
    }
    return acc;
}

Value *JointMatrixFuncsResolutionPass::ResolveFill(CallInst *CI) {
    Value *fillValue = CI->getArgOperand(0);

    JointMatrixTypeDescription desc;
    Type *matTy = ResolveType(CI->getType(), &desc);

    IRBuilder builder(CI);
    const int sliceSize = getSliceSize(&desc);
    const int vectorSize = getResolvedVectorSize(matTy);
    /* Case with packing: */
    if (sliceSize > vectorSize) {
        IntegerType *sliceElmentType = Type::getIntNTy(builder.getContext(), desc.bitWidth);
        IntegerType *vectorElementType = dyn_cast<IntegerType>(getResolvedVectorElementType(matTy));
        fillValue = packFillValue(&builder, fillValue, sliceElmentType, vectorElementType);
    /* Case without packing: */
    } else if (sliceSize != vectorSize) {
        IGC_ASSERT_MESSAGE(false, "Malformed matrix slice.");
    }

    Value *slice = UndefValue::get(matTy);
    for (int i = 0; i < vectorSize; i++) {
        slice = builder.CreateInsertElement(slice, fillValue, i);
    }

    InstsToErase.insert(CI);
    return slice;
}

Value *JointMatrixFuncsResolutionPass::ResolveWILength(CallInst *CI) {
    JointMatrixTypeDescription desc;
    ResolveType(CI->getArgOperand(0)->getType(), &desc);

    const int sliceSize = getSliceSize(&desc);
    Value *lenght = ConstantInt::get(CI->getType(), sliceSize, "matrix.slice.size");

    CI->replaceAllUsesWith(lenght);
    InstsToErase.insert(CI);
    return lenght;
}

template <class BuilderT>
static Value *createSliceExtract
      (BuilderT *builder, Value *matrix, Value *index, const JointMatrixTypeDescription *desc) {
    const int sliceSize = getSliceSize(desc);
    const int vectorSize = getResolvedVectorSize(matrix->getType());
    /* Unpacking: */
    if (sliceSize > vectorSize) {
        uint64_t packFactor = sliceSize / vectorSize;
        index = builder->CreateUDiv(index, ConstantInt::get(index->getType(), packFactor));
    }
    Value *element = builder->CreateExtractElement(matrix, index, "matrix.element");
    return element;
}

Value *JointMatrixFuncsResolutionPass::ResolveSliceInsert(CallInst *CI) {
    Value *matrix = Resolve(CI->getArgOperand(0));
    Value *component = CI->getArgOperand(1);
    Value *index = CI->getArgOperand(2);

    JointMatrixTypeDescription desc;
    Type *rawMatTy = ResolveType(CI->getArgOperand(0)->getType(), &desc);
    IGCLLVM::FixedVectorType *matTy = dyn_cast<IGCLLVM::FixedVectorType>(rawMatTy);

    IRBuilder builder(CI);
    const int sliceSize = getSliceSize(&desc);
    const int vectorSize = getResolvedVectorSize(matTy);

    Value *slice = nullptr;
    if (sliceSize > vectorSize) {
        Value *element = createSliceExtract(&builder, matrix, index, &desc);
        if (!isa<IntegerType>(element->getType())) {
            unsigned vecElemSize = matTy->getElementType()->getScalarSizeInBits();
            element = builder.CreateBitCast(element, Type::getIntNTy(builder.getContext(), vecElemSize));
        }

        uint64_t packFactor = sliceSize / vectorSize;
        Value *offset = builder.CreateURem(index, ConstantInt::get(index->getType(), packFactor));
        offset = builder.CreateMul(offset, ConstantInt::get(offset->getType(), desc.bitWidth));

        index = builder.CreateUDiv(index, ConstantInt::get(index->getType(), packFactor));

        if (!isa<IntegerType>(component->getType())) {
            component = builder.CreateBitCast(component, Type::getIntNTy(builder.getContext(), desc.bitWidth));
        }

        unsigned vecElemSize = matTy->getElementType()->getScalarSizeInBits();
        component = builder.CreateZExtOrBitCast(component, Type::getIntNTy(builder.getContext(), vecElemSize));
        offset = builder.CreateTruncOrBitCast(offset, Type::getIntNTy(builder.getContext(), vecElemSize));

        /* clear element bits: */
        uint64_t maskValue = (1 << desc.bitWidth) - 1;
        Value *mask = builder.CreateShl(ConstantInt::get(element->getType(), maskValue), offset);
        mask = builder.CreateNot(mask);
        element = builder.CreateAnd(element, mask);

        /* shift component and merge with element: */
        component = builder.CreateShl(component, offset);
        component = builder.CreateOr(element, component);
    }
    slice = builder.CreateInsertElement(matrix, component, index);

    InstsToErase.insert(CI);
    return slice;
}

Value *JointMatrixFuncsResolutionPass::ResolveSliceExtract(CallInst *CI) {
    Value *matrix = Resolve(CI->getArgOperand(0));
    Value *index = CI->getArgOperand(1);

    JointMatrixTypeDescription desc;
    Type *matTy = ResolveType(CI->getArgOperand(0)->getType(), &desc);

    IRBuilder builder(CI);
    Value *element = createSliceExtract(&builder, matrix, index, &desc);
    /* Unpacking: */
    const int sliceSize = getSliceSize(&desc);
    const int vectorSize = getResolvedVectorSize(matTy);
    if (sliceSize > vectorSize) {
        index = builder.CreateTruncOrBitCast(index, element->getType());
        uint64_t packFactor = sliceSize / vectorSize;
        Value *offset = builder.CreateURem(index, ConstantInt::get(index->getType(), packFactor));
        offset = builder.CreateMul(offset, ConstantInt::get(offset->getType(), desc.bitWidth));
        element = builder.CreateAShr(element, offset);
        uint64_t mask = (1 << desc.bitWidth) - 1;
        element = builder.CreateAnd(element, mask);

        element = builder.CreateTruncOrBitCast(element, Type::getIntNTy(builder.getContext(), desc.bitWidth));
        element = builder.CreateBitCast(element, CI->getType());
    }

    CI->replaceAllUsesWith(element);
    InstsToErase.insert(CI);
    return element;
}

void JointMatrixFuncsResolutionPass::InsertPlaceholder(Value *v) {
    if (ResolvedValues.count(v) > 0) {
        return;
    }

    Type *type = v->getType();
    if (type->isPointerTy()) {
        type = ResolveType(v->getType(), nullptr);
    }
    if (type->isVoidTy()) {
        return;
    }

    Instruction *predecesor = nullptr;
    if (Instruction *inst = dyn_cast<Instruction>(v)) {
        predecesor = inst;
    }
    /* Using bit-casts as placeholder values. Undefs of each type are unique per
     * module and cannot be used as unique placeholders. */
    Instruction *placeholder =
        BitCastInst::Create(Instruction::BitCast, UndefValue::get(type),
                            type, "tmp.value", predecesor);
    ResolvedValues[v] = placeholder;
    PlaceholderInstructions[v] = placeholder;
}

Value *JointMatrixFuncsResolutionPass::ResolveCall(CallInst *CI) {
    Function* func = CI->getCalledFunction();
    if (!func)
        return nullptr;

    IGC_ASSERT_MESSAGE(func, "Unexpected missing function.");

    Value *NewValue = nullptr;
    StringRef funcName = func->getName();
    if (funcName.startswith(JointMatrixLoadPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveLoad(CI);
    } else if (funcName.startswith(JointMatrixStorePrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveStore(CI);
    } else if (funcName.startswith(JointMatrixMadPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveMad(CI, MadOpSS);
    } else if (funcName.startswith(JointMatrixSUMadPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveMad(CI, MadOpSU);
    } else if (funcName.startswith(JointMatrixUSMadPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveMad(CI, MadOpUS);
    } else if (funcName.startswith(JointMatrixUUMadPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveMad(CI, MadOpUU);
    } else if (funcName.startswith(JointMatrixFillPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveFill(CI);
    } else if (funcName.startswith(JointMatrixWorkItemLengthPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveWILength(CI);
    } else if (funcName.startswith(JointMatrixSliceInsert)) {
        InsertPlaceholder(CI);
        NewValue = ResolveSliceInsert(CI);
    } else if (funcName.startswith(JointMatrixSliceExtract)) {
        InsertPlaceholder(CI);
        NewValue = ResolveSliceExtract(CI);
    }

    CacheResolvedValue(CI, NewValue);
    return NewValue;
}

void JointMatrixFuncsResolutionPass::CacheResolvedValue(Value *oldValue, Value *newValue) {
    if (newValue == nullptr)
        return;

    if (PlaceholderInstructions.count(oldValue) > 0) {
        Instruction *placeholder = PlaceholderInstructions[oldValue];
        PlaceholderInstructions.erase(oldValue);
        placeholder->replaceAllUsesWith(newValue);
        InstsToErase.insert(placeholder);
    }

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
    } else if (isa<UndefValue>(v)) {
        Type *type = ResolveType(v->getType(), nullptr);
        return UndefValue::get(type);
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
