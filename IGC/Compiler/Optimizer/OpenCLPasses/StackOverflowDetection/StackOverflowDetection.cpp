/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/StackOverflowDetection/StackOverflowDetection.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstIterator.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/ADT/StringRef.h"
using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-stackoverflow-detection"
#define PASS_DESCRIPTION "Insert calls to stack overflow detection builtins."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(StackOverflowDetectionPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(StackOverflowDetectionPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char StackOverflowDetectionPass::ID = 0;

StackOverflowDetectionPass::StackOverflowDetectionPass() : ModulePass(ID) {
  initializeStackOverflowDetectionPassPass(*PassRegistry::getPassRegistry());
}

StackOverflowDetectionPass::StackOverflowDetectionPass(Mode mode_) : StackOverflowDetectionPass() { mode = mode_; }

bool StackOverflowDetectionPass::removeDummyCalls(Module &M) {
  std::vector<llvm::Instruction *> ToDeleteInstructions;

  for (Function &F : M) {
    for (auto &&BB : F) {
      for (auto &I : BB) {
        if (auto callI = llvm::dyn_cast<llvm::CallInst>(&I)) {
          Function *callFunction = callI->getCalledFunction();
          if (callFunction) {
            auto callFunctionName = callFunction->getName();
            if (IGCLLVM::starts_with(callFunctionName, STACK_OVERFLOW_INIT_BUILTIN_NAME) ||
                IGCLLVM::starts_with(callFunctionName, STACK_OVERFLOW_DETECTION_BUILTIN_NAME)) {
              ToDeleteInstructions.push_back(&I);
            }
          }
        }
      }
    }
  }

  for (auto I : ToDeleteInstructions) {
    I->eraseFromParent();
  }
  return !ToDeleteInstructions.empty();
}

bool StackOverflowDetectionPass::removeCallsAndFunctionsIfNoStackCallsOrVLA(Module &M, IGCMD::MetaDataUtils *pMdUtils,
                                                                            ModuleMetaData *pModMD) {
  bool changed = false;
  bool HasStackCallsOrVLA = std::any_of(M.getFunctionList().begin(), M.getFunctionList().end(), [](auto &F) {
    return F.hasFnAttribute("visaStackCall") || F.hasFnAttribute("hasVLA");
  });
  if (!HasStackCallsOrVLA) {
    for (auto FuncName : {STACK_OVERFLOW_INIT_BUILTIN_NAME, STACK_OVERFLOW_DETECTION_BUILTIN_NAME}) {
      if (auto F = M.getFunction(FuncName)) {
        std::vector<llvm::CallInst *> CallersToDelete;
        for (auto User : F->users()) {
          if (auto I = dyn_cast<llvm::CallInst>(User)) {
            CallersToDelete.push_back(I);
          }
        }
        std::for_each(CallersToDelete.begin(), CallersToDelete.end(),
                      [](auto CallInst) { CallInst->eraseFromParent(); });

        changed = true;

        IGCMD::IGCMetaDataHelper::removeFunction(*pMdUtils, *pModMD, F);
        F->removeDeadConstantUsers();
        F->eraseFromParent();
      }
    }
  }
  return changed;
}

