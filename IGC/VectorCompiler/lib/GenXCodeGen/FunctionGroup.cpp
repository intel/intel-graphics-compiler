/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file implements FunctionGroup, FunctionGroupAnalysis.
// See FunctionGroup.h for more details.
//
// The FunctionGroupPass part was adapted from CallGraphSCCPass.cpp.
//
// This file is currently in lib/Target/GenX, as that is the only place it
// is used. It could be moved somewhere more general.
//
//===----------------------------------------------------------------------===//

#include "FunctionGroup.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include <algorithm>

#include "GenXUtil.h"

#include "llvmWrapper/IR/LegacyPassManagers.h"
#include "llvmWrapper/IR/PassTimingInfo.h"

#include "Probe/Assertion.h"

#define DEBUG_TYPE "functiongroup-passmgr"

using namespace llvm;

static cl::opt<bool> PrintFunctionsUsers(
    "fga-print-functions-users", cl::init(true), cl::Hidden,
    cl::desc("FunctionGroupAnalysis::print emits users of functions"));

bool FunctionGroup::verify() const {
  // TODO: ideally, we'd like to access call-graph here. However,
  // we do not maintain it here.
  for (auto I = Functions.begin(), E = Functions.end(); I != E; ++I) {
    Function *F = &(**I);
    // Note: we do not check FG heads here -
    // users of FG heads can belong to different FG
    if (F == getHead())
      continue;
    for (auto *U : F->users()) {
      auto *CI = genx::checkFunctionCall(U, F);
      if (!CI)
        continue;

      // Note: it is expected that all users of any function from
      // Functions array belong to the same FG
      const Function *Caller = CI->getFunction();
      auto *OtherGroup = FGA->getAnyGroup(Caller);
      IGC_ASSERT_MESSAGE(OtherGroup == this,
                         "inconsistent function group detected!");
      if (OtherGroup != this)
        return false;
    }
  }
  return true;
}

