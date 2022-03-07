/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _INC_LINEARSCANRA_H_
#define _INC_LINEARSCANRA_H_

#include <list>
#include "G4_Opcode.h"
#include "FlowGraph.h"
#include "BuildIR.h"
#include "BitSet.h"
#include "SpillManagerGMRF.h"

// Forward decls
namespace vISA
{
class G4_Declare;
class G4_INST;
class G4_BB;
class globalLinearScan;
class LSLiveRange;
class PhyRegLocalRA;
class PhyRegsManager;
class PhyRegSummary;
class BankConflictPass;
class GlobalRA;
}

#define MAXIMAL_ITERATIONS 10
#define SCRATCH_MSG_LIMIT (128 * 1024)
vISA::G4_Declare* GetTopDclFromRegRegion(vISA::G4_Operand* opnd);

// Each declaration will have a LSLiveRange object allocated for it
namespace vISA
{
    class PhyRegsLocalRA;
    class LSInputLiveRange;

    class G4_BB_LS {
    private:
        G4_BB* bb;
        bool refInput;
        bool backEdgeIn;
        bool backEdgeOut;

    public:
        G4_BB_LS(G4_BB* block) :bb(block)
        {
            refInput = false;
            backEdgeIn = false;
            backEdgeOut = false;
        }

        ~G4_BB_LS()
        {

        }
        void* operator new(size_t sz, Mem_Manager& m) { return m.alloc(sz); }
        void     setBackEdgeIn(bool val) { backEdgeIn = val; }
        bool     hasBackEdgeIn() { return backEdgeIn; }
        void     setBackEdgeOut(bool val) { backEdgeOut = val; }
        bool     hasBackEdgeOut() { return backEdgeOut; }
        void     setRefInput(bool val) { refInput = val; }
        bool     hasRefInput() { return refInput; }
    };

    typedef std::vector<G4_BB_LS*> BB_LS_VECTOR;

    class LinearScanRA
    {
    private:
        G4_Kernel& kernel;
        IR_Builder& builder;
        LivenessAnalysis& l;
        BB_LS_VECTOR BBVector;
        PhyRegsLocalRA* pregs = nullptr;
        std::vector<LSLiveRange*> globalLiveIntervals;
        std::vector<LSLiveRange*> preAssignedLiveIntervals;
        std::vector<LSLiveRange*> liveThroughIntervals;
        std::map<G4_BB*, std::vector<G4_BB*>> loopHeadExitMap;
        unsigned int numRegLRA = 0;
        unsigned int numRowsEOT = 0;
        unsigned int globalLRSize = 0;
        bool doSplitLLR = false;
        Mem_Manager& mem;
        std::list<LSInputLiveRange*, std_arena_based_allocator<LSInputLiveRange*>> inputIntervals;
        BankConflictPass& bc;
        GlobalRA& gra;
        bool doBCR = false;
        bool highInternalConflict = false;
        bool hasSplitInsts = false;
        int regionID = -1;
        LSLiveRange* stackCallArgLR;
        LSLiveRange* stackCallRetLR;
        std::vector<G4_Declare *> globalDeclares;
        unsigned int funcCnt = 0;
        unsigned int lastInstLexID = 0;
        std::vector<unsigned int> funcLastLexID;

        LSLiveRange* GetOrCreateLocalLiveRange(G4_Declare* topdcl);
        LSLiveRange* CreateLocalLiveRange(G4_Declare* topdcl);
        void createLiveIntervals();
        void linearScanMarkReferencesInOpnd(G4_Operand* opnd, bool isEOT, bool isCall);
        void linearScanMarkReferencesInInst(INST_LIST_ITER inst_it);
        void linearScanMarkReferences(unsigned int& numRowsEOT);
        void markBackEdges();
        void getGlobalDeclares();
        void preRAAnalysis();
        void getCalleeSaveRegisters();
        void getCallerSaveRegisters();
        void getSaveRestoreRegister();
        void calculateFuncLastID();
        int linearScanRA();
        void removeUnrequiredLifetimeOps();
        void setLexicalID();
        bool hasDstSrcOverlapPotential(G4_DstRegRegion* dst, G4_SrcRegRegion* src);
        void setPreAssignedLR(LSLiveRange* lr, std::vector<LSLiveRange*>& preAssignedLiveIntervals);
        void setDstReferences(G4_BB* bb, INST_LIST_ITER inst_it, G4_Declare* dcl, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals);
        void setSrcReferences(G4_BB* bb, INST_LIST_ITER inst_it, int srcIdx, G4_Declare* dcl, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals);
        void generateInputIntervals(G4_Declare* topdcl, G4_INST* inst, std::vector<uint32_t>& inputRegLastRef, PhyRegsLocalRA& initPregs, bool avoidSameInstOverlap);
        void calculateInputIntervalsGlobal(PhyRegsLocalRA& initPregs);
        void calculateLiveInIntervals(G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals);
        void calculateCurrentBBLiveIntervals(G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals);
        void calculateLiveOutIntervals(G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals);
        void calculateLiveThroughIntervals();
        void calculateLiveIntervalsGlobal(G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals);
        void printLiveIntervals(std::vector<LSLiveRange*>& liveIntervals);
        void printSpillLiveIntervals(std::list<LSLiveRange*>& liveIntervals);
        void printInputLiveIntervalsGlobal();
        bool isUseUnAvailableRegister(uint32_t startReg, uint32_t regNum);
        bool assignEOTLiveRanges(IR_Builder& builder, std::vector<LSLiveRange*>& liveIntervals);

