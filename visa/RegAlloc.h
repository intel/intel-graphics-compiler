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

struct pointInfo {
    vISA::G4_RegVar* var;
    unsigned char off;
};

struct addrExpInfo {
    vISA::G4_AddrExp* exp;
    unsigned char off;
};

typedef std::vector<pointInfo> REGVAR_VECTOR;
typedef std::vector<addrExpInfo> ADDREXP_VECTOR;
typedef std::vector<vISA::G4_RegVar*> ORG_REGVAR_VECTOR;

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
    // vector of address expression for each address variable
    std::vector<ADDREXP_VECTOR> addrExpSets;
    // index of an address's points-to set in the pointsToSets vector
    std::vector<unsigned> addrPointsToSetIndex;
    // original regvar ptrs
    ORG_REGVAR_VECTOR regVars;
    // store mapping of each address dcl -> vector of all pointees
    std::unordered_map<G4_Declare*, std::vector<G4_Declare*>> addrTakenMap;
    // store mapping of each pointee -> addr regs pointing to it
    std::unordered_map<G4_Declare*, std::vector<G4_Declare*>> revAddrTakenMap;

    void resizePointsToSet(unsigned int newsize)
    {
        // Reallocate larger sets, change numAddrs.
        // Number of basic blocks is assumed to be unchanged.

        pointsToSets.resize(newsize);
        addrExpSets.resize(newsize);
        addrPointsToSetIndex.resize(newsize);
        for (unsigned i = numAddrs; i < newsize; i++)
        {
            addrPointsToSetIndex[i] = i;
        }

        numAddrs = newsize;
    }

    void addPointsToSetToBB(int bbId, const G4_RegVar* addr)
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS || addr->getDeclare()->getRegFile() == G4_SCALAR,
            "expect address variable");
        const REGVAR_VECTOR& addrTakens = pointsToSets[addrPointsToSetIndex[addr->getId()]];
        for (auto addrTaken : addrTakens)
        {
            addIndirectUseToBB(bbId, addrTaken);
        }
    }

    bool isVar(G4_RegVar* var, pointInfo &pt)
    {
        return pt.var == var;
    }

    void addIndirectUseToBB(unsigned int bbId, pointInfo pt)
    {
        MUST_BE_TRUE(bbId < numBBs, "invalid basic block id");
        REGVAR_VECTOR& vec = indirectUses[bbId];
        auto it = std::find_if(vec.begin(), vec.end(),
            [&pt](const pointInfo& element) {return element.var == pt.var && element.off == pt.off; });

        if (it == vec.end())
        {
            vec.push_back(pt);
        }
    }

    void addToPointsToSet(const G4_RegVar* addr, G4_AddrExp* opnd, unsigned char offset)
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS || addr->getDeclare()->getRegFile() == G4_SCALAR,
            "expect address variable");
        MUST_BE_TRUE(addr->getId() < numAddrs, "addr id is not set");
        int addrPTIndex = addrPointsToSetIndex[addr->getId()];
        REGVAR_VECTOR& vec = pointsToSets[addrPTIndex];
        pointInfo pi = { opnd->getRegVar(), offset };

        auto it = std::find_if(vec.begin(), vec.end(),
            [&pi](const pointInfo& element) {return element.var == pi.var && element.off == pi.off; });
        if (it == vec.end())
        {
            vec.push_back(pi);
            DEBUG_VERBOSE("Addr " << addr->getId() << " <-- " << var->getDeclare()->getName() << "\n");
        }

        addrExpInfo pi1 = { opnd, offset };
        ADDREXP_VECTOR& vec1 = addrExpSets[addrPTIndex];
        auto it1 = std::find_if(vec1.begin(), vec1.end(),
            [&pi1](addrExpInfo& element) {return element.exp == pi1.exp && element.off == pi1.off; });
        if (it1 == vec1.end())
        {
            vec1.push_back(pi1);
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
         ADDREXP_VECTOR& vec = addrExpSets[addr2PTIndex];
         for (int i = 0; i < (int)vec.size(); i++)
         {
             addToPointsToSet(addr1, vec[i].exp, vec[i].off);
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
    // addr reg -> pointee regs
    const std::unordered_map<G4_Declare*, std::vector<G4_Declare*>>& getPointsToMap();
    // pointee -> addr reg
    const std::unordered_map<G4_Declare*, std::vector<G4_Declare*>>& getRevPointsToMap();

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

        resizePointsToSet(numAddrs + 1);

        regVars.push_back(addr2);

        mergePointsToSet(addr1, addr2);
        addr2->setId(oldid);
    }

    const REGVAR_VECTOR* getAllInPointsTo(const G4_RegVar* addr) const
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS || addr->getDeclare()->getRegFile() == G4_SCALAR,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
            return nullptr;

        const REGVAR_VECTOR* vec = &pointsToSets[addrPointsToSetIndex[id]];

        return vec;
    }

    ADDREXP_VECTOR* getAllInPointsToAddrExps(G4_RegVar* addr)
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS || addr->getDeclare()->getRegFile() == G4_SCALAR,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
            return nullptr;

        ADDREXP_VECTOR* vec = &addrExpSets[addrPointsToSetIndex[id]];

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
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS || addr->getDeclare()->getRegFile() == G4_SCALAR,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
            return NULL;

        const REGVAR_VECTOR& vec = pointsToSets[addrPointsToSetIndex[id]];

        if (idx < (int)vec.size())
            return vec[idx].var;
        else
            return NULL;
    }

    G4_RegVar* getPointsTo(const G4_RegVar* addr, int idx, unsigned char& offset) const
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS || addr->getDeclare()->getRegFile() == G4_SCALAR,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
            return NULL;
        int addrPTIndex = addrPointsToSetIndex[id];

        const REGVAR_VECTOR& vec = pointsToSets[addrPTIndex];

        if (idx < (int)vec.size())
        {
            offset = vec[idx].off;
            return vec[idx].var;
        }
        else
            return NULL;
    }

    bool isPresentInPointsTo(const G4_RegVar* addr, const G4_RegVar* var) const
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS || addr->getDeclare()->getRegFile() == G4_SCALAR,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
            return false;

        const REGVAR_VECTOR& vec = pointsToSets[addrPointsToSetIndex[id]];
        for (const pointInfo pointsTo : vec)
        {
            if (pointsTo.var->getId() == var->getId())
            {
                return true;
            }
        }

        return false;
    }

    void addFillToPointsTo(unsigned int bbid, G4_RegVar* addr, G4_RegVar* newvar)
    {
        // Adds to points to as well as indirect use in basic block
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS || addr->getDeclare()->getRegFile() == G4_SCALAR,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);

        if (id == UINT_MAX)
        {
            MUST_BE_TRUE(false, "Could not find addr in points to set");
        }

        REGVAR_VECTOR& vec = pointsToSets[addrPointsToSetIndex[id]];
        pointInfo pt = { newvar, 0 };
        vec.push_back(pt);

        addIndirectUseToBB(bbid, pt);
    }

    void patchPointsToSet(const G4_RegVar* addr, G4_AddrExp* opnd, unsigned char offset)
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS,
            "expect address variable");
        unsigned int id = getIndexOfRegVar(addr);
        int addrPTIndex = addrPointsToSetIndex[id];
        REGVAR_VECTOR& vec = pointsToSets[addrPTIndex];
        pointInfo pi = { opnd->getRegVar(), offset };

        auto it = std::find_if(vec.begin(), vec.end(),
            [&pi](const pointInfo& element) {return element.var == pi.var && element.off == pi.off; });
        if (it == vec.end())
        {
            vec.push_back(pi);
            DEBUG_VERBOSE("Addr " << addr->getId() << " <-- " << var->getDeclare()->getName() << "\n");
        }

        addrExpInfo pi1 = { opnd, offset };
        ADDREXP_VECTOR& vec1 = addrExpSets[addrPTIndex];
        auto it1 = std::find_if(vec1.begin(), vec1.end(),
            [&pi1](addrExpInfo& element) {return element.exp == pi1.exp && element.off == pi1.off; });
        if (it1 == vec1.end())
        {
            vec1.push_back(pi1);
        }
    }

    void removeFromPointsTo(G4_RegVar* addr, G4_RegVar* vartoremove)
    {
        MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS || addr->getDeclare()->getRegFile() == G4_SCALAR,
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
            pointInfo cur = (*it);

            if (cur.var->getId() == vartoremove->getId())
            {
                vec.erase(it);
                removed = true;
                break;
            }
        }

        ADDREXP_VECTOR& vec1 = addrExpSets[addrPointsToSetIndex[id]];
        for (ADDREXP_VECTOR::iterator it = vec1.begin();
            it != vec1.end();
            it++)
        {
            addrExpInfo cur = (*it);

            if (cur.exp->getRegVar()->getId() == vartoremove->getId())
            {
                vec1.erase(it);
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
                pointInfo cur = (*it);

                if (cur.var->getId() == vartoremove->getId())
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
        SparseBitSet& def_out,
        SparseBitSet& use_in,
        SparseBitSet& use_gen,
        SparseBitSet& use_kill) const;

    bool contextFreeUseAnalyze(G4_BB* bb, bool isChanged);
    bool contextFreeDefAnalyze(G4_BB* bb, bool isChanged);

    bool livenessCandidate(const G4_Declare* decl, bool verifyRA) const;

    void dump_bb_vector(char* vname, std::vector<BitSet>& vec);
    void dump_fn_vector(char* vname, std::vector<FuncInfo*>& fns, std::vector<BitSet>& vec);

    void updateKillSetForDcl(G4_Declare* dcl, SparseBitSet* curBBGen, SparseBitSet* curBBKill, G4_BB* curBB, SparseBitSet* entryBBGen, SparseBitSet* entryBBKill,
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
    std::vector<SparseBitSet> def_in;
    std::vector<SparseBitSet> def_out;
    std::vector<SparseBitSet> use_in;
    std::vector<SparseBitSet> use_out;
    std::vector<SparseBitSet> use_gen;
    std::vector<SparseBitSet> use_kill;
    std::vector<SparseBitSet> indr_use;
    std::unordered_map<FuncInfo*, SparseBitSet> subroutineMaydef;

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

    void performScoping(SparseBitSet* curBBGen, SparseBitSet* curBBKill, G4_BB* curBB, SparseBitSet* entryBBGen, SparseBitSet* entryBBKill, G4_BB* entryBB);

    void hierarchicalIPA(const SparseBitSet& kernelInput, const SparseBitSet& kernelOutput);
    void useAnalysis(FuncInfo* subroutine);
    void useAnalysisWithArgRetVal(FuncInfo* subroutine,
        const std::unordered_map<FuncInfo*, SparseBitSet>& args, const std::unordered_map<FuncInfo*, SparseBitSet>& retVal);
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
