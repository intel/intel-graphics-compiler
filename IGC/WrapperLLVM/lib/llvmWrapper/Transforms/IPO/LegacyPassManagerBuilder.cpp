/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Transforms/IPO/ElimAvailExtern.h"
#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
#include "llvm/Analysis/ScopedNoAliasAA.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/ForceFunctionAttrs.h"
#include "llvm/Transforms/IPO/FunctionAttrs.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/LICM.h"
#include "llvm/Transforms/Scalar/SimpleLoopUnswitch.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Transforms/Vectorize.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/IPO/LegacyPassManagerBuilder.h"
#include "llvmWrapper/Transforms/IPO/StripDeadPrototypes.h"
#include "llvmWrapper/Transforms/Scalar/LoopDeletion.h"
#include "llvmWrapper/Transforms/IPO/ElimAvailExtern.h"
#include "llvmWrapper/Transforms/IPO/ConstantMerge.h"
#include "llvmWrapper/Transforms/Scalar/LoopIdiomRecognize.h"
#include "llvmWrapper/Transforms/Utils/LibCallsShrinkWrap.h"
#include "llvmWrapper/Transforms/Vectorize/LoopVectorize.h"
#include "llvmWrapper/Transforms/Vectorize/SLPVectorizer.h"
#include "llvmWrapper/Transforms/Vectorize/VectorCombine.h"
#include "llvmWrapper/Transforms/Scalar/WarnMissedTransforms.h"
#include "llvmWrapper/Transforms/Scalar/DivRemPairs.h"
#include "llvmWrapper/Transforms/Scalar/JumpThreading.h"
#include "llvmWrapper/Transforms/Scalar/SCCP.h"
#include "llvmWrapper/Transforms/IPO/SCCP.h"
#include "llvmWrapper/Transforms/IPO/GlobalDCE.h"
#include "llvmWrapper/Transforms/Scalar/CorrelatedValuePropagation.h"
#include "llvmWrapper/Transforms/Scalar/DeadStoreElimination.h"
#include "llvmWrapper/Transforms/Scalar/LoopLoadElimination.h"
#include "llvmWrapper/Transforms/IPO/Annotation2Metadata.h"
#include "llvmWrapper/Transforms/IPO/ForceFunctionAttrs.h"
#include "llvmWrapper/Transforms/Scalar/ADCE.h"
#include "llvmWrapper/Transforms/Scalar/BDCE.h"
#include "llvmWrapper/Transforms/Scalar/AlignmentFromAssumptions.h"
#include "llvmWrapper/Transforms/Scalar/LICM.h"
#include "llvmWrapper/Transforms/Scalar/CallSiteSplitting.h"
#include "llvmWrapper/Transforms/IPO/CalledValuePropagation.h"
#include "llvmWrapper/Transforms/IPO/GlobalOpt.h"
#include "llvmWrapper/Transforms/IPO/FunctionAttrs.h"
#include "llvmWrapper/Transforms/Scalar/Float2Int.h"
#include "llvmWrapper/Transforms/Scalar/LoopDistribute.h"
#include "llvmWrapper/Transforms/IPO/PostOrderFunctionAttrs.h"
#include "llvmWrapper/Transforms/Scalar/MemCpyOptimizer.h"
#include "llvmWrapper/Transforms/Scalar/LoopUnrollPass.h"
#include "llvmWrapper/Transforms/Scalar/IndVarSimplify.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR >= 16
PassManagerBuilder::PassManagerBuilder() {
  OptLevel = 2;
  SizeLevel = 0;
  Inliner = nullptr;
  DisableUnrollLoops = false;
  SLPVectorize = false;
  LoopVectorize = true;
  LicmMssaOptCap = SetLicmMssaOptCap;
  LicmMssaNoAccForPromotionCap = SetLicmMssaNoAccForPromotionCap;
  ForgetAllSCEVInLoopUnroll = ForgetSCEVInLoopUnroll;
}

PassManagerBuilder::~PassManagerBuilder() { delete Inliner; }

void PassManagerBuilder::addInitialAliasAnalysisPasses(legacy::PassManagerBase &PM) const {
  // Add TypeBasedAliasAnalysis before BasicAliasAnalysis so that
  // BasicAliasAnalysis wins if they disagree. This is intended to help
  // support "obvious" type-punning idioms.
  PM.add(createTypeBasedAAWrapperPass());
  PM.add(createScopedNoAliasAAWrapperPass());
}

