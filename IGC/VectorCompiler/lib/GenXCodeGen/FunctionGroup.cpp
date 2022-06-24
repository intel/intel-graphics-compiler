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
#include "llvm/ADT/STLExtras.h"
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
#include "llvmWrapper/IR/Value.h"

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

static FunctionGroupAnalysis::CallGraphTy buildCallGraph(Module &M) {
  FunctionGroupAnalysis::CallGraphTy CG;
  std::unordered_map<Function *, std::unordered_set<Function *>> Visited;
  for (auto &F : M) {
    CG[&F];
    for (auto *U : F.users()) {
      auto *Inst = dyn_cast<Instruction>(U);
      if (!Inst)
        continue;
      if (!F.empty() && Visited[Inst->getFunction()].count(&F) == 0) {
        CG[Inst->getFunction()].push_back(&F);
        Visited[Inst->getFunction()].insert(&F);
      }
      // Recursive functions must use stack.
      if (Inst->getFunction() == &F) {
        const bool UsesStack = vc::requiresStackCall(&F);
        IGC_ASSERT_MESSAGE(
            UsesStack,
            "Found recursive function without CMStackCall attribute");
        (void)UsesStack;
      }
    }
  }

  return CG;
}

// Depth-first traversal of all reachable functions from StartPoint in call
// graph CG. Does not visit functions for which Pred(Function *) returns false.
// Calls OnNode(Function *F) for each function F of the subgraph that is
// traversed. Calls OnEdge(Function *F, Function *Callee) for each
// function-callee pair of the subgraph that is traversed if both Pred(F) and
// Pred(Callee) return true.
template <typename CallbackOnNode, typename CallbackOnEdge,
          typename UnaryPredicate>
static void traverseCallGraph(const FunctionGroupAnalysis::CallGraphTy &CG,
                              Function *StartPoint, CallbackOnNode OnNode,
                              CallbackOnEdge OnEdge, UnaryPredicate Pred) {
  if (!Pred(StartPoint))
    return;
  std::vector<Function *> Stack = {StartPoint};
  std::unordered_set<Function *> Visited = {StartPoint};
  while (!Stack.empty()) {
    Function *F = Stack.back();
    Stack.pop_back();
    IGC_ASSERT_MESSAGE(CG.count(F), "Inconsistent call graph");

    OnNode(F);
    for (Function *Callee : CG.at(F)) {
      if (!Pred(Callee))
        continue;
      OnEdge(F, Callee);
      if (Visited.count(Callee))
        continue;
      Visited.insert(Callee);
      Stack.push_back(Callee);
    }
  }
}

template <typename CallbackOnNode, typename UnaryPredicate>
static void traverseCallGraphNodes(const FunctionGroupAnalysis::CallGraphTy &CG,
                                   Function *StartPoint, CallbackOnNode OnNode,
                                   UnaryPredicate Pred) {
  traverseCallGraph(
      CG, StartPoint, OnNode, [](Function *, Function *) {}, Pred);
}

template <typename CallbackOnEdge, typename UnaryPredicate>
static void traverseCallGraphEdges(const FunctionGroupAnalysis::CallGraphTy &CG,
                                   Function *StartPoint, CallbackOnEdge OnEdge,
                                   UnaryPredicate Pred) {
  traverseCallGraph(
      CG, StartPoint, [](Function *) {}, OnEdge, Pred);
}

using FGHead = Function;
// Maps the function to the heads of all function groups to which this function
// belongs.
using FuncToGroupsMapTy = std::unordered_map<Function *, std::vector<FGHead *>>;
// Maps the original function to this function in a specific function group, it
// can be the original function itself or its clone.
using FuncToClonesMapTy =
    std::unordered_map<Function *, std::unordered_map<FGHead *, Function *>>;

static FuncToGroupsMapTy
buildFuncToGroupsMap(const FunctionGroupAnalysis::CallGraphTy &CG,
                     const std::vector<Function *> &Heads) {
  FuncToGroupsMapTy FuncToGroupsMap;
  for (Function *Head : Heads) {
    traverseCallGraphNodes(
        CG, Head,
        [&FuncToGroupsMap, Head](Function *F) {
          FuncToGroupsMap[F].push_back(Head);
        },
        // Do not process stack calls, except for heads of subgroups.
        [Head](Function *F) { return F == Head || !vc::requiresStackCall(F); });
  }
  return FuncToGroupsMap;
}

// Clones each function for each function group (except for one) to which it
// belongs. The second return value is whether the module has been changed.
static std::pair<FuncToClonesMapTy, bool>
cloneFunctions(const FuncToGroupsMapTy &FuncToGroupsMap) {
  FuncToClonesMapTy FuncToClonesMap;
  bool ModuleModified = false;
  for (const auto &[F, FGs] : FuncToGroupsMap) {
    IGC_ASSERT(!FGs.empty());
    FuncToClonesMap[F][FGs.front()] = F;
    for (Function *FG : drop_begin(FGs, 1)) {
      ModuleModified = true;
      ValueToValueMapTy VMap;
      Function *ClonedFunc = CloneFunction(F, VMap);
      FuncToClonesMap[F][FG] = ClonedFunc;
    }
    // Rename clones if the function belongs to several function groups
    if (FGs.size() > 1) {
      auto FuncName = F->getName();
      for (auto [FG, ActualFunc] : FuncToClonesMap[F])
        ActualFunc->setName(FuncName + "." + FG->getName());
    }
  }
  return {std::move(FuncToClonesMap), ModuleModified};
}

