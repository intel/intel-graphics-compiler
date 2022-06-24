/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// GenXLiveness is an analysis that contains the liveness information for the
// values in the code. See the comment at the top of GenXLiveness.h for further
// details.
//
//===----------------------------------------------------------------------===//
#include "GenXLiveness.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXIntrinsics.h"
#include "GenXNumbering.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include "vc/GenXOpts/GenXAnalysis.h"
#include "vc/Utils/GenX/GlobalVariable.h"
#include "vc/Utils/GenX/InternalMetadata.h"
#include "vc/Utils/GenX/PredefinedVariable.h"
#include "vc/Utils/GenX/RegCategory.h"

#include "llvm/GenXIntrinsics/GenXMetadata.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include "llvm/ADT/SmallSet.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "GENX_LIVENESS"

#include <unordered_set>

using namespace llvm;
using namespace genx;

void GenXLiveness::getAnalysisUsage(AnalysisUsage &AU) {
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesAll();
}

/***********************************************************************
 * runOnFunctionGroup : do nothing
 */
bool GenXLiveness::runOnFunctionGroup(FunctionGroup &ArgFG)
{
  LLVM_DEBUG(dbgs() << "init GenXLiveness\n");
  FG = &ArgFG;
  Subtarget = &getAnalysis<TargetPassConfig>()
                   .getTM<GenXTargetMachine>()
                   .getGenXSubtarget();
  DL = &ArgFG.getModule()->getDataLayout();
  return false;
}

/***********************************************************************
 * releaseMemory: clear the GenXLiveness
 */
void GenXLiveness::releaseMemory() {
  LLVM_DEBUG(dbgs() << "releaseMemory for GenXLivness\n");
  while (!LiveRangeMap.empty()) {
    LiveRange *LR = begin()->second;
    for (auto i = LR->value_begin(), e = LR->value_end(); i != e; ++i) {
      SimpleValue V = *i;
      LiveRangeMap.erase(V);
    }
    delete LR;
  }
  FG = 0;
  CG.reset();
  for (auto i = UnifiedRets.begin(), e = UnifiedRets.end(); i != e; ++i)
    i->second->deleteValue();
  UnifiedRets.clear();
  UnifiedRetToFunc.clear();
  ArgAddressBaseMap.clear();
  BaseToArgAddrMap.clear();
}

static vc::RegCategory getCategoryForPredefinedVariable(SimpleValue SV) {
  const vc::RegCategory Category =
      llvm::StringSwitch<vc::RegCategory>(SV.getValue()->getName())
          .Case(vc::PredefVar::BSSName, vc::RegCategory::Surface)
          .Case(vc::PredefVar::ImplicitArgsBufferName, vc::RegCategory::General)
          .Case(vc::PredefVar::LocalIDBufferName, vc::RegCategory::General)
          .Default(vc::RegCategory::None);
  IGC_ASSERT_MESSAGE(Category != vc::RegCategory::None,
                     "Unhandled predefined variable");
  return Category;
}

static bool isPredefinedVariable(SimpleValue SV) {
  Value *V = SV.getValue();
  IGC_ASSERT_MESSAGE(V, "Expected value");
  bool Result = vc::PredefVar::isPV(*V);
  if (Result)
    IGC_ASSERT_MESSAGE(SV.getIndex() == 0,
                       "Expected single simple value for predefined variable");
  return Result;
}

/***********************************************************************
 * getDefaultCategory: get default category based on SimpleValue
 *
 * The default category is PREDICATE for i1 or a vector of i1,
 * Surface for BSSName predefined variable or GENERAL for anything else.
 */
static vc::RegCategory getDefaultCategory(SimpleValue SV) {
  if (isPredefinedVariable(SV))
    return getCategoryForPredefinedVariable(SV);
  Type *Ty =
      IndexFlattener::getElementType(SV.getValue()->getType(), SV.getIndex());
  if (Ty->getScalarType()->isIntegerTy(1))
    return vc::RegCategory::Predicate;
  return vc::RegCategory::General;
}

/***********************************************************************
 * setLiveRange : add a SimpleValue to a LiveRange
 *
 * This:
 * 1. adds the SimpleValue to the LiveRange's value list;
 * 2. sets the SimpleValue's entry in the map to point to the LiveRange.
 */
void GenXLiveness::setLiveRange(SimpleValue V, LiveRange *LR)
{
  IGC_ASSERT_MESSAGE(
      LiveRangeMap.find(V) == end(),
      "Attempting to set LiveRange for Value that already has one");
  LLVM_DEBUG(dbgs() << "Setting LiveRange " << *LR << " for SV: " << V << "\n");
  LR->addValue(V);
  LiveRangeMap[V] = LR;
  LR->setAlignmentFromValue(
      *DL, V, Subtarget ? Subtarget->getGRFByteSize() : defaultGRFByteSize);
}

/***********************************************************************
 * setAlignmentFromValue : set a live range's alignment from a value
 */
void LiveRange::setAlignmentFromValue(const DataLayout &DL, const SimpleValue V,
                                      const unsigned GRFWidth) {
  LLVM_DEBUG(dbgs() << "Setting Alignment for value " << V);
  const unsigned AlignInBytes = getValueAlignmentInBytes(*(V.getValue()), DL);
  const unsigned LogByteAlign = Log2_32(AlignInBytes);
  // Set max alignment to GRF.
  const unsigned MaxLogAlign =
      genx::getLogAlignment(VISA_Align::ALIGN_GRF, GRFWidth);
  const unsigned SaturatedLogAlign = std::clamp(LogByteAlign, 0u, MaxLogAlign);
  unsigned ResultAlign = ceilLogAlignment(SaturatedLogAlign, GRFWidth);
  LLVM_DEBUG(dbgs() << " result: " << ResultAlign << "\n");
  setLogAlignment(ResultAlign);
}

/***********************************************************************
 * rebuildCallGraph : rebuild GenXLiveness's call graph
 */
void GenXLiveness::rebuildCallGraph()
{
  LLVM_DEBUG(dbgs() << "Rebuilding CallGraph\n");
  CG = std::make_unique<genx::CallGraph>(FG);
  CG->build(this);
}

/***********************************************************************
 * buildSubroutineLRs : build the subroutine LRs
 *
 * If the FunctionGroup has subroutines, then each one (each Function other
 * than the head one) gets a "subroutine LR", giving the live range
 * of the whole subroutine plus any other subroutines it can call.
 * Then, when building a real live range later, if it goes over a call,
 * we can add the subroutine LR.
 *
 * The subroutine LR has weak liveness, as that's what we want to add to
 * anything live over a call to the subroutine.
 */
void GenXLiveness::buildSubroutineLRs()
{
  LLVM_DEBUG(dbgs() << "BuildingSubroutineLRs\n");
  if (FG->size() == 1) {
    LLVM_DEBUG(dbgs() << "No subroutines\n");
    return; // no subroutines
  }
  // Build a call graph for the FunctionGroup. It is acyclic because there is
  // no recursion.
  rebuildCallGraph();
  // Depth-first walk the graph to propagate live ranges upwards.
  visitPropagateSLRs(FG->getHead());
}

/***********************************************************************
 * visitPropagateSLRs : visit a callgraph node to propagate subroutine LR
 *
 * This is recursive.
 */
LiveRange *GenXLiveness::visitPropagateSLRs(Function *F)
{
  LLVM_DEBUG(dbgs() << "VisitPropagateSLRs begin\n");
  LiveRange *LR = getOrCreateLiveRange(F);
  // Add a segment for just this function.
  LR->push_back(Segment(Numbering->getNumber(F),
      Numbering->getNumber(F->back().getTerminator()) + 1, Segment::WEAK));
  // For each child...
  genx::CallGraph::Node *N = CG->getNode(F);
  for (auto i = N->begin(), e = N->end(); i != e; ++i) {
    // Visit the child to calculate its LR.
    LiveRange *ChildLR = visitPropagateSLRs(i->Call->getCalledFunction());
    // Merge it into ours.
    LR->addSegments(ChildLR);
  }
  LR->sortAndMerge();
  LLVM_DEBUG(dbgs() << "VisitPropagateSLRs return: " << *LR << "\n");
  return LR;
}

/***********************************************************************
 * buildLiveRange : build live range for one value (arg or non-baled inst)
 *
 * For an aggregate value, each element's live range is built separately, even
 * though they are almost identical. They are not exactly identical,
 * differing at the def if it is the return value of a call, and at a use
 * that is a call arg.
 */
void GenXLiveness::buildLiveRange(Value *V)
{
  LLVM_DEBUG(dbgs() << "Building LiveRange for :" << *V << "\n");
  Type *Ty = V->getType();
  if (!Ty->isAggregateType()) {
    LLVM_DEBUG(dbgs() << "It is not aggregate, build for one\n");
    buildLiveRange(SimpleValue(V));
    return;
  }
  for (unsigned i = 0, e = IndexFlattener::getNumElements(Ty); i != e; ++i) {
    LLVM_DEBUG(dbgs() << "Bulding for aggregate Index " << i << " from " << e
                      << "\n");
    buildLiveRange(SimpleValue(V, i));
  }
}

