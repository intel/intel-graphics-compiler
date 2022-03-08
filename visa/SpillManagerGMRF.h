/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPILLMANAGERGMRF_H__
#define __SPILLMANAGERGMRF_H__

#include "Mem_Manager.h"
#include "G4_Opcode.h"
#include "BuildIR.h"

#include <list>
#include <utility>

// Forward declarations
namespace vISA
{
class G4_Kernel;
class G4_Declare;
class G4_Operand;
class G4_RegVar;
class G4_RegVarTransient;
class G4_DstRegRegion;
class G4_SrcRegRegion;
class G4_Imm;
class G4_Predicate;
class G4_INST;
class IR_Builder;
class LivenessAnalysis;
class Interference;
class LiveRange;
class LSLiveRange;
class PointsToAnalysis;
class GlobalRA;
class GraphColor;
}
struct RegionDesc;
// Class definitions
namespace vISA
{

class SpillManagerGRF
{
public:

    using DECLARE_LIST = std::list<G4_Declare *> ;
    using LR_LIST = std::list<LiveRange *>;
    using LSLR_LIST = std::list<LSLiveRange *>;
    using INST_LIST = std::list<G4_INST *,INST_LIST_NODE_ALLOCATOR>;
    typedef struct Edge
    {
        unsigned first;
        unsigned second;
    } EDGE;
    using INTF_LIST = std::list<EDGE>;

    // Methods

    SpillManagerGRF(
        GlobalRA&                g,
        unsigned                 spillAreaOffset,
        unsigned                 varIdCount,
        const LivenessAnalysis * lvInfo,
        LiveRange **             lrInfo,
        const Interference *     intf,
        const LR_LIST *          spilledLRs,
        unsigned                 iterationNo,
        bool                     useSpillReg,
        unsigned                 spillRegSize,
        unsigned                 indrSpillRegSize,
        bool                     enableSpillSpaceCompression,
        bool                     useScratchMsg,
        bool                     avoidDstSrcOverlap
    );

    SpillManagerGRF(
        GlobalRA& g,
        unsigned spillAreaOffset,
        unsigned varIdCount,
        const LivenessAnalysis* lvInfo,
        LSLR_LIST* spilledLSLRs,
        bool enableSpillSpaceCompression,
        bool useScratchMsg,
        bool avoidDstSrcOverlap
    );

    ~SpillManagerGRF() {}


    bool insertSpillFillCode(
        G4_Kernel*         kernel,
        PointsToAnalysis& pointsToAnalysis
    );

    void expireRanges(unsigned int idx, std::list<LSLiveRange*>* liveList);

    void updateActiveList(LSLiveRange* lr, std::list<LSLiveRange*>* liveList);

    bool spillLiveRanges(G4_Kernel* kernel);

    unsigned getNumGRFSpill() const { return numGRFSpill; }
    unsigned getNumGRFFill() const { return numGRFFill; }
    unsigned getNumGRFMove() const { return numGRFMove; }
    // return the next cumulative logical offset. This does not non-spilled stuff like
    // private variables placed by IGC (marked by spill_mem_offset)
    // this should only be called after insertSpillFillCode()
    uint32_t getNextOffset() const { return nextSpillOffset_; }
    // return the cumulative scratch space offset for the next spilled variable.
    // This adjusts for scratch space reserved for file scope vars and IGC/GT-pin
    uint32_t getNextScratchOffset() const
    {
        int offset = nextSpillOffset_;
        getSpillOffset(offset);
        return offset;
    }

    // convert zero-based logical offset into the scratch space offset.
    void getSpillOffset(int& logicalOffset) const
    {
        logicalOffset += globalScratchOffset;
    }

