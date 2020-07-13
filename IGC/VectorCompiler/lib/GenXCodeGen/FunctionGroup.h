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
/// FunctionGroup
/// -------------
///
/// FunctionGroup is a generic mechanism for maintaining a group of Functions.
///
/// FunctionGroupAnalysis is a Module analysis that maintains all the
/// FunctionGroups in the Module. It is up to some other pass to use
/// FunctionGroupAnalysis to create and populate the FunctionGroups, and thus
/// attach some semantics to what a FunctionGroup represents.
///
/// FunctionGroupPass is a type of pass (with associated pass manager) that
/// runs a pass instance per FunctionGroup.
///
/// This file is currently in lib/Target/GenX, as that is the only place it
/// is used. It could be moved somewhere more general.
///
//===----------------------------------------------------------------------===//
#ifndef FUNCTIONGROUP_H
#define FUNCTIONGROUP_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/Pass.h"

#include <list>

namespace llvm {

class FunctionGroupAnalysis;
class LLVMContext;
class PMStack;

//----------------------------------------------------------------------
// FunctionGroup : a group of Functions
//
class FunctionGroup {
  FunctionGroupAnalysis *FGA;
  // Vector of Functions in the FunctionGroup. Element 0 is the head.
  // Elements are asserting value handles, so we spot when a Function
  // in the group gets destroyed too early.
  SmallVector<AssertingVH<Function>, 8> Functions;

public:
  FunctionGroup(FunctionGroupAnalysis *FGA) : FGA(FGA) {}
  FunctionGroupAnalysis *getParent() { return FGA; }
  // push_back : push a Function into the group. The first time this is done,
  // the Function is the head Function.
  void push_back(Function *F) { Functions.push_back(AssertingVH<Function>(F)); }
  // iterator and forwarders. The iterator iterates over the Functions in the
  // group, starting with the head Function.
  AssertingVH<Function> &at(unsigned i) { return Functions[i]; }
  typedef SmallVectorImpl<AssertingVH<Function>>::iterator iterator;
  iterator begin() { return Functions.begin(); }
  iterator end() { return Functions.end(); }
  typedef SmallVectorImpl<AssertingVH<Function>>::reverse_iterator
      reverse_iterator;
  reverse_iterator rbegin() { return Functions.rbegin(); }
  reverse_iterator rend() { return Functions.rend(); }
  size_t size() { return Functions.size(); }
  // accessors
  Function *getHead() {
    assert(size());
    return *begin();
  }
  StringRef getName() { return getHead()->getName(); }
  LLVMContext &getContext() { return getHead()->getContext(); }
  Module *getModule() { return getHead()->getParent(); }
};

//----------------------------------------------------------------------
// FunctionGroupAnalysis - a Module analysis that maintains all the
// FunctionGroups in the Module. It is up to some other pass to use
// FunctionGroupAnalysis to create the FunctionGroups and then populate them.
//
class FunctionGroupAnalysis : public ModulePass {
public:
  // FunctionGroup types:
  // * GROUP - GENX_MAIN kernel and its underlying callgraph
  // * SUBGROUP - GENX_STACKCALL function and its underlying callgraph including
  //    subroutines only
  // Groups are necessary to perform cloning of subroutines
  // called from different kernels and/or stack functions
  enum class FGType { GROUP, SUBGROUP, MAX };
  const FGType TypesToProcess[static_cast<size_t>(FGType::MAX)] = {
      FGType::GROUP, FGType::SUBGROUP};

private:
  Module *M;
  SmallVector<FunctionGroup *, 8> Groups;

  // storage for FunctionGroups that aren't of type GROUP,
  // i.e. not necessarily GENX_MAIN headed
  // TODO: mb increase 8 as there can be many stack funcs hence may subgroups
  SmallVector<FunctionGroup *, 8> NonMainGroups;

  class FGMap {
    using ElementType = std::map<Function *, FunctionGroup *>;
    ElementType data[static_cast<size_t>(FGType::MAX)];

  public:
    ElementType &operator[](FGType type) {
      auto index = static_cast<size_t>(type);
      return data[index];
    }
  };