/***********************************************************************
 * buildLiveRange : build live range for one SimpleValue
 *
 * rebuildLiveRange : rebuild live range for a LiveRange struct
 *
 * The BBs[] array, one entry per basic block, is temporarily used here to
 * store the live range for the value within that block. We start by
 * registering the short live range for the definition, then, for each use,
 * create a live range in the use's block then recursively scan back
 * through predecessors until we meet a block where there is already a
 * live range. This is guaranteed to terminate because of the dominance
 * property of SSA.
 *
 * See Appel "Modern Compiler Implementation in C" 19.6.
 *
 * rebuildLiveRange can be called from later passes to rebuild the segments
 * for a particular live range. If used after coalescing, the live range might
 * have more than one value, in which case segments are added for each value
 * and then merged. Thus we assume that, after whatever code change a pass made
 * to require rebuilding the live range, the coalesced values can still be
 * validly coalesced, without having any way of checking that.
 *
 */
LiveRange *GenXLiveness::buildLiveRange(SimpleValue V)
{
  LLVM_DEBUG(dbgs() << "Building LiveRange for SimpleValue: " << V << "\n");
  LiveRange *LR = getOrCreateLiveRange(V);
  rebuildLiveRange(LR);
  LLVM_DEBUG(dbgs() << "Building LiveRange for SimpleValue return: " << *LR
                    << "\n");
  return LR;
}

void GenXLiveness::rebuildLiveRange(LiveRange *LR)
{
  LLVM_DEBUG(dbgs() << "Rebuilding LiveRange: " << *LR << "\n");
  LR->getOrDefaultCategory();
  LR->Segments.clear();
  for (auto vi = LR->value_begin(), ve = LR->value_end(); vi != ve; ++vi)
    rebuildLiveRangeForValue(LR, *vi);
  LR->sortAndMerge();
  LLVM_DEBUG(dbgs() << "Rebuilding LiveRange result: " << *LR << "\n");
}

void GenXLiveness::rebuildLiveRangeForValue(LiveRange *LR, SimpleValue SV)
{
  LLVM_DEBUG(dbgs() << "rebuilding LiveRange: " << *LR
                    << " for SimpleValue: " << SV << "\n");
  Value *V = SV.getValue();

  // This value is a global variable. Its live range is the entire kernel.
  if (vc::getUnderlyingGlobalVariable(V)) {
    LR->push_back(0, Numbering->getLastNumber());
    LLVM_DEBUG(dbgs() << "It is global value, rebuilded LR: " << *LR << "\n");
    return;
  }

  std::map<BasicBlock *, Segment> BBRanges;
  if (auto Func = isUnifiedRet(V)) {
    // This value is the unified return value of the function Func. Its live
    // range is from the call to where its post-copy would go just afterwards
    // for each call site, also from the site of the pre-copy to the return
    // instruction.
    LLVM_DEBUG(dbgs() << "It is UnifiedRet\n");
    for (auto *U: Func->users()) {
      if (auto *CI = genx::checkFunctionCall(U, Func)) {
        LR->push_back(Numbering->getNumber(CI),
                      Numbering->getRetPostCopyNumber(CI, SV.getIndex()));
        LLVM_DEBUG(dbgs() << "Adding Segment: " << *LR << "\n");
      }
    }
    for (auto fi = Func->begin(), fe = Func->end(); fi != fe; ++fi)
      if (auto RI = dyn_cast<ReturnInst>(fi->getTerminator())) {
        LR->push_back(Numbering->getRetPreCopyNumber(RI, SV.getIndex()),
            Numbering->getNumber(RI));
        LLVM_DEBUG(dbgs() << "It is ReturnInst, Adding Segment: " << *LR
                          << "\n");
      }
    LLVM_DEBUG(dbgs() << "It is UnifiedRet, return LR: " << *LR << "\n");
    return;
  }

  // Mark the value as live and then almost immediately dead again at the
  // point where it is defined.
  unsigned StartNum = 0, EndNum = 0;
  Function *Func = 0;
  auto Arg = dyn_cast<Argument>(V);
  BasicBlock *BB = nullptr;
  if (Arg) {
    Func = Arg->getParent();
    StartNum = Numbering->getNumber(Func);
    EndNum = StartNum + 1;
    BB = &Func->front();
  } else if (auto Phi = dyn_cast<PHINode>(V)) {
    // Phi node. Treat as defined at the start of the block.
    EndNum = Numbering->getNumber(Phi) + 1;
    BB = Phi->getParent();
    StartNum = Numbering->getNumber(BB);
    // For a phi node, we also need to register an extra little live range at
    // the end of each predecessor, from where we will insert a copy to the
    // end. This is done lower down in this function.
  } else {
    StartNum = Numbering->getNumber(V);
    auto Inst = cast<Instruction>(V);
    BB = Inst->getParent();
    auto CI = dyn_cast<CallInst>(V);
    if (CI) {
      if (!GenXIntrinsic::isAnyNonTrivialIntrinsic(V)) {
        // For the return value from a call, move the definition point to the ret
        // post-copy slot after the call, where the post-copy will be inserted if
        // it fails to be coalesced with the function's unified return value.
        StartNum = Numbering->getRetPostCopyNumber(CI, SV.getIndex());
      }
    }
    EndNum = StartNum + 1;
    if (CI && getTwoAddressOperandNum(CI)) {
      // Two address op. Move the definition point one earlier, to where
      // GenXCoalescing will need to insert a copy if coalescing fails.
      --StartNum;
    }
  }
  BBRanges[BB] = Segment(StartNum, EndNum);
  // The stack for predecessors that need to be processed:
  std::vector<BasicBlock *> Stack;
  // Process each use.
  for (Value::use_iterator i = V->use_begin(), e = V->use_end();
      i != e; ++i) {
    BasicBlock *BB = nullptr;
    Instruction *user = cast<Instruction>(i->getUser());
    unsigned Num;
    if (PHINode *Phi = dyn_cast<PHINode>(user)) {
      // Use in a phi node. We say that the use is where the phi copy will be
      // placed in the predecessor block.
      BB = Phi->getIncomingBlock(*i);
      Num = Numbering->getPhiNumber(Phi, BB);
    } else {
      // Normal use.
      // For live range purposes, an instruction is considered to be at the
      // same place as the head of its bale. We need to use getBaleHead to
      // ensure that we consider it to be there.
      Instruction *UserHead = Baling->getBaleHead(user);
      BB = UserHead->getParent();
      Num = Numbering->getNumber(UserHead);
      if (auto CI = dyn_cast<CallInst>(user)) {
        if (CI->isInlineAsm() || IGCLLVM::isIndirectCall(*CI))
          Num = Numbering->getNumber(UserHead);
        else {
        switch (vc::getAnyIntrinsicID(CI)) {
          case GenXIntrinsic::not_any_intrinsic:
            // Use as a call arg. We say that the use is at the arg pre-copy
            // slot, where the arg copy will be inserted in coalescing. This
            // assumes that the copies will be in the same order as args in the
            // call, with struct elements in order too.
            Num = Numbering->getArgPreCopyNumber(CI, i->getOperandNo(),
                                                 SV.getIndex());
            break;
          default:
            if (auto OpndNum = getTwoAddressOperandNum(CI);
                OpndNum && *OpndNum == i->getOperandNo()) {
              // The use is the two address operand in a two address op. Move
              // the use point one earlier, to where GenXCoalescing will need
              // to insert a copy if coalescing fails. If there is any other
              // use of this value in the same bale, that will not have its use
              // point one number earlier. The unnecessary interference that
              // would cause is fixed in the way that twoAddrInterfere()
              // detects interference.
              --Num;
            }
            break;
          case GenXIntrinsic::genx_simdcf_goto:
            // Use in a goto. Treat it as at the branch, as GenXCisaBuilder
            // writes the goto just before the branch, after any intervening IR.
            Num = Numbering->getNumber(CI->getParent()->getTerminator());
            break;
          }
        }
      } else if (auto RI = dyn_cast<ReturnInst>(user)) {
        // Use in a return. We say that the use is where the ret value
        // pre-copy will be inserted in coalescing. This assumes that the
        // copies will be in the same order as the struct elements in the
        // return value.
        Num = Numbering->getRetPreCopyNumber(RI, SV.getIndex());
      }
    }
    auto BBRange = &BBRanges[BB];
    if (BBRange->getEnd()) {
      // There is already a live range in this block. Extend it if
      // necessary. No need to scan back from here, so we're done with
      // this use.
      if (BBRange->getEnd() < Num)
        BBRange->setEnd(Num);
      continue;
    }
    // Add a new live range from the start of this block, and remember the
    // range of blocks that contain a live range (so we don't have to scan
    // all of them at the end).
    *BBRange = Segment(Numbering->getNumber(BB), Num);
    // Push this block's predecessors onto the stack.
    std::copy(pred_begin(BB), pred_end(BB), std::back_inserter(Stack));
    // Process stack until empty.
    while (Stack.size()) {
      BB = Stack.back();
      Stack.pop_back();
      BBRange = &BBRanges[BB];
      auto BBNum = Numbering->getBBNumber(BB);
      if (BBRange->getEnd()) {
        // There is already a live range in this block. Extend it to the end.
        // No need to scan back from here.
        BBRange->setEnd(BBNum->EndNumber);
        continue;
      }
      // Add a new live range through the whole of this block, and remember the
      // range of blocks that contain a live range (so we don't have to scan
      // all of them at the end).
      BBRange->setStartEnd(Numbering->getNumber(BB), BBNum->EndNumber);
      // Push this block's predecessors onto the stack.
      std::copy(pred_begin(BB), pred_end(BB), std::back_inserter(Stack));
    }
  }
  // Now we can build the live range.
  for (auto bri = BBRanges.begin(), bre = BBRanges.end(); bri != bre; ++bri) {
    auto BBRange = &bri->second;
    LR->push_back(*BBRange);
  }
  if (PHINode *Phi = dyn_cast<PHINode>(V)) {
    // For a phi node, we also need to register an extra little live range at
    // the end of each predecessor, from where we will insert a copy to the
    // end.
    for (unsigned i = 0, e = Phi->getNumIncomingValues(); i != e; ++i) {
      auto Pred = Phi->getIncomingBlock(i);
      auto BBNum = Numbering->getBBNumber(Pred);
      LR->push_back(Segment(Numbering->getPhiNumber(Phi, Pred),
            BBNum->EndNumber, Segment::PHICPY));
    }
  }
  LR->sortAndMerge();
  if (CG) {
    // Check if the live range crosses any call instruction. If so, add the
    // appropriate subroutine live range.
    bool NeedSort = false;
    auto N = CG->getNode(Func);
    for (auto i = N->begin(), e = N->end(); i != e; ++i) {
      auto E = &*i;
      // See if this call is in a segment of the LR.
      auto Seg = LR->find(E->Number);
      if (Seg != LR->end() && Seg->getStart() <= E->Number && Seg->getEnd() > E->Number) {
        // Yes it is. Merge the subroutine LR of the callee into our LR.
        if (!E->Call->getCalledFunction()->hasFnAttribute("CMStackCall"))
          LR->addSegments(getLiveRange(E->Call->getCalledFunction()));
        NeedSort = true;
      }
    }
    if (NeedSort)
      LR->sortAndMerge();
  }
  if (Arg) {
    // For a function arg, for each call site, add a segment from the arg
    // pre-copy site, the point just before the call at which it will be copied
    // into, up to the call.  We assume that any copies before the call
    // inserted by coalescing will be in the obvious order of args and elements
    // within args.
    Function *F = Arg->getParent();
    if (*FG->begin() != F) { // is a subroutine
      for (auto *U : F->users()) {
        if (auto *CI = genx::checkFunctionCall(U, F))
          LR->push_back(Numbering->getArgPreCopyNumber(CI, Arg->getArgNo(),
                                                       SV.getIndex()),
                        Numbering->getNumber(CI));
      }
    }
  }
}

