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

#include "SpillManagerGMRF.h"
#include "Gen4_IR.hpp"
#include "Mem_Manager.h"
#include "FlowGraph.h"
#include "GraphColor.h"
#include "BuildIR.h"
#include "DebugInfo.h"

#include <math.h>
#include <sstream>
#include <fstream>

using namespace std;
using namespace vISA;

// Configurations

#define ADDRESS_SENSITIVE_SPILLS_IMPLEMENTED
#define REG_DWORD_SIZE (getGRFSize() / 4)
#define REG_BYTE_SIZE (getGRFSize())
#define SCRATCH_SPACE_ADDRESS_UNIT 5

//#define DISABLE_SPILL_MEMORY_COMPRESSION
//#define VERIFY_SPILL_ASSIGNMENTS

// Constant declarations

static const unsigned DWORD_BYTE_SIZE                        = 4;
static const unsigned OWORD_BYTE_SIZE                        = 16;
static const unsigned HWORD_BYTE_SIZE                        = 32;
static const unsigned PAYLOAD_INPUT_REG_OFFSET                = 0;
static const unsigned PAYLOAD_INPUT_SUBREG_OFFSET            = 0;
static const unsigned OWORD_PAYLOAD_SPOFFSET_REG_OFFSET        = 0;
static const unsigned OWORD_PAYLOAD_SPOFFSET_SUBREG_OFFSET    = 2;
static const unsigned DWORD_PAYLOAD_SPOFFSET_REG_OFFSET        = 1;
static const unsigned DWORD_PAYLOAD_SPOFFSET_SUBREG_OFFSET    = 0;
static const unsigned OWORD_PAYLOAD_WRITE_REG_OFFSET        = 1;
static const unsigned OWORD_PAYLOAD_WRITE_SUBREG_OFFSET        = 0;
// dword scatter is always in SIMD8 mode
static const unsigned DWORD_PAYLOAD_WRITE_REG_OFFSET        = 2;
static const unsigned DWORD_PAYLOAD_WRITE_SUBREG_OFFSET        = 0;
static const unsigned OWORD_PAYLOAD_HEADER_MIN_HEIGHT        = 1;
static const unsigned DWORD_PAYLOAD_HEADER_MIN_HEIGHT        = 2;
static const unsigned OWORD_PAYLOAD_HEADER_MAX_HEIGHT        = 1;
static const unsigned DWORD_PAYLOAD_HEADER_MAX_HEIGHT        = 3;
static const unsigned SCALAR_EXEC_SIZE                        = 1;
static const unsigned DEF_HORIZ_STRIDE                        = 1;
static const unsigned REG_ORIGIN                            = 0;
static const unsigned SUBREG_ORIGIN                            = 0;

static const unsigned SEND_GT_READ_TYPE_BIT_OFFSET            = 13;
static const unsigned SEND_GT_WRITE_TYPE_BIT_OFFSET            = 13;
static const unsigned SEND_GT_DESC_DATA_SIZE_BIT_OFFSET        = 8;
static const unsigned SEND_GT_OW_READ_TYPE                    = 0;
static const unsigned SEND_GT_OW_WRITE_TYPE                    = 8;
static const unsigned SEND_GT_SC_READ_TYPE                    = 6;
static const unsigned SEND_GT_SC_WRITE_TYPE                    = 11;
static const unsigned SEND_GT_DP_RD_EX_DESC_IMM             = 5;
static const unsigned SEND_GT_DP_SC_RD_EX_DESC_IMM          = 4;    //scatter reads go to sampler cache
static const unsigned SEND_GT_DP_WR_EX_DESC_IMM             = 5;

static const unsigned SEND_IVB_MSG_TYPE_BIT_OFFSET         = 14;
static const unsigned SEND_IVB_OW_READ_TYPE                = 0;
static const unsigned SEND_IVB_OW_WRITE_TYPE               = 8;
static const unsigned SEND_IVB_SC_READ_TYPE                = 3;
static const unsigned SEND_IVB_SC_WRITE_TYPE               = 11;
static const unsigned SEND_IVB_DP_RD_EX_DESC_IMM           = 10; //data cache
static const unsigned SEND_IVB_DP_WR_EX_DESC_IMM           = 10; //data cache

// Scratch msg
static const unsigned SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT        = 1;
static const unsigned SCRATCH_MSG_DESC_CATEORY                = 18;
static const unsigned SCRATCH_MSG_DESC_OPERATION_MODE        = 17;
static const unsigned SCRATCH_MSG_DESC_CHANNEL_MODE            = 16;
static const unsigned SCRATCH_MSG_INVALIDATE_AFTER_READ        = 15;
static const unsigned SCRATCH_MSG_DESC_BLOCK_SIZE            = 12;

// Macros

#define LIMIT_SEND_EXEC_SIZE(EXEC_SIZE)(((EXEC_SIZE) > 16)? 16: (EXEC_SIZE))
#define SPILL_PAYLOAD_HEIGHT_LIMIT 4

extern unsigned int getStackCallRegSize(bool reserveStackCallRegs);

// spill/fill temps are always GRF-aligned, and are also even/odd aligned
// following the original declare's alignment
static void setNewDclAlignment(GlobalRA& gra, G4_Declare* newDcl, bool evenAlign)
{
    newDcl->setSubRegAlign(GRFALIGN);
    if (evenAlign)
    {
        newDcl->setEvenAlign();
    }

    gra.setSubRegAlign(newDcl, GRFALIGN);
    gra.setEvenAligned(newDcl, evenAlign);
}

// Constructor

SpillManagerGRF::SpillManagerGRF(
    GlobalRA& g,
    unsigned spillAreaOffset,
    unsigned varIdCount,
    const LivenessAnalysis* lvInfo,
    LiveRange** lrInfo,
    Interference* intf,
    std::vector<EDGE>& prevIntfEdges,
    LR_LIST& spilledLRs,
    unsigned iterationNo,
    bool failSafeSpill,
    unsigned spillRegSize,
    unsigned indrSpillRegSize,
    bool enableSpillSpaceCompression,
    bool useScratchMsg)
    : gra(g)
    , builder_(g.kernel.fg.builder)
    , varIdCount_(varIdCount)
    , latestImplicitVarIdCount_(0)
    , lvInfo_(lvInfo)
    , lrInfo_(lrInfo)
    , prevIntfEdges_(prevIntfEdges)
    , spilledLRs_(spilledLRs)
    , nextSpillOffset_(spillAreaOffset)
    , iterationNo_(iterationNo)
    , bbId_(UINT_MAX)
    , doSpillSpaceCompression(enableSpillSpaceCompression)
    , failSafeSpill_(failSafeSpill)
    , spillIntf_(intf)
    , mem_(1024)
    , numGRFSpill(0)
    , numGRFFill(0)
    , numGRFMove(0)
    , useScratchMsg_(useScratchMsg)
{
    const unsigned size = sizeof(unsigned) * varIdCount;
    spillRangeCount_ = (unsigned*)allocMem(size);
    memset(spillRangeCount_, 0, size);
    fillRangeCount_ = (unsigned*)allocMem(size);
    memset(fillRangeCount_, 0, size);
    tmpRangeCount_ = (unsigned*)allocMem(size);
    memset(tmpRangeCount_, 0, size);
    msgSpillRangeCount_ = (unsigned*)allocMem(size);
    memset(msgSpillRangeCount_, 0, size);
    msgFillRangeCount_ = (unsigned*)allocMem(size);
    memset(msgFillRangeCount_, 0, size);
    spillAreaOffset_ = spillAreaOffset;
    if (enableSpillSpaceCompression) {
        computeSpillIntf();
    }
    builder_->instList.clear();
    spillRegStart_ = g.kernel.getNumRegTotal();
    indrSpillRegStart_ = spillRegStart_;
    spillRegOffset_ = spillRegStart_;
    if (failSafeSpill) {
        unsigned int stackCallRegSize = getStackCallRegSize(builder_->kernel.fg.getHasStackCalls() || builder_->kernel.fg.getIsStackCallFunc());
        indrSpillRegStart_ -= (stackCallRegSize + indrSpillRegSize);
        spillRegStart_ = indrSpillRegStart_ - spillRegSize;
    }
    curInst = NULL;
    globalScratchOffset = gra.kernel.getIntKernelAttribute(Attributes::ATTR_SpillMemOffset);

}

// Compute the interference graph for intereference of the memory segments
// occupied by the spilled live ranges.

void
SpillManagerGRF::computeSpillIntf (
)
{
    // Apply previous interferences that are relevant for this iteration.

    for (auto& edge : prevIntfEdges_)
    {

        if (shouldSpillRegister (getRegVar (edge.first)) ||
            shouldSpillRegister (getRegVar (edge.second))) {
            spillIntf_->checkAndSetIntf(edge.first, edge.second);
        }
    }

    LR_LIST::const_iterator ltEnd = spilledLRs_.end();
    for (LR_LIST::const_iterator lt = spilledLRs_.begin();
        lt != ltEnd; ++lt)
    {
        LiveRange* lr = (*lt);
        unsigned int i = lr->getVar()->getId();

        std::vector<unsigned int>& intfs = spillIntf_->getSparseIntfForVar(i);
        for (auto it : intfs)
        {
            EDGE tempEdge;
            tempEdge.first = it;
            tempEdge.second = i;
            prevIntfEdges_.push_back(tempEdge);
        }
    }
}

// Get the base regvar for the source or destination region.

template <class REGION_TYPE>
inline G4_RegVar *
SpillManagerGRF::getRegVar (
    REGION_TYPE * region
) const
{
    G4_RegVar * spilledRegVar = (G4_RegVar *) region->getBase();
    return spilledRegVar;
}

// Get the representative regvar that will be assigned a unique spill
// disp and not a relative spill disp.

inline G4_RegVar *
SpillManagerGRF::getReprRegVar (
    G4_RegVar * regVar
) const
{
    G4_RegVar * absBase = regVar->getAbsBaseRegVar ();
    if (absBase->isAliased ())
        return getReprRegVar(absBase->getDeclare ()->getAliasDeclare ()->getRegVar ());
    else
        return absBase;
}

// Obtain the register file type of the regvar.

inline G4_RegFileKind
SpillManagerGRF::getRFType (
    G4_RegVar * regvar
) const
{
    return regvar->getDeclare ()->getRegFile ();
}

// Obtain the register file type of the region.

template <class REGION_TYPE>
inline G4_RegFileKind
SpillManagerGRF::getRFType (
    REGION_TYPE * region
) const
{
    if (region->getBase ()->isRegVar ())
        return getRFType (region->getBase ()->asRegVar ());
    else if (region->getBase ()->isGreg ())
        return G4_GRF;
    else
        return G4_ADDRESS;
}

// Get the byte offset of the origin of the source or destination region.
// The row offset component is calculated based on the the parameters of
// the corresponding declare directive, while the column offset is calculated
// based on the region parameters.

template <class REGION_TYPE>
inline unsigned
SpillManagerGRF::getRegionOriginOffset (
    REGION_TYPE * region
) const
{
    unsigned rowOffset = REG_BYTE_SIZE * region->getRegOff ();
    unsigned columnOffset = region->getSubRegOff () * region->getElemSize ();
    return rowOffset + columnOffset;
}

// Check if the destination region is discontiguous or not.
// A destination region is discontiguous if there are portions of the
// region that are not written and unaffected.

bool isDisContRegion (
    G4_DstRegRegion * region,
    unsigned          execSize
)
{
    // If the horizontal stride is greater than 1, then it has gaps.
    // NOTE: Horizontal stride of 0 is not allowed for destination regions.
    return region->getHorzStride() != 1;

}

// Check if the source region is discontiguous or not.
// A source region is discontiguous in there are portions of the region
// that are not read.

bool isDisContRegion (
    G4_SrcRegRegion * region,
    unsigned          execSize
)
{
    return region->getRegion()->isContiguous(execSize);
}

// Get a GRF aligned mask

unsigned SpillManagerGRF::grfMask() const
{
    unsigned mask = 0;
    mask = (mask - 1);
    MUST_BE_TRUE(std::log2(G4_GRF_REG_NBYTES) == (float)((int)(std::log2(G4_GRF_REG_NBYTES))), "expected integral value");
    unsigned int bits = (unsigned int)std::log2(G4_GRF_REG_NBYTES);
    mask = mask << bits;
    return mask;
}

// Get an hexal word mask with the lower 5 bits zeroed.

inline unsigned
SpillManagerGRF::hwordMask () const
{
    unsigned mask = 0;
    mask = (mask - 1);
    mask = mask << 5;
    return mask;
}

// Get an octal word mask with the lower 4 bits zeroed.

inline unsigned
SpillManagerGRF::owordMask () const
{
    unsigned mask = 0;
    mask = (mask - 1);
    mask = mask << 4;
    return mask;
}

// Get an dword word mask with the lower 2 bits zeroed.

inline unsigned
SpillManagerGRF::dwordMask () const
{
    unsigned mask = 0;
    mask = (mask - 1);
    mask = mask << 2;
    return mask;
}

// Test of the offset is oword aligned.

inline bool
SpillManagerGRF::owordAligned (
    unsigned offset
) const
{
    return (offset & owordMask ()) == offset;
}

// Test of the offset is oword aligned.

inline bool
SpillManagerGRF::dwordAligned (
    unsigned offset
) const
{
    return (offset & dwordMask ()) == offset;
}

// Get the ceil of the division.

inline unsigned
SpillManagerGRF::cdiv (
    unsigned dvd,
    unsigned dvr
)
{
    return (dvd / dvr) + ((dvd % dvr)? 1: 0);
}

// Get the live range corresponding to id.

inline bool
SpillManagerGRF::shouldSpillRegister (
    G4_RegVar * regVar
) const
{

    if (getRFType (regVar) == G4_ADDRESS)
    {
        return false;
    }
    G4_RegVar * actualRegVar =
        (regVar->getDeclare ()->getAliasDeclare ())?
        regVar->getDeclare ()->getAliasDeclare ()->getRegVar ():
        regVar;
    if (actualRegVar->getId () == UNDEFINED_VAL)
        return false;
    else if (regVar->isRegVarTransient () || regVar->isRegVarTmp ())
        return false;
#ifndef ADDRESS_SENSITIVE_SPILLS_IMPLEMENTED
    else if    (lvInfo_->isAddressSensitive (regVar->getId ()))
        return false;
#endif

    else if (builder_->kernel.fg.isPseudoVCADcl(actualRegVar->getDeclare()) ||
        builder_->kernel.fg.isPseudoVCEDcl(actualRegVar->getDeclare()))
        return false;
    else
        return lrInfo_ [actualRegVar->getId ()]->getPhyReg () == NULL;
}

// Get the regvar with the id.

inline G4_RegVar *
SpillManagerGRF::getRegVar (
    unsigned id
) const
{
    return (lvInfo_->vars)[id];
}

// Get the byte size of the live range.

inline unsigned
SpillManagerGRF::getByteSize (
    G4_RegVar * regVar
) const
{
    unsigned normalizedRowSize =
        (regVar->getDeclare ()->getNumRows () > 1)?
        REG_BYTE_SIZE:
        regVar->getDeclare ()->getNumElems () *
        regVar->getDeclare ()->getElemSize ();
    return
        normalizedRowSize * regVar->getDeclare ()->getNumRows ();
}

// Check if the lifetime of the spill/fill memory of live range i interferes
// with the lifetime of the spill/fill memory of live range j

bool
SpillManagerGRF::spillMemLifetimeInterfere (
    unsigned i,
    unsigned j
) const
{
    G4_RegVar * ireg = getRegVar (i);
    G4_RegVar * jreg = getRegVar (j);
    G4_RegVar * irep = getReprRegVar (ireg);
    G4_RegVar * jrep = getReprRegVar (jreg);
    G4_RegVar * inont = ireg->getNonTransientBaseRegVar ();
    G4_RegVar * jnont = jreg->getNonTransientBaseRegVar ();

    if (ireg->isRegVarTmp ()) {
        return
            ireg->getBaseRegVar () == jrep ||
            spillMemLifetimeInterfere (ireg->getBaseRegVar ()->getId (), j);
    }

    else if (jreg->isRegVarTmp ()) {
        return
            jreg->getBaseRegVar () == irep ||
            spillMemLifetimeInterfere (jreg->getBaseRegVar ()->getId (), i);
    }

    else if (inont->isRegVarTmp ()) {
        return
            inont->getBaseRegVar () == jrep ||
            spillMemLifetimeInterfere (inont->getBaseRegVar ()->getId (), j);

    }

    else if (jnont->isRegVarTmp ()) {
        return
            jnont->getBaseRegVar () == irep ||
            spillMemLifetimeInterfere (jnont->getBaseRegVar ()->getId (), i);
    }

    else {
        if (spillIntf_->interfereBetween (irep->getId (), jrep->getId ()))
            return true;
        else if (getRFType (irep) != getRFType (jrep))
            return true;
        else
#ifdef DISABLE_SPILL_MEMORY_COMPRESSION
            return irep != jrep;
#else
            return false;
#endif
    }
}

// Calculate the spill memory displacement for the regvar.

unsigned
SpillManagerGRF::calculateSpillDisp (
    G4_RegVar *   regVar
) const
{
    assert (regVar->getDisp () == UINT_MAX);

    // Locate the blocked locations calculated from the interfering
    // spilled live ranges and put them into a list in ascending order.

    typedef std::list < G4_RegVar * > LocList;
    LocList locList;
    unsigned lrId =
        (regVar->getId () >= varIdCount_)?
        regVar->getBaseRegVar ()->getId (): regVar->getId ();
    assert (lrId < varIdCount_);

    for (unsigned i = 0; i < varIdCount_; i++) {

        if (spillMemLifetimeInterfere (lrId, i)) {
            G4_RegVar * intfRegVar = getRegVar (i);
            assert (getRegVar (i)->isAliased () == false);
            if (intfRegVar->isRegVarTransient ()) continue;
            unsigned iDisp = intfRegVar->getDisp ();
            if (iDisp == UINT_MAX) continue;
            LocList::iterator loc;
            for (loc = locList.begin ();
                 loc != locList.end () && (*loc)->getDisp () < iDisp;
                 ++loc);
            if (loc != locList.end ())
                locList.insert (loc, intfRegVar);
            else
                locList.push_back (intfRegVar);
        }
    }

    // Find a spill slot for lRange within the locList.
    // we always start searching from 0 to facilitate cross-iteration reuse
    unsigned regVarLocDisp = 0;
    unsigned regVarSize = getByteSize (regVar);

    for (LocList::iterator curLoc = locList.begin (), end = locList.end(); curLoc != end;
        ++curLoc) {
        unsigned curLocDisp = (*curLoc)->getDisp ();
        if (regVarLocDisp < curLocDisp &&
            regVarLocDisp + regVarSize <= curLocDisp)
            break;
        unsigned curLocEnd = curLocDisp + getByteSize (*curLoc);
        {
            if (curLocEnd % G4_GRF_REG_NBYTES != 0)
                curLocEnd = ROUND(curLocEnd, G4_GRF_REG_NBYTES);
        }

        regVarLocDisp = (regVarLocDisp > curLocEnd)? regVarLocDisp: curLocEnd;
    }

    return regVarLocDisp;
}

