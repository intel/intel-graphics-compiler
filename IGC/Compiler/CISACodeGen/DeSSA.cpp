/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

//===-------- DeSSA.cpp - divide phi variables into congruent class -------===//
//
//                     Intel LLVM Extention
//===----------------------------------------------------------------------===//
//
// This pass is originated from the StrongPHIElimination on the machine-ir.
// We have adopted it to work on llvm-ir. Also note that we have changed it
// from a transformation to an analysis, meaning which only divides phi-vars
// into congruent classes, and does NOT insert the copies. A separate code-gen
// pass can use this analysis to emit non-ssa target code.
//
// Algorithm and References:
//
// This pass consider how to eliminates PHI instructions by aggressively
// coalescing the copies that would otherwise be inserted by a naive algorithm
// and only inserting the copies that are necessary. The coalescing technique
// initially assumes that all registers appearing in a PHI instruction do not
// interfere. It then eliminates proven interferences, using dominators to only
// perform a linear number of interference tests instead of the quadratic number
// of interference tests that this would naively require.
// This is a technique derived from:
//
//    Budimlic, et al. Fast copy coalescing and live-range identification.
//    In Proceedings of the ACM SIGPLAN 2002 Conference on Programming Language
//    Design and Implementation (Berlin, Germany, June 17 - 19, 2002).
//    PLDI '02. ACM, New York, NY, 25-32.
//
// The original implementation constructs a data structure they call a dominance
// forest for this purpose. The dominance forest was shown to be unnecessary,
// as it is possible to emulate the creation and traversal of a dominance forest
// by directly using the dominator tree, rather than actually constructing the
// dominance forest.  This technique is explained in:
//
//   Boissinot, et al. Revisiting Out-of-SSA Translation for Correctness, Code
//     Quality and Efficiency,
//   In Proceedings of the 7th annual IEEE/ACM International Symposium on Code
//   Generation and Optimization (Seattle, Washington, March 22 - 25, 2009).
//   CGO '09. IEEE, Washington, DC, 114-125.
//
// Careful implementation allows for all of the dominator forest interference
// checks to be performed at once in a single depth-first traversal of the
// dominator tree, which is what is implemented here.
//===----------------------------------------------------------------------===//

#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/Instructions.h"
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InlineAsm.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"
#include <algorithm>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::Debug;
using namespace IGC::IGCMD;

#define PASS_FLAG "DeSSA"
#define PASS_DESCRIPTION "coalesce moves coming from phi nodes"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(DeSSA, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(LiveVarsAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenPatternMatch)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(DeSSA, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char DeSSA::ID = 0;

DeSSA::DeSSA() : FunctionPass(ID) { initializeDeSSAPass(*PassRegistry::getPassRegistry()); }

void DeSSA::print(raw_ostream &OS, const Module *) const {
  // Assign each inst/arg a unique integer so that the output
  // would be in order. It is useful when doing comparison.
  DenseMap<const Value *, int> Val2IntMap;
  int id = 0;
  if (m_F) {
    // All arguments
    for (auto AI = m_F->arg_begin(), AE = m_F->arg_end(); AI != AE; ++AI) {
      Value *aVal = &*AI;
      Val2IntMap[aVal] = (++id);
    }
    // All instructions
    for (auto II = inst_begin(m_F), IE = inst_end(m_F); II != IE; ++II) {
      Instruction *Inst = &*II;
      Val2IntMap[(Value *)Inst] = (++id);
    }
  }

  bool doSort = (!Val2IntMap.empty());

  auto valCmp = [&](const Value *V0, const Value *V1) {
    int n0 = Val2IntMap[V0];
    int n1 = Val2IntMap[V1];
    return n0 < n1;
  };

  SmallVector<Value *, 64> ValKeyVec;
  DenseMap<Value *, SmallVector<Value *, 8>> output;
  {
    OS << "---- AliasMap ----\n\n";
    for (auto &I : AliasMap) {
      Value *aliaser = I.first;
      Value *aliasee = I.second;
      SmallVector<Value *, 8> &allAliasers = output[aliasee];
      if (aliaser != aliasee) {
        allAliasers.push_back(aliaser);
      }
    }

    for (auto &I : output) {
      Value *key = I.first;
      ValKeyVec.push_back(key);
    }
    if (doSort) {
      std::sort(ValKeyVec.begin(), ValKeyVec.end(), valCmp);
    }
    for (auto &I : ValKeyVec) {
      Value *aliasee = I;
      SmallVector<Value *, 8> &allAliasers = output[aliasee];

      if (doSort) {
        std::sort(allAliasers.begin(), allAliasers.end(), valCmp);
      }

      OS << "  Aliasee: ";
      aliasee->print(OS);
      OS << "\n";
      for (int i = 0, sz = (int)allAliasers.size(); i < sz; ++i) {
        OS << "     ";
        allAliasers[i]->print(OS);
        OS << "\n";
      }
    }
    OS << "\n\n";
  }

  OS << "---- InsEltMap ----\n\n";
  output.clear();
  ValKeyVec.clear();
  for (auto &I : InsEltMap) {
    Value *val = I.first;
    Value *rootV = I.second;
    SmallVector<Value *, 8> &allVals = output[rootV];
    if (rootV != val) {
      allVals.push_back(val);
    }
  }

  for (auto &I : output) {
    Value *key = I.first;
    ValKeyVec.push_back(key);
  }
  if (doSort) {
    std::sort(ValKeyVec.begin(), ValKeyVec.end(), valCmp);
  }
  for (auto &I : ValKeyVec) {
    Value *rootV = I;
    SmallVector<Value *, 8> &allVals = output[rootV];

    if (doSort) {
      std::sort(allVals.begin(), allVals.end(), valCmp);
    }

    OS << "  Root Value : ";
    rootV->print(OS);
    OS << "\n";
    for (int i = 0, sz = (int)allVals.size(); i < sz; ++i) {
      OS << "       ";
      allVals[i]->print(OS);
      OS << "\n";
    }
  }
  OS << "\n\n";

  OS << "---- Alias for composite/vector values"
     << " (value in both AliasMap & InsEltMap)----\n";
  // All InsElt output has been sorted
  for (auto &I : ValKeyVec) {
    Value *rootV = I;
    SmallVector<Value *, 8> &allVals = output[rootV];

    OS << "  Root Value: ";
    rootV->printAsOperand(OS);
    if (isAliasee(rootV)) {
      OS << " [aliasee]";
    }

    int num = 0;
    for (int i = 0, sz = (int)allVals.size(); i < sz; ++i) {
      Value *val = allVals[i];
      if (!isAliasee(val))
        continue;
      if ((num % 8) == 0) {
        OS << "\n       ";
      }

      allVals[i]->printAsOperand(OS);
      OS << " [aliasee]  ";
      ++num;
    }
    OS << "\n";
  }
  OS << "\n\n";

  OS << "---- Phi-Var Isolations ----\n";
  SmallVector<Node *, 64> NodeKeyVec;
  std::map<Node *, SmallVector<Node *, 8>> nodeOutput;
  // std::map<Node*, int> LeaderVisited;
  for (auto I = RegNodeMap.begin(), E = RegNodeMap.end(); I != E; ++I) {
    Node *N = I->second;
    // We don't want to change behavior of DeSSA by invoking
    // dumping/printing functions. Thus, don't use getLeader()
    // as it has side-effect (doing path halving).
    Node *Leader = N->parent;
    while (Leader != Leader->parent) {
      Leader = Leader->parent;
    }

    SmallVector<Node *, 8> &allNodes = nodeOutput[Leader];
    if (N != Leader) {
      allNodes.push_back(N);
    }
  }

  auto nodeCmp = [&](const Node *N0, const Node *N1) {
    const Value *V0 = N0->value;
    const Value *V1 = N1->value;
    return valCmp(V0, V1);
  };

  for (auto &I : nodeOutput) {
    Node *key = I.first;
    NodeKeyVec.push_back(key);
  }
  if (doSort) {
    std::sort(NodeKeyVec.begin(), NodeKeyVec.end(), nodeCmp);
  }
  for (auto &I : NodeKeyVec) {
    Node *Leader = I;
    SmallVector<Node *, 8> &allNodes = nodeOutput[Leader];
    if (doSort) {
      std::sort(allNodes.begin(), allNodes.end(), nodeCmp);
    }

    Value *VL;
    if (isIsolated(Leader)) {
      IGC_ASSERT_MESSAGE(allNodes.size() == 0, "ICE: isolated node still have other in its CC!");
      VL = Leader->value;
      OS << "\nVar isolated : ";
      VL->print(OS);
      OS << "\n";
    } else {
      OS << "\nLeader : ";
      Leader->value->print(OS);
      OS << "\n";
      for (auto &II : allNodes) {
        Node *N = II;
        VL = N->value;
        OS << "    ";
        VL->print(OS);
        OS << "\n";
        N = N->next;
      }
    }
  }
}

