/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _INC_LOCALRA_H_
#define _INC_LOCALRA_H_

#include <list>
#include "G4_Opcode.h"
#include "FlowGraph.h"
#include "BuildIR.h"
#include "BitSet.h"

// Forward decls
namespace vISA
{
class G4_Declare;
class G4_INST;
class G4_BB;
class LinearScan;
class LocalLiveRange;
class PhyRegLocalRA;
class PhyRegsManager;
class PhyRegSummary;
class BankConflictPass;
class GlobalRA;
}

vISA::G4_Declare* GetTopDclFromRegRegion(vISA::G4_Operand* opnd);

typedef std::map<vISA::LocalLiveRange*, std::vector<std::pair<INST_LIST_ITER, unsigned int>>> LLR_USE_MAP;
typedef std::map<vISA::LocalLiveRange*, std::vector<std::pair<INST_LIST_ITER, unsigned int>>>::iterator LLR_USE_MAP_ITER;

// Each declaration will have a LocalLiveRange object allocated for it
namespace vISA
{
    class PhyRegsLocalRA;
    class InputLiveRange;

    class LocalRA
    {
    private:
        G4_Kernel& kernel;
        IR_Builder& builder;
        PhyRegsLocalRA* pregs = nullptr;
        LLR_USE_MAP LLRUseMap;
        unsigned int numRegLRA = 0;
        unsigned int globalLRSize = 0;
        bool doSplitLLR = false;
        Mem_Manager& mem;
        std::list<InputLiveRange*, std_arena_based_allocator<InputLiveRange*>> inputIntervals;
        BankConflictPass& bc;
        GlobalRA& gra;
        bool doBCR = false;
        bool highInternalConflict = false;
        bool hasSplitInsts = false;

        BankAlign getBankAlignForUniqueAssign(G4_Declare *dcl);
        bool hasBackEdge();
        void evenAlign();
        void preLocalRAAnalysis();
        void trivialAssignRA(bool& needGlobalRA, bool threeSourceCandidate);
        bool localRAPass(bool doRoundRobin, bool doSplitLLR);
        void resetMasks();
        void blockOutputPhyRegs();
        void removeUnrequiredLifetimeOps();
        bool assignUniqueRegisters(bool twoBanksRA, bool twoDirectionsAssign, bool hybridWithSpill);
        bool unassignedRangeFound();
        void updateRegUsage(PhyRegSummary* summary, unsigned int& numRegsUsed);
        void localRAOptReport();
        void markReferencesInOpnd(G4_Operand* opnd, bool isEOT, INST_LIST_ITER inst_it,
            unsigned int pos);
        void markReferencesInInst(INST_LIST_ITER inst_it);
        void setLexicalID(bool includePseudo);
        void markReferences(unsigned int& numRowsEOT, bool& lifetimeOpFound);
        void calculateInputIntervals();
        bool hasDstSrcOverlapPotential(G4_DstRegRegion* dst, G4_SrcRegRegion* src);
        void calculateLiveIntervals(G4_BB* bb, std::vector<LocalLiveRange*>& liveIntervals);
        void printAddressTakenDecls();
        void printLocalRACandidates();
        void printLocalLiveIntervals(G4_BB* bb, std::vector<LocalLiveRange*>& liveIntervals);
        void printInputLiveIntervals();
        bool countLiveIntervals();

        // scratch fields used for parameter passing
        G4_BB* curBB = nullptr;

    public:
        static void getRowInfo(int size, int& nrows, int& lastRowSize, const IR_Builder& builder);
        static unsigned int convertSubRegOffFromWords(G4_Declare* dcl, int subregnuminwords);
        static unsigned int convertSubRegOffToWords(G4_Declare* dcl, int subregnum);
        static void countLocalLiveIntervals(std::vector<LocalLiveRange*>& liveIntervals, unsigned grfSize);
        static void printLocalLiveIntervalDistribution(unsigned int numScalars,
            unsigned int numHalfGRF, unsigned int numOneGRF, unsigned int numTwoOrMoreGRF,
            unsigned int numTotal);

        LocalRA(BankConflictPass&, GlobalRA&);
        bool localRA();
        void undoLocalRAAssignments(bool clearInterval);
        bool doHybridBCR() const { return doBCR; }
        bool hasHighInternalBC() const { return highInternalConflict; }
    };

class LocalLiveRange
{
private:
    G4_Declare* topdcl;
    G4_INST* firstRef;
    G4_INST* lastRef;
    unsigned int lrStartIdx, lrEndIdx;
    bool isIndirectAccess;
    G4_VarBase* preg;
    // pregoff is stored in word here
    // But subreg offset stored in regvar should be in units of dcl's element size
    int pregoff;