// Get the spill/fill displacement of the segment containing the region.
// A segment is the smallest dword or oword aligned portion of memory
// containing the destination or source operand that can be read or saved.

template <class REGION_TYPE>
inline unsigned
SpillManagerGRF::getSegmentDisp (
    REGION_TYPE * region,
    unsigned      execSize
)
{
    assert (region->getElemSize () && execSize);
    if (isUnalignedRegion (region, execSize))
        return getEncAlignedSegmentDisp (region, execSize);
    else
        return getRegionDisp (region);
}

// Get the spill/fill displacement of the regvar.

unsigned
SpillManagerGRF::getDisp(
G4_RegVar * regVar
)
{
    // Already calculated spill memory disp

    if (regVar->getDisp() != UINT_MAX)
    {
        return regVar->getDisp();
    }

    // If it is an aliased regvar then calculate the disp for the
    // actual regvar and then calculate the disp of the aliased regvar
    // based on it.

    else if (regVar->isAliased()) {
        G4_Declare * regVarDcl = regVar->getDeclare();
        return
            getDisp(regVarDcl->getAliasDeclare()->getRegVar()) +
            regVarDcl->getAliasOffset();
    }

    // If its base regvar has been assigned a disp, then the spill memory
    // has already been allocated for it, simply calculate the disp based
    // on the enclosing segment disp.

    else if (regVar->isRegVarTransient() &&
        getDisp(regVar->getBaseRegVar()) != UINT_MAX) {
        assert(regVar->getBaseRegVar() != regVar);
        unsigned itsDisp;

        if (regVar->isRegVarSpill()) {
            G4_RegVarTransient * tRegVar = static_cast <G4_RegVarTransient*> (regVar);
            assert(
                getSegmentByteSize(
                tRegVar->getDstRepRegion(), tRegVar->getExecSize()) <=
                getByteSize(tRegVar));
            itsDisp =
                getSegmentDisp(
                tRegVar->getDstRepRegion(), tRegVar->getExecSize());
        }

        else if (regVar->isRegVarFill()) {
            G4_RegVarTransient * tRegVar = static_cast <G4_RegVarTransient*> (regVar);
            assert(
                getSegmentByteSize(
                tRegVar->getSrcRepRegion(), tRegVar->getExecSize()) <=
                getByteSize(tRegVar));
            itsDisp =
                getSegmentDisp(
                tRegVar->getSrcRepRegion(), tRegVar->getExecSize());
        }

        else {
            MUST_BE_TRUE(false, "Incorrect spill/fill ranges.");
            itsDisp = 0;
        }

        regVar->setDisp(itsDisp);
    }

    // Allocate the spill and evaluate its disp

    else {
        if (doSpillSpaceCompression)
        {
            assert(regVar->isRegVarTransient() == false);
            regVar->setDisp(calculateSpillDisp(regVar));
        }
        else
        {
            assert(regVar->isRegVarTransient() == false);
            if (regVar->getId() >= varIdCount_)
            {
                if (regVar->getBaseRegVar()->getDisp() != UINT_MAX)
                {
                    regVar->setDisp(regVar->getBaseRegVar()->getDisp());
                    return regVar->getDisp();
                }
            }

            if ((spillAreaOffset_) % G4_GRF_REG_NBYTES != 0)
            {
                (spillAreaOffset_) = ROUND(spillAreaOffset_, G4_GRF_REG_NBYTES);
            }

            regVar->setDisp(spillAreaOffset_);
            spillAreaOffset_ += getByteSize(regVar);
        }
    }

    // ToDo: log this in some dump to help debug
    //regVar->getDeclare()->dump();
    //std::cerr << "spill offset = " << regVar->getDisp() << "\n";

    return regVar->getDisp();
}

// Get the spill/fill displacement of the region.

template <class REGION_TYPE>
inline unsigned
SpillManagerGRF::getRegionDisp (
    REGION_TYPE * region
)
{
    return getDisp (getRegVar (region)) +  getRegionOriginOffset (region);
}

// Get the type of send message to use to spill/fill the region.
// The type can be either on oword read/write or a scatter read/write.
// If the segment corresponding to the region is dword sized then a
// dword read/write is used else an oword read/write is used.

template <class REGION_TYPE>
inline unsigned
SpillManagerGRF::getMsgType (
    REGION_TYPE * region,
    unsigned      execSize
)
{
    unsigned regionDisp = getRegionDisp (region);
    unsigned regionByteSize = getRegionByteSize (region, execSize);
    if (owordAligned (regionDisp) && owordAligned (regionByteSize))
        return owordMask ();
    else
        return getEncAlignedSegmentMsgType (region, execSize);
}

// Determine if the region is unaligned w.r.t spill/fill memory read/writes.
// If the exact region cannot be read/written from spill/fill memory using
// one send instruction, then it is unaligned.

template <class REGION_TYPE>
inline bool
SpillManagerGRF::isUnalignedRegion (
    REGION_TYPE * region,
    unsigned      execSize
)
{
    unsigned regionDisp = getRegionDisp (region);
    unsigned regionByteSize = getRegionByteSize (region, execSize);

    if( useScratchMsg_)
    {
        if( regionDisp%G4_GRF_REG_NBYTES == 0 && regionByteSize%G4_GRF_REG_NBYTES == 0 )
            return
                regionByteSize / G4_GRF_REG_NBYTES != 1 &&
                regionByteSize / G4_GRF_REG_NBYTES != 2 &&
                regionByteSize / G4_GRF_REG_NBYTES != 4;
        else
            return true;
    }
    else
    {
        if (owordAligned (regionDisp) && owordAligned (regionByteSize))
            return
                regionByteSize / OWORD_BYTE_SIZE != 1 &&
                regionByteSize / OWORD_BYTE_SIZE != 2 &&
                regionByteSize / OWORD_BYTE_SIZE != 4;
        else
            return true;

    }
}

// Calculate the smallest aligned segment encompassing the region.

template <class REGION_TYPE>
void
SpillManagerGRF::calculateEncAlignedSegment (
    REGION_TYPE * region,
    unsigned      execSize,
    unsigned &    start,
    unsigned &    end,
    unsigned &    type
)
{
    unsigned regionDisp = getRegionDisp (region);
    unsigned regionByteSize = getRegionByteSize (region, execSize);

    if (need32ByteAlignedOffset())
    {
        unsigned hwordLB = regionDisp & grfMask();
        unsigned hwordRB = hwordLB + G4_GRF_REG_NBYTES;
        unsigned blockSize = G4_GRF_REG_NBYTES;

        while (regionDisp + regionByteSize > hwordRB) {
            hwordRB += blockSize;
        }

        assert((hwordRB - hwordLB) / REG_BYTE_SIZE <= 4);
        start = hwordLB;
        end = hwordRB;
        type = grfMask();
    }
    else
    {
        unsigned owordLB = regionDisp & owordMask();
        unsigned owordRB = owordLB + OWORD_BYTE_SIZE;
        unsigned blockSize = OWORD_BYTE_SIZE;

        while (regionDisp + regionByteSize > owordRB) {
            owordRB += blockSize;
            blockSize *= 2;
        }

        assert((owordRB - owordLB) / REG_BYTE_SIZE <= 4);
        start = owordLB;
        end = owordRB;
        type = owordMask();
    }
}

// Get the byte size of the aligned segment for the region.

template <class REGION_TYPE>
inline unsigned
SpillManagerGRF::getEncAlignedSegmentByteSize (
    REGION_TYPE * region,
    unsigned      execSize
)
{
    unsigned start, end, type;
    calculateEncAlignedSegment (region, execSize, start, end, type);
    return end - start;
}

// Get the start offset of the aligned segment for the region.

template <class REGION_TYPE>
inline unsigned
SpillManagerGRF::getEncAlignedSegmentDisp (
    REGION_TYPE * region,
    unsigned      execSize
)
{
    unsigned start, end, type;
    calculateEncAlignedSegment (region, execSize, start, end, type);
    return start;
}

// Get the type of message to be used to read/write the enclosing aligned
// segment for the region.

template <class REGION_TYPE>
inline unsigned
SpillManagerGRF::getEncAlignedSegmentMsgType (
    REGION_TYPE * region,
    unsigned      execSize
)
{
    unsigned start, end, type;
    calculateEncAlignedSegment (region, execSize, start, end, type);
    return type;
}

// Get the byte size of the segment for the region.

template <class REGION_TYPE>
inline unsigned
SpillManagerGRF::getSegmentByteSize (
    REGION_TYPE * region,
    unsigned      execSize
)
{
    assert (region->getElemSize () && execSize);
    if (isUnalignedRegion (region, execSize))
        return getEncAlignedSegmentByteSize (region, execSize);
    else
        return getRegionByteSize (region, execSize);
}

// Get the byte size of the destination region.

inline unsigned
SpillManagerGRF::getRegionByteSize (
    G4_DstRegRegion * region,
    unsigned          execSize
) const
{
    unsigned size = region->getHorzStride() * region->getElemSize() *
        (execSize - 1) + region->getElemSize();

    return size;
}

// Get the byte size of the source region.

inline unsigned
SpillManagerGRF::getRegionByteSize (
    G4_SrcRegRegion * region,
    unsigned          execSize
) const
{
    assert (execSize % region->getRegion ()->width == 0);
    unsigned nRows = execSize / region->getRegion ()->width;
    unsigned size = 0;

    for (unsigned int i = 0; i < nRows - 1; i++) {
        size += region->getRegion ()->vertStride * region->getElemSize ();
    }

    size +=
        region->getRegion ()->horzStride * region->getElemSize () *
        (region->getRegion ()->width - 1) + region->getElemSize ();
    return size;
}

// Get the max exec size on a 256 bit vector for the input operand.

inline unsigned
SpillManagerGRF::getMaxExecSize (
    G4_Operand * operand
) const
{
    const unsigned size = Type_UNDEF + 1;
    static unsigned maxExecSize [size] = {8, 8, 16, 16, 16, 16, 8, 8, 0};
    return maxExecSize [operand->getType ()];
}

// Check if the instruction is a SIMD 16 or 32 instruction that is logically
// equivalent to two instructions the second of which uses register operands
// at the following row with the same sub-register index.

inline bool
SpillManagerGRF::isComprInst (
    G4_INST * inst
) const
{
    return inst->isComprInst ();
}

// Check if the source in a compressed instruction operand occupies a second
// register.

bool
SpillManagerGRF::isMultiRegComprSource (
    G4_SrcRegRegion* src,
    G4_INST *        inst
) const
{
    if (inst->isComprInst () == false) {
        return false;
    }

    else if (isScalarReplication(src)) {
        return false;
    }

    else if (inst->getExecSize() <= 8) {
        return false;
    }
    else if (!src->asSrcRegRegion()->crossGRF())
    {
        return false;
    }

    else if (inst->getExecSize () == 16 &&
             inst->getDst () &&
             G4_Type_Table[inst->getDst ()->getType ()].byteSize == 4 &&
             inst->getDst()->getHorzStride () == 1 ) {

        if (G4_Type_Table[src->getType()].byteSize == 2 &&
            src->isNativePackedRegion()) {
            return false;
        }

        else {
            return true;
        }
    }

    else {
        return true;
    }
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendRspLengthBitOffset () const
{
    return SEND_GT_RSP_LENGTH_BIT_OFFSET;
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendMaxResponseLength () const
{
    //return SEND_GT_MAX_RESPONSE_LENGTH;
    return 8;
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendMsgLengthBitOffset ()
{
    return SEND_GT_MSG_LENGTH_BIT_OFFSET;
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendMaxMessageLength () const
{
    return SEND_GT_MAX_MESSAGE_LENGTH;
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendDescDataSizeBitOffset ()
{
    return SEND_GT_DESC_DATA_SIZE_BIT_OFFSET;
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendReadTypeBitOffset () const
{
    return SEND_IVB_MSG_TYPE_BIT_OFFSET;
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendWriteTypeBitOffset ()
{
    return SEND_IVB_MSG_TYPE_BIT_OFFSET;
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendScReadType () const
{
    return SEND_IVB_SC_READ_TYPE;
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendScWriteType () const
{
    return SEND_IVB_SC_WRITE_TYPE;
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendOwordReadType () const
{
    return SEND_IVB_OW_READ_TYPE;
}

// Send message information query

inline unsigned
SpillManagerGRF::getSendOwordWriteType ()
{
    return SEND_IVB_OW_WRITE_TYPE;
}

inline unsigned
SpillManagerGRF::getSendExDesc( bool isWrite, bool isScatter ) const
{
    return isWrite ? SEND_IVB_DP_WR_EX_DESC_IMM : SEND_IVB_DP_RD_EX_DESC_IMM;
}

// Custom memory allocator

inline void *
SpillManagerGRF::allocMem (
    unsigned size
) const
{
    return builder_->mem.alloc (size);
}

bool SpillManagerGRF::useSplitSend() const
{
    return builder_->useSends();
}

// Get a unique spill range index for regvar.

inline unsigned
SpillManagerGRF::getSpillIndex (
    G4_RegVar *  spilledRegVar
)
{
    return spillRangeCount_ [spilledRegVar->getId ()]++;
}

// Get a unique fill range index for regvar.

inline unsigned
SpillManagerGRF::getFillIndex (
    G4_RegVar *  spilledRegVar
)
{
    return fillRangeCount_ [spilledRegVar->getId ()]++;
}

// Get a unique tmp index for spilled regvar.

inline unsigned
SpillManagerGRF::getTmpIndex (
    G4_RegVar *  spilledRegVar
)
{
    return tmpRangeCount_ [spilledRegVar->getId ()]++;
}

// Get a unique msg index for spilled regvar.

inline unsigned
SpillManagerGRF::getMsgSpillIndex (
    G4_RegVar *  spilledRegVar
)
{
    return msgSpillRangeCount_ [spilledRegVar->getId ()]++;
}

// Get a unique msg index for filled regvar.

inline unsigned
SpillManagerGRF::getMsgFillIndex (
    G4_RegVar *  spilledRegVar
)
{
    return msgFillRangeCount_ [spilledRegVar->getId ()]++;
}

// Create a unique name for a regvar representing a spill/fill/msg live range.

inline const char *
SpillManagerGRF::createImplicitRangeName (
    const char * baseName,
    G4_RegVar *  spilledRegVar,
    unsigned     index
)
{
    stringstream nameStrm;
    nameStrm << baseName << "_" << spilledRegVar->getName ()
             << "_" << index << ends;
    int nameLen = unsigned(nameStrm.str().length()) + 1;
    char * name = (char *) allocMem (nameLen);
    strcpy_s(name, nameLen, nameStrm.str().c_str ());
    return name;
}

// Check if the region is a scalar replication region.

inline bool
SpillManagerGRF::isScalarReplication (
    G4_SrcRegRegion * region
) const
{
    return region->isScalar();
}

// Check if we have to repeat the simd16 source in the simd8 equivalents.
// The BPSEC mentions that if a replicated scalar appears in an simd16
// instruction, logically we need to repeat the source region used in
// the first simd8 instruction in the second simd8 instruction as well
// (i.e. the reg no is not incremented by one for the second).

inline bool
SpillManagerGRF::repeatSIMD16or32Source (
    G4_SrcRegRegion * region
) const
{
    return isScalarReplication (region);
}

// Create a declare directive for a new live range (spill/fill/msg)
// introduced as part of the spill code generation.

G4_Declare *
SpillManagerGRF::createRangeDeclare (
    const char*    name,
    G4_RegFileKind regFile,
    unsigned short nElems,
    unsigned short nRows,
    G4_Type        type,
    DeclareType    kind,
    G4_RegVar *    base,
    G4_Operand *   repRegion,
    unsigned       execSize
)
{
    G4_Declare * rangeDeclare =
        builder_->createDeclareNoLookup (
            name, regFile, nElems, nRows, type, kind,
            base, repRegion, execSize);
    rangeDeclare->getRegVar ()->setId (
        varIdCount_ + latestImplicitVarIdCount_++);
    gra.setBBId(rangeDeclare, bbId_);
    return rangeDeclare;
}

// Create a GRF regvar and its declare directive to represent the spill/fill
// live range.
// The size of the regvar is calculated from the size of the spill/fill
// region. If the spill/fill region fits into one row, then width of the
// regvar is exactly as needed for the spill/fill segment, else it is
// made to occupy exactly two full rows. In either case the regvar is made
// to have 16 word alignment requirement. This is to satisfy the requirements
// of the send instruction used to save/load the value from memory. For
// region's in simd16 instruction contexts we multiply the height by 2
// except for source region's with scalar replication.

template <class REGION_TYPE>
G4_Declare *
SpillManagerGRF::createTransientGRFRangeDeclare (
    REGION_TYPE * region,
    const char  * baseName,
    unsigned      index,
    unsigned      execSize,
    G4_INST     * inst
)
{
    const char * name =
        createImplicitRangeName (baseName, getRegVar (region), index);
    G4_Type type = region->getType ();
    unsigned segmentByteSize = getSegmentByteSize (region, execSize);
    DeclareType regVarKind =
        (region->isDstRegRegion ())? DeclareType::Spill : DeclareType::Fill;
    unsigned short width, height;

    if (segmentByteSize > REG_BYTE_SIZE || region->crossGRF()) {
        assert (REG_BYTE_SIZE % region->getElemSize () == 0);
        width = REG_BYTE_SIZE / region->getElemSize ();
        assert (segmentByteSize / REG_BYTE_SIZE <= 2);
        height = 2;
    }

    else {
        assert (segmentByteSize % region->getElemSize () == 0);
        width = segmentByteSize / region->getElemSize ();
        height = 1;
    }

    if (need32ByteAlignedOffset())
    {
        // the message will read/write a minimum of one GRF
        if (height == 1 && width < getGRFSize())
            width = getGRFSize() / region->getElemSize();
    }

    G4_Declare * transientRangeDeclare =
        createRangeDeclare(
            name, G4_GRF, width, height, type,
            regVarKind, region->getBase ()->asRegVar (), region, execSize);

    if( failSafeSpill_ )
    {
        transientRangeDeclare->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegOffset_), 0);
        spillRegOffset_ += height;
    }

    // FIXME: We should take the original declare's alignment too, but I'm worried
    // we may get perf regression if FE is over-aligning or the alignment is not necessary for this inst.
    // So Either is used for now and we can change it later if there are bugs
    setNewDclAlignment(gra, transientRangeDeclare, false);
    return transientRangeDeclare;
}

static unsigned short getSpillRowSizeForSendDst(
    G4_INST *         inst,
    G4_DstRegRegion * spilledRegion
)
{
    unsigned short nRows = 0;

    if (inst->isSend())
    {
        G4_SendMsgDescriptor* msgDesc = inst->getMsgDesc();
        nRows = msgDesc->ResponseLength();
    }
    else
    {
        assert(spilledRegion->getLinearizedStart() % GENX_GRF_REG_SIZ == 0);
        nRows = (spilledRegion->getLinearizedEnd() - spilledRegion->getLinearizedStart() + 1) / GENX_GRF_REG_SIZ;
    }
    return nRows;
}

// Create a regvar and its declare directive to represent the spill live
// range that appears as a send instruction post destination GRF.
// The type of the regvar is set as dword and its width 8. The type of
// the post destination does not matter, so we just use type dword, and
// a width of 8 so that a row corresponds to a physical register.

G4_Declare *
SpillManagerGRF::createPostDstSpillRangeDeclare (
    G4_INST *         sendOut,
    G4_DstRegRegion * spilledRegion
)
{
    G4_RegVar * spilledRegVar = getRegVar (spilledRegion);
    const char * name =
        createImplicitRangeName (
            "SP_GRF", spilledRegVar, getSpillIndex (spilledRegVar));
    unsigned short nRows = getSpillRowSizeForSendDst(sendOut, spilledRegion);

      G4_DstRegRegion * normalizedPostDst = builder_->createDst(
        spilledRegVar, spilledRegion->getRegOff (), SUBREG_ORIGIN,
        DEF_HORIZ_STRIDE, Type_UD);

    // We use the width as the user specified, the height however is
    // calculated based on the message descriptor to limit register
    // pressure induced by the spill range.

    G4_Declare * transientRangeDeclare =
        createRangeDeclare (
            name, G4_GRF, REG_DWORD_SIZE, nRows, Type_UD,
            DeclareType::Spill, spilledRegVar, normalizedPostDst, REG_DWORD_SIZE);

    if( failSafeSpill_ )
    {
        if( useSplitSend() )
        {
            transientRangeDeclare->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegStart_), 0);
            spillRegOffset_ += nRows;
        }
        else
        {
            transientRangeDeclare->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegStart_+1), 0);
            spillRegOffset_ += nRows + 1;
        }
    }

    return transientRangeDeclare;
}

// Create a regvar and its declare directive to represent the spill live range.

inline G4_Declare *
SpillManagerGRF::createSpillRangeDeclare (
    G4_DstRegRegion * spilledRegion,
    unsigned          execSize,
    G4_INST         * inst
)
{
    return
        createTransientGRFRangeDeclare (
            spilledRegion, "SP_GRF",
            getSpillIndex (getRegVar (spilledRegion)),
            execSize, inst);
}

// Create a regvar and its declare directive to represent the GRF fill live
// range.

inline G4_Declare *
SpillManagerGRF::createGRFFillRangeDeclare (
    G4_SrcRegRegion * fillRegion,
    unsigned          execSize,
    G4_INST         * inst
)
{
    assert (getRFType (fillRegion) == G4_GRF);
    G4_Declare * fillRangeDecl =
        createTransientGRFRangeDeclare (
            fillRegion, "FL_GRF", getFillIndex (getRegVar (fillRegion)),
            execSize, inst);
    return fillRangeDecl;
}

static unsigned short getSpillRowSizeForSendSrc(
    G4_INST *         inst,
    G4_SrcRegRegion * filledRegion
)
{
    unsigned short nRows = 0;

    if (inst->isSend())
    {
        G4_SendMsgDescriptor* msgDesc = inst->getMsgDesc();
        if (inst->isSplitSend() &&
            (inst->getSrc(1)->asSrcRegRegion() == filledRegion))
        {
            nRows = msgDesc->extMessageLength();
        }
        else
        {
            nRows = msgDesc->MessageLength();
        }
    }
    else
    {
        nRows = (filledRegion->getLinearizedEnd() - filledRegion->getLinearizedStart() + 1) / GENX_GRF_REG_SIZ;
    }

    return nRows;
}


// Create a regvar and its declare directive to represent the GRF fill live range.
inline G4_Declare *
SpillManagerGRF::createSendFillRangeDeclare (
    G4_SrcRegRegion * filledRegion,
    G4_INST *         sendInst
)
{
    G4_RegVar * filledRegVar = getRegVar (filledRegion);
    const char * name =
        createImplicitRangeName (
            "FL_Send", filledRegVar, getFillIndex (filledRegVar));
    unsigned short nRows = getSpillRowSizeForSendSrc(sendInst, filledRegion);

    G4_SrcRegRegion * normalizedSendSrc =
        builder_->createSrcRegRegion(
        filledRegion->getModifier(), Direct, filledRegVar,
        filledRegion->getRegOff(), filledRegion->getSubRegOff(), filledRegion->getRegion(),
        filledRegion->getType());
    unsigned short width = REG_BYTE_SIZE / filledRegion->getElemSize ();
    assert (REG_BYTE_SIZE % filledRegion->getElemSize () == 0);

    // We use the width as the user specified, the height however is
    // calculated based on the message descriptor to limit register
    // pressure induced by the spill range.

    G4_Declare * transientRangeDeclare =
        createRangeDeclare(
        name,
        G4_GRF,
        width, nRows, filledRegion->getType(),
        DeclareType::Fill, filledRegVar, normalizedSendSrc,
        width);

    setNewDclAlignment(gra, transientRangeDeclare, gra.isEvenAligned(filledRegVar->getDeclare()));

    if( failSafeSpill_ )
    {
        if (sendInst->isEOT() && builder_->hasEOTGRFBinding())
        {
            // make sure eot src is in last 16 GRF
            uint32_t eotStart = gra.kernel.getNumRegTotal() - 16;
            if (spillRegOffset_ < eotStart)
            {
                spillRegOffset_ = eotStart;
            }
        }
        transientRangeDeclare->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegOffset_), 0);
        spillRegOffset_ += nRows;
    }

    return transientRangeDeclare;
}

// Create a regvar and its declare directive to represent the temporary live
// range.

G4_Declare *
SpillManagerGRF::createTemporaryRangeDeclare (
    G4_DstRegRegion * spilledRegion,
    unsigned          execSize,
    bool              forceSegmentAlignment
)
{
    const char * name =
        createImplicitRangeName (
            "TM_GRF", getRegVar (spilledRegion),
            getTmpIndex (getRegVar (spilledRegion)));
    unsigned byteSize =
        (forceSegmentAlignment)?
        getSegmentByteSize (spilledRegion, execSize):
        getRegionByteSize (spilledRegion, execSize);

    assert (byteSize <= 2u * REG_BYTE_SIZE);
    assert (byteSize % spilledRegion->getElemSize () == 0);

    G4_Type type = spilledRegion->getType ();
    DeclareType regVarKind = DeclareType::Tmp;

    unsigned short width, height;
    if( byteSize > REG_BYTE_SIZE )
    {
        height = 2;
        width = REG_BYTE_SIZE/spilledRegion->getElemSize();
    }
    else
    {
        height = 1;
        width = byteSize/spilledRegion->getElemSize();
    }

    G4_RegVar* spilledRegVar = getRegVar(spilledRegion);

    G4_Declare * temporaryRangeDeclare =
        createRangeDeclare(
            name, G4_GRF, width, height, type,
            regVarKind, spilledRegVar, NULL, 0);

    if( failSafeSpill_ )
    {
        temporaryRangeDeclare->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegOffset_), 0);
        spillRegOffset_ += height;
    }

    setNewDclAlignment(gra, temporaryRangeDeclare, false);
    return temporaryRangeDeclare;
}

