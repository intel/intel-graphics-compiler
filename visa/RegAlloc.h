/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _REGALLOC_H_
#define _REGALLOC_H_
#include "PhyRegUsage.h"
#include <memory>
#include <vector>

#include "BitSet.h"
#include "LocalRA.h"
#include "LinearScanRA.h"

//
// Indirect tracking type
//
enum G4_IndrTrackType
{
    Track_IndrAddr = 0,        // Track indirect addressing
    Track_MsgGateWay = 1    // Track message gateway
};

//
// Structure for Indirect Accessing Tracking Parameter
//
struct IndrAccessTrackPara
{
    vISA::G4_BB * bb;            // starting bb of the indirect accessing tracking
    vISA::G4_Operand * ptr;    // a0 pointer for indirect accessing
    vISA::G4_RegVar * addrTaken;    // a0 pointed GRF variable
    INST_LIST_ITER currInst;    // starting instruction for the indirect accessing tracking
    G4_IndrTrackType trackType;    // tracking type

    IndrAccessTrackPara(vISA::G4_BB * startBB, vISA::G4_Operand * pointer, vISA::G4_RegVar * pointedVar, INST_LIST_ITER startInst,
        G4_IndrTrackType type)
        : bb(startBB), ptr(pointer), addrTaken(pointedVar), currInst(startInst), trackType(type)  {}
    //void* operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}
};

typedef std::vector<vISA::G4_RegVar*> REGVAR_VECTOR;

/*
 *  Performs flow-insensitive points-to analysis.
 *  Points-to analysis is performed once at the beginning of RA,
 *  and is used to compute the indirect uses of GRF variables for liveness analysis.
 *  Each address variable in the declare list is associated with a points-to set,
 *  which is a list of GRF RegVars.
 */
namespace vISA
{
class PointsToAnalysis
{
private:
    const unsigned int numBBs;
    unsigned int numAddrs;

    // keeps track of the indirect variables used in each BB
    const std::unique_ptr<REGVAR_VECTOR[]> indirectUses;
    // vector of points-to set for each address variable
    std::vector<REGVAR_VECTOR> pointsToSets;
    // index of an address's points-to set in the pointsToSets vector
    std::vector<unsigned> addrPointsToSetIndex;
    // original regvar ptrs
    REGVAR_VECTOR regVars;

    void resizePointsToSet(unsigned int newsize)
    {
        // Reallocate larger sets, change numAddrs.
        // Number of basic blocks is assumed to be unchanged.

        pointsToSets.resize(newsize);

        addrPointsToSetIndex.resize(newsize);
        for (unsigned i = numAddrs; i < newsize; i++)
        {
            addrPointsToSetIndex[i] = i;
        }

        numAddrs = newsize;
    }

