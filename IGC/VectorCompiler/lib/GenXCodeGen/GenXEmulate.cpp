/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//
/// GenXEmulate
/// -----------
///
/// GenXEmulate is a mudule pass that emulates certain LLVM IR instructions.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "GENX_EMULATION"

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace genx;

namespace {

using IRBuilder = IRBuilder<TargetFolder>;

class GenXEmulate : public ModulePass {
  // Maps <opcode, type> to its corresponding emulation function.
  using OpType = std::pair<unsigned, Type *>;
  std::vector<Instruction *> ToErase;
  std::map<OpType, Function *> EmulationFuns;
  const GenXSubtarget *ST = nullptr;

public:
  static char ID;
  explicit GenXEmulate() : ModulePass(ID) {}
  virtual StringRef getPassName() const { return "GenX emulation"; }
  void getAnalysisUsage(AnalysisUsage &AU) const;
  bool runOnModule(Module &M);
  void runOnFunction(Function &F);

private:
  Value *emulateInst(Instruction *Inst);
  Function *getEmulationFunction(Instruction *Inst);
  // Check if a function is to emulate instructions.
  static bool isEmulationFunction(const Function* F) {
    if (F->empty())
      return false;
    if (F->hasFnAttribute("CMBuiltin"))
      return true;
    // FIXME: The above attribute is lost during SPIR-V translation.
    if (F->getName().contains("__cm_intrinsic_impl_"))
      return true;
    return false;
  }
};

} // end namespace

char GenXEmulate::ID = 0;
namespace llvm {
void initializeGenXEmulatePass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXEmulate, "GenXEmulate", "GenXEmulate", false, false)
INITIALIZE_PASS_END(GenXEmulate, "GenXEmulate", "GenXEmulate", false, false)

ModulePass *llvm::createGenXEmulatePass() {
  initializeGenXEmulatePass(*PassRegistry::getPassRegistry());
  return new GenXEmulate;
}

void GenXEmulate::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
}

bool GenXEmulate ::runOnModule(Module &M) {
  bool Changed = false;
  EmulationFuns.clear();
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();

  // TODO: consider just an iteration over instructions

  // Process non-builtin functions.
  for (auto &F : M.getFunctionList()) {
    if (!isEmulationFunction(&F))
      runOnFunction(F);
  }
  Changed |= !ToErase.empty();
  for (auto *I : ToErase)
    I->eraseFromParent();
  ToErase.clear();

  // Delete unuse builtins or make used builtins internal.
  for (auto I = M.begin(); I != M.end();) {
    Function &F = *I++;
    if (isEmulationFunction(&F)) {
      Changed = true;
      if (F.use_empty())
        F.eraseFromParent();
      else
        F.setLinkage(GlobalValue::InternalLinkage);
    }
  }

  return Changed;
}

void GenXEmulate::runOnFunction(Function &F) {
  for (auto &BB : F.getBasicBlockList()) {
    for (auto I = BB.begin(); I != BB.end(); ++I) {

      Instruction *Inst = &*I;
      auto *NewVal = emulateInst(Inst);
      if (NewVal) {
        Inst->replaceAllUsesWith(NewVal);
        ToErase.push_back(Inst);
      }
    }
  }
  return;
}

Function *GenXEmulate::getEmulationFunction(Instruction *Inst) {
  unsigned Opcode = Inst->getOpcode();
  Type *Ty = Inst->getType();
  OpType OpAndType = std::make_pair(Opcode, Ty);

  // Check if this emulation function has been cached.
  auto Iter = EmulationFuns.find(OpAndType);
  if (Iter != EmulationFuns.end())
    return Iter->second;

  IGC_ASSERT(ST && "subtarget expected");
  StringRef EmuFnName = ST->getEmulateFunction(Inst);
  if (EmuFnName.empty())
    return nullptr;

  Module *M = Inst->getParent()->getParent()->getParent();
  for (auto &F : M->getFunctionList()) {
    if (!isEmulationFunction(&F))
      continue;
    if (F.getReturnType() != Inst->getType())
      continue;
    StringRef FnName = F.getName();
    if (FnName.contains(EmuFnName)) {
      EmulationFuns[OpAndType] = &F;
      return &F;
    }
  }

  return nullptr;
}

Value *GenXEmulate::emulateInst(Instruction *Inst) {
  Function *EmuFn = getEmulationFunction(Inst);
  if (EmuFn) {
    IGC_ASSERT(!isa<CallInst>(Inst) && "call emulation not supported yet");
    llvm::IRBuilder<> Builder(Inst);
    SmallVector<Value *, 8> Args(Inst->operands());
    return Builder.CreateCall(EmuFn, Args);
  }

  return nullptr;
}