// Create a destination region that could be used in place of the spill regvar.
// If the region is unaligned then the origin of the destination region
// is the displacement of the orginal region from its segment, else the
// origin is 0.

G4_DstRegRegion *
SpillManagerGRF::createSpillRangeDstRegion (
    G4_RegVar *       spillRangeRegVar,
    G4_DstRegRegion * spilledRegion,
    unsigned          execSize,
    unsigned          regOff
)
{
    if (isUnalignedRegion (spilledRegion, execSize)) {
        unsigned segmentDisp =
            getEncAlignedSegmentDisp (spilledRegion, execSize);
        unsigned regionDisp = getRegionDisp (spilledRegion);
        assert (regionDisp >= segmentDisp);
        unsigned short subRegOff =
            (regionDisp - segmentDisp) / spilledRegion->getElemSize ();
        assert (
            (regionDisp - segmentDisp) % spilledRegion->getElemSize () == 0);
        assert (subRegOff * spilledRegion->getElemSize () +
                getRegionByteSize (spilledRegion, execSize) <=
                2u * REG_BYTE_SIZE);

        if(useScratchMsg_ )
        {
            G4_Declare* parent_dcl = spilledRegion->getBase()->asRegVar()->getDeclare();
            unsigned off = 0;
            while( parent_dcl->getAliasDeclare() != NULL )
            {
                // off is in bytes
                off += parent_dcl->getAliasOffset();
                parent_dcl = parent_dcl->getAliasDeclare();
            }
            off = off%G4_GRF_REG_NBYTES;
            // sub-regoff is in units of element size
            subRegOff = spilledRegion->getSubRegOff() + off/spilledRegion->getElemSize();
        }

        return builder_->createDst(
            spillRangeRegVar, (unsigned short) regOff, subRegOff,
            spilledRegion->getHorzStride (), spilledRegion->getType ());
    }

    else {
        return builder_->createDst(
            spillRangeRegVar, (short) regOff, SUBREG_ORIGIN,
            spilledRegion->getHorzStride (), spilledRegion->getType ());
    }
}

// Create a source region that could be used to copy out the temporary range
// (that was created to replace the portion of the spilled live range appearing
// in an instruction destination) into the segment aligned spill range for the
// spilled live range that can be written out to spill memory.

G4_SrcRegRegion *
SpillManagerGRF::createTemporaryRangeSrcRegion (
    G4_RegVar *       tmpRangeRegVar,
    G4_DstRegRegion * spilledRegion,
    uint16_t          execSize,
    unsigned          regOff
)
{
    uint16_t horzStride = spilledRegion->getHorzStride();
    // A scalar region is returned when execsize is 1.
    const RegionDesc *rDesc = builder_->createRegionDesc(execSize, horzStride, 1, 0);

    return builder_->createSrcRegRegion(
        Mod_src_undef, Direct, tmpRangeRegVar, (short) regOff, SUBREG_ORIGIN,
        rDesc, spilledRegion->getType () );
}

// Create a source region that could be used in place of the fill regvar.
// If the region is unaligned then the origin of the destination region
// is the displacement of the orginal region from its segment, else the
// origin is 0.

G4_SrcRegRegion *
SpillManagerGRF::createFillRangeSrcRegion (
    G4_RegVar *       fillRangeRegVar,
    G4_SrcRegRegion * filledRegion,
    unsigned          execSize
)
{
    // we need to preserve accRegSel if it's set
    if (isUnalignedRegion (filledRegion, execSize)) {
        unsigned segmentDisp =
            getEncAlignedSegmentDisp (filledRegion, execSize);
        unsigned regionDisp = getRegionDisp (filledRegion);
        assert (regionDisp >= segmentDisp);
        unsigned short subRegOff =
            (regionDisp - segmentDisp) / filledRegion->getElemSize ();
        assert (
            (regionDisp - segmentDisp) % filledRegion->getElemSize () == 0);

        return builder_->createSrcRegRegion(
            filledRegion->getModifier (), Direct, fillRangeRegVar, REG_ORIGIN,
            subRegOff, filledRegion->getRegion(), filledRegion->getType(), filledRegion->getAccRegSel());
    }
    else
    {
        return builder_->createSrcRegRegion(
            filledRegion->getModifier (), Direct, fillRangeRegVar,
            REG_ORIGIN, SUBREG_ORIGIN, filledRegion->getRegion (),
            filledRegion->getType(), filledRegion->getAccRegSel());
    }
}

// Create a source region for the spill regvar that can be used as an operand
// for a mov instruction used to copy the value to an send payload for
// an oword block write message. The spillRangeRegVar segment is guaranteed
// to start at an dword boundary and of a dword aligned size by construction.
// The whole spillRangeRegVar segment needs to be copied out to the send
// payload. The source region generated is <4;4,1>:ud so that a row occupies
// a packed oword. The exec size used in the copy instruction needs to be a
// multiple of 4 depending on the size of the spill regvar - 4 or 8 for the
// the spill regvar appearing as the destination in a regulat 2 cycle
// instructions and 16 when appearing in simd16 instructions.

inline G4_SrcRegRegion *
SpillManagerGRF::createBlockSpillRangeSrcRegion (
    G4_RegVar *       spillRangeRegVar,
    unsigned          regOff,
    unsigned          subregOff
)
{
    assert (getByteSize (spillRangeRegVar) % DWORD_BYTE_SIZE == 0);
    const RegionDesc * rDesc =
        builder_->rgnpool.createRegion (DWORD_BYTE_SIZE, DWORD_BYTE_SIZE, 1);
    return builder_->createSrcRegRegion(
        Mod_src_undef, Direct, spillRangeRegVar, (short) regOff, (short) subregOff,
        rDesc, Type_UD);
}

// Create a GRF regvar and a declare directive for it, to represent an
// implicit MFR live range that will be used as the send message payload
// header and write payload for spilling a regvar to memory.

G4_Declare *
SpillManagerGRF::createMRangeDeclare (
    G4_RegVar * regVar
)
{
    if (useSplitSend() && useScratchMsg_)
    {
        return builder_->getBuiltinR0();
    }

    G4_RegVar * repRegVar =
        (regVar->isRegVarTransient ())? regVar->getBaseRegVar (): regVar;
    const char * name =
        createImplicitRangeName (
            "SP_MSG", repRegVar, getMsgSpillIndex (repRegVar));
    unsigned regVarByteSize = getByteSize (regVar);
    unsigned writePayloadHeight = cdiv (regVarByteSize, REG_BYTE_SIZE);

    if (writePayloadHeight > SPILL_PAYLOAD_HEIGHT_LIMIT) {
        writePayloadHeight = SPILL_PAYLOAD_HEIGHT_LIMIT;
    }

    unsigned payloadHeaderHeight =
        (regVarByteSize != DWORD_BYTE_SIZE)?
        OWORD_PAYLOAD_HEADER_MAX_HEIGHT: DWORD_PAYLOAD_HEADER_MAX_HEIGHT;
    unsigned short height = payloadHeaderHeight + writePayloadHeight;
    unsigned short width = REG_DWORD_SIZE;

    // We should not find ourselves using dword scattered write
    if( useScratchMsg_ )
    {
        assert( payloadHeaderHeight != DWORD_PAYLOAD_HEADER_MAX_HEIGHT );
    }

    G4_Declare * msgRangeDeclare =
        createRangeDeclare (
            name,
            G4_GRF,
            width, height, Type_UD,
            DeclareType::Tmp, regVar->getNonTransientBaseRegVar (), NULL, 0);

    if( failSafeSpill_ )
    {
        msgRangeDeclare->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegStart_), 0);
    }

    return msgRangeDeclare;
}

// Create a GRF regvar and a declare directive for it, to represent an
// implicit MFR live range that will be used as the send message payload
// header and write payload for spilling a regvar region to memory.

G4_Declare *
SpillManagerGRF::createMRangeDeclare (
    G4_DstRegRegion * region,
    unsigned          execSize
)
{
    if (useSplitSend() && useScratchMsg_)
    {
        return builder_->getBuiltinR0();
    }

    const char * name =
        createImplicitRangeName (
            "SP_MSG", getRegVar (region),
            getMsgSpillIndex (getRegVar (region)));
    unsigned regionByteSize = getSegmentByteSize (region, execSize);
    unsigned writePayloadHeight = cdiv (regionByteSize, REG_BYTE_SIZE);
    unsigned msgType = getMsgType (region, execSize);
    unsigned payloadHeaderHeight =
        ( msgType == owordMask () ||
          msgType == hwordMask () )?
        OWORD_PAYLOAD_HEADER_MAX_HEIGHT: DWORD_PAYLOAD_HEADER_MAX_HEIGHT;

    // We should not find ourselves using dword scattered write
    if( useScratchMsg_ )
    {
        assert( payloadHeaderHeight != DWORD_PAYLOAD_HEADER_MAX_HEIGHT );
    }

    unsigned height = payloadHeaderHeight + writePayloadHeight;
    unsigned short width = REG_DWORD_SIZE;
    G4_Declare * msgRangeDeclare =
        createRangeDeclare (
            name,
            G4_GRF,
            width, (unsigned short) height, Type_UD,
            DeclareType::Tmp, region->getBase ()->asRegVar (), NULL, 0);

    if( failSafeSpill_ )
    {
        msgRangeDeclare->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegOffset_), 0);
        spillRegOffset_ += height;
    }

    return msgRangeDeclare;
}

// Create a GRF regvar and a declare directive for it, that will be used as
// the send message payload header and write payload for filling a regvar
// from memory.

G4_Declare *
SpillManagerGRF::createMRangeDeclare (
    G4_SrcRegRegion * region,
    unsigned          execSize
)
{
    if (useSplitSend() && useScratchMsg_)
    {
        return builder_->getBuiltinR0();
    }

    const char * name =
        createImplicitRangeName (
            "FL_MSG", getRegVar (region),
            getMsgFillIndex (getRegVar (region)));
    getSegmentByteSize(region, execSize);
    unsigned payloadHeaderHeight =
        (getMsgType (region, execSize) == owordMask ())?
        OWORD_PAYLOAD_HEADER_MIN_HEIGHT: DWORD_PAYLOAD_HEADER_MIN_HEIGHT;

    // We should not find ourselves using dword scattered write
    if( useScratchMsg_ )
    {
        assert( payloadHeaderHeight != DWORD_PAYLOAD_HEADER_MAX_HEIGHT );
        // When using scratch msg descriptor we dont need to use a
        // separate GRF for payload. Source operand of send can directly
        // use r0.0.
        return builder_->getBuiltinR0();
    }

    unsigned height = payloadHeaderHeight;
    unsigned width = REG_DWORD_SIZE;
    G4_Declare * msgRangeDeclare =
        createRangeDeclare (
            name,
            G4_GRF,
            (unsigned short) width, (unsigned short) height, Type_UD,
            DeclareType::Tmp, region->getBase ()->asRegVar (), NULL, 0);

    if( failSafeSpill_ )
    {
        msgRangeDeclare->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegOffset_), 0);
        spillRegOffset_ += height;
    }

    return msgRangeDeclare;
}

// Create a destination region for the GRF regvar for the write payload
// portion of the oword block send message (used for spill). The exec size
// can be either 4 or 8 for a regular 2 cycle instruction detination spills or
// 16 for simd16 instruction destination spills.

inline G4_DstRegRegion *
SpillManagerGRF::createMPayloadBlockWriteDstRegion (
    G4_RegVar *       grfRange,
    unsigned          regOff,
    unsigned          subregOff
)
{
    regOff += OWORD_PAYLOAD_WRITE_REG_OFFSET;
    subregOff += OWORD_PAYLOAD_WRITE_SUBREG_OFFSET;
    return builder_->createDst(
        grfRange, (short) regOff, (short) subregOff, DEF_HORIZ_STRIDE, Type_UD);
}

// Create a destination region for the GRF regvar for the input header
// payload portion of the send message to the data port. The exec size
// needs to be 8 for the mov instruction that uses this as a destination.