void GenXLiveness::removeBale(Bale &B) {
  LLVM_DEBUG(dbgs() << "Removing Bale: " << B << "\n");
  for (auto bi = B.begin(), be = B.end(); bi != be; ++bi)
    removeValue(bi->Inst);
}

/***********************************************************************
 * removeValue : remove the supplied value from its live range, and delete
 *               the range if it now has no values
 *
 * removeValueNoDelete : same, but do not delete the LR if it is now
 *               valueless
 *
 * Calling this with a value that does not have a live range is silently
 * ignored.
 */
void GenXLiveness::removeValue(Value *V)
{
  LLVM_DEBUG(dbgs() << "Removing Value: " << *V << "\n");
  for (unsigned i = 0, e = IndexFlattener::getNumElements(V->getType()); i != e; ++i)
    removeValue(SimpleValue(V, i));
}

void GenXLiveness::removeValue(SimpleValue V)
{
  LLVM_DEBUG(dbgs() << "Removing SimpleValue: " << V << "\n");
  LiveRange *LR = removeValueNoDelete(V);
  if (LR && !LR->Values.size()) {
    // V was the only value in LR. Remove LR completely.
    delete LR;
  }
}

LiveRange *GenXLiveness::removeValueNoDelete(SimpleValue V)
{
  LiveRangeMap_t::iterator i = LiveRangeMap.find(V);
  if (i == end())
    return nullptr;
  LiveRange *LR = i->second;
  LiveRangeMap.erase(i);
  // Remove V from LR.
  unsigned j;
  for (j = 0; LR->Values[j].get() != V; ++j) {
    IGC_ASSERT(j != LR->Values.size());
  }
  if (&LR->Values[j] != &LR->Values.back())
    LR->Values[j] = LR->Values.back();
  LR->Values.pop_back();
  return LR;
}

/***********************************************************************
 * removeValuesNoDelete : remove all values from the live range, but do not
 *        delete the LR
 */
void GenXLiveness::removeValuesNoDelete(LiveRange *LR)
{
  LLVM_DEBUG(dbgs() << "Removing all Values in LR, not delete " << *LR << "\n");
  for (auto vi = LR->value_begin(), ve = LR->value_end(); vi != ve; ++vi)
    LiveRangeMap.erase(*vi);
  LR->value_clear();
}

/***********************************************************************
 * replaceValue : update liveness such that NewVal has OldVal's live range,
 *    and OldVal does not have one at all.
 */
void GenXLiveness::replaceValue(Value *OldVal, Value *NewVal)
{
  LLVM_DEBUG(dbgs() << "Replace Values: from " << *OldVal << " to " << *NewVal
                    << "\n");
  for (unsigned i = 0, e = IndexFlattener::getNumElements(OldVal->getType());
      i != e; ++i)
    replaceValue(SimpleValue(OldVal, i), SimpleValue(NewVal, i));
}

void GenXLiveness::replaceValue(SimpleValue OldVal, SimpleValue NewVal)
{
  LLVM_DEBUG(dbgs() << "Replace SimpleValues: from " << OldVal << " to "
                    << NewVal << "\n");
  LiveRangeMap_t::iterator i = LiveRangeMap.find(OldVal);
  IGC_ASSERT(i != end());
  LiveRange *LR = i->second;
  LiveRangeMap.erase(i);
  LiveRangeMap[NewVal] = LR;
  unsigned j = 0;
  IGC_ASSERT(!LR->Values.empty());
  for (j = 0; LR->Values[j].get() != OldVal; ++j)
    IGC_ASSERT(j != LR->Values.size());
  LR->Values[j] = NewVal;
}

/***********************************************************************
 * getOrCreateLiveRange : get live range for a value, creating if necessary
 */
LiveRange *GenXLiveness::getOrCreateLiveRange(SimpleValue V)
{
  auto [i, isInserted] = LiveRangeMap.emplace(V, nullptr);
  LLVM_DEBUG(dbgs() << "getOrCreateLiveRange for SimpleValue: " << V << " "
                    << (isInserted ? "Inserted" : "Not inserted") << "\n");
  LiveRange *LR = i->second;
  if (!LR) {
    // Newly created map entry. Create the LiveRange for it.
    LR = new LiveRange;
    LR->Values.push_back(V);
    i->second = LR;
    LR->setAlignmentFromValue(
        *DL, V, Subtarget ? Subtarget->getGRFByteSize() : defaultGRFByteSize);
  }
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  // Give the Value a name if it doesn't already have one.
  if (!V.getValue()->getName().size()) {
    std::string NameBuf;
    StringRef Name = "arg";
    if (auto Inst = dyn_cast<Instruction>(V.getValue())) {
      unsigned IID = vc::getAnyIntrinsicID(V.getValue());
      if (GenXIntrinsic::isAnyNonTrivialIntrinsic(IID)) {
        // For an intrinsic call, use the intrinsic name after the
        // final period.
        NameBuf = GenXIntrinsic::getAnyName(IID, None);
        Name = NameBuf;
        size_t Period = Name.rfind('.');
        if (Period != StringRef::npos)
          Name = Name.slice(Period + 1, Name.size());
      } else
        Name = Inst->getOpcodeName();
    }
    V.getValue()->setName(Name);
  }
#endif
  LLVM_DEBUG(dbgs() << "Resulted LR: " << *LR << "\n");
  return LR;
}

LiveRange *GenXLiveness::getOrCreateLiveRange(SimpleValue V,
                                              vc::RegCategory Cat,
                                              unsigned LogAlign) {
  LLVM_DEBUG(dbgs() << "getOrCreateLiveRange with SimpleValue: " << V
                    << " Category: " << vc::getRegCategoryName(Cat)
                    << " LogAlign: " << LogAlign << "\n");
  auto LR = getOrCreateLiveRange(V);
  LR->setCategory(Cat);
  LR->setLogAlignment(LogAlign);
  LLVM_DEBUG(dbgs() << "getOrCreateLiveRange return: " << *LR << "\n");
  return LR;
}

/***********************************************************************
 * eraseLiveRange : get rid of live range for a Value, possibly multiple
 *  ones if it is an aggregate value
 */
