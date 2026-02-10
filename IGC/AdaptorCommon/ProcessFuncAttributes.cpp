/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ProcessFuncAttributes.h"
#include "Compiler/CISACodeGen/EstimateFunctionSize.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/Optimizer/OpenCLPasses/StackOverflowDetection/StackOverflowDetection.hpp"
#include "common/BuiltinTypes.h"

#include "common/LLVMWarningsPush.hpp"

#include "llvm/IR/Attributes.h"

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Attributes.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Regex.h>
#include <llvm/Analysis/CallGraph.h>
#include "llvm/ADT/SCCIterator.h"
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/Function.h>
#include <llvmWrapper/IR/Type.h>
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "common/igc_regkeys.hpp"
#include <fstream>
#include <string>
#include <set>
#include <algorithm>
#include <BiFModule/Headers/bif_control_common.h>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

static const std::unordered_set<std::string> OCLExtOpBuiltinNames = {"acos",
                                                                     "acosh",
                                                                     "acospi",
                                                                     "asin",
                                                                     "asinh",
                                                                     "asinpi",
                                                                     "atan",
                                                                     "atan2",
                                                                     "atanh",
                                                                     "atanpi",
                                                                     "atan2pi",
                                                                     "cbrt",
                                                                     "ceil",
                                                                     "copysign",
                                                                     "cos",
                                                                     "cosh",
                                                                     "cospi",
                                                                     "erfc",
                                                                     "erf",
                                                                     "exp",
                                                                     "exp2",
                                                                     "exp10",
                                                                     "expm1",
                                                                     "fabs",
                                                                     "fdim",
                                                                     "floor",
                                                                     "fma",
                                                                     "fmax",
                                                                     "fmin",
                                                                     "fmod",
                                                                     "fract",
                                                                     "frexp",
                                                                     "hypot",
                                                                     "ilogb",
                                                                     "ldexp",
                                                                     "lgamma",
                                                                     "lgamma_r",
                                                                     "log",
                                                                     "log2",
                                                                     "log10",
                                                                     "log1p",
                                                                     "logb",
                                                                     "mad",
                                                                     "maxmag",
                                                                     "minmag",
                                                                     "modf",
                                                                     "nan",
                                                                     "nextafter",
                                                                     "pow",
                                                                     "pown",
                                                                     "powr",
                                                                     "remainder",
                                                                     "remquo",
                                                                     "rint",
                                                                     "rootn",
                                                                     "round",
                                                                     "rsqrt",
                                                                     "sin",
                                                                     "sincos",
                                                                     "sinh",
                                                                     "sinpi",
                                                                     "sqrt",
                                                                     "tan",
                                                                     "tanh",
                                                                     "tanpi",
                                                                     "tgamma",
                                                                     "trunc",
                                                                     "half_cos",
                                                                     "half_divide",
                                                                     "half_exp",
                                                                     "half_exp2",
                                                                     "half_exp10",
                                                                     "half_log",
                                                                     "half_log2",
                                                                     "half_log10",
                                                                     "half_powr",
                                                                     "half_recip",
                                                                     "half_rsqrt",
                                                                     "half_sin",
                                                                     "half_sqrt",
                                                                     "half_tan",
                                                                     "native_cos",
                                                                     "native_divide",
                                                                     "native_exp",
                                                                     "native_exp2",
                                                                     "native_exp10",
                                                                     "native_log",
                                                                     "native_log2",
                                                                     "native_log10",
                                                                     "native_powr",
                                                                     "native_recip",
                                                                     "native_rsqrt",
                                                                     "native_sin",
                                                                     "native_sqrt",
                                                                     "native_tan",
                                                                     "fclamp",
                                                                     "degrees",
                                                                     "mix",
                                                                     "fmax_common",
                                                                     "fmin_common",
                                                                     "radians",
                                                                     "step",
                                                                     "smoothstep",
                                                                     "sign",
                                                                     "cross",
                                                                     "distance",
                                                                     "length",
                                                                     "normalize",
                                                                     "fast_distance",
                                                                     "fast_length",
                                                                     "fast_normalize",
                                                                     "s_abs",
                                                                     "s_abs_diff",
                                                                     "s_add_sat",
                                                                     "u_add_sat",
                                                                     "s_hadd",
                                                                     "u_hadd",
                                                                     "s_rhadd",
                                                                     "u_rhadd",
                                                                     "s_clamp",
                                                                     "u_clamp",
                                                                     "clz",
                                                                     "ctz",
                                                                     "s_mad_hi",
                                                                     "s_mad_sat",
                                                                     "u_mad_sat",
                                                                     "s_max",
                                                                     "s_min",
                                                                     "u_max",
                                                                     "u_min",
                                                                     "s_mul_hi",
                                                                     "rotate",
                                                                     "s_sub_sat",
                                                                     "u_sub_sat",
                                                                     "u_upsample",
                                                                     "s_upsample",
                                                                     "popcount",
                                                                     "s_mad24",
                                                                     "u_mad24",
                                                                     "s_mul24",
                                                                     "u_mul24",
                                                                     "vloadn",
                                                                     "vstoren",
                                                                     "vload_half",
                                                                     "vload_halfn",
                                                                     "vstore_half",
                                                                     "vstore_half_r",
                                                                     "vstore_halfn",
                                                                     "vstore_halfn_r",
                                                                     "vloada_halfn",
                                                                     "vstorea_halfn",
                                                                     "vstorea_halfn_r",
                                                                     "shuffle",
                                                                     "shuffle2",
                                                                     "printf",
                                                                     "prefetch",
                                                                     "bitselect",
                                                                     "select",
                                                                     "u_abs",
                                                                     "u_abs_diff",
                                                                     "u_mul_hi",
                                                                     "u_mad_hi"};

