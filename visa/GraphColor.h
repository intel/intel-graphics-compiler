/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __GRAPHCOLOR_H__
#define __GRAPHCOLOR_H__

#include "BitSet.h"
#include "G4_IR.hpp"
#include "RegAlloc.h"
#include "RPE.h"
#include "SpillManagerGMRF.h"
#include "VarSplit.h"

#include <list>
#include <limits>
#include <memory>
#include <map>
#include <unordered_set>
#include <vector>

#define BITS_DWORD 32
#define ROUND(x,y)    ((x) + ((y - x % y) % y))

namespace vISA
{
    const float MAXSPILLCOST = (std::numeric_limits<float>::max());
    const float MINSPILLCOST = -(std::numeric_limits<float>::max());

    class BankConflictPass
    {
    private:
        GlobalRA& gra;
        bool forGlobal;

        BankConflict setupBankAccordingToSiblingOperand(BankConflict assignedBank, unsigned offset, bool oneGRFBank);
        void setupEvenOddBankConflictsForDecls(G4_Declare * dcl_1, G4_Declare * dcl_2, unsigned offset1, unsigned offset2,
            BankConflict &srcBC1, BankConflict &srcBC2);
        void setupBankConflictsOneGRFOld(G4_INST* inst, int &bank1RegNum, int &bank2RegNum, float GRFRatio, unsigned &internalConflict);
        bool isOddOffset(unsigned offset) const;
        void setupBankConflictsforDPAS(G4_INST* inst);
        bool hasDpasInst = false;

        void setupBankConflictsforTwoGRFs(G4_INST* inst);
        void setupBankConflictsforMad(G4_INST* inst);
        void setupBankConflictsForBB(G4_BB* bb, unsigned &threeSourceInstNum, unsigned &sendInstNum,
            unsigned numRegLRA, unsigned & internalConflict);
        void setupBankConflictsForBBTGL(G4_BB* bb, unsigned& threeSourceInstNum, unsigned& sendInstNum, unsigned numRegLRA, unsigned& internalConflict);
        bool hasInternalConflict3Srcs(BankConflict *srcBC);
        void setupBankForSrc0(G4_INST* inst, G4_INST* prevInst);
        void getBanks(G4_INST* inst, BankConflict *srcBC, G4_Declare **dcls, G4_Declare **opndDcls, unsigned *offset);
        void getPrevBanks(G4_INST* inst, BankConflict *srcBC, G4_Declare **dcls, G4_Declare **opndDcls, unsigned *offset);

    public:
        bool setupBankConflictsForKernel(bool doLocalRR, bool &threeSourceCandidate, unsigned numRegLRA, bool &highInternalConflict);

        BankConflictPass(GlobalRA& g, bool global) : gra(g), forGlobal(global)
        {

        }
    };

class LiveRange final
{
    G4_RegVar* const var;
    G4_Declare* const dcl;
    const G4_RegFileKind regKind;
    bool* forbidden = nullptr;
    int numForbidden = -1;
    bool spilled = false;

    GlobalRA& gra;
    unsigned numRegNeeded;
    unsigned degree = 0;
    unsigned refCount = 0;
    unsigned parentLRID;
    AssignedReg reg;
    float spillCost;
    BankConflict bc = BankConflict::BANK_CONFLICT_NONE;
    const static unsigned UndefHint = 0xffffffff;
    unsigned allocHint = UndefHint;

    union {
        uint16_t bunch = 0;
        struct {
            uint16_t calleeSaveBias : 1; // indicates if the var is biased to get a callee-save assignment or not
            uint16_t callerSaveBias : 1; // indicates if the var is biased to get a caller-save assignment or not
            uint16_t isEOTSrc : 1; //Gen7 only, Whether the liveRange is the message source of an EOT send
            uint16_t retIp : 1;   // variable is the return ip and should not be spilled

            uint16_t active : 1;
            uint16_t isInfiniteCost : 1;
            uint16_t isCandidate : 1;
            uint16_t isPseudoNode : 1;
            uint16_t isPartialDeclare : 1;
            uint16_t isSplittedDeclare : 1;
        };
    };

public:

    LiveRange(G4_RegVar* v, GlobalRA&);

    void* operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }

    void setDegree(unsigned d) {degree = d;}
    unsigned getDegree() const {return degree;}

    unsigned getNumRegNeeded() const {return numRegNeeded;}

    void subtractDegree(unsigned d)
    {
        MUST_BE_TRUE(d <= degree, ERROR_INTERNAL_ARGUMENT);
        degree -= d;
    }

    void setActive(bool v) {active = v;}
    bool getActive() const {return active;}

    void emit(std::ostream& output, bool symbolreg = false) const
    {
        output << getVar ()->getDeclare ()->getName();
        if (reg.phyReg != NULL)
        {
            output << "(";
            reg.phyReg->emit(output);
            output << '.' << reg.subRegOff << ':';
            output << TypeSymbol(getVar()->getDeclare()->getElemType()) << ")";
        }
        output << "(size = " << getDcl()->getByteSize() <<
            ", spill cost = " << getSpillCost() << ", degree = " << getDegree() << ")";
    }

    unsigned getRefCount() const {return refCount;}
    void setRefCount(unsigned count) {refCount = count;}

    float getSpillCost() const {return spillCost;}
    void setSpillCost(float cost) {spillCost = cost;}

    bool getIsInfiniteSpillCost() const { return isInfiniteCost; }
    void checkForInfiniteSpillCost(G4_BB* bb, std::list<G4_INST*>::reverse_iterator& it);

    G4_VarBase* getPhyReg() const { return reg.phyReg; }

    unsigned getPhyRegOff() const { return reg.subRegOff; }

    void setPhyReg(G4_VarBase* pr, unsigned off)
    {
        MUST_BE_TRUE(pr->isPhyReg(), ERROR_UNKNOWN);
        reg.phyReg = pr;
        reg.subRegOff = off;
    }

    void resetPhyReg()
    {
        reg.phyReg = nullptr;
        reg.subRegOff = 0;
    }

    bool getIsPseudoNode() const { return isPseudoNode; }
    void setIsPseudoNode() { isPseudoNode = true; }
    bool getIsPartialDcl() const { return isPartialDeclare; }
    void setIsPartialDcl() { isPartialDeclare = true; }
    bool getIsSplittedDcl() const { return isSplittedDeclare; }
    void setIsSplittedDcl(bool v) { isSplittedDeclare = v; }
    BankConflict getBC() const { return bc;  }
    void setBC(BankConflict c) { bc = c; }
    void setParentLRID(int id) { parentLRID = id; }
    unsigned getParentLRID() const { return parentLRID; }

    unsigned getAllocHint() const { return allocHint; }
    bool hasAllocHint() const { return allocHint != UndefHint; }
    void setAllocHint(unsigned h);
    void resetAllocHint() { allocHint = UndefHint; }

    // From VarBasis
    public:
        void allocForbidden(vISA::Mem_Manager& mem, bool reserveStackCallRegs, unsigned reserveSpillSize, unsigned rerservedRegNum);
        void allocForbiddenCallerSave(vISA::Mem_Manager& mem, G4_Kernel* kernel);
        void allocForbiddenCalleeSave(vISA::Mem_Manager& mem, G4_Kernel* kernel);
        const bool* getForbidden() const { return forbidden; }
        void markForbidden(int reg, int numReg)
        {
            MUST_BE_TRUE(((int)getForbiddenVectorSize()) >= reg + numReg, "forbidden register is out of bound");
            for (int i = reg; i < reg + numReg; ++i)
            {
                forbidden[i] = true;
            }
            numForbidden = -1;
        }
        int getNumForbidden()
        {
            if (forbidden == nullptr)
            {
                return 0;
            }
            if (numForbidden == -1)
            {
                numForbidden = 0;
                for (int i = 0, size = getForbiddenVectorSize(); i < size; ++i)
                {
                    if (forbidden[i])
                    {
                        ++numForbidden;
                    }
                }
            }
            return numForbidden;
        }
        G4_RegVar* getVar() const { return var; }
        G4_Declare* getDcl() const { return dcl; }
        G4_RegFileKind getRegKind() const { return regKind; }
        void dump() const;