void DeSSA::dump() const { print(dbgs()); }

bool DeSSA::runOnFunction(Function &MF) {
  if (IGC_IS_FLAG_DISABLED(EnableDeSSA)) {
    LV = nullptr;
    WIA = nullptr;
    // getRootValue(), isIsolated(), getLiveVars(), etc. still work.
    return false;
  }

  m_F = &MF;
  CurrColor = 0;
  MetaDataUtils *pMdUtils = nullptr;
  pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  if (pMdUtils->findFunctionsInfoItem(&MF) == pMdUtils->end_FunctionsInfo()) {
    return false;
  }
  CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  WIA = &getAnalysis<WIAnalysis>();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  CG = &getAnalysis<CodeGenPatternMatch>();
  DL = &MF.getParent()->getDataLayout();
  LV = &getAnalysis<LiveVarsAnalysis>().getLiveVars();

  // make sure we do not run WIAnalysis between CodeGen and DeSSA,
  // therefore m_program's Uniform Helper is still valid, which is
  // used indirectly in DeSSA::GetPhiTemp().
  // If we cannot maintain this assertion, then we should do
  //   m_program->SetUniformHelper(WIA);

  //
  // The DeSSA/Coalescing procedure:
  //   1. Follow Dominance tree to set up alias map. While setting up alias map,
  //      update liveness for aliasee so that alasee's liveness is the sum of
  //      all its aliasers.
  //      By aliaser/aliasee, it means the following:
  //             aliaser = bitcast aliasee
  //      (Most aliasing is from bitcast, some can be from other cast instructions
  //       such as inttoptr/ptrtoint. It could be also from insElt/extElt.)
  //
  //      By traversing dominance tree depth-first (DF), it is guaranteed that
  //      a def will be visited before its use except PHI node. Since PHI inst
  //      is not a candidate for aliasing, this means that the def of aliasee has
  //      been visited before the aliaser instruction. For example,
  //            x = bitcast y
  //         The def of y should be visited before visiting this bitcast inst.
  //      Let alias(v0, v1) denote that v0 is an alias to v1. This visiting order
  //      under DF dominance-tree traversal will not see aliasing in the following
  //      order:
  //              alias(v0, v1)
  //              alias(v1, v2)
  //      rather, it must be in the order
  //              alias(v1, v2)
  //              alias(v0, v1)
  //
  //   2. Set up InsEltMap, which coalesces vector values used in InsertElement
  //      instructions. It is treated as "alias", meaning the root value's
  //      liveness is the sum of all its non-root values. The difference b/w
  //      AliasMap and InsEltMap is that AliasMap is pure alias in that all
  //      aliasers have the same values as its aliasee (ones of primitive types);
  //      while InsElt has composite/vector values. This difference does not matter
  //      in dessa, but it would matter when handling sub-vector aliasing after
  //      dessa (VariableReuseAnaysis uses experimental sub-vector aliasing under
  //      the key whose default is off).
  //
  //      We could remove InsEltMap/AliasMap by adding each value into DeSSA node.
  //      To do so, the dessa algo needs to be improved to isolate them together
  //      if they need to be isolated. It also may involve several subtle changes
  //      in implementation. Also, adding all values instead of a single one into
  //      CC increases the size of CC, which could increase compiling time. With
  //      this, let us stay with insEltMap (as well as aliasMap).
  //   3. Make sure DeSSA node only use the 'node value', that is, given value V,
  //        its Node value:
  //                V_aliasee = AliasMap[V] if V is in map, or V otherwise
  //                node_value = InsEltMap[V_aliasee] if in InsEltMap; or V_aliasee
  //                             otherwise.
  //      Note that since the type of aliasess may be different from aliaser,
  //      the values in the same CC will have different types.  Keep this in mind
  //      when creating CVariable in GetSymbol().
  //
  //  Note that the algorithem forces coalescing of aliasing inst and InsertElement
  //  inst before PHI-coalescing, which means it favors coaslescing of those aliasing
  //  inst and InsertElement instructions. Thus, values in AliasMap/InsEltMap are
  //  guananteed to be coalesced together at the end of DeSSA. PHI coalescing may
  //  extend those maps by adding other values.
  //
  CoalesceAliasInst();
  CoalesceInsertElements();

  // checkPHILoopInput
  //  PreHeader:
  //      x = ...
  //  Header:
  //      phi0 = [x, PreHeader], [t0, End]
  //      phi1 = [x, PreHeader], [t1, End]
  //      phi2 = [x, PreHeader], [t2, End]
  //      ...
  //  End:
  //      ...
  //      goto Header
  //
  //  The algorithme below will start with a largest congruent class possible,
  //  which unions all phi's with the same source operands. This ends up with
  //  a single congruent class of all phi's with x as their source operand.
  //  Later, the algorithm isolates phi's as they interfere with each other,
  //  causing mov instructions to be generated within the loop at BB End.
  //
  //  However, since all phi instructions are live at the same time, we will
  //  not be able to coalesce them. In another word, there is no need to put
  //  all phi's into the same congruent class in the first place. To achieve
  //  this, we use a Value-to-int map to keep how many times a value is used
  //  in the phi's,  and if the number of uses is over a threshold, we will
  //  isolate the source operand and do not union it with its phi. In doing
  //  so it is likely for the algorithm to coalesce the phi's dst and the
  //  other src that is used in the loop, and therefore remove mov instrutions
  //  in the loop.
  //
  //  Note that isolating a value introduce additional copy, thus a threshold
  //  is used here as a heuristic to try to make sure that a benefit is more
  //  than the cost.
  enum { PHI_SRC_USE_THRESHOLD = 3 }; // arbitrary number
  DenseMap<Value *, int> PHILoopPreHeaderSrcs;

  // build initial congruent class using union-find
  for (Function::iterator I = MF.begin(), E = MF.end(); I != E; ++I) {
    // First, initialize PHILoopPreHeaderSrcs map
    BasicBlock *MBB = &*I;
    Loop *LP = LI ? LI->getLoopFor(MBB) : nullptr;
    BasicBlock *PreHeader = LP ? LP->getLoopPredecessor() : nullptr;
    bool checkPHILoopInput = LP && (LP->getHeader() == MBB) && PreHeader;
    PHILoopPreHeaderSrcs.clear();
    if (checkPHILoopInput) {
      for (BasicBlock::iterator BBI = I->begin(), BBE = I->end(); BBI != BBE; ++BBI) {
        PHINode *PHI = dyn_cast<PHINode>(BBI);
        if (!PHI) {
          break;
        }

        int srcIx = PHI->getBasicBlockIndex(PreHeader);
        if (srcIx < 0) {
          continue;
        }
        Value *SrcVal = PHI->getOperand(srcIx);
        if (isa<Constant>(SrcVal)) {
          continue;
        }
        if (PHILoopPreHeaderSrcs.count(SrcVal) == 0) {
          PHILoopPreHeaderSrcs[SrcVal] = 0; // initialize to zero
        }
        PHILoopPreHeaderSrcs[SrcVal] += 1;
      }
    }

    for (BasicBlock::iterator BBI = I->begin(), BBE = I->end(); BBI != BBE; ++BBI) {
      PHINode *PHI = dyn_cast<PHINode>(BBI);
      if (!PHI) {
        break;
      }

      e_alignment DefAlign = GetPreferredAlignment(PHI, WIA, CTX);
      IGC_ASSERT(PHI == getNodeValue(PHI));

      addReg(PHI, DefAlign);
      PHISrcDefs[&(*I)].push_back(PHI);

      for (unsigned i = 0; i < PHI->getNumOperands(); ++i) {
        Value *OrigSrcVal = PHI->getOperand(i);
        // skip constant
        if (isa<Constant>(OrigSrcVal))
          continue;

        // condition for preheader-src-isolation
        bool PreheaderSrcIsolation =
            (checkPHILoopInput && !isa<InsertElementInst>(OrigSrcVal) && !isa<PHINode>(OrigSrcVal) &&
             PHI->getIncomingBlock(i) == PreHeader && PHILoopPreHeaderSrcs.count(OrigSrcVal) > 0 &&
             PHILoopPreHeaderSrcs[OrigSrcVal] >= PHI_SRC_USE_THRESHOLD);
        // add src to the union
        Value *SrcVal;
        SrcVal = getNodeValue(OrigSrcVal);
        e_alignment SrcAlign = GetPreferredAlignment(OrigSrcVal, WIA, CTX);
        Instruction *DefMI = dyn_cast<Instruction>(SrcVal);
        if (DefMI) {
          if (CG->SIMDConstExpr(DefMI)) {
            continue; // special case, simdSize becomes a constant in vISA
          }
          addReg(SrcVal, SrcAlign);
          PHISrcDefs[DefMI->getParent()].push_back(DefMI);
          if (WIA->whichDepend(PHI) == WIA->whichDepend(SrcVal) && !PreheaderSrcIsolation) {
            unionRegs(PHI, SrcVal);
          }
        } else if (isa<Argument>(SrcVal)) {
          addReg(SrcVal, SrcAlign);
          PHISrcArgs.insert(SrcVal);
          if (WIA->whichDepend(PHI) == WIA->whichDepend(SrcVal) && !PreheaderSrcIsolation) {
            unionRegs(PHI, SrcVal);
          }
        }
        // cases that we need to isolate source
        if (CG->IsForceIsolated(SrcVal) || PreheaderSrcIsolation) {
          isolateReg(SrcVal);
        }
      } // end of source-operand loop
      // isolate complex type that IGC does not handle
      if (PHI->getType()->isStructTy() || PHI->getType()->isArrayTy()) {
        isolateReg(PHI);
      }
    }
  }

  // \todo, the original paper talks aibout some before-hand quick
  // isolation. The idea is to identify those essential splitting first
  // in order to avoid unnecessary splitting in the next loop.

  // Perform a depth-first traversal of the dominator tree, splitting
  // interferences amongst PHI-congruence classes.
  if (!RegNodeMap.empty()) {
    DenseMap<int, Value *> CurrentDominatingParent;
    DenseMap<Value *, Value *> ImmediateDominatingParent;
    // first, go through the function arguments
    SplitInterferencesForArgument(CurrentDominatingParent, ImmediateDominatingParent);
    // Then all the blocks
    for (df_iterator<DomTreeNode *> DI = df_begin(DT->getRootNode()), DE = df_end(DT->getRootNode()); DI != DE; ++DI) {
      SplitInterferencesForBasicBlock(DI->getBlock(), CurrentDominatingParent, ImmediateDominatingParent);
    }
  }

  // Handle values that have specific alignment requirement.
  SplitInterferencesForAlignment();

  if (IGC_IS_FLAG_ENABLED(DumpDeSSA)) {
    const char *fname = MF.getName().data();
    using namespace IGC::Debug;
    auto name = DumpName(GetShaderOutputName())
                    .Hash(CTX->hash)
                    .Type(CTX->type)
                    .Pass("dessa")
                    .PostFix(fname)
                    .Retry(CTX->m_retryManager->GetRetryId())
                    .Extension("txt");

    Dump dessaDump(name, DumpType::DBG_MSG_TEXT);

    DumpLock();
    print(dessaDump.stream());
    DumpUnlock();
  } else if (IGC_IS_FLAG_ENABLED(PrintToConsole)) {
    print(ods());
  }
  m_F = nullptr;
  return false;
}