inline G4_DstRegRegion *
SpillManagerGRF::createMHeaderInputDstRegion (
    G4_RegVar *       grfRange,
    unsigned          subregOff
)
{
    return builder_->createDst(
        grfRange, PAYLOAD_INPUT_REG_OFFSET, (short) subregOff,
        DEF_HORIZ_STRIDE, Type_UD);
}

// Create a destination region for the GRF regvar for the payload offset
// portion of the oword block send message. The exec size needs to be 1
// for the mov instruction that uses this as a destination.

inline G4_DstRegRegion *
SpillManagerGRF::createMHeaderBlockOffsetDstRegion (
    G4_RegVar *       grfRange
)
{
    return builder_->createDst(
        grfRange, OWORD_PAYLOAD_SPOFFSET_REG_OFFSET,
        OWORD_PAYLOAD_SPOFFSET_SUBREG_OFFSET, DEF_HORIZ_STRIDE,
        Type_UD);
}

// Create a source region for the input payload (r0.0). The exec size
// needs to be 8 for the mov instruction that uses this as a source.

inline G4_SrcRegRegion *
SpillManagerGRF::createInputPayloadSrcRegion ()
{
    G4_RegVar * inputPayloadDirectReg = builder_->getBuiltinR0()->getRegVar();
    const RegionDesc * rDesc =
        builder_->rgnpool.createRegion (
            REG_DWORD_SIZE, REG_DWORD_SIZE, DEF_HORIZ_STRIDE);
    return builder_->createSrcRegRegion(
        Mod_src_undef, Direct, inputPayloadDirectReg,
        PAYLOAD_INPUT_REG_OFFSET, PAYLOAD_INPUT_SUBREG_OFFSET,
        rDesc, Type_UD);
}

// Create and initialize the message header for the send instruction for
// save/load of value to/from memory.
// The header includes the input payload and the offset (for spill disp).

template <class REGION_TYPE>
inline G4_Declare *
SpillManagerGRF::createAndInitMHeader (
    REGION_TYPE * region,
    unsigned      execSize
)
{
    G4_Declare * mRangeDcl = createMRangeDeclare (region, execSize);
    return initMHeader (mRangeDcl, region, execSize);
}

// Initialize the message header for the send instruction for save/load
// of value to/from memory.
// The header includes the input payload and the offset (for spill disp).

template <class REGION_TYPE>
G4_Declare *
SpillManagerGRF::initMHeader (
    G4_Declare *  mRangeDcl,
    REGION_TYPE * region,
    unsigned      execSize
)
{
    // Initialize the message header with the input payload.

    if ((useScratchMsg_ && mRangeDcl == builder_->getBuiltinR0()) || !headerNeeded())
    {
        // mRangeDcl is NULL for fills
        return mRangeDcl;
    }

    G4_DstRegRegion * mHeaderInputDstRegion =
        createMHeaderInputDstRegion (mRangeDcl->getRegVar ());
    G4_SrcRegRegion * inputPayload = createInputPayloadSrcRegion ();
    createMovInst (REG_DWORD_SIZE, mHeaderInputDstRegion, inputPayload);
    numGRFMove ++;

    if( useScratchMsg_ )
    {
        // Initialize msg header when region is a spill
        // When using scratch msg description, we only need to copy
        // r0.0 in to msg header. Memory offset will be
        // specified in the msg descriptor.
    }
    else
    // Initialize the message header with the spill disp for block
    // read/write.
    {
        G4_DstRegRegion * mHeaderOffsetDstRegion =
            createMHeaderBlockOffsetDstRegion (mRangeDcl->getRegVar ());
        int offset = getSegmentDisp(region, execSize);
        getSpillOffset(offset);
        unsigned segmentDisp = offset / OWORD_BYTE_SIZE;
        G4_Imm * segmentDispImm = builder_->createImm (segmentDisp, Type_UD);

        if (!region->isSrcRegRegion() && !region->isDstRegRegion())
        {
            MUST_BE_TRUE (false, ERROR_GRAPHCOLOR);
        }

        if (builder_->getIsKernel() == false)
        {
            createAddFPInst (
                SCALAR_EXEC_SIZE, mHeaderOffsetDstRegion, segmentDispImm);
        }
        else
        {
            createMovInst (
                SCALAR_EXEC_SIZE, mHeaderOffsetDstRegion, segmentDispImm);
        }
        numGRFMove ++;
    }

    // Initialize the message header with the spill disp for scatter
    // read/write.
    return mRangeDcl;
}

// Create and initialize the message header for the send instruction.
// The header includes the input payload (for spill disp).

inline G4_Declare *
SpillManagerGRF::createAndInitMHeader (
    G4_RegVar *             regVar
)
{
    G4_Declare * mRangeDcl = createMRangeDeclare (regVar);
    return initMHeader (mRangeDcl);
}

// Initialize the message header for the send instruction.
// The header includes the input payload (for spill disp).

G4_Declare *
SpillManagerGRF::initMHeader (
    G4_Declare *         mRangeDcl
)
{
    // Initialize the message header with the input payload.
    if ((useScratchMsg_ && mRangeDcl == builder_->getBuiltinR0()) || !headerNeeded())
    {
        // mRangeDcl is NULL for fills
        return mRangeDcl;
    }

    G4_DstRegRegion * mHeaderInputDstRegion =
        createMHeaderInputDstRegion (mRangeDcl->getRegVar ());
    G4_SrcRegRegion * inputPayload = createInputPayloadSrcRegion ();
    createMovInst (REG_DWORD_SIZE, mHeaderInputDstRegion, inputPayload);
    numGRFMove ++;

    return mRangeDcl;
}

// Initialize the the write payload part of the message for spilled regvars.
// Either of the following restrictions for spillRangeDcl are assumed:
//        - the regvar element type is dword and its 2 <= width <= 8 and
//        height - regOff == 1
//        - the regvar element type is dword and its width = 8 and
//        2 <= height - regOff <= 8
//      - the regvar element type is dword and its width and height are 1

void
SpillManagerGRF::initMWritePayload (
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    unsigned          regOff,
    unsigned          height
)
{
    if (useSplitSend())
    {
        // no need for payload moves if using sends
        return;
    }

    // We use an block write when the spilled regvars's segment is greater
    // than a dword. Generate a mov to copy the oword aligned segment into
    // the write payload part of the message.
    {
        unsigned nRows = height;

        for (unsigned i = 0; i < nRows; i++) {
            G4_SrcRegRegion * spillRangeSrcRegion =
                createBlockSpillRangeSrcRegion (
                    spillRangeDcl->getRegVar (), i + regOff);
            G4_DstRegRegion * mPayloadWriteDstRegion =
                createMPayloadBlockWriteDstRegion (
                    mRangeDcl->getRegVar (), i);
            unsigned char movExecSize =
                (nRows > 1)? REG_DWORD_SIZE: spillRangeDcl->getNumElems ();
            createMovInst (
                movExecSize, mPayloadWriteDstRegion, spillRangeSrcRegion);
            numGRFMove ++;
        }
    }
}

// Initialize the the write payload part of the message for spilled regions.

void
SpillManagerGRF::initMWritePayload (
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    G4_DstRegRegion * spilledRangeRegion,
    unsigned          execSize,
    unsigned          regOff
)
{
    // We use an block write when the spilled region's segment is greater
    // than a dword. Generate a mov to copy the oword aligned segment into
    // the write payload part of the message.
    if (useSplitSend())
    {
        // no need for payload moves
        return;
    }
    else
    {
        G4_SrcRegRegion * spillRangeSrcRegion =
            createBlockSpillRangeSrcRegion (
                spillRangeDcl->getRegVar (), regOff);
        G4_DstRegRegion * mPayloadWriteDstRegion =
            createMPayloadBlockWriteDstRegion (mRangeDcl->getRegVar ());
        unsigned segmentByteSize =
            getSegmentByteSize (spilledRangeRegion, execSize);
        unsigned char movExecSize = segmentByteSize / DWORD_BYTE_SIZE;

        // Write entire GRF when using scratch msg descriptor
        if( useScratchMsg_)
        {
            if( movExecSize <= 8 )
                movExecSize = 8;
            else if( movExecSize < 16 )
                movExecSize = 16;
        }

        assert (segmentByteSize % DWORD_BYTE_SIZE == 0);
        assert (movExecSize <= 16);
        createMovInst (
            movExecSize, mPayloadWriteDstRegion, spillRangeSrcRegion);
        numGRFMove ++;
    }
}

// Return the block size encoding for oword block reads.

inline unsigned
SpillManagerGRF::blockSendBlockSizeCode (
    unsigned size
)
{
    unsigned code;

    switch (size) {
        case 1:
            code = 0;
            break;
        case 2:
            code = 2;
            break;
        case 4:
            code = 3;
            break;
        case 8:
            code = 4;
            break;
        default:
            assert (0);
            code = 0;
    }

    return code << getSendDescDataSizeBitOffset ();
}

// Return the block size encoding for dword scatter reads.

inline unsigned
SpillManagerGRF::scatterSendBlockSizeCode (
    unsigned size
) const
{
    unsigned code;

    switch (size) {
        case 1:
            // We will use an exec size of 1 to perform 1 dword read/write.
        case 8:
            code = 0x02;
            break;
        case 16:
            code = 0x03;
            break;
        default:
            assert (0);
            code = 0;
    }

    return code << getSendDescDataSizeBitOffset ();
}

static uint32_t getScratchBlocksizeEncoding(int numGRF)
{

    int size = (numGRF * getGRFSize()) / 32; // in HWwords
    unsigned blocksize_encoding = 0;
    if (size == 1)
    {
        blocksize_encoding = 0x0;
    }
    else if (size == 2)
    {
        blocksize_encoding = 0x1;
    }
    else if (size == 4)
    {
        blocksize_encoding = 0x2;
    }
    else if (size == 8)
    {
        assert(getGenxPlatform() >= GENX_SKL);
        blocksize_encoding = 0x3;
    }
    else
        assert(false);
    return blocksize_encoding;
}

unsigned int SpillManagerGRF::createSpillSendMsgDescOWord(unsigned int height, unsigned int& execSize)
{
    unsigned segmentByteSize = height * REG_BYTE_SIZE;
    unsigned writePayloadCount = cdiv(segmentByteSize, REG_BYTE_SIZE);
    unsigned statelessSurfaceIndex = 0xFF;
    unsigned int message = statelessSurfaceIndex;

    unsigned headerPresent = 0x80000;
    message |= headerPresent;
    unsigned messageType = getSendOwordWriteType();
    message |= messageType << getSendWriteTypeBitOffset();
    unsigned payloadHeaderCount = OWORD_PAYLOAD_HEADER_MAX_HEIGHT;
    // split send not used since msg type is oword
    unsigned messageLength = writePayloadCount + payloadHeaderCount;
    message |= messageLength << getSendMsgLengthBitOffset();
    unsigned segmentOwordSize = cdiv(segmentByteSize, OWORD_BYTE_SIZE);
    message |= blockSendBlockSizeCode(segmentOwordSize);
    execSize = LIMIT_SEND_EXEC_SIZE(segmentOwordSize * DWORD_BYTE_SIZE);

    return message;
}

// Create the message descriptor for a spill send instruction for spilled
// post destinations of send instructions.

