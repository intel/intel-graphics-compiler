/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

#include <unordered_map>

namespace IGC {
enum Accuracy { HIGH_ACCURACY, LOW_ACCURACY, ENHANCED_PERFORMANCE, CORRECTLY_ROUNDED };

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class AccuracyDecoratedCallsBiFResolution : public llvm::InstVisitor<AccuracyDecoratedCallsBiFResolution> {
public:
  // Accuracy --> Builtin (3 entries in each AccurateBuiltins)
  typedef std::unordered_map<Accuracy, std::string> AccurateBuiltins;

  AccuracyDecoratedCallsBiFResolution() {}
  ~AccuracyDecoratedCallsBiFResolution() {}

  static llvm::StringRef getPassName() { return "AccuracyDecoratedCallsBiFResolution"; }

  void initNameToBuiltinMap();

  void visitCallInst(llvm::CallInst &callInst);
  void visitBinaryOperator(llvm::BinaryOperator &inst);

  bool run(llvm::Module &M, CodeGenContext *pCtx);

private:
  bool m_changed = false;
  llvm::Module *m_Module = nullptr;
  CodeGenContext *m_pCtx = nullptr;
  // m_nameToBuiltin["_Z15__spirv_ocl_sinf"][ENHANCED_PERFORMANCE] --> "__ocl_svml_sinf_ep"
  std::unordered_map<std::string, AccurateBuiltins> m_nameToBuiltin{};

  llvm::Function *getOrInsertNewFunc(const llvm::StringRef oldFuncName, llvm::Type *funcType,
                                     const llvm::ArrayRef<llvm::Value *> args, const llvm::AttributeList attributes,
                                     llvm::CallingConv::ID callingConv, const llvm::StringRef maxErrorStr,
                                     const llvm::Instruction *currInst);
  std::string getFunctionName(const llvm::StringRef oldFuncName, Accuracy accuracy,
                              const llvm::Instruction *currInst) const;
  Accuracy getAccuracy(double maxError, double cutOff, const llvm::Instruction *currInst) const;
};

// Legacy Pass Manager wrapper.
class AccuracyDecoratedCallsBiFResolutionLPM : public llvm::ModulePass {
public:
  static char ID;

  AccuracyDecoratedCallsBiFResolutionLPM();
  ~AccuracyDecoratedCallsBiFResolutionLPM() {}

  llvm::StringRef getPassName() const override { return AccuracyDecoratedCallsBiFResolution::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<CodeGenContextWrapper>(); }

  bool runOnModule(llvm::Module &M) override {
    return AccuracyDecoratedCallsBiFResolution().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class AccuracyDecoratedCallsBiFResolutionNPM : public llvm::PassInfoMixin<AccuracyDecoratedCallsBiFResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-accuracy-decorated-calls-bif-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