void GenXLiveness::eraseLiveRange(Value *V)
{
  LLVM_DEBUG(dbgs() << "Erasing LiveRange for Value: " << *V << "\n");
  Type *Ty = V->getType();
  if (!Ty->isAggregateType()) {
    eraseLiveRange(SimpleValue(V));
    return;
  }
  for (unsigned i = 0, e = IndexFlattener::getNumElements(Ty); i != e; ++i)
    eraseLiveRange(SimpleValue(V, i));
}

/***********************************************************************
 * eraseLiveRange : get rid of live range for a Value, if any
 */
void GenXLiveness::eraseLiveRange(SimpleValue V)
{
  LLVM_DEBUG(dbgs() << "Erasing LiveRange for SimpleValue: " << V << "\n");
  auto LR = getLiveRangeOrNull(V);
  if (LR)
    eraseLiveRange(LR);
}

/***********************************************************************
 * eraseLiveRange : get rid of the specified live range, and remove its
 *        values from the map
 */
void GenXLiveness::eraseLiveRange(LiveRange *LR)
{
  LLVM_DEBUG(dbgs() << "Erasing LiveRange: " << *LR << "\n");
  for (auto vi = LR->value_begin(), ve = LR->value_end(); vi != ve; ++vi)
    LiveRangeMap.erase(*vi);
  delete LR;
}

/***********************************************************************
 * getLiveRangeOrNull : get live range for value, or 0 if none
 *
 * The returned LiveRange pointer is valid only until the next time the
 * live ranges are modified, including the case of coalescing.
 */
const LiveRange *GenXLiveness::getLiveRangeOrNull(SimpleValue V) const
{
  LLVM_DEBUG(dbgs() << "Getting LiveRangeOrNull for: " << V);
  auto i = LiveRangeMap.find(V);
  if (i == end()) {
    LLVM_DEBUG(dbgs() << " Not found, nullptr return\n");
    return nullptr;
  }
  LLVM_DEBUG(dbgs() << " Found: " << *(i->second) << "\n");
  return i->second;
}

LiveRange *GenXLiveness::getLiveRangeOrNull(SimpleValue V)
{
  return const_cast<LiveRange*>(static_cast<const GenXLiveness*>(this)->getLiveRangeOrNull(V));
}

/***********************************************************************
 * getLiveRange : get live range for value
 *
 * The returned LiveRange pointer is valid only until the next time the
 * live ranges are modified, including the case of coalescing.
 */
LiveRange *GenXLiveness::getLiveRange(SimpleValue V)
{
  LiveRange *LR = getLiveRangeOrNull(V);
  IGC_ASSERT_MESSAGE(LR, "no live range found");
  return LR;
}

/***********************************************************************
 * getUnifiedRet : get/create unified return value for a function
 *
 * Returns already created unified value, or creates new one
 * if there was no such.
 */
Value *GenXLiveness::getUnifiedRet(Function *F) {
  auto *Ret = getUnifiedRetIfExist(F);
  if (!Ret)
    return createUnifiedRet(F);
  return Ret;
}

Value *GenXLiveness::getUnifiedRetIfExist(Function *F) const {
  auto RetIt = UnifiedRets.find(F);
  if (RetIt == UnifiedRets.end())
    return nullptr;
  return RetIt->second;
}

/***********************************************************************
 * createUnifiedRet : create unified return value for a function
 *
 * To allow all returns in a function and all results of calls to that
 * function to use the same register, we have a dummy "unified return
 * value".
 *
 * Cannot be called on a function with void return type.
 *
 * This also creates the LiveRange for the unified return value, or
 * multiple ones if it is aggregate type, and sets the category to the same as
 * in one of the return instructions.
 */
Value *GenXLiveness::createUnifiedRet(Function *F) {
  IGC_ASSERT_MESSAGE(!F->isDeclaration(), "must be a function definition");
  IGC_ASSERT_MESSAGE(UnifiedRets.find(F) == UnifiedRets.end(),
    "Unified ret must not have been already created");
  Type *Ty = F->getReturnType();
  IGC_ASSERT(!Ty->isVoidTy());
  auto URet = genx::createUnifiedRet(Ty, "", F->getParent());
  // Find some return inst.
  ReturnInst *Ret = nullptr;
  for (auto fi = F->begin(), fe = F->end(); fi != fe; ++fi)
    if ((Ret = dyn_cast<ReturnInst>(fi->getTerminator())))
      break;
  IGC_ASSERT_MESSAGE(Ret, "must find return instruction");
  Value *RetVal = Ret->getOperand(0);
  // Use the categories of its operand to set the categories of the unified
  // return value.
  for (unsigned AggrIdx = 0, NumElements = IndexFlattener::getNumElements(Ty);
       AggrIdx != NumElements; ++AggrIdx) {
    auto RetValSV = SimpleValue(RetVal, AggrIdx);
    auto *RetValLR = getLiveRangeOrNull(RetValSV);
    auto URetSV = SimpleValue(URet, AggrIdx);
    auto Cat = RetValLR ? RetValLR->getOrDefaultCategory()
                        : getDefaultCategory(RetValSV);
    getOrCreateLiveRange(URetSV)->setCategory(Cat);
  }

  UnifiedRets[F] = URet;
  UnifiedRetToFunc[URet] = F;
  return URet;
}

/***********************************************************************
 * isUnifiedRet : test whether a value is a unified return value
 *
 * A unified ret value is a call instruction that is
 * not attached to any BB, and is in the UnifiedRetFunc map.
 */
Function *GenXLiveness::isUnifiedRet(Value *V)
{
  // Quick checks first.
  auto Inst = dyn_cast<CallInst>(V);
  if (!Inst)
    return nullptr;
  if (Inst->getParent())
    return nullptr;
  // Then map lookup.
  auto i = UnifiedRetToFunc.find(V);
  if (i == UnifiedRetToFunc.end())
    return nullptr;
  return i->second;
}

/***********************************************************************
 * moveUnifiedRet : move a function's unified return value to another function
 *
 * This is used when replacing a function with a new one in GenXArgIndirection.
 */
void GenXLiveness::moveUnifiedRet(Function *OldF, Function *NewF)
{
  auto i = UnifiedRets.find(OldF);
  if (i == UnifiedRets.end())
    return;
  Value *UR = i->second;
  UnifiedRets[NewF] = UR;
  UnifiedRets.erase(i);
  UnifiedRetToFunc[UR] = NewF;
}

/***********************************************************************
 * find : given an instruction number, find a segment in a live range
 *
 * If the number is within a segment, or is just on its end point, that
 * segment is returned. If the number is in a hole, the next segment
 * after the hole is returned. If the number is before the first
 * segment, the first segment is returned. If the number is after the
 * last segment, end() is returned.
 */
LiveRange::iterator LiveRange::find(unsigned Pos)
{
  size_t Len = size();
  if (!Len)
    return end();
  if (Pos > Segments[Len - 1].getEnd())
    return end();
  iterator I = begin();
  do {
    size_t Mid = Len >> 1;
    if (Pos <= I[Mid].getEnd())
      Len = Mid;
    else
      I += Mid + 1, Len -= Mid + 1;
  } while (Len);
  IGC_ASSERT(I->getEnd() >= Pos);
  return I;
}

/***********************************************************************
 * getOrDefaultCategory : get category; if none, set default
 *
 * The default category is PREDICATE for i1 or a vector of i1, or GENERAL
 * for anything else.
 */
vc::RegCategory LiveRange::getOrDefaultCategory() {
  vc::RegCategory Cat = getCategory();
  if (Cat != vc::RegCategory::None)
    return Cat;
  IGC_ASSERT(!value_empty());
  Cat = getDefaultCategory(*value_begin());
  setCategory(Cat);
  return Cat;
}

/***********************************************************************
 * interfere : check whether two live ranges interfere
 *
 * Two live ranges interfere if there is a segment from each that overlap
 * and they are considered to cause interference by
 * checkIfOverlappingSegmentsInterfere below.
 */
bool GenXLiveness::interfere(LiveRange *LR1, LiveRange *LR2) {
  if (CoalescingDisabled)
    return true;
  return getSingleInterferenceSites(LR1, LR2, nullptr);
}

/***********************************************************************
 * twoAddrInterfere : check whether two live ranges interfere, allowing for
 *    single number interference sites at two address ops
 *
 * Return:  true if they interfere
 *
 * Two live ranges interfere if there is a segment from each that overlap
 * and are not both weak.
 *
 * But, if each interfering segment is a single number that is the precopy
 * site of a two address op, and the result of the two address op is in one LR
 * and the two address operand is in the other, then that is not counted as
 * interference.
 *
 * That provision allows for coalescing at a two address op where the two
 * address operand has already been copy coalesced with, or is the same value
 * as, a different operand in the same bale, as follows:
 *
 * Suppose the two address op a has number N, and it has two address operand b
 * and some other operand c in the same bale:
 *
 * N-1: (space for precopy)
 * N:   a = op(b, c)
 *
 * with live ranges
 * a:[N-1,..)
 * b:[..,N-1)
 * c:[..,N)
 *
 * Then a and b can coalesce.
 *
 * But suppose b and c are the same value, or had previously been copy coalesced.
 * Then we have live ranges
 * a:[N-1,..)
 * b,c:[..,N)
 *
 * and a and b now interfere needlessly.
 *
 * This function is called on an attempt to coalesce a and b (or rather the
 * live range containing a and the live range containing b).  In it, we see
 * that there is a single number segment of interference [N-1,N), where a is
 * the result and b is the two address operand of the two address op at N. Thus
 * we discount that segment of interference, and a and b can still coalesce.
 *
 * Note that this formulation allows for there to be multiple such sites because
 * of multiple two address results being already coalesced together through phi
 * nodes.
 */
