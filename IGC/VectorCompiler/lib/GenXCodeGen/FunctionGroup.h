/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
#ifndef LIB_GENXCODEGEN_FUNCTIONGROUP_H
#define LIB_GENXCODEGEN_FUNCTIONGROUP_H

#include "vc/Utils/GenX/KernelInfo.h"

#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/Pass.h>

#include "Probe/Assertion.h"

#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace llvm {

class FunctionGroupAnalysis;
class LLVMContext;
class PMStack;

ModulePass *createGenXGroupPrinterPass(raw_ostream &O,
                                       const std::string &Banner);

namespace genx {
namespace fg {
inline bool isGroupHead(const Function &F) { return vc::isKernel(&F); }
inline bool isSubGroupHead(const Function &F) {
  return vc::requiresStackCall(F);
}
inline bool isHead(const Function &F) {
  return isGroupHead(F) || isSubGroupHead(F);
}
} // namespace fg
} // namespace genx

//----------------------------------------------------------------------
// FunctionGroup : a group of Functions
//
class FunctionGroup {
  FunctionGroupAnalysis *FGA = nullptr;
  // Vector of Functions in the FunctionGroup. Element 0 is the head.
  // Elements are asserting value handles, so we spot when a Function
  // in the group gets destroyed too early.
  SmallVector<AssertingVH<Function>, 8> Functions;
  // FunctionGroup can call the head function of another FunctionGroups with
  // SUBGROUP type.
  SetVector<const FunctionGroup *> Subgroups;

public:
  FunctionGroup(FunctionGroupAnalysis *FGA) : FGA(FGA) {}
  FunctionGroupAnalysis *getParent() const { return FGA; }
  // push_back : push a Function into the group. The first time this is done,
  // the Function is the head Function.
  void push_back(Function *F) { Functions.push_back(AssertingVH<Function>(F)); }
  // iterator and forwarders. The iterator iterates over the Functions in the
  // group, starting with the head Function.
  AssertingVH<Function> &at(unsigned i) { return Functions[i]; }
  typedef SmallVectorImpl<AssertingVH<Function>>::iterator iterator;
  typedef SmallVectorImpl<AssertingVH<Function>>::const_iterator const_iterator;
  iterator begin() { return Functions.begin(); }
  iterator end() { return Functions.end(); }
  const_iterator begin() const { return Functions.begin(); }
  const_iterator end() const { return Functions.end(); }
  typedef SmallVectorImpl<AssertingVH<Function>>::reverse_iterator
      reverse_iterator;
  reverse_iterator rbegin() { return Functions.rbegin(); }
  reverse_iterator rend() { return Functions.rend(); }
  size_t size() const { return Functions.size(); }
  // accessors
  Function *getHead() const {
    IGC_ASSERT(size());
    return *begin();
  }
  StringRef getName() const { return getHead()->getName(); }
  LLVMContext &getContext() const { return getHead()->getContext(); }
  Module *getModule() const { return getHead()->getParent(); }
  void addSubgroup(FunctionGroup *FG) {
    IGC_ASSERT(FG);
    IGC_ASSERT(FG->getHead());
    IGC_ASSERT_MESSAGE(genx::fg::isSubGroupHead(*FG->getHead()),
                       "Provided function group has incorrect type");
    Subgroups.insert(FG);
  }
  using subgroup_iterator = decltype(Subgroups)::iterator;
  using const_subgroup_iterator = decltype(Subgroups)::const_iterator;
  subgroup_iterator begin_subgroup() { return Subgroups.begin(); }
  subgroup_iterator end_subgroup() { return Subgroups.end(); }
  const_subgroup_iterator begin_subgroup() const { return Subgroups.begin(); }
  const_subgroup_iterator end_subgroup() const { return Subgroups.end(); }
  iterator_range<subgroup_iterator> subgroups() {
    return make_range(begin_subgroup(), end_subgroup());
  }
  iterator_range<const_subgroup_iterator> subgroups() const {
    return make_range(begin_subgroup(), end_subgroup());
  }

  bool verify() const;

  void print(raw_ostream &OS) const;
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  void dump() const;
#endif // if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
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
  Module *M = nullptr;
  SmallVector<std::unique_ptr<FunctionGroup>, 8> Groups;

