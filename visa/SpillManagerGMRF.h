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
class PointsToAnalysis;
class GlobalRA;
}
struct RegionDesc;
// Class definitions
namespace vISA
{
class SpillManagerGRF
{
public:

    // Types

    typedef std::list < G4_Declare * > DECLARE_LIST;
    typedef std::list < LiveRange * > LR_LIST;
    typedef std::list < G4_INST *, INST_LIST_NODE_ALLOCATOR > INST_LIST;
    typedef struct Edge
    {
        unsigned first;
        unsigned second;
    } EDGE;
    typedef std::list < EDGE > INTF_LIST;

    // Methods

    SpillManagerGRF (
        GlobalRA&                g,
        unsigned                 spillAreaOffset,
        unsigned                 varIdCount,
        const LivenessAnalysis * lvInfo,
        LiveRange **             lrInfo,
        Interference *           intf,
        std::vector<EDGE> &      prevIntfEdges,
        LR_LIST &                spilledLRs,
        unsigned                 iterationNo,
        bool                     useSpillReg,
        unsigned                 spillRegSize,
        unsigned                 indrSpillRegSize,
        bool                     enableSpillSpaceCompression,
        bool                     useScratchMsg
    );

    ~SpillManagerGRF () {}

    bool
    insertSpillFillCode    (
        G4_Kernel*         kernel,
        PointsToAnalysis& pointsToAnalysis
    );

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

    static unsigned int createSpillSendMsgDescOWord(
        unsigned int height,
        unsigned int& execSize);

private:

    // Methods

    bool handleAddrTakenSpills( G4_Kernel * kernel, PointsToAnalysis& pointsToAnalysis );
    void insertAddrTakenSpillFill( G4_Kernel * kernel, PointsToAnalysis& pointsToAnalysis );
    void insertAddrTakenSpillAndFillCode( G4_Kernel* kernel, G4_BB* bb, INST_LIST::iterator inst_it,
        G4_Operand* opnd, PointsToAnalysis& pointsToAnalysis, bool spill, unsigned int bbid);
    void prunePointsTo( G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis );

    void
    computeSpillIntf (
    );

    unsigned
    getMaxExecSize (
        G4_Operand * operand
    ) const;

    bool
    isComprInst (
        G4_INST * inst
    ) const;

    bool
    isMultiRegComprSource (
        G4_SrcRegRegion* src,
        G4_INST *        inst
    ) const;

    unsigned
    getSendRspLengthBitOffset () const;

    unsigned
    getSendMaxResponseLength () const;

    static unsigned
    getSendMsgLengthBitOffset ();

    unsigned
    getSendMaxMessageLength () const;

    static unsigned
    getSendDescDataSizeBitOffset ();

    unsigned
    getSendReadTypeBitOffset () const;

    static unsigned
    getSendWriteTypeBitOffset ();

    unsigned
    getSendScReadType () const;

    unsigned
    getSendScWriteType () const;

    unsigned getSendOwordReadType() const;
    static unsigned getSendOwordWriteType();
    unsigned getSendExDesc(bool isWrite, bool isScatter) const;

    unsigned
    getSpillIndex (
        G4_RegVar *  spilledRegVar
    );

    unsigned
    getFillIndex (
        G4_RegVar *  spilledRegVar
    );

    unsigned
    getTmpIndex (
        G4_RegVar *  spilledRegVar
    );

    unsigned
    getMsgSpillIndex (
        G4_RegVar *  spilledRegVar
    );

    unsigned
    getMsgFillIndex (
        G4_RegVar *  spilledRegVar
    );

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
    G4_RegFileKind
    getRFType (
        REGION_TYPE * region
    ) const;

    template <class REGION_TYPE>
    unsigned
    getRegionOriginOffset (
        REGION_TYPE * region
    ) const;

    unsigned
    grfMask() const;

    unsigned
    hwordMask () const;

    unsigned
    owordMask () const;

    unsigned
    dwordMask () const;

    bool
    owordAligned (
        unsigned offset
    ) const;

    bool
    dwordAligned (
        unsigned offset
    ) const;

    static unsigned
    cdiv (
        unsigned dvd,
        unsigned dvr
    );

    G4_RegVar *
    getRegVar (
        unsigned id
    ) const;

    bool
    shouldSpillRegister (
        G4_RegVar * regVar
    ) const;

    unsigned
    getByteSize (
        G4_RegVar * regVar
    ) const;

