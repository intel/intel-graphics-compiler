/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef FLOWGRAPH_H
#define FLOWGRAPH_H

#include "Assertions.h"
#include "G4_BB.hpp"
#include "G4_Declare.h"
#include "G4_IR.hpp"
#include "LoopAnalysis.h"

#include <list>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vISA {
class FlowGraph;
class G4_BB;
class G4_Kernel;
class IR_Builder;
class PhyRegSummary;
class VarSplitPass;

//
// FuncInfo - Function CFG information
//    This class maintains a CFG summary of the function (its INIT block, EXIT
//    block and number of call sites). The functions's INIT block will contain a
//    pointer to its related FuncInfo object. The FuncInfo definition is used
//    for inter-procedural liveness analysis (IPA).
class FuncInfo {
private:
  unsigned id;        // the function id
  G4_BB *initBB;      // the init node
  G4_BB *exitBB;      // the exit node
  unsigned callCount; // the number of call sites

  std::vector<G4_BB *> BBList;   // the list of BBs
  std::unordered_set<G4_BB *> BBSet; // for fast lookup
  std::list<FuncInfo *> callees; // the list of callees
  unsigned scopeID;              // the function scope ID

  bool visited;
  unsigned preID;
  unsigned postID;

public:
  FuncInfo(unsigned p_id, G4_BB *p_initBB, G4_BB *p_exitBB)
      : id(p_id), initBB(p_initBB), exitBB(p_exitBB), callCount(1), scopeID(0),
        visited(false), preID(0), postID(0) {}

  ~FuncInfo() {
    BBList.clear();
    callees.clear();
  }
  FuncInfo(const FuncInfo&) = delete;
  FuncInfo& operator=(const FuncInfo&) = delete;

  void clear() {
    BBSet.clear();
    BBList.clear();
    callees.clear();
  }

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  bool doIPA() const { return callCount > 1; }

  unsigned getId() const { return id; }
  void setId(unsigned val) { id = val; }

  G4_BB *getInitBB() const { return initBB; }
  G4_BB *getExitBB() const { return exitBB; }

  void incrementCallCount() { ++callCount; }

  void updateInitBB(G4_BB *p_initBB) { initBB = p_initBB; }
  void updateExitBB(G4_BB *p_exitBB) { exitBB = p_exitBB; }

  void addCallee(FuncInfo *fn) { callees.push_back(fn); }
  std::list<FuncInfo *> &getCallees() { return callees; }

  // const version that does O(n) lookup.
  bool contains(G4_BB *bb) const {
    auto it = std::find(BBList.begin(), BBList.end(), bb);
    return it != BBList.end();
  }

  bool contains(G4_BB *bb) {
    if (BBSet.size() != BBList.size()) {
      BBSet.clear();
      std::for_each(BBList.begin(), BBList.end(),
                    [&](G4_BB *curBB) { BBSet.insert(curBB); });
    }
#ifdef _DEBUG
    // verify both containers are in sync
    for (auto subBB : BBList) {
      vISA_ASSERT(BBSet.count(subBB) == 1, "out of sync containers");
    }
#endif

    vISA_ASSERT(BBSet.size() == BBList.size(), "size mismatch");
    return BBSet.count(bb) > 0;
  }

  void addBB(G4_BB *bb) {
    vISA_ASSERT(!contains(bb), "duplicate insertion");
    BBList.push_back(bb);
  }
  std::vector<G4_BB *> &getBBList() { return BBList; }

  bool eraseBB(G4_BB *bb) {
    for (auto it = BBList.begin(); it != BBList.end(); ++it) {
      if (bb == (*it)) {
        BBList.erase(it);
        BBSet.erase(bb);
        return true;
      }
    }
    return false;
  }

  unsigned getScopeID() const { return scopeID; }
  void setScopeID(unsigned id) { scopeID = id; }

  bool getVisited() const { return visited; }
  void setVisited() { visited = true; }

  unsigned getPreID() const { return preID; }
  void setPreID(unsigned id) { preID = id; }

  unsigned getPostID() const { return postID; }
  void setPostID(unsigned id) { postID = id; }

  void dump(std::ostream &os = std::cerr) const;
}; // FuncInfo

