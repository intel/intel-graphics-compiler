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
// This file implements FunctionGroup, FunctionGroupAnalysis and
// FunctionGroupPass. See FunctionGroup.h for more details.
//
// The FunctionGroupPass part was adapted from CallGraphSCCPass.cpp.
//
// This file is currently in lib/Target/GenX, as that is the only place it
// is used. It could be moved somewhere more general.
//
//===----------------------------------------------------------------------===//

#include "FunctionGroup.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
using namespace llvm;

#include "llvmWrapper/IR/LegacyPassManagers.h"
#include "llvmWrapper/IR/PassTimingInfo.h"


#define DEBUG_TYPE "functiongroup-passmgr"

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

  for (auto i = begin(), e = end(); i != e; ++i)
    delete *i;
  for (auto i = NonMainGroups.begin(), e = NonMainGroups.end(); i != e; ++i)
    delete *i;

  Groups.clear();
  NonMainGroups.clear();
  M = nullptr;
}

FunctionGroup *FunctionGroupAnalysis::getGroup(Function *F, FGType Type) {
  auto i = GroupMap[Type].find(F);
  if (i == GroupMap[Type].end())
    return nullptr;
  return i->second;
}

// getGroup : get the FunctionGroup containing Function F, else 0
FunctionGroup *FunctionGroupAnalysis::getGroup(Function *F) {
  return getGroup(F, FGType::GROUP);
}

FunctionGroup *FunctionGroupAnalysis::getSubGroup(Function *F) {
  return getGroup(F, FGType::SUBGROUP);
}

// getGroupForHead : get the FunctionGroup for which Function F is the
// head, else 0
FunctionGroup *FunctionGroupAnalysis::getGroupForHead(Function *F) {
  auto FG = getGroup(F);
  assert(FG->size());
  if (*FG->begin() == F)
    return FG;
  return nullptr;
}

// replaceFunction : replace a Function in a FunctionGroup
// An in-use iterator in the modified FunctionGroup remains valid.
void FunctionGroupAnalysis::replaceFunction(Function *OldF, Function *NewF) {
  for (auto T : TypesToProcess) {
    auto OldFIt = GroupMap[T].find(OldF);
    assert(OldFIt != GroupMap[T].end());
    FunctionGroup *FG = OldFIt->second;
    GroupMap[T].erase(OldFIt);
    GroupMap[T][NewF] = FG;
    for (auto i = FG->begin();; ++i) {
      assert(i != FG->end());
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
  assert(FG);
  assert(FG->getParent()->getModule() == M &&
         "attaching to FunctionGroup from wrong Module");
  assert(!GroupMap[Type][F] && "Function already attached to FunctionGroup");
  GroupMap[Type][F] = FG;
  FG->push_back(F);
}

// createFunctionGroup : create new FunctionGroup for which F is the head
FunctionGroup *FunctionGroupAnalysis::createFunctionGroup(Function *F,
                                                          FGType Type) {
  auto FG = new FunctionGroup(this);
  if (Type == FGType::GROUP)
    Groups.push_back(FG);
  else
    NonMainGroups.push_back(FG);
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
    llvm_unreachable("Can't gen attribute for nox-existent FG type");
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
    if (GroupMap[Type].count(F) > 0 && GroupMap[Type][F] != curGr &&
        !F->hasFnAttribute(TypeToAttr(Type))) {
      ValueToValueMapTy VMap;
      Function *ClonedFunc = CloneFunction(F, VMap);
      LLVM_DEBUG(dbgs() << "Cloning: " << ClonedFunc->getName() << "\n");

      result = true;

      for (auto it = F->use_begin(); it != F->use_end();) {
        Use *u = &*it++;
        auto *CI = dyn_cast<CallInst>(u->getUser());
        assert(CI);
        if (GroupMap[Type][CI->getFunction()] == curGr)
          *u = ClonedFunc;
      }
      for (auto T : TypesToProcess) {
        if (T >= Type)
          break;
        addToFunctionGroup(getGroup(F, T), ClonedFunc, T);
      }
      addToFunctionGroup(curGr, ClonedFunc, Type);

      for (auto &Callee : Callees[F]) {
        if (Callee == F)
          continue;
        LLVM_DEBUG(dbgs() << "Next callee: " << Callee->getName() << "\n");
        result |= buildGroup(Callees, Callee, curGr, Type);
      }
    }
  } else if (!Visited.count(F)) {
    Visited[F] = true;
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
      LLVM_DEBUG(dbgs() << "Next callee: " << Callee->getName() << "\n");
      result |= buildGroup(Callees, Callee, curGr, Type);
    }
  }
  LLVM_DEBUG(dbgs() << "finish processing function " << F->getName()
                    << " on level " << Type << "\n");
  return result;
}