  // storage for FunctionGroups that aren't of type GROUP,
  // i.e. not necessarily GENX_MAIN headed
  // TODO: mb increase 8 as there can be many stack funcs hence may subgroups
  SmallVector<std::unique_ptr<FunctionGroup>, 8> NonMainGroups;

  class FGMap {
    using ElementType = std::map<const Function *, FunctionGroup *>;
    std::array<ElementType, static_cast<size_t>(FGType::MAX)> data = {};
  public:
    ElementType &operator[](FGType type) {
      auto index = static_cast<size_t>(type);
      IGC_ASSERT(index < data.size());
      return data[index];
    }
    const ElementType &operator[](FGType type) const {
      auto index = static_cast<size_t>(type);
      IGC_ASSERT(index < data.size());
      return data[index];
    }
  };

  FGMap GroupMap;

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
  FunctionGroup *getGroup(const Function *F, FGType Type) const;
  FunctionGroup *getGroup(const Function *F) const;
  FunctionGroup *getSubGroup(const Function *F) const;
  // get group or subgroup depending on where the function is.
  FunctionGroup *getAnyGroup(const Function *F) const;
  // getGroupForHead : get the FunctionGroup for which Function F is the
  // head, else 0
  FunctionGroup *getGroupForHead(const Function *F) const;
  // replaceFunction : replace a Function in a FunctionGroup
  void replaceFunction(Function *OldF, Function *NewF);
  // iterator for FunctionGroups in the analysis
  typedef SmallVectorImpl<FunctionGroup *>::iterator iterator;
  typedef SmallVectorImpl<FunctionGroup *>::const_iterator const_iterator;
  using all_iterator = concat_iterator<FunctionGroup *, iterator, iterator>;
  using const_all_iterator =
      concat_iterator<FunctionGroup *const, const_iterator, const_iterator>;
  iterator begin() { return iterator(Groups.begin()); }
  iterator end() { return iterator(Groups.end()); }
  const_iterator begin() const { return const_iterator(Groups.begin()); }
  const_iterator end() const { return const_iterator(Groups.end()); }

  iterator subgroup_begin() { return iterator(NonMainGroups.begin()); }
  iterator subgroup_end() { return iterator(NonMainGroups.end()); }
  const_iterator subgroup_begin() const {
    return const_iterator(NonMainGroups.begin());
  }
  const_iterator subgroup_end() const {
    return const_iterator(NonMainGroups.end());
  }
  iterator_range<iterator> subgroups() {
    return make_range(subgroup_begin(), subgroup_end());
  }
  iterator_range<const_iterator> subgroups() const {
    return make_range(subgroup_begin(), subgroup_end());
  }

  iterator_range<all_iterator> AllGroups() {
    return concat<FunctionGroup *>(
        make_range(begin(), end()),
        make_range(subgroup_begin(), subgroup_end()));
  }
  iterator_range<const_all_iterator> AllGroups() const {
    return concat<FunctionGroup *const>(
        make_range(begin(), end()),
        make_range(subgroup_begin(), subgroup_end()));
  }

  size_t size() const { return Groups.size(); }
  bool legalizeGroups();
  // addToFunctionGroup : add Function F to FunctionGroup FG
  // Using this (rather than calling push_back directly on the FunctionGroup)
  // means that the mapping from F to FG will be created, and getGroup() will
  // work for this Function.
  void addToFunctionGroup(FunctionGroup *FG, Function *F, FGType Type);
  // createFunctionGroup : create new FunctionGroup for which F is the head
  FunctionGroup *createFunctionGroup(Function *F, FGType Type);
  using CallGraphTy = std::unordered_map<Function *, std::vector<Function *>>;
  void buildGroup(const CallGraphTy &CG, Function *Head,
                  FGType Type = FGType::GROUP);
  void buildGroups();

  bool verify() const;

  void print(raw_ostream &OS, const Module *) const override;
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  void dump() const;
#endif // if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
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
    IGC_ASSERT_EXIT_MESSAGE(0, "Invalid FG type");
    break;
  }
  return OS;
}

template <typename FGPassImpl> struct IDMixin { inline static char ID = 0; };