class ProcessFuncAttributes : public ModulePass {
public:
  static char ID;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<EstimateFunctionSize>();
    AU.addRequired<llvm::CallGraphWrapperPass>();
  }

  ProcessFuncAttributes();

  ~ProcessFuncAttributes() {}

  virtual bool runOnModule(Module &M);

  virtual llvm::StringRef getPassName() const { return "ProcessFuncAttributes"; }

private:
  bool isGASPointer(Value *arg);
};

} // namespace

// Register pass to igc-opt
#define PASS_FLAG "igc-process-func-attributes"
#define PASS_DESCRIPTION "Set Functions' linkage and attributes"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ProcessFuncAttributes, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(EstimateFunctionSize)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_END(ProcessFuncAttributes, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ProcessFuncAttributes::ID = 0;

ProcessFuncAttributes::ProcessFuncAttributes() : ModulePass(ID) {
  initializeProcessFuncAttributesPass(*PassRegistry::getPassRegistry());
}

inline bool ProcessFuncAttributes::isGASPointer(Value *V) {
  if (PointerType *PTy = dyn_cast<PointerType>(V->getType())) {
    return PTy->getAddressSpace() == ADDRESS_SPACE_GENERIC;
  }
  return false;
}

ModulePass *createProcessFuncAttributesPass() { return new ProcessFuncAttributes(); }

extern bool isSupportedAggregateArgument(Argument *arg);

static bool deduceIfUsesImageParams(Function *F, ModuleMetaData *ModMD) {
  auto It = ModMD->FuncMD.find(F);
  if (It != ModMD->FuncMD.end()) {
    const FunctionMetaData &FuncMD = It->second;
    IGC_ASSERT_MESSAGE(FuncMD.m_OpenCLArgBaseTypes.size() == F->arg_size(),
                       "m_OpenCLArgBaseTypes and function argument size mismatch!");

    for (const auto &BaseTy : FuncMD.m_OpenCLArgBaseTypes) {
      if (BaseTy.find("image") != std::string::npos)
        return true;
    }
  }

  MDNode *Hints = F->getMetadata("non_kernel_arg_type_hints");
  if (Hints) {
    for (auto &Operand : Hints->operands())
      if (MDString *MDStr = dyn_cast<MDString>(Operand))
        if (MDStr->getString().contains_insensitive("image"))
          return true;
  }

  // SYCL/DPC++ generates image accessor helpers that take image parameters indirectly (through a class referenced
  // using a pointer). With typed pointers enabled, the pointer type is inspected and the helpers are inlined. This
  // is not possible with opaque pointers and in this case the parameter types must be deduced based on the mangled
  // function name.
  auto IsSyclImageAccessor = [](StringRef Name) {
    return Name.startswith("_Z") && Name.contains("sycl") && Name.contains("accessor") && Name.contains("image");
  };

  if (IsSyclImageAccessor(F->getName()))
    return true;

  for (auto &BB : *F) {
    for (auto &I : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&I)) {
        if (Function *Callee = CI->getCalledFunction()) {
          if (IsSyclImageAccessor(Callee->getName()))
            return true;
        }
      }
    }
  }

  return false;
}

// Only pointer, struct and array types are considered. E.g. vector type
// cannot contain opaque subtypes, function type may contain but ignored.
static void getBuiltinType(Type *T, SmallPtrSetImpl<Type *> &BuiltinTypes) {
  if (StructType *ST = dyn_cast<llvm::StructType>(T)) {
    // Check if this has been checked, to avoid spinning on %T = { %T *}.
    if (!BuiltinTypes.count(ST)) {
      BuiltinTypes.insert(ST);
      for (auto I = ST->element_begin(), E = ST->element_end(); I != E; ++I) {
        getBuiltinType(*I, BuiltinTypes);
      }
    }
  } else if (T->isPointerTy() && !IGCLLVM::isPointerTy(T)) {
    return getBuiltinType(IGCLLVM::getNonOpaquePtrEltTy(cast<PointerType>(T)), BuiltinTypes);
  }
#if LLVM_VERSION_MAJOR >= 16
  else if (isa<TargetExtType>(T)) {
    BuiltinTypes.insert(T);
  }
#endif
  else if (auto AT = dyn_cast<ArrayType>(T)) {
    return getBuiltinType(AT->getElementType(), BuiltinTypes);
  }
}

// Check the existence of an image type.
static bool containsImageType(llvm::Type *T) {
  // Get the builtin type of T. This can be either TargetExtTy (LLVM 16+) or
  // "pointer to opaque struct" (can be nested) representing a builtin type.
  SmallPtrSet<Type *, 8> BuiltinTypes;
  getBuiltinType(T, BuiltinTypes);

  return llvm::any_of(BuiltinTypes, [](Type *Ty) { return isImageBuiltinType(Ty); });
}

static bool isOptNoneBuiltin(StringRef name) {
  return name == "__intel_typedmemfence_optnone" || name == "__intel_memfence_optnone";
}

// Convert functions with recursion to stackcall, since subroutines do not support recursion
static bool convertRecursionToStackCall(CallGraph &CG) {
  bool hasRecursion = false;
  // Use Tarjan's algorithm to detect recursions.
  for (auto I = scc_begin(&CG), E = scc_end(&CG); I != E; ++I) {
    const std::vector<CallGraphNode *> &SCCNodes = *I;
    if (SCCNodes.size() >= 2) {
      hasRecursion = true;
      // Convert all functions in the SCC to stackcall
      for (auto Node : SCCNodes) {
        Node->getFunction()->addFnAttr("visaStackCall");
        Node->getFunction()->addFnAttr("hasRecursion");
      }
    } else {
      // Check self-recursion.
      auto Node = SCCNodes.back();
      for (const auto &Callee : *Node) {
        if (Callee.second == Node) {
          hasRecursion = true;
          Node->getFunction()->addFnAttr("visaStackCall");
          Node->getFunction()->addFnAttr("hasRecursion");
          break;
        }
      }
    }
  }
  return hasRecursion;
}

