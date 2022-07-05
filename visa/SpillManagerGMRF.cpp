/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SpillManagerGMRF.h"
#include "G4_IR.hpp"
#include "Mem_Manager.h"
#include "FlowGraph.h"
#include "GraphColor.h"
#include "BuildIR.h"
#include "DebugInfo.h"

#include <math.h>
#include <sstream>
#include <fstream>
#include <unordered_set>

using namespace vISA;

// Configurations

#define ADDRESS_SENSITIVE_SPILLS_IMPLEMENTED
#define SCRATCH_SPACE_ADDRESS_UNIT 5

//#define DISABLE_SPILL_MEMORY_COMPRESSION
//#define VERIFY_SPILL_ASSIGNMENTS

// Constant declarations

static const unsigned DWORD_BYTE_SIZE                        = 4;
static const unsigned OWORD_BYTE_SIZE                        = 16;
static const unsigned HWORD_BYTE_SIZE                        = 32;
static const unsigned PAYLOAD_INPUT_REG_OFFSET               = 0;
static const unsigned PAYLOAD_INPUT_SUBREG_OFFSET            = 0;
static const unsigned OWORD_PAYLOAD_SPOFFSET_REG_OFFSET      = 0;
static const unsigned OWORD_PAYLOAD_SPOFFSET_SUBREG_OFFSET   = 2;
static const unsigned DWORD_PAYLOAD_SPOFFSET_REG_OFFSET      = 1;
static const unsigned DWORD_PAYLOAD_SPOFFSET_SUBREG_OFFSET   = 0;
static const unsigned OWORD_PAYLOAD_WRITE_REG_OFFSET         = 1;
static const unsigned OWORD_PAYLOAD_WRITE_SUBREG_OFFSET      = 0;
// dword scatter is always in SIMD8 mode
static const unsigned DWORD_PAYLOAD_WRITE_REG_OFFSET         = 2;
static const unsigned DWORD_PAYLOAD_WRITE_SUBREG_OFFSET      = 0;
static const unsigned OWORD_PAYLOAD_HEADER_MIN_HEIGHT        = 1;
static const unsigned DWORD_PAYLOAD_HEADER_MIN_HEIGHT        = 2;
static const unsigned OWORD_PAYLOAD_HEADER_MAX_HEIGHT        = 1;
static const unsigned DWORD_PAYLOAD_HEADER_MAX_HEIGHT        = 3;
static const unsigned DEF_HORIZ_STRIDE                       = 1;
static const unsigned REG_ORIGIN                             = 0;
static const unsigned SUBREG_ORIGIN                          = 0;

static const unsigned SEND_GT_READ_TYPE_BIT_OFFSET           = 13;
static const unsigned SEND_GT_WRITE_TYPE_BIT_OFFSET          = 13;
static const unsigned SEND_GT_DESC_DATA_SIZE_BIT_OFFSET      = 8;
static const unsigned SEND_GT_OW_READ_TYPE                   = 0;
static const unsigned SEND_GT_OW_WRITE_TYPE                  = 8;
static const unsigned SEND_GT_SC_READ_TYPE                   = 6;
static const unsigned SEND_GT_SC_WRITE_TYPE                  = 11;
static const unsigned SEND_GT_DP_RD_EX_DESC_IMM              = 5;
static const unsigned SEND_GT_DP_SC_RD_EX_DESC_IMM           = 4;    //scatter reads go to sampler cache
static const unsigned SEND_GT_DP_WR_EX_DESC_IMM              = 5;

static const unsigned SEND_IVB_MSG_TYPE_BIT_OFFSET         = 14;
static const unsigned SEND_IVB_OW_READ_TYPE                = 0;
static const unsigned SEND_IVB_OW_WRITE_TYPE               = 8;
static const unsigned SEND_IVB_SC_READ_TYPE                = 3;
static const unsigned SEND_IVB_SC_WRITE_TYPE               = 11;
static const unsigned SEND_IVB_DP_RD_EX_DESC_IMM           = 10; //data cache
static const unsigned SEND_IVB_DP_WR_EX_DESC_IMM           = 10; //data cache

// Scratch msg
static const unsigned SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT     = 1;
static const unsigned SCRATCH_MSG_DESC_CATEORY             = 18;
static const unsigned SCRATCH_MSG_DESC_OPERATION_MODE      = 17;
static const unsigned SCRATCH_MSG_DESC_CHANNEL_MODE        = 16;
static const unsigned SCRATCH_MSG_INVALIDATE_AFTER_READ    = 15;
static const unsigned SCRATCH_MSG_DESC_BLOCK_SIZE          = 12;


#define LIMIT_SEND_EXEC_SIZE(EXEC_SIZE)  (((EXEC_SIZE) > 16) ? 16 : (EXEC_SIZE))
#define SPILL_PAYLOAD_HEIGHT_LIMIT 4

void splice(G4_BB* bb, INST_LIST_ITER iter, INST_LIST& instList, unsigned int CISAOff)
{
    // Update CISA offset of all instructions in instList before splicing
    // operation.
    for (auto inst : instList)
    {
        inst->setCISAOff(CISAOff);
    }

    bb->splice(iter, instList);
}

// spill/fill temps are always GRF-aligned, and are also even/odd aligned
// following the original declare's alignment
static void setNewDclAlignment(GlobalRA& gra, G4_Declare* newDcl, bool evenAlign)
{
    newDcl->setSubRegAlign(gra.builder.getGRFAlign());
    if (evenAlign)
    {
        newDcl->setEvenAlign();
    }

    gra.setSubRegAlign(newDcl, gra.builder.getGRFAlign());
    gra.setEvenAligned(newDcl, evenAlign);
}

SpillManagerGRF::SpillManagerGRF(
    GlobalRA& g,
    unsigned spillAreaOffset,
    unsigned varIdCount,
    const LivenessAnalysis* lvInfo,
    LiveRange** lrInfo,
    const Interference* intf,
    const LR_LIST* spilledLRs,
    unsigned iterationNo,
    bool failSafeSpill,
    unsigned spillRegSize,
    unsigned indrSpillRegSize,
    bool enableSpillSpaceCompression,
    bool useScratchMsg,
    bool avoidDstSrcOverlap)
    : gra(g)
    , builder_(g.kernel.fg.builder)
    , varIdCount_(varIdCount)
    , latestImplicitVarIdCount_(0)
    , lvInfo_(lvInfo)
    , lrInfo_(lrInfo)
    , spilledLRs_(spilledLRs)
    , nextSpillOffset_(spillAreaOffset)
    , iterationNo_(iterationNo)
    , doSpillSpaceCompression(enableSpillSpaceCompression)
    , failSafeSpill_(failSafeSpill)
    , spillIntf_(intf)
    , mem_(1024)
    , useScratchMsg_(useScratchMsg)
    , avoidDstSrcOverlap_(avoidDstSrcOverlap)
    , refs(g.kernel)
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
    addrSpillFillRangeCount_ = (unsigned*)allocMem(size);
    memset(addrSpillFillRangeCount_, 0, size);
    spillAreaOffset_ = spillAreaOffset;
    builder_->instList.clear();
    spillRegStart_ = g.kernel.getNumRegTotal();
    indrSpillRegStart_ = spillRegStart_;
    spillRegOffset_ = spillRegStart_;
    if (failSafeSpill) {
        bool isStackCall = builder_->usesStack();
        unsigned int stackCallRegSize = isStackCall ? builder_->kernel.numReservedABIGRF() : 0;
        indrSpillRegStart_ -= (stackCallRegSize + indrSpillRegSize);
        spillRegStart_ = indrSpillRegStart_ - spillRegSize;
    }
    curInst = nullptr;
    globalScratchOffset = gra.kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
    spilledLSLRs_ = nullptr;
    if (builder_->hasScratchSurface())
    {
        builder_->initScratchSurfaceOffset();
        auto entryBB = builder_->kernel.fg.getEntryBB();
        auto iter = std::find_if(entryBB->begin(), entryBB->end(), [](G4_INST* inst) { return !inst->isLabel(); });
        splice(entryBB, iter, builder_->instList, UNMAPPABLE_VISA_INDEX);
    }

    // LSC messages are used when:
    // a. Stack call is used on PVC+,
    // b. Spill size exceeds what can be represented using hword msg on PVC+
    useLSCMsg = gra.useLscForSpillFill;
    useLscNonstackCall = gra.useLscForNonStackCallSpillFill;
}

SpillManagerGRF::SpillManagerGRF(
    GlobalRA& g,
    unsigned spillAreaOffset,
    unsigned varIdCount,
    const LivenessAnalysis* lvInfo,
    LSLR_LIST* spilledLSLRs,
    bool enableSpillSpaceCompression,
    bool useScratchMsg,
    bool avoidDstSrcOverlap)
    : gra(g)
    , builder_(g.kernel.fg.builder)
    , varIdCount_(varIdCount)
    , latestImplicitVarIdCount_(0)
    , lvInfo_(lvInfo)
    , spilledLSLRs_(spilledLSLRs)
    , nextSpillOffset_(spillAreaOffset)
    , doSpillSpaceCompression(enableSpillSpaceCompression)
    , failSafeSpill_(false)
    , mem_(1024)
    , useScratchMsg_(useScratchMsg)
    , avoidDstSrcOverlap_(avoidDstSrcOverlap)
    , refs(g.kernel)
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
    addrSpillFillRangeCount_ = (unsigned*)allocMem(size);
    memset(addrSpillFillRangeCount_, 0, size);
    spillAreaOffset_ = spillAreaOffset;
    builder_->instList.clear();
    curInst = NULL;
    globalScratchOffset = gra.kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);

    if (builder_->hasScratchSurface())
    {
        builder_->initScratchSurfaceOffset();
        auto entryBB = builder_->kernel.fg.getEntryBB();
        auto iter = std::find_if(entryBB->begin(), entryBB->end(), [](G4_INST* inst) { return !inst->isLabel(); });
        splice(entryBB, iter, builder_->instList, UNMAPPABLE_VISA_INDEX);
    }
    // LSC messages are used when:
    // a. Stack call is used on PVC+,
    // b. Spill size exceeds what can be represented using hword msg on PVC+
    useLSCMsg = gra.useLscForSpillFill;
    useLscNonstackCall = gra.useLscForNonStackCallSpillFill;
}

// Get the base regvar for the source or destination region.
template <class REGION_TYPE>
G4_RegVar *SpillManagerGRF::getRegVar(REGION_TYPE * region) const
{
    G4_RegVar * spilledRegVar = (G4_RegVar *) region->getBase();
    return spilledRegVar;
}

// Get the representative regvar that will be assigned a unique spill
// disp and not a relative spill disp.
G4_RegVar *SpillManagerGRF::getReprRegVar(G4_RegVar * regVar) const
{
    G4_RegVar * absBase = regVar->getAbsBaseRegVar();
    if (absBase->isAliased())
        return getReprRegVar(absBase->getDeclare()->getAliasDeclare()->getRegVar());
    else
        return absBase;
}

// Obtain the register file type of the regvar.
G4_RegFileKind SpillManagerGRF::getRFType(G4_RegVar * regvar) const
{
    return regvar->getDeclare()->getRegFile();
}

// Obtain the register file type of the region.
template <class REGION_TYPE>
G4_RegFileKind SpillManagerGRF::getRFType(REGION_TYPE * region) const
{
    if (region->getBase()->isRegVar())
        return getRFType(region->getBase()->asRegVar());
    else if (region->getBase()->isGreg())
        return G4_GRF;
    else
        return G4_ADDRESS;
}

// Get the byte offset of the origin of the source or destination region.
// The row offset component is calculated based on the the parameters of
// the corresponding declare directive, while the column offset is calculated
// based on the region parameters.
template <class REGION_TYPE>
unsigned SpillManagerGRF::getRegionOriginOffset(REGION_TYPE * region) const
{
    unsigned rowOffset = builder_->numEltPerGRF<Type_UB>() * region->getRegOff();
    unsigned columnOffset = region->getSubRegOff() * region->getElemSize();
    return rowOffset + columnOffset;
}

// Get a GRF aligned mask
unsigned SpillManagerGRF::grfMask() const
{
    unsigned mask = 0;
    mask = (mask - 1);
    MUST_BE_TRUE(std::log2(builder_->numEltPerGRF<Type_UB>()) == (float)((int)(std::log2(builder_->numEltPerGRF<Type_UB>()))), "expected integral value");
    unsigned int bits = (unsigned int)std::log2(builder_->numEltPerGRF<Type_UB>());
    mask = mask << bits;
    return mask;
}

// Get an hex word mask with the lower 5 bits zeroed.
unsigned SpillManagerGRF::hwordMask() const
{
    unsigned mask = 0;
    mask = (mask - 1);
    mask = mask << 5;
    return mask;
}

// Get an octal word mask with the lower 4 bits zeroed.
unsigned SpillManagerGRF::owordMask() const
{
    unsigned mask = 0;
    mask = (mask - 1);
    mask = mask << 4;
    return mask;
}

// Get an dword word mask with the lower 2 bits zeroed.
unsigned SpillManagerGRF::dwordMask() const
{
    unsigned mask = 0;
    mask = (mask - 1);
    mask = mask << 2;
    return mask;
}

// Test of the offset is oword aligned.
bool SpillManagerGRF::owordAligned(unsigned offset) const
{
    return (offset & owordMask()) == offset;
}

// Test of the offset is oword aligned.
bool SpillManagerGRF::dwordAligned(unsigned offset) const
{
    return (offset & dwordMask ()) == offset;
}

// Get the ceil of the ratio.
unsigned SpillManagerGRF::cdiv(unsigned dvd, unsigned dvr)
{
    return (dvd / dvr) + ((dvd % dvr) ? 1 : 0);
}

// Get the live range corresponding to id.
bool SpillManagerGRF::shouldSpillRegister(G4_RegVar * regVar) const
{
    if (getRFType(regVar) == G4_ADDRESS)
    {
        return false;
    }
    G4_RegVar * actualRegVar =
        (regVar->getDeclare()->getAliasDeclare()) ?
            regVar->getDeclare()->getAliasDeclare()->getRegVar() :
            regVar;
    if (actualRegVar->getId() == UNDEFINED_VAL)
        return false;
    else if (regVar->isRegVarTransient() || regVar->isRegVarTmp())
        return false;
#ifndef ADDRESS_SENSITIVE_SPILLS_IMPLEMENTED
    else if (lvInfo_->isAddressSensitive (regVar->getId()))
        return false;
#endif
    else if (builder_->kernel.fg.isPseudoVCADcl(actualRegVar->getDeclare()) ||
        builder_->kernel.fg.isPseudoVCEDcl(actualRegVar->getDeclare()))
        return false;
    else
        return lrInfo_[actualRegVar->getId()]->getPhyReg() == NULL;
}

// Get the regvar with the id.
G4_RegVar *SpillManagerGRF::getRegVar(unsigned id) const
{
    return (lvInfo_->vars)[id];
}

// Get the byte size of the live range.
unsigned SpillManagerGRF::getByteSize(G4_RegVar * regVar) const
{
    unsigned normalizedRowSize =
        (regVar->getDeclare()->getNumRows() > 1) ?
            builder_->numEltPerGRF<Type_UB>() :
            regVar->getDeclare()->getNumElems() *
                regVar->getDeclare()->getElemSize();
    return normalizedRowSize * regVar->getDeclare()->getNumRows();
}

