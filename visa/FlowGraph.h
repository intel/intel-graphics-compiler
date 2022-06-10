/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef FLOWGRAPH_H
#define FLOWGRAPH_H

#include "VISADefines.h"
#include "G4_BB.hpp"
#include "G4_IR.hpp"
#include "RelocationInfo.h"
#include "LoopAnalysis.h"

#include <list>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace vISA
{
class FlowGraph;
class G4_BB;
class G4_Kernel;
class IR_Builder;
class PhyRegSummary;
class VarSplitPass;

//
// FuncInfo - Function CFG information
//    This class maintains a CFG summary of the function (its INIT block, EXIT block and
//    number of call sites). The functions's INIT block will contain a pointer to its
//    related FuncInfo object. The FuncInfo definition is used for inter-procedural liveness
//    analysis (IPA).
class FuncInfo
{
private:
    unsigned id;        // the function id
    G4_BB*   initBB;    // the init node
    G4_BB*   exitBB;    // the exit node
    unsigned callCount; // the number of call sites

    std::vector<G4_BB*>  BBList;      // the list of BBs
    std::list<FuncInfo *>  callees; // the list of callees
    unsigned scopeID;               // the function scope ID

    bool visited;
    unsigned preID;
    unsigned postID;

public:

    FuncInfo(unsigned p_id, G4_BB* p_initBB, G4_BB* p_exitBB)
        : id(p_id), initBB(p_initBB), exitBB(p_exitBB), callCount(1)
        , scopeID(0), visited(false), preID(0), postID(0)
    {
    }

    ~FuncInfo()
    {
        BBList.clear();
        callees.clear();
    }

    void *operator new(size_t sz, Mem_Manager& m)    {return m.alloc(sz);}

    bool doIPA() const {return callCount > 1;}

    unsigned getId() const {return id;}
    void setId(unsigned val) {id = val;}

    G4_BB* getInitBB() const {return initBB;}
    G4_BB* getExitBB() const {return exitBB;}

    void incrementCallCount() {++callCount;}

    void updateInitBB(G4_BB* p_initBB) {initBB = p_initBB;}
    void updateExitBB(G4_BB* p_exitBB) {exitBB = p_exitBB;}

    void                    addCallee(FuncInfo *fn) {callees.push_back(fn);}
    std::list<FuncInfo *>&  getCallees() {return callees;}

    void                  addBB(G4_BB* bb) {BBList.push_back(bb);}
    std::vector<G4_BB*>&  getBBList() {return BBList;}

    unsigned getScopeID() const {return scopeID;}
    void     setScopeID(unsigned id) {scopeID = id;}

    bool     getVisited() const {return visited;}
    void     setVisited() {visited = true;}

    unsigned getPreID() const {return preID;}
    void     setPreID(unsigned id) {preID = id;}

    unsigned getPostID() const {return postID;}
    void     setPostID(unsigned id) {postID = id;}

    void dump() const;
}; // FuncInfo


//
// A table mapping the subroutine (INIT) block id's to their FuncInfo nodes.
//
typedef std::unordered_map<int, FuncInfo*> FuncInfoHashTable;
typedef std::unordered_map<G4_Label*, G4_BB*> Label_BB_Map;

class FlowGraph
{
    // This list maintains the ordering of the basic blocks (i.e., asm and binary emission will output
    // the blocks in list oder.
    // Important: Due to the nature of SIMD CF, it is unsafe to change the order of basic blocks
    // Once the list is populated in constructFlowGraph(), the only changes allowed are
    // 1. insertion of new exit BBs due to handleExit/handleReturn/handleFRet.  The exit BB
    //    must be the last BB for the kernel/subroutine/function it belongs to
    // 2. deletion of unreachable blocks
    // 3. merging of blocks that only contain one label with its (single) successor
    // If you need to change the block ordering for any reason, create another data structure instead of
    // modifying this one
    BB_LIST BBs;

    unsigned traversalNum;                      // used for flow graph traversals
    unsigned numBBId;                            // number of basic blocks
    bool     reducible;                            // reducibility of the graph
    bool     doIPA;                             // requires inter-procedural liveness analysis
    bool     hasStackCalls;                     // indicates that the flowgraph contains STACK_CALL calls
    bool     isStackCallFunc;                    // indicates the function itself is a STACK_CALL function
    G4_Kernel* pKernel;                         // back pointer to the kernel object

    // map each BB to its local RA GRF usage summary, populated in local RA
    std::map<G4_BB*, PhyRegSummary*> bbLocalRAMap;
    // vector of summaries created for each BB, needed for deallocation later
    std::vector<PhyRegSummary*> localRASummaries;

    // list of all BBs ever created
    // This list only grows and is freed when the FlowGraph is destroyed
    std::vector<G4_BB*> BBAllocList;

    // stores all INST that may be target of indirect jump. Currently these inst must be jmpi themselves
    std::unordered_set<G4_INST*> indirectJmpTarget;

    // stores all endift inst that have labels associated with it
    std::unordered_map<G4_INST*, G4_Label*> endifWithLabels;

    // label to subroutine BB's map. This is used to add edges between subroutine caller/callee
    // ToDo: We should use FuncInfo instead, but at the time it was needed FuncInfo was not constructed yet..
    std::unordered_map<G4_Label*, std::vector<G4_BB*>> subroutines;

    vISA::ImmDominator immDom;
    vISA::PostDom pDom;
    vISA::LoopDetection loops;

public:
    typedef std::pair<G4_BB*, G4_BB*> Edge;
    typedef std::set<G4_BB*> Blocks;
    typedef std::map<Edge, Blocks> Loop;

    Mem_Manager& mem;                            // mem mananger for creating BBs & starting IP table
    INST_LIST_NODE_ALLOCATOR& instListAlloc;     // a reference to dedicated mem allocator for holding instruction list nodes

    std::list<Edge> backEdges;                  // list of all backedges (tail->head)
    Loop naturalLoops;

    // function info nodes. entry function is not included.
    std::vector<FuncInfo*> funcInfoTable;

    std::vector<FuncInfo*> sortedFuncTable;     // subroutines in reverse topographical order (leaf at top)
                                                // kernelInfo is the last element with invalid func id

    FuncInfo* kernelInfo;                       // the call info for the kernel function

    IR_Builder *builder;                        // needed to create new instructions (mainly labels)
    GlobalOpndHashTable globalOpndHT;

    G4_Declare *            framePtrDcl;
    G4_Declare *            stackPtrDcl;
    G4_Declare *            scratchRegDcl;
    G4_Declare *            pseudoVCEDcl;

    // pseudo declares used by RA to model the save/restore variables at each call site
    struct PseudoDcls
    {
        G4_Declare* VCA;
        G4_Declare* A0;
        G4_Declare* Flag;
    };

    std::unordered_map<G4_InstCF*, struct PseudoDcls> fcallToPseudoDclMap;

    // offset in unit of OW
    unsigned                    callerSaveAreaOffset = 0;
    unsigned                    calleeSaveAreaOffset = 0;
    unsigned                    frameSizeInOWord = 0;

    // Bank conflict statistics.
    struct BankConflictStatistics
    {
        unsigned NumOfGoodInsts = 0;
        unsigned NumOfBadInsts = 0;
        unsigned NumOfOKInsts = 0;

        void addGood() { ++NumOfGoodInsts; }
        void addBad() { ++NumOfBadInsts; }
        void addOK() { ++NumOfOKInsts; }
        void clear()
        {
            NumOfGoodInsts = 0;
            NumOfBadInsts = 0;
            NumOfOKInsts = 0;
        }
    } BCStats;

    struct XeBankConflictStatistics
    {
        unsigned simd8 = 0;    //The number of simd8 instructions, one simd16 is treated as two simd8 if it's not HF
        unsigned BCNum = 0;  //The number of band conflict.
        int sameBankConflicts = 0;
        int simd16ReadSuppression = 0;
        int twoSrcBC = 0;

        //For the static profiling of acc regsiter substituion ratio
        //The operand numbers which are be replaced with acc.
        //Def:dst operand, Use:src operand
        unsigned accSubDef = 0;
        unsigned accSubUse = 0;

        //Candidates, which may be substituted with acc, or not because of spill
        unsigned accSubCandidateDef = 0;
        unsigned accSubCandidateUse = 0;

        void clear()
        {
            simd8 = 0;
            BCNum = 0;
            sameBankConflicts = 0;
            simd16ReadSuppression = 0;
            twoSrcBC = 0;
        }
        void addBC(unsigned num) { BCNum += num; }
        void addSameBankBC(unsigned num) { sameBankConflicts += num; }
        void addSimd16RSBC(unsigned num) { simd16ReadSuppression += num; }
        void add2SrcBC(unsigned num) { twoSrcBC += num; }
        void addSIMD8() { ++simd8; }

        void setAccSubDef(unsigned num) { accSubDef = num; }
        void setAccSubUse(unsigned num) { accSubUse = num; }
        void setAccSubCandidateDef(unsigned num) { accSubCandidateDef = num; }
        void setAccSubCandidateUse(unsigned num) { accSubCandidateUse = num; }

    } XeBCStats;
    unsigned numRMWs = 0;    // counting the number of read-modify-write
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
    G4_BB* back() const {return BBs.back(); }

    static void setPhysicalLink(G4_BB* pred, G4_BB* succ);

    BB_LIST_ITER insert(BB_LIST_ITER iter, G4_BB* bb);


    void push_back(G4_BB* bb) {insert(BBs.end(), bb);}

    void erase(BB_LIST_ITER iter);

    BB_LIST& getBBList() { return BBs; }

    // add BB to be the first BB
    void addPrologBB(G4_BB* BB);


    // append another CFG's BBs to this CFG.
    // note that we don't add additional CFG edges as its purpose is just to
    // stitch the two binaries togather
    void append(const FlowGraph& otherFG);

    G4_BB* getLabelBB(Label_BB_Map& map, G4_Label* label);
    G4_BB* beginBB(Label_BB_Map& map, G4_INST* first);

    bool performIPA() const {return doIPA;}

    bool getHasStackCalls() const { return hasStackCalls; }
    void setHasStackCalls() { hasStackCalls = true; }
    void resetHasStackCalls() { hasStackCalls = false;}

    bool getIsStackCallFunc() const {return isStackCallFunc;}
    void setIsStackCallFunc() {isStackCallFunc = true;}

    G4_Kernel* getKernel() {return pKernel;}

    void mergeFReturns();

    G4_Declare*& getFramePtrDcl()                   {return framePtrDcl;}
    G4_Declare*& getStackPtrDcl()                   {return stackPtrDcl;}
    G4_Declare*& getScratchRegDcl()                 {return scratchRegDcl;}

    bool isPseudoVCEDcl(const G4_Declare* dcl) const { return dcl == pseudoVCEDcl; }
    bool isPseudoVCADcl(const G4_Declare* dcl) const
    {
        for (auto iter : fcallToPseudoDclMap)
        {
            if (iter.second.VCA == dcl)
            {
                return true;
            }
        }
        return false;
    }
    bool isPseudoA0Dcl(const G4_Declare* dcl) const
    {
        for (auto iter : fcallToPseudoDclMap)
        {
            if (iter.second.A0 == dcl)
            {
                return true;
            }
        }
        return false;
    }
    bool isPseudoFlagDcl(const G4_Declare* dcl) const
    {
        for (auto iter : fcallToPseudoDclMap)
        {
            if (iter.second.Flag == dcl)
            {
                return true;
            }
        }
        return false;
    }
    bool isPseudoDcl(const G4_Declare* dcl) const
    {
        if (!getHasStackCalls() && !getIsStackCallFunc())
        {
            return false;
        }
        if (isPseudoVCEDcl(dcl))
        {
            return true;
        }
        for (auto iter : fcallToPseudoDclMap)
        {
            if (iter.second.A0 == dcl || iter.second.Flag == dcl || iter.second.VCA == dcl)
            {
                return true;
            }
        }
        return false;
    }

    //
    // Merge multiple returns into one, prepare for spill code insertion
    //
    void mergeReturn(FuncInfoHashTable& funcInfoTable);
    G4_BB* mergeSubRoutineReturn(G4_Label* subroutine);
    void normalizeSubRoutineBB(FuncInfoHashTable& funcInfoTable);
    void processGoto(bool HasSIMDCF);
    void processSCF(FuncInfoHashTable& FuncInfoMap);
    // Insert a join at the beginning of 'bb' with given 'execsize' and 'maskoffset'.
    // If a join is already present, update that join to cover the given 'execsize' and 'maskoffset'.
    void insertJoinToBB(G4_BB* bb, G4_ExecSize execSize, G4_Label* jip, uint8_t maskOffset = 0);

    // functions for structure analysis
    G4_Kernel *getKernel() const { return pKernel; }
    G4_Label* insertEndif(G4_BB* bb, G4_ExecSize execSize, bool createLabel);
    void setJIPForEndif(G4_INST* endif, G4_INST* target, G4_BB* targetBB);
    void convertGotoToJmpi(G4_INST *gotoInst);
    G4_BB* getSinglePredecessor(G4_BB* BB, G4_BB* ExcludedPred) const;
    bool convertJmpiToGoto();

    unsigned getNumFuncs() const {return unsigned(funcInfoTable.size());}

    void handleReturn(Label_BB_Map& map, FuncInfoHashTable& funcInfoTable);
    void linkReturnAddr(G4_BB* bb, G4_BB* returnAddr);

    void handleExit(G4_BB* lastKernelBB);
    void handleWait();

    void preprocess(INST_LIST& instlist);

    FlowGraph() = delete;
    FlowGraph(const FlowGraph&) = delete;
    FlowGraph& operator=(const FlowGraph&) = delete;

    FlowGraph(INST_LIST_NODE_ALLOCATOR& alloc, G4_Kernel* kernel, Mem_Manager& m) :
      traversalNum(0), numBBId(0), reducible(true),
      doIPA(false), hasStackCalls(false), isStackCallFunc(false),
      pKernel(kernel), mem(m), instListAlloc(alloc),
      kernelInfo(NULL), builder(NULL), globalOpndHT(m), framePtrDcl(NULL),
      stackPtrDcl(NULL), scratchRegDcl(NULL), pseudoVCEDcl(NULL),
      immDom(*kernel), pDom(*kernel), loops(*kernel) {}

    ~FlowGraph();

    void setBuilder(IR_Builder *pBuilder) {builder = pBuilder;}

    void addPredSuccEdges(G4_BB* pred, G4_BB* succ, bool tofront=true)
    {
        markStale();

        if (tofront)
            pred->Succs.push_front(succ);
        else
            pred->Succs.push_back(succ);

        succ->Preds.push_front(pred);
    }

    void addUniquePredSuccEdges(G4_BB* pred, G4_BB* succ, bool tofront=true)
    {
        // like above, but check for duplicate edges
        auto iter = std::find(pred->Succs.begin(), pred->Succs.end(), succ);
        if (iter == pred->Succs.end())
        {
            addPredSuccEdges(pred, succ, tofront);
        }
    }

    void removePredSuccEdges(G4_BB* pred, G4_BB* succ);

    G4_INST* createNewLabelInst(G4_Label* label, int lineNo = 0, int CISAOff = -1);

    G4_BB* createNewBB(bool insertInFG = true);
    G4_BB* createNewBBWithLabel(const char* LabelSuffix, int Lineno = 0, int CISAoff = -1);
    int64_t insertDummyUUIDMov();
    //
    // Increase by one so that all BBs' traversal are less than traversalNum
    //
    void prepareTraversal()    {traversalNum++;}
    unsigned getTraversalNum() {return traversalNum;}

    //
    // Check if the graph is reducible
    //
    bool isReducible() { return reducible; }

    //
    // Remove any placeholder empty blocks that could have been inserted to aid analysis
    //
    void removeRedundantLabels();
    //
    // remove any mov with the same src and dst opnds
    //
    void removeRedundMov();
    //
    // Remove any placeholder empty blocks that could have been inserted to aid analysis.
    //
    void removeEmptyBlocks();
    //
    // Add a dummy BB for multiple-exit flow graph
    //
    void linkDummyBB();
    //
    // Re-assign block ID so that we can use id to determine the ordering of two blocks in
    // the code layout
    //
    void reassignBlockIDs();

    //
    // Remove blocks that are unreachable via control flow of program
    //
    void removeUnreachableBlocks(FuncInfoHashTable& funcInfoHT);

    void constructFlowGraph(INST_LIST& instlist);
    bool matchBranch(int &sn, INST_LIST& instlist, INST_LIST_ITER &it);

    void localDataFlowAnalysis();
    void resetLocalDataFlowData();

    unsigned getNumBB() const {return numBBId;}
    G4_BB* getEntryBB()       {return BBs.front();}

    void addFrameSetupDeclares(IR_Builder& builder, PhyRegPool& regPool);
    void addSaveRestorePseudoDeclares(IR_Builder& builder);
    void markDivergentBBs();

    // Used for CISA 3.0
    void incrementNumBBs() { numBBId++ ; }
    G4_BB* getUniqueReturnBlock();

    void normalizeFlowGraph();

    // This is mainly used to link subroutine call-return BBs
    // ToDo: maintain this during BB add/delete instead of having to call it explicitly
    void setPhysicalPredSucc();

    void markRPOTraversal();

    void findBackEdges();
    void findNaturalLoops();

    void traverseFunc(FuncInfo* func, unsigned *ptr);
    void topologicalSortCallGraph();
    void findDominators(std::map<FuncInfo*, std::set<FuncInfo*>>& domMap);
    unsigned resolveVarScope(G4_Declare* dcl, FuncInfo* func);
    void markVarScope(std::vector<G4_BB*>& BBList, FuncInfo* func);
    void markScope();

    void addSIMDEdges();

    void addBBLRASummary(G4_BB* bb, PhyRegSummary* summary)
    {
        bbLocalRAMap.insert(std::make_pair(bb, summary));
        localRASummaries.push_back(summary);
    }

    void clearBBLRASummaries()
    {
        bbLocalRAMap.clear();
    }

    PhyRegSummary* getBBLRASummary(G4_BB* bb) const
    {
        auto&& iter = bbLocalRAMap.find(bb);
        return iter != bbLocalRAMap.end() ? iter->second : nullptr;
    }

    uint32_t getNumCalls() const
    {
        uint32_t numCalls = 0;
        for (auto bb : BBs)
        {
            if (bb->isEndWithCall())
            {
                ++numCalls;
            }
        }
        return numCalls;
    }

    bool isIndirectJmpTarget(G4_INST* inst) const
    {
        return indirectJmpTarget.count(inst) > 0;
    }

    G4_Label* getLabelForEndif(G4_INST* inst) const
    {
        auto iter = endifWithLabels.find(inst);
        if (iter != endifWithLabels.end())
        {
            return iter->second;
        }
        else
        {
            return nullptr;
        }
    }

    bool endWithGotoInLastBB() const
    {
        if (BBs.empty())
        {
            return false;
        }
        G4_BB* lastBB = back();
        return lastBB->isEndWithGoto();
    }

    /// Return true if PredBB->SuccBB is a backward branch goto/jmpi/while.
    bool isBackwardBranch(G4_BB* PredBB, G4_BB* SuccBB) const
    {
        if (PredBB->size() == 0) return false;
        G4_INST* bInst = PredBB->back();
        G4_BB* targetBB = PredBB->Succs.size() > 0 ? PredBB->Succs.back() : nullptr;
        bool isBr = (bInst->opcode() == G4_goto || bInst->opcode() == G4_jmpi);
        // Note that isBackward() should return true for while as well.
        return targetBB == SuccBB &&
            ((isBr && bInst->asCFInst()->isBackward()) || bInst->opcode() == G4_while);
    }

    void setABIForStackCallFunctionCalls();

    // This is for TGL WA
    void findNestedDivergentBBs(std::unordered_map<G4_BB*, int>& nestedDivergentBBs);

    void print(std::ostream& OS) const;
    void dump() const;  // used in debugger

    ImmDominator& getImmDominator() { return immDom; }
    PostDom& getPostDominator() { return pDom; }
    LoopDetection& getLoops() { return loops; }
    void markStale();

private:
    // Use normalized region descriptors for each source operand if possible.
    void normalizeRegionDescriptors();

    // Find the BB that has the given label from the range [StartIter, EndIter).
    static G4_BB* findLabelBB(
        BB_LIST_ITER StartIter,
        BB_LIST_ITER EndIter,
        const char* Label);

    void decoupleReturnBlock(G4_BB*);
    void decoupleInitBlock(G4_BB*, FuncInfoHashTable& funcInfoTable);
    void DFSTraverse(G4_BB* bb, unsigned& preId, unsigned& postId, FuncInfo* fn);

}; // FlowGraph

} // vISA::
#endif // FLOWGRAPH_H
