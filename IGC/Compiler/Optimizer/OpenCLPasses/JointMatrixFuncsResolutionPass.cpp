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
#include <llvm/ADT/Sequence.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/PostOrderIterator.h>

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Module.h"
#include "llvmWrapper/Support/Alignment.h"
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
    ResolvedTypes.clear();
    InstsToErase.clear();
    Changed = false;

    // Use reverse post order traversal to reduce level or recursion
    ReversePostOrderTraversal<Function *> RPOT(&F);
    for (BasicBlock *BB : RPOT)
        visit(BB);

    for (Instruction *I : InstsToErase) {
        if (ResolvedValues[I] && I->getType() == ResolvedValues[I]->getType())
        {
            I->replaceAllUsesWith(ResolvedValues[I]);
        }
        else
        {
            Value *undef = UndefValue::get(I->getType());
            I->replaceAllUsesWith(undef);
        }
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

    LayoutMax
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

struct SupportedParams {
    int maxRows = -1; /* -1 means: don't check */
    int rows = -1;
    int columns = -1;
    unsigned bitWidth = 0; /* All supported sizes are powers of two, this field is
                              used as a bitfield with union of suported sizes */
    unsigned layouts = 0; /* Each bit of this field corresponds to a single layout. */
};

static SupportedParams getSupportedParams(const JointMatrixTypeDescription *desc, bool useSG16) {
    /* slices are represented as vectors from <1 x i32> to <8 x i32>, resulting in the maximum slice size: */
    const unsigned maxSliceBitWidth = 256;
    SupportedParams params;
    if (desc->layout == LayoutPackedA) {
        params.maxRows = 8;
        params.columns = maxSliceBitWidth / desc->bitWidth;
        params.bitWidth = 8 | 16;
        params.layouts = 1 << LayoutRowMajor;
    } else if (desc->layout == LayoutPackedB) {
        params.rows = maxSliceBitWidth / desc->bitWidth;
        params.columns = useSG16 ? 16 : 8;
        params.bitWidth = 8 | 16;
        params.layouts |= 1 << LayoutColumnMajor;
        params.layouts |= 1 << LayoutPackedB;
        params.layouts |= 1 << LayoutPackedA; /* PackedA means just packed in the new version of spec. */
    } else { /* accumulator */
        params.maxRows = maxSliceBitWidth / desc->bitWidth;
        params.columns = useSG16 ? 16 : 8;
        params.bitWidth = 8 | 32;
        params.layouts = 1 << LayoutRowMajor;
    }
    return params;
}

enum ParamsCheckResult : unsigned {
    ALL_VALID        = 0,
    INVALID_ROWS     = 1 << 0,
    INVALID_COLS     = 1 << 1,
    INVALID_ELEM     = 1 << 2,
    INVALID_LAYOUT   = 1 << 3,
    INVALID_PLATFORM = 1 << 4,
};

static ParamsCheckResult checkSupportedParams
        (const JointMatrixTypeDescription *desc, unsigned operationLayout,
         const SupportedParams &params, const IGC::CPlatform *platform) {
    unsigned result = ALL_VALID;
    if (params.maxRows != -1 && (int)desc->rows > params.maxRows) {
        result |= INVALID_ROWS;
    }
    if (params.rows != -1 && (int)desc->rows != params.rows) {
        result |= INVALID_ROWS;
    }
    if (params.columns != -1 && (int)desc->columns != params.columns) {
        result |= INVALID_COLS;
    }
    if ((params.bitWidth & desc->bitWidth) != desc->bitWidth) {
        result |= INVALID_ELEM;
    }
    if (((1 << operationLayout) & params.layouts) == 0) {
        result |= INVALID_LAYOUT;
    }
    if (!platform->supportDpasInstruction()) {
        result |= INVALID_PLATFORM;
    }
    return static_cast<ParamsCheckResult>(result);
}

static const char *nameLayout(unsigned layout) {
    switch (layout) {
        case LayoutPackedA:
        case LayoutPackedB: return "packed layout";
        case LayoutRowMajor: return "row major layout";
        case LayoutColumnMajor: return "column major layout";
        default: return "unknown";
    }
}

bool JointMatrixFuncsResolutionPass::ValidateLoadStore
        (bool isLoad, unsigned operationLayout, const JointMatrixTypeDescription *desc, llvm::Value *ctx) {
    SupportedParams params = getSupportedParams(desc, m_Ctx->platform.hasExecSize16DPAS());
    ParamsCheckResult result = checkSupportedParams(desc, operationLayout, params, &m_Ctx->platform);
    if (result != ALL_VALID) {
        std::string msg = "Unsupported JointMatrix operation: ";
        msg += isLoad ? "load " : "store ";
        msg += "matrix ";
        msg += (desc->layout == LayoutPackedA ? "A" : (desc->layout == LayoutPackedB ? "B" : "C"));
        msg +=  " <" + std::to_string(desc->rows)
            + " x " + std::to_string(desc->columns)
            + " x i" + std::to_string(desc->bitWidth)
            + "> with " + nameLayout(operationLayout);
        if (result & INVALID_ROWS) {
            msg += "\n -> unsupported number of rows: " + std::to_string(desc->rows);
            msg += "\n    supported values: ";
            if (params.maxRows != -1) {
                msg += "lower or equal " + std::to_string(params.maxRows);
            } else if (params.rows != -1) {
                msg += std::to_string(params.rows);
            }
        }
        if (result & INVALID_COLS) {
            msg += "\n -> unsupported number of columns: " + std::to_string(desc->columns);
            msg += "\n    supported values: ";
            if (params.columns != -1) {
                msg += std::to_string(params.columns);
            }
        }
        if (result & INVALID_ELEM) {
            msg += "\n -> unsupported matrix element size: " + std::to_string(desc->bitWidth) + " bits";
            msg += "\n    supported values: ";
            /* check powers of two from 3 to 5, i.e. 8 to 32 */
            for (unsigned i = 3; i <= 5; i++) {
                unsigned bitWidth = 1 << i;
                if (bitWidth & params.bitWidth) {
                    if (i > 3) msg += ", ";
                    msg += std::to_string(bitWidth);
                }
            }
        }
        if (result & INVALID_LAYOUT) {
            msg += "\n -> unsupported operation layout";
            msg += "\n    supported values: ";
            for (unsigned i = 0; i < LayoutMax; i++) {
                if ((1 << i) & params.layouts) {
                    if (i > 0) msg += ", ";
                    msg += nameLayout(i);
                }
            }
        }
        if (result & INVALID_PLATFORM) {
            msg += "\n -> targeted GPU device does not support SYCL joint matrix API";
        }
        m_Ctx->EmitError(msg.c_str(), ctx);
    }
    return result == ALL_VALID;
}

std::string JointMatrixFuncsResolutionPass::GetLoadStoreMatrixFuncName
        (bool isLoad, unsigned operationLayout, unsigned address_space, const JointMatrixTypeDescription *desc)
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

    /* New version of the JointMatrix specification uses single value to
     * represent PackedA and PackedB layouts, named simply; 'Packed'. The value
     * of 'Packed' is equal to the value of legacy 'PackedA'. If we meet
     * load/store that tries to load/store packedA data into B matrix, we can
     * assume that the intended layout was PackedB (load of A into B would be illegal).
     * This should be removed when we stop to support the legacy version of the spec. */
    if (matrixLayout == LayoutPackedB && operationLayout == LayoutPackedA) {
        operationLayout = LayoutPackedB;
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
     * Additionally we use block 2d operations on PVC, so it's easier to
     * implement SG16 loads and stores as separate builtins. */
    if (m_Ctx->platform.hasExecSize16DPAS()) {
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

    if (address_space == ADDRESS_SPACE_GLOBAL) {
        name += "global_";
    } else if (address_space == ADDRESS_SPACE_LOCAL) {
        name += "local_";
    } else {
        name += "generic_";
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
    StringRef name = IGCLLVM::getNonOpaquePtrEltTy(ptrType)->getStructName();

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

static bool isMatrixType(const Type *type)
{
    if (!type->isPointerTy())
        return false;

    Type *eltType = IGCLLVM::getNonOpaquePtrEltTy(type);
    if (!eltType || !eltType->isStructTy())
        return false;

    StringRef name = eltType->getStructName();
    if (name.startswith("intel.joint_matrix"))
        return true;

    return false;
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
        if (desc.rows == 1)
            return baseType;
        return IGCLLVM::FixedVectorType::get(baseType, desc.rows);
    } else if (desc.layout == LayoutPackedB) {
        Type *baseType = Type::getInt32Ty(ctx);
        return IGCLLVM::FixedVectorType::get(baseType, 8);
    } else if (desc.layout == LayoutRowMajor) {
        Type *baseType = Type::getInt32Ty(ctx);
        if (desc.isFloating) {
            baseType = Type::getFloatTy(ctx);
        }
        if (desc.rows == 1)
            return baseType;
        return IGCLLVM::FixedVectorType::get(baseType, desc.rows);
    }

    IGC_ASSERT_EXIT_MESSAGE(false, "Failed to resolve matrix type.");
    return nullptr;
}

static uint64_t constIntValue(const Value *v) {
    return cast<ConstantInt>(v)->getLimitedValue();
}

static Type *getIntegerEquivalent(Type *matTy) {
    if (IGCLLVM::FixedVectorType *VT = dyn_cast<IGCLLVM::FixedVectorType>(matTy)) {
        unsigned elements = (unsigned) VT->getNumElements();
        unsigned size = VT->getElementType()->getScalarSizeInBits();
        Type *elementType = Type::getIntNTy(matTy->getContext(), size);
        if (elements >= 5 && elements < 8) {
            elements = 8;
        }
        return IGCLLVM::FixedVectorType::get(elementType, elements);
    } else {
        unsigned size = matTy->getScalarSizeInBits();
        return Type::getIntNTy(matTy->getContext(), size);
    }
}

template <class BuilderT>
static Instruction *shrinkExpandCastVector(BuilderT *builder,
                                           IGCLLVM::FixedVectorType *rawMatTy, Value *origVal)
{
    SmallVector<IGCLLVM::ShuffleVectorMaskType, 8> mask;
    llvm::copy(llvm::seq<IGCLLVM::ShuffleVectorMaskType>(
                   0,
                   (IGCLLVM::ShuffleVectorMaskType)rawMatTy->getNumElements()),
               std::back_inserter(mask));

    Value *newMat = builder->CreateShuffleVector(origVal,
                                                 UndefValue::get(origVal->getType()),
                                                 mask, origVal->getName() + ".shuffle");
    newMat = builder->CreateBitCast(newMat, rawMatTy, newMat->getName() + ".cast");
    return cast<Instruction>(newMat);
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
    unsigned address_space = ptrVal->getType()->getPointerAddressSpace();

    ValidateLoadStore(true, loadLayout, &desc, CI);
    std::string funcName = GetLoadStoreMatrixFuncName(true, loadLayout, address_space, &desc);
    FunctionType *funcType = FunctionType::get(retTy, { ptrVal->getType(), strideVal->getType() }, false);
    std::vector<Value *> Args = { ptrVal, strideVal };

    InstsToErase.insert(CI);

    IRBuilder builder(CI);
    Instruction *newCall = builder.CreateCall(M->getOrInsertFunction(funcName, funcType), Args, "matrix");
    if (retTy != matTy) {
        IGCLLVM::FixedVectorType *rawMatTy = dyn_cast<IGCLLVM::FixedVectorType>(matTy);
        IGCLLVM::FixedVectorType *rawRetTy = dyn_cast<IGCLLVM::FixedVectorType>(retTy);
        if (rawMatTy != nullptr && rawMatTy->getNumElements() < rawRetTy->getNumElements()) {
            newCall = shrinkExpandCastVector(&builder, rawMatTy, newCall);
        } else {
            Value *bitcast = builder.CreateBitCast(newCall, matTy, "matrix.load.cast");
            newCall = dyn_cast<Instruction>(bitcast);
        }
    }
    newCall->setDebugLoc(CI->getDebugLoc());
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
    IRBuilder builder(CI);

    Value *matVal = Resolve(matrixVal);
    if (matVal->getType() != matTy) {
      IGCLLVM::FixedVectorType *rawMatTy =
          dyn_cast<IGCLLVM::FixedVectorType>(matTy);
      IGCLLVM::FixedVectorType *rawArgTy =
          dyn_cast<IGCLLVM::FixedVectorType>(matVal->getType());

      if (rawMatTy != nullptr &&
          rawArgTy->getNumElements() < rawMatTy->getNumElements()) {
        matVal = shrinkExpandCastVector(&builder, rawMatTy, matVal);
      } else {
        matVal = BitCastInst::Create(Instruction::BitCast, matVal, matTy,
                                     "matrix.store.cast", CI);
      }
    }

    unsigned address_space = ptrVal->getType()->getPointerAddressSpace();

    ValidateLoadStore(false, storeLayout, &desc, CI);
    std::string funcName = GetLoadStoreMatrixFuncName(false, storeLayout, address_space, &desc);
    FunctionType *funcType =
        FunctionType::get(Type::getVoidTy(M->getContext()),
            { ptrVal->getType(), matTy, strideVal->getType() }, false);
    std::vector<Value *> Args = { ptrVal, matVal, strideVal };

    InstsToErase.insert(CI);
    Instruction *newCall = CallInst::Create(M->getOrInsertFunction(funcName, funcType), Args, "", CI);
    newCall->setDebugLoc(CI->getDebugLoc());
    return newCall;
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
    dpasCall->setDebugLoc(CI->getDebugLoc());

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

static int getSliceSize(const JointMatrixTypeDescription *desc, Type *matTy) {
    IGCLLVM::FixedVectorType *ty = dyn_cast<IGCLLVM::FixedVectorType>(matTy);
    IGC_ASSERT_MESSAGE(ty, "Expecting vector type in calculating slice size");

    unsigned contribTypeWidth = ty->getElementType()->getScalarSizeInBits();
    if (desc->layout == LayoutRowMajor) {
        return desc->rows;
    }
    if (desc->bitWidth != 0) {
        if (desc->layout == LayoutPackedA) {
            return desc->rows * (contribTypeWidth / desc->bitWidth);
        }
        if (desc->layout == LayoutPackedB) {
            return 8 * (contribTypeWidth / desc->bitWidth);
        }
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
    const int sliceSize = getSliceSize(&desc, matTy);
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

    if (fillValue->getType()->isPointerTy())
    {
        IntegerType *vectorElementType = dyn_cast<IntegerType>(getResolvedVectorElementType(matTy));
        PointerType *PT = dyn_cast<PointerType>(fillValue->getType());
        fillValue = builder.CreateBitCast(fillValue, PointerType::get(vectorElementType, PT->getAddressSpace()));
        fillValue = builder.CreateLoad(vectorElementType, fillValue);
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
    Type *matTy = ResolveType(CI->getArgOperand(0)->getType(), &desc);

    const int sliceSize = getSliceSize(&desc, matTy);
    Value *lenght = ConstantInt::get(CI->getType(), sliceSize, "matrix.slice.size");

    CI->replaceAllUsesWith(lenght);
    InstsToErase.insert(CI);
    return lenght;
}

template <class BuilderT>
static Value *createSliceExtract
      (BuilderT *builder, Value *matrix, Value *index, const JointMatrixTypeDescription *desc, Type *matTy) {
    const int sliceSize = getSliceSize(desc, matTy);
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
    const int sliceSize = getSliceSize(&desc, rawMatTy);
    const int vectorSize = getResolvedVectorSize(matTy);

    Value *slice = nullptr;
    if (sliceSize > vectorSize) {
        Value *element = createSliceExtract(&builder, matrix, index, &desc, rawMatTy);
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

    IntegerType *vectorElementType = dyn_cast<IntegerType>(getResolvedVectorElementType(rawMatTy));
    component = builder.CreateBitCast(component, vectorElementType);

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
    Value *element = createSliceExtract(&builder, matrix, index, &desc, matTy);
    /* Unpacking: */
    const int sliceSize = getSliceSize(&desc, matTy);
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

    // We need the bitcast, especially for half, as the function call that is
    // being replaced has a half return type and the vectorElementType is i16
    element = builder.CreateBitCast(element, CI->getType());

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
        type = ResolveTypes(v->getType(), nullptr);
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
    IGC_ASSERT_MESSAGE(func, "Unexpected missing function.");
    if (!func)
        return nullptr;

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

void JointMatrixFuncsResolutionPass::CacheResolvedTypes(Type *oldType, Type *newType)
{
    IGC_ASSERT_MESSAGE(newType, "Type should not be null.");
    if (newType == nullptr)
        return;
    ResolvedTypes[oldType] = newType;
}

Value *JointMatrixFuncsResolutionPass::ResolveLLVMLoad(LoadInst *LI)
{
    InsertPlaceholder(LI);
    Type *type = ResolveTypes(LI->getType(), nullptr);
    Value *operand = Resolve(LI->getPointerOperand());

    LoadInst *NewLI = new LoadInst(type, operand, "matrix.load.arr", LI->isVolatile(), IGCLLVM::getAlign(*LI), LI);

    NewLI->setDebugLoc(LI->getDebugLoc());
    CacheResolvedValue(LI, NewLI);
    InstsToErase.insert(LI);

    return NewLI;
}

Value *JointMatrixFuncsResolutionPass::ResolveLLVMStore(StoreInst *SI)
{
    InsertPlaceholder(SI);
    StoreInst *newSI = new StoreInst(Resolve(SI->getValueOperand()),
                                     Resolve(SI->getPointerOperand()),
                                     SI->isVolatile(), IGCLLVM::getAlign(*SI),
                                     SI);
    newSI->setDebugLoc(SI->getDebugLoc());
    CacheResolvedValue(SI, newSI);
    InstsToErase.insert(SI);

    return newSI;
}

Type *JointMatrixFuncsResolutionPass::ResolveTypes(llvm::Type *t, bool *isJointMatrixOp)
{
    IGC_ASSERT_MESSAGE(isJointMatrixOp == nullptr || (*isJointMatrixOp) == false, "Expected either nullptr or false value.");

    if (StructType *ST = dyn_cast<StructType>(t))
        return ResolveStructType(ST, isJointMatrixOp);
    else if (ArrayType *AT = dyn_cast<ArrayType>(t))
        return ResolveArrayType(AT, isJointMatrixOp);
    else if (PointerType *PT = dyn_cast<PointerType>(t))
    {
        if (isMatrixType(t))
        {
            if (isJointMatrixOp != nullptr)
                *isJointMatrixOp = true;
            return ResolveType(t, nullptr);
        }
        else
            return ResolvePointerType(PT, isJointMatrixOp);
    }
    return t;
}

Value *JointMatrixFuncsResolutionPass::ResolveGetElementPtr(GetElementPtrInst *GEPI)
{
    InsertPlaceholder(GEPI);
    std::vector<Value *> IdxList(GEPI->idx_begin(), GEPI->idx_end());
    GetElementPtrInst *NewGEPI = GetElementPtrInst::Create(
        ResolveTypes(GEPI->getSourceElementType(), nullptr),
        Resolve(GEPI->getPointerOperand()), IdxList, GEPI->getName(), GEPI);
    NewGEPI->setIsInBounds(GEPI->isInBounds());
    NewGEPI->setDebugLoc(GEPI->getDebugLoc());

    CacheResolvedValue(GEPI, NewGEPI);
    InstsToErase.insert(GEPI);
    return NewGEPI;
}

Value *JointMatrixFuncsResolutionPass::ResolveAlloca(AllocaInst *AI)
{
    InsertPlaceholder(AI);
    AllocaInst *newAI = new AllocaInst(
        ResolveTypes(AI->getAllocatedType(), nullptr),
        AI->getType()->getAddressSpace(), AI->getName(), AI);
    newAI->setAlignment(IGCLLVM::getAlign(*AI));
    newAI->setDebugLoc(AI->getDebugLoc());

    CacheResolvedValue(AI, newAI);
    InstsToErase.insert(AI);
    return newAI;
}

Value *JointMatrixFuncsResolutionPass::ResolveBitCast(BitCastInst *BCI)
{
    InsertPlaceholder(BCI);
    BitCastInst *newBCI = new BitCastInst(Resolve(BCI->getOperand(0)),
                                          ResolveTypes(BCI->getDestTy(), nullptr),
                                          BCI->getName(), BCI);
    newBCI->setDebugLoc(BCI->getDebugLoc());

    CacheResolvedValue(BCI, newBCI);
    InstsToErase.insert(BCI);
    return newBCI;
}

Value *JointMatrixFuncsResolutionPass::ResolveAddrSpaceCast(AddrSpaceCastInst *ASCI)
{
    InsertPlaceholder(ASCI);
    AddrSpaceCastInst *newASCI = new AddrSpaceCastInst(Resolve(ASCI->getOperand(0)),
                                          ResolveTypes(ASCI->getDestTy(), nullptr),
                                          ASCI->getName(), ASCI);
    newASCI->setDebugLoc(ASCI->getDebugLoc());

    CacheResolvedValue(ASCI, newASCI);
    InstsToErase.insert(ASCI);
    return newASCI;
}

Value *JointMatrixFuncsResolutionPass::ResolvePtrToInt(PtrToIntInst *PTII)
{
    InsertPlaceholder(PTII);
    PtrToIntInst *newPTII = new PtrToIntInst(Resolve(PTII->getPointerOperand()),
                                             PTII->getDestTy(), PTII->getName(),
                                             PTII);
    newPTII->setDebugLoc(PTII->getDebugLoc());

    CacheResolvedValue(PTII, newPTII);
    InstsToErase.insert(PTII);
    return newPTII;
}

Type *JointMatrixFuncsResolutionPass::ResolveStructType(Type *oldType, bool *isJointMatrixOp)
{
    IGC_ASSERT_MESSAGE(isJointMatrixOp == nullptr || (*isJointMatrixOp) == false, "Expected either nullptr or false value.");
    if (ResolvedTypes.count(oldType) > 0)
    {
        if (isJointMatrixOp != nullptr)
            *isJointMatrixOp = true;
        return ResolvedTypes[oldType];
    }

    StructType *structType = dyn_cast<StructType>(oldType);
    SmallVector<Type *, 1> elements;
    bool isJointMatrixOpLocal = false;
    llvm::transform(structType->elements(), std::back_inserter(elements), [&](Type *t)
                    {
                        bool isJointMatrixType = false;
                        Type *resolvedType = ResolveTypes(t, &isJointMatrixType);
                        if (isJointMatrixType)
                            isJointMatrixOpLocal = true;
                        return resolvedType; });

    if (!isJointMatrixOpLocal)
        return oldType;

    if (isJointMatrixOp != nullptr)
        *isJointMatrixOp = true;

    SmallString<28> name;
    Type *newType = StructType::create(elements, (structType->getName() + ".resolved").toStringRef(name));
    CacheResolvedTypes(oldType, newType);
    return newType;
}

Type *JointMatrixFuncsResolutionPass::ResolveArrayType(Type *oldType, bool *isJointMatrixOp)
{
    IGC_ASSERT_MESSAGE(isJointMatrixOp == nullptr || (*isJointMatrixOp) == false, "Expected either nullptr or false value.");
    if (ResolvedTypes.count(oldType) > 0)
    {
        if (isJointMatrixOp != nullptr)
            *isJointMatrixOp = true;
        return ResolvedTypes[oldType];
    }

    ArrayType *arrayType = dyn_cast<ArrayType>(oldType);
    bool isJointMatrixOpLocal = false;
    Type *resolvedType = ResolveTypes(arrayType->getElementType(), &isJointMatrixOpLocal);
    if (!isJointMatrixOpLocal)
        return oldType;

    if (isJointMatrixOp != nullptr)
        *isJointMatrixOp = true;

    Type *newType = ArrayType::get(resolvedType, arrayType->getNumElements());
    CacheResolvedTypes(oldType, newType);
    return newType;
}

Type *JointMatrixFuncsResolutionPass::ResolvePointerType(Type *oldType, bool *isJointMatrixOp)
{
    IGC_ASSERT_MESSAGE(isJointMatrixOp == nullptr || (*isJointMatrixOp) == false, "Expected either nullptr or false value.");

    if (ResolvedTypes.count(oldType) > 0)
    {
        if (isJointMatrixOp != nullptr)
            *isJointMatrixOp = true;
        return ResolvedTypes[oldType];
    }

    PointerType *ptrType = dyn_cast<PointerType>(oldType);
    bool isJointMatrixOpLocal = false;
    Type *resolvedType = ResolveTypes(IGCLLVM::getNonOpaquePtrEltTy(ptrType), &isJointMatrixOpLocal);
    if (!isJointMatrixOpLocal)
        return oldType;

    if (isJointMatrixOp != nullptr)
        *isJointMatrixOp = true;

    Type *newType = PointerType::get(resolvedType, ptrType->getAddressSpace());
    CacheResolvedTypes(oldType, newType);
    return newType;
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

        bool isJointMatrixOp = false;
        Type *type = ResolveTypes(v->getType(), &isJointMatrixOp);
        if (!isJointMatrixOp)
            return nullptr;

        PHINode *NewPN = PHINode::Create(type, IncomingCount, PN->getName(), PN);
        NewPN->setDebugLoc(PN->getDebugLoc());
        CacheResolvedValue(v, NewPN);

        for (unsigned i = 0; i < IncomingCount; i++) {
            Value *oldOperand = PN->getIncomingValue(i);
            Value *operand = Resolve(oldOperand);
            NewPN->addIncoming(operand, PN->getIncomingBlock(i));
        }

        InstsToErase.insert(PN);
        return NewPN;
    } else if (LoadInst *LI = dyn_cast<LoadInst>(v)) {
        return ResolveLLVMLoad(LI);
    } else if (GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(v)) {
        return ResolveGetElementPtr(GEPI);
    } else if (AllocaInst *AI = dyn_cast<AllocaInst>(v)) {
        return ResolveAlloca(AI);
    } else if (AddrSpaceCastInst *ASCI = dyn_cast<AddrSpaceCastInst>(v)){
        return ResolveAddrSpaceCast(ASCI);
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

void JointMatrixFuncsResolutionPass::visitAllocaInst(AllocaInst &I)
{
    if (ResolvedValues.count(&I) > 0)
        return;

    bool isJointMatrixOp = false;
    ResolveTypes(I.getAllocatedType(), &isJointMatrixOp);
    if (!isJointMatrixOp)
        return;

    ResolveAlloca(&I);
}

void JointMatrixFuncsResolutionPass::visitGetElementPtrInst(GetElementPtrInst &I)
{
    if (ResolvedValues.count(&I) > 0)
        return;

    bool isJointMatrixOp = false;
    ResolveTypes(I.getSourceElementType(), &isJointMatrixOp);
    if (!isJointMatrixOp)
        return;

    ResolveGetElementPtr(&I);
}

void JointMatrixFuncsResolutionPass::visitStoreInst(StoreInst &I)
{
    if (ResolvedValues.count(&I) > 0)
        return;

    Value *val = I.getValueOperand();
    bool isJointMatrixOp = false;
    ResolveTypes(val->getType(), &isJointMatrixOp);
    if (isJointMatrixOp)
    {
        ResolveLLVMStore(&I);
        return;
    }

    // In cases when Joint Matrix is used in arrays, front end sometimes
    // inserts pointer manipulations, which are incorrect for
    // pointers to matrix types. Hence, need to remove ptrtoint
    // instruciton, which becomes invalid after matrix type resolution.
    // For example, before resolution, this is valid:
    // %59 = ptrtoint %intel.joint_matrix_acc_8x16_f32_t addrspace(1)* %23 to i64
    // After resolution, this is invalid:
    // %59 = ptrtoint <8 x float> %23 to i64
    // Since this value is used in store, we need to replace it's usage in store.
    // The same for bitcast which is done for the Ptr value, where the val is stored.
    PtrToIntInst *PTI = dyn_cast<PtrToIntInst>(val);
    BitCastInst *BC = dyn_cast<BitCastInst>(I.getPointerOperand());

    if (PTI == nullptr || !isMatrixType(PTI->getPointerOperand()->getType()) ||
        !PTI->getDestTy()->isIntegerTy(64) || BC == nullptr)
        return;

    InsertPlaceholder(&I);
    Value *PTIOperand = PTI->getOperand(0);
    Type *newBCElementType = ResolveTypes(
        dyn_cast<PointerType>(PTIOperand->getType()), nullptr);
    PointerType *BCDstType = dyn_cast<PointerType>(BC->getDestTy());

    BitCastInst *newBC = new BitCastInst(Resolve(BC->getOperand(0)),
                                         PointerType::get(newBCElementType,
                                                          BCDstType->getPointerAddressSpace()),
                                         BC->getName(), BC);

    StoreInst *newSI = new StoreInst(Resolve(PTIOperand),
                                     newBC,
                                     I.isVolatile(), IGCLLVM::getAlign(I),
                                     &I);

    newSI->setDebugLoc(I.getDebugLoc());
    CacheResolvedValue(&I, newSI);
    InstsToErase.insert(&I);

    // Remove incorrect PtrToInt and BitCast instructions
    InstsToErase.insert(PTI);
    InstsToErase.insert(BC);
}

void JointMatrixFuncsResolutionPass::visitBitCastInst(BitCastInst &I)
{
    // In cases when Joint Matrix is used in arrays, front end sometimes
    // inserts pointer manipulations, which are incorrect for
    // pointers to matrix types. Hence, need to remove bitcast
    // instruciton, which becomes invalid after matrix type resolution.
    // Example:
    // %25 = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix.11"* %arrayidx50113.i to i64*
    PointerType *srcPtr = dyn_cast<PointerType>(I.getSrcTy());
    PointerType *dstPtr = dyn_cast<PointerType>(I.getDestTy());
    if (srcPtr != nullptr && dstPtr != nullptr)
    {
        Type *srcPtrType = IGCLLVM::getNonOpaquePtrEltTy(srcPtr);
        Type *dstPtrType = IGCLLVM::getNonOpaquePtrEltTy(dstPtr);

        StructType *srcStructType = dyn_cast<StructType>(srcPtrType);
        if (srcStructType != nullptr)
        {
            Type *srcElemType = srcStructType->getElementType(0);
            if (srcStructType->getNumElements() == 1 && isMatrixType(srcElemType) &&
                dstPtrType->isIntegerTy(64))
                return;
        }
    }

    if (ResolvedValues.count(&I) > 0)
        return;

    bool isSrcJointMatrixType = false;
    ResolveTypes(I.getSrcTy(), &isSrcJointMatrixType);
    bool isDstJointMatrixType = false;
    ResolveTypes(I.getDestTy(), &isDstJointMatrixType);
    if (!isSrcJointMatrixType && !isDstJointMatrixType)
        return;

    ResolveBitCast(&I);
}

void JointMatrixFuncsResolutionPass::visitPtrToIntInst(PtrToIntInst &I)
{
    // In cases when Joint Matrix is used in arrays, front end sometimes
    // inserts pointer manipulations, which are incorrect for
    // pointers to matrix types. Hence, need to remove ptrtoint
    // instruciton, which becomes invalid after matrix type resolution.
    // For example, before resolution, this is valid:
    // %59 = ptrtoint %intel.joint_matrix_acc_8x16_f32_t addrspace(1)* %23 to i64
    // After resolution, this is invalid:
    // %59 = ptrtoint <8 x float> %23 to i64
    // Here we just ignore this ptrtoint instruction. We will replace the use of value
    // it returns in visitStoreInst and then remove it.
    if (isMatrixType(I.getPointerOperand()->getType()) &&
        I.getDestTy()->isIntegerTy(64))
        return;

    if (ResolvedValues.count(&I) > 0)
        return;

    bool isSrcJointMatrixType = false;
    ResolveTypes(I.getSrcTy(), &isSrcJointMatrixType);
    if (!isSrcJointMatrixType)
        return;

    ResolvePtrToInt(&I);
}
