/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/Optimizer/OpenCLPasses/TransformUnmaskedFunctionsPass/TransformUnmaskedFunctionsPass.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/SetVector.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// This pass scans the code for functions marked with 'unmasked' annotations.
// When an unmaksed function is detected each of its basic blocks is makred
// with UnmaskedRegionBegin and UnmaskedRegionEnd intrinsics. Those intrinsics
// are later used by EmitVISA pass to mark all instructions in between with
// NoMask attribute.
//
// This pass must be called early, before any inlining to work correctly.
// This pass will fail compilation if non-uniform control flow is detected.
#define PASS_FLAG "transform-unmasked"
#define PASS_DESCRIPTION "Handle unmaksed functions."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(TransformUnmaskedFunctionsPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(TransformUnmaskedFunctionsPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char TransformUnmaskedFunctionsPass::ID = 0;

TransformUnmaskedFunctionsPass::TransformUnmaskedFunctionsPass() : FunctionPass(ID), MMD(nullptr) {}

static void annotateUnmaskedCallSite(CallInst *CI) {
  IRBuilder<> builder(CI);

  Module *M = CI->getModule();

  Function *unmaskedBegin = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_UnmaskedRegionBegin);
  Function *unmaskedEnd = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_UnmaskedRegionEnd);

  builder.CreateCall(unmaskedBegin);
  builder.SetInsertPoint(CI->getNextNonDebugInstruction());
  builder.CreateCall(unmaskedEnd);
}

[[maybe_unused]] static void annotateUnmaskedBasicBlock(BasicBlock *BB) {
  IRBuilder<> builder(&*BB->begin());

  Module *M = BB->getModule();

  Function *unmaskedBegin = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_UnmaskedRegionBegin);
  Function *unmaskedEnd = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_UnmaskedRegionEnd);

  builder.CreateCall(unmaskedBegin);
  builder.SetInsertPoint(BB->getTerminator());
  builder.CreateCall(unmaskedEnd);
}

struct TrivialUniformity {
  static TrivialUniformity Unifrom() { return {UNIFORM, ""}; }

  static TrivialUniformity NonUnifrom(const std::string &reason) { return {NONUNIFORM, reason}; }

  static TrivialUniformity FormalArgument() { return {FORMAL_ARG, ""}; }

  static TrivialUniformity PhiResult() { return {PHI_RESULT, ""}; }

  enum { UNIFORM, NONUNIFORM, FORMAL_ARG, PHI_RESULT } kind;
  std::string reason;
};

using UniformityCache = DenseMap<const Value *, TrivialUniformity>;

static TrivialUniformity checkValue(const Value *Val, UniformityCache *Cache);
static bool isFunctionTriviallyUniform(const Function *F, TrivialUniformity *outResult, UniformityCache *Cache);

static TrivialUniformity mergeUnifromity(TrivialUniformity a, TrivialUniformity b) {
  if (a.kind == TrivialUniformity::NONUNIFORM || b.kind == TrivialUniformity::NONUNIFORM) {
    return TrivialUniformity::NonUnifrom(a.reason + " " + b.reason);
  }
  if (a.kind == TrivialUniformity::FORMAL_ARG || b.kind == TrivialUniformity::FORMAL_ARG) {
    return TrivialUniformity::FormalArgument();
  }
  /* note: && instead of ||*/
  if (a.kind == TrivialUniformity::PHI_RESULT && b.kind == TrivialUniformity::PHI_RESULT) {
    return TrivialUniformity::PhiResult();
  }
  return TrivialUniformity::Unifrom();
}

/* Allow only simple conversions. In case of other functions user should
 * rewrite the kernel. */
static const char *KnownPrefixes[] = {
    "__spirv_SConvert",
    "__spirv_UConvert",
};
static bool isKnownUniformLibraryFunction(const Function *F) {
  if (F == nullptr) {
    return false;
  }
  for (const char *prefix : KnownPrefixes) {
    if (F->getName().contains(prefix))
      return true;
  }
  return false;
}