    template <class REGION_TYPE>
    unsigned
    getSegmentDisp (
        REGION_TYPE * region,
        unsigned      execSize
    ) ;

    unsigned
    getDisp (
        G4_RegVar *   lRange
    ) ;

    template <class REGION_TYPE>
    unsigned
    getRegionDisp (
        REGION_TYPE * region
    ) ;

    bool
    spillMemLifetimeInterfere (
        unsigned i,
        unsigned j
    ) const;

    unsigned
    calculateSpillDisp (
        G4_RegVar * lRange
    ) const;

    template <class REGION_TYPE>
    unsigned
    getMsgType (
        REGION_TYPE * region,
        unsigned      execSize
    );

    template <class REGION_TYPE>
    bool
    isUnalignedRegion (
        REGION_TYPE * region,
        unsigned      execSize
    );

    template <class REGION_TYPE>
    void
    calculateEncAlignedSegment (
        REGION_TYPE * region,
        unsigned      execSize,
        unsigned &    start,
        unsigned &    end,
        unsigned &    type
    );

    template <class REGION_TYPE>
    unsigned
    getEncAlignedSegmentByteSize (
        REGION_TYPE * region,
        unsigned      execSize
    );

    template <class REGION_TYPE>
    unsigned
    getEncAlignedSegmentDisp (
        REGION_TYPE * region,
        unsigned      execSize
    );

    template <class REGION_TYPE>
    unsigned
    getEncAlignedSegmentMsgType (
        REGION_TYPE * region,
        unsigned      execSize
    );

    template <class REGION_TYPE>
    unsigned
    getSegmentByteSize (
        REGION_TYPE * region,
        unsigned      execSize
    );

    unsigned
    getRegionByteSize (
        G4_DstRegRegion * region,
        unsigned          execSize
    ) const;

    unsigned
    getRegionByteSize (
        G4_SrcRegRegion * region,
        unsigned          execSize
    ) const;

    bool
    isScalarReplication (
        G4_SrcRegRegion * region
    ) const;

    bool
    repeatSIMD16or32Source (
        G4_SrcRegRegion * region
    ) const;

    G4_Declare *
    createRangeDeclare (
        const char*    name,
        G4_RegFileKind regFile,
        unsigned short nElems,
        unsigned short nRows,
        G4_Type        type,
        DeclareType    kind,
        G4_RegVar *    base,
        G4_Operand *   repRegion,
        unsigned       execSize
    );

    template <class REGION_TYPE>
    G4_Declare *
    createTransientGRFRangeDeclare (
        REGION_TYPE * region,
        const char  * name,
        unsigned      index,
        unsigned      execSize,
        G4_INST     * inst
    );

    G4_Declare *
    createPostDstSpillRangeDeclare (
        G4_INST *         sendOut,
        G4_DstRegRegion * spillRegion
    );

    G4_Declare *
    createSpillRangeDeclare (
        G4_DstRegRegion * spillRegion,
        unsigned          execSize,
        G4_INST         * inst
    );

    G4_Declare *
    createGRFFillRangeDeclare (
        G4_SrcRegRegion * fillRegion,
        unsigned          execSize,
        G4_INST         * inst
    );

    G4_Declare *
    createSendFillRangeDeclare (
        G4_SrcRegRegion * filledRegion,
        G4_INST *         sendInst
    );

    G4_Declare *
    createTemporaryRangeDeclare (
        G4_DstRegRegion * fillRegion,
        unsigned          execSize,
        bool              forceSegmentAlignment = false
    );

    G4_DstRegRegion *
    createSpillRangeDstRegion (
        G4_RegVar *       spillRangeRegVar,
        G4_DstRegRegion * spilledRegion,
        unsigned          execSize,
        unsigned          regOff = 0
    );

    G4_SrcRegRegion *
    createFillRangeSrcRegion (
        G4_RegVar *       fillRangeRegVar,
        G4_SrcRegRegion * filledRegion,
        unsigned          execSize
    );

    G4_SrcRegRegion *
    createTemporaryRangeSrcRegion (
        G4_RegVar *       tmpRangeRegVar,
        G4_DstRegRegion * spilledRegion,
        uint16_t          execSize,
        unsigned          regOff = 0
    );

    G4_SrcRegRegion *
    createBlockSpillRangeSrcRegion (
        G4_RegVar *       spillRangeRegVar,
        unsigned          regOff            = 0,
        unsigned          subregOff         = 0
    );

