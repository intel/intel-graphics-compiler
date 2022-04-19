/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef G4_BB_H
#define G4_BB_H

#include "G4_IR.hpp"

#include <list>
#include <unordered_map>

namespace vISA
{
class G4_BB;

typedef std::list<G4_BB*>                 BB_LIST;
typedef BB_LIST::iterator                 BB_LIST_ITER;
typedef BB_LIST::const_iterator           BB_LIST_CITER;
typedef BB_LIST::reverse_iterator         BB_LIST_RITER;

//
// Block types
//
enum G4_BB_TYPE
{
    G4_BB_NONE_TYPE   = 0x00,
    G4_BB_CALL_TYPE   = 0x01,
    G4_BB_RETURN_TYPE = 0x02,
    G4_BB_INIT_TYPE   = 0x04,
    G4_BB_EXIT_TYPE   = 0x08,
    G4_BB_NM_WA_TYPE  = 0x10,  // For NoMaskWA
    G4_BB_FCALL_TYPE  = 0x20   // For NoMaskWA
};

class FuncInfo;
class FlowGraph;

class G4_BB
{
    //
    // basic block id
    //
    unsigned id;
    //
    // preorder block id
    //
    unsigned preId;
    //
    // reverse postorder block id
    //
    unsigned rpostId;
    //
    // traversal is for traversing control flow graph (to indicate the
    // block is visited)
    //
    unsigned traversal;

    //
    // if the current BB ends with a CALL subroutine, then the calleeInfo points
    // to the FuncInfo node corresponding to the called function.
    // else if the block is an INIT/EXIT block of a function, then the funcInfo
    // points to the FuncInfo node of its function.
    //
    union {
        FuncInfo * calleeInfo;
        FuncInfo * funcInfo;
    };

    //
    // the block classification
    //
    unsigned BBType;

    // indicates if the block is part of a natural loop or not
    bool inNaturalLoop;
    bool hasSendInBB;

    // indicate the nest level of the loop
    unsigned char loopNestLevel;

    // indicates the scoping info in call graph
    unsigned scopeID;

    // If a BB is divergent, this field is set to true. By divergent, it means
    // that among all active lanes on entry to shader/kernel, not all lanes are
    // active in this BB.
    bool divergent;

    bool Latency_Sched;

    // the physical pred/succ for this block (i.e., the pred/succ for this
    // block in the BB list).  Note that some transformations may rearrange
    // BB layout, so for safety it's best to recompute this
    G4_BB * physicalPred;
    G4_BB * physicalSucc;

    FlowGraph * parent;

    INST_LIST instList;

    INST_LIST_ITER insert(INST_LIST::iterator iter, G4_INST* inst)
    {
        return instList.insert(iter, inst);
    }
public:
    // forwarding functions to this BB's instList
    INST_LIST_ITER begin() { return instList.begin(); }
    INST_LIST_ITER end() { return instList.end(); }
    INST_LIST::reverse_iterator rbegin() { return instList.rbegin(); }
    INST_LIST::reverse_iterator rend() { return instList.rend(); }
    INST_LIST& getInstList() { return instList; }

    template <class InputIt>
    INST_LIST_ITER insert(INST_LIST::iterator iter, InputIt first, InputIt last)
    {
        return instList.insert(iter, first, last);
    }

    INST_LIST_ITER insertBefore(INST_LIST::iterator iter, G4_INST* inst,
        bool inheritDI = true)
    {
        if (inheritDI &&
            iter != instList.end() && !inst->isCISAOffValid())
            inst->inheritDIFrom(*iter);
        return instList.insert(iter, inst);
    }

    INST_LIST_ITER insertAfter(INST_LIST::iterator iter, G4_INST* inst,
        bool inheritDI = true)
    {
        auto next = iter;
        ++next;
        if (inheritDI &&
            !inst->isCISAOffValid())
        {
            // Inheriting from iter seems more reasonable
            // since invoking invokeAfter on iter means
            // we're processing iter and not ++iter
            inst->inheritDIFrom(*iter);
        }
        return instList.insert(next, inst);
    }