        // scratch fields used for parameter passing
        G4_BB* curBB_ = nullptr;
        uint32_t nextSpillOffset = 0;
        uint32_t scratchOffset = 0;

    public:
        static void getRowInfo(int size, int& nrows, int& lastRowSize, const IR_Builder& builder);
        static unsigned int convertSubRegOffFromWords(G4_Declare* dcl, int subregnuminwords);

        LinearScanRA(BankConflictPass&, GlobalRA&, LivenessAnalysis&);
        void allocForbiddenVector(LSLiveRange* lr);
        int doLinearScanRA();
        void undoLinearScanRAAssignments();
        bool hasHighInternalBC() const { return highInternalConflict; }
        uint32_t getSpillSize() { return nextSpillOffset; }
    };

class LSLiveRange
{
private:
    G4_Declare* topdcl;
    G4_INST* firstRef;
    G4_INST* lastRef;
    unsigned int lrStartIdx, lrEndIdx;
    bool pushed;
    G4_VarBase* preg;
    // pregoff is stored in word here
    // But subreg offset stored in regvar should be in units of dcl's element size
    int pregoff;

    unsigned int numRefsInFG;
    unsigned int numRefs;
    G4_BB* prevBBRef;

    bool* forbidden = nullptr;
    bool* retGRFs = nullptr;

    bool isIndirectAccess;
    bool eot;
    bool assigned;
    bool preAssigned;
    bool useUnAvailableReg;
    bool isActive;
    bool _isCall;
    bool _isCallSite;

    //std::unordered_set<unsigned int> forbiddenGRFs;
    //std::unordered_set<unsigned int> retGRFs;
public:
    LSLiveRange()
    {
        topdcl = NULL;
        firstRef = lastRef = NULL;
        lrStartIdx = lrEndIdx = 0;
        isIndirectAccess = false;
        numRefsInFG = 0;
        numRefs = 0;
        prevBBRef = NULL;
        preg = NULL;
        pregoff = 0;
        assigned = false;
        preAssigned = false;
        eot = false;
        useUnAvailableReg = false;
        pushed = false;
        isActive = false;
        _isCall = false;
        _isCallSite = false;
    }

    void setActiveLR(bool a) { isActive = a; }
    bool isActiveLR() { return isActive; }
    const bool* getForbidden() { return forbidden; }
    void setForbidden(bool* f) { forbidden = f; }
    void setRegGRFs(bool* f) { retGRFs = f; }

    void setUseUnAvailableReg(bool avail) { useUnAvailableReg = avail; }
    bool isUseUnAvailableReg() { return useUnAvailableReg; }
    void setPushed(bool p) { pushed = p; }
    bool isPushedToIntervalList() { return pushed; }

    // A reference to this live range exists in bb basic block, record it
    void markIndirectRef(bool indirectAccess) { isIndirectAccess = indirectAccess; }

    void recordRef(G4_BB* bb, bool fromEntry);
    unsigned int getNumRefs() const
    {
        return numRefs;
    }
    bool isGRFRegAssigned();

    void setTopDcl(G4_Declare* dcl)
    {
        MUST_BE_TRUE(topdcl == NULL, "Redefining top dcl");
        topdcl = dcl;
    }

    G4_Declare* getTopDcl() const { return topdcl; }

    void* operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    bool hasIndirectAccess() const { return isIndirectAccess; }

    void setFirstRef(G4_INST* inst, unsigned int idx)
    {
        if (!firstRef && lrStartIdx == 0)
        {
            firstRef = inst;
            lrStartIdx = idx;
            return;
        }

        if (idx < lrStartIdx)
        {
            firstRef = inst;
            lrStartIdx = idx;
        }
    }

    G4_INST* getFirstRef(unsigned int& idx)
    {
        idx = lrStartIdx;
        return firstRef;
    }

    void setLastRef(G4_INST* inst, unsigned int idx)
    {
        lastRef = inst;
        if (idx > lrEndIdx)
        {
            lrEndIdx = idx;
        }
    }

    G4_INST* getLastRef(unsigned int& idx) const
    {
        idx = lrEndIdx;
        return lastRef;
    }

    void setPhyReg(G4_VarBase* pr, int subreg) { preg = pr; pregoff = subreg; }
    G4_VarBase* getPhyReg(int& subreg) { subreg = pregoff; return preg; }
    void        resetPhyReg() { preg = NULL; pregoff = 0; }

    unsigned int getSizeInWords(const IR_Builder& builder);

