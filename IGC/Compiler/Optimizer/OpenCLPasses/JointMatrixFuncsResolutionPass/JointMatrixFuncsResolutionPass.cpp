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
#include <llvmWrapper/ADT/Optional.h>
#include <llvmWrapper/Analysis/ValueTracking.h>

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

// We need module pass, since:
// 1) we inspect multiple functions to find entry function to get sub group size
// 2) we maintain map of functions to entry functions across functions we process
// so the pass is not local to one function.
JointMatrixFuncsResolutionPass::JointMatrixFuncsResolutionPass() : ModulePass(ID)
{
    initializeJointMatrixFuncsResolutionPassPass(*PassRegistry::getPassRegistry());
}

bool JointMatrixFuncsResolutionPass::runOnModule(Module &M)
{
    m_Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_mdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    FunctionsMap.clear();
    Changed = false;

    for (auto &F : M)
    {
        if (F.isDeclaration())
            continue;

        if (runOnFunction(F))
            Changed = true;
    }

    return Changed;
}

// Finds entry function for input function. If there are several entry
// functions, it will return only one (first found).
// So currently the case, when the same function which is using Joint Matrix
// is called from several kernels with different sub-group size required,
// not supported and behavior is undefined.
Function *JointMatrixFuncsResolutionPass::getEntryFunction(Function *F)
{
    if (FunctionsMap.count(F) > 0)
        return FunctionsMap[F];

    if (isEntryFunc(m_mdUtils, F))
    {
        FunctionsMap[F] = F;
        return F;
    }

    SmallVector<Function *, 8> toProcess;
    toProcess.push_back(F);
    SmallPtrSet<Function *, 8> toSetEntry;
    toSetEntry.insert(F);

    while (!toProcess.empty())
    {
        Function *curFunc = toProcess.pop_back_val();
        for (auto It = curFunc->use_begin(); It != curFunc->use_end(); It++)
        {
            auto user = It->getUser();
            if (!isa<CallInst>(user))
                continue;

            auto *CI = cast<CallInst>(user);
            if (CI->getCalledFunction() == curFunc)
            {
                auto caller = CI->getFunction();
                Function *entryFunc = nullptr;
                if (FunctionsMap.count(caller) > 0)
                    entryFunc = FunctionsMap[caller];
                if (!entryFunc && isEntryFunc(m_mdUtils, caller))
                    entryFunc = caller;
                if (entryFunc)
                {
                    for (auto *fToSetEntry : toSetEntry)
                        FunctionsMap[fToSetEntry] = entryFunc;
                    return entryFunc;
                }
                if (toSetEntry.count(caller) == 0) {
                    toProcess.push_back(caller);
                    toSetEntry.insert(caller);
                }
            }
        }
    }
    FunctionsMap[F] = nullptr;
    return nullptr;
}

int32_t JointMatrixFuncsResolutionPass::DetermineForcedSIMDSize()
{
    int32_t forcedSIMDSize = m_Ctx->getModuleMetaData()->csInfo.forcedSIMDSize;

    if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD32) && IGC_IS_FLAG_DISABLED(ForceCSSIMD16) && (forcedSIMDSize == 32 || IGC_IS_FLAG_ENABLED(ForceCSSIMD32)))
    {
        if (forcedSIMDSize == 0)
            m_Ctx->getModuleMetaData()->csInfo.forcedSIMDSize = 32;
        return 32;
    }

    if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD16) && IGC_IS_FLAG_DISABLED(ForceCSSIMD32) && (forcedSIMDSize == 16 || IGC_IS_FLAG_ENABLED(ForceCSSIMD16)))
    {
        if (forcedSIMDSize == 0)
            m_Ctx->getModuleMetaData()->csInfo.forcedSIMDSize = 16;
        return 16;
    }

    return forcedSIMDSize;
}

int32_t JointMatrixFuncsResolutionPass::DefineKernelSIMDSize()
{
    if (m_Ctx->platform.hasExecSize16DPAS())
    {
        if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD16) && IGC_IS_FLAG_DISABLED(ForceCSSIMD32))
            return 16;
        if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD32) && IGC_IS_FLAG_DISABLED(ForceCSSIMD16))
            return 32;
        std::string msg = "Sub group sizes supported by Joint Matrix for this platform are disabled by flags or non-supported sub group size forced.";
        m_Ctx->EmitError(msg.c_str(), nullptr);
        return 0;
    }
    if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD32) && IGC_IS_FLAG_ENABLED(ForceCSSIMD32))
    {
        std::string msg = "Sub group size 32 forced by flags but not supported by Joint Matrix on this platform.";
        m_Ctx->EmitError(msg.c_str(), nullptr);
        return 0;
    }
    if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD16) && IGC_IS_FLAG_ENABLED(ForceCSSIMD16))
    {
        std::string msg = "Sub group size 16 forced by flags but not supported by Joint Matrix on this platform.";
        m_Ctx->EmitError(msg.c_str(), nullptr);
        return 0;
    }
    return 8;
}

bool JointMatrixFuncsResolutionPass::IsSIMDSizeValid(int32_t simdSize)
{
    return ((m_Ctx->platform.hasExecSize16DPAS() && (simdSize == 16 || simdSize == 32)) ||
            (!m_Ctx->platform.hasExecSize16DPAS() && simdSize == 8));
}

void JointMatrixFuncsResolutionPass::ForceKernelSIMDSize(Function *F, int32_t forcedSIMDSize)
{
    Function *entryFunction = getEntryFunction(F);
    if (entryFunction) // if can find entry function
    {
        IGCMD::FunctionInfoMetaDataHandle funcInfoMD = m_mdUtils->getFunctionsInfoItem(entryFunction);
        IGCMD::SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
        subGroupSize->setSIMDSize(forcedSIMDSize);
    }
}

void JointMatrixFuncsResolutionPass::ResolveSIMDSize(Function *F)
{
    if (m_SIMDSize != 0)
        return;

    int32_t forcedSIMDSize = DetermineForcedSIMDSize();
    if (forcedSIMDSize != 0)
    {
        if (IsSIMDSizeValid(forcedSIMDSize))
        {
            m_SIMDSize = forcedSIMDSize;
            ForceKernelSIMDSize(F, m_SIMDSize);
            return;
        }
        // if forced and not ok for platform exit with error
        std::string msg = "Sub group size " + std::to_string(forcedSIMDSize) + " is forced by flags but not supported by Joint Matrix on this platform.";
        m_Ctx->EmitError(msg.c_str(), nullptr);
        return;
    }

    // if not forced by driver of flags, check on entry function level
    Function *entryFunction = getEntryFunction(F);
    if (entryFunction) // if can find entry function
    {
        IGCMD::FunctionInfoMetaDataHandle funcInfoMD = m_mdUtils->getFunctionsInfoItem(entryFunction);
        IGCMD::SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
        if (subGroupSize->hasValue())
        {
            int32_t kernelSIMDSize = subGroupSize->getSIMDSize();
            if (kernelSIMDSize != 0)
            {
                if (IsSIMDSizeValid(kernelSIMDSize))
                {
                    m_SIMDSize = kernelSIMDSize;
                    return;
                }
                // if set on entry function level and not ok for this platform exit with error
                std::string msg = "Sub group size " + std::to_string(kernelSIMDSize) + " is forced by attribute but not supported by Joint Matrix on this platform.";
                m_Ctx->EmitError(msg.c_str(), nullptr);
                return;
            }
        }
        // if not set on entry function level, define ourselves
        m_SIMDSize = DefineKernelSIMDSize();
        // and set to entry level function
        subGroupSize->setSIMDSize(m_SIMDSize);
        return;
    }

    // If no entry function found (it means that we could not detect that current function is called
    // from any kernel), we anyway will resolve function, just in case, using default sub group size.
    m_SIMDSize = DefineKernelSIMDSize();
    // Force SIMD size if not set, as Joint Matrix need it to define numer of elements in WI
    m_Ctx->getModuleMetaData()->csInfo.forcedSIMDSize = (unsigned char)m_SIMDSize;
}