//
// A table mapping the subroutine (INIT) block id's to their FuncInfo nodes.
//
typedef std::unordered_map<int, FuncInfo *> FuncInfoHashTable;
typedef std::unordered_map<G4_Label *, G4_BB *> Label_BB_Map;

///
/// A hashtable of <declare, node> where every node is a vector of
/// {LB, RB} (left-bounds and right-bounds)
/// A source operand (either SrcRegRegion or Predicate) is considered to be
/// global if it is not fully defined in one BB
///
class GlobalOpndHashTable {
  Mem_Manager &mem;
  std_arena_based_allocator<uint32_t> private_arena_allocator;

  static uint32_t packBound(uint16_t lb, uint16_t rb) {
    return (rb << 16) + lb;
  }

  static uint16_t getLB(uint32_t value) { return (uint16_t)(value & 0xFFFF); }
  static uint16_t getRB(uint32_t value) { return (uint16_t)(value >> 16); }

  struct HashNode {
    // each elements is {LB, RB} pair where [0:15] is LB and [16:31] is RB
    std::vector<uint32_t, std_arena_based_allocator<uint32_t>> bounds;

    HashNode(uint16_t lb, uint16_t rb, std_arena_based_allocator<uint32_t> &m)
        : bounds(m) {
      bounds.push_back(packBound(lb, rb));
    }

    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

    void insert(uint16_t newLB, uint16_t newRB);
    bool isInNode(uint16_t lb, uint16_t rb) const;
  };

  // "global" refers to declares with elements that are used without a preceding
  // define in the same BB
  std::map<G4_Declare *, HashNode *> globalVars;
  // for debugging it's often useful to dump out the global operands, not just
  // declares. Note that this may not be an exhaustive list, for example it does
  // not cover dst global operands; for accuracy one should use isOpndGlobal()
  std::vector<G4_Operand *> globalOpnds;

public:
  GlobalOpndHashTable(Mem_Manager &m) : mem(m) {}

  void addGlobalOpnd(G4_Operand *opnd);
  // returns true if def may possibly define a global variable
  bool isOpndGlobal(G4_Operand *def) const;
  void clearHashTable();

  void dump(std::ostream &os = std::cerr) const;
}; // GlobalOpndHashTable

using BBIDMap = std::unordered_map<G4_BB *, uint32_t>;

class FlowGraph {
  // This list maintains the ordering of the basic blocks (i.e., asm and binary
  // emission will output the blocks in list oder. Important: Due to the nature
  // of SIMD CF, it is unsafe to change the order of basic blocks Once the list
  // is populated in constructFlowGraph(), the only changes allowed are
  // 1. insertion of new exit BBs due to handleExit/handleReturn/handleFRet. The
  // exit BB
  //    must be the last BB for the kernel/subroutine/function it belongs to
  // 2. deletion of unreachable blocks
  // 3. merging of blocks that only contain one label with its (single)
  // successor If you need to change the block ordering for any reason, create
  // another data structure instead of modifying this one
  BB_LIST BBs;

  unsigned traversalNum = 0;    // used for flow graph traversals
  unsigned numBBId = 0;         // number of basic blocks
  bool reducible = true;        // reducibility of the graph
  bool doIPA = false;           // requires inter-procedural liveness analysis
  bool hasStackCalls = false;   // indicates that the flowgraph contains STACK_CALL calls
  bool isStackCallFunc = false; // indicates the function is a STACK_CALL function
  G4_Kernel *pKernel;           // back pointer to the kernel object

  // list of all BBs ever created
  // This list only grows and is freed when the FlowGraph is destroyed
  std::vector<G4_BB *> BBAllocList;

  // stores all INST that may be target of indirect jump. Currently these inst
  // must be jmpi themselves
  std::unordered_set<G4_INST *> indirectJmpTarget;

  // stores all endift inst that have labels associated with it
  std::unordered_map<G4_INST *, G4_Label *> endifWithLabels;

  // label to subroutine BB's map. This is used to add edges between subroutine
  // caller/callee.
  // TODO: We should use FuncInfo instead, but at the time it was
  // needed FuncInfo was not constructed yet..
  std::unordered_map<G4_Label *, std::vector<G4_BB *>> subroutines;