void DeSSA::CoalesceAliasInst() {
  for (df_iterator<DomTreeNode *> DI = df_begin(DT->getRootNode()), DE = df_end(DT->getRootNode()); DI != DE; ++DI) {
    BasicBlock *Blk = DI->getBlock();

    for (BasicBlock::iterator BBI = Blk->begin(), BBE = Blk->end(); BBI != BBE; ++BBI) {
      Instruction *I = &(*BBI);

      // We are after patternmatch, would it make more sense to
      // iterate all patterns instead of instructions ?
      if (!CG->NeedInstruction(*I)) {
        continue;
      }

      if (InsertElementInst *IEI = dyn_cast<InsertElementInst>(I)) {
        if (isa<UndefValue>(I->getOperand(0))) {
          SmallVector<Value *, 16> AllIEIs;
          int nelts = checkInsertElementAlias(IEI, AllIEIs);
          if (nelts > 1) {
            //  Consider the following as an alias if all
            //  Vi, i=0, n-1 (except Vn) has a single use.
            //     V0 = InsElt undef, S0, 0
            //     V1 = InsElt V0, S1, 1
            //     ...
            //     Vn = InsElt Vn-1, Sn, n
            //
            //  AliasMap has the following:
            //     alias(V0, V0)
            //     alias(V1, V0)
            //     alias(V2, V0)
            //     ......
            //     alias(Vn, V0)  <-- V0 is the root!
            //
            //  Note that elements could be sparse like
            //     V0 = InsElt Undef, S1, 1
            //     V1 = InsElt V0,    s3, 2
            //
            Value *aliasee = AllIEIs[0];
            AddAlias(aliasee);
            for (int i = 1; i < nelts; ++i) {
              Value *V = AllIEIs[i];
              AliasMap[V] = aliasee;

              // union liveness info
              LV->mergeUseFrom(aliasee, V);
            }
          }
        }
      }
      if (InsertValueInst *IVI = dyn_cast<InsertValueInst>(I)) {
        // Handle insertValue to struct.
        //
        // insertvalue exists when
        //   1. there are calls to functions with struct-type arguments
        //      and struct is simple (for example, no struct type as its
        //      field type) and its size is small (<= 16 bytes ?) so that
        //      it will be  passed as struct. (Normally, FE will turn a
        //      larger/not-simple struct arg into a "byval" pointer.)
        //   2. igc generates it internally
        coalesceAliasInsertValue(IVI);
      } else if (CastInst *CastI = dyn_cast<CastInst>(I)) {
        Value *D = CastI;
        Value *S = CastI->getOperand(0);
        // For vector bitcase, if both D and S are uniform, they have
        // the same layout in GRF and can be aliased.
        //
        // For now, only do it if S's element type is larger; otherwise
        // visa variable's alignment might use the smaller element type
        // and it is not clear what is a clean way to fix that.
        // For example:
        //    b = bitcast i64 to <2xi32>
        // it is okay, but the following isn't
        //    b = bitcast <2xi32> to i64
        Type *dTy = D->getType();
        Type *sTy = S->getType();
        IGCLLVM::FixedVectorType *dVTy = dyn_cast<IGCLLVM::FixedVectorType>(dTy);
        IGCLLVM::FixedVectorType *sVTy = dyn_cast<IGCLLVM::FixedVectorType>(sTy);
        int d_nelts = dVTy ? (int)dVTy->getNumElements() : 1;
        int s_nelts = sVTy ? (int)sVTy->getNumElements() : 1;
        const bool canAliasVecCast = (!dVTy || sVTy) && d_nelts > s_nelts && // S's element type is larger
                                     !isa<PointerType>(dTy->getScalarType()) && !isa<PointerType>(sTy->getScalarType());

        if (isArgOrNeededInst(S) && WIA->whichDepend(D) == WIA->whichDepend(S) &&
            (isNoOpInst(CastI, CTX) || (WIA->isUniform(D) && canAliasVecCast))) {
          if (AliasMap.count(D) == 0) {
            AddAlias(S);
            Value *aliasee = AliasMap[S];
            AliasMap[D] = aliasee;

            // D will be deleted due to aliasing
            NoopAliasMap[D] = 1;

            // union liveness info
            LV->mergeUseFrom(aliasee, D);
          } else {
            // Only src operands of a phi can be visited before
            // operands' definition. For other instructions such
            // as castInst, this shall never happen
            IGC_ASSERT_MESSAGE(0, "ICE: Use visited before definition!");
          }
        }
      } else if (isa<CallInst>(I)) {
        if (GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(I)) {
          auto GIIid = GII->getIntrinsicID();
          if ((GIIid == GenISAIntrinsic::GenISA_bitcastfromstruct ||
               GIIid == GenISAIntrinsic::GenISA_bitcasttostruct) &&
              !isa<Constant>(GII->getOperand(0))) {
            // special cast just for load/store.
            Value *D = GII;
            Value *S = GII->getOperand(0);

            if (GIIid == GenISAIntrinsic::GenISA_bitcastfromstruct) {
              // D must be int or int vector type; S must be struct type.
              IGC_ASSERT(D->getType()->getScalarType()->isIntegerTy());
              IGC_ASSERT(S->getType()->isStructTy());
            } else if (GIIid == GenISAIntrinsic::GenISA_bitcasttostruct) {
              // S must be int or int vector type; D must be struct type.
              IGC_ASSERT(S->getType()->getScalarType()->isIntegerTy());
              IGC_ASSERT(D->getType()->isStructTy());
            }

            AddAlias(S);
            Value *aliasee = AliasMap[S];
            AliasMap[D] = aliasee;

            // D will be deleted due to aliasing
            NoopAliasMap[D] = 1;

            // union liveness info
            LV->mergeUseFrom(aliasee, D);
          }
        }
      }
    }
  }
}

