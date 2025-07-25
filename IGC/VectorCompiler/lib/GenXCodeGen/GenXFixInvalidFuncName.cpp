/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXFixInvalidFuncName
/// ------------
///
/// GenXFixInvalidFuncName is a function pass that replaces all '.' and '$'
/// characters with '_' in function names generated by llvm in all call
/// instructions in the current function and in the name of the current
/// function. It needs to be kept in sync with the same pass on the SPMD side.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"

#include "llvm/IR/Function.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"

#include "vc/GenXCodeGen/GenXFixInvalidFuncName.h"

#define DEBUG_TYPE "GENX_FIX_INVALID_FUNC_NAMES"

using namespace llvm;

class GenXFixInvalidFuncName : public FunctionPass {
public:
  static char ID;
  explicit GenXFixInvalidFuncName() : FunctionPass(ID) {}
  llvm::StringRef getPassName() const override {
    return "Fix Invalid Func Name";
  }
  bool runOnFunction(Function &F) override;

private:
  std::string replaceInvalidCharToUnderline(std::string str);
  bool isSupportedCallingConv(CallingConv::ID callingConv);
  bool changeFuncName(Function &F);
};

bool GenXFixInvalidFuncName::runOnFunction(Function &F) {
  bool modified = false;
  if (isSupportedCallingConv(F.getCallingConv())) {
    modified = changeFuncName(F);
  }
  for (Function::iterator fi = F.begin(), fe = F.end(); fi != fe; ++fi) {
    BasicBlock *BB = &*fi;
    for (BasicBlock::iterator bi = BB->begin(), be = BB->end(); bi != be;
         ++bi) {
      Instruction *Inst = &*bi;
      if (CallInst *callInst = dyn_cast<CallInst>(Inst)) {
        if (isSupportedCallingConv(callInst->getCallingConv())) {
          Function *func = callInst->getCalledFunction();
          if (func) {
            modified = changeFuncName(*func);
          }
        }
      }
    }
  }
  return modified;
}

std::string
GenXFixInvalidFuncName::replaceInvalidCharToUnderline(std::string str) {
  std::replace_if(
      str.begin(), str.end(), [](const char c) { return c == '.' || c == '$'; },
      '_');
  return str;
}

bool GenXFixInvalidFuncName::isSupportedCallingConv(
    CallingConv::ID callingConv) {
  return callingConv == CallingConv::SPIR_FUNC;
}

bool GenXFixInvalidFuncName::changeFuncName(Function &F) {
  bool isNameChanged = false;
  std::string original = F.getName().str();
  std::string changed = replaceInvalidCharToUnderline(original);
  if (original != changed) {
    F.setName(changed);
    isNameChanged = true;
  }
  return isNameChanged;
}

char GenXFixInvalidFuncName::ID = 0;
namespace llvm {
void initializeGenXFixInvalidFuncNamePass(PassRegistry &);
}

#if LLVM_VERSION_MAJOR >= 16
llvm::PreservedAnalyses
GenXFixInvalidFuncNamePass::run(Function &F, FunctionAnalysisManager &AM) {
  GenXFixInvalidFuncName GenXFixInvFunc;
  if (GenXFixInvFunc.runOnFunction(F))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}
#endif

INITIALIZE_PASS_BEGIN(GenXFixInvalidFuncName, "GenXFixInvalidFuncName",
                      "GenXFixInvalidFuncName", false, false)
INITIALIZE_PASS_END(GenXFixInvalidFuncName, "GenXFixInvalidFuncName",
                    "GenXFixInvalidFuncName", false, false)
FunctionPass *llvm::createGenXFixInvalidFuncNamePass() {
  initializeGenXFixInvalidFuncNamePass(*PassRegistry::getPassRegistry());
  return new GenXFixInvalidFuncName;
}
