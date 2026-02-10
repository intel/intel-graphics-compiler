/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "JointMatrixFuncsResolutionPass.h"

#include "AdaptorOCL/Utils/CacheControlsHelper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "llvmWrapper/ADT/Optional.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/IR/Value.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "Probe/Assertion.h"
#include <optional>
#include <type_traits>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/PostOrderIterator.h>
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/Support/Debug.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char JointMatrixFuncsResolutionPass::ID = 0;

#define PASS_FLAG "igc-joint-matrix-resolution"
#define PASS_DESC "Lowering of INTEL Joint Matrix SPIR-V instructions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
#define DEBUG_TYPE "joint-matrix-resolution"

IGC_INITIALIZE_PASS_BEGIN(JointMatrixFuncsResolutionPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(JointMatrixFuncsResolutionPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

static const char *SPIRVPrefix = "__spirv_";
static const char *JointMatrixBIPrefix = "__builtin_spirv_OpJointMatrix";
static const char *JointMatrixBISuffix = "JointMatrixINTEL_";
static const char *JointMatrixPrefetchPrefx = "CooperativeMatrixPrefetch";
static const char *JointMatrixLoadPrefx = "JointMatrixLoadINTEL";
static const char *JointMatrixLoadCheckedPrefx = "CooperativeMatrixLoadCheckedINTEL";
static const char *JointMatrixStorePrefx = "JointMatrixStoreINTEL";
static const char *JointMatrixStoreCheckedPrefx = "CooperativeMatrixStoreCheckedINTEL";
static const char *JointMatrixMadPrefx = "JointMatrixMadINTEL";
static const char *JointMatrixSUMadPrefx = "JointMatrixSUMadINTEL";
static const char *JointMatrixUSMadPrefx = "JointMatrixUSMadINTEL";
static const char *JointMatrixUUMadPrefx = "JointMatrixUUMadINTEL";
static const char *JointMatrixFillPrefx = "CompositeConstruct";
static const char *JointMatrixFillCheckedPrefx = "CooperativeMatrixConstructCheckedINTEL";
static const char *JointMatrixWorkItemLengthPrefx = "JointMatrixWorkItemLengthINTEL";
static const char *JointMatrixSliceInsert = "VectorInsertDynamic";
static const char *JointMatrixSliceExtract = "VectorExtractDynamic";
static const char *JointMatrixGetCoordPrefx = "JointMatrixGetElementCoordINTEL";

static const char *CooperativeMatrixBISuffix = "CooperativeMatrixKHR_";
static const char *CooperativeMatrixConstantCompositePrefx = "ConstantComposite";
static const char *CooperativeMatrixLoadPrefx = "CooperativeMatrixLoadKHR";
static const char *CooperativeMatrixStorePrefx = "CooperativeMatrixStoreKHR";
static const char *CooperativeMatrixMadPrefx = "CooperativeMatrixMulAddKHR";
static const char *CooperativeMatrixLengthPrefx = "CooperativeMatrixLengthKHR";
static const char *CooperativeMatrixGetElementCoordPrefx = "CooperativeMatrixGetElementCoordINTEL";
static const char *AccessChainPrefx = "__spirv_AccessChain";

// We need module pass, since:
// 1) we inspect multiple functions to find entry function to get sub group size
// 2) we maintain map of functions to entry functions across functions we process
// so the pass is not local to one function.
JointMatrixFuncsResolutionPass::JointMatrixFuncsResolutionPass() : ModulePass(ID) {
  initializeJointMatrixFuncsResolutionPassPass(*PassRegistry::getPassRegistry());
}

// Static helper functions for type traversal
static bool isMatrixType(const Type *type) {
  // isPointerTy check here and below in other places will skip resolving of matrix types,
  // when opaque pointers are enabled, but target extension types are not.
  StringRef name = "";

  if (IGCLLVM::isTargetExtTy(type)) {
    name = IGCLLVM::getTargetExtName(type);
  } else {
    if (!type->isPointerTy() || IGCLLVM::isPointerTy(type))
      return false;

    Type *eltType = IGCLLVM::getNonOpaquePtrEltTy(type);
    if (!eltType || !eltType->isStructTy())
      return false;

    if (cast<StructType>(eltType)->isLiteral())
      return false;

    name = eltType->getStructName();
  }

  if (name.startswith("intel.joint_matrix") || name.startswith("spirv.JointMatrixINTEL") ||
      name.startswith("spirv.CooperativeMatrixKHR"))
    return true;

  return false;
}

static void PreOrderTypeTraversal(Type *t, std::unordered_set<Type *> &set) {
  if (set.find(t) != set.end())
    return;

  if (StructType *ST = dyn_cast<StructType>(t)) {
    set.insert(t);
    for (auto subT : ST->elements())
      PreOrderTypeTraversal(subT, set);
    return;
  }

  if (ArrayType *AT = dyn_cast<ArrayType>(t)) {
    set.insert(t);
    return PreOrderTypeTraversal(AT->getElementType(), set);
  }

  if (PointerType *PT = dyn_cast<PointerType>(t)) {
    set.insert(t);
    if (IGCLLVM::isPointerTy(PT))
      return;
    return PreOrderTypeTraversal(IGCLLVM::getNonOpaquePtrEltTy(PT), set);
  }

#if LLVM_VERSION_MAJOR >= 16
  if (isa<TargetExtType>(t))
    set.insert(t);
#endif
}

static Type *getContainedMatrixType(Type *root) {
  std::unordered_set<Type *> set;
  PreOrderTypeTraversal(root, set);

  for (auto &t : set) {
    if (isMatrixType(t))
      return t;
  }

  return nullptr;
}

static bool isOrContainsMatrixType(Type *root) { return getContainedMatrixType(root) != nullptr; }

static bool isAnyFunctionArgMatrixType(Function *F) {
  if (!F)
    return false;

  for (Argument &arg : F->args()) {
    if (isOrContainsMatrixType(arg.getType()))
      return true;
  }

  return false;
}

template <typename F> static bool isAnyOperand(const User &U, F &&lambda) {
  return any_of(U.operands(), [&lambda](const Use &op) { return lambda(op->getType()); });
}

// Member functions
bool JointMatrixFuncsResolutionPass::runOnModule(Module &M) {
  m_Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_mdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  FunctionEntryMap.clear();
  ResolvedFuncSignatures.clear();
  NewFuncWithResolvedSignatures.clear();
  ResolvedTypes.clear();
  Changed = false;

  for (auto &F : M) {
    if (!F.isDeclaration())
      continue;

    if (F.getName().contains(AccessChainPrefx))
      Changed |= preprocessAccessChain(&F);
  }

  for (auto &F : M) {
    if (F.isDeclaration() || NewFuncWithResolvedSignatures.count(&F) > 0)
      continue;
    if (isAnyFunctionArgMatrixType(&F)) {
      LLVM_DEBUG(dbgs() << "\nRESOLVE FUNC: " << F.getName() << "\n");
      clearFunctionCache();
      Changed |= ResolveFunction(&F);
    }
  }

  for (auto &F : M) {
    if (F.isDeclaration() || ResolvedFuncSignatures.count(&F) > 0 || NewFuncWithResolvedSignatures.count(&F) > 0)
      continue;

    LLVM_DEBUG(dbgs() << "\nRUN ON FUNC:\n" << F);
    clearFunctionCache();
    Changed |= runOnFunction(F);
  }

  for (const auto &ResolvedFunctionPair : ResolvedFuncSignatures) {
    Function *OriginalFunction = ResolvedFunctionPair.first;

    if (OriginalFunction->use_empty()) {
      OriginalFunction->eraseFromParent();
    }
  }

  return Changed;
}

// Finds entry function for input function. If there are several entry
// functions, it will return only one (first found).
// So currently the case, when the same function which is using Joint Matrix
// is called from several kernels with different sub-group size required,
// not supported and behavior is undefined.
Function *JointMatrixFuncsResolutionPass::getEntryFunction(Function *F) {
  if (FunctionEntryMap.count(F) > 0) {
    LLVM_DEBUG(dbgs() << " - FOUND CACHED ENTRY FUNCTION: " << FunctionEntryMap[F]->getName() << "\n");
    return FunctionEntryMap[F];
  }

  if (isEntryFunc(m_mdUtils, F)) {
    FunctionEntryMap[F] = F;
    LLVM_DEBUG(dbgs() << " - FUNCTION IS ENTRY FUNCTION\n");
    return F;
  }

  SmallVector<Function *, 8> toProcess;
  toProcess.push_back(F);

  while (!toProcess.empty()) {
    Function *curFunc = toProcess.pop_back_val();

    for (auto user : curFunc->users()) {
      auto CI = dyn_cast<CallInst>(user);

      if (!CI || CI->getCalledFunction() != curFunc)
        continue;

      auto parentFunction = CI->getFunction();
      LLVM_DEBUG(dbgs() << " - CHECK PARENT FUNCTION: " << parentFunction->getName() << "\n");
      Function *entryFunc = nullptr;

      if (FunctionEntryMap.count(parentFunction) > 0) {
        entryFunc = FunctionEntryMap[parentFunction];
      } else if (isEntryFunc(m_mdUtils, parentFunction)) {
        entryFunc = parentFunction;
      }

      if (entryFunc == nullptr) {
        toProcess.push_back(parentFunction);
        continue;
      }

      FunctionEntryMap[curFunc] = entryFunc;
      LLVM_DEBUG(dbgs() << " - FOUND ENTRY FUNCTION: " << entryFunc->getName() << "\n");
      return entryFunc;
    }
  }

  FunctionEntryMap[F] = nullptr;
  LLVM_DEBUG(dbgs() << " - NOT FOUND ENTRY FUNCTION\n");
  return nullptr;
}

int32_t JointMatrixFuncsResolutionPass::DetermineForcedSIMDSize() {
  int32_t forcedSIMDSize = m_Ctx->getModuleMetaData()->csInfo.forcedSIMDSize;

  if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD32) && IGC_IS_FLAG_DISABLED(ForceCSSIMD16) &&
      (forcedSIMDSize == 32 || IGC_IS_FLAG_ENABLED(ForceCSSIMD32))) {
    if (forcedSIMDSize == 0)
      m_Ctx->getModuleMetaData()->csInfo.forcedSIMDSize = 32;
    return 32;
  }

  if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD16) && IGC_IS_FLAG_DISABLED(ForceCSSIMD32) &&
      (forcedSIMDSize == 16 || IGC_IS_FLAG_ENABLED(ForceCSSIMD16))) {
    if (forcedSIMDSize == 0)
      m_Ctx->getModuleMetaData()->csInfo.forcedSIMDSize = 16;
    return 16;
  }

  return forcedSIMDSize;
}

int32_t JointMatrixFuncsResolutionPass::DefineKernelSIMDSize() {
  if (m_Ctx->platform.hasExecSize16DPAS()) {
    if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD16) && IGC_IS_FLAG_DISABLED(ForceCSSIMD32))
      return 16;
    if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD32) && IGC_IS_FLAG_DISABLED(ForceCSSIMD16))
      return 32;
    std::string msg = "Sub group sizes supported by Joint Matrix for this platform are disabled by flags or "
                      "non-supported sub group size forced.";
    m_Ctx->EmitError(msg.c_str(), nullptr);
    return 0;
  }
  if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD32) && IGC_IS_FLAG_ENABLED(ForceCSSIMD32)) {
    std::string msg = "Sub group size 32 forced by flags but not supported by Joint Matrix on this platform.";
    m_Ctx->EmitError(msg.c_str(), nullptr);
    return 0;
  }
  if (IGC_IS_FLAG_ENABLED(EnableOCLSIMD16) && IGC_IS_FLAG_ENABLED(ForceCSSIMD16)) {
    std::string msg = "Sub group size 16 forced by flags but not supported by Joint Matrix on this platform.";
    m_Ctx->EmitError(msg.c_str(), nullptr);
    return 0;
  }
  return 8;
}

bool JointMatrixFuncsResolutionPass::IsSIMDSizeValid(int32_t simdSize) {
  return ((m_Ctx->platform.hasExecSize16DPAS() && (simdSize == 16 || simdSize == 32)) ||
          (!m_Ctx->platform.hasExecSize16DPAS() && simdSize == 8));
}

void JointMatrixFuncsResolutionPass::ForceKernelSIMDSize(Function *F, int32_t forcedSIMDSize) {
  Function *entryFunction = getEntryFunction(F);
  if (entryFunction) // if can find entry function
  {
    IGCMD::FunctionInfoMetaDataHandle funcInfoMD = m_mdUtils->getFunctionsInfoItem(entryFunction);
    IGCMD::SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
    subGroupSize->setSIMDSize(forcedSIMDSize);
  }
}