void DeSSA::addReg(Value *Val, e_alignment Align) {
  if (RegNodeMap.count(Val))
    return;
  RegNodeMap[Val] = new (Allocator) Node(Val, ++CurrColor, Align);
}
// Using Path Halving in union-find
DeSSA::Node *DeSSA::Node::getLeader() {
  Node *N = this;
  Node *Parent = parent;
  Node *Grandparent = Parent->parent;

  while (Parent != Grandparent) {
    N->parent = Grandparent;
    N = Grandparent;
    Parent = N->parent;
    Grandparent = Parent->parent;
  }

  return Parent;
}

Value *DeSSA::getRegRoot(Value *Val, e_alignment *pAlign) const {
  auto RI = RegNodeMap.find(Val);
  if (RI == RegNodeMap.end())
    return nullptr;
  Node *TheNode = RI->second;
  if (isIsolated(TheNode))
    return nullptr;
  Node *TheLeader = TheNode->getLeader();
  if (pAlign)
    *pAlign = TheLeader->alignment;
  return TheLeader->value;
}

int DeSSA::getRootColor(Value *V) {
  auto RI = RegNodeMap.find(V);
  if (RI == RegNodeMap.end())
    return 0;
  Node *TheNode = RI->second;
  if (isIsolated(TheNode))
    return 0;
  Node *TheLeader = TheNode->getLeader();
  return TheLeader->color;
}

void DeSSA::unionRegs(Node *Nd1, Node *Nd2) {
  Node *N1 = Nd1->getLeader();
  Node *N2 = Nd2->getLeader();
  Node *NewLeader = nullptr;
  Node *Leadee = nullptr;

  if (N1 == N2)
    return;

  if (N1->rank > N2->rank) {
    NewLeader = N1;
    Leadee = N2;
  } else if (N1->rank < N2->rank) {
    NewLeader = N2;
    Leadee = N1;
  } else {
    NewLeader = N1;
    Leadee = N2;
    NewLeader->rank++;
  }

  IGC_ASSERT_MESSAGE(nullptr != NewLeader, "ICE: both leader and leadee shall not be null!");
  IGC_ASSERT_MESSAGE(nullptr != Leadee, "ICE: both leader and leadee shall not be null!");
  Leadee->parent = NewLeader;

  // Link the circular list of Leadee right before NewLeader
  Node *Leadee_prev = Leadee->prev;
  Node *NewLeader_prev = NewLeader->prev;
  NewLeader_prev->next = Leadee;
  Leadee->prev = NewLeader_prev;
  Leadee_prev->next = NewLeader;
  NewLeader->prev = Leadee_prev;
}

void DeSSA::isolateReg(Value *Val) {
  Node *ND = RegNodeMap[Val];
  splitNode(ND);
}

bool DeSSA::isIsolated(Value *V) const {
  auto RI = RegNodeMap.find(V);
  if (RI == RegNodeMap.end()) {
    return true;
  }
  Node *DestNode = RI->second;
  return isIsolated(DestNode);
}

// Split node ND from its existing congurent class, and the
// node ND itself becomes a new single-value congruent class.
void DeSSA::splitNode(Node *ND) {
  Node *N = ND->next;
  if (N == ND) {
    // ND is already in a single-value congruent class
    return;
  }

  Node *Leader = ND->getLeader();

  // Remove ND from the congruent class
  Node *P = ND->prev;
  N->prev = P;
  P->next = N;

  // ND : a new single-value congruent class
  ND->parent = ND;
  ND->next = ND;
  ND->prev = ND;
  ND->rank = 0;

  // If leader is removed, need to have a new leader.
  if (Leader == ND) {
    // P will be the new leader. Also swap ND's color with P's
    // so that the original congruent class still have the original
    // color (this is important as Dom traversal assumes that the
    // color of any congruent class remains unchanged).
    int t = P->color;
    P->color = ND->color;
    ND->color = t;

    // New leader
    Leader = P;
  }

  // If ND has children, those children need to set their parent.
  // Since we don't know if ND has children, we conservatively set
  // parent for all remaining nodes using "a path compression", so
  // that all nodes remains in the same rooted tree.
  N = Leader->next;
  Leader->parent = Leader;
  Leader->rank = (Leader == N) ? 0 : 1;
  while (N != Leader) {
    N->parent = Leader;
    N->rank = 0;
    N = N->next;
  }
}