// __builtin_spirv related OpGroup call implementations contain both
// workgroup and subgroup code in them that is switched on based on the
// 'Execution' and 'Operation' parameters and these will almost always
// be compile time literals.  Let's inline these functions so we have a chance
// at optimizing away the branches that contain workgroup code that will cause
// SLM allocations when we're really doing a subgroup calls.
static DenseSet<Function *> collectMemPoolUsage(const Module &M) {
  const char *BUILTIN_MEMPOOL = "__builtin_IB_AllocLocalMemPool";
  auto *MemPool = M.getFunction(BUILTIN_MEMPOOL);

  DenseSet<Function *> FuncsToInline;

  if (!MemPool)
    return FuncsToInline;

  for (auto *U : MemPool->users()) {
    if (auto *CI = dyn_cast<CallInst>(U)) {
      FuncsToInline.insert(CI->getFunction());
    }
  }

  return FuncsToInline;
}

static void addFnAttrRecursive(Function *F, StringRef Attr, StringRef Val) {
  F->addFnAttr(Attr, Val);
  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    if (CallInst *CI = dyn_cast<CallInst>(&*i)) {
      Function *Callee = CI->getCalledFunction();
      if (Callee != nullptr) {
        addFnAttrRecursive(Callee, Attr, Val);
      }
    }
  }
}

static void setAlwaysInline(Function *F) {
  F->addFnAttr(llvm::Attribute::AlwaysInline);
  F->removeFnAttr(llvm::Attribute::NoInline);
  // optnone requires noinline and is incompatible with alwaysinline
  F->removeFnAttr(llvm::Attribute::OptimizeNone);
};

static void setAlwaysInlineRecursive(Function *F) {
  if (F->hasFnAttribute(llvm::Attribute::AlwaysInline))
    return;
  setAlwaysInline(F);
  for (auto &I : instructions(F)) {
    if (CallInst *CI = dyn_cast<CallInst>(&I)) {
      if (Function *Callee = CI->getCalledFunction()) {
        setAlwaysInlineRecursive(Callee);
      }
    }
  }
}

static void addAlwaysInlineForImageBuiltinUserFunctions(Module &M) {
  SmallVector<Function *, 16> SampledImageFunctions;
  for (auto &F : M) {
    if (F.isDeclaration()) {
      continue;
    }

    // Check if return type is image.
    if (isImageBuiltinType(F.getReturnType())) {
      SampledImageFunctions.push_back(&F);
    }
  }

  // Operand of __builtin_IB_get_image/__builtin_IB_get_sampler could be
  // result of a call instruction. The call should be inlined as well,
  // otherwise ResolveSampledImageBuiltins isn't able to resolve the two builtins.
  for (auto *F : SampledImageFunctions) {
    setAlwaysInlineRecursive(F);
  }
}

static void collectGlobalAnnotationsPointers(const Module &M, DenseSet<const Value *> &GlobalAnnotations) {
  auto *GA = M.getGlobalVariable("llvm.global.annotations");
  if (!GA)
    return;

  auto *CA = dyn_cast<ConstantArray>(GA->getInitializer());
  if (!CA)
    return;

  for (Value *Op : CA->operands()) {
    auto *Annot = dyn_cast<ConstantStruct>(Op);
    if (!Annot)
      continue;

    // In case of opaque pointers add whole struct, since it
    // will be visible as function user, otherwise add bitcast
    if (IGCLLVM::isPointerTy(Annot->getOperand(0)->getType())) {
      GlobalAnnotations.insert(Annot);
    } else {
      GlobalAnnotations.insert(Annot->getOperand(0));
    }
  }
}