  vISA::ImmDominator immDom;
  vISA::PostDom pDom;
  vISA::LoopDetection loops;

  typedef std::pair<G4_BB *, G4_BB *> Edge;
  typedef std::set<G4_BB *> Blocks;
  typedef std::map<Edge, Blocks> Loop;
  std::vector<Edge> backEdges; // list of all backedges (tail->head)
  // Each natural loop is represented by the loop back edge and all of its basic
  // blocks; this including all child loops recursively, but not any subroutines
  // that are called in the loop.
  // TODO: remove this in favor of LoopAnalysis.
  Loop naturalLoops;

  // Caches to speed up the lookup of pseudo declares
  std::unordered_set<const G4_Declare *> pseudoDcls;
  std::unordered_set<const G4_Declare *> pseudoVCADcls;
  std::unordered_set<const G4_Declare *> pseudoA0Dcls;
public:
  Mem_Manager &mem; // mem mananger for creating BBs & starting IP table
  INST_LIST_NODE_ALLOCATOR &instListAlloc;

  Loop &getAllNaturalLoops() { return naturalLoops; }

  // function info nodes. entry function is not included.
  std::vector<FuncInfo *> funcInfoTable;

  std::vector<FuncInfo *>
      sortedFuncTable; // subroutines in reverse topographical order (leaf at
                       // top) kernelInfo is the last element with invalid func
                       // id. When there are no subroutines, this container
                       // is empty, ie it doesn't contain kernelInfo.

  FuncInfo *kernelInfo = nullptr; // the call info for the kernel function

  IR_Builder *builder = nullptr; // needed to create new instructions (mainly labels)

  // TODO: It's rather strange that global operand table is part of FlowGraph.
  //       Consider moving it and the class elsewhere.
  GlobalOpndHashTable globalOpndHT;

  G4_Declare *framePtrDcl = nullptr;
  G4_Declare *stackPtrDcl = nullptr;
  G4_Declare *scratchRegDcl = nullptr;
  G4_Declare *pseudoVCEDcl = nullptr;
  // When this is true, we reserve physical register assigned to SR.
  bool reserveSR = false;

  // pseudo declares used by RA to model the save/restore variables at each call
  // site
  struct PseudoDcls {
    G4_Declare *VCA;
    G4_Declare *A0;
    G4_Declare *Flag;
  };

  std::unordered_map<G4_InstCF *, PseudoDcls> fcallToPseudoDclMap;

  // offset in unit of OW
  unsigned callerSaveAreaOffset = 0;
  unsigned calleeSaveAreaOffset = 0;
  unsigned frameSizeInOWord = 0;

  // Bank conflict statistics.
  struct BankConflictStatistics {
    unsigned NumOfGoodInsts = 0;
    unsigned NumOfBadInsts = 0;
    unsigned NumOfOKInsts = 0;

    void addGood() { ++NumOfGoodInsts; }
    void addBad() { ++NumOfBadInsts; }
    void addOK() { ++NumOfOKInsts; }
    void clear() {
      NumOfGoodInsts = 0;
      NumOfBadInsts = 0;
      NumOfOKInsts = 0;
    }
  } BCStats;

  // This flag controls whether addPredSucc will link FuncInfo* to new BB
  bool canUpdateFuncInfo = false;

public:
  // forwarding functions to the BBs list
  BB_LIST_ITER begin() { return BBs.begin(); }
  BB_LIST_ITER end() { return BBs.end(); }
  BB_LIST::reverse_iterator rbegin() { return BBs.rbegin(); }
  BB_LIST::reverse_iterator rend() { return BBs.rend(); }
  BB_LIST::const_iterator cbegin() const { return BBs.cbegin(); }
  BB_LIST::const_iterator cend() const { return BBs.cend(); }
  BB_LIST::const_reverse_iterator crbegin() const { return BBs.crbegin(); }
  BB_LIST::const_reverse_iterator crend() const { return BBs.crend(); }

  size_t size() { return BBs.size(); }
  bool empty() const { return BBs.empty(); }
  G4_BB *back() const { return BBs.back(); }

  static void setPhysicalLink(G4_BB *pred, G4_BB *succ);