// FunctionGroupWrapperPass - a type of pass that
// runs a pass instance per FunctionGroup, and for each FunctionGroup data holds
// separately
struct FunctionGroupWrapperMapComparator {
  bool operator()(const FunctionGroup *Lhs, const FunctionGroup *Rhs) const {
    return Lhs->getName() < Rhs->getName();
  }
};
template <typename FGPassImpl>
class FunctionGroupWrapperPass : public IDMixin<FGPassImpl>, public ModulePass {
  using StoreStruct =
      std::map<FunctionGroup *, FGPassImpl, FunctionGroupWrapperMapComparator>;
  StoreStruct Passes;
  std::function<FGPassImpl &(FunctionGroup *, StoreStruct &)>
      createPassImplForFunctionGroup;

public:
  using IDMixin<FGPassImpl>::ID;

  // NOTE: arguments are copied and used in construction during runOnModule, so
  // MAKE SURE that all arguments are alive
  template <typename... FGPassArgs>
  explicit FunctionGroupWrapperPass(FGPassArgs... FGArgs)
      : ModulePass(ID),
        createPassImplForFunctionGroup(
            [FGArgs...](FunctionGroup *FG,
                        StoreStruct &Passes) -> FGPassImpl & {
              [[maybe_unused]] bool isEmplaced =
                  Passes.try_emplace(FG, FGArgs...).second;
              IGC_ASSERT_MESSAGE(isEmplaced == true, "PassImpl not created");
              return Passes.at(FG);
            }) {}
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<FunctionGroupAnalysis>();
    AU.addPreserved<FunctionGroupAnalysis>();
    FGPassImpl::getAnalysisUsage(AU);
  }
  StringRef getPassName() const override { return FGPassImpl::getPassName(); }

  // need to set up Passes that collect any data
  void releaseMemory() override {
    for (auto &[FG, PassImpl] : Passes)
      PassImpl.releaseMemory();
    Passes.clear();
  }
  void verifyAnalysis() const override {
    for (auto &[FG, PassImpl] : Passes)
      PassImpl.verifyAnalysis();
  }

  void print(raw_ostream &OS, const Module *M) const override {
    auto PassName = FGPassImpl::getPassName();
    const auto *PassInfo = Pass::lookupPassInfo(getPassID());
    if (PassInfo)
      PassName = PassInfo->getPassArgument();

    for (auto &[FG, PassImpl] : Passes) {
      OS << "Dump of <" << PassName << ">"
         << " for FunctionGroup: " << FG->getName() << " --start\n";
      PassImpl.getAsFGPassImplInterface().print(OS, FG);
      OS << "Dump of <" << PassName << ">"
         << " for FunctionGroup: " << FG->getName() << " --end\n";
      OS << "\n";
    }
  }
  // createPrinterPass : get a pass to print the IR, together with the GenX
  // specific analyses
  Pass *createPrinterPass(raw_ostream &O,
                          const std::string &Banner) const override {
    return createGenXGroupPrinterPass(O, Banner);
  }
  bool runOnModule(Module &M) override {
    bool Changed = false;
    FunctionGroupAnalysis &FGA =
        this->template getAnalysis<FunctionGroupAnalysis>();
    for (auto *currentFG : FGA.AllGroups()) {
      FGPassImpl &CurrentPass =
          createPassImplForFunctionGroup(currentFG, Passes);
      CurrentPass.Parent = this;
      CurrentPass.AnalyzedFG = currentFG;
      Changed |= CurrentPass.runOnFunctionGroup(*currentFG);
    }
    return Changed;
  }
  FGPassImpl &getFGPassImpl(FunctionGroup *FG) {
    IGC_ASSERT_MESSAGE(FG, "Nullptr input in getFGPassImpl");
    IGC_ASSERT_MESSAGE(
        Passes.count(FG) == 1,
        "Wrapper does not have PassImpl, associated with this FunctionGroup");
    return Passes[FG];
  }
};

struct FGAnalysisGetter {
  ModulePass *Parent = nullptr;
  FunctionGroup *AnalyzedFG = nullptr;
  template <typename AnalysisPass>
  typename std::enable_if_t<std::is_base_of_v<FGAnalysisGetter, AnalysisPass>,
                            AnalysisPass &>
  getAnalysis() const {
    return Parent->getAnalysis<FunctionGroupWrapperPass<AnalysisPass>>()
        .getFGPassImpl(AnalyzedFG);
  }
  template <typename AnalysisPass>
  typename std::enable_if_t<!std::is_base_of_v<FGAnalysisGetter, AnalysisPass>,
                            AnalysisPass &>
  getAnalysis() const {
    return Parent->getAnalysis<AnalysisPass>();
  }