void FunctionGroup::print(raw_ostream &OS) const {
  OS << "{" << getName() << "}\n";

  std::vector<StringRef> FuncsNames;
  llvm::transform(Functions, std::back_inserter(FuncsNames),
                  [](const Function *F) { return F->getName(); });
  // The head remains the first.
  std::sort(std::next(FuncsNames.begin()), FuncsNames.end());
  for (const auto &F : FuncsNames) {
    OS << "  " << F << "\n";
  }

  for (const auto &EnItem : enumerate(Subgroups)) {
    OS << "--SGR[" << EnItem.index() << "]: ";
    OS << "<" << EnItem.value()->getHead()->getName() << ">\n";
  }
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void FunctionGroup::dump() const { print(dbgs()); }
#endif // if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)

/***********************************************************************
 * FunctionGroupAnalysis implementation
 */
char FunctionGroupAnalysis::ID = 0;
INITIALIZE_PASS(FunctionGroupAnalysis, "FunctionGroupAnalysis",
                "FunctionGroupAnalysis", false, true /*analysis*/)

ModulePass *llvm::createFunctionGroupAnalysisPass() {
  initializeFunctionGroupAnalysisPass(*PassRegistry::getPassRegistry());
  return new FunctionGroupAnalysis();
}

// clear : clear out the analysis
void FunctionGroupAnalysis::clear() {
  for (auto T : TypesToProcess)
    GroupMap[T].clear();

  Groups.clear();
  NonMainGroups.clear();
  Visited.clear();
  M = nullptr;
}

FunctionGroup *FunctionGroupAnalysis::getGroup(const Function *F,
                                               FGType Type) const {
  auto i = GroupMap[Type].find(F);
  if (i == GroupMap[Type].end())
    return nullptr;
  return i->second;
}

// getGroup : get the FunctionGroup containing Function F, else 0
FunctionGroup *FunctionGroupAnalysis::getGroup(const Function *F) const {
  return getGroup(F, FGType::GROUP);
}

FunctionGroup *FunctionGroupAnalysis::getSubGroup(const Function *F) const {
  return getGroup(F, FGType::SUBGROUP);
}

FunctionGroup *FunctionGroupAnalysis::getAnyGroup(const Function *F) const {
  auto *Group = getGroup(F, FGType::SUBGROUP);
  if (!Group)
    Group = getGroup(F, FGType::GROUP);
  IGC_ASSERT_MESSAGE(Group, "Function isn't assigned to any function group");
  return Group;
}

// getGroupForHead : get the FunctionGroup for which Function F is the
// head, else 0
FunctionGroup *FunctionGroupAnalysis::getGroupForHead(const Function *F) const {
  auto *FG = getGroup(F);
  IGC_ASSERT(FG->size());
  if (*FG->begin() == F)
    return FG;
  return nullptr;
}

// replaceFunction : replace a Function in a FunctionGroup
// An in-use iterator in the modified FunctionGroup remains valid.
void FunctionGroupAnalysis::replaceFunction(Function *OldF, Function *NewF) {
  for (auto T : TypesToProcess) {
    auto OldFIt = GroupMap[T].find(OldF);
    if (OldFIt == GroupMap[T].end())
      continue;
    FunctionGroup *FG = OldFIt->second;
    GroupMap[T].erase(OldFIt);
    GroupMap[T][NewF] = FG;
    for (auto i = FG->begin();; ++i) {
      IGC_ASSERT(i != FG->end());
      if (*i == OldF) {
        *i = NewF;
        break;
      }
    }
  }
}

// addToFunctionGroup : add Function F to FunctionGroup FG
// Using this (rather than calling push_back directly on the FunctionGroup)
// means that the mapping from F to FG will be created, and getGroup() will
// work for this Function.
void FunctionGroupAnalysis::addToFunctionGroup(FunctionGroup *FG, Function *F,
                                               FGType Type) {
  IGC_ASSERT(FG);
  IGC_ASSERT_MESSAGE(FG->getParent()->getModule() == M,
    "attaching to FunctionGroup from wrong Module");
  IGC_ASSERT_MESSAGE(!GroupMap[Type][F],
    "Function already attached to FunctionGroup");
  GroupMap[Type][F] = FG;
  FG->push_back(F);
}

// createFunctionGroup : create new FunctionGroup for which F is the head
FunctionGroup *FunctionGroupAnalysis::createFunctionGroup(Function *F,
                                                          FGType Type) {
  auto *FG = new FunctionGroup(this);
  auto FGOwner = std::unique_ptr<FunctionGroup>(FG);
  if (Type == FGType::GROUP)
    Groups.push_back(std::move(FGOwner));
  else
    NonMainGroups.push_back(std::move(FGOwner));
  addToFunctionGroup(FG, F, Type);
  return FG;
}

// Returns true if pass is simple module pass,
// e.g. it is neither FG pass nor function pass manager.
static bool isModulePass(Pass *P) {
  if (P->getPassKind() != PT_Module)
    return false;
  return !P->getAsPMDataManager();
}

static StringRef TypeToAttr(FunctionGroupAnalysis::FGType Type) {
  switch (Type) {
  case FunctionGroupAnalysis::FGType::GROUP:
    return genx::FunctionMD::CMGenXMain;
  case FunctionGroupAnalysis::FGType::SUBGROUP:
    return genx::FunctionMD::CMStackCall;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "Can't gen attribute for nox-existent FG type");
    break;
  }
  return "";
}