bool ProcessFuncAttributes::runOnModule(Module &M) {
  MetaDataUtilsWrapper &mduw = getAnalysis<MetaDataUtilsWrapper>();
  MetaDataUtils *pMdUtils = mduw.getMetaDataUtils();
  ModuleMetaData *modMD = mduw.getModuleMetaData();
  auto MemPoolFuncs = collectMemPoolUsage(M);
  CodeGenContext *pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  EstimateFunctionSize &efs = getAnalysis<EstimateFunctionSize>();
  bool isOptDisable = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->compOpt.OptDisable;
  auto FCtrl = getFunctionControl(pCtx);

  // For controling inline/noinline on DP math builtin functions.
  pCtx->checkDPEmulationEnabled();

  std::set<llvm::Function *> fastMathFunct;
  GlobalVariable *gv_fastMath = M.getGlobalVariable(BIF_FLAG_CTRL_N_S(FastRelaxedMath), true);
  if (gv_fastMath) {
    if (gv_fastMath->getInitializer()->isOneValue()) {
      // Find the functions which FastRelaxedMath belongs to....
      for (Value::user_iterator U = gv_fastMath->user_begin(), UE = gv_fastMath->user_end(); U != UE; ++U) {
        if (Instruction *user = dyn_cast<Instruction>(*U))
          fastMathFunct.insert(user->getParent()->getParent());
      }
    }
  }

  DenseSet<const Value *> GlobalAnnotationsPtr;
  collectGlobalAnnotationsPointers(M, GlobalAnnotationsPtr);

  auto SetNoInline = [](Function *F) {
    F->addFnAttr(llvm::Attribute::NoInline);
    F->removeFnAttr(llvm::Attribute::AlwaysInline);
  };

  // Returns true if a function is either import or export and requires external linking
  auto NeedsLinking = [](Function *F) {
    // SPIRV FE translate import/export linkage to "ExternalLinkage" in LLVMIR
    // Check all "ExternalLinkage" functions. Func declarations = Import, Func definition = Export
    if (F->hasExternalLinkage() && F->getCallingConv() == CallingConv::SPIR_FUNC) {
      // builtins should not be externally linked, they will always be resolved by IGC
      return !(F->hasFnAttribute("OclBuiltin") || F->getName().startswith("__builtin_") ||
               F->getName().startswith("__igcbuiltin_") || F->getName().startswith("llvm.") ||
               F->getName().equals("printf") || Regex("^_Z[0-9]+__builtin_bf16").match(F->getName()) ||
               Regex("^_Z[0-9]+__spirv_").match(F->getName()) || Regex("^_Z[0-9]+__builtin_spirv").match(F->getName()));
    }
    return false;
  };

  // If a builtin func is a FP64 one with the given prefix, return true.
  auto IsBuiltinFP64WithPrefix = [](Function *F, const std::string &Prefix) {
    if (F->getName().startswith(Prefix)) {
      if (F->getReturnType()->isDoubleTy() ||
          (F->getReturnType()->isVectorTy() && F->getReturnType()->getContainedType(0)->isDoubleTy())) {
        auto functionName = F->getName();
        functionName = functionName.drop_front(Prefix.size());
        functionName = functionName.take_front(functionName.find('_'));

        if (OCLExtOpBuiltinNames.find(functionName.str()) != OCLExtOpBuiltinNames.end()) {
          return true;
        }
      }
    }
    return false;
  };

  // Returns true if a function is built-in double math function
  // Our implementations of double math built-in functions are precise only
  // if we don't make any fast relaxed math optimizations.
  auto IsBuiltinFP64 = [&IsBuiltinFP64WithPrefix](Function *F) { return IsBuiltinFP64WithPrefix(F, "__spirv_ocl_"); };

  // Process through all functions and add the appropriate function attributes
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function *F = &(*I);
    if (F->isDeclaration()) {
      if (F->getName() == "__translate_sampler_initializer")
        IGCLLVM::setOnlyReadsMemory(*F);

      // Functions requiring import from external module
      if (F->hasFnAttribute("referenced-indirectly") || NeedsLinking(F)) {
        pCtx->m_enableFunctionPointer = true;
        F->addFnAttr("referenced-indirectly");
        F->addFnAttr("visaStackCall");
      }

      // It is not a defined function
      continue;
    }

    // Do not reset it for critical section builtins
    if (F->hasFnAttribute("KMPLOCK")) {
      continue;
    }
    // Do not reset attributes for SYCL unmasked functions.
    if (IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions) && F->hasFnAttribute("sycl-unmasked")) {
      continue;
    }
    // Always inline for non-compute
    if (pCtx->type != ShaderType::OPENCL_SHADER && pCtx->type != ShaderType::RAYTRACING_SHADER &&
        pCtx->type != ShaderType::COMPUTE_SHADER) {
      setAlwaysInline(F);
      continue;
    }
    // Set noinline on optnone user functions.
    if (!F->hasFnAttribute("OclBuiltin") && F->hasFnAttribute(llvm::Attribute::OptimizeNone)) {
      SetNoInline(F);
    }

    for (auto I : F->users()) {
      if (CallInst *callInst = dyn_cast<CallInst>(&*I)) {
        // Go through call sites and remove NoInline atrributes.
        // Verifier fails if a call has optnone but not noinline, so if we remove noinline, we must also remove optnone
        if (callInst->hasFnAttr(llvm::Attribute::NoInline)) {
          callInst->removeFnAttr(llvm::Attribute::NoInline);
          callInst->removeFnAttr(llvm::Attribute::OptimizeNone);
        }
        // Remove AlwaysInline at callsites
        if (isOptDisable && callInst->hasFnAttr(llvm::Attribute::AlwaysInline)) {
          callInst->removeFnAttr(llvm::Attribute::AlwaysInline);
        }
      }
    }

    // set function attributes according to build options so
    // inliner doesn't conservatively turn off unsafe optimizations
    // when inlining BIFs (see mergeAttributesForInlining() in inliner).
    const auto &opts = modMD->compOpt;
    if (opts.MadEnable)
      F->addFnAttr("less-precise-fpmad", "true");

    // Fast relaxed math implies all other flags.
    if (opts.UnsafeMathOptimizations || opts.FastRelaxedMath)
      F->addFnAttr("unsafe-fp-math", "true");

    // Finite math implies no infs and nans.
    if (opts.FiniteMathOnly || opts.FastRelaxedMath) {
      F->addFnAttr("no-infs-fp-math", "true");
      F->addFnAttr("no-nans-fp-math", "true");
    }

    // Unsafe math implies no signed zeros.
    if (opts.NoSignedZeros || opts.UnsafeMathOptimizations || opts.FastRelaxedMath) {
      F->addFnAttr("no-signed-zeros-fp-math", "true");
    }

    // set hasVLA function attribute
    {
      bool isSet = false;
      for (auto &BB : *F) {
        for (auto &Inst : BB) {
          if (AllocaInst *AI = dyn_cast<AllocaInst>(&Inst)) {
            if (!isa<ConstantInt>(AI->getArraySize())) {
              F->addFnAttr("hasVLA");
              isSet = true;
              break;
            }
          }
        }
        if (isSet)
          break;
      }
    }

    // Add Optnone to user functions but not on builtins. This allows to run optimizations on builtins.
    if (isOptDisable) {
      if (!F->hasFnAttribute("OclBuiltin") && !F->hasFnAttribute(llvm::Attribute::AlwaysInline)) {
        F->addFnAttr(llvm::Attribute::OptimizeNone);
      }
    }

    // Set for kernel functions
    const bool isKernel = isEntryFunc(pMdUtils, F);

    // Functions that have the spir_kernel calling convention
    // This may be true even if isEntryFunc returns false, for invoke kernels and cloned callable kernels
    if (!isKernel && (F->getCallingConv() == CallingConv::SPIR_KERNEL)) {
      // WA for callable kernels, always inline these.
      setAlwaysInline(F);
      continue;
    }

    // Check for functions that can be indirectly called
    bool isIndirect = false;
    if (!isKernel) {
      isIndirect =
          F->hasFnAttribute("referenced-indirectly") ||
          (getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->compOpt.IsLibraryCompilation && NeedsLinking(F));

      if (!isIndirect) {
        // Functions without Export Linkage and does not have the indirect attribute set can still be called indirectly.
        // Set the indirect flag if the function's address is taken by a non-call instruction.
        for (auto u = F->user_begin(), e = F->user_end(); u != e; u++) {
          CallInst *call = dyn_cast<CallInst>(*u);

          // If user is function bitcast or function ptr in @llvm.global.annotations we should skip it.
          if (GlobalAnnotationsPtr.find(*u) != GlobalAnnotationsPtr.end())
            continue;

          if (!call || IGCLLVM::getCalledValue(call) != F) {
            isIndirect = true;
            break;
          }
        }
      }
    }

    if (isIndirect) {
      // Add indirect call function attributes
      pCtx->m_enableFunctionPointer = true;
      F->addFnAttr("referenced-indirectly");
      F->addFnAttr("visaStackCall");
      F->setLinkage(GlobalValue::ExternalLinkage);
    }

    if (isKernel) {
      // No need to process further for kernels
      continue;
    } else if (!isIndirect) {
      // Set internal linkage for remaining non-kernel functions
      F->setLinkage(GlobalValue::InternalLinkage);
    }

    // Flag for function calls where alwaysinline must be true
    bool mustAlwaysInline = false;

    // If FunctionControl is default, 'defaultStackCall' is used to
    // control whether a call is a subroutine call or function call.
    // If FunctionControl isn't default, FunctionControl decides and
    // 'defaultStackCall' has no effect.
    bool defaultStackCall = IGC_IS_FLAG_ENABLED(EnableStackCallFuncCall);

    // Add always attribute if function is a builtin
    if (F->hasFnAttribute("OclBuiltin") || F->getName().startswith("__builtin_spirv_")) {
      // OptNone builtins are special versions of builtins assuring that all
      // theirs parameters are constant values.
      if (isOptNoneBuiltin(F->getName())) {
        // OptimizeNone attribute was only required to prevent clang optimizations.
        // We can remove it now to unblock IGC optimizations.
        F->removeFnAttr(llvm::Attribute::OptimizeNone);
        // Remove the noinline attribute to allow IGC inlining heuristic to determine inlining
        F->removeFnAttr(llvm::Attribute::NoInline);
      } else if (pCtx->m_hasDPEmu && IsBuiltinFP64WithPrefix(F, "__ocl_svml_")) {
        defaultStackCall = true;
        mustAlwaysInline = false;
      } else {
        mustAlwaysInline = true;
      }
    }
    // inline all OCL math functions if BIF_FLAG_CTRL_GET(FastRelaxedMath) is set
    else if (fastMathFunct.find(F) != fastMathFunct.end()) {
      mustAlwaysInline = true;
    } else {
      // Curently, ExtensionArgAnalysis assumes that all functions with image arguments
      // to be inlined. We add always inline for such cases.
      for (auto &arg : F->args()) {
        // TODO: Remove following the migration to opaque pointers. The following method won't be able to recognize
        // builtin types. TargetExtTy information is lost at this point (following the retyping in PreprocessSPVIR) and
        // type hints in the metadata should be used instead, see deduceIfUsesImageParams below.
        if (containsImageType(arg.getType())) {
          mustAlwaysInline = true;
          break;
        }
      }

      if (deduceIfUsesImageParams(F, modMD))
        mustAlwaysInline = true;

      if (pCtx->m_hasDPEmu && !mustAlwaysInline && !isKernel) {
        // Prefer stackcall if a func has double operations
        bool isSet = false;
        for (auto &BB : *F) {
          for (auto &aI : BB) {
            auto opc = aI.getOpcode();
            if (opc == Instruction::FMul || opc == Instruction::FDiv || opc == Instruction::FAdd ||
                opc == Instruction::FSub || opc == Instruction::SIToFP || opc == Instruction::UIToFP ||
                opc == Instruction::FPExt) {
              isSet = aI.getType()->isDoubleTy();
            } else if (opc == Instruction::FPToSI || opc == Instruction::FCmp || opc == Instruction::FPToUI ||
                       opc == Instruction::FPTrunc) {
              isSet = aI.getOperand(0)->getType()->isDoubleTy();
            } else if (isa<IntrinsicInst>(&aI) || isa<GenIntrinsicInst>(&aI)) {
              CallInst *callI = cast<CallInst>(&aI);
              isSet =
                  (callI->getType()->isDoubleTy() || std::any_of(callI->arg_begin(), callI->arg_end(),
                                                                 [](Value *v) { return v->getType()->isDoubleTy(); }));
            }
            if (isSet)
              break;
          }
          if (isSet)
            break;
        }
        if (isSet) {
          defaultStackCall = true;
          mustAlwaysInline = false;
        }
      }
    }

    // WA for scheduler kernel, must inline all calls otherwise we cannot prevent spilling
    if (pCtx->type == ShaderType::OPENCL_SHADER) {
      auto ClContext = static_cast<OpenCLProgramContext *>(pCtx);
      if (ClContext->m_InternalOptions.NoSpill) {
        mustAlwaysInline = true;
      }
    }

    // Respect user defined alwaysinline attribute
    if (FCtrl == FLAG_FCALL_DEFAULT && F->hasFnAttribute(llvm::Attribute::AlwaysInline)) {
      mustAlwaysInline = true;
    }

    if (mustAlwaysInline) {
      setAlwaysInline(F);
      continue;
    }

    // Fixme: Can we support user noinline attrib from non-OCL shader?
    if (pCtx->type != ShaderType::OPENCL_SHADER) {
      F->removeFnAttr(llvm::Attribute::NoInline);
    }

    // Set default inline mode
    if (FCtrl == FLAG_FCALL_DEFAULT) {
      // Set default function call mode to stack call
      if (defaultStackCall) {
        F->addFnAttr("visaStackCall");
        SetNoInline(F);
      }
      // default stackcall for -O0, or when FE force stackcall using attribute
      if (isOptDisable || F->hasFnAttribute("igc-force-stackcall")) {
        F->addFnAttr("visaStackCall");
        SetNoInline(F);
      }

      if (IGC_IS_FLAG_ENABLED(PartitionUnit) && efs.isStackCallAssigned(F)) {
        F->addFnAttr("visaStackCall");
        SetNoInline(F);
      }

      if (F->hasFnAttribute("hasVLA")) {
        SetNoInline(F);
      }

      if (!F->hasFnAttribute(llvm::Attribute::NoInline) && IGC_IS_FLAG_DISABLED(DisableAddingAlwaysAttribute)) {
        bool shouldAlwaysInline = (MemPoolFuncs.count(F) != 0);

        if (!shouldAlwaysInline) {
          for (auto &arg : F->args()) {
            // If argument is a pointer to GAS or aggregate type, always inline it for perf reasons
            if (isSupportedAggregateArgument(&arg) || isGASPointer(&arg)) {
              shouldAlwaysInline = true;
              break;
            }
          }
        }
        if (shouldAlwaysInline) {
          if ((IGC_IS_FLAG_ENABLED(ControlKernelTotalSize) || IGC_IS_FLAG_ENABLED(ControlUnitSize)) &&
              efs.shouldEnableSubroutine() && efs.isTrimmedFunction(F)) {
            if (IGC_IS_FLAG_ENABLED(AddNoInlineToTrimmedFunctions) || efs.isLargeKernelThresholdExceeded()) {
              SetNoInline(F);
            }
          } else {
            setAlwaysInline(F);
          }
        }
      }
    } else if (FCtrl == FLAG_FCALL_FORCE_INLINE) {
      // Forced inlining all functions
      setAlwaysInline(F);
    } else {
      // Forcing subroutines/stack-call/indirect-call
      bool forceSubroutine = FCtrl == FLAG_FCALL_FORCE_SUBROUTINE;
      bool forceStackCall = FCtrl == FLAG_FCALL_FORCE_STACKCALL;
      bool forceIndirectCall = (FCtrl == FLAG_FCALL_FORCE_INDIRECTCALL || F->hasFnAttribute("IFCALL_BUILTIN"));

      if (forceSubroutine || forceStackCall || forceIndirectCall) {
        SetNoInline(F);
        if (forceStackCall) {
          F->addFnAttr("visaStackCall");
        } else if (forceIndirectCall) {
          pCtx->m_enableFunctionPointer = true;
          F->addFnAttr("referenced-indirectly");
          F->addFnAttr("visaStackCall");
          F->setLinkage(GlobalValue::ExternalLinkage);
        }
      }
    }
  }

  // This selectively sets the FunctionControl mode for the list of line-separated
  // functions existing in 'FunctionDebug.txt' in the default IGC output folder.
  // This flag will override the default FunctionControl setting for these functions.
  //
  // For example, in FunctionDebug.txt:
  //
  // FLAG_FCALL_FORCE_INLINE:
  // foo1
  // foo2
  // FLAG_FCALL_FORCE_STACKCALL:
  // foo3
  //
  // In this file, foo1 and foo2 will be inlined, and foo3 will use stackcall. Any other functions will use the mode set
  // by FunctionControl. Can be used to set different FunctionControl modes for individual functions.
  //
  // If setting SelectiveFunctionControl=2, print out a list of callable functions to FunctionDebug.txt
  //
  auto SelectFCtrl = IGC_GET_FLAG_VALUE(SelectiveFunctionControl);
  auto printDbg = IGC_IS_FLAG_ENABLED(PrintStackCallDebugInfo);
  if (SelectFCtrl != 0) {
    if (SelectFCtrl == 2) {
      // Dump all callable function names
      std::ofstream outputFile(IGC::Debug::GetFunctionDebugFile());
      if (outputFile.is_open()) {
        for (auto &F : M) {
          if (!F.isDeclaration() && !isEntryFunc(pMdUtils, &F))
            outputFile << F.getName().str() << std::endl;
        }
      }
      outputFile.close();
    } else {
      std::ifstream inputFile(IGC::Debug::GetFunctionDebugFile());
      if (printDbg)
        dbgs() << "\nSelectiveFunctionControl enabled read from " << IGC::Debug::GetFunctionDebugFile() << "\n";

      if (inputFile.is_open()) {
        auto FunctionControlMode = FLAG_FCALL_DEFAULT;
        std::string line;
        while (std::getline(inputFile, line)) {
          StringRef sline(line);

          // Ignore empty, whitespace lines, or is comment
          if (sline.trim().empty() || sline.startswith("//"))
            continue;

          if (sline.equals("FLAG_FCALL_DEFAULT:"))
            FunctionControlMode = FLAG_FCALL_DEFAULT;
          else if (sline.equals("FLAG_FCALL_FORCE_INLINE:"))
            FunctionControlMode = FLAG_FCALL_FORCE_INLINE;
          else if (sline.equals("FLAG_FCALL_FORCE_SUBROUTINE:"))
            FunctionControlMode = FLAG_FCALL_FORCE_SUBROUTINE;
          else if (sline.equals("FLAG_FCALL_FORCE_STACKCALL:"))
            FunctionControlMode = FLAG_FCALL_FORCE_STACKCALL;
          else if (sline.equals("FLAG_FCALL_FORCE_INDIRECTCALL:"))
            FunctionControlMode = FLAG_FCALL_FORCE_INDIRECTCALL;

          else if (Function *F = M.getFunction(line)) {
            if (FunctionControlMode == FLAG_FCALL_FORCE_INLINE) {
              F->removeFnAttr("referenced-indirectly");
              F->removeFnAttr("visaStackCall");
              setAlwaysInline(F);
            } else if (FunctionControlMode == FLAG_FCALL_FORCE_SUBROUTINE) {
              F->removeFnAttr("referenced-indirectly");
              F->removeFnAttr("visaStackCall");
              SetNoInline(F);
            } else if (FunctionControlMode == FLAG_FCALL_FORCE_STACKCALL) {
              F->removeFnAttr("referenced-indirectly");
              F->addFnAttr("visaStackCall");
              SetNoInline(F);
            } else if (FunctionControlMode == FLAG_FCALL_FORCE_INDIRECTCALL) {
              pCtx->m_enableFunctionPointer = true;
              F->addFnAttr("referenced-indirectly");
              F->addFnAttr("visaStackCall");
              F->setLinkage(GlobalValue::ExternalLinkage);
              SetNoInline(F);
            }
          } else {
            if (printDbg)
              dbgs() << "Function Not in Module: ";
          }

          if (printDbg)
            dbgs() << line << "\n";
        }
        inputFile.close();
      }
    }
  }

  // Process through all functions and reset the *-fp-math attributes
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function *F = &(*I);
    if (!F->isDeclaration()) {
      if (IsBuiltinFP64(F)) {
        addFnAttrRecursive(F, "unsafe-fp-math", "false");
        addFnAttrRecursive(F, "no-infs-fp-math", "false");
        addFnAttrRecursive(F, "no-nans-fp-math", "false");
        addFnAttrRecursive(F, "no-signed-zeros-fp-math", "false");
      }
    }
  }

  // Detect recursive calls, and convert them to stack calls, since subroutines does not support recursion
  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  if (convertRecursionToStackCall(CG)) {
    if (IGC_IS_FLAG_DISABLED(EnableRecursionOpenCL) && !pCtx->m_DriverInfo.AllowRecursion()) {
      IGC_ASSERT_MESSAGE(0, "Recursion detected but not enabled!");
    }
    if (IGC::ForceAlwaysInline(pCtx)) {
      IGC_ASSERT_MESSAGE(0, "Cannot have recursion when forcing inline!");
    }
  }

  // Enable subroutines flag based on function attributes
  pCtx->CheckEnableSubroutine(M);

  if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
    // stackoverflow detection functions will be subroutines, so that we can
    // call them easily in emit pass as well.
    std::array<StringRef, 2> DetectionFunctions = {StackOverflowDetectionPass::STACK_OVERFLOW_INIT_BUILTIN_NAME,
                                                   StackOverflowDetectionPass::STACK_OVERFLOW_DETECTION_BUILTIN_NAME};

    for (StringRef FuncName : DetectionFunctions) {
      if (auto F = M.getFunction(FuncName)) {
        F->removeFnAttr("referenced-indirectly");
        F->removeFnAttr("visaStackCall");
        SetNoInline(F);
      }
    }
  }

  // Curently, ExtensionArgAnalysis assumes that for all functions that use
  // image builtins directly or indirectly, we add alwaysinline attribute.
  // For SYCL bindless image, checking function argument isn't enough because
  // * image handle could be an integer.
  // * image handle could be from a call instead of function argument.
  // Therefore, we explore all users functions of image builtin recursively.
  addAlwaysInlineForImageBuiltinUserFunctions(M);

  return true;
}

