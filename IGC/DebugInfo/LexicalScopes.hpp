/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

///////////////////////////////////////////////////////////////////////////////
// This file is based on llvm-3.4\include\llvm\CodeGen\LexicalScopes.h
///////////////////////////////////////////////////////////////////////////////

#pragma once

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/ValueHandle.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include <unordered_map>
#include <utility>

#include "Probe/Assertion.h"

namespace IGC {
class VISAModule;

//===----------------------------------------------------------------------===//
/// InsnRange - This is used to track range of instructions with identical
/// lexical scope.
///
typedef std::pair<const llvm::Instruction *, const llvm::Instruction *>
    InsnRange;

//===----------------------------------------------------------------------===//
/// LexicalScope - This class is used to track scope information.
///
class LexicalScope {
public:
  LexicalScope(LexicalScope *P, const llvm::DILocalScope *D,
               const llvm::DILocation *I, bool A)
      : Parent(P), Desc(D), InlinedAtLocation(I), AbstractScope(A),
        LastInsn(nullptr), FirstInsn(nullptr), DFSIn(0), DFSOut(0) {
    IGC_ASSERT_MESSAGE((!D || D->isResolved()), "Expected resolved node");
    IGC_ASSERT_MESSAGE((!I || I->isResolved()), "Expected resolved node");
    if (Parent)
      Parent->addChild(this);
  }

  virtual ~LexicalScope() {}

  // Accessors.
  LexicalScope *getParent() const { return Parent; }
  const llvm::MDNode *getDesc() const { return Desc; }
  const llvm::MDNode *getInlinedAt() const { return InlinedAtLocation; }
  const llvm::MDNode *getScopeNode() const { return Desc; }
  bool isAbstractScope() const { return AbstractScope; }
  llvm::SmallVectorImpl<LexicalScope *> &getChildren() { return Children; }
  llvm::SmallVectorImpl<InsnRange> &getRanges() { return Ranges; }

  /// addChild - Add a child scope.
  void addChild(LexicalScope *S) { Children.push_back(S); }

  /// openInsnRange - This scope covers instruction range starting from MI.
  void openInsnRange(const llvm::Instruction *MI) {
    if (!FirstInsn)
      FirstInsn = MI;

    if (Parent)
      Parent->openInsnRange(MI);
  }

  /// extendInsnRange - Extend the current instruction range covered by
  /// this scope.
  void extendInsnRange(const llvm::Instruction *MI) {
    IGC_ASSERT_MESSAGE(FirstInsn, "MI Range is not open!");
    LastInsn = MI;
    if (Parent)
      Parent->extendInsnRange(MI);
  }

  /// closeInsnRange - Create a range based on FirstInsn and LastInsn collected
  /// until now. This is used when a new scope is encountered while walking
  /// machine instructions.
  void closeInsnRange(LexicalScope *NewScope = NULL) {
    IGC_ASSERT_MESSAGE(LastInsn, "Last insn missing!");
    Ranges.push_back(InsnRange(FirstInsn, LastInsn));
    FirstInsn = NULL;
    LastInsn = NULL;
    // If Parent dominates NewScope then do not close Parent's instruction
    // range.
    if (Parent && (!NewScope || !Parent->dominates(NewScope))) {
      Parent->closeInsnRange(NewScope);
    }
  }

  /// dominates - Return true if current scope dominates given lexical scope.
  bool dominates(const LexicalScope *S) const {
    if (S == this)
      return true;
    if (DFSIn < S->getDFSIn() && DFSOut > S->getDFSOut())
      return true;
    return false;
  }

  // Depth First Search support to walk and manipulate LexicalScope hierarchy.
  unsigned getDFSOut() const { return DFSOut; }
  void setDFSOut(unsigned O) { DFSOut = O; }
  unsigned getDFSIn() const { return DFSIn; }
  void setDFSIn(unsigned I) { DFSIn = I; }

  /// dump - print lexical scope.
  void dump(unsigned Indent = 0) const;

private:
  LexicalScope *Parent;                      // Parent to this scope.
  const llvm::DILocalScope *Desc;            // Debug info descriptor.
  const llvm::DILocation *InlinedAtLocation; // Location at which this
  // scope is inlined.
  bool AbstractScope;                            // Abstract Scope
  llvm::SmallVector<LexicalScope *, 4> Children; // Scopes defined in scope.
  // Contents not owned.
  llvm::SmallVector<InsnRange, 4> Ranges;

  const llvm::Instruction *LastInsn;  // Last instruction of this scope.
  const llvm::Instruction *FirstInsn; // First instruction of this scope.
  unsigned DFSIn, DFSOut;             // In & Out Depth use to determine
                                      // scope nesting.
};

//===----------------------------------------------------------------------===//
/// LexicalScopes -  This class provides interface to collect and use lexical
/// scoping information from machine instruction.
///
class LexicalScopes {
public:
  LexicalScopes() : VisaM(NULL), CurrentFnLexicalScope(NULL) {}
  virtual ~LexicalScopes();