  template <typename AnalysisPass>
  typename std::enable_if_t<std::is_base_of_v<FGAnalysisGetter, AnalysisPass>,
                            AnalysisPass *>
  getAnalysisIfAvailable() const {
    using WrapperT = FunctionGroupWrapperPass<AnalysisPass>;
    WrapperT *WrapperPtr = Parent->getAnalysisIfAvailable<WrapperT>();
    if (WrapperPtr)
      return &(WrapperPtr->getFGPassImpl(AnalyzedFG));
    return nullptr;
  }
  template <typename AnalysisPass>
  typename std::enable_if_t<!std::is_base_of_v<FGAnalysisGetter, AnalysisPass>,
                            AnalysisPass *>
  getAnalysisIfAvailable() const {
    return Parent->getAnalysisIfAvailable<AnalysisPass>();
  }
};

struct FGPassImplInterface : public FGAnalysisGetter {
  virtual void releaseMemory() {}
  virtual void print(raw_ostream &OS, const FunctionGroup *FG) const {}
  virtual bool runOnFunctionGroup(FunctionGroup &FG) = 0;
  virtual void verifyAnalysis() const {}
  const FGPassImplInterface &getAsFGPassImplInterface() const { return *this; }
  // Please define those static function members too:
  // static getAnalysisUsage(AnalysisUsage& AU)
  // static getPassName()
};

//----------------------------------------------------------------------
// DominatorTreeGroupWrapperPass : Analysis pass which computes a DominatorTree
// per Function in the FunctionGroup.
class DominatorTree;

class DominatorTreeGroupWrapperPass
    : public FGPassImplInterface,
      public IDMixin<DominatorTreeGroupWrapperPass> {
  std::map<Function *, DominatorTree *> DTs;

public:
  DominatorTreeGroupWrapperPass() {}
  ~DominatorTreeGroupWrapperPass() { releaseMemory(); }

  DominatorTree *getDomTree(Function *F) { return DTs[F]; }

  bool runOnFunctionGroup(FunctionGroup &FG) override;

  void verifyAnalysis() const override;

  static void getAnalysisUsage(AnalysisUsage &AU) { AU.setPreservesAll(); }
  static StringRef getPassName() {
    return "DominatorTreeGroupWrapperPassWrapper";
  }

  void releaseMemory() override;

  void print(raw_ostream &OS, const FunctionGroup *FG = nullptr) const override;
};
using DominatorTreeGroupWrapperPassWrapper =
    FunctionGroupWrapperPass<DominatorTreeGroupWrapperPass>;
void initializeDominatorTreeGroupWrapperPassWrapperPass(PassRegistry &);

//----------------------------------------------------------------------
// LoopInfoGroupWrapperPass : Analysis pass which computes a LoopInfo
// per Function in the FunctionGroup.
class LoopInfo;

class LoopInfoGroupWrapperPass : public FGPassImplInterface,
                                 public IDMixin<LoopInfoGroupWrapperPass> {
  std::map<Function *, LoopInfo *> LIs;

public:
  LoopInfoGroupWrapperPass() {}
  ~LoopInfoGroupWrapperPass() { releaseMemory(); }

  LoopInfo *getLoopInfo(Function *F) { return LIs[F]; }
  const DominatorTree &getDomTree();

  bool runOnFunctionGroup(FunctionGroup &FG) override;

  void verifyAnalysis() const override;

  static void getAnalysisUsage(AnalysisUsage &AU) {
    AU.addRequired<DominatorTreeGroupWrapperPass>();
    AU.setPreservesAll();
  }
  static StringRef getPassName() { return "LoopInfoGroupWrapperPassWrapper"; }

  void releaseMemory() override;

  void print(raw_ostream &OS, const FunctionGroup *FG = nullptr) const override;
};
using LoopInfoGroupWrapperPassWrapper =
    FunctionGroupWrapperPass<LoopInfoGroupWrapperPass>;
void initializeLoopInfoGroupWrapperPassWrapperPass(PassRegistry &);
} // end namespace llvm

#endif // LIB_GENXCODEGEN_FUNCTIONGROUP_H