/// SplitInterferencesForBasicBlock - traverses a basic block, splitting any
/// interferences found between registers in the same congruence class. It
/// takes two DenseMaps as arguments that it also updates:
///
/// 1) CurrentDominatingParent, which maps a color to the register in that
///    congruence class whose definition was most recently seen.
///
/// 2) ImmediateDominatingParent, which maps a register to the register in the
///    same congruence class that most immediately dominates it.
///
/// This function assumes that it is being called in a depth-first traversal
/// of the dominator tree.
///
/// The algorithm used here is a generalization of the dominance-based SSA test
/// for two variables. If there are variables a_1, ..., a_n such that
///
///   def(a_1) dom ... dom def(a_n),
///
/// then we can test for an interference between any two a_i by only using O(n)
/// interference tests between pairs of variables. If i < j and a_i and a_j
/// interfere, then a_i is alive at def(a_j), so it is also alive at def(a_i+1).
/// Thus, in order to test for an interference involving a_i, we need only check
/// for a potential interference with a_i+1.
///
/// This method can be generalized to arbitrary sets of variables by performing
/// a depth-first traversal of the dominator tree. As we traverse down a branch
/// of the dominator tree, we keep track of the current dominating variable and
/// only perform an interference test with that variable. However, when we go to
/// another branch of the dominator tree, the definition of the current dominating
/// variable may no longer dominate the current block. In order to correct this,
/// we need to use a stack of past choices of the current dominating variable
/// and pop from this stack until we find a variable whose definition actually
/// dominates the current block.
///
/// There will be one push on this stack for each variable that has become the
/// current dominating variable, so instead of using an explicit stack we can
/// simply associate the previous choice for a current dominating variable with
/// the new choice. This works better in our implementation, where we test for
/// interference in multiple distinct sets at once.
void DeSSA::SplitInterferencesForBasicBlock(BasicBlock *MBB, DenseMap<int, Value *> &CurrentDominatingParent,
                                            DenseMap<Value *, Value *> &ImmediateDominatingParent) {
  // Sort defs by their order in the original basic block, as the code below
  // assumes that it is processing definitions in dominance order.
  std::vector<Instruction *> &DefInstrs = PHISrcDefs[MBB];
  std::sort(DefInstrs.begin(), DefInstrs.end(), MIIndexCompare(LV));

  for (std::vector<Instruction *>::const_iterator BBI = DefInstrs.begin(), BBE = DefInstrs.end(); BBI != BBE; ++BBI) {

    Instruction *DefMI = *BBI;

    // If the virtual register being defined is not used in any PHI or has
    // already been isolated, then there are no more interferences to check.
    int RootC = getRootColor(DefMI);
    if (!RootC)
      continue;

    // The input to this pass sometimes is not in SSA form in every basic
    // block, as some virtual registers have redefinitions. We could eliminate
    // this by fixing the passes that generate the non-SSA code, or we could
    // handle it here by tracking defining machine instructions rather than
    // virtual registers. For now, we just handle the situation conservatively
    // in a way that will possibly lead to false interferences.
    Value *NewParent = CurrentDominatingParent[RootC];
    if (NewParent == DefMI)
      continue;

    // Pop registers from the stack represented by ImmediateDominatingParent
    // until we find a parent that dominates the current instruction.
    while (NewParent) {
      if (getRootColor(NewParent)) {
        // we have added the another condition because the domination-test
        // does not work between two phi-node. See the following comments
        // from the DT::dominates:
        // " It is not possible to determine dominance between two PHI nodes
        //   based on their ordering
        //  if (isa<PHINode>(A) && isa<PHINode>(B))
        //    return false;"
        if (isa<Argument>(NewParent)) {
          break;
        } else if (DT->dominates(cast<Instruction>(NewParent), DefMI)) {
          break;
        } else if (cast<Instruction>(NewParent)->getParent() == MBB && isa<PHINode>(DefMI) && isa<PHINode>(NewParent)) {
          break;
        }
      }
      NewParent = ImmediateDominatingParent[NewParent];
    }
    // If NewParent is nonzero, then its definition dominates the current
    // instruction, so it is only necessary to check for the liveness of
    // NewParent in order to check for an interference.
    if (NewParent && LV->isLiveAt(NewParent, DefMI)) {
      // If there is an interference, always isolate the new register. This
      // could be improved by using a heuristic that decides which of the two
      // registers to isolate.
      isolateReg(DefMI);
      CurrentDominatingParent[RootC] = NewParent;
    } else {
      // If there is no interference, update ImmediateDominatingParent and set
      // the CurrentDominatingParent for this color to the current register.
      ImmediateDominatingParent[DefMI] = NewParent;
      CurrentDominatingParent[RootC] = DefMI;
    }
  }

  // We now walk the PHIs in successor blocks and check for interferences. This
  // is necessary because the use of a PHI's operands are logically contained in
  // the predecessor block. The def of a PHI's destination register is processed
  // along with the other defs in a basic block.

  CurrentPHIForColor.clear();

  for (succ_iterator SI = succ_begin(MBB), E = succ_end(MBB); SI != E; ++SI) {
    for (BasicBlock::iterator BBI = (*SI)->begin(), BBE = (*SI)->end(); BBI != BBE; ++BBI) {
      PHINode *PHI = dyn_cast<PHINode>(BBI);
      if (!PHI) {
        break;
      }

      int RootC = getRootColor(PHI);
      // check live-out interference
      if (IGC_IS_FLAG_ENABLED(EnableDeSSAWA) && !RootC) {
        // [todo] delete this code
        if (CTX->type == ShaderType::COMPUTE_SHADER) {
          for (unsigned i = 0; !RootC && i < PHI->getNumOperands(); i++) {
            Value *SrcVal = PHI->getOperand(i);
            if (!isa<Constant>(SrcVal)) {
              SrcVal = getNodeValue(SrcVal);
              RootC = getRootColor(SrcVal);
            }
          }
        }
      }
      if (!RootC) {
        continue;
      }
      // Find the index of the PHI operand that corresponds to this basic block.
      unsigned PredIndex;
      for (PredIndex = 0; PredIndex < PHI->getNumOperands(); ++PredIndex) {
        if (PHI->getIncomingBlock(PredIndex) == MBB)
          break;
      }
      IGC_ASSERT(PredIndex < PHI->getNumOperands());
      Value *PredValue = PHI->getOperand(PredIndex);
      PredValue = getNodeValue(PredValue);
      std::pair<Instruction *, Value *> &CurrentPHI = CurrentPHIForColor[RootC];
      // If two PHIs have the same operand from every shared predecessor, then
      // they don't actually interfere. Otherwise, isolate the current PHI. This
      // could possibly be improved, e.g. we could isolate the PHI with the
      // fewest operands.
      if (CurrentPHI.first && CurrentPHI.second != PredValue) {
        isolateReg(PHI);
        continue;
      } else {
        CurrentPHI = std::make_pair(PHI, PredValue);
      }

      // check live-out interference
      // Pop registers from the stack represented by ImmediateDominatingParent
      // until we find a parent that dominates the current instruction.
      Value *NewParent = CurrentDominatingParent[RootC];
      while (NewParent) {
        if (getRootColor(NewParent)) {
          if (isa<Argument>(NewParent)) {
            break;
          } else if (DT->dominates(cast<Instruction>(NewParent)->getParent(), MBB)) {
            break;
          }
        }
        NewParent = ImmediateDominatingParent[NewParent];
      }
      CurrentDominatingParent[RootC] = NewParent;

      // If there is an interference with a register, always isolate the
      // register rather than the PHI. It is also possible to isolate the
      // PHI, but that introduces copies for all of the registers involved
      // in that PHI.
      if (NewParent && NewParent != PredValue && LV->isLiveOut(NewParent, *MBB)) {
        isolateReg(NewParent);
      }
    }
    // fix situation in which uniform-phi is inside a divergent-join.
    // The following is an example of 3-way join by nested-if
    //
    //   %22 = phi i32 [ 13, blk19], [%16, blk0], [ 13, blk17]
    //   %23 = phi i32 [%16, blk19], [  0, blk0], [%16, blk17]
    // blk0 is from the uniform-if
    // blk17 is from the divergent-if
    // blk19 is from the divergent-if
    //
    //  if we coalesce %16 and %22 with the same register V5
    //  then we still need to insert the following move in on divergent
    //  edges
    //  blk-17:
    //     mov(W) v5, 13
    //  blk-19:
    //     mov(W) v5, 13
    //
    //  however, %16 and %23 are not coalesced. so we add the other two moves
    //  blk-17:
    //     mov(W) v6, v5
    //     mov(W) v5, 13  NOTE: this move changs v5, subsequently wrong v6
    //  blk-19:
    //     mov(W) v6, v5
    //     mov(W) v5, 13
    //  Due to divergent CF, both blk-17 and blk-19 are executed, causes problem

    // so we know MBB-target, i.e. the phi-block is a divergent-join
    if (WIA->insideDivergentCF(MBB->getTerminator())) {
      for (BasicBlock::iterator BBI = (*SI)->begin(), BBE = (*SI)->end(); BBI != BBE; ++BBI) {
        PHINode *PHI = dyn_cast<PHINode>(BBI);
        if (!PHI)
          break;
        if (!WIA->isUniform(PHI))
          continue;
        auto RootC = getRootColor(PHI);
        if (!RootC)
          continue;
        // so this is uniform phi and not-isolated
        unsigned PredIndex;
        for (PredIndex = 0; PredIndex < PHI->getNumOperands(); ++PredIndex) {
          if (PHI->getIncomingBlock(PredIndex) == MBB)
            break;
        }
        IGC_ASSERT(PredIndex < PHI->getNumOperands());
        Value *PredValue = PHI->getOperand(PredIndex);
        // check if we will need to add a phi-copy from MBB
        if (isa<Constant>(PredValue) || isIsolated(PredValue)) {
          // adding a phi-copy interferes with current live-out
          if (CurrentDominatingParent[RootC])
            isolateReg(PHI);
        }
      }
    }
  }
}