void PassManagerBuilder::populateFunctionPassManager(legacy::FunctionPassManager &FPM) {
  if (OptLevel == 0)
    return;
  addInitialAliasAnalysisPasses(FPM);

  // Lower llvm.expect to metadata before attempting transforms.
  // Compare/branch metadata may alter the behavior of passes like SimplifyCFG.
  FPM.add(createLowerExpectIntrinsicPass());
  FPM.add(createCFGSimplificationPass());
  FPM.add(createSROAPass());
  FPM.add(createEarlyCSEPass());
}

void PassManagerBuilder::addFunctionSimplificationPasses(legacy::PassManagerBase &MPM) {
  // Start of function pass.
  // Break up aggregate allocas, using SSAUpdater.
  assert(OptLevel >= 1 && "Calling function optimizer with no optimization level!");
  MPM.add(createSROAPass());
  MPM.add(createEarlyCSEPass(true /* Enable mem-ssa. */)); // Catch trivial redundancies

  if (OptLevel > 1) {
    // Speculative execution if the target has divergent branches; otherwise nop.
    MPM.add(createSpeculativeExecutionIfHasBranchDivergencePass());

    MPM.add(IGCLLVM::createLegacyWrappedJumpThreadingPass());              // Thread jumps.
    MPM.add(IGCLLVM::createLegacyWrappedCorrelatedValuePropagationPass()); // Propagate conditionals
  }
  MPM.add(createCFGSimplificationPass(SimplifyCFGOptions().convertSwitchRangeToICmp(true))); // Merge & remove BBs
  // Combine silly seq's
  MPM.add(createInstructionCombiningPass());
  if (SizeLevel == 0)
    MPM.add(IGCLLVM::createLegacyWrappedLibCallsShrinkWrapPass());

  // TODO: Investigate the cost/benefit of tail call elimination on debugging.
  if (OptLevel > 1)
    MPM.add(createTailCallEliminationPass());                                                // Eliminate tail
                                                                                             // calls
  MPM.add(createCFGSimplificationPass(SimplifyCFGOptions().convertSwitchRangeToICmp(true))); // Merge & remove BBs
  MPM.add(createReassociatePass());                                                          // Reassociate expressions

  // Begin the loop pass pipeline.

  // The simple loop unswitch pass relies on separate cleanup passes. Schedule
  // them first so when we re-process a loop they run before other loop
  // passes.
  MPM.add(createLoopInstSimplifyPass());
  MPM.add(createLoopSimplifyCFGPass());

  // Try to remove as much code from the loop header as possible,
  // to reduce amount of IR that will have to be duplicated. However,
  // do not perform speculative hoisting the first time as LICM
  // will destroy metadata that may not need to be destroyed if run
  // after loop rotation.
  // TODO: Investigate promotion cap for O1.
  MPM.add(IGCLLVM::createLegacyWrappedLICMPass(LicmMssaOptCap, LicmMssaNoAccForPromotionCap,
                                               /*AllowSpeculation=*/false));
  // Rotate Loop - disable header duplication at -Oz
  MPM.add(createLoopRotatePass(SizeLevel == 2 ? 0 : -1, false));
  // TODO: Investigate promotion cap for O1.
  MPM.add(IGCLLVM::createLegacyWrappedLICMPass(LicmMssaOptCap, LicmMssaNoAccForPromotionCap,
                                               /*AllowSpeculation=*/true));
  MPM.add(createSimpleLoopUnswitchLegacyPass(OptLevel == 3));
  // FIXME: We break the loop pass pipeline here in order to do full
  // simplifycfg. Eventually loop-simplifycfg should be enhanced to replace the
  // need for this.
  MPM.add(createCFGSimplificationPass(SimplifyCFGOptions().convertSwitchRangeToICmp(true)));
  MPM.add(createInstructionCombiningPass());
  // We resume loop passes creating a second loop pipeline here.
  MPM.add(IGCLLVM::createLegacyWrappedLoopIdiomRecognizePass()); // Recognize idioms like memset.
  MPM.add(IGCLLVM::createLegacyWrappedIndVarSimplifyPass());     // Canonicalize indvars
  MPM.add(IGCLLVM::createLegacyWrappedLoopDeletionPass());       // Delete dead loops

  // Unroll small loops and perform peeling.
  MPM.add(IGCLLVM::createLegacyWrappedSimpleLoopUnrollPass(OptLevel, DisableUnrollLoops, ForgetAllSCEVInLoopUnroll));
  // This ends the loop pass pipelines.

  // Break up allocas that may now be splittable after loop unrolling.
  MPM.add(createSROAPass());

  if (OptLevel > 1) {
    MPM.add(createMergedLoadStoreMotionPass()); // Merge ld/st in diamonds
    MPM.add(createGVNPass(false));              // Remove redundancies
  }
  MPM.add(IGCLLVM::createLegacyWrappedSCCPPass()); // Constant prop with SCCP

  // Delete dead bit computations (instcombine runs after to fold away the dead
  // computations, and then ADCE will run later to exploit any new DCE
  // opportunities that creates).
  MPM.add(IGCLLVM::createLegacyWrappedBDCEPass()); // Delete dead bit computations

  // Run instcombine after redundancy elimination to exploit opportunities
  // opened up by them.
  MPM.add(createInstructionCombiningPass());
  if (OptLevel > 1) {
    MPM.add(IGCLLVM::createLegacyWrappedJumpThreadingPass()); // Thread jumps
    MPM.add(IGCLLVM::createLegacyWrappedCorrelatedValuePropagationPass());
  }
  MPM.add(IGCLLVM::createLegacyWrappedADCEPass()); // Delete dead instructions

  MPM.add(IGCLLVM::createLegacyWrappedMemCpyOptPass()); // Remove memcpy / form memset
  // TODO: Investigate if this is too expensive at O1.
  if (OptLevel > 1) {
    MPM.add(IGCLLVM::createLegacyWrappedDeadStoreEliminationPass()); // Delete dead stores
    MPM.add(IGCLLVM::createLegacyWrappedLICMPass(LicmMssaOptCap, LicmMssaNoAccForPromotionCap,
                                                 /*AllowSpeculation=*/true));
  }

  // Merge & remove BBs and sink & hoist common instructions.
  MPM.add(createCFGSimplificationPass(SimplifyCFGOptions().hoistCommonInsts(true).sinkCommonInsts(true)));
  // Clean up after everything.
  MPM.add(createInstructionCombiningPass());
}