// Check if the lifetime of the spill/fill memory of live range i interferes
// with the lifetime of the spill/fill memory of live range j
bool SpillManagerGRF::spillMemLifetimeInterfere(
    unsigned i, unsigned j) const
{
    G4_RegVar * ireg = getRegVar(i);
    G4_RegVar * jreg = getRegVar(j);
    G4_RegVar * irep = getReprRegVar(ireg);
    G4_RegVar * jrep = getReprRegVar(jreg);
    G4_RegVar * inont = ireg->getNonTransientBaseRegVar();
    G4_RegVar * jnont = jreg->getNonTransientBaseRegVar();

    if (ireg->isRegVarTmp()) {
        return
            ireg->getBaseRegVar() == jrep ||
            spillMemLifetimeInterfere(ireg->getBaseRegVar()->getId(), j);
    }
    else if (jreg->isRegVarTmp()) {
        return
            jreg->getBaseRegVar() == irep ||
            spillMemLifetimeInterfere (jreg->getBaseRegVar()->getId(), i);
    }

    else if (inont->isRegVarTmp()) {
        return
            inont->getBaseRegVar() == jrep ||
            spillMemLifetimeInterfere(inont->getBaseRegVar()->getId(), j);

    }

    else if (jnont->isRegVarTmp()) {
        return
            jnont->getBaseRegVar() == irep ||
            spillMemLifetimeInterfere (jnont->getBaseRegVar()->getId(), i);
    }

    else {
        if (spillIntf_->interfereBetween(irep->getId(), jrep->getId()))
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

void SpillManagerGRF::getOverlappingIntervals(G4_Declare* dcl, std::vector<G4_Declare*>& intervals) const
{
    auto& allIntervals = spillingIntervals;
    auto dclStart = gra.getStartInterval(dcl);
    auto dclEnd = gra.getEndInterval(dcl);
    auto dclMask = gra.getAugmentationMask(dcl);

    // non-default variables have all interferences marked in
    // interferences graph
    if (dclMask == AugmentationMasks::NonDefault)
        return;

    for (auto otherDcl : allIntervals)
    {
        if (otherDcl == dcl)
            continue;

        auto start = gra.getStartInterval(otherDcl);
        auto end = gra.getEndInterval(otherDcl);

        // First overlap not yet seen
        if (end->getLexicalId() < dclStart->getLexicalId())
            continue;

        // allIntervals are sorted on start. All overlapping
        // intervals are guaranteed to be visited when
        // current interval start is beyond end of dcl.
        if (start->getLexicalId() > dclEnd->getLexicalId())
            return;

        // end lexical id >= dclStart lexical id
        // add interval to list only if it is has same aug mask
        // because overlapping intervals with different masks are already
        // marked in intf graph.
        if (start->getLexicalId() <= dclEnd->getLexicalId() &&
            dclMask == gra.getAugmentationMask(otherDcl))
            intervals.push_back(otherDcl);
    }
}

// Calculate the spill memory displacement for the regvar.
unsigned SpillManagerGRF::calculateSpillDisp(G4_RegVar *   regVar) const
{
    assert(regVar->getDisp () == UINT_MAX);

    // Locate the blocked locations calculated from the interfering
    // spilled live ranges and put them into a list in ascending order.

    using LocList = std::list<G4_RegVar*>;
    LocList locList;
    unsigned lrId =
        (regVar->getId() >= varIdCount_)?
        regVar->getBaseRegVar()->getId(): regVar->getId();
    assert(lrId < varIdCount_);

    const std::vector<unsigned int>& intfs = spillIntf_->getSparseIntfForVar(lrId);
    for (auto edge : intfs)
    {
        auto lrEdge = getRegVar(edge);
        if (lrEdge->isRegVarTransient())
            continue;
        if (lrEdge->getDisp() == UINT_MAX)
            continue;
        locList.push_back(lrEdge);
    }

    std::vector<G4_Declare*> overlappingDcls;
    getOverlappingIntervals(regVar->getDeclare()->getRootDeclare(), overlappingDcls);
    for (auto overlap : overlappingDcls)
    {
        auto lrEdge = overlap->getRegVar();
        if (lrEdge->getDisp() == UINT_MAX)
            continue;
        locList.push_back(lrEdge);
    }

    locList.sort([](G4_RegVar* v1, G4_RegVar* v2) { return v1->getDisp() < v2->getDisp(); });
    locList.unique();

    // Find a spill slot for lRange within the locList.
    // we always start searching from nextSpillOffset_ to facilitate intra-iteration reuse.
    // cross iteration reuse is not done in interest of compile time.
    unsigned regVarLocDisp = ROUND(nextSpillOffset_, builder_->numEltPerGRF<Type_UB>());
    unsigned regVarSize = getByteSize (regVar);

    for (G4_RegVar *curLoc : locList) {
        unsigned curLocDisp = curLoc->getDisp ();
        if (regVarLocDisp < curLocDisp &&
            regVarLocDisp + regVarSize <= curLocDisp)
            break;
        unsigned curLocEnd = curLocDisp + getByteSize(curLoc);
        {
            if (curLocEnd % builder_->numEltPerGRF<Type_UB>() != 0)
                curLocEnd = ROUND(curLocEnd, builder_->numEltPerGRF<Type_UB>());
        }

        regVarLocDisp = (regVarLocDisp > curLocEnd)? regVarLocDisp: curLocEnd;
    }

    return regVarLocDisp;
}

unsigned SpillManagerGRF::calculateSpillDispForLS(G4_RegVar* regVar) const
{
    assert(regVar->getDisp() == UINT_MAX);

    // Locate the blocked locations calculated from the interfering
    // spilled live ranges and put them into a list in ascending order.

    typedef std::deque < G4_RegVar* > LocList;
    LocList locList;
    unsigned lrId =
        (regVar->getId() >= varIdCount_) ?
            regVar->getBaseRegVar()->getId() : regVar->getId();
    assert(lrId < varIdCount_);

    for (auto lr : activeLR_)
    {
        G4_RegVar* intfRegVar = lr->getTopDcl()->getRegVar();
        if (intfRegVar->isRegVarTransient()) continue;

        unsigned iDisp = intfRegVar->getDisp();
        if (iDisp == UINT_MAX) continue;

        LocList::iterator loc;
        for (loc = locList.begin();
            loc != locList.end() && (*loc)->getDisp() < iDisp;
            ++loc);
        if (loc != locList.end())
            locList.insert(loc, intfRegVar);
        else
            locList.push_back(intfRegVar);
    }

    // Find a spill slot for lRange within the locList.
    // we always start searching from nextSpillOffset_ to facilitate intra-iteration reuse.
    // cross iteration reuse is not done in interest of compile time.
    unsigned regVarLocDisp = ROUND(nextSpillOffset_, builder_->numEltPerGRF<Type_UB>());
    unsigned regVarSize = getByteSize(regVar);

    for (LocList::iterator curLoc = locList.begin(), end = locList.end(); curLoc != end;
        ++curLoc) {
        unsigned curLocDisp = (*curLoc)->getDisp();
        if (regVarLocDisp < curLocDisp &&
            regVarLocDisp + regVarSize <= curLocDisp)
            break;
        unsigned curLocEnd = curLocDisp + getByteSize(*curLoc);
        {
            if (curLocEnd % builder_->numEltPerGRF<Type_UB>() != 0)
                curLocEnd = ROUND(curLocEnd, builder_->numEltPerGRF<Type_UB>());
        }

        regVarLocDisp = (regVarLocDisp > curLocEnd) ? regVarLocDisp : curLocEnd;
    }

    return regVarLocDisp;
}

// Get the spill/fill displacement of the segment containing the region.
// A segment is the smallest dword or oword aligned portion of memory
// containing the destination or source operand that can be read or saved.
template <class REGION_TYPE>
unsigned SpillManagerGRF::getSegmentDisp (
    REGION_TYPE * region,
    G4_ExecSize  execSize
)
{
    assert(region->getElemSize () && execSize);
    if (isUnalignedRegion(region, execSize))
        return getEncAlignedSegmentDisp(region, execSize);
    else
        return getRegionDisp(region);
}

// Get the spill/fill displacement of the regvar.
unsigned SpillManagerGRF::getDisp(G4_RegVar * regVar)
{
    // Already calculated spill memory disp

    if (regVar->getDisp() != UINT_MAX)
    {
        return regVar->getDisp();
    }
    else if (regVar->isAliased()) {
        // If it is an aliased regvar then calculate the disp for the
        // actual regvar and then calculate the disp of the aliased regvar
        // based on it.
        G4_Declare * regVarDcl = regVar->getDeclare();
        return getDisp(regVarDcl->getAliasDeclare()->getRegVar()) +
            regVarDcl->getAliasOffset();
    }
    else if (gra.splitResults.find(regVar->getDeclare()->getRootDeclare()) !=
        gra.splitResults.end())
    {
        // this variable is result of variable splitting optimization.
        // original variable is guaranteed to have spilled. if split
        // variable also spills then reuse original variable's spill
        // location.
        auto it = gra.splitResults.find(regVar->getDeclare()->getRootDeclare());
        auto disp = getDisp((*it).second.origDcl->getRegVar());
        regVar->setDisp(disp);
    }
    else if (regVar->isRegVarTransient() &&
        getDisp(regVar->getBaseRegVar()) != UINT_MAX)
    {
        // If its base regvar has been assigned a disp, then the spill memory
        // has already been allocated for it, simply calculate the disp based
        // on the enclosing segment disp.
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
                    tRegVar->getSrcRepRegion(),
                    tRegVar->getExecSize()) <= getByteSize(tRegVar));
            itsDisp =
                getSegmentDisp(tRegVar->getSrcRepRegion(), tRegVar->getExecSize());
        }
        else {
            MUST_BE_TRUE(false, "Incorrect spill/fill ranges.");
            itsDisp = 0;
        }

        regVar->setDisp(itsDisp);
    }
    else {
        // Allocate the spill and evaluate its disp
        if (doSpillSpaceCompression)
        {
            assert(regVar->isRegVarTransient() == false);
            if (spilledLSLRs_ != nullptr)
            {
                regVar->setDisp(calculateSpillDispForLS(regVar));
            }
            else
            {
                regVar->setDisp(calculateSpillDisp(regVar));
            }
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

            if ((spillAreaOffset_) % builder_->numEltPerGRF<Type_UB>() != 0)
            {
                (spillAreaOffset_) = ROUND(spillAreaOffset_, builder_->numEltPerGRF<Type_UB>());
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
unsigned SpillManagerGRF::getRegionDisp(REGION_TYPE * region)
{
    return getDisp (getRegVar(region)) + getRegionOriginOffset(region);
}

// Get the type of send message to use to spill/fill the region.
// The type can be either on oword read/write or a scatter read/write.
// If the segment corresponding to the region is dword sized then a
// dword read/write is used else an oword read/write is used.
template <class REGION_TYPE>
unsigned SpillManagerGRF::getMsgType(
    REGION_TYPE * region, G4_ExecSize execSize)
{
    unsigned regionDisp = getRegionDisp(region);
    unsigned regionByteSize = getRegionByteSize(region, execSize);
    if (owordAligned (regionDisp) && owordAligned (regionByteSize))
        return owordMask();
    else
        return getEncAlignedSegmentMsgType(region, execSize);
}

// Determine if the region is unaligned w.r.t spill/fill memory read/writes.
// If the exact region cannot be read/written from spill/fill memory using
// one send instruction, then it is unaligned.
template <class REGION_TYPE>
bool SpillManagerGRF::isUnalignedRegion(
    REGION_TYPE * region, G4_ExecSize execSize)
{
    unsigned regionDisp = getRegionDisp(region);
    unsigned regionByteSize = getRegionByteSize(region, execSize);

    bool needs32ByteAlign = useScratchMsg_;
    needs32ByteAlign |= useLSCMsg;

    auto bytePerGRF = builder_->numEltPerGRF<Type_UB>();
    if (needs32ByteAlign)
    {
        if (regionDisp % bytePerGRF == 0 && regionByteSize % bytePerGRF == 0)
        {
            return
                regionByteSize / bytePerGRF != 1 &&
                regionByteSize / bytePerGRF != 2 &&
                regionByteSize / bytePerGRF != 4;
        }
        else
            return true;
    }
    else
    {
        if (owordAligned(regionDisp) && owordAligned(regionByteSize))
        {
            //  Current intrinsic spill/fill cannot handle partial region spill.
            //  If it's the partial region of a large size variable, such as V91 in following instructions, the preload is needed.
            //  mov (16) V91(6,0)<1>:ub  %retval_ub(0,0)<1;1,0>:ub {H1, Align1}
            //  mov (16) V91(6,16)<1>:ub %retval_ub(0,16)<1;1,0>:ub {H1, Align1}
            G4_RegVar* var = getRegVar(region);
            if ((var->getDeclare()->getByteSize() > bytePerGRF) &&
                (regionByteSize < bytePerGRF || regionDisp % bytePerGRF))
            {
                return true;
            }
            return
                regionByteSize / OWORD_BYTE_SIZE != 1 &&
                regionByteSize / OWORD_BYTE_SIZE != 2 &&
                regionByteSize / OWORD_BYTE_SIZE != 4;
        }
        else
            return true;
    }
}

// Calculate the smallest aligned segment encompassing the region.
template <class REGION_TYPE>
void SpillManagerGRF::calculateEncAlignedSegment(
    REGION_TYPE * region,
    G4_ExecSize  execSize,
    unsigned &    start,
    unsigned &    end,
    unsigned &    type)
{
    unsigned regionDisp = getRegionDisp(region);
    unsigned regionByteSize = getRegionByteSize(region, execSize);

    if (needGRFAlignedOffset())
    {
        unsigned hwordLB = regionDisp & grfMask();
        unsigned hwordRB = hwordLB + builder_->numEltPerGRF<Type_UB>();
        unsigned blockSize = builder_->numEltPerGRF<Type_UB>();

        while (regionDisp + regionByteSize > hwordRB) {
            hwordRB += blockSize;
        }

        assert((hwordRB - hwordLB) / builder_->numEltPerGRF<Type_UB>() <= 4);
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

        assert((owordRB - owordLB) / builder_->numEltPerGRF<Type_UB>() <= 4);
        start = owordLB;
        end = owordRB;
        type = owordMask();
    }
}

// Get the byte size of the aligned segment for the region.

template <class REGION_TYPE>
unsigned
SpillManagerGRF::getEncAlignedSegmentByteSize(
    REGION_TYPE * region,
    G4_ExecSize  execSize
)
{
    unsigned start, end, type;
    calculateEncAlignedSegment(region, execSize, start, end, type);
    return end - start;
}

// Get the start offset of the aligned segment for the region.
template <class REGION_TYPE>
unsigned
SpillManagerGRF::getEncAlignedSegmentDisp(
    REGION_TYPE * region,
    G4_ExecSize  execSize
)
{
    unsigned start, end, type;
    calculateEncAlignedSegment(region, execSize, start, end, type);
    return start;
}

// Get the type of message to be used to read/write the enclosing aligned
// segment for the region.
template <class REGION_TYPE>
unsigned SpillManagerGRF::getEncAlignedSegmentMsgType(
    REGION_TYPE * region,
    G4_ExecSize   execSize
)
{
    unsigned start, end, type;
    calculateEncAlignedSegment(region, execSize, start, end, type);
    return type;
}

// Get the byte size of the segment for the region.
template <class REGION_TYPE>
unsigned SpillManagerGRF::getSegmentByteSize(
    REGION_TYPE * region,
    G4_ExecSize   execSize
)
{
    assert(region->getElemSize () && execSize);
    if (isUnalignedRegion(region, execSize))
        return getEncAlignedSegmentByteSize(region, execSize);
    else
        return getRegionByteSize(region, execSize);
}

// Get the byte size of the destination region.
unsigned SpillManagerGRF::getRegionByteSize(
    G4_DstRegRegion * region,
    G4_ExecSize       execSize
) const
{
    unsigned size = region->getHorzStride() * region->getElemSize() *
        (execSize - 1) + region->getElemSize();

    return size;
}

// Get the byte size of the source region.

unsigned SpillManagerGRF::getRegionByteSize(
    G4_SrcRegRegion * region,
    G4_ExecSize       execSize) const
{
    assert(execSize % region->getRegion ()->width == 0);
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
static unsigned getMaxExecSize(G4_Operand * operand)
{
    const unsigned size = Type_UNDEF + 1;
    static unsigned maxExecSize [size] {8, 8, 16, 16, 16, 16, 8, 8, 0};
    return maxExecSize[operand->getType()];
}

// Check if the instruction is a SIMD 16 or 32 instruction that is logically
// equivalent to two instructions the second of which uses register operands
// at the following row with the same sub-register index.
bool SpillManagerGRF::isComprInst(G4_INST * inst) const
{
    return inst->isComprInst();
}

// Check if the source in a compressed instruction operand occupies a second
// register.
bool SpillManagerGRF::isMultiRegComprSource(
    G4_SrcRegRegion* src,
    G4_INST *        inst) const
{
    if (!inst->isComprInst ()) {
        return false;
    }
    else if (isScalarReplication(src)) {
        return false;
    }
    else if (inst->getExecSize() <= 8) {
        return false;
    }
    else if (!src->asSrcRegRegion()->crossGRF(*builder_))
    {
        return false;
    }
    else if (inst->getExecSize() == 16 &&
             inst->getDst() &&
             inst->getDst()->getTypeSize() == 4 &&
             inst->getDst()->getHorzStride() == 1)
    {
        if (src->getTypeSize() == 2 && src->isNativePackedRegion()) {
            return false;
        } else {
            return true;
        }
    }
    else {
        return true;
    }
}

// Send message information query
unsigned SpillManagerGRF::getSendRspLengthBitOffset() const
{
    return SEND_GT_RSP_LENGTH_BIT_OFFSET;
}

// Send message information query
unsigned SpillManagerGRF::getSendMaxResponseLength() const
{
    //return SEND_GT_MAX_RESPONSE_LENGTH;
    return 8;
}

// Send message information query
unsigned SpillManagerGRF::getSendMsgLengthBitOffset()
{
    return SEND_GT_MSG_LENGTH_BIT_OFFSET;
}

// Send message information query
unsigned SpillManagerGRF::getSendMaxMessageLength() const
{
    return SEND_GT_MAX_MESSAGE_LENGTH;
}

// Send message information query
unsigned SpillManagerGRF::getSendDescDataSizeBitOffset()
{
    return SEND_GT_DESC_DATA_SIZE_BIT_OFFSET;
}

// Send message information query
unsigned SpillManagerGRF::getSendReadTypeBitOffset() const
{
    return SEND_IVB_MSG_TYPE_BIT_OFFSET;
}

// Send message information query
unsigned SpillManagerGRF::getSendWriteTypeBitOffset()
{
    return SEND_IVB_MSG_TYPE_BIT_OFFSET;
}

// Send message information query
unsigned SpillManagerGRF::getSendScReadType() const
{
    return SEND_IVB_SC_READ_TYPE;
}

// Send message information query
unsigned SpillManagerGRF::getSendScWriteType() const
{
    return SEND_IVB_SC_WRITE_TYPE;
}

// Send message information query
unsigned SpillManagerGRF::getSendOwordReadType() const
{
    return SEND_IVB_OW_READ_TYPE;
}

// Send message information query
unsigned SpillManagerGRF::getSendOwordWriteType()
{
    return SEND_IVB_OW_WRITE_TYPE;
}

unsigned SpillManagerGRF::getSendExDesc(bool isWrite, bool isScatter) const
{
    return isWrite ? SEND_IVB_DP_WR_EX_DESC_IMM : SEND_IVB_DP_RD_EX_DESC_IMM;
}

// Allocate from custom memory allocator
void *SpillManagerGRF::allocMem(unsigned size) const
{
    return builder_->mem.alloc(size);
}

bool SpillManagerGRF::useSplitSend() const
{
    return builder_->useSends();
}

// Get a unique spill range index for regvar.
unsigned SpillManagerGRF::getSpillIndex(G4_RegVar *  spilledRegVar)
{
    return spillRangeCount_[spilledRegVar->getId()]++;
}

// Get a unique fill range index for regvar.
unsigned SpillManagerGRF::getFillIndex(
    G4_RegVar *  spilledRegVar
)
{
    return fillRangeCount_[spilledRegVar->getId()]++;
}

// Get a unique tmp index for spilled regvar.
unsigned SpillManagerGRF::getTmpIndex(G4_RegVar *  spilledRegVar)
{
    return tmpRangeCount_[spilledRegVar->getId()]++;
}

// Get a unique msg index for spilled regvar.
unsigned SpillManagerGRF::getMsgSpillIndex(
    G4_RegVar *  spilledRegVar)
{
    return msgSpillRangeCount_[spilledRegVar->getId()]++;
}

// Get a unique msg index for filled regvar.
unsigned SpillManagerGRF::getMsgFillIndex(
    G4_RegVar *  spilledRegVar)
{
    return msgFillRangeCount_[spilledRegVar->getId()]++;
}

// Get a unique msg index for addr spill fill regvar.
unsigned SpillManagerGRF::getAddrSpillFillIndex(
    G4_RegVar *  spilledRegVar)
{
    return addrSpillFillRangeCount_[spilledRegVar->getId()]++;
}

// Create a unique name for a regvar representing a spill/fill/msg live range.
const char *SpillManagerGRF::createImplicitRangeName(
    const char * baseName,
    G4_RegVar *  spilledRegVar,
    unsigned     index)
{
    std::stringstream nameStrm;
    nameStrm << baseName << "_" << spilledRegVar->getName()
             << "_" << index << std::ends;
    int nameLen = unsigned(nameStrm.str().length()) + 1;
    char * name = (char *) allocMem(nameLen);
    strcpy_s(name, nameLen, nameStrm.str().c_str ());
    return name;
}

// Check if the region is a scalar replication region.
bool SpillManagerGRF::isScalarReplication(G4_SrcRegRegion * region) const
{
    return region->isScalar();
}

// Check if we have to repeat the simd16 source in the simd8 equivalents.
// The BPSEC mentions that if a replicated scalar appears in an simd16
// instruction, logically we need to repeat the source region used in
// the first simd8 instruction in the second simd8 instruction as well
// (i.e. the reg no is not incremented by one for the second).
bool SpillManagerGRF::repeatSIMD16or32Source(G4_SrcRegRegion * region) const
{
    return isScalarReplication(region);
}

// Create a declare directive for a new live range (spill/fill/msg)
// introduced as part of the spill code generation.
G4_Declare *
SpillManagerGRF::createRangeDeclare(
    const char*    name,
    G4_RegFileKind regFile,
    unsigned short nElems,
    unsigned short nRows,
    G4_Type        type,
    DeclareType    kind,
    G4_RegVar *    base,
    G4_Operand *   repRegion,
    G4_ExecSize    execSize)
{
    G4_Declare * rangeDeclare =
        builder_->createDeclareNoLookup(
            name, regFile, nElems, nRows, type, kind,
            base, repRegion, execSize);
    rangeDeclare->getRegVar()->setId(
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
G4_Declare * SpillManagerGRF::createTransientGRFRangeDeclare(
    REGION_TYPE * region,
    const char  * baseName,
    unsigned      index,
    G4_ExecSize   execSize,
    G4_INST     * inst)
{
    const char * name =
        createImplicitRangeName(baseName, getRegVar(region), index);
    G4_Type type = region->getType();
    unsigned segmentByteSize = getSegmentByteSize(region, execSize);
    DeclareType regVarKind =
        (region->isDstRegRegion ())? DeclareType::Spill : DeclareType::Fill;
    unsigned short width, height;

    if (segmentByteSize > builder_->numEltPerGRF<Type_UB>() || region->crossGRF(*builder_)) {
        assert(builder_->numEltPerGRF<Type_UB>() % region->getElemSize () == 0);
        width = builder_->numEltPerGRF<Type_UB>() / region->getElemSize ();
        assert(segmentByteSize / builder_->numEltPerGRF<Type_UB>() <= 2);
        height = 2;
    } else {
        assert(segmentByteSize % region->getElemSize () == 0);
        width = segmentByteSize / region->getElemSize ();
        height = 1;
    }

    if (needGRFAlignedOffset())
    {
        // the message will read/write a minimum of one GRF
        if (height == 1 && width < (builder_->getGRFSize() / region->getElemSize()))
            width = builder_->getGRFSize() / region->getElemSize();
    }

    G4_Declare * transientRangeDeclare =
        createRangeDeclare(
            name, G4_GRF, width, height, type,
            regVarKind, region->getBase()->asRegVar(), region, execSize);

    if (failSafeSpill_)
    {
        transientRangeDeclare->getRegVar()->setPhyReg(
            builder_->phyregpool.getGreg(spillRegOffset_), 0);
        spillRegOffset_ += height;
    }

    // FIXME: We should take the original declare's alignment too, but I'm worried
    // we may get perf regression if FE is over-aligning or the alignment is not necessary for this inst.
    // So Either is used for now and we can change it later if there are bugs
    setNewDclAlignment(gra, transientRangeDeclare, false);
    return transientRangeDeclare;
}

static unsigned short getSpillRowSizeForSendDst(G4_INST * inst)
{
    assert(inst);
    unsigned short nRows = 0;
    const IR_Builder& builder = inst->getBuilder();
    auto dst = inst->getDst();

    if (inst->isSend())
    {
        G4_SendDesc* msgDesc = inst->getMsgDesc();
        nRows = msgDesc->getDstLenRegs();
        if (dst->getTopDcl()->getByteSize() <= inst->getBuilder().getGRFSize())
        {
            // we may have a send that that writes to a <1 GRF variable, but due to A64 message requirements
            // the send has a response length > 1. We return row size as one instead as we've only allocated
            // one GRF for the spilled variable in scratch space
            nRows = 1;
        }
    }
    else
    {
        assert(dst->getLinearizedStart() % builder.numEltPerGRF<Type_UB>() == 0);
        nRows = (dst->getLinearizedEnd() - dst->getLinearizedStart() + 1) / builder.numEltPerGRF<Type_UB>();
    }
    return nRows;
}

// Create a regvar and its declare directive to represent the spill live
// range that appears as a send instruction post destination GRF.
// The type of the regvar is set as dword and its width 8. The type of
// the post destination does not matter, so we just use type dword, and
// a width of 8 so that a row corresponds to a physical register.
G4_Declare * SpillManagerGRF::createPostDstSpillRangeDeclare(G4_INST *sendOut)
{
    auto dst = sendOut->getDst();
    G4_RegVar * spilledRegVar = getRegVar(dst);
    const char * name =
        createImplicitRangeName(
            "SP_GRF", spilledRegVar, getSpillIndex (spilledRegVar));
    unsigned short nRows = getSpillRowSizeForSendDst(sendOut);

      G4_DstRegRegion * normalizedPostDst = builder_->createDst(
        spilledRegVar, dst->getRegOff(), SUBREG_ORIGIN,
        DEF_HORIZ_STRIDE, Type_UD);

    // We use the width as the user specified, the height however is
    // calculated based on the message descriptor to limit register
    // pressure induced by the spill range.

    G4_Declare * transientRangeDeclare =
        createRangeDeclare(
            name, G4_GRF, builder_->numEltPerGRF<Type_UD>(), nRows, Type_UD,
            DeclareType::Spill, spilledRegVar, normalizedPostDst,
            G4_ExecSize(builder_->numEltPerGRF<Type_UD>()));

    if (failSafeSpill_)
    {
        if (useSplitSend())
        {
            transientRangeDeclare->getRegVar()->setPhyReg(
                builder_->phyregpool.getGreg(spillRegStart_), 0);
            spillRegOffset_ += nRows;
        }
        else
        {
            transientRangeDeclare->getRegVar()->setPhyReg(
                builder_->phyregpool.getGreg(spillRegStart_+1), 0);
            spillRegOffset_ += nRows + 1;
        }
    }

    return transientRangeDeclare;
}

// Create a regvar and its declare directive to represent the spill live range.
G4_Declare * SpillManagerGRF::createSpillRangeDeclare(
    G4_DstRegRegion * spilledRegion,
    G4_ExecSize       execSize,
    G4_INST         * inst
)
{
    return
        createTransientGRFRangeDeclare(
            spilledRegion, "SP_GRF",
            getSpillIndex (getRegVar(spilledRegion)),
            execSize, inst);
}

// Create a regvar and its declare directive to represent the GRF fill live
// range.
G4_Declare * SpillManagerGRF::createGRFFillRangeDeclare(
    G4_SrcRegRegion * fillRegion,
    G4_ExecSize       execSize,
    G4_INST         * inst
)
{
    assert(getRFType (fillRegion) == G4_GRF);
    G4_Declare * fillRangeDecl =
        createTransientGRFRangeDeclare(
            fillRegion, "FL_GRF", getFillIndex(getRegVar(fillRegion)),
            execSize, inst);
    return fillRangeDecl;
}

static unsigned short getSpillRowSizeForSendSrc(
    G4_INST *         inst,
    G4_SrcRegRegion * filledRegion)
{
    assert(inst);
    const IR_Builder& builder = inst->getBuilder();
    unsigned short nRows = 0;

    if (inst->isSend())
    {
        G4_SendDesc* msgDesc = inst->getMsgDesc();
        if (inst->isSplitSend() &&
            (inst->getSrc(1)->asSrcRegRegion() == filledRegion))
        {
            nRows = msgDesc->getSrc1LenRegs();
        }
        else
        {
            nRows = msgDesc->getSrc0LenRegs();
        }
    }
    else
    {
        unsigned int rangeSize = filledRegion->getLinearizedEnd() - filledRegion->getLinearizedStart() + 1;
        unsigned int grfSize = builder.numEltPerGRF<Type_UB>();
        nRows = (rangeSize / grfSize) + ((rangeSize % grfSize) == 0 ? 0 : 1);
    }

    return nRows;
}


// Create a regvar and its declare directive to represent the GRF fill live range.
G4_Declare * SpillManagerGRF::createSendFillRangeDeclare(
    G4_SrcRegRegion * filledRegion,
    G4_INST *         sendInst)
{
    G4_RegVar * filledRegVar = getRegVar(filledRegion);
    const char * name =
        createImplicitRangeName(
            "FL_Send", filledRegVar, getFillIndex(filledRegVar));
    unsigned short nRows = getSpillRowSizeForSendSrc(sendInst, filledRegion);

    G4_SrcRegRegion * normalizedSendSrc =
        builder_->createSrcRegRegion(
        filledRegion->getModifier(), Direct, filledRegVar,
        filledRegion->getRegOff(), filledRegion->getSubRegOff(), filledRegion->getRegion(),
        filledRegion->getType());
    unsigned short width = builder_->numEltPerGRF<Type_UB>() / filledRegion->getElemSize ();
    assert(builder_->numEltPerGRF<Type_UB>() % filledRegion->getElemSize () == 0);

    // We use the width as the user specified, the height however is
    // calculated based on the message descriptor to limit register
    // pressure induced by the spill range.

    G4_Declare * transientRangeDeclare =
        createRangeDeclare(
        name,
        G4_GRF,
        width, nRows, filledRegion->getType(),
        DeclareType::Fill, filledRegVar, normalizedSendSrc,
        G4_ExecSize(width));

    setNewDclAlignment(gra, transientRangeDeclare, gra.isEvenAligned(filledRegVar->getDeclare()));

    if (failSafeSpill_)
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
G4_Declare * SpillManagerGRF::createTemporaryRangeDeclare(
    G4_DstRegRegion * spilledRegion,
    G4_ExecSize       execSize,
    bool              forceSegmentAlignment)
{
    const char * name =
        createImplicitRangeName(
            "TM_GRF", getRegVar(spilledRegion),
            getTmpIndex(getRegVar(spilledRegion)));
    unsigned byteSize =
        (forceSegmentAlignment)?
        getSegmentByteSize(spilledRegion, execSize):
        getRegionByteSize(spilledRegion, execSize);

    // ensure tmp reg is large enough to hold all data when sub-reg offset is non-zero
    byteSize += spilledRegion->getSubRegOff() * spilledRegion->getElemSize();

    assert(byteSize <= 2u * builder_->numEltPerGRF<Type_UB>());
    assert(byteSize % spilledRegion->getElemSize () == 0);

    G4_Type type = spilledRegion->getType();
    DeclareType regVarKind = DeclareType::Tmp;

    unsigned short width, height;
    if (byteSize > builder_->numEltPerGRF<Type_UB>())
    {
        height = 2;
        width = builder_->numEltPerGRF<Type_UB>() / spilledRegion->getElemSize();
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
            regVarKind, spilledRegVar, NULL, G4_ExecSize(0));

    if (failSafeSpill_)
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
G4_DstRegRegion * SpillManagerGRF::createSpillRangeDstRegion(
    G4_RegVar *       spillRangeRegVar,
    G4_DstRegRegion * spilledRegion,
    G4_ExecSize       execSize,
    unsigned          regOff)
{
    if (isUnalignedRegion  (spilledRegion, execSize)) {
        unsigned segmentDisp =
            getEncAlignedSegmentDisp(spilledRegion, execSize);
        unsigned regionDisp = getRegionDisp(spilledRegion);
        assert(regionDisp >= segmentDisp);
        unsigned short subRegOff =
            (regionDisp - segmentDisp) / spilledRegion->getElemSize ();
        assert(
            (regionDisp - segmentDisp) % spilledRegion->getElemSize () == 0);
        assert(subRegOff * spilledRegion->getElemSize () +
                getRegionByteSize(spilledRegion, execSize) <=
                2u * builder_->numEltPerGRF<Type_UB>());

        if (useScratchMsg_)
        {
            G4_Declare* parent_dcl = spilledRegion->getBase()->asRegVar()->getDeclare();
            unsigned off = 0;
            while (parent_dcl->getAliasDeclare() != NULL)
            {
                // off is in bytes
                off += parent_dcl->getAliasOffset();
                parent_dcl = parent_dcl->getAliasDeclare();
            }
            off = off % builder_->numEltPerGRF<Type_UB>();
            // sub-regoff is in units of element size
            subRegOff = spilledRegion->getSubRegOff() + off/spilledRegion->getElemSize();
        }

        return builder_->createDst(
            spillRangeRegVar, (unsigned short) regOff, subRegOff,
            spilledRegion->getHorzStride(), spilledRegion->getType());
    }

    else {
        return builder_->createDst(
            spillRangeRegVar, (short) regOff, SUBREG_ORIGIN,
            spilledRegion->getHorzStride(), spilledRegion->getType());
    }
}

// Create a source region that could be used to copy out the temporary range
// (that was created to replace the portion of the spilled live range appearing
// in an instruction destination) into the segment aligned spill range for the
// spilled live range that can be written out to spill memory.
G4_SrcRegRegion * SpillManagerGRF::createTemporaryRangeSrcRegion (
    G4_RegVar *       tmpRangeRegVar,
    G4_DstRegRegion * spilledRegion,
    G4_ExecSize       execSize,
    unsigned          regOff)
{
    uint16_t horzStride = spilledRegion->getHorzStride();
    // A scalar region is returned when execsize is 1.
    const RegionDesc *rDesc = builder_->createRegionDesc(execSize, horzStride, 1, 0);

    return builder_->createSrc(tmpRangeRegVar, (short) regOff, spilledRegion->getSubRegOff(),
        rDesc, spilledRegion->getType());
}

// Create a source region that could be used in place of the fill regvar.
// If the region is unaligned then the origin of the destination region
// is the displacement of the orginal region from its segment, else the
// origin is 0.
G4_SrcRegRegion * SpillManagerGRF::createFillRangeSrcRegion (
    G4_RegVar *       fillRangeRegVar,
    G4_SrcRegRegion * filledRegion,
    G4_ExecSize       execSize)
{
    // we need to preserve accRegSel if it's set
    if (isUnalignedRegion(filledRegion, execSize)) {
        unsigned segmentDisp =
            getEncAlignedSegmentDisp(filledRegion, execSize);
        unsigned regionDisp = getRegionDisp(filledRegion);
        assert(regionDisp >= segmentDisp);
        unsigned short subRegOff =
            (regionDisp - segmentDisp) / filledRegion->getElemSize ();
        assert(
            (regionDisp - segmentDisp) % filledRegion->getElemSize () == 0);

        return builder_->createSrcRegRegion(
            filledRegion->getModifier (), Direct, fillRangeRegVar, REG_ORIGIN,
            subRegOff, filledRegion->getRegion(), filledRegion->getType(), filledRegion->getAccRegSel());
    }
    else
    {
        // fill intrinsic's sub-reg offset is always 0 since it is GRF aligned.
        // but original filled range's offset may not be 0, so actual filled
        // src needs to use sub-reg offset from original region.
        return builder_->createSrcRegRegion(
            filledRegion->getModifier (), Direct, fillRangeRegVar,
            REG_ORIGIN, filledRegion->getSubRegOff(), filledRegion->getRegion (),
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
G4_SrcRegRegion * SpillManagerGRF::createBlockSpillRangeSrcRegion(
    G4_RegVar *       spillRangeRegVar,
    unsigned          regOff,
    unsigned          subregOff)
{
    assert(getByteSize (spillRangeRegVar) % DWORD_BYTE_SIZE == 0);
    const RegionDesc * rDesc =
        builder_->rgnpool.createRegion(DWORD_BYTE_SIZE, DWORD_BYTE_SIZE, 1);
    return builder_->createSrc(spillRangeRegVar, (short) regOff, (short) subregOff,
        rDesc, Type_UD);
}

// Create a GRF regvar and a declare directive for it, to represent an
// implicit MFR live range that will be used as the send message payload
// header and write payload for spilling a regvar to memory.
G4_Declare * SpillManagerGRF::createMRangeDeclare(G4_RegVar * regVar)
{
    if (useSplitSend() && useScratchMsg_)
    {
        return builder_->getBuiltinR0();
    }
    else if (useLSCMsg)
    {
        return nullptr;
    }

    G4_RegVar * repRegVar =
        (regVar->isRegVarTransient ()) ? regVar->getBaseRegVar(): regVar;
    const char * name =
        createImplicitRangeName(
            "SP_MSG", repRegVar, getMsgSpillIndex(repRegVar));
    unsigned regVarByteSize = getByteSize (regVar);
    unsigned writePayloadHeight = cdiv(regVarByteSize, builder_->numEltPerGRF<Type_UB>());

    if (writePayloadHeight > SPILL_PAYLOAD_HEIGHT_LIMIT) {
        writePayloadHeight = SPILL_PAYLOAD_HEIGHT_LIMIT;
    }

    unsigned payloadHeaderHeight =
        (regVarByteSize != DWORD_BYTE_SIZE)?
        OWORD_PAYLOAD_HEADER_MAX_HEIGHT: DWORD_PAYLOAD_HEADER_MAX_HEIGHT;
    unsigned short height = payloadHeaderHeight + writePayloadHeight;
    unsigned short width = builder_->numEltPerGRF<Type_UD>();

    // We should not find ourselves using dword scattered write
    if (useScratchMsg_)
    {
        assert(payloadHeaderHeight != DWORD_PAYLOAD_HEADER_MAX_HEIGHT);
    }

    G4_Declare * msgRangeDeclare =
        createRangeDeclare(
            name,
            G4_GRF,
            width, height, Type_UD,
            DeclareType::Tmp, regVar->getNonTransientBaseRegVar (), NULL, G4_ExecSize(0));

    if (failSafeSpill_)
    {
        msgRangeDeclare->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegStart_), 0);
    }

    return msgRangeDeclare;
}

// Create a GRF regvar and a declare directive for it, to represent an
// implicit MFR live range that will be used as the send message payload
// header and write payload for spilling a regvar region to memory.
G4_Declare * SpillManagerGRF::createMRangeDeclare(
    G4_DstRegRegion * region,
    G4_ExecSize       execSize)
{
    if (useSplitSend() && useScratchMsg_)
    {
        return builder_->getBuiltinR0();
    }
    else if (useLSCMsg)
    {
        return nullptr;
    }

    const char * name =
        createImplicitRangeName(
            "SP_MSG", getRegVar(region),
            getMsgSpillIndex(getRegVar(region)));
    unsigned regionByteSize = getSegmentByteSize(region, execSize);
    unsigned writePayloadHeight = cdiv(regionByteSize, builder_->numEltPerGRF<Type_UB>());
    unsigned msgType = getMsgType (region, execSize);
    unsigned payloadHeaderHeight =
        (msgType == owordMask() || msgType == hwordMask ()) ?
        OWORD_PAYLOAD_HEADER_MAX_HEIGHT: DWORD_PAYLOAD_HEADER_MAX_HEIGHT;

    // We should not find ourselves using dword scattered write
    if (useScratchMsg_)
    {
        assert(payloadHeaderHeight != DWORD_PAYLOAD_HEADER_MAX_HEIGHT);
    }

    unsigned height = payloadHeaderHeight + writePayloadHeight;
    unsigned short width = builder_->numEltPerGRF<Type_UD>();
    G4_Declare * msgRangeDeclare =
        createRangeDeclare(
            name,
            G4_GRF,
            width, (unsigned short) height, Type_UD,
            DeclareType::Tmp, region->getBase()->asRegVar(), NULL, G4_ExecSize(0));

    if (failSafeSpill_)
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
SpillManagerGRF::createMRangeDeclare(
    G4_SrcRegRegion * region,
    G4_ExecSize       execSize
)
{
    if (useSplitSend() && useScratchMsg_)
    {
        return builder_->getBuiltinR0();
    }
    else if (useLSCMsg)
    {
        return nullptr;
    }

    const char * name =
        createImplicitRangeName(
            "FL_MSG", getRegVar(region),
            getMsgFillIndex(getRegVar(region)));
    getSegmentByteSize(region, execSize);
    unsigned payloadHeaderHeight =
        (getMsgType (region, execSize) == owordMask()) ?
        OWORD_PAYLOAD_HEADER_MIN_HEIGHT : DWORD_PAYLOAD_HEADER_MIN_HEIGHT;

    // We should not find ourselves using dword scattered write
    if (useScratchMsg_)
    {
        assert(payloadHeaderHeight != DWORD_PAYLOAD_HEADER_MAX_HEIGHT);
        // When using scratch msg descriptor we don't need to use a
        // separate GRF for payload. Source operand of send can directly
        // use r0.0.
        return builder_->getBuiltinR0();
    }

    unsigned height = payloadHeaderHeight;
    unsigned width = builder_->numEltPerGRF<Type_UD>();
    G4_Declare * msgRangeDeclare =
        createRangeDeclare (
            name,
            G4_GRF,
            (unsigned short) width, (unsigned short) height, Type_UD,
            DeclareType::Tmp, region->getBase()->asRegVar(), NULL, G4_ExecSize(0));

    if (failSafeSpill_)
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
G4_DstRegRegion * SpillManagerGRF::createMPayloadBlockWriteDstRegion(
    G4_RegVar *       grfRange,
    unsigned          regOff,
    unsigned          subregOff)
{
    regOff += OWORD_PAYLOAD_WRITE_REG_OFFSET;
    subregOff += OWORD_PAYLOAD_WRITE_SUBREG_OFFSET;
    return builder_->createDst(
        grfRange, (short) regOff, (short) subregOff, DEF_HORIZ_STRIDE, Type_UD);
}

// Create a destination region for the GRF regvar for the input header
// payload portion of the send message to the data port. The exec size
// needs to be 8 for the mov instruction that uses this as a destination.
G4_DstRegRegion * SpillManagerGRF::createMHeaderInputDstRegion(
    G4_RegVar *       grfRange,
    unsigned          subregOff)
{
    return builder_->createDst(
        grfRange, PAYLOAD_INPUT_REG_OFFSET, (short) subregOff,
        DEF_HORIZ_STRIDE, Type_UD);
}

// Create a destination region for the GRF regvar for the payload offset
// portion of the oword block send message. The exec size needs to be 1
// for the mov instruction that uses this as a destination.
G4_DstRegRegion * SpillManagerGRF::createMHeaderBlockOffsetDstRegion(
    G4_RegVar *       grfRange)
{
    return builder_->createDst(
        grfRange, OWORD_PAYLOAD_SPOFFSET_REG_OFFSET,
        OWORD_PAYLOAD_SPOFFSET_SUBREG_OFFSET, DEF_HORIZ_STRIDE,
        Type_UD);
}

// Create a source region for the input payload (r0.0). The exec size
// needs to be 8 for the mov instruction that uses this as a source.
G4_SrcRegRegion *
SpillManagerGRF::createInputPayloadSrcRegion()
{
    G4_RegVar * inputPayloadDirectReg = builder_->getBuiltinR0()->getRegVar();
    const RegionDesc * rDesc =
        builder_->rgnpool.createRegion(
            builder_->numEltPerGRF<Type_UD>(), builder_->numEltPerGRF<Type_UD>(), DEF_HORIZ_STRIDE);
    return builder_->createSrc(inputPayloadDirectReg,
        PAYLOAD_INPUT_REG_OFFSET, PAYLOAD_INPUT_SUBREG_OFFSET,
        rDesc, Type_UD);
}

// Create and initialize the message header for the send instruction for
// save/load of value to/from memory.
// The header includes the input payload and the offset (for spill disp).
template <class REGION_TYPE>
G4_Declare * SpillManagerGRF::createAndInitMHeader(
    REGION_TYPE * region,
    G4_ExecSize  execSize)
{
    G4_Declare * mRangeDcl = createMRangeDeclare(region, execSize);
    return initMHeader (mRangeDcl, region, execSize);
}

// Initialize the message header for the send instruction for save/load
// of value to/from memory.
// The header includes the input payload and the offset (for spill disp).
template <class REGION_TYPE>
G4_Declare * SpillManagerGRF::initMHeader(
    G4_Declare *  mRangeDcl,
    REGION_TYPE * region,
    G4_ExecSize  execSize)
{
    // Initialize the message header with the input payload.
    if ((useScratchMsg_ && mRangeDcl == builder_->getBuiltinR0()) || !headerNeeded())
    {
        // mRangeDcl is NULL for fills
        return mRangeDcl;
    }

    G4_DstRegRegion * mHeaderInputDstRegion =
        createMHeaderInputDstRegion(mRangeDcl->getRegVar());
    G4_SrcRegRegion * inputPayload = createInputPayloadSrcRegion();
    createMovInst(G4_ExecSize(builder_->numEltPerGRF<Type_UD>()), mHeaderInputDstRegion, inputPayload);
    numGRFMove++;

    if (useScratchMsg_)
    {
        // Initialize msg header when region is a spill
        // When using scratch msg description, we only need to copy
        // r0.0 in to msg header. Memory offset will be
        // specified in the msg descriptor.
    }
    else
    {
      // Initialize the message header with the spill disp for block
      // read/write.
        G4_DstRegRegion * mHeaderOffsetDstRegion =
            createMHeaderBlockOffsetDstRegion(mRangeDcl->getRegVar());
        int offset = getSegmentDisp(region, execSize);
        getSpillOffset(offset);
        unsigned segmentDisp = offset / OWORD_BYTE_SIZE;
        G4_Imm * segmentDispImm = builder_->createImm (segmentDisp, Type_UD);

        if (!region->isSrcRegRegion() && !region->isDstRegRegion())
        {
            MUST_BE_TRUE(false, ERROR_GRAPHCOLOR);
        }

        if (builder_->getIsKernel() == false)
        {
            createAddFPInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
        }
        else
        {
            createMovInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
        }
        numGRFMove++;
    }

    // Initialize the message header with the spill disp for scatter
    // read/write.
    return mRangeDcl;
}

// Create and initialize the message header for the send instruction.
// The header includes the input payload (for spill disp).
G4_Declare * SpillManagerGRF::createAndInitMHeader(G4_RegVar * regVar)
{
    G4_Declare * mRangeDcl = createMRangeDeclare(regVar);
    return initMHeader (mRangeDcl);
}

// Initialize the message header for the send instruction.
// The header includes the input payload (for spill disp).
G4_Declare * SpillManagerGRF::initMHeader(G4_Declare * mRangeDcl)
{
    // Initialize the message header with the input payload.
    if ((useScratchMsg_ && mRangeDcl == builder_->getBuiltinR0()) || !headerNeeded())
    {
        // mRangeDcl is NULL for fills
        return mRangeDcl;
    }

    G4_DstRegRegion * mHeaderInputDstRegion =
        createMHeaderInputDstRegion(mRangeDcl->getRegVar());
    G4_SrcRegRegion * inputPayload = createInputPayloadSrcRegion();
    createMovInst(G4_ExecSize(builder_->numEltPerGRF<Type_UD>()), mHeaderInputDstRegion, inputPayload);
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
void SpillManagerGRF::initMWritePayload(
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    unsigned          regOff,
    unsigned          height)
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
                createBlockSpillRangeSrcRegion(
                    spillRangeDcl->getRegVar(), i + regOff);
            G4_DstRegRegion * mPayloadWriteDstRegion =
                createMPayloadBlockWriteDstRegion (
                    mRangeDcl->getRegVar(), i);
            G4_ExecSize movExecSize =
                G4_ExecSize((nRows > 1) ? builder_->numEltPerGRF<Type_UD>() : spillRangeDcl->getNumElems());
            createMovInst(
                movExecSize, mPayloadWriteDstRegion, spillRangeSrcRegion);
            numGRFMove ++;
        }
    }
}

// Initialize the the write payload part of the message for spilled regions.
void SpillManagerGRF::initMWritePayload(
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    G4_DstRegRegion * spilledRangeRegion,
    G4_ExecSize       execSize,
    unsigned          regOff)
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
            createBlockSpillRangeSrcRegion(
                spillRangeDcl->getRegVar(), regOff);
        G4_DstRegRegion * mPayloadWriteDstRegion =
            createMPayloadBlockWriteDstRegion (mRangeDcl->getRegVar());
        unsigned segmentByteSize =
            getSegmentByteSize(spilledRangeRegion, execSize);
        G4_ExecSize movExecSize {segmentByteSize / DWORD_BYTE_SIZE};

        // Write entire GRF when using scratch msg descriptor
        if (useScratchMsg_)
        {
            if (movExecSize <= 8)
                movExecSize = g4::SIMD8;
            else if (movExecSize < g4::SIMD16)
                movExecSize = g4::SIMD16;
        }

        assert(segmentByteSize % DWORD_BYTE_SIZE == 0);
        assert(movExecSize <= g4::SIMD16);
        createMovInst(
            movExecSize, mPayloadWriteDstRegion, spillRangeSrcRegion);
        numGRFMove ++;
    }
}

// Return the block size encoding for oword block reads.
unsigned SpillManagerGRF::blockSendBlockSizeCode(unsigned size)
{
    auto code = GlobalRA::sendBlockSizeCode(size);
    return code << getSendDescDataSizeBitOffset();
}

// Return the block size encoding for dword scatter reads.
unsigned SpillManagerGRF::scatterSendBlockSizeCode(unsigned size) const
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
            assert(0);
            code = 0;
    }

    return code << getSendDescDataSizeBitOffset();
}

static uint32_t getScratchBlocksizeEncoding(int numGRF, const IR_Builder& builder)
{

    int size = (numGRF * builder.getGRFSize()) / 32; // in HWwords
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
        assert(builder.getPlatform() >= GENX_SKL);
        blocksize_encoding = 0x3;
    }
    else
        assert(false);
    return blocksize_encoding;
}

std::tuple<uint32_t, G4_ExecSize>
SpillManagerGRF::createSpillSendMsgDescOWord(const IR_Builder& builder, unsigned int height)
{
    unsigned segmentByteSize = height * builder.numEltPerGRF<Type_UB>();
    unsigned writePayloadCount = cdiv(segmentByteSize, builder.numEltPerGRF<Type_UB>());
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
    auto execSize = G4_ExecSize(LIMIT_SEND_EXEC_SIZE(segmentOwordSize * DWORD_BYTE_SIZE));

    return std::make_tuple(message, execSize);
}

// Create the message descriptor for a spill send instruction for spilled
// post destinations of send instructions.
G4_Imm * SpillManagerGRF::createSpillSendMsgDesc(
    unsigned      regOff,
    unsigned      height,
    G4_ExecSize & execSize,
    G4_RegVar*    base)
{
    unsigned message = 0;

    if (useScratchMsg_)
    {
        unsigned headerPresent = 0x80000;
        message = headerPresent;
        unsigned msgLength = useSplitSend() ? SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT : SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT + height;
        message |= (msgLength << getSendMsgLengthBitOffset());
        message |= (1 << SCRATCH_MSG_DESC_CATEORY);
        message |= (1 << SCRATCH_MSG_DESC_CHANNEL_MODE);
        message |= (1 << SCRATCH_MSG_DESC_OPERATION_MODE);
        unsigned blocksize_encoding = getScratchBlocksizeEncoding(height, *builder_);
        message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
        int offset = getDisp(base);
        getSpillOffset(offset);
        // message expects offsets to be in HWord
        message |= (offset + regOff * builder_->getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;
        execSize = g4::SIMD16;
    }
    else
    {
        auto [message, retSize] = createSpillSendMsgDescOWord(*builder_, height);
        execSize = retSize;
    }
    return builder_->createImm (message, Type_UD);
}

// Create the message descriptor for a spill send instruction for spilled
// destination regions.
std::tuple<G4_Imm*, G4_ExecSize>
SpillManagerGRF::createSpillSendMsgDesc(
    G4_DstRegRegion * spilledRangeRegion,
    G4_ExecSize     execSize)
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
        unsigned segmentByteSize = getSegmentByteSize(spilledRangeRegion, execSize);
        unsigned writePayloadCount = cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
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
        unsigned numGRFs = cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());

        unsigned blocksize_encoding = getScratchBlocksizeEncoding(numGRFs, *builder_);

        message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
        int offset = getRegionDisp(spilledRangeRegion);
        getSpillOffset(offset);
        message |= offset >> SCRATCH_SPACE_ADDRESS_UNIT;
        if (numGRFs > 1)
        {
            execSize = g4::SIMD16;
        }
        else
        {
            if (execSize > g4::SIMD8)
            {
                execSize = g4::SIMD16;
            }
            else
            {
                execSize = g4::SIMD8;
            }
        }
    }
    else
    {
        unsigned segmentByteSize =
            getSegmentByteSize(spilledRangeRegion, execSize);
        unsigned writePayloadCount = cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
        unsigned statelessSurfaceIndex = 0xFF;
        message = statelessSurfaceIndex;

        unsigned headerPresent = 0x80000;
        message |= headerPresent;
        unsigned messageType = getSendOwordWriteType();
        message |= messageType << getSendWriteTypeBitOffset();
        unsigned payloadHeaderCount = OWORD_PAYLOAD_HEADER_MAX_HEIGHT;
        unsigned messageLength = useSplitSend() ? payloadHeaderCount : writePayloadCount + payloadHeaderCount;
        message |= messageLength << getSendMsgLengthBitOffset();
        unsigned segmentOwordSize = cdiv(segmentByteSize, OWORD_BYTE_SIZE);
        message |= blockSendBlockSizeCode (segmentOwordSize);
        execSize = G4_ExecSize(LIMIT_SEND_EXEC_SIZE(segmentOwordSize * DWORD_BYTE_SIZE));
    }
    return std::make_tuple(builder_->createImm (message, Type_UD), execSize);
}

// Create an add instruction to add the FP needed for generating spill/fill code.
// We always set the NoMask flag and use a null conditional modifier.
G4_INST * SpillManagerGRF::createAddFPInst(
    G4_ExecSize       execSize,
    G4_DstRegRegion * dst,
    G4_Operand *      src)
{
    const RegionDesc* rDesc = builder_->getRegionScalar();
    G4_Operand* fp = builder_->createSrc(builder_->kernel.fg.framePtrDcl->getRegVar(),
        0, 0, rDesc, Type_UD);
    auto newInst = builder_->createBinOp(G4_add, execSize, dst, fp, src, InstOpt_WriteEnable, true);
    newInst->inheritDIFrom(curInst);

    return newInst;

}

// Create a mov instruction needed for generating spill/fill code.
// We always set the NoMask flag and use a null conditional modifier.
G4_INST * SpillManagerGRF::createMovInst(
    G4_ExecSize       execSize,
    G4_DstRegRegion * dst,
    G4_Operand *      src,
    G4_Predicate *    predicate,
    unsigned int      options)
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
G4_INST * SpillManagerGRF::createSendInst(
    G4_ExecSize       execSize,
    G4_DstRegRegion * postDst,
    G4_SrcRegRegion * payload,
    G4_Imm *          desc,
    SFID              funcID,
    bool              isWrite,
    unsigned          option)
{
    // ToDo: create exDesc in createSendMsgDesc()
    uint32_t exDesc = G4_SendDescRaw::createExtDesc(funcID);
    auto msgDesc = builder_->createSendMsgDesc(funcID, (uint32_t)desc->getInt(), exDesc, 0,
        isWrite ? SendAccess::WRITE_ONLY : SendAccess::READ_ONLY, nullptr);
    auto sendInst = builder_->createSendInst(
        NULL, G4_send, execSize, postDst,
        payload, desc, option, msgDesc, true);
    sendInst->inheritDIFrom(curInst);

    return sendInst;
}

// Create the send instructions to fill in the value of spillRangeDcl into
// fillRangeDcl in aligned portions.
static int getNextSize(int height, bool useHWordMsg, const IR_Builder& irb)
{
    bool has8GRFMessage = useHWordMsg && irb.getPlatform() >= GENX_SKL &&
        irb.getGRFSize() == 32;
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
SpillManagerGRF::sendInSpilledRegVarPortions(
    G4_Declare *      fillRangeDcl,
    G4_Declare *      mRangeDcl,
    unsigned          regOff,
    unsigned          height,
    unsigned          srcRegOff)
{
    //if (!headerNeeded())
    if ((useScratchMsg_ && mRangeDcl == builder_->getBuiltinR0()) || !headerNeeded())
    {
        // Skip initializing message header
    }
    else
    {
        // Initialize the message header with the spill disp for portion.
        int offset = getDisp(fillRangeDcl->getRegVar()) + regOff * builder_->numEltPerGRF<Type_UB>();
        getSpillOffset(offset);

        unsigned segmentDisp = offset / OWORD_BYTE_SIZE;
        G4_Imm * segmentDispImm = builder_->createImm(segmentDisp, Type_UD);
        G4_DstRegRegion * mHeaderOffsetDstRegion =
            createMHeaderBlockOffsetDstRegion(mRangeDcl->getRegVar());

        if (builder_->getIsKernel() == false)
        {
            createAddFPInst(
                g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
        }
        else
        {
            createMovInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
        }
        numGRFMove ++;
    }

    // Read in the portions using a greedy approach.
    int currentStride = getNextSize(height, useScratchMsg_, *builder_);

    if (currentStride)
    {
        if (useLSCMsg)
        {
            createLSCFill(fillRangeDcl, mRangeDcl, regOff, currentStride, srcRegOff);
        }
        else
        {
            createFillSendInstr(fillRangeDcl, mRangeDcl, regOff, currentStride, srcRegOff);
        }

        if (height - currentStride > 0)
        {
            sendInSpilledRegVarPortions(
                fillRangeDcl, mRangeDcl, regOff + currentStride,
                height - currentStride, srcRegOff + currentStride);
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
bool SpillManagerGRF::shouldPreloadSpillRange(
    G4_INST* instContext, G4_BB* parentBB)
{
    // Check for partial and unaligned regions and add pre-load code, if
    // necessary.
    auto spilledRangeRegion = instContext->getDst();
    G4_ExecSize execSize = instContext->getExecSize();

    if (isPartialRegion(spilledRangeRegion, execSize) ||
        isUnalignedRegion(spilledRangeRegion, execSize) ||
        instContext->isPartialWriteForSpill(!parentBB->isAllLaneActive()))
    {
        // special check for scalar variables: no need for pre-fill if instruction writes to whole variable and is not predicated
        auto spilledDcl = spilledRangeRegion->getTopDcl()->getRootDeclare();
        if (execSize == g4::SIMD1 && spilledRangeRegion->getTypeSize() == spilledDcl->getByteSize() && !instContext->getPredicate())
        {
            //ToDo: investigate why we are spilling so many scalar variables
            return false;
        }
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
void SpillManagerGRF::preloadSpillRange(
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    G4_DstRegRegion * spilledRangeRegion,
    G4_ExecSize       execSize)
{
    // When execSize is 32, regions <32, 32, 1> or <64; 32, 2> are invalid.
    // Use a uniform region descriptor <stride; 1, 0>. Note that stride could
    // be 0 when execsize is 1.
    uint16_t hstride = spilledRangeRegion->getHorzStride();
    const RegionDesc *rDesc = builder_->createRegionDesc(execSize, hstride, 1, 0);

    G4_SrcRegRegion* preloadRegion = builder_->createSrc(spillRangeDcl->getRegVar(),
        REG_ORIGIN, spilledRangeRegion->getSubRegOff(),
        rDesc, spilledRangeRegion->getType());

    if (useScratchMsg_)
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
    }

    if (useLSCMsg)
    {
        createLSCFill(spillRangeDcl, mRangeDcl, preloadRegion, execSize);
    }
    else
    {
        createFillSendInstr(spillRangeDcl, mRangeDcl, preloadRegion, execSize);
    }

}

G4_SrcRegRegion* vISA::getSpillFillHeader(IR_Builder& builder, G4_Declare * decl)
{
    if (builder.supportsLSC())
    {
        // LSC in its current incarnation needs a header to store the address
        return builder.createSrcRegRegion(builder.getSpillFillHeader(), builder.getRegionStride1());
    }
    return builder.createSrcRegRegion(decl, builder.getRegionStride1());
}

// Create the send instruction to perform the spill of the spilled regvars's
// segment into spill memory.
// regOff - Offset of sub-spill. If one spill is split into more than one spill,
// this is the offset of them, unit in register size
//  spillOff - Offset of the original variable being spilled, unit in register size.
G4_INST *SpillManagerGRF::createSpillSendInstr(
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    unsigned          regOff,
    unsigned          height,
    unsigned          spillOff)
{
    G4_ExecSize execSize (0);

    G4_Imm * messageDescImm = NULL;

    if (useScratchMsg_)
    {
        G4_RegVar* r = spillRangeDcl->getRegVar();
        G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
        messageDescImm =
            createSpillSendMsgDesc(spillOff, height, execSize, rvar->getBaseRegVar());
#ifdef _DEBUG
        int offset = (messageDescImm->getInt() & 0xFFF) * builder_->numEltPerGRF<Type_UB>();
        MUST_BE_TRUE(offset >= globalScratchOffset, "incorrect offset");
#endif
    }
    else
    {
        messageDescImm =
            createSpillSendMsgDesc(regOff, height, execSize);
    }

    G4_DstRegRegion * postDst = builder_->createNullDst(execSize > g4::SIMD8 ? Type_UW : Type_UD);

    G4_INST* sendInst = NULL;
    if (useSplitSend())
    {
        auto headerOpnd = getSpillFillHeader(*builder_, mRangeDcl);
        G4_SrcRegRegion* srcOpnd = createBlockSpillRangeSrcRegion(spillRangeDcl->getRegVar(), regOff);

        auto off = G4_SpillIntrinsic::InvalidOffset;
        G4_Declare* fp = nullptr;
        if (useScratchMsg_)
            off = (messageDescImm->getInt() & 0xfff);
        else
        {
            if (builder_->usesStack())
            {
                G4_RegVar* r = spillRangeDcl->getRegVar();
                G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
                int offset = getDisp(rvar->getBaseRegVar());
                getSpillOffset(offset);
                // message expects offsets to be in HWord
                off = (offset + spillOff * builder_->getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;
                if (builder_->usesStack())
                    fp = builder_->kernel.fg.getFramePtrDcl();

                if (!fp && offset < SCRATCH_MSG_LIMIT)
                    headerOpnd = builder_->createSrcRegRegion(builder_->getBuiltinR0(), builder_->getRegionStride1());
            }
        }
        sendInst = builder_->createSpill(postDst, headerOpnd, srcOpnd, execSize, height, off, fp, InstOpt_WriteEnable, true);
        sendInst->inheritDIFrom(curInst);
    }
    else
    {
        G4_SrcRegRegion * payload = builder_->createSrc(
            mRangeDcl->getRegVar(), 0, 0, builder_->getRegionStride1(), Type_UD);
        sendInst = createSendInst(execSize, postDst, payload, messageDescImm, SFID::DP_DC0, true, InstOpt_WriteEnable);
    }

    return sendInst;
}

// Create the send instruction to perform the spill of the spilled region's
// segment into spill memory.
G4_INST *SpillManagerGRF::createSpillSendInstr(
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    G4_DstRegRegion * spilledRangeRegion,
    G4_ExecSize       execSize,
    unsigned          option)
{

    G4_DstRegRegion * postDst =
        builder_->createNullDst(execSize > g4::SIMD8 ? Type_UW : Type_UD);

    G4_INST* sendInst = NULL;
    if (useSplitSend())
    {
        unsigned extMsgLength = spillRangeDcl->getNumRows();
        const RegionDesc* region = builder_->getRegionStride1();
        auto headerOpnd = getSpillFillHeader(*builder_, mRangeDcl);
        G4_SrcRegRegion* srcOpnd = builder_->createSrcRegRegion(spillRangeDcl, region);

        auto off = G4_SpillIntrinsic::InvalidOffset;
        G4_Declare* fp = nullptr;
        auto spillExecSize = execSize;
        if (useScratchMsg_)
        {
            auto [messageDescImm, retSize] =
                createSpillSendMsgDesc(spilledRangeRegion, execSize);
            spillExecSize = retSize;
            off = (messageDescImm->getInt() & 0xfff);
        }
        else
        {
            if (builder_->usesStack())
            {
                G4_RegVar* r = spillRangeDcl->getRegVar();
                G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
                int offset = getDisp(rvar->getBaseRegVar());
                getSpillOffset(offset);
                // message expects offsets to be in HWord
                auto regOff = spilledRangeRegion->getRegOff();
                off = (offset + regOff * builder_->getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;
                if (builder_->usesStack())
                    fp = builder_->kernel.fg.getFramePtrDcl();

                if (!fp && offset < SCRATCH_MSG_LIMIT)
                    headerOpnd = builder_->createSrcRegRegion(builder_->getBuiltinR0(), builder_->getRegionStride1());
            }
        }
        sendInst = builder_->createSpill(postDst, headerOpnd, srcOpnd, spillExecSize, (uint16_t)extMsgLength,
            off, fp, static_cast<G4_InstOption>(option), true);
        sendInst->inheritDIFrom(curInst);
    }
    else
    {
        auto [messageDescImm, spillExecSize] =
            createSpillSendMsgDesc(spilledRangeRegion, execSize);
        G4_SrcRegRegion * payload = builder_->createSrc(
            mRangeDcl->getRegVar(), 0, 0, builder_->getRegionStride1(), Type_UD);
        sendInst = createSendInst(spillExecSize, postDst, payload, messageDescImm, SFID::DP_DC0, true, static_cast<G4_InstOption>(option));
    }

    return sendInst;
}

// Create the message description for a fill send instruction for filled
// regvars.
G4_Imm *SpillManagerGRF::createFillSendMsgDesc(
    unsigned          regOff,
    unsigned          height,
    G4_ExecSize &     execSize,
    G4_RegVar *       base)
{
    unsigned message = 0;

    if (useScratchMsg_)
    {
        unsigned segmentByteSize = height * builder_->numEltPerGRF<Type_UB>();
        unsigned responseLength = cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
        message = responseLength << getSendRspLengthBitOffset();
        unsigned headerPresent = 0x80000;
        message |= SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT << getSendMsgLengthBitOffset();
        message |= headerPresent;

        message |= (1 << SCRATCH_MSG_DESC_CATEORY);
        message |= (0 << SCRATCH_MSG_INVALIDATE_AFTER_READ);
        unsigned blocksize_encoding = getScratchBlocksizeEncoding(height, *builder_);

        message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);

        int offset = getDisp(base);
        getSpillOffset(offset);
        // message expects offsets to be in HWord
        message |= (offset + regOff * builder_->getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;

        execSize = g4::SIMD16;
    }
    else
    {
        unsigned segmentByteSize = height * builder_->numEltPerGRF<Type_UB>();
        unsigned statelessSurfaceIndex = 0xFF;
        unsigned responseLength = cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
        responseLength = responseLength << getSendRspLengthBitOffset();
        message = statelessSurfaceIndex | responseLength;

        unsigned headerPresent = 0x80000;
        message |= headerPresent;
        unsigned messageType = getSendOwordReadType();
        message |= messageType << getSendReadTypeBitOffset();
        unsigned messageLength = OWORD_PAYLOAD_HEADER_MIN_HEIGHT;
        message |= messageLength << getSendMsgLengthBitOffset();
        unsigned segmentOwordSize =
            cdiv(segmentByteSize, OWORD_BYTE_SIZE);
        message |= blockSendBlockSizeCode (segmentOwordSize);
        execSize = G4_ExecSize(LIMIT_SEND_EXEC_SIZE (segmentOwordSize * DWORD_BYTE_SIZE));
    }
    return builder_->createImm (message, Type_UD);
}

// Create the message description for a fill send instruction for filled
// source regions.
template <class REGION_TYPE>
G4_Imm *SpillManagerGRF::createFillSendMsgDesc(
    REGION_TYPE * filledRangeRegion,
    G4_ExecSize    execSize)
{
    unsigned message = 0;

    if (useScratchMsg_)
    {
        unsigned segmentByteSize =
            getSegmentByteSize(filledRangeRegion, execSize);
        if (filledRangeRegion->crossGRF(*builder_)) {
            segmentByteSize = 2 * builder_->numEltPerGRF<Type_UB>();
        }

        unsigned responseLength = cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
        message = responseLength << getSendRspLengthBitOffset();

        unsigned headerPresent = 0x80000;
        message |= headerPresent;

        message |= (SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT << getSendMsgLengthBitOffset());
        message |= (1 << SCRATCH_MSG_DESC_CATEORY);
        message |= (0 << SCRATCH_MSG_INVALIDATE_AFTER_READ);
        unsigned blocksize_encoding = getScratchBlocksizeEncoding(responseLength, *builder_);

        message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
        int offset = getRegionDisp(filledRangeRegion);
        getSpillOffset(offset);
        message |= offset >> SCRATCH_SPACE_ADDRESS_UNIT;
    }
    else
    {
        unsigned segmentByteSize =
            getSegmentByteSize(filledRangeRegion, execSize);
        unsigned statelessSurfaceIndex = 0xFF;
        unsigned responseLength = cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
        responseLength = responseLength << getSendRspLengthBitOffset();
        message = statelessSurfaceIndex | responseLength;

        unsigned headerPresent = 0x80000;
        message |= headerPresent;
        unsigned messageType = getSendOwordReadType();
        message |= messageType << getSendReadTypeBitOffset();
        unsigned messageLength = OWORD_PAYLOAD_HEADER_MIN_HEIGHT;
        message |= messageLength << getSendMsgLengthBitOffset();
        unsigned segmentOwordSize =
            cdiv(segmentByteSize, OWORD_BYTE_SIZE);
        message |= blockSendBlockSizeCode (segmentOwordSize);
    }
    return builder_->createImm(message, Type_UD);
}

// Create the send instruction to perform the fill of the spilled regvars's
// segment from spill memory.
// spillOff - spill offset to the fillRangeDcl, in unit of grf size
G4_INST * SpillManagerGRF::createFillSendInstr (
    G4_Declare *      fillRangeDcl,
    G4_Declare *      mRangeDcl,
    unsigned          regOff,
    unsigned          height,
    unsigned          spillOff)
{
    G4_ExecSize execSize {0};

    G4_Imm * messageDescImm = NULL;

    if (useScratchMsg_)
    {
        G4_RegVar* r = fillRangeDcl->getRegVar();
        G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
        messageDescImm =
            createFillSendMsgDesc (spillOff, height, execSize, rvar->getBaseRegVar());
#ifdef _DEBUG
        int offset = (messageDescImm->getInt() & 0xFFF) * builder_->numEltPerGRF<Type_UB>();
        MUST_BE_TRUE(offset >= globalScratchOffset, "incorrect offset");
#endif
    }
    else
    {
        messageDescImm =
            createFillSendMsgDesc (regOff, height, execSize);
    }

    G4_DstRegRegion * postDst = builder_->createDst(
        fillRangeDcl->getRegVar(), (short) regOff, SUBREG_ORIGIN,
        DEF_HORIZ_STRIDE, (execSize > 8)? Type_UW: Type_UD);

    auto payload = getSpillFillHeader(*builder_, mRangeDcl);

    unsigned int off = G4_FillIntrinsic::InvalidOffset;
    G4_Declare* fp = nullptr;
    if (useScratchMsg_)
        off = (messageDescImm->getInt() & 0xfff);
    else
    {
        if (builder_->usesStack())
        {
            // compute hword offset to emit later when expanding spill/fill intrinsic
            G4_RegVar* r = fillRangeDcl->getRegVar();
            G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
            int offset = getDisp(rvar->getBaseRegVar());
            getSpillOffset(offset);
            // message expects offsets to be in HWord
            off = (offset + spillOff * builder_->getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;
            if (builder_->usesStack())
                fp = builder_->kernel.fg.getFramePtrDcl();

            if (!fp && offset < SCRATCH_MSG_LIMIT)
                payload = builder_->createSrcRegRegion(builder_->getBuiltinR0(), builder_->getRegionStride1());
        }
    }
    auto fillInst = builder_->createFill(payload, postDst, execSize, height, off, fp, InstOpt_WriteEnable, true);
    fillInst->inheritDIFrom(curInst);
    return fillInst;

}

// Create the send instruction to perform the fill of the filled region's
// segment into fill memory.
G4_INST * SpillManagerGRF::createFillSendInstr(
    G4_Declare *      fillRangeDcl,
    G4_Declare *      mRangeDcl,
    G4_SrcRegRegion * filledRangeRegion,
    G4_ExecSize       execSize)
{
    auto oldExecSize = execSize;

    if (useScratchMsg_)
    {
        execSize = g4::SIMD16;
    }

    G4_DstRegRegion * postDst = builder_->createDst(
        fillRangeDcl->getRegVar(), 0, SUBREG_ORIGIN,
        DEF_HORIZ_STRIDE, (execSize > 8)? Type_UW : Type_UD);

    auto payload = getSpillFillHeader(*builder_, mRangeDcl);

    unsigned int off = G4_FillIntrinsic::InvalidOffset;
    unsigned segmentByteSize = getSegmentByteSize(filledRangeRegion, oldExecSize);
    G4_Declare* fp = nullptr;
    if (useScratchMsg_)
    {
        G4_Imm* messageDescImm =
            createFillSendMsgDesc(filledRangeRegion, oldExecSize);

        off = (messageDescImm->getInt() & 0xfff);
        if (filledRangeRegion->crossGRF(*builder_))
        {
            segmentByteSize = 2 * builder_->numEltPerGRF<Type_UB>();
        }
    }
    else
    {
        if (builder_->usesStack())
        {
            // compute hword offset to emit later when expanding spill/fill intrinsic
            int offset = getRegionDisp(filledRangeRegion);
            getSpillOffset(offset);
            off = offset >> SCRATCH_SPACE_ADDRESS_UNIT;
            if (builder_->usesStack())
                fp = builder_->kernel.fg.getFramePtrDcl();

            if (!fp && offset < SCRATCH_MSG_LIMIT)
                payload = builder_->createSrcRegRegion(builder_->getBuiltinR0(), builder_->getRegionStride1());
        }
    }

    unsigned responseLength = cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
    auto fillInst = builder_->createFill(payload, postDst, execSize, responseLength, off, fp, InstOpt_WriteEnable, true);
    fillInst->inheritDIFrom(curInst);
    return fillInst;
}

// LSC versions of spill/fill, useLSCMsg must be true for these functions

G4_SrcRegRegion *SpillManagerGRF::getLSCSpillFillHeader(
    G4_Declare* mRangeDcl, const G4_Declare *fp, int offset)
{
    G4_SrcRegRegion* headerOpnd = nullptr;
    if (!fp && offset < SCRATCH_MSG_LIMIT && !useLscNonstackCall) {
        // using LSC because we exceed 128k of DC0 message
        headerOpnd = builder_->createSrcRegRegion(builder_->getBuiltinR0(), builder_->getRegionStride1());
    }
    else
    {
        headerOpnd = getSpillFillHeader(*builder_, mRangeDcl);
    }
    return headerOpnd;
}

// Create the send instruction to perform the spill of the spilled regvars's
// segment into spill memory.
//
// regOff - Offset of sub-spill. If one spill is splitted into more than one spill,
// this is the offset of them, unit in register size
// spillOff - Offset of the original variable being spilled, unit in register size.
G4_INST * SpillManagerGRF::createLSCSpill(
    G4_Declare*       spillRangeDcl,
    G4_Declare*       mRangeDcl,
    unsigned          regOff,
    unsigned          height,
    unsigned          spillOff)
{
    G4_ExecSize execSize(16);

    G4_DstRegRegion* postDst = builder_->createNullDst(Type_UD);

    G4_SrcRegRegion* srcOpnd = createBlockSpillRangeSrcRegion(spillRangeDcl->getRegVar(), regOff);
    G4_Declare* fp = builder_->usesStack() ? builder_->kernel.fg.getFramePtrDcl() : nullptr;

    G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*>(spillRangeDcl->getRegVar());
    int offset = getDisp(rvar->getBaseRegVar());
    getSpillOffset(offset);
    // message expects offsets to be in HWord
    uint32_t offsetHwords = (offset + spillOff * builder_->getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;

    G4_SrcRegRegion* header = getLSCSpillFillHeader(mRangeDcl, fp, offset);
    auto sendInst = builder_->createSpill(postDst, header, srcOpnd, execSize,
        height, offsetHwords, fp, InstOpt_WriteEnable, true);
    sendInst->inheritDIFrom(curInst);

    return sendInst;
}

// Create the send instruction to perform the spill of the spilled region's
// segment into spill memory.
G4_INST * SpillManagerGRF::createLSCSpill(
    G4_Declare*       spillRangeDcl,
    G4_Declare*       mRangeDcl,
    G4_DstRegRegion*  spilledRangeRegion,
    G4_ExecSize       execSize,
    unsigned          option)
{
    G4_DstRegRegion* postDst = builder_->createNullDst(Type_UD);

    unsigned extMsgLength = spillRangeDcl->getNumRows();
    const RegionDesc* region = builder_->getRegionStride1();
    G4_SrcRegRegion* srcOpnd = builder_->createSrcRegRegion(spillRangeDcl, region);

    G4_Declare* fp = builder_->usesStack() ? builder_->kernel.fg.getFramePtrDcl() : nullptr;

    G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*>(spillRangeDcl->getRegVar());
    int offset = getDisp(rvar->getBaseRegVar());
    getSpillOffset(offset);
    // message expects offsets to be in HWord
    auto regOff = spilledRangeRegion->getRegOff();
    uint32_t offsetHwords = (offset + regOff * builder_->getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;

    G4_SrcRegRegion* header = getLSCSpillFillHeader(mRangeDcl, fp, offset);
    auto sendInst = builder_->createSpill(postDst, header, srcOpnd, execSize,
        (uint16_t)extMsgLength, offsetHwords, fp, static_cast<G4_InstOption>(option), true);
    sendInst->inheritDIFrom(curInst);

    return sendInst;
}

// Create the send instruction to perform the fill of the spilled regvars's
// segment from spill memory.
// spillOff - spill offset to the fillRangeDcl, in unit of grf size
G4_INST * SpillManagerGRF::createLSCFill(
    G4_Declare * fillRangeDcl,
    G4_Declare * mRangeDcl,
    unsigned     regOff,
    unsigned     height,
    unsigned     spillOff)
{
    G4_DstRegRegion* postDst = builder_->createDst(
        fillRangeDcl->getRegVar(), (short)regOff, SUBREG_ORIGIN,
        DEF_HORIZ_STRIDE, Type_UD);

    G4_Declare* fp = builder_->usesStack() ? builder_->kernel.fg.getFramePtrDcl() : nullptr;

    // compute hword offset to emit later when expanding spill/fill intrinsic
    G4_RegVar* r = fillRangeDcl->getRegVar();
    G4_RegVarTmp* rvar = static_cast<G4_RegVarTmp*> (r);
    int offset = getDisp(rvar->getBaseRegVar());
    getSpillOffset(offset);
    // fill intrinsic expects offsets to be in HWord
    uint32_t offsetHwords = (offset + spillOff * builder_->getGRFSize()) >> SCRATCH_SPACE_ADDRESS_UNIT;

    G4_SrcRegRegion* header = getLSCSpillFillHeader(mRangeDcl, fp, offset);
    auto fillInst = builder_->createFill(header, postDst, g4::SIMD16, height,
        offsetHwords, fp, InstOpt_WriteEnable, true);
    fillInst->inheritDIFrom(curInst);
    return fillInst;
}

// Create the send instruction to perform the fill of the filled region's
// segment into fill memory.
G4_INST * SpillManagerGRF::createLSCFill(
    G4_Declare *      fillRangeDcl,
    G4_Declare *      mRangeDcl,
    G4_SrcRegRegion * filledRangeRegion,
    G4_ExecSize       execSize)
{
    auto oldExecSize = execSize;

    G4_DstRegRegion* postDst = builder_->createDst(
        fillRangeDcl->getRegVar(), 0, SUBREG_ORIGIN,
        DEF_HORIZ_STRIDE, Type_UD);

    unsigned segmentByteSize = getSegmentByteSize(filledRangeRegion, oldExecSize);
    G4_Declare* fp = builder_->usesStack() ? builder_->kernel.fg.getFramePtrDcl() : nullptr;

    // compute hword offset to emit later when expanding spill/fill intrinsic
    int offset = getRegionDisp(filledRangeRegion);
    getSpillOffset(offset);
    uint32_t offsetHwords = offset >> SCRATCH_SPACE_ADDRESS_UNIT;

    unsigned responseLength = cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
    G4_SrcRegRegion* header = getLSCSpillFillHeader(mRangeDcl, fp, offset);
    auto fillInst = builder_->createFill(header, postDst, execSize,
        responseLength, offsetHwords, fp, InstOpt_WriteEnable, true);
    fillInst->inheritDIFrom(curInst);
    return fillInst;
}

// Replace the reference to the spilled region with a reference to an
// equivalent reference to the spill range region.
void SpillManagerGRF::replaceSpilledRange(
    G4_Declare* spillRangeDcl,
    G4_DstRegRegion* spilledRegion,
    G4_INST* spilledInst,
    uint32_t subRegOff)
{
    // we need to preserve accRegSel if it's set
    G4_DstRegRegion * tmpRangeDstRegion = builder_->createDst(
        spillRangeDcl->getRegVar(), REG_ORIGIN, subRegOff,
        spilledRegion->getHorzStride(), spilledRegion->getType(), spilledRegion->getAccRegSel());
    spilledInst->setDest (tmpRangeDstRegion);
}

// Replace the reference to the filled region with a reference to an
// equivalent reference to the fill range region.
void SpillManagerGRF::replaceFilledRange(
    G4_Declare *      fillRangeDcl,
    G4_SrcRegRegion * filledRegion,
    G4_INST *         filledInst)
{
    G4_ExecSize execSize =
        isMultiRegComprSource(filledRegion, filledInst) ?
            G4_ExecSize(filledInst->getExecSize() / 2):
            filledInst->getExecSize();

    for (int i = 0; i < G4_MAX_SRCS; i++) {
        G4_Operand * src = filledInst->getSrc(i);

        if (src && src->isSrcRegRegion())
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
void SpillManagerGRF::sendOutSpilledRegVarPortions (
    G4_Declare *      spillRangeDcl,
    G4_Declare *      mRangeDcl,
    unsigned          regOff,
    unsigned          height,
    unsigned          srcRegOff)
{
    if (!headerNeeded())
    {
        // No need to make a copy of offset because when using
        // scratch msg descriptor, the offset is part of send
        // msg descriptor and not the header.
    }
    else
    {
        // Initialize the message header with the spill disp for portion.
        int offset = getDisp(spillRangeDcl->getRegVar()) + regOff * builder_->numEltPerGRF<Type_UB>();
        getSpillOffset(offset);
        unsigned segmentDisp = offset / OWORD_BYTE_SIZE;

        G4_Imm * segmentDispImm = builder_->createImm (segmentDisp, Type_UD);
        G4_DstRegRegion * mHeaderOffsetDstRegion =
            createMHeaderBlockOffsetDstRegion(mRangeDcl->getRegVar());

        if (builder_->getIsKernel() == false)
        {
            createAddFPInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
        }
        else
        {
            createMovInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
        }
        numGRFMove ++;
    }


    // Write out the portions using a greedy approach.
    int currentStride = getNextSize(height, useScratchMsg_, *builder_);

    if (currentStride)
    {
        initMWritePayload(spillRangeDcl, mRangeDcl, regOff, currentStride);

        if (useLSCMsg)
        {
            createLSCSpill(spillRangeDcl, mRangeDcl, regOff, currentStride, srcRegOff);
        }
        else
        {
            createSpillSendInstr(spillRangeDcl, mRangeDcl, regOff, currentStride, srcRegOff);
        }

        if (height - currentStride > 0) {
            sendOutSpilledRegVarPortions(
                spillRangeDcl, mRangeDcl, regOff + currentStride, height - currentStride, srcRegOff + currentStride);
        }
    }
}

bool SpillManagerGRF::checkDefUseDomRel(G4_DstRegRegion* dst, G4_BB* defBB)
{
    if (!refs.isUniqueDef(dst))
        return false;

    auto dcl = dst->getTopDcl();

    // check whether this def dominates all its uses
    auto uses = refs.getUses(dcl);

    for (auto& use : *uses)
    {
        auto useBB = std::get<1>(use);

        // check if def dominates use
        if (!defBB->dominates(useBB))
            return false;

        if (defBB == useBB)
        {
            // defBB dominates useBB since its the same BB.
            // ensure def instruction appears lexically before use BB.
            auto useInst = std::get<0>(use);
            if (dst->getInst()->getLexicalId() > useInst->getLexicalId())
                return false;
        }
    }

    // if def is in loop then ensure all uses are in same loop level
    // or inner loop nest of def's closest loop.
    auto defLoop = gra.kernel.fg.getLoops().getInnerMostLoop(defBB);
    if (defLoop)
    {
        // since def is in loop, check whether uses are also in same loop.
        for (auto& use : *uses)
        {
            auto useBB = std::get<1>(use);
            auto useLoop = gra.kernel.fg.getLoops().getInnerMostLoop(useBB);
            if (!useLoop)
                return false;

            if (!useLoop->fullSubset(defLoop))
                return false;
        }
    }

    return true;
}

bool SpillManagerGRF::checkUniqueDefAligned(G4_DstRegRegion* dst, G4_BB* defBB)
{
    // return true if dst is unique definition considering alignment
    // for spill code.

    if (!refs.isUniqueDef(dst))
        return false;

    // dst dcl may have multiple defs. As long as each def defines
    // different part of the variable, each def is marked as unique.
    // However, spill/fill is done on GRF granularity. So although
    // defs are unique in following sequence, we still need RMW for
    // 2nd def:
    //
    // .decl V361 type=w size=32
    //
    // add (M1, 8) V361(0,0)<1>   V358(0,0)<1;1,0>   0x10:w
    // add (M1, 8) V361(0,8)<1>   V358(0,0)<1;1,0>   0x18:w
    //
    // Return false if any other dominating def exists that defines
    // part of same row of variable dst.
    auto dcl = dst->getTopDcl();

    if (dcl->getAddressed())
        return false;

    auto defs = refs.getDefs(dcl);
    unsigned int GRFSize = builder_->numEltPerGRF<Type_UB>();
    unsigned int lb = dst->getLeftBound();
    unsigned int rb = dst->getRightBound();
    unsigned int startRow = lb / GRFSize;
    unsigned int endRow = rb / GRFSize;

    for (auto& def : *defs)
    {
        // check whether dst and def write same row
        auto otherDefInst = std::get<0>(def);

        if (otherDefInst == dst->getInst())
            continue;

        auto otherDefDstRgn = otherDefInst->getDst();
        unsigned int otherLb = otherDefDstRgn->getLeftBound();
        unsigned int otherRb = otherDefDstRgn->getRightBound();
        unsigned int otherTypeSize = otherDefDstRgn->getTypeSize();
        bool commonRow = false;
        for (unsigned int i = otherLb; i <= otherRb; i += otherTypeSize)
        {
            auto rowWritten = i / GRFSize;
            if (rowWritten >= startRow && rowWritten <= endRow)
            {
                commonRow = true;
                break;
            }
        }

        // No common row between defs, so it is safe to skip fill
        // wrt current def. Check with next def.
        if (!commonRow)
            continue;

        auto otherDefBB = std::get<1>(def);

        if (!defBB->dominates(otherDefBB))
            return false;

        if (defBB == otherDefBB)
        {
            if (dst->getInst()->getLexicalId() > otherDefInst->getLexicalId())
                return false;
        }
    }

    return true;
}

// This function checks whether each spill dst region requires a read-modify-write operation
// when inserting spill code. Dominator/unique defs don't require redundant read operation.
// Dst regions that do not need RMW are added to a set. This functionality isnt needed for
// functional correctness. This function is executed before inserting spill code because
// we need all dst regions of dcl available to decide whether read is redundant. If this is
// executed when inserting spill then dst regions of dcl appearing earlier than current one
// would be translated to spill code already. Spill/fill code insertion replaces dst region
// of spills with new temp region. This makes it difficult to check whether current dst and
// an earlier spilled dst write to same GRF row.
void SpillManagerGRF::updateRMWNeeded()
{
    if (!gra.kernel.getOption(vISA_SkipRedundantFillInRMW))
        return;

    auto isRMWNeededForSpilledDst = [&](G4_BB* bb, G4_DstRegRegion* spilledRegion)
    {
        auto isUniqueDef = checkUniqueDefAligned(spilledRegion, bb);

        // Check0 : Def is NoMask, -- checked in isPartialWriteForSpill()
        // Check1 : Def is unique def,
        // Check2 : Def is in loop L and all use(s) of dcl are in loop L or it's inner loop nest,
        // Check3 : Flowgraph is reducible
        // RMW_Not_Needed = Check0 || (Check1 && Check2 && Check3)
        bool RMW_Needed = true;

        if (isUniqueDef && builder_->kernel.fg.isReducible() && checkDefUseDomRel(spilledRegion, bb))
        {
            RMW_Needed = false;
        }

        return RMW_Needed;
    };

    // First pass to setup lexical ids of instruction so dominator relation can be
    // computed correctly intra-BB.
    unsigned int lexId = 0;
    for (auto bb : gra.kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            inst->setLexicalId(lexId++);
        }
    }

    for (auto bb : gra.kernel.fg.getBBList())
    {
        for (auto inst : bb->getInstList())
        {
            if (inst->isPseudoKill())
                continue;

            auto dst = inst->getDst();
            if (dst)
            {
                if (dst->getBase()->isRegVar())
                {
                    auto dstRegVar = dst->getBase()->asRegVar();
                    if (dstRegVar && shouldSpillRegister(dstRegVar))
                    {
                        if (getRFType(dstRegVar) == G4_GRF)
                        {
                            auto RMW_Needed = isRMWNeededForSpilledDst(bb, dst);
                            if (!RMW_Needed)
                            {
                                // Any spilled dst region that doesnt need RMW
                                // is added to noRMWNeeded set. This set is later
                                // checked when inserting spill/fill code.
                                noRMWNeeded.insert(dst);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Create the code to create the spill range and save it to spill memory.
void SpillManagerGRF::insertSpillRangeCode(
    INST_LIST::iterator spilledInstIter, G4_BB* bb)
{
    G4_ExecSize execSize = (*spilledInstIter)->getExecSize();
    G4_Declare * replacementRangeDcl;
    builder_->instList.clear();

    bool optimizeSplitLLR = false;
    G4_INST* inst = *spilledInstIter;
    G4_INST* spillSendInst = NULL;
    auto spilledRegion = inst->getDst();

    auto spillDcl = spilledRegion->getTopDcl()->getRootDeclare();
    if (scalarImmSpill.find(spillDcl) != scalarImmSpill.end())
    {
        // do not spill scalar immediate values
        bb->erase(spilledInstIter);
        return;
    }

    auto checkRMWNeeded = [this, spilledRegion]()
    {
        return noRMWNeeded.find(spilledRegion) == noRMWNeeded.end();
    };

    //subreg offset for new dst that replaces the spilled dst
    auto newSubregOff = 0;

    if (inst->mayExceedTwoGRF())
    {
        // Handle send instructions (special treatment)
        // Create the spill range for the whole post destination, assign spill
        // offset to the spill range and create the instructions to load the
        // save the spill range to spill memory.
        INST_LIST::iterator sendOutIter = spilledInstIter;
        assert(getRFType (spilledRegion) == G4_GRF);
        G4_Declare * spillRangeDcl =
            createPostDstSpillRangeDeclare (*sendOutIter);
        G4_Declare * mRangeDcl =
            createAndInitMHeader (
                (G4_RegVarTransient *) spillRangeDcl->getRegVar());

        bool needRMW = inst->isPartialWriteForSpill(!bb->isAllLaneActive()) &&
            checkRMWNeeded();
        if (needRMW)
        {
            sendInSpilledRegVarPortions(
                spillRangeDcl, mRangeDcl, 0,
                spillRangeDcl->getNumRows(),
                spilledRegion->getRegOff());

            INST_LIST::iterator insertPos = sendOutIter;
            splice(bb, insertPos, builder_->instList, curInst->getCISAOff());
        }

        sendOutSpilledRegVarPortions(
            spillRangeDcl, mRangeDcl, 0, spillRangeDcl->getNumRows(),
            spilledRegion->getRegOff());

        replacementRangeDcl = spillRangeDcl;
    }
    else
    {
        // Handle other regular single/multi destination register instructions.
        // Create the spill range for the destination region, assign spill
        // offset to the spill range and create the instructions to load the
        // save the spill range to spill memory.

        // Create the segment aligned spill range
        G4_Declare * spillRangeDcl =
            createSpillRangeDeclare(spilledRegion, execSize, *spilledInstIter);

        // Create and initialize the message header
        G4_Declare * mRangeDcl =
            createAndInitMHeader(spilledRegion, execSize);

        // Unaligned region specific handling.
        unsigned int spillSendOption = InstOpt_WriteEnable;
        auto preloadNeeded = shouldPreloadSpillRange(*spilledInstIter, bb);
        if (preloadNeeded && checkRMWNeeded())
        {

            // Preload the segment aligned spill range from memory to use
            // as an overlay

            preloadSpillRange(
                spillRangeDcl, mRangeDcl, spilledRegion, execSize);

            // Create the temporary range to use as a replacement range.

            G4_Declare* tmpRangeDcl =
                createTemporaryRangeDeclare(spilledRegion, execSize);

            // Copy out the value in the temporary range into its
            // location in the spill range.

            G4_DstRegRegion* spillRangeDstRegion =
                createSpillRangeDstRegion(
                    spillRangeDcl->getRegVar(), spilledRegion, execSize);

            G4_SrcRegRegion* tmpRangeSrcRegion =
                createTemporaryRangeSrcRegion(
                    tmpRangeDcl->getRegVar(), spilledRegion, execSize);

            // NOTE: Never use a predicate for the final mov if the spilled
            //       instruction was a sel (even in a SIMD CF context).

            G4_Predicate* predicate =
                ((*spilledInstIter)->opcode() != G4_sel) ?
                (*spilledInstIter)->getPredicate() : nullptr;

            if (tmpRangeSrcRegion->getType() == spillRangeDstRegion->getType() && IS_TYPE_FLOAT_ALL(tmpRangeSrcRegion->getType()))
            {
                // use int copy when possible as floating-point copy moves may need further legalization
                auto equivIntTy = floatToSameWidthIntType(tmpRangeSrcRegion->getType());
                tmpRangeSrcRegion->setType(*builder_, equivIntTy);
                spillRangeDstRegion->setType(*builder_, equivIntTy);
            }

            createMovInst(
                execSize, spillRangeDstRegion, tmpRangeSrcRegion,
                builder_->duplicateOperand(predicate),
                (*spilledInstIter)->getMaskOption());
            numGRFMove++;

            replacementRangeDcl = tmpRangeDcl;
            // maintain the spilled dst's subreg to not break the regioning restriction
            newSubregOff = spilledRegion->getSubRegOff();
        }
        else
        {
            // We're here because:
            // 1. preloadNeeded = false AND checkRMWNeeded = true OR
            // 2. preloadNeeded = true AND checkRMWNeeded = false OR
            // 3. both are false
            //
            // Case (1) occurs when:
            // Def uses dword type and writes entire row. But def doesnt define
            // complete variable, ie it isnt a kill. For such cases, we need to
            // use def's EM on spill msg.
            //
            // Case (2) occurs when:
            // Def is partial but it is unique in the program. For such cases,
            // we should use WriteEnable msg.
            //
            // Case (3) occurs when:
            // Def uses dword type and write entire row. Def defines complete
            // variable. We can use either EM.

            // Aligned regions do not need a temporary range.
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

                if (gra.getSubRegAlign(srcDcl) == builder_->getGRFAlign() &&
                    lb %  builder_->numEltPerGRF<Type_UB>() == 0 &&
                    (rb + 1) % builder_->numEltPerGRF<Type_UB>() == 0 &&
                    (rb - lb + 1) == spillRangeDcl->getByteSize() &&
                    regVar &&
                    !shouldSpillRegister(regVar))
                {
                    optimizeSplitLLR = true;
                }
            }

            replacementRangeDcl = spillRangeDcl;
            // maintain the spilled dst's subreg since the spill is done on a per-GRF basis
            newSubregOff = spilledRegion->getSubRegOff();

            if (preloadNeeded &&
                isUnalignedRegion(spilledRegion, execSize))
            {
                // A dst region may be not need pre-fill, however, if it is unaligned,
                // we need to use non-zero sub-reg offset in newly created spill dcl.
                // This section of code computes sub-reg offset to use for such cases.
                // It is insufficient to simply use spilledRegion's sub-reg offset in
                // case the region dcl is an alias of another dcl. This typically happens
                // when 2 scalar dcls are merged by merge scalar pass, merged dcl is
                // spilled, and dominating def writes non-zeroth element.
                unsigned segmentDisp = getEncAlignedSegmentDisp(spilledRegion, execSize);
                unsigned regionDisp = getRegionDisp(spilledRegion);
                assert(regionDisp >= segmentDisp);
                unsigned short subRegOff = (regionDisp - segmentDisp) / spilledRegion->getElemSize();
                assert((regionDisp - segmentDisp) % spilledRegion->getElemSize() == 0);
                assert(subRegOff * spilledRegion->getElemSize() +
                    getRegionByteSize(spilledRegion, execSize) <=
                    2u * builder_->numEltPerGRF<Type_UB>());
                newSubregOff = subRegOff;
            }

            if (!bb->isAllLaneActive() &&
                !preloadNeeded)
            {
                spillSendOption = (*spilledInstIter)->getMaskOption();
            }
        }

        // Save the spill range to memory.

        initMWritePayload(
            spillRangeDcl, mRangeDcl, spilledRegion, execSize);


        if (useLSCMsg)
        {
            spillSendInst = createLSCSpill(
                spillRangeDcl, mRangeDcl, spilledRegion, execSize, spillSendOption);
        }
        else
        {
            spillSendInst = createSpillSendInstr(
                spillRangeDcl, mRangeDcl, spilledRegion, execSize, spillSendOption);
        }

        if (failSafeSpill_ && !avoidDstSrcOverlap_)
        {
            spillRegOffset_ = spillRegStart_;
        }
    }

    if (builder_->getOption(vISA_DoSplitOnSpill))
    {
        if (inst->isRawMov())
        {
            // check whether mov is copy in loop preheader or exit
            auto it = gra.splitResults.find(inst->getSrc(0)->getTopDcl());
            if (it != gra.splitResults.end())
            {
                if ((*it).second.origDcl == spillDcl)
                {
                    // srcRegion is a split var temp
                    // this is a copy in either preheader or loop exit.
                    // add it to list so we know it shouldnt be optimized
                    // by spill cleanup.
                    for (auto addedInst : builder_->instList)
                    {
                        (*it).second.insts[bb].insert(addedInst);
                    }
                }
            }
        }
    }

    // Replace the spilled range with the spill range and insert spill
    // instructions.

    INST_LIST::iterator insertPos = std::next(spilledInstIter);
    replaceSpilledRange (replacementRangeDcl, spilledRegion, *spilledInstIter, newSubregOff);

    splice(bb, insertPos, builder_->instList, curInst->getCISAOff());

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
        splice(bb, spilledInstIter, builder_->instList, curInst->getCISAOff());
    }
}

// Create the code to create the GRF fill range and load it to spill memory.
void SpillManagerGRF::insertFillGRFRangeCode(
    G4_SrcRegRegion *   filledRegion,
    INST_LIST::iterator filledInstIter,
    G4_BB* bb)
{
    G4_ExecSize execSize = (*filledInstIter)->getExecSize();

    // Create the fill range, assign spill offset to the fill range and
    // create the instructions to load the fill range from spill memory.

    G4_Declare * fillRangeDcl = nullptr;

    bool optimizeSplitLLR = false;
    G4_INST* inst = *filledInstIter;
    auto dstRegion = inst->getDst();
    G4_INST* fillSendInst = nullptr;
    auto spillDcl = filledRegion->getTopDcl()->getRootDeclare();

    auto sisIt = scalarImmSpill.find(spillDcl);
    if (sisIt != scalarImmSpill.end())
    {
        //re-materialize the scalar immediate value
        auto imm = sisIt->second;
        auto tempDcl = builder_->createTempVar(1, imm->getType(), spillDcl->getSubRegAlign());
        auto movInst = builder_->createMov(g4::SIMD1, builder_->createDstRegRegion(tempDcl, 1), imm, InstOpt_WriteEnable, false);
        bb->insertBefore(filledInstIter, movInst);
        assert(!filledRegion->isIndirect());
        auto newSrc = builder_->createSrc(tempDcl->getRegVar(), filledRegion->getRegOff(), filledRegion->getSubRegOff(), filledRegion->getRegion(),
            filledRegion->getType(), filledRegion->getAccRegSel());
        int i = 0;
        for (; i < inst->getNumSrc(); ++i)
        {
            if (inst->getSrc(i) == filledRegion)
            {
                break;
            }
        }
        inst->setSrc(newSrc, i);
        return;
    }

    {
        fillRangeDcl =
            createGRFFillRangeDeclare(filledRegion, execSize, *filledInstIter);
        G4_Declare * mRangeDcl =
            createAndInitMHeader(filledRegion, execSize);

        if (useLSCMsg)
        {
            fillSendInst = createLSCFill(fillRangeDcl, mRangeDcl, filledRegion, execSize);
        }
        else
        {
            fillSendInst = createFillSendInstr(fillRangeDcl, mRangeDcl, filledRegion, execSize);
        }

        LocalLiveRange* filledLLR = gra.getLocalLR(filledRegion->getBase()->asRegVar()->getDeclare());
        if (filledLLR && filledLLR->getSplit())
        {
            G4_Declare* dstDcl = dstRegion->getBase()->asRegVar()->getDeclare();
            unsigned int lb = dstRegion->getLeftBound();
            unsigned int rb = dstRegion->getRightBound();

            if (gra.getSubRegAlign(dstDcl) == builder_->getGRFAlign() &&
                lb %  builder_->numEltPerGRF<Type_UB>() == 0 &&
                (rb + 1) % builder_->numEltPerGRF<Type_UB>() == 0 &&
                (rb - lb + 1) == fillRangeDcl->getByteSize())
            {
                optimizeSplitLLR = true;
            }
        }
    }

    if (builder_->getOption(vISA_DoSplitOnSpill))
    {
        if (inst->isRawMov())
        {
            // check whether mov is copy in loop preheader or exit
            auto it = gra.splitResults.find(dstRegion->getTopDcl());
            if (it != gra.splitResults.end())
            {
                if ((*it).second.origDcl == filledRegion->getTopDcl())
                {
                    // dstRegion is a split var temp
                    // this is a copy in either preheader or loop exit.
                    // add it to list so we know it shouldnt be optimized
                    // by spill cleanup.
                    for (auto addedInst : builder_->instList)
                    {
                        (*it).second.insts[bb].insert(addedInst);
                    }
                }
            }
        }
    }

    // Replace the spilled range with the fill range and insert spill
    // instructions.
    replaceFilledRange (fillRangeDcl, filledRegion, *filledInstIter);
    INST_LIST::iterator insertPos = filledInstIter;

    splice(bb, insertPos, builder_->instList, curInst->getCISAOff());
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
    }
}

// Create the code to create the GRF fill range and load it to spill memory.
INST_LIST::iterator SpillManagerGRF::insertSendFillRangeCode(
    G4_SrcRegRegion *   filledRegion,
    INST_LIST::iterator filledInstIter,
    G4_BB *             bb)
{
    G4_INST * sendInst = *filledInstIter;

    unsigned width = builder_->numEltPerGRF<Type_UB>() / filledRegion->getElemSize();

    // Create the fill range, assign spill offset to the fill range

    G4_Declare * fillGRFRangeDcl =
        createSendFillRangeDeclare(filledRegion, sendInst);

    // Create the instructions to load the fill range from spill memory.

    G4_Declare * mRangeDcl = createMRangeDeclare(filledRegion, G4_ExecSize(width));
    initMHeader(mRangeDcl);
    sendInSpilledRegVarPortions(
        fillGRFRangeDcl, mRangeDcl, 0,
        fillGRFRangeDcl->getNumRows(), filledRegion->getRegOff());

    // Replace the spilled range with the fill range and insert spill
    // instructions.

    replaceFilledRange(fillGRFRangeDcl, filledRegion, *filledInstIter);
    INST_LIST::iterator insertPos = filledInstIter;

    splice(bb, insertPos, builder_->instList, curInst->getCISAOff());

    // Return the next instruction

    return ++filledInstIter;
}

G4_Declare* SpillManagerGRF::getOrCreateAddrSpillFillDcl(G4_RegVar* addrDcl, G4_Declare* spilledAddrTakenDcl, G4_Kernel* kernel)
{
    auto pointsToSet = gra.pointsToAnalysis.getAllInPointsToAddrExps(addrDcl);
    G4_Declare* temp = nullptr;
    bool created = false;

    //Create a spill/fill declare for the AddrExps which are assigned to same address varaible and with same addressed varaible.
    //Note that we are trying to capture senorious (A1, 1:&V1), (A2, 2::&V1), where "1:" and "2:" means different AddrExp. IGC is trying to use different address registers.
    //Scenarios   (A1, 1:&V1), (A1, 2:&V1) may happen, by should be rare. In this case, only one declare will be created.
    for (auto pt : *pointsToSet)
    {
        G4_AddrExp* addrExp = pt.exp;
        G4_Declare* dcl = addrExp->getRegVar()->getDeclare();
        while (dcl->getAliasDeclare())
        {
            dcl = dcl->getAliasDeclare();
        }

        //The variable V is spilled
        if (dcl == spilledAddrTakenDcl)
        {
            G4_AddrExp *currentAddrExp = addrExp->getAddrTakenSpillFill();

            //Either all are created, or none.
            if (created)
            {
                //Either all addr expressoins are null, or all are not null.
                assert(currentAddrExp == nullptr);
            }

            if (currentAddrExp != nullptr)
            {
                return currentAddrExp->getRegVar()->getDeclare();;
            }

            //Create the spill fill variable
            if (!created)
            {
                // If spilledAddrTakenDcl already has a spill/fill range created, return it.
                // Else create new one and return it.
#define ADDR_SPILL_FILL_NAME_SIZE 32
                const char* dclName = kernel->fg.builder->getNameString(kernel->fg.mem, ADDR_SPILL_FILL_NAME_SIZE,
                    "ADDR_SP_FL_V%d_%d", spilledAddrTakenDcl->getDeclId(), getAddrSpillFillIndex(spilledAddrTakenDcl->getRegVar()));

                // temp is created of sub-class G4_RegVarTmp so that is
                // assigned infinite spill cost when coloring.
                temp = kernel->fg.builder->createDeclareNoLookup(dclName,
                    G4_GRF, spilledAddrTakenDcl->getNumElems(),
                    spilledAddrTakenDcl->getNumRows(), spilledAddrTakenDcl->getElemType(), DeclareType::Tmp, spilledAddrTakenDcl->getRegVar());
                temp->setAddrSpillFill();
                G4_AddrExp* newAddExp = builder_->createAddrExp(temp->getRegVar(), addrExp->getOffset(), addrExp->getType());
                addrExp->setAddrTakenSpillFill(newAddExp);
                gra.pointsToAnalysis.patchPointsToSet(addrDcl, newAddExp, newAddExp->getOffset());
                created = true;
            }
            else
            {
                G4_AddrExp* newAddExp = builder_->createAddrExp(temp->getRegVar(), addrExp->getOffset(), addrExp->getType());
                gra.pointsToAnalysis.patchPointsToSet(addrDcl, newAddExp, newAddExp->getOffset());
                addrExp->setAddrTakenSpillFill(newAddExp);
            }
        }
    }

    return temp;
}

// For each address taken register spill find an available physical register
// and assign it to the decl. This physical register will be used for inserting
// spill/fill code for indirect reference instructions that point to the
// spilled range.
// Return true if enough registers found, false if sufficient registers unavailable.
bool SpillManagerGRF::handleAddrTakenSpills(
    G4_Kernel * kernel, PointsToAnalysis& pointsToAnalysis)
{
    bool success = true;
    unsigned int numAddrTakenSpills = 0;

    for (const LiveRange* lr : *spilledLRs_)
    {
        if (lvInfo_->isAddressSensitive(lr->getVar()->getId()))
        {
            numAddrTakenSpills++;
        }
    }

    if (numAddrTakenSpills > 0)
    {
        insertAddrTakenSpillFill(kernel, pointsToAnalysis);
        prunePointsTo(kernel, pointsToAnalysis);
    }

    return success;
}

unsigned int SpillManagerGRF::handleAddrTakenLSSpills(
    G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis)
{
    unsigned int numAddrTakenSpills = 0;

    for (LSLiveRange* lr : *spilledLSLRs_)
    {
        if (lvInfo_->isAddressSensitive(lr->getTopDcl()->getRegVar()->getId()))
        {
            numAddrTakenSpills++;
        }
    }

    if (numAddrTakenSpills > 0)
    {
        insertAddrTakenLSSpillFill(kernel, pointsToAnalysis);
        prunePointsToLS(kernel, pointsToAnalysis);
    }

    return numAddrTakenSpills;
}

// Insert spill and fill code for indirect GRF accesses
void SpillManagerGRF::insertAddrTakenSpillAndFillCode(
    G4_Kernel* kernel, G4_BB* bb,
    INST_LIST::iterator inst_it, G4_Operand* opnd,
    PointsToAnalysis& pointsToAnalysis, bool spill, unsigned int bbid)
{
    curInst = (*inst_it);
    INST_LIST::iterator next_inst_it = ++inst_it;
    inst_it--;

    // Check whether spill operand points to any spilled range
    for (const LiveRange* lr : *spilledLRs_) {
        G4_RegVar* var = nullptr;

        if (opnd->isDstRegRegion() && opnd->asDstRegRegion()->getBase()->asRegVar())
            var = opnd->asDstRegRegion()->getBase()->asRegVar();

        if (opnd->isSrcRegRegion() && opnd->asSrcRegRegion()->getBase()->asRegVar())
            var = opnd->asSrcRegRegion()->getBase()->asRegVar();

        MUST_BE_TRUE(var != NULL, "Fill operand is neither a source nor dst region");

        if (var &&
            pointsToAnalysis.isPresentInPointsTo(var,
            lr->getVar()))
        {
            unsigned int numrows = lr->getDcl()->getNumRows();
            G4_Declare* temp = getOrCreateAddrSpillFillDcl(var, lr->getDcl(), kernel);

            if (failSafeSpill_ &&
                temp->getRegVar()->getPhyReg() == NULL)
            {
                temp->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegOffset_), 0);
                spillRegOffset_ += numrows;
            }

            if (numrows > 1 || (lr->getDcl()->getNumElems() * lr->getDcl()->getElemSize() == builder_->getGRFSize()))
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

                    splice(bb, inst_it, builder_->instList, curInst->getCISAOff());

                    if (spill)
                    {
                        sendOutSpilledRegVarPortions (
                            temp, mRangeDcl, 0, temp->getNumRows(),
                            0);

                        splice(bb, next_inst_it, builder_->instList, curInst->getCISAOff());
                    }
                }
                else
                {

                    for (unsigned int i = 0; i < numrows; i++)
                    {
                        G4_INST* inst;
                        const RegionDesc* rd = kernel->fg.builder->getRegionStride1();
                        G4_ExecSize curExSize {builder_->numEltPerGRF<Type_UD>()};

                        if ((i + 1) < numrows)
                            curExSize = G4_ExecSize(builder_->numEltPerGRF<Type_UD>()*2);

                        G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrc(lr->getVar(), (short)i, 0, rd, Type_F);

                        G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(temp->getRegVar(), (short)i, 0, 1, Type_F);

                        inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                        bb->insertBefore(inst_it, inst);

                        if (spill)
                        {
                            // Also insert spill code
                            G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrc(temp->getRegVar(), (short)i, 0, rd, Type_F);

                            G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(lr->getVar(), (short)i, 0, 1, Type_F);

                            inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                            bb->insertBefore(next_inst_it, inst);
                        }

                        // If 2 rows were processed then increment induction var suitably
                        if (   curExSize == 16)
                            i++;
                    }
                }

                // Update points to
                // Note: points2 set should be updated after inserting fill code,
                // however, this sets a bit in liveness bit-vector that
                // causes the temp variable to be marked as live-out from
                // that BB. A general fix should treat address taken variables
                // more accurately wrt liveness so they don't escape via
                // unfeasible paths.
                //pointsToAnalysis.addFillToPointsTo(bbid, var, temp->getRegVar());
            }
            else if (numrows == 1)
            {
                // Insert spill/fill when there decl uses a single row, that too not completely
                G4_ExecSize curExSize = g4::SIMD16;
                unsigned short numbytes = lr->getDcl()->getNumElems() * lr->getDcl()->getElemSize();

                //temp->setAddressed();
                short off = 0;

                while (numbytes > 0)
                {
                    G4_INST* inst;
                    G4_Type type = Type_W;

                    if (numbytes >= 16)
                        curExSize = g4::SIMD8;
                    else if (numbytes >= 8 && numbytes < 16)
                        curExSize = g4::SIMD4;
                    else if (numbytes >= 4 && numbytes < 8)
                        curExSize = g4::SIMD2;
                    else if (numbytes >= 2 && numbytes < 4)
                        curExSize = g4::SIMD1;
                    else if (numbytes == 1)
                    {
                        // If a region has odd number of bytes, copy last byte in final iteration
                        curExSize = g4::SIMD1;
                        type = Type_UB;
                    }
                    else {
                        MUST_BE_TRUE(false, "Cannot emit SIMD1 for byte");
                        curExSize = G4_ExecSize(0);
                    }

                    const RegionDesc* rd = kernel->fg.builder->getRegionStride1();

                    G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrc(lr->getVar(), 0, off, rd, type);

                    G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(temp->getRegVar(), 0, off, 1, type);

                    inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                    bb->insertBefore(inst_it, inst);

                    if (spill)
                    {
                        // Also insert spill code
                        G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrc(temp->getRegVar(), 0, off, rd, type);

                        G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(lr->getVar(), 0, off, 1, type);

                        inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                        bb->insertBefore(next_inst_it, inst);
                    }

                    off += curExSize;
                    numbytes -= curExSize*2;
                }

                // Update points to
                //pointsToAnalysis.addFillToPointsTo(bbid, var, temp->getRegVar());
            }

            if (!spill)
            {
                // Insert pseudo_use node so that liveness keeps the
                // filled variable live through the indirect access.
                // Not required for spill because for spill we will
                // anyway insert a ues of the variable to emit store.
                const RegionDesc* rd = kernel->fg.builder->getRegionScalar();

                G4_SrcRegRegion* pseudoUseSrc =
                    kernel->fg.builder->createSrc(temp->getRegVar(), 0, 0, rd, Type_F);

                G4_INST* pseudoUseInst = kernel->fg.builder->createInternalIntrinsicInst(
                    nullptr, Intrinsic::Use, g4::SIMD1,
                    nullptr, pseudoUseSrc, nullptr, nullptr, InstOpt_NoOpt);

                bb->insertBefore(next_inst_it, pseudoUseInst);
            }

        }
    }
}

// Insert spill and fill code for indirect GRF accesses
void SpillManagerGRF::insertAddrTakenLSSpillAndFillCode(
    G4_Kernel* kernel, G4_BB* bb,
    INST_LIST::iterator inst_it, G4_Operand* opnd,
    PointsToAnalysis& pointsToAnalysis, bool spill, unsigned int bbid)
{
    curInst = (*inst_it);
    INST_LIST::iterator next_inst_it = ++inst_it;
    inst_it--;

    // Check whether spill operand points to any spilled range
    for (LSLiveRange* lr : *spilledLSLRs_)
    {
        G4_RegVar* var = nullptr;

        if (opnd->isDstRegRegion() && opnd->asDstRegRegion()->getBase()->asRegVar())
            var = opnd->asDstRegRegion()->getBase()->asRegVar();

        if (opnd->isSrcRegRegion() && opnd->asSrcRegRegion()->getBase()->asRegVar())
            var = opnd->asSrcRegRegion()->getBase()->asRegVar();

        MUST_BE_TRUE(var != NULL, "Fill operand is neither a source nor dst region");

        if (var &&
            pointsToAnalysis.isPresentInPointsTo(var,
                lr->getTopDcl()->getRegVar()))
        {
            unsigned int numrows = lr->getTopDcl()->getNumRows();
            G4_Declare* temp = getOrCreateAddrSpillFillDcl(var, lr->getTopDcl(), kernel);
            MUST_BE_TRUE(temp != nullptr, "Failed to get the fill variable for indirect GRF access");
            if (failSafeSpill_ &&
                temp->getRegVar()->getPhyReg() == NULL)
            {
                temp->getRegVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegOffset_), 0);
                spillRegOffset_ += numrows;
            }

            if (!lr->isActiveLR())
            {
                lr->setActiveLR(true);
                updateActiveList(lr, &activeLR_);
            }

            if (numrows > 1 || (lr->getTopDcl()->getNumElems() * lr->getTopDcl()->getElemSize() == builder_->getGRFSize()))
            {
                if (useScratchMsg_ || useSplitSend())
                {
                    G4_Declare* fillGRFRangeDcl = temp;
                    G4_Declare* mRangeDcl =
                        createAndInitMHeader(
                            (G4_RegVarTransient*)temp->getRegVar()->getBaseRegVar());

                    sendInSpilledRegVarPortions(
                        fillGRFRangeDcl, mRangeDcl, 0,
                        temp->getNumRows(), 0);

                    splice(bb, inst_it, builder_->instList, curInst->getCISAOff());

                    if (spill)
                    {
                        sendOutSpilledRegVarPortions(
                            temp, mRangeDcl, 0, temp->getNumRows(),
                            0);

                        splice(bb, next_inst_it, builder_->instList, curInst->getCISAOff());
                    }
                }
                else
                {

                    for (unsigned int i = 0; i < numrows; i++)
                    {
                        G4_INST* inst;
                        const RegionDesc* rd = kernel->fg.builder->getRegionStride1();
                        G4_ExecSize curExSize{ builder_->numEltPerGRF<Type_UD>() };

                        if ((i + 1) < numrows)
                            curExSize = G4_ExecSize(builder_->numEltPerGRF<Type_UD>() * 2);

                        G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrc(lr->getTopDcl()->getRegVar(), (short)i, 0, rd, Type_F);

                        G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(temp->getRegVar(), (short)i, 0, 1, Type_F);

                        inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                        bb->insertBefore(inst_it, inst);

                        if (spill)
                        {
                            // Also insert spill code
                            G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrc(temp->getRegVar(), (short)i, 0, rd, Type_F);

                            G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(lr->getTopDcl()->getRegVar(), (short)i, 0, 1, Type_F);

                            inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                            bb->insertBefore(next_inst_it, inst);
                        }

                        // If 2 rows were processed then increment induction var suitably
                        if (curExSize == 16)
                            i++;
                    }
                }

                // Update points to
                // Note: points2 set should be updated after inserting fill code,
                // however, this sets a bit in liveness bit-vector that
                // causes the temp variable to be marked as live-out from
                // that BB. A general fix should treat address taken variables
                // more accurately wrt liveness so they don't escape via
                // unfeasible paths.
                //pointsToAnalysis.addFillToPointsTo(bbid, var, temp->getRegVar());
            }
            else if (numrows == 1)
            {
                // Insert spill/fill when there decl uses a single row, that too not completely
                G4_ExecSize curExSize = g4::SIMD16;
                unsigned short numbytes = lr->getTopDcl()->getNumElems() * lr->getTopDcl()->getElemSize();

                //temp->setAddressed();
                short off = 0;

                while (numbytes > 0)
                {
                    G4_INST* inst;
                    G4_Type type = Type_W;

                    if (numbytes >= 16)
                        curExSize = g4::SIMD8;
                    else if (numbytes >= 8 && numbytes < 16)
                        curExSize = g4::SIMD4;
                    else if (numbytes >= 4 && numbytes < 8)
                        curExSize = g4::SIMD2;
                    else if (numbytes >= 2 && numbytes < 4)
                        curExSize = g4::SIMD1;
                    else if (numbytes == 1)
                    {
                        // If a region has odd number of bytes, copy last byte in final iteration
                        curExSize = g4::SIMD1;
                        type = Type_UB;
                    }
                    else {
                        MUST_BE_TRUE(false, "Cannot emit SIMD1 for byte");
                        curExSize = G4_ExecSize(0);
                    }

                    const RegionDesc* rd = kernel->fg.builder->getRegionStride1();

                    G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrc(lr->getTopDcl()->getRegVar(), 0, off, rd, type);

                    G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(temp->getRegVar(), 0, off, 1, type);

                    inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                    bb->insertBefore(inst_it, inst);

                    if (spill)
                    {
                        // Also insert spill code
                        G4_SrcRegRegion* srcRex = kernel->fg.builder->createSrc(temp->getRegVar(), 0, off, rd, type);

                        G4_DstRegRegion* dstRex = kernel->fg.builder->createDst(lr->getTopDcl()->getRegVar(), 0, off, 1, type);

                        inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex, InstOpt_WriteEnable, false);

                        bb->insertBefore(next_inst_it, inst);
                    }

                    off += curExSize;
                    numbytes -= curExSize * 2;
                }

                // Update points to
                //pointsToAnalysis.addFillToPointsTo(bbid, var, temp->getRegVar());
            }

            if (!spill)
            {
                // Insert pseudo_use node so that liveness keeps the
                // filled variable live through the indirect access.
                // Not required for spill because for spill we will
                // anyway insert a ues of the variable to emit store.
                const RegionDesc* rd = kernel->fg.builder->getRegionScalar();

                G4_SrcRegRegion* pseudoUseSrc =
                    kernel->fg.builder->createSrc(temp->getRegVar(), 0, 0, rd, Type_F);

                G4_INST* pseudoUseInst = kernel->fg.builder->createInternalIntrinsicInst(
                    nullptr, Intrinsic::Use, g4::SIMD1,
                    nullptr, pseudoUseSrc, nullptr, nullptr, InstOpt_NoOpt);

                bb->insertBefore(next_inst_it, pseudoUseInst);
            }

        }
    }
}

// Insert any spill/fills for address taken
void SpillManagerGRF::insertAddrTakenSpillFill(
    G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis)
{
    for (auto bb : kernel->fg)
    {
        for (INST_LIST_ITER inst_it = bb->begin();
            inst_it != bb->end();
            inst_it++)
        {
            G4_INST* curInst = (*inst_it);

            if (failSafeSpill_)
            {
                spillRegOffset_ = indrSpillRegStart_;
            }

            // Handle indirect destination
            G4_DstRegRegion* dst = curInst->getDst();

            if (dst && dst->getRegAccess() == IndirGRF)
            {
                insertAddrTakenSpillAndFillCode(kernel, bb, inst_it, dst, pointsToAnalysis, true, bb->getId());
            }

            for (int i = 0; i < G4_MAX_SRCS; i++)
            {
                G4_Operand* src = curInst->getSrc(i);

                if (src && src->isSrcRegRegion() && src->asSrcRegRegion()->getRegAccess() == IndirGRF)
                {
                    insertAddrTakenSpillAndFillCode(kernel, bb, inst_it, src, pointsToAnalysis, false, bb->getId());
                }
            }
        }
    }
}

void SpillManagerGRF::insertAddrTakenLSSpillFill(
    G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis)
{
    for (auto bb : kernel->fg)
    {
        for (INST_LIST_ITER inst_it = bb->begin();
            inst_it != bb->end();
            inst_it++)
        {
            G4_INST* curInst = (*inst_it);

            unsigned int instID = curInst->getLexicalId();
            if (instID != (unsigned int)-1)
            {
                expireRanges(instID * 2, &activeLR_);
            }

            if (failSafeSpill_)
            {
                spillRegOffset_ = indrSpillRegStart_;
            }

            // Handle indirect destination
            G4_DstRegRegion* dst = curInst->getDst();

            if (dst && dst->getRegAccess() == IndirGRF)
            {
                insertAddrTakenLSSpillAndFillCode(kernel, bb, inst_it, dst, pointsToAnalysis, true, bb->getId());
            }

            for (int i = 0; i < G4_MAX_SRCS; i++)
            {
                G4_Operand* src = curInst->getSrc(i);

                if (src && src->isSrcRegRegion() && src->asSrcRegRegion()->getRegAccess() == IndirGRF)
                {
                    insertAddrTakenLSSpillAndFillCode(kernel, bb, inst_it, src, pointsToAnalysis, false, bb->getId());
                }
            }
        }
    }

    if (activeLR_.size() > 0)
    {
        // Expire any remaining ranges
        LSLiveRange* lastActive = activeLR_.back();
        unsigned int endIdx;

        lastActive->getLastRef(endIdx);

        expireRanges(endIdx, &activeLR_);
    }

}

// For address spill/fill code inserted remove from points of each indirect operand
// the original regvar that is spilled.
void SpillManagerGRF::prunePointsTo(
    G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis)
{
    for (auto bb : kernel->fg)
    {
        for (INST_LIST_ITER inst_it = bb->begin();
            inst_it != bb->end();
            inst_it++)
        {
            G4_INST* curInst = (*inst_it);
            std::stack<G4_Operand*> st;

            // Handle indirect destination
            G4_DstRegRegion* dst = curInst->getDst();

            if (dst && dst->getRegAccess() == IndirGRF)
            {
                st.push(dst);
            }

            for (int i = 0; i < G4_MAX_SRCS; i++)
            {
                G4_Operand* src = curInst->getSrc(i);

                if (src && src->isSrcRegRegion() && src->asSrcRegRegion()->getRegAccess() == IndirGRF)
                {
                    st.push(src);
                }

                //Replace the variable in the address expression with fill variable
                if (src && src->isAddrExp())
                {
                    G4_AddrExp* addExp = src->asAddrExp()->getAddrTakenSpillFill();

                    if (addExp != nullptr)
                    {
                        curInst->setSrc(addExp, i);
                    }
                }
            }

            while (st.size() > 0)
            {
                G4_Operand* cur = st.top();
                st.pop();

                // Check whether spill operand points to any spilled range
                for (const LiveRange* lr : *spilledLRs_) {
                    G4_RegVar* var = nullptr;

                    if (cur->isDstRegRegion() && cur->asDstRegRegion()->getBase()->asRegVar())
                        var = cur->asDstRegRegion()->getBase()->asRegVar();

                    if (cur->isSrcRegRegion() && cur->asSrcRegRegion()->getBase()->asRegVar())
                        var = cur->asSrcRegRegion()->getBase()->asRegVar();

                    MUST_BE_TRUE(var != nullptr, "Operand is neither a source nor dst region");

                    if (var &&
                        pointsToAnalysis.isPresentInPointsTo(var,
                        lr->getVar()))
                    {
                        // Remove this from points to
                        pointsToAnalysis.removeFromPointsTo(var, lr->getVar());
                    }
                }
            }
        }
    }
}

void SpillManagerGRF::prunePointsToLS(
    G4_Kernel* kernel, PointsToAnalysis& pointsToAnalysis)
{
    for (auto bb : kernel->fg)
    {
        for (INST_LIST_ITER inst_it = bb->begin();
            inst_it != bb->end();
            inst_it++)
        {
            G4_INST* curInst = (*inst_it);
            std::stack<G4_Operand*> st;

            // Handle indirect destination
            G4_DstRegRegion* dst = curInst->getDst();

            if (dst && dst->getRegAccess() == IndirGRF)
            {
                st.push(dst);
            }

            for (int i = 0; i < G4_MAX_SRCS; i++)
            {
                G4_Operand* src = curInst->getSrc(i);

                if (src && src->isSrcRegRegion() && src->asSrcRegRegion()->getRegAccess() == IndirGRF)
                {
                    st.push(src);
                }

                //Replace the variable in the address expression with fill variable
                if (src && src->isAddrExp())
                {
                    G4_AddrExp* addExp = src->asAddrExp()->getAddrTakenSpillFill();

                    if (addExp != nullptr)
                    {
                        curInst->setSrc(addExp, i);
                    }
                }
            }

            while (st.size() > 0)
            {
                G4_Operand* cur = st.top();
                st.pop();

                // Check whether spill operand points to any spilled range
                for (LSLiveRange* lr : *spilledLSLRs_)
                {
                    G4_RegVar* var = nullptr;

                    if (cur->isDstRegRegion() && cur->asDstRegRegion()->getBase()->asRegVar())
                        var = cur->asDstRegRegion()->getBase()->asRegVar();

                    if (cur->isSrcRegRegion() && cur->asSrcRegRegion()->getBase()->asRegVar())
                        var = cur->asSrcRegRegion()->getBase()->asRegVar();

                    MUST_BE_TRUE(var != NULL, "Operand is neither a source nor dst region");

                    if (var &&
                        pointsToAnalysis.isPresentInPointsTo(var,
                            lr->getTopDcl()->getRegVar()))
                    {
                        // Remove this from points to
                        pointsToAnalysis.removeFromPointsTo(var, lr->getTopDcl()->getRegVar());
                    }
                }
            }
        }
    }
}

void SpillManagerGRF::runSpillAnalysis()
{
    if (failSafeSpill_)
    {
        // ToDo: use the reserved GRFs to perform scalar immediate rematerialization
        return;
    }

    std::unordered_set<G4_Declare*> spilledDcl;
    scalarImmSpill.clear();

    for (auto bb : gra.kernel.fg)
    {
        for (auto inst : *bb)
        {
            auto dst = inst->getDst();
            auto dcl = dst && dst->getTopDcl() ? dst->getTopDcl()->getRootDeclare() : nullptr;
            if (!dcl || dcl->getAddressed() || dcl->getNumElems() != 1 || !shouldSpillRegister(dcl->getRegVar()))
            {
                // declare must be a scalar without address taken
                continue;
            }
            if (spilledDcl.count(dcl))
            {
                // this spilled declare is defined more than once
                scalarImmSpill.erase(dcl);
                continue;
            }
            spilledDcl.insert(dcl);
            if (inst->opcode() == G4_mov && inst->getExecSize() == g4::SIMD1 && inst->getSrc(0)->isImm() && !inst->getPredicate() && !inst->getSaturate())
            {
                scalarImmSpill[dcl] = inst->getSrc(0)->asImm();
            }
        }
    }
}

// Insert spill/fill code for all registers that have not been assigned
// physical registers in the current iteration of the graph coloring
// allocator.
// returns false if spill fails somehow
bool SpillManagerGRF::insertSpillFillCode(
    G4_Kernel * kernel, PointsToAnalysis& pointsToAnalysis)
{
    //runSpillAnalysis();
    // Set the spill flag of all spilled regvars.
    for (const LiveRange* lr : *spilledLRs_) {

        // Ignore request to spill/fill the spill/fill ranges
        // as it does not help the allocator.
        if (shouldSpillRegister(lr->getVar()) == false)
        {
            bool needsEOTGRF = lr->getEOTSrc() && builder_->hasEOTGRFBinding();
            if (failSafeSpill_ && needsEOTGRF &&
                (lr->getVar()->isRegVarTransient() ||
                    lr->getVar()->isRegVarTmp()))
            {
                lr->getVar()->setPhyReg(builder_->phyregpool.getGreg(spillRegStart_ > (kernel->getNumRegTotal() - 16) ? spillRegStart_ : (kernel->getNumRegTotal() - 16)), 0);
                continue;
            }
            return false;
        }
        else
        {
            lr->getVar()->getDeclare()->setSpillFlag();
        }
    }

    if (doSpillSpaceCompression)
    {
        // cache spilling intervals in sorted order so we can use these
        // for spill compression.
        for (auto dcl : spillIntf_->getAugmentation().getSortedLiveIntervals())
        {
            if (dcl->isSpilled())
                spillingIntervals.push_back(dcl);
        }
    }

    // Handle address taken spills
    bool success = handleAddrTakenSpills(kernel, pointsToAnalysis);

    if (!success)
    {
        DEBUG_MSG("Enough physical register not available for handling address taken spills" << std::endl);
        return false;
    }

    if (kernel->getOption(vISA_DoSplitOnSpill))
    {
        // remove all spilled splits
        for (const LiveRange* lr : *spilledLRs_)
        {
            auto dcl = lr->getDcl();
            // check whether spilled variable is one of split vars
            if (gra.splitResults.find(dcl) == gra.splitResults.end())
                continue;

            LoopVarSplit::removeAllSplitInsts(&gra, dcl);
        }
    }

    // Insert spill/fill code for all basic blocks.
    updateRMWNeeded();
    FlowGraph& fg = kernel->fg;

    unsigned int id = 0;
    for (BB_LIST_ITER it = fg.begin(); it != fg.end(); it++)
    {
        bbId_ = (*it)->getId();
        INST_LIST::iterator jt = (*it)->begin();

        while (jt != (*it)->end()) {
            INST_LIST::iterator kt = jt;
            ++kt;
            G4_INST* inst = *jt;

            curInst = inst;
            curInst->setLexicalId(id++);

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
                            inst->isDpas() ||
                            (inst->isSplitSend() && i == 1) ||
                            (inst->isSpillIntrinsic() && inst->asSpillIntrinsic()->getNumRows() > 2);

                        if (mayExceedTwoGRF)
                        {
                            // this path may be taken if current instruction is a spill intrinsic. for eg,
                            // V81 below is not spilled in 1st RA iteration:
                            // mov (16)             SP_GRF_V167_0(0,0)<1>:w  V81(0,0)<1;1,0>:w // $412:&234:
                            // (W) intrinsic.spill.1 (16)  Scratch[0x32]  R0_Copy0(0,0)<1;1,0>:ud  SP_GRF_V167_0(0,0)<1;1,0>:w // $412:
                            // mov (16)             SP_GRF_V167_1(0,0)<1>:w  V81(1,0)<1;1,0>:w // $413:&235:
                            // (W) intrinsic.spill.1 (16)  Scratch[1x32]  R0_Copy0(0,0)<1;1,0>:ud  SP_GRF_V167_1(0,0)<1;1,0>:w // $413:
                            // mov (16)             SP_GRF_V167_2(0,0)<1>:w  V81(2,0)<1;1,0>:w // $414:&236:
                            // (W) intrinsic.spill.1 (16)  Scratch[2x32]  R0_Copy0(0,0)<1;1,0>:ud  SP_GRF_V167_2(0,0)<1;1,0>:w // $414:
                            // mov (16)             SP_GRF_V167_3(0,0)<1>:w  V81(3,0)<1;1,0>:w // $415:&237:
                            // (W) intrinsic.spill.1 (16)  Scratch[3x32]  R0_Copy0(0,0)<1;1,0>:ud  SP_GRF_V167_3(0,0)<1;1,0>:w // $415:
                            //
                            // ==> spill cleanup emits (notice spill range V81 appears in spill intrinsic but is not spilled itself):
                            //
                            // (W) intrinsic.spill.4 (16)  Scratch[0x32]  R0_Copy0(0,0)<1;1,0>:ud  V81(0,0)<1;1,0>:ud // $412:
                            //
                            // in next RA iteration assume V81 spills. so we should emit:
                            //
                            // (W) intrinsic.fill.4 (16)  FL_Send_V81_0(0,0)<1>:uw  R0_Copy0(0,0)<1;1,0>:ud  Scratch[88x32]  // $412:
                            // (W) intrinsic.spill.4 (16)  Scratch[0x32]  R0_Copy0(0,0)<1;1,0>:ud  FL_Send_V81_0(0,0)<1;1,0>:ud // $412:&393:
                            //
                            // V81 appears in spill intrinsic instruction as an optimization earlier even though
                            // it wasnt spilled in 1st RA iteration. however, since it spills in later RA iteration
                            // and its coalesced size > 2, we should handle it like send.
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

    for (auto spill : *spilledLRs_)
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
    sout.open(fname.str());
    kernel->emitDeviceAsm(sout, true, 0);
    sout.close ();
#endif
#endif

    return true;
}


void SpillManagerGRF::expireRanges(
    unsigned int idx, std::list<LSLiveRange*> * liveList)
{
    //active list is sorted in ascending order of starting index

    while (liveList->size() > 0)
    {
        unsigned int endIdx;
        LSLiveRange* lr = liveList->front();

        lr->getLastRef(endIdx);

        if (endIdx <= idx)
        {
#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE("Expiring range " << lr->getTopDcl()->getName() << std::endl);
#endif
            // Remove range from active list
            liveList->pop_front();
            lr->setActiveLR(false);
        }
        else
        {
            // As soon as we find first range that ends after ids break loop
            break;
        }
    }

    return;
}

void SpillManagerGRF::updateActiveList(
    LSLiveRange * lr, std::list<LSLiveRange*> * liveList)
{
    bool done = false;
    unsigned int newlr_end;

    lr->getLastRef(newlr_end);

    for (auto active_it = liveList->begin();
        active_it != liveList->end();
        active_it++)
    {
        unsigned int end_idx;
        LSLiveRange* active_lr = (*active_it);

        active_lr->getLastRef(end_idx);

        if (end_idx > newlr_end)
        {
            liveList->insert(active_it, lr);
            done = true;
            break;
        }
    }

    if (done == false)
        liveList->push_back(lr);
}

bool SpillManagerGRF::spillLiveRanges(G4_Kernel * kernel)
{
    // Set the spill flag of all spilled regvars.
    for (LSLiveRange* lr : *spilledLSLRs_) {
        lr->getTopDcl()->setSpillFlag();
    }

    // Handle address taken spills
    unsigned addrSpillNum = handleAddrTakenLSSpills(kernel, gra.pointsToAnalysis);

    if (addrSpillNum)
    {
        for (auto spill : *spilledLSLRs_)
        {
            unsigned disp = spill->getTopDcl()->getRegVar()->getDisp();

            if (spill->getTopDcl()->getRegVar()->isSpilled())
            {
                if (disp != UINT_MAX)
                {
                    nextSpillOffset_ = std::max(nextSpillOffset_, disp + getByteSize(spill->getTopDcl()->getRegVar()));
                }
            }
        }
    }

    // Insert spill/fill code for all basic blocks.
    FlowGraph& fg = kernel->fg;
    for (BB_LIST_ITER it = fg.begin(); it != fg.end(); it++)
    {
        bbId_ = (*it)->getId();
        INST_LIST::iterator jt = (*it)->begin();

        while (jt != (*it)->end())
        {
            INST_LIST::iterator kt = jt;
            ++kt;
            G4_INST* inst = *jt;
            unsigned int instID = inst->getLexicalId();
            curInst = inst;
            if (instID != (unsigned int)-1)
            {
                expireRanges(instID * 2, &activeLR_);
            }

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

                if (regVar && regVar->getDeclare()->isSpilled())
                {
                    G4_Declare* dcl = regVar->getDeclare();
                    while (dcl->getAliasDeclare())
                    {
                        dcl = dcl->getAliasDeclare();
                    }
                    LSLiveRange* lr = gra.getLSLR(dcl);
                    if (!lr->isActiveLR())
                    {
                        lr->setActiveLR(true);
                        updateActiveList(lr, &activeLR_);
                    }

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

                    if (regVar && regVar->getDeclare()->isSpilled())
                    {
                        G4_Declare* dcl = regVar->getDeclare();
                        while (dcl->getAliasDeclare())
                        {
                            dcl = dcl->getAliasDeclare();
                        }
                        LSLiveRange* lr = gra.getLSLR(dcl);
                        if (!lr->isActiveLR())
                        {
                            lr->setActiveLR(true);
                            updateActiveList(lr, &activeLR_);
                        }

                        if (inst->isLifeTimeEnd())
                        {
                            (*it)->erase(jt);
                            break;
                        }
                        bool mayExceedTwoGRF = (inst->isSend() && i == 0) ||
                            inst->isDpas() ||
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
    for (auto spill : (*spilledLSLRs_))
    {
        unsigned disp = spill->getTopDcl()->getRegVar()->getDisp();

        if (spill->getTopDcl()->getRegVar()->isSpilled ())
        {
            if (disp != UINT_MAX)
            {
                nextSpillOffset_ = std::max(nextSpillOffset_, disp + getByteSize(spill->getTopDcl()->getRegVar()));
            }
        }
    }

    return true;
}

//
// For XeHP_SDV and later, scratch surface is used for the vISA stack.  This means when
// the scratch message cannot be used for spill/fill (e.g., stack call),
// a0.2 will be used as the message descriptor for the spill/fill.
// As address RA is done before GRF, we don't know if a0.2 is live at the
// point of the spill/fill inst and thus may need to preserve its value.
// The good news is that all spill/fill may share the same A0, so we only
// need to save/restore A0 when it's actually referenced in the BB.
//
void GlobalRA::saveRestoreA0(G4_BB * bb)
{
    G4_Declare* tmpDcl = nullptr;
    unsigned int subReg = 0;
    if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc())
    {
        // Use r126.6:ud for storing old a0.2 as it isn't caller/callee save
        tmpDcl = builder.kernel.fg.getScratchRegDcl();
        subReg = 6;
    }
    else
    {
        MUST_BE_TRUE(builder.hasValidOldA0Dot2(), "old a0.2 not saved");
        tmpDcl = builder.getOldA0Dot2Temp();
        subReg = 0;
    }

    auto usesAddr = [](G4_INST* inst)
    {
        // ToDo: handle send with A0 msg desc better.
        if (inst->isSpillIntrinsic() || inst->isFillIntrinsic())
        {
            return false;
        }
        if (inst->getDst() && inst->getDst()->isAddress())
        {
            return true;
        }
        for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
        {
            if (inst->getSrc(i)->isAddress() || (inst->getSrc(i)->isSrcRegRegion() && inst->getSrc(i)->asSrcRegRegion()->isIndirect()))
            {
                return true;
            }
        }
        return false;
    };

    // a0.2 is spilled to r126.6 (r126 is scratch reg reserved for stack call)
    auto a0SaveMov = [this, tmpDcl, subReg]()
    {
        auto dstSave = builder.createDst(tmpDcl->getRegVar(), 0, subReg, 1, Type_UD);
        auto srcSave = builder.createSrc(builder.getBuiltinA0Dot2()->getRegVar(), 0, 0, builder.getRegionScalar(), Type_UD);
        auto saveInst = builder.createMov(g4::SIMD1, dstSave, srcSave, InstOpt_WriteEnable, false);
        return saveInst;
    };

    auto a0RestoreMov = [this, tmpDcl, subReg]()
    {
        auto dstRestore = builder.createDstRegRegion(builder.getBuiltinA0Dot2(), 1);
        auto srcRestore = builder.createSrc(tmpDcl->getRegVar(), 0, subReg, builder.getRegionScalar(), Type_UD);
        auto restoreInst = builder.createMov(g4::SIMD1, dstRestore, srcRestore, InstOpt_WriteEnable, false);
        return restoreInst;
    };

    auto a0SSOMove = [this]()
    {
        // SSO is stored in r126.7
        auto dst = builder.createDstRegRegion(builder.getBuiltinA0Dot2(), 1);
        auto SSOsrc = builder.createSrc(builder.getSpillSurfaceOffset()->getRegVar(),
            0, 0, builder.getRegionScalar(), Type_UD);
        {
            // shr (1) a0.2   SSO   0x4 {NM}
            auto imm4 = builder.createImm(4, Type_UD);
            return builder.createBinOp(G4_shr, g4::SIMD1, dst, SSOsrc, imm4, InstOpt_WriteEnable, false);
        }
    };

    auto isPrologOrEpilog = [this](G4_INST* inst)
    {
        // a0 is a caller save register. Don't save/restore it if it is used in callee save/restore sequence or
        // for frame descriptor spill instruction.
        if (inst == kernel.fg.builder->getFDSpillInst())
            return false;

        if (calleeSaveInsts.find(inst) != calleeSaveInsts.end() ||
            calleeRestoreInsts.find(inst) != calleeRestoreInsts.end())
            return false;

        return true;
    };

    bool hasActiveSpillFill = false;

    for (auto instIt = bb->begin(); instIt != bb->end(); ++instIt)
    {
        auto inst = (*instIt);

        if (inst->isSpillIntrinsic() || inst->isFillIntrinsic())
        {
            if (!hasActiveSpillFill)
            {
                // save a0.2 to addrSpillLoc, then overwrite it with the scratch surface offset
                if (isPrologOrEpilog(inst))
                {
                    auto addrSpill = a0SaveMov();
                    bb->insertBefore(instIt, addrSpill);
                }
               auto a0SSO = a0SSOMove();
               bb->insertBefore(instIt, a0SSO);
               // Need Call WA.
               // NoMask WA: save/restore code does not need NoMask WA.
               if (EUFusionCallWANeeded())
               {
                   addEUFusionCallWAInst(a0SSO);
               }
               hasActiveSpillFill = true;
            }
        }
        else if (hasActiveSpillFill && usesAddr(inst))
        {
            // restore A0
            auto addrFill = a0RestoreMov();
            bb->insertBefore(instIt, addrFill);
            hasActiveSpillFill = false;
        }
    }

    if (hasActiveSpillFill && !bb->isLastInstEOT() && !bb->isEndWithFRet())
    {
        // restore A0 before BB exit. BB is guaranteed to be non-empty as there's at least one spill/fill
        // If last inst is branch, insert restore before it. Otherwise insert it as last inst
        auto endIt = bb->back()->isCFInst() ? std::prev(bb->end()) : bb->end();
        bb->insertBefore(endIt, a0RestoreMov());
    }
}

static uint32_t computeSpillMsgDesc(unsigned int payloadSize, unsigned int offsetInGrfUnits, const IR_Builder& irb)
{
    // Compute msg descriptor given payload size and offset.
    unsigned headerPresent = 0x80000;
    uint32_t message = headerPresent;
    unsigned msgLength = SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT;
    message |= (msgLength << getSendMsgLengthBitOffset());
    message |= (1 << SCRATCH_MSG_DESC_CATEORY);
    message |= (1 << SCRATCH_MSG_DESC_CHANNEL_MODE);
    message |= (1 << SCRATCH_MSG_DESC_OPERATION_MODE);
    unsigned blocksize_encoding = getScratchBlocksizeEncoding(payloadSize, irb);
    message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
    int offset = offsetInGrfUnits;
    message |= offset;

    return message;
}

static uint32_t computeFillMsgDesc(unsigned int payloadSize, unsigned int offsetInGrfUnits, const IR_Builder& irb)
{
    // Compute msg descriptor given payload size and offset.
    unsigned headerPresent = 0x80000;
    uint32_t message = headerPresent;
    unsigned msgLength = 1;
    message |= (msgLength << getSendMsgLengthBitOffset());
    message |= (1 << SCRATCH_MSG_DESC_CATEORY);
    message |= (0 << SCRATCH_MSG_INVALIDATE_AFTER_READ);
    unsigned blocksize_encoding = getScratchBlocksizeEncoding(payloadSize, irb);
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

unsigned int GlobalRA::owordToGRFSize(unsigned int numOwords, const IR_Builder& builder)
{
    unsigned int GRFSize = numOwords / (2 * (builder.numEltPerGRF<Type_UB>() / HWORD_BYTE_SIZE));

    return GRFSize;
}

unsigned int GlobalRA::hwordToGRFSize(unsigned int numHwords, const IR_Builder& builder)
{
    return owordToGRFSize(numHwords * 2, builder);
}

unsigned int GlobalRA::GRFToHwordSize(unsigned int numGRFs, const IR_Builder& builder)
{
    return GRFSizeToOwords(numGRFs, builder) / 2;
}

unsigned int GlobalRA::GRFSizeToOwords(unsigned int numGRFs, const IR_Builder& builder)
{
    return numGRFs * (builder.numEltPerGRF<Type_UB>() / OWORD_BYTE_SIZE);
}

unsigned int GlobalRA::getHWordByteSize()
{
    return HWORD_BYTE_SIZE;
}

static G4_INST* createSpillFillAddr(
    IR_Builder& builder, G4_Declare* addr, G4_Declare* fp, int offset)
{
    auto imm = builder.createImm(offset, Type_UD);
    auto dst = builder.createDstRegRegion(addr, 1);
    if (fp)
    {
        auto src0 = builder.createSrcRegRegion(fp, builder.getRegionScalar());
        return builder.createBinOp(G4_add, g4::SIMD1, dst, src0, imm, InstOpt_WriteEnable, true);
    }
    else
    {
        return builder.createMov(g4::SIMD1, dst, imm, InstOpt_WriteEnable, true);
    }
}

static std::string makeSpillFillComment(
    const char *spillFill,
    const char *toFrom,
    const char *base,
    uint32_t spillOffset,
    const char *of,
    unsigned grfSize)
{
    std::stringstream comment;
    comment << spillFill << " " <<  toFrom << " ";
    comment << base << "[" << spillOffset / grfSize << "*" << (int)grfSize << "]";
    if (!of || *of == 0) // some have "" as name
        of = "?";
    comment << " of " << of;
    return comment.str();
}

void GlobalRA::expandSpillLSC(G4_BB* bb, INST_LIST_ITER& instIt)
{
    auto& builder = kernel.fg.builder;
    auto inst = (*instIt)->asSpillIntrinsic();
    // offset into scratch surface in bytes
    auto spillOffset = inst->getOffsetInBytes();
    uint32_t numRows = inst->getNumRows();
    auto payload = inst->getSrc(1)->asSrcRegRegion();
    auto rowOffset = payload->getRegOff();

    LSC_OP op = LSC_STORE;
    LSC_SFID lscSfid = LSC_UGM;
    LSC_CACHE_OPTS cacheOpts{ LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT };

    LSC_ADDR addrInfo;
    addrInfo.type = LSC_ADDR_TYPE_SS; //Scratch memory
    addrInfo.immScale = 1;
    addrInfo.immOffset = 0;
    addrInfo.size = LSC_ADDR_SIZE_32b;

    builder->instList.clear();
    while (numRows > 0)
    {
        auto numGRFToWrite = getPayloadSizeGRF(numRows);

        G4_Declare* spillAddr = inst->getFP() ? kernel.fg.scratchRegDcl : inst->getHeader()->getTopDcl();
        {
            // need to calculate spill address
            createSpillFillAddr(*builder, spillAddr, inst->getFP(), spillOffset);
        }

        LSC_DATA_SHAPE dataShape;
        dataShape.size = LSC_DATA_SIZE_32b;
        dataShape.order = LSC_DATA_ORDER_TRANSPOSE;
        dataShape.elems = builder->lscGetElementNum(numGRFToWrite * builder->getGRFSize() / 4);

        auto src0Addr = builder->createSrcRegRegion(spillAddr, builder->getRegionStride1());
        auto payloadToUse = builder->createSrcWithNewRegOff(payload, rowOffset);

        auto surface = builder->createSrcRegRegion(builder->getSpillSurfaceOffset(),
            builder->getRegionScalar());

        G4_DstRegRegion* postDst = builder->createNullDst(Type_UD);
        G4_SendDescRaw* desc = builder->createLscMsgDesc(
            op,
            lscSfid,
            EXEC_SIZE_1,
            cacheOpts,
            addrInfo,
            dataShape,
            surface,
            0,
            1);

        auto sendInst = builder->createLscSendInst(
            nullptr,
            postDst,
            src0Addr,
            payloadToUse,
            g4::SIMD1,
            desc,
            inst->getOption(),
            LSC_ADDR_TYPE_SS,
            false);

        sendInst->addComment(makeSpillFillComment(
            "spill", "to",
            inst->getFP() ? "FP" : "offset",
            spillOffset,
            payload->getTopDcl()->getName(),
            builder->getGRFSize()));

        numRows -= numGRFToWrite;
        rowOffset += numGRFToWrite;
        spillOffset += numGRFToWrite * builder->getGRFSize();
    }

    if (inst->getFP() &&
        kernel.getOption(vISA_GenerateDebugInfo))
    {
        for (auto newInst : builder->instList)
        {
            kernel.getKernelDebugInfo()->updateExpandedIntrinsic(inst->asSpillIntrinsic(), newInst);
        }
    }

    // Call WA and NoMask WA are mutual exclusive.
    if (getEUFusionCallWAInsts().count(inst) > 0)
    {
        removeEUFusionCallWAInst(inst);
        for (auto inst : builder->instList)
            addEUFusionCallWAInst(inst);
    } else if (getEUFusionNoMaskWAInsts().count(inst) > 0)
    {
        // no NoMask WA needed for stack call spill/fill
        removeEUFusionNoMaskWAInst(inst);
        if (!inst->getFP() && inst->isWriteEnableInst())
        {
            for (auto inst : builder->instList)
                addEUFusionNoMaskWAInst(bb, inst);
        }
    }

    splice(bb, instIt, builder->instList, inst->getCISAOff());
}

void GlobalRA::expandFillLSC(G4_BB* bb, INST_LIST_ITER& instIt)
{
    auto& builder = kernel.fg.builder;
    auto inst = (*instIt)->asFillIntrinsic();
    // offset into scratch surface in bytes
    auto fillOffset = inst->getOffsetInBytes();
    uint32_t numRows = inst->getNumRows();
    auto rowOffset = inst->getDst()->getRegOff();

    LSC_OP op = LSC_LOAD;
    LSC_SFID lscSfid = LSC_UGM;
    LSC_CACHE_OPTS cacheOpts{ LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT };

    LSC_ADDR addrInfo;
    addrInfo.type = LSC_ADDR_TYPE_SS; //Scratch memory
    addrInfo.immScale = 1;
    addrInfo.immOffset = 0;
    addrInfo.size = LSC_ADDR_SIZE_32b;

    builder->instList.clear();

    while (numRows > 0)
    {
        unsigned responseLength = getPayloadSizeGRF(numRows);
        LSC_DATA_SHAPE dataShape;
        dataShape.size = LSC_DATA_SIZE_32b;
        dataShape.order = LSC_DATA_ORDER_TRANSPOSE;
        dataShape.elems = builder->lscGetElementNum(responseLength * builder->getGRFSize() / 4);

        G4_Declare* fillAddr = inst->getFP() ? kernel.fg.scratchRegDcl : inst->getHeader()->getTopDcl();
        {
            // need to calculate fill address
            createSpillFillAddr(*builder, fillAddr, inst->getFP(), fillOffset);
        }
        auto dstRead = builder->createDst(inst->getDst()->getTopDcl()->getRegVar(),
            (short)rowOffset, 0, 1, Type_UD);

        auto surface = builder->createSrcRegRegion(builder->getSpillSurfaceOffset(),
            builder->getRegionScalar());

        G4_SendDescRaw* desc = builder->createLscMsgDesc(
            op,
            lscSfid,
            EXEC_SIZE_1,
            cacheOpts,
            addrInfo,
            dataShape,
            surface,
            responseLength,
            1);

        auto sendInst = builder->createLscSendInst(
            nullptr,
            dstRead,
            builder->createSrcRegRegion(fillAddr, builder->getRegionScalar()),
            nullptr,
            g4::SIMD1,
            desc,
            inst->getOption(),
            LSC_ADDR_TYPE_SS,
            false);

        sendInst->addComment(makeSpillFillComment(
            "fill", "from",
            inst->getFP() ? "FP" : "offset",
            fillOffset,
            dstRead->getTopDcl()->getName(),
            builder->getGRFSize()));

        numRows -= responseLength;
        rowOffset += responseLength;
        fillOffset += responseLength * builder->getGRFSize();
    }

    if (inst->getFP() &&
        kernel.getOption(vISA_GenerateDebugInfo))
    {
        for (auto newInst : builder->instList)
        {
            kernel.getKernelDebugInfo()->updateExpandedIntrinsic(inst->asFillIntrinsic(), newInst);
        }
    }

    // Call WA and NoMask WA are mutual exclusive.
    if (getEUFusionCallWAInsts().count(inst) > 0)
    {
        removeEUFusionCallWAInst(inst);
        for (auto inst : builder->instList)
            addEUFusionCallWAInst(inst);
    }
    else if (getEUFusionNoMaskWAInsts().count(inst) > 0)
    {
        removeEUFusionNoMaskWAInst(inst);
        // no WA needed for stack call spill/fill
        if (!inst->getFP() && inst->isWriteEnableInst())
        {
            for (auto inst : builder->instList)
                addEUFusionNoMaskWAInst(bb, inst);
        }
    }

    splice(bb, instIt, builder->instList, inst->getCISAOff());
}

void GlobalRA::expandSpillNonStackcall(
    uint32_t numRows, uint32_t offset, short rowOffset,
    G4_SrcRegRegion* header, G4_SrcRegRegion* payload, G4_BB* bb,
    INST_LIST_ITER& instIt)
{
    auto& builder = kernel.fg.builder;
    auto inst = (*instIt);

    if (offset == G4_SpillIntrinsic::InvalidOffset)
    {
        // oword msg
        auto payloadToUse = builder->createSrcRegRegion(*payload);
        auto [spillMsgDesc, execSize] = SpillManagerGRF::createSpillSendMsgDescOWord(*builder, numRows);
        G4_INST* sendInst = nullptr;
        // Use bindless for XeHP_SDV and later
        if (builder->hasScratchSurface())
        {
            G4_Imm* descImm = createMsgDesc(GRFSizeToOwords(numRows, *builder), true, true);
            // Update BTI to 251
            auto spillMsgDesc = descImm->getInt();
            spillMsgDesc = spillMsgDesc & 0xffffff00;
            spillMsgDesc |= 251;

            auto msgDesc = builder->createWriteMsgDesc(SFID::DP_DC0, (uint32_t)spillMsgDesc, numRows);
            G4_Imm* msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);

            // a0 is set by saveRestoreA0()
            auto a0Src = builder->createSrcRegRegion(builder->getBuiltinA0Dot2(), builder->getRegionScalar());
            sendInst = builder->createInternalSplitSendInst(execSize, inst->getDst(),
                header, payloadToUse, msgDescImm, inst->getOption(), msgDesc, a0Src);
        }
        else
        {
            G4_SendDescRaw * msgDesc =
                kernel.fg.builder->createSendMsgDesc(
                    spillMsgDesc & 0x000FFFFFu, 0, 1, SFID::DP_DC0, numRows, 0, SendAccess::WRITE_ONLY);
            G4_Imm* msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);
            G4_Imm* extDesc = builder->createImm(msgDesc->getExtendedDesc(), Type_UD);
            sendInst = builder->createInternalSplitSendInst(execSize,
                inst->getDst(), header, payloadToUse, msgDescImm, inst->getOption(),
                msgDesc, extDesc);
        }
        instIt = bb->insertBefore(instIt, sendInst);
        if (EUFusionNoMaskWANeeded() && sendInst->isWriteEnableInst())
        {
            addEUFusionNoMaskWAInst(bb, sendInst);
        }
    }
    else
    {
        while (numRows >= 1)
        {
            auto payloadToUse = builder->createSrcWithNewRegOff(payload, rowOffset);

            auto region = builder->getRegionStride1();

            uint32_t spillMsgDesc = computeSpillMsgDesc(getPayloadSizeGRF(numRows), offset, *builder);
            auto msgDesc = builder->createWriteMsgDesc(SFID::DP_DC0, spillMsgDesc, getPayloadSizeGRF(numRows));
            G4_Imm* msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);

            G4_SrcRegRegion* headerOpnd = builder->createSrcRegRegion(builder->getBuiltinR0(), region);
            G4_Imm* extDesc = builder->createImm(msgDesc->getExtendedDesc(), Type_UD);
            G4_ExecSize execSize = numRows > 1 ? g4::SIMD16 : g4::SIMD8;

            auto sendInst = builder->createInternalSplitSendInst(execSize,
                inst->getDst(), headerOpnd, payloadToUse, msgDescImm,
                inst->getOption(), msgDesc, extDesc);

            std::stringstream comments;
            comments << "scratch space spill: " << payloadToUse->getTopDcl()->getName() << " from offset[" << offset << "x32]";
            sendInst->addComment(comments.str());

            instIt = bb->insertBefore(instIt, sendInst);

            if (EUFusionNoMaskWANeeded() && sendInst->isWriteEnableInst())
            {
                addEUFusionNoMaskWAInst(bb, sendInst);
            }

            numRows -= getPayloadSizeGRF(numRows);
            offset += getPayloadSizeGRF(numRows);
            rowOffset += getPayloadSizeGRF(numRows);
        }
    }
}

void GlobalRA::expandSpillStackcall(
    uint32_t numRows, uint32_t offset, short rowOffset,
    G4_SrcRegRegion* payload, G4_BB* bb, INST_LIST_ITER& instIt)
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
            G4_ExecSize execSize = (owordSize > 2) ? g4::SIMD16 : g4::SIMD8;
            G4_DstRegRegion* dst = builder->createNullDst((execSize > g4::SIMD8) ? Type_UW : Type_UD);
            auto sendSrc0 = builder->createSrc(scratchRegDcl->getRegVar(),
                0, 0, builder->rgnpool.createRegion(8, 8, 1), Type_UD);
            unsigned messageLength = owordToGRFSize(owordSize, *builder);
            G4_Imm* descImm = createMsgDesc(owordSize, true, true);
            G4_INST* sendInst = nullptr;
            // Use bindless for XeHP_SDV and later
            if (builder->getPlatform() >= Xe_XeHPSDV)
            {
                // Update BTI to 251
                auto spillMsgDesc = descImm->getInt();
                spillMsgDesc = spillMsgDesc & 0xffffff00;
                spillMsgDesc |= 251;

                auto msgDesc = builder->createWriteMsgDesc(SFID::DP_DC0, (uint32_t)spillMsgDesc, messageLength);
                G4_Imm* msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);

                // a0 is set by saveRestoreA0()
                auto a0Src = builder->createSrcRegRegion(builder->getBuiltinA0Dot2(), builder->getRegionScalar());
                sendInst = builder->createInternalSplitSendInst(execSize, inst->getDst(),
                    sendSrc0, payloadToUse, msgDescImm, inst->getOption(), msgDesc, a0Src);
            }
            else
            {
                auto msgDesc = builder->createWriteMsgDesc(SFID::DP_DC0, (uint32_t)descImm->getInt(), messageLength);
                G4_Imm* msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);
                G4_Imm* extDesc = builder->createImm(msgDesc->getExtendedDesc(), Type_UD);
                sendInst = builder->createInternalSplitSendInst(execSize, dst, sendSrc0, payloadToUse,
                    msgDescImm, inst->getOption() | InstOpt_WriteEnable, msgDesc, extDesc);
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
                src0 = builder->createSrc(framePtr->getRegVar(), 0, 0, builder->getRegionScalar(), Type_UD);
                src1 = builder->createImm(offsetOword, Type_UD);
                hdrSetInst = builder->createBinOp(G4_add, g4::SIMD1, dst, src0, src1, InstOpt_WriteEnable, false);
            }
            else
            {
                src0 = builder->createImm(offsetOword, Type_UD);
                hdrSetInst = builder->createMov(g4::SIMD1, dst, src0, InstOpt_WriteEnable, false);
            }

            bb->insertBefore(spillIt, hdrSetInst);
        }

        auto spillSends = createOwordSpill(payloadSizeInOwords, payloadToUse);
        std::stringstream comments;
        comments <<  "stack spill: " << payload->getTopDcl()->getName() << " to FP[" << inst->asSpillIntrinsic()->getOffset() << "x32]";
        spillSends->addComment(comments.str());

        bb->insertBefore(spillIt, spillSends);

        if (getEUFusionCallWAInsts().count(inst) > 0)
        {
            removeEUFusionCallWAInst(inst);
            addEUFusionCallWAInst(spillSends);
            if (hdrSetInst)
                addEUFusionCallWAInst(hdrSetInst);
        }

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
    for (auto instIt = bb->begin(); instIt != bb->end();)
    {
        auto inst = (*instIt);
        if (inst->isSpillIntrinsic())
        {
            bool isOffBP = inst->asSpillIntrinsic()->isOffBP();
            uint32_t numRows = inst->asSpillIntrinsic()->getNumRows();
            uint32_t offset = inst->asSpillIntrinsic()->getOffset() *
                (builder.numEltPerGRF<Type_UB>() / HWORD_BYTE_SIZE);
            auto header = inst->getSrc(0)->asSrcRegRegion();
            auto payload = inst->getSrc(1)->asSrcRegRegion();
            auto spillIt = instIt;

            auto rowOffset = payload->getRegOff();
            if (useLscForNonStackCallSpillFill || spillFillIntrinUsesLSC(inst)) {
                expandSpillLSC(bb, instIt);
            }
            else
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

 void GlobalRA::expandFillNonStackcall(uint32_t numRows, uint32_t offset, short rowOffset, G4_SrcRegRegion* header, G4_DstRegRegion* resultRgn, G4_BB* bb, INST_LIST_ITER& instIt)
 {
     auto& builder = kernel.fg.builder;
     auto inst = (*instIt);

     if (offset == G4_FillIntrinsic::InvalidOffset)
     {
         // oword msg
         G4_ExecSize execSize = g4::SIMD16;
         auto numRowsOword = GRFSizeToOwords(numRows, *builder);
         auto fillDst = builder->createDst(resultRgn->getBase(), rowOffset,
             0, resultRgn->getHorzStride(), resultRgn->getType());
         auto sendSrc0 = builder->createSrc(header->getBase(),
             0, 0, builder->rgnpool.createRegion(8, 8, 1), Type_UD);
         G4_Imm* desc = createMsgDesc(numRowsOword, false, false);
         G4_INST* sendInst = nullptr;
         auto sfId = SFID::DP_DC0;

         // Use bindless for XeHP_SDV and later
         if (builder->hasScratchSurface())
         {
             // Update BTI to 251
             auto newDesc = desc->getInt() & 0xffffff00;
             newDesc |= 251;
             desc = builder->createImm(newDesc, Type_UD);

             auto msgDesc = builder->createReadMsgDesc(sfId, (uint32_t)desc->getInt());
             G4_Operand* msgDescOpnd = builder->createImm(msgDesc->getDesc(), Type_UD);

             // a0 is set by saveRestoreA0()
             auto src1 = builder->createSrc(builder->getBuiltinA0Dot2()->getRegVar(), 0, 0,
                 builder->getRegionScalar(), Type_UD);

             sendInst = builder->createInternalSplitSendInst(execSize, fillDst, sendSrc0,
                 nullptr, msgDescOpnd, InstOpt_WriteEnable, msgDesc, src1);
         }
         else
         {
             auto msgDesc = builder->createReadMsgDesc(sfId, (uint32_t)desc->getInt());
             G4_Operand* msgDescOpnd = builder->createImm(msgDesc->getDesc(), Type_UD);
             sendInst = builder->createInternalSendInst(nullptr, G4_send, execSize, fillDst, sendSrc0, msgDescOpnd,
                 InstOpt_WriteEnable, msgDesc);
         }
         instIt = bb->insertBefore(instIt, sendInst);

         if (EUFusionNoMaskWANeeded() && sendInst->isWriteEnableInst())
         {
             addEUFusionNoMaskWAInst(bb, sendInst);
         }
     }
     else
     {
         while (numRows >= 1)
         {
             auto fillDst = builder->createDst(resultRgn->getBase(), rowOffset,
                 0, resultRgn->getHorzStride(), resultRgn->getType());

             auto region = builder->getRegionStride1();
             G4_SrcRegRegion* headerOpnd = builder->createSrcRegRegion(builder->getBuiltinR0(), region);

             uint32_t fillMsgDesc = computeFillMsgDesc(getPayloadSizeGRF(numRows), offset, *builder);

             G4_SendDescRaw* msgDesc = kernel.fg.builder->createSendMsgDesc(fillMsgDesc,
                 getPayloadSizeGRF(numRows), 1, SFID::DP_DC0, 0, 0, SendAccess::READ_ONLY);

             G4_Imm* msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);

             auto sendInst = builder->createInternalSendInst(nullptr,
                 G4_send, g4::SIMD16, fillDst, headerOpnd, msgDescImm, inst->getOption(),
                 msgDesc);

             std::stringstream comments;
             comments << "scratch space fill: " << inst->getDst()->getTopDcl()->getName() << " from offset[" << offset << "x32]";
             sendInst->addComment(comments.str());

             instIt = bb->insertBefore(instIt, sendInst);

             if (EUFusionNoMaskWANeeded() && sendInst->isWriteEnableInst())
             {
                 addEUFusionNoMaskWAInst(bb, sendInst);
             }

             numRows -= getPayloadSizeGRF(numRows);
             offset += getPayloadSizeGRF(numRows);
             rowOffset += getPayloadSizeGRF(numRows);
         }
     }
 }

void GlobalRA::expandFillStackcall(uint32_t numRows, uint32_t offset, short rowOffset, G4_SrcRegRegion* header, G4_DstRegRegion* resultRgn, G4_BB* bb, INST_LIST_ITER& instIt)
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
            G4_ExecSize execSize = (owordSize > 2) ? g4::SIMD16 : g4::SIMD8;
            auto sendSrc0 = builder->createSrc(scratchRegDcl->getRegVar(),
                0, 0, builder->rgnpool.createRegion(8, 8, 1), Type_UD);
            G4_Imm* desc = createMsgDesc(owordSize, false, false);
            G4_INST* sendInst = nullptr;
            auto sfId = SFID::DP_DC0;

            // Use bindless for XeHP_SDV and later
            if (builder->getPlatform() >= Xe_XeHPSDV)
            {
                // Update BTI to 251
                auto newDesc = desc->getInt() & 0xffffff00;
                newDesc |= 251;
                desc = builder->createImm(newDesc, Type_UD);

                auto msgDesc = builder->createReadMsgDesc(sfId, (uint32_t)desc->getInt());
                G4_Operand* msgDescOpnd = builder->createImm(msgDesc->getDesc(), Type_UD);

                // a0 is set by saveRestoreA0()
                auto src1 = builder->createSrc(builder->getBuiltinA0Dot2()->getRegVar(), 0, 0,
                    builder->getRegionScalar(), Type_UD);

                sendInst = builder->createInternalSplitSendInst(
                    execSize, fillVar, sendSrc0,
                    nullptr, msgDescOpnd, InstOpt_WriteEnable, msgDesc, src1);
            }
            else
            {
                auto msgDesc = builder->createReadMsgDesc(SFID::DP_DC0, (uint32_t)desc->getInt());
                auto msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);
                sendInst = builder->createInternalSendInst(
                    nullptr, G4_send, execSize, fillVar, sendSrc0, msgDescImm,
                    InstOpt_WriteEnable, msgDesc);
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
                src0 = builder->createSrc(framePtr->getRegVar(), 0, 0, builder->getRegionScalar(), Type_UD);
                src1 = builder->createImm(offsetOword, Type_UD);
                hdrSetInst = builder->createBinOp(G4_add, g4::SIMD1, dst, src0, src1, InstOpt_WriteEnable, false);
            }
            else
            {
                src0 = builder->createImm(offsetOword, Type_UD);
                hdrSetInst = builder->createMov(g4::SIMD1, dst, src0, InstOpt_WriteEnable, false);
            }

            bb->insertBefore(fillIt, hdrSetInst);
        }

        auto fillSends = createOwordFill(respSizeInOwords, fillDst);

        if (getEUFusionCallWAInsts().count(inst) > 0)
        {
            removeEUFusionCallWAInst(inst);
            addEUFusionCallWAInst(fillSends);
            if (hdrSetInst)
                addEUFusionCallWAInst(hdrSetInst);
        }

        std::stringstream comments;
        comments << "stack fill: " << resultRgn->getTopDcl()->getName() << " from FP[" << inst->asFillIntrinsic()->getOffset() << "x32]";
        fillSends->addComment(comments.str());

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

bool GlobalRA::spillFillIntrinUsesLSC(G4_INST* spillFillIntrin)
{
    G4_Declare* headerDcl = nullptr;
    if (!spillFillIntrin)
        return false;

    if (spillFillIntrin->isFillIntrinsic())
        headerDcl = spillFillIntrin->asFillIntrinsic()->getHeader()->getTopDcl();
    else if (spillFillIntrin->isSpillIntrinsic())
        headerDcl = spillFillIntrin->asSpillIntrinsic()->getHeader()->getTopDcl();

    if (useLscForSpillFill && headerDcl != builder.getBuiltinR0()->getRootDeclare())
    {
        return true;
    }
    return false;
}

void GlobalRA::expandFillIntrinsic(G4_BB* bb)
{
    // fill (1) fill_var:ud     bitmask:ud     offset:ud
    for (auto instIt = bb->begin(); instIt != bb->end();)
    {
        auto inst = (*instIt);
        if (inst->isFillIntrinsic())
        {
            bool isOffBP = inst->asFillIntrinsic()->isOffBP();
            uint32_t numRows = inst->asFillIntrinsic()->getNumRows();
            uint32_t offset = inst->asFillIntrinsic()->getOffset() *
                (builder.numEltPerGRF<Type_UB>() / HWORD_BYTE_SIZE);
            auto header = inst->getSrc(0)->asSrcRegRegion();
            auto resultRgn = inst->getDst();
            auto fillIt = instIt;

            auto rowOffset = resultRgn->getRegOff();
            if (useLscForNonStackCallSpillFill || spillFillIntrinUsesLSC(inst)) {
                expandFillLSC(bb, instIt);
            }
            else
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


void GlobalRA::expandSpillFillIntrinsics(unsigned int spillSizeInBytes)
{
    auto globalScratchOffset = kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);

    for (auto bb : kernel.fg)
    {
        if (builder.hasScratchSurface() &&
            (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc() || kernel.fg.builder->hasValidOldA0Dot2()
                || (useLscForSpillFill && (spillSizeInBytes + globalScratchOffset) > SCRATCH_MSG_LIMIT &&
                    spillSizeInBytes > 0)
                || (useLscForNonStackCallSpillFill && spillSizeInBytes > 0)
            // Following cases exist:
            // a. XeHP_SDV without stackcall => use hword scratch msg
            // b. XeHP_SDV without stackcall => using oword block msg
            // c. XeHP_SDV with stackcall
            // d. DG2+ without stackcall => hword scratch msg
            // e. DG2+ without stackcall => using LSC
            // f. DG2+ with stackcall    => using LSC
            //
            // (a), (d) are similar to SKL with hword scratch msg.
            //
            // (c), (f):
            // a0.2 is saved/restored from r126.6:ud
            // SSO is saved in r126.7:ud (in replaceSSO function)
            // XeHP_SDV uses oword msg, DG2+ uses LSC msg
            // For DG2+, offset is computed in r126.0
            //
            // (b):
            // oword header is prepared in a temp variable, allocated by RA
            // a0.2 is saved/restored in oldA0Dot2(0,0) whenever required
            // SSO is allocated to a live-out temp (not tied to r126.7:ud)
            //
            // (e):
            // LSC msg is used for spill/fill
            // Spill offset is computed in spillHeader(0,0)
            // a0.2 is saved/restored in oldA0Dot2(0,0) whenever required
            // spillHeader is marked as live-out
            //
            // When needed:
            // SSO is marked as live-out
            // r0 is stored in r127
            //
        ))
        {
            saveRestoreA0(bb);
        }
        expandSpillIntrinsic(bb);
        expandFillIntrinsic(bb);
    }
    kernel.fg.builder->getcompilerStats().SetI64(CompilerStats::numGRFSpillStr(), numGRFSpill, kernel.getSimdSize());
    kernel.fg.builder->getcompilerStats().SetI64(CompilerStats::numGRFFillStr(), numGRFFill, kernel.getSimdSize());

}

SpillAnalysis::~SpillAnalysis()
{
    if (Refs)
    {
        delete Refs;
        Refs = nullptr;
    }
}

void SpillAnalysis::Dump(std::ostream& OS)
{
    auto& GRA = GC->getGRA();
    auto& Kernel = GRA.kernel;
    auto& Loops = Kernel.fg.getLoops();
    const auto& Spills = GC->getSpilledLiveRanges();
    std::unordered_map<G4_INST*, G4_BB*> InstBBMap;

    for (auto* BB : Kernel.fg.getBBList())
        for (auto* Inst : BB->getInstList())
            InstBBMap[Inst] = BB;

    OS << "Name, Dcl Byte Size, Spill Cost, Degree, #Defs, #Uses, Distance, #BBs, All BBs Where Live" << std::endl;

    for (auto* Spill : Spills)
    {
        // dump - {Dcl size, Spill cost, Live BBs (loop annotation)}
        auto Dcl = Spill->getDcl();
        auto DclSizeBytes = Dcl->getByteSize();
        auto SpillCost = Spill->getSpillCost();
        auto Degree = DclDegree[Dcl];
        auto LiveBBs = GetLiveBBs(Dcl, InstBBMap);
        auto Distance = GetDistance(Dcl);
        auto NumDefs = Refs->getDefCount(Dcl);
        auto NumUses = Refs->getUseCount(Dcl);

        OS << Dcl->getName() << "," << DclSizeBytes << ", " << SpillCost << ", " << Degree << ", "
            << NumDefs << ", " << NumUses << ", "
            << Distance << ", " << LiveBBs.size() << ", ";

        for (auto* LiveBB : LiveBBs)
        {
            OS << "BB" << LiveBB->getId();
            auto* ClosestLoop = Loops.getInnerMostLoop(LiveBB);
            if (ClosestLoop)
            {
                OS << " [L" << ClosestLoop->id << "]";
            }
            OS << " ";
        }

        OS << std::endl;
    }
}

unsigned int SpillAnalysis::GetDistance(G4_Declare* Dcl)
{
    if (AugIntervals.count(Dcl) == 0)
    {
        // Construct distance in conventional way
        auto& Kernel = GC->getGRA().kernel;
        unsigned int Start = 0xffffffff, End = 0x0;

        auto* Defs = Refs->getDefs(Dcl);
        auto* Uses = Refs->getUses(Dcl);

        for (auto& Def : *Defs)
        {
            auto* DefInst = std::get<0>(Def);
            Start = std::min(Start, DefInst->getLexicalId());
            End = std::max(End, DefInst->getLexicalId());
        }

        for (auto& Use : *Uses)
        {
            auto* UseInst = std::get<0>(Use);
            Start = std::min(Start, UseInst->getLexicalId());
            End = std::max(End, UseInst->getLexicalId());
        }

        for (auto* BB : Kernel.fg.getBBList())
        {
            if (LA->isLiveAtEntry(BB, Dcl->getRegVar()->getId()))
                Start = std::min(Start, BB->front()->getLexicalId());
            if (LA->isLiveAtExit(BB, Dcl->getRegVar()->getId()))
                End = std::max(End, BB->back()->getLexicalId());
        }

        return End - Start;
    }

    // Return augmentation distance when available
    auto Distance = AugIntervals[Dcl];
    return Distance.second->getLexicalId() - Distance.first->getLexicalId();
}

void SpillAnalysis::LoadAugIntervals(DECLARE_LIST& SortedIntervals, GlobalRA& GRA)
{
    for (auto& LR : SortedIntervals)
    {
        auto* Start = GRA.getStartInterval(LR);
        auto* End = GRA.getEndInterval(LR);
        AugIntervals[LR] = std::make_pair(Start, End);
    }
}

void SpillAnalysis::LoadDegree(G4_Declare* Dcl, unsigned int degree)
{
    // This should be called after degree computation and before simplification.
    DclDegree[Dcl] = degree;
}

void SpillAnalysis::Clear()
{
    if(Refs)
        delete Refs;

    Refs = nullptr;
    LA = nullptr;
    GC = nullptr;
    SM = nullptr;
    AugIntervals.clear();
    DclDegree.clear();

}

void SpillAnalysis::DumpHistogram(std::ostream& OS)
{
    // Compute and dump histogram
    std::map<unsigned int, unsigned int> SpillSizeHistogram;
    for (auto Spill : GC->getSpilledLiveRanges())
    {
        auto ByteSize = Spill->getDcl()->getByteSize();
        SpillSizeHistogram[ByteSize] += 1;
    }

    OS << "Spill Size Histogram For Iter#" << GC->getGRA().getIterNo() << " : " << std::endl;
    for (auto& Item : SpillSizeHistogram)
    {
        OS << "# vars of " << Item.first << " bytes spilled: " << Item.second << std::endl;
    }

    OS << std::endl;
}

void SpillAnalysis::Do(LivenessAnalysis* L, GraphColor* C, SpillManagerGRF* S)
{
    SetLivenessAnalysis(L);
    SetGraphColor(C);
    SetSpillManager(S);

    unsigned int LexId = 0;
    for (auto* BB : C->getGRA().kernel.fg.getBBList())
        for (auto* Inst : BB->getInstList())
            Inst->setLexicalId(LexId++);

    Refs = new VarReferences(C->getGRA().kernel);

    auto IterNo = C->getGRA().getIterNo();

    std::string FN = "spill-iter-";
    FN += std::to_string(IterNo);
    FN += std::string(".csv");
    std::ofstream OF;
    OF.open(FN, std::ofstream::out);
    Dump(OF);
    OF.close();

    FN = "misc-data";
    OF.open(FN, IterNo == 0 ? std::ofstream::out : std::ofstream::app);
    if (IterNo == 0)
    {
        ((vISA::Analysis*)&C->getGRA().kernel.fg.getLoops())->dump(OF);
    }
    DumpHistogram(OF);
    OF.close();
}

std::vector<G4_BB*> SpillAnalysis::GetLiveBBs(G4_Declare* Dcl, std::unordered_map<G4_INST*, G4_BB*>& InstBBMap)
{
    // Return all BBs over which Dcl is live. This includes augmentation data.
    auto Order = [](const G4_BB* First, const G4_BB* Second)
    {
        return First->getId() < Second->getId();
    };
    std::set<G4_BB*, decltype(Order)> BBs(Order);
    auto& Kernel = GC->getGRA().kernel;

    VarReferences VarRefs(Kernel);
    auto* Defs = VarRefs.getDefs(Dcl);
    auto* Uses = VarRefs.getUses(Dcl);

    for (auto Def : *Defs)
    {
        auto* BB = std::get<1>(Def);
        BBs.insert(BB);
    }

    for (auto Use : *Uses)
    {
        auto* BB = std::get<1>(Use);
        BBs.insert(BB);
    }

    for (auto BB : Kernel.fg.getBBList())
    {
        if (LA->isLiveAtEntry(BB, Dcl->getRegVar()->getId()) ||
            LA->isLiveAtExit(BB, Dcl->getRegVar()->getId()))
        {
            BBs.insert(BB);
        }
    }

    if (AugIntervals.count(Dcl))
    {
        auto& Interval = AugIntervals[Dcl];
        auto AugBBs = GetIntervalBBs(Interval.first, Interval.second, InstBBMap);
        std::for_each(AugBBs.begin(), AugBBs.end(), [&](G4_BB* BB) {BBs.insert(BB); });
    }

    std::vector<G4_BB*> RetBBs;
    std::for_each(BBs.begin(), BBs.end(), [&](G4_BB* BB) {RetBBs.push_back(BB); });

    return RetBBs;
}

std::vector<G4_BB*> vISA::SpillAnalysis::GetIntervalBBs(G4_INST* Start, G4_INST* End, std::unordered_map<G4_INST*, G4_BB*>& InstBBMap)
{
    // Return vector of BBs given Start/End G4_INST*s
    std::vector<G4_BB*> BBs;
    auto& Kernel = GC->getGRA().kernel;
    bool Started = false;
    for (auto* BB : Kernel.fg.getBBList())
    {
        bool BBAdded = false;
        for (auto* Inst : BB->getInstList())
        {
            if (Inst == Start)
                Started = true;

            if (Started && !BBAdded)
            {
                BBs.push_back(BB);
                BBAdded = true;
            }

            if (Inst == End)
                return BBs;
        }
    }

    return BBs;
}