bool GenXLiveness::twoAddrInterfere(LiveRange *LR1, LiveRange *LR2) {
  if (CoalescingDisabled)
    return true;
  SmallVector<unsigned, 4> Sites;
  if (getSingleInterferenceSites(LR1, LR2, &Sites))
    return true; // interferes, not just single number sites
  if (Sites.empty())
    return false; // does not interfere at all
  // Put the single number sites in a set.
  SmallSet<unsigned, 4> SitesSet;
  LLVM_DEBUG(dbgs() << "got single number interference sites:");
  for (auto i = Sites.begin(), e = Sites.end(); i != e; ++i) {
    LLVM_DEBUG(dbgs() << " " << *i);
    SitesSet.insert(*i);
  }
  LLVM_DEBUG(dbgs() << "\nbetween:\n" << *LR1 << "\n" << *LR2 << "\n");
  Sites.clear();
  // Check each def in LR1 and LR2 for being a two address op that causes us to
  // discount a single number interference site.
  for (auto LR = LR1, OtherLR = LR2; LR;
      LR = LR == LR1 ? LR2 : nullptr, OtherLR = LR1) {
    for (auto vi = LR->value_begin(), ve = LR->value_end(); vi != ve; ++vi) {
      auto CI = dyn_cast<CallInst>(vi->getValue());
      if (!CI)
        continue;
      auto OperandNum = getTwoAddressOperandNum(CI);
      if (!OperandNum)
        continue;
      // Got a two addr op. Check whether the two addr operand is in the other
      // LR.
      if (getLiveRangeOrNull(CI->getOperand(*OperandNum)) != OtherLR)
        continue;
      // Discount the single number interference site here, if there is one.
      SitesSet.erase(getNumbering()->getNumber(CI) - 1);
    }
  }
  // If we have discounted all sites, then there is no interference.
  return !SitesSet.empty();
}

/***********************************************************************
 * getSingleInterferenceSites : check whether two live ranges interfere,
 *      returning single number interference sites
 *
 * Enter:   LR1, LR2 = live ranges to check
 *          Sites = vector in which to store single number interference sites,
 *                  or 0 if we do not want to collect them
 *
 * Return:  true if the live ranges interfere other than as reflected in Sites
 *
 * Two live ranges interfere if there is a segment from each that overlap
 * and are not both weak.
 *
 * If Sites is 0 (the caller does not want the Sites list), then the function
 * returns true if there is any interference.
 *
 * If Sites is not 0, then any interference in a single number segment, for
 * example [19,20), causes the start number to be pushed into Sites. The
 * function returns true only if there is interference that cannot be described
 * in Sites.
 */
bool GenXLiveness::getSingleInterferenceSites(LiveRange *LR1, LiveRange *LR2,
    SmallVectorImpl<unsigned> *Sites)
{
  // Swap if necessary to make LR1 the one with more segments.
  if (LR1->size() < LR2->size())
    std::swap(LR1, LR2);
  auto Idx2 = LR2->begin(), End2 = LR2->end();
  // Find segment in LR1 that contains or is the next after the start
  // of the first segment in LR2, including the case that the start of
  // the LR2 segment abuts the end of the LR1 segment.
  auto Idx1 = LR1->find(Idx2->getStart()), End1 = LR1->end();
  if (Idx1 == End1)
    return false;
  for (;;) {
    // Check for overlap.
    if (Idx1->getStart() < Idx2->getStart()) {
      if (Idx1->getEnd() > Idx2->getStart())
        if (checkIfOverlappingSegmentsInterfere(LR1, Idx1, LR2, Idx2)) {
          // Idx1 overlaps Idx2. Check if it is a single number overlap that can
          // be pushed into Sites.
          if (!Sites || Idx1->getEnd() != Idx2->getStart() + 1)
            return true;
          Sites->push_back(Idx2->getStart());
        }
    } else {
      if (Idx1->getStart() < Idx2->getEnd())
        if (checkIfOverlappingSegmentsInterfere(LR1, Idx1, LR2, Idx2)) {
          // Idx2 overlaps Idx1. Check if it is a single number overlap that can
          // be pushed into Sites.
          if (!Sites || Idx2->getEnd() != Idx1->getStart() + 1)
            return true;
          Sites->push_back(Idx1->getStart());
        }
    }
    // Advance whichever one has the lowest End.
    if (Idx1->getEnd() < Idx2->getEnd()) {
      if (++Idx1 == End1)
        return false;
    } else {
      if (++Idx2 == End2)
        return false;
    }
  }
}

/***********************************************************************
 * checkIfOverlappingSegmentsInterfere : given two segments that have been
 *    shown to overlap, check whether their strengths make them interfere
 *
 * If both segments are weak, they do not interfere.
 *
 * Interference between a normal segment in one LR and a phicpy segment in the
 * other LR is ignored, as long as the phicpy segment relates to a phi incoming
 * where the phi node is in the LR with the phicpy segment and the incoming
 * value is in the LR with the strong segment. This is used to avoid
 * unnecessary interference for a phi incoming through a critical edge, where
 * the incoming is likely to be used in the other successor as well.
 */
bool GenXLiveness::checkIfOverlappingSegmentsInterfere(
    LiveRange *LR1, Segment *S1, LiveRange *LR2, Segment *S2)
{
  if (S1->isWeak() && S2->isWeak())
    return false; // both segments weak
  if (S2->Strength == Segment::PHICPY) {
    // Swap so that if either segment is phicpy, then it is S1 for the check
    // below.
    std::swap(LR1, LR2);
    std::swap(S1, S2);
  }
  if (S1->Strength != Segment::PHICPY)
    return true;
  // S1 is phicpy. If its corresponding phi cpy insertion point is for a phi
  // node in LR1 and an incoming in LR2, then this does not cause interference.
  auto PhiIncoming = Numbering->getPhiIncomingFromNumber(S1->getStart());
  auto PhiFound = std::find_if(
      PhiIncoming.begin(), PhiIncoming.end(), [LR1, this](auto It) {
        PHINode *Phi = It.first;
        IGC_ASSERT_MESSAGE(Phi, "phi incoming not found");
        return getLiveRange(Phi) == LR1;
      });
  if (PhiFound == PhiIncoming.end())
    return true; // phi not in LR1, interferes

  PHINode *Phi = PhiFound->first;
  unsigned IncomingBBIndex = PhiFound->second;
  if (getLiveRangeOrNull(Phi->getIncomingValue(IncomingBBIndex)) != LR2)
    return true; // phi incoming not in LR2, interferes
  // Conditions met -- does not cause interference.
  return false;
}

/***********************************************************************
 * coalesce : coalesce two live ranges that do not interfere
 *
 * Enter:   LR1 = first live range
 *          LR2 = second live range
 *          DisallowCASC = true to disallow call arg special coalescing
 *                         into the resulting live range
 *
 * Return:  new live range (LR1 and LR2 now invalid)
 */
LiveRange *GenXLiveness::coalesce(LiveRange *LR1, LiveRange *LR2,
    bool DisallowCASC)
{
  IGC_ASSERT_MESSAGE(LR1 != LR2, "cannot coalesce an LR to itself");
  IGC_ASSERT_MESSAGE(LR1->Category == LR2->Category,
    "cannot coalesce two LRs with different categories");
  // Make LR1 the one with the longer list of segments.
  if (LR2->Segments.size() > LR1->Segments.size()) {
    LiveRange *temp = LR1;
    LR1 = LR2;
    LR2 = temp;
  }
  LLVM_DEBUG(
    dbgs() << "Coalescing \"";
    LR1->print(dbgs());
    dbgs() << "\" and \"";
    LR2->print(dbgs());
    dbgs() << "\"\n"
  );
  // Do the merge of the segments.
  merge(LR1, LR2);
  // Copy LR2's values across to LR1.
  for (auto i = LR2->value_begin(), e = LR2->value_end(); i != e; ++i)
    LiveRangeMap[LR1->addValue(*i)] = LR1;
  // Use the largest alignment from the two LRs.
  LR1->LogAlignment = std::max(LR1->LogAlignment, LR2->LogAlignment);
  // If either LR has a non-zero offset, use it.
  IGC_ASSERT(!LR1->Offset || !LR2->Offset);
  LR1->Offset |= LR2->Offset;
  // Set DisallowCASC.
  LR1->DisallowCASC |= LR2->DisallowCASC | DisallowCASC;
  delete LR2;
  LLVM_DEBUG(
    dbgs() << "  giving \"";
    LR1->print(dbgs());
    dbgs() << "\"\n"
  );
  return LR1;
}