        void setCalleeSaveBias(bool v) { calleeSaveBias = v; }
        bool getCalleeSaveBias() const { return calleeSaveBias; }

        void setCallerSaveBias(bool v) { callerSaveBias = v; }
        bool getCallerSaveBias() const { return callerSaveBias; }

        void setEOTSrc() { isEOTSrc = true; }
        bool getEOTSrc() const { return isEOTSrc; }

        void setRetIp() { retIp = true; }
        bool isRetIp() const { return retIp; }

        bool isSpilled() const { return spilled; }
        void setSpilled(bool v) { spilled = v; }

private:
    //const Options *m_options;
    unsigned getForbiddenVectorSize() const;
    void allocForbiddenVector(vISA::Mem_Manager& mem);
}; // class LiveRange
} // vISA::
typedef std::list<vISA::LiveRange*> LIVERANGE_LIST;
typedef std::list<vISA::LiveRange*>::iterator LIVERANGE_LIST_ITER;

// A mapping from the pseudo decl created for caller save/restore, to the ret val
// This is used in augmentIntfGraph to prune interference edges for fcall ret val
typedef std::map<vISA::G4_Declare*, vISA::G4_Declare*> FCALL_RET_MAP;
typedef std::map<vISA::G4_Declare*, vISA::G4_Declare*>::iterator FCALL_RET_MAP_ITER;

typedef std::map<vISA::G4_Declare*, std::pair<vISA::G4_INST*, unsigned>> CALL_DECL_MAP;
typedef std::map<vISA::G4_Declare*, std::pair<vISA::G4_INST*, unsigned>>::iterator CALL_DECL_MAP_ITER;

//
// A bit array records all interference information.
// (2D matrix is flatten to 1D array)
// Since the interference information is symmetric, we can use only
// half of the size. To simplify the implementation, we use the full
// size of the bit array.
//
namespace vISA
{
    class Augmentation
    {
    private:
        // pair of default mask, non-default mask
        using MaskDeclares = std::pair<SparseBitSet, SparseBitSet>;
        G4_Kernel& kernel;
        Interference& intf;
        GlobalRA& gra;
        const LivenessAnalysis& liveAnalysis;
        LiveRange** const& lrs;
        FCALL_RET_MAP& fcallRetMap;
        CALL_DECL_MAP callDclMap;
        std::unordered_map<FuncInfo*, PhyRegSummary> localSummaryOfCallee;
        std::vector<G4_Declare*> sortedIntervals;
        std::list<G4_Declare*> defaultMask;
        std::list<G4_Declare*> nonDefaultMask;
        std::unordered_map<FuncInfo*, MaskDeclares> callsiteDeclares;
        std::unordered_map <G4_Declare*, MaskDeclares> retDeclares;
        Mem_Manager& m;

        bool updateDstMaskForGather(G4_INST* inst, std::vector<unsigned char>& mask);
        bool updateDstMaskForGatherRaw(G4_INST* inst, std::vector<unsigned char>& mask, const G4_SendDescRaw* raw);
        bool updateDstMaskForGatherLdSt(G4_INST* inst, std::vector<unsigned char>& mask, const G4_SendDescLdSt *ldst);
        void updateDstMask(G4_INST* inst, bool checkCmodOnly);
        static unsigned getByteSizeFromMask(AugmentationMasks type);
        bool isDefaultMaskDcl(G4_Declare* dcl, unsigned simdSize, AugmentationMasks type);
        bool isDefaultMaskSubDeclare(unsigned char* mask, unsigned lb, unsigned rb, G4_Declare* dcl, unsigned simdSize);
        bool verifyMaskIfInit(G4_Declare* dcl, AugmentationMasks mask);
        bool checkGRFPattern3(G4_Declare* dcl, G4_DstRegRegion* dst, unsigned maskOff,
            unsigned lb, unsigned rb, unsigned execSize);
        bool checkGRFPattern2(G4_Declare* dcl, G4_DstRegRegion* dst, unsigned maskOff,
            unsigned lb, unsigned rb, unsigned execSize);
        bool checkGRFPattern1(G4_Declare* dcl, G4_DstRegRegion* dst, unsigned maskOff,
            unsigned lb, unsigned rb, unsigned execSize);
        void markNonDefaultDstRgn(G4_INST* inst, G4_Operand* opnd);
        bool markNonDefaultMaskDef();
        G4_BB* getTopmostBBDst(G4_BB* src, G4_BB* end, G4_BB* origSrc, unsigned traversal);
        void updateStartIntervalForSubDcl(G4_Declare* dcl, G4_INST* curInst, G4_Operand *opnd);
        void updateEndIntervalForSubDcl(G4_Declare* dcl, G4_INST* curInst, G4_Operand *opnd);
        void updateStartInterval(const G4_Declare* dcl, G4_INST* curInst);
        void updateEndInterval(const G4_Declare* dcl, G4_INST* curInst);
        void updateStartIntervalForLocal(G4_Declare* dcl, G4_INST* curInst, G4_Operand *opnd);
        void updateEndIntervalForLocal(G4_Declare* dcl, G4_INST* curInst, G4_Operand *opnd);
        void buildLiveIntervals();
        void sortLiveIntervals();
        unsigned getEnd(const G4_Declare* dcl) const;
        bool isNoMask(const G4_Declare* dcl, unsigned size) const;
        bool isConsecutiveBits(const G4_Declare* dcl, unsigned size) const;
        bool isCompatible(const G4_Declare* testDcl, const G4_Declare* biggerDcl) const;
        void buildInterferenceIncompatibleMask();
        void buildInteferenceForCallSiteOrRetDeclare(G4_Declare* newDcl, MaskDeclares* mask);
        void buildInteferenceForCallsite(FuncInfo* func);
        void buildInteferenceForRetDeclares();
        void buildSummaryForCallees();
        void expireIntervals(unsigned startIdx);
        void buildSIMDIntfDcl(G4_Declare* newDcl, bool isCall);
        void buildSIMDIntfAll(G4_Declare* newDcl);
        void buildSIMDIntfAllOld(G4_Declare* newDcl);
        void updateActiveList(G4_Declare* newDcl, std::list<G4_Declare*>* dclMaskList);
        void handleSIMDIntf(G4_Declare* firstDcl, G4_Declare* secondDcl, bool isCall);
        bool weakEdgeNeeded(AugmentationMasks, AugmentationMasks);

        void addSIMDIntfDclForCallSite(MaskDeclares* maskDeclares);
        void addSIMDIntfForRetDclares(G4_Declare* newDcl);

    public:
        Augmentation(G4_Kernel& k, Interference& i, const LivenessAnalysis& l, LiveRange** const& ranges, GlobalRA& g);
        ~Augmentation();

        void augmentIntfGraph();

        const std::vector<G4_Declare*>& getSortedLiveIntervals() const { return sortedIntervals; }
    };

    class Interference
    {
        friend class Augmentation;

        // This stores compatible ranges for each variable. Such
        // compatible ranges will not be present in sparseIntf set.
        // We store G4_Declare* instead of id is because variables
        // allocated by LRA will not have a valid id.
        std::map<G4_Declare*, std::vector<G4_Declare*>> compatibleSparseIntf;

        GlobalRA& gra;
        G4_Kernel& kernel;
        LiveRange** const & lrs;
        IR_Builder& builder;
        const unsigned maxId;
        const unsigned rowSize;
        const unsigned splitStartId;
        const unsigned splitNum;
        unsigned* matrix = nullptr;
        const LivenessAnalysis* const liveAnalysis;
        Augmentation aug;