/// FIXME: Should LTO cause any differences to this set of passes?
void PassManagerBuilder::addVectorPasses(legacy::PassManagerBase &PM, bool IsFullLTO) {
  PM.add(IGCLLVM::createLegacyWrappedLoopVectorizePass());

  if (IsFullLTO) {
    // The vectorizer may have significantly shortened a loop body; unroll
    // again. Unroll small loops to hide loop backedge latency and saturate any
    // parallel execution resources of an out-of-order processor. We also then
    // need to clean up redundancies and loop invariant code.
    // FIXME: It would be really good to use a loop-integrated instruction
    // combiner for cleanup here so that the unrolling and LICM can be pipelined
    // across the loop nests.
    PM.add(IGCLLVM::createLegacyWrappedLoopUnrollPass(OptLevel, DisableUnrollLoops, ForgetAllSCEVInLoopUnroll));
    PM.add(IGCLLVM::createLegacyWrappedWarnMissedTransformsPass());
  }

  if (!IsFullLTO) {
    // Eliminate loads by forwarding stores from the previous iteration to loads
    // of the current iteration.
    PM.add(IGCLLVM::createLegacyWrappedLoopLoadEliminationPass());
  }
  // Cleanup after the loop optimization passes.
  PM.add(createInstructionCombiningPass());

  // Now that we've formed fast to execute loop structures, we do further
  // optimizations. These are run afterward as they might block doing complex
  // analyses and transforms such as what are needed for loop vectorization.

  // Cleanup after loop vectorization, etc. Simplification passes like CVP and
  // GVN, loop transforms, and others have already run, so it's now better to
  // convert to more optimized IR using more aggressive simplify CFG options.
  // The extra sinking transform can create larger basic blocks, so do this
  // before SLP vectorization.
  PM.add(createCFGSimplificationPass(SimplifyCFGOptions()
                                         .forwardSwitchCondToPhi(true)
                                         .convertSwitchRangeToICmp(true)
                                         .convertSwitchToLookupTable(true)
                                         .needCanonicalLoops(false)
                                         .hoistCommonInsts(true)
                                         .sinkCommonInsts(true)));

  if (IsFullLTO) {
    PM.add(IGCLLVM::createLegacyWrappedSCCPPass()); // Propagate exposed constants
    PM.add(createInstructionCombiningPass()); // Clean up again
    PM.add(IGCLLVM::createLegacyWrappedBDCEPass());
  }

  // Optimize parallel scalar instruction chains into SIMD instructions.
  if (SLPVectorize) {
    PM.add(IGCLLVM::createLegacyWrappedSLPVectorizerPass());
  }

  // Enhance/cleanup vector code.
  PM.add(IGCLLVM::createLegacyWrappedVectorCombinePass());

  if (!IsFullLTO) {
    PM.add(createInstructionCombiningPass());

    // Unroll small loops
    PM.add(IGCLLVM::createLegacyWrappedLoopUnrollPass(OptLevel, DisableUnrollLoops, ForgetAllSCEVInLoopUnroll));

    if (!DisableUnrollLoops) {
      // LoopUnroll may generate some redundency to cleanup.
      PM.add(createInstructionCombiningPass());

      // Runtime unrolling will introduce runtime check in loop prologue. If the
      // unrolled loop is a inner loop, then the prologue will be inside the
      // outer loop. LICM pass can help to promote the runtime check out if the
      // checked value is loop invariant.
      PM.add(IGCLLVM::createLegacyWrappedLICMPass(LicmMssaOptCap, LicmMssaNoAccForPromotionCap,
                                                  /*AllowSpeculation=*/true));
    }

    PM.add(IGCLLVM::createLegacyWrappedWarnMissedTransformsPass());
  }

  // After vectorization and unrolling, assume intrinsics may tell us more
  // about pointer alignments.
  PM.add(IGCLLVM::createLegacyWrappedAlignmentFromAssumptionsPass());

  if (IsFullLTO)
    PM.add(createInstructionCombiningPass());
}