    void addPointsToSetToBB(int bbId, const G4_RegVar* addr)
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS,
            "expect address variable");
        const REGVAR_VECTOR& addrTakens = pointsToSets[addrPointsToSetIndex[addr->getId()]];
        for (G4_RegVar* addrTaken : addrTakens)
        {
            addIndirectUseToBB(bbId, addrTaken);
        }
    }

    void addIndirectUseToBB(unsigned int bbId, G4_RegVar* var)
    {
        MUST_BE_TRUE(bbId < numBBs, "invalid basic block id");
        REGVAR_VECTOR& vec = indirectUses[bbId];
        bool isPresent = std::find(vec.begin(), vec.end(), var) != vec.end();
        if (!isPresent)
        {
            vec.push_back(var);
        }
    }

    void addToPointsToSet(const G4_RegVar* addr, G4_RegVar* var)
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS,
            "expect address variable");
        MUST_BE_TRUE(addr->getId() < numAddrs, "addr id is not set");
        int addrPTIndex = addrPointsToSetIndex[addr->getId()];
        REGVAR_VECTOR& vec = pointsToSets[addrPTIndex];
        bool isPresent = std::find(vec.begin(), vec.end(), var) != vec.end();
        if (!isPresent)
        {
            vec.push_back(var);
            DEBUG_VERBOSE("Addr " << addr->getId() << " <-- " << var->getDeclare()->getName() << "\n");
        }
    }

    // Merge addr2's points-to set into addr1's
    // basically we copy the content of addr2's points-to to addr1,
    // and have addr2 point to addr1's points-to set
    void mergePointsToSet(const G4_RegVar* addr1, const G4_RegVar* addr2)
    {
         MUST_BE_TRUE(addr1->getDeclare()->getRegFile() == G4_ADDRESS &&
             addr2->getDeclare()->getRegFile() == G4_ADDRESS,
             "expect address variable");
         int addr2PTIndex = addrPointsToSetIndex[addr2->getId()];
         REGVAR_VECTOR& vec = pointsToSets[addr2PTIndex];
         for (G4_RegVar* regVar : vec)
         {
             addToPointsToSet(addr1, regVar);
         }
         int addr1PTIndex = addrPointsToSetIndex[addr1->getId()];
         addrPointsToSetIndex[addr2->getId()] = addr1PTIndex;
         DEBUG_VERBOSE("merge Addr " << addr1->getId() << " with Addr " << addr2->getId());
    }

    unsigned int getIndexOfRegVar(const G4_RegVar* r) const
    {
        // Given a regvar pointer, return the index it was
        // found. This function is useful when regvar ids
        // are reset.

        const auto it = std::find(regVars.begin(), regVars.end(), r);
        return it == regVars.end() ? UINT_MAX : static_cast<unsigned int>(it - regVars.begin());
    }

public:
    PointsToAnalysis(const DECLARE_LIST& declares, unsigned numBBs);

    void doPointsToAnalysis(FlowGraph& fg);

    const REGVAR_VECTOR& getIndrUseVectorForBB(unsigned int bbId) const
    {
        MUST_BE_TRUE(bbId < numBBs, "invalid basic block id");
        return indirectUses[bbId];
    }

    // Following methods were added to support address taken spill/fill
    // Since ids of addr regvars will be reset, we fall back to using
    // the regvar ptr
    void insertAndMergeFilledAddr(const G4_RegVar* addr1, G4_RegVar* addr2)
    {
        unsigned int oldid = addr2->getId();
        addr2->setId(numAddrs);
        MUST_BE_TRUE(regVars.size() == numAddrs, "Inconsistency found between size of regvars and number of addr vars");

        if (addr2->getId() >= numAddrs)
            resizePointsToSet(numAddrs + 1);

        regVars.push_back(addr2);

        mergePointsToSet(addr1, addr2);
        addr2->setId(oldid);
    }

    const REGVAR_VECTOR* getAllInPointsTo(const G4_RegVar* addr) const
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
            return nullptr;

        const REGVAR_VECTOR* vec = &pointsToSets[addrPointsToSetIndex[id]];

        return vec;
    }

    const REGVAR_VECTOR& getAllInPointsToOrIndrUse(const G4_Operand* opnd, const G4_BB* bb) const
    {
        const REGVAR_VECTOR* pointsToSet = getAllInPointsTo(opnd->getBase()->asRegVar());
        if (pointsToSet != nullptr)
            return *pointsToSet;
        // this can happen if the address is coming from addr spill
        // ToDo: we can avoid this by linking the spilled addr with its new temp addr
        return getIndrUseVectorForBB(bb->getId());
    }

    G4_RegVar* getPointsTo(const G4_RegVar* addr, int idx) const
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
            return NULL;

        const REGVAR_VECTOR& vec = pointsToSets[addrPointsToSetIndex[id]];

        if (idx < (int)vec.size())
            return vec[idx];
        else
            return NULL;
    }

    bool isPresentInPointsTo(const G4_RegVar* addr, const G4_RegVar* var) const
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
            return false;

        const REGVAR_VECTOR& vec = pointsToSets[addrPointsToSetIndex[id]];
        for (const G4_RegVar* pointsTo : vec)
        {
            if (pointsTo->getId() == var->getId())
            {
                return true;
            }
        }

        return false;
    }

    void addFillToPointsTo(unsigned int bbid, G4_RegVar* addr, G4_RegVar* newvar)
    {
        // Adds to points to as well as indirect use in basic block
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
        {
            MUST_BE_TRUE(false, "Could not find addr in points to set");
        }

        REGVAR_VECTOR& vec = pointsToSets[addrPointsToSetIndex[id]];
        vec.push_back(newvar);

        addIndirectUseToBB(bbid, newvar);
    }

    void removeFromPointsTo(G4_RegVar* addr, G4_RegVar* vartoremove)
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
        {
            MUST_BE_TRUE(false, "Could not find addr in points to set");
        }

        REGVAR_VECTOR& vec = pointsToSets[addrPointsToSetIndex[id]];
        bool removed = false;

        for (REGVAR_VECTOR::iterator it = vec.begin();
            it != vec.end();
            it++)
        {
            G4_RegVar* cur = (*it);

            if (cur->getId() == vartoremove->getId())
            {
                vec.erase(it);
                removed = true;
                break;
            }
        }

        MUST_BE_TRUE(removed == true, "Could not find spilled ref from points to");

        // If an addr taken live-range is spilled then any basic block that has
        // an indirect use of it will no longer have it because we would have
        // inserted addr taken spill/fill code. So remove any indirect uses of
        // the var from all basic blocks. Currently this set is used when
        // constructing liveness sets before RA.
        for (unsigned int i = 0; i < numBBs; i++)
        {
            REGVAR_VECTOR& vec = indirectUses[i];

            for (REGVAR_VECTOR::iterator it = vec.begin();
                it != vec.end();
                it++)
            {
                G4_RegVar* cur = (*it);

                if (cur->getId() == vartoremove->getId())
                {
                    vec.erase(it);
                    break;
                }
            }
        }
    }
};