    INST_LIST_ITER erase(INST_LIST::iterator iter) {
        return instList.erase(iter);
    }
    INST_LIST_ITER erase(INST_LIST::iterator first, INST_LIST::iterator last) {
        return instList.erase(first, last);
    }
    void remove(G4_INST* inst) { instList.remove(inst); }
    void clear() { instList.clear(); }
    void pop_back() { instList.pop_back(); }
    void pop_front() { instList.pop_front(); }
    void push_back(G4_INST* inst, bool inheritDI = true) {insertBefore(instList.end(), inst, inheritDI);}
    void push_front(G4_INST* inst, bool inheritDI = true) {insertBefore(instList.begin(), inst, inheritDI);}

    size_t size() const { return instList.size(); }
    bool empty() const { return instList.empty(); }
    G4_INST* front() { return instList.front(); }
    const G4_INST* front() const { return instList.front(); }
    G4_INST* back() { return instList.back(); }
    const G4_INST* back() const { return instList.back(); }

    // splice functions below expect caller to have correctly set CISA offset
    // in instructions to be spliced. CISA offsets must be maintained to
    // preserve debug info links.
    void splice(INST_LIST::iterator pos, INST_LIST& other)
    {
        instList.splice(pos, other);
    }
    void splice(INST_LIST::iterator pos, G4_BB* otherBB)
    {
        instList.splice(pos, otherBB->getInstList());
    }
    void splice(INST_LIST::iterator pos, INST_LIST& other, INST_LIST::iterator it)
    {
        instList.splice(pos, other, it);
    }
    void splice(INST_LIST::iterator pos, G4_BB* otherBB, INST_LIST::iterator it)
    {
        instList.splice(pos, otherBB->getInstList(), it);
    }
    void splice(INST_LIST::iterator pos, INST_LIST& other,
        INST_LIST::iterator first, INST_LIST::iterator last)
    {
        instList.splice(pos, other, first, last);
    }
    void splice(INST_LIST::iterator pos, G4_BB* otherBB,
        INST_LIST::iterator first, INST_LIST::iterator last)
    {
        instList.splice(pos, otherBB->getInstList(), first, last);
    }

    //
    // Important invariant: fall-through BB must be at the front of Succs.
    // If we don't maintain this property, extra checking (e.g., label
    // comparison) is needed to retrieve fallThroughBB
    //
    BB_LIST    Preds;
    BB_LIST    Succs;

    G4_BB(INST_LIST_NODE_ALLOCATOR& alloc, unsigned i, FlowGraph* fg) :
        id(i), preId(0), rpostId(0),
        traversal(0), calleeInfo(NULL), BBType(G4_BB_NONE_TYPE),
        inNaturalLoop(false), hasSendInBB(false), loopNestLevel(0), scopeID(0),
        divergent(false), physicalPred(NULL), physicalSucc(NULL),
        parent(fg), instList(alloc), Latency_Sched(false)
    {
    }

    ~G4_BB() {instList.clear();}

    void *operator new(size_t sz, Mem_Manager& m)    {return m.alloc(sz);}

    FlowGraph& getParent() const { return *parent; }
    G4_Kernel& getKernel() const;

    bool       isLastInstEOT() const; // to check if the last instruction in list is EOT
    G4_opcode  getLastOpcode() const;

    unsigned   getId() const {return id;}
    void       setId(unsigned i);

    unsigned getPreId() const      {return preId;}
    void     setPreId(unsigned i)  {preId = i;}

    unsigned getRPostId() const    {return rpostId;}
    void     setRPostId(unsigned i) {rpostId = i;}

    void       markTraversed(unsigned num)      {traversal = num;}
    bool       isAlreadyTraversed(unsigned num) const {return traversal >= num;}

    void       removePredEdge(G4_BB* pred);
    void       removeSuccEdge(G4_BB* succ);

    void       writeBBId(std::ostream& os) const {os << "BB" << id;}
    G4_BB*     fallThroughBB();
    G4_BB*     BBBeforeCall() const;
    G4_BB*     BBAfterCall() const;

    FuncInfo*  getCalleeInfo() const          {return calleeInfo;}
    void       setCalleeInfo(FuncInfo* callee) {calleeInfo = callee;}
    FuncInfo*  getFuncInfo() const            {return funcInfo;}
    void       setFuncInfo(FuncInfo* func)    {funcInfo = func;}