    G4_Declare *
    createMRangeDeclare (
        G4_RegVar *          regVar
    );

    G4_Declare *
    createMRangeDeclare (
        G4_DstRegRegion * region,
        unsigned          execSize
    );

    G4_Declare *
    createMRangeDeclare (
        G4_SrcRegRegion * region,
        unsigned          execSize
    );

    G4_DstRegRegion *
    createMPayloadBlockWriteDstRegion (
        G4_RegVar *       grfRange,
        unsigned          regOff = 0,
        unsigned          subregOff = 0
    );

    G4_DstRegRegion *
    createMHeaderInputDstRegion (
        G4_RegVar *       grfRange,
        unsigned          subregOff = 0
    );

    G4_DstRegRegion *
    createMHeaderBlockOffsetDstRegion (
        G4_RegVar *          grfRange
    );


    G4_SrcRegRegion *
    createInputPayloadSrcRegion ();

    G4_Declare *
    initMHeader (
        G4_Declare *      mRangeDcl
    );

    G4_Declare *
    createAndInitMHeader (
        G4_RegVar *          regVar
    );

    template <class REGION_TYPE>
    G4_Declare *
    initMHeader (
        G4_Declare *      mRangeDcl,
        REGION_TYPE *      region,
        unsigned          execSize
    );

    template <class REGION_TYPE>
    G4_Declare *
    createAndInitMHeader (
        REGION_TYPE *      region,
        unsigned          execSize
    );

    void
    sendInSpilledRegVarPortions (
        G4_Declare *      fillRangeDcl,
        G4_Declare *      mRangeDcl,
        unsigned          regOff,
        unsigned          height,
        unsigned          srcRegOff = 0
    );

    void
    sendOutSpilledRegVarPortions (
        G4_Declare *      spillRangeDcl,
        G4_Declare *      mRangeDcl,
        unsigned          regOff,
        unsigned          height,
        unsigned          srcRegOff = 0
    );

    void
    initMWritePayload (
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
        unsigned          execSize,
        unsigned          regOff = 0
    );

    static unsigned
    blockSendBlockSizeCode (
        unsigned        regOff
    );

    unsigned
    scatterSendBlockSizeCode (
        unsigned        regOff
    ) const;

    G4_Imm *
    createSpillSendMsgDesc (
        unsigned        regOff,
        unsigned        height,
        unsigned &      execSize,
        G4_RegVar* base = NULL
    );

    G4_Imm *
    createSpillSendMsgDesc (
        G4_DstRegRegion * spilledRangeRegion,
        unsigned &        execSize
    );

    G4_Imm *
        createSpillSendMsgDesc(
        int size,
        int offset
        );

    G4_INST *
    createAddFPInst (
        unsigned char      execSize,
        G4_DstRegRegion *      dst,
        G4_Operand *      src
    );

    G4_INST *
    createMovInst (
        unsigned char      execSize,
        G4_DstRegRegion *      dst,
        G4_Operand *      src,
        G4_Predicate *    predicate = NULL,
        unsigned int      options = InstOpt_WriteEnable
    );

    G4_INST *
        createSendInst(
        unsigned char      execSize,
        G4_DstRegRegion * postDst,
        G4_SrcRegRegion * payload,
        G4_Imm *          desc,
        SFID funcID,
        bool              isWrite,
        unsigned          option
        );

    bool shouldPreloadSpillRange(
        G4_INST* instContext,
        G4_BB* parentBB
    );

    void
    preloadSpillRange (
        G4_Declare *      spillRangeDcl,
        G4_Declare *      mRangeDcl,
        G4_DstRegRegion * spilledRangeRegion,
        uint8_t     execSize
    );

    G4_INST *
    createSpillSendInstr (
        G4_Declare *      spillRangeDcl,
        G4_Declare *      mRangeDcl,
        unsigned          regOff,
        unsigned          height,
        unsigned          srcRegOff = 0
    );

    G4_INST *
    createSpillSendInstr (
        G4_Declare *      spillRangeDcl,
        G4_Declare *      mRangeDcl,
        G4_DstRegRegion * spilledRangeRegion,
        unsigned          execSize,
        unsigned          option
    );

    G4_Imm *
    createFillSendMsgDesc (
        unsigned          regOff,
        unsigned          height,
        unsigned &        execSize,
        G4_RegVar* base = NULL
    );