void PassManagerBuilder::populateModulePassManager(legacy::PassManagerBase &MPM) {
  MPM.add(IGCLLVM::createLegacyWrappedAnnotation2MetadataPass());
  // Allow forcing function attributes as a debugging and tuning aid.
  MPM.add(IGCLLVM::createLegacyWrappedForceFunctionAttrsPass());

  // If all optimizations are disabled, just run the always-inline pass and,
  // if enabled, the function merging pass.
  if (OptLevel == 0) {
    if (Inliner) {
      MPM.add(Inliner);
      Inliner = nullptr;
    }
    return;
  }

  addInitialAliasAnalysisPasses(MPM);
  if (OptLevel > 2)
    MPM.add(IGCLLVM::createLegacyWrappedCallSiteSplittingPass());

  MPM.add(IGCLLVM::createLegacyWrappedIPSCCPPass()); // IP SCCP
  MPM.add(IGCLLVM::createLegacyWrappedCalledValuePropagationPass());

  MPM.add(IGCLLVM::createLegacyWrappedGlobalOptPass()); // Optimize out global vars
  // Promote any localized global vars.
  MPM.add(createPromoteMemoryToRegisterPass());

  MPM.add(createDeadArgEliminationPass()); // Dead argument elimination

  MPM.add(createInstructionCombiningPass()); // Clean up after IPCP & DAE
  MPM.add(
      createCFGSimplificationPass(SimplifyCFGOptions().convertSwitchRangeToICmp(true))); // Clean up after IPCP & DAE

  // We add a module alias analysis pass here. In part due to bugs in the
  // analysis infrastructure this "works" in that the analysis stays alive
  // for the entire SCC pass run below.
  MPM.add(createGlobalsAAWrapperPass());

  // Start of CallGraph SCC passes.
  bool RunInliner = false;
  if (Inliner) {
    MPM.add(Inliner);
    Inliner = nullptr;
    RunInliner = true;
  }

  MPM.add(IGCLLVM::createLegacyWrappedPostOrderFunctionAttrsPass());

  addFunctionSimplificationPasses(MPM);

  // FIXME: This is a HACK! The inliner pass above implicitly creates a CGSCC
  // pass manager that we are specifically trying to avoid. To prevent this
  // we must insert a no-op module pass to reset the pass manager.
  MPM.add(createBarrierNoopPass());

  if (OptLevel > 1)
    // Remove avail extern fns and globals definitions if we aren't
    // compiling an object file for later LTO. For LTO we want to preserve
    // these so they are eligible for inlining at link-time. Note if they
    // are unreferenced they will be removed by GlobalDCE later, so
    // this only impacts referenced available externally globals.
    // Eventually they will be suppressed during codegen, but eliminating
    // here enables more opportunity for GlobalDCE as it may make
    // globals referenced by available external functions dead
    // and saves running remaining passes on the eliminated functions.
    MPM.add(IGCLLVM::createLegacyWrappedEliminateAvailableExternallyPass());

  MPM.add(IGCLLVM::createLegacyWrappedReversePostOrderFunctionAttrsPass());

  // The inliner performs some kind of dead code elimination as it goes,
  // but there are cases that are not really caught by it. We might
  // at some point consider teaching the inliner about them, but it
  // is OK for now to run GlobalOpt + GlobalDCE in tandem as their
  // benefits generally outweight the cost, making the whole pipeline
  // faster.
  if (RunInliner) {
    MPM.add(IGCLLVM::createLegacyWrappedGlobalOptPass());
    MPM.add(IGCLLVM::createLegacyWrappedGlobalDCEPass());
  }

  // We add a fresh GlobalsModRef run at this point. This is particularly
  // useful as the above will have inlined, DCE'ed, and function-attr
  // propagated everything. We should at this point have a reasonably minimal
  // and richly annotated call graph. By computing aliasing and mod/ref
  // information for all local globals here, the late loop passes and notably
  // the vectorizer will be able to use them to help recognize vectorizable
  // memory operations.
  //
  // Note that this relies on a bug in the pass manager which preserves
  // a module analysis into a function pass pipeline (and throughout it) so
  // long as the first function pass doesn't invalidate the module analysis.
  // Thus both Float2Int and LoopRotate have to preserve AliasAnalysis for
  // this to work. Fortunately, it is trivial to preserve AliasAnalysis
  // (doing nothing preserves it as it is required to be conservatively
  // correct in the face of IR changes).
  MPM.add(createGlobalsAAWrapperPass());

  MPM.add(IGCLLVM::createLegacyWrappedFloat2IntPass());
  MPM.add(createLowerConstantIntrinsicsPass());

  // Re-rotate loops in all our loop nests. These may have fallout out of
  // rotated form due to GVN or other transformations, and the vectorizer relies
  // on the rotated form. Disable header duplication at -Oz.
  MPM.add(createLoopRotatePass(SizeLevel == 2 ? 0 : -1, false));

  // Distribute loops to allow partial vectorization.  I.e. isolate dependences
  // into separate loop that would otherwise inhibit vectorization.  This is
  // currently only performed for loops marked with the metadata
  // llvm.loop.distribute=true or when -enable-loop-distribute is specified.
  MPM.add(IGCLLVM::createLegacyWrappedLoopDistributePass());

  addVectorPasses(MPM, /* IsFullLTO */ false);

  // FIXME: We shouldn't bother with this anymore.
  MPM.add(IGCLLVM::createLegacyWrappedStripDeadPrototypesPass()); // Get rid of dead prototypes

  // GlobalOpt already deletes dead functions and globals, at -O2 try a
  // late pass of GlobalDCE.  It is capable of deleting dead cycles.
  if (OptLevel > 1) {
    MPM.add(IGCLLVM::createLegacyWrappedGlobalDCEPass());     // Remove dead fns and globals.
    MPM.add(IGCLLVM::createLegacyWrappedConstantMergePass()); // Merge dup global constants
  }

  // LoopSink pass sinks instructions hoisted by LICM, which serves as a
  // canonicalization pass that enables other optimizations. As a result,
  // LoopSink pass needs to be a very late IR pass to avoid undoing LICM
  // result too early.
  MPM.add(createLoopSinkPass());
  // Get rid of LCSSA nodes.
  MPM.add(createInstSimplifyLegacyPass());

  // This hoists/decomposes div/rem ops. It should run after other sink/hoist
  // passes to avoid re-sinking, but before SimplifyCFG because it can allow
  // flattening of blocks.
  MPM.add(IGCLLVM::createLegacyWrappedDivRemPairsPass());

  // LoopSink (and other loop passes since the last simplifyCFG) might have
  // resulted in single-entry-single-exit or empty blocks. Clean up the CFG.
  MPM.add(createCFGSimplificationPass(SimplifyCFGOptions().convertSwitchRangeToICmp(true)));
}
#endif
} // namespace IGCLLVM
