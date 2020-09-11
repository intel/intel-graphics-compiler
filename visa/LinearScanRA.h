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

vISA::G4_Declare* GetTopDclFromRegRegion(vISA::G4_Operand* opnd);

typedef std::vector<std::tuple<G4_BB*, INST_LIST_ITER, Gen4_Operand_Number>> OPND_REFS;
typedef std::vector<std::tuple<G4_BB*, INST_LIST_ITER, Gen4_Operand_Number>>::iterator OPND_REFS_ITER;
typedef std::map<vISA::LSLiveRange*, OPND_REFS> LLR_REF_MAP;
typedef std::map<vISA::LSLiveRange*, OPND_REFS>::iterator LLR_REF_MAP_ITER;

// Each declaration will have a LSLiveRange object allocated for it
namespace vISA
{
    class PhyRegsLocalRA;
    class LSInputLiveRange;

    class LinearScanRA
    {
    private:
        G4_Kernel& kernel;
        IR_Builder& builder;
        PhyRegsLocalRA* pregs = nullptr;
        LLR_REF_MAP LLRRefMap;
        std::vector<LSLiveRange*> globalLiveIntervals;
        std::vector<LSLiveRange*> preAssignedLiveIntervals;
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
        std::list<LSLiveRange> localLiveRanges;
        int regionID = -1;

        LSLiveRange* GetOrCreateLocalLiveRange(G4_Declare* topdcl);
        void linearScanMarkReferencesInOpnd(G4_Operand* opnd, bool isEOT, INST_LIST_ITER inst_it, unsigned int pos);
        void linearScanMarkReferencesInInst(INST_LIST_ITER inst_it);
        void linearScanMarkReferences(unsigned int& numRowsEOT);
        void preRAAnalysis();
        void saveRegs(unsigned startReg, unsigned owordSize, G4_Declare* scratchRegDcl, G4_Declare* framePtr, unsigned frameOwordOffset, G4_BB* bb, INST_LIST_ITER insertIt);
        void saveActiveRegs(std::vector<bool>& saveRegs, unsigned startReg, unsigned frameOffset, G4_BB* bb, INST_LIST_ITER insertIt);
        void restoreActiveRegs(std::vector<bool>& restoreRegs, unsigned startReg, unsigned frameOffset, G4_BB* bb, INST_LIST_ITER insertIt);
        void restoreRegs(unsigned startReg, unsigned owordSize, G4_Declare* scratchRegDcl, G4_Declare* framePtr, unsigned frameOwordOffset, G4_BB* bb, INST_LIST_ITER insertIt);
        void OptimizeActiveRegsFootprint(std::vector<bool>& saveRegs);
        void OptimizeActiveRegsFootprint(std::vector<bool>& saveRegs, std::vector<bool>& retRegs);
        void addCallerSaveRestoreCode();
        void addGenxMainStackSetupCode();
        void addCalleeStackSetupCode();
        void stackCallProlog();
        void addCalleeSaveRestoreCode();
        void addSaveRestoreCode(unsigned localSpillAreaOwordSize);
        bool linearScanRA();
        void blockOutputPhyRegs();
        void removeUnrequiredLifetimeOps();
        void setLexicalID(bool includePseudo);
        bool hasDstSrcOverlapPotential(G4_DstRegRegion* dst, G4_SrcRegRegion* src);
        void addLiveRangeRef(LLR_REF_MAP& LLRRefMap, G4_BB* bb, LSLiveRange* lr, INST_LIST_ITER inst_it, Gen4_Operand_Number opnd_num);
        void setPreAssignedLR(LSLiveRange* lr, std::vector<LSLiveRange*>& preAssignedLiveIntervals);
        void setDstReferences(G4_BB* bb, INST_LIST_ITER inst_it, G4_Declare* dcl, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals);
        void setSrcReferences(G4_BB* bb, INST_LIST_ITER inst_it, int srcIdx, G4_Declare* dcl, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals);
        void generateInputIntervals(G4_Declare* topdcl, G4_INST* inst, std::vector<uint32_t>& inputRegLastRef, PhyRegsLocalRA& initPregs, bool avoidSameInstOverlap);
        void calculateInputIntervalsGlobal(LivenessAnalysis* l, PhyRegsLocalRA& initPregs, std::list<vISA::G4_BB*>& bbList);
        void calculateLiveInIntervals(LivenessAnalysis* l, G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals);
        void calculateCurrentBBLiveIntervals(LivenessAnalysis* l, G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals);
        void calculateLiveOutIntervals(LivenessAnalysis* l, G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals);
        void calculateLiveIntervalsGlobal(LivenessAnalysis* l, G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals);
        void printLiveIntervals(std::vector<LSLiveRange*>& liveIntervals);
        void printInputLiveIntervalsGlobal();
        bool isUseUnAvailableRegister(uint32_t startReg, uint32_t regNum);
        bool assignEOTLiveRanges(IR_Builder& builder, std::vector<LSLiveRange*>& liveIntervals);
        void spillLiveRange(LSLiveRange* lr, SpillManagerGRF* spillGRF);
        void spillLiveRanges(std::list<LSLiveRange*>& spillLRs, SpillManagerGRF* spillGRF);