    static std::tuple<uint32_t, G4_ExecSize> createSpillSendMsgDescOWord(
        const IR_Builder& builder, unsigned int height);

private:
    G4_Declare* getOrCreateAddrSpillFillDcl(G4_RegVar* addrDcl, G4_Declare* spilledAddrTakenDcl, G4_Kernel* kernel);
    bool handleAddrTakenSpills(G4_Kernel * kernel, PointsToAnalysis& pointsToAnalysis);
    unsigned int handleAddrTakenLSSpills(G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis);
    void insertAddrTakenSpillFill(G4_Kernel * kernel, PointsToAnalysis& pointsToAnalysis);
    void insertAddrTakenLSSpillFill(G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis);
    void insertAddrTakenSpillAndFillCode(
        G4_Kernel* kernel, G4_BB* bb, INST_LIST::iterator inst_it,
        G4_Operand* opnd, PointsToAnalysis& pointsToAnalysis,
        bool spill, unsigned int bbid);
    void insertAddrTakenLSSpillAndFillCode(G4_Kernel* kernel, G4_BB* bb,
        INST_LIST::iterator inst_it, G4_Operand* opnd,
        PointsToAnalysis& pointsToAnalysis, bool spill, unsigned int bbid);
    void prunePointsTo(G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis);

    void prunePointsToLS(G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis);

    bool isComprInst (G4_INST * inst) const;

    bool isMultiRegComprSource (
        G4_SrcRegRegion* src,
        G4_INST *        inst) const;

    unsigned getSendRspLengthBitOffset() const;

    unsigned getSendMaxResponseLength() const;

    static unsigned getSendMsgLengthBitOffset();

    unsigned getSendMaxMessageLength() const;

    static unsigned getSendDescDataSizeBitOffset();

    unsigned getSendReadTypeBitOffset() const;

    static unsigned getSendWriteTypeBitOffset();

    unsigned getSendScReadType() const;

    unsigned getSendScWriteType() const;

    unsigned getSendOwordReadType() const;
    static unsigned getSendOwordWriteType();
    unsigned getSendExDesc(bool isWrite, bool isScatter) const;

    unsigned getSpillIndex(G4_RegVar *  spilledRegVar);

    unsigned getFillIndex(G4_RegVar *  spilledRegVar);

    unsigned getTmpIndex(G4_RegVar *  spilledRegVar);

    unsigned getMsgSpillIndex(G4_RegVar *  spilledRegVar);

    unsigned getMsgFillIndex(G4_RegVar *  spilledRegVar);

    unsigned getAddrSpillFillIndex(G4_RegVar *  spilledRegVar);

    const char *
    createImplicitRangeName (
        const char * baseName,
        G4_RegVar *  spilledRegVar,
        unsigned     index
    );

    G4_RegVar *
    getReprRegVar (
        G4_RegVar * regVar
    ) const;

    template <class REGION_TYPE>
    G4_RegVar *
    getRegVar (
        REGION_TYPE * region
    ) const;

    G4_RegFileKind
    getRFType (
        G4_RegVar * regvar
    ) const;

    template <class REGION_TYPE>
    G4_RegFileKind getRFType(REGION_TYPE * region) const;

    template <class REGION_TYPE>
    unsigned getRegionOriginOffset(REGION_TYPE * region) const;

    unsigned grfMask() const;

    unsigned hwordMask() const;

    unsigned owordMask() const;

    unsigned dwordMask() const;

    bool owordAligned(unsigned offset) const;

    bool dwordAligned(unsigned offset) const;

    static unsigned cdiv(unsigned dvd, unsigned dvr);

    G4_RegVar *getRegVar(unsigned id) const;

    bool shouldSpillRegister(G4_RegVar * regVar) const;

    unsigned getByteSize(G4_RegVar * regVar) const;

    template <class REGION_TYPE>
    unsigned getSegmentDisp(REGION_TYPE * region, G4_ExecSize   execSize);

    unsigned getDisp(G4_RegVar *   lRange);

    template <class REGION_TYPE>
    unsigned getRegionDisp(REGION_TYPE * region);

    bool spillMemLifetimeInterfere(unsigned i, unsigned j) const;

    unsigned calculateSpillDisp(G4_RegVar * lRange) const;

    unsigned calculateSpillDispForLS(G4_RegVar* regVar) const;

    template <class REGION_TYPE>
    unsigned getMsgType(REGION_TYPE * region, G4_ExecSize   execSize);

    template <class REGION_TYPE>
    bool isUnalignedRegion(REGION_TYPE * region, G4_ExecSize   execSize);

    template <class REGION_TYPE>
    void calculateEncAlignedSegment(
        REGION_TYPE * region,
        G4_ExecSize   execSize,
        unsigned &    start,
        unsigned &    end,
        unsigned &    type);