void JointMatrixFuncsResolutionPass::ResolveSIMDSize(Function *F) {
  if (m_SIMDSize != 0)
    return;

  int32_t forcedSIMDSize = DetermineForcedSIMDSize();
  if (forcedSIMDSize != 0) {
    if (IsSIMDSizeValid(forcedSIMDSize)) {
      m_SIMDSize = forcedSIMDSize;
      ForceKernelSIMDSize(F, m_SIMDSize);
      return;
    }
    // if forced and not ok for platform exit with error
    std::string msg = "Sub group size " + std::to_string(forcedSIMDSize) +
                      " is forced by flags but not supported by Joint Matrix on this platform.";
    m_Ctx->EmitError(msg.c_str(), nullptr);
    return;
  }

  // if not forced by driver of flags, check on entry function level
  Function *entryFunction = getEntryFunction(F);
  if (entryFunction) // if can find entry function
  {
    IGCMD::FunctionInfoMetaDataHandle funcInfoMD = m_mdUtils->getFunctionsInfoItem(entryFunction);
    IGCMD::SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
    if (subGroupSize->hasValue()) {
      int32_t kernelSIMDSize = subGroupSize->getSIMDSize();
      if (kernelSIMDSize != 0) {
        if (IsSIMDSizeValid(kernelSIMDSize)) {
          m_SIMDSize = kernelSIMDSize;
          return;
        }
        // if set on entry function level and not ok for this platform exit with error
        std::string msg = "Sub group size " + std::to_string(kernelSIMDSize) +
                          " is forced by attribute but not supported by Joint Matrix on this platform.";
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

void JointMatrixFuncsResolutionPass::clearFunctionCache() {
  PlaceholderInstructions.clear();
  ResolvedValues.clear();
  InstsToErase.clear();
  MatrixAllocas.clear();
  m_SIMDSize = 0;
}

bool JointMatrixFuncsResolutionPass::runOnFunction(Function &F) {
  // Use reverse post order traversal to reduce level or recursion.
  ReversePostOrderTraversal<Function *> RPOT(&F);
  for (BasicBlock *BB : RPOT)
    visit(BB);

  for (Instruction *I : InstsToErase) {
    if (ResolvedValues[I] && I->getType() == ResolvedValues[I]->getType()) {
      I->replaceAllUsesWith(ResolvedValues[I]);
    } else {
      Value *undef = UndefValue::get(I->getType());
      I->replaceAllUsesWith(undef);
    }
    I->eraseFromParent();
  }

  return !ResolvedValues.empty();
}

// See https://github.com/intel/llvm/blob/sycl/sycl/doc/design/spirv-extensions/SPV_INTEL_joint_matrix_legacy.asciidoc
namespace JointMatrix {
namespace Load {
enum Op { Pointer = 0, Stride = 1, Layout = 2, MemOperand = 3 };
} // namespace Load

namespace LoadChecked {
enum Op { Pointer = 0, Y = 1, X = 2, Layout = 3, Height = 4, Width = 5, Stride = 6, MemOperand = 7 };
} // namespace LoadChecked

namespace Store {
enum Op { Pointer = 0, Matrix = 1, Stride = 2, Layout = 3, MemOperand = 4 };
} // namespace Store

namespace StoreChecked {
enum Op { Pointer = 0, Y = 1, X = 2, Matrix = 3, Layout = 4, Height = 5, Width = 6, Stride = 7, MemOperand = 8 };
} // namespace StoreChecked

namespace Construct {
enum Op {
  Value = 0,
};
} // namespace Construct

namespace ConstructChecked {
enum Op {
  Y = 0,
  X = 1,
  Height = 2,
  Width = 3,
  Value = 4,
};
} // namespace ConstructChecked
} // namespace JointMatrix

// See https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/KHR/SPV_KHR_cooperative_matrix.asciidoc
namespace CoopMatrix {
namespace Load {
enum Op { Pointer = 0, Layout = 1, Stride = 2, MemOperand = 3 };
} // namespace Load

namespace Store {
enum Op { Pointer = 0, Matrix = 1, Layout = 2, Stride = 3, MemOperand = 4 };
} // namespace Store
} // namespace CoopMatrix

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
  // OpCooperativeMatrixMulAddKHR specifies signess by a masked parameter
  CooperativeOp
};

enum {
  None = 0,
  MatrixASignedComponentsKHR = 0x1,
  MatrixBSignedComponentsKHR = 0x2,
  MatrixCSignedComponentsKHR = 0x4,
  MatrixResultSignedComponentsKHR = 0x8,
  MatrixABFloat8ComponentsINTEL = 0x400,
  MatrixBBFloat8ComponentsINTEL = 0x800,
  MatrixAHFloat8ComponentsINTEL = 0x1000,
  MatrixBHFloat8ComponentsINTEL = 0x2000,
  // Unused right now
  SaturatingAccumulationKHR = 0x10,
  MatrixAAndBTF32ComponentsINTEL = 0x20,
  MatrixAAndBBFloat16ComponentsINTEL = 0x40
};

namespace IGC {
struct JointMatrixTypeDescription {
  unsigned layout = 0;
  unsigned rows = 0;
  unsigned columns = 0;
  unsigned bitWidth = 0;
  unsigned contribBitWidth = 0; // bit width of type used internally to store matrix elements
  unsigned use = UseMax;
  bool isFloating = false;
};
} // namespace IGC

static bool isOperandUnsigned(unsigned OperationType, unsigned OperandId) {
  switch (OperationType) {
  default:
  case MadOpSS:
    return false;
  case MadOpUU:
    return true;
  case MadOpSU:
    return OperandId != 0;
  case MadOpUS:
    return OperandId == 0;
  }
}

struct SupportedParams {
  int maxRows = -1; /* -1 means: don't check */
  int rows = -1;
  int columns = -1;
  unsigned bitWidth = 0; /* All supported sizes are powers of two, this field is
                            used as a bitfield with union of suported sizes */
  unsigned layouts = 0;  /* Each bit of this field corresponds to a single layout. */
};

static SupportedParams getSupportedParams(const JointMatrixTypeDescription *desc, bool useSG16) {
  /* slices are represented as vectors from <1 x i32> to <8 x i32>, resulting in the maximum slice size: */
  unsigned maxSliceBitWidth = 256;
  if (desc->bitWidth == 64) {
    maxSliceBitWidth = 512;
  }
  SupportedParams params;
  params.bitWidth = 8 | 32;
  params.bitWidth |= 64;

  if (desc->layout == LayoutPackedA) {
    params.maxRows = 8;
    params.columns = maxSliceBitWidth / desc->bitWidth;
    params.bitWidth |= 16;
    params.layouts = 1 << LayoutRowMajor;
    params.layouts |= 1 << LayoutColumnMajor;
  } else if (desc->layout == LayoutPackedB) {
    params.rows = maxSliceBitWidth / desc->bitWidth;
    params.columns = useSG16 ? 16 : 8;
    params.bitWidth |= 16;
    params.layouts |= 1 << LayoutColumnMajor;
    params.layouts |= 1 << LayoutPackedB;
    params.layouts |= 1 << LayoutPackedA;  /* PackedA means just packed in the new version of spec. */
    params.layouts |= 1 << LayoutRowMajor; /* for tf32 and VNNIed layouts */
  } else {                                 /* accumulator */
    params.maxRows = maxSliceBitWidth / desc->bitWidth;
    params.columns = useSG16 ? 16 : 8;
    params.bitWidth |= 16;
    params.layouts |= 1 << LayoutRowMajor;
    params.layouts |= 1 << LayoutColumnMajor;
  }

  return params;
}

enum ParamsCheckResult : unsigned {
  ALL_VALID = 0,
  INVALID_ROWS = 1 << 0,
  INVALID_COLS = 1 << 1,
  INVALID_ELEM = 1 << 2,
  INVALID_LAYOUT = 1 << 3,
  INVALID_PLATFORM = 1 << 4,
};

static ParamsCheckResult checkSupportedParams(const JointMatrixTypeDescription *desc, unsigned operationLayout,
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
  case LayoutPackedB:
    return "packed layout";
  case LayoutRowMajor:
    return "row major layout";
  case LayoutColumnMajor:
    return "column major layout";
  default:
    return "unknown";
  }
}

// TODO: Get rid of this function and update ValidateLoadStore and Validate2DBlockLoadStore
// accordingly.
static bool isSupprtedLargeSlice(const JointMatrixTypeDescription *desc, bool useSG16) {
  if (!useSG16) {
    if (desc->layout == LayoutPackedA) {
      if (desc->rows == 32 && desc->columns == 16 && desc->bitWidth == 16)
        return true;
    }
    if (desc->layout == LayoutPackedB) {
      if (desc->rows == 16 && desc->columns == 32 && desc->bitWidth == 16)
        return true;
    }
    if (desc->layout == LayoutRowMajor) {
      if (desc->rows == 32 && desc->columns == 32 && desc->bitWidth == 32)
        return true;
    }
    return false;
  }

  if (desc->layout == LayoutPackedA) {
    if (desc->bitWidth != 16)
      return false;
    if (desc->rows == 1 && desc->columns == 32)
      return true;
    if (desc->rows == 16 && desc->columns == 16)
      return true;
    if (desc->rows == 32 && desc->columns == 16)
      return true;
    if (desc->rows == 32 && desc->columns == 32)
      return true;
  }

  if (desc->layout == LayoutPackedB) {
    if (desc->rows == 16 && desc->columns == 64 && desc->bitWidth == 16)
      return true;
    if (desc->rows == 32 && desc->columns == 64 && desc->bitWidth == 16)
      return true;
  }

  if (desc->layout == LayoutRowMajor) {
    if (desc->rows == 1 && desc->columns == 64 && desc->bitWidth == 16)
      return true;
    if (desc->rows == 1 && desc->columns == 64 && desc->bitWidth == 32)
      return true;
    if (desc->rows == 16 && desc->columns == 16 && desc->bitWidth == 16)
      return true;
    if (desc->rows == 16 && desc->columns == 16 && desc->bitWidth == 32)
      return true;
    if (desc->rows == 32 && desc->columns == 64 && desc->bitWidth == 16)
      return true;
    if (desc->rows == 32 && desc->columns == 64 && desc->bitWidth == 32)
      return true;
  }

  return false;
}

bool JointMatrixFuncsResolutionPass::ValidateIntegerBitWidth(unsigned int bitWidth) {
  bool result = bitWidth == 8 || bitWidth == 16 || bitWidth == 32;
  result |= bitWidth == 64;
  return result;
}

// TODO: Currently this function doesn't take into account large slices, when reporting
// supported parameters. This should be fixed.
bool JointMatrixFuncsResolutionPass::ValidateLoadStore(bool isLoad, unsigned operationLayout,
                                                       const JointMatrixTypeDescription *desc, Value *ctx) {
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
    msg += " <" + std::to_string(desc->rows) + " x " + std::to_string(desc->columns) + " x i" +
           std::to_string(desc->bitWidth) + "> with " + nameLayout(operationLayout);
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
          if (i > 3)
            msg += ", ";
          msg += std::to_string(bitWidth);
        }
      }
    }
    if (result & INVALID_LAYOUT) {
      msg += "\n -> unsupported operation layout";
      msg += "\n    supported values: ";
      for (unsigned i = 0; i < LayoutMax; i++) {
        if ((1 << i) & params.layouts) {
          if (i > 0)
            msg += ", ";
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

static void AppendSetToString(std::string &msg, const std::set<unsigned> &numbers) {
  bool first = true;
  for (unsigned n : numbers) {
    if (!first)
      msg += ", ";
    msg += std::to_string(n);
    first = false;
  }
}

void JointMatrixFuncsResolutionPass::Validate2DBlockLoadStore(GetMatrixFuncNameOperation operation,
                                                              unsigned operationLayout, unsigned address_space,
                                                              const JointMatrixTypeDescription *desc, Value *ctx) {
  std::string operationName;
  if (operation == LoadChecked) {
    operationName = "checked load";
  } else if (operation == StoreChecked) {
    operationName = "checked store";
  } else {
    operationName = "prefetch";
  }
  if (IGC_GET_FLAG_VALUE(JointMatrixLoadStoreOpt) < 3) {
    std::string msg =
        "Matrix " + operationName + " requires Joint Matrix load/store flag (JointMatrixLoadStoreOpt) to be set to 3.";
    m_Ctx->EmitError(msg.c_str(), ctx);
  }
  if (address_space != ADDRESS_SPACE_GENERIC && address_space != ADDRESS_SPACE_GLOBAL) {
    std::string msg = "Unsupported address space. Matrix " + operationName + " supports generic and global pointers.";
    m_Ctx->EmitError(msg.c_str(), ctx);
  }
  if (!m_Ctx->platform.hasExecSize16DPAS()) {
    std::string msg = "SYCL Joint Matrix " + operationName + " API is not supported on targeted GPU device.";
    m_Ctx->EmitError(msg.c_str(), ctx);
  }
  if ((operation == LoadChecked || operation == StoreChecked) && isSupprtedLargeSlice(desc, true)) {
    return;
  }

  std::set<unsigned> supported_rows = {1, 2, 4, 8, 16, 32};
  std::set<unsigned> supported_cols = {8, 16, 32, 64};
  if (m_Ctx->platform.isCoreChildOf(IGFX_XE3P_CORE)) {
    supported_cols.insert(128);
    supported_cols.insert(256);
  }
  if (supported_rows.find(desc->rows) == supported_rows.end()) {
    std::string msg = "Unsupported row parameter for matrix " + operationName + ": " + std::to_string(desc->rows) +
                      ". Supported values: ";
    AppendSetToString(msg, supported_rows);
    msg += ".";
    m_Ctx->EmitError(msg.c_str(), ctx);
  }
  if (supported_cols.find(desc->columns) == supported_cols.end()) {
    std::string msg = "Unsupported column parameter for matrix " + operationName + ": " +
                      std::to_string(desc->columns) + ". Supported values: ";
    AppendSetToString(msg, supported_cols);
    msg += ".";
    m_Ctx->EmitError(msg.c_str(), ctx);
  }
  if (operation == LoadChecked || operation == Prefetch) {
    unsigned elemBytes = desc->bitWidth / 8;
    unsigned perRowBytes = desc->columns * elemBytes;
    bool supported = (perRowBytes <= 64);
    // blockWidth * data size should be <= 64B or equal to 256B for 2D block loads for XE3P+
    if (m_Ctx->platform.isCoreChildOf(IGFX_XE3P_CORE))
      supported |= (perRowBytes == 256);

    if (!supported) {
      std::string msg = "Matrix " + operationName +
                        " size limit exceeded. "
                        "(columns * dataSize) has to be (equal or less than 64B)";
      if (m_Ctx->platform.isCoreChildOf(IGFX_XE3P_CORE))
        msg += " or (equal to 256B)";
      msg += ".\nLimit exceeded with values: " + std::to_string(desc->columns) + " * " + std::to_string(elemBytes) +
             "B = " + std::to_string(perRowBytes) + "B";
      m_Ctx->EmitError(msg.c_str(), ctx);
    }
  }

  if (operation == StoreChecked) {
    if (operationLayout == LayoutColumnMajor) {
      std::string msg = "Matrix " + operationName + " is not supported for ColumnMajor layout.";
      m_Ctx->EmitError(msg.c_str(), ctx);
    }
  }
}

std::string JointMatrixFuncsResolutionPass::GetMatrixFuncName(GetMatrixFuncNameOperation operation,
                                                              unsigned operationLayout, unsigned address_space,
                                                              const JointMatrixTypeDescription *desc,
                                                              const std::string &prefix) {

  /* For matrix A Row major and PackedA are the same.
   * Both are in row major format. */
  unsigned matrixLayout = desc->layout;
  if ((operation == Load || operation == LoadChecked) && matrixLayout == LayoutRowMajor && desc->use == UseMatrixA) {
    matrixLayout = LayoutPackedA;
  }

  std::string name = prefix;

  if (operation != Prefetch) {
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
  }

  if (operation != GetCoord && operation != Prefetch) {
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
      IGC_ASSERT_MESSAGE(matrixLayout == operationLayout, "Unexpected load/store layout.");
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

  // We are done creating the mangling for get_coord and prefetch
  if (operation == Prefetch || operation == GetCoord)
    return name;

  name += "_" + std::to_string(getNumRowsPerWI(desc));

  // Checked load/store is available only for global address space. Generic address space is accepted and assumed
  // global.
  if (operation != LoadChecked && operation != StoreChecked) {
    if (address_space == ADDRESS_SPACE_GLOBAL) {
      name += "_global";
    } else if (address_space == ADDRESS_SPACE_LOCAL) {
      name += "_local";
    } else {
      name += "_generic";
    }
  }

  if (operation == Load || operation == LoadChecked) {
    name += "_v8i8_pi32_i32";
  } else if (operation == Store || operation == StoreChecked) {
    name += "_pi64_v8i8";
  }
  return name;
}

static unsigned parseNumber(StringRef name, unsigned *offset) {
#define BUFFER_SIZE 16
  char buffer[BUFFER_SIZE + 1];
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
bool JointMatrixFuncsResolutionPass::parseMatrixTypeNameLegacy(const Type *opaqueType,
                                                               JointMatrixTypeDescription *outDescription) {
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
    LLVM_DEBUG(dbgs() << msg << "\n");
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

  [[maybe_unused]] bool isBitWidthSupported = ValidateIntegerBitWidth(outDescription->bitWidth);
  IGC_ASSERT_MESSAGE(isBitWidthSupported, "Unexpected matrix element size.");

  return true;
}

/* Format is either
 * %spirv.JointMatrixINTEL._int_8_16_3_3_2   --> C
 * %spirv.JointMatrixINTEL._char_8_32_0_3_0  --> A
 * %spirv.JointMatrixINTEL._char_32_16_2_3_1 --> B
 * type, rows, cols, layout, scope, use
 *
 * or
 *
 * %spirv.CooperativeMatrixKHR._int_3_8_16_2   --> C
 * %spirv.CooperativeMatrixKHR._char_3_8_32_0  --> A
 * %spirv.CooperativeMatrixKHR._char_3_32_16_1 --> B
 * type, scope, rows, cols, use
 * */
bool JointMatrixFuncsResolutionPass::ParseMatrixTypeName(Type *opaqueType, JointMatrixTypeDescription *outDescription) {
  StringRef name = GetMatrixTypeName(opaqueType);

  if (name.startswith("intel.joint_matrix")) {
    return parseMatrixTypeNameLegacy(opaqueType, outDescription);
  }

  auto isTET = IGCLLVM::isTargetExtTy(opaqueType);

  StringRef jointMatrixPrefix = isTET ? "spirv.JointMatrixINTEL" : "spirv.JointMatrixINTEL._";
  StringRef coopMatrixPrefix = isTET ? "spirv.CooperativeMatrixKHR" : "spirv.CooperativeMatrixKHR._";
  bool IsJointMatrix = name.consume_front(jointMatrixPrefix);

  if (!IsJointMatrix && !name.consume_front(coopMatrixPrefix)) {
    std::string msg = "Unexpected Matrix type name: '" + name.str() + "', unknown prefix.";
    LLVM_DEBUG(dbgs() << msg << "\n");
    m_Ctx->EmitError(msg.c_str(), nullptr);
    return false;
  }

  if (isTET) {
#if LLVM_VERSION_MAJOR >= 16
    return ParseMatrixTypeNameExtTypeDetails(opaqueType, IsJointMatrix, outDescription);
#else
    return false;
#endif
  }

  return ParseMatrixTypeNameNonExtTypeDetails(opaqueType, name, IsJointMatrix, outDescription);
}

StringRef JointMatrixFuncsResolutionPass::GetMatrixTypeName(Type *opaqueType) {
  if (IGCLLVM::isTargetExtTy(opaqueType)) {
    return IGCLLVM::getTargetExtName(opaqueType);
  }

  const PointerType *ptrType = cast<PointerType>(opaqueType);
  return IGCLLVM::getNonOpaquePtrEltTy(ptrType)->getStructName();
}

#if LLVM_VERSION_MAJOR >= 16
bool IGC::JointMatrixFuncsResolutionPass::ParseMatrixTypeNameExtTypeDetails(
    Type *opaqueType, bool IsJointMatrix, IGC::JointMatrixTypeDescription *outDescription) {
  auto *targetTy = dyn_cast<TargetExtType>(opaqueType);

  if (!targetTy) {
    std::string msg = "Unexpected Matrix type. Expected TargetExt";
    LLVM_DEBUG(dbgs() << msg << "\n");
    m_Ctx->EmitError(msg.c_str(), nullptr);
    return false;
  }

  // Example of LLVM IR of target type extension with "float" type_param
  // -------------------------------------[....] single type_param
  // target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2)
  int typeParamExpectedCount = 1;
  if (targetTy->type_params().size() != typeParamExpectedCount) {
    std::string msg =
        "Unexpected Matrix type parameter(s) count. Expected count: " + std::to_string(typeParamExpectedCount) +
        ". Full Name: '" + GetMatrixTypeName(opaqueType).str() + "'.";

    LLVM_DEBUG(dbgs() << msg << "\n");
    m_Ctx->EmitError(msg.c_str(), nullptr);
    return false;
  }

  llvm::Type *typeParam = *(targetTy->type_param_begin());

  if (typeParam->isIntegerTy()) {
    outDescription->bitWidth = typeParam->getIntegerBitWidth();

    if (!ValidateIntegerBitWidth(outDescription->bitWidth)) {
      std::string msg = "Unexpected Matrix integer size: '" + std::to_string(outDescription->bitWidth) + "'.";
      LLVM_DEBUG(dbgs() << msg << "\n");
      m_Ctx->EmitError(msg.c_str(), nullptr);
      return false;
    }

    outDescription->isFloating = false;
  } else if (typeParam->isDoubleTy()) {
    outDescription->bitWidth = 64;
    outDescription->isFloating = true;
  } else if (typeParam->isFloatTy()) {
    outDescription->bitWidth = 32;
    outDescription->isFloating = true;
  } else if (typeParam->isHalfTy()) {
    outDescription->bitWidth = 16;
    outDescription->isFloating = true;
  } else {
    std::string msg =
        "Unexpected Matrix type name: '" + GetMatrixTypeName(opaqueType).str() + "', unknown element type.";
    LLVM_DEBUG(dbgs() << msg << "\n");
    m_Ctx->EmitError(msg.c_str(), nullptr);
    return false;
  }

  // As of now int_param called "scope" is not used
  unsigned rows = 0;
  unsigned cols = 0;
  unsigned use = UseMax;

  if (!IsJointMatrix) {
    // Example of LLVM IR of target type extension
    // with 4 params and its format description
    //                               type, [scope, rows, cols, use]
    // -------------------------------------------[............] int_params
    // target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2)
    if (targetTy->int_params().size() == 4) {
      rows = targetTy->int_params()[1];
      cols = targetTy->int_params()[2];
      use = targetTy->int_params()[3];
    } else {
      std::string msg = "Unexpected Matrix type format parameters count. Expected count: 4. "
                        "Full Name: '" +
                        GetMatrixTypeName(opaqueType).str() + "'.";
      LLVM_DEBUG(dbgs() << msg << "\n");
      m_Ctx->EmitError(msg.c_str(), nullptr);
      return false;
    }
  } else {
    // Example of LLVM IR of target type extension
    // with 4 params and its format description
    //                               type, [rows, cols, layout, scope]
    // ------------------------------------[..........] int_params
    // target("spirv.JointMatrixINTEL", i8, 3, 8, 8, 2)
    if (targetTy->int_params().size() == 4) {
      // Basing on original comments "use" param may not always be present
      rows = targetTy->int_params()[0];
      cols = targetTy->int_params()[1];
      unsigned legacyLayout = targetTy->int_params()[2];

      // If use parameter is not present deduce the correct use from legacy layout
      use = GetUseFromLegacyLayout(legacyLayout);
    }
    // Example of LLVM IR of target type extension
    // with 5 params and its format description
    //                               type, [rows, cols, layout, scope, use]
    // -------------------------------------[.............] int_params
    // target("spirv.JointMatrixINTEL", i32, 3, 8, 8, 2, 0)
    else if (targetTy->int_params().size() == 5) {
      rows = targetTy->int_params()[0];
      cols = targetTy->int_params()[1];
      // not used
      // unsigned layout = targetTy->int_params()[2];
      use = targetTy->int_params()[4];
    } else {
      std::string msg = "Unexpected Matrix type format parameters count. Expected count: 4 or 5. "
                        "Full Name: '" +
                        GetMatrixTypeName(opaqueType).str() + "'.";
      LLVM_DEBUG(dbgs() << msg << "\n");
      m_Ctx->EmitError(msg.c_str(), nullptr);
      return false;
    }
  }

  outDescription->rows = rows;
  outDescription->columns = cols;
  outDescription->use = use;

  if (!SetLayoutFromUse(outDescription))
    return false;

  return true;
}
#endif

bool JointMatrixFuncsResolutionPass::ParseMatrixTypeNameNonExtTypeDetails(Type *opaqueType, StringRef name,
                                                                          bool IsJointMatrix,
                                                                          JointMatrixTypeDescription *outDescription) {
  if (name.consume_front("int_")) {
    outDescription->bitWidth = 32;
    outDescription->isFloating = false;
  } else if (name.consume_front("short_")) {
    outDescription->bitWidth = 16;
    outDescription->isFloating = false;
  } else if (name.consume_front("char_")) {
    outDescription->bitWidth = 8;
    outDescription->isFloating = false;
  } else if (name.consume_front("double_")) {
    outDescription->bitWidth = 64;
    outDescription->isFloating = true;
  } else if (name.consume_front("float_")) {
    outDescription->bitWidth = 32;
    outDescription->isFloating = true;
  } else if (name.consume_front("half_")) {
    outDescription->bitWidth = 16;
    outDescription->isFloating = true;
  } else {
    std::string msg =
        "Unexpected Matrix type name: '" + GetMatrixTypeName(opaqueType).str() + "', unknown element type.";
    LLVM_DEBUG(dbgs() << msg << "\n");
    m_Ctx->EmitError(msg.c_str(), nullptr);
    return false;
  }

  unsigned offset = 0;
  unsigned scope = 0;
  /* Use parameter might not be present in older version of SPIR-V. In such
   * case it should be reconstructed from the layout. Handling of this
   * special case should be removed once we stop to support legacy SPIR-V
   * specification.*/

  if (!IsJointMatrix) {
    scope = parseNumber(name, &offset);
    offset += 1; /* Skip delimiter, '_'. */
    outDescription->rows = parseNumber(name, &offset);
    offset += 1; /* Skip delimiter, '_'. */
    outDescription->columns = parseNumber(name, &offset);
    offset += 1; /* Skip delimiter, '_' */
    outDescription->use = parseNumber(name, &offset);
  } else {
    outDescription->rows = parseNumber(name, &offset);
    offset += 1; /* Skip delimiter, '_'. */
    outDescription->columns = parseNumber(name, &offset);
    offset += 1; /* Skip delimiter, '_' */
    unsigned legacyLayout = parseNumber(name, &offset);
    offset += 1; /* Skip delimiter, '_' */
    scope = parseNumber(name, &offset);

    if (offset < name.size()) {
      offset += 1; /* Skip delimiter, '_' */
      outDescription->use = parseNumber(name, &offset);
    } else {
      /* If use parameter is not present deduce the correct use from legacy
       * layout: */
      outDescription->use = GetUseFromLegacyLayout(legacyLayout);
    }
  }

  /* currently unused: */
  (void)scope;

  if (!SetLayoutFromUse(outDescription))
    return false;

  return true;
}

unsigned JointMatrixFuncsResolutionPass::GetUseFromLegacyLayout(unsigned int legacyLayout) {
  if (legacyLayout == LayoutPackedA)
    return UseMatrixA;
  if (legacyLayout == LayoutPackedB)
    return UseMatrixB;
  return UseAccumulator;
}

bool JointMatrixFuncsResolutionPass::SetLayoutFromUse(IGC::JointMatrixTypeDescription *outDescription) {
  if (outDescription->use == UseMatrixA) {
    outDescription->layout = LayoutPackedA;
  } else if (outDescription->use == UseMatrixB) {
    outDescription->layout = LayoutPackedB;
  } else if (outDescription->use == UseAccumulator) {
    outDescription->layout = LayoutRowMajor;
  } else {
    std::string msg = "Unexpected Matrix 'use' value: '" + std::to_string(outDescription->use) + "'. Unknown use type.";
    LLVM_DEBUG(dbgs() << msg << "\n");
    m_Ctx->EmitError(msg.c_str(), nullptr);
    return false;
  }

  return true;
}

// As both float and tf32 types are represented as float, the TF32 type info
// is lost starting at the SPIRV level. Therefore, we use the following
// heuristics for determing the TF32 type.  We know that the K value is fixed
// for a given platform, matrix and data type. Also TF32 is supported starting
// from PVC.  Therefore we use the K == 8, along with data type check to
// determine the TF32 type.  For float the K value is 16 (hence this function
// will return false).
static bool isTF32(const JointMatrixTypeDescription *desc) {
  bool matATF32 = desc->layout == LayoutPackedA && desc->isFloating && desc->bitWidth == 32 && desc->columns == 8;
  bool matBTF32 = desc->layout == LayoutPackedB && desc->isFloating && desc->bitWidth == 32 && desc->rows == 8;
  if (matATF32 || matBTF32)
    return true;
  return false;
}

unsigned JointMatrixFuncsResolutionPass::getNumRowsPerWI(const JointMatrixTypeDescription *desc) {
  IGC_ASSERT_MESSAGE(m_SIMDSize > 0, "Unexpected sub group size.");
  IGC_ASSERT_MESSAGE(desc->contribBitWidth > 0, "Unexpected bit width of contribution type.");
  unsigned totalBits = desc->rows * desc->columns * desc->bitWidth;
  unsigned canHandleBits = desc->contribBitWidth * m_SIMDSize;
  return totalBits / canHandleBits + (totalBits % canHandleBits ? 1 : 0);
}

static Type *getAccElemTypeFromDesc(LLVMContext &ctx, const JointMatrixTypeDescription &desc) {
  IGC_ASSERT_MESSAGE(desc.bitWidth == 16 || desc.bitWidth == 32 || desc.bitWidth == 64,
                     "Unsupported accumulator bitwidth.");
  if (desc.bitWidth == 32)
    return desc.isFloating ? Type::getFloatTy(ctx) : Type::getInt32Ty(ctx);
  return desc.isFloating ? Type::getHalfTy(ctx) : Type::getInt16Ty(ctx);
}

// Create <type x 64> type used for Accumulator 32x64 and 32x32 in structure
// {<type x 64>, <type x 64>}.
static Type *getAccHalfVec64Type(Type *type) { return IGCLLVM::FixedVectorType::get(type, 64); }

// Create <type x 64> type used for Accumulator 32x64 and 32x32 in structure
// {<type x 64>, <type x 64>}.
static Type *getAccHalfVec64Type(LLVMContext &ctx, const JointMatrixTypeDescription &desc) {
  Type *elementType = getAccElemTypeFromDesc(ctx, desc);
  return getAccHalfVec64Type(elementType);
}

// Check if it is special case: Accumulator 32x64.
static bool isAccumulator32x64(const JointMatrixTypeDescription &desc) {
  return (desc.layout == LayoutRowMajor && desc.rows == 32 && desc.columns == 64);
}

// Check if it is special case: Accumulator 32x32.
static bool isAccumulator32x32(const JointMatrixTypeDescription &desc) {
  return (desc.layout == LayoutRowMajor && desc.rows == 32 && desc.columns == 32);
}

#if LLVM_VERSION_MAJOR >= 16
// When we alloca target extension type, later it is "ptr".
// We need to figure out the type of "ptr" by traversing up.
// Example:
// %0 = alloca target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0)
// %ptr = call spir_func ptr
// @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR(ptr %0,
// i64 4)
Type *JointMatrixFuncsResolutionPass::TryFindTargetExtensionTypeOfOpaquePtr(Value *V) {
  if (!V)
    return nullptr;

  for (auto &use : V->uses()) {
    if (auto *ai = dyn_cast<AllocaInst>(use)) {
      auto aiTy = ai->getAllocatedType();
      if (IGCLLVM::isTargetExtTy(aiTy))
        return aiTy;
    } else if (auto *cast = dyn_cast<CastInst>(use)) {
      return TryFindTargetExtensionTypeOfOpaquePtr(cast->getOperand(0));
    } else if (auto *gep = dyn_cast<GetElementPtrInst>(use)) {
      auto gepTy = gep->getResultElementType();
      if (IGCLLVM::isTargetExtTy(gepTy))
        return gepTy;
    }
  }

  return nullptr;
}
// When resolving e.g prefetch we need to figure out type of ptr
// We can do it by traversing up.
// It is similar to approach TryFindTargetExtensionTypeOfOpaquePtr
Type *JointMatrixFuncsResolutionPass::TryFindTypeOfOpaquePtr(Value *V) {
  if (!V)
    return nullptr;

  for (auto &use : V->uses()) {
    if (auto *ai = dyn_cast<AllocaInst>(use)) {
      auto aiTy = ai->getAllocatedType();
      return aiTy;
    } else if (auto *cast = dyn_cast<CastInst>(use)) {
      return TryFindTypeOfOpaquePtr(cast->getOperand(0));
    } else if (auto *gep = dyn_cast<GetElementPtrInst>(use)) {
      return gep->getResultElementType();
    } else if (auto *bitcast = dyn_cast<BitCastInst>(use)) {
      if (!IGCLLVM::isPointerTy(bitcast->getSrcTy()))
        return bitcast->getSrcTy();

      return TryFindTypeOfOpaquePtr(bitcast->getOperand(0));
    }
  }

  return nullptr;
}
#endif

Type *JointMatrixFuncsResolutionPass::ResolveType(Type *inputType, JointMatrixTypeDescription *outDesc) {
  IGC_ASSERT_EXIT_MESSAGE(inputType && (inputType->isPointerTy() || IGCLLVM::isTargetExtTy(inputType)),
                          "Unexpected type in matrix function resolution.");

#if LLVM_VERSION_MAJOR >= 16
  IGC_ASSERT_EXIT_MESSAGE(!IGCLLVM::isPointerTy(inputType),
                          "Unexpected opaque pointer. Expected TargetExtensionType instead.");
#endif

  JointMatrixTypeDescription desc;
  bool parseResult = ParseMatrixTypeName(inputType, &desc);
  IGC_ASSERT_EXIT_MESSAGE(parseResult, "Failed to parse matrix type.");
  /* For matrix A Row major and PackedA are the same.
   * Both are in row major format. */
  if (desc.layout == LayoutRowMajor && desc.use == UseMatrixA) {
    desc.layout = LayoutPackedA;
  }

  LLVMContext &ctx = inputType->getContext();
  Type *resolvedType = nullptr;

  Type *baseType = Type::getInt32Ty(ctx);
  desc.contribBitWidth = 32;

  if (desc.layout == LayoutPackedA) {
    if (!isTF32(&desc) && m_Ctx->platform.hasExecSize16DPAS()) {
      baseType = Type::getInt16Ty(ctx);
      desc.contribBitWidth = 16;
    }
    // At this point only Accumulator can be RowMajor
    // in ParseMatrixTypeName layout is set based on use
  } else if (desc.layout == LayoutRowMajor) {
    baseType = getAccElemTypeFromDesc(ctx, desc);
    desc.contribBitWidth = desc.bitWidth;
  }

  if (desc.bitWidth == 64) {
    IGC_ASSERT(desc.isFloating);
    desc.contribBitWidth = desc.bitWidth;
    baseType = Type::getDoubleTy(ctx);
  }

  if (outDesc != nullptr)
    *outDesc = desc;

  // Special case: this should ideally be a vector of <float x 128>. However since IGC
  // code gen supports vector operations only on vectors up to 64
  // entries, we model this slice as structure of {<float x 64>, <float x 64>}.
  // Alternative approaches summary:
  // 1. [2 x <64 x float>] does not always work because ArrayTy is not supported in IGC code gen.
  // 2. Replacing [2 x <64 x float>] later or implementing support for ArrayTy in IGC code gen looks more complicated
  // and doesn't bring benefits comparing to selected option to use structure.

  if (isAccumulator32x64(desc) || isAccumulator32x32(desc)) {
    Type *memberType = getAccHalfVec64Type(ctx, desc);
    resolvedType = StructType::get(ctx, ArrayRef<Type *>({memberType, memberType}));
  } else {
    unsigned vectorSize = getNumRowsPerWI(&desc);
    if (vectorSize == 1)
      resolvedType = baseType;
    else
      resolvedType = IGCLLVM::FixedVectorType::get(baseType, vectorSize);
  }

  IGC_ASSERT_EXIT_MESSAGE(resolvedType != nullptr, "Failed to resolve matrix type.");

  CacheResolvedTypes(inputType, resolvedType);
  return resolvedType;
}

static uint64_t constIntValue(const Value *v) { return cast<ConstantInt>(v)->getLimitedValue(); }

// create value {type, type} with val0 and val1 as values of each element
template <class BuilderT> static Value *createPair(BuilderT *builder, Type *type, Value *val0, Value *val1) {
  Value *pair = UndefValue::get(StructType::get(builder->getContext(), ArrayRef<Type *>({type, type})));
  pair = builder->CreateInsertValue(pair, val0, {0});
  pair = builder->CreateInsertValue(pair, val1, {1});
  return pair;
}

template <typename T> static int resolveCacheControlDecorations(CodeGenContext *ctx, Value *pointerValue) {
  static_assert(std::is_same_v<T, LoadCacheControl> || std::is_same_v<T, StoreCacheControl>);
  auto spirvDecorations = parseSPIRVDecorationsFromMD(pointerValue);
  for (auto &[DecorationId, MDNodes] : spirvDecorations) {
    if (DecorationId == getDecorationIdCacheControl<T>()) {
      CacheControlFromMDNodes controls = resolveCacheControlFromMDNodes<T>(ctx, MDNodes);
      if (controls.isInvalid)
        ctx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.");
      return controls.value;
    }
  }
  return 0;
}

Instruction *JointMatrixFuncsResolutionPass::ResolvePrefetch(CallInst *CI) {
  Value *ptrVal = CI->getArgOperand(0);
  Value *numRowsVal = CI->getArgOperand(1);
  Value *numColsVal = CI->getArgOperand(2);
  Value *cacheLevelVal = CI->getArgOperand(3);
  Value *layoutVal = CI->getArgOperand(4);
  Value *strideVal = CI->getArgOperand(5);
  unsigned loadLayout = (unsigned)constIntValue(layoutVal);
  unsigned cacheLevel = (unsigned)constIntValue(cacheLevelVal);

  JointMatrixTypeDescription desc;
  desc.rows = (unsigned)constIntValue(numRowsVal);
  desc.columns = (unsigned)constIntValue(numColsVal);

  // Pointer type resolution
  Type *ptrElemType = nullptr;

#if LLVM_VERSION_MAJOR >= 16
  if (IGCLLVM::isPointerTy(ptrVal->getType()))
    ptrElemType = TryFindTypeOfOpaquePtr(ptrVal);
  else
#endif
  {
    // To be removed after switch to LLVM 16 + full opaque pointers enablement
    PointerType *ptrType = cast<PointerType>(ptrVal->getType());
    ptrElemType = IGCLLVM::getNonOpaquePtrEltTy(ptrType);
  }

  if (!ptrElemType) {
    m_Ctx->EmitError("Pointer type not found when resolving prefetch", ptrVal);
    return nullptr;
  }

  if (StructType *structTy = dyn_cast<StructType>(ptrElemType)) {
    if (structTy->getNumElements() == 1) {
      ptrElemType = structTy->getElementType(0);
      // we assume that only custom floating point types are wrapped into
      // structs
      desc.isFloating = true;
    }
  }

  if (ptrElemType->isHalfTy()) {
    desc.bitWidth = 16;
    desc.isFloating = true;
  } else if (ptrElemType->isFloatTy()) {
    desc.bitWidth = 32;
    desc.isFloating = true;
  } else if (ptrElemType->isDoubleTy()) {
    desc.bitWidth = 64;
    desc.isFloating = true;
  } else if (ptrElemType->isIntegerTy()) {
    desc.bitWidth = cast<IntegerType>(ptrElemType)->getBitWidth();

    if (!ValidateIntegerBitWidth(desc.bitWidth)) {
      std::string msg = "Unexpected Matrix integer size: '" + std::to_string(desc.bitWidth) + "'.";
      LLVM_DEBUG(dbgs() << msg << "\n");
      m_Ctx->EmitError(msg.c_str(), ptrVal);
      return nullptr;
    }
  } else {
    m_Ctx->EmitError("Failed to resolve matrix prefetch pointer type", ptrVal);
  }

  LLVMContext &ctx = CI->getContext();
  Type *retTy = Type::getVoidTy(ctx);
  Module *M = CI->getParent()->getModule();
  unsigned address_space = ptrVal->getType()->getPointerAddressSpace();

  // Prefetch validation
  Validate2DBlockLoadStore(Prefetch, loadLayout, address_space, &desc, CI);

  std::string funcName =
      GetMatrixFuncName(Prefetch, loadLayout, address_space, &desc, "__builtin_spriv_OpJointMatrixPrefetchINTEL_");
  FunctionType *funcType =
      FunctionType::get(retTy, {ptrVal->getType(), strideVal->getType(), Type::getInt32Ty(ctx)}, false);

  InstsToErase.insert(CI);
  IRBuilder<> builder(CI);

  int targetCacheOpt =
      (cacheLevel <= 1 ? LSC_L1C_WT_L3C_WB
                       : LSC_L1UC_L3C_WB); // This is an arbitrary mapping that hopefully gets specified in the future
  Value *cacheOpt = builder.getInt32(targetCacheOpt);

  std::vector<Value *> Args = {ptrVal, strideVal, cacheOpt};
  Instruction *newCall = builder.CreateCall(M->getOrInsertFunction(funcName, funcType), Args);
  newCall->setDebugLoc(CI->getDebugLoc());
  return newCall;
}

template <bool IsJointMatrix, bool IsChecked> Instruction *JointMatrixFuncsResolutionPass::ResolveLoad(CallInst *CI) {
  LLVM_DEBUG(dbgs() << "   -- RESOLVE LOAD: " << *CI << "\n");
  using OpVariant =
      typename std::conditional_t<IsJointMatrix,
                                  std::conditional_t<IsChecked, JointMatrix::LoadChecked::Op, JointMatrix::Load::Op>,
                                  CoopMatrix::Load::Op>;
  Value *ptrVal = CI->getArgOperand(OpVariant::Pointer);
  Value *strideVal = CI->getArgOperand(OpVariant::Stride);
  unsigned loadLayout = (unsigned)constIntValue(CI->getArgOperand(OpVariant::Layout));

  // Matrix builtins expect i64 stride
  Type *i64Ty = Type::getInt64Ty(CI->getContext());
  if (strideVal->getType() != i64Ty) {
    IRBuilder<> builder(CI);
    strideVal = builder.CreateZExtOrBitCast(strideVal, i64Ty);
  }

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
  if constexpr (IsChecked) {
    Validate2DBlockLoadStore(LoadChecked, loadLayout, address_space, &desc, CI);
  }

  InstsToErase.insert(CI);

  // Create alloca in the entry node of the function
  IRBuilder<> builder(&*CI->getFunction()->getEntryBlock().getFirstInsertionPt());
  builder.SetCurrentDebugLocation(CI->getDebugLoc());
  Value *sliceArray = builder.CreateAlloca(matTy, ADDRESS_SPACE_PRIVATE);

  builder.SetInsertPoint(CI);
  Value *dst = builder.CreateBitCast(sliceArray, arrayTy);
  Value *cacheOpt = builder.getInt32(resolveCacheControlDecorations<LoadCacheControl>(m_Ctx, ptrVal));

  std::string instructionName =
      IsChecked ? "__builtin_spriv_OpJointMatrixLoadCheckedINTEL_" : "__builtin_spriv_OpJointMatrixLoadINTEL_";
  std::string funcName =
      GetMatrixFuncName(IsChecked ? LoadChecked : Load, loadLayout, address_space, &desc, instructionName);
  FunctionType *funcType;
  std::vector<Value *> Args;
  if constexpr (IsChecked) {
    Value *yVal = CI->getArgOperand(OpVariant::Y);
    Value *xVal = CI->getArgOperand(OpVariant::X);
    Value *heightVal = CI->getArgOperand(OpVariant::Height);
    Value *widthVal = CI->getArgOperand(OpVariant::Width);
    funcType = FunctionType::get(retTy,
                                 {arrayTy, ptrVal->getType(), yVal->getType(), xVal->getType(), heightVal->getType(),
                                  widthVal->getType(), strideVal->getType(), Type::getInt32Ty(ctx)},
                                 false);
    Args = {dst, ptrVal, yVal, xVal, heightVal, widthVal, strideVal, cacheOpt};
  } else {
    funcType = FunctionType::get(retTy, {arrayTy, ptrVal->getType(), strideVal->getType(), cacheOpt->getType()}, false);
    Args = {dst, ptrVal, strideVal, cacheOpt};
  }
  Instruction *newCall = builder.CreateCall(M->getOrInsertFunction(funcName, funcType), Args);
  newCall->setDebugLoc(CI->getDebugLoc());

  newCall = builder.CreateLoad(matTy, sliceArray);

  return newCall;
}

template <bool IsJointMatrix, bool IsChecked> Instruction *JointMatrixFuncsResolutionPass::ResolveStore(CallInst *CI) {
  LLVM_DEBUG(dbgs() << "   -- RESOLVE STORE: " << *CI << "\n");
  using OpVariant =
      typename std::conditional_t<IsJointMatrix,
                                  std::conditional_t<IsChecked, JointMatrix::StoreChecked::Op, JointMatrix::Store::Op>,
                                  CoopMatrix::Store::Op>;
  Value *ptrVal = CI->getArgOperand(OpVariant::Pointer);
  Value *matrixVal = CI->getArgOperand(OpVariant::Matrix);
  Value *strideVal = CI->getArgOperand(OpVariant::Stride);

  // Matrix builtins expect i64 stride
  Type *i64Ty = Type::getInt64Ty(CI->getContext());
  if (strideVal->getType() != i64Ty) {
    IRBuilder<> builder(CI);
    strideVal = builder.CreateZExtOrBitCast(strideVal, i64Ty);
  }

  unsigned storeLayout = (unsigned)constIntValue(CI->getArgOperand(OpVariant::Layout));

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
  if constexpr (IsChecked) {
    Validate2DBlockLoadStore(StoreChecked, storeLayout, address_space, &desc, CI);
  }

  InstsToErase.insert(CI);

  // Create alloca in the entry node of the function
  IRBuilder<> builder(&*CI->getFunction()->getEntryBlock().getFirstInsertionPt());
  builder.SetCurrentDebugLocation(CI->getDebugLoc());
  Value *sliceArray = builder.CreateAlloca(matVal->getType(), ADDRESS_SPACE_PRIVATE);

  builder.SetInsertPoint(CI);
  builder.CreateStore(matVal, sliceArray);
  Value *src = builder.CreateBitCast(sliceArray, arrayTy);
  Value *cacheOpt = builder.getInt32(resolveCacheControlDecorations<StoreCacheControl>(m_Ctx, ptrVal));

  std::string instructionName =
      IsChecked ? "__builtin_spriv_OpJointMatrixStoreCheckedINTEL_" : "__builtin_spriv_OpJointMatrixStoreINTEL_";
  std::string funcName =
      GetMatrixFuncName(IsChecked ? StoreChecked : Store, storeLayout, address_space, &desc, instructionName);
  FunctionType *funcType;
  std::vector<Value *> Args;
  if constexpr (IsChecked) {
    Value *yVal = CI->getArgOperand(OpVariant::Y);
    Value *xVal = CI->getArgOperand(OpVariant::X);
    Value *heightVal = CI->getArgOperand(OpVariant::Height);
    Value *widthVal = CI->getArgOperand(OpVariant::Width);
    funcType = FunctionType::get(Type::getVoidTy(M->getContext()),
                                 {ptrVal->getType(), arrayTy, yVal->getType(), xVal->getType(), heightVal->getType(),
                                  widthVal->getType(), strideVal->getType(), Type::getInt32Ty(ctx)},
                                 false);
    Args = {ptrVal, src, yVal, xVal, heightVal, widthVal, strideVal, cacheOpt};
  } else {
    funcType = FunctionType::get(Type::getVoidTy(M->getContext()),
                                 {ptrVal->getType(), arrayTy, strideVal->getType(), cacheOpt->getType()}, false);
    Args = {ptrVal, src, strideVal, cacheOpt};
  }
  Instruction *newCall = CallInst::Create(M->getOrInsertFunction(funcName, funcType), Args, "", CI);
  newCall->setDebugLoc(CI->getDebugLoc());
  return newCall;
}

static PrecisionType getJointMatrixElementPrecison(const JointMatrixTypeDescription *desc, bool floatOp,
                                                   bool isUnsigned) {
  const unsigned width = desc->bitWidth;
  if (floatOp && width == 64) {
    return PrecisionType::DF;
  }
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

static PrecisionType getCoopMatrixElementPrecison(const JointMatrixTypeDescription *desc, unsigned OperandsMask,
                                                  unsigned Use, bool floatOp) {
  const unsigned width = desc->bitWidth;
  if (floatOp && width == 64) {
    return PrecisionType::DF;
  }
  if (OperandsMask & MatrixAAndBBFloat16ComponentsINTEL) {
    IGC_ASSERT_MESSAGE(floatOp && width == 16, "Wrong OpCooperativeMatrixMulAddKHR ops for BFloat16");
    return PrecisionType::BF16;
  }
  if (floatOp && width == 16) {
    IGC_ASSERT_MESSAGE(!OperandsMask, "Wrong OpCooperativeMatrixMulAddKHR ops for FP16");
    /* bf is passed as uint16_t, hf is using halfs */
    return desc->isFloating ? PrecisionType::FP16 : PrecisionType::BF16;
  }
  if (OperandsMask & MatrixAAndBTF32ComponentsINTEL || (floatOp && width == 32)) {
    return PrecisionType::TF32;
  }
  if (!floatOp && width == 8) {
    if (OperandsMask & MatrixASignedComponentsKHR && OperandsMask & MatrixBSignedComponentsKHR) {
      return PrecisionType::S8;
    } else if (OperandsMask & MatrixASignedComponentsKHR) {
      return Use == UseMatrixA ? PrecisionType::S8 : PrecisionType::U8;
    } else if (OperandsMask & MatrixBSignedComponentsKHR) {
      return Use == UseMatrixB ? PrecisionType::S8 : PrecisionType::U8;
    }
    return PrecisionType::U8;
  }

  if (floatOp && width == 8) {
    if (Use == UseMatrixA) {
      if (OperandsMask & MatrixABFloat8ComponentsINTEL) {
        return PrecisionType::BF8;
      } else if (OperandsMask & MatrixAHFloat8ComponentsINTEL) {
        return PrecisionType::HF8;
      }
    } else if (Use == UseMatrixB) {
      if (OperandsMask & MatrixBBFloat8ComponentsINTEL) {
        return PrecisionType::BF8;
      } else if (OperandsMask & MatrixBHFloat8ComponentsINTEL) {
        return PrecisionType::HF8;
      }
    }
  }
  return PrecisionType::PRECISION_UNUSED;
}

static const char *getElementName(PrecisionType P) {
  switch (P) {
  case PrecisionType::FP16:
    return "_fp16";
  case PrecisionType::BF16:
    return "_bf16";
  case PrecisionType::U8:
    return "_u8";
  case PrecisionType::S8:
    return "_s8";
  default:
    return "_i32";
  };
}

static bool isMADSupportedAsBuiltin(unsigned M, unsigned N, unsigned K) {
  if (M == 1 && N == 64 && K == 16)
    return true;
  if (M == 1 && N == 64 && K == 32)
    return true;
  if (M == 16 && N == 16 && K == 16)
    return true;
  if (M == 32 && N == 32 && K == 16)
    return true;
  if (M == 32 && N == 64 && K == 16)
    return true;
  if (M == 32 && N == 64 && K == 32)
    return true;
  return false;
}

static std::string getMADMatrixTypeName(JointMatrixTypeDescription desc) {
  if (desc.isFloating) {
    if (desc.bitWidth == 16)
      return "_fp16";
    return "_fp32";
  }
  if (desc.bitWidth == 16)
    return "_bf16";
  return "_i32";
}

static std::string getMADBuiltinName(unsigned M, unsigned N, unsigned K, PrecisionType PA, PrecisionType PB,
                                     JointMatrixTypeDescription cDesc, JointMatrixTypeDescription dDesc) {
  std::string funcName = "__builtin_spriv_OpJointMatrixMadINTEL_";
  funcName += std::to_string(M) + "x" + std::to_string(N) + "x" + std::to_string(K);

  funcName += getElementName(PA);
  funcName += getElementName(PB);
  funcName += getMADMatrixTypeName(cDesc);
  funcName += getMADMatrixTypeName(dDesc);
  return funcName;
}

static Function *getMADBuiltin(Module *Mod, unsigned M, unsigned N, unsigned K, PrecisionType PA, PrecisionType PB,
                               JointMatrixTypeDescription cDesc, JointMatrixTypeDescription dDesc) {
  std::string funcName = getMADBuiltinName(M, N, K, PA, PB, cDesc, dDesc);

  Type *retTy = Type::getVoidTy(Mod->getContext());
  Type *argTy = Type::getInt8PtrTy(Mod->getContext(), ADDRESS_SPACE_PRIVATE);

  FunctionType *funcType = FunctionType::get(retTy, {argTy, argTy, argTy, argTy}, false);

  Function *f = Mod->getFunction(funcName);
  if (f == nullptr) {
    f = Function::Create(funcType, GlobalValue::ExternalLinkage, funcName, Mod);
    f->setCallingConv(CallingConv::SPIR_FUNC);
    f->addFnAttr(llvm::Attribute::NoUnwind);
  }
  return f;
}

Instruction *JointMatrixFuncsResolutionPass::ResolveMad(CallInst *CI, unsigned OperationType) {
  if (!m_Ctx->platform.supportDpasInstruction()) {
    std::string msg = "OpJointMatrixMadINTEL/OpCooperativeMatrixMulAddKHR is not supported on this platform!";
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

  JointMatrixTypeDescription dDesc;
  Type *dMatTy = ResolveType(CI->getCalledFunction()->getReturnType(), &dDesc);

  IGC_ASSERT_MESSAGE(aDesc.layout == LayoutPackedA || aDesc.layout == LayoutRowMajor,
                     "Unexpected layout for matrix A in MAD operation.");
  IGC_ASSERT_MESSAGE(bDesc.layout == LayoutPackedB, "Unexpected layout for matrix B in MAD operation.");
  IGC_ASSERT_MESSAGE(cDesc.layout == LayoutRowMajor, "Unexpected layout for matrix C in MAD operation.");

  /* For Accumulator only supported types with bitwidth 16 are bfloat16 and
   * half both are floating point. bfloat16 is represented as short so
   * isFloating can't be used */
  const bool floatMad = cDesc.bitWidth == 16 ? true : cDesc.isFloating;

  PrecisionType PA = PrecisionType::PRECISION_UNUSED;
  PrecisionType PB = PrecisionType::PRECISION_UNUSED;
  if (OperationType == CooperativeOp) {
    const unsigned MulAddArgSize = CI->arg_size();
    const auto OperandsMask = MulAddArgSize > 3 ? cast<ConstantInt>(CI->getArgOperand(3))->getZExtValue() : 0;
    PA = getCoopMatrixElementPrecison(&aDesc, OperandsMask, UseMatrixA, floatMad);
    PB = getCoopMatrixElementPrecison(&bDesc, OperandsMask, UseMatrixB, floatMad);
  } else {
    PA = getJointMatrixElementPrecison(&aDesc, floatMad, isOperandUnsigned(OperationType, 0));
    PB = getJointMatrixElementPrecison(&bDesc, floatMad, isOperandUnsigned(OperationType, 1));
  }

  IGC_ASSERT_MESSAGE(PA != PrecisionType::PRECISION_UNUSED, "Invalid matrix A element type.");
  IGC_ASSERT_MESSAGE(PB != PrecisionType::PRECISION_UNUSED, "Invalid matrix B element type.");

  const unsigned M = aDesc.rows;
  const unsigned N = bDesc.columns;
  const unsigned K = bDesc.rows;

  Module *Mod = CI->getParent()->getModule();
  Instruction *dpasCall = nullptr;
  if (isMADSupportedAsBuiltin(M, N, K)) {
    Function *madFunc = getMADBuiltin(Mod, M, N, K, PA, PB, cDesc, dDesc);

    Value *aMat = Resolve(aMatVal);
    Value *bMat = Resolve(bMatVal);
    Value *cMat = Resolve(cMatVal);

    // Create alloca in the entry node of the function
    IRBuilder<> builder(&*CI->getFunction()->getEntryBlock().getFirstInsertionPt());
    builder.SetCurrentDebugLocation(CI->getDebugLoc());

    Value *sliceA = builder.CreateAlloca(aMat->getType(), ADDRESS_SPACE_PRIVATE);
    Value *sliceB = builder.CreateAlloca(bMat->getType(), ADDRESS_SPACE_PRIVATE);
    Value *sliceC = builder.CreateAlloca(cMat->getType(), ADDRESS_SPACE_PRIVATE);
    Value *sliceD = builder.CreateAlloca(dMatTy, ADDRESS_SPACE_PRIVATE);

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

    Value *args[4] = {ptrA, ptrB, ptrC, ptrD};

    builder.CreateCall(madFunc, args);
    dpasCall = builder.CreateLoad(dMatTy, sliceD);
  } else {
    int SD = 8;          // systolic depth, only 8 supported currently
    int RC = aDesc.rows; // repeat count, from 1 to 8

    IGC_ASSERT_MESSAGE(RC >= 1 && RC <= 8, "Unexpected repeat count in MAD operaion.");

    bool IsDpasw = false; // is wide

    LLVMContext &Ctx = CI->getContext();
    Type *intTy = Type::getInt32Ty(Ctx);
    Type *boolTy = Type::getInt1Ty(Ctx);

    Value *args[8];
    args[0] = Resolve(cMatVal);
    args[1] = Resolve(aMatVal);
    args[2] = Resolve(bMatVal);
    args[3] = ConstantInt::get(intTy, PA);
    args[4] = ConstantInt::get(intTy, PB);
    args[5] = ConstantInt::get(intTy, SD);
    args[6] = ConstantInt::get(intTy, RC);
    args[7] = ConstantInt::get(boolTy, IsDpasw);

    Type *ITys[4] = {dMatTy, cMatTy, aMatTy, bMatTy};

    GenISAIntrinsic::ID iid = GenISAIntrinsic::GenISA_sub_group_dpas;
    Function *dpasFunc = GenISAIntrinsic::getDeclaration(Mod, iid, ITys);
    dpasCall = CallInst::Create(dpasFunc, args, VALUE_NAME("dpas"), CI);
  }

  /* TODO: null check and throw error here */
  dpasCall->setDebugLoc(CI->getDebugLoc());
  InstsToErase.insert(CI);

  return dpasCall;
}

template <class BuilderT> static Type *getResolvedVectorElementType(Type *matrixType, BuilderT *builder) {
  if (IGCLLVM::FixedVectorType *ty = dyn_cast<IGCLLVM::FixedVectorType>(matrixType))
    return ty->getElementType();
  if (matrixType->isIntegerTy() || matrixType->isFloatingPointTy())
    return matrixType;
  if (matrixType->isStructTy() && matrixType->getNumContainedTypes() == 2) {
    return Type::getFloatTy(builder->getContext());
  }

  IGC_ASSERT_MESSAGE(false, "Unexpected type of matrix slice.");
  return nullptr;
}

int JointMatrixFuncsResolutionPass::getSliceSize(const JointMatrixTypeDescription *desc) {
  if (desc->bitWidth == 0) {
    IGC_ASSERT_MESSAGE(false, "Unexpected matrix element bit width.");
    return 1;
  }
  return getNumRowsPerWI(desc) * (desc->contribBitWidth / desc->bitWidth);
}

// expectation is both value and target type are IntegerType
template <class BuilderT> static Value *packFillValue(BuilderT *Builder, Value *V, IntegerType *TargetType) {
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
  Value *fillValue = CI->getArgOperand(JointMatrix::Construct::Op::Value);

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

  Value *slice = fillValue;

  // We create a vector only for rows > 1, as for rows = 1, we have one signle element instead of a one-element vector.
  if (IGCLLVM::FixedVectorType *ty = dyn_cast<IGCLLVM::FixedVectorType>(matTy)) {
    const int vectorSize = (int)ty->getNumElements();
    slice = UndefValue::get(matTy);
    for (int i = 0; i < vectorSize; i++) {
      slice = builder.CreateInsertElement(slice, fillValue, i);
    }
  }
  // Special cases Accumulator 32x64 and 32x32 is represented as {<type x 64>, <type x 64>}.
  else if (isAccumulator32x64(desc) || isAccumulator32x32(desc)) {
    Type *halfTy = getAccHalfVec64Type(builder.getContext(), desc);
    Value *slice0 = UndefValue::get(halfTy);
    Value *slice1 = UndefValue::get(halfTy);

    const int vectorSize = 64;
    for (int i = 0; i < vectorSize; i++) {
      slice0 = builder.CreateInsertElement(slice0, fillValue, i);
      slice1 = builder.CreateInsertElement(slice1, fillValue, i);
    }

    slice = createPair(&builder, halfTy, slice0, slice1);
  }

  InstsToErase.insert(CI);
  return slice;
}

Instruction *JointMatrixFuncsResolutionPass::ResolveFillChecked(CallInst *CI) {
  Value *fillValue = CI->getArgOperand(JointMatrix::ConstructChecked::Op::Value);
  Value *xVal = CI->getArgOperand(JointMatrix::ConstructChecked::Op::X);
  Value *yVal = CI->getArgOperand(JointMatrix::ConstructChecked::Op::Y);
  Value *heightVal = CI->getArgOperand(JointMatrix::ConstructChecked::Op::Height);
  Value *widthVal = CI->getArgOperand(JointMatrix::ConstructChecked::Op::Width);

  JointMatrixTypeDescription desc;
  Type *matTy = ResolveType(CI->getType(), &desc);
  LLVMContext &ctx = CI->getContext();
  Type *retTy = Type::getVoidTy(ctx);
  Type *arrayTy = Type::getInt8PtrTy(ctx, ADDRESS_SPACE_PRIVATE);

  Module *M = CI->getParent()->getModule();

  InstsToErase.insert(CI);

  // Create alloca in the entry node of the function
  IRBuilder<> builder(&*CI->getFunction()->getEntryBlock().getFirstInsertionPt());
  builder.SetCurrentDebugLocation(CI->getDebugLoc());
  Value *sliceArray = builder.CreateAlloca(matTy, ADDRESS_SPACE_PRIVATE);

  builder.SetInsertPoint(CI);
  Value *dst = builder.CreateBitCast(sliceArray, arrayTy);
  /* Cast floating types to integer types of the same size. This allows to
   * have a single set of store builtins for floats and integer */
  Value *fillValueCast = builder.CreateBitCast(fillValue, Type::getIntNTy(builder.getContext(), desc.bitWidth));

  std::string funcName = "__builtin_spirv_OpJointMatrixFillCheckedINTEL_i" + std::to_string(desc.bitWidth) + "_i" +
                         std::to_string(desc.contribBitWidth) + "_k" + std::to_string(desc.columns) + "_wi" +
                         std::to_string(getNumRowsPerWI(&desc));
  FunctionType *funcType = FunctionType::get(
      retTy,
      {arrayTy, yVal->getType(), xVal->getType(), heightVal->getType(), widthVal->getType(), fillValueCast->getType()},
      false);
  std::vector<Value *> Args = {dst, yVal, xVal, heightVal, widthVal, fillValueCast};

  Instruction *newCall = CallInst::Create(M->getOrInsertFunction(funcName, funcType), Args, "", CI);
  newCall->setDebugLoc(CI->getDebugLoc());
  newCall = builder.CreateLoad(matTy, sliceArray);

  return newCall;
}

Value *JointMatrixFuncsResolutionPass::ResolveWILength(CallInst *CI) {
  LLVM_DEBUG(dbgs() << "   -- RESOLVE WI LENGTH: " << *CI << "\n");
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

  std::string funcName =
      GetMatrixFuncName(GetCoord, -1 /*placeholder*/, -1 /*placeholder*/, &desc,
                        "__builtin_spirv_OpJointMatrixGetCoordINTEL_"); // GetCoordMatrixFuncName(&desc);

  IRBuilder builder(CI);

  // Argument type should be a i32??
  Type *argType = Type::getIntNTy(builder.getContext(), 32);
  elemIdx = builder.CreateTruncOrBitCast(elemIdx, argType);

  FunctionType *funcType = FunctionType::get(CI->getCalledFunction()->getReturnType(), {argType}, false);
  std::vector<Value *> Args = {elemIdx};

  Module *M = CI->getParent()->getModule();

  Instruction *newCall = builder.CreateCall(M->getOrInsertFunction(funcName, funcType), Args, "get_coord");
  newCall->setDebugLoc(CI->getDebugLoc());

  CI->replaceAllUsesWith(newCall);
  InstsToErase.insert(CI);

  return newCall;
}

// Updates index, in case value is packed and extracts element from vector using new index
template <class BuilderT>
static Value *updateIndexAndCreateSliceExtract(BuilderT *builder, Value *matrix, Value **index,
                                               unsigned contribBitWidth, unsigned bitWidth) {
  if (bitWidth != contribBitWidth) {
    /* Unpacking: */
    IGC_ASSERT_MESSAGE(bitWidth > 0, "Unexpected matrix element bit width.");
    uint64_t packFactor = contribBitWidth / bitWidth;
    *index = builder->CreateUDiv(*index, ConstantInt::get((*index)->getType(), packFactor));
  }
  return builder->CreateExtractElement(matrix, *index, "matrix.element");
}

// Creates offset to point to element we need to adress by index inside packed value
// For example if value is i64 (valBitWidth == 64) and element is i32 (elemBitWidth == 32)
// Then depending on index, offset can be either 0 or 32
template <class BuilderT>
static Value *createOffsetForPackedValue(BuilderT *builder, Value *index, unsigned valBitWidth, unsigned elemBitWidth) {
  uint64_t packFactor = valBitWidth / elemBitWidth;
  Value *offset = builder->CreateURem(index, ConstantInt::get(index->getType(), packFactor));
  offset = builder->CreateMul(offset, ConstantInt::get(offset->getType(), elemBitWidth));
  return builder->CreateTruncOrBitCast(offset, Type::getIntNTy(builder->getContext(), valBitWidth));
}

// Returns element extracted from a packed value by index
// First we find out offset inside the value
// Then we shift right value to offset to move data we need to the low part of value
// Then we trunkate to the bit width of element
template <class BuilderT>
static Value *unpackElementFromPackedValue(BuilderT *builder, Value *index, Value *value, unsigned valBitWidth,
                                           unsigned elemBitWidth) {
  Value *offset = createOffsetForPackedValue(builder, index, valBitWidth, elemBitWidth);
  Value *element = builder->CreateAShr(value, offset);
  return builder->CreateTruncOrBitCast(element, Type::getIntNTy(builder->getContext(), elemBitWidth));
}

// Merges component into packed value, addressed by index
template <class BuilderT>
static Value *mergeComponentToPackedValue(BuilderT *builder, Value *value, Value *component, Value *offset,
                                          unsigned valBitWidth, unsigned elemBitWidth) {
  if (!isa<IntegerType>(component->getType())) {
    component = builder->CreateBitCast(component, Type::getIntNTy(builder->getContext(), elemBitWidth));
  }
  component = builder->CreateZExtOrBitCast(component, Type::getIntNTy(builder->getContext(), valBitWidth));

  /* clear value bits: */
  uint64_t maskValue = (1ULL << elemBitWidth) - 1;
  Value *mask = builder->CreateShl(ConstantInt::get(value->getType(), maskValue), offset);
  mask = builder->CreateNot(mask);
  value = builder->CreateAnd(value, mask);

  /* shift component and merge with element: */
  component = builder->CreateShl(component, offset);
  return builder->CreateOr(value, component);
}

// Gets pointer to element to process in joint_matrix_apply loop for Accumulator
// 32x64 and 32x32 Also updates MatPtr to point to alloca of {<type x 64>, <type
// x 64>} used inside joint_matrix_apply loop
Value *JointMatrixFuncsResolutionPass::getAcc2x64ElementPtr(CallInst *CI, Value *matrix, Value *index,
                                                            IRBuilder<> *builder, Value **MatPtr,
                                                            const JointMatrixTypeDescription &desc) {
  if (LoadInst *loadInst = dyn_cast<LoadInst>(matrix)) {
    *MatPtr = Resolve(loadInst->getPointerOperand());
  } else {
    // Use existing alloca or create alloca in the entry node of the function
    *MatPtr = MatrixAllocas[matrix];
    if (!*MatPtr) {
      builder->SetInsertPoint(&*CI->getFunction()->getEntryBlock().getFirstInsertionPt());
      builder->SetCurrentDebugLocation(CI->getDebugLoc());
      *MatPtr = builder->CreateAlloca(matrix->getType(), ADDRESS_SPACE_PRIVATE);
      MatrixAllocas[matrix] = *MatPtr;
      builder->SetInsertPoint(CI);
    }
    builder->CreateStore(matrix, *MatPtr);
  }

  Type *returnType = getAccElemTypeFromDesc(builder->getContext(), desc);

  Value *AccElementPtr =
      builder->CreateBitCast(*MatPtr, returnType->getPointerTo((*MatPtr)->getType()->getPointerAddressSpace()));

  return builder->CreateGEP(returnType, AccElementPtr, index);
}

Value *JointMatrixFuncsResolutionPass::ResolveSliceInsert(CallInst *CI) {
  LLVM_DEBUG(dbgs() << "   -- RESOLVE SLICE INSERT: " << *CI << "\n");

  Value *matrix = Resolve(CI->getArgOperand(0));
  Value *component = CI->getArgOperand(1);
  Value *index = CI->getArgOperand(2);

  JointMatrixTypeDescription desc;
  Type *matTy = ResolveType(CI->getArgOperand(0)->getType(), &desc);
  IRBuilder builder(CI);
  Value *slice = nullptr;

  if (desc.bitWidth != desc.contribBitWidth) {
    // Unpacking:
    Value *offset = createOffsetForPackedValue(&builder, index, desc.contribBitWidth, desc.bitWidth);

    // prepare element to update
    Value *element = matrix; // If rows == 1, we do not need an extract, so directly use the value.
    if (dyn_cast<IGCLLVM::FixedVectorType>(matTy))
      element = updateIndexAndCreateSliceExtract(&builder, matrix, &index, desc.contribBitWidth, desc.bitWidth);
    if (!isa<IntegerType>(element->getType()))
      element = builder.CreateBitCast(element, Type::getIntNTy(builder.getContext(), desc.contribBitWidth));

    component = mergeComponentToPackedValue(&builder, element, component, offset, desc.contribBitWidth, desc.bitWidth);
  } else if (IntegerType *vectorElementType = dyn_cast<IntegerType>(getResolvedVectorElementType(matTy, &builder)))
    component = builder.CreateBitCast(component, vectorElementType);

  // Special case Accumulator 32x64 and 32x32 is represented as {<type x 64>, <type x 64>}.
  if (isAccumulator32x64(desc) || isAccumulator32x32(desc)) {
    Value *MatPtr = nullptr;
    Value *ptrToElem = getAcc2x64ElementPtr(CI, matrix, index, &builder, &MatPtr, desc);
    builder.CreateStore(component, ptrToElem);
    slice = builder.CreateLoad(matTy, MatPtr);
  } else if (dyn_cast<IGCLLVM::FixedVectorType>(matTy))
    slice = builder.CreateInsertElement(matrix, component, index);
  else
    slice = component;

  InstsToErase.insert(CI);
  return slice;
}

Value *JointMatrixFuncsResolutionPass::ResolveSliceExtract(CallInst *CI) {
  LLVM_DEBUG(dbgs() << "   -- RESOLVE SLICE EXTRACT: " << *CI << "\n");
  Value *matrix = Resolve(CI->getArgOperand(0));
  Value *index = CI->getArgOperand(1);

  JointMatrixTypeDescription desc;
  Type *matTy = ResolveType(CI->getArgOperand(0)->getType(), &desc);
  IRBuilder builder(CI);
  Value *element = matrix; // if it is a single value, we can directly use the value

  // If we are dealing with a vector, extract the element
  if (dyn_cast<IGCLLVM::FixedVectorType>(matTy)) {
    Value *indexVec = index;
    element = updateIndexAndCreateSliceExtract(&builder, matrix, &indexVec, desc.contribBitWidth, desc.bitWidth);
  } else if (isAccumulator32x64(desc) || isAccumulator32x32(desc)) {
    Value *MatPtr = nullptr;
    Value *ptrToElem = getAcc2x64ElementPtr(CI, matrix, index, &builder, &MatPtr, desc);
    element = builder.CreateLoad(getAccElemTypeFromDesc(builder.getContext(), desc), ptrToElem);
  }

  // unpack element we need from packed value
  if (desc.bitWidth != desc.contribBitWidth)
    element = unpackElementFromPackedValue(&builder, index, element, desc.contribBitWidth, desc.bitWidth);

  // We need the bitcast, e.g. for half, as the function call that is
  // being replaced has a half return type and the vectorElementType is i16
  element = builder.CreateBitCast(element, CI->getType());

  // Add metadata to mark this value as part of joint_matrix_apply loop
  // It will be used in getUnrollingPreferences to make sure this loop is fully unrolled
  Instruction *elementInst = cast<Instruction>(element);
  MDNode *node = MDNode::get(CI->getContext(), ConstantAsMetadata::get(builder.getInt1(true)));
  elementInst->setMetadata("joint_matrix_apply", node);

  InstsToErase.insert(CI);
  return element;
}

bool JointMatrixFuncsResolutionPass::preprocessAccessChain(Function *F) {
  // We could resolve __spirv_AccessChain call, but apparently it's easier
  // and more effecient to preprocess it and map on VectorInsertDynamic
  // and VectorExtractDynamic here, before resolving and substituting matrix
  // types
  SmallVector<Instruction *, 16> toErase;
  SmallVector<std::pair<Instruction *, Instruction *>, 8> replaces;
  for (auto It = F->use_begin(); It != F->use_end(); It++) {
    auto user = It->getUser();
    if (!isa<CallInst>(user))
      continue;
    auto *CI = cast<CallInst>(user);
    IRBuilder<> builder(CI);
    if (CI->getNumUses() == 0) {
      toErase.push_back(CI);
      continue;
    }

#if LLVM_VERSION_MAJOR < 16
    if (IGCLLVM::isPointerTy(CI->getArgOperand(0)->getType()))
      continue;
#endif

    LLVM_DEBUG(dbgs() << " - PREPROCESS ACCESS CHAIN: " << *CI << "\n");

    Type *chainBaseTy = nullptr;
    auto operand0 = CI->getArgOperand(0);

#if LLVM_VERSION_MAJOR >= 16
    if (IGCLLVM::isPointerTy(operand0->getType())) {
      chainBaseTy = TryFindTargetExtensionTypeOfOpaquePtr(operand0);

      if (!chainBaseTy) {
        m_Ctx->EmitError("__spirv_AccessChain call 1st argument must be pointer to target extension type", operand0);
        continue;
      }
    } else
#endif
    {
      // to be removed after we switch to LLVM 16 with opaque pointers by
      // default
      chainBaseTy = IGCLLVM::getNonOpaquePtrEltTy(operand0->getType());
      IGC_ASSERT_MESSAGE(chainBaseTy, "__spirv_AccessChain call 1st argument is invalid");
    }

    IGC_ASSERT_MESSAGE(isMatrixType(chainBaseTy), "__spirv_AccessChain call 1st argument must be cooperative matrix");
    Value *ptrToMatrix = CI->getArgOperand(0);
    Value *matrix = builder.CreateLoad(chainBaseTy, ptrToMatrix, "");
    Value *index = CI->getArgOperand(1);

    for (const auto &U : CI->users()) {
      Instruction *memInst = dyn_cast<Instruction>(U);
      if (!memInst)
        continue;
      // In case of sycl::half and sycl::bfloat16 storage will be accessed
      // via zero GEP or pointer cast that we need to strip, for example:
      // %call = call spir_func %structtype addrspace(4)* @__spirv_AccessChain(
      //     %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* addrspace(4)* %matrix,
      //     i64 %idx)
      // %gep = getelementptr inbounds %structtype, %structtype addrspace(4)* %call, i64 0, i32 0
      // %cast = bitcast i16 addrspace(4)* %gep to half addrspace(4)*
      // %extract = load half, half addrspace(4)* %cast
      while (isa<GetElementPtrInst>(memInst) || isa<BitCastInst>(memInst) || isa<AddrSpaceCastInst>(memInst)) {
        if (isa<GetElementPtrInst>(memInst))
          IGC_ASSERT_MESSAGE(cast<GetElementPtrInst>(memInst)->hasAllZeroIndices(),
                             "Unexpected matrix insert/extract format");
        toErase.push_back(memInst);
        memInst = cast<Instruction>(IGCLLVM::getUniqueUndroppableUser(memInst));
      }
      // Expected format is:
      // %matrix_ptr = alloca %matrix_type
      // (may be some casts)
      // %element_ptr = __spirv_AccessChain(%matrix_ptr, %index)
      // 1. For extract
      // %element = load %element_ptr
      // 2. For insert
      // store %element %element_ptr
      IGC_ASSERT_MESSAGE(isa<LoadInst>(memInst) || isa<StoreInst>(memInst), "Unexpected matrix insert/extract format");
      // Get __spirv_AccessChain function's mangling postfix to reuse it
      // for overloading of insert/extract
      constexpr unsigned ACNameLength = 23; // "_Z19__spirv_AccessChain"
      std::string funcPostfix = (F->getName().drop_front(ACNameLength)).str();
      builder.SetInsertPoint(memInst);
      if (isa<LoadInst>(memInst)) {
        std::vector<Value *> Args = {matrix, index};
        FunctionType *funcType = FunctionType::get(memInst->getType(), {matrix->getType(), index->getType()}, false);
        std::string funcName =
            std::string("_Z28") + std::string(SPIRVPrefix) + std::string(JointMatrixSliceExtract) + funcPostfix;
        Instruction *extractCall = builder.CreateCall(F->getParent()->getOrInsertFunction(funcName, funcType), Args);
        replaces.push_back(std::make_pair(memInst, extractCall));
      }
      if (isa<StoreInst>(memInst)) {
        Value *component = cast<StoreInst>(memInst)->getValueOperand();
        std::vector<Value *> Args = {matrix, component, index};
        FunctionType *funcType =
            FunctionType::get(chainBaseTy, {matrix->getType(), component->getType(), index->getType()}, false);
        std::string funcName =
            std::string("_Z27") + std::string(SPIRVPrefix) + std::string(JointMatrixSliceInsert) + funcPostfix;
        Instruction *extractCall = builder.CreateCall(F->getParent()->getOrInsertFunction(funcName, funcType), Args);
        builder.CreateStore(extractCall, ptrToMatrix);
      }
      toErase.push_back(memInst);
    }
    toErase.push_back(CI);
  }
  for (const auto &InstPair : replaces) {
    InstPair.first->replaceAllUsesWith(InstPair.second);
  }
  for (Instruction *I : reverse(toErase)) {
    I->dropAllReferences();
  }
  for (Instruction *I : toErase) {
    I->eraseFromParent();
  }

  return !replaces.empty() || !toErase.empty();
}

void JointMatrixFuncsResolutionPass::InsertPlaceholder(Value *v) {
  if (ResolvedValues.count(v) > 0) {
    return;
  }

  LLVM_DEBUG(dbgs() << "   -- INSERT PLACEHOLDER FOR: " << *v << "\n");
  Type *type = v->getType();

  if (type->isVoidTy())
    return;

  if (type->isPointerTy() || IGCLLVM::isTargetExtTy(type))
    type = ResolveTypes(type);

  Instruction *predecesor = nullptr;
  if (Instruction *inst = dyn_cast<Instruction>(v)) {
    predecesor = inst;
  }

  Instruction *placeholder = nullptr;
  if (!type->isStructTy()) {
    /* Using bit-casts as placeholder values. Undefs of each type are unique per
     * module and cannot be used as unique placeholders. */
    LLVM_DEBUG(dbgs() << "   -- CREATE UNDEF BITCAST TO: " << *type << "\n");
    placeholder = BitCastInst::Create(Instruction::BitCast, UndefValue::get(type), type, "tmp.value", predecesor);
  } else {
    /* Structure types cannot be bitcasted. Use insert element with two undefs
     * to create unique placeholder for structure value.*/
    Type *elemType = type->getContainedType(0)->getScalarType();
    LLVMContext &ctx = v->getContext();
    Type *memberType = getAccHalfVec64Type(elemType);
    Value *memberValue = UndefValue::get(memberType);
    Value *structValue = UndefValue::get(StructType::get(ctx, ArrayRef<Type *>({memberType, memberType})));
    placeholder = InsertValueInst::Create(structValue, memberValue, {0}, "tmp.value", predecesor);
  }
  ResolvedValues[v] = placeholder;
  PlaceholderInstructions[v] = placeholder;
  LLVM_DEBUG(dbgs() << "   -- PLACEHOLDER: " << *placeholder << "\n");
}

Value *JointMatrixFuncsResolutionPass::ResolveCall(CallInst *CI) {
  LLVM_DEBUG(dbgs() << "   -- RESOLVE CALL: " << *CI << "\n");
  Function *func = CI->getCalledFunction();
  IGC_ASSERT_MESSAGE(func, "Unexpected missing function.");
  if (!func)
    return nullptr;

  Value *NewValue = nullptr;
  StringRef funcName = func->getName();
  if (funcName.contains(JointMatrixPrefetchPrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolvePrefetch(CI);
  } else if (funcName.contains(JointMatrixLoadPrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolveLoad</*isJointMatrix*/ true, /*isChecked*/ false>(CI);
  } else if (funcName.contains(JointMatrixLoadCheckedPrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolveLoad</*isJointMatrix*/ true, /*isChecked*/ true>(CI);
  } else if (funcName.contains(CooperativeMatrixLoadPrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolveLoad</*isJointMatrix*/ false, /*isChecked*/ false>(CI);
  } else if (funcName.contains(JointMatrixStorePrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolveStore</*isJointMatrix*/ true, /*isChecked*/ false>(CI);
  } else if (funcName.contains(JointMatrixStoreCheckedPrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolveStore</*isJointMatrix*/ true, /*isChecked*/ true>(CI);
  } else if (funcName.contains(CooperativeMatrixStorePrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolveStore</*isJointMatrix*/ false, /*isChecked*/ false>(CI);
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
  } else if (funcName.contains(CooperativeMatrixMadPrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolveMad(CI, CooperativeOp);
  } else if (funcName.contains(JointMatrixFillPrefx) || funcName.contains(CooperativeMatrixConstantCompositePrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolveFill(CI);
  } else if (funcName.contains(JointMatrixFillCheckedPrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolveFillChecked(CI);
  } else if (funcName.contains(JointMatrixWorkItemLengthPrefx) || funcName.contains(CooperativeMatrixLengthPrefx)) {
    InsertPlaceholder(CI);
    NewValue = ResolveWILength(CI);
  } else if (funcName.contains(JointMatrixSliceInsert)) {
    InsertPlaceholder(CI);
    NewValue = ResolveSliceInsert(CI);
  } else if (funcName.contains(JointMatrixSliceExtract)) {
    InsertPlaceholder(CI);
    NewValue = ResolveSliceExtract(CI);
  } else if (funcName.contains(JointMatrixGetCoordPrefx) || funcName.contains(CooperativeMatrixGetElementCoordPrefx)) {
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
  LLVM_DEBUG(dbgs() << " - RESOLVED: " << *newValue << "\n");
}

void JointMatrixFuncsResolutionPass::CacheResolvedTypes(Type *oldType, Type *newType) {
  IGC_ASSERT_MESSAGE(newType, "Type should not be null.");
  if (newType == nullptr)
    return;
  ResolvedTypes[oldType] = newType;
}

Value *JointMatrixFuncsResolutionPass::ResolveGeneric(Instruction *OldInst) {
  LLVM_DEBUG(dbgs() << "   -- RESOLVE GENERIC: " << *OldInst << "\n");
  InsertPlaceholder(OldInst);
  Instruction *NewInst = OldInst->clone();

  for (unsigned i = 0; i < NewInst->getNumOperands(); i++) {
    Value *oldOp = NewInst->getOperand(i);
    if (!isOrContainsMatrixType(oldOp->getType()))
      continue;

    NewInst->setOperand(i, Resolve(oldOp));
  }

  if (GetElementPtrInst *NewGEPI = dyn_cast<GetElementPtrInst>(NewInst)) {
    GetElementPtrInst *OldGEPI = cast<GetElementPtrInst>(OldInst);
    NewGEPI->setSourceElementType(ResolveTypes(OldGEPI->getSourceElementType()));
    NewGEPI->setResultElementType(ResolveTypes(OldGEPI->getResultElementType()));
  } else if (AllocaInst *NewAlloca = dyn_cast<AllocaInst>(NewInst)) {
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

Type *JointMatrixFuncsResolutionPass::ResolveTypes(Type *t) {
  if (ResolvedTypes.count(t) > 0)
    return ResolvedTypes[t];

  if (StructType *ST = dyn_cast<StructType>(t))
    return ResolveStructType(ST);

  if (ArrayType *AT = dyn_cast<ArrayType>(t))
    return ResolveArrayType(AT);

  if (PointerType *PT = dyn_cast<PointerType>(t)) {
    if (isMatrixType(t))
      return ResolveType(t, nullptr);
    return ResolvePointerType(PT);
  }

#if LLVM_VERSION_MAJOR >= 16
  if (isa<TargetExtType>(t)) {
    if (isMatrixType(t))
      return ResolveType(t, nullptr);
  }
#endif

  return t;
}

Type *JointMatrixFuncsResolutionPass::ResolveStructType(Type *oldType) {
  if (ResolvedTypes.count(oldType) > 0)
    return ResolvedTypes[oldType];

  StructType *structType = cast<StructType>(oldType);

  // Check if any element in the structure is a matrix type and needs to be resolved
  bool membersNeedResolve = std::any_of(structType->elements().begin(), structType->elements().end(),
                                        [](Type *t) { return isOrContainsMatrixType(t); });

  if (!membersNeedResolve)
    return oldType;

  StructType *newType = nullptr;
  if (!structType->isLiteral()) {
    SmallString<28> name;
    newType = StructType::create(oldType->getContext(), (structType->getName() + ".resolved").toStringRef(name));
  } else {
    newType = StructType::create(oldType->getContext());
  }

  // caching now to avoid recursion, in case struct contains itself as an element
  CacheResolvedTypes(oldType, newType);

  SmallVector<Type *, 1> elements;
  llvm::transform(structType->elements(), std::back_inserter(elements), [&](Type *t) {
    if (isOrContainsMatrixType(t))
      return ResolveTypes(t);
    return t;
  });
  newType->setBody(elements, structType->isPacked());

  return newType;
}

Type *JointMatrixFuncsResolutionPass::ResolveArrayType(Type *oldType) {
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

Type *JointMatrixFuncsResolutionPass::ResolvePointerType(Type *oldType) {
  if (ResolvedTypes.count(oldType) > 0)
    return ResolvedTypes[oldType];

  PointerType *ptrType = dyn_cast<PointerType>(oldType);
  if (IGCLLVM::isPointerTy(ptrType))
    return oldType;

  Type *elemType = IGCLLVM::getNonOpaquePtrEltTy(ptrType);
  if (!isOrContainsMatrixType(elemType))
    return oldType;

  Type *newType = PointerType::get(ResolveTypes(elemType), ptrType->getAddressSpace());
  CacheResolvedTypes(oldType, newType);
  return newType;
}

Value *JointMatrixFuncsResolutionPass::Resolve(Value *v) {
  LLVM_DEBUG(dbgs() << "   -- RESOLVE: " << *v << "\n");

  if (ResolvedValues.count(v) > 0) {
    LLVM_DEBUG(dbgs() << "   -- FOUND IN CACHE: " << *ResolvedValues[v] << "\n");
    return ResolvedValues[v];
  }

  if (CallInst *CI = dyn_cast<CallInst>(v))
    return ResolveCall(CI);

  if (PHINode *PN = dyn_cast<PHINode>(v)) {
    unsigned IncomingCount = PN->getNumIncomingValues();

    Type *type = ResolveTypes(v->getType());
    LLVM_DEBUG(dbgs() << "   -- RESOLVED PHI TYPE: " << *type << "\n");
    PHINode *NewPN = PHINode::Create(type, IncomingCount, PN->getName(), PN);
    NewPN->setDebugLoc(PN->getDebugLoc());
    CacheResolvedValue(v, NewPN);

    for (unsigned i = 0; i < IncomingCount; i++) {
      Value *operand = Resolve(PN->getIncomingValue(i));
      LLVM_DEBUG(dbgs() << "   -- RESOLVED PHI INCOMING BLOCK " << i << ": " << *operand << "\n");
      NewPN->addIncoming(operand, PN->getIncomingBlock(i));
    }

    InstsToErase.insert(PN);
    LLVM_DEBUG(dbgs() << " - RESOLVED: " << *NewPN << "\n");
    return NewPN;
  }

  if (Instruction *I = dyn_cast<Instruction>(v))
    return ResolveGeneric(I);

  if (isa<UndefValue>(v)) {
    Value *resolved = UndefValue::get(ResolveType(v->getType(), nullptr));
    LLVM_DEBUG(dbgs() << " - RESOLVED: " << *resolved << "\n");
    return resolved;
  }

  IGC_ASSERT_MESSAGE(false, "Resolve failure.");
  return nullptr;
}

Function *JointMatrixFuncsResolutionPass::CloneFunction(Function *pOriginalFunction) {
  if (!pOriginalFunction)
    return nullptr;

  std::vector<Type *> params;
  for (auto &arg : pOriginalFunction->args()) {
    auto type = isOrContainsMatrixType(arg.getType()) ? ResolveTypes(arg.getType()) : arg.getType();
    params.push_back(type);
  }

  auto newFunctionTy =
      FunctionType::get(ResolveTypes(pOriginalFunction->getReturnType()), params, pOriginalFunction->isVarArg());

  Function *pNewFunction =
      Function::Create(newFunctionTy, pOriginalFunction->getLinkage(), pOriginalFunction->getAddressSpace(),
                       pOriginalFunction->getName() + "_resolved", pOriginalFunction->getParent());

  pNewFunction->setCallingConv(pOriginalFunction->getCallingConv());
  pNewFunction->setSubprogram(pOriginalFunction->getSubprogram());
  pNewFunction->copyAttributesFrom(pOriginalFunction);

  ValueToValueMapTy VMap;
  auto originalFunctionArgIt = pOriginalFunction->arg_begin();
  auto newFunctionArgIt = pNewFunction->arg_begin();
  std::unordered_set<Argument *> needsResolve;

  while (originalFunctionArgIt != pOriginalFunction->arg_end()) {
    newFunctionArgIt->setName(originalFunctionArgIt->getName());
    if (newFunctionArgIt->getType() != originalFunctionArgIt->getType())
      needsResolve.insert(&*newFunctionArgIt);
    VMap[&(*originalFunctionArgIt++)] = newFunctionArgIt++;
  }

  SmallVector<ReturnInst *, 8> Returns;
  IGCLLVM::CloneFunctionInto(pNewFunction, pOriginalFunction, VMap, IGCLLVM::CloneFunctionChangeType::LocalChangesOnly,
                             Returns);

  for (auto *arg : needsResolve) {
    for (auto *U : arg->users()) {
      if (Instruction *I = dyn_cast<Instruction>(U)) {
        for (unsigned i = 0; i < I->getNumOperands(); ++i) {
          Value *oldOp = I->getOperand(i);
          if (!isOrContainsMatrixType(oldOp->getType()))
            continue;
          I->setOperand(i, Resolve(oldOp));
        }
      }
    }
  }

  runOnFunction(*pNewFunction);
  return pNewFunction;
}

void JointMatrixFuncsResolutionPass::visitCallInst(CallInst &CI) {
  LLVM_DEBUG(dbgs() << " - VISIT: " << CI << "\n");
  Function *func = CI.getCalledFunction();
  if (!func)
    return;

  /* Check if already resolved: */
  if (ResolvedValues.count(&CI) > 0)
    return;

  StringRef funcName = func->getName();

#if LLVM_VERSION_MAJOR < 16
  if (IGCLLVM::isPointerTy(CI.getType()) || isAnyOperand(CI, IGCLLVM::isPointerTy))
    return;
#endif

  /* Resolve calls to JointMatrix BIs that haven't been resolved yet. In
   * future when returning and passing matrices by argument is
   * supported also basic block terminators should be used as
   * transformation starting point */
  if (funcName.startswith(JointMatrixBIPrefix) || funcName.contains(JointMatrixBISuffix) ||
      funcName.contains(CooperativeMatrixBISuffix)) {
    ResolveSIMDSize(CI.getParent()->getParent());
    ResolveCall(&CI);
    return;
  }

  if (funcName.startswith("_Z") &&
      (funcName.contains("__spirv_JointMatrix") || funcName.contains("__spirv_CooperativeMatrix") ||
       funcName.contains(JointMatrixFillPrefx))) {
    ResolveSIMDSize(CI.getParent()->getParent());
    ResolveCall(&CI);
    return;
  }

  if (isAnyFunctionArgMatrixType(func))
    ResolveCallFuncWithMatrixArgs(ResolvedFuncSignatures[func], &CI);
}

bool JointMatrixFuncsResolutionPass::ResolveCallFuncWithMatrixArgs(Function *ResolvedFunction, CallInst *CI) {
  if (!CI || !ResolvedFunction)
    return false;

  LLVM_DEBUG(dbgs() << "   -- RESOLVE CALL: " << *CI << "\n");
  InsertPlaceholder(CI);
  std::vector<Value *> params;
  for (auto &callArg : CI->args()) {
    auto callArgInst = callArg.get();
    if (isOrContainsMatrixType(callArgInst->getType()))
      params.push_back(Resolve(callArgInst));
    else
      params.push_back(callArgInst);
  }

  IRBuilder<> b(CI);
  auto newCall = b.CreateCall(ResolvedFunction, params);
  newCall->setDebugLoc(CI->getDebugLoc());
  newCall->setCallingConv(CI->getCallingConv());
  newCall->setAttributes(CI->getAttributes());

  if (CI->hasName()) {
    newCall->setName(CI->getName());
  }

  CacheResolvedValue(CI, newCall);
  InstsToErase.insert(CI);
  return true;
}

bool JointMatrixFuncsResolutionPass::ResolveFunction(Function *OriginalFunction) {
  ResolveSIMDSize(OriginalFunction);
  Function *newFunction = CloneFunction(OriginalFunction);

  if (FunctionEntryMap.count(OriginalFunction) > 0 && FunctionEntryMap[OriginalFunction] != nullptr) {
    FunctionEntryMap[newFunction] = FunctionEntryMap[OriginalFunction];
  }

  ResolvedFuncSignatures[OriginalFunction] = newFunction;
  NewFuncWithResolvedSignatures.insert(newFunction);
  LLVM_DEBUG(dbgs() << " - RESOLVED FUNC:\n" << *newFunction);
  return true;
}

std::string getTypeName(Type *T) {
  std::string TypeName;
  raw_string_ostream TypeStream(TypeName);
  if (T)
    T->print(TypeStream);
  else
    TypeStream << "Printing <null> Type";
  TypeStream.flush();
  return TypeName;
}

DIType *getOrCreateType(Type *T, Module *M) {
  DIType *diType = nullptr;
  DIBuilder Builder(*M, true);
  DataLayout Layout(M);

  if (T->isPointerTy()) {

    uint align = 0;
    align = IGCLLVM::getPrefTypeAlign(Layout, T).value();

    std::optional<unsigned int> opt(std::nullopt);
    diType = Builder.createPointerType(nullptr, Layout.getPointerTypeSizeInBits(T), align * CHAR_BIT,
                                       /*DWARFAddressSpace=*/IGCLLVM::makeLLVMOptional(opt), getTypeName(T));
  } else {
    int encoding = dwarf::DW_ATE_signed;
    if (T->isIntegerTy())
      encoding = dwarf::DW_ATE_unsigned;
    else if (T->isFloatingPointTy())
      encoding = dwarf::DW_ATE_float;

    diType = Builder.createBasicType(getTypeName(T), T->getPrimitiveSizeInBits(), encoding);
  }

  return diType;
}

void JointMatrixFuncsResolutionPass::RecursiveSearchAndFixCanonicalizdGEPandLifetime(
    std::unordered_set<llvm::Value *> &visited, const DataLayout &DL, Value *root, uint64_t matrixTypeAllocSize,
    uint64_t totalAllocationSize) {
  auto insertToVisited = visited.insert(root);
  if (!insertToVisited.second) // root was already visited
    return;

  // Depth first recursive traversal of root users
  for (auto U : root->users()) {
    // Only traverse children nodes if current node is a cast or a PHI node.
    if (isa<CastInst>(U) || isa<PHINode>(U)) {
      LLVM_DEBUG(dbgs() << "DFS: visiting users of " << *U << '\n');
      RecursiveSearchAndFixCanonicalizdGEPandLifetime(visited, DL, U, matrixTypeAllocSize, totalAllocationSize);
      continue;
    }

    if (auto GEP = dyn_cast<GetElementPtrInst>(U)) {
      // Update canonicalized i8 GEP:
      //   getelementptr i8, ptr %x, i64 <const>
      if (GEP->getSourceElementType()->isIntegerTy(8) && GEP->hasAllConstantIndices() && GEP->getNumIndices() == 1) {
        LLVM_DEBUG(dbgs() << "Found canonicalized i8 GEP: " << *GEP << "\n");
        auto offset = cast<ConstantInt>(GEP->getOperand(1));
        uint64_t pointerSize = DL.getPointerSizeInBits(GEP->getPointerAddressSpace()) / 8;
        uint64_t offsetInElements = offset->getZExtValue() / pointerSize;
        uint64_t correctOffset = offsetInElements * matrixTypeAllocSize;
        ConstantInt *newOffsetConstant = ConstantInt::get(offset->getType(), correctOffset);
        GEP->setOperand(1, newOffsetConstant);
        LLVM_DEBUG(dbgs().indent(2) << "Fixed index: " << *GEP << "\n");
      }
    } else if (auto II = dyn_cast<IntrinsicInst>(U)) {
      // Update size for lifetime intrinsics
      if (II->getIntrinsicID() == Intrinsic::lifetime_start || II->getIntrinsicID() == Intrinsic::lifetime_end) {
        if (auto constInt = dyn_cast<ConstantInt>(II->getOperand(0))) {
          LLVM_DEBUG(dbgs() << "Found lifetime intrinsic for joint matrix array allocation" << *II << '\n');
          II->setOperand(0, ConstantInt::get(constInt->getType(), totalAllocationSize));
          LLVM_DEBUG(dbgs().indent(2) << "Fixed size: " << *II << "\n");
        }
      }
    } else {
      LLVM_DEBUG(dbgs() << "Skipping joint matrix array alloca user: " << *U << '\n');
    }
  }
}

void JointMatrixFuncsResolutionPass::visitAllocaInst(AllocaInst &I) {
  LLVM_DEBUG(dbgs() << " - VISIT: " << I << "\n");

  if (ResolvedValues.count(&I) > 0)
    return;

  if (!isOrContainsMatrixType(I.getAllocatedType()))
    return;

  ResolveSIMDSize(I.getParent()->getParent());

  AllocaInst *newInst = cast<AllocaInst>(ResolveGeneric(&I));

  // update debug info
  {
    TinyPtrVector<DbgDeclareInst *> DDIs = FindDbgDeclareUses(&I);

    for (DbgDeclareInst *ddi : DDIs) {
      auto loc = ddi->getDebugLoc();
      auto var = ddi->getVariable();
      auto file = var->getFile();
      auto lineNo = var->getLine();
      auto scope = var->getScope();

      auto type = getOrCreateType(newInst->getType(), I.getModule());

      DIBuilder builder(*(I.getModule()));
      auto created = builder.createAutoVariable(scope, var->getName(), file, lineNo, type);
      builder.insertDbgValueIntrinsic(newInst, created, builder.createExpression(), loc, ddi);
      ddi->eraseFromParent();
    }
  }

  // update GEPs and lifetime intrinsics
  {
    Type *unresolvedMatrixType = getContainedMatrixType(I.getAllocatedType());
    Type *resolvedMatrixType = ResolveTypes(unresolvedMatrixType);

    const DataLayout &DL = newInst->getModule()->getDataLayout();
    uint64_t matrixTypeSize = DL.getTypeAllocSize(resolvedMatrixType);
    uint64_t totalAllocSize =
        IGCLLVM::makeOptional(newInst->getAllocationSizeInBits(DL)).value_or(TypeSize(0, false)) / 8;
    // We have to use old alloca instruction I because its uses weren't replaced by newInst yet.
    std::unordered_set<Value *> visited;
    RecursiveSearchAndFixCanonicalizdGEPandLifetime(visited, DL, &I, matrixTypeSize, totalAllocSize);
  }
}

void JointMatrixFuncsResolutionPass::visitAddrSpaceCastInst(AddrSpaceCastInst &I) {
  LLVM_DEBUG(dbgs() << " - VISIT: " << I << "\n");
  if (ResolvedValues.count(&I) > 0)
    return;

  if (!isOrContainsMatrixType(I.getType()))
    return;

  ResolveSIMDSize(I.getParent()->getParent());
  ResolveGeneric(&I);
}

void JointMatrixFuncsResolutionPass::visitLoadInst(LoadInst &I) {
  LLVM_DEBUG(dbgs() << " - VISIT: " << I << "\n");
  if (ResolvedValues.count(&I) > 0)
    return;

  if (!isOrContainsMatrixType(I.getType()))
    return;

  ResolveSIMDSize(I.getParent()->getParent());
  ResolveGeneric(&I);
}

void JointMatrixFuncsResolutionPass::visitPHINode(PHINode &I) {
  LLVM_DEBUG(dbgs() << " - VISIT: " << I << "\n");
  if (ResolvedValues.count(&I) > 0)
    return;

  if (!isOrContainsMatrixType(I.getType()))
    return;

  ResolveSIMDSize(I.getParent()->getParent());
  ResolveGeneric(&I);
}

void JointMatrixFuncsResolutionPass::visitReturnInst(ReturnInst &I) {
  LLVM_DEBUG(dbgs() << " - VISIT: " << I << "\n");
  if (ResolvedValues.count(&I) > 0)
    return;

  if (I.getReturnValue() == nullptr || !isOrContainsMatrixType(I.getReturnValue()->getType()))
    return;

  ResolveSIMDSize(I.getParent()->getParent());
  ResolveGeneric(&I);
}

void JointMatrixFuncsResolutionPass::visitGetElementPtrInst(GetElementPtrInst &GEP) {
  LLVM_DEBUG(dbgs() << " - VISIT: " << GEP << "\n");
  if (ResolvedValues.count(&GEP) > 0)
    return;

  Type *GEPEltType = GEP.getSourceElementType();
  if (!isOrContainsMatrixType(GEPEltType))
    return;

  ResolveSIMDSize(GEP.getParent()->getParent());
  ResolveGeneric(&GEP);
}

void JointMatrixFuncsResolutionPass::visitStoreInst(StoreInst &I) {
  LLVM_DEBUG(dbgs() << " - VISIT: " << I << "\n");
  if (ResolvedValues.count(&I) > 0)
    return;

  if (isAnyOperand(I, isOrContainsMatrixType)) {
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
  PtrToIntInst *PTI = dyn_cast<PtrToIntInst>(I.getValueOperand());
  BitCastInst *BC = dyn_cast<BitCastInst>(I.getPointerOperand());

  if (PTI == nullptr || !isMatrixType(PTI->getPointerOperand()->getType()) || !PTI->getDestTy()->isIntegerTy(64) ||
      BC == nullptr)
    return;

  ResolveSIMDSize(I.getParent()->getParent());
  InsertPlaceholder(&I);
  Value *PTIOperand = PTI->getOperand(0);
  Type *newBCElementType = ResolveTypes(dyn_cast<PointerType>(PTIOperand->getType()));
  PointerType *BCDstType = dyn_cast<PointerType>(BC->getDestTy());

  BitCastInst *newBC =
      new BitCastInst(Resolve(BC->getOperand(0)),
                      PointerType::get(newBCElementType, BCDstType->getPointerAddressSpace()), BC->getName(), BC);

  StoreInst *newSI = new StoreInst(Resolve(PTIOperand), newBC, I.isVolatile(), IGCLLVM::getAlign(I), &I);

  newSI->setDebugLoc(I.getDebugLoc());
  CacheResolvedValue(&I, newSI);
  InstsToErase.insert(&I);

  // Remove incorrect PtrToInt and BitCast instructions
  InstsToErase.insert(PTI);
  InstsToErase.insert(BC);
}

void JointMatrixFuncsResolutionPass::visitBitCastInst(BitCastInst &I) {
  LLVM_DEBUG(dbgs() << " - VISIT: " << I << "\n");
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
  if (srcPtr && dstPtr && !IGCLLVM::isPointerTy(srcPtr) && !IGCLLVM::isPointerTy(dstPtr)) {
    Type *srcPtrType = IGCLLVM::getNonOpaquePtrEltTy(srcPtr);
    Type *dstPtrType = IGCLLVM::getNonOpaquePtrEltTy(dstPtr);

    StructType *srcStructType = dyn_cast<StructType>(srcPtrType);
    if (srcStructType != nullptr && srcStructType->getNumElements() == 1) {
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

void JointMatrixFuncsResolutionPass::visitPtrToIntInst(PtrToIntInst &I) {
  LLVM_DEBUG(dbgs() << " - VISIT: " << I << "\n");
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
  if (isMatrixType(I.getPointerOperand()->getType()) && I.getDestTy()->isIntegerTy(64))
    return;

  if (ResolvedValues.count(&I) > 0)
    return;

  if (!isOrContainsMatrixType(I.getSrcTy()))
    return;

  ResolveSIMDSize(I.getParent()->getParent());
  ResolveGeneric(&I);
}