        std::vector<std::vector<unsigned>> sparseIntf;

        // sparse interference matrix.
        // we don't directly update sparseIntf to ensure uniqueness
        // like dense matrix, interference is not symmetric (that is, if v1 and v2 interfere and v1 < v2,
        // we insert (v1, v2) but not (v2, v1)) for better cache behavior
        std::vector<std::unordered_set<uint32_t> > sparseMatrix;
        static const uint32_t denseMatrixLimit = 0x80000;

        static void updateLiveness(SparseBitSet& live, uint32_t id, bool val)
        {
            live.set(id, val);
        }

        G4_Declare* getGRFDclForHRA(int GRFNum) const;

        bool useDenseMatrix() const
        {
            return maxId < denseMatrixLimit;
        }

        // Only upper-half matrix is now used in intf graph.
        inline void safeSetInterference(unsigned v1, unsigned v2)
        {
            // Assume v1 < v2
            if (useDenseMatrix())
            {
                unsigned col = v2 / BITS_DWORD;
                matrix[v1 * rowSize + col] |= 1 << (v2 % BITS_DWORD);
            }
            else
            {
                sparseMatrix[v1].emplace(v2);
            }
        }

        inline void setBlockInterferencesOneWay(unsigned v1, unsigned col, unsigned block)
        {
            if (useDenseMatrix())
            {
#ifdef _DEBUG
                MUST_BE_TRUE(sparseIntf.size() == 0, "Updating intf graph matrix after populating sparse intf graph");
#endif

                matrix[v1 * rowSize + col] |= block;
            }
            else
            {
                auto&& intfSet = sparseMatrix[v1];
                for (int i = 0; i < BITS_DWORD; ++i)
                {
                    if (block & (1 << i))
                    {
                        uint32_t v2 = col * BITS_DWORD + i;
                        intfSet.emplace(v2);
                    }
                }
            }
        }

        unsigned getInterferenceBlk(unsigned idx) const
        {
            assert(useDenseMatrix() && "matrix is not initialized");
            return matrix[idx];
        }

        void addCalleeSaveBias(const SparseBitSet& live);

        void buildInterferenceAtBBExit(const G4_BB* bb, SparseBitSet& live);
        void buildInterferenceWithinBB(G4_BB* bb, SparseBitSet& live);
        void buildInterferenceForDst(G4_BB* bb, SparseBitSet& live, G4_INST* inst, std::list<G4_INST*>::reverse_iterator i, G4_DstRegRegion* dst);
        void buildInterferenceForFcall(G4_BB* bb, SparseBitSet& live, G4_INST* inst, std::list<G4_INST*>::reverse_iterator i, const G4_VarBase* regVar);

        inline void filterSplitDclares(unsigned startIdx, unsigned endIdx, unsigned n, unsigned col, unsigned &elt, bool is_split);

        void buildInterferenceWithLive(const SparseBitSet& live, unsigned i);
        void buildInterferenceWithSubDcl(unsigned lr_id, G4_Operand *opnd, SparseBitSet& live, bool setLive, bool setIntf);
        void buildInterferenceWithAllSubDcl(unsigned v1, unsigned v2);

        void markInterferenceForSend(G4_BB* bb, G4_INST* inst, G4_DstRegRegion* dst);

        void buildInterferenceWithLocalRA(G4_BB* bb);

        void buildInterferenceAmongLiveOuts();
        void buildInterferenceAmongLiveIns();

        void markInterferenceToAvoidDstSrcOverlap(G4_BB* bb, G4_INST* inst);

        void generateSparseIntfGraph();

    public:
        Interference(const LivenessAnalysis* l, LiveRange** const & lr, unsigned n, unsigned ns, unsigned nm,
            GlobalRA& g);

        ~Interference()
        {
            if (useDenseMatrix())
            {
                delete[] matrix;
            }
        }

        const std::vector<G4_Declare*>* getCompatibleSparseIntf(G4_Declare* d) const
        {
            if (compatibleSparseIntf.size() > 0)
            {
                auto it = compatibleSparseIntf.find(d);
                if (it == compatibleSparseIntf.end())
                {
                    return nullptr;
                }
                return &it->second;
            }
            return nullptr;
        }

        void init(vISA::Mem_Manager& m)
        {
            if (useDenseMatrix())
            {
                auto N = (size_t)rowSize * (size_t)maxId;
                matrix = new uint32_t[N](); // zero-initialize
            }
            else
            {
                sparseMatrix.resize(maxId);
            }
        }

        void computeInterference();
        void applyPartitionBias();
        bool interfereBetween(unsigned v1, unsigned v2) const;
        const std::vector<unsigned>& getSparseIntfForVar(unsigned id) const { return sparseIntf[id]; }

        inline bool varSplitCheckBeforeIntf(unsigned v1, unsigned v2) const;

        void checkAndSetIntf(unsigned v1, unsigned v2)
        {
            if (v1 < v2)
            {
                safeSetInterference(v1, v2);
            }
            else if (v1 > v2)
            {
                safeSetInterference(v2, v1);
            }
        }

        void dumpInterference() const;
        void dumpVarInterference() const;
        bool dumpIntf(const char*) const;
        void interferenceVerificationForSplit() const;

        bool linearScanVerify() const;

        bool isStrongEdgeBetween(const G4_Declare*, const G4_Declare*) const;

        const Augmentation& getAugmentation() const { return aug; }
    };

    // Class to compute reg chart dump and dump it to ostream.
    // Used only when -dumpregchart is passed.
    class RegChartDump
    {
        const GlobalRA& gra;
        std::vector<G4_Declare*> sortedLiveIntervals;
        std::unordered_map<G4_Declare*, std::pair<G4_INST*, G4_INST*>> startEnd;
    public:
        void recordLiveIntervals(const std::vector<G4_Declare*>& dcls);
        void dumpRegChart(std::ostream&, LiveRange** lrs = nullptr, unsigned numLRs = 0);

        RegChartDump(const GlobalRA& g) : gra(g) {}
    };

    class GraphColor
    {
        GlobalRA& gra;

        unsigned totalGRFRegCount; // .reg_count_total
        const unsigned numVar;
        const unsigned numSplitStartID;
        const unsigned numSplitVar;
        unsigned *spAddrRegSig;
        Interference intf;
        PhyRegPool& regPool;
        IR_Builder& builder;
        LiveRange** lrs = nullptr;
        bool isHybrid;
        LIVERANGE_LIST spilledLRs;
        bool forceSpill;
        vISA::Mem_Manager mem;
        const Options *m_options;

        unsigned evenTotalDegree = 1;
        unsigned oddTotalDegree = 1;
        unsigned evenTotalRegNum = 1;
        unsigned oddTotalRegNum = 1;
        unsigned evenMaxRegNum = 1;
        unsigned oddMaxRegNum = 1;

        G4_Kernel& kernel;
        LivenessAnalysis& liveAnalysis;

        std::vector<LiveRange*> colorOrder;
        LIVERANGE_LIST unconstrainedWorklist;
        LIVERANGE_LIST constrainedWorklist;
        unsigned numColor = 0;

        bool failSafeIter = false;

        unsigned edgeWeightGRF(const LiveRange* lr1, const LiveRange* lr2);
        unsigned edgeWeightARF(const LiveRange* lr1, const LiveRange* lr2);

        void computeDegreeForGRF();
        void computeDegreeForARF();
        void computeSpillCosts(bool useSplitLLRHeuristic);
        void determineColorOrdering();
        void removeConstrained();
        void relaxNeighborDegreeGRF(LiveRange* lr);
        void relaxNeighborDegreeARF(LiveRange* lr);
        bool assignColors(ColorHeuristic heuristicGRF, bool doBankConflict, bool highInternalConflict, bool honorHints = true);