    template <class REGION_TYPE>
    unsigned getEncAlignedSegmentByteSize(REGION_TYPE * region, G4_ExecSize execSize);

    template <class REGION_TYPE>
    unsigned getEncAlignedSegmentDisp(REGION_TYPE * region, G4_ExecSize execSize);

    template <class REGION_TYPE>
    unsigned getEncAlignedSegmentMsgType(REGION_TYPE * region, G4_ExecSize execSize);

    template <class REGION_TYPE>
    unsigned getSegmentByteSize(REGION_TYPE * region, G4_ExecSize   execSize);

    unsigned getRegionByteSize (G4_DstRegRegion * region, G4_ExecSize execSize) const;

    unsigned getRegionByteSize(G4_SrcRegRegion * region, G4_ExecSize execSize) const;

    bool isScalarReplication(G4_SrcRegRegion * region) const;

    bool repeatSIMD16or32Source(G4_SrcRegRegion * region) const;

    G4_Declare *
    createRangeDeclare(
        const char*    name,
        G4_RegFileKind regFile,
        unsigned short nElems,
        unsigned short nRows,
        G4_Type        type,
        DeclareType    kind,
        G4_RegVar *    base,
        G4_Operand *   repRegion,
        G4_ExecSize    execSize);

    template <class REGION_TYPE>
    G4_Declare *
    createTransientGRFRangeDeclare(
        REGION_TYPE * region,
        const char  * name,
        unsigned      index,
        G4_ExecSize   execSize,
        G4_INST     * inst
    );

    G4_Declare *
    createPostDstSpillRangeDeclare(G4_INST * sendOut);

    G4_Declare *
    createSpillRangeDeclare(
        G4_DstRegRegion * spillRegion,
        G4_ExecSize       execSize,
        G4_INST         * inst);

    G4_Declare *
    createGRFFillRangeDeclare (
        G4_SrcRegRegion * fillRegion,
        G4_ExecSize       execSize,
        G4_INST         * inst
    );

    G4_Declare *
    createSendFillRangeDeclare (
        G4_SrcRegRegion * filledRegion,
        G4_INST *         sendInst
    );

    G4_Declare *
    createTemporaryRangeDeclare(
        G4_DstRegRegion * fillRegion,
        G4_ExecSize       execSize,
        bool              forceSegmentAlignment = false);

    G4_DstRegRegion *
    createSpillRangeDstRegion(
        G4_RegVar *       spillRangeRegVar,
        G4_DstRegRegion * spilledRegion,
        G4_ExecSize       execSize,
        unsigned          regOff = 0);

    G4_SrcRegRegion *
    createFillRangeSrcRegion(
        G4_RegVar *       fillRangeRegVar,
        G4_SrcRegRegion * filledRegion,
        G4_ExecSize       execSize);

    G4_SrcRegRegion *
    createTemporaryRangeSrcRegion(
        G4_RegVar *       tmpRangeRegVar,
        G4_DstRegRegion * spilledRegion,
        G4_ExecSize       execSize,
        unsigned          regOff = 0);

    G4_SrcRegRegion *
    createBlockSpillRangeSrcRegion(
        G4_RegVar *       spillRangeRegVar,
        unsigned          regOff = 0,
        unsigned          subregOff = 0);

    G4_Declare *
    createMRangeDeclare(G4_RegVar * regVar);

    G4_Declare *
    createMRangeDeclare(G4_DstRegRegion * region, G4_ExecSize execSize);

    G4_Declare *
    createMRangeDeclare(G4_SrcRegRegion * region, G4_ExecSize execSize);

    G4_DstRegRegion *
    createMPayloadBlockWriteDstRegion(
        G4_RegVar *       grfRange,
        unsigned          regOff = 0,
        unsigned          subregOff = 0);

    G4_DstRegRegion * createMHeaderInputDstRegion(
        G4_RegVar *       grfRange,
        unsigned          subregOff = 0);

    G4_DstRegRegion *
    createMHeaderBlockOffsetDstRegion(G4_RegVar * grfRange);


    G4_SrcRegRegion * createInputPayloadSrcRegion();

    G4_Declare * initMHeader(G4_Declare * mRangeDcl);

    G4_Declare * createAndInitMHeader(G4_RegVar * regVar);