G4_Imm *
SpillManagerGRF::createSpillSendMsgDesc (
    unsigned    regOff,
    unsigned    height,
    unsigned &  execSize,
    G4_RegVar* base
)
{
    unsigned message = 0;

    if( useScratchMsg_)
    {
        unsigned headerPresent = 0x80000;
        message = headerPresent;
        unsigned msgLength = useSplitSend() ? SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT : SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT + height;
        message |= (msgLength << getSendMsgLengthBitOffset() );
        message |= (1 << SCRATCH_MSG_DESC_CATEORY);
        message |= (1 << SCRATCH_MSG_DESC_CHANNEL_MODE);
        message |= (1 << SCRATCH_MSG_DESC_OPERATION_MODE);
        unsigned blocksize_encoding = getScratchBlocksizeEncoding(height);
        message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
        int offset = getDisp(base);
        getSpillOffset(offset);
        // message expects offsets to be in HWord
        message |= (offset + regOff * getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;
        execSize = 16;
    }
    else
    {
        message = createSpillSendMsgDescOWord(height, execSize);
    }
    return builder_->createImm (message, Type_UD);
}

// Create the message descriptor for a spill send instruction for spilled
// destination regions.

G4_Imm *
SpillManagerGRF::createSpillSendMsgDesc (
    G4_DstRegRegion * spilledRangeRegion,
    unsigned &        execSize
)
{
    unsigned message = 0;

    if( useScratchMsg_)
    {
        /*
        bits    description
        18:0    function control
        19    Header present
        24:20    Response length
        28:25    Message length
        31:29    MBZ

        18:0
        11:0    Offset (12b hword offset)
        13:12    Block size (00 - 1 register, 01 - 2 regs, 10 - reserved, 11 - 4 regs)
        14    MBZ
        15    Invalidate after read (0 - no invalidate, 1 - invalidate)
        16    Channel mode (0 - oword, 1 - dword)
        17    Operation type (0 - read, 1 - write)
        18    Category (1 - scratch block read/write)
        */
        unsigned segmentByteSize = getSegmentByteSize (spilledRangeRegion, execSize);
        unsigned writePayloadCount = cdiv (segmentByteSize, REG_BYTE_SIZE);
        unsigned headerPresent = 0x80000;
        message |= headerPresent;

        unsigned payloadHeaderCount = SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT;
        // message length = 1 if we are using sends, 1 + payload otherwise
        unsigned messageLength = useSplitSend() ? payloadHeaderCount :
            writePayloadCount + payloadHeaderCount;
        message |= (messageLength << getSendMsgLengthBitOffset() );
        message |= (1 << SCRATCH_MSG_DESC_CATEORY); // category
        message |= (1 << SCRATCH_MSG_DESC_CHANNEL_MODE); // channel mode
        message |= (1 << SCRATCH_MSG_DESC_OPERATION_MODE); // write operation
        unsigned numGRFs = cdiv(segmentByteSize, G4_GRF_REG_NBYTES);

        unsigned blocksize_encoding = getScratchBlocksizeEncoding(numGRFs);

        message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
        int offset = getRegionDisp(spilledRangeRegion);
        getSpillOffset(offset);
        message |= offset >> SCRATCH_SPACE_ADDRESS_UNIT;
        if (numGRFs > 1)
        {
            execSize = 16;
        }
        else
        {
            if (execSize > 8)
            {
                execSize = 16;
            }
            else
            {
                execSize = 8;
            }
        }
    }
    else
    {
        unsigned segmentByteSize =
            getSegmentByteSize (spilledRangeRegion, execSize);
        unsigned writePayloadCount = cdiv (segmentByteSize, REG_BYTE_SIZE);
        unsigned statelessSurfaceIndex = 0xFF;
        message = statelessSurfaceIndex;

        unsigned headerPresent = 0x80000;
        message |= headerPresent;
        unsigned messageType = getSendOwordWriteType();
        message |= messageType << getSendWriteTypeBitOffset ();
        unsigned payloadHeaderCount = OWORD_PAYLOAD_HEADER_MAX_HEIGHT;
        unsigned messageLength = useSplitSend() ? payloadHeaderCount : writePayloadCount + payloadHeaderCount;
        message |= messageLength << getSendMsgLengthBitOffset ();
        unsigned segmentOwordSize = cdiv(segmentByteSize, OWORD_BYTE_SIZE);
        message |= blockSendBlockSizeCode (segmentOwordSize);
        execSize = LIMIT_SEND_EXEC_SIZE (segmentOwordSize * DWORD_BYTE_SIZE);
    }
    return builder_->createImm (message, Type_UD);
}


// Create the message descriptor for a spill send instruction for spilled
// destination regions.
G4_Imm *
SpillManagerGRF::createSpillSendMsgDesc(
    int size,
    int offset
)
{
    unsigned message = 0;

    if (useScratchMsg_)
    {
        /*
        bits    description
        18:0    function control
        19    Header present
        24:20    Response length
        28:25    Message length
        31:29    MBZ

        18:0
        11:0    Offset (12b hword offset)
        13:12    Block size (00 - 1 register, 01 - 2 regs, 10 - reserved, 11 - 4 regs)
        14    MBZ
        15    Invalidate after read (0 - no invalidate, 1 - invalidate)
        16    Channel mode (0 - oword, 1 - dword)
        17    Operation type (0 - read, 1 - write)
        18    Category (1 - scratch block read/write)
        */
        unsigned writePayloadCount = size;
        unsigned headerPresent = 0x80000;
        message |= headerPresent;

        unsigned payloadHeaderCount = SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT;
        // message length = 1 if we are using sends, 1 + payload otherwise
        unsigned messageLength = useSplitSend() ? payloadHeaderCount :
            writePayloadCount + payloadHeaderCount;
        message |= (messageLength << getSendMsgLengthBitOffset());
        message |= (1 << SCRATCH_MSG_DESC_CATEORY); // category
        message |= (1 << SCRATCH_MSG_DESC_CHANNEL_MODE); // channel mode
        message |= (1 << SCRATCH_MSG_DESC_OPERATION_MODE); // write operation
        unsigned numGRFs = size;

        unsigned blocksize_encoding = getScratchBlocksizeEncoding(numGRFs);

        message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
        message |= (offset >> SCRATCH_SPACE_ADDRESS_UNIT); // displacement
    }
    else
    {
        MUST_BE_TRUE(false, "should not reach here");
    }
    return builder_->createImm(message, Type_UD);
}

// Create an add instruction to add the FP needed for generating spill/fill code.
// We always set the NoMask flag and use a null conditional modifier.

inline G4_INST *
SpillManagerGRF::createAddFPInst (
    unsigned char      execSize,
    G4_DstRegRegion *      dst,
    G4_Operand *      src
)
{
    const RegionDesc* rDesc = builder_->getRegionScalar();
    G4_Operand* fp = builder_->createSrcRegRegion(
        Mod_src_undef, Direct, builder_->kernel.fg.framePtrDcl->getRegVar(),
        0, 0, rDesc, Type_UD);
    auto newInst = builder_->createBinOp(G4_add, execSize, dst, fp, src, InstOpt_WriteEnable, true);
    newInst->setCISAOff(curInst->getCISAOff());

    return newInst;

}

// Create a mov instruction needed for generating spill/fill code.
// We always set the NoMask flag and use a null conditional modifier.

inline G4_INST *
SpillManagerGRF::createMovInst (
    unsigned char      execSize,
    G4_DstRegRegion *      dst,
    G4_Operand *      src,
    G4_Predicate *    predicate,
    unsigned int      options
)
{
    auto newInst = builder_->createMov(execSize, dst, src, options, true);

    if (predicate)
    {
        newInst->setPredicate(predicate);
    }

    return newInst;
}

// Create a send instruction needed for generating spill/fill code.
// We always set the NoMask flag and use a null predicate and conditional
// modifier.

inline G4_INST *
SpillManagerGRF::createSendInst(
    unsigned char      execSize,
    G4_DstRegRegion *      postDst,
    G4_SrcRegRegion *      payload,
    G4_Imm *          desc,
    SFID funcID,
    bool              isWrite,
    unsigned          option
)
{
    // ToDo: create exDesc in createSendMsgDesc()
    uint32_t exDesc = G4_SendMsgDescriptor::createExtDesc(funcID);
    auto msgDesc = builder_->createSendMsgDesc(funcID, (uint32_t)desc->getInt(), exDesc, 0,
        isWrite ? SendAccess::WRITE_ONLY : SendAccess::READ_ONLY, nullptr);
    auto sendInst = builder_->createSendInst(
        NULL, G4_send, execSize, postDst,
        payload, desc, option, msgDesc);
    sendInst->setCISAOff(curInst->getCISAOff());

    return sendInst;
}

// Create the send instructions to fill in the value of spillRangeDcl into
// fillRangeDcl in aligned portions.

static int getNextSize(int height, bool useHWordMsg)
{

    bool has8GRFMessage = useHWordMsg && getGenxPlatform() >= GENX_SKL &&
        getGRFSize() == 32;
    if (has8GRFMessage && height >= 8)
    {
        return 8;
    }
    else if (height >= 4)
    {
        return 4;
    }
    else if (height >= 2)
    {
        return 2;
    }
    else if (height >= 1)
    {
        return 1;
    }
    return 0;
}

void
SpillManagerGRF::sendInSpilledRegVarPortions (
    G4_Declare *      fillRangeDcl,
    G4_Declare *      mRangeDcl,
    unsigned          regOff,
    unsigned          height,
    unsigned          srcRegOff
)
{
    //if (!headerNeeded())
    if ((useScratchMsg_ && mRangeDcl == builder_->getBuiltinR0()) || !headerNeeded())
    {
        // Skip initializing message header
    }
    else
    {
        // Initialize the message header with the spill disp for portion.
        int offset = getDisp(fillRangeDcl->getRegVar()) + regOff * REG_BYTE_SIZE;
        getSpillOffset(offset);

        unsigned segmentDisp = offset / OWORD_BYTE_SIZE;
        G4_Imm * segmentDispImm = builder_->createImm (segmentDisp, Type_UD);
        G4_DstRegRegion * mHeaderOffsetDstRegion =
            createMHeaderBlockOffsetDstRegion (mRangeDcl->getRegVar ());

        if (builder_->getIsKernel() == false)
        {
            createAddFPInst (
                SCALAR_EXEC_SIZE, mHeaderOffsetDstRegion, segmentDispImm);
        }
        else
        {
            createMovInst (SCALAR_EXEC_SIZE, mHeaderOffsetDstRegion, segmentDispImm);
        }
        numGRFMove ++;
    }

    // Read in the portions using a greedy approach.
    int currentStride = getNextSize(height, useScratchMsg_);

    if (currentStride)
    {
        createFillInstr(fillRangeDcl, mRangeDcl, regOff, currentStride, srcRegOff);
        if (height - currentStride > 0)
        {
            sendInSpilledRegVarPortions (
                fillRangeDcl, mRangeDcl, regOff + currentStride, height -currentStride, srcRegOff + currentStride);
        }
    }
}

// Check if we need to perform the pre-load of the spilled region's
// segment from spill memory. A pre-load is required under the following
// circumstances:
//        - for partial writes - horizontal stride greater than one, and when
//          the emask and predicate can possibly disable channels (for now if
//        predicates or condition modofoers are present then we conservatively
//        assume a partial write)
//        - write's where the segment is larger than the actaully written region
//        (either because the spill offset for the region or its size is not
//         oword or dword aligned for writing the exact region)

bool
SpillManagerGRF::shouldPreloadSpillRange(
    G4_INST* instContext,
    G4_BB* parentBB
)
{
    // Check for partial and unaligned regions and add pre-load code, if
    // necessary.
    auto spilledRangeRegion = instContext->getDst();
    uint8_t execSize = instContext->getExecSize();

    if (isPartialRegion(spilledRangeRegion, execSize) ||
        isUnalignedRegion(spilledRangeRegion, execSize) ||
        instContext->isPartialWriteForSpill(!parentBB->isAllLaneActive()))
    {
#if 0
        // special check for scalar variables: no need for pre-fill if instruction is not predicated
        // FIXME: need to update this if we ever decide to pack scalar variables in memory
        if (spilledRangeRegion->getTopDcl()->getNumElems() == 1 &&
            instContext->getPredicate() == nullptr)
        {
            return false;
        }
#endif
        return true;
    }
    // No pre-load for whole and aligned region writes
    else
    {
        return false;
    }
}

// Create the send instruction to perform the pre-load of the spilled region's
// segment into spill memory.

void SpillManagerGRF::preloadSpillRange (
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    G4_DstRegRegion * spilledRangeRegion,
    uint8_t     execSize
)
{
    // When execSize is 32, regions <32, 32, 1> or <64; 32, 2> are invalid.
    // Use a uniform region descriptor <stride; 1, 0>. Note that stride could
    // be 0 when execsize is 1.
    uint16_t hstride = spilledRangeRegion->getHorzStride();
    const RegionDesc *rDesc = builder_->createRegionDesc(execSize, hstride, 1, 0);

    G4_SrcRegRegion* preloadRegion = builder_->createSrcRegRegion(
        Mod_src_undef, Direct, spillRangeDcl->getRegVar(),
        REG_ORIGIN, spilledRangeRegion->getSubRegOff(),
        rDesc, spilledRangeRegion->getType());

    if( useScratchMsg_)
    {
        // src region's base refers to the filled region's base
        // The size of src region is equal to number of rows that
        // have to be filled, starting at the reg offset specified
        // in the original operand. For eg,
        // Let the spilled operand be V40(3,3)
        //
        // => mov (1) V40(3,3)<1>:ud    V30(0,0)<0;1,0>:ud
        // When this will be replaced with a preload fill,
        // => mov (1) TM_GRF_V40_0(0,0)<1>:ud   V30(0,0)<0;1,0>:ud
        // => send (16) SP_V40_0(0,0)<1>:ud ...                            <---  load V40's 3rd row in SP_V40_0
        // => mov (1) SP_V40_0(0,3)<1>:ud   TM_GRF_V40_0(0,0)<8;8,1>:ud <--- overlay
        // => send (16) null ...                                        <--- store V40's updated 3rd row to memory
        //
        // Since the filled register's register offset is 0,0 in first
        // send instruction, this change is made when creating the operand
        // itself.

        // Attach preloadRegion to dummy mov so getLeftBound/getRightBound won't crash when called from crossGRF in createFillSendMsgDesc
        builder_->createMov(execSize, builder_->createNullDst(Type_UD), preloadRegion, InstOpt_NoOpt, false);
        createFillSendInstr(spillRangeDcl, mRangeDcl, preloadRegion, execSize);
    }
    else
    {
        createFillSendInstr(spillRangeDcl, mRangeDcl, preloadRegion, execSize);
    }
}

static G4_SrcRegRegion* getSpillFillHeader(IR_Builder& builder, G4_Declare* decl)
{
    return builder.Create_Src_Opnd_From_Dcl(decl, builder.getRegionStride1());
}

// Create the send instruction to perform the spill of the spilled regvars's
// segment into spill memory.
// regOff - Offset of sub-spill. If one spill is splitted into more than one spill, this is the offset of them, unit in register size
// spillOff - Offset of the original variable being spilled, unit in register size.
G4_INST *
SpillManagerGRF::createSpillSendInstr (
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    unsigned          regOff,
    unsigned          height,
    unsigned          spillOff
)
{
    unsigned execSize (0);

    G4_Imm * messageDescImm = NULL;

    if( useScratchMsg_)
    {
        G4_RegVar* r = spillRangeDcl->getRegVar();
        G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
        messageDescImm =
            createSpillSendMsgDesc (spillOff, height, execSize, rvar->getBaseRegVar());
#ifdef _DEBUG
        int offset = (messageDescImm->getInt() & 0xFFF) * GENX_GRF_REG_SIZ;
        MUST_BE_TRUE(offset >= globalScratchOffset, "incorrect offset");
#endif
    }
    else
    {
        messageDescImm =
            createSpillSendMsgDesc (regOff, height, execSize);
    }

    G4_DstRegRegion * postDst = builder_->createNullDst(execSize > 8 ? Type_UW : Type_UD);

    G4_INST* sendInst = NULL;
    if (useSplitSend())
    {
        auto headerOpnd = getSpillFillHeader(*builder_, mRangeDcl);
        G4_SrcRegRegion* srcOpnd = createBlockSpillRangeSrcRegion(spillRangeDcl->getRegVar (), regOff);

        auto off = G4_SpillIntrinsic::InvalidOffset;
        G4_Declare* fp = nullptr;
        if (useScratchMsg_)
            off = (messageDescImm->getInt() & 0xfff);
        else
        {
            if (builder_->kernel.fg.getIsStackCallFunc() || builder_->kernel.fg.getHasStackCalls())
            {
                G4_RegVar* r = spillRangeDcl->getRegVar();
                G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
                int offset = getDisp(rvar->getBaseRegVar());
                getSpillOffset(offset);
                // message expects offsets to be in HWord
                off = (offset + spillOff * getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;
                fp = builder_->kernel.fg.getFramePtrDcl();
            }
        }
        sendInst = builder_->createSpill(postDst, headerOpnd, srcOpnd, execSize, height, off, fp, InstOpt_WriteEnable);
        sendInst->setLocation(curInst->getLocation());
        sendInst->setCISAOff(curInst->getCISAOff());
    }
    else
    {
        G4_SrcRegRegion * payload = builder_->createSrcRegRegion(Mod_src_undef, Direct,
            mRangeDcl->getRegVar(), 0, 0, builder_->getRegionStride1(), Type_UD);
        sendInst = createSendInst ((unsigned char) execSize, postDst, payload, messageDescImm, SFID::DP_DC, true, InstOpt_WriteEnable);
    }

    return sendInst;
}

// Create the send instruction to perform the spill of the spilled region's
// segment into spill memory.

G4_INST *
SpillManagerGRF::createSpillSendInstr (
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    G4_DstRegRegion * spilledRangeRegion,
    unsigned          execSize,
    unsigned          option
)
{
    G4_Imm * messageDescImm =
        createSpillSendMsgDesc (spilledRangeRegion, execSize);

#ifdef _DEBUG
    if (useScratchMsg_)
    {
        int offset = (messageDescImm->getInt() & 0xFFF) * GENX_GRF_REG_SIZ;
        MUST_BE_TRUE(offset >= globalScratchOffset, "incorrect offset");
    }
#endif

    G4_DstRegRegion * postDst = builder_->createNullDst(execSize > 8 ? Type_UW : Type_UD);

    G4_INST* sendInst = NULL;
    if (useSplitSend())
    {
        unsigned extMsgLength = spillRangeDcl->getNumRows();
        const RegionDesc* region = builder_->getRegionStride1();
        auto headerOpnd = getSpillFillHeader(*builder_, mRangeDcl);
        G4_SrcRegRegion* srcOpnd = builder_->Create_Src_Opnd_From_Dcl(spillRangeDcl, region);

        auto off = G4_SpillIntrinsic::InvalidOffset;
        G4_Declare* fp = nullptr;
        if (useScratchMsg_)
            off = (messageDescImm->getInt() & 0xfff);
        else
        {
            if (builder_->kernel.fg.getIsStackCallFunc() || builder_->kernel.fg.getHasStackCalls())
            {
                G4_RegVar* r = spillRangeDcl->getRegVar();
                G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
                int offset = getDisp(rvar->getBaseRegVar());
                getSpillOffset(offset);
                // message expects offsets to be in HWord
                auto regOff = spilledRangeRegion->getRegOff();
                off = (offset + regOff * getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;
                fp = builder_->kernel.fg.getFramePtrDcl();
            }
        }
        sendInst = builder_->createSpill(postDst, headerOpnd, srcOpnd, execSize, (uint16_t)extMsgLength,
            off, fp, static_cast<G4_InstOption>(option));
        sendInst->setLocation(curInst->getLocation());
        sendInst->setCISAOff(curInst->getCISAOff());
    }
    else
    {
        G4_SrcRegRegion * payload = builder_->createSrcRegRegion(Mod_src_undef, Direct,
            mRangeDcl->getRegVar(), 0, 0, builder_->getRegionStride1(), Type_UD);
        sendInst = createSendInst((unsigned char)execSize, postDst, payload, messageDescImm, SFID::DP_DC, true, static_cast<G4_InstOption>(option));
    }

    return sendInst;
}

// Create the message description for a fill send instruction for filled
// regvars.

G4_Imm *
SpillManagerGRF::createFillSendMsgDesc (
    unsigned          regOff,
    unsigned          height,
    unsigned &        execSize,
    G4_RegVar* base
)
{
    unsigned message = 0;

    if( useScratchMsg_)
    {
        unsigned segmentByteSize = height * REG_BYTE_SIZE;
        unsigned responseLength = cdiv (segmentByteSize, REG_BYTE_SIZE);
        message = responseLength << getSendRspLengthBitOffset ();
        unsigned headerPresent = 0x80000;
        message |= SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT << getSendMsgLengthBitOffset ();
        message |= headerPresent;

        message |= (1 << SCRATCH_MSG_DESC_CATEORY);
        message |= (0 << SCRATCH_MSG_INVALIDATE_AFTER_READ);
        unsigned blocksize_encoding = getScratchBlocksizeEncoding(height);

        message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);

        int offset = getDisp(base);
        getSpillOffset(offset);
        // message expects offsets to be in HWord
        message |= (offset + regOff * getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;

        execSize = 16;
    }
    else
    {
        unsigned segmentByteSize = height * REG_BYTE_SIZE;
        unsigned statelessSurfaceIndex = 0xFF;
        unsigned responseLength = cdiv (segmentByteSize, REG_BYTE_SIZE);
        responseLength = responseLength << getSendRspLengthBitOffset ();
        message = statelessSurfaceIndex | responseLength;

        unsigned headerPresent = 0x80000;
        message |= headerPresent;
        unsigned messageType = getSendOwordReadType ();
        message |= messageType << getSendReadTypeBitOffset ();
        unsigned messageLength = OWORD_PAYLOAD_HEADER_MIN_HEIGHT;
        message |= messageLength << getSendMsgLengthBitOffset ();
        unsigned segmentOwordSize =
            cdiv (segmentByteSize, OWORD_BYTE_SIZE);
        assert (segmentOwordSize <= 8);
        message |= blockSendBlockSizeCode (segmentOwordSize);
        execSize = LIMIT_SEND_EXEC_SIZE (segmentOwordSize * DWORD_BYTE_SIZE);
    }
    return builder_->createImm (message, Type_UD);
}

// Create the message description for a fill send instruction for filled
// source regions.

template <class REGION_TYPE>
G4_Imm *
SpillManagerGRF::createFillSendMsgDesc (
    REGION_TYPE * filledRangeRegion,
    unsigned &    execSize
)
{
    unsigned message = 0;

    if( useScratchMsg_)
    {
        unsigned segmentByteSize =
            getSegmentByteSize (filledRangeRegion, execSize);
        if (filledRangeRegion->crossGRF()) {
            segmentByteSize = 2 * REG_BYTE_SIZE;
        }

        unsigned responseLength = cdiv (segmentByteSize, REG_BYTE_SIZE);
        message = responseLength << getSendRspLengthBitOffset ();

        unsigned headerPresent = 0x80000;
        message |= headerPresent;

        message |= (SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT << getSendMsgLengthBitOffset());
        message |= (1 << SCRATCH_MSG_DESC_CATEORY);
        message |= (0 << SCRATCH_MSG_INVALIDATE_AFTER_READ);
        // Scratch msg descriptor requires a special encoding for block size
        /*
        00 - 1 GRF
        01 - 2 GRFs
        10 - reserved
        11 - 4 GRFs
        */
        unsigned blocksize_encoding = getScratchBlocksizeEncoding(responseLength);

        message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
        int offset = getRegionDisp(filledRangeRegion);
        getSpillOffset(offset);
        message |= offset >> SCRATCH_SPACE_ADDRESS_UNIT;

        execSize = 16;
    }
    else
    {
        unsigned segmentByteSize =
            getSegmentByteSize (filledRangeRegion, execSize);
        unsigned statelessSurfaceIndex = 0xFF;
        unsigned responseLength = cdiv (segmentByteSize, REG_BYTE_SIZE);
        responseLength = responseLength << getSendRspLengthBitOffset ();
        message = statelessSurfaceIndex | responseLength;

        unsigned headerPresent = 0x80000;
        message |= headerPresent;
        unsigned messageType = getSendOwordReadType ();
        message |= messageType << getSendReadTypeBitOffset ();
        unsigned messageLength = OWORD_PAYLOAD_HEADER_MIN_HEIGHT;
        message |= messageLength << getSendMsgLengthBitOffset ();
        unsigned segmentOwordSize =
            cdiv (segmentByteSize, OWORD_BYTE_SIZE);
        message |= blockSendBlockSizeCode (segmentOwordSize);
        execSize = LIMIT_SEND_EXEC_SIZE (segmentOwordSize * DWORD_BYTE_SIZE);
    }
    return builder_->createImm (message, Type_UD);
}


// size -- number of GRFs to read
// offset -- in bytes
G4_Imm* SpillManagerGRF::createFillSendMsgDesc(
    int size,
    int offset)
{
    uint32_t message = 0;
    if (useScratchMsg_)
    {
        unsigned responseLength = size;
        message = responseLength << getSendRspLengthBitOffset();

        unsigned headerPresent = 0x80000;
        message |= headerPresent;

        message |= (SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT << getSendMsgLengthBitOffset());
        message |= (1 << SCRATCH_MSG_DESC_CATEORY);
        message |= (0 << SCRATCH_MSG_INVALIDATE_AFTER_READ);
        // Scratch msg descriptor requires a special encoding for block size
        /*
        00 - 1 GRF
        01 - 2 GRFs
        10 - reserved
        11 - 4 GRFs
        */
        unsigned blocksize_encoding = getScratchBlocksizeEncoding(responseLength);

        message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
        message |= (offset >> SCRATCH_SPACE_ADDRESS_UNIT);
    }
    else
    {
        MUST_BE_TRUE(false, "should not reach here");
    }
    return builder_->createImm(message, Type_UD);
}

G4_INST*
SpillManagerGRF::createFillInstr(
    G4_Declare* fillRangeDcl,
    G4_Declare* mRangeDcl,
    unsigned          regOff,
    unsigned          height,
    unsigned          srcRegOff
)
{
    return createFillSendInstr(fillRangeDcl, mRangeDcl, regOff, height, srcRegOff);
}

// Create the send instruction to perform the fill of the spilled regvars's
// segment from spill memory.
// spillOff - spill offset to the fillRangeDcl, in unit of grf size
G4_INST *
SpillManagerGRF::createFillSendInstr (
    G4_Declare *      fillRangeDcl,
    G4_Declare *      mRangeDcl,
    unsigned          regOff,
    unsigned          height,
    unsigned          spillOff
)
{
    unsigned execSize (0);

    G4_Imm * messageDescImm = NULL;

    if( useScratchMsg_)
    {
        G4_RegVar* r = fillRangeDcl->getRegVar();
        G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
        messageDescImm =
            createFillSendMsgDesc (spillOff, height, execSize, rvar->getBaseRegVar());
#ifdef _DEBUG
        int offset = (messageDescImm->getInt() & 0xFFF) * GENX_GRF_REG_SIZ;
        MUST_BE_TRUE(offset >= globalScratchOffset, "incorrect offset");
#endif
    }
    else
    {
        messageDescImm =
            createFillSendMsgDesc (regOff, height, execSize);
    }

    G4_DstRegRegion * postDst = builder_->createDst(
        fillRangeDcl->getRegVar (), (short) regOff, SUBREG_ORIGIN,
        DEF_HORIZ_STRIDE, (execSize > 8)? Type_UW: Type_UD);

    auto payload = getSpillFillHeader(*builder_, mRangeDcl);

    unsigned int off = G4_FillIntrinsic::InvalidOffset;
    G4_Declare* fp = nullptr;
    if (useScratchMsg_)
        off = (messageDescImm->getInt() & 0xfff);
    else
    {
        if (builder_->kernel.fg.getIsStackCallFunc() || builder_->kernel.fg.getHasStackCalls())
        {
            // compute hword offset to emit later when expanding spill/fill intrinsic
            G4_RegVar* r = fillRangeDcl->getRegVar();
            G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
            int offset = getDisp(rvar->getBaseRegVar());
            getSpillOffset(offset);
            // message expects offsets to be in HWord
            off = (offset + spillOff * getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;
            fp = builder_->kernel.fg.getFramePtrDcl();
        }
    }
    auto fillInst = builder_->createFill(payload, postDst, execSize, height, off, fp, InstOpt_WriteEnable);
    fillInst->setLocation(curInst->getLocation());
    fillInst->setCISAOff(curInst->getCISAOff());
    return fillInst;

}

// Create the send instruction to perform the fill of the filled region's
// segment into fill memory.

G4_INST *
SpillManagerGRF::createFillSendInstr (
    G4_Declare *      fillRangeDcl,
    G4_Declare *      mRangeDcl,
    G4_SrcRegRegion * filledRangeRegion,
    unsigned          execSize
)
{
    auto oldExecSize = execSize;
    G4_Imm * messageDescImm =
        createFillSendMsgDesc (filledRangeRegion, execSize);

    if( useScratchMsg_)
    {
        execSize = 16;
    }

    G4_DstRegRegion * postDst = builder_->createDst(
        fillRangeDcl->getRegVar (), 0, SUBREG_ORIGIN,
        DEF_HORIZ_STRIDE, (execSize > 8)? Type_UW : Type_UD);

    auto payload = getSpillFillHeader(*builder_, mRangeDcl);

    unsigned int off = G4_FillIntrinsic::InvalidOffset;
    unsigned segmentByteSize = getSegmentByteSize(filledRangeRegion, oldExecSize);
    G4_Declare* fp = nullptr;
    if (useScratchMsg_)
    {
        off = (messageDescImm->getInt() & 0xfff);
        if (filledRangeRegion->crossGRF())
        {
            segmentByteSize = 2 * REG_BYTE_SIZE;
        }
    }
    else
    {
        if (builder_->kernel.fg.getIsStackCallFunc() || builder_->kernel.fg.getHasStackCalls())
        {
            // compute hword offset to emit later when expanding spill/fill intrinsic
            int offset = getRegionDisp(filledRangeRegion);
            getSpillOffset(offset);
            off = offset >> SCRATCH_SPACE_ADDRESS_UNIT;
            fp = builder_->kernel.fg.getFramePtrDcl();
        }
    }

    unsigned responseLength = cdiv(segmentByteSize, REG_BYTE_SIZE);
    auto fillInst = builder_->createFill(payload, postDst, execSize, responseLength, off, fp, InstOpt_WriteEnable);
    fillInst->setLocation(curInst->getLocation());
    fillInst->setCISAOff(curInst->getCISAOff());
    return fillInst;
}

// Replace the reference to the spilled region with a reference to an
// equivalent reference to the spill range region.

void
SpillManagerGRF::replaceSpilledRange (
    G4_Declare *      spillRangeDcl,
    G4_DstRegRegion * spilledRegion,
    G4_INST *         spilledInst
)
{
    // we need to preserve accRegSel if it's set
    G4_DstRegRegion * tmpRangeDstRegion = builder_->createDst(
        spillRangeDcl->getRegVar (), REG_ORIGIN, SUBREG_ORIGIN,
        spilledRegion->getHorzStride (), spilledRegion->getType(), spilledRegion->getAccRegSel() );
    spilledInst->setDest (tmpRangeDstRegion);
}

// Replace the reference to the filled region with a reference to an
// equivalent reference to the fill range region.

void
SpillManagerGRF::replaceFilledRange (
    G4_Declare *      fillRangeDcl,
    G4_SrcRegRegion * filledRegion,
    G4_INST *         filledInst
)
{
    unsigned execSize =
        isMultiRegComprSource (filledRegion, filledInst)?
        filledInst->getExecSize () / 2:
        filledInst->getExecSize ();

    for (int i = 0; i < G4_MAX_SRCS; i++) {
        G4_Operand * src = filledInst->getSrc(i);

        if (src != NULL && src->isSrcRegRegion())
        {
            G4_SrcRegRegion* srcRgn = src->asSrcRegRegion();
            if (*srcRgn == *filledRegion)
            {
                G4_SrcRegRegion* fillRangeSrcRegion =
                    createFillRangeSrcRegion(
                        fillRangeDcl->getRegVar(), filledRegion, execSize);
                filledInst->setSrc(fillRangeSrcRegion, i);
            }
        }
    }
}

// Create the send instructions to write out the spillRangeDcl in aligned
// portions.
void
SpillManagerGRF::sendOutSpilledRegVarPortions (
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    unsigned          regOff,
    unsigned          height,
    unsigned          srcRegOff
)
{
    if(!headerNeeded())
    {
        // No need to make a copy of offset because when using
        // scratch msg descriptor, the offset is part of send
        // msg descriptor and not the header.
    }
    else
    {
        // Initialize the message header with the spill disp for portion.
        int offset = getDisp(spillRangeDcl->getRegVar()) + regOff * REG_BYTE_SIZE;
        getSpillOffset(offset);
        unsigned segmentDisp = offset / OWORD_BYTE_SIZE;

        G4_Imm * segmentDispImm = builder_->createImm (segmentDisp, Type_UD);
        G4_DstRegRegion * mHeaderOffsetDstRegion =
            createMHeaderBlockOffsetDstRegion (mRangeDcl->getRegVar ());

        if (builder_->getIsKernel() == false)
        {
            createAddFPInst (
                SCALAR_EXEC_SIZE, mHeaderOffsetDstRegion, segmentDispImm);
        }
        else
        {
            createMovInst (SCALAR_EXEC_SIZE, mHeaderOffsetDstRegion, segmentDispImm);
        }
        numGRFMove ++;
    }


    // Write out the portions using a greedy approach.
    int currentStride = getNextSize(height, useScratchMsg_);

    if (currentStride)
    {
        initMWritePayload (spillRangeDcl, mRangeDcl, regOff, currentStride);

        createSpillSendInstr(spillRangeDcl, mRangeDcl, regOff, currentStride, srcRegOff);

        if (height - currentStride > 0) {
            sendOutSpilledRegVarPortions (
                spillRangeDcl, mRangeDcl, regOff + currentStride, height - currentStride, srcRegOff + currentStride);
        }
    }
}

// Create the code to create the spill range and save it to spill memory.

INST_LIST::iterator
SpillManagerGRF::insertSpillRangeCode (
    INST_LIST::iterator spilledInstIter,
    G4_BB* bb
)
{
    unsigned char execSize = (*spilledInstIter)->getExecSize ();
    G4_Declare * replacementRangeDcl;
    builder_->instList.clear();

    bool optimizeSplitLLR = false;
    G4_INST* inst = *spilledInstIter;
    G4_INST* spillSendInst = NULL;
    auto spilledRegion = inst->getDst();

    // Handle send instructions (special treatment)
    // Create the spill range for the whole post destination, assign spill
    // offset to the spill range and create the instructions to load the
    // save the spill range to spill memory.

    if (inst->mayExceedTwoGRF())
    {
        INST_LIST::iterator sendOutIter = spilledInstIter;
        assert (getRFType (spilledRegion) == G4_GRF);
        G4_Declare * spillRangeDcl =
            createPostDstSpillRangeDeclare (*sendOutIter, spilledRegion);
        G4_Declare * mRangeDcl =
            createAndInitMHeader (
                (G4_RegVarTransient *) spillRangeDcl->getRegVar ());

        // Assumption here is that a NoMask send doesn't need read-modify-write
        // since it writes the entire GRF(s).
        // May need to revisit if we have some strange sends that update partial GRFs. (e.g., SIMD4 scatter read)
        if (!inst->isWriteEnableInst())
        {
            sendInSpilledRegVarPortions(
                spillRangeDcl, mRangeDcl, 0,
                spillRangeDcl->getNumRows(),
                spilledRegion->getRegOff());

            INST_LIST::iterator insertPos = sendOutIter;
            bb->splice(insertPos, builder_->instList);
        }

        sendOutSpilledRegVarPortions (
            spillRangeDcl, mRangeDcl, 0, spillRangeDcl->getNumRows (),
            spilledRegion->getRegOff());

        replacementRangeDcl = spillRangeDcl;
    }

    // Handle other regular single/multi destination register instructions.
    // Create the spill range for the destination region, assign spill
    // offset to the spill range and create the instructions to load the
    // save the spill range to spill memory.
    else {
        // Create the segment aligned spill range

        G4_Declare * spillRangeDcl =
            createSpillRangeDeclare (
                spilledRegion, execSize,
                *spilledInstIter);

        // Create and initialize the message header

        G4_Declare * mRangeDcl =
            createAndInitMHeader (spilledRegion, execSize);

        // Unaligned region specific handling.

        unsigned int spillSendOption = InstOpt_WriteEnable;
        if (shouldPreloadSpillRange (*spilledInstIter, bb)) {

            // Preload the segment aligned spill range from memory to use
            // as an overlay

            preloadSpillRange (
                spillRangeDcl, mRangeDcl, spilledRegion, execSize);

            // Create the temporary range to use as a replacement range.

            G4_Declare * tmpRangeDcl =
                createTemporaryRangeDeclare (spilledRegion, execSize);

            // Copy out the value in the temporary range into its
            // location in the spill range.

            G4_DstRegRegion * spillRangeDstRegion =
                createSpillRangeDstRegion (
                    spillRangeDcl->getRegVar (), spilledRegion, execSize);

            G4_SrcRegRegion * tmpRangeSrcRegion =
                createTemporaryRangeSrcRegion (
                    tmpRangeDcl->getRegVar (), spilledRegion, execSize);

            // NOTE: Never use a predicate for the final mov if the spilled
            //       instruction was a sel (even in a SIMD CF context).

            G4_Predicate* predicate =
                ((*spilledInstIter)->opcode() != G4_sel)?
                (*spilledInstIter)->getPredicate () : nullptr;

            if (tmpRangeSrcRegion->getType() == spillRangeDstRegion->getType() && IS_TYPE_FLOAT_ALL(tmpRangeSrcRegion->getType()))
            {
                // use int copy when possible as floating-point copy moves may need further legalization
                auto equivIntTy = floatToSameWidthIntType(tmpRangeSrcRegion->getType());
                tmpRangeSrcRegion->setType(equivIntTy);
                spillRangeDstRegion->setType(equivIntTy);
            }

            createMovInst (
                execSize, spillRangeDstRegion, tmpRangeSrcRegion,
                builder_->duplicateOperand(predicate),
                (*spilledInstIter)->getMaskOption());
            numGRFMove ++;

            replacementRangeDcl = tmpRangeDcl;
        }

        // Aligned regions do not need a temporary range.

        else {
            LocalLiveRange* spilledLLR = gra.getLocalLR(spilledRegion->getBase()->asRegVar()->getDeclare());
            if (spilledLLR && spilledLLR->getSplit())
            {
                // if we are spilling the dest of a copy move introduced by local live-range splitting,
                // we can spill the source value instead and delete the move
                // ToDo: we should generalize this to cover all moves
                G4_SrcRegRegion* srcRegion = inst->getSrc(0)->asSrcRegRegion();
                G4_Declare* srcDcl = srcRegion->getBase()->asRegVar()->getDeclare();
                unsigned int lb = srcRegion->getLeftBound();
                unsigned int rb = srcRegion->getRightBound();

                G4_RegVar * regVar = NULL;
                if (srcRegion->getBase()->isRegVar())
                {
                    regVar = getRegVar(srcRegion);
                }

                if (gra.getSubRegAlign(srcDcl) == GRFALIGN &&
                    lb %  REG_BYTE_SIZE == 0 &&
                    (rb + 1) % REG_BYTE_SIZE == 0 &&
                    (rb - lb + 1) == spillRangeDcl->getByteSize() &&
                    regVar &&
                    !shouldSpillRegister(regVar))
                {
                    optimizeSplitLLR = true;
                }
            }

            replacementRangeDcl = spillRangeDcl;
            if (!bb->isAllLaneActive())
            {
                spillSendOption = (*spilledInstIter)->getMaskOption();
            }
        }

        // Save the spill range to memory.
        {
            initMWritePayload(
                spillRangeDcl, mRangeDcl, spilledRegion, execSize);

            spillSendInst = createSpillSendInstr(
                spillRangeDcl, mRangeDcl, spilledRegion, execSize, spillSendOption);
        }
        if (failSafeSpill_)
        {
            spillRegOffset_ = spillRegStart_;
        }
    }

    // Replace the spilled range with the spill range and insert spill
    // instructions.

    INST_LIST::iterator insertPos = spilledInstIter;
    insertPos++;
    replaceSpilledRange (replacementRangeDcl, spilledRegion, *spilledInstIter);
    INST_LIST::iterator nextIter = spilledInstIter;
    ++nextIter;

    bb->splice (insertPos, builder_->instList);

    if (optimizeSplitLLR && spillSendInst && spillSendInst->isSplitSend())
    {
        // delete the move and spill the source instead. Note that we can't do this if split send
        // is not enabled, as payload contains header
        bb->erase(spilledInstIter);
        unsigned int pos = 1;
        spillSendInst->setSrc(inst->getSrc(0), pos);
    }
    else
    {
        bb->splice(spilledInstIter, builder_->instList);
    }

    return nextIter;
}

// Create the code to create the GRF fill range and load it to spill memory.

INST_LIST::iterator
SpillManagerGRF::insertFillGRFRangeCode (
    G4_SrcRegRegion *   filledRegion,
    INST_LIST::iterator filledInstIter,
    G4_BB* bb
)
{
    unsigned  execSize = (*filledInstIter)->getExecSize ();

    // Create the fill range, assign spill offset to the fill range and
    // create the instructions to load the fill range from spill memory.

    G4_Declare * fillRangeDcl = nullptr;

    bool optimizeSplitLLR = false;
    G4_INST* inst = *filledInstIter;
    G4_DstRegRegion* dstRegion = inst->getDst();
    G4_INST* fillSendInst = NULL;

    {
        fillRangeDcl =
            createGRFFillRangeDeclare(
                filledRegion, execSize,
                *filledInstIter);
        G4_Declare * mRangeDcl =
            createAndInitMHeader(filledRegion, execSize);
        fillSendInst = createFillSendInstr(fillRangeDcl, mRangeDcl, filledRegion, execSize);

        LocalLiveRange* filledLLR = gra.getLocalLR(filledRegion->getBase()->asRegVar()->getDeclare());
        if (filledLLR && filledLLR->getSplit())
        {
            G4_Declare* dstDcl = dstRegion->getBase()->asRegVar()->getDeclare();
            unsigned int lb = dstRegion->getLeftBound();
            unsigned int rb = dstRegion->getRightBound();

            if (gra.getSubRegAlign(dstDcl) == GRFALIGN  &&
                lb %  REG_BYTE_SIZE == 0 &&
                (rb + 1) % REG_BYTE_SIZE == 0 &&
                (rb - lb + 1) == fillRangeDcl->getByteSize())
            {
                optimizeSplitLLR = true;
            }
        }
    }

    // Replace the spilled range with the fill range and insert spill
    // instructions.

    replaceFilledRange (fillRangeDcl, filledRegion, *filledInstIter);
    INST_LIST::iterator insertPos = filledInstIter;

    bb->splice (insertPos, builder_->instList);
    if (optimizeSplitLLR)
    {
        INST_LIST::iterator nextIter = filledInstIter;
        INST_LIST::iterator prevIter = filledInstIter;
        nextIter++;
        prevIter--;
        prevIter--;
        bb->erase(filledInstIter);
        fillSendInst->setDest(dstRegion);
        G4_INST* prevInst = (*prevIter);
        if (prevInst->isPseudoKill() &&
            GetTopDclFromRegRegion(prevInst->getDst()) == fillRangeDcl)
        {
            prevInst->setDest(builder_->createDst(GetTopDclFromRegRegion(dstRegion)->getRegVar(), 0, 0, 1, Type_UD));
        }
        return nextIter;
    }
    else
    {
        return ++filledInstIter;
    }
}

// Create the code to create the GRF fill range and load it to spill memory.

INST_LIST::iterator
SpillManagerGRF::insertSendFillRangeCode (
    G4_SrcRegRegion *   filledRegion,
    INST_LIST::iterator filledInstIter,
    G4_BB* bb
)
{
    G4_INST * sendInst = *filledInstIter;

    unsigned width = REG_BYTE_SIZE / filledRegion->getElemSize();

    // Create the fill range, assign spill offset to the fill range

    G4_Declare * fillGRFRangeDcl =
        createSendFillRangeDeclare(filledRegion, sendInst);

    // Create the instructions to load the fill range from spill memory.

    G4_Declare * mRangeDcl = createMRangeDeclare(filledRegion, width);
    initMHeader(mRangeDcl);
    sendInSpilledRegVarPortions(
        fillGRFRangeDcl, mRangeDcl, 0,
        fillGRFRangeDcl->getNumRows(), filledRegion->getRegOff());

    // Replace the spilled range with the fill range and insert spill
    // instructions.

    replaceFilledRange(fillGRFRangeDcl, filledRegion, *filledInstIter);
    INST_LIST::iterator insertPos = filledInstIter;

    bb->splice(insertPos, builder_->instList);

    // Return the next instruction

    return ++filledInstIter;
}

G4_Declare* getOrCreateSpillFillDcl(G4_Declare* spilledAddrTakenDcl, G4_Kernel* kernel)
{
    // If spilledAddrTakenDcl already has a spill/fill range created, return it.
    // Else create new one and return it.
    G4_Declare* temp = spilledAddrTakenDcl->getAddrTakenSpillFill();
    if (temp == NULL)
    {
#define ADDR_SPILL_FILL_NAME_SIZE 32
        const char* dclName = kernel->fg.builder->getNameString(kernel->fg.mem, ADDR_SPILL_FILL_NAME_SIZE,
            "ADDR_SP_FL_V%d", spilledAddrTakenDcl->getDeclId());

        // temp is created of sub-class G4_RegVarTmp so that is
        // assigned infinite spill cost when coloring.
        temp = kernel->fg.builder->createDeclareNoLookup((const char*)dclName,
            G4_GRF, spilledAddrTakenDcl->getNumElems(),
            spilledAddrTakenDcl->getNumRows(), spilledAddrTakenDcl->getElemType() , DeclareType::Tmp, spilledAddrTakenDcl->getRegVar());
        spilledAddrTakenDcl->setAddrTakenSpillFill(temp);
    }

    return temp;
}

// For each address taken register spill find an available physical register
// and assign it to the decl. This physical register will be used for inserting
// spill/fill code for indirect reference instructions that point to the
// spilled range.
// Return true if enough registers found, false if sufficient registers unavailable.
bool SpillManagerGRF::handleAddrTakenSpills( G4_Kernel * kernel, PointsToAnalysis& pointsToAnalysis )
{
    bool success = true;
    unsigned int numAddrTakenSpills = 0;

    for (LR_LIST::const_iterator lt = spilledLRs_.begin (), end = spilledLRs_.end();
        lt != end; ++lt)
    {
        LiveRange* lr = (*lt);

        if( lr->getDcl()->getAddressed() )
        {
            getOrCreateSpillFillDcl(lr->getDcl(), kernel);
        }

        if( lvInfo_->isAddressSensitive( lr->getVar()->getId() ) )
        {
            numAddrTakenSpills++;
        }
    }

    if(numAddrTakenSpills > 0)
    {
        insertAddrTakenSpillFill( kernel, pointsToAnalysis );
        prunePointsTo( kernel, pointsToAnalysis );
    }

#ifdef _DEBUG
    if( success )
    {
        // Verify that each spilled address taken has a spill/fill registers assigned
        for (LR_LIST::const_iterator lt = spilledLRs_.begin ();
            lt != spilledLRs_.end (); ++lt)
        {
            if( (*lt)->getDcl()->getAddressed() )
                MUST_BE_TRUE( (*lt)->getDcl()->getAddrTakenSpillFill() != NULL, "Spilled addr taken does not have assigned spill/fill GRF");
        }
    }
#endif

    return success;
}

// Insert spill and fill code for indirect GRF accesses
void SpillManagerGRF::insertAddrTakenSpillAndFillCode( G4_Kernel* kernel, G4_BB* bb,
    INST_LIST::iterator inst_it, G4_Operand* opnd, PointsToAnalysis& pointsToAnalysis, bool spill, unsigned int bbid)
{
    curInst = (*inst_it);
    INST_LIST::iterator next_inst_it = ++inst_it;
    inst_it--;

    // Check whether spill operand points to any spilled range
    for (LR_LIST::const_iterator lr_it = spilledLRs_.begin ();
        lr_it != spilledLRs_.end (); ++lr_it) {
        LiveRange* lr = (*lr_it);
        G4_RegVar* var = NULL;

        if( opnd->isDstRegRegion() && opnd->asDstRegRegion()->getBase()->asRegVar() )
            var = opnd->asDstRegRegion()->getBase()->asRegVar();

        if( opnd->isSrcRegRegion() && opnd->asSrcRegRegion()->getBase()->asRegVar() )
            var = opnd->asSrcRegRegion()->getBase()->asRegVar();

        MUST_BE_TRUE( var != NULL, "Fill operand is neither a source nor dst region");

        if( var &&
            pointsToAnalysis.isPresentInPointsTo( var,
            lr->getVar() ) )
        {
            unsigned int numrows = lr->getDcl()->getNumRows();
            G4_Declare* temp = getOrCreateSpillFillDcl(lr->getDcl(), kernel);

            if (failSafeSpill_ &&
                temp->getRegVar()->getPhyReg() == NULL)
            {
                temp->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegOffset_), 0);
                spillRegOffset_ += numrows;
            }

            if( numrows > 1 || (lr->getDcl()->getNumElems() * lr->getDcl()->getElemSize() == getGRFSize()) )
            {
                if (useScratchMsg_ || useSplitSend())
                {
                    G4_Declare * fillGRFRangeDcl = temp;
                    G4_Declare * mRangeDcl =
                        createAndInitMHeader(
                        (G4_RegVarTransient *)temp->getRegVar()->getBaseRegVar());

                    sendInSpilledRegVarPortions(
                        fillGRFRangeDcl, mRangeDcl, 0,
                        temp->getNumRows(), 0);

                    bb->splice(inst_it, builder_->instList);

                    if (spill)
                    {
                        sendOutSpilledRegVarPortions (
                            temp, mRangeDcl, 0, temp->getNumRows(),
                            0);

                        bb->splice(next_inst_it, builder_->instList);
                    }
                }
                else
                {

                    for( unsigned int i = 0; i < numrows; i++ )
                    {
                        G4_INST* inst;
                        const RegionDesc* rd = kernel->fg.builder->getRegionStride1();
                        unsigned char curExSize = 8;

                        if( (i + 1 ) < numrows )
                            curExSize = 16;

                        G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrcRegRegion(Mod_src_undef, Direct, lr->getVar(), (short)i, 0, rd, Type_F);

                        G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(temp->getRegVar(), (short)i, 0, 1, Type_F);

                        inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                        bb->insertBefore( inst_it, inst );

                        if( spill )
                        {
                            // Also insert spill code
                            G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrcRegRegion(Mod_src_undef, Direct, temp->getRegVar(), (short)i, 0, rd, Type_F);

                            G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(lr->getVar(), (short)i, 0, 1, Type_F);

                            inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                            bb->insertBefore( next_inst_it, inst );
                        }

                        // If 2 rows were processed then increment induction var suitably
                        if(    curExSize == 16 )
                            i++;
                    }
                }

                // Update points to
                // Note: points2 set should be updated after inserting fill code,
                // however, this sets a bit in liveness bit-vector that
                // causes the temp variable to be marked as live-out from
                // that BB. A general fix should treat address taken variables
                // more accurately wrt liveness so they dont escape via
                // unfeasible paths.
                //pointsToAnalysis.addFillToPointsTo( bbid, var, temp->getRegVar() );
            }
            else if( numrows == 1 )
            {
                // Insert spill/fill when there decl uses a single row, that too not completely
                unsigned char curExSize = 16;
                unsigned short numbytes = lr->getDcl()->getNumElems() * lr->getDcl()->getElemSize();

                //temp->setAddressed();
                short off = 0;

                while( numbytes > 0 )
                {
                    G4_INST* inst;
                    G4_Type type = Type_W;

                    if( numbytes >= 16 )
                        curExSize = 8;
                    else if( numbytes >= 8 && numbytes < 16 )
                        curExSize = 4;
                    else if( numbytes >= 4 && numbytes < 8 )
                        curExSize = 2;
                    else if( numbytes >= 2 && numbytes < 4 )
                        curExSize = 1;
                    else if( numbytes == 1 )
                    {
                        // If a region has odd number of bytes, copy last byte in final iteration
                        curExSize = 1;
                        type = Type_UB;
                    }
                    else {
                        MUST_BE_TRUE( false, "Cannot emit SIMD1 for byte");
                        curExSize = 0;
                    }

                    const RegionDesc* rd = kernel->fg.builder->getRegionStride1();

                    G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrcRegRegion(Mod_src_undef, Direct, lr->getVar(), 0, off, rd, type);

                    G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(temp->getRegVar(), 0, off, 1, type);

                    inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                    bb->insertBefore( inst_it, inst );

                    if( spill )
                    {
                        // Also insert spill code
                        G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrcRegRegion(Mod_src_undef, Direct, temp->getRegVar(), 0, off, rd, type);

                        G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(lr->getVar(), 0, off, 1, type);

                        inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                        bb->insertBefore( next_inst_it, inst );
                    }

                    off += curExSize;
                    numbytes -= curExSize*2;
                }

                // Update points to
                //pointsToAnalysis.addFillToPointsTo( bbid, var, temp->getRegVar() );
            }

            if (!spill)
            {
                // Insert pseudo_use node so that liveness keeps the
                // filled variable live through the indirect access.
                // Not required for spill because for spill we will
                // anyway insert a ues of the variable to emit store.
                const RegionDesc* rd = kernel->fg.builder->getRegionScalar();

                G4_SrcRegRegion* pseudoUseSrc = kernel->fg.builder->createSrcRegRegion(Mod_src_undef, Direct, temp->getRegVar(),
                    0, 0, rd, Type_F);

                G4_INST* pseudoUseInst = kernel->fg.builder->createInternalIntrinsicInst(nullptr, Intrinsic::Use, 1, nullptr, pseudoUseSrc, nullptr, nullptr, InstOpt_NoOpt);

                bb->insertBefore(next_inst_it, pseudoUseInst);
            }

        }
    }
}

// Insert any spill/fills for address taken
void SpillManagerGRF::insertAddrTakenSpillFill( G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis )
{
    for( auto bb : kernel->fg)
    {
        for( INST_LIST_ITER inst_it = bb->begin();
            inst_it != bb->end();
            inst_it++ )
        {
            G4_INST* curInst = (*inst_it);

            if (failSafeSpill_)
            {
                spillRegOffset_ = indrSpillRegStart_;
            }

            // Handle indirect destination
            G4_DstRegRegion* dst = curInst->getDst();

            if( dst && dst->getRegAccess() == IndirGRF )
            {
                insertAddrTakenSpillAndFillCode( kernel, bb, inst_it, dst, pointsToAnalysis, true, bb->getId() );
            }

            for( int i = 0; i < G4_MAX_SRCS; i++ )
            {
                G4_Operand* src = curInst->getSrc(i);

                if( src && src->isSrcRegRegion() && src->asSrcRegRegion()->getRegAccess() == IndirGRF )
                {
                    insertAddrTakenSpillAndFillCode( kernel, bb, inst_it, src, pointsToAnalysis, false, bb->getId() );
                }
            }
        }
    }
}

// For address spill/fill code inserted remove from points of each indirect operand
// the original regvar that is spilled.
void SpillManagerGRF::prunePointsTo( G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis )
{
    for( auto bb : kernel->fg)
    {
        for( INST_LIST_ITER inst_it = bb->begin();
            inst_it != bb->end();
            inst_it++ )
        {
            G4_INST* curInst = (*inst_it);
            std::stack<G4_Operand*> st;

            // Handle indirect destination
            G4_DstRegRegion* dst = curInst->getDst();

            if( dst && dst->getRegAccess() == IndirGRF )
            {
                st.push( dst );
            }

            for( int i = 0; i < G4_MAX_SRCS; i++ )
            {
                G4_Operand* src = curInst->getSrc(i);

                if( src && src->isSrcRegRegion() && src->asSrcRegRegion()->getRegAccess() == IndirGRF )
                {
                    st.push( src );
                }
            }

            while (st.size() > 0 )
            {
                G4_Operand* cur = st.top();
                st.pop();

                // Check whether spill operand points to any spilled range
                for (LR_LIST::const_iterator lr_it = spilledLRs_.begin ();
                lr_it != spilledLRs_.end (); ++lr_it) {
                    LiveRange* lr = (*lr_it);
                    G4_RegVar* var = NULL;

                    if( cur->isDstRegRegion() && cur->asDstRegRegion()->getBase()->asRegVar() )
                        var = cur->asDstRegRegion()->getBase()->asRegVar();

                    if( cur->isSrcRegRegion() && cur->asSrcRegRegion()->getBase()->asRegVar() )
                        var = cur->asSrcRegRegion()->getBase()->asRegVar();

                    MUST_BE_TRUE( var != NULL, "Operand is neither a source nor dst region");

                    if( var &&
                        pointsToAnalysis.isPresentInPointsTo( var,
                        lr->getVar() ) )
                    {
                        // Remove this from points to
                        pointsToAnalysis.removeFromPointsTo( var, lr->getVar() );
                    }
                }
            }
        }
    }
}

// Insert spill/fill code for all registers that have not been assigned
// physical registers in the current iteration of the graph coloring
// allocator.
// returns false if spill fails somehow

bool
SpillManagerGRF::insertSpillFillCode (
    G4_Kernel * kernel, PointsToAnalysis& pointsToAnalysis
)
{
    // Set the spill flag of all spilled regvars.
    for (LR_LIST::const_iterator lt = spilledLRs_.begin ();
        lt != spilledLRs_.end (); ++lt) {

        // Ignore request to spill/fill the spill/fill ranges
        // as it does not help the allocator.
        if (shouldSpillRegister ((*lt)->getVar ()) == false)
        {
            bool needsEOTGRF = (*lt)->getEOTSrc() && builder_->hasEOTGRFBinding();
            if (failSafeSpill_ && needsEOTGRF &&
                ((*lt)->getVar()->isRegVarTransient() ||
                 (*lt)->getVar()->isRegVarTmp()))
            {
                (*lt)->getVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegStart_ > (kernel->getNumRegTotal() - 16) ? spillRegStart_ : (kernel->getNumRegTotal() - 16)), 0);
                continue;
            }
            else if (lvInfo_->isAddressSensitive((*lt)->getVar()->getId())) {
                DEBUG_MSG("Register allocation warning: Spilling of variable("
                     << (*lt)->getVar ()->getDeclare ()->getName()
                     << ") whose address is taken!"
                     << endl);
            }
            else {
                DEBUG_MSG("Register allocation warning: Spilling infinite live range ("
                     << (*lt)->getVar ()->getDeclare ()->getName()
                     << ")!"
                     << endl);

            }
            return false;
        }
        else
        {
            (*lt)->getVar ()->getDeclare ()->setSpillFlag ();
        }
    }

    // Handle address taken spills
    bool success = handleAddrTakenSpills( kernel, pointsToAnalysis );

    if( !success )
    {
        DEBUG_MSG( "Enough physical register not available for handling address taken spills" << std::endl );
        return false;
    }

    // Insert spill/fill code for all basic blocks.

    FlowGraph& fg = kernel->fg;

    for (BB_LIST_ITER it = fg.begin(); it != fg.end(); it++)
    {
        bbId_ = (*it)->getId();
        INST_LIST::iterator jt = (*it)->begin();

        while (jt != (*it)->end()) {
            INST_LIST::iterator kt = jt;
            ++kt;
            G4_INST* inst = *jt;

            curInst = inst;

            if (failSafeSpill_)
            {
                spillRegOffset_ = spillRegStart_;
            }

            // Insert spill code, when the target is a spilled register.

            if (inst->getDst())
            {
                G4_RegVar* regVar = nullptr;
                if (inst->getDst()->getBase()->isRegVar())
                {
                    regVar = getRegVar(inst->getDst());
                }

                if (regVar && shouldSpillRegister(regVar))
                {
                    if (getRFType(regVar) == G4_GRF)
                    {
                        if (inst->isPseudoKill())
                        {
                            (*it)->erase(jt);
                            jt = kt;
                            continue;
                        }

                        insertSpillRangeCode(jt, (*it));
                    }
                    else
                    {
                        assert(0);
                    }
                }
            }


            // Insert fill code, when the source is a spilled register.

            for (unsigned i = 0; i < G4_MAX_SRCS; i++)
            {
                if (inst->getSrc(i) &&
                    inst->getSrc(i)->isSrcRegRegion ())
                {
                    auto srcRR = inst->getSrc(i)->asSrcRegRegion();
                    G4_RegVar* regVar = nullptr;
                    if (srcRR->getBase()->isRegVar())
                    {
                        regVar = getRegVar(srcRR);
                    }

                    if (regVar && shouldSpillRegister(regVar))
                    {
                        if (inst->isLifeTimeEnd())
                        {
                            (*it)->erase(jt);
                            break;
                        }
                        bool mayExceedTwoGRF = (inst->isSend() && i == 0) ||
                            (inst->isSplitSend() && i == 1);

                        if (mayExceedTwoGRF)
                        {
                            insertSendFillRangeCode(srcRR, jt, *it);
                        }
                        else if (getRFType(regVar) == G4_GRF)
                            insertFillGRFRangeCode(srcRR, jt, *it);
                        else
                            assert(0);
                    }
                }
            }

            jt = kt;
        }
    }

    bbId_ = UINT_MAX;

    // Calculate the spill memory used in this iteration

    for (auto spill : spilledLRs_)
    {
        unsigned disp = spill->getVar ()->getDisp ();

        if (spill->getVar ()->isSpilled ())
        {
            if (disp != UINT_MAX)
            {
                nextSpillOffset_ = std::max(nextSpillOffset_, disp + getByteSize(spill->getVar()));
            }
        }
    }

    // Emit the instruction with the introduced spill/fill ranges in the
    // current iteration.

#ifndef NDEBUG
#ifdef DEBUG_VERBOSE_ON1
    std::stringstream fname;
    fname << "spill_code_" << iterationNo_++ << "_" << kernel->getName()
          << ends;
    std::ofstream sout;
    sout.open (fname.str ().c_str ());
    kernel->emit_asm (sout, true, 0);
    sout.close ();
#endif
#endif

    return true;
}




