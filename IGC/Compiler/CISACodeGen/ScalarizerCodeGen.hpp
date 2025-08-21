/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
class ScalarizerCodeGen : public llvm::FunctionPass, public llvm::InstVisitor<ScalarizerCodeGen> {
public:
  static char ID;

  ScalarizerCodeGen();

  virtual llvm::StringRef getPassName() const { return "Scalarizer in Codegen"; }

  virtual bool runOnFunction(llvm::Function &F);
  void visitBinaryOperator(llvm::BinaryOperator &I);
  void visitCastInst(llvm::CastInst &I);
  void visitFNeg(llvm::UnaryOperator &I);

private:
  llvm::IRBuilder<> *m_builder = nullptr;
};
} // namespace IGC