//===----------------------------------------------------------------------===//
// FGPassManager
//
/// FGPassManager manages FPPassManagers and FunctionGroupPasses.
/// It actually now imitates MPPassManager because there is no way
/// to extend pass manager structure without modification of
/// LLVM pass managers code.
/// This pass is injected into pass manager stack instead of top-level
/// MPPassManager when there is first time FunctionGroupPass is created.
/// After this manager replaces MPPassManager, it handles all Module and
/// FunctionGroup passes. This manager itself is module pass so it is
/// actually contained in list of module passes of module pass manager
/// as last pass that should be run. However, top-level pass manager do
/// not know anything about this FGPassManager except that it is indirect
/// pass manager, so it will not run it directly.

namespace {

class FGPassManager : public ModulePass, public IGCLLVM::PMDataManager {
public:
  static char ID;
  explicit FGPassManager() : ModulePass(ID), IGCLLVM::PMDataManager() {}

  /// run - Execute all of the passes scheduled for execution.  Keep track of
  /// whether any of the passes modifies the module, and if so, return true.
  bool runOnModule(Module &M) override;

  bool doInitialization(Module &M) override;
  bool doFinalization(Module &M) override;

  /// Pass Manager itself does not invalidate any analysis info.
  void getAnalysisUsage(AnalysisUsage &Info) const override {
    // FGPassManager needs FunctionGroupAnalysis.
    Info.addRequired<FunctionGroupAnalysis>();
    Info.setPreservesAll();
  }

  StringRef getPassName() const override {
    return "FunctionGroup Pass Manager";
  }

  PMDataManager *getAsPMDataManager() override { return this; }
  Pass *getAsPass() override { return this; }

  // Print passes managed by this manager
  void dumpPassStructure(unsigned Offset) override {
    errs().indent(Offset * 2) << "FunctionGroup Pass Manager\n";
    for (unsigned Index = 0; Index < getNumContainedPasses(); ++Index) {
      Pass *P = getContainedPass(Index);
      unsigned DumpOffset = Offset + 1;
      // Pretend that there is no FGPassManager when we need to dump
      // module pass indentation.
      if (isModulePass(P))
        DumpOffset -= 1;
      P->dumpPassStructure(DumpOffset);
      dumpLastUses(P, DumpOffset);
    }
  }

  Pass *getContainedPass(unsigned N) {
    assert(N < PassVector.size() && "Pass number out of range!");
    return static_cast<Pass *>(PassVector[N]);
  }

  PassManagerType getPassManagerType() const override {
    return PMT_ModulePassManager;
  }

private:
  bool runPassesOnFunctionGroup(unsigned Begin, unsigned End, FunctionGroup &FG);
  bool runPassOnFunctionGroup(Pass *P, FunctionGroup &FG);
  bool doFGInitialization(unsigned Begin, unsigned End, FunctionGroupAnalysis &FGA);
  bool doFGFinalization(unsigned Begin, unsigned End, FunctionGroupAnalysis &FGA);
  bool runFGPassSequence(unsigned &Pass);
  bool runModulePassSequence(unsigned &Pass, Module &M);
};

} // end anonymous namespace.

char FGPassManager::ID = 0;