bool FunctionGroupAnalysis::buildGroup(CallGraph &Callees, Function *F,
                                       FunctionGroup *curGr, FGType Type) {
  bool result = false;
  LLVM_DEBUG(dbgs() << "process function " << F->getName() << " from " << curGr
                    << ", type = " << Type << "\n");
  if (Visited.count(F) > 0) {
    bool NeedCloning =
        std::any_of(std::begin(TypesToProcess), std::end(TypesToProcess),
                    [&GM = GroupMap, curGr, F](FGType CurType) {
                      return GM[CurType].count(F) && GM[CurType][F] != curGr;
                    });
    if (NeedCloning && !F->hasFnAttribute(TypeToAttr(Type))) {
      ValueToValueMapTy VMap;
      Function *ClonedFunc = CloneFunction(F, VMap);
      ClonedFunc->setName(F->getName() + "." + curGr->getHead()->getName());
      LLVM_DEBUG(dbgs() << "Cloning: " << ClonedFunc->getName() << "\n");

      result = true;

      for (auto it = F->use_begin(); it != F->use_end();) {
        Use *u = &*it++;
        auto *CI = dyn_cast<CallInst>(u->getUser());
        IGC_ASSERT(CI);
        if (GroupMap[Type][CI->getFunction()] == curGr)
          *u = ClonedFunc;
      }
      addToFunctionGroup(curGr, ClonedFunc, Type);

      for (auto &Callee : Callees[F]) {
        if (Callee == F)
          continue;
        if (vc::requiresStackCall(Callee)) {
          LLVM_DEBUG(dbgs()
                     << "\tDo not process next callee " << Callee->getName()
                     << " because it's a stack call\n");
          continue;
        }
        LLVM_DEBUG(dbgs() << "Next callee: " << Callee->getName() << "\n");
        result |= buildGroup(Callees, Callee, curGr, Type);
      }
    }
  } else if (!Visited.count(F)) {
    Visited.insert(F);
    // group is created either on a function with a corresponding attribute
    // or on a root of a whole function tree that is kernel (genx_main)
    if (F->hasFnAttribute(TypeToAttr(Type)) ||
        F->hasFnAttribute(genx::FunctionMD::CMGenXMain)) {
      LLVM_DEBUG(dbgs() << "Create new group of type " << Type << "\n");
      curGr = createFunctionGroup(F, Type);
    } else if (curGr) {
      LLVM_DEBUG(dbgs() << "Add to group " << curGr->getHead()->getName()
                        << " of type " << Type << "\n");
      addToFunctionGroup(curGr, F, Type);
    }
    for (auto &Callee : Callees[F]) {
      if (vc::requiresStackCall(Callee)) {
        LLVM_DEBUG(dbgs() << "\tDo not process next callee "
                          << Callee->getName()
                          << " because it's a stack call\n");
        continue;
      }

      LLVM_DEBUG(dbgs() << "Next callee: " << Callee->getName() << "\n");
      result |= buildGroup(Callees, Callee, curGr, Type);
    }
  }
  LLVM_DEBUG(dbgs() << "finish processing function " << F->getName()
                    << " on level " << Type << "\n");
  return result;
}

bool FunctionGroupAnalysis::verify() const {
  return llvm::all_of(AllGroups(), [](const auto &GR) { return GR->verify(); });
}