// Restores correct uses between functions clones. The CG itself is not
// modified.
//
// Let's name actual clones of F and Callee in the current function group as
// ActualF and ActualCallee. When the clones were created the uses remained the
// same, so ActualF uses Callee. But we need to make ActualF use ActualCallee,
// and so for each function-callee pair in the original CG and for each FG.
// --------------------------------------
// | Original CG |    Functions uses    |
// --------------------------------------
// |             |                      |
// |   Callee    | Callee  ActualCallee |
// |     ^       |      ^      ^        |
// |     |       |       \     |        |
// |     |       |        X    |        |
// |     |       |         \   |        |
// |     F       |         ActualF      |
// |             |                      |
// --------------------------------------
// Callee may coincide with ActualCallee.
static void recoverEdges(const FunctionGroupAnalysis::CallGraphTy &CG,
                         const std::vector<Function *> &Heads,
                         const FuncToClonesMapTy &FuncToClonesMap) {
  for (Function *Head : Heads) {
    // The original graph is traversed, but edges are constructed between the
    // actual functions of the current function group.
    traverseCallGraphEdges(
        CG, Head,
        [&FuncToClonesMap, Head](Function *F, Function *Callee) {
          Function *ActualF = FuncToClonesMap.at(F).at(Head);
          Function *ActualCallee = FuncToClonesMap.at(Callee).at(Head);
          IGCLLVM::replaceUsesWithIf(Callee, ActualCallee, [ActualF](Use &U) {
            auto *CI = dyn_cast<CallInst>(U.getUser());
            IGC_ASSERT(CI);
            // Callee use should be replaced only if it is called from ActualF,
            // i.e. in the current function group.
            return CI->getFunction() == ActualF;
          });
        },
        // Do not process stack calls, except for heads of subgroups.
        [Head](Function *F) { return F == Head || !vc::requiresStackCall(F); });
  }
}

// Makes each function of the module belong to only one function group. If a
// function belongs to several function groups, it is copied.
bool FunctionGroupAnalysis::legalizeGroups() {
  const CallGraphTy CG = buildCallGraph(*M);

  std::vector<Function *> Heads;
  auto HeadsRange =
      make_filter_range(*M, [](Function &F) { return genx::fg::isHead(F); });
  transform(HeadsRange, std::back_inserter(Heads),
            [](Function &F) { return &F; });

  auto FuncToGroupsMap = buildFuncToGroupsMap(CG, Heads);
  auto [FuncToClonesMap, ModuleModified] = cloneFunctions(FuncToGroupsMap);
  if (!ModuleModified)
    return false;
  recoverEdges(CG, Heads, FuncToClonesMap);
  return true;
}

void FunctionGroupAnalysis::buildGroup(const CallGraphTy &CG, Function *Head,
                                       FGType Type) {
  FunctionGroup *FG = createFunctionGroup(Head, Type);
  traverseCallGraphNodes(
      CG, Head,
      [this, Head, FG, Type](Function *F) {
        if (F == Head)
          return;
        addToFunctionGroup(FG, F, Type);
      },
      [Head](Function *F) { return F == Head || !vc::requiresStackCall(F); });
}

bool FunctionGroupAnalysis::verify() const {
  return llvm::all_of(AllGroups(), [](const auto &GR) { return GR->verify(); });
}

// Fills in Groups and NonMainGroups. It is assumed that all function groups
// have already been legalized, i.e. no function of a module is called from
// two different heads.
void FunctionGroupAnalysis::buildGroups() {
  const CallGraphTy CG = buildCallGraph(*M);
  for (auto T : TypesToProcess) {
    for (auto &F : *M) {
      if (F.isDeclaration())
        continue;
      if (!genx::fg::isHead(F))
        continue;
      // Do not process kernels at subgroup level.
      if (genx::fg::isGroupHead(F) &&
          T == FunctionGroupAnalysis::FGType::SUBGROUP)
        continue;
      // Do not process stack calls at group level.
      if (genx::fg::isSubGroupHead(F) &&
          T == FunctionGroupAnalysis::FGType::GROUP)
        continue;
      buildGroup(CG, &F, T);
    }
  }

  for (auto SubFG : subgroups()) {
    const Function *Head = SubFG->getHead();
    IGC_ASSERT(Head);

    for (auto U : Head->users()) {
      const auto *UserInst = dyn_cast<CallInst>(U);
      if (!UserInst)
        continue;

      const Function *UserFunction = UserInst->getFunction();
      IGC_ASSERT(UserFunction);
      FunctionGroup *UserFG = getAnyGroup(UserFunction);
      IGC_ASSERT(UserFG);

      UserFG->addSubgroup(SubFG);
    }
  }

  IGC_ASSERT(verify());
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
