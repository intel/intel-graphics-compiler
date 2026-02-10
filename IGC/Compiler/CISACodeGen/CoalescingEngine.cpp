/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===-------- CoalescingEngine.cpp - Pass and analysis for coalescing-------===//
//
//  Intel Extention to LLVM core
//===----------------------------------------------------------------------===//
//
// This pass is an implementation of idea of coalescing originated from USC
// but re-designed to work (and take advantage of) on SSA form.
//
//===----------------------------------------------------------------------===//

#include "Compiler/CISACodeGen/CoalescingEngine.hpp"
#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/CISACodeGen/helper.h"
#include "PayloadMapping.hpp"
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::Debug;
using namespace IGC::IGCMD;

char CoalescingEngine::ID = 0;
#define PASS_FLAG "CoalescingEngine"
#define PASS_DESCRIPTION "coalesce moves coming payloads, insert and extract element"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(CoalescingEngine, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(LiveVarsAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenPatternMatch)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(DeSSA)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(ResourceLoopAnalysis)
IGC_INITIALIZE_PASS_END(CoalescingEngine, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC {

CoalescingEngine::CoalescingEngine() : FunctionPass(ID) {
  initializeCoalescingEnginePass(*PassRegistry::getPassRegistry());
}

void CoalescingEngine::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<llvm::DominatorTreeWrapperPass>();
  AU.addRequired<WIAnalysis>();
  AU.addRequired<LiveVarsAnalysis>();
  AU.addRequired<CodeGenPatternMatch>();
  AU.addRequired<DeSSA>();
  AU.addRequired<MetaDataUtilsWrapper>();
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<ResourceLoopAnalysis>();
}

void CoalescingEngine::CCTuple::print(raw_ostream &OS, const Module *) const {
  OS << "CC Tuple ";

  auto IT = OffsetToCCMap.begin();
  while (IT != OffsetToCCMap.end()) {
    OS << IT->first << " : ";
    IT++;
  }
}

void CoalescingEngine::CCTuple::dump() const { print(ods()); }

void CoalescingEngine::print(raw_ostream &OS, const Module *) const {
  OS << "CC Tuple list: \n";

  for (auto itr = m_CCTupleList.begin(), iend = m_CCTupleList.end(); itr != iend; ++itr) {
    CCTuple *ccTuple = *itr;
    ccTuple->print(OS);
  }

  OS << "CC Tuple list end. \n";
}

void CoalescingEngine::dump() const { print(ods()); }

/// \brief Entry point.
bool CoalescingEngine::runOnFunction(Function &MF) {
  MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  m_ModuleMetadata = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  m_CodeGenContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_Platform = m_CodeGenContext->platform;
  m_PayloadMapping = PayloadMapping(m_CodeGenContext);
  if (IGC_IS_FLAG_ENABLED(DisablePayloadCoalescing) || m_ModuleMetadata->compOpt.WaDisablePayloadCoalescing) {
    return false;
  }
  if (!isEntryFunc(pMdUtils, &MF)) {
    return false;
  }
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  WIA = &getAnalysis<WIAnalysis>();
  CG = &getAnalysis<CodeGenPatternMatch>();
  LV = &getAnalysis<LiveVarsAnalysis>().getLiveVars();
  RLA = &getAnalysis<ResourceLoopAnalysis>();
  if (IGC_IS_FLAG_ENABLED(EnableDeSSA)) {
    m_DeSSA = &getAnalysis<DeSSA>();
  } else {
    m_DeSSA = nullptr;
  }

  for (Function::iterator I = MF.begin(), E = MF.end(); I != E; ++I) {
    ProcessBlock(&(*I));
  }

  for (df_iterator<DomTreeNode *> DI = df_begin(DT->getRootNode()), DE = df_end(DT->getRootNode()); DI != DE; ++DI) {

    IncrementalCoalesce(DI->getBlock());
  }
  if (IGC_IS_FLAG_ENABLED(PrintToConsole))
    dump();
  return false;
}

/// Coalesce instructions within a single-BB
void CoalescingEngine::IncrementalCoalesce(BasicBlock *MBB) {
  std::vector<Instruction *> &DefInstrs = BBProcessingDefs[MBB];
  std::sort(DefInstrs.begin(), DefInstrs.end(), MIIndexCompare(LV));

  for (std::vector<Instruction *>::const_iterator BBI = DefInstrs.begin(), BBE = DefInstrs.end(); BBI != BBE; ++BBI) {
    Instruction *DefMI = *BBI;

    auto RI = ValueNodeMap.find((Value *)DefMI);
    if (RI == ValueNodeMap.end()) {
      // Intrinsics themselves are not mapped to a node, they are
      // tuple generators.

      // NOTE: Please leave this commented code snippet! It is useful when debugging coalescing regressions:
      //       (used to isolate coalescing to particular shader to check failing/passing using binary search)
      // DWORD hash = (DWORD) static_cast<IGC::CodeGenContext&>(MBB->getParent()->getContext()).hash.getAsmHash();
      ////if (hash >= 0xA27Affff && hash <= 0xA3CFFF00)
      // if (hash >= 0xA3C00000 && hash <= 0xA3CFFFFF)
      //{
      //     continue;
      // }

      if (GenIntrinsicInst *intrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(DefMI)) {
        if ((isURBWriteIntrinsic(intrinsic) && !IGC_IS_FLAG_ENABLED(DisablePayloadCoalescing_URB)) ||
            (llvm::isa<llvm::RTWriteIntrinsic>(intrinsic) && !IGC_IS_FLAG_ENABLED(DisablePayloadCoalescing_RT)) ||
            (llvm::isa<llvm::AtomicTypedIntrinsic>(intrinsic) &&
             !IGC_IS_FLAG_ENABLED(DisablePayloadCoalescing_AtomicTyped)) ||
            (llvm::isa<llvm::RTDualBlendSourceIntrinsic>(intrinsic) &&
             !IGC_IS_FLAG_ENABLED(DisablePayloadCoalescing_RT) && m_Platform.hasDualKSPPS())) {
          ProcessTuple(DefMI);
        }
      }

      if (isSampleInstruction(DefMI) && !IGC_IS_FLAG_ENABLED(DisablePayloadCoalescing_Sample)) {
        ProcessTuple(DefMI);
      } else if (isLdInstruction(DefMI)) {
        ProcessTuple(DefMI);
      }
    }
  }
}

static bool IsBindlessSampler(Instruction *inst) {
  if (SampleIntrinsic *sample = dyn_cast<SampleIntrinsic>(inst)) {
    Value *sampler = sample->getSamplerValue();
    uint bufferIndex = 0;
    bool directIndexing = false;
    if (sampler->getType()->isPointerTy()) {
      BufferType bufType =
          DecodeAS4GFXResource(sampler->getType()->getPointerAddressSpace(), directIndexing, bufferIndex);
      if (bufType == BINDLESS_SAMPLER) {
        return true;
      }
    }
  }
  return false;
}

/// Core algorithm.
void CoalescingEngine::ProcessTuple(Instruction *tupleGeneratingInstruction) {
  const uint numOperands = m_PayloadMapping.GetNumPayloadElements(tupleGeneratingInstruction);
  // bindless sampler always have a header
  bool needsHeader = IsBindlessSampler(tupleGeneratingInstruction);

  // There should be an early exit in the case we have split-send available
  // and this instruction is sure to generate two-Element payload.
  if (m_Platform.supportSplitSend() && !needsHeader) {
    if (numOperands < 3) {
      return;
    }
  } else {
    if (numOperands < 2) {
      return;
    }
  }
#if 0
        // turn off coalescing when inst is inside a fused resource-loop
        // feels like this approach may not be strong enough.
        // So opt to the solution of disabling coalescing in whole BB
        auto loopMarker =
            RLA->GetResourceLoopMarker(tupleGeneratingInstruction);
        if ((loopMarker & ResourceLoopAnalysis::MarkResourceLoopInside) ||
            ((loopMarker & ResourceLoopAnalysis::MarkResourceLoopStart) &&
             !(loopMarker & ResourceLoopAnalysis::MarkResourceLoopEnd)))
        {
            return;
        }
#endif
  IGC_ASSERT(numOperands >= 2);

  // No result, but has side effects of updating the split mapping.
  DecideSplit(tupleGeneratingInstruction);

  uint numSplits = GetNumSplitParts(tupleGeneratingInstruction);
  for (uint numPart = 0; numPart < numSplits; numPart++) {
    SetCurrentPart(tupleGeneratingInstruction, numPart);
    const uint numOperands = GetNumPayloadElements(tupleGeneratingInstruction);
    bool isAnyNodeAnchored = false, isAnyNodeCoalescable = false;
    SmallPtrSet<CCTuple *, MaxTouchedTuples> touchedTuplesSet;
    SmallVector<CCTuple *, MaxTouchedTuples> touchedTuples;

    // Step: Prepare.
    PrepareTuple(numOperands, tupleGeneratingInstruction,
                 // out:
                 touchedTuplesSet, touchedTuples, isAnyNodeAnchored, isAnyNodeCoalescable);

    if (!isAnyNodeCoalescable) {
      continue;
    }

    // So at this point, there is at least one node that is candidate for coalescing.
    // It might be already a part of CCtuple, or it might be yet unconstrained.

    // Tuple Generating Instruction enforces sequential order of registers allocated to values.
    // Each value might belong to (up to) one CCtuple.
    // If it belongs to CCTuple, it has an unique offset in that CCTuple.
    // Three cases are considered:
    // 1) No value have CCtuple assigned -> create new CCTuple and assign offsets.
    //     a) if platform supports split-send, then some (arbitrary) split point should be determined
    // 2) At least one value belongs to a CCtuple (we say it is 'anchored' (constrained) to that tuple)
    //    and all values belong to the same CCtuple
    // 3) More than one value belong to a tuple, and they belong to different (>1) tuples: NOT implemented

    // Case 1) : create new CCTuple
    if (!isAnyNodeAnchored) {
      CreateTuple(numOperands, tupleGeneratingInstruction);
    } else {
      // Case 2) : at least one value is 'anchored'(constrained) to a CCtuple
      // Sub-case: All arguments belong to the same tuple.
      if (touchedTuples.size() == 1) {
        // Step I: find an anchoring element.
        // If arguments of this tuple-instruction are already constrained by CCtuple, it means
        // that they also constrain the tuple that would be generated by this payload.
        // They 'anchor' the new tuple, so pick the element that anchors the tuple and
        // determine its offset.

        // E.g. :
        //  ..     v1 (0) v2 (1) CCtuple
        //  v0 (0) v1 (1) v2 (2) (this payload)
        //  v1 will have: thisTupleStartOffset = 1, rootTupleStartOffset = 0
        //  CCtuple will be 'grown' with element v0, that is at relative offset =  -1
        //  v0(-1) v1 (0) v2 (1) CCtuple

        CCTuple *ccTuple = NULL;
        int rootTupleStartOffset = 0;
        int thisTupleStartOffset = 0;

        DetermineAnchor(numOperands, tupleGeneratingInstruction,
                        // out:
                        ccTuple, rootTupleStartOffset, thisTupleStartOffset);

        IGC_ASSERT(ccTuple);

        /* Step II: Interference checking */
        int offsetDiff = rootTupleStartOffset - thisTupleStartOffset;

        bool interferes = false;

        // Heuristic: we do not want to grow the ccTuple size too much
        //  [c0 c1 c2 c3  c4  c5  c6  c7]
        //(offsetDiff=4)[e0' e1' e2' e3' e4' e5' e6' e7']
        //  here, tuple numElements = 8, and offsetDiff = 4, so it would result in
        //  12 element CC tuple if joined together. We want to prevent growing
        //  more than MaxTupleSize elements.
        if (abs(offsetDiff) > 0 && ccTuple->GetNumElements() >= MaxTupleSize) {
          interferes = true;
        }

        ProcessInterferencesElementFunctor interferencesFunctor(false /*forceEviction */, tupleGeneratingInstruction,
                                                                offsetDiff, ccTuple, this);

        if (!interferes) {
          interferes = InterferenceCheck(numOperands, tupleGeneratingInstruction, offsetDiff, ccTuple,
                                         // out:
                                         &interferencesFunctor);

          if (!interferes && (ccTuple->HasNonHomogeneousElements() ||
                              m_PayloadMapping.HasNonHomogeneousPayloadElements(tupleGeneratingInstruction))) {
            interferes =
                CheckIntersectionForNonHomogeneous(numOperands, tupleGeneratingInstruction, offsetDiff, ccTuple);
          }
        }

        // If 'interferes' is false, it means that whole transaction could be committed,
        // provided that elements in dominatorsForDisplacement are displaced, and other nodes are attached.
        // If interferes is true, then no element will be attached to the ccTuple.
        if (!interferes) {
          SmallPtrSet<Value *, MaxTouchedTuples> touchedValuesSet;
          for (uint i = 0; i < numOperands; i++) {
            Value *val = GetPayloadElementToValueMapping(tupleGeneratingInstruction, i);

            ccTuple->CheckIndexForNonInitializedSlot(i + offsetDiff);

            if (touchedValuesSet.count(val)) {
              continue;
            } else {
              touchedValuesSet.insert(val);
            }

            if (IsValConstOrIsolated(val)) {
              // do nothing
            } else {
              Value *RootV = getRegRoot(val);
              if (!RootV) // bail out if isolated
                continue;

              // We do not expect to get Argument here, it must have been isolated.
              IGC_ASSERT(dyn_cast<llvm::Instruction>(val));
              IGC_ASSERT(ValueNodeMap.count(val));
              // auto RI = ValueNodeMap.find(val);
              // ElementNode *Node = RI->second;
              ElementNode *Node = ValueNodeMap[val];

              if (!interferencesFunctor.GetComputedValuesForIsolation().count(val)) {
                ccTuple->AttachNodeAtIndex(i + offsetDiff, Node, *this);
                IGC_ASSERT(getRegRoot(val));
              }
            }
          } // for
        } else // (interferes) is true
        {
          CreateTuple(numOperands, tupleGeneratingInstruction);
        }
      } else {
        // Case 3)
        // More than one value belong to a tuple, and they belong to different (>1) tuples.
        // FIXME: not implemented yet
        // Need to implement code for joining two non-overlapping tuples.
        // This probably does not bring much benefit, but is nevertheless complex to implement.
        // Having two congruence class tuples, we need to find whether piecewise congruent classes
        // of the tuples interfere. Here a 'two-set interference check' due to Boissinot could be
        // used (first sorting elements)
      }
    }
  }
}

///
bool CoalescingEngine::CheckIntersectionForNonHomogeneous(const uint numOperands,
                                                          Instruction *tupleGeneratingInstruction, const int offsetDiff,
                                                          CCTuple *ccTuple) {
  // Picturesque description:
  //  Hi - homogeneous slots
  //  <left_part>[H_0 H_1 .. H_n]<right_part>

  // ccTuple:
  //  <left_part> [H_0  H_1  ..  H_n]<right_part>
  //
  // 1) this tuple instruction contains non-homogeneous part:
  //  <left_part'>[H_0' H_1' .. H_n']<right_part'>
  // 2) or this tuple instruction does not contain non-homogeneous part
  //              [H_0' H_1' .. H_n']

  bool interferes = false;
  if (ccTuple->HasNonHomogeneousElements()) {
    // Finding a supremum instruction for a homogeneous part is implemented
    // only for render target write instructions (RTWriteIntrinsic).
    // An only other possible combination for non-homogeneous instructions comprises
    // RTWriteIntrinsic and RTDualBlendSourceIntrinsic.
    // In some cases there is an opportunity to coalesce their payloads but there exists
    // a danger that they are compiled in different SIMD modes so there is a safe assumption
    // that they cannot be coalesced.
    // A comparison of the payload of RTWriteIntrinsic and RTDualBlendSourceIntrinsic:
    // +----------------------------+----------------------------+
    // | RTWriteIntrinsic            | RTDualBlendSourceIntrinsic |
    // +----------------------------+----------------------------+
    // | src0 Alpha (optional)      | src0 Alpha (unavailable)*  |
    // +----------------------------+----------------------------+
    // | output mask (optional)     | output mask (optional)     |
    // +----------------------------+----------------------------+
    // | -                          | R0 channel                 |
    // +----------------------------+----------------------------+
    // | -                          | G0 channel                 |
    // +----------------------------+----------------------------+
    // | -                          | B0 channel                 |
    // +----------------------------+----------------------------+
    // | -                          | A0 channel                 |
    // +----------------------------+----------------------------+
    // | R0 channel                 | R1 channel                 |
    // +----------------------------+----------------------------+
    // | G0 channel                 | G1 channel                 |
    // +----------------------------+----------------------------+
    // | B0 channel                 | B1 channel                 |
    // +----------------------------+----------------------------+
    // | A0 channel                 | A1 channel                 |
    // +----------------------------+----------------------------+
    // | Depth (optional)           | Depth (optional)           |
    // +----------------------------+----------------------------+
    // | Stencil (optional)         | Stencil (optional)         |
    // +----------------------------+----------------------------+
    // All optional fields except for src0 Alpha are consistent for both instruction called in a fragment shader.
    // * RTDualBlendSourceIntrinsic doesn't have such an argument but it is defined in its payload.
    if (offsetDiff == 0 && ccTuple->GetNumElements() == numOperands &&
        llvm::isa<llvm::RTWriteIntrinsic>(ccTuple->GetRoot()) &&
        llvm::isa<llvm::RTWriteIntrinsic>(tupleGeneratingInstruction)) {
      if (m_PayloadMapping.HasNonHomogeneousPayloadElements(tupleGeneratingInstruction)) {
        // TODO: test for 'supremum' of TGI and ccTuple root element can be obtained
        const Instruction *supremum =
            m_PayloadMapping.GetSupremumOfNonHomogeneousPart(tupleGeneratingInstruction, ccTuple->GetRoot());
        if (supremum) {
        } else {
          interferes = true;
        }
      }
    } else {
      // Since ccTuple contains non-homogeneous, and this homogeneous
      // part is not perfectly aligned to ccTuple, we cannot reason about
      // interferences safely, thus assume interferes.
      interferes = true;
    }
  } else {
    // ccTuple:
    //              [H_0  H_1  ..  H_n]
    //
    // 1) this tuple instruction contains non-homogeneous part:
    //  <left_part'>[H_0' H_1' .. H_n']<right_part'>

    if (m_PayloadMapping.HasNonHomogeneousPayloadElements(tupleGeneratingInstruction)) {
      if (offsetDiff == 0 && ccTuple->GetNumElements() == numOperands) {
        ccTuple->SetHasNonHomogeneousElements(tupleGeneratingInstruction);
      } else {
        interferes = true;
      }
    }
  }

  return interferes;
}

void CoalescingEngine::DecideSplit(Instruction *tupleGeneratingInstruction) {
  if (m_PayloadMapping.DoPeelFirstElement(tupleGeneratingInstruction)) {
    splitPoint_[tupleGeneratingInstruction] = 1;
    return;
  }

  uint splitPoint = 0;
  const uint numOperands = m_PayloadMapping.GetNumPayloadElements(tupleGeneratingInstruction);
  // No sense entering here if we have less than 2 operands.
  IGC_ASSERT(numOperands >= 2);
  Value *val = m_PayloadMapping.GetPayloadElementToValueMapping(tupleGeneratingInstruction, 0);
  if (IsValConstOrIsolated(val) || !getRegRoot(val)) {
    splitPoint = 1;
  }

  val = m_PayloadMapping.GetPayloadElementToValueMapping(tupleGeneratingInstruction, numOperands - 1);
  if (IsValConstOrIsolated(val) || !getRegRoot(val)) {
    splitPoint = numOperands - 1;
  }

  if (splitPoint > 0 && m_PayloadMapping.DoesAllowSplit(tupleGeneratingInstruction)) {
    splitPoint_[tupleGeneratingInstruction] = splitPoint;
  }
}

/// Determines if any value that is an argument of (possible) tuple generating instruction
/// can participate in payload coalescing. If so, it also determines if there are any
/// 'anchored'(constrained) values and the number of constraining tuples.
void CoalescingEngine::PrepareTuple(const uint numOperands, Instruction *tupleGeneratingInstruction,
                                    SmallPtrSet<CCTuple *, MaxTouchedTuples> &touchedTuplesSet,
                                    SmallVector<CCTuple *, MaxTouchedTuples> &touchedTuples, bool &isAnyNodeAnchored,
                                    bool &isAnyNodeCoalescable) {
  uint numConstants = 0;
  // Step: Prepare.
  for (uint i = 0; i < numOperands; i++) {
    Value *val = GetPayloadElementToValueMapping(tupleGeneratingInstruction, i);

    if (IsValConstOrIsolated(val) || !getRegRoot(val)) {
      numConstants++;
      continue;
    }

    Value *RootV = getRegRoot(val);
    IGC_ASSERT(RootV);
    {
      isAnyNodeCoalescable = true;

      IGC_ASSERT(ValueNodeMap.count(RootV));
      auto RI = ValueNodeMap.find(RootV);
      ElementNode *Node = RI->second;

      auto CCI = NodeCCTupleMap.find(Node);
      if (CCI != NodeCCTupleMap.end()) {
        CCTuple *ccTuple = NodeCCTupleMap[Node];
        if (!touchedTuplesSet.count(ccTuple)) {
          touchedTuplesSet.insert(ccTuple);
          touchedTuples.push_back(ccTuple);
        }

        isAnyNodeAnchored = true;
      }

      // Do not coalesce values having excessive number of uses.
      // This otfen introduces many anti-dependencies.
      const unsigned MAX_USE_COUNT = 20;
      if (val->hasNUsesOrMore(MAX_USE_COUNT) && !llvm::isa<llvm::AtomicTypedIntrinsic>(tupleGeneratingInstruction)) {
        isAnyNodeAnchored = true;
      }
    }
  }

  // heuristic:
  // we do not want to create 'wide' root variables, where most of the slots
  // will be anyway non-coalesced (due to immediate constants/isolated variables)
  // vISA will anyway not benefit, while this increases pressure and makes coloring
  // with round-robin heuristic harder
  if (numOperands > 2 && numConstants > (numOperands / 2)) {
    if (!m_PayloadMapping.HasNonHomogeneousPayloadElements(tupleGeneratingInstruction)) {
      // force non-coalescing:
      isAnyNodeCoalescable = false;
    }
  }
}

/// Creates fresh tuple.
/// Fresh tuple consists of nodes which do not have any tuple assigned yet.
/// Precondition is that at least one value is coalescable (otherwise does not make sense).
void CoalescingEngine::CreateTuple(const uint numOperands, Instruction *tupleGeneratingInstruction) {
  CCTuple *ccTuple = new CCTuple();
  m_CCTupleList.push_back(ccTuple);

  for (uint i = 0; i < numOperands; i++) {
    Value *val = GetPayloadElementToValueMapping(tupleGeneratingInstruction, i);

    if (IsValConstOrIsolated(val)) {
      // It is important to initialize CC with empty slots, in order to get the proper
      // size of the vISA variable that will back CCtuple that corresponds to payload.
      ccTuple->ResizeBounds(i);
      continue;
    }

    Value *RootV = getRegRoot(val);
    if (!RootV) { // isolated
      ccTuple->ResizeBounds(i);
      continue;
    }

    IGC_ASSERT(ValueNodeMap.count(RootV));
    ElementNode *RootNode = ValueNodeMap[RootV];

    auto CCTI = NodeCCTupleMap.find(RootNode);
    if (CCTI == NodeCCTupleMap.end()) {
      NodeCCTupleMap[RootNode] = ccTuple;
      NodeOffsetMap[RootNode] = i;
      ccTuple->InitializeIndexWithCCRoot(i, RootNode);
      CurrentDominatingParent[RootV] = RootV;
      ImmediateDominatingParent[RootV] = NULL;
    } else {
      // This must have been a value that is copied to payload multiple times.
      // OR: The new CCtuple has been isolated, and this is the element that
      // belongs to other tuple.
      // Since this value belongs to another tuple, we should not increase the
      // current tuple's size, no?
      // ccTuple->ResizeBounds(i);
    }
  } // loop over arguments

  if (m_PayloadMapping.HasNonHomogeneousPayloadElements(tupleGeneratingInstruction)) {
    ccTuple->SetHasNonHomogeneousElements(tupleGeneratingInstruction);
  }
}

/// Given a tuple generating instruction, picks out an 'anchored'(constrained)
/// value and determines the relative offset
/// output:
/// ccTuple              - a tuple that is constraining a picked value
/// thisTupleStartOffset - the offset of the value in the current tuple generating
///                        instruction
/// rootTupleStartOffset - the offset of the value in constraining tuple
/// Assumption: it is already checked all values belong to exactly one tuple!
/// (otherwise the result might pick an arbitrary tuple)
void CoalescingEngine::DetermineAnchor(const uint numOperands, const Instruction *tupleGeneratingInstruction,
                                       CCTuple *&ccTuple, int &rootTupleStartOffset, int &thisTupleStartOffset) {
  for (uint i = 0; i < numOperands; i++) {
    Value *val = GetPayloadElementToValueMapping(tupleGeneratingInstruction, i);

    if (IsValConstOrIsolated(val)) {
      continue;
    }
    Value *RootV = getRegRoot(val);
    if (!RootV)
      continue;

    IGC_ASSERT(ValueNodeMap.count(RootV));
    ElementNode *RootNode = ValueNodeMap[RootV];

    auto CCTI = NodeCCTupleMap.find(RootNode);
    if (CCTI != NodeCCTupleMap.end()) {
      // Element is constrained by ccTuple.
      IGC_ASSERT(NodeOffsetMap.count(RootNode));
      rootTupleStartOffset = NodeOffsetMap[RootNode];
      thisTupleStartOffset = i;
      ccTuple = NodeCCTupleMap[RootNode];
      // Just a sanity check.. :
      IGC_ASSERT(RootNode == ccTuple->GetCCNodeWithIndex(NodeOffsetMap[RootNode]));
      break;
    }

  } // for
}

/// Prepares the slot for moves (e.g. induced by const, isolated, uniform to vector)
/// into payload by evicting any value that is currently occupying this slot.
/// Assumption is that heuristic has determined it is profitable to do so.
/// input: bool evictFullCongruenceClass - tells whether the full congruence class
/// should be evicted, or only the element that occupies the tuple instruction
/// for the lifetime of a 'mov' (this is sufficient for issuing a mov to the payload slot).
void CoalescingEngine::PrepareInsertionSlot(CCTuple *ccTuple, const int index, Instruction *inst,
                                            const bool evictFullCongruenceClass) {
  ElementNode *RootNode = ccTuple->OffsetToCCMap[index];
  IGC_ASSERT(RootNode);

  if (evictFullCongruenceClass) {
    Value *dominatingParent = GetActualDominatingParent(RootNode->value, inst);
    // It might turn out that the root node does not dominate the
    // 'inst' since it is in an another branch of DT. In such case,
    // isolate all nodes in the CC.
    llvm::Value *CCVal = dominatingParent ? dominatingParent : CurrentDominatingParent[RootNode->value];
    while (CCVal) {
      if (getRegRoot(CCVal)) {
        isolateReg(CCVal);
      }
      CCVal = ImmediateDominatingParent[CCVal];
    }
  } else {
    // Evict dominating parent from CC.
    Value *NewParent = GetActualDominatingParent(RootNode->value, inst);
    if (NewParent)
      isolateReg(NewParent);
  }
}

bool CoalescingEngine::IsInsertionSlotAvailable(CCTuple *ccTuple, const int index,
                                                Instruction *tupleGeneratingInstruction, const bool canExtend) {
  bool avaiable = true;

  // ccTuple->OffsetToCCMap.count(index) == 0
  if (!canExtend) {
    const int tupleNumElements = ccTuple->GetNumElements();

    if (!(0 <= index && index < tupleNumElements)) {
      return false;
    }
  }

  ElementNode *nodeToCheck = ccTuple->GetCCNodeWithIndex(index);

  if (nodeToCheck == NULL) {
    // tuple   :  X       CC1   CC2
    // payload :  <slot>   v1        v2
    // Means: Tuple has no live interval (X) that is live at the point the payload is
    // defined. It means, that a given 'register slot' can be used to issue a 'mov'
    // with an immediate (or isolated) value.
  } else {
    // tuple   :  CC0       CC1   CC2
    //  ....       | <- lifetime
    // payload :  <slot>    v1       v2
    //  .....      | <- lifetime
    // Here we need to check whether the value in CC is live
    // and intersects with the desired <slot> .

    // Sanity check: tuple contains the root element.

    // IGC_ASSERT(getRegRoot(nodeToCheck->value) == nodeToCheck->value);
    Value *RootV = nodeToCheck->value;
    Value *NewParent = GetActualDominatingParent(RootV, tupleGeneratingInstruction);
    // FIXME: what if we do not have pure ssa form? Look at de-ssa comment.
    if (NewParent && LV->isLiveAt(NewParent, tupleGeneratingInstruction)) {
      avaiable = false;
      // interferes = true;
      // break;
    }
  }

  return avaiable;
}

/// Processes the tuple elements in sequential order.
/// A function object is passed as an argument, which responds to
/// specific events (whether given payload elements is const/isolated/copied etc.)
/// It is meant to be used as a template method in various context where different
/// functors could be passed, but the walking mechanism is the same.
void CoalescingEngine::ProcessElements(const uint numOperands, Instruction *tupleInst, const int offsetDiff,
                                       CCTuple *ccTuple, ElementFunctor *functor) {
  SmallPtrSet<Value *, MaxTouchedTuples> touchedValuesSet;

  for (uint i = 0; i < numOperands; i++) {
    functor->SetIndex(i);
    Value *val = GetPayloadElementToValueMapping(tupleInst, i);
    if (touchedValuesSet.count(val)) {
      if (functor->visitCopy())
        continue;
      else
        break;
    } else {
      touchedValuesSet.insert(val);
    }
    // Case 1: Constant
    if (llvm::isa<llvm::Constant>(val)) {
      if (functor->visitConstant())
        continue;
      else
        break;
    } else // Case 2: Variable
    {
      if (IsValIsolated(val) || !getRegRoot(val)) {
        // If value is isolated, a copy <slot> will be necessary.
        if (functor->visitIsolated())
          continue;
        else
          break;
      }

      IGC_ASSERT(ValueNodeMap.count(val));
      ElementNode *Node = ValueNodeMap[val];
      ElementNode *ccRootNode = ccTuple->GetCCNodeWithIndex(i + offsetDiff);

      // Interferes if and only if: live at this definition point AND has different value
      if (ccRootNode == Node) {
        // No interference, since it is the same value. We have got a reuse of the same value in
        // different tuple.
        if (functor->visitAnchored())
          continue;
        else
          break;
      } else {
        IGC_ASSERT(llvm::isa<llvm::Instruction>(val));
        // Here comes the meat, and the most interesting case:
        if (ccRootNode != NULL) {
          Value *CCRootV = ccRootNode->value;
          Value *dominating = nullptr;
          Value *dominated = nullptr;

          SymmetricInterferenceTest(CCRootV, dyn_cast<llvm::Instruction>(val), dominating, dominated);

          // Case 1):
          //    (dominating=null)
          //
          //!   (value)
          //
          //    CC dominance tree:
          //!   v67 <- (dominated)
          //    ^
          //    |
          //!   v190
          //
          //!   (tupleInst)
          // Value has no dominator inside CC class, but dominates elements in CC class
          // and its lifetime spans until (but not including) tupleInst.
          if (dominating == nullptr && dominated != nullptr) {
            // IGC_ASSERT(LV->isLiveAt(val, tupleInst));
            // Interference check: value is live at dominated
            if (functor->visitInterfering(val, false))
              continue;
            else
              break;
          } else if (dominating != nullptr && dominated != nullptr) {
            // Case 2):
            //  CC dominance tree
            //!  v67
            //    ^
            //    |
            //!  v121 (dominating)
            //    ^
            //!   |  <-----(value)
            //    |
            //!  v190 <- (dominated)
            // TODO: it would be possible to 'fit' v181 between v121 and v190 in some
            //       cases, but this would require redesign of the 'dominance forest' logic
            //       (regarding current dominating and immediate dominator handling)
            //       For now, just assume there is an interference.
            if (functor->visitInterfering(val, false))
              continue;
            else
              break;
          } else if (dominating != nullptr && dominated == nullptr) {
            // Case 3):
            //  CC dominance tree
            //!  v121
            //    ^
            //    |
            //!  v190 <- (dominating)
            //    |
            //!   |    ----- (value)
            //    |
            //    | (dominated == null)
            if (LV->isLiveAt(dominating, dyn_cast<llvm::Instruction>(val))) {
              if (functor->visitInterfering(val, false))
                continue;
              else
                break;
            }
          } else {
            // It is possible that the ccRootNode is not empty, but all
            // elements that belong to it have been isolated in previously
            // dominating tuples.
            // IGC_ASSERT(0);
          }
        } else {
          // ccRootNode == NULL -> congruence class not occupied yet
          if (functor->visitPackedNonInterfering(val))
            continue;
          else
            break;
        }
      } // if( ccRootNode == Node )
    } // if( llvm::isa<llvm::Constant>( val ) )
  } // for loop
}

/// Here we have two major differences with the implementation in de-ssa:
/// 1) Transactional character: even if one 'element' of CC-tuple does not interfere,
/// some other element in CC-tuple might be interfering, rendering the whole group
/// non-'coalescable'.
/// 2) Values in payload coalescing are by default isolated, so if there is an interference,
/// do nothing, no need for explicit isolation.
bool CoalescingEngine::InterferenceCheck(const uint numOperands, Instruction *tupleInst, const int offsetDiff,
                                         CCTuple *ccTuple, ProcessInterferencesElementFunctor *interferencesFunctor) {
  SmallPtrSet<Value *, MaxTouchedTuples> touchedValuesSet;
  GatherWeightElementFunctor gatherFunctor;
  ProcessElements(numOperands, tupleInst, offsetDiff, ccTuple, &gatherFunctor);
  bool forceEviction = (gatherFunctor.GetNumInsertionSlotsRequired() + gatherFunctor.GetNumNeedsDisplacement() <=
                        gatherFunctor.GetNumAlignedAnchors());

  interferencesFunctor->SetForceEviction(forceEviction);

  ProcessElements(numOperands, tupleInst, offsetDiff, ccTuple, interferencesFunctor);

  return interferencesFunctor->IsInterfering();
}

/// Given an instruction and numOperands (corresponding to the number of coalesce-partaking
/// payload elements), return whether any 'value' that is an argument of an intrinsic
/// takes part in the coalescing CC tuple. If yes, return non-zero value that is
/// the representative tuple for this coalescing.
/// output: zeroBasedPayloadElementOffset - an offset of this instructions payload
/// relative to the containing ccTuple - this is necessary to correctly slice the
/// the relevant part of the payload from ccTuple (e.g. for preparing a payload for URB writes)
///     offset [payload]
///     [ccTuple ...... ]
/// note: obviously, if return value is null, then the meaning of zeroBasedPayloadElementOffset
/// is undefined
CoalescingEngine::CCTuple *CoalescingEngine::IsAnyValueCoalescedInCCTuple(llvm::Instruction *inst,
                                                                          const uint numOperands,
                                                                          int &zeroBasedPayloadElementOffset,
                                                                          Value *&outVal) {
  outVal = nullptr;
  zeroBasedPayloadElementOffset = 0; // provide some meaningful default
  CoalescingEngine::CCTuple *ccTuple = nullptr;
  uint index = 0;
  while (index < numOperands) {
    Value *val = GetPayloadElementToValueMapping(inst, index);
    CoalescingEngine::CCTuple *ccTupleCheck = GetValueCCTupleMapping(val);
    if (!isa<Constant>(val) && ccTupleCheck) {
      ccTuple = ccTupleCheck;

      Value *valRoot = getRegRoot(val);
      IGC_ASSERT(valRoot);
      ElementNode *Node = ValueNodeMap[valRoot];
      int offset = NodeOffsetMap[Node];
      zeroBasedPayloadElementOffset = (offset - ccTuple->GetLeftBound()) - index;
      outVal = val;
      return ccTuple;
    }
    index++;
  }
  return ccTuple;
}

/// Return true if payload tuple might be completed (either by directly
/// aliasing or by inserting an appropriate copy instruction).
bool CoalescingEngine::IsPayloadCovered(llvm::Instruction *inst, CCTuple *ccTuple, const uint numOperands,
                                        const int payloadToCCTupleRelativeOffset) {
  uint relativeOffset = 0;
  bool payloadCovered = false;

  if (ccTuple) {
    SmallPtrSet<Value *, MaxTouchedTuples> touchedValuesSet;

    // index = 0;
    payloadCovered = true;
    for (uint index = 0; index < numOperands; index++) {
      Value *val = GetPayloadElementToValueMapping(inst, index);
      const int ccIndex = index + payloadToCCTupleRelativeOffset;

      if (touchedValuesSet.count(val)) {
        if (!IsInsertionSlotAvailable(ccTuple, ccIndex, inst, false)) {
          payloadCovered = false;
          break;
        }
        continue;
      } else {
        touchedValuesSet.insert(val);
      }

      if (IsValConstOrIsolated(val)) {
        if (!IsInsertionSlotAvailable(ccTuple, ccIndex, inst, false)) {
          payloadCovered = false;
          break;
        }
      } else {
        if (CoalescingEngine::CCTuple *thisCCTuple = GetValueCCTupleMapping(val)) {
          if (thisCCTuple == ccTuple) {
            relativeOffset = GetValueOffsetInCCTuple(val);
            if (relativeOffset != (ccIndex)) {
              payloadCovered = false;
              break;
            }
          } else {
            // different cc tuple
            payloadCovered = false;
            break;
          }

        } else {
          if (!IsInsertionSlotAvailable(ccTuple, ccIndex, inst, false)) {
            payloadCovered = false;
            break;
          }
        }
      } // if constant
    } // while
  } // if cctuple

  return payloadCovered;
}

/// Uses the tuple information provided by coalescing engine in order
/// to generate optimized sequence of 'movs' for preparing a payload.
CVariable *CoalescingEngine::PrepareExplicitPayload(CShader *outProgram, CEncoder *encoder, SIMDMode simdMode,
                                                    const DataLayout *pDL, llvm::Instruction *inst,
                                                    int &payloadOffsetInBytes) {
  SetCurrentPart(inst, 0);
  const uint numOperands = m_PayloadMapping.GetNumPayloadElements(inst);
  const uint grfSize = outProgram->GetContext()->platform.getGRFSize();
  const uint numSubVarsPerOperand = numLanes(simdMode) / (grfSize / 4);
  IGC_ASSERT(numSubVarsPerOperand == 1 || numSubVarsPerOperand == 2);
  CoalescingEngine::CCTuple *ccTuple = nullptr;
  int payloadToCCTupleRelativeOffset = 0;
  Value *dummyValPtr = nullptr;
  ccTuple = IsAnyValueCoalescedInCCTuple(inst, numOperands, payloadToCCTupleRelativeOffset, dummyValPtr);
  bool payloadCovered = IsPayloadCovered(inst, ccTuple, numOperands, payloadToCCTupleRelativeOffset);
  bool payloadOffsetComputed = false;
  payloadOffsetInBytes = 0;
  CVariable *payload = nullptr;

  if (payloadToCCTupleRelativeOffset < 0) {
    payloadCovered = false;
  }

  // Check for EOT payload.
  if (payloadCovered) {
    if (IGCLLVM::TerminatorInst *terminator = inst->getParent()->getTerminator()) {
      if (terminator != &(*terminator->getParent()->begin())) {
        IGC_ASSERT(dyn_cast<GenIntrinsicInst>(inst));
        IGC_ASSERT(terminator->getPrevNode());
        if (terminator->getPrevNode() == inst) {
          // heuristic:
          // this inst is an EOT node. Be very careful about payload size:
          // coalescing: hard constraint
          if (ccTuple->GetNumElements() > MaxTupleSize) {
            payloadCovered = false;
          }
        }
      }
    }
  }

  if (payloadCovered) {
    SmallPtrSet<Value *, MaxTouchedTuples> touchedValuesSet;

    for (uint index = 0; index < numOperands; index++) {
      Value *val = m_PayloadMapping.GetPayloadElementToValueMapping(inst, index);

      CVariable *dataElement = outProgram->GetSymbol(val);
      // FIXME: need to do additional checks for size
      uint subVar = (payloadToCCTupleRelativeOffset + index) * numSubVarsPerOperand;
      encoder->SetDstSubVar(subVar);
      payload = outProgram->GetCCTupleToVariableMapping(ccTuple);

      if (touchedValuesSet.count(val)) {
        encoder->Copy(payload, dataElement);
        encoder->Push();
        continue;
      } else {
        touchedValuesSet.insert(val);
      }

      if (IsValConstOrIsolated(val)) {
        encoder->Copy(payload, dataElement);
        encoder->Push();
      } else {
        if (GetValueCCTupleMapping(val)) {
          if (!payloadOffsetComputed) {
            payloadOffsetComputed = true;

            payloadOffsetInBytes = payloadToCCTupleRelativeOffset * GetSingleElementWidth(simdMode, pDL, val);
          }
        } else {
          // this one actually encompasses the case for !getRegRoot(val)
          encoder->Copy(payload, dataElement);
          encoder->Push();
        }
      } // if constant
    } // for

  } else {
    payload = outProgram->GetNewVariable(
        numOperands * numLanes(simdMode), ISA_TYPE_F,
        outProgram->GetContext()->platform.getGRFSize() == 64 ? EALIGN_32WORD : EALIGN_HWORD, "CEExplicitPayload");

    // insert explicit lifetime start in case some of the operands are undefs
    // otherwise, VISA will see the variable as not fully initialized and will extend the lifetime all the way to the
    // beginning of the kernel
    encoder->Lifetime(VISAVarLifetime::LIFETIME_START, payload);

    for (uint i = 0; i < numOperands; i++) {
      Value *val = m_PayloadMapping.GetPayloadElementToValueMapping(inst, i);
      CVariable *data = outProgram->GetSymbol(val);
      encoder->SetDstSubVar(i * numSubVarsPerOperand);
      encoder->Copy(payload, data);
      encoder->Push();
    }
  }

  return payload;
}


// Prepares payload for the uniform URB Write messages that are used in
// mesh and task shader stages.
// On DG2 platform a uniform URB write is a simd1 message that writes data
// from the fist DWORDs of each GRF of payload, e.g.:
//   (W)     mov (1|M0)               r16.0<1>:ud   0x0:ud
//   (W)     mov (1|M0)               r17.0<1>:f   -1.0:f
//   (W)     mov (1|M0)               r18.0<1>:f    0.0:f
//   (W)     mov (1|M0)               r19.0<1>:f    1.0:f
//   (W)     send.urb (1|M0)          null     r2   r16:4  0x0  0x020827F7
// On Xe2+ URB write messages are transposed LSC store messages that write
// consecutive DWORDs of payload, e.g.:
//   (W)     mov (1|M0)               r16.0<1>:ud   0x0:ud
//   (W)     mov (1|M0)               r16.1<1>:f   -1.0:f
//   (W)     mov (1|M0)               r16.2<1>:f    0.0:f
//   (W)     mov (1|M0)               r16.3<1>:f    1.0:f
//   (W)     store.urb.d32x4t.a32 (1|M0)  [r2:1+0x27F0] r16:1
CVariable *CoalescingEngine::PrepareUniformUrbWritePayload(CShader *shader, CEncoder *encoder,
                                                           llvm::GenIntrinsicInst *inst) {
  IGC_ASSERT(inst->getIntrinsicID() == GenISAIntrinsic::GenISA_URBWrite);
  CVariable *payload = nullptr;
  const uint numOperands = m_PayloadMapping.GetNumPayloadElements(inst);
  const uint grfSize = shader->GetContext()->platform.getGRFSize();
  if (shader->GetContext()->platform.hasLSCUrbMessage()) {
    // Payload for transposed LSC URB Write - all data chunks stored in subsequent DWORDs
    payload = shader->GetNewVariable(numOperands, ISA_TYPE_F, (grfSize == 64 ? EALIGN_32WORD : EALIGN_HWORD),
                                     true /*uniform*/, "CEExplicitPayload_Uniform");
  } else {
    // Payload spans multiple GRFs, only the first DWORD of each GRF
    // will be populated with (uniform) data.
    const uint elemsPerGrf = grfSize / CVariable::GetCISADataTypeSize(ISA_TYPE_F);
    payload = shader->GetNewVariable(numOperands * elemsPerGrf, ISA_TYPE_F,
                                     (grfSize == 64 ? EALIGN_32WORD : EALIGN_HWORD), "CEExplicitPayload");
  }
  for (uint i = 0; i < numOperands; i++) {
    Value *val = m_PayloadMapping.GetPayloadElementToValueMapping(inst, i);
    CVariable *data = shader->GetSymbol(val);
    IGC_ASSERT(isUniform(val) && data->IsUniform());
    encoder->SetNoMask();
    encoder->SetSimdSize(SIMDMode::SIMD1);
    if (shader->GetContext()->platform.hasLSCUrbMessage()) {
      encoder->SetDstSubReg(i);
    } else {
      encoder->SetDstSubVar(i);
    }
    encoder->Copy(payload, data);
  }
  return payload;
}

// Returns URB write payload for a split simd8 message in a simd16 mesh/task
// shader.
// TODO: This method does not use the CoalescingEngine currently and mearly
// copies payload to a newly created variable.
CVariable *CoalescingEngine::PrepareSplitUrbWritePayload(
    CShader *outProgram, CEncoder *encoder,
    SIMDMode simdMode,    ///< must be SIMD16
    uint32_t splitPartNo, ///< can be either 0 - lower quarter, or 1 upper quarter
    llvm::Instruction *inst) {
  SetCurrentPart(inst, 0);
  IGC_ASSERT(outProgram->m_SIMDSize == SIMDMode::SIMD16);
  IGC_ASSERT(splitPartNo < 2);
  [[maybe_unused]] const GenIntrinsicInst *const intrinsicInst = dyn_cast<GenIntrinsicInst>(inst);
  IGC_ASSERT(nullptr != intrinsicInst);
  IGC_ASSERT(intrinsicInst->getIntrinsicID() == GenISAIntrinsic::GenISA_URBWrite);

  const uint numOperands = m_PayloadMapping.GetNumPayloadElements(inst);
  CVariable *payload = outProgram->GetNewVariable(
      numOperands * numLanes(SIMDMode::SIMD8), ISA_TYPE_F,
      outProgram->GetContext()->platform.getGRFSize() == 64 ? EALIGN_32WORD : EALIGN_HWORD, CName::NONE);

  for (uint i = 0; i < numOperands; i++) {
    Value *val = m_PayloadMapping.GetPayloadElementToValueMapping(inst, i);
    CVariable *data = outProgram->GetSymbol(val);
    encoder->SetSimdSize(SIMDMode::SIMD8);
    encoder->SetMask((splitPartNo == 0) ? EMASK_Q1 : EMASK_Q2);
    encoder->SetSrcSubVar(0, data->IsUniform() ? 0 : splitPartNo);
    encoder->SetDstSubVar(i);
    encoder->Copy(payload, data);
    encoder->Push();
  }
  return payload;
}

CoalescingEngine::ElementNode *CoalescingEngine::ElementNode::getLeader() {
  ElementNode *N = this;
  ElementNode *Parent = parent.getPointer();
  ElementNode *Grandparent = Parent->parent.getPointer();

  while (Parent != Grandparent) {
    N->parent.setPointer(Grandparent);
    N = Grandparent;
    Parent = Parent->parent.getPointer();
    Grandparent = Parent->parent.getPointer();
  }

  return Parent;
}

Value *CoalescingEngine::getRegRoot(Value *Val) const {
  auto RI = ValueNodeMap.find(Val);
  if (RI == ValueNodeMap.end())
    return nullptr;
  ElementNode *Node = RI->second;
  if (Node->parent.getInt() & ElementNode::kRegisterIsolatedFlag)
    return 0x0;
  return Node->getLeader()->value;
}

Value *CoalescingEngine::getPHIRoot(Instruction *PHI) const {
  IGC_ASSERT(dyn_cast<PHINode>(PHI));
  auto RI = ValueNodeMap.find(PHI);
  IGC_ASSERT(RI != ValueNodeMap.end());
  ElementNode *DestNode = RI->second;
  if (DestNode->parent.getInt() & ElementNode::kPHIIsolatedFlag)
    return 0x0;
  for (unsigned i = 0; i < PHI->getNumOperands(); i++) {
    Value *SrcVal = PHI->getOperand(i);
    if (isa<Instruction>(SrcVal)) { // skip constant source
      Value *SrcRoot = getRegRoot(SrcVal);
      if (SrcRoot)
        return SrcRoot;
    }
  }
  return 0x0;
}

void CoalescingEngine::unionRegs(Value *Val1, Value *Val2) {
  ElementNode *Node1 = ValueNodeMap[Val1]->getLeader();
  ElementNode *Node2 = ValueNodeMap[Val2]->getLeader();

  if (Node1->rank > Node2->rank) {
    Node2->parent.setPointer(Node1->getLeader());
  } else if (Node1->rank < Node2->rank) {
    Node1->parent.setPointer(Node2->getLeader());
  } else if (Node1 != Node2) {
    Node2->parent.setPointer(Node1->getLeader());
    Node1->rank++;
  }
}

// For now, return true if V is dessa-aliased/InsEltMap-ed/phi-coalesced.
bool CoalescingEngine::isCoalescedByDeSSA(Value *V) const {
  if (m_DeSSA && m_DeSSA->getRootValue(V))
    return true;
  return false;
}

void CoalescingEngine::ProcessBlock(llvm::BasicBlock *bb) {
  llvm::BasicBlock::InstListType::iterator I, E;

  // Loop through instructions top to bottom
  for (I = bb->begin(), E = bb->end(); I != E; ++I) {
    llvm::Instruction &inst = (*I);
    auto loopMarker = RLA->GetResourceLoopMarker(&inst);
    // turn off coalescing in this block when there is a fused resource-loop
    if ((loopMarker & ResourceLoopAnalysis::MarkResourceLoopInside) ||
        ((loopMarker & ResourceLoopAnalysis::MarkResourceLoopStart) &&
         !(loopMarker & ResourceLoopAnalysis::MarkResourceLoopEnd))) {
      BBProcessingDefs[bb].clear();
      return;
    }

    visit(inst);
  }
}

bool CoalescingEngine::MatchSingleInstruction(llvm::GenIntrinsicInst *inst) {
  if (isSampleInstruction(inst) || isLdInstruction(inst) || isURBWriteIntrinsic(inst) ||
      llvm::isa<llvm::AtomicTypedIntrinsic>(inst) || llvm::isa<llvm::RTWriteIntrinsic>(inst) ||
      (llvm::isa<llvm::RTDualBlendSourceIntrinsic>(inst) && m_Platform.hasDualKSPPS())) {
    uint numOperands = inst->getNumOperands();
    for (uint i = 0; i < numOperands; i++) {
      Value *val = inst->getOperand(i);
      if (llvm::isa<llvm::Constant>(val)) {
        continue;
      }

      if (!ValueNodeMap.count(val)) {
        Instruction *DefMI = dyn_cast<Instruction>(val);
        if (DefMI) {
          BBProcessingDefs[DefMI->getParent()].push_back(DefMI);
        }
        ValueNodeMap[val] = new (Allocator) ElementNode(val);
      }
    }

    // Add itself to processing stack as well.
    BBProcessingDefs[inst->getParent()].push_back(inst);
  }

  return true;
}

void CoalescingEngine::visitCallInst(llvm::CallInst &I) {
  // we do not care if we do not match
  // bool match = true;
  if (GenIntrinsicInst *CI = llvm::dyn_cast<GenIntrinsicInst>(&I)) {
    switch (CI->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_ROUNDNE:
    case GenISAIntrinsic::GenISA_imulH:
    case GenISAIntrinsic::GenISA_umulH:
    case GenISAIntrinsic::GenISA_uaddc:
    case GenISAIntrinsic::GenISA_usubb:
    case GenISAIntrinsic::GenISA_bfrev:
      break;
    case GenISAIntrinsic::GenISA_intatomicraw:
    case GenISAIntrinsic::GenISA_floatatomicraw:
    case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_dwordatomicstructured:
    case GenISAIntrinsic::GenISA_floatatomicstructured:
    case GenISAIntrinsic::GenISA_cmpxchgatomicstructured:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicstructured:
    case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
    case GenISAIntrinsic::GenISA_floatatomictyped:
    case GenISAIntrinsic::GenISA_fcmpxchgatomictyped:
    case GenISAIntrinsic::GenISA_ldstructured:
    case GenISAIntrinsic::GenISA_atomiccounterinc:
    case GenISAIntrinsic::GenISA_atomiccounterpredec:
      break;
    case GenISAIntrinsic::GenISA_GradientX:
    case GenISAIntrinsic::GenISA_GradientY:
    case GenISAIntrinsic::GenISA_GradientXfine:
    case GenISAIntrinsic::GenISA_GradientYfine:
      break;
    default:
      MatchSingleInstruction(CI);
      // no pattern for the rest of the intrinsics
      break;
    }
  } else if (IntrinsicInst *CI = llvm::dyn_cast<IntrinsicInst>(&I)) {
    switch (CI->getIntrinsicID()) {
    case Intrinsic::sqrt:
    case Intrinsic::log2:
    case Intrinsic::cos:
    case Intrinsic::sin:
    case Intrinsic::pow:
    case Intrinsic::floor:
    case Intrinsic::ceil:
    case Intrinsic::trunc:
    case Intrinsic::maxnum:
    case Intrinsic::minnum:
      break;
    case Intrinsic::exp2:
      break;

    default:
      break;
    }
  }
}

void CoalescingEngine::visitCastInst(llvm::CastInst &I) {}
void CoalescingEngine::visitBinaryOperator(llvm::BinaryOperator &I) {}

void CoalescingEngine::visitCmpInst(llvm::CmpInst &I) {}

void CoalescingEngine::visitPHINode(llvm::PHINode &I) {}

void CoalescingEngine::visitUnaryInstruction(llvm::UnaryInstruction &I) {}

void CoalescingEngine::visitSelectInst(llvm::SelectInst &I) {}

void CoalescingEngine::visitBitCastInst(llvm::BitCastInst &I) {}

void CoalescingEngine::visitInstruction(llvm::Instruction &I) {}

void CoalescingEngine::visitLoadInst(LoadInst &I) {}

void CoalescingEngine::visitStoreInst(StoreInst &I) {}

} // namespace IGC
