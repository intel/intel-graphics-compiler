/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Module.h"
#include "common/IGCIRBuilder.h"

#include <map>
namespace IGC {
// Attributes for each __builtin_IB_atomic function.
struct OCLAtomicAttrs {
  AtomicOp op;
  BufferType bufType;
};

// A pass that walks over call instructions and replaces all __builtin_IB_atomic calls
// with corresponding GenISA intrinsics.
//
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ResolveOCLAtomics : public llvm::InstVisitor<ResolveOCLAtomics> {
public:
  ResolveOCLAtomics() {}
  ~ResolveOCLAtomics() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "ResolveOCLAtomics"; }

  // Entry point of the pass.
  bool run(llvm::Module &M, CodeGenContext *pCtx);

  // Call instructions visitor.
  void visitCallInst(llvm::CallInst &callInst);

  static llvm::Instruction *CallAtomicSingleLane(AtomicOp AtomicType, llvm::Value *ptr, llvm::Value *data,
                                                 llvm::Instruction *pInsertBefore);

protected:
  CodeGenContext *m_CGCtx = nullptr;
  IGCLLVM::Module *m_pModule = nullptr;
  llvm::IGCIRBuilder<> *m_builder = nullptr;
  llvm::IntegerType *m_Int32Ty = nullptr;

  // A map that keeps attributes for each "__builtin_IB_atomic_*" function name.
  std::map<llvm::StringRef, OCLAtomicAttrs> m_AtomicDescMap;

  void initResolveOCLAtomics();

  /// @brief  Initialize the "atomic name <--> attributes" map.
  void initOCLAtomicsMap();

  /// @brief  Replace the "__builtin_IB_atomic_*" call with a call to GenISA atomic intrinsic.
  /// @param    callInst  call to "__builtin_IB_atomic_*" function.
  /// @param    op        atomic operation.
  /// @param    bufType   buffer type.
  void processOCLAtomic(llvm::CallInst &callInst, AtomicOp op, BufferType bufType);

  /// @brief  Generates a call to "GenISA_GetBufferPtr" intrinsic.
  /// @param    callInst   call instruction to "__builtin_IB_atomic_*" built-in function.
  /// @param    bufType    corresponding buffer type.
  /// @returns  call instruction to generated GenISA_GetBufferPtr.
  llvm::CallInst *genGetBufferPtr(llvm::CallInst &callInst, BufferType bufType);

  /// @brief  Replace the "__builtin_IB_get_local_lock" call with a pointer to a local memory variable.
  /// @param    callInst  call to "__builtin_IB_get_local_lock*" function.
  void processGetLocalLock(llvm::CallInst &callInst);

  /// @brief  Replace the "__builtin_IB_get_global_lock" call with a pointer to a global memory variable.
  /// @param    callInst  call to "__builtin_IB_get_global_lock*" function.
  void processGetGlobalLock(llvm::CallInst &callInst);

  /// @brief  Recursively search for kernel functions using local lock variable
  /// @param    value   currently analyzed function
  void findLockUsers(llvm::Value *V);

  /// @brief  Generates control flow which initializes local lock memory at the beginning of the kernel
  /// @param    function   kernel function using local lock variable
  void generateLockInitilization(llvm::Function *F);

  /// @brief  Initializes local lock variable in all related kernel functions
  void initilizeLocalLock();

  /// @brief  Initializes global lock variable in all related kernel functions
  void initializeGlobalLock();

  /// @brief  Stores the value of local value used for spinlock for i64 local atomics emulation.
  llvm::GlobalVariable *m_localLock = nullptr;

  /// @brief  Stores the value of global value used for spinlock for f64 global atomics emulation.
  llvm::GlobalVariable *m_globalLock = nullptr;

  /// @brief  Stores all the kernel functions using local lock variable
  std::unordered_set<llvm::Function *> m_localLockUsers;

  /// @brief  Indicates if the pass changed the processed function
  bool m_changed = false;
};

// Legacy Pass Manager wrapper.
class ResolveOCLAtomicsLPM : public llvm::ModulePass {
public:
  static char ID;

  ResolveOCLAtomicsLPM();
  ~ResolveOCLAtomicsLPM() {}

  llvm::StringRef getPassName() const override { return ResolveOCLAtomics::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<CodeGenContextWrapper>(); }

  bool runOnModule(llvm::Module &M) override {
    return ResolveOCLAtomics().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class ResolveOCLAtomicsNPM : public llvm::PassInfoMixin<ResolveOCLAtomicsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-resolve-atomics"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