static TrivialUniformity checkCallInst(const CallInst *CI, UniformityCache *Cache) {
  const Function *F = CI->getCalledFunction();
  if (F == nullptr) { /* indirect call */
    return TrivialUniformity::Unifrom();
  }

  if (F->isDeclaration() && isKnownUniformLibraryFunction(CI->getCalledFunction()) == false) {
    const std::string name = CI->getCalledFunction()->getName().str();
    return TrivialUniformity::NonUnifrom("Expression depends on function result "
                                         "that isn't a known uniform function: '" +
                                         name + "'.");
  }

  TrivialUniformity result = TrivialUniformity::Unifrom();
  isFunctionTriviallyUniform(F, &result, Cache);

  if (result.kind == TrivialUniformity::FORMAL_ARG) {
    /* If uniformity of the function depends on a formal argument, check if all actual arguments are uniform: */
    result = TrivialUniformity::Unifrom();
    const size_t count = IGCLLVM::getNumArgOperands(CI);
    for (size_t i = 0; i < count; ++i) {
      result = mergeUnifromity(result, checkValue(CI->getArgOperand(i), Cache));
      if (result.kind == TrivialUniformity::NONUNIFORM) {
        return result;
      }
    }
  }
  return result;
}

static TrivialUniformity checkValue(const Value *Val, UniformityCache *Cache) {
  if (isa<Argument>(Val)) {
    return TrivialUniformity::FormalArgument();
  }
  if (isa<Constant>(Val) || isa<BasicBlock>(Val)) { /* Assume constants to be uniform. */
    return TrivialUniformity::Unifrom();
  }
  if (!isa<Instruction>(Val) && !isa<Operator>(Val)) { /* The check supports only instructions and opeators. */
    return TrivialUniformity::NonUnifrom("Unexpected IR value type.");
  }

  auto cached = Cache->find(Val);
  if (cached != Cache->end()) {
    return cached->second;
  }

  TrivialUniformity result = TrivialUniformity::Unifrom();
  /* Check instruction uniformity: */
  if (const CallInst *CI = dyn_cast<CallInst>(Val)) {
    result = checkCallInst(CI, Cache);
  } else if (const PHINode *PI = dyn_cast<PHINode>(Val)) {
    /* insert temporary PHI_RESULT to break any potential cycles */
    Cache->insert(std::make_pair(Val, TrivialUniformity::PhiResult()));
    const size_t count = PI->getNumOperands();
    for (size_t i = 0; i < count; i++) {
      result = mergeUnifromity(result, checkValue(PI->getOperand(i), Cache));
      if (result.kind == TrivialUniformity::NONUNIFORM) {
        break;
      }
    }
    /* override temporary value with the actual result */
    Cache->insert(std::make_pair(Val, result));
  } else {
    const User *U = dyn_cast<User>(Val);
    IGC_ASSERT_MESSAGE(U, "Expected instruction or operator.");

    const size_t count = U->getNumOperands();
    for (size_t i = 0; i < count; i++) {
      result = mergeUnifromity(result, checkValue(U->getOperand(i), Cache));
      if (result.kind == TrivialUniformity::NONUNIFORM) {
        break;
      }
    }
  }

  Cache->insert(std::make_pair(Val, result));
  return result;
}

static bool isFunctionTriviallyUniform(const Function *F, TrivialUniformity *outResult, UniformityCache *Cache) {
  TrivialUniformity result = TrivialUniformity::Unifrom();
  for (const BasicBlock &BB : *F) {
    const Instruction *I = BB.getTerminator();
    result = mergeUnifromity(result, checkValue(I, Cache));
    if (result.kind == TrivialUniformity::NONUNIFORM) {
      break;
    }
  }
  if (outResult) {
    *outResult = result;
  }
  return result.kind == TrivialUniformity::UNIFORM;
}