uint32_t computeSpillMsgDesc(unsigned int payloadSize, unsigned int offsetInGrfUnits)
{
    // Compute msg descriptor given payload size and offset.
    unsigned headerPresent = 0x80000;
    uint32_t message = headerPresent;
    unsigned msgLength = SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT;
    message |= (msgLength << getSendMsgLengthBitOffset());
    message |= (1 << SCRATCH_MSG_DESC_CATEORY);
    message |= (1 << SCRATCH_MSG_DESC_CHANNEL_MODE);
    message |= (1 << SCRATCH_MSG_DESC_OPERATION_MODE);
    unsigned blocksize_encoding = getScratchBlocksizeEncoding(payloadSize);
    message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
    int offset = offsetInGrfUnits;
    message |= offset;

    return message;
}

uint32_t computeFillMsgDesc(unsigned int payloadSize, unsigned int offsetInGrfUnits)
{
    // Compute msg descriptor given payload size and offset.
    unsigned headerPresent = 0x80000;
    uint32_t message = headerPresent;
    unsigned msgLength = 1;
    message |= (msgLength << getSendMsgLengthBitOffset());
    message |= (1 << SCRATCH_MSG_DESC_CATEORY);
    message |= (0 << SCRATCH_MSG_INVALIDATE_AFTER_READ);
    unsigned blocksize_encoding = getScratchBlocksizeEncoding(payloadSize);
    message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
    message |= offsetInGrfUnits;

    return message;
}

