/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvmWrapper/Transforms/IPO/InlineHelper.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/CallGraphSCCPass.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Analysis/CallGraph.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

#define DEBUG_TYPE "inline"

namespace IGCLLVM {

bool removeDeadFunctions(CallGraph &CG, bool AlwaysInlineOnly) {
  SmallVector<CallGraphNode *, 16> FunctionsToRemove;
  SmallVector<Function *, 16> DeadFunctionsInComdats;

  auto RemoveCGN = [&](CallGraphNode *CGN) {
    // Remove any call graph edges from the function to its callees.
    CGN->removeAllCalledFunctions();

    // Remove any edges from the external node to the function's call graph
    // node.  These edges might have been made irrelegant due to
    // optimization of the program.
    CG.getExternalCallingNode()->removeAnyCallEdgeTo(CGN);

    // Removing the node for callee from the call graph and delete it.
    FunctionsToRemove.push_back(CGN);
  };

  // Scan for all of the functions, looking for ones that should now be removed
  // from the program.  Insert the dead ones in the FunctionsToRemove set.
  for (const auto &I : CG) {
    CallGraphNode *CGN = I.second.get();
    Function *F = CGN->getFunction();
    if (!F || F->isDeclaration())
      continue;

    // Handle the case when this function is called and we only want to care
    // about always-inline functions. This is a bit of a hack to share code
    // between here and the InlineAlways pass.
    if (AlwaysInlineOnly && !F->hasFnAttribute(llvm::Attribute::AlwaysInline))
      continue;

    // If the only remaining users of the function are dead constants, remove
    // them.
    F->removeDeadConstantUsers();

    if (!F->isDefTriviallyDead())
      continue;

    // It is unsafe to drop a function with discardable linkage from a COMDAT
    // without also dropping the other members of the COMDAT.
    // The inliner doesn't visit non-function entities which are in COMDAT
    // groups so it is unsafe to do so *unless* the linkage is local.
    if (!F->hasLocalLinkage()) {
      if (F->hasComdat()) {
        DeadFunctionsInComdats.push_back(F);
        continue;
      }
    }

    RemoveCGN(CGN);
  }
  if (!DeadFunctionsInComdats.empty()) {
    // Filter out the functions whose comdats remain alive.
    filterDeadComdatFunctions(DeadFunctionsInComdats);
    // Remove the rest.
    for (Function *F : DeadFunctionsInComdats)
      RemoveCGN(CG[F]);
  }

  if (FunctionsToRemove.empty())
    return false;

  // Now that we know which functions to delete, do so.  We didn't want to do
  // this inline, because that would invalidate our CallGraph::iterator
  // objects. :(
  //
  // Note that it doesn't matter that we are iterating over a non-stable order
  // here to do this, it doesn't matter which order the functions are deleted
  // in.
  array_pod_sort(FunctionsToRemove.begin(), FunctionsToRemove.end());
  FunctionsToRemove.erase(std::unique(FunctionsToRemove.begin(), FunctionsToRemove.end()), FunctionsToRemove.end());
  for (CallGraphNode *CGN : FunctionsToRemove) {
    delete CG.removeFunctionFromModule(CGN);
  }
  return true;
}

void mergeInlinedArrayAllocas(Function *Caller, InlineFunctionInfo &IFI, InlinedArrayAllocasTy &InlinedArrayAllocas,
                              int InlineHistory) {
  SmallPtrSet<AllocaInst *, 16> UsedAllocas;

  // When processing our SCC, check to see if the call site was inlined from
  // some other call site.  For example, if we're processing "A" in this code:
  //   A() { B() }
  //   B() { x = alloca ... C() }
  //   C() { y = alloca ... }
  // Assume that C was not inlined into B initially, and so we're processing A
  // and decide to inline B into A.  Doing this makes an alloca available for
  // reuse and makes a callsite (C) available for inlining.  When we process
  // the C call site we don't want to do any alloca merging between X and Y
  // because their scopes are not disjoint.  We could make this smarter by
  // keeping track of the inline history for each alloca in the
  // InlinedArrayAllocas but this isn't likely to be a significant win.
  if (InlineHistory != -1) // Only do merging for top-level call sites in SCC.
    return;

  // Loop over all the allocas we have so far and see if they can be merged with
  // a previously inlined alloca.  If not, remember that we had it.
  for (unsigned AllocaNo = 0, E = IFI.StaticAllocas.size(); AllocaNo != E; ++AllocaNo) {
    AllocaInst *AI = IFI.StaticAllocas[AllocaNo];

    // Don't bother trying to merge array allocations (they will usually be
    // canonicalized to be an allocation *of* an array), or allocations whose
    // type is not itself an array (because we're afraid of pessimizing SRoA).
    ArrayType *ATy = dyn_cast<ArrayType>(AI->getAllocatedType());
    if (!ATy || AI->isArrayAllocation())
      continue;

    // Get the list of all available allocas for this array type.
    std::vector<AllocaInst *> &AllocasForType = InlinedArrayAllocas[ATy];

    // Loop over the allocas in AllocasForType to see if we can reuse one.  Note
    // that we have to be careful not to reuse the same "available" alloca for
    // multiple different allocas that we just inlined, we use the 'UsedAllocas'
    // set to keep track of which "available" allocas are being used by this
    // function.  Also, AllocasForType can be empty of course!
    bool MergedAwayAlloca = false;
    for (AllocaInst *AvailableAlloca : AllocasForType) {
      Align Align1 = AI->getAlign();
      Align Align2 = AvailableAlloca->getAlign();

      // The available alloca has to be in the right function, not in some other
      // function in this SCC.
      if (AvailableAlloca->getParent() != AI->getParent())
        continue;

      // If the inlined function already uses this alloca then we can't reuse
      // it.
      if (!UsedAllocas.insert(AvailableAlloca).second)
        continue;

      // Otherwise, we *can* reuse it, RAUW AI into AvailableAlloca and declare
      // success!
      LLVM_DEBUG(dbgs() << "    ***MERGED ALLOCA: " << *AI << "\n\t\tINTO: " << *AvailableAlloca << '\n');

      // Move affected dbg.declare calls immediately after the new alloca to
      // avoid the situation when a dbg.declare precedes its alloca.
      if (auto *L = LocalAsMetadata::getIfExists(AI))
        if (auto *MDV = MetadataAsValue::getIfExists(AI->getContext(), L))
          for (User *U : MDV->users())
            if (DbgDeclareInst *DDI = dyn_cast<DbgDeclareInst>(U))
              DDI->moveBefore(AvailableAlloca->getNextNode());

      AI->replaceAllUsesWith(AvailableAlloca);

      if (Align1 > Align2)
        AvailableAlloca->setAlignment(AI->getAlign());

      AI->eraseFromParent();
      MergedAwayAlloca = true;
      IFI.StaticAllocas[AllocaNo] = nullptr;
      break;
    }

    // If we already nuked the alloca, we're done with it.
    if (MergedAwayAlloca)
      continue;

    // If we were unable to merge away the alloca either because there are no
    // allocas of the right type available or because we reused them all
    // already, remember that this alloca came from an inlined function and mark
    // it used so we don't reuse it for other allocas from this inline
    // operation.
    AllocasForType.push_back(AI);
    UsedAllocas.insert(AI);
  }
}

bool inlineHistoryIncludes(Function *F, int InlineHistoryID,
                           const SmallVectorImpl<std::pair<Function *, int>> &InlineHistory) {
  while (InlineHistoryID != -1) {
    assert(unsigned(InlineHistoryID) < InlineHistory.size() && "Invalid inline history ID");
    if (InlineHistory[InlineHistoryID].first == F)
      return true;
    InlineHistoryID = InlineHistory[InlineHistoryID].second;
  }
  return false;
}

static cl::opt<bool> DisableInlinedAllocaMerging("igc-disable-inlined-alloca-merging", cl::init(false), cl::Hidden);
InlineResult inlineCallIfPossible(CallBase &CB, InlineFunctionInfo &IFI, InlinedArrayAllocasTy &InlinedArrayAllocas,
                                  int InlineHistory, bool InsertLifetime,
                                  function_ref<AAResults &(Function &)> &AARGetter,
                                  ImportedFunctionsInliningStatistics &ImportedFunctionsStats) {
  Function *Callee = CB.getCalledFunction();
  Function *Caller = CB.getCaller();

  AAResults &AAR = AARGetter(*Callee);

  // Try to inline the function.  Get the list of static allocas that were
  // inlined.
  InlineResult IR =
#if LLVM_VERSION_MAJOR < 16
      llvm::InlineFunction(CB, IFI, &AAR, InsertLifetime);
#else
      llvm::InlineFunction(CB, IFI,
                           /*MergeAttributes=*/true, &AAR, InsertLifetime);
#endif

  if (!IR.isSuccess())
    return IR;

  if (!DisableInlinedAllocaMerging)
    mergeInlinedArrayAllocas(Caller, IFI, InlinedArrayAllocas, InlineHistory);

  return IR; // success
}

bool inlineCallsImpl(CallGraphSCC &SCC, CallGraph &CG, std::function<AssumptionCache &(Function &)> GetAssumptionCache,
                     ProfileSummaryInfo *PSI, std::function<const TargetLibraryInfo &(Function &)> GetTLI,
                     bool InsertLifetime, function_ref<InlineCost(CallBase &CB)> GetInlineCost,
                     function_ref<AAResults &(Function &)> AARGetter,
                     ImportedFunctionsInliningStatistics &ImportedFunctionsStats) {
  SmallPtrSet<Function *, 8> SCCFunctions;
  LLVM_DEBUG(dbgs() << "Inliner visiting SCC:");
  for (CallGraphNode *Node : SCC) {
    Function *F = Node->getFunction();
    if (F)
      SCCFunctions.insert(F);
    LLVM_DEBUG(dbgs() << " " << (F ? F->getName() : "INDIRECTNODE"));
  }

  // Scan through and identify all call sites ahead of time so that we only
  // inline call sites in the original functions, not call sites that result
  // from inlining other functions.
  SmallVector<std::pair<CallBase *, int>, 16> CallSites;

  // When inlining a callee produces new call sites, we want to keep track of
  // the fact that they were inlined from the callee.  This allows us to avoid
  // infinite inlining in some obscure cases.  To represent this, we use an
  // index into the InlineHistory vector.
  SmallVector<std::pair<Function *, int>, 8> InlineHistory;

  for (CallGraphNode *Node : SCC) {
    Function *F = Node->getFunction();
    if (!F || F->isDeclaration())
      continue;

    OptimizationRemarkEmitter ORE(F);
    for (BasicBlock &BB : *F)
      for (Instruction &I : BB) {
        auto *CB = dyn_cast<CallBase>(&I);
        // If this isn't a call, or it is a call to an intrinsic, it can
        // never be inlined.
        if (!CB || isa<IntrinsicInst>(I))
          continue;

        // If this is a direct call to an external function, we can never inline
        // it.  If it is an indirect call, inlining may resolve it to be a
        // direct call, so we keep it.
        if (Function *Callee = CB->getCalledFunction())
          if (Callee->isDeclaration()) {
            setInlineRemark(*CB, "unavailable definition");
            ORE.emit([&]() {
              return llvm::OptimizationRemarkMissed(DEBUG_TYPE, "NoDefinition", &I)
                     << ore::NV("Callee", Callee) << " will not be inlined into " << ore::NV("Caller", CB->getCaller())
                     << " because its definition is unavailable" << ore::setIsVerbose();
            });
            continue;
          }

        CallSites.push_back(std::make_pair(CB, -1));
      }
  }

  LLVM_DEBUG(dbgs() << ": " << CallSites.size() << " call sites.\n");

  // If there are no calls in this function, exit early.
  if (CallSites.empty())
    return false;

  // Now that we have all of the call sites, move the ones to functions in the
  // current SCC to the end of the list.
  unsigned FirstCallInSCC = CallSites.size();
  for (unsigned I = 0; I < FirstCallInSCC; ++I)
    if (Function *F = CallSites[I].first->getCalledFunction())
      if (SCCFunctions.count(F))
        std::swap(CallSites[I--], CallSites[--FirstCallInSCC]);

  InlinedArrayAllocasTy InlinedArrayAllocas;
  InlineFunctionInfo InlineInfo(
#if LLVM_VERSION_MAJOR <= 16 || defined(IGC_LLVM_TRUNK_REVISION)
      &CG,
#endif
      GetAssumptionCache, PSI);

  // Now that we have all of the call sites, loop over them and inline them if
  // it looks profitable to do so.
  bool Changed = false;
  bool LocalChange;
  do {
    LocalChange = false;
    // Iterate over the outer loop because inlining functions can cause indirect
    // calls to become direct calls.
    // CallSites may be modified inside so ranged for loop can not be used.
    for (unsigned CSi = 0; CSi != CallSites.size(); ++CSi) {
      auto &P = CallSites[CSi];
      CallBase &CB = *P.first;
      const int InlineHistoryID = P.second;

      if (!CB.getParent())
        continue;

      Function *Caller = CB.getCaller();
      Function *Callee = CB.getCalledFunction();

      // We can only inline direct calls to non-declarations.
      if (!Callee || Callee->isDeclaration())
        continue;

      bool IsTriviallyDead = isInstructionTriviallyDead(&CB, &GetTLI(*Caller));

      if (!IsTriviallyDead) {
        // If this call site was obtained by inlining another function, verify
        // that the include path for the function did not include the callee
        // itself.  If so, we'd be recursively inlining the same function,
        // which would provide the same callsites, which would cause us to
        // infinitely inline.
        if (InlineHistoryID != -1 && inlineHistoryIncludes(Callee, InlineHistoryID, InlineHistory)) {
          setInlineRemark(CB, "recursive");
          continue;
        }
      }

      // FIXME for new PM: because of the old PM we currently generate ORE and
      // in turn BFI on demand.  With the new PM, the ORE dependency should
      // just become a regular analysis dependency.
      llvm::OptimizationRemarkEmitter ORE(Caller);

      auto OIC = shouldInline(CB, GetInlineCost, ORE);
      // If the policy determines that we should inline this function,
      // delete the call instead.
      if (!OIC)
        continue;

      // If this call site is dead and it is to a readonly function, we should
      // just delete the call instead of trying to inline it, regardless of
      // size.  This happens because IPSCCP propagates the result out of the
      // call and then we're left with the dead call.
      if (IsTriviallyDead) {
        LLVM_DEBUG(dbgs() << "    -> Deleting dead call: " << CB << "\n");
        // Update the call graph by deleting the edge from Callee to Caller.
        setInlineRemark(CB, "trivially dead");
        CG[Caller]->removeCallEdgeFor(CB);
        CB.eraseFromParent();
      } else {
        // Get DebugLoc to report. CB will be invalid after Inliner.
        DebugLoc DLoc = CB.getDebugLoc();
        BasicBlock *Block = CB.getParent();

        // Attempt to inline the function.
        using namespace ore;

        InlineResult IR = inlineCallIfPossible(CB, InlineInfo, InlinedArrayAllocas, InlineHistoryID, InsertLifetime,
                                               AARGetter, ImportedFunctionsStats);
        if (!IR.isSuccess()) {
          setInlineRemark(CB, std::string(IR.getFailureReason()) + "; " + inlineCostStr(*OIC));
          ORE.emit([&]() {
            return llvm::OptimizationRemarkMissed(DEBUG_TYPE, "NotInlined", DLoc, Block)
                   << NV("Callee", Callee) << " will not be inlined into " << NV("Caller", Caller) << ": "
                   << NV("Reason", IR.getFailureReason());
          });
          continue;
        }

        emitInlinedIntoBasedOnCost(ORE, DLoc, Block, *Callee, *Caller, *OIC);

        // If inlining this function gave us any new call sites, throw them
        // onto our worklist to process.  They are useful inline candidates.
        if (!InlineInfo.InlinedCalls.empty()) {
          // Create a new inline history entry for this, so that we remember
          // that these new callsites came about due to inlining Callee.
          int NewHistoryID = InlineHistory.size();
          InlineHistory.push_back(std::make_pair(Callee, InlineHistoryID));

#ifndef NDEBUG
          // Make sure no dupplicates in the inline candidates. This could
          // happen when a callsite is simpilfied to reusing the return value
          // of another callsite during function cloning, thus the other
          // callsite will be reconsidered here.
          DenseSet<CallBase *> DbgCallSites;
          for (auto &II : CallSites)
            DbgCallSites.insert(II.first);
#endif

          for (Value *Ptr : InlineInfo.InlinedCalls) {
#ifndef NDEBUG
            assert(DbgCallSites.count(dyn_cast<CallBase>(Ptr)) == 0);
#endif
            CallSites.push_back(std::make_pair(dyn_cast<CallBase>(Ptr), NewHistoryID));
          }
        }
      }

      // If we inlined or deleted the last possible call site to the function,
      // delete the function body now.
      if (Callee && Callee->use_empty() && Callee->hasLocalLinkage() &&
          // TODO: Can remove if in SCC now.
          !SCCFunctions.count(Callee) &&
          // The function may be apparently dead, but if there are indirect
          // callgraph references to the node, we cannot delete it yet, this
          // could invalidate the CGSCC iterator.
          CG[Callee]->getNumReferences() == 0) {
        LLVM_DEBUG(dbgs() << "    -> Deleting dead function: " << Callee->getName() << "\n");
        CallGraphNode *CalleeNode = CG[Callee];

        // Remove any call graph edges from the callee to its callees.
        CalleeNode->removeAllCalledFunctions();

        // Removing the node for callee from the call graph and delete it.
        delete CG.removeFunctionFromModule(CalleeNode);
      }

      // Remove this call site from the list.  If possible, use
      // swap/pop_back for efficiency, but do not use it if doing so would
      // move a call site to a function in this SCC before the
      // 'FirstCallInSCC' barrier.
      if (SCC.isSingular()) {
        CallSites[CSi] = CallSites.back();
        CallSites.pop_back();
      } else {
        CallSites.erase(CallSites.begin() + CSi);
      }
      --CSi;

      Changed = true;
      LocalChange = true;
    }
  } while (LocalChange);

  return Changed;
}

AAResults createLegacyPMAAResults(Pass &P, Function &F, BasicAAResult &BAR) {
  AAResults AAR(P.getAnalysis<TargetLibraryInfoWrapperPass>().getTLI());
  AAR.addAAResult(BAR);

  // Populate the results with the other currently available AAs.
  if (auto *WrapperPass = P.getAnalysisIfAvailable<ScopedNoAliasAAWrapperPass>())
    AAR.addAAResult(WrapperPass->getResult());
  if (auto *WrapperPass = P.getAnalysisIfAvailable<TypeBasedAAWrapperPass>())
    AAR.addAAResult(WrapperPass->getResult());
  if (auto *WrapperPass = P.getAnalysisIfAvailable<GlobalsAAWrapperPass>())
    AAR.addAAResult(WrapperPass->getResult());
  if (auto *WrapperPass = P.getAnalysisIfAvailable<ExternalAAWrapperPass>())
    if (WrapperPass->CB)
      WrapperPass->CB(P, F, AAR);

  return AAR;
}

BasicAAResult createLegacyPMBasicAAResult(Pass &P, Function &F) {
  return BasicAAResult(F.getParent()->getDataLayout(), F, P.getAnalysis<TargetLibraryInfoWrapperPass>().getTLI(),
                       P.getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F));
}

} // namespace IGCLLVM