bool TransformUnmaskedFunctionsPass::runOnFunction(llvm::Function &F) {
  if (!F.hasFnAttribute("sycl-unmasked"))
    return false;

  UniformityCache Cache;
  TrivialUniformity result;
  if (isFunctionTriviallyUniform(&F, &result, &Cache) == false) {
    std::stringstream stream;
    stream << "\nDetected non-uniform control flow inside unmasked function '" << F.getName().str() << "': '"
           << result.reason << "'\n";
    std::string errorMessage = stream.str();
    getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitError(errorMessage.c_str(), &F);
  }

  F.removeFnAttr(llvm::Attribute::AlwaysInline);
  F.addFnAttr(llvm::Attribute::NoInline);
  for (User *U : F.users()) {
    if (CallInst *CI = dyn_cast<CallInst>(U)) {
      CI->removeFnAttr(llvm::Attribute::AlwaysInline);
      CI->addFnAttr(llvm::Attribute::NoInline);
    }
  }
  return true;
}

#define IPASS_FLAG "inline-unmasked"
#define IPASS_DESCRIPTION "Handle unmaksed functions."
#define IPASS_CFG_ONLY false
#define IPASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InlineUnmaskedFunctionsPass, IPASS_FLAG, IPASS_DESCRIPTION, IPASS_CFG_ONLY, IPASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(InlineUnmaskedFunctionsPass, IPASS_FLAG, IPASS_DESCRIPTION, IPASS_CFG_ONLY, IPASS_ANALYSIS)

char InlineUnmaskedFunctionsPass::ID = 0;

InlineUnmaskedFunctionsPass::InlineUnmaskedFunctionsPass() : ModulePass(ID), MMD(nullptr) {}

bool InlineUnmaskedFunctionsPass::runOnModule(llvm::Module &M) {
  MMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

  CodeGenContext *pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  IGCMD::MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

  if (IGC_IS_FLAG_ENABLED(LateInlineUnmaskedFunc)) {
    // Clear function groups to safely remove functions later
    auto m_FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();
    if (m_FGA) {
      m_FGA->clear();
    }
  }
  // There is a case in Embree where two iterations of inlining is
  // required. Consider such a case:
  // F: ... call F1 ...
  // F1: ... call F2 ...
  // F2: ...
  // There are two call sites here. If F1 is inlined first then
  // call to F2 is cloned and there are two calls to F2 after F1
  // inlining. And only one call to F2 is collected in 'Calls'
  // vector during first iteration.
  bool changed = false;
  do {

    llvm::SmallSetVector<Function *, 16> Funcs;
    for (Function &F : M) {
      if (F.hasFnAttribute("sycl-unmasked")) {
        Funcs.insert(&F);
      }
    }

    if (Funcs.size() == 0)
      break;

    changed = true;
    auto &FuncMD = pContext->getModuleMetaData()->FuncMD;
    llvm::SmallSetVector<CallInst *, 16> Calls;

    for (Function *F : Funcs) {
      F->removeFnAttr(llvm::Attribute::NoInline);
      F->addFnAttr(llvm::Attribute::AlwaysInline);

      F->setLinkage(llvm::GlobalValue::InternalLinkage);

      auto Iter = pMdUtils->findFunctionsInfoItem(F);
      if (Iter != pMdUtils->end_FunctionsInfo()) {
        pMdUtils->eraseFunctionsInfoItem(Iter);
      }
      if (FuncMD.find(F) != FuncMD.end()) {
        FuncMD.erase(F);
      }

      for (User *U : F->users()) {
        if (auto *CB = dyn_cast<CallInst>(U)) {
          if (CB->getCalledFunction() == F) {
            Calls.insert(CB);
            annotateUnmaskedCallSite(CB);
          }
        }
      }
    }

    llvm::InlineFunctionInfo IFI;
    for (auto *CB : Calls)
      IGCLLVM::InlineFunction(*CB, IFI);

    for (Function *F : Funcs) {
      F->removeDeadConstantUsers();
      if (F->isDefTriviallyDead())
        F->eraseFromParent();
    }

  } while (changed);

  if (changed) {
    pMdUtils->save(*pContext->getLLVMContext());
    // The module has functions with unmasked region(s). Keep this info
    // for later optimizations tuning (at least disabling).
    pContext->m_instrTypes.hasUnmaskedRegion = true;
  }
  return changed;
}