struct VarRange
{
    unsigned int leftBound;
    unsigned int rightBound;
};

typedef std::vector<VarRange* >   VAR_RANGE_LIST;
typedef std::vector<VarRange* >::iterator   VAR_RANGE_LIST_ITER;
typedef std::vector<VarRange* >::reverse_iterator   VAR_RANGE_LIST_REV_ITER;

struct VarRangeListPackage
{
    bool usedInSend;
    unsigned char rangeUnit;
    VAR_RANGE_LIST list;
};

class LivenessAnalysis
{
    unsigned numVarId = 0;         // the var count
    unsigned numGlobalVarId = 0;   // the global var count
    unsigned numSplitVar = 0;      // the split var count
    unsigned numSplitStartID = 0;      // the split var count
    unsigned numUnassignedVarId = 0;         // the unassigned var count
    unsigned numAddrId = 0;     // the addr count
    unsigned numBBId = 0;          // the block count
    unsigned numFnId = 0;          // the function count
    const unsigned char selectedRF = 0;  // the selected reg file kind for performing liveness
    const PointsToAnalysis& pointsToAnalysis;
    std::unordered_map<G4_Declare*, BitSet> neverDefinedRows;

    vISA::Mem_Manager m;

    void computeGenKillandPseudoKill(G4_BB* bb,
        BitSet& def_out,
        BitSet& use_in,
        BitSet& use_gen,
        BitSet& use_kill) const;

    bool contextFreeUseAnalyze(G4_BB* bb, bool isChanged);
    bool contextFreeDefAnalyze(G4_BB* bb, bool isChanged);

    bool livenessCandidate(const G4_Declare* decl, bool verifyRA) const;

    void dump_bb_vector(char* vname, std::vector<BitSet>& vec);
    void dump_fn_vector(char* vname, std::vector<FuncInfo*>& fns, std::vector<BitSet>& vec);