/***********************************************************************
 * copyInterfere : check whether two live ranges copy-interfere
 *
 * Two live ranges LR1 and LR2 copy-interfere (a non-commutative relation)
 * if LR1 includes a value that is a phi node whose definition is within
 * LR2.
 */
bool GenXLiveness::copyInterfere(LiveRange *LR1, LiveRange *LR2) {
  if (CoalescingDisabled)
    return true;
  // Find a phi node value in LR1. It can have at most one, because only
  // copy coalescing has occurred up to now, and copy coalescing does not
  // occur at a phi node.
  for (unsigned i = 0, e = LR1->Values.size(); i != e; ++i) {
    auto Phi = dyn_cast<PHINode>(LR1->Values[i].getValue());
    if (!Phi)
      continue;
    // Found a phi node in LR1. A phi node has multiple instruction numbers,
    // one for each incoming block. See if any one of those is in LR2's
    // live range.
    for (unsigned i = 0, e = Phi->getNumIncomingValues(); i != e; ++i)
      if (LR2->contains(Numbering->getPhiNumber(Phi, Phi->getIncomingBlock(i))))
        return true;
    break;
  }
  return false; // no phi node found
}

/***********************************************************************
 * wrapsAround : detects if V1 is a phi node and V2 wraps round to a use
 *              in a phi node in the same basic block as V1 and after it
 */
bool GenXLiveness::wrapsAround(Value *V1, Value *V2)
{
  auto PhiDef = dyn_cast<PHINode>(V1);
  if (!PhiDef)
    return false;
  for (auto ui = V2->use_begin(), ue = V2->use_end(); ui != ue; ++ui) {
    if (auto PhiUse = dyn_cast<PHINode>(ui->getUser())) {
      if (PhiUse->getParent() == PhiDef->getParent()) {
        // Phi use in the same BB. Scan until we find PhiDef or the end
        // of the phi nodes.
        while (PhiUse != PhiDef) {
          PhiUse = dyn_cast<PHINode>(PhiUse->getNextNode());
          if (!PhiUse)
            return true; // reach end of phi nodes
        }
      }
    }
  }
  return false;
}

/***********************************************************************
 * insertCopy : insert a copy of a non-aggregate value
 *
 * Enter:   InputVal = value to copy
 *          LR = live range to add the new value to (0 to avoid adjusting
 *                live ranges)
 *          InsertBefore = insert copy before this inst
 *          Name = name to give the new value
 *          Number = number to give the new instruction(s), 0 for none
 *
 * Return:  The new copy instruction
 *
 * This inserts multiple copies if the input value is a vector that is
 * bigger than two GRFs or a non power of two size.
 *
 * This method is mostly used from GenXCoalescing, which passes an LR to
 * add the new copied value to.
 *
 * It is also used from GenXLiveRange if it needs to add a copy to break an
 * overlapping circular phi value, in which case LR is 0 as we do not want to
 * adjust live ranges. Also at this stage there is no baling info to update.
 */
Instruction *GenXLiveness::insertCopy(Value *InputVal, LiveRange *LR,
                                      Instruction *InsertBefore,
                                      const Twine &Name, unsigned Number,
                                      const GenXSubtarget *ST) {
  IGC_ASSERT(!isa<Constant>(InputVal));
  bool AdjustLRs = LR != nullptr;
  LiveRange *SourceLR = nullptr;
  if (AdjustLRs)
    SourceLR = getLiveRange(InputVal);
  auto InputTy = InputVal->getType();
  if (InputTy->getScalarType()->isIntegerTy(1)) {
    // The input is a predicate.
    if (!isa<Constant>(InputVal)) {
      // The predicate input is not a constant.
      // There is no way in vISA of copying from one
      // predicate to another, so we copy all 0s into the destination
      // then "or" the source into it.
      Instruction *NewInst = CastInst::Create(Instruction::BitCast,
          Constant::getNullValue(InputTy), InputTy, Name, InsertBefore);
      Numbering->setNumber(NewInst, Number);
      if (AdjustLRs)
        setLiveRange(SimpleValue(NewInst), LR);
      NewInst = BinaryOperator::Create(Instruction::Or, NewInst, InputVal, Name,
          InsertBefore);
      Numbering->setNumber(NewInst, Number);
      if (AdjustLRs)
        setLiveRange(SimpleValue(NewInst), LR);
      return NewInst;
    }
    // Predicate input is constant.
    auto NewInst = CastInst::Create(Instruction::BitCast, InputVal,
        InputTy, Name, InsertBefore);
    Numbering->setNumber(NewInst, Number);
    if (AdjustLRs)
      setLiveRange(SimpleValue(NewInst), LR);
    return NewInst;
  }
  Instruction *NewInst = nullptr;
  if (InputTy->isPointerTy()) {
    // BitCast used to represent a normal copy.
    NewInst = CastInst::Create(Instruction::BitCast, InputVal,
                               InputVal->getType(), Name, InsertBefore);
    if (Number)
      Numbering->setNumber(NewInst, Number);
    if (AdjustLRs)
      setLiveRange(SimpleValue(NewInst), LR);
    return NewInst;
  }

  Region R(InputVal);
  IGC_ASSERT(ST);
  unsigned MaxNum = getLegalRegionSizeForTarget(
      *ST, R, /* StartIdx */ 0, /* Allow2D */ false, R.NumElements);
  // Adjust size to Exec size
  MaxNum = std::min(MaxNum, TotalEMSize);
  if (exactLog2(R.NumElements) >= 0 && R.NumElements <= MaxNum) {
    // Can be done with a single copy.
    if (SourceLR && (SourceLR->Category != vc::RegCategory::General ||
                     (LR && LR->Category != vc::RegCategory::General))) {
      // Need a category conversion (including the case that the two
      // categories are the same but not GENERAL).
      NewInst = createConvert(InputVal, Name, InsertBefore);
    } else {
      // BitCast used to represent a normal copy.
      NewInst = CastInst::Create(Instruction::BitCast, InputVal,
          InputVal->getType(), Name, InsertBefore);
    }
    if (Number)
      Numbering->setNumber(NewInst, Number);
    if (AdjustLRs)
      setLiveRange(SimpleValue(NewInst), LR);
    return NewInst;
  }

  auto collectFragment = [](Value *V, unsigned MaxFrag,
                         SmallVectorImpl<std::pair<unsigned, unsigned>>& Frag,
                         unsigned MaxElt) {
    while (!isa<UndefValue>(V)) {
      if (!GenXIntrinsic::isWrRegion(V))
        return true;
      IntrinsicInst *WII = cast<IntrinsicInst>(V);
      Region R = makeRegionFromBaleInfo(WII, BaleInfo());
      if (R.Indirect || !R.isContiguous() || !R.isWholeNumRows())
        return true;
      if ((R.Offset % R.ElementBytes) != 0)
        return true;
      unsigned Base = R.getOffsetInElements();
      for (unsigned Offset = 0; Offset < R.NumElements; /*EMPTY*/) {
        unsigned NumElts = std::min(MaxElt, R.NumElements - Offset);
        // Round NumElts down to power of 2. That is how many elements we
        // are copying this time round the loop.
        NumElts = 1 << genx::log2(NumElts);
        Frag.push_back(std::make_pair(Base + Offset, NumElts));
        Offset += NumElts;
      }
      V = WII->getOperand(0);
    }
    if (Frag.size() > MaxFrag)
      return true;
    std::sort(Frag.begin(), Frag.end());
    return false;
  };

  unsigned NumElements = R.NumElements;
  SmallVector<std::pair<unsigned, unsigned>, 8> Fragments;
  unsigned MaxCopies = (NumElements + MaxNum - 1) / MaxNum;
  if (collectFragment(InputVal, MaxCopies, Fragments, MaxNum)) {
    Fragments.clear();
    for (unsigned Offset = 0; Offset < NumElements; /*EMPTY*/) {
      unsigned NumElts = std::min(MaxNum, NumElements - Offset);
      // Round NumElts down to power of 2. That is how many elements we
      // are copying this time round the loop.
      NumElts = 1 << genx::log2(NumElts);
      Fragments.push_back(std::make_pair(Offset, NumElts));
      Offset += NumElts;
    }
  }
  // Need to split the copy up. Start with an undef destination.
  Value *Res = UndefValue::get(InputVal->getType());
  for (auto &I : Fragments) {
    unsigned Offset = I.first;
    // Set the subregion.
    R.NumElements = I.second;
    R.Width = R.NumElements;
    R.Offset = Offset * R.ElementBytes;
    // Create the rdregion. Do not add this to a live range because it is
    // going to be baled in to the wrregion.
    Instruction *RdRegion = R.createRdRegion(InputVal, Name, InsertBefore,
        DebugLoc(), true/*AllowScalar*/);
    if (Baling)
      Baling->setBaleInfo(RdRegion, BaleInfo(BaleInfo::RDREGION, 0));
    if (Number)
      Numbering->setNumber(RdRegion, Number);
    // Create the wrregion, and mark that it bales in the rdregion (operand 1).
    NewInst = R.createWrRegion(Res, RdRegion, Name, InsertBefore, DebugLoc());
    if (Number)
      Numbering->setNumber(NewInst, Number);
    if (Baling) {
      BaleInfo BI(BaleInfo::WRREGION);
      BI.setOperandBaled(1);
      Baling->setBaleInfo(NewInst, BI);
    }
    if (AdjustLRs) {
      // Add the last wrregion to the live range (thus coalescing them all
      // together and in with the phi node or two address op that we're doing
      // the copy for).
      setLiveRange(SimpleValue(NewInst), LR);
    }
    Res = NewInst;
  }
  return NewInst;
}