//
// ProcessBuiltinMetaData
//
namespace {

class ProcessBuiltinMetaData : public ModulePass {
public:
  static char ID;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  ProcessBuiltinMetaData();

  ~ProcessBuiltinMetaData() {}

  virtual bool runOnModule(Module &M);

  virtual llvm::StringRef getPassName() const { return "ProcessBuiltinMetaData"; }

private:
  void updateBuiltinFunctionMetaData(llvm::Function *);
  bool removeDeletedFunctionsMetaData(Module &M, MetaDataUtils *MdUtil);

  MetaDataUtils *m_pMdUtil;
};

} // namespace

// Register pass to igc-opt
#define PASS_FLAG2 "igc-process-builtin-metaData"
#define PASS_DESCRIPTION2 "Set builtin MetaData"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(ProcessBuiltinMetaData, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ProcessBuiltinMetaData, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

char ProcessBuiltinMetaData::ID = 0;

ProcessBuiltinMetaData::ProcessBuiltinMetaData() : ModulePass(ID) {
  initializeProcessBuiltinMetaDataPass(*PassRegistry::getPassRegistry());
}

ModulePass *createProcessBuiltinMetaDataPass() { return new ProcessBuiltinMetaData(); }

bool ProcessBuiltinMetaData::runOnModule(Module &M) {
  CodeGenContext *pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (IGC::ForceAlwaysInline(pCtx)) {
    return false;
  }

  m_pMdUtil = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

  bool Changed = false;
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function *F = &(*I);
    if (!F || F->isDeclaration())
      continue;

    // disable JumpThread optimization on the block that contains this function
    F->setConvergent();

    if (m_pMdUtil->findFunctionsInfoItem(F) == m_pMdUtil->end_FunctionsInfo()) {
      // It is user Function
      updateBuiltinFunctionMetaData(F);
    }
    Changed = true;
  }

  Changed |= removeDeletedFunctionsMetaData(M, m_pMdUtil);

  if (Changed)
    m_pMdUtil->save(M.getContext());

  return Changed;
}