void DeSSA::SplitInterferencesForArgument(DenseMap<int, Value *> &CurrentDominatingParent,
                                          DenseMap<Value *, Value *> &ImmediateDominatingParent) {
  // No two arguments can be in the same congruent class
  for (auto BBI = PHISrcArgs.begin(), BBE = PHISrcArgs.end(); BBI != BBE; ++BBI) {
    Value *AV = *BBI;
    // If the virtual register being defined is not used in any PHI or has
    // already been isolated, then there are no more interferences to check.
    int RootC = getRootColor(AV);
    if (!RootC)
      continue;
    Value *NewParent = CurrentDominatingParent[RootC];
    if (NewParent) {
      isolateReg(AV);
    } else {
      CurrentDominatingParent[RootC] = AV;
    }
  }
}

// [todo] get rid of alignment-based isolation in dessa.
// Using alignment in isolation seems over-kill. The right approach
// would be one that avoids adding values with conflicting alignment
// requirement in the same congruent, not adding them in the same
// congruent class first and trying to isolate them later.
void DeSSA::SplitInterferencesForAlignment() {
  for (auto I = RegNodeMap.begin(), E = RegNodeMap.end(); I != E; ++I) {
    // Find a root Node
    Node *rootNode = I->second;
    if (rootNode->parent != rootNode) {
      continue;
    }

    e_alignment Align = EALIGN_AUTO;
    // Find the most restrictive alignment, i.e. GRF aligned ones.
    Node *N = rootNode;
    Node *Curr;
    do {
      Curr = N;
      N = Curr->next;
      if (Curr->alignment == EALIGN_GRF) {
        Align = EALIGN_GRF;
        break;
      }
    } while (N != rootNode);

    if (Align != EALIGN_GRF)
      continue;

    // Isolate any mis-aligned value.
    // Start with Curr node as it cannot be isolated
    // (rootNode could be isolated), therefore, it remains
    // in the linked list and can be used to test stop looping.
    Node *Head = Curr;
    N = Head;
    do {
      Curr = N;
      N = N->next;
      if (Curr->alignment != EALIGN_AUTO && Curr->alignment != EALIGN_GRF) {
        IGC_ASSERT(nullptr != Curr);
        IGC_ASSERT_MESSAGE((Curr != Head), "Head Node cannot be isolated, something wrong!");
        isolateReg(Curr->value);
      }
    } while (N != Head);

    // Update root's alignment.
    Head->getLeader()->alignment = Align;
  }
}

Value *DeSSA::getInsEltRoot(Value *Val) const {
  auto RI = InsEltMap.find(Val);
  if (RI == InsEltMap.end())
    return Val;
  return RI->second;
}

/// <summary>
/// Identify if an instruction has partial write semantics
/// </summary>
/// <param name="Inst"></param>
/// <returns> the index of the source partial-write operand</returns>
static int getPartialWriteSource(Value *Inst) {
  if (isa<InsertElementInst>(Inst))
    return 0; // source 0 is the original value
  if (auto CI = dyn_cast<CallInst>(Inst)) {
    // only handle inline-asm with simple destination
    if (CI->isInlineAsm() && !CI->getType()->isStructTy()) {
      InlineAsm *IA = cast<InlineAsm>(IGCLLVM::getCalledValue(CI));
      StringRef constraintStr(IA->getConstraintString());
      SmallVector<StringRef, 8> constraints;
      constraintStr.split(constraints, ',');
      for (int i = 0; i < (int)constraints.size(); i++) {
        unsigned destID = 0;
        if (constraints[i].getAsInteger(10, destID) == 0) {
          // constraint-string indicates that source(i-1) and
          // destination should be the same vISA variable
          if (i > 0 && destID == 0)
            return (i - 1);
        }
      }
    }
  }
  return -1;
}

void DeSSA::CoalesceInsertElements() {
  for (df_iterator<DomTreeNode *> DI = df_begin(DT->getRootNode()), DE = df_end(DT->getRootNode()); DI != DE; ++DI) {
    BasicBlock *Blk = DI->getBlock();

    for (BasicBlock::iterator BBI = Blk->begin(), BBE = Blk->end(); BBI != BBE; ++BBI) {
      Instruction *Inst = &(*BBI);

      if (!CG->NeedInstruction(*Inst)) {
        continue;
      }
      // Only Aliasee needs to be handled.
      if (getAliasee(Inst) != Inst) {
        continue;
      }

      // For keeping the existing behavior of InsEltMap unchanged
      auto PWSrcIdx = getPartialWriteSource(Inst);
      if (PWSrcIdx >= 0) {
        Value *origSrcV = Inst->getOperand(PWSrcIdx);
        Value *SrcV = getAliasee(origSrcV);
        if (SrcV != Inst && isArgOrNeededInst(origSrcV)) {
          // union them
          e_alignment InstAlign = GetPreferredAlignment(Inst, WIA, CTX);
          e_alignment SrcVAlign = GetPreferredAlignment(SrcV, WIA, CTX);
          if (!LV->isLiveAt(SrcV, Inst) && !alignInterfere(InstAlign, SrcVAlign) &&
              (WIA->whichDepend(SrcV) == WIA->whichDepend(Inst))) {
            InsEltMapAddValue(SrcV);
            InsEltMapAddValue(Inst);

            Value *SrcVRoot = getInsEltRoot(SrcV);
            Value *InstRoot = getInsEltRoot(Inst);

            // union them and their liveness info
            InsEltMapUnionValue(SrcV, Inst);
            LV->mergeUseFrom(SrcVRoot, InstRoot);
          }
        }
      }
    }
  }
}

Value *DeSSA::getRootValue(Value *Val, e_alignment *pAlign) const {
  Value *mapVal = nullptr;
  auto AI = AliasMap.find(Val);
  if (AI != AliasMap.end()) {
    mapVal = AI->second;
  }
  auto IEI = InsEltMap.find(mapVal ? mapVal : Val);
  if (IEI != InsEltMap.end()) {
    mapVal = IEI->second;
  }
  Value *PhiRootVal = getRegRoot(mapVal ? mapVal : Val, pAlign);
  return (PhiRootVal ? PhiRootVal : mapVal);
}