// Returns payload size in units of GRF rows
static unsigned int getPayloadSizeGRF(unsigned int numRows)
{
    if (numRows >= 8)
        return 8u;

    if (numRows >= 4)
        return 4u;

    if (numRows >= 2)
        return 2u;

    return 1u;
}

static unsigned int getPayloadSizeOword(unsigned int numOwords)
{
    if (numOwords >= 8)
        return 8u;

    if (numOwords >= 4)
        return 4u;

    if (numOwords >= 2)
        return 2u;

    return 1u;
}

unsigned int GlobalRA::owordToGRFSize(unsigned int numOwords)
{
    unsigned int GRFSize = numOwords / (2 * (G4_GRF_REG_NBYTES / HWORD_BYTE_SIZE));

    return GRFSize;
}

unsigned int GlobalRA::hwordToGRFSize(unsigned int numHwords)
{
    return owordToGRFSize(numHwords * 2);
}

unsigned int GlobalRA::GRFToHwordSize(unsigned int numGRFs)
{
    return GRFSizeToOwords(numGRFs) / 2;
}

unsigned int GlobalRA::GRFSizeToOwords(unsigned int numGRFs)
{
    return numGRFs * (G4_GRF_REG_NBYTES / OWORD_BYTE_SIZE);
}

static G4_INST* createSpillFillAddr(IR_Builder& builder, G4_Declare* addr, G4_Declare* fp, int offset)
{
    auto imm = builder.createImm(offset, Type_UD);
    auto dst = builder.Create_Dst_Opnd_From_Dcl(addr, 1);
    if (fp)
    {
        auto src0 = builder.Create_Src_Opnd_From_Dcl(fp, builder.getRegionScalar());
        return builder.createBinOp(G4_add, 1, dst, src0, imm, InstOpt_WriteEnable, true);
    }
    else
    {
        // ToDo: make all spill/fill relative to FP (kernel FP = 0)
        return builder.createMov(1, dst, imm, InstOpt_WriteEnable, true);
    }
};