void ProcessBuiltinMetaData::updateBuiltinFunctionMetaData(llvm::Function *pFunc) {
  IGCMD::FunctionInfoMetaDataHandle fHandle = IGCMD::FunctionInfoMetaDataHandle(new IGCMD::FunctionInfoMetaData());
  IGC::ModuleMetaData *modMD = getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->getModuleMetaData();
  FunctionMetaData *funcMD = &modMD->FuncMD[pFunc]; // okay to insert if not present
  funcMD->functionType = IGC::FunctionTypeMD::UserFunction;
  fHandle->setType(FunctionTypeMD::UserFunction);
  for (auto arg = pFunc->arg_begin(); arg != pFunc->arg_end(); ++arg) {
    std::string typeStr;
    llvm::raw_string_ostream x(typeStr);
    arg->getType()->print(x);

    funcMD->m_OpenCLArgNames.push_back(arg->getName().str());
    funcMD->m_OpenCLArgAccessQualifiers.push_back("none");
    funcMD->m_OpenCLArgBaseTypes.push_back(x.str());
  }
  m_pMdUtil->setFunctionsInfoItem(pFunc, fHandle);
}

bool ProcessBuiltinMetaData::removeDeletedFunctionsMetaData(Module &M, MetaDataUtils *MdUtil) {
  bool changed = false;

  MetaDataUtils::FunctionsInfoMap::iterator i = MdUtil->begin_FunctionsInfo();
  MetaDataUtils::FunctionsInfoMap::iterator e = MdUtil->end_FunctionsInfo();
  while (i != e) {
    Function *F_meta = (*i).first;

    bool deleted = true;
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
      Function *F = &(*I);
      if (F_meta == F) {
        deleted = false;
        break;
      }
    }

    MetaDataUtils::FunctionsInfoMap::iterator prev = i++;

    if (deleted)
      MdUtil->eraseFunctionsInfoItem(prev);

    changed |= deleted;
  }

  return changed;
}