        void clearSpillAddrLocSignature()
        {
            memset(spAddrRegSig, 0, getNumAddrRegisters() * sizeof(unsigned));
        }
        void pruneActiveSpillAddrLocs(G4_DstRegRegion*, unsigned, G4_Type);
        void updateActiveSpillAddrLocs(G4_DstRegRegion*, G4_SrcRegRegion*, unsigned execSize);
        bool redundantAddrFill(G4_DstRegRegion*, G4_SrcRegRegion*, unsigned execSize);

    public:
        G4_Declare* findDeclare(char* name);
        void getExtraInterferenceInfo();
        GraphColor(LivenessAnalysis& live, unsigned totalGRF, bool hybrid, bool forceSpill_);

        const Options * getOptions() const { return m_options; }

        bool regAlloc(
            bool doBankConflictReduction,
            bool highInternalConflict,
            bool reserveSpillReg, unsigned& spillRegSize, unsigned& indrSpillRegSize, const RPE* rpe);
        bool requireSpillCode() const { return !spilledLRs.empty(); }
        const Interference * getIntf() const { return &intf; }
        void createLiveRanges(unsigned reserveSpillSize = 0);
        LiveRange ** getLiveRanges() const { return lrs; }
        const LIVERANGE_LIST & getSpilledLiveRanges() const { return spilledLRs; }
        void confirmRegisterAssignments();
        void resetTemporaryRegisterAssignments();
        void cleanupRedundantARFFillCode();
        void getCalleeSaveRegisters();
        void addA0SaveRestoreCode();
        void addFlagSaveRestoreCode();
        void getSaveRestoreRegister();
        void getCallerSaveRegisters();
        void dumpRegisterPressure();
        GlobalRA & getGRA() { return gra; }
        G4_SrcRegRegion* getScratchSurface() const;
        LiveRange** getLRs() const { return lrs; }
        unsigned int getNumVars() const { return numVar; }
    };

    struct BundleConflict
    {
        const G4_Declare * const dcl;
        const int offset;
        BundleConflict(const G4_Declare *dcl, int offset) : dcl(dcl), offset(offset) { }
    };

    struct RAVarInfo
    {
        unsigned numSplit = 0;
        unsigned bb_id = UINT_MAX;      // block local variable's block id.
        G4_Declare* splittedDCL = nullptr;
        LocalLiveRange* localLR = nullptr;
        LSLiveRange* LSLR = nullptr;
        unsigned numRefs = 0;
        BankConflict conflict = BANK_CONFLICT_NONE;      // used to indicate bank that should be assigned to dcl if possible
        G4_INST* startInterval = nullptr;
        G4_INST* endInterval = nullptr;
        std::vector<unsigned char> mask;
        std::vector<const G4_Declare*> subDclList;
        unsigned subOff = 0;
        std::vector<BundleConflict> bundleConflicts;
        G4_SubReg_Align subAlign = G4_SubReg_Align::Any;
        bool isEvenAlign = false;
    };

    class VerifyAugmentation
    {
    private:
        G4_Kernel* kernel = nullptr;
        GlobalRA* gra = nullptr;
        std::vector<G4_Declare*> sortedLiveRanges;
        std::unordered_map<const G4_Declare*, std::tuple<LiveRange*, AugmentationMasks, G4_INST*, G4_INST*>> masks;
        LiveRange* const * lrs = nullptr;
        unsigned numVars = 0;
        const Interference* intf = nullptr;
        std::unordered_map<G4_Declare*, LiveRange*> DclLRMap;
        std::unordered_map<G4_BB*, std::string> bbLabels;
        std::vector<std::tuple<G4_BB*, unsigned, unsigned>> BBLexId;

        static const char* getStr(AugmentationMasks a)
        {
            if (a == AugmentationMasks::Default16Bit)
                return "Default16Bit";
            else if (a == AugmentationMasks::Default32Bit)
                return "Default32Bit";
            else if (a == AugmentationMasks::Default64Bit)
                return "Default64Bit";
            else if (a == AugmentationMasks::NonDefault)
                return "NonDefault";
            else if (a == AugmentationMasks::Undetermined)
                return "Undetermined";

            return "-----";
        };
        void labelBBs();
        void populateBBLexId();
        bool interfereBetween(G4_Declare*, G4_Declare*);
        void verifyAlign(G4_Declare* dcl);
        unsigned getGRFBaseOffset(const G4_Declare* dcl) const;

    public:
        void verify();
        void reset()
        {
            sortedLiveRanges.clear();
            masks.clear();
            kernel = nullptr;
            lrs = nullptr;
            gra = nullptr;
            numVars = 0;
            intf = nullptr;
            DclLRMap.clear();
            bbLabels.clear();
            BBLexId.clear();
        }
        void loadAugData(std::vector<G4_Declare*>& s, LiveRange* const * l, unsigned n, const Interference* i, GlobalRA& g);
        void dump(const char* dclName);
        bool isClobbered(LiveRange* lr, std::string& msg);
    };

    class GlobalRA
    {
    private:
        std::unordered_set<G4_INST*> EUFusionCallWAInsts;
        bool m_EUFusionCallWANeeded;
        std::unordered_set<G4_INST*> EUFusionNoMaskWAInsts;
    public:
        bool EUFusionCallWANeeded() const { return m_EUFusionCallWANeeded; }
        void addEUFusionCallWAInst(G4_INST* inst);
        void removeEUFusionCallWAInst(G4_INST* inst) { EUFusionCallWAInsts.erase(inst); }
        const std::unordered_set<G4_INST*>& getEUFusionCallWAInsts() { return EUFusionCallWAInsts; }
        bool EUFusionNoMaskWANeeded() const { return builder.hasFusedEUNoMaskWA(); }
        void addEUFusionNoMaskWAInst(G4_BB* BB, G4_INST* Inst);
        void removeEUFusionNoMaskWAInst(G4_INST* Inst);
        const std::unordered_set<G4_INST*>& getEUFusionNoMaskWAInsts() { return EUFusionNoMaskWAInsts; }
    public:
        std::unique_ptr<VerifyAugmentation> verifyAugmentation;
        std::unique_ptr<RegChartDump> regChart;
        std::unique_ptr<SpillAnalysis> spillAnalysis;
        static bool useGenericAugAlign(PlatformGen gen)
        {
            if (gen == PlatformGen::GEN9 ||
                gen == PlatformGen::GEN8)
                return false;
            return true;
        }
        static const char StackCallStr[];

    private:
        template <class REGION_TYPE> static unsigned getRegionDisp(REGION_TYPE * region, const IR_Builder& irb);
        unsigned getRegionByteSize(G4_DstRegRegion * region, unsigned execSize);
        static bool owordAligned(unsigned offset) { return offset % 16 == 0; }
        template <class REGION_TYPE> bool isUnalignedRegion(REGION_TYPE * region, unsigned execSize);
        bool shouldPreloadDst(G4_INST* instContext, G4_BB* curBB);
        bool livenessCandidate(const G4_Declare* decl) const;
        void updateDefSet(std::set<G4_Declare*>& defs, G4_Declare* referencedDcl);
        void detectUndefinedUses(LivenessAnalysis& liveAnalysis, G4_Kernel& kernel);
        void markBlockLocalVar(G4_RegVar* var, unsigned bbId);
        void markBlockLocalVars();
        void computePhyReg();
        void fixAlignment();
        void expandSpillIntrinsic(G4_BB*);
        void expandFillIntrinsic(G4_BB*);
        void expandSpillFillIntrinsics(unsigned);
        void saveRestoreA0(G4_BB*);