  BB_LIST_ITER insert(BB_LIST_ITER iter, G4_BB *bb);

  void push_back(G4_BB *bb) { insert(BBs.end(), bb); }

  void erase(BB_LIST_ITER iter);

  BB_LIST &getBBList() { return BBs; }

  // add BB to be the first BB
  void addPrologBB(G4_BB *BB);

  // append another CFG's BBs to this CFG.
  // note that we don't add additional CFG edges as its purpose is just to
  // stitch the two binaries togather
  void append(const FlowGraph &otherFG);

  G4_BB *getLabelBB(Label_BB_Map &map, G4_Label *label);
  G4_BB *beginBB(Label_BB_Map &map, G4_INST *first);

  bool performIPA() const { return doIPA; }

  bool getHasStackCalls() const { return hasStackCalls; }
  void setHasStackCalls() { hasStackCalls = true; }
  void resetHasStackCalls() { hasStackCalls = false; }

  bool getIsStackCallFunc() const { return isStackCallFunc; }
  void setIsStackCallFunc() { isStackCallFunc = true; }

  G4_Kernel *getKernel() { return pKernel; }

  void mergeFReturns();

  G4_Declare *&getFramePtrDcl() { return framePtrDcl; }
  G4_Declare *&getStackPtrDcl() { return stackPtrDcl; }
  G4_Declare *&getScratchRegDcl() { return scratchRegDcl; }

  bool isPseudoVCEDcl(const G4_Declare *dcl) const {
    return dcl == pseudoVCEDcl;
  }
  bool isPseudoVCADcl(const G4_Declare *dcl) const {
   return pseudoVCADcls.count(dcl) == 1;
  }
  bool isPseudoA0Dcl(const G4_Declare *dcl) const {
    return pseudoA0Dcls.count(dcl) == 1;
  }
  bool isPseudoDcl(const G4_Declare *dcl) const {
    if (!getHasStackCalls() && !getIsStackCallFunc()) {
      return false;
    }
    return pseudoDcls.count(dcl) == 1;
  }

  //
  // Merge multiple returns into one, prepare for spill code insertion
  //
  void mergeReturn(FuncInfoHashTable &funcInfoTable);
  G4_BB *mergeSubRoutineReturn(G4_Label *subroutine);
  void normalizeSubRoutineBB(FuncInfoHashTable &funcInfoTable);
  void processGoto();
  void processSCF();
  // Insert a join at the beginning of 'bb' with given 'execsize' and
  // 'maskoffset'. If a join is already present, update that join to cover the
  // given 'execsize' and 'maskoffset'.
  void insertJoinToBB(G4_BB *bb, G4_ExecSize execSize, G4_Label *jip,
                      uint8_t maskOffset = 0);

  // functions for structure analysis
  G4_Kernel *getKernel() const { return pKernel; }
  void setJIPForEndif(G4_INST *endif, G4_INST *target, G4_BB *targetBB);
  void convertGotoToJmpi(G4_INST *gotoInst);
  G4_BB *getSinglePredecessor(G4_BB *BB, G4_BB *ExcludedPred) const;
  bool convertJmpiToGoto();
  bool
  convertPredCall(std::unordered_map<G4_Label *, G4_BB *> &aLabelMap); // for WA

  unsigned getNumFuncs() const { return unsigned(funcInfoTable.size()); }

  void handleReturn(Label_BB_Map &map, FuncInfoHashTable &funcInfoTable);
  void linkReturnAddr(G4_BB *bb, G4_BB *returnAddr);

  void handleExit(G4_BB *lastKernelBB);
  void handleWait();

  void preprocess(INST_LIST &instlist);

  FlowGraph() = delete;

  FlowGraph(INST_LIST_NODE_ALLOCATOR& alloc, G4_Kernel* kernel, Mem_Manager& m)
      : pKernel(kernel), immDom(*kernel), pDom(*kernel), loops(*kernel), mem(m),
        instListAlloc(alloc), globalOpndHT(m)
  {}

  ~FlowGraph();
  FlowGraph(const FlowGraph&) = delete;
  FlowGraph& operator=(const FlowGraph&) = delete;

  void setBuilder(IR_Builder *pBuilder) { builder = pBuilder; }