void FunctionGroupAnalysis::print(raw_ostream &OS, const Module *) const {
  OS << "Number of Groups = " << Groups.size() << "\n";
  for (const auto &X : enumerate(Groups)) {
    OS << "GR[" << X.index() << "] = <\n";
    X.value()->print(OS);
    OS << ">\n";
  }
  OS << "Number of SubGroups = " << NonMainGroups.size() << "\n";
  for (const auto &X : enumerate(NonMainGroups)) {
    OS << "SGR[" << X.index() << "] = <\n";
    X.value()->print(OS);
    OS << ">\n";
  }
  if (!PrintFunctionsUsers)
    return;

  for (auto T : TypesToProcess) {
    std::map<StringRef, std::set<StringRef>> FuncUsers;
    for (auto [F, FG] : GroupMap[T]) {
      FuncUsers[F->getName()];
      for (auto *U : F->users()) {
        auto *CI = genx::checkFunctionCall(U, F);
        if (!CI)
          continue;
        const Function *Caller = CI->getFunction();
        FuncUsers[F->getName()].insert(Caller->getName());
      }
    }
    for (const auto &[FuncName, UsersNames] : FuncUsers) {
      OS << "Users of " << FuncName << ":";
      for (const auto &UserName : UsersNames) {
        OS << " " << UserName;
      }
      OS << "\n";
    }
  }
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void FunctionGroupAnalysis::dump() const { print(dbgs(), nullptr); }
#endif // if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)

//===----------------------------------------------------------------------===//
//  DominatorTreeGroupWrapperPass Implementation
//===----------------------------------------------------------------------===//
//
// The implementation details of the wrapper pass that holds a DominatorTree
// per Function in a FunctionGroup.
//
//===----------------------------------------------------------------------===//
INITIALIZE_PASS_BEGIN(DominatorTreeGroupWrapperPassWrapper,
                      "groupdomtreeWrapper",
                      "Group Dominator Tree Construction Wrapper", true, true)
INITIALIZE_PASS_END(DominatorTreeGroupWrapperPassWrapper, "groupdomtreeWrapper",
                    "Group Dominator Tree Construction", true, true)

void DominatorTreeGroupWrapperPass::releaseMemory() {
  for (auto i = DTs.begin(), e = DTs.end(); i != e; ++i)
    delete i->second;
  DTs.clear();
}

bool DominatorTreeGroupWrapperPass::runOnFunctionGroup(FunctionGroup &FG) {
  for (auto fgi = FG.begin(), fge = FG.end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    auto DT = new DominatorTree;
    DT->recalculate(*F);
    DTs[F] = DT;
  }
  return false;
}

void DominatorTreeGroupWrapperPass::verifyAnalysis() const {
  for (auto i = DTs.begin(), e = DTs.end(); i != e; ++i)
    i->second->verify();
}

void DominatorTreeGroupWrapperPass::print(raw_ostream &OS,
                                          const FunctionGroup *) const {
  for (auto i = DTs.begin(), e = DTs.end(); i != e; ++i)
    i->second->print(OS);
}

//===----------------------------------------------------------------------===//
//  LoopInfoGroupWrapperPass Implementation
//===----------------------------------------------------------------------===//
//
// The implementation details of the wrapper pass that holds a LoopInfo
// per Function in a FunctionGroup.
//
//===----------------------------------------------------------------------===//
INITIALIZE_PASS_BEGIN(LoopInfoGroupWrapperPassWrapper, "grouploopinfoWrapper",
                      "Group Loop Info Construction Wrapper", true, true)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeGroupWrapperPassWrapper)
INITIALIZE_PASS_END(LoopInfoGroupWrapperPassWrapper, "grouploopinfoWrapper",
                    "Group Loop Info Construction Wrapper", true, true)

void LoopInfoGroupWrapperPass::releaseMemory() {
  for (auto i = LIs.begin(), e = LIs.end(); i != e; ++i)
    delete i->second;
  LIs.clear();
}

bool LoopInfoGroupWrapperPass::runOnFunctionGroup(FunctionGroup &FG) {
  auto &DTs = getAnalysis<DominatorTreeGroupWrapperPass>();
  for (auto fgi = FG.begin(), fge = FG.end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    auto LI = new LoopInfo;
    LI->analyze(*DTs.getDomTree(F));
    LIs[F] = LI;
  }
  return false;
}

void LoopInfoGroupWrapperPass::verifyAnalysis() const {
  auto &DTs = getAnalysis<DominatorTreeGroupWrapperPass>();
  for (auto i = LIs.begin(), e = LIs.end(); i != e; ++i)
    i->second->verify(*DTs.getDomTree(i->first));
}

void LoopInfoGroupWrapperPass::print(raw_ostream &OS,
                                     const FunctionGroup *) const {
  for (auto i = LIs.begin(), e = LIs.end(); i != e; ++i)
    i->second->print(OS);
}