void GlobalRA::expandSpillNonStackcall(uint32_t& numRows, uint32_t& offset, short& rowOffset, G4_SrcRegRegion* header, G4_SrcRegRegion* payload, G4_BB* bb, INST_LIST_ITER& instIt)
{
    auto& builder = kernel.fg.builder;
    auto inst = (*instIt);

    if (offset == G4_SpillIntrinsic::InvalidOffset)
    {
        // oword msg
        auto payloadToUse = builder->createSrcRegRegion(*payload);
        unsigned int execSize = inst->getExecSize(); //(numRows > 1) ? 16 : 8;
        uint32_t spillMsgDesc = SpillManagerGRF::createSpillSendMsgDescOWord(numRows, execSize);
        G4_SendMsgDescriptor* msgDesc = kernel.fg.builder->createSendMsgDesc(spillMsgDesc & 0x000FFFFFu,
            0, 1, SFID::DP_DC, numRows, 0, SendAccess::WRITE_ONLY);
        G4_Imm* msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);
        G4_Imm* extDesc = builder->createImm(msgDesc->getExtendedDesc(), Type_UD);
        auto sendInst = builder->createInternalSplitSendInst(nullptr, G4_sends, execSize,
            inst->getDst(), header, payloadToUse, msgDescImm, inst->getOption(),
            msgDesc, extDesc, inst->getLineNo(), inst->getCISAOff(), inst->getSrcFilename());
        instIt = bb->insertBefore(instIt, sendInst);
    }
    else
    {
        while (numRows >= 1)
        {
            auto payloadToUse = builder->createSrcWithNewRegOff(payload, rowOffset);

            auto region = builder->getRegionStride1();

            uint32_t spillMsgDesc = computeSpillMsgDesc(getPayloadSizeGRF(numRows), offset);
            auto msgDesc = builder->createWriteMsgDesc(SFID::DP_DC, spillMsgDesc, getPayloadSizeGRF(numRows));
            G4_Imm* msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);

            G4_SrcRegRegion* headerOpnd = builder->Create_Src_Opnd_From_Dcl(builder->getBuiltinR0(), region);
            G4_Imm* extDesc = builder->createImm(msgDesc->getExtendedDesc(), Type_UD);
            uint8_t execSize = inst->getExecSize(); //numRows > 1 ? 16 : 8;

            auto sendInst = builder->createInternalSplitSendInst(nullptr, G4_sends, execSize,
                inst->getDst(), headerOpnd, payloadToUse, msgDescImm,
                inst->getOption(), msgDesc, extDesc, inst->getLineNo(), inst->getCISAOff(),
                inst->getSrcFilename());

            instIt = bb->insertBefore(instIt, sendInst);

            numRows -= getPayloadSizeGRF(numRows);
            offset += getPayloadSizeGRF(numRows);
            rowOffset += getPayloadSizeGRF(numRows);
        }
    }
}

void GlobalRA::expandSpillStackcall(uint32_t& numRows, uint32_t& offset, short& rowOffset, G4_SrcRegRegion* payload, G4_BB* bb, INST_LIST_ITER& instIt)
{
    auto& builder = kernel.fg.builder;
    auto inst = (*instIt);

    auto spillIt = instIt;

    // Use oword ld for stackcall. Lower intrinsic to:
    // (W)      add(1 | M0)         r126.2 < 1 > :ud  r125.7 < 0; 1, 0 > : ud  0x0 : ud
    // (W)      sends(8 | M0)         null : ud       r126              payload - src2                0x4A      0x20A02FF
    G4_Operand* src0 = nullptr;
    G4_Imm* src1 = nullptr;
    G4_Declare* scratchRegDcl = builder->kernel.fg.scratchRegDcl;
    G4_Declare* framePtr = inst->asSpillIntrinsic()->getFP();

    // convert hword to oword offset
    auto numRowsOword = numRows * 2;
    auto offsetOword = offset * 2;
    auto rowOffsetOword = rowOffset * 2;

    while (numRowsOword >= 1)
    {
        auto createOwordSpill = [&](unsigned int owordSize, G4_SrcRegRegion* payloadToUse)
        {
            uint8_t execSize = (owordSize > 2) ? 16 : 8;
            G4_DstRegRegion* dst = builder->createNullDst((execSize > 8) ? Type_UW : Type_UD);
            auto sendSrc0 = builder->createSrcRegRegion(Mod_src_undef, Direct, scratchRegDcl->getRegVar(),
                0, 0, builder->rgnpool.createRegion(8, 8, 1), Type_UD);
            unsigned messageLength = owordToGRFSize(owordSize);
            G4_Imm* descImm = createMsgDesc(owordSize, true, true);
            G4_INST* sendInst = nullptr;
            {
                auto msgDesc = builder->createWriteMsgDesc(SFID::DP_DC, (uint32_t)descImm->getInt(), messageLength);
                G4_Imm* msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);
                G4_Imm* extDesc = builder->createImm(msgDesc->getExtendedDesc(), Type_UD);
                sendInst = builder->createInternalSplitSendInst(nullptr, G4_sends, execSize, dst, sendSrc0, payloadToUse,
                    msgDescImm, inst->getOption() | InstOpt_WriteEnable, msgDesc, extDesc, inst->getLineNo(), inst->getCISAOff(),
                    inst->getSrcFilename());
            }
            return sendInst;
        };

        auto payloadSizeInOwords = getPayloadSizeOword(numRowsOword);

        auto payloadToUse = builder->createSrcWithNewRegOff(payload, rowOffsetOword / 2);

        G4_DstRegRegion* dst = builder->createDst(scratchRegDcl->getRegVar(), 0, 2, 1, Type_UD);

        G4_INST* hdrSetInst = nullptr;
        if (inst->asSpillIntrinsic()->isOffsetValid())
        {
            // Skip header if spill module emits its own header
            if (framePtr)
            {
                src0 = builder->createSrcRegRegion(
                    Mod_src_undef, Direct, framePtr->getRegVar(), 0, 0, builder->getRegionScalar(), Type_UD);
                src1 = builder->createImm(offsetOword, Type_UD);
                hdrSetInst = builder->createBinOp(G4_add, 1, dst, src0, src1, InstOpt_WriteEnable, false);
            }
            else
            {
                src0 = builder->createImm(offsetOword, Type_UD);
                hdrSetInst = builder->createMov(1, dst, src0, InstOpt_WriteEnable, false);
            }

            bb->insertBefore(spillIt, hdrSetInst);
        }

        auto spillSends = createOwordSpill(payloadSizeInOwords, payloadToUse);
        std::stringstream comments;
        comments <<  "stack spill: " << payload->getTopDcl()->getName() << " to FP[" << inst->asSpillIntrinsic()->getOffset() << "x32]";
        spillSends->setComments(comments.str());

        bb->insertBefore(spillIt, spillSends);

        if (kernel.getOption(vISA_GenerateDebugInfo))
        {
            kernel.getKernelDebugInfo()->updateExpandedIntrinsic(inst->asSpillIntrinsic(), hdrSetInst);
            kernel.getKernelDebugInfo()->updateExpandedIntrinsic(inst->asSpillIntrinsic(), spillSends);
        }

        numRowsOword -= payloadSizeInOwords;
        offsetOword += payloadSizeInOwords;
        rowOffsetOword += payloadSizeInOwords;
    }
}

// Non-stack call:
//  sends <-- scratch - default, supported
//  send  <-- scratch - disable split send using compiler option, not supported by intrinsic
//  send  <-- non-scratch - used when scratch space usage is very high, supported

//  Stack call :
//  sends <-- non-scratch - default spill, supported
//  send  <-- non-scratch - default fill, supported
void GlobalRA::expandSpillIntrinsic(G4_BB* bb)
{
    // spill (1) null:ud   bitmask:ud   r0:ud   payload:ud
    auto& builder = kernel.fg.builder;
    for (auto instIt = bb->begin(); instIt != bb->end();)
    {
        auto inst = (*instIt);
        if (inst->isSpillIntrinsic())
        {
            bool isOffBP = inst->asSpillIntrinsic()->isOffBP();
            uint32_t numRows = inst->asSpillIntrinsic()->getNumRows();
            uint32_t offset = inst->asSpillIntrinsic()->getOffset() *
                (G4_GRF_REG_NBYTES / HWORD_BYTE_SIZE);
            auto header = inst->getSrc(0)->asSrcRegRegion();
            auto payload = inst->getSrc(1)->asSrcRegRegion();
            auto spillIt = instIt;

            auto rowOffset = payload->getRegOff();
            {
                if (!isOffBP)
                {
                    expandSpillNonStackcall(numRows, offset, rowOffset, header, payload, bb, instIt);
                }
                else
                {
                    expandSpillStackcall(numRows, offset, rowOffset, payload, bb, instIt);
                }
            }
            numGRFSpill++;
            instIt = bb->erase(spillIt);
            continue;
        }
        instIt++;
    }
}

 void GlobalRA::expandFillNonStackcall(uint32_t& numRows, uint32_t& offset, short& rowOffset, G4_SrcRegRegion* header, G4_DstRegRegion* resultRgn, G4_BB* bb, INST_LIST_ITER& instIt)
 {
     auto& builder = kernel.fg.builder;
     auto inst = (*instIt);

     if (offset == G4_FillIntrinsic::InvalidOffset)
     {
         // oword msg
         uint8_t execSize = inst->getExecSize();
         auto numRowsOword = numRows * 2;
         auto fillDst = builder->createDst(resultRgn->getBase(), rowOffset,
             0, resultRgn->getHorzStride(), resultRgn->getType());
         auto sendSrc0 = builder->createSrcRegRegion(Mod_src_undef, Direct, header->getBase(),
             0, 0, builder->rgnpool.createRegion(8, 8, 1), Type_UD);
         G4_Imm* desc = createMsgDesc(numRowsOword, false, false);
         auto sfId = SFID::DP_DC;
         auto msgDesc = builder->createReadMsgDesc(sfId, (uint32_t)desc->getInt());
         G4_Operand* msgDescOpnd = builder->createImm(msgDesc->getDesc(), Type_UD);
         auto sendInst = builder->createInternalSendInst(nullptr, G4_send, execSize, fillDst, sendSrc0, msgDescOpnd,
             InstOpt_WriteEnable, msgDesc, inst->getLineNo(), inst->getCISAOff(), inst->getSrcFilename());
         instIt = bb->insertBefore(instIt, sendInst);
     }
     else
     {
         while (numRows >= 1)
         {
             auto fillDst = builder->createDst(resultRgn->getBase(), rowOffset,
                 0, resultRgn->getHorzStride(), resultRgn->getType());

             auto region = builder->getRegionStride1();
             G4_SrcRegRegion* headerOpnd = builder->Create_Src_Opnd_From_Dcl(builder->getBuiltinR0(), region);

             uint32_t fillMsgDesc = computeFillMsgDesc(getPayloadSizeGRF(numRows), offset);

             G4_SendMsgDescriptor* msgDesc = kernel.fg.builder->createSendMsgDesc(fillMsgDesc,
                 getPayloadSizeGRF(numRows), 1, SFID::DP_DC, 0, 0, SendAccess::READ_ONLY);

             G4_Imm* msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);

             auto sendInst = builder->createInternalSendInst(nullptr,
                 G4_send, 16, fillDst, headerOpnd, msgDescImm, inst->getOption(),
                 msgDesc, inst->getLineNo(), inst->getCISAOff(),
                 inst->getSrcFilename());

             sendInst->setCISAOff(inst->getCISAOff());

             instIt = bb->insertBefore(instIt, sendInst);

             numRows -= getPayloadSizeGRF(numRows);
             offset += getPayloadSizeGRF(numRows);
             rowOffset += getPayloadSizeGRF(numRows);
         }
     }
 }

void GlobalRA::expandFillStackcall(uint32_t& numRows, uint32_t& offset, short& rowOffset, G4_SrcRegRegion* header, G4_DstRegRegion* resultRgn, G4_BB* bb, INST_LIST_ITER& instIt)
{
    auto& builder = kernel.fg.builder;
    auto inst = (*instIt);
    auto fillIt = instIt;

    // Use oword ld for stackcall. Lower intrinsic to:
    // add (1) r126.2<1>:d FP<0;1,0>:d offset
    //  send (16) r[startReg]<1>:uw r126 0xa desc:ud
    G4_Operand* src0 = nullptr;
    G4_Imm* src1 = nullptr;
    G4_Declare* scratchRegDcl = builder->kernel.fg.scratchRegDcl;
    G4_Declare* framePtr = inst->asFillIntrinsic()->getFP();

    // convert hword to oword offset
    auto numRowsOword = numRows * 2;
    auto offsetOword = offset * 2;
    auto rowOffsetOword = rowOffset * 2;

    while (numRowsOword >= 1)
    {
        auto createOwordFill = [&](unsigned int owordSize, G4_DstRegRegion* fillVar)
        {
            uint8_t execSize = (owordSize > 2) ? 16 : 8;
            auto sendSrc0 = builder->createSrcRegRegion(Mod_src_undef, Direct, scratchRegDcl->getRegVar(),
                0, 0, builder->rgnpool.createRegion(8, 8, 1), Type_UD);
            G4_Imm* desc = createMsgDesc(owordSize, false, false);
            G4_INST* sendInst = nullptr;
            auto sfId = SFID::DP_DC;
            {
                auto msgDesc = builder->createReadMsgDesc(SFID::DP_DC, (uint32_t)desc->getInt());
                auto msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);
                sendInst = builder->createInternalSendInst(nullptr, G4_send, execSize, fillVar, sendSrc0, msgDescImm,
                    InstOpt_WriteEnable, msgDesc, inst->getLineNo(), inst->getCISAOff(), inst->getSrcFilename());
            }
            return sendInst;
        };

        auto respSizeInOwords = getPayloadSizeOword(numRowsOword);
        auto fillDst = builder->createDst(resultRgn->getBase(), rowOffsetOword / 2,
            0, resultRgn->getHorzStride(), resultRgn->getType());

        G4_DstRegRegion* dst = builder->createDst(scratchRegDcl->getRegVar(), 0, 2, 1, Type_UD);

        G4_INST* hdrSetInst = nullptr;
        if (inst->asFillIntrinsic()->isOffsetValid())
        {
            // Skip header if spill module emits its own header
            if (framePtr)
            {
                src0 = builder->createSrcRegRegion(
                    Mod_src_undef, Direct, framePtr->getRegVar(), 0, 0, builder->getRegionScalar(), Type_UD);
                src1 = builder->createImm(offsetOword, Type_UD);
                hdrSetInst = builder->createBinOp(G4_add, 1, dst, src0, src1, InstOpt_WriteEnable, false);
            }
            else
            {
                src0 = builder->createImm(offsetOword, Type_UD);
                hdrSetInst = builder->createMov(1, dst, src0, InstOpt_WriteEnable, false);
            }

            bb->insertBefore(fillIt, hdrSetInst);
        }

        auto fillSends = createOwordFill(respSizeInOwords, fillDst);

        std::stringstream comments;
        comments << "stack fill: " << resultRgn->getTopDcl()->getName() << " from FP[" << inst->asFillIntrinsic()->getOffset() << "x32]";
        fillSends->setComments(comments.str());

        bb->insertBefore(fillIt, fillSends);

        if (kernel.getOption(vISA_GenerateDebugInfo))
        {
            kernel.getKernelDebugInfo()->updateExpandedIntrinsic(inst->asFillIntrinsic(), hdrSetInst);
            kernel.getKernelDebugInfo()->updateExpandedIntrinsic(inst->asFillIntrinsic(), fillSends);
        }

        numRowsOword -= respSizeInOwords;
        offsetOword += respSizeInOwords;
        rowOffsetOword += respSizeInOwords;
    }
}

void GlobalRA::expandFillIntrinsic(G4_BB* bb)
{
    // fill (1) fill_var:ud     bitmask:ud     offset:ud
    auto& builder = kernel.fg.builder;
    for (auto instIt = bb->begin(); instIt != bb->end();)
    {
        auto inst = (*instIt);
        if (inst->isFillIntrinsic())
        {
            bool isOffBP = inst->asFillIntrinsic()->isOffBP();
            uint32_t numRows = inst->asFillIntrinsic()->getNumRows();
            uint32_t offset = inst->asFillIntrinsic()->getOffset() *
                (G4_GRF_REG_NBYTES / HWORD_BYTE_SIZE);
            auto header = inst->getSrc(0)->asSrcRegRegion();
            auto resultRgn = inst->getDst();
            auto fillIt = instIt;

            auto rowOffset = resultRgn->getRegOff();
            {
                if (!isOffBP)
                {
                    expandFillNonStackcall(numRows, offset, rowOffset, header, resultRgn, bb, instIt);
                }
                else
                {
                    expandFillStackcall(numRows, offset, rowOffset, header, resultRgn, bb, instIt);
                }
            }
            numGRFFill++;
            instIt = bb->erase(fillIt);
            continue;
        }
        instIt++;
    }
}


void GlobalRA::expandSpillFillIntrinsics()
{
    for (auto bb : kernel.fg)
    {
        expandSpillIntrinsic(bb);
        expandFillIntrinsic(bb);
    }
    kernel.fg.builder->getcompilerStats().SetI64(CompilerStats::numGRFSpillStr(), numGRFSpill, kernel.getSimdSize());
    kernel.fg.builder->getcompilerStats().SetI64(CompilerStats::numGRFFillStr(), numGRFFill, kernel.getSimdSize());

}
