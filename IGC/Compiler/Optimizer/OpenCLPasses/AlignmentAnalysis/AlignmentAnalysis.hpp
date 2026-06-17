/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Support/Alignment.h"

namespace IGC {

/// @brief  This pass correctly sets the alignment of all loads
///         and stores according to OpenCL rules.
///         It perform a data-flow analysis to keep track of the highest power
///         of 2 that divides a value.
///         This tracking is performed for integers and for pointers, and is
///         based on the assumption that global variables and kernel arguments
///         are always aligned on their data type.
///         The result is an underapproximation of the actual alignment, so it
///         is always safe.
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class AlignmentAnalysis : public llvm::InstVisitor<AlignmentAnalysis, llvm::Align> {
public:
  AlignmentAnalysis() {}
  ~AlignmentAnalysis() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "AlignmentAnalysisPass"; }

  /// @brief  Main entry point.
  bool run(llvm::Function &F);

  // @ brief Instruction visitors
  llvm::Align visitInstruction(llvm::Instruction &I);
  llvm::Align visitAllocaInst(llvm::AllocaInst &I);
  llvm::Align visitIntToPtrInst(llvm::IntToPtrInst &I);
  llvm::Align visitPtrToIntInst(llvm::PtrToIntInst &I);
  llvm::Align visitSelectInst(llvm::SelectInst &I);
  llvm::Align visitGetElementPtrInst(llvm::GetElementPtrInst &I);
  llvm::Align visitPHINode(llvm::PHINode &I);
  llvm::Align visitBitCastInst(llvm::BitCastInst &I);
  llvm::Align visitAdd(llvm::BinaryOperator &I);
  llvm::Align visitMul(llvm::BinaryOperator &I);
  llvm::Align visitShl(llvm::BinaryOperator &I);
  llvm::Align visitAnd(llvm::BinaryOperator &I);
  llvm::Align visitTruncInst(llvm::TruncInst &I);
  llvm::Align visitZExtInst(llvm::ZExtInst &I);
  llvm::Align visitSExtInst(llvm::SExtInst &I);
  llvm::Align visitCallInst(llvm::CallInst &I);

  bool SetInstAlignment(llvm::Instruction &I);
  bool SetInstAlignment(llvm::LoadInst &I);
  bool SetInstAlignment(llvm::StoreInst &I);
  bool SetInstAlignment(llvm::MemSetInst &I);
  bool SetInstAlignment(llvm::MemCpyInst &I);
  bool SetInstAlignment(llvm::MemMoveInst &I);

protected:
  // Check if the function has OpenCL metadata that specifies the alignment of
  // its arguments. If it does, set the LLVM alignment attribute of the
  // arguments accordingly. This is helpful for passes like InferAlignment.
  void setArgumentAlignmentBasedOnOptionalMetadata(llvm::Function &F);

  /// @breif Evaluates the alignment of I based on its operands.
  ///        For Load and Store instructions, also sets the alignment
  ///        of the operation itself.
  /// @param I The instruction to process
  bool processInstruction(llvm::Instruction *I);

  /// @brief Returns the alignment for V, if it is known.
  ///        Otherwise, returns the maximum alignment.
  /// @param V the value the alignment of which we're interested in
  llvm::Align getAlignValue(llvm::Value *V) const;

  /// @brief Returns the alignment of a constant integer.
  ///        This is normally 1 << ctz(C) (the highest power of 2 that divides C),
  ///        except when C is 0, when it is the max alignment
  llvm::Align getConstantAlignment(uint64_t C) const;

  /// @brief This map stores the known alignment of every value.
  llvm::MapVector<llvm::Value *, llvm::Align> m_alignmentMap;

  const llvm::DataLayout *m_DL = nullptr;
};

// Legacy Pass Manager wrapper.
class AlignmentAnalysisLPM : public llvm::FunctionPass {
public:
  static char ID;

  AlignmentAnalysisLPM();
  ~AlignmentAnalysisLPM() {}

  llvm::StringRef getPassName() const override { return AlignmentAnalysis::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

  bool runOnFunction(llvm::Function &F) override { return AlignmentAnalysis().run(F); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined functions (the pass
// has no analysis dependencies) so that it is added directly to the module pass manager and gets the
// same per-pass IR dump as the legacy pass. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class AlignmentAnalysisNPM : public llvm::PassInfoMixin<AlignmentAnalysisNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-fix-alignment"; }
  static llvm::StringRef getPassName() { return "AlignmentAnalysisPass"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
