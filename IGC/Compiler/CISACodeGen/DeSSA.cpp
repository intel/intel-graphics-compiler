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
//===- StrongPHIElimination.cpp - Eliminate PHI nodes by inserting copies -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
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

using namespace llvm;
using namespace IGC;
using namespace IGC::Debug;
using namespace IGC::IGCMD;

#define PASS_FLAG "DeSSA"
#define PASS_DESCRIPTION "coalesce moves coming from phi nodes"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN( DeSSA, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS )
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis )
IGC_INITIALIZE_PASS_DEPENDENCY(LiveVarsAnalysis )
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenPatternMatch )
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass )
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper )
IGC_INITIALIZE_PASS_END( DeSSA, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS )

char DeSSA::ID = 0;

DeSSA::DeSSA() : FunctionPass( ID )
{
    initializeDeSSAPass( *PassRegistry::getPassRegistry( ) );
}

void DeSSA::print(raw_ostream &OS, const Module* ) const
{
    Banner(OS, "Phi-Var Isolations");
    DenseMap<Node*, int> LeaderVisited;
    for (auto I = RegNodeMap.begin(),
        E = RegNodeMap.end(); I != E; ++I) {
        Node* N = I->second;
        // We don't want to change behavior of DeSSA by invoking
        // dumping/printing functions. Thus, don't use getLeader()
        // as it has side-effect (doing path halving).
        Node* Leader = N->parent;
        while (Leader != Leader->parent) {
            Leader = Leader->parent;
        }
        if (LeaderVisited.count(Leader)) {
            continue;
        }
        LeaderVisited[Leader] = 1;
        Value *VL;
        if (isIsolated(N)) {
            VL = N->value;
            OS << "Var isolated : ";
            VL->print(IGC::Debug::ods());
            OS << "\n";
        } else {
            OS << "Leader : ";
            Leader->value->print(IGC::Debug::ods());
            OS << "\n";
            N = Leader->next;
            while (N != Leader) {
                VL = N->value;
                OS << "    ";
                VL->print(IGC::Debug::ods());
                OS << "\n";
                N = N->next;
            }
        }
    }
}

void DeSSA::dump() const {
  print(ods());
}