    unsigned int numRefsInFG;
    G4_BB* prevBBRef;
    bool eot;

    bool assigned;
    bool isSplit;

    IR_Builder& builder;

    std::unordered_set<unsigned int> forbiddenGRFs;
    const static unsigned int UndefHint = 0xffffffff;
    unsigned int hint = UndefHint;

public:
    LocalLiveRange(IR_Builder& b) : builder(b)
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
        eot = false;
        isSplit = false;
    }

    // A reference to this live range exists in bb basic block, record it
    void recordRef(G4_BB*);

    void markIndirectRef() { isIndirectAccess = true; }

    // A live range is local if it is never accessed indirectly (via address taken register) and
    // only a single basic block references the range
    bool isLiveRangeLocal() const;

    bool isLiveRangeGlobal();

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
        firstRef = inst;
        lrStartIdx = idx;
    }

    G4_INST* getFirstRef(unsigned int& idx) const
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

    unsigned int getSizeInWords();

    void setAssigned(bool a) { assigned = a; }
    bool getAssigned() const { return assigned; }

    void markEOT() { eot = true; }
    bool isEOT() const { return eot; }

    void markSplit() { isSplit = true; }
    bool getSplit() const { return isSplit; }

    void addForbidden(unsigned int f) { forbiddenGRFs.insert(f); }
    std::unordered_set<unsigned int>& getForbidden() { return forbiddenGRFs; }

    void setHint(unsigned int h) { hint = h; }
    bool hasHint() const { return hint != UndefHint; }
    unsigned int getHint() const { return hint; }
};

class InputLiveRange
{
private:
    unsigned int regWordIdx;
    unsigned int lrEndIdx;

public:
    InputLiveRange(unsigned int regId, unsigned int endId) : regWordIdx(regId), lrEndIdx(endId)
    {

    }

    void* operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    unsigned int getRegWordIdx() { return regWordIdx; }
    unsigned int getLrEndIdx() { return lrEndIdx; }
};
}
#define SECOND_HALF_BANK_START_GRF 64
#define RR_HEURISTIC  0.66
enum
{
    WORD_FREE = 0,
    WORD_BUSY = 1,
};

namespace vISA
{
class PhyRegsLocalRA
{
private:
    const IR_Builder& builder;
    unsigned int numRegs;
    // nth bit represents whether the register's nth word is free/busy
    // 1 - busy, 0 - free
    // It is possible to use bit-vector in place of this array
    // bitvector does not provide coarse grained access to mark
    // entire grf busy/available
    std::vector<uint32_t> regBusyVector;
    std::vector<int32_t> regLastUse;
    std::vector<bool> grfAvialable;
    int lastUseSum1;
    int lastUseSum2;
    int bank1AvailableRegNum;
    int bank2AvailableRegNum;

    bool twoBanksRA;
    bool simpleGRFAvailable;
    bool r0Forbidden;
    bool r1Forbidden;

    int LraFFWindowSize;

public:
    PhyRegsLocalRA(const IR_Builder& _builder, uint32_t nregs) : builder(_builder), numRegs(nregs)
    {
        uint32_t grfFree = 0;

        regBusyVector.resize(numRegs);
        regLastUse.resize(numRegs);
        grfAvialable.resize(numRegs);

        for (int i = 0; i < (int) nregs; i++)
        {
            regBusyVector[i] = grfFree;
            regLastUse[i] = 0;
            grfAvialable[i] = true;
        }

        lastUseSum1 = 0;
        lastUseSum2 = 0;
        bank1AvailableRegNum = 0;
        bank2AvailableRegNum = 0;

        twoBanksRA = false;
        simpleGRFAvailable = false;
        r0Forbidden = false;
        r1Forbidden = false;
       LraFFWindowSize = (int)builder.getOptions()->getuInt32Option(vISA_LraFFWindowSize);
    }

    void* operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    void findRegisterCandiateWithAlignForward(int& i, BankAlign align, bool evenAlign);

    unsigned int get_bundle(unsigned int baseReg, int offset);

    int findBundleConflictFreeRegister(int curReg, int endReg, unsigned short occupiedBundles, BankAlign align, bool evenAlign);

    void findRegisterCandiateWithAlignBackward(int& i, BankAlign align, bool evenAlign);