  bool updateFuncInfoPredSucc(G4_BB* pred, G4_BB* succ) const {
    // When this flag is set, this method updates FuncInfo with
    // new G4_BB. When it's false, FuncInfo is not updated.
    // The flag is false when:
    // 1. Creating CFG as FuncInfo* is computed towards end of
    //    constructFlowGraph().
    // 2. Just before binary emission in presence of stack call.
    //    Because at this stage, we link kernel and stack call
    //    functions. However, this linking of CFGs is not
    //    expected to change FuncInfo* for either pred or succ.
    // 3. When inserting BB with pred and succ in different
    //    FuncInfo instances. This is a corner case that requires
    //    setting the flag to false, inserting the pred/succ link
    //    and then setting the flag back to true. It's user's
    //    responsibility to set FuncInfo* of newly inserted G4_BB.
    if (canUpdateFuncInfo) {
      // Either pred or succ is newly created. Add newly
      // created G4_BB to appropriate FuncInfo.
      // Note that when CFG is still being constructed,
      // all G4_BBs will have funcInfo == nullptr. This
      // logic works only after CFG is constructed and
      // all G4_BBs have valid FuncInfo* attached.
      if (!pred->getFuncInfo() && succ->getFuncInfo()) {
        pred->setFuncInfo(succ->getFuncInfo());
        pred->getFuncInfo()->addBB(pred);
      } else if (!succ->getFuncInfo() && pred->getFuncInfo()) {
        succ->setFuncInfo(pred->getFuncInfo());
        succ->getFuncInfo()->addBB(succ);
      }

      vISA_ASSERT(pred->getFuncInfo() == succ->getFuncInfo(),
                  "invalid func info");
      // check that pred, succ don't straddle subroutine boundary
      vISA_ASSERT(!(pred->getBBType() == G4_BB_CALL_TYPE &&
                    succ->getBBType() == G4_BB_INIT_TYPE),
                  "not expecting to set same FuncInfo for pred/succ from "
                  "different subroutines");
      vISA_ASSERT(!(pred->getBBType() == G4_BB_EXIT_TYPE &&
                    succ->getBBType() == G4_BB_RETURN_TYPE),
                  "not expecting to set same FuncInfo for pred/succ from "
                  "different subroutines");

      return true;
    }

    return false;
  }

  void addPredSuccEdges(G4_BB *pred, G4_BB *succ, bool tofront = true) {
    markStale();

    if (tofront)
      pred->Succs.push_front(succ);
    else
      pred->Succs.push_back(succ);

    succ->Preds.push_front(pred);

    updateFuncInfoPredSucc(pred, succ);
  }

  void addUniquePredSuccEdges(G4_BB *pred, G4_BB *succ, bool tofront = true) {
    // like above, but check for duplicate edges
    auto iter = std::find(pred->Succs.begin(), pred->Succs.end(), succ);
    if (iter == pred->Succs.end()) {
      addPredSuccEdges(pred, succ, tofront);
    }
  }

  void removePredSuccEdges(G4_BB *pred, G4_BB *succ);

  G4_INST *createNewLabelInst(G4_Label *label);

  G4_BB *createNewBB(bool insertInFG = true);
  G4_BB *createNewBBWithLabel(const char *LabelSuffix);
  int64_t insertDummyUUIDMov();
  //
  // Increase by one so that all BBs' traversal are less than traversalNum
  //
  void prepareTraversal() { traversalNum++; }
  unsigned getTraversalNum() { return traversalNum; }

  //
  // Check if the graph is reducible
  //
  bool isReducible() { return reducible; }

  //
  // Remove any placeholder empty blocks that could have been inserted to aid
  // analysis
  //
  void removeRedundantLabels();
  //
  // remove any mov with the same src and dst opnds
  //
  void removeRedundMov();
  //
  // Remove any placeholder empty blocks that could have been inserted to aid
  // analysis.
  //
  void removeEmptyBlocks();
  //
  // Re-assign block ID so that we can use id to determine the ordering of two
  // blocks in the code layout
  //
  void reassignBlockIDs();

  //
  // Remove blocks that are unreachable via control flow of program
  //
  void removeUnreachableBlocks(FuncInfoHashTable &funcInfoHT);