void DeSSA::getAllValuesInCongruentClass(Value *V, SmallVector<Value *, 16> &ValsInCC) {
  // Handle InsertElement specially. Note that only rootValue from
  // a sequence of insertElement is in congruent class. The RootValue
  // has its liveness modified to cover all InsertElements that are
  // grouped together.
  Value *RootV = getNodeValue(V);

  IGC_ASSERT_MESSAGE(nullptr != RootV, "ICE: Node value should not be nullptr!");
  ValsInCC.push_back(RootV);
  auto RI = RegNodeMap.find(RootV);
  if (RI != RegNodeMap.end()) {
    Node *First = RI->second;
    for (Node *N = First->next; N != First; N = N->next) {
      ValsInCC.push_back(N->value);
    }
  }
  return;
}

// All values that are coalesced together, including values that are
// handled specially, such as ones in aliasMap and insEltMap.
void DeSSA::getAllCoalescedValues(Value *V, SmallVector<Value *, 16> &Vals) {
  getAllValuesInCongruentClass(V, Vals);
  IGC_ASSERT_MESSAGE(Vals.size() > 0, "ICE: Vals should not be empty!");

  // First, add values from InsEltMap
  for (int i = 0, sz = (int)Vals.size(); i < sz; ++i) {
    Value *ccVal = Vals[i];
    for (const auto &II : InsEltMap) {
      Value *R = II.second;
      if (R != ccVal) {
        continue;
      }
      Value *A = II.first;
      if (A != R) {
        Vals.push_back(A);
      }
    }
  }

  // second, add aliasers from AliasMap
  for (int i = 0, sz = (int)Vals.size(); i < sz; ++i) {
    Value *aliasee = Vals[i];
    for (const auto &II : AliasMap) {
      Value *R = II.second;
      if (R != aliasee) {
        continue;
      }
      Value *aliaser = II.first;
      if (aliaser != aliasee) {
        Vals.push_back(aliaser);
      }
    }
  }
  return;
}

void DeSSA::coalesceAliasInsertValue(InsertValueInst *theIVI) {
  // Find a chain of insertvalue, and return the lead (last one).
  auto getInsValChain = [this](InsertValueInst *aIVI, SmallVector<Value *, 8> &IVIs) {
    IGC_ASSERT(aIVI);
    InsertValueInst *lead = aIVI;
    const bool isUniform = WIA->isUniform(lead);
    IVIs.push_back(lead);
    while (lead->hasOneUse()) {
      auto next = dyn_cast<InsertValueInst>(lead->user_back());
      if (!next || isUniform != WIA->isUniform(next)) {
        break;
      }
      lead = next;
      IVIs.push_back(lead);
    }
    return lead;
  };

  auto setInsValAlias = [this](SmallVector<Value *, 8> &IVIs) {
    int nelts = (int)IVIs.size();
    if (nelts > 1) {
      Value *aliasee = IVIs[0];
      AddAlias(aliasee);
      for (int i = 1; i < nelts; ++i) {
        Value *V = IVIs[i];
        AliasMap[V] = aliasee;

        // union liveness info
        LV->mergeUseFrom(aliasee, V);
      }
    }
  };

  // Get all fields' indices if there are at most 2 level fields.
  // Using int = <i16 level_1-ix, i16 level_0-ix> to represent two
  // level indices.
  auto getIndices = [](DenseSet<int> &aFields, SmallVector<Value *, 8> &IVIs) {
    int nelts = (int)IVIs.size();
    for (int i = 0; i < nelts; ++i) {
      InsertValueInst *tI = cast<InsertValueInst>(IVIs[i]);
      switch (tI->getNumIndices()) {
      case 1: {
        uint16_t ix = (uint16_t)tI->getIndices().front();
        aFields.insert((int)ix);
        break;
      }
      case 2: {
        ArrayRef<unsigned> ids = tI->getIndices();
        uint16_t lvl0_ix = (uint16_t)ids[0];
        uint16_t lvl1_ix = (uint16_t)ids[1];
        uint32_t ix = ((lvl1_ix << 16) | lvl0_ix);
        aFields.insert((int)ix);
        break;
      }
      default:
        aFields.clear();
        return;
      }
    }
  };

  auto isDisjoin = [](DenseSet<int> &RHS, DenseSet<int> &LHS) {
    for (auto II : LHS) {
      int e = II;
      if (RHS.find(e) != RHS.end()) {
        return false;
      }
    }
    return true;
  };

  // InsertValueInst chain:
  //        V0 = InsVal undef/const, S0, 0
  //        V1 = InsVal V0, S1, 1
  //        ...
  //        Vn = InsVal Vn-1, Sn,
  //  where all Vi, i=0, n-1 has a single use. This sequence is called
  //  InsertValueInst chain (IVI Chain). And they (all Vi) are set to alias
  //  each other.
  //
  //  Start finding IVI chains from an inst that cannot be an operand of
  //  the other InsertValueInst. It's possible to have several IVI chains.
  //  For example,
  //       Chain0:   V0 = insval undef
  //                 V1 = insVal V0
  //       Chain1:   V2 = insVal V1
  //                 V3 = insVal V2
  //       Chain2:   V4 = insVal V1
  //       Chain3:   V5 = insVal V4
  //       Chain4:   V6 = insVal V4
  //  This can be viewed as a tree of IVI chains, rooted at V0, with edge
  //  denoting def-use relation between two chains. The tree for the above
  //  is as below:
  //
  //           Chain0
  //          /      \
    //      Chain1    Chain2
  //                /     \
    //            Chain3   Chain4
  //
  // The algorithm does the following:
  //   1. Find all chains. All insts (Vs) of each chain are aliased to
  //      each other in the same chain.
  //   2. Do aggressive aliasing : alias several chains along one path from
  //      root to a leaf node, as long as no struct field is defined by
  //      more than one chains, which is a strong condition and implies that
  //      the root inst should start like the followning:
  //          V0 = insval undef, s, 10
  //
  const Value *Oprd0 = theIVI->getOperand(0);
  if (theIVI->getType()->isStructTy() && (isa<Constant>(Oprd0) || isa<Argument>(Oprd0) || isa<PHINode>(Oprd0))) {
    SmallVector<Value *, 8> IVIChain;
    InsertValueInst *LeadInst = getInsValChain(theIVI, IVIChain);
    setInsValAlias(IVIChain);

    // For aggressive aliasing. lead inst is used to identify its
    // successor chain in the chain tree.
    bool aggressiveAliasing = false;
    DenseSet<int> commonFields;
    const bool isChainUniform = WIA->isUniform(LeadInst);
    Value *theAliasee = getAliasee(LeadInst);
    if (isa<UndefValue>(Oprd0)) {
      getIndices(commonFields, IVIChain);
      if (!commonFields.empty()) { // Fields inserted are known, can do aggressive coalescing.
        aggressiveAliasing = true;
      }
    }

    // Make sure every sub-chains rooted at theIVI are processed.
    std::list<InsertValueInst *> worklist;
    worklist.push_back(LeadInst);
    while (!worklist.empty()) {
      InsertValueInst *aI = worklist.front();
      worklist.pop_front();
      for (auto UI = aI->user_begin(), UE = aI->user_end(); UI != UE; ++UI) {
        Value *v = *UI;
        InsertValueInst *I = dyn_cast<InsertValueInst>(v);
        if (!I) {
          continue;
        }
        IVIChain.clear();
        InsertValueInst *nextLead = getInsValChain(I, IVIChain);
        setInsValAlias(IVIChain);
        worklist.push_back(nextLead);

        if (aggressiveAliasing && LeadInst == aI && isChainUniform == WIA->isUniform(nextLead)) {
          DenseSet<int> fields;
          getIndices(fields, IVIChain);
          if (!fields.empty() && isDisjoin(commonFields, fields)) {
            LeadInst = nextLead;
            commonFields.insert(fields.begin(), fields.end());

            // update alias
            AddAlias(theAliasee);
            for (auto II : IVIChain) {
              Value *V = II;
              AliasMap[V] = theAliasee;

              // union liveness info
              LV->mergeUseFrom(theAliasee, V);
            }
          }
        }
      }
    }
  }
}