    void setGRFBusy(int which);
    void setGRFBusy(int which, bool isInput);
    void setGRFBusy(int which, int howmany, bool isInput);
    void setGRFBusy(int which, int howmany);
    void setWordBusy(int whichgrf, int word, bool isInput);
    void setGRFNotBusy(int which, int instID);
    void setH1GRFBusy(int which);
    void setH2GRFBusy(int which);
    void setWordBusy(int whichgrf, int word);
    void setWordBusy(int whichgrf, int word, int howmany, bool isInput);
    void setWordBusy(int whichgrf, int word, int howmany);
    void setWordNotBusy(int whichgrf, int word, int instID);

    inline bool isGRFBusy(int which) const
    {
        MUST_BE_TRUE(isGRFAvailable(which), "Invalid register");
        return (regBusyVector[which] != 0);
    }

    inline bool isGRFBusy(int which, int howmany) const
    {
        bool retval = false;

        for (int i = 0; i < howmany && !retval; i++)
        {
            retval |= isGRFBusy(which + i);
        }

        return retval;
    }

    bool isH1GRFBusy(int which);
    bool isH2GRFBusy(int which);
    inline bool isWordBusy(int whichgrf, int word);
    bool isWordBusy(int whichgrf, int word, int howmany);

    bool findFreeMultipleRegsForward(int regIdx, BankAlign align, int & regnum, int nrows, int lastRowSize, int endReg, unsigned short occupiedBundles,
        int instID, bool isHybridAlloc, std::unordered_set<unsigned int>& forbidden, bool hintSet);

    void markPhyRegs(G4_Declare* topdcl);

    // Available/unavailable is different from busy/free
    // Unavailable GRFs are not available for allocation
    void setGRFUnavailable(int which) { grfAvialable[which] = false; }
    bool isGRFAvailable(int which) const
    {

        if (simpleGRFAvailable)
        {
            if (which > 1)
            {
                return true;
            }
            else
            {
                if (r0Forbidden && which == 0)
                {
                    return false;
                }

                if (r1Forbidden && which <= 1)
                {
                    return false;
                }
                return true;
            }
        }
        else
        {
            MUST_BE_TRUE(which < (int) numRegs, "invalid GRF");
            return (grfAvialable[which] == true);
        }
    }

    bool isGRFAvailable(int which, int howmany) const
    {
        if (simpleGRFAvailable)
        {
            if (which > 1)
            {
                return true;
            }
            else
            {
                if (r0Forbidden && which == 0)
                {
                    return false;
                }

                if (r1Forbidden && which <= 1)
                {
                    return false;
                }
                return true;
            }
        }
        else
        {
            for (int i = 0; i < howmany; i++)
            {
                if (!isGRFAvailable(which + i))
                {
                    return false;
                }
            }
        }

        return true;
    }

    void printBusyRegs();
    int getRegLastUse(int reg) {return regLastUse[reg];}

    int getLastUseSum1() {return lastUseSum1;}
    int getLastUseSum2() {return lastUseSum2;}
    int getBank1AvailableRegNum() {return bank1AvailableRegNum;}
    int getBank2AvailableRegNum() {return bank2AvailableRegNum;}

    void setBank1AvailableRegNum(int avail1) { bank1AvailableRegNum = avail1;}
    void setBank2AvailableRegNum(int avail2) { bank2AvailableRegNum = avail2;}

    void setTwoBanksRA(bool twoBanks) { twoBanksRA = twoBanks;}
    void setSimpleGRFAvailable(bool simple) {simpleGRFAvailable = simple; }
    void setR0Forbidden() {r0Forbidden = true;}
    void setR1Forbidden() {r1Forbidden = true;}
    bool findFreeMultipleRegsBackward(int regIdx, BankAlign align, int &regnum, int nrows, int lastRowSize, int endReg, int instID,
        bool isHybridAlloc, std::unordered_set<unsigned int>& forbidden);
    bool findFreeSingleReg(int regIdx, G4_SubReg_Align subalign, int &regnum, int &subregnum, int size);
    bool findFreeSingleReg(int regIdx, int size, BankAlign align, G4_SubReg_Align subalign, int &regnum, int &subregnum, int endReg,
        int instID, bool isHybridAlloc, bool forward, std::unordered_set<unsigned int>& forbidden);

    bool findFreeMultipleRegsForward(int regIdx, BankAlign align, int& regnum, int nrows, int lastRowSize, int endReg, int instID, const bool* forbidden);