        static const RAVarInfo defaultValues;
        std::vector<RAVarInfo> vars;
        std::vector<AugmentationMasks> varMasks;
        std::vector<G4_Declare *> UndeclaredVars;

        // fake declares for each GRF reg, used by HRA
        // note only GRFs that are used by LRA get a declare
        std::vector<G4_Declare*> GRFDclsForHRA;

        // Store all LocalLiveRange instances created so they're
        // appropriately destroyed alongwith instance of GlobalRA.
        // This needs to be a list because we'll take address of
        // its elements and std::vector cannot be used due to its
        // reallocation policy.
        std::list<LocalLiveRange> localLiveRanges;

        std::unordered_map<const G4_BB*, unsigned> subretloc;
        // map ret location to declare for call/ret
        std::map<uint32_t, G4_Declare*> retDecls;

        // store instructions that shouldnt be rematerialized.
        std::unordered_set<G4_INST*> dontRemat;

        RAVarInfo &allocVar(const G4_Declare* dcl)
        {
            auto dclid = dcl->getDeclId();
            if (dclid >= vars.size())
                vars.resize(dclid + 1);
            return vars[dclid];
        }

        const RAVarInfo &getVar(const G4_Declare* dcl) const
        {
            auto dclid = dcl->getDeclId();
            return dclid >= vars.size() ? defaultValues : vars[dclid];
        }

        // temp variable storing the FP dcl's old value
        // created in addStoreRestoreForFP
        G4_Declare* oldFPDcl = nullptr;

        // instruction to save/restore vISA FP, only present in functions
        G4_INST* saveBE_FPInst = nullptr;
        G4_INST* restoreBE_FPInst = nullptr;

        // instruction go update BE_FP, only present in functions
        G4_INST* setupBE_FP = nullptr;

        // new temps for each reference of spilled address/flag decls
        std::unordered_set<G4_Declare*> addrFlagSpillDcls;

        // store iteration number for GRA loop
        unsigned iterNo = 0;

        uint32_t numGRFSpill = 0;
        uint32_t numGRFFill = 0;

        bool spillFillIntrinUsesLSC(G4_INST* spillFillIntrin);
        void expandFillLSC(G4_BB* bb, INST_LIST_ITER& instIt);
        void expandSpillLSC(G4_BB* bb, INST_LIST_ITER& instIt);
        void expandFillNonStackcall(uint32_t numRows, uint32_t offset, short rowOffset, G4_SrcRegRegion* header, G4_DstRegRegion* resultRgn, G4_BB* bb, INST_LIST_ITER& instIt);
        void expandSpillNonStackcall(uint32_t numRows, uint32_t offset, short rowOffset, G4_SrcRegRegion* header, G4_SrcRegRegion* payload, G4_BB* bb, INST_LIST_ITER& instIt);
        void expandFillStackcall(uint32_t numRows, uint32_t offset, short rowOffset, G4_SrcRegRegion* header, G4_DstRegRegion* resultRgn, G4_BB* bb, INST_LIST_ITER& instIt);
        void expandSpillStackcall(uint32_t numRows, uint32_t offset, short rowOffset, G4_SrcRegRegion* payload, G4_BB* bb, INST_LIST_ITER& instIt);

    public:
        static unsigned sendBlockSizeCode(unsigned owordSize);

        // For current program, store caller/callee save/restore instructions
        std::unordered_set<G4_INST*> calleeSaveInsts;
        std::unordered_set<G4_INST*> calleeRestoreInsts;
        std::unordered_map<G4_INST*, std::unordered_set<G4_INST*>> callerSaveInsts;
        std::unordered_map<G4_INST*, std::unordered_set<G4_INST*>> callerRestoreInsts;
        std::unordered_map<G4_BB*, std::vector<bool>> callerSaveRegsMap;
        std::unordered_map<G4_BB*, unsigned> callerSaveRegCountMap;
        std::unordered_map<G4_BB*, std::vector<bool>> retRegsMap;
        std::vector<bool> calleeSaveRegs;
        unsigned calleeSaveRegCount = 0;

        std::unordered_map<G4_Declare*, SplitResults> splitResults;

    public:
        G4_Kernel& kernel;
        IR_Builder& builder;
        PhyRegPool& regPool;
        PointsToAnalysis& pointsToAnalysis;
        FCALL_RET_MAP fcallRetMap;

        bool useLscForSpillFill = false;
        bool useLscForNonStackCallSpillFill = false;

        VarSplitPass* getVarSplitPass() const { return kernel.getVarSplitPass(); }

        unsigned getSubRetLoc(const G4_BB* bb)
        {
            auto it = subretloc.find(bb);
            if (it == subretloc.end())
                return UNDEFINED_VAL;
            return it->second;
        }

        void setSubRetLoc(const G4_BB* bb, unsigned s) { subretloc[bb] = s; }

        bool isSubRetLocConflict(G4_BB *bb, std::vector<unsigned> &usedLoc, unsigned stackTop);
        void assignLocForReturnAddr();
        unsigned determineReturnAddrLoc(unsigned entryId, unsigned* retLoc, G4_BB* bb);
        void insertCallReturnVar();
        void insertSaveAddr(G4_BB*);
        void insertRestoreAddr(G4_BB*);
        void setIterNo(unsigned i) { iterNo = i; }
        unsigned getIterNo() const { return iterNo; }
        void fixSrc0IndirFcall();

        G4_Declare* getRetDecl(uint32_t retLoc)
        {
            auto result = retDecls.find(retLoc);
            if (result != retDecls.end())
            {
                return result->second;
            }

            const char* name = builder.getNameString(kernel.fg.mem, 24, "RET__loc%d", retLoc);
            G4_Declare* dcl = builder.createDeclareNoLookup(name, G4_GRF, 2, 1, Type_UD);


            // call destination must still be QWord aligned
            dcl->setSubRegAlign(Four_Word);
            setSubRegAlign(dcl, Four_Word);

            retDecls[retLoc] = dcl;
            return dcl;
        }

        G4_INST* getSaveBE_FPInst() const { return saveBE_FPInst; };
        G4_INST* getRestoreBE_FPInst() const { return restoreBE_FPInst; };

        G4_INST* getBEFPSetupInst() { return setupBE_FP; }
        void setBEFPSetupInst(G4_INST* i) { setupBE_FP = i; }

        static unsigned owordToGRFSize(unsigned numOwords, const IR_Builder& builder);
        static unsigned hwordToGRFSize(unsigned numHwords, const IR_Builder& builder);
        static unsigned GRFToHwordSize(unsigned numGRFs, const IR_Builder& builder);
        static unsigned GRFSizeToOwords(unsigned numGRFs, const IR_Builder& builder);
        static unsigned getHWordByteSize();

        // RA specific fields
        G4_Declare* getGRFDclForHRA(int GRFNum) const { return GRFDclsForHRA[GRFNum]; }

        G4_Declare* getOldFPDcl() const { return oldFPDcl; }

        bool isAddrFlagSpillDcl(G4_Declare* dcl) const
        {
            return addrFlagSpillDcls.count(dcl) != 0;
        }

        void addAddrFlagSpillDcl(G4_Declare* dcl)
        {
            addrFlagSpillDcls.insert(dcl);
        }

        void addUndefinedDcl(G4_Declare* dcl)
        {
            UndeclaredVars.push_back(dcl);
        }

        bool isUndefinedDcl(const G4_Declare* dcl) const
        {
            return std::find(UndeclaredVars.begin(), UndeclaredVars.end(), dcl) != UndeclaredVars.end();
        }

        unsigned getSplitVarNum(const G4_Declare* dcl) const
        {
            return getVar(dcl).numSplit;
        }

        void setSplitVarNum(const G4_Declare* dcl, unsigned val)
        {
            allocVar(dcl).numSplit = val;
        }

        unsigned getBBId(const G4_Declare* dcl) const
        {
            return getVar(dcl).bb_id;
        }