    template <class REGION_TYPE>
    G4_Imm *
    createFillSendMsgDesc (
        REGION_TYPE *     filledRangeRegion,
        unsigned &        execSize
    );

    G4_Imm*
        createFillSendMsgDesc(
        int size,
        int memOffset);

    G4_INST *
        createFillSendInstr (
        G4_Declare *      fillRangeDcl,
        G4_Declare *      mRangeDcl,
        unsigned          regOff,
        unsigned          height,
        unsigned          srcRegOff = 0
    );

    G4_INST *
    createFillSendInstr (
        G4_Declare *       fillRangeDcl,
        G4_Declare *       mRangeDcl,
        G4_SrcRegRegion *  filledRangeRegion,
        unsigned           execSize,
        unsigned           regOff = 0
    );

    G4_INST* createFillInstr(G4_Declare* fillRangeDcl, G4_Declare* mRangeDcl, unsigned regOff, unsigned height, unsigned srcRegOff = 0);
    G4_INST* createFillInstr(G4_Declare* fillRangeDcl, G4_Declare* mRangeDcl, G4_SrcRegRegion* filledRangeRegion, unsigned execSize, unsigned regOff = 0);

    void
    replaceSpilledRange (
        G4_Declare *      spillRangeDcl,
        G4_DstRegRegion * spilledRegion,
        G4_INST *         spilledInst
    );

    void
    replaceFilledRange (
        G4_Declare *      fillRangeDcl,
        G4_SrcRegRegion * filledRegion,
        G4_INST *         filledInst
    );

    INST_LIST::iterator
    insertSpillRangeCode (
        INST_LIST::iterator spilledInstIter,
        G4_BB* bb
    );

    INST_LIST::iterator
    insertSendFillRangeCode (
        G4_SrcRegRegion *   filledRegion,
        INST_LIST::iterator filledInstIter,
        G4_BB* bb
    );

    INST_LIST::iterator
    insertFillGRFRangeCode (
        G4_SrcRegRegion *   filledRegion,
        INST_LIST::iterator filledInstIter,
        G4_BB* bb
    );

    void *
    allocMem (
        unsigned size
    ) const;

    SpillManagerGRF (
        const SpillManagerGRF & other
    );

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
    std::vector<EDGE> &      prevIntfEdges_;
    LR_LIST &                spilledLRs_;
    unsigned *               spillRangeCount_;
    unsigned *               fillRangeCount_;
    unsigned *               tmpRangeCount_;
    unsigned *               msgSpillRangeCount_;
    unsigned *               msgFillRangeCount_;
    unsigned                 nextSpillOffset_;
    unsigned                 iterationNo_;
    unsigned                 bbId_;
    unsigned                 spillAreaOffset_;
    bool                     doSpillSpaceCompression;

    bool                     failSafeSpill_;
    unsigned                 spillRegStart_;
    unsigned                 indrSpillRegStart_;
    unsigned                 spillRegOffset_;


    Interference *             spillIntf_;
    vISA::Mem_Manager              mem_;

    // The number of GRF spill.
    unsigned numGRFSpill;

    // The number of GRF fill.
    unsigned numGRFFill;

    // The number of mov.
    unsigned numGRFMove;

    // CISA instruction id of current instruction
    G4_INST* curInst;

    int globalScratchOffset;

    bool useScratchMsg_;

    bool headerNeeded()
    {
        bool needed = true;

        if (useScratchMsg_ && builder_->getPlatform() >= GENX_SKL)
            needed = false;

        if (builder_->kernel.fg.getHasStackCalls() ||
            builder_->kernel.fg.getIsStackCallFunc())
            needed = false;

        return needed;
    }

    void saveRestoreA0(G4_BB*);
};
}
bool isDisContRegion (
    vISA::G4_DstRegRegion * region,
    unsigned          execSize
);

bool isDisContRegion (
    vISA::G4_SrcRegRegion * region,
    unsigned          execSize
);

// Check if the region is partial or not, i.e does it read/write the
// whole segment.

template <class REGION_TYPE>
bool isPartialRegion (
    REGION_TYPE * region,
    unsigned      execSize
)
{
    // If the region is discontiguous then it is partial.

    if (isDisContRegion (region, execSize)) {
        return true;
    }
    else
    {
        return false;
    }
}

#endif // __SPILLMANAGERGMRF_H__