    int        getBBType() const              {return BBType;}
    void       setBBType(int type)            {BBType |= type;}
    void       unsetBBType(G4_BB_TYPE type)   {BBType &= ~unsigned(type);}

    void       setInNaturalLoop(bool val)     {inNaturalLoop = val;}
    bool       isInNaturalLoop() const        {return inNaturalLoop;}

    bool       isSuccBB(G4_BB* succ) const; // return true if succ is in bb's Succss
    void       setSendInBB(bool val)          { hasSendInBB = val; }
    bool       isSendInBB() const             { return hasSendInBB; }

    void       setNestLevel()                 { loopNestLevel++;}
    unsigned   getNestLevel() const           { return loopNestLevel; }
    void       resetNestLevel()               { loopNestLevel = 0; }

    void       setDivergent(bool val) { divergent = val; }
    bool       isDivergent() const    { return divergent; }
    void       setLatencySched(bool val) { Latency_Sched = val; }
    bool       isLatencySched() const { return Latency_Sched; }
    bool       isAllLaneActive() const;

    unsigned   getScopeID() const { return scopeID; }
    void       setScopeID(unsigned id) { scopeID = id; }

    G4_BB * getPhysicalPred() const     { return physicalPred; }
    void    setPhysicalPred(G4_BB* pred)  { physicalPred = pred; }

    G4_BB * getPhysicalSucc() const     { return physicalSucc; }
    void    setPhysicalSucc(G4_BB* succ)  { physicalSucc = succ; }

    void emit(std::ostream& output);
    void emitInstruction(std::ostream& output, INST_LIST_ITER &it);
    void emitBasicInstruction(std::ostream& output, INST_LIST_ITER &it);
    void emitBasicInstructionComment(std::ostream& output, INST_LIST_ITER &it, int *suppressRegs, int *lastRegs);
    void emitInstructionSourceLineMapping(std::ostream& output, INST_LIST_ITER &it);
    void emitBankConflict(std::ostream& output, const G4_INST *inst);

    int getConflictTimesForTGL(std::ostream& output, int* firstRegCandidate, int& sameBankConflicts, bool zeroOne, bool isTGLLP, bool reducedBundles);

    uint32_t emitBankConflictXe(
        std::ostream& os, const G4_INST *inst,
        int* suppressRegs,
        int& sameConflictTimes, int& twoSrcConflicts, int& simd16RS,
        bool zeroOne, bool isTGLLP, bool hasReducedBundles);
    uint32_t emitBankConflictXeLP(
        std::ostream& os, const G4_INST *inst,
        int * suppressRegs, int * lastRegs,
        int & sameConflictTimes, int & twoSrcConflicts, int & simd16RS);
    uint32_t countReadModifyWrite(std::ostream& os, const G4_INST *inst);

    // SWSB_G4IR.cpp
    void emitRegInfo(std::ostream& os, G4_INST * inst, int offset);

    bool isEndWithCall() const { return getLastOpcode() == G4_call; }
    bool isEndWithFCall() const { return getLastOpcode() == G4_pseudo_fcall; }
    bool isEndWithFCCall() const { return getLastOpcode() == G4_pseudo_fc_call; }
    bool isEndWithFRet() const { return getLastOpcode() == G4_pseudo_fret; }
    bool isEndWithGoto() const { return getLastOpcode() == G4_goto; }

    G4_Label* getLabel();

    // Return the first non-label instruction if any.
    G4_INST* getFirstInst();

    // Return the first insert position if any; otherwise return end().
    INST_LIST_ITER getFirstInsertPos();

    void addEOTSend(G4_INST* lastInst = NULL);

    std::string getBBTypeStr() const;

    void print(std::ostream& os = std::cerr) const;
    /// Dump instructions into the standard error.
    void dump() const;  // used in debugger
    void dumpDefUse(std::ostream& os = std::cerr) const;

    void emitBbInfo(std::ostream& os) const;

    // reset this BB's instruction's local id so they are [0,..#BBInst-1]
    void resetLocalIds();

    void removeIntrinsics(Intrinsic intrinId);

    void addSamplerFlushBeforeEOT();

    bool dominates(G4_BB* other);
}; // class G4_BB
} // vISA::

#endif // G4_BB_H