bool FGPassManager::runPassOnFunctionGroup(Pass *P, FunctionGroup &FG) {
  bool Changed = false;
  llvm::PMDataManager *PM = P->getAsPMDataManager();

  if (!PM) {
    FunctionGroupPass *CGSP = (FunctionGroupPass *)P;
    {
      TimeRegion PassTimer(getPassTimer(CGSP));
      Changed = CGSP->runOnFunctionGroup(FG);
    }
    return Changed;
  }

  // TODO: there may be also SCC pass manager.
  assert(PM->getPassManagerType() == PMT_FunctionPassManager &&
         "Invalid FGPassManager member");
  FPPassManager *FPP = (FPPassManager *)P;

  // Run pass P on all functions in the current FunctionGroup.
  for (auto &F : FG) {
    dumpPassInfo(P, EXECUTION_MSG, ON_FUNCTION_MSG, F->getName());
    {
      TimeRegion PassTimer(getPassTimer(FPP));
      Changed |= FPP->runOnFunction(*F);
    }
    F->getContext().yield();
  }
  return Changed;
}


/// RunPassesOnFunctionGroup -  Execute sequential passes of pass manager
/// on the specified FunctionGroup
bool FGPassManager::runPassesOnFunctionGroup(unsigned Begin, unsigned End,
                                             FunctionGroup &FG) {
  bool Changed = false;

  // Run selected passes on current FunctionGroup.
  for (unsigned PassNo = Begin; PassNo != End; ++PassNo) {
    Pass *P = getContainedPass(PassNo);
    dumpRequiredSet(P);

    initializeAnalysisImpl(P);

    // Actually run this pass on the current FunctionGroup.
    Changed |= runPassOnFunctionGroup(P, FG);
    if (Changed)
      dumpPassInfo(P, MODIFICATION_MSG, ON_MODULE_MSG, "");
    dumpPreservedSet(P);

    verifyPreservedAnalysis(P);
    removeNotPreservedAnalysis(P);
    recordAvailableAnalysis(P);
    removeDeadPasses(P, "", ON_MODULE_MSG);
  }

  return Changed;
}

/// Initialize sequential FG passes
bool FGPassManager::doFGInitialization(unsigned Begin, unsigned End,
                                       FunctionGroupAnalysis &FGA) {
  bool Changed = false;

  for (unsigned i = Begin; i != End; ++i) {
    if (llvm::PMDataManager *PM = getContainedPass(i)->getAsPMDataManager()) {
      // TODO: SCC PassManager?
      assert(PM->getPassManagerType() == PMT_FunctionPassManager &&
        "Invalid FGPassManager member");
      Changed |= ((FPPassManager*)PM)->doInitialization(*FGA.getModule());
    } else {
      Changed |=
          ((FunctionGroupPass *)getContainedPass(i))->doInitialization(FGA);
    }
  }

  return Changed;
}

/// Finalize sequential FG passes
bool FGPassManager::doFGFinalization(unsigned Begin, unsigned End,
                                     FunctionGroupAnalysis &FGA) {
  bool Changed = false;

  for (int i = End - 1; i >= static_cast<int>(Begin); --i) {
    if (llvm::PMDataManager *PM = getContainedPass(i)->getAsPMDataManager()) {
      // TODO: SCC PassManager?
      assert(PM->getPassManagerType() == PMT_FunctionPassManager &&
        "Invalid FGPassManager member");
      Changed |= ((FPPassManager*)PM)->doFinalization(*FGA.getModule());
    } else {
      Changed |=
          ((FunctionGroupPass *)getContainedPass(i))->doFinalization(FGA);
    }
  }

  return Changed;
}

bool FGPassManager::runFGPassSequence(unsigned &Pass) {
  const unsigned BeginPass = Pass;
  const unsigned NumPasses = getNumContainedPasses();
  while (Pass < NumPasses && !isModulePass(getContainedPass(Pass)))
    ++Pass;

  // Function group analysis may be invalidated by previous
  // module passes so we will need to query it every time we
  // execute sequence of passes.
  FunctionGroupAnalysis &FGA = getAnalysis<FunctionGroupAnalysis>();
  bool Changed = false;

  Changed |= doFGInitialization(BeginPass, Pass, FGA);
  for (auto *FG : FGA)
    Changed |= runPassesOnFunctionGroup(BeginPass, Pass, *FG);
  Changed |= doFGFinalization(BeginPass, Pass, FGA);

  return Changed;
}