        void setBBId(const G4_Declare* dcl, unsigned id)
        {
            allocVar(dcl).bb_id = id;
        }

        bool isBlockLocal(const G4_Declare* dcl) const
        {
            return getBBId(dcl) < (UINT_MAX - 1);
        }

        G4_Declare* getSplittedDeclare(const G4_Declare* dcl) const
        {
            return getVar(dcl).splittedDCL;
        }

        void setSplittedDeclare(const G4_Declare* dcl, G4_Declare* sd)
        {
            allocVar(dcl).splittedDCL = sd;
        }

        LocalLiveRange* getLocalLR(const G4_Declare* dcl) const
        {
            return getVar(dcl).localLR;
        }

        void setLocalLR(G4_Declare* dcl, LocalLiveRange* lr)
        {
            RAVarInfo &var = allocVar(dcl);
            MUST_BE_TRUE(var.localLR == NULL, "Local live range already allocated for declaration");
            var.localLR = lr;
            lr->setTopDcl(dcl);
        }

        LSLiveRange* getSafeLSLR(const G4_Declare* dcl) const
        {
            auto dclid = dcl->getDeclId();
            assert(dclid < vars.size());
            return vars[dclid].LSLR;
        }

        LSLiveRange* getLSLR(const G4_Declare* dcl) const
        {
            return getVar(dcl).LSLR;
        }

        void setLSLR(G4_Declare* dcl, LSLiveRange* lr)
        {
            RAVarInfo &var = allocVar(dcl);
            MUST_BE_TRUE(var.LSLR == NULL, "Local live range already allocated for declaration");
            var.LSLR = lr;
            lr->setTopDcl(dcl);
        }

        void resetLSLR(const G4_Declare* dcl)
        {
            allocVar(dcl).LSLR = nullptr;
        }

        void resetLocalLR(const G4_Declare* dcl)
        {
            allocVar(dcl).localLR = nullptr;
        }

        void clearStaleLiveRanges()
        {
            for (auto dcl : kernel.Declares)
            {
                setBBId(dcl, UINT_MAX);
                resetLocalLR(dcl);
            }
        }

        void clearLocalLiveRanges()
        {
            for (auto dcl : kernel.Declares)
            {
                resetLocalLR(dcl);
            }
        }

        void recordRef(const G4_Declare* dcl)
        {
            allocVar(dcl).numRefs++;
        }

        unsigned getNumRefs(const G4_Declare* dcl) const
        {
            return getVar(dcl).numRefs;
        }

        void setNumRefs(const G4_Declare* dcl, unsigned refs)
        {
            allocVar(dcl).numRefs = refs;
        }

        BankConflict getBankConflict(const G4_Declare* dcl) const
        {
            return getVar(dcl).conflict;
        }

        void setBankConflict(const G4_Declare* dcl, BankConflict c)
        {
            allocVar(dcl).conflict = c;
        }

        G4_INST* getStartInterval(const G4_Declare* dcl) const
        {
            return getVar(dcl).startInterval;
        }

        void setStartInterval(const G4_Declare* dcl, G4_INST* inst)
        {
            allocVar(dcl).startInterval = inst;
        }

        G4_INST* getEndInterval(const G4_Declare* dcl) const
        {
            return getVar(dcl).endInterval;
        }

        void setEndInterval(const G4_Declare* dcl, G4_INST* inst)
        {
            allocVar(dcl).endInterval = inst;
        }

        const std::vector<unsigned char>& getMask(const G4_Declare* dcl) const
        {
            return getVar(dcl).mask;
        }

        void setMask(const G4_Declare* dcl, std::vector<unsigned char> m)
        {
            allocVar(dcl).mask = m;
        }

        AugmentationMasks getAugmentationMask(const G4_Declare* dcl) const
        {
            auto dclid = dcl->getDeclId();
            if (dclid >= varMasks.size())
            {
                return AugmentationMasks::Undetermined;
            }
            return varMasks[dclid];
        }

        void setAugmentationMask(const G4_Declare* dcl, AugmentationMasks m)
        {
            auto dclid = dcl->getDeclId();
            if (dclid >= varMasks.size())
                varMasks.resize(dclid + 1);
            varMasks[dclid] = m;
            if (dcl->getIsSplittedDcl())
            {
                for (const G4_Declare *subDcl : getSubDclList(dcl))
                {
                    setAugmentationMask(subDcl, m);
                }
            }
        }

        bool getHasNonDefaultMaskDef(const G4_Declare* dcl) const
        {
            return (getAugmentationMask(dcl) == AugmentationMasks::NonDefault);
        }

        void addBundleConflictDcl(const G4_Declare *dcl, const G4_Declare* subDcl, int offset)
        {
            allocVar(dcl).bundleConflicts.emplace_back(subDcl, offset);
        }

        void clearBundleConflictDcl(const G4_Declare* dcl)
        {
            allocVar(dcl).bundleConflicts.clear();
        }

        const std::vector<BundleConflict> &getBundleConflicts(const G4_Declare* dcl) const
        {
            return getVar(dcl).bundleConflicts;
        }

        unsigned get_bundle(unsigned baseReg, int offset) const
        {
            if (builder.hasPartialInt64Support())
            {
                return (((baseReg + offset) % 32) / 2);
            }
            return (((baseReg + offset) % 64) / 4);
        }

        unsigned get_bank(unsigned baseReg, int offset)
        {
            int bankID = (baseReg + offset) % 2;

            if (builder.hasTwoGRFBank16Bundles())
            {
                bankID = ((baseReg + offset) % 4) / 2;
            }


            if (builder.hasOneGRFBank16Bundles())
            {
                bankID = (baseReg + offset) % 2;
            }

            return bankID;
        }

        void addSubDcl(const G4_Declare *dcl, G4_Declare* subDcl)
        {
            allocVar(dcl).subDclList.push_back(subDcl);
        }

        void clearSubDcl(const G4_Declare* dcl)
        {
            allocVar(dcl).subDclList.clear();
        }

        const std::vector<const G4_Declare*> &getSubDclList(const G4_Declare* dcl) const
        {
            return getVar(dcl).subDclList;
        }

        unsigned getSubOffset(const G4_Declare* dcl) const
        {
            return getVar(dcl).subOff;
        }

        void setSubOffset(const G4_Declare* dcl, unsigned offset)
        {
            allocVar(dcl).subOff = offset;
        }

        G4_SubReg_Align getSubRegAlign(const G4_Declare* dcl) const
        {
            return getVar(dcl).subAlign;
        }

        void setSubRegAlign(const G4_Declare* dcl, G4_SubReg_Align subAlg)
        {
            auto& subAlign = allocVar(dcl).subAlign;
            // sub reg alignment can only be more restricted than prior setting
            MUST_BE_TRUE(subAlign == Any || subAlign == subAlg || subAlign % 2 == 0,
                ERROR_UNKNOWN);
            if (subAlign > subAlg)
            {
                MUST_BE_TRUE(subAlign % subAlg == 0, "Sub reg alignment conflict");
                // do nothing; keep the original alignment (more restricted)
            }
            else
            {
                MUST_BE_TRUE(subAlg % subAlign == 0, "Sub reg alignment conflict");
                subAlign = subAlg;
            }
        }

        bool hasAlignSetup(const G4_Declare* dcl) const
        {
            if (getVar(dcl).subAlign == G4_SubReg_Align::Any &&
                dcl->getSubRegAlign() != G4_SubReg_Align::Any)
                return false;
            return true;
        }

        bool isEvenAligned(const G4_Declare* dcl) const
        {
            return getVar(dcl).isEvenAlign;
        }

        void setEvenAligned(const G4_Declare* dcl, bool e)
        {
            allocVar(dcl).isEvenAlign = e;
        }