    bool isLiveRangeGlobal() const;

    void setAssigned(bool a) { assigned = a; }
    bool getAssigned() { return assigned; }

    void setPreAssigned(bool a) { preAssigned = a; }
    bool getPreAssigned() { return preAssigned; }

    void setIsCall(bool a) { _isCall = a; }
    bool isCall() { return _isCall; }

    void setIsCallSite(bool a) { _isCallSite = a; }
    bool isCallSite() { return _isCallSite; }

    void markEOT() { eot = true; }
    bool isEOT() { return eot; }

    void addForbidden(unsigned int f) { /*forbiddenGRFs.insert(f);*/ forbidden[f] = true;}
    void addRetRegs(unsigned int f) { retGRFs[f] = true; }
    const bool* getRetGRFs() { return retGRFs; }
    void clearForbiddenGRF(unsigned GRFSize)
    {
        if (retGRFs)
        {
            memset(retGRFs, false, GRFSize);
        }
        memset(forbidden, false, GRFSize);
    }
};

class LSInputLiveRange
{
private:
    unsigned int regWordIdx;
    unsigned int lrEndIdx;

public:
    LSInputLiveRange(unsigned int regId, unsigned int endId) : regWordIdx(regId), lrEndIdx(endId)
    {

    }

    void* operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    unsigned int getRegWordIdx() { return regWordIdx; }
    unsigned int getLrEndIdx() { return lrEndIdx; }
    void setLrEndIdx(unsigned int idx)  { lrEndIdx = idx; }
};

}

namespace vISA
{
    typedef struct _ACTIVE_GRFS
    {
        std::vector<LSLiveRange*> activeLV;
        std::vector<LSInputLiveRange*> activeInput;
    } ACTIVE_GRFS;

    typedef std::map<vISA::G4_Declare*, std::pair<vISA::G4_INST*, FuncInfo*>> CALL_DECL_MAP;
    typedef std::map<vISA::G4_Declare*, std::pair<vISA::G4_INST*, FuncInfo*>>::iterator CALL_DECL_MAP_ITER;

    class globalLinearScan {
    private:
        GlobalRA& gra;
        IR_Builder& builder;
        Mem_Manager& mem;
        PhyRegsManager& pregManager;
        std::vector<LSLiveRange*>& liveIntervals;
        std::vector<LSLiveRange*>* preAssignedIntervals;
        std::list<LSInputLiveRange*, std_arena_based_allocator<LSInputLiveRange*>>& inputIntervals;
        std::list<LSLiveRange*> active;
        std::vector<ACTIVE_GRFS> activeGRF;
        LSLiveRange* calleeSaveLR = nullptr;
        LivenessAnalysis* liveAnalysis = nullptr;

        void printActives();
        void expireGlobalRanges(unsigned int idx);
        void expireInputRanges(unsigned int global_idx);
        BankAlign getBankAlign(LSLiveRange* lr);
        bool allocateRegsLinearScan(LSLiveRange* lr, IR_Builder& builder);
        void allocRetRegsVector(LSLiveRange* lr);
        void freeAllocedRegs(LSLiveRange*, bool);
        void updateGlobalActiveList(LSLiveRange* lr);
        bool insertLiveRange(std::list<LSLiveRange*>* liveIntervals, LSLiveRange* lr);
        bool canBeSpilledLR(LSLiveRange* lr);
        int findSpillCandidate(LSLiveRange* tlr);
        void freeSelectedRegistsers(int startGRF, LSLiveRange* tlr, std::list<LSLiveRange*>& spillLRs);
        bool spillFromActiveList(LSLiveRange* tlr, std::list<LSLiveRange*>& spillLRs);

        unsigned int startGRFReg = 0;
        unsigned int numRegLRA = 0;
        unsigned int numRowsEOT = 0;
        unsigned int lastLexicalID = 0;

        bool doBankConflict = false;
        bool highInternalConflict = false;

    public:
        globalLinearScan(GlobalRA& g, LivenessAnalysis* l, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>* eotLiveIntervals,
            std::list<LSInputLiveRange*, std_arena_based_allocator<LSInputLiveRange*>>& inputLivelIntervals,
            PhyRegsManager& pregMgr, Mem_Manager& memmgr, //CALL_DECL_MAP & callMap,
            unsigned int numReg, unsigned int numEOT, unsigned int lastLexID, bool bankConflict,
            bool internalConflict);

        void getCalleeSaveGRF(std::vector<unsigned int>& regNum, G4_Kernel* kernel);

        void getCallerSaveGRF(std::vector<unsigned int>& regNum, std::vector<unsigned int>& regRegNum, G4_Kernel* kernel);

        void updateCallSiteLiveIntervals(LSLiveRange* callSiteLR);

        bool runLinearScan(IR_Builder& builder, std::vector<LSLiveRange*>& liveIntervals, std::list<LSLiveRange*>& spillLRs);
        void expireAllActive();
    };
}
#endif // _INC_LINEARSCANRA_H_