bool FGPassManager::runModulePassSequence(unsigned &Pass, Module &M) {
  const unsigned BeginPass = Pass;
  const unsigned NumPasses = getNumContainedPasses();
  while (Pass < NumPasses && isModulePass(getContainedPass(Pass)))
    ++Pass;

  bool Changed = false;

  // Copied from MPPassManager in LegacyPassManager.cpp.
  unsigned InstrCount, ModuleCount = 0;
  StringMap<std::pair<unsigned, unsigned>> FunctionToInstrCount;
  bool EmitICRemark = M.shouldEmitInstrCountChangedRemark();
  // Collect the initial size of the module.
  if (EmitICRemark) {
    InstrCount = initSizeRemarkInfo(M, FunctionToInstrCount);
    ModuleCount = InstrCount;
  }

  for (unsigned Index = BeginPass; Index < Pass; ++Index) {
    auto *MP = static_cast<ModulePass *>(getContainedPass(Index));
    bool LocalChanged = false;

    dumpPassInfo(MP, EXECUTION_MSG, ON_MODULE_MSG, M.getModuleIdentifier());
    dumpRequiredSet(MP);

    initializeAnalysisImpl(MP);

    {
      PassManagerPrettyStackEntry X(MP, M);
      TimeRegion PassTimer(getPassTimer(MP));

      LocalChanged |= MP->runOnModule(M);
      if (EmitICRemark) {
        // Update the size of the module.
        ModuleCount = M.getInstructionCount();
        if (ModuleCount != InstrCount) {
          int64_t Delta = static_cast<int64_t>(ModuleCount) -
            static_cast<int64_t>(InstrCount);
          emitInstrCountChangedRemark(MP, M, Delta, InstrCount,
            FunctionToInstrCount);
          InstrCount = ModuleCount;
        }
      }
    }

    Changed |= LocalChanged;
    if (LocalChanged)
      dumpPassInfo(MP, MODIFICATION_MSG, ON_MODULE_MSG,
        M.getModuleIdentifier());
    dumpPreservedSet(MP);
    dumpUsedSet(MP);

    verifyPreservedAnalysis(MP);
    removeNotPreservedAnalysis(MP);
    recordAvailableAnalysis(MP);
    removeDeadPasses(MP, M.getModuleIdentifier(), ON_MODULE_MSG);
  }

  return Changed;
}

/// run - Execute all of the passes scheduled for execution.  Keep track of
/// whether any of the passes modifies the module, and if so, return true.
bool FGPassManager::runOnModule(Module &M) {
  bool Changed = false;

  unsigned CurPass = 0;
  unsigned NumPasses = getNumContainedPasses();
  while (CurPass != NumPasses) {
    // We will always have chain of fg passes followed by
    // module passes repeating until there are no passes.
    Changed |= runFGPassSequence(CurPass);
    Changed |= runModulePassSequence(CurPass, M);
  }

  return Changed;
}

bool FGPassManager::doInitialization(Module &M) {
  bool Changed = false;

  // Initialize module passes
  for (unsigned Index = 0; Index < getNumContainedPasses(); ++Index) {
    auto *P = getContainedPass(Index);
    if (isModulePass(P))
      Changed |= P->doInitialization(M);
  }

  return Changed;
}

bool FGPassManager::doFinalization(Module &M) {
  bool Changed = false;

  // Finalize module passes
  for (int Index = getNumContainedPasses() - 1; Index >= 0; --Index) {
    auto *P = getContainedPass(Index);
    if (isModulePass(P))
      Changed |= P->doFinalization(M);
  }

  return Changed;
}