  //
  // Recompute preId of all BBs in FlowGraph
  void recomputePreId(BBIDMap &IDMap);

  void constructFlowGraph(INST_LIST &instlist);
  bool matchBranch(int &sn, INST_LIST &instlist, INST_LIST_ITER &it);

  void localDataFlowAnalysis();
  void resetLocalDataFlowData();

  unsigned getNumBB() const { return numBBId; }
  G4_BB *getEntryBB() { return BBs.front(); }

  void addFrameSetupDeclares(IR_Builder &builder, PhyRegPool &regPool);
  void addSaveRestorePseudoDeclares(IR_Builder &builder);
  void markDivergentBBs();

  // Used for CISA 3.0
  void incrementNumBBs() { numBBId++; }
  G4_BB *getUniqueReturnBlock();

  void normalizeFlowGraph();

  // This is mainly used to link subroutine call-return BBs
  // ToDo: maintain this during BB add/delete instead of having to call it
  // explicitly
  void setPhysicalPredSucc();

  void findBackEdges();
  void findNaturalLoops();

  void traverseFunc(FuncInfo *func, unsigned *ptr);
  void topologicalSortCallGraph();
  void findDominators(std::map<FuncInfo *, std::set<FuncInfo *>> &domMap);
  unsigned resolveVarScope(G4_Declare *dcl, FuncInfo *func);
  void markVarScope(std::vector<G4_BB *> &BBList, FuncInfo *func);
  void markScope();

  void addSIMDEdges();

  uint32_t getNumCalls() const {
    uint32_t numCalls = 0;
    for (auto bb : BBs) {
      if (bb->isEndWithCall()) {
        ++numCalls;
      }
    }
    return numCalls;
  }

  bool isIndirectJmpTarget(G4_INST *inst) const {
    return indirectJmpTarget.count(inst) > 0;
  }

  G4_Label *getLabelForEndif(G4_INST *inst) const {
    auto iter = endifWithLabels.find(inst);
    if (iter != endifWithLabels.end()) {
      return iter->second;
    } else {
      return nullptr;
    }
  }

  bool endWithGotoInLastBB() const {
    if (BBs.empty()) {
      return false;
    }
    G4_BB *lastBB = back();
    return lastBB->isEndWithGoto();
  }

  /// Return true if PredBB->SuccBB is a backward branch goto/jmpi/while.
  bool isBackwardBranch(G4_BB *PredBB, G4_BB *SuccBB) const {
    if (PredBB->size() == 0)
      return false;
    G4_INST *bInst = PredBB->back();
    G4_BB *targetBB = PredBB->Succs.size() > 0 ? PredBB->Succs.back() : nullptr;
    bool isBr = (bInst->opcode() == G4_goto || bInst->opcode() == G4_jmpi);
    // Note that isBackward() should return true for while as well.
    return targetBB == SuccBB && ((isBr && bInst->asCFInst()->isBackward()) ||
                                  bInst->opcode() == G4_while);
  }

  void setABIForStackCallFunctionCalls();

  // This is for TGL WA
  void
  findNestedDivergentBBs(std::unordered_map<G4_BB *, int> &nestedDivergentBBs);

  void print(std::ostream &OS) const;
  void dump() const; // used in debugger

  ImmDominator &getImmDominator() { return immDom; }
  PostDom &getPostDominator() { return pDom; }
  LoopDetection &getLoops() { return loops; }
  void markStale();

private:
  // Use normalized region descriptors for each source operand if possible.
  void normalizeRegionDescriptors();

  void decoupleReturnBlock(G4_BB *);
  void decoupleInitBlock(G4_BB *, FuncInfoHashTable &funcInfoTable);
  using BBPrePostIDMap = std::unordered_map<G4_BB *, std::array<uint32_t, 2>>;
  void DFSTraverse(G4_BB *bb, unsigned &preId, unsigned &postId, FuncInfo *fn,
                   BBPrePostIDMap &BBIdMap);
  void fillPseudoDclMap(G4_InstCF *cfInst, G4_Declare *VCA, G4_Declare *saveA0,
                        G4_Declare *saveFlag);

}; // FlowGraph

} // namespace vISA
#endif // FLOWGRAPH_H
