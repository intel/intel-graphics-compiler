/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_DEBUG_INFO_H
#define VC_UTILS_GENERAL_DEBUG_INFO_H

#include <llvm/IR/DebugInfoMetadata.h>

#include <llvm/ADT/StringRef.h>

namespace llvm {
class GlobalVariable;
class DbgDeclareInst;
class Instruction;
class Function;
class Module;
class Type;
} // namespace llvm

namespace vc {

// Utility class to construct new debug information nodes
// The interface is vaguely similar to llvm::DIBuilder, however it can
// be used on a Module that already contains DICompilation units, allowing
// for an addition or modification of an existing debug information.
class DIBuilder {
  llvm::Module &M;

  llvm::DIType *translateScalarTypeToDIType(llvm::Type &Ty) const;

  void registerNewGlobalVariable(llvm::DIGlobalVariableExpression *NewGV) const;

public:
  static bool checkIfModuleHasDebugInfo(const llvm::Module &M);
  static bool checkIfFunctionHasDebugInfo(const llvm::Function &F);

  static llvm::DbgDeclareInst *
  createDbgDeclareForLocalizedGlobal(llvm::Instruction &Address,
                                     const llvm::GlobalVariable &GV,
                                     llvm::Instruction &InsertPt);

  DIBuilder(llvm::Module &MIn) : M(MIn) {}
  llvm::DIType *translateTypeToDIType(llvm::Type &Ty) const;

  llvm::DICompileUnit *getDICompileUnit() const;

  llvm::DILocation *createLocationForFunctionScope(llvm::Function &Fn) const;

  llvm::DIGlobalVariableExpression *
  createGlobalVariableExpression(llvm::StringRef Name,
                                 llvm::StringRef LinkageName,
                                 llvm::DIType *Type) const;

  llvm::DbgDeclareInst *createDbgDeclare(llvm::Value &Address,
                                         llvm::DILocalVariable &LocalVar,
                                         llvm::DIExpression &Expr,
                                         llvm::DILocation &Loc,
                                         llvm::Instruction &InsertPt) const;

  llvm::DILocalVariable *
  createLocalVariable(llvm::DILocalScope *Scope, llvm::StringRef Name,
                      llvm::DIFile *File, unsigned LineNo, llvm::DIType *Type,
                      unsigned ArgNo, llvm::DINode::DIFlags Flags,
                      unsigned AlignInBits) const;
};

} // namespace vc

#endif // VC_UTILS_GENERAL_DEBUG_INFO_H