//
// InsertDummyKernelForSymbolTable
//
namespace {

class InsertDummyKernelForSymbolTable : public ModulePass {
public:
  static char ID;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  InsertDummyKernelForSymbolTable();

  ~InsertDummyKernelForSymbolTable() {}

  virtual bool runOnModule(Module &M);

  virtual llvm::StringRef getPassName() const { return "InsertDummyKernelForSymbolTable"; }

private:
};

} // namespace

// Register pass to igc-opt
#define PASS_FLAG3 "igc-insert-dummy-kernel-for-symbol-table"
#define PASS_DESCRIPTION3 "If symbol table is needed, insert a dummy kernel to attach it to"
#define PASS_CFG_ONLY3 false
#define PASS_ANALYSIS3 false
IGC_INITIALIZE_PASS_BEGIN(InsertDummyKernelForSymbolTable, PASS_FLAG3, PASS_DESCRIPTION3, PASS_CFG_ONLY3,
                          PASS_ANALYSIS3)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(InsertDummyKernelForSymbolTable, PASS_FLAG3, PASS_DESCRIPTION3, PASS_CFG_ONLY3, PASS_ANALYSIS3)

char InsertDummyKernelForSymbolTable::ID = 0;

InsertDummyKernelForSymbolTable::InsertDummyKernelForSymbolTable() : ModulePass(ID) {
  initializeInsertDummyKernelForSymbolTablePass(*PassRegistry::getPassRegistry());
}