        // scratch fields used for parameter passing
        G4_BB* curBB = nullptr;
        int globalIndex = 0;

    public:
        static void getRowInfo(int size, int& nrows, int& lastRowSize);
        static unsigned int convertSubRegOffFromWords(G4_Declare* dcl, int subregnuminwords);

        LinearScanRA(BankConflictPass&, GlobalRA&);
        bool doLinearScanRA();
        void undoLinearScanRAAssignments();
        bool hasHighInternalBC() const { return highInternalConflict; }
    };

class LSLiveRange
{
private:
    G4_Declare* topdcl;
    G4_INST* firstRef;
    G4_INST* lastRef;
    unsigned int lrStartIdx, lrEndIdx;
    int regionID;
    bool isIndirectAccess;
    G4_VarBase* preg;
    // pregoff is stored in word here
    // But subreg offset stored in regvar should be in units of dcl's element size
    int pregoff;

    unsigned int numRefsInFG;
    G4_BB* prevBBRef;
    bool eot;

    bool assigned;
    bool preAssigned;
    bool useUnAvailableReg;

    IR_Builder& builder;

    std::unordered_set<unsigned int> forbiddenGRFs;
    std::unordered_set<unsigned int> retGRFs;
    const static unsigned int UndefHint = 0xffffffff;
    unsigned int hint = UndefHint;
public:
    OPND_REFS opndRef;
    LSLiveRange(IR_Builder& b) : builder(b)
    {
        topdcl = NULL;
        firstRef = lastRef = NULL;
        lrStartIdx = lrEndIdx = 0;
        isIndirectAccess = false;
        numRefsInFG = 0;
        prevBBRef = NULL;
        preg = NULL;
        pregoff = 0;
        assigned = false;
        preAssigned = false;
        eot = false;
        useUnAvailableReg = false;
        regionID = -1;
    }

    void setUseUnAvailableReg(bool avail) { useUnAvailableReg = avail; }
    bool isUseUnAvailableReg() { return useUnAvailableReg; }
    void setRegionID(int id) { regionID = id; }
    int getRegionID() { return regionID; }
    // A reference to this live range exists in bb basic block, record it
    void recordRef(G4_BB*);

    void markIndirectRef() { isIndirectAccess = true; }

    bool isGRFRegAssigned();

    void setTopDcl(G4_Declare* dcl)
    {
        MUST_BE_TRUE(topdcl == NULL, "Redefining top dcl");
        topdcl = dcl;
    }

    G4_Declare* getTopDcl() { return topdcl; }

    void* operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    bool hasIndirectAccess() { return isIndirectAccess; }

    void setFirstRef(G4_INST* inst, unsigned int idx)
    {
        firstRef = inst;
        lrStartIdx = idx;
    }

    G4_INST* getFirstRef(unsigned int& idx)
    {
        idx = lrStartIdx;
        return firstRef;
    }