        BankAlign getBankAlign(const G4_Declare*) const;
        bool areAllDefsNoMask(G4_Declare*);
        void removeUnreferencedDcls();
        LocalLiveRange* GetOrCreateLocalLiveRange(G4_Declare* topdcl);

        GlobalRA(G4_Kernel& k, PhyRegPool& r, PointsToAnalysis& p2a) : kernel(k), builder(*k.fg.builder), regPool(r),
            pointsToAnalysis(p2a)
        {
            vars.resize(k.Declares.size());
            varMasks.resize(k.Declares.size());

            if (kernel.getOptions()->getOption(vISA_VerifyAugmentation))
            {
                verifyAugmentation = std::make_unique<VerifyAugmentation>();
            }

            // Need call WA for EU Fusion for non-entry function
            m_EUFusionCallWANeeded = builder.hasFusedEU()
                && builder.getOption(vISA_fusedCallWA)
                && (kernel.fg.getHasStackCalls() || kernel.hasIndirectCall());
                //&& !builder.getIsKernel(); // if caller save tmp is forced w/ M16, caller save/restore must be forced with M16.
        }

        void emitFGWithLiveness(const LivenessAnalysis& liveAnalysis) const;
        void reportSpillInfo(const LivenessAnalysis& liveness, const GraphColor& coloring) const;
        static uint32_t getRefCount(int loopNestLevel);
        bool isReRAPass();
        void updateSubRegAlignment(G4_SubReg_Align subAlign);
        bool isChannelSliced();
        void evenAlign();
        bool evenAlignNeeded(G4_Declare*);
        void getBankAlignment(LiveRange* lr, BankAlign &align);
        void printLiveIntervals();
        void reportUndefinedUses(LivenessAnalysis& liveAnalysis, G4_BB* bb, G4_INST* inst, G4_Declare* referencedDcl, std::set<G4_Declare*>& defs, std::ofstream& optreport, Gen4_Operand_Number opndNum);
        void detectNeverDefinedUses();
        void emitVarLiveIntervals();

        void determineSpillRegSize(unsigned& spillRegSize, unsigned& indrSpillRegSize);
        G4_Imm* createMsgDesc(unsigned owordSize, bool writeType, bool isSplitSend);
        void stackCallProlog();
        void saveRegs(unsigned startReg, unsigned owordSize, G4_Declare* scratchRegDcl, G4_Declare* framePtr, unsigned frameOwordOffset, G4_BB* bb, INST_LIST_ITER insertIt, std::unordered_set<G4_INST*>& group);
        void saveActiveRegs(std::vector<bool>& saveRegs, unsigned startReg, unsigned frameOffset, G4_BB* bb, INST_LIST_ITER insertIt, std::unordered_set<G4_INST*>& group);
        void addrRegAlloc();
        void flagRegAlloc();
        bool hybridRA(bool doBankConflictReduction, bool highInternalConflict, LocalRA& lra);
        void assignRegForAliasDcl();
        void removeSplitDecl();
        int coloringRegAlloc();
        void restoreRegs(unsigned startReg, unsigned owordSize, G4_Declare* scratchRegDcl, G4_Declare* framePtr, unsigned frameOwordOffset, G4_BB* bb, INST_LIST_ITER insertIt, std::unordered_set<G4_INST*>& group, bool caller);
        void restoreActiveRegs(std::vector<bool>& restoreRegs, unsigned startReg, unsigned frameOffset, G4_BB* bb, INST_LIST_ITER insertIt, std::unordered_set<G4_INST*>& group, bool caller);
        void OptimizeActiveRegsFootprint(std::vector<bool>& saveRegs);
        void OptimizeActiveRegsFootprint(std::vector<bool>& saveRegs, std::vector<bool>& retRegs);
        void addCallerSaveRestoreCode();
        void addCalleeSaveRestoreCode();
        void addGenxMainStackSetupCode();
        void addCalleeStackSetupCode();
        void addSaveRestoreCode(unsigned localSpillAreaOwordSize);
        void addCallerSavePseudoCode();
        void addCalleeSavePseudoCode();
        void addStoreRestoreToReturn();
        void markGraphBlockLocalVars();
        void verifyRA(LivenessAnalysis & liveAnalysis);
        void resetGlobalRAStates();

        void insertPhyRegDecls();

        void copyMissingAlignment()
        {
            // Insert alignment for vars created in RA
            for (auto dcl : kernel.Declares)
            {
                if (dcl->getAliasDeclare())
                    continue;

                if (!hasAlignSetup(dcl))
                {
                    // Var may be temp created in RA
                    setSubRegAlign(dcl, dcl->getSubRegAlign());
                    setEvenAligned(dcl, dcl->isEvenAlign());
                }
            }
        }

        void copyAlignment(G4_Declare* dst, G4_Declare* src)
        {
            setEvenAligned(dst, isEvenAligned(src));
            setSubRegAlign(dst, getSubRegAlign(src));
        }

        void copyAlignment()
        {
            for (auto dcl : kernel.Declares)
            {
                if (dcl->getAliasDeclare())
                    continue;

                setSubRegAlign(dcl, dcl->getSubRegAlign());
                setEvenAligned(dcl, dcl->isEvenAlign());
            }
        }

        bool isNoRemat(G4_INST* inst)
        {
            return dontRemat.find(inst) != dontRemat.end();
        }

        void addNoRemat(G4_INST* inst)
        {
            dontRemat.insert(inst);
        }
    };

    inline G4_Declare* Interference::getGRFDclForHRA(int GRFNum) const
    {
        return gra.getGRFDclForHRA(GRFNum);
    }

    class VarSplit
    {
    private:
        G4_Kernel& kernel;
        GlobalRA& gra;

        VarRange* splitVarRange(VarRange *src1, VarRange *src2, std::stack<VarRange*> *toDelete);
        void rangeListSpliting(VAR_RANGE_LIST *rangeList, G4_Operand *opnd, std::stack<VarRange*> *toDelete);
        void getHeightWidth(G4_Type type, unsigned numberElements, unsigned short &dclWidth, unsigned short &dclHeight, int &totalByteSize) const;
        void createSubDcls(G4_Kernel& kernel, G4_Declare* oldDcl, std::vector<G4_Declare*> &splitDclList);
        void insertMovesToTemp(IR_Builder& builder, G4_Declare* oldDcl, G4_Operand *dstOpnd, G4_BB* bb, INST_LIST_ITER instIter, std::vector<G4_Declare*> &splitDclList);
        void insertMovesFromTemp(G4_Kernel& kernel, G4_Declare* oldDcl, int index, G4_Operand *srcOpnd, int pos, G4_BB* bb, INST_LIST_ITER instIter, std::vector<G4_Declare*> &splitDclList);

    public:
        bool didLocalSplit = false;
        bool didGlobalSplit = false;

        void localSplit(IR_Builder& builder, G4_BB* bb);
        void globalSplit(IR_Builder& builder, G4_Kernel &kernel);
        bool canDoGlobalSplit(IR_Builder& builder, G4_Kernel &kernel, uint32_t sendSpillRefCount);

        VarSplit(GlobalRA& g) : kernel(g.kernel), gra(g)
        {

        }
    };

    //
    // Spill code clean up
    //
    typedef struct _CLEAN_NUM_PROFILE
    {
        unsigned spill_clean_num[10] {};
        unsigned fill_clean_num[10] {};
    } CLEAN_NUM_PROFILE;

    typedef struct _SCRATCH_RANGE
    {
        unsigned leftOff;
        unsigned rightOff;
    }SCRATCH_RANGE;

    typedef std::vector<SCRATCH_RANGE >   SCRATCH_RANGE_VEC;
    typedef std::vector<SCRATCH_RANGE >::iterator   SCRATCH_RANGE_VEC_ITER;