    template <class REGION_TYPE>
    G4_Declare * initMHeader(
        G4_Declare *      mRangeDcl,
        REGION_TYPE *     region,
        G4_ExecSize       execSize);

    template <class REGION_TYPE>
    G4_Declare * createAndInitMHeader(
        REGION_TYPE *      region,
        G4_ExecSize        execSize);

    void sendInSpilledRegVarPortions(
        G4_Declare *      fillRangeDcl,
        G4_Declare *      mRangeDcl,
        unsigned          regOff,
        unsigned          height,
        unsigned          srcRegOff = 0);

    void sendOutSpilledRegVarPortions(
        G4_Declare *      spillRangeDcl,
        G4_Declare *      mRangeDcl,
        unsigned          regOff,
        unsigned          height,
        unsigned          srcRegOff = 0);

    void initMWritePayload(
        G4_Declare *      spillRangeDcl,
        G4_Declare *      mRangeDcl,
        unsigned          regOff,
        unsigned          height
    );

    void
    initMWritePayload (
        G4_Declare *      spillRangeDcl,
        G4_Declare *      mRangeDcl,
        G4_DstRegRegion * spilledRangeRegion,
        G4_ExecSize       execSize,
        unsigned          regOff = 0);

    static unsigned blockSendBlockSizeCode(unsigned regOff);

    unsigned scatterSendBlockSizeCode(unsigned regOff) const;

    G4_Imm * createSpillSendMsgDesc(
        unsigned        regOff,
        unsigned        height,
        G4_ExecSize &   execSize,
        G4_RegVar*      base = NULL);

    std::tuple<G4_Imm*, G4_ExecSize> createSpillSendMsgDesc(
        G4_DstRegRegion * spilledRangeRegion,
        G4_ExecSize     execSize);

    G4_INST * createAddFPInst(
        G4_ExecSize       execSize,
        G4_DstRegRegion * dst,
        G4_Operand *      src
    );

    G4_INST * createMovInst(
        G4_ExecSize       execSize,
        G4_DstRegRegion * dst,
        G4_Operand *      src,
        G4_Predicate *    predicate = NULL,
        G4_InstOpts       options = InstOpt_WriteEnable
    );

    G4_INST * createSendInst(
        G4_ExecSize       execSize,
        G4_DstRegRegion * postDst,
        G4_SrcRegRegion * payload,
        G4_Imm *          desc,
        SFID              funcID,
        bool              isWrite,
        G4_InstOpts       option);

    bool shouldPreloadSpillRange(
        G4_INST* instContext,
        G4_BB* parentBB);

    void preloadSpillRange(
        G4_Declare *      spillRangeDcl,
        G4_Declare *      mRangeDcl,
        G4_DstRegRegion * spilledRangeRegion,
        G4_ExecSize       execSize);

    G4_INST * createSpillSendInstr(
        G4_Declare *      spillRangeDcl,
        G4_Declare *      mRangeDcl,
        unsigned          regOff,
        unsigned          height,
        unsigned          spillOff);

    G4_INST *createSpillSendInstr (
        G4_Declare *      spillRangeDcl,
        G4_Declare *      mRangeDcl,
        G4_DstRegRegion * spilledRangeRegion,
        G4_ExecSize       execSize,
        unsigned          option);

    G4_Imm *createFillSendMsgDesc (
        unsigned          regOff,
        unsigned          height,
        G4_ExecSize &     execSize,
        G4_RegVar* base = NULL);

    template <class REGION_TYPE>
    G4_Imm* createFillSendMsgDesc(
        REGION_TYPE* filledRangeRegion,
        G4_ExecSize     execSize
    );

    G4_INST * createFillSendInstr (
        G4_Declare *      fillRangeDcl,
        G4_Declare *      mRangeDcl,
        unsigned          regOff,
        unsigned          height,
        unsigned          spillOff);

    G4_INST * createFillSendInstr(
        G4_Declare *       fillRangeDcl,
        G4_Declare *       mRangeDcl,
        G4_SrcRegRegion *  filledRangeRegion,
        G4_ExecSize        execSize);

    G4_SrcRegRegion* getLSCSpillFillHeader(
        G4_Declare* mRangeDcl,
        const G4_Declare *fp,
        int offset);