/***********************************************************************
 * merge : merge segments of LR2 into LR1
 *
 * This is equivalent to addSegments followed by sortAndMerge.
 *
 * Previously there was some code here that attempted to optimize on the
 * assumption that the caller passed the one with the longer list of segments
 * as LR1. However that became too complicated once we introduced weak and
 * strong liveness.
 *
 * One day we could re-introduce some simple optimized paths, such as when
 * LR2 has a single segment.
 */
void GenXLiveness::merge(LiveRange *LR1, LiveRange *LR2)
{
  LR1->addSegments(LR2);
  LR1->sortAndMerge();
}

/***********************************************************************
 * eraseUnusedTree : erase unused tree of instructions
 *
 * Enter:   Inst = root of unused tree
 *
 * This erases Inst, then recursively erases other instructions that become
 * unused. Erased instructions are also removed from liveness.
 *
 * Other than the given Inst, this does not erase a non-intrinsic call, or
 * an intrinsic call with a side effect.
 *
 * Instead of erasing as we go, we undef operands to make them unused and then
 * erase everything at the end. This is required for the case that we have an
 * unused DAG of instructions rather than just an unused tree, for example
 * where we have a rd-wr sequence and all the rds use the same input.
 */
void GenXLiveness::eraseUnusedTree(Instruction *TopInst)
{
  SmallVector<Instruction *, 4> Stack;
  std::set<Instruction *> ToErase;
  Stack.push_back(TopInst);
  while (!Stack.empty()) {
    auto Inst = Stack.back();
    Stack.pop_back();
    if (!Inst->use_empty())
      continue;
    if (TopInst != Inst) {
      if (auto CI = dyn_cast<CallInst>(Inst)) {
        if (!GenXIntrinsic::isAnyNonTrivialIntrinsic(CI))
          continue;
        if (!CI->doesNotAccessMemory())
          continue;
      }
    }
    for (unsigned oi = 0, oe = Inst->getNumOperands(); oi != oe; ++oi)
      if (auto OpndInst = dyn_cast<Instruction>(Inst->getOperand(oi))) {
        Stack.push_back(OpndInst);
        Inst->setOperand(oi, UndefValue::get(OpndInst->getType()));
      }
    removeValue(Inst);
    ToErase.insert(Inst);
  }
  for (auto i = ToErase.begin(), e = ToErase.end(); i != e; ++i)
    (*i)->eraseFromParent();
}

/***********************************************************************
 * setArgAddressBase : set the base value of an argument indirect address
 *
 * Enter:    Addr = genx.convert.addr instruction
 *           Base = a base register for the Addr
 */
void GenXLiveness::setArgAddressBase(Value *Addr, Value *Base) {
  // There might be several addresses for the one base. So, find and erase an
  // old address for the input base.
  auto Res = ArgAddressBaseMap.find(Addr);
  if (Res != ArgAddressBaseMap.end()) {
    Value *PreviousBase = Res->second;
    auto AddrsRange = BaseToArgAddrMap.equal_range(PreviousBase);
    auto ToRemove = std::find_if(AddrsRange.first, AddrsRange.second,
                                 [Addr](auto It) { return It.second == Addr; });
    IGC_ASSERT_MESSAGE(
        ToRemove != AddrsRange.second,
        "Addr -> PreviousBase exists in ArgAddressBaseMap. It must "
        "exist in BaseToArgAddrMap too.");
    BaseToArgAddrMap.erase(ToRemove);
  }

  // Set the new connection between address and base.
  ArgAddressBaseMap[Addr] = Base;
  BaseToArgAddrMap.insert({Base, Addr});
}

/***********************************************************************
 * getAddressBase : get the base register of an address
 *
 * Enter:   Addr = address conversion (genx.convert.addr instruction)
 *
 * Return:  The Value for the base that the address is used with, or some
 *          other Value that is coalesced with that
 */
Value *GenXLiveness::getAddressBase(Value *Addr)
{
  // Get the base register from the rdregion/wrregion that the index is used
  // in. This might involve going via an add or an rdregion.
  Use *U = &*Addr->use_begin();
  auto user = cast<Instruction>(U->getUser());
  while (!U->getOperandNo()) {
    U = &*user->use_begin();
    user = cast<Instruction>(U->getUser());
  }
  if (GenXIntrinsic::isRdRegion(user))
    return user->getOperand(0);
  if (GenXIntrinsic::isWrRegion(user)) {
    auto Head = Baling->getBaleHead(user);
    if (Head && isa<StoreInst>(Head)) {
      Value *V = Head->getOperand(1);
      V = vc::getUnderlyingGlobalVariable(V);
      IGC_ASSERT_MESSAGE(V, "null base not expected");
      return V;
    }
    return user;
  }
  // The above scheme does not work for an address conversion added by
  // GenXArgIndirection. Instead we have AddressBaseMap to provide the mapping.
  auto i = ArgAddressBaseMap.find(Addr);
  IGC_ASSERT_MESSAGE(i != ArgAddressBaseMap.end(),
    "base register not found for address");
  Value *BaseV = i->second;
  LiveRange *LR = getLiveRange(BaseV);
  // Find a SimpleValue in the live range that is not an aggregate member.
  for (auto vi = LR->value_begin(), ve = LR->value_end(); vi != ve; ++vi) {
    Value *V = vi->getValue();
    if (!V->getType()->isAggregateType())
      return V;
  }
  IGC_ASSERT_EXIT_MESSAGE(0, "non-aggregate value not found");
}

/***********************************************************************
 * getAddressWithBase : get addresses that base register is a Base
 *
 * Enter:   Base = assumed base
 *
 * Return:  If the input value is a base, return the vector of the corresponding
 *          arguments indirect addresses, empty vector otherwise.
 */
std::vector<Value *> GenXLiveness::getAddressWithBase(Value *Base) {
  auto Res = BaseToArgAddrMap.equal_range(Base);
  std::vector<Value *> Bases;
  std::transform(Res.first, Res.second, std::back_inserter(Bases),
                 [](auto It) { return It.second; });
  return Bases;
}

/***********************************************************************
 * isNoopCastCoalesced : see if the no-op cast has been coalesced away
 *
 * This handles the case that the input and result of the no-op cast are coalesced
 * in to the same live range.
 */
bool GenXLiveness::isNoopCastCoalesced(CastInst *CI)
{
  IGC_ASSERT(genx::isNoopCast(CI));
  return getLiveRangeOrNull(CI) == getLiveRangeOrNull(CI->getOperand(0));
}

/***********************************************************************
 * dump, print : dump the liveness info
 */
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void GenXLiveness::dump()
{
  print(errs()); errs() << '\n';
}
void LiveRange::dump() const
{
  print(errs()); errs() << '\n';
}
#endif

void GenXLiveness::printValueLiveness(SimpleValue SV, raw_ostream &OS) const {
  const LiveRange *LR = getLiveRangeOrNull(SV);
  if (!LR)
    return;
  // Only show an LR if the map iterator is on the value that appears first
  // in the LR. That avoids printing the same LR multiple times.
  IGC_ASSERT(!LR->value_empty());
  auto &&ASV = *(LR->value_begin());
  if (SV.getValue() != ASV.getValue() || SV.getIndex() != ASV.getIndex())
    return;

  LR->print(OS);
  OS << "\n";
}

void GenXLiveness::print(raw_ostream &OS, const FunctionGroup *dummy) const {
  OS << "GenXLiveness for FunctionGroup " << FG->getName() << "\n";
  // Print live ranges for global variables;
  for (auto &G : FG->getModule()->globals())
    printValueLiveness(&G, OS);
  for (auto i = FG->begin(), e = FG->end(); i != e; ++i) {
    Function *Func = *i;
    // Print live ranges for args.
    for (auto fi = Func->arg_begin(), fe = Func->arg_end(); fi != fe; ++fi)
      printValueLiveness(&*fi, OS);
    // Print live range(s) for unified return value.
    if (!Func->getReturnType()->isVoidTy()) {
      auto Ret = getUnifiedRetIfExist(Func);
      if (Ret)
        printValueLiveness(Ret, OS);
    }
    // Print live ranges for code.
    for (auto &Inst : instructions(Func)) {
      auto *InstStructType = dyn_cast<StructType>(Inst.getType());
      if (!InstStructType) {
        printValueLiveness(&Inst, OS);
        continue;
      }
      for (unsigned i = 0, e = IndexFlattener::getNumElements(InstStructType);
           i != e; ++i) {
        auto SV = SimpleValue(&Inst, i);
        printValueLiveness(SV, OS);
      }
    }
  }
  OS << "\n";
}