bool DeSSA::runOnFunction(Function &MF)
{
  CurrColor = 0;
  MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  if (pMdUtils->findFunctionsInfoItem(&MF) == pMdUtils->end_FunctionsInfo())
  {
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

  if (IGC_IS_FLAG_ENABLED(EnableDeSSAAlias))
  {
      //
      // The DeSSA/Coalescing procedure:
      //   1. Follow Dominance tree to set up alias map. While setting up alias map,
      //      update liveness for aliasee so that alasee's liveness is the sum of
      //      all its aliasers.
      //
      //      By traversing dominance tree depth-first (DF), it is guaranteed that
      //      the aliasee will be handled before its aliasers. For example, let
      //      alias(v0, v1) denote that v0 is an alias to v1. DF dominance-tree
      //      traversal may not handle aliasing in the following order:
      //              alias(v0, v1)
      //              alias(v1, v2)
      //      rather, it must be in the order
      //              alias(v1, v2)
      //              alias(v0, v1)
      //      By doing DF dominance-tree traversal, this kind of aliasing chain will
      //      be handled nicely.
      //   2. Set up InsEltMap, which coalesces vector values used
      //      in the InsertElement instruction.
      //   3. Make sure DeSSA node only use the root value of aliases (that is,
      //      only aliasee may have DeSSA node).
      //      Given alias(v0, aliasee),
      //        if aliasee is in the InsEltMap, uses its InsEltRoot
      //        otherwise, use aliasee
      //
      //  Note that the algorithem forces coalescing of aliasing inst
      //  and InsertElement inst before PHI-coalescing, which means
      //  it favors coaslescing of those aliasing inst and InsertElement
      //  inst.

      for (df_iterator<DomTreeNode*> DI = df_begin(DT->getRootNode()),
          DE = df_end(DT->getRootNode()); DI != DE; ++DI) {
          CoalesceAliasInstForBasicBlock(DI->getBlock());
      }
  }

  for (df_iterator<DomTreeNode*> DI = df_begin(DT->getRootNode()),
      DE = df_end(DT->getRootNode()); DI != DE; ++DI) {
      CoalesceInsertElementsForBasicBlock(DI->getBlock());
  }

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
  enum { PHI_SRC_USE_THRESHOLD = 3 };  // arbitrary number
  DenseMap<Value *, int> PHILoopPreHeaderSrcs;

  // build initial congruent class using union-find
  for (Function::iterator I = MF.begin(), E = MF.end();
       I != E; ++I)
  {
    // First, initialize PHILoopPreHeaderSrcs map
    BasicBlock *MBB = &*I;
    Loop *LP = LI ? LI->getLoopFor(MBB) : nullptr;
    BasicBlock *PreHeader = LP ? LP->getLoopPredecessor() : nullptr;
    bool checkPHILoopInput = LP && (LP->getHeader() == MBB) && PreHeader;
    PHILoopPreHeaderSrcs.clear();
    if (checkPHILoopInput)
    {
      for (BasicBlock::iterator BBI = I->begin(), BBE = I->end();
           BBI != BBE; ++BBI) {
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
          PHILoopPreHeaderSrcs[SrcVal] = 0;  // initialize to zero
        }
        PHILoopPreHeaderSrcs[SrcVal] += 1;
      }
    }

    for (BasicBlock::iterator BBI = I->begin(), BBE = I->end();
         BBI != BBE; ++BBI) {
      PHINode *PHI = dyn_cast<PHINode>(BBI);
      if (!PHI) {
        break;
      }

      e_alignment DefAlign = GetPreferredAlignment(PHI, WIA, CTX);
      if (IGC_IS_FLAG_ENABLED(EnableDeSSAAlias)) {
          assert(PHI == getAliasee(PHI));
      }
      else {
          assert(PHI == getInsEltRoot(PHI));
      }
      addReg(PHI, DefAlign);
      PHISrcDefs[&(*I)].push_back(PHI);

      for (unsigned i = 0; i < PHI->getNumOperands(); ++i) {
        Value* OrigSrcVal = PHI->getOperand(i);
        // skip constant
        if (isa<Constant>(OrigSrcVal))
          continue;

        // condition for preheader-src-isolation
        bool PreheaderSrcIsolation = (checkPHILoopInput &&
            !isa<InsertElementInst>(OrigSrcVal) && !isa<PHINode>(OrigSrcVal) &&
            PHI->getIncomingBlock(i) == PreHeader &&
            PHILoopPreHeaderSrcs.count(OrigSrcVal) > 0 &&
            PHILoopPreHeaderSrcs[OrigSrcVal] >= PHI_SRC_USE_THRESHOLD);
        // add src to the union
        Value *SrcVal;
        if (IGC_IS_FLAG_ENABLED(EnableDeSSAAlias)) {
            SrcVal = getAliasee(OrigSrcVal);
        }
        else {
            SrcVal = getInsEltRoot(OrigSrcVal);
        }
        e_alignment SrcAlign = GetPreferredAlignment(OrigSrcVal, WIA, CTX);
        Instruction *DefMI = dyn_cast<Instruction>(SrcVal);
        if (DefMI) {
          if (CG->SIMDConstExpr(DefMI)) {
              continue;  // special case, simdSize becomes a constant in vISA
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
        if ( CG->IsForceIsolated(SrcVal) || PreheaderSrcIsolation) {
          isolateReg(SrcVal);
        }
      } // end of source-operand loop
      // isolate complex type that IGC does not handle
      if (PHI->getType()->isStructTy() ||
          PHI->getType()->isArrayTy()) {
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
      DenseMap<int, Value*> CurrentDominatingParent;
      DenseMap<Value*, Value*> ImmediateDominatingParent;
      // first, go through the function arguments
      SplitInterferencesForArgument(CurrentDominatingParent, ImmediateDominatingParent);
      // Then all the blocks
      for (df_iterator<DomTreeNode*> DI = df_begin(DT->getRootNode()),
          DE = df_end(DT->getRootNode()); DI != DE; ++DI) {
          SplitInterferencesForBasicBlock(DI->getBlock(),
              CurrentDominatingParent,
              ImmediateDominatingParent);
      }
  }

  // Handle values that have specific alignment requirement.
  SplitInterferencesForAlignment();
  return false;
}

void DeSSA::addReg(Value *Val, e_alignment Align) {
  if (RegNodeMap.count(Val))
    return;
  RegNodeMap[Val] = new (Allocator) Node(Val, ++CurrColor, Align);
}

// Using Path Halving in union-find
DeSSA::Node*
DeSSA::Node::getLeader() {
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

Value* DeSSA::getRegRoot(Value* Val, e_alignment *pAlign) const {
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

int DeSSA::getRootColor(Value* V)
{
    auto RI = RegNodeMap.find(V);
    if (RI == RegNodeMap.end())
        return 0;
    Node *TheNode = RI->second;
    if (isIsolated(TheNode))
        return 0;
    Node *TheLeader = TheNode->getLeader();
    return TheLeader->color;
}

void DeSSA::unionRegs(Node* Nd1, Node* Nd2)
{
    Node* N1 = Nd1->getLeader();
    Node* N2 = Nd2->getLeader();
    Node *NewLeader = nullptr;
    Node *Leadee = nullptr;

    if (N1 == N2)
        return;

    if (N1->rank > N2->rank) {
        NewLeader = N1;
        Leadee = N2;
    }
    else if (N1->rank < N2->rank) {
        NewLeader = N2;
        Leadee = N1;
    }
    else {
        NewLeader = N1;
        Leadee = N2;
        NewLeader->rank++;
    }

    assert(NewLeader && Leadee &&
        "ICE: both leader and leadee shall not be null!");
    Leadee->parent = NewLeader;

    // Link the circular list of Leadee right before NewLeader
    Node *Leadee_prev = Leadee->prev;
    Node *NewLeader_prev = NewLeader->prev;
    NewLeader_prev->next = Leadee;
    Leadee->prev = NewLeader_prev;
    Leadee_prev->next = NewLeader;
    NewLeader->prev = Leadee_prev;
}

void DeSSA::isolateReg(Value* Val) {
  Node *ND = RegNodeMap[Val];

  // Make sure that if Val is in preferred CC map, all nodes
  // of the same preferred CC should be all isolated as well.
  Value* PrefRootV = getPrefCCRoot(Val);
  if (PrefRootV) {
      SmallVector<Node*, 4> PrefNodes;
      Node *N = ND;
      while (N->next != ND) {
          N = N->next;
          Value* prefV = getPrefCCRoot(N->value);
          if (prefV == PrefRootV) {
              PrefNodes.push_back(N);
          }
      }

      splitNode(ND);
      for (int i = 0, sz = (int)PrefNodes.size(); i < sz; ++i)
      {
          Node* N = PrefNodes[i];
          splitNode(N);
          unionRegs(ND, N);
      }
  }
  else {
      splitNode(ND);
  }
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
void DeSSA::splitNode(Node* ND)
{
    Node* N = ND->next;
    if (N == ND) {
        // ND is already in a single-value congruent class
        return;
    }

    Node* Leader = ND->getLeader();

    // Remove ND from the congruent class
    Node* P = ND->prev;
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

    // If ND is a leaf node, no need to set parent. As we don't
    // know if it has any children. A path compression is done
    // always to set "Leader' as the new leader, so that all nodes
    // within a same congruent class remains in the same rooted tree.
    N = Leader->next;
    Leader->parent = Leader;
    Leader->rank = (Leader == N) ? 0 : 1;
    while (N != Leader)
    {
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
void
DeSSA::SplitInterferencesForBasicBlock(
    BasicBlock *MBB,
    DenseMap<int, Value*> &CurrentDominatingParent,
    DenseMap<Value*, Value*> &ImmediateDominatingParent) {
  // Sort defs by their order in the original basic block, as the code below
  // assumes that it is processing definitions in dominance order.
  std::vector<Instruction*> &DefInstrs = PHISrcDefs[MBB];
  std::sort(DefInstrs.begin(), DefInstrs.end(), MIIndexCompare(LV));

  for (std::vector<Instruction*>::const_iterator BBI = DefInstrs.begin(),
       BBE = DefInstrs.end(); BBI != BBE; ++BBI) {
 
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
    Value* NewParent = CurrentDominatingParent[RootC];
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
        } else if (cast<Instruction>(NewParent)->getParent() == MBB &&
                   isa<PHINode>(DefMI) && isa<PHINode>(NewParent)) {
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
    for (BasicBlock::iterator BBI = (*SI)->begin(), BBE = (*SI)->end();
         BBI != BBE; ++BBI) {
      PHINode *PHI = dyn_cast<PHINode>(BBI);
      if (!PHI) {
        break;
      }

      int RootC = getRootColor(PHI);
      // check live-out interference
      if (IGC_IS_FLAG_ENABLED(EnableDeSSAWA) && !RootC)
      {
          // [todo] delete this code
          if (CTX->type == ShaderType::COMPUTE_SHADER)
          {
              for (unsigned i = 0; !RootC && i < PHI->getNumOperands(); i++) {
                  Value* SrcVal = PHI->getOperand(i);
                  if (!isa<Constant>(SrcVal)) {
                      if (IGC_IS_FLAG_ENABLED(EnableDeSSAAlias)) {
                          SrcVal = getAliasee(SrcVal);
                      }
                      else {
                          SrcVal = getInsEltRoot(SrcVal);
                      }
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
      assert(PredIndex < PHI->getNumOperands());
      Value* PredValue = PHI->getOperand(PredIndex);
      if (IGC_IS_FLAG_ENABLED(EnableDeSSAAlias)) {
          PredValue = getAliasee(PredValue);
      }
      else {
          PredValue = getInsEltRoot(PredValue);
      }
      std::pair<Instruction*, Value*> &CurrentPHI = CurrentPHIForColor[RootC];
      // If two PHIs have the same operand from every shared predecessor, then
      // they don't actually interfere. Otherwise, isolate the current PHI. This
      // could possibly be improved, e.g. we could isolate the PHI with the
      // fewest operands.
      if (CurrentPHI.first && CurrentPHI.second != PredValue) {
        isolateReg(PHI);
        continue;
      }
      else {
        CurrentPHI = std::make_pair(PHI, PredValue);
      }

      // check live-out interference
#if 0
      Value *RootV = getRegRoot(PHI);
      for (unsigned i = 0; !RootV && i < PHI->getNumOperands(); i++) {
          Value* SrcVal = PHI->getOperand(i);
          if (!isa<Constant>(SrcVal)) {
              Value *SrcRootV = getRegRoot(SrcVal);
              if (SrcRootV && SrcRootV == OrigRootV) {
                  RootV = SrcRootV;
                  printf("JGU_DEBUG: enter this code!\n");
              }
          }
      }
#endif

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
  }
}

void
DeSSA::SplitInterferencesForArgument(
    DenseMap<int, Value*> &CurrentDominatingParent,
    DenseMap<Value*, Value*> &ImmediateDominatingParent) {
  // No two arguments can be in the same congruent class
  for (auto BBI = PHISrcArgs.begin(),
       BBE = PHISrcArgs.end(); BBI != BBE; ++BBI) {
    Value *AV = *BBI;
    // If the virtual register being defined is not used in any PHI or has
    // already been isolated, then there are no more interferences to check.
    int RootC = getRootColor(AV);
    if (!RootC)
      continue;
    Value* NewParent = CurrentDominatingParent[RootC];
    if (NewParent) {
      isolateReg(AV);
    } else {
      CurrentDominatingParent[RootC] = AV;
    }
  }
}

void DeSSA::SplitInterferencesForAlignment()
{
    for (auto I = RegNodeMap.begin(), E = RegNodeMap.end(); I != E; ++I)
    {
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
        Node* Head = Curr;
        N = Head;
        do {
            Curr = N;
            N = N->next;
            if (Curr->alignment != EALIGN_AUTO && Curr->alignment != EALIGN_GRF)
            {
                assert(Curr != Head && "Head Node cannot be isolated, something wrong!");
                isolateReg(Curr->value);
            }
        } while (N != Head);

        // Update root's alignment.
        Head->getLeader()->alignment = Align;
    }
}

Value*
DeSSA::getInsEltRoot(Value* Val) const 
{
    auto RI = InsEltMap.find(Val);
    if (RI == InsEltMap.end())
        return Val;
    return RI->second;
}

void
DeSSA::CoalesceInsertElementsForBasicBlock(BasicBlock *Blk)
{
    if (IGC_IS_FLAG_ENABLED(EnableDeSSAAlias))
    {
        for (BasicBlock::iterator BBI = Blk->begin(), BBE = Blk->end();
            BBI != BBE; ++BBI) {
            Instruction *Inst = &(*BBI);

            if (!CG->NeedInstruction(*Inst)) {
                continue;
            }
            // Only Aliasee needs to be handled.
            if (!isAliasee(Inst)) {
                continue;
            }

            // Set up preferred CC for InsertElement.
            //
            // Not sure if we really need preferred CC for insElt.
            // Since the existing code keeps them together, here keep
            // them in the preferred CC.
            if (isa<InsertElementInst>(Inst))
            {
                Value *origSrcV = Inst->getOperand(0);
                Value *SrcV = getAliasee(origSrcV);
                Value *DstV = getAliasee(Inst);
                if (SrcV != DstV &&
                    (isa<Instruction>(SrcV) || isa<Argument>(SrcV)))
                {
                    if (!interfere(SrcV, DstV) &&
                        (WIA->whichDepend(SrcV) == WIA->whichDepend(DstV)))
                    {
                        PrefCCMapAddValue(DstV);
                        PrefCCMapAddValue(SrcV);
                        PrefCCMap[DstV] = PrefCCMap[SrcV];

                        // union them
                        e_alignment InstAlign = GetPreferredAlignment(Inst, WIA, CTX);
                        addReg(Inst, InstAlign);
                        e_alignment SrcVAlign = GetPreferredAlignment(SrcV, WIA, CTX);
                        addReg(SrcV, SrcVAlign);
                        unionRegs(DstV, SrcV);
                    }
                }
            }
        }
        return;
    }

    for (BasicBlock::iterator BBI = Blk->begin(), BBE = Blk->end();
        BBI != BBE; ++BBI) {
        Instruction *Inst = &(*BBI);
        // skip phi, phi is handled in the 2nd loop
        if (isa<PHINode>(Inst))
        {
            continue;
        }

        // extend the liveness of InsertElement due to union
        for (unsigned i = 0; i < Inst->getNumOperands(); ++i) {
            Value *SrcV = Inst->getOperand(i);
            if (isa<InsertElementInst>(SrcV)) {
                Value *RootV = getInsEltRoot(SrcV);
                if (RootV != SrcV) {
                    LV->HandleVirtRegUse(RootV, Blk, Inst, true);
                }
            }
        }

        if (!isa<InsertElementInst>(Inst)) {
            continue;
        }
        // handle InsertElement
        InsEltMapAddValue(Inst);

        Value *SrcV = Inst->getOperand(0);
        if (isa<Instruction>(SrcV) || isa<Argument>(SrcV)) {
            if (!LV->isLiveAt(SrcV, Inst)) {
                Instruction *SrcDef = dyn_cast<Instruction>(SrcV);
                if (SrcDef && WIA->whichDepend(SrcDef) == WIA->whichDepend(Inst)) {
                    // passed the liveness and alignment test
                    // may need to create a node for srcv, for example, when srcv is phi/arg
                    InsEltMapAddValue(SrcV);
                    InsEltMapUnionValue(SrcV, Inst);
                 }
            }
        }
    }
    // look at all the phis in the successor blocks
    // extend live-ranges due to the union of insert-element
    for (succ_iterator SI = succ_begin(Blk), E = succ_end(Blk); SI != E; ++SI)
    {
        for (BasicBlock::iterator BBI = (*SI)->begin(), BBE = (*SI)->end(); BBI != BBE; ++BBI)
        {
            PHINode *phi = dyn_cast<PHINode>(BBI);
            if (phi)
            {
                // extend the liveness of InsertElement due to union
                Value *SrcV = phi->getIncomingValueForBlock(Blk);
                if (isa<InsertElementInst>(SrcV)) {
                    Value *RootV = getInsEltRoot(SrcV);
                    if (RootV != SrcV) {
                        BasicBlock *DefBlk = (isa<Instruction>(RootV)) ?
                            cast<Instruction>(RootV)->getParent() : NULL;
                        LV->MarkVirtRegAliveInBlock(LV->getLVInfo(RootV), DefBlk, Blk);
                    }
                }
            }
            else
            {
                break;
            }
        }
    }
}

Value* DeSSA::getRootValue(Value* Val, e_alignment *pAlign) const
{
    auto RI = InsEltMap.find(Val);
    if (RI != InsEltMap.end()) {
        Value *InsEltRoot = RI->second;
        Value *PhiRootVal = getRegRoot(InsEltRoot, pAlign);
        return (PhiRootVal ? PhiRootVal : InsEltRoot);
    }
    return getRegRoot(Val, pAlign);
}

void DeSSA::getAllValuesInCongruentClass(
	Value* V,
	SmallVector<Value*, 8>& ValsInCC)
{
	// Handle InsertElement specially. Note that only rootValue from
	// a sequence of insertElement is in congruent class. The RootValue
	// has its liveness modified to cover all InsertElements that are
	// grouped together.
    Value* RootV = V;
    if (IGC_IS_FLAG_ENABLED(EnableDeSSAAlias))
    {
        RootV = getAliasee(RootV);
    }
    else {
        RootV = getInsEltRoot(RootV);
    }
    ValsInCC.push_back(RootV);
	auto RI = RegNodeMap.find(RootV);
	if (RI != RegNodeMap.end()) {
		Node* First = RI->second;
		for (Node* N = First->next; N != First; N = N->next)
        {
            ValsInCC.push_back(N->value);
		}
	}
	return;
}

void DeSSA::CoalesceAliasInstForBasicBlock(BasicBlock *Blk)
{
    for (BasicBlock::iterator BBI = Blk->begin(), BBE = Blk->end();
        BBI != BBE; ++BBI) {
        Instruction *I = &(*BBI);

        // Now, better to think of code as a sequence Codegen Patterns,
        // not a sequence of llvm instructions.
        if (!CG->NeedInstruction(*I)) {
            continue;
        }

        if (InsertElementInst* IEI = dyn_cast<InsertElementInst>(I))
        {
            if (isa<UndefValue>(I->getOperand(0)))
            {
                SmallVector<Value*, 16> AllIEIs;
                int nelts = checkInsertElementAlias(IEI, AllIEIs);
                if (nelts > 0)
                {
                    //  This will generate the following alias:
                    //     alias(V0, Vn)
                    //     alias(V1, Vn)
                    //     alias(V2, Vn)
                    //     ......
                    //     alias(Vn, Vn)  <-- Vn is the root!
                    Value* aliasee = AllIEIs[0];
                    for (int i = 0; i < nelts; ++i) {
                        Value* V = AllIEIs[i];
                        AliasMap[V] = aliasee;

                        // union liveness info
                        LV->mergeUseFrom(aliasee, V);
                    }
                }
            }
        }
        else if (CastInst* CastI = dyn_cast<CastInst>(I))
        {
            if (IGC_GET_FLAG_VALUE(EnableDeSSAAlias) > 1)
            {
                Value* D = I;
                Value* S = I->getOperand(0);
                if (!isa<Constant>(S) &&
                    WIA->whichDepend(D) == WIA->whichDepend(S) &&
                    isNoOpInst(CastI, CTX))
                {
                    AddAlias(S);
                    AliasMap[D] = AliasMap[S];

                    // union liveness info
                    LV->mergeUseFrom(S, D);
                }
            }
        }
    }
}

int DeSSA::checkInsertElementAlias(
    InsertElementInst* IEI, SmallVector<Value*, 16>& AllIEIs)
{
    assert(isa<UndefValue>(IEI->getOperand(0)) &&
           "ICE: need to pass first IEI as the argument");

    // Find the the alias pattern:
    //     V0 = IEI UndefValue, S0, 0
    //     V1 = IEI V0,         S1, 1
    //     V2 = IEI V1,         S2, 2
    //     ......
    //     Vn = IEI Vn_1,       Sn_1, n
    // All Vi (i=0,n_1, except i=n) has a single-use.
    //
    // If found, return the vector size (non zero);
    // otherwise, return 0.
    VectorType* VTy = cast<VectorType>(IEI->getType());
    int nelts = (int)VTy->getNumElements();
    AllIEIs.resize(nelts, nullptr);
    InsertElementInst* Inst = IEI;
    WIBaseClass::WIDependancy Dep = WIA->whichDepend(Inst);
    while (Inst)
    {
        // Check if Inst has constant index, stop if not.
        ConstantInt* CI = dyn_cast<ConstantInt>(Inst->getOperand(2));
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

    // Make sure all elements are present
    for (int i = 0; i < nelts; ++i) {
        if (AllIEIs[i] == 0)
            return 0;
    }
    return nelts;
}

Value* DeSSA::getAliasee(Value* V) const
{
    auto AI = AliasMap.find(V);
    if (AI == AliasMap.end())
        return V;
    return AI->second;
}

bool DeSSA::isAlias(Value* V) const
{
    auto AI = AliasMap.find(V);
    return AI != AliasMap.end();
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
bool DeSSA::interfere(llvm::Value* V0, llvm::Value* V1)
{
    SmallVector<Value*, 8> allCC0;
    SmallVector<Value*, 8> allCC1;
    getAllValuesInCongruentClass(V0, allCC0);
    getAllValuesInCongruentClass(V1, allCC1);

    for (int i0 = 0, sz0 = (int)allCC0.size(); i0 < sz0; ++i0)
    {
        Value* val0 = allCC0[i0];
        for (int i1 = 0, sz1 = (int)allCC1.size(); i1 < sz1; ++i1)
        {
            Value* val1 = allCC1[i1];
            if (LV->hasInterference(val0, val1)) {
                return true;
            }
        }
    }
    return false;
}