    void setLastRef(G4_INST* inst, unsigned int idx)
    {
        lastRef = inst;
        lrEndIdx = idx;
    }

    G4_INST* getLastRef(unsigned int& idx)
    {
        idx = lrEndIdx;
        return lastRef;
    }

    void setPhyReg(G4_VarBase* pr, int subreg) { preg = pr; pregoff = subreg; }
    G4_VarBase* getPhyReg(int& subreg) { subreg = pregoff; return preg; }
    void        resetPhyReg() { preg = NULL; pregoff = 0; }

    unsigned int getSizeInWords();

    void setAssigned(bool a) { assigned = a; }
    bool getAssigned() { return assigned; }

    void setPreAssigned(bool a) { preAssigned = a; }
    bool getPreAssigned() { return preAssigned; }

    void markEOT() { eot = true; }
    bool isEOT() { return eot; }

    void addForbidden(unsigned int f) { forbiddenGRFs.insert(f); }
    void addRetRegs(unsigned int f) { retGRFs.insert(f); }
    std::unordered_set<unsigned int>& getForbidden() { return forbiddenGRFs; }
    std::unordered_set<unsigned int>& getRetGRFs() { return retGRFs; }
    void clearForbidden() { forbiddenGRFs.clear(); }

    void setHint(unsigned int h) { hint = h; }
    bool hasHint() { return hint != UndefHint; }
    unsigned int getHint() { return hint; }
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
};
}

namespace vISA
{
    typedef struct _ACTIVE_GRFS
    {
        std::list<LSLiveRange*> activeLV;
        std::list<LSInputLiveRange*> activeInput;
    } ACTIVE_GRFS;

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
        LivenessAnalysis* liveAnalysis = nullptr;

        void expireRanges(unsigned int);
        void printActives();
        void expireGlobalRanges(unsigned int idx);
        void expireInputRanges(unsigned int global_idx);
        unsigned short getOccupiedBundle(G4_Declare* dcl);
        BankAlign getBankAlign(LSLiveRange* lr);
        bool allocateRegsLinearScan(LSLiveRange* lr, IR_Builder& builder);
        void freeAllocedRegs(LSLiveRange*, bool);
        void updateGlobalActiveList(LSLiveRange* lr);
        bool insertLiveRange(std::list<LSLiveRange*>* liveIntervals, LSLiveRange* lr);
        bool shouldSpillRegisterLinearScan(G4_Declare* dcl) const;
        bool canBeSpilledLR(LSLiveRange* tlr, LSLiveRange* lr, int GRFNum);
        int findSpillCandidate(LSLiveRange* tlr);
        void freeSelectedRegistsers(int startGRF, LSLiveRange* tlr, std::list<LSLiveRange*>& spillLRs);
        bool spillFromActiveList(LSLiveRange* tlr, std::list<LSLiveRange*>& spillLRs);

        unsigned int globalLRSize;
        unsigned int startGRFReg;
        unsigned int numRegLRA;
        unsigned int numRowsEOT;
        unsigned int lastLexicalID;

        bool doBankConflict;
        bool highInternalConflict;

    public:
        globalLinearScan(GlobalRA& g, LivenessAnalysis* l, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>* eotLiveIntervals,
            std::list<LSInputLiveRange*, std_arena_based_allocator<LSInputLiveRange*>>& inputLivelIntervals,
            PhyRegsManager& pregMgr, Mem_Manager& memmgr,
            unsigned int numReg, unsigned int numEOT, unsigned int lastLexID, bool bankConflict,
            bool internalConflict);

        void getCalleeSaveGRF(vector<unsigned int>& regNum, G4_Kernel* kernel);

        void getCallerSaveGRF(vector<unsigned int>& regNum, vector<unsigned int>& regRegNum, G4_Kernel* kernel);

        bool runLinearScan(IR_Builder& builder, LLR_REF_MAP& LLRRefMap, std::vector<LSLiveRange*>& liveIntervals, std::list<LSLiveRange*>& spillLRs);
        void expireAllActive();
    };
}
#endif // _INC_LINEARSCANRA_H_