bool JointMatrixFuncsResolutionPass::runOnFunction(Function& F)
{
    PlaceholderInstructions.clear();
    ResolvedValues.clear();
    ResolvedTypes.clear();
    InstsToErase.clear();
    m_SIMDSize = 0;

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

    return !ResolvedValues.empty();
}

static const char *JointMatrixBIPrefix = "__builtin_spirv_OpJointMatrix";
static const char *JointMatrixBISuffix = "JointMatrixINTEL_";
static const char *JointMatrixLoadPrefx  = "JointMatrixLoadINTEL";
static const char *JointMatrixStorePrefx = "JointMatrixStoreINTEL";
static const char *JointMatrixMadPrefx   = "JointMatrixMadINTEL";
static const char *JointMatrixSUMadPrefx = "JointMatrixSUMadINTEL";
static const char *JointMatrixUSMadPrefx = "JointMatrixUSMadINTEL";
static const char *JointMatrixUUMadPrefx = "JointMatrixUUMadINTEL";
static const char *JointMatrixFillPrefx  = "CompositeConstruct";
static const char *JointMatrixWorkItemLengthPrefx = "JointMatrixWorkItemLengthINTEL";
static const char *JointMatrixSliceInsert  = "VectorInsertDynamic";
static const char *JointMatrixSliceExtract = "VectorExtractDynamic";
static const char *JointMatrixGetCoordPrefx = "JointMatrixGetElementCoordINTEL";