int DeSSA::checkInsertElementAlias(InsertElementInst *IEI, SmallVector<Value *, 16> &AllIEIs) {
  IGC_ASSERT(nullptr != IEI);
  IGC_ASSERT_MESSAGE(isa<UndefValue>(IEI->getOperand(0)), "ICE: need to pass first IEI as the argument");

  // Find the the alias pattern:
  //     V0 = IEI UndefValue, S0, 0
  //     V1 = IEI V0,         S1, 1
  //     V2 = IEI V1,         S2, 2
  //     ......
  //     Vn = IEI Vn_1,       Sn_1, n
  // All Vi (i=0,n_1, except i=n) has a single-use.
  //
  // If found, return the actual vector size;
  // otherwise, return 0.
  IGCLLVM::FixedVectorType *VTy = cast<IGCLLVM::FixedVectorType>(IEI->getType());
  IGC_ASSERT(nullptr != VTy);
  int nelts = (int)VTy->getNumElements();
  AllIEIs.resize(nelts, nullptr);
  InsertElementInst *Inst = IEI;
  IGC_ASSERT(nullptr != WIA);
  WIAnalysis::WIDependancy Dep = WIA->whichDepend(Inst);
  while (Inst) {
    // Check if Inst has constant index, stop if not.
    // (This is for catching a common case, a variable index
    //  can be handled as well if needed.)
    ConstantInt *CI = dyn_cast<ConstantInt>(Inst->getOperand(2));
    if (!CI) {
      return 0;
    }
    int ix = (int)CI->getZExtValue();
    AllIEIs[ix] = Inst;
    if (!Inst->hasOneUse() || Dep != WIA->whichDepend(Inst)) {
      break;
    }
    Inst = dyn_cast<InsertElementInst>(Inst->user_back());
  }

  // Return the number of elements found
  int num = 0;
  for (int i = 0; i < nelts; ++i) {
    if (AllIEIs[i] == nullptr)
      continue;
    if (num < i) {
      // Pack them
      AllIEIs[num] = AllIEIs[i];
      AllIEIs[i] = nullptr;
    }
    ++num;
  }
  return num;
}

Value *DeSSA::getAliasee(Value *V) const {
  auto AI = AliasMap.find(V);
  if (AI == AliasMap.end())
    return V;
  return AI->second;
}

bool DeSSA::isAliaser(Value *V) const {
  auto AI = AliasMap.find(V);
  if (AI == AliasMap.end()) {
    return false;
  }
  return AI->first != AI->second;
}

bool DeSSA::isNoopAliaser(Value *V) const { return NoopAliasMap.count(V) > 0; }

bool DeSSA::isAliasee(Value *V) const {
  auto AI = AliasMap.find(V);
  if (AI == AliasMap.end()) {
    return false;
  }
  return AI->first == AI->second;
}

// Single-valued:
//   If a variable takes one and only one value during its entire life time,
//   it is called single-valued. If a variable is of vector or struct, single
//   valued means that all its component (vector elements or struct members)
//   are all singled-valued.
//
// This concept is useful when setting up aliasing.  For example,
//   a = bitcast b to i32
// If both a and b are singled valued, a will be set to alias b without
// doing futher checking. However, if either isn't single valued, checking
// if a can be aliased to b needs to interfere checking. For example
//   dessa CC : [b, c]  // b and c are coalesced into a single variable.
//
//      b = 1
//      a = bitcast b to i32
//      ...
//      c = 2
//      ...
//   L:   = a
//        = c
//
//  In this case, if a is aliased to b, a would get 2 at L, but the correct
//  value should be 1. In order to find out if a can be aliased to b, it
//  requires to do interference checking with c.
bool DeSSA::isSingleValued(llvm::Value *V) const {
  // InsEltMap and non-isolated : not single-valued
  Value *aliasee = getAliasee(V);
  Value *insEltRootV = getInsEltRoot(aliasee);
  if (InsEltMap.count(aliasee) || !isIsolated(insEltRootV)) {
    return false;
  }
  return true;
}

// The following paper explains an approach to check if two
// congruent classes interfere using a linear approach.
//
//    Boissinot, et al. Revisiting Out-of-SSA Translation for Correctness,
//    Code Quality and Efficiency,
//      In Proceedings of the 7th annual IEEE/ACM International Symposium
//      on Code Generation and Optimization (Seattle, Washington,
//      March 22 - 25, 2009). CGO '09. IEEE, Washington, DC, 114-125.
//
// Here, we simply use a naive pair-wise comparison.
//
// TODO: check if using linear approach described in the paper is
//   necessary;  To do so, it needs to get PN (preorder number of BB)
//   and sort congruent classes before doing interference checking.
bool DeSSA::interfere(llvm::Value *V0, llvm::Value *V1) {
  SmallVector<Value *, 16> allCC0;
  SmallVector<Value *, 16> allCC1;
  getAllValuesInCongruentClass(V0, allCC0);
  getAllValuesInCongruentClass(V1, allCC1);

  for (int i0 = 0, sz0 = (int)allCC0.size(); i0 < sz0; ++i0) {
    Value *val0 = allCC0[i0];
    for (int i1 = 0, sz1 = (int)allCC1.size(); i1 < sz1; ++i1) {
      Value *val1 = allCC1[i1];
      if (LV->hasInterference(val0, val1)) {
        return true;
      }
    }
  }
  return false;
}

// Alias interference checking.
//    The caller is trying to check if V0 can alias to V1. For example,
//      V0 = bitcast V1, or
//      V0 = extractElement V1, ...
//    As V0 and V1 hold the same value, the interference between these two
//    does not matter. Thus, this function is a variant of interfere()
//    with V0 and V1 interference ignored.
bool DeSSA::aliasInterfere(llvm::Value *V0, llvm::Value *V1) {
  SmallVector<Value *, 16> allCC0;
  SmallVector<Value *, 16> allCC1;
  getAllValuesInCongruentClass(V0, allCC0);
  getAllValuesInCongruentClass(V1, allCC1);

  Value *V0_aliasee = getAliasee(V0);
  Value *V1_aliasee = getAliasee(V1);

  //
  // If aliasee is in InsEltMap, it is not single valued
  // and cannot be excluded from interfere checking.
  //
  // For example:
  //     x = bitcast y
  //     z = InsElt y, ...
  //       =  x
  //       =  y
  //
  //     {y, z} are coalesced via InsElt, interfere(x, y)
  //     must be checked.
  // However, if y (and x too) is not in InsEltMap, no need
  // to check interfere(x, y) as they have the same value
  // as the following:
  //     x = bitcast y
  //       = x
  //       = y
  //
  bool V0_oneValue = (InsEltMap.count(V0_aliasee) == 0);
  bool V1_oneValue = (InsEltMap.count(V1_aliasee) == 0);
  bool both_singleValue = (V0_oneValue && V1_oneValue);

  for (int i0 = 0, sz0 = (int)allCC0.size(); i0 < sz0; ++i0) {
    Value *val0 = allCC0[i0];
    for (int i1 = 0, sz1 = (int)allCC1.size(); i1 < sz1; ++i1) {
      Value *val1 = allCC1[i1];
      if (both_singleValue && val0 == V0_aliasee && val1 == V1_aliasee) {
        continue;
      }
      if (LV->hasInterference(val0, val1)) {
        return true;
      }
    }
  }
  return false;
}

// The existing code does align interference checking. Just
// keep it for now. Likely to improve it later.
bool DeSSA::alignInterfere(e_alignment a1, e_alignment a2) {
  if (a1 == EALIGN_GRF && !(a2 == EALIGN_GRF || a2 == EALIGN_AUTO)) {
    return true;
  }
  if (a2 == EALIGN_GRF && !(a1 == EALIGN_GRF || a1 == EALIGN_AUTO)) {
    return true;
  }
  return false;
}