bool StackOverflowDetectionPass::runOnModule(Module &M) {
  if (IGC_IS_FLAG_DISABLED(StackOverflowDetection)) {
    return false;
  }

  bool changed = false;
  auto &MDUWAnalysis = getAnalysis<MetaDataUtilsWrapper>();
  auto pMdUtils = MDUWAnalysis.getMetaDataUtils();
  auto pModMD = MDUWAnalysis.getModuleMetaData();
  const bool isLibraryCompilation = pModMD->compOpt.IsLibraryCompilation;

  // The pass is designed to be run at least two times.
  // In the first run (Mode::Initialize) it inserts the detection functions
  // to entry points without analysis if they are really needed,
  // as it is not possible at this stage - it is meant to be called before BIImport
  // and PrintfResoulution passes, as the implementation of the detection functions is in the
  // builtin module. So the first run is before inlining and we are not sure if stack calls will be present.
  // Also VLAs can be optimized later in the compilation.
  //
  // The next run (Mode::AnalyzeAndCleanup) of this pass is meant to cleanup the calls if they are not really needed.
  //
  // Third run (Mode::RemoveDummyCalls) removes previously inserted calls to __stackoverflow_init and
  // __stackoverflow_detection. These will be inserted again in EmitVisaPass and it's impossible to do it
  // correctly before this pass.
  // Two dummy calls that we insterted in Mode::Initialize run were there to prevent dead code elimination
  // of __stackoverflow_init and __stackoverflow_detection implementations before reaching EmitVisaPass.

  if (mode == Mode::RemoveDummyCalls || mode == Mode::AnalyzeAndCleanup) {
    if (mode == Mode::RemoveDummyCalls) {
      changed = removeDummyCalls(M);
    } else {
      changed = removeCallsAndFunctionsIfNoStackCallsOrVLA(M, pMdUtils, pModMD);
    }

    if (changed) {
      CodeGenContext *pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
      pMdUtils->save(*pContext->getLLVMContext());
    }

    // Attach debug info to stack overflow detection functions so gdb-oneapi
    // can identify the reason for the software exception
    if (mode == Mode::RemoveDummyCalls) {
      changed |= attachDebugInfo(M);
    }

    return changed;
  }

  IGC_ASSERT(mode == Mode::Initialize);
  for (Function &F : M) {
    const bool isEntryFunction = isEntryFunc(pMdUtils, &F);
    if (isEntryFunction || isLibraryCompilation) {
      if (F.isDeclaration())
        continue;
      IGCLLVM::IRBuilder<> builder(&*F.begin()->begin());

      if (isEntryFunction) {
        auto StackOverflowInitFunc = M.getOrInsertFunction(
            STACK_OVERFLOW_INIT_BUILTIN_NAME, FunctionType::get(Type::getVoidTy(M.getContext()), {}, false));
        Function *Callee = cast<Function>(StackOverflowInitFunc.getCallee());
        IGC_ASSERT(Callee);
        auto InitCall = builder.CreateCall(Callee);
        InitCall->setCallingConv(CallingConv::SPIR_FUNC);
      }
      auto StackOverflowDetectionFunc = M.getOrInsertFunction(
          STACK_OVERFLOW_DETECTION_BUILTIN_NAME, FunctionType::get(Type::getVoidTy(M.getContext()), {}, false));
      Function *Callee = cast<Function>(StackOverflowDetectionFunc.getCallee());
      IGC_ASSERT(Callee);
      auto CallInst = builder.CreateCall(Callee);
      CallInst->setCallingConv(CallingConv::SPIR_FUNC);

      changed = true;
    }
  }

  return changed;
}

bool StackOverflowDetectionPass::attachDebugInfo(Module &M) {
  // Check if the module has debug info. If not, there's nothing to attach to.
  if (M.debug_compile_units().begin() == M.debug_compile_units().end())
    return false;

  DICompileUnit *CU = *M.debug_compile_units().begin();
  if (!CU)
    return false;

  bool changed = false;

  StringRef BuiltinNameRef(STACK_OVERFLOW_DETECTION_BUILTIN_NAME);

  // Check if cloned variants (e.g. _GenXClone) exist. If they do, only
  // attach debug info to the clones — they are the functions that actually
  // get compiled as vISA subroutines and receive address ranges in DWARF.
  // Attaching to the original would create an orphaned DW_TAG_subprogram
  // without address information.
  bool HasClone = false;
  for (const Function &F : M) {
    if (IGCLLVM::starts_with(F.getName(), BuiltinNameRef) && F.getName() != BuiltinNameRef && !F.isDeclaration()) {
      HasClone = true;
      break;
    }
  }

  for (Function &F : M) {
    if (!IGCLLVM::starts_with(F.getName(), BuiltinNameRef))
      continue;
    if (F.isDeclaration())
      continue;

    // If clones exist, strip the DISubprogram from the original function.
    // The original was given a DISubprogram speculatively in Initialize mode
    // (before cloning), but the clone is the function that actually gets
    // compiled as a vISA subroutine. Keeping the original's DISubprogram
    // would create an orphaned DW_TAG_subprogram without address ranges.
    if (HasClone && F.getName() == BuiltinNameRef) {
      if (F.getSubprogram()) {
        F.setSubprogram(nullptr);
        changed = true;
      }
      continue;
    }

    // Skip if the function already has debug info attached.
    if (F.getSubprogram())
      continue;

    // Create a DISubroutineType for a void() function.
    DISubroutineType *SubroutineType = DISubroutineType::get(M.getContext(), DINode::FlagZero, 0, DITypeRefArray());

    // Create a DISubprogram with the debugger-visible name.
    // Using FlagArtificial to indicate this is compiler-generated.
    DISubprogram *SP = DISubprogram::getDistinct(M.getContext(),
                                                 /*Scope=*/CU,
                                                 /*Name=*/BuiltinNameRef,
                                                 /*LinkageName=*/F.getName(),
                                                 /*File=*/CU->getFile(),
                                                 /*Line=*/0,
                                                 /*Type=*/SubroutineType,
                                                 /*ScopeLine=*/0,
                                                 /*ContainingType=*/nullptr,
                                                 /*VirtualIndex=*/0,
                                                 /*ThisAdjustment=*/0,
                                                 /*Flags=*/DINode::FlagArtificial,
                                                 /*SPFlags=*/DISubprogram::SPFlagDefinition,
                                                 /*Unit=*/CU);

    F.setSubprogram(SP);

    DebugLoc DL = DILocation::get(M.getContext(), 0, 0, SP);
    if (auto *EntryI = &*inst_begin(F)) {
      EntryI->setDebugLoc(DL);
    }

    changed = true;
  }

  return changed;
}