ModulePass *createInsertDummyKernelForSymbolTablePass() { return new InsertDummyKernelForSymbolTable(); }

bool InsertDummyKernelForSymbolTable::runOnModule(Module &M) {
  MetaDataUtilsWrapper &mduw = getAnalysis<MetaDataUtilsWrapper>();
  MetaDataUtils *pMdUtils = mduw.getMetaDataUtils();
  ModuleMetaData *modMD = mduw.getModuleMetaData();
  CodeGenContext *pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  bool needDummyKernel = false;

  // Check when we need to generate a dummy kernel. This is only useful for attaching
  // the symbol table to its program output for indirect calls and global variable relocation.
  if (IGC_IS_FLAG_ENABLED(EnableFunctionPointer)) {
    if (pCtx->m_enableFunctionPointer) {
      // Symbols are needed for external functions and function pointers
      needDummyKernel = true;
    } else if (!modMD->inlineProgramScopeOffsets.empty()) {
      // Create one also if global variables are present and require symbols
      needDummyKernel = true;
    } else if (pCtx->m_hasStackCalls && !getUniqueEntryFunc(pMdUtils, modMD)) {
      // If there are stackcalls and multiple kernels from which it could be called, conservatively create a
      // dummy kernel in case we need to transform them into indirect calls to avoid cloning in GenCodeGenModule.cpp
      needDummyKernel = true;
    }
  }

  if (needDummyKernel) {
    // Create empty kernel function
    IGC_ASSERT(IGC::getIntelSymbolTableVoidProgram(&M) == nullptr);
    Type *voidTy = Type::getVoidTy(M.getContext());
    FunctionType *fTy = FunctionType::get(voidTy, false);
    Function *pNewFunc = Function::Create(fTy, GlobalValue::ExternalLinkage, IGC::INTEL_SYMBOL_TABLE_VOID_PROGRAM, &M);
    BasicBlock *entry = BasicBlock::Create(M.getContext(), "entry", pNewFunc);
    IRBuilder<> builder(entry);
    builder.CreateRetVoid();

    // Set spirv calling convention and kernel metadata
    pNewFunc->setCallingConv(llvm::CallingConv::SPIR_KERNEL);
    IGCMD::FunctionInfoMetaDataHandle fHandle = IGCMD::FunctionInfoMetaDataHandle(new IGCMD::FunctionInfoMetaData());
    FunctionMetaData *funcMD = &modMD->FuncMD[pNewFunc];
    funcMD->functionType = IGC::FunctionTypeMD::KernelFunction;
    fHandle->setType(FunctionTypeMD::KernelFunction);

    pMdUtils->setFunctionsInfoItem(pNewFunc, fHandle);
    pMdUtils->save(M.getContext());

    return true;
  }
  return false;
}