    typedef struct _RANGE
    {
        unsigned linearizedStart;
        unsigned linearizedEnd;
        bool         predicate;
    }REG_RANGE;

    typedef std::vector<REG_RANGE >   REG_RANGE_VEC;
    typedef std::vector<REG_RANGE >::iterator   REG_RANGE_VEC_ITER;

    typedef std::pair<vISA::G4_INST *, int > RENAME_OPND;
    typedef std::vector<RENAME_OPND>  RANAME_VEC;

    typedef struct _SCRATCH_ACCESS
    {
        //Basic info
#ifdef _DEBUG
        int            regNum;
#endif
        vISA::G4_Declare*    scratchDcl;       //The scrach access
        vISA::G4_Operand*    flagOpnd;
        INST_LIST_ITER inst_it;

        unsigned   linearizedStart; //linearized start register address
        unsigned   linearizedEnd;   //linearized end register address
        unsigned   leftOff;         //left offset in scratch space
        unsigned   rightOff;        //right offset in the scratch space
        unsigned   useCount;

        bool     isSpill = false;
        bool     isBlockLocal = false;
        bool     directKill = false;

        bool     regKilled = false;
        bool     regPartialKilled = false;
        bool     regOverKilled = false;
        bool     inRangePartialKilled = false;
        bool     regInUse = false;

        bool     fillInUse = false;
        bool     removeable = false;
        bool     instKilled = false;
        bool     evicted = false;
        bool     scratchDefined = false;

        unsigned   maskFlag;

        RANAME_VEC              renameOperandVec;
        SCRATCH_RANGE_VEC       killedScratchRange;
        REG_RANGE_VEC           killedRegRange;
        struct _SCRATCH_ACCESS* preScratchAccess;
        struct _SCRATCH_ACCESS* prePreScratchAccess;
        struct _SCRATCH_ACCESS* preFillAccess;

    } SCRATCH_ACCESS;

    typedef std::vector< SCRATCH_ACCESS *> SCRATCH_PTR_VEC;

    typedef vISA::std_arena_based_allocator<SCRATCH_ACCESS*> SCRATCH_PTR_ALLOCATOR;
    typedef std::list<SCRATCH_ACCESS*, SCRATCH_PTR_ALLOCATOR>           SCRATCH_PTR_LIST;
    typedef std::list<SCRATCH_ACCESS*, SCRATCH_PTR_ALLOCATOR>::iterator SCRATCH_PTR_LIST_ITER;

    class FlagSpillCleanup
    {
    private:
        GlobalRA& gra;

        void FlagLineraizedStartAndEnd(G4_Declare*  topdcl, unsigned& linearizedStart, unsigned& linearizedEnd);
        bool replaceWithPreDcl(IR_Builder& builder, SCRATCH_ACCESS* scratchAccess, SCRATCH_ACCESS* preScratchAccess);
        bool scratchKilledByPartial(SCRATCH_ACCESS* scratchAccess, SCRATCH_ACCESS* preScratchAccess);
        bool addKilledGRFRanges(unsigned linearizedStart, unsigned linearizedEnd, SCRATCH_ACCESS* scratchAccess,
            G4_Predicate*   predicate);
        bool regFullyKilled(SCRATCH_ACCESS* scratchAccess, unsigned linearizedStart, unsigned linearizedEnd,
            unsigned short maskFlag);
        bool inRangePartialKilled(SCRATCH_ACCESS* scratchAccess, unsigned linearizedStart, unsigned linearizedEnd,
            unsigned short maskFlag);
        bool regDefineAnalysis(SCRATCH_ACCESS* scratchAccess, unsigned linearizedStart, unsigned linearizedEnd,
            unsigned short maskFlag, G4_Predicate* predicate);
        void regDefineFlag(SCRATCH_PTR_LIST* scratchTraceList, G4_INST* inst, G4_Operand* opnd);
        bool regUseAnalysis(SCRATCH_ACCESS* scratchAccess, unsigned linearizedStart, unsigned linearizedEnd);
        void regUseFlag(SCRATCH_PTR_LIST* scratchTraceList, G4_INST* inst, G4_Operand* opnd, int opndIndex);
        void regUseScratch(SCRATCH_PTR_LIST * scratchTraceList, G4_INST * inst, G4_Operand * opnd, Gen4_Operand_Number opndIndex);
        void initializeScratchAccess(SCRATCH_ACCESS *scratchAccess, INST_LIST_ITER inst_it);
        bool initializeFlagScratchAccess(SCRATCH_PTR_VEC* scratchAccessList, SCRATCH_ACCESS*& scratchAccess, INST_LIST_ITER inst_it);
        void freeScratchAccess(SCRATCH_PTR_VEC *scratchAccessList);
        void flagDefine(SCRATCH_PTR_LIST& scratchTraceList, G4_INST* inst);
        void scratchUse(SCRATCH_PTR_LIST & scratchTraceList, G4_INST * inst);
        void flagUse(SCRATCH_PTR_LIST& scratchTraceList, G4_INST* inst);
        bool flagScratchDefineUse(G4_BB* bb, SCRATCH_PTR_LIST* scratchTraceList, SCRATCH_PTR_VEC* candidateList,
            SCRATCH_ACCESS* scratchAccess, CLEAN_NUM_PROFILE* clean_num_profile);
        void flagSpillFillClean(G4_BB* bb, INST_LIST_ITER inst_it, SCRATCH_PTR_VEC& scratchAccessList,
            SCRATCH_PTR_LIST& scratchTraceList, SCRATCH_PTR_VEC& candidateList, CLEAN_NUM_PROFILE* clean_num_profile);
        void regFillClean(IR_Builder& builder, G4_BB* bb, SCRATCH_PTR_VEC& candidateList,
            CLEAN_NUM_PROFILE* clean_num_profile);
        void regSpillClean(IR_Builder& builder, G4_BB* bb, SCRATCH_PTR_VEC& candidateList, CLEAN_NUM_PROFILE* clean_num_profile);

    public:
        void spillFillCodeCleanFlag(IR_Builder& builder, G4_Kernel& kernel, CLEAN_NUM_PROFILE* clean_num_profile);
        FlagSpillCleanup(GlobalRA& g) : gra(g)
        {

        }

    };

    class DynPerfModel
    {
    private:
        std::string Buffer;

    public:
        G4_Kernel& Kernel;
        unsigned int NumSpills = 0;
        unsigned int NumFills = 0;
        unsigned int NumRAIters = 0;
        unsigned long long TotalDynInst = 0;
        unsigned long long FillDynInst = 0;
        unsigned long long SpillDynInst = 0;
        // vector item at index i corresponds to nesting level i
        // #Loops at this nesting level, #Spills, #Fills
        std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> SpillFillPerNestingLevel;

        DynPerfModel(G4_Kernel& K) : Kernel(K)
        {}

        void run();
        void dump();
    };
}

// TODO: Refactor code so that stack call related enums,
// methods, etc. should be part of this class. Right now
// code is scattered across FlowGraph and GraphColor.
class StackCall
{
public:
    // Following enum holds offsets of various fields in
    // frame descriptor as per VISA ABI.
    enum class FrameDescriptorOfsets
    {
        FE_FP = vISA::IR_Builder::SubRegs_Stackcall::FE_FP * 8,
        FE_SP = vISA::IR_Builder::SubRegs_Stackcall::FE_SP * 8,
        Ret_IP = vISA::IR_Builder::SubRegs_Stackcall::Ret_IP * 4,
        Ret_EM = vISA::IR_Builder::SubRegs_Stackcall::Ret_EM * 4,
        BE_FP = vISA::IR_Builder::SubRegs_Stackcall::BE_FP * 4,
        BE_SP = vISA::IR_Builder::SubRegs_Stackcall::BE_SP * 4
    };
};

#endif // __GRAPHCOLOR_H__
