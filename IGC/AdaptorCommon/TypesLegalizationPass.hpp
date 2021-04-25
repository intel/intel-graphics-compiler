/*========================== begin_copyright_notice ============================

Copyright (c) 2015-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "../Compiler/IGCPassSupport.h"


class TypesLegalizationPass: public llvm::FunctionPass,public llvm::InstVisitor<TypesLegalizationPass>
{

public:

  TypesLegalizationPass();
  TypesLegalizationPass(bool legalizePhi, bool legalizeExtractValue, bool legalizeStore);
  ~TypesLegalizationPass()
  {}

  virtual llvm::StringRef getPassName() const override
  {
      return "Types Legalization Pass";
  }

  bool LegalizeTypes();
  virtual bool runOnFunction(llvm::Function &function) override;
  void visitStoreInst(llvm::StoreInst &I);
  void visitExtractValueInst(llvm::ExtractValueInst &I);
  void visitPHINode(llvm::PHINode &I);
  void ResolveStoreInst( llvm::StoreInst *st );
  void ResolvePhiNode( llvm::PHINode* phi );
  void ResolveExtractValue( llvm::ExtractValueInst* extractVal );
  void ResolveStoreInst( llvm::StoreInst *st, llvm::Type *ty,llvm::SmallVector<unsigned, 8> &index );
  llvm::Value* ResolveValue( llvm::Instruction *st,llvm::Value* arg,llvm::SmallVector<unsigned,8> &index );
  llvm::Value* CreateGEP( llvm::IRBuilder<> &builder,llvm::Value*  ptr,llvm::SmallVector<unsigned,8> &indices );
  llvm::AllocaInst* CreateAlloca( llvm::Instruction *phi );

  static char ID;
  llvm::SmallVector<llvm::StoreInst*,10> m_StoreInst;
  llvm::SmallVector<llvm::ExtractValueInst*,10> m_ExtractValueInst;
  llvm::SmallVector<llvm::PHINode*,10> m_PhiNodes;
  llvm::SmallVector<unsigned, 8> Indicies;

protected:
  bool m_LegalizePhi = true;
  bool m_LegalizeExtractValue = true;
  bool m_LegalizeStore = true;
};