/***********************************************************************
 * LiveRange::testLiveRanges : tests if no segments abut or overlap or are
 * in the wrong order. Live range's segments should be well formed.
 */
bool LiveRange::testLiveRanges() const
{
  auto testAdjacentSegments = [](const Segment& A, const Segment& B)
  {
    const bool P1 = (A.Strength != B.Strength);
    const bool P2 = (A.getEnd() < B.getStart());
    const bool SegmentIsGood = (P1 || P2);
    IGC_ASSERT(SegmentIsGood);
    const bool StopSearch = !SegmentIsGood;
    return StopSearch;
  };

  const_iterator Begin = begin();
  const_iterator End = end();
  const_iterator Failed = std::adjacent_find(Begin, End, testAdjacentSegments);

  const bool Result = (Failed == End);
  return Result;
}

/***********************************************************************
 * LiveRange::addSegment : add a segment to a live range
 *
 * The segment might already be covered by an existing segment, in which
 * case nothing changes.
 *
 * It would be inefficient to implement coalesce() in terms of this, because
 * it might have to shuffle lots of elements along by one each time.
 * This function only gets called when adding a single segment to a live
 * range when inserting a copy in coalescing.
 */
void LiveRange::addSegment(Segment Seg)
{
  iterator i = find(Seg.getStart()), e = end();
  if (i == e) {
    // New segment off end.
    Segments.push_back(Seg);
  } else if (i->getStart() <= Seg.getStart()) {
    // New segment is covered by or overlaps the end of old segment i.
    if (i->getEnd() < Seg.getEnd()) {
      i->setEnd(Seg.getEnd());
      // See if it covers or overlaps any later segments.
      iterator j = i + 1;
      while (j != e) {
        if (j->getStart() > Seg.getEnd())
          break;
        i->setEnd(j->getEnd());
        if (j->getEnd() >= Seg.getEnd())
          break;
        ++j;
      }
      Segments.erase(i + 1, j);
    }
  } else if (i->getStart() == Seg.getEnd()) {
    // New segment abuts start of old segment i, without abutting or
    // overlapping anything before.
    i->setStart(Seg.getStart());
  } else {
    // New segment is completely in a hole just before i.
    Segments.insert(i, Seg);
  }
  IGC_ASSERT(testLiveRanges());
}

/***********************************************************************
 * LiveRange::setSegmentsFrom : for this live range, clear out its segments
 *    and copy them from the other live range
 */
void LiveRange::setSegmentsFrom(LiveRange *Other)
{
  Segments.clear();
  Segments.append(Other->Segments.begin(), Other->Segments.end());
}

/***********************************************************************
 * LiveRange::addSegments : add segments of LR2 into this
 */
void LiveRange::addSegments(LiveRange *LR2)
{
  Segments.append(LR2->Segments.begin(), LR2->Segments.end());
}

/***********************************************************************
 * LiveRange::sortAndMerge : after doing some push_backs, sort the segments,
 *      and merge overlapping/adjacent ones
 */
void LiveRange::sortAndMerge() {
  std::sort(Segments.begin(), Segments.end());

  // Ensure that there are no duplicate segments:
  Segments_t::iterator ip;
  ip = std::unique(Segments.begin(), Segments.end());
  Segments.resize(std::distance(Segments.begin(), ip));

  Segments_t SegmentsSortedEnd = Segments;
  std::sort(SegmentsSortedEnd.begin(), SegmentsSortedEnd.end(),
            [](Segment L, Segment R) {
              if (L.getEnd() != R.getEnd())
                return L.getEnd() < R.getEnd();
              return L.getStart() < R.getStart();
            });

  Segments_t NewSegments;
  std::unordered_set<Segment> OpenedSegments;
  Segment *SS = Segments.begin();
  Segment *ES = SegmentsSortedEnd.begin();
  unsigned prevBorder = 0;
  unsigned curBorder = 0;
  bool isStartBorder;

  // Split & Merge
  while (ES != SegmentsSortedEnd.end()) {
    if (SS != Segments.end() && SS->getStart() < ES->getEnd()) {
      isStartBorder = true;
      curBorder = SS->getStart();
    } else {
      isStartBorder = false;
      curBorder = ES->getEnd();
    }

    // To create or extend segment, first check that there are
    // open segments or that we haven't already created or extended one
    if (OpenedSegments.size() > 0 && prevBorder < curBorder) {
      Segment NS =
          *std::max_element(OpenedSegments.begin(), OpenedSegments.end(),
                            [](Segment L, Segment R) {
                               return L.Strength < R.Strength;
                            }); // New segment
      if (NewSegments.size() > 0 &&
          NewSegments.rbegin()->getEnd() == prevBorder &&
          // This segment and previous segment abut or overlap. Merge
          // as long as they have the same strength.
          (NS.Strength == NewSegments.rbegin()->Strength ||
           // Also allow for the case that the first one is strong and the
           // second one is phicpy. The resulting merged segment is strong,
           // because a phicpy segment is valid only if it starts in the
           // same place as when it was originally created and there is no
           // liveness just before it.
           (NS.Strength == Segment::PHICPY &&
            NewSegments.rbegin()->Strength == Segment::STRONG))) {
        // In these cases we can extend
        NewSegments.rbegin()->setEnd(curBorder);
      } else {
        NS.setStart(prevBorder);
        NS.setEnd(curBorder);
        NewSegments.push_back(NS);
      }
    }
    prevBorder = curBorder;
    if (isStartBorder)
      OpenedSegments.insert(*SS++);
    else
      OpenedSegments.erase(*ES++);
  }
  Segments = NewSegments;
}

/***********************************************************************
 * LiveRange::getLength : add up the number of instructions covered by this LR
 */
unsigned LiveRange::getLength(bool WithWeak) const {
  unsigned Length = 0;
  for (auto i = begin(), e = end(); i != e; ++i) {
    if (i->isWeak() && !WithWeak)
      continue;
    Length += i->getEnd() - i->getStart();
  }
  return Length;
}

/***********************************************************************
 * LiveRange::print : print the live range
 * Simplevalues of LR : segments { details }
 * Detailed mode exists to print LLVM values
 */
void LiveRange::print(raw_ostream &OS, bool Details) const {
  if (Details)
    OS << "LR values:\n";

  if (Values.empty()) {
    OS << "<Empty LR>";
    return;
  }

  for (auto &&V : Values) {
    if (!Details)
      OS << V << "; ";
    else
      OS << *V.getValue() << "\n";
  }

  if (!Details)
    OS << ":";
  else
    OS << "LR Segments and details: ";

  if (Segments.empty())
    OS << "<Empty Segments>";
  else
    printSegments(OS);

  StringRef Cat = vc::getRegCategoryName(Category);
  OS << "{" << Cat << ",align" << (1U << LogAlignment);
  if (Offset)
    OS << ",offset" << Offset;
  OS << "}";
}

/***********************************************************************
 * LiveRange::printSegments : print the live range's segments
 */
void LiveRange::printSegments(raw_ostream &OS) const
{
  for (auto ri = Segments.begin(), re = Segments.end();
      ri != re; ++ri) {
    OS << "[";
    switch (ri->Strength) {
      case Segment::WEAK: OS << "w"; break;
      case Segment::PHICPY: OS << "ph"; break;
      case Segment::STRONG: /* do nothing */ break;
    }
    OS << ri->getStart() << "," << ri->getEnd() << ")";
  }
}

/***********************************************************************
 * CallGraph::build : build the call graph for the FunctionGroup
 *
 * The call graph is acyclic because no recursive edges added here
 * CM supports recursion though
 */
void genx::CallGraph::build(GenXLiveness *Liveness) {
  Nodes.clear();
  // Create a node for each Function.
  for (auto fgi = FG->begin(), fge = FG->end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    (void)Nodes[F];
  }
  // For each Function, find its call sites and add edges for them.
  for (auto fgi = FG->begin() + 1, fge = FG->end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    for (Value::use_iterator ui = F->use_begin(), ue = F->use_end(); ui != ue;
         ++ui) {
      // TODO: deduce possible callsites thru cast chains
      if (isa<CallInst>(ui->getUser())) {
        auto Call = cast<CallInst>(ui->getUser());
        auto Caller = Call->getParent()->getParent();
        // do not add edges for indirect, recursive and intrinsic calls
        if (Call->getCalledFunction() &&
            !GenXIntrinsic::isAnyNonTrivialIntrinsic(Call->getCalledFunction()) &&
            Caller != F)
          Nodes[Caller].insert(
              Edge(Liveness->getNumbering()->getNumber(Call), Call));
      }
    }
  }
}

INITIALIZE_PASS_BEGIN(GenXLivenessWrapper, "GenXLivenessWrapper",
                      "GenXLivenessWrapper", false, false)
INITIALIZE_PASS_END(GenXLivenessWrapper, "GenXLivenessWrapper",
                    "GenXLivenessWrapper", false, false)

ModulePass *llvm::createGenXLivenessWrapperPass() {
  initializeGenXLivenessWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXLivenessWrapper();
}