enum {
    UseMatrixA = 0,
    UseMatrixB = 1,
    UseAccumulator = 2,
    UseMax,
};

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
    unsigned contribBitWidth = 0; // bit width of type used internally to store matrix elements
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
        params.bitWidth = 8 | 16 | 32;
        params.layouts = 1 << LayoutRowMajor;
    } else if (desc->layout == LayoutPackedB) {
        params.rows = maxSliceBitWidth / desc->bitWidth;
        params.columns = useSG16 ? 16 : 8;
        params.bitWidth = 8 | 16 | 32;
        params.layouts |= 1 << LayoutColumnMajor;
        params.layouts |= 1 << LayoutPackedB;
        params.layouts |= 1 << LayoutPackedA; /* PackedA means just packed in the new version of spec. */
        params.layouts |= 1 << LayoutRowMajor; /* for tf32 and VNNIed layouts */
    } else { /* accumulator */
        params.maxRows = maxSliceBitWidth / desc->bitWidth;
        params.columns = useSG16 ? 16 : 8;
        params.bitWidth = 8 | 32;
        params.layouts |= 1 << LayoutRowMajor;
        params.layouts |= 1 << LayoutColumnMajor;
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
    if (!platform->supportJointMatrixOCLExtension()) {
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

static bool isSupprtedLargeSlice(const JointMatrixTypeDescription *desc, bool useSG16) {
    if (!useSG16)
        return false;

    if (desc->layout == LayoutPackedA) {
        if (desc->rows == 16 && desc->columns == 16 && desc->bitWidth == 16)
            return true;
        if (desc->rows == 32 && desc->columns == 16 && desc->bitWidth == 16)
            return true;
    }

    if (desc->layout == LayoutPackedB) {
        if (desc->rows == 16 && desc->columns == 16 && desc->bitWidth == 16)
            return true;
        if (desc->rows == 16 && desc->columns == 64 && desc->bitWidth == 16)
            return true;
    }

    if (desc->layout == LayoutRowMajor) {
        if (desc->rows == 16 && desc->columns == 16 && desc->bitWidth == 32)
            return true;
        if (desc->rows == 32 && desc->columns == 64 && desc->bitWidth == 32)
            return true;
    }

    return false;
}

bool JointMatrixFuncsResolutionPass::ValidateLoadStore
        (bool isLoad, unsigned operationLayout, const JointMatrixTypeDescription *desc, llvm::Value *ctx) {
    if (isSupprtedLargeSlice(desc, m_Ctx->platform.hasExecSize16DPAS())) {
        return true;
    }
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

std::string JointMatrixFuncsResolutionPass::GetMatrixFuncName(
    bool isGetCoord, bool isLoad, unsigned operationLayout,
    unsigned address_space, const JointMatrixTypeDescription *desc,
    std::string prefix) {
    /* Treat row major matrices with types not supported by accumulators as
     * PackedA matrices. Both are in row major format. */
    unsigned matrixLayout = desc->layout;
    if (!isGetCoord && isLoad && matrixLayout == LayoutRowMajor &&
        desc->bitWidth <= 16) {
        matrixLayout = LayoutPackedA;
    }

    std::string name = std::move(prefix);

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

    if (!isGetCoord) {
        /* New version of the JointMatrix specification uses single value to
         * represent PackedA and PackedB layouts, named simply; 'Packed'. The
         * value of 'Packed' is equal to the value of legacy 'PackedA'. If we
         * meet load/store that tries to load/store packedA data into B matrix,
         * we can assume that the intended layout was PackedB (load of A into B
         * would be illegal). This should be removed when we stop to support the
         * legacy version of the spec. */

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
            IGC_ASSERT_MESSAGE(matrixLayout == operationLayout,
                               "Unexpected load/store layout.");
            name += "PackedB_";
            break;
        default:
            IGC_ASSERT_MESSAGE(false, "Unexpected load/store layout.");
        }
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
    name += "_i" + std::to_string(desc->bitWidth);

    // We are done creating the mangling for get_coord here()
    if (isGetCoord)
        return name;

    name += "_" + std::to_string(getNumRowsPerWI(desc));

    // Continue mangling for load and store
    if (address_space == ADDRESS_SPACE_GLOBAL) {
        name += "_global_";
    } else if (address_space == ADDRESS_SPACE_LOCAL) {
        name += "_local_";
    } else {
        name += "_generic_";
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
    const unsigned lenght = name.size();
    while (*offset < lenght && std::isdigit(name[*offset]) && count < BUFFER_SIZE) {
        buffer[count] = name[*offset];
        *offset += 1;
        count += 1;
    }
    buffer[count] = '\0';
    return std::stoi(buffer);
}

/* This function extracts metadata from JointMatrix type names. They use the
 * following convention: intel.joint_matrix_acc_8x8_i32_t */
bool JointMatrixFuncsResolutionPass::parseMatrixTypeNameLegacy(const Type *opaqueType, JointMatrixTypeDescription *outDescription) {
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
    } else {
        std::string msg = "Unexpected Joint Matrix type name: '" + name.str() + "', unknown layout.";
        m_Ctx->EmitError(msg.c_str(), nullptr);
        return false;
    }
    offset -= 1; /* Go back to the end of prefix. */
    outDescription->rows = parseNumber(name, &offset);

    offset += 1; /* Skip delimiter, 'x'. */
    outDescription->columns = parseNumber(name, &offset);

    offset += 1; /* Skip delimiter, '_' */
    outDescription->isFloating = name[offset] == 'f';

    offset += 1; /* Skip type specifier, [f|i] */
    outDescription->bitWidth = parseNumber(name, &offset);
    IGC_ASSERT_MESSAGE(outDescription->bitWidth == 8 ||
                           outDescription->bitWidth == 16 ||
                           outDescription->bitWidth == 32,
                       "Unexpected matrix element size.");

    return true;
}

/* %spirv.JointMatrixINTEL._int_8_16_3_3_2   --> C
 * %spirv.JointMatrixINTEL._char_8_32_0_3_0  --> A
 * %spirv.JointMatrixINTEL._char_32_16_2_3_1 --> B
 * type, rows, cols, layout, scope, use
 * */
bool JointMatrixFuncsResolutionPass::ParseMatrixTypeName(Type *opaqueType, JointMatrixTypeDescription *outDescription) {
    const PointerType *ptrType = cast<PointerType>(opaqueType);
    StringRef name = IGCLLVM::getNonOpaquePtrEltTy(ptrType)->getStructName();
    StringRef fullName = name;

    if (name.startswith("intel.joint_matrix")) {
        return parseMatrixTypeNameLegacy(opaqueType, outDescription);
    }

    if (!name.consume_front("spirv.JointMatrixINTEL._")) {
        std::string msg = "Unexpected Joint Matrix type name: '"
                        + fullName.str() + "', unknown prefix.";
        m_Ctx->EmitError(msg.c_str(), nullptr);
        return false;
    }

    if (name.consume_front("int_")) {
        outDescription->bitWidth = 32;
        outDescription->isFloating = false;
    } else if (name.consume_front("short_")) {
        outDescription->bitWidth = 16;
        outDescription->isFloating = false;
    } else if (name.consume_front("char_")) {
        outDescription->bitWidth = 8;
        outDescription->isFloating = false;
    } else if (name.consume_front("float_")) {
        outDescription->bitWidth = 32;
        outDescription->isFloating = true;
    } else if (name.consume_front("half_")) {
        outDescription->bitWidth = 16;
        outDescription->isFloating = true;
    } else {
        std::string msg = "Unexpected Joint Matrix type name: '"
                        + fullName.str() + "', unknown element type.";
        m_Ctx->EmitError(msg.c_str(), nullptr);
        return false;
    }

    unsigned offset = 0;
    outDescription->rows = parseNumber(name, &offset);
    offset += 1; /* Skip delimiter, '_'. */
    outDescription->columns = parseNumber(name, &offset);
    offset += 1; /* Skip delimiter, '_' */
    unsigned legacyLayout = parseNumber(name, &offset);
    offset += 1; /* Skip delimiter, '_' */
    unsigned scope = parseNumber(name, &offset);

    /* Use parameter might not be present in older version of SPIR-V. In such
     * case it should be reconstructed from the layout. Handling of this
     * special case should be removed once we stop to support legacy SPIR-V
     * specification.*/
    unsigned use = UseMax;
    if (offset < name.size()) {
        offset += 1; /* Skip delimiter, '_' */
        use = parseNumber(name, &offset);
    } else {
        /* If use parameter is not present deduce the correct use from legacy
         * layout: */
        if (legacyLayout == LayoutPackedA) {
            use = UseMatrixA;
        } else if (legacyLayout == LayoutPackedB) {
            use = UseMatrixB;
        } else {
            use = UseAccumulator;
        }
    }

    /* currently unused: */
    (void)scope;

    if (use == UseMatrixA) {
        outDescription->layout = LayoutPackedA;
    } else if (use == UseMatrixB) {
        outDescription->layout = LayoutPackedB;
    } else if (use == UseAccumulator) {
        outDescription->layout = LayoutRowMajor;
    } else {
        std::string msg = "Unexpected Joint Matrix type name: '"
                        + fullName.str() + "', unknown use type.";
        m_Ctx->EmitError(msg.c_str(), nullptr);
        return false;
    }
    return true;
}

static bool isMatrixType(const Type *type)
{
    if (!type->isPointerTy())
        return false;

    Type *eltType = IGCLLVM::getNonOpaquePtrEltTy(type);
    if (!eltType || !eltType->isStructTy())
        return false;

    if (cast<StructType>(eltType)->isLiteral())
        return false;

    StringRef name = eltType->getStructName();
    if (name.startswith("intel.joint_matrix") || name.startswith("spirv.JointMatrixINTEL._"))
        return true;

    return false;
}

static void PreOrderTypeTraversal(Type *t, std::unordered_set<Type *> &set)
{
    if (set.find(t) != set.end())
        return;

    if (StructType *ST = dyn_cast<StructType>(t))
    {
        set.insert(t);
        for (auto subT : ST->elements())
            PreOrderTypeTraversal(subT, set);
        return;
    }

    if (ArrayType *AT = dyn_cast<ArrayType>(t))
    {
        set.insert(t);
        return PreOrderTypeTraversal(AT->getElementType(), set);
    }

    if (PointerType *PT = dyn_cast<PointerType>(t))
    {
        set.insert(t);
        return PreOrderTypeTraversal(IGCLLVM::getNonOpaquePtrEltTy(PT), set);
    }

    return;
}

static Type* getContainedMatrixType(Type *root)
{
    std::unordered_set<Type *> set;
    PreOrderTypeTraversal(root, set);

    for (auto &t : set)
    {
        if (isMatrixType(t))
            return t;
    }

    return nullptr;
}

static bool isOrContainsMatrixType(Type *root)
{
    return getContainedMatrixType(root) != nullptr;
}

// As both float and tf32 types are represented as float, the TF32 type info
// is lost starting at the SPIRV level. Therefore, we use the following
// heuristics for determing the TF32 type.  We know that the K value is fixed
// for a given platform, matrix and data type. Also TF32 is supported starting
// from PVC.  Therefore we use the K == 8, along with data type check to
// determine the TF32 type.  For float the K value is 16 (hence this function
// will return false).
static bool isTF32(const JointMatrixTypeDescription *desc) {
    bool matATF32 = desc->layout == LayoutPackedA && desc->isFloating &&
                    desc->bitWidth == 32 && desc->columns == 8;
    bool matBTF32 = desc->layout == LayoutPackedB && desc->isFloating &&
                    desc->bitWidth == 32 && desc->rows == 8;
    if (matATF32 || matBTF32)
        return true;
    return false;
}

unsigned JointMatrixFuncsResolutionPass::getNumRowsPerWI(const JointMatrixTypeDescription *desc) {
    IGC_ASSERT_MESSAGE(m_SIMDSize > 0, "Unexpected sub group size.");
    IGC_ASSERT_MESSAGE(desc->contribBitWidth > 0, "Unexpected bit width of contribution type.");
    unsigned totalBits = desc->rows * desc->columns * desc->bitWidth;
    unsigned canHandleBits = desc->contribBitWidth * m_SIMDSize;
    return totalBits % canHandleBits ? totalBits / canHandleBits + 1 : totalBits / canHandleBits;
}

// Create <i64 x 32> type used for Accumulator 32x64 in array [2 x <i64 x 32>].
static Type* getAcc32x64HalfType(LLVMContext &ctx) {
    return IGCLLVM::FixedVectorType::get(Type::getInt64Ty(ctx), 32);
}

// Check if it is one off special case: Accumulator 32x64.
static bool isAccumulator32x64(const JointMatrixTypeDescription &desc) {
    return (desc.layout == LayoutRowMajor && desc.rows == 32 && desc.columns == 64);
}

Type *JointMatrixFuncsResolutionPass::ResolveType(Type *opaqueType, JointMatrixTypeDescription *outDesc)
{
    IGC_ASSERT_EXIT_MESSAGE(opaqueType && opaqueType->isPointerTy(),
        "Unexpected type in matrix function resolution.");

    JointMatrixTypeDescription desc;
    bool parseResult = ParseMatrixTypeName(opaqueType, &desc);
    IGC_ASSERT_EXIT_MESSAGE(parseResult, "Failed to parse matrix type.");
    /* Treat row major matrices with types not supported by accumulators as
     * PackedA matrices. Both are in row major format. */
    if (desc.layout == LayoutRowMajor && desc.bitWidth <= 16) {
        desc.layout = LayoutPackedA;
    }

    LLVMContext &ctx = opaqueType->getContext();
    Type *resolvedType = nullptr;

    // One off special case: this should ideally be a vector of <i32 x 128>. However since IGC
    // code gen supports vector operations only on vectors up to 32
    // entries, we model this slice as array of [2 x <i64 x 32>].
    if (isAccumulator32x64(desc)) {
        desc.contribBitWidth = 32; // we still consider contrib bit width to be 32

        if (outDesc != nullptr)
            *outDesc = desc;

        resolvedType = ArrayType::get(getAcc32x64HalfType(ctx), 2);
        CacheResolvedTypes(opaqueType, resolvedType);
        return resolvedType;
    }

    // The rest follows common rules for type resolution
    Type *baseType = Type::getInt32Ty(ctx);
    desc.contribBitWidth = 32;
    if (desc.layout == LayoutPackedA) {
        if (!isTF32(&desc) && m_Ctx->platform.hasExecSize16DPAS()) {
            baseType = Type::getInt16Ty(ctx);
            desc.contribBitWidth = 16;
        }
    } else if (desc.layout == LayoutRowMajor && desc.isFloating) {
        baseType = Type::getFloatTy(ctx);
    }

    if (outDesc != nullptr)
        *outDesc = desc;

    unsigned vectorSize = getNumRowsPerWI(&desc);
    if (vectorSize == 1) {
        resolvedType = baseType;
    } else {
        resolvedType = IGCLLVM::FixedVectorType::get(baseType, vectorSize);
    }

    IGC_ASSERT_EXIT_MESSAGE(resolvedType != nullptr, "Failed to resolve matrix type.");

    CacheResolvedTypes(opaqueType, resolvedType);
    return resolvedType;
}

static uint64_t constIntValue(const Value *v) {
    return cast<ConstantInt>(v)->getLimitedValue();
}

// create value [2 x type] with val0 and val1 as values of each element
template <class BuilderT>
static Value *createPair(BuilderT *builder, Type* type, Value* val0, Value* val1) {
    Value *pair = UndefValue::get(ArrayType::get(type, 2));
    pair = builder->CreateInsertValue(pair, val0, {0});
    pair = builder->CreateInsertValue(pair, val1, {1});
    return pair;
}

template <class BuilderT>
static Instruction *loadSlice(BuilderT *builder, Type *matTy, Value *sliceArray) {
    IGCLLVM::FixedVectorType *sliceTy = dyn_cast<IGCLLVM::FixedVectorType>(matTy);
    if ((sliceTy && sliceTy->getNumElements() <= 32)
          || matTy->isIntegerTy() || matTy->isFloatingPointTy()) {
        return builder->CreateLoad(matTy, sliceArray);
    } else if (matTy->isArrayTy() && matTy->getArrayNumElements() == 2) {
        Type *halfTy = getAcc32x64HalfType(builder->getContext());
        Type *halfPtrTy = halfTy->getPointerTo(ADDRESS_SPACE_PRIVATE);

        Value *ptr0 = builder->CreateBitCast(sliceArray, halfPtrTy);
        Value *slice0 = builder->CreateLoad(halfTy, ptr0);

        Value *ptr1 = builder->CreateGEP(halfTy, ptr0, { builder->getInt32(1) });
        Value *slice1 = builder->CreateLoad(halfTy, ptr1);

        return dyn_cast<Instruction>(createPair(builder, halfTy, slice0, slice1));
    }

    IGC_ASSERT_MESSAGE(false, "Unexpected number of elements in matrix slice.");
    return nullptr;
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
    LLVMContext &ctx = CI->getContext();
    Type *retTy = Type::getVoidTy(ctx);
    Type *arrayTy = Type::getInt8PtrTy(ctx, ADDRESS_SPACE_PRIVATE);

    Module *M = CI->getParent()->getModule();
    unsigned address_space = ptrVal->getType()->getPointerAddressSpace();

    ValidateLoadStore(true, loadLayout, &desc, CI);
    std::string funcName =
        GetMatrixFuncName(false, true, loadLayout, address_space, &desc,
                          "__builtin_spriv_OpJointMatrixLoadINTEL_");
    FunctionType *funcType = FunctionType::get(retTy, { arrayTy, ptrVal->getType(), strideVal->getType() }, false);

    InstsToErase.insert(CI);

    // Create alloca in the entry node of the function
    IRBuilder<> builder(&*CI->getFunction()->getEntryBlock().getFirstInsertionPt());
    builder.SetCurrentDebugLocation(CI->getDebugLoc());
    Value *sliceArray = builder.CreateAlloca(matTy, ADDRESS_SPACE_PRIVATE);

    builder.SetInsertPoint(CI);
    Value *dst = builder.CreateBitCast(sliceArray, arrayTy);

    std::vector<Value *> Args = { dst, ptrVal, strideVal };
    Instruction *newCall = builder.CreateCall(M->getOrInsertFunction(funcName, funcType), Args);
    newCall->setDebugLoc(CI->getDebugLoc());

    newCall = loadSlice(&builder, matTy, sliceArray);

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
    (void)matTy;
    /* Cast floating types to integer types of the same size. This allows to
     * have a single set of store builtins for floats and integers */

    LLVMContext &ctx = CI->getContext();
    Type *arrayTy = Type::getInt8PtrTy(ctx, ADDRESS_SPACE_PRIVATE);

    Module *M = CI->getParent()->getModule();

    Value *matVal = Resolve(matrixVal);

    unsigned address_space = ptrVal->getType()->getPointerAddressSpace();

    ValidateLoadStore(false, storeLayout, &desc, CI);
    std::string funcName =
        GetMatrixFuncName(false, false, storeLayout, address_space, &desc,
                          "__builtin_spriv_OpJointMatrixStoreINTEL_");
    FunctionType *funcType =
        FunctionType::get(Type::getVoidTy(M->getContext()),
            { ptrVal->getType(), arrayTy, strideVal->getType() }, false);

    InstsToErase.insert(CI);

    // Create alloca in the entry node of the function
    IRBuilder<> builder(&*CI->getFunction()->getEntryBlock().getFirstInsertionPt());
    builder.SetCurrentDebugLocation(CI->getDebugLoc());
    Value *sliceArray = builder.CreateAlloca(matVal->getType(), ADDRESS_SPACE_PRIVATE);

    builder.SetInsertPoint(CI);
    builder.CreateStore(matVal, sliceArray);
    Value *src = builder.CreateBitCast(sliceArray, arrayTy);

    std::vector<Value *> Args = { ptrVal, src, strideVal };
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
  if (floatOp && width == 32) {
      return PrecisionType::TF32;
  }
  if (!floatOp && width == 8) {
      return isUnsigned ? PrecisionType::U8 : PrecisionType::S8;
  }
  return PrecisionType::PRECISION_UNUSED;
}

static const char *getElementName(PrecisionType P) {
    switch (P) {
        case PrecisionType::FP16: return "fp16_";
        case PrecisionType::BF16: return "bf16_";
        case PrecisionType::U8: return "u8_";
        case PrecisionType::S8: return "s8_";
        default: return "i32_";
    };
}

static bool isMADSupportedAsBuiltin(unsigned M, unsigned N, unsigned K) {
    if (M == 16 && N == 16 && K == 16)
        return true;
    if (M == 32 && N == 64 && K == 16)
        return true;
    return false;
}

static std::string getMADBuiltinName
        (unsigned M, unsigned N, unsigned K, PrecisionType PA, PrecisionType PB, bool isFloating) {
    std::string funcName = "__builtin_spriv_OpJointMatrixMadINTEL_";
    funcName += std::to_string(M) + "x" + std::to_string(N) + "x" + std::to_string(K) + "_";

    funcName += getElementName(PA);
    funcName += getElementName(PB);

    if (isFloating) {
        funcName += "fp32";
    } else {
        funcName += "i32";
    }

    return funcName;
}

static Function *getMADBuiltin(Module *Mod, unsigned M, unsigned N, unsigned K, PrecisionType PA, PrecisionType PB, bool isFloating) {
    std::string funcName = getMADBuiltinName(M, N, K, PA, PB, isFloating);

    Type *retTy = Type::getVoidTy(Mod->getContext());
    Type *argTy = Type::getInt8PtrTy(Mod->getContext(), ADDRESS_SPACE_PRIVATE);

    FunctionType *funcType =
        FunctionType::get(retTy, { argTy, argTy, argTy, argTy }, false);

    Function *f = Mod->getFunction(funcName);
    if (f == nullptr) {
        f = Function::Create(funcType, GlobalValue::ExternalLinkage, funcName, Mod);
        f->setCallingConv(CallingConv::SPIR_FUNC);
        f->addFnAttr(llvm::Attribute::NoUnwind);
    }
    return f;
}

Instruction *JointMatrixFuncsResolutionPass::ResolveMad(CallInst *CI, unsigned OperationType) {
    if (!m_Ctx->platform.supportDpasInstruction())
    {
        std::string msg = "OpJointMatrixMadINTEL is not supported on this platform!";
        m_Ctx->EmitError(msg.c_str(), CI);
    }

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

    const unsigned M = aDesc.rows;
    const unsigned N = bDesc.columns;
    const unsigned K = bDesc.rows;

    Module *Mod = CI->getParent()->getModule();
    Instruction *dpasCall = nullptr;
    if (isMADSupportedAsBuiltin(M, N, K)) {
        Function *madFunc = getMADBuiltin(Mod, M, N, K, PA, PB, cDesc.isFloating);

        Value *aMat = Resolve(aMatVal);
        Value *bMat = Resolve(bMatVal);
        Value *cMat = Resolve(cMatVal);

        // Create alloca in the entry node of the function
        IRBuilder<> builder(&*CI->getFunction()->getEntryBlock().getFirstInsertionPt());
        builder.SetCurrentDebugLocation(CI->getDebugLoc());

        Value *sliceA = builder.CreateAlloca(aMat->getType(), ADDRESS_SPACE_PRIVATE);
        Value *sliceB = builder.CreateAlloca(bMat->getType(), ADDRESS_SPACE_PRIVATE);
        Value *sliceC = builder.CreateAlloca(cMat->getType(), ADDRESS_SPACE_PRIVATE);
        Value *sliceD = builder.CreateAlloca(cMat->getType(), ADDRESS_SPACE_PRIVATE);

        builder.SetInsertPoint(CI);

        builder.CreateStore(aMat, sliceA);
        builder.CreateStore(bMat, sliceB);
        builder.CreateStore(cMat, sliceC);

        LLVMContext &ctx = CI->getContext();
        Type *arrayTy = Type::getInt8PtrTy(ctx, ADDRESS_SPACE_PRIVATE);

        Value *ptrA = builder.CreateBitCast(sliceA, arrayTy);
        Value *ptrB = builder.CreateBitCast(sliceB, arrayTy);
        Value *ptrC = builder.CreateBitCast(sliceC, arrayTy);
        Value *ptrD = builder.CreateBitCast(sliceD, arrayTy);

        Value* args[4] = { ptrA, ptrB, ptrC, ptrD };

        builder.CreateCall(madFunc, args);
        dpasCall = loadSlice(&builder, cMat->getType(), sliceD);
    } else {
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

        GenISAIntrinsic::ID iid = GenISAIntrinsic::GenISA_sub_group_dpas;
        Function *dpasFunc = GenISAIntrinsic::getDeclaration(Mod, iid, ITys);
        dpasCall = CallInst::Create(dpasFunc, args, VALUE_NAME("dpas"), CI);
    }

    /* TODO: null check and throw error here */
    dpasCall->setDebugLoc(CI->getDebugLoc());
    InstsToErase.insert(CI);

    return dpasCall;
}

template <class BuilderT>
static Type *getResolvedVectorElementType(Type *matrixType, BuilderT *builder) {
    IGCLLVM::FixedVectorType *ty = dyn_cast<IGCLLVM::FixedVectorType>(matrixType);
    if (ty && ty->getNumElements() <= 32)
        return ty->getElementType();
    if (matrixType->isIntegerTy() || matrixType->isFloatingPointTy())
        return matrixType;
    if (matrixType->isArrayTy() && matrixType->getArrayNumElements() == 2) {
        return Type::getInt32Ty(builder->getContext());
    }

    IGC_ASSERT_MESSAGE(false, "Unexpected type of matrix slice.");
    return nullptr;
}

int JointMatrixFuncsResolutionPass::getSliceSize(const JointMatrixTypeDescription *desc){
    if (desc->bitWidth == 0){
        IGC_ASSERT_MESSAGE(false, "Unexpected matrix element bit width.");
        return 1;
    }
    return getNumRowsPerWI(desc) * (desc->contribBitWidth / desc->bitWidth);
}

// expectation is both value and target type are IntegerType
template <class BuilderT>
static Value *packFillValue(BuilderT *Builder, Value *V, IntegerType *TargetType) {
    IntegerType *currentType = dyn_cast<IntegerType>(V->getType());
    IGC_ASSERT_MESSAGE(currentType && TargetType, "Expected integer types for packing.");

    uint64_t sourceBitWidth = currentType->getBitWidth();
    uint64_t packFactor = TargetType->getBitWidth() / sourceBitWidth;

    if (ConstantInt *Constant = dyn_cast<ConstantInt>(V)) {
        uint64_t value = Constant->getLimitedValue();
        if (value == 0) {
            return ConstantInt::get(TargetType, 0);
        }

        uint64_t packedValue = 0;
        for (unsigned i = 0; i < packFactor; i++) {
            packedValue |= value << (sourceBitWidth * i);
        }
        return ConstantInt::get(TargetType, packedValue);
    }

    Value *extendedValue = Builder->CreateZExt(V, TargetType);
    Value *acc = extendedValue;
    for (unsigned i = 1; i < packFactor; i++) {
        Value *shl = Builder->CreateShl(extendedValue, sourceBitWidth * i);
        acc = Builder->CreateOr(shl, acc);
    }
    return acc;
}

Value *JointMatrixFuncsResolutionPass::ResolveFill(CallInst *CI) {
    IRBuilder builder(CI);
    Value *fillValue = CI->getArgOperand(0);

    JointMatrixTypeDescription desc;
    Type *matTy = ResolveType(CI->getType(), &desc);

    if (fillValue->getType()->isPointerTy()) {
        IntegerType *sliceElmentType = Type::getIntNTy(builder.getContext(), desc.bitWidth);
        PointerType *PT = dyn_cast<PointerType>(fillValue->getType());
        fillValue = builder.CreateBitCast(fillValue, PointerType::get(sliceElmentType, PT->getAddressSpace()));
        fillValue = builder.CreateLoad(sliceElmentType, fillValue);
    }

    IntegerType *vecElementIntType = dyn_cast<IntegerType>(getResolvedVectorElementType(matTy, &builder));
    // If the slice is an integer type and value is float type, we need a bitcast.
    if (vecElementIntType && !dyn_cast<IntegerType>(fillValue->getType()))
        fillValue = builder.CreateBitCast(fillValue, Type::getIntNTy(builder.getContext(), desc.bitWidth));

    if (desc.bitWidth != desc.contribBitWidth)
        fillValue = packFillValue(&builder, fillValue, vecElementIntType);

    // Special case Accumulator 32x64 is represented as [2 x <i64 x 32>].
    // Hence need to pack 2 i32 values into 1 i64
    if (isAccumulator32x64(desc))
        fillValue = packFillValue(&builder, fillValue, Type::getIntNTy(builder.getContext(), 64));

    Value *slice = fillValue;

    // We create a vector only for rows > 1, as for rows = 1, we have one signle element instead of a one-element vector.
    if (IGCLLVM::FixedVectorType *ty = dyn_cast<IGCLLVM::FixedVectorType>(matTy)) {
        const int vectorSize = (int) ty->getNumElements();
        slice = UndefValue::get(matTy);
        for (int i = 0; i < vectorSize; i++) {
            slice = builder.CreateInsertElement(slice, fillValue, i);
        }
    }
    // Special case Accumulator 32x64 is represented as [2 x <i64 x 32>].
    else if (isAccumulator32x64(desc))
    {
        Type *halfTy = getAcc32x64HalfType(builder.getContext());
        Value *slice0 = UndefValue::get(halfTy);
        Value *slice1 = UndefValue::get(halfTy);

        const int vectorSize = 32;
        for (int i = 0; i < vectorSize; i++) {
            slice0 = builder.CreateInsertElement(slice0, fillValue, i);
            slice1 = builder.CreateInsertElement(slice1, fillValue, i);
        }

        slice = createPair(&builder, halfTy, slice0, slice1);
    }

    InstsToErase.insert(CI);
    return slice;
}

Value *JointMatrixFuncsResolutionPass::ResolveWILength(CallInst *CI) {
    JointMatrixTypeDescription desc;
    ResolveType(CI->getArgOperand(0)->getType(), &desc);

    const int sliceSize = getSliceSize(&desc);
    Value *length = ConstantInt::get(CI->getType(), sliceSize);

    CI->replaceAllUsesWith(length);
    InstsToErase.insert(CI);
    return length;
}

Instruction *JointMatrixFuncsResolutionPass::ResolveGetCoord(CallInst *CI) {
    Value *jointMatArg = CI->getArgOperand(0);
    Value *elemIdx = CI->getArgOperand(1);
    JointMatrixTypeDescription desc;
    ResolveType(jointMatArg->getType(), &desc);

    std::string funcName = GetMatrixFuncName(
        true, false, -1 /*placeholder*/, -1 /*placeholder*/, &desc,
        "__builtin_spirv_OpJointMatrixGetCoordINTEL_"); // GetCoordMatrixFuncName(&desc);

    IRBuilder builder(CI);

    // Argument type should be a i32??
    Type *argType = Type::getIntNTy(builder.getContext(), 32);
    elemIdx = builder.CreateTruncOrBitCast(elemIdx, argType);

    FunctionType *funcType = FunctionType::get(
        CI->getCalledFunction()->getReturnType(), {argType}, false);
    std::vector<Value *> Args = {elemIdx};

    Module *M = CI->getParent()->getModule();

    Instruction *newCall = builder.CreateCall(
        M->getOrInsertFunction(funcName, funcType), Args, "get_coord");
    newCall->setDebugLoc(CI->getDebugLoc());

    CI->replaceAllUsesWith(newCall);
    InstsToErase.insert(CI);

    return newCall;
}

Value *JointMatrixFuncsResolutionPass::createSliceExtract
      (IRBuilder<> *builder, Value *matrix, Value *index, const JointMatrixTypeDescription *desc) {
    if (desc->bitWidth != desc->contribBitWidth) {
        /* Unpacking: */
        IGC_ASSERT_MESSAGE(desc->bitWidth > 0, "Unexpected matrix element bit width.");
        uint64_t packFactor = desc->contribBitWidth / desc->bitWidth;
        index = builder->CreateUDiv(index, ConstantInt::get(index->getType(), packFactor));
    }
    return builder->CreateExtractElement(matrix, index, "matrix.element");
}

// TODO: this method may need to be updated to support accumulator 32x64 due to
// non-standard representation [2 x <i64 x 32>]
Value *JointMatrixFuncsResolutionPass::ResolveSliceInsert(CallInst *CI) {
    Value *matrix = Resolve(CI->getArgOperand(0));
    Value *component = CI->getArgOperand(1);
    Value *index = CI->getArgOperand(2);

    JointMatrixTypeDescription desc;
    Type *rawMatTy = ResolveType(CI->getArgOperand(0)->getType(), &desc);
    IGCLLVM::FixedVectorType *matTy = dyn_cast<IGCLLVM::FixedVectorType>(rawMatTy);

    IRBuilder builder(CI);
    Value *slice = nullptr;
    if (desc.bitWidth != desc.contribBitWidth) {
        // If rows = 1, we do not need an extract, so directly use the value.
        Value *element = matrix;
        if (matTy) {
            // We have a vector e.g. a matrix with rows > 1
            element = createSliceExtract(&builder, matrix, index, &desc);
        }
        if (!isa<IntegerType>(element->getType())) {
            element = builder.CreateBitCast(element, Type::getIntNTy(builder.getContext(), desc.contribBitWidth));
        }

        uint64_t packFactor = desc.contribBitWidth / desc.bitWidth;
        Value *offset = builder.CreateURem(index, ConstantInt::get(index->getType(), packFactor));
        offset = builder.CreateMul(offset, ConstantInt::get(offset->getType(), desc.bitWidth));

        index = builder.CreateUDiv(index, ConstantInt::get(index->getType(), packFactor));

        if (!isa<IntegerType>(component->getType())) {
            component = builder.CreateBitCast(component, Type::getIntNTy(builder.getContext(), desc.bitWidth));
        }

        component = builder.CreateZExtOrBitCast(component, Type::getIntNTy(builder.getContext(), desc.contribBitWidth));
        offset = builder.CreateTruncOrBitCast(offset, Type::getIntNTy(builder.getContext(), desc.contribBitWidth));

        /* clear element bits: */
        uint64_t maskValue = (1ULL << desc.bitWidth) - 1;
        Value *mask = builder.CreateShl(ConstantInt::get(element->getType(), maskValue), offset);
        mask = builder.CreateNot(mask);
        element = builder.CreateAnd(element, mask);

        /* shift component and merge with element: */
        component = builder.CreateShl(component, offset);
        component = builder.CreateOr(element, component);
    }

    if (IntegerType *vectorElementType = dyn_cast<IntegerType>(getResolvedVectorElementType(rawMatTy, &builder)))
        component = builder.CreateBitCast(component, vectorElementType);

    if (matTy) {
        slice = builder.CreateInsertElement(matrix, component, index);
    } else {
        slice = component;
    }

    InstsToErase.insert(CI);
    return slice;
}

Value *JointMatrixFuncsResolutionPass::ResolveSliceExtract(CallInst *CI) {
    Value *matrix = Resolve(CI->getArgOperand(0));
    Value *index = CI->getArgOperand(1);

    JointMatrixTypeDescription desc;
    Type *matTy = ResolveType(CI->getArgOperand(0)->getType(), &desc);

    IRBuilder builder(CI);

    // If we are dealing with a vector, extract the element, else we have a
    // single value, we can directly use the value
    Value *element = matrix;
    if (IGCLLVM::FixedVectorType *ty = dyn_cast<IGCLLVM::FixedVectorType>(matTy))
        element = createSliceExtract(&builder, matrix, index, &desc);

    /* Unpacking: */
    if (desc.bitWidth != desc.contribBitWidth) {
        index = builder.CreateTruncOrBitCast(index, element->getType());
        uint64_t packFactor = desc.contribBitWidth / desc.bitWidth;
        Value *offset = builder.CreateURem(index, ConstantInt::get(index->getType(), packFactor));
        offset = builder.CreateMul(offset, ConstantInt::get(offset->getType(), desc.bitWidth));
        element = builder.CreateAShr(element, offset);
        uint64_t mask = (1ULL << desc.bitWidth) - 1;
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
        type = ResolveTypes(v->getType());
    }
    if (type->isVoidTy()) {
        return;
    }

    Instruction *predecesor = nullptr;
    if (Instruction *inst = dyn_cast<Instruction>(v)) {
        predecesor = inst;
    }

    Instruction *placeholder = nullptr;
    if (!type->isArrayTy()) {
        /* Using bit-casts as placeholder values. Undefs of each type are unique per
         * module and cannot be used as unique placeholders. */
        placeholder =
            BitCastInst::Create(Instruction::BitCast, UndefValue::get(type),
                                type, "tmp.value", predecesor);
    } else {
        /* Array types cannot be bitcasted. Use instert element with two undefs
         * to create unique placeholder for array value.*/
        Value *array = UndefValue::get(type);
        Value *element = UndefValue::get(type->getArrayElementType());
        placeholder = InsertValueInst::Create(array, element, { 0 }, "tmp.value", predecesor);
    }
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
    if (funcName.contains(JointMatrixLoadPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveLoad(CI);
    } else if (funcName.contains(JointMatrixStorePrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveStore(CI);
    } else if (funcName.contains(JointMatrixMadPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveMad(CI, MadOpSS);
    } else if (funcName.contains(JointMatrixSUMadPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveMad(CI, MadOpSU);
    } else if (funcName.contains(JointMatrixUSMadPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveMad(CI, MadOpUS);
    } else if (funcName.contains(JointMatrixUUMadPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveMad(CI, MadOpUU);
    } else if (funcName.contains(JointMatrixFillPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveFill(CI);
    } else if (funcName.contains(JointMatrixWorkItemLengthPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveWILength(CI);
    } else if (funcName.contains(JointMatrixSliceInsert)) {
        InsertPlaceholder(CI);
        NewValue = ResolveSliceInsert(CI);
    } else if (funcName.contains(JointMatrixSliceExtract)) {
        InsertPlaceholder(CI);
        NewValue = ResolveSliceExtract(CI);
    } else if (funcName.contains(JointMatrixGetCoordPrefx)) {
        InsertPlaceholder(CI);
        NewValue = ResolveGetCoord(CI);
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

Value *JointMatrixFuncsResolutionPass::ResolveGeneric(Instruction *OldInst)
{
    InsertPlaceholder(OldInst);
    Instruction *NewInst = OldInst->clone();

    for (unsigned i = 0; i < NewInst->getNumOperands(); i++)
    {
        Value *oldOp = NewInst->getOperand(i);
        if (!isOrContainsMatrixType(oldOp->getType()))
            continue;

        NewInst->setOperand(i, Resolve(oldOp));
    }

    if (GetElementPtrInst *NewGEPI = dyn_cast<GetElementPtrInst>(NewInst))
    {
        GetElementPtrInst *OldGEPI = cast<GetElementPtrInst>(OldInst);
        NewGEPI->setSourceElementType(ResolveTypes(OldGEPI->getSourceElementType()));
        NewGEPI->setResultElementType(ResolveTypes(OldGEPI->getResultElementType()));
    }
    else if (AllocaInst *NewAlloca = dyn_cast<AllocaInst>(NewInst))
    {
        AllocaInst *OldAlloca = cast<AllocaInst>(OldInst);
        NewAlloca->setAllocatedType(ResolveTypes(OldAlloca->getAllocatedType()));
    }

    NewInst->mutateType(ResolveTypes(NewInst->getType()));
    NewInst->setName(OldInst->getName());
    NewInst->insertBefore(OldInst);
    NewInst->setDebugLoc(OldInst->getDebugLoc());

    CacheResolvedValue(OldInst, NewInst);
    InstsToErase.insert(OldInst);
    return NewInst;
}

Type *JointMatrixFuncsResolutionPass::ResolveTypes(llvm::Type *t)
{
    if (ResolvedTypes.count(t) > 0)
        return ResolvedTypes[t];

    if (StructType *ST = dyn_cast<StructType>(t))
        return ResolveStructType(ST);

    if (ArrayType *AT = dyn_cast<ArrayType>(t))
        return ResolveArrayType(AT);

    if (PointerType *PT = dyn_cast<PointerType>(t))
    {
        if (isMatrixType(t))
            return ResolveType(t, nullptr);
        return ResolvePointerType(PT);
    }

    return t;
}

Type *JointMatrixFuncsResolutionPass::ResolveStructType(Type *oldType)
{
    if (ResolvedTypes.count(oldType) > 0)
        return ResolvedTypes[oldType];

    StructType *structType = cast<StructType>(oldType);
    SmallString<28> name;
    StructType *newType = StructType::create(oldType->getContext(),
                                             (structType->getName() +
                                              ".resolved")
                                                 .toStringRef(name));

    // caching now to avoid recursion, in case struct contains itself as an element
    CacheResolvedTypes(oldType, newType);

    SmallVector<Type *, 1> elements;
    llvm::transform(structType->elements(), std::back_inserter(elements), [&](Type *t)
                    {
                        if (isOrContainsMatrixType(t))
                            return ResolveTypes(t);
                        return t; });
    newType->setBody(elements, structType->isPacked());

    return newType;
}

Type *JointMatrixFuncsResolutionPass::ResolveArrayType(Type *oldType)
{
    if (ResolvedTypes.count(oldType) > 0)
        return ResolvedTypes[oldType];

    ArrayType *arrayType = cast<ArrayType>(oldType);
    Type *elemType = arrayType->getElementType();
    if (!isOrContainsMatrixType(elemType))
        return oldType;

    Type *newType = ArrayType::get(ResolveTypes(elemType), arrayType->getNumElements());
    CacheResolvedTypes(oldType, newType);
    return newType;
}

Type *JointMatrixFuncsResolutionPass::ResolvePointerType(Type *oldType)
{
    if (ResolvedTypes.count(oldType) > 0)
        return ResolvedTypes[oldType];

    PointerType *ptrType = dyn_cast<PointerType>(oldType);
    Type *elemType = IGCLLVM::getNonOpaquePtrEltTy(ptrType);
    if (!isOrContainsMatrixType(elemType))
        return oldType;

    Type *newType = PointerType::get(ResolveTypes(elemType), ptrType->getAddressSpace());
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

        Type *type = ResolveTypes(v->getType());
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
    } else if (Instruction *I = dyn_cast<Instruction>(v)) {
        return ResolveGeneric(I);
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

    /* Check if already resolved: */
    if (ResolvedValues.count(&CI) > 0)
      return;

    StringRef funcName = func->getName();

    /* Resolve calls to JointMatrix BIs that haven't been resolved yet. In
     * future when returning and passing matrices by argument is
     * supported also basic block terminators should be used as
     * transformation starting point */
    if (funcName.startswith(JointMatrixBIPrefix) || funcName.contains(JointMatrixBISuffix)) {
        ResolveSIMDSize(CI.getParent()->getParent());
        ResolveCall(&CI);
        return;
    }

    if (funcName.startswith("_Z") && funcName.contains("__spirv_JointMatrix")) {
        ResolveSIMDSize(CI.getParent()->getParent());
        ResolveCall(&CI);
        return;
    }

    // Update size of allocated element in llvm.lifetime.start/end intrincics
    if (auto II = dyn_cast<IntrinsicInst>(&CI)) {
        if (II->getIntrinsicID() == Intrinsic::lifetime_start ||
            II->getIntrinsicID() == Intrinsic::lifetime_end) {

            // track pointer operand to alloca instr
            auto &DL = CI.getModule()->getDataLayout();
            Value *obj = IGCLLVM::getUnderlyingObject(II->getOperand(1), DL);

            if (AllocaInst *AI = dyn_cast_or_null<AllocaInst>(obj)) {
                // if alloca requires resolving, resolve alloca, otherwise do not touch intrinsic
                // as it is not related to Joint Matrix type
                if (!isOrContainsMatrixType(AI->getAllocatedType()))
                    return;

                ResolveSIMDSize(CI.getParent()->getParent());
                AllocaInst *NAI = cast<AllocaInst>(Resolve(AI));
                auto allocaSizeInBits = IGCLLVM::wrapOptional(NAI->getAllocationSizeInBits(DL));
                if (!allocaSizeInBits.hasValue())
                    return;
                uint64_t newSize = (uint64_t)(allocaSizeInBits.getValue() / 8);

                // update first argument, if it is constant int
                if (auto *ConstInt = dyn_cast<ConstantInt>(CI.getOperand(0))) {
                    CI.setOperand(0, ConstantInt::get(ConstInt->getType(), newSize));
                }
            }
        }
    }
}

void JointMatrixFuncsResolutionPass::visitAllocaInst(AllocaInst &I)
{
    if (ResolvedValues.count(&I) > 0)
        return;

    if (!isOrContainsMatrixType(I.getAllocatedType()))
        return;
    ResolveSIMDSize(I.getParent()->getParent());
    ResolveGeneric(&I);
}

void JointMatrixFuncsResolutionPass::visitGetElementPtrInst(GetElementPtrInst &GEP)
{
    if (ResolvedValues.count(&GEP) > 0)
        return;

    Type *GEPEltType = GEP.getSourceElementType();

    // After constant GEPs are canonicalized to i8 types, we may get patterns like below:
    //
    // %8 = bitcast [4 x [4 x %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix"]]* %tC.i to i8*
    // %arrayctor.end.i = getelementptr inbounds i8, i8* %8, i64 128
    //
    // It is not correct, because
    // original offset was 16 elements of matrix type. Matrix type before resolution is represented as pointer
    // Pointer is typically 8 bytes, hence offset of 128 bytes is calculated as 16 x 8 = 128
    // The real offset would be 16 matrix types after resolution, not pointer types.
    // So to fix the offset, we need calculate the offset in matrix type, taking into account pointer type size
    // Then we need calculate real matrix type size after resolution in bytes
    // Then real offset in bytes will be multiplicaiton of offset in matrix types and size of matrix type in bytes
    //
    // For example, if matrix type was resolved like that:
    // %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix.resolved" = type { <8 x float> }
    // offset will be 16 * (8 * 4) = 512:
    //
    // %arrayctor.end.i = getelementptr inbounds i8, i8* %8, i64 512
    BitCastInst *BC = dyn_cast<BitCastInst>(GEP.getOperand(0));

    if (GEPEltType->isIntegerTy(8) && BC && (GEP.getNumIndices() == 1) && GEP.hasAllConstantIndices()) {
        if (Type *BCSrcTy = BC->getSrcTy(); BCSrcTy->isPointerTy()){
            if (Type *unresolvedMatTy = getContainedMatrixType(BCSrcTy)) {

                // Calculate offset based on matrix type
                ConstantInt *index = cast<ConstantInt>(GEP.getOperand(1));
                auto &DL = GEP.getModule()->getDataLayout();
                uint64_t pointerSizeInBytes = DL.getPointerSizeInBits(GEP.getPointerAddressSpace()) / 8;
                uint64_t offsetInElements = index->getZExtValue() / pointerSizeInBytes;

                // Calculate correct offset in bytes and update GEP
                uint64_t elementSize = (uint64_t) DL.getTypeAllocSize(ResolveTypes(unresolvedMatTy));
                uint64_t correctOffset = offsetInElements * elementSize;
                GEP.idx_begin()->set(ConstantInt::get(index->getType(), correctOffset));
                return;
            }
        }
    }

    if (!isOrContainsMatrixType(GEPEltType))
        return;

    ResolveSIMDSize(GEP.getParent()->getParent());
    ResolveGeneric(&GEP);
}

void JointMatrixFuncsResolutionPass::visitStoreInst(StoreInst &I)
{
    if (ResolvedValues.count(&I) > 0)
        return;

    Value *val = I.getValueOperand();
    if (isOrContainsMatrixType(val->getType()))
    {
        ResolveSIMDSize(I.getParent()->getParent());
        ResolveGeneric(&I);
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

    ResolveSIMDSize(I.getParent()->getParent());
    InsertPlaceholder(&I);
    Value *PTIOperand = PTI->getOperand(0);
    Type *newBCElementType = ResolveTypes(
        dyn_cast<PointerType>(PTIOperand->getType()));
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
    // Here we just ignore this BitCast instruction. We will replace the use of value
    // it returns in visitStoreInst and then remove it.
    PointerType *srcPtr = dyn_cast<PointerType>(I.getSrcTy());
    PointerType *dstPtr = dyn_cast<PointerType>(I.getDestTy());
    if (srcPtr != nullptr && dstPtr != nullptr)
    {
        Type *srcPtrType = IGCLLVM::getNonOpaquePtrEltTy(srcPtr);
        Type *dstPtrType = IGCLLVM::getNonOpaquePtrEltTy(dstPtr);

        StructType *srcStructType = dyn_cast<StructType>(srcPtrType);
        if (srcStructType != nullptr && srcStructType->getNumElements() == 1)
        {
            Type *srcElemType = srcStructType->getElementType(0);
            if (isMatrixType(srcElemType) && dstPtrType->isIntegerTy(64))
                return;
        }
    }

    if (ResolvedValues.count(&I) > 0)
        return;

    if (!isOrContainsMatrixType(I.getSrcTy()) && !isOrContainsMatrixType(I.getDestTy()))
        return;

    ResolveSIMDSize(I.getParent()->getParent());
    ResolveGeneric(&I);
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

    if (!isOrContainsMatrixType(I.getSrcTy()))
        return;

    ResolveSIMDSize(I.getParent()->getParent());
    ResolveGeneric(&I);
}