    bool findFreeSingleReg(int regIdx, int size, BankAlign align, G4_SubReg_Align subalign, int& regnum, int& subregnum, int endReg, const bool* forbidden);

};

class PhyRegsManager
{
private:
    const IR_Builder& builder;
    PhyRegsLocalRA availableRegs;
    bool twoBanksRA;

public:
    PhyRegsManager(const IR_Builder& irb, PhyRegsLocalRA pregs, bool _twoBanksRA) : builder(irb), availableRegs(pregs), twoBanksRA(_twoBanksRA)
    {
        availableRegs.setTwoBanksRA(_twoBanksRA);
    }

    int findFreeRegs(int size, BankAlign align, G4_SubReg_Align subalign, int & regnum, int & subregnum, int startRegNum, int endRegNum,
        unsigned short occupiedBundles, unsigned int instID, bool isHybridAlloc, std::unordered_set<unsigned int>& forbidden, bool hasHint,
        unsigned int hintReg);

    void freeRegs(int regnum, int subregnum, int numwords, int instID);
    PhyRegsLocalRA * getAvailableRegs() { return &availableRegs; }
    int findFreeRegs(int size, BankAlign align, G4_SubReg_Align subalign, int& regnum, int& subregnum, int startRegNum, int endRegNum, unsigned short occupiedBundles, unsigned int instID, bool isHybridAlloc, std::unordered_set<unsigned int>& forbidden);
    int findFreeRegs(int size, BankAlign align, G4_SubReg_Align subalign, int& regnum, int& subregnum, int startRegNum, int endRegNum, unsigned int instID, const bool* forbidden);
};

class LinearScan {
private:
    GlobalRA& gra;
    IR_Builder& builder;
    Mem_Manager& mem;
    PhyRegsManager& pregManager;
    PhyRegsLocalRA& initPregs;
    std::vector<LocalLiveRange*>& liveIntervals;
    std::list<InputLiveRange*, std_arena_based_allocator<InputLiveRange*>>& inputIntervals;
    std::list<LocalLiveRange*> active;
    PhyRegSummary* summary;

    void expireRanges(unsigned int);
    void expireInputRanges(unsigned int, unsigned int, unsigned int);
    void expireAllActive();
    bool allocateRegsFromBanks(LocalLiveRange*);
    bool allocateRegs(LocalLiveRange* lr, G4_BB* bb, IR_Builder& builder, LLR_USE_MAP& LLRUseMap);
    void coalesceSplit(LocalLiveRange* lr);
    void freeAllocedRegs(LocalLiveRange*, bool);
    void updateActiveList(LocalLiveRange*);

    BitSet pregs;
    unsigned int simdSize;

    unsigned int globalLRSize;
    unsigned int* startGRFReg;
    unsigned int numRegLRA;

    unsigned int bank1StartGRFReg;
    unsigned int bank2StartGRFReg;
    unsigned int bank1_start;
    unsigned int bank1_end;
    unsigned int bank2_start;
    unsigned int bank2_end;

    bool useRoundRobin;
    bool doBankConflict;
    bool highInternalConflict;
    bool doSplitLLR;

public:
    LinearScan(GlobalRA& g, std::vector<LocalLiveRange*>& localLiveIntervals,
        std::list<InputLiveRange*, std_arena_based_allocator<InputLiveRange*>>& inputLivelIntervals,
        PhyRegsManager& pregMgr, PhyRegsLocalRA& pregs, Mem_Manager& memmgr, PhyRegSummary* s,
        unsigned int numReg, unsigned int glrs, bool roundRobin, bool bankConflict,
        bool internalConflict, bool splitLLR, unsigned int simdS);

    void run(G4_BB* bb, IR_Builder& builder, LLR_USE_MAP& LLRUseMap);
};

class PhyRegSummary
{
private:
    const IR_Builder* builder = nullptr;
    uint32_t totalNumGRF = 0;
    std::vector<bool> GRFUsage;

public:
    PhyRegSummary() {}

    PhyRegSummary(const IR_Builder* irb, uint32_t numGRF) : builder(irb), totalNumGRF(numGRF)
    {
        GRFUsage.resize(totalNumGRF, false);
    }

    uint32_t getNumGRF() const { return totalNumGRF; }

    void* operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    void markPhyRegs(G4_VarBase* pr, unsigned int words);

    bool isGRFBusy(int regnum) const { return GRFUsage[regnum]; }

    void setGRFBusy(int regnum) { GRFUsage[regnum] = true; }

    void printBusyRegs();

    uint32_t getNumFreeGRF() const
    {
        uint32_t numFreeGRF = 0;
        for (int i = 0, size = getNumGRF(); i < size; ++i)
        {
            if (!isGRFBusy(i))
            {
                numFreeGRF++;
            }
        }
        return numFreeGRF;
    }
};
}
#endif // _INC_LOCALRA_H_