    void updateKillSetForDcl(G4_Declare* dcl, BitSet* curBBGen, BitSet* curBBKill, G4_BB* curBB, BitSet* entryBBGen, BitSet* entryBBKill,
        G4_BB* entryBB, unsigned scopeID);
    void footprintDst(const G4_BB* bb, const G4_INST* i, G4_Operand* opnd, BitSet* dstfootprint) const;
    static void footprintSrc(const G4_INST* i, G4_Operand *opnd, BitSet* srcfootprint);
    void detectNeverDefinedVarRows();

public:
    GlobalRA& gra;
    std::vector<G4_RegVar*>        vars;
    BitSet addr_taken;
    FlowGraph&                    fg;

    //
    // Bitsets used for data flow.
    //
    std::vector<BitSet> def_in;
    std::vector<BitSet> def_out;
    std::vector<BitSet> use_in;
    std::vector<BitSet> use_out;
    std::vector<BitSet> use_gen;
    std::vector<BitSet> use_kill;
    std::vector<BitSet> indr_use;
    std::unordered_map<FuncInfo*, BitSet> subroutineMaydef;

    bool isLocalVar(G4_Declare* decl) const;
    bool setGlobalVarIDs(bool verifyRA, bool areAllPhyRegAssigned);
    bool setLocalVarIDs(bool verifyRA, bool areAllPhyRegAssigned);
    bool setVarIDs(bool verifyRA, bool areAllPhyRegAssigned);
    LivenessAnalysis(GlobalRA& gra, unsigned char kind, bool verifyRA = false, bool forceRun = false);
    ~LivenessAnalysis();
    void computeLiveness();
    bool isLiveAtEntry(const G4_BB* bb, unsigned var_id) const;
    bool isUseThrough(const G4_BB* bb, unsigned var_id) const;
    bool isDefThrough(const G4_BB* bb, unsigned var_id) const;
    bool isLiveAtExit(const G4_BB* bb, unsigned var_id) const;
    bool isUseOut(const G4_BB* bb, unsigned var_id) const;
    bool isUseIn(const G4_BB* bb, unsigned var_id) const;
    bool isAddressSensitive (unsigned num) const  // returns true if the variable is address taken and also has indirect access
    {
        return addr_taken.isSet(num);
    }
    bool livenessClass(G4_RegFileKind regKind) const
    {
        return (selectedRF & regKind) != 0;
    }
    unsigned getNumSelectedVar() const {return numVarId;}
    unsigned getNumSelectedGlobalVar() const { return numGlobalVarId; }
    unsigned getNumSplitVar() const {return numSplitVar;}
    unsigned getNumSplitStartID() const {return numSplitStartID;}
    unsigned getNumUnassignedVar() const {return numUnassignedVarId;}
    void dump() const;
    void dumpBB(G4_BB* bb) const;
    void dumpLive(BitSet& live) const;
    void dumpGlobalVarNum() const;
    bool isEmptyLiveness() const;
    bool writeWholeRegion(const G4_BB* bb, const G4_INST* prd, G4_DstRegRegion* dst, const Options *opt) const;

    bool writeWholeRegion(const G4_BB* bb, const G4_INST* prd, const G4_VarBase* flagReg) const;

    void performScoping(BitSet* curBBGen, BitSet* curBBKill, G4_BB* curBB, BitSet* entryBBGen, BitSet* entryBBKill, G4_BB* entryBB);

    void hierarchicalIPA(const BitSet& kernelInput, const BitSet& kernelOutput);
    void useAnalysis(FuncInfo* subroutine);
    void useAnalysisWithArgRetVal(FuncInfo* subroutine,
        const std::unordered_map<FuncInfo*, BitSet>& args, const std::unordered_map<FuncInfo*, BitSet>& retVal);
    void defAnalysis(FuncInfo* subroutine);
    void maydefAnalysis();

    const PointsToAnalysis& getPointsToAnalysis() const { return pointsToAnalysis; }

    bool performIPA() const
    {
        return fg.builder->getOption(vISA_IPA) && livenessClass(G4_GRF) &&
            fg.getNumCalls() > 0;
    }
};

//
// entry point of register allocation
//
class IR_Builder; // forward declaration
}
int regAlloc(vISA::IR_Builder& builder, vISA::PhyRegPool& regPool, vISA::G4_Kernel& kernel);

#endif