    G4_INST* createLSCSpill(
        G4_Declare* spillRangeDcl,
        G4_Declare* mRangeDcl,
        unsigned          regOff,
        unsigned          height,
        unsigned          spillOff);

    G4_INST* createLSCSpill(
        G4_Declare* spillRangeDcl,
        G4_Declare* mRangeDcl,
        G4_DstRegRegion* spilledRangeRegion,
        G4_ExecSize       execSize,
        unsigned          option);

    G4_INST* createLSCFill(
        G4_Declare* fillRangeDcl,
        G4_Declare* mRangeDcl,
        unsigned          regOff,
        unsigned          height,
        unsigned          spillOff);

    G4_INST* createLSCFill(
        G4_Declare* fillRangeDcl,
        G4_Declare* mRangeDcl,
        G4_SrcRegRegion* filledRangeRegion,
        G4_ExecSize        execSize);

    void replaceSpilledRange(
        G4_Declare *      spillRangeDcl,
        G4_DstRegRegion * spilledRegion,
        G4_INST *         spilledInst,
        uint32_t subregOff);

    void replaceFilledRange(
        G4_Declare *      fillRangeDcl,
        G4_SrcRegRegion * filledRegion,
        G4_INST *         filledInst);

    void insertSpillRangeCode(
        INST_LIST::iterator spilledInstIter,
        G4_BB* bb
    );

    INST_LIST::iterator insertSendFillRangeCode(
        G4_SrcRegRegion *   filledRegion,
        INST_LIST::iterator filledInstIter,
        G4_BB* bb);

    void insertFillGRFRangeCode(
        G4_SrcRegRegion *   filledRegion,
        INST_LIST::iterator filledInstIter,
        G4_BB* bb);

    void *allocMem (unsigned size) const;

    SpillManagerGRF(const SpillManagerGRF & other);

    bool useSplitSend() const;

    int getHWordEncoding(int numHWord)
    {
        switch (numHWord)
        {
        case 1:
            return 0;
        case 2:
            return 1;
        case 4:
            return 2;
        case 8:
            return 3;
        default:
            MUST_BE_TRUE(false, "only 1/2/4/8 HWords are supported");
            return 0;
        }
    }

    ChannelMask getChMaskForSpill(int numHWord) const
    {
        switch (numHWord)
        {
        case 1:
        case 2:
            return ChannelMask::createFromAPI(CHANNEL_MASK_R);
        case 4:
            return ChannelMask::createFromAPI(CHANNEL_MASK_RG);
        case 8:
            return ChannelMask::createFromAPI(CHANNEL_MASK_RGBA);
        default:
            assert(false && "illegal spill size");
            return ChannelMask::createFromAPI(CHANNEL_MASK_R);
        }
    }

    // Data
    GlobalRA&                gra;
    IR_Builder *             builder_;
    unsigned                 varIdCount_;
    unsigned                 latestImplicitVarIdCount_;
    const LivenessAnalysis * lvInfo_;
    LiveRange **             lrInfo_;
    const LR_LIST *          spilledLRs_;
    LSLR_LIST*               spilledLSLRs_;
    unsigned *               spillRangeCount_;
    unsigned *               fillRangeCount_;
    unsigned *               tmpRangeCount_;
    unsigned *               msgSpillRangeCount_;
    unsigned *               msgFillRangeCount_;
    unsigned *              addrSpillFillRangeCount_;
    unsigned                 nextSpillOffset_;
    unsigned                 iterationNo_;
    unsigned                 bbId_ = UINT_MAX;
    unsigned                 spillAreaOffset_;
    bool                     doSpillSpaceCompression;

    bool                     failSafeSpill_;
    unsigned                 spillRegStart_;
    unsigned                 indrSpillRegStart_;
    unsigned                 spillRegOffset_;
    LSLR_LIST                activeLR_;
    std::unordered_set<G4_DstRegRegion*> noRMWNeeded;

    const Interference *     spillIntf_;
    vISA::Mem_Manager              mem_;

    // The number of GRF spill.
    unsigned numGRFSpill = 0;

    // The number of GRF fill.
    unsigned numGRFFill = 0;

    // The number of mov.
    unsigned numGRFMove = 0;