  FGMap GroupMap;
  std::map<Function *, bool> Visited;
  using CallGraph = std::map<Function *, std::list<Function *>>;

public:
  static char ID;
  explicit FunctionGroupAnalysis() : ModulePass(ID) {}
  ~FunctionGroupAnalysis() { clear(); }
  StringRef getPassName() const override { return "function group analysis"; }
  // runOnModule : does almost nothing
  bool runOnModule(Module &ArgM) override {
    clear();
    M = &ArgM;
    return false;
  }
  void releaseMemory() override {
    clear();
  }
  // getModule : get the Module that this FunctionGroupAnalysis is for
  Module *getModule() { return M; }
  // clear : clear out the FunctionGroupAnalysis
  void clear();
  // getGroup : get the FunctionGroup containing Function F, else 0
  FunctionGroup *getGroup(Function *F, FGType Type);
  FunctionGroup *getGroup(Function *F);
  FunctionGroup *getSubGroup(Function *F);
  // getGroupForHead : get the FunctionGroup for which Function F is the
  // head, else 0
  FunctionGroup *getGroupForHead(Function *F);
  // replaceFunction : replace a Function in a FunctionGroup
  void replaceFunction(Function *OldF, Function *NewF);
  // iterator for FunctionGroups in the analysis
  typedef SmallVectorImpl<FunctionGroup *>::iterator iterator;
  iterator begin() { return iterator(Groups.begin()); }
  iterator end() { return iterator(Groups.end()); }
  size_t size() { return Groups.size(); }
  // addToFunctionGroup : add Function F to FunctionGroup FG
  // Using this (rather than calling push_back directly on the FunctionGroup)
  // means that the mapping from F to FG will be created, and getGroup() will
  // work for this Function.
  void addToFunctionGroup(FunctionGroup *FG, Function *F, FGType Type);
  // createFunctionGroup : create new FunctionGroup for which F is the head
  FunctionGroup *createFunctionGroup(Function *F, FGType Type);
  bool buildGroup(CallGraph &callees, Function *F,
                  FunctionGroup *curGr = nullptr, FGType Type = FGType::GROUP);

  void clearVisited() { Visited.clear(); }
};

ModulePass *createFunctionGroupAnalysisPass();
void initializeFunctionGroupAnalysisPass(PassRegistry &);

inline raw_ostream &operator<<(raw_ostream &OS,
                               const FunctionGroupAnalysis::FGType &T) {
  switch (T) {
  case FunctionGroupAnalysis::FGType::GROUP:
    OS << "Group";
    break;
  case FunctionGroupAnalysis::FGType::SUBGROUP:
    OS << "Subgroup";
    break;
  default:
    llvm_unreachable("Invalid FG type");
    break;
  }
  return OS;
}

//----------------------------------------------------------------------
// FunctionGroupPass - a type of pass (with associated pass manager) that
// runs a pass instance per FunctionGroup.
//
class FunctionGroupPass : public Pass {
public:
  static constexpr unsigned PassType = PT_PassManager + 1;

  explicit FunctionGroupPass(char &pid) : Pass(static_cast<PassKind>(PassType), pid) {}

  // createPrinterPass - Get a pass that prints the Module
  // corresponding to a FunctionGroupAnalysis.
  Pass *createPrinterPass(raw_ostream &O,
                          const std::string &Banner) const override;

  using llvm::Pass::doFinalization;
  using llvm::Pass::doInitialization;

  // doInitialization - This method is called before the FunctionGroups of the
  // program have been processed, allowing the pass to do initialization as
  // necessary.
  virtual bool doInitialization(FunctionGroupAnalysis &FGA) { return false; }

  // runOnFunctionGroup - This method should be implemented by the subclass to
  // perform whatever action is necessary for the specified FunctionGroup.
  //
  virtual bool runOnFunctionGroup(FunctionGroup &FG) = 0;

  // doFinalization - This method is called after the FunctionGroups of the
  // program have been processed, allowing the pass to do final cleanup as
  // necessary.
  virtual bool doFinalization(FunctionGroupAnalysis &FGA) { return false; }

  // Assign pass manager to manager this pass
  void assignPassManager(PMStack &PMS, PassManagerType PMT) override;

  //  Return what kind of Pass Manager can manage this pass.
  PassManagerType getPotentialPassManagerType() const override {
    return PMT_ModulePassManager;
  }

  // getAnalysisUsage - For this class, we declare that we require and
  // preserve the FunctionGroupAnalysis.
  // If the derived class implements this method, it should
  // always explicitly call the implementation here.
  void getAnalysisUsage(AnalysisUsage &Info) const override;
};

//----------------------------------------------------------------------
// DominatorTreeGroupWrapperPass : Analysis pass which computes a DominatorTree
// per Function in the FunctionGroup.
class DominatorTree;

class DominatorTreeGroupWrapperPass : public FunctionGroupPass {
  std::map<Function *, DominatorTree *> DTs;

public:
  static char ID;

  DominatorTreeGroupWrapperPass() : FunctionGroupPass(ID) {}
  ~DominatorTreeGroupWrapperPass() { releaseMemory(); }

  DominatorTree *getDomTree(Function *F) { return DTs[F]; }
  const DominatorTree &getDomTree();

  bool runOnFunctionGroup(FunctionGroup &FG) override;

  void verifyAnalysis() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    FunctionGroupPass::getAnalysisUsage(AU);
    AU.setPreservesAll();
  }

  void releaseMemory() override;

  void print(raw_ostream &OS, const Module *M = nullptr) const override;
};
void initializeDominatorTreeGroupWrapperPassPass(PassRegistry &);

} // end namespace llvm
#endif // ndef FUNCTIONGROUP_H