  /// initialize - Scan machine function and constuct lexical scope nest.
  virtual void initialize(const VISAModule *M);

  /// releaseMemory - release memory.
  virtual void releaseMemory();

  /// empty - Return true if there is any lexical scope information available.
  bool empty() { return CurrentFnLexicalScope == NULL; }

  /// isCurrentFunctionScope - Return true if given lexical scope represents
  /// current function.
  bool isCurrentFunctionScope(const LexicalScope *LS) {
    return LS == CurrentFnLexicalScope;
  }

  /// getCurrentFunctionScope - Return lexical scope for the current function.
  LexicalScope *getCurrentFunctionScope() const {
    return CurrentFnLexicalScope;
  }

  /// findLexicalScope - Find lexical scope, either regular or inlined, for the
  /// given DebugLoc. Return NULL if not found.
  LexicalScope *findLexicalScope(const llvm::DILocation *DL);

  /// getAbstractScopesList - Return a reference to list of abstract scopes.
  llvm::ArrayRef<LexicalScope *> getAbstractScopesList() const {
    return AbstractScopesList;
  }

  /// findAbstractScope - Find an abstract scope or return NULL.
  LexicalScope *findAbstractScope(const llvm::DILocalScope *N) {
    auto I = AbstractScopeMap.find(N);
    return I != AbstractScopeMap.end() ? &I->second : nullptr;
  }

  /// findInlinedScope - Find an inlined scope for the given scope/inlined-at.
  LexicalScope *findInlinedScope(const llvm::DILocalScope *N,
                                 const llvm::DILocation *IA) {
    auto I = InlinedLexicalScopeMap.find(std::make_pair(N, IA));
    return I != InlinedLexicalScopeMap.end() ? &I->second : nullptr;
  }

  /// findLexicalScope - Find regular lexical scope or return null.
  LexicalScope *findLexicalScope(const llvm::DILocalScope *N) {
    auto I = LexicalScopeMap.find(N);
    return I != LexicalScopeMap.end() ? &I->second : nullptr;
  }

  /// getOrCreateAbstractScope - Find or create an abstract lexical scope.
  LexicalScope *getOrCreateAbstractScope(const llvm::DILocalScope *Scope);

  /// dump - Print data structures to dbgs().
  void dump();

private:
  /// getOrCreateLexicalScope - Find lexical scope for the given Scope/IA. If
  /// not available then create new lexical scope.
  LexicalScope *getOrCreateLexicalScope(const llvm::DILocalScope *Scope,
                                        const llvm::DILocation *IA = nullptr);
  LexicalScope *getOrCreateLexicalScope(const llvm::DILocation *DL) {
    return DL ? getOrCreateLexicalScope(DL->getScope(), DL->getInlinedAt())
              : nullptr;
  }

  /// getOrCreateRegularScope - Find or create a regular lexical scope.
  LexicalScope *getOrCreateRegularScope(const llvm::DILocalScope *Scope);

  /// getOrCreateInlinedScope - Find or create an inlined lexical scope.
  LexicalScope *getOrCreateInlinedScope(const llvm::DILocalScope *Scope,
                                        const llvm::DILocation *InlinedAt);

  /// extractLexicalScopes - Extract instruction ranges for each lexical scopes
  /// for the given machine function.
  void extractLexicalScopes(
      llvm::SmallVectorImpl<InsnRange> &MIRanges,
      llvm::DenseMap<const llvm::Instruction *, LexicalScope *> &M);
  void constructScopeNest(LexicalScope *Scope);
  void assignInstructionRanges(
      llvm::SmallVectorImpl<InsnRange> &MIRanges,
      llvm::DenseMap<const llvm::Instruction *, LexicalScope *> &M);

private:
  const VISAModule *VisaM;

  /// LexicalScopeMap - Tracks the scopes in the current function.
  // Use an unordered_map to ensure value pointer validity over insertion.
  std::unordered_map<const llvm::DILocalScope *, LexicalScope> LexicalScopeMap;

  /// InlinedLexicalScopeMap - Tracks inlined function scopes in current
  /// function.
  std::unordered_map<
      std::pair<const llvm::DILocalScope *, const llvm::DILocation *>,
      LexicalScope,
      llvm::pair_hash<const llvm::DILocalScope *, const llvm::DILocation *>>
      InlinedLexicalScopeMap;

  /// AbstractScopeMap - These scopes are  not included LexicalScopeMap.
  // Use an unordered_map to ensure value pointer validity over insertion.
  std::unordered_map<const llvm::DILocalScope *, LexicalScope> AbstractScopeMap;

  /// AbstractScopesList - Tracks abstract scopes constructed while processing
  /// a function.
  llvm::SmallVector<LexicalScope *, 4> AbstractScopesList;

  /// CurrentFnLexicalScope - Top level scope for the current function.
  ///
  LexicalScope *CurrentFnLexicalScope;
};

} // namespace IGC