//===----------------------------------------------------------------------===//
// FunctionGroupPass Implementation
//===----------------------------------------------------------------------===//

/// Assign pass manager to manage this pass.
void FunctionGroupPass::assignPassManager(PMStack &PMS,
                                          PassManagerType PreferredType) {
  // Find module pass manager.
  while (!PMS.empty() &&
         PMS.top()->getPassManagerType() > PMT_ModulePassManager)
    PMS.pop();

  assert(!PMS.empty() && "Unable to handle FunctionGroup Pass");
  FGPassManager *GFP;
  
  // Check whether this ModulePassManager is our injected function
  // group pass manager. If not, replace old module pass manager
  // with one for function groups.
  auto *PM = PMS.top();
  assert(PM->getPassManagerType() == PMT_ModulePassManager &&
         "Bad pass manager type for function group pass manager");
  if (PM->getAsPass()->getPassID() == &FGPassManager::ID)
    GFP = static_cast<FGPassManager *>(PM);
  else {
    // Create new FunctionGroup Pass Manager if it does not exist. 

    // [1] Create new FunctionGroup Pass Manager
    GFP = new FGPassManager();

    // [2] Set up new manager's top level manager
    PMTopLevelManager *TPM = PM->getTopLevelManager();
    TPM->addIndirectPassManager(GFP);
    GFP->setTopLevelManager(TPM);

    // [3] Assign manager to manage this new manager. This should not create
    // and push new managers into PMS
    TPM->schedulePass(GFP);
    assert(PMS.top() == PM && "Pass manager unexpectedly changed");

    // [4] Steal analysis info from module pass manager.
    *GFP->getAvailableAnalysis() = std::move(*PM->getAvailableAnalysis());

    // [5] Replace module pass manager with function group pass manager.
    PMS.pop();
    PMS.push(GFP);
  }

  GFP->add(this);
}

/// getAnalysisUsage - For this class, we declare that we require and preserve
/// FunctionGroupAnalysis.  If the derived class implements this method, it
/// should always explicitly call the implementation here.
void FunctionGroupPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<FunctionGroupAnalysis>();
  AU.addPreserved<FunctionGroupAnalysis>();
}

//===----------------------------------------------------------------------===//
// PrintFunctionGroupPass Implementation
//===----------------------------------------------------------------------===//

namespace {
/// PrintFunctionGroupPass - Print a FunctionGroup
///
class PrintFunctionGroupPass : public FunctionGroupPass {
  std::string Banner;
  raw_ostream &Out; // raw_ostream to print on.
public:
  static char ID;
  PrintFunctionGroupPass(const std::string &B, raw_ostream &o)
      : FunctionGroupPass(ID), Banner(B), Out(o) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }

  bool runOnFunctionGroup(FunctionGroup &FG) override {
    Out << Banner;
    for (auto I = FG.begin(), E = FG.end(); I != E; ++I) {
      Function *F = *I;
      Out << Banner << static_cast<Value &>(*F);
    }
    return false;
  }
};
} // end anonymous namespace.

char PrintFunctionGroupPass::ID = 0;

Pass *FunctionGroupPass::createPrinterPass(raw_ostream &O,
                                           const std::string &Banner) const {
  return new PrintFunctionGroupPass(Banner, O);
}

//===----------------------------------------------------------------------===//
//  DominatorTreeGroupWrapperPass Implementation
//===----------------------------------------------------------------------===//
//
// The implementation details of the wrapper pass that holds a DominatorTree
// per Function in a FunctionGroup.
//
//===----------------------------------------------------------------------===//
char DominatorTreeGroupWrapperPass::ID = 0;
INITIALIZE_PASS_BEGIN(DominatorTreeGroupWrapperPass, "groupdomtree",
                      "Group Dominator Tree Construction", true, true)
INITIALIZE_PASS_END(DominatorTreeGroupWrapperPass, "groupdomtree",
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
                                          const Module *) const {
  for (auto i = DTs.begin(), e = DTs.end(); i != e; ++i)
    i->second->print(OS);
}