    // CISA instruction id of current instruction
    G4_INST* curInst;

    int globalScratchOffset;

    bool useScratchMsg_;
    bool avoidDstSrcOverlap_;
    // spilled declares that represent a scalar immediate (created due to encoding restrictions)
    // We rematerialize the immediate value instead of spill/fill them
    std::unordered_map<G4_Declare*, G4_Imm*> scalarImmSpill;

    VarReferences refs;

    // analysis pass to assist in spill/fill code gen
    // currently it identifies scalar imm variables that should be re-mat
    // later on we can add detection to avoid unncessary read-modify-write for spills
    void runSpillAnalysis();

    bool checkUniqueDefAligned(G4_DstRegRegion* dst, G4_BB* defBB);
    bool checkDefUseDomRel(G4_DstRegRegion* dst, G4_BB* bb);
    void updateRMWNeeded();

    bool useLSCMsg = false;
    bool useLscNonstackCall = false;

    bool headerNeeded() const
    {
        bool needed = true;

        if (useScratchMsg_ && builder_->getPlatform() >= GENX_SKL)
            needed = false;

        if (builder_->kernel.fg.getHasStackCalls() ||
            builder_->kernel.fg.getIsStackCallFunc())
            needed = false;

        if (useLSCMsg)
            needed = false;

        return needed;
    }

    // return true if offset for spill/fill message needs to be GRF-aligned
    bool needGRFAlignedOffset() const
    {
        return useScratchMsg_ || useSplitSend();
    }
}; // class SpillManagerGRF

   // Check if the destination region is discontiguous or not.
   // A destination region is discontiguous if there are portions of the
   // region that are not written and unaffected.
static inline bool isDisContRegion(G4_DstRegRegion * region, unsigned execSize)
{
    // If the horizontal stride is greater than 1, then it has gaps.
    // NOTE: Horizontal stride of 0 is not allowed for destination regions.
    return region->getHorzStride() != 1;
}

// Check if the source region is discontiguous or not.
// A source region is discontiguous in there are portions of the region
// that are not read.
static inline bool isDisContRegion(G4_SrcRegRegion * region, unsigned execSize)
{
    return region->getRegion()->isContiguous(execSize);
}


// Check if the region is partial or not, i.e does it read/write the
// whole segment.
template <class REGION_TYPE>
static inline bool isPartialRegion(REGION_TYPE * region, unsigned execSize)
{
    // If the region is discontiguous then it is partial.
    if (isDisContRegion(region, execSize)) {
        return true;
    }
    else
    {
        return false;
    }
}

G4_SrcRegRegion* getSpillFillHeader(IR_Builder& builder, G4_Declare* decl);

// Class used to analyze spill/fill decisions
class SpillAnalysis
{
public:
    SpillAnalysis() = default;
    ~SpillAnalysis();

    void Dump(std::ostream& OS = std::cerr);
    void DumpHistogram(std::ostream& OS = std::cerr);

    unsigned int GetDistance(G4_Declare* Dcl);
    void LoadAugIntervals(DECLARE_LIST&, GlobalRA&);
    void LoadDegree(G4_Declare* Dcl, unsigned int degree);

    void SetLivenessAnalysis(LivenessAnalysis* L) { LA = L; }
    void SetGraphColor(GraphColor* C) { GC = C; }
    void SetSpillManager(SpillManagerGRF* S) { SM = S; }

    void Clear();

    void Do(LivenessAnalysis* L, GraphColor* C, SpillManagerGRF* S);

private:
    VarReferences* Refs = nullptr;

    LivenessAnalysis* LA = nullptr;
    GraphColor* GC = nullptr;
    SpillManagerGRF* SM = nullptr;

    std::unordered_map<G4_Declare*, std::pair<G4_INST*, G4_INST*>> AugIntervals;
    std::unordered_map<G4_Declare*, unsigned int> DclDegree;

    std::vector<G4_BB*> GetLiveBBs(G4_Declare*, std::unordered_map<G4_INST*, G4_BB*>&);
    std::vector<G4_BB*> GetIntervalBBs(G4_INST* Start, G4_INST* End, std::unordered_map<G4_INST*, G4_BB*>& InstBBMap);
};

} // vISA::

#endif // __SPILLMANAGERGMRF_H__
