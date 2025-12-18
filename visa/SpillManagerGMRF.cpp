/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SpillManagerGMRF.h"
#include "BuildIR.h"
#include "DebugInfo.h"
#include "FlowGraph.h"
#include "G4_IR.hpp"
#include "GraphColor.h"
#include "PointsToAnalysis.h"

#include <cmath>
#include <unordered_set>

using namespace vISA;

// Configurations

#define ADDRESS_SENSITIVE_SPILLS_IMPLEMENTED
#define SCRATCH_SPACE_ADDRESS_UNIT 5

// #define DISABLE_SPILL_MEMORY_COMPRESSION
// #define VERIFY_SPILL_ASSIGNMENTS

// Constant declarations

static const unsigned DWORD_BYTE_SIZE = 4;
static const unsigned OWORD_BYTE_SIZE = 16;
static const unsigned HWORD_BYTE_SIZE = 32;
static const unsigned PAYLOAD_INPUT_REG_OFFSET = 0;
static const unsigned PAYLOAD_INPUT_SUBREG_OFFSET = 0;
static const unsigned OWORD_PAYLOAD_SPOFFSET_REG_OFFSET = 0;
static const unsigned OWORD_PAYLOAD_SPOFFSET_SUBREG_OFFSET = 2;
static const unsigned DWORD_PAYLOAD_SPOFFSET_REG_OFFSET = 1;
static const unsigned DWORD_PAYLOAD_SPOFFSET_SUBREG_OFFSET = 0;
static const unsigned OWORD_PAYLOAD_WRITE_REG_OFFSET = 1;
static const unsigned OWORD_PAYLOAD_WRITE_SUBREG_OFFSET = 0;
// dword scatter is always in SIMD8 mode
static const unsigned DWORD_PAYLOAD_WRITE_REG_OFFSET = 2;
static const unsigned DWORD_PAYLOAD_WRITE_SUBREG_OFFSET = 0;
static const unsigned OWORD_PAYLOAD_HEADER_MIN_HEIGHT = 1;
static const unsigned DWORD_PAYLOAD_HEADER_MIN_HEIGHT = 2;
static const unsigned OWORD_PAYLOAD_HEADER_MAX_HEIGHT = 1;
static const unsigned DWORD_PAYLOAD_HEADER_MAX_HEIGHT = 3;
static const unsigned DEF_HORIZ_STRIDE = 1;
static const unsigned REG_ORIGIN = 0;
static const unsigned SUBREG_ORIGIN = 0;

static const unsigned SEND_GT_READ_TYPE_BIT_OFFSET = 13;
static const unsigned SEND_GT_WRITE_TYPE_BIT_OFFSET = 13;
static const unsigned SEND_GT_DESC_DATA_SIZE_BIT_OFFSET = 8;
static const unsigned SEND_GT_OW_READ_TYPE = 0;
static const unsigned SEND_GT_OW_WRITE_TYPE = 8;
static const unsigned SEND_GT_SC_READ_TYPE = 6;
static const unsigned SEND_GT_SC_WRITE_TYPE = 11;
static const unsigned SEND_GT_DP_RD_EX_DESC_IMM = 5;
static const unsigned SEND_GT_DP_SC_RD_EX_DESC_IMM =
    4; // scatter reads go to sampler cache
static const unsigned SEND_GT_DP_WR_EX_DESC_IMM = 5;

static const unsigned SEND_IVB_MSG_TYPE_BIT_OFFSET = 14;
static const unsigned SEND_IVB_OW_READ_TYPE = 0;
static const unsigned SEND_IVB_OW_WRITE_TYPE = 8;
static const unsigned SEND_IVB_SC_READ_TYPE = 3;
static const unsigned SEND_IVB_SC_WRITE_TYPE = 11;
static const unsigned SEND_IVB_DP_RD_EX_DESC_IMM = 10; // data cache
static const unsigned SEND_IVB_DP_WR_EX_DESC_IMM = 10; // data cache

// Scratch msg
static const unsigned SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT = 1;
static const unsigned SCRATCH_MSG_DESC_CATEORY = 18;
static const unsigned SCRATCH_MSG_DESC_OPERATION_MODE = 17;
static const unsigned SCRATCH_MSG_DESC_CHANNEL_MODE = 16;
static const unsigned SCRATCH_MSG_INVALIDATE_AFTER_READ = 15;
static const unsigned SCRATCH_MSG_DESC_BLOCK_SIZE = 12;

#define LIMIT_SEND_EXEC_SIZE(EXEC_SIZE) (((EXEC_SIZE) > 16) ? 16 : (EXEC_SIZE))
#define SPILL_PAYLOAD_HEIGHT_LIMIT 4

void splice(G4_BB *bb, INST_LIST_ITER iter, INST_LIST &instList,
            unsigned int CISAOff) {
  // Update CISA offset of all instructions in instList before splicing
  // operation.
  // FIXME: shouldn't we just take the vISA offset of iter? Under what condition
  // do we want to override it?
  for (auto inst : instList) {
    inst->setVISAId(CISAOff);
  }

  bb->splice(iter, instList);
}

// spill/fill temps are always GRF-aligned, and are also even/odd aligned
// following the original declare's alignment
static void setNewDclAlignment(GlobalRA &gra, G4_Declare *newDcl,
                               bool evenAlign) {
  newDcl->setSubRegAlign(gra.builder.getGRFAlign());
  if (evenAlign) {
    newDcl->setEvenAlign();
  }

  gra.setSubRegAlign(newDcl, gra.builder.getGRFAlign());
  gra.setEvenAligned(newDcl, evenAlign);
}

SpillManagerGRF::SpillManagerGRF(
    GlobalRA &g, unsigned spillAreaOffset, const LivenessAnalysis *lvInfo,
    const Interference *intf, const LR_LIST *spilledLRs, bool failSafeSpill,
    unsigned spillRegSize, unsigned indrSpillRegSize,
    bool enableSpillSpaceCompression, bool useScratchMsg)
  : gra(g), builder_(g.kernel.fg.builder), varIdCount_(lvInfo->getNumSelectedVar()),
    latestImplicitVarIdCount_(0),
    lvInfo_(lvInfo), lrInfo_(&g.incRA.getLRs()), spilledLRs_(spilledLRs),
    nextSpillOffset_(spillAreaOffset),
    doSpillSpaceCompression(enableSpillSpaceCompression),
    failSafeSpill_(failSafeSpill), spillIntf_(intf),
    useScratchMsg_(useScratchMsg), refs(g.kernel),
    context(g, &g.incRA.getLRs()) {

  spillAreaOffset_ = spillAreaOffset;
  builder_->instList.clear();
  spillRegStart_ = g.kernel.getNumRegTotal();
  indrSpillRegStart_ = spillRegStart_;
  spillRegOffset_ = spillRegStart_;
  if (failSafeSpill) {
    bool isStackCall = builder_->usesStack();
    unsigned int stackCallRegSize =
        isStackCall ? g.kernel.stackCall.numReservedABIGRF() : 0;
    indrSpillRegStart_ -= (stackCallRegSize + indrSpillRegSize);
    spillRegStart_ = indrSpillRegStart_ - spillRegSize;
  }
  curInst = nullptr;
  globalScratchOffset =
      gra.kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
  spilledLSLRs_ = nullptr;
  if (builder_->hasScratchSurface()) {
    builder_->initScratchSurfaceOffset();
    auto entryBB = builder_->kernel.fg.getEntryBB();
    auto iter = std::find_if(entryBB->begin(), entryBB->end(),
                             [](G4_INST *inst) { return !inst->isLabel(); });
    splice(entryBB, iter, builder_->instList, UNMAPPABLE_VISA_INDEX);
  }

  if (failSafeSpill_) {
    if (gra.kernel.getOption(vISA_NewFailSafeRA)) {
      unsigned int spaceForPhyGRFSpill =
          BoundedRA::getNumPhyVarSlots(gra.kernel);
      int off = (int)spillAreaOffset;
      getSpillOffset(off);
      unsigned int aligned_off = ROUND(off, builder_->numEltPerGRF<Type_UB>());
      context.setSpillOff(aligned_off);
      vISA_ASSERT(spillAreaOffset_ == nextSpillOffset_, "expecting equality");
      spillAreaOffset_ =
          ROUND(spillAreaOffset_, builder_->numEltPerGRF<Type_UB>());
      spillAreaOffset_ += spaceForPhyGRFSpill;
      vISA_ASSERT(globalScratchOffset + spillAreaOffset_ >=
                      (ROUND(off, builder_->numEltPerGRF<Type_UB>()) +
                       spaceForPhyGRFSpill),
                  "unexpected overlap");
      nextSpillOffset_ = spillAreaOffset_;
      if (gra.getNumReservedGRFs() > 0)
        context.setReservedStart(spillRegStart_);
    }
  }
}

SpillManagerGRF::SpillManagerGRF(GlobalRA &g, unsigned spillAreaOffset,
                                 const LivenessAnalysis *lvInfo,
                                 LSLR_LIST *spilledLSLRs,
                                 bool enableSpillSpaceCompression,
                                 bool useScratchMsg)
  : gra(g), builder_(g.kernel.fg.builder), varIdCount_(lvInfo->getNumSelectedVar()),
    latestImplicitVarIdCount_(0), lvInfo_(lvInfo), lrInfo_(nullptr),
    spilledLSLRs_(spilledLSLRs), nextSpillOffset_(spillAreaOffset),
    doSpillSpaceCompression(enableSpillSpaceCompression),
    failSafeSpill_(false), useScratchMsg_(useScratchMsg), refs(g.kernel),
    context(g) {
  spillAreaOffset_ = spillAreaOffset;
  builder_->instList.clear();
  curInst = nullptr;
  globalScratchOffset =
      gra.kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);

  if (builder_->hasScratchSurface()) {
    builder_->initScratchSurfaceOffset();
    auto entryBB = builder_->kernel.fg.getEntryBB();
    auto iter = std::find_if(entryBB->begin(), entryBB->end(),
                             [](G4_INST *inst) { return !inst->isLabel(); });
    splice(entryBB, iter, builder_->instList, UNMAPPABLE_VISA_INDEX);
  }
}

bool SpillManagerGRF::headerNeeded() const {
  if (useScratchMsg_ && builder_->getPlatform() >= GENX_SKL)
    return false;

  if (builder_->kernel.fg.getHasStackCalls() ||
      builder_->kernel.fg.getIsStackCallFunc())
    return false;

  if (gra.useLscForSpillFill)
    return false;

  return true;
}

// Get the base regvar for the source or destination region.
template <class REGION_TYPE>
G4_RegVar *SpillManagerGRF::getRegVar(REGION_TYPE *region) const {
  G4_RegVar *spilledRegVar = (G4_RegVar *)region->getBase();
  return spilledRegVar;
}

// Obtain the register file type of the regvar.
G4_RegFileKind SpillManagerGRF::getRFType(G4_RegVar *regvar) const {
  return regvar->getDeclare()->getRegFile();
}

// Obtain the register file type of the region.
template <class REGION_TYPE>
G4_RegFileKind SpillManagerGRF::getRFType(REGION_TYPE *region) const {
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
unsigned SpillManagerGRF::getRegionOriginOffset(REGION_TYPE *region) const {
  unsigned rowOffset = builder_->numEltPerGRF<Type_UB>() * region->getRegOff();
  unsigned columnOffset = region->getSubRegOff() * region->getElemSize();
  return rowOffset + columnOffset;
}

// Get a GRF aligned mask
unsigned SpillManagerGRF::grfMask() const {
  unsigned mask = 0;
  mask = (mask - 1);
  vISA_ASSERT(std::log2(builder_->numEltPerGRF<Type_UB>()) ==
                  (float)((int)(std::log2(builder_->numEltPerGRF<Type_UB>()))),
              "expected integral value");
  unsigned int bits =
      (unsigned int)std::log2(builder_->numEltPerGRF<Type_UB>());
  mask = mask << bits;
  return mask;
}

// Get an hex word mask with the lower 5 bits zeroed.
unsigned SpillManagerGRF::hwordMask() const {
  unsigned mask = 0;
  mask = (mask - 1);
  mask = mask << 5;
  return mask;
}

// Get an octal word mask with the lower 4 bits zeroed.
unsigned SpillManagerGRF::owordMask() const {
  unsigned mask = 0;
  mask = (mask - 1);
  mask = mask << 4;
  return mask;
}

// Test of the offset is oword aligned.
bool SpillManagerGRF::owordAligned(unsigned offset) const {
  return (offset & owordMask()) == offset;
}

// Get the ceil of the ratio.
unsigned SpillManagerGRF::cdiv(unsigned dvd, unsigned dvr) {
  return (dvd / dvr) + ((dvd % dvr) ? 1 : 0);
}

// Get the live range corresponding to id.
bool SpillManagerGRF::shouldSpillRegister(G4_RegVar *regVar) const {
  if (getRFType(regVar) == G4_ADDRESS) {
    return false;
  }
  G4_RegVar *actualRegVar =
      (regVar->getDeclare()->getAliasDeclare())
          ? regVar->getDeclare()->getAliasDeclare()->getRegVar()
          : regVar;
  if (actualRegVar->getId() == UNDEFINED_VAL)
    return false;
  else if (regVar->isRegVarTransient() || regVar->isRegVarTmp())
    return false;
#ifndef ADDRESS_SENSITIVE_SPILLS_IMPLEMENTED
  else if (lvInfo_->isAddressSensitive(regVar->getId()))
    return false;
#endif
  else if (builder_->kernel.fg.isPseudoVCADcl(actualRegVar->getDeclare()) ||
           builder_->kernel.fg.isPseudoVCEDcl(actualRegVar->getDeclare()))
    return false;
  else
    return (*lrInfo_)[actualRegVar->getId()]->getPhyReg() == nullptr;
}

// Get the regvar with the id.
G4_RegVar *SpillManagerGRF::getRegVar(unsigned id) const {
  return (lvInfo_->vars)[id];
}

// Get the byte size of the live range.
unsigned SpillManagerGRF::getByteSize(G4_RegVar *regVar) const {
  unsigned normalizedRowSize = (regVar->getDeclare()->getNumRows() > 1)
                                   ? builder_->numEltPerGRF<Type_UB>()
                                   : regVar->getDeclare()->getNumElems() *
                                         regVar->getDeclare()->getElemSize();
  return normalizedRowSize * regVar->getDeclare()->getNumRows();
}

// Return all overlapping variables with dcl. Vector is not sorted
// so order of variables can change across runs.
void SpillManagerGRF::getOverlappingIntervals(
    G4_Declare *dcl, std::vector<G4_Declare *> &intervals) const {
  auto dclMask = gra.getAugmentationMask(dcl);

  // non-default variables have complete interference marked
  if (dclMask == AugmentationMasks::NonDefault)
    return;

  std::unordered_set<G4_Declare *> overlappingDcls;
  auto &allDclIntervals = gra.getAllIntervals(dcl);

  for (auto &dclInterval : allDclIntervals) {
    for (auto *otherDcl : spillingIntervals) {
      if (otherDcl == dcl)
        continue;

      if (dclMask != gra.getAugmentationMask(otherDcl))
        continue;

      auto &otherDclAllIntervals = gra.getAllIntervals(otherDcl);
      for (auto &otherDclInterval : otherDclAllIntervals) {
        if (dclInterval.intervalsOverlap(otherDclInterval)) {
          overlappingDcls.insert(otherDcl);
          break;
        }
      }
    }
  }

  intervals.insert(intervals.end(), overlappingDcls.begin(),
                   overlappingDcls.end());
}

// Calculate the spill memory displacement for the regvar.
unsigned SpillManagerGRF::calculateSpillDisp(G4_RegVar *regVar) const {
  vASSERT(regVar->getDisp() == UINT_MAX);

  // Locate the blocked locations calculated from the interfering
  // spilled live ranges and put them into a list in ascending order.

  using LocList = std::list<G4_RegVar *>;
  LocList locList;
  [[maybe_unused]] unsigned lrId = (regVar->getId() >= varIdCount_)
                      ? regVar->getBaseRegVar()->getId()
                      : regVar->getId();
  vASSERT(lrId < varIdCount_);

  const std::vector<unsigned int> &intfs =
      spillIntf_->getSparseIntfForVar(lrId);
  for (auto edge : intfs) {
    auto lrEdge = getRegVar(edge);
    if (lrEdge->isRegVarTransient())
      continue;
    if (lrEdge->getDisp() == UINT_MAX)
      continue;
    locList.push_back(lrEdge);
  }

  std::vector<G4_Declare *> overlappingDcls;
  getOverlappingIntervals((*lrInfo_)[lrId]->getDcl()->getRootDeclare(),
                          overlappingDcls);
  for (auto overlap : overlappingDcls) {
    auto lrEdge = overlap->getRegVar();
    if (lrEdge->getDisp() == UINT_MAX)
      continue;
    locList.push_back(lrEdge);
  }

  locList.sort([](G4_RegVar *v1, G4_RegVar *v2) {
    return v1->getDisp() < v2->getDisp();
  });
  locList.unique();

  // Find a spill slot for lRange within the locList.
  // we always start searching from nextSpillOffset_ to facilitate
  // intra-iteration reuse. cross iteration reuse is not done in interest of
  // compile time.
  unsigned regVarLocDisp =
      ROUND(nextSpillOffset_, builder_->numEltPerGRF<Type_UB>());
  unsigned regVarSize = getByteSize(regVar);

  for (G4_RegVar *curLoc : locList) {
    unsigned curLocDisp = curLoc->getDisp();
    if (regVarLocDisp < curLocDisp && regVarLocDisp + regVarSize <= curLocDisp)
      break;
    unsigned curLocEnd = curLocDisp + getByteSize(curLoc);
    {
      if (curLocEnd % builder_->numEltPerGRF<Type_UB>() != 0)
        curLocEnd = ROUND(curLocEnd, builder_->numEltPerGRF<Type_UB>());
    }

    regVarLocDisp = (regVarLocDisp > curLocEnd) ? regVarLocDisp : curLocEnd;
  }

  return regVarLocDisp;
}

unsigned SpillManagerGRF::calculateSpillDispForLS(G4_RegVar *regVar) const {
  vASSERT(regVar->getDisp() == UINT_MAX);

  // Locate the blocked locations calculated from the interfering
  // spilled live ranges and put them into a list in ascending order.

  std::vector<G4_RegVar *> locList;
  [[maybe_unused]] unsigned lrId = (regVar->getId() >= varIdCount_)
                      ? regVar->getBaseRegVar()->getId()
                      : regVar->getId();
  vASSERT(lrId < varIdCount_);

  for (auto* lr : (*spilledLSLRs_)) {
    G4_Declare* dcl = regVar->getDeclare();
    while (dcl->getAliasDeclare()) {
      dcl = dcl->getAliasDeclare();
    }
    LSLiveRange* curLr = gra.getLSLR(dcl);
    unsigned int curSid, curEid, sid, eid;
    curLr->getFirstRef(curSid);
    curLr->getLastRef(curEid);
    lr->getFirstRef(sid);
    lr->getLastRef(eid);
    if (curSid <= eid && sid <= curEid) {
      G4_RegVar* intfRegVar = lr->getTopDcl()->getRegVar();
      if (intfRegVar->isRegVarTransient())
        continue;

      unsigned iDisp = intfRegVar->getDisp();
      if (iDisp == UINT_MAX)
        continue;
      locList.push_back(intfRegVar);
    }
  }

  std::sort(locList.begin(), locList.end(), [](G4_RegVar *v1, G4_RegVar *v2) {
    return v1->getDisp() < v2->getDisp();
  });

  // Find a spill slot for lRange within the locList.
  // we always start searching from nextSpillOffset_ to facilitate
  // intra-iteration reuse. cross iteration reuse is not done in interest of
  // compile time.
  unsigned regVarLocDisp =
      ROUND(nextSpillOffset_, builder_->numEltPerGRF<Type_UB>());
  unsigned regVarSize = getByteSize(regVar);

  for (auto* curLoc : locList) {
    unsigned curLocDisp = curLoc->getDisp();
    if (regVarLocDisp < curLocDisp && regVarLocDisp + regVarSize <= curLocDisp)
      break;
    unsigned curLocEnd = curLocDisp + getByteSize(curLoc);
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
unsigned SpillManagerGRF::getSegmentDisp(REGION_TYPE *region,
                                         G4_ExecSize execSize) {
  vASSERT(region->getElemSize() && execSize);
  if (isUnalignedRegion(region, execSize))
    return getEncAlignedSegmentDisp(region, execSize);
  else
    return getRegionDisp(region);
}

// Get the spill/fill displacement of the regvar.
unsigned SpillManagerGRF::getDisp(G4_RegVar *regVar) {
  // Already calculated spill memory disp

  if (regVar->getDisp() != UINT_MAX) {
    return regVar->getDisp();
  } else if (regVar->isAliased()) {
    // If it is an aliased regvar then calculate the disp for the
    // actual regvar and then calculate the disp of the aliased regvar
    // based on it.
    G4_Declare *regVarDcl = regVar->getDeclare();
    return getDisp(regVarDcl->getAliasDeclare()->getRegVar()) +
           regVarDcl->getAliasOffset();
  } else if (gra.splitResults.find(regVar->getDeclare()->getRootDeclare()) !=
             gra.splitResults.end()) {
    // this variable is result of variable splitting optimization.
    // original variable is guaranteed to have spilled. if split
    // variable also spills then reuse original variable's spill
    // location.
    auto it = gra.splitResults.find(regVar->getDeclare()->getRootDeclare());
    auto disp = getDisp((*it).second.origDcl->getRegVar());
    regVar->setDisp(disp);
  } else if (regVar->isRegVarTransient() &&
             getDisp(regVar->getBaseRegVar()) != UINT_MAX) {
    // If its base regvar has been assigned a disp, then the spill memory
    // has already been allocated for it, simply calculate the disp based
    // on the enclosing segment disp.
    vASSERT(regVar->getBaseRegVar() != regVar);
    unsigned itsDisp;

    if (regVar->isRegVarSpill()) {
      G4_RegVarTransient *tRegVar = static_cast<G4_RegVarTransient *>(regVar);
      auto dstRR = tRegVar->getRepRegion()->asDstRegRegion();
      vASSERT(getSegmentByteSize(dstRR, tRegVar->getExecSize()) <=
              getByteSize(tRegVar));
      itsDisp = getSegmentDisp(dstRR, tRegVar->getExecSize());
    } else if (regVar->isRegVarFill()) {
      G4_RegVarTransient *tRegVar = static_cast<G4_RegVarTransient *>(regVar);
      auto srcRR = tRegVar->getRepRegion()->asSrcRegRegion();
      vASSERT(getSegmentByteSize(srcRR, tRegVar->getExecSize()) <=
              getByteSize(tRegVar));
      itsDisp = getSegmentDisp(srcRR, tRegVar->getExecSize());
    } else {
      vISA_ASSERT(false, "Incorrect spill/fill ranges.");
      itsDisp = 0;
    }

    regVar->setDisp(itsDisp);
  } else {
    // Allocate the spill and evaluate its disp
    if (doSpillSpaceCompression) {
      vASSERT(regVar->isRegVarTransient() == false);
      if (spilledLSLRs_ != nullptr) {
        regVar->setDisp(calculateSpillDispForLS(regVar));
      } else {
        regVar->setDisp(calculateSpillDisp(regVar));
      }
    } else {
      vASSERT(regVar->isRegVarTransient() == false);
      if (regVar->getId() >= varIdCount_) {
        if (regVar->getBaseRegVar()->getDisp() != UINT_MAX) {
          regVar->setDisp(regVar->getBaseRegVar()->getDisp());
          return regVar->getDisp();
        }
      }

      if ((spillAreaOffset_) % builder_->numEltPerGRF<Type_UB>() != 0) {
        (spillAreaOffset_) =
            ROUND(spillAreaOffset_, builder_->numEltPerGRF<Type_UB>());
      }

      regVar->setDisp(spillAreaOffset_);
      spillAreaOffset_ += getByteSize(regVar);
    }
  }

  // ToDo: log this in some dump to help debug
  // regVar->getDeclare()->dump();
  // std::cerr << "spill offset = " << regVar->getDisp() << "\n";

  return regVar->getDisp();
}

// Get the spill/fill displacement of the region.
template <class REGION_TYPE>
unsigned SpillManagerGRF::getRegionDisp(REGION_TYPE *region) {
  return getDisp(getRegVar(region)) + getRegionOriginOffset(region);
}

// Get the type of send message to use to spill/fill the region.
// The type can be either on oword read/write or a scatter read/write.
// If the segment corresponding to the region is dword sized then a
// dword read/write is used else an oword read/write is used.
template <class REGION_TYPE>
unsigned SpillManagerGRF::getMsgType(REGION_TYPE *region,
                                     G4_ExecSize execSize) {
  unsigned regionDisp = getRegionDisp(region);
  unsigned regionByteSize = getRegionByteSize(region, execSize);
  if (owordAligned(regionDisp) && owordAligned(regionByteSize))
    return owordMask();
  else
    return getEncAlignedSegmentMsgType(region, execSize);
}

// Determine if the region is unaligned w.r.t spill/fill memory read/writes.
// If the exact region cannot be read/written from spill/fill memory using
// one send instruction, then it is unaligned.
template <class REGION_TYPE>
bool SpillManagerGRF::isUnalignedRegion(REGION_TYPE *region,
                                        G4_ExecSize execSize) {
  unsigned regionDisp = getRegionDisp(region);
  unsigned regionByteSize = getRegionByteSize(region, execSize);

  bool needs32ByteAlign = useScratchMsg_;
  needs32ByteAlign |= gra.useLscForSpillFill;

  auto bytePerGRF = builder_->numEltPerGRF<Type_UB>();
  if (needs32ByteAlign) {
    if (regionDisp % bytePerGRF == 0 && regionByteSize % bytePerGRF == 0) {
      return regionByteSize / bytePerGRF != 1 &&
             regionByteSize / bytePerGRF != 2 &&
             regionByteSize / bytePerGRF != 4;
    } else
      return true;
  } else {
    if (owordAligned(regionDisp) && owordAligned(regionByteSize)) {
      //  Current intrinsic spill/fill cannot handle partial region spill.
      //  If it's the partial region of a large size variable, such as V91 in
      //  following instructions, the preload is needed. mov (16) V91(6,0)<1>:ub
      //  %retval_ub(0,0)<1;1,0>:ub {H1, Align1} mov (16) V91(6,16)<1>:ub
      //  %retval_ub(0,16)<1;1,0>:ub {H1, Align1}
      G4_RegVar *var = getRegVar(region);
      if ((var->getDeclare()->getByteSize() > bytePerGRF) &&
          (regionByteSize < bytePerGRF || regionDisp % bytePerGRF)) {
        return true;
      }
      return regionByteSize / OWORD_BYTE_SIZE != 1 &&
             regionByteSize / OWORD_BYTE_SIZE != 2 &&
             regionByteSize / OWORD_BYTE_SIZE != 4;
    } else
      return true;
  }
}

// Calculate the smallest aligned segment encompassing the region.
template <class REGION_TYPE>
void SpillManagerGRF::calculateEncAlignedSegment(REGION_TYPE *region,
                                                 G4_ExecSize execSize,
                                                 unsigned &start, unsigned &end,
                                                 unsigned &type) {
  unsigned regionDisp = getRegionDisp(region);
  unsigned regionByteSize = getRegionByteSize(region, execSize);

  if (needGRFAlignedOffset()) {
    unsigned hwordLB = regionDisp & grfMask();
    unsigned hwordRB = hwordLB + builder_->numEltPerGRF<Type_UB>();
    unsigned blockSize = builder_->numEltPerGRF<Type_UB>();

    while (regionDisp + regionByteSize > hwordRB) {
      hwordRB += blockSize;
    }

    vASSERT((hwordRB - hwordLB) / builder_->numEltPerGRF<Type_UB>() <= 4);
    start = hwordLB;
    end = hwordRB;
    type = grfMask();
  } else {
    unsigned owordLB = regionDisp & owordMask();
    unsigned owordRB = owordLB + OWORD_BYTE_SIZE;
    unsigned blockSize = OWORD_BYTE_SIZE;

    while (regionDisp + regionByteSize > owordRB) {
      owordRB += blockSize;
      blockSize *= 2;
    }

    vASSERT((owordRB - owordLB) / builder_->numEltPerGRF<Type_UB>() <= 4);
    start = owordLB;
    end = owordRB;
    type = owordMask();
  }
}

// Get the byte size of the aligned segment for the region.

template <class REGION_TYPE>
unsigned SpillManagerGRF::getEncAlignedSegmentByteSize(REGION_TYPE *region,
                                                       G4_ExecSize execSize) {
  unsigned start, end, type;
  calculateEncAlignedSegment(region, execSize, start, end, type);
  return end - start;
}

// Get the start offset of the aligned segment for the region.
template <class REGION_TYPE>
unsigned SpillManagerGRF::getEncAlignedSegmentDisp(REGION_TYPE *region,
                                                   G4_ExecSize execSize) {
  unsigned start, end, type;
  calculateEncAlignedSegment(region, execSize, start, end, type);
  return start;
}

// Get the type of message to be used to read/write the enclosing aligned
// segment for the region.
template <class REGION_TYPE>
unsigned SpillManagerGRF::getEncAlignedSegmentMsgType(REGION_TYPE *region,
                                                      G4_ExecSize execSize) {
  unsigned start, end, type;
  calculateEncAlignedSegment(region, execSize, start, end, type);
  return type;
}

// Get the byte size of the segment for the region.
template <class REGION_TYPE>
unsigned SpillManagerGRF::getSegmentByteSize(REGION_TYPE *region,
                                             G4_ExecSize execSize) {
  vASSERT(region->getElemSize() && execSize);
  if (isUnalignedRegion(region, execSize))
    return getEncAlignedSegmentByteSize(region, execSize);
  else
    return getRegionByteSize(region, execSize);
}

// Get the byte size of the destination region.
unsigned SpillManagerGRF::getRegionByteSize(G4_DstRegRegion *region,
                                            G4_ExecSize execSize) const {
  unsigned size =
      region->getHorzStride() * region->getElemSize() * (execSize - 1) +
      region->getElemSize();

  return size;
}

// Get the byte size of the source region.

unsigned SpillManagerGRF::getRegionByteSize(G4_SrcRegRegion *region,
                                            G4_ExecSize execSize) const {
  vASSERT(execSize % region->getRegion()->width == 0);
  unsigned nRows = execSize / region->getRegion()->width;
  unsigned size = 0;

  for (unsigned int i = 0; i < nRows - 1; i++) {
    size += region->getRegion()->vertStride * region->getElemSize();
  }

  size += region->getRegion()->horzStride * region->getElemSize() *
              (region->getRegion()->width - 1) +
          region->getElemSize();
  return size;
}

// Check if the instruction is a SIMD 16 or 32 instruction that is logically
// equivalent to two instructions the second of which uses register operands
// at the following row with the same sub-register index.
bool SpillManagerGRF::isComprInst(G4_INST *inst) const {
  return inst->isComprInst();
}

// Check if the source in a compressed instruction operand occupies a second
// register.
bool SpillManagerGRF::isMultiRegComprSource(G4_SrcRegRegion *src,
                                            G4_INST *inst) const {
  if (!inst->isComprInst()) {
    return false;
  } else if (src->isScalar()) {
    return false;
  } else if (inst->getExecSize() <= 8) {
    return false;
  } else if (!src->asSrcRegRegion()->crossGRF(*builder_)) {
    return false;
  } else if (inst->getExecSize() == 16 && inst->getDst() &&
             inst->getDst()->getTypeSize() == 4 &&
             inst->getDst()->getHorzStride() == 1) {
    if (src->getTypeSize() == 2 && src->isNativePackedRegion()) {
      return false;
    } else {
      return true;
    }
  } else {
    return true;
  }
}

// Send message information query
unsigned SpillManagerGRF::getSendMaxResponseLength() const {
  return 8;
}


// Send message information query
#define SEND_GT_MAX_MESSAGE_LENGTH 15
unsigned SpillManagerGRF::getSendMaxMessageLength() const {
  return SEND_GT_MAX_MESSAGE_LENGTH;
}

// Send message information query
unsigned SpillManagerGRF::getSendDescDataSizeBitOffset() {
  return SEND_GT_DESC_DATA_SIZE_BIT_OFFSET;
}

// Send message information query
unsigned SpillManagerGRF::getSendReadTypeBitOffset() const {
  return SEND_IVB_MSG_TYPE_BIT_OFFSET;
}

// Send message information query
unsigned SpillManagerGRF::getSendWriteTypeBitOffset() {
  return SEND_IVB_MSG_TYPE_BIT_OFFSET;
}

// Send message information query
unsigned SpillManagerGRF::getSendScReadType() const {
  return SEND_IVB_SC_READ_TYPE;
}

// Send message information query
unsigned SpillManagerGRF::getSendScWriteType() const {
  return SEND_IVB_SC_WRITE_TYPE;
}

// Send message information query
unsigned SpillManagerGRF::getSendOwordReadType() const {
  return SEND_IVB_OW_READ_TYPE;
}

// Send message information query
unsigned SpillManagerGRF::getSendOwordWriteType() {
  return SEND_IVB_OW_WRITE_TYPE;
}

unsigned SpillManagerGRF::getSendExDesc(bool isWrite, bool isScatter) const {
  return isWrite ? SEND_IVB_DP_WR_EX_DESC_IMM : SEND_IVB_DP_RD_EX_DESC_IMM;
}

bool SpillManagerGRF::useSplitSend() const { return builder_->useSends(); }

// Get a unique spill range index.
unsigned SpillManagerGRF::getSpillIndex() {
  return spillRangeCount_++;
}

// Get a unique fill range index.
unsigned SpillManagerGRF::getFillIndex() {
  return fillRangeCount_++;
}

// Get a unique tmp index.
unsigned SpillManagerGRF::getTmpIndex() {
  return tmpRangeCount_++;
}

// Get a unique msg spill index.
unsigned SpillManagerGRF::getMsgSpillIndex() {
  return msgSpillRangeCount_++;
}

// Get a unique msg fill index.
unsigned SpillManagerGRF::getMsgFillIndex() {
  return msgFillRangeCount_++;
}

// Get a unique addr spill/fill index.
unsigned SpillManagerGRF::getAddrSpillFillIndex() {
  return addrSpillFillRangeCount_++;
}

// Create a declare directive for a new live range (spill/fill/msg)
// introduced as part of the spill code generation.
G4_Declare *SpillManagerGRF::createRangeDeclare(
    const char *name, G4_RegFileKind regFile, unsigned short nElems,
    unsigned short nRows, G4_Type type, DeclareType kind, G4_RegVar *base,
    G4_Operand *repRegion, G4_ExecSize execSize) {
  G4_Declare *rangeDeclare = builder_->createDeclare(
      name, regFile, nElems, nRows, type, kind, base, repRegion, execSize);
  // TODO: Remove call to setId() as ids are to be computed/set by LivenessAnalysis
  if (!gra.incRA.isEnabled())
    rangeDeclare->getRegVar()->setId(varIdCount_ + latestImplicitVarIdCount_++);
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
G4_Declare *SpillManagerGRF::createTransientGRFRangeDeclare(
    REGION_TYPE *region, bool isFill, unsigned index, G4_ExecSize execSize,
    G4_INST *inst) {
  const char *nameStr = isFill ? "FL_%s_%d" : "SP_%s_%d";
  const char *name = gra.builder.getNameString(
      64, nameStr, getRegVar(region)->getName(), index);
  G4_Type type = region->getType();
  unsigned segmentByteSize = getSegmentByteSize(region, execSize);
  DeclareType regVarKind =
      (region->isDstRegRegion()) ? DeclareType::Spill : DeclareType::Fill;
  unsigned short width, height;

  if (segmentByteSize > builder_->numEltPerGRF<Type_UB>() ||
      region->crossGRF(*builder_)) {
    vASSERT(builder_->numEltPerGRF<Type_UB>() % region->getElemSize() == 0);
    width = builder_->numEltPerGRF<Type_UB>() / region->getElemSize();
    height = 2;
  } else {
    vASSERT(segmentByteSize % region->getElemSize() == 0);
    width = segmentByteSize / region->getElemSize();
    height = 1;
  }

  if (needGRFAlignedOffset()) {
    // the message will read/write a minimum of one GRF
    if (height == 1 && width < (builder_->getGRFSize() / region->getElemSize()))
      width = builder_->getGRFSize() / region->getElemSize();
  }

  G4_Declare *transientRangeDeclare =
      createRangeDeclare(name, G4_GRF, width, height, type, regVarKind,
                         region->getBase()->asRegVar(), region, execSize);

  if (failSafeSpill_) {
    if (!gra.kernel.getOption(vISA_NewFailSafeRA)) {
      transientRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(spillRegOffset_), 0);
      spillRegOffset_ += height;
    } else {
      transientRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(context.getFreeGRF(height)), 0);
    }
  }

  // FIXME: We should take the original declare's alignment too, but I'm worried
  // we may get perf regression if FE is over-aligning or the alignment is not
  // necessary for this inst. So Either is used for now and we can change it
  // later if there are bugs
  setNewDclAlignment(gra, transientRangeDeclare, false);
  return transientRangeDeclare;
}

static unsigned short getSpillRowSizeForSendDst(G4_INST *inst) {
  vASSERT(inst);
  unsigned short nRows = 0;
  const IR_Builder &builder = inst->getBuilder();
  auto dst = inst->getDst();

  if (inst->isSend()) {
    G4_SendDesc *msgDesc = inst->getMsgDesc();
    nRows = msgDesc->getDstLenRegs();
    if (dst->getTopDcl()->getByteSize() <= inst->getBuilder().getGRFSize()) {
      // we may have a send that that writes to a <1 GRF variable, but due to
      // A64 message requirements the send has a response length > 1. We return
      // row size as one instead as we've only allocated one GRF for the spilled
      // variable in scratch space
      nRows = 1;
    }
  } else {
    vASSERT(dst->getLinearizedStart() % builder.numEltPerGRF<Type_UB>() == 0);
    unsigned int rangeSize = (dst->getLinearizedEnd() - dst->getLinearizedStart() + 1);
    unsigned int grfSize = builder.numEltPerGRF<Type_UB>();
    nRows = (rangeSize / grfSize) + ((rangeSize % grfSize) == 0 ? 0 : 1);
  }
  return nRows;
}

// Create a regvar and its declare directive to represent the spill live
// range that appears as a send instruction post destination GRF.
// The type of the regvar is set as dword and its width 8. The type of
// the post destination does not matter, so we just use type dword, and
// a width of 8 so that a row corresponds to a physical register.
G4_Declare *SpillManagerGRF::createPostDstSpillRangeDeclare(G4_INST *sendOut) {
  auto dst = sendOut->getDst();
  G4_RegVar *spilledRegVar = getRegVar(dst);
  const char *name =
      gra.builder.getNameString(64, "SP_GRF_%s_%d", spilledRegVar->getName(),
                                getSpillIndex());
  unsigned short nRows = getSpillRowSizeForSendDst(sendOut);

  G4_DstRegRegion *normalizedPostDst =
      builder_->createDst(spilledRegVar, dst->getRegOff(), SUBREG_ORIGIN,
                          DEF_HORIZ_STRIDE, Type_UD);

  // We use the width as the user specified, the height however is
  // calculated based on the message descriptor to limit register
  // pressure induced by the spill range.

  G4_Declare *transientRangeDeclare = createRangeDeclare(
      name, G4_GRF, builder_->numEltPerGRF<Type_UD>(), nRows, Type_UD,
      DeclareType::Spill, spilledRegVar, normalizedPostDst,
      G4_ExecSize(builder_->numEltPerGRF<Type_UD>()));

  if (failSafeSpill_) {
    if (!builder_->getOption(vISA_NewFailSafeRA)) {
      if (useSplitSend()) {
        transientRangeDeclare->getRegVar()->setPhyReg(
            builder_->phyregpool.getGreg(spillRegStart_), 0);
        spillRegOffset_ += nRows;
      } else {
        transientRangeDeclare->getRegVar()->setPhyReg(
            builder_->phyregpool.getGreg(spillRegStart_ + 1), 0);
        spillRegOffset_ += nRows + 1;
      }
    } else {
      vISA_ASSERT(useSplitSend(), "split send disabled");
      transientRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(context.getFreeGRF(nRows)), 0);
    }
  }

  return transientRangeDeclare;
}

// Create a regvar and its declare directive to represent the spill live range.
G4_Declare *
SpillManagerGRF::createSpillRangeDeclare(G4_DstRegRegion *spilledRegion,
                                         G4_ExecSize execSize, G4_INST *inst) {
  return createTransientGRFRangeDeclare(spilledRegion, false /*isFill*/,
                                        getSpillIndex(), execSize, inst);
}

// Create a regvar and its declare directive to represent the GRF fill live
// range.
G4_Declare *SpillManagerGRF::createGRFFillRangeDeclare(
    G4_SrcRegRegion *fillRegion, G4_ExecSize execSize, G4_INST *inst) {
  vASSERT(getRFType(fillRegion) == G4_GRF);
  G4_Declare *fillRangeDecl = createTransientGRFRangeDeclare(
      fillRegion, true /*isFill*/, getFillIndex(), execSize, inst);
  return fillRangeDecl;
}

static unsigned short getSpillRowSizeForSendSrc(G4_INST *inst,
                                                G4_SrcRegRegion *filledRegion) {
  vASSERT(inst);
  const IR_Builder &builder = inst->getBuilder();
  unsigned short nRows = 0;

  if (inst->isSend()) {
    G4_SendDesc *msgDesc = inst->getMsgDesc();
    if (inst->isSplitSend() &&
        (inst->getSrc(1)->asSrcRegRegion() == filledRegion)) {
      nRows = msgDesc->getSrc1LenRegs();
    } else {
      nRows = msgDesc->getSrc0LenRegs();
    }
  } else {
    unsigned int rangeSize = filledRegion->getLinearizedEnd() -
                             filledRegion->getLinearizedStart() + 1;
    unsigned int grfSize = builder.numEltPerGRF<Type_UB>();
    nRows = (rangeSize / grfSize) + ((rangeSize % grfSize) == 0 ? 0 : 1);
  }

  return nRows;
}

// Create a regvar and its declare directive to represent the GRF fill live
// range.
G4_Declare *
SpillManagerGRF::createSendFillRangeDeclare(G4_SrcRegRegion *filledRegion,
                                            G4_INST *sendInst) {
  G4_RegVar *filledRegVar = getRegVar(filledRegion);
  const char *name = gra.builder.getNameString(
      64, "FL_Send_%s_%d", filledRegVar->getName(), getFillIndex());
  unsigned short nRows = getSpillRowSizeForSendSrc(sendInst, filledRegion);

  G4_SrcRegRegion *normalizedSendSrc = builder_->createSrcRegRegion(
      filledRegion->getModifier(), Direct, filledRegVar,
      filledRegion->getRegOff(), filledRegion->getSubRegOff(),
      filledRegion->getRegion(), filledRegion->getType());
  unsigned short width =
      builder_->numEltPerGRF<Type_UB>() / filledRegion->getElemSize();
  vASSERT(builder_->numEltPerGRF<Type_UB>() % filledRegion->getElemSize() == 0);

  // We use the width as the user specified, the height however is
  // calculated based on the message descriptor to limit register
  // pressure induced by the spill range.

  G4_Declare *transientRangeDeclare = createRangeDeclare(
      name, G4_GRF, width, nRows, filledRegion->getType(), DeclareType::Fill,
      filledRegVar, normalizedSendSrc, G4_ExecSize(width));

  setNewDclAlignment(gra, transientRangeDeclare,
                     filledRegVar->getDeclare()->isEvenAlign());

  if (failSafeSpill_) {
    if (!builder_->getOption(vISA_NewFailSafeRA)) {
      if (sendInst->isEOT() && builder_->hasEOTGRFBinding()) {
        // make sure eot src is in last 16 GRF
        uint32_t eotStart = gra.kernel.getNumRegTotal() - 16;
        if (spillRegOffset_ < eotStart) {
          spillRegOffset_ = eotStart;
        }
      }
      transientRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(spillRegOffset_), 0);
      spillRegOffset_ += nRows;
    } else {
      bool isEOT = sendInst->isEOT() && builder_->hasEOTGRFBinding();
      unsigned int startGRF =
          isEOT ? gra.kernel.getNumRegTotal() - 16 : BoundedRA::NOT_FOUND;
      transientRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(context.getFreeGRF(nRows, startGRF)), 0);
    }
  }

  return transientRangeDeclare;
}

// Create a regvar and its declare directive to represent the temporary live
// range.
G4_Declare *
SpillManagerGRF::createTemporaryRangeDeclare(G4_DstRegRegion *spilledRegion,
                                             G4_ExecSize execSize,
                                             bool forceSegmentAlignment) {
  const char *name = gra.builder.getNameString(
      64, "TM_GRF_%s_%d", getRegVar(spilledRegion)->getName(), getTmpIndex());
  unsigned byteSize = (forceSegmentAlignment)
                          ? getSegmentByteSize(spilledRegion, execSize)
                          : getRegionByteSize(spilledRegion, execSize);

  // ensure tmp reg is large enough to hold all data when sub-reg offset is
  // non-zero
  byteSize += spilledRegion->getSubRegOff() * spilledRegion->getElemSize();

  vASSERT(byteSize <= 4u * builder_->numEltPerGRF<Type_UB>());
  vASSERT(byteSize % spilledRegion->getElemSize() == 0);

  G4_Type type = spilledRegion->getType();
  DeclareType regVarKind = DeclareType::Tmp;

  unsigned short width, height;
  if (byteSize > (2 * builder_->numEltPerGRF<Type_UB>())) {
    height = 4;
    width = builder_->numEltPerGRF<Type_UB>() / spilledRegion->getElemSize();
  } else if (byteSize > builder_->numEltPerGRF<Type_UB>()) {
    height = 2;
    width = builder_->numEltPerGRF<Type_UB>() / spilledRegion->getElemSize();
  } else {
    height = 1;
    width = byteSize / spilledRegion->getElemSize();
  }

  G4_RegVar *spilledRegVar = getRegVar(spilledRegion);

  G4_Declare *temporaryRangeDeclare =
      createRangeDeclare(name, G4_GRF, width, height, type, regVarKind,
                         spilledRegVar, NULL, G4_ExecSize(0));

  if (failSafeSpill_) {
    if (!builder_->getOption(vISA_NewFailSafeRA)) {
      temporaryRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(spillRegOffset_), 0);
      spillRegOffset_ += height;
    } else {
      temporaryRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(context.getFreeGRF(height)), 0);
    }
  }

  setNewDclAlignment(gra, temporaryRangeDeclare, false);
  return temporaryRangeDeclare;
}

// Create a destination region that could be used in place of the spill regvar.
// If the region is unaligned then the origin of the destination region
// is the displacement of the orginal region from its segment, else the
// origin is 0.
G4_DstRegRegion *SpillManagerGRF::createSpillRangeDstRegion(
    G4_RegVar *spillRangeRegVar, G4_DstRegRegion *spilledRegion,
    G4_ExecSize execSize, unsigned regOff) {
  if (isUnalignedRegion(spilledRegion, execSize)) {
    unsigned segmentDisp = getEncAlignedSegmentDisp(spilledRegion, execSize);
    unsigned regionDisp = getRegionDisp(spilledRegion);
    vASSERT(regionDisp >= segmentDisp);
    unsigned short subRegOff =
        (regionDisp - segmentDisp) / spilledRegion->getElemSize();
    vASSERT((regionDisp - segmentDisp) % spilledRegion->getElemSize() == 0);
    vASSERT(subRegOff * spilledRegion->getElemSize() +
                getRegionByteSize(spilledRegion, execSize) <=
            2u * builder_->numEltPerGRF<Type_UB>());

    if (useScratchMsg_) {
      G4_Declare *parent_dcl =
          spilledRegion->getBase()->asRegVar()->getDeclare();
      unsigned off = 0;
      while (parent_dcl->getAliasDeclare() != NULL) {
        // off is in bytes
        off += parent_dcl->getAliasOffset();
        parent_dcl = parent_dcl->getAliasDeclare();
      }
      off = off % builder_->numEltPerGRF<Type_UB>();
      // sub-regoff is in units of element size
      subRegOff =
          spilledRegion->getSubRegOff() + off / spilledRegion->getElemSize();
    }

    return builder_->createDst(spillRangeRegVar, (unsigned short)regOff,
                               subRegOff, spilledRegion->getHorzStride(),
                               spilledRegion->getType());
  }

  else {
    return builder_->createDst(spillRangeRegVar, (short)regOff, SUBREG_ORIGIN,
                               spilledRegion->getHorzStride(),
                               spilledRegion->getType());
  }
}

// Create a source region that could be used to copy out the temporary range
// (that was created to replace the portion of the spilled live range appearing
// in an instruction destination) into the segment aligned spill range for the
// spilled live range that can be written out to spill memory.
G4_SrcRegRegion *SpillManagerGRF::createTemporaryRangeSrcRegion(
    G4_RegVar *tmpRangeRegVar, G4_DstRegRegion *spilledRegion,
    G4_ExecSize execSize, unsigned regOff) {
  uint16_t horzStride = spilledRegion->getHorzStride();
  // A scalar region is returned when execsize is 1.
  const RegionDesc *rDesc =
      builder_->createRegionDesc(execSize, horzStride, 1, 0);

  return builder_->createSrc(tmpRangeRegVar, (short)regOff,
                             spilledRegion->getSubRegOff(), rDesc,
                             spilledRegion->getType());
}

// Create a source region that could be used in place of the fill regvar.
// If the region is unaligned then the origin of the destination region
// is the displacement of the orginal region from its segment, else the
// origin is 0.
G4_SrcRegRegion *
SpillManagerGRF::createFillRangeSrcRegion(G4_RegVar *fillRangeRegVar,
                                          G4_SrcRegRegion *filledRegion,
                                          G4_ExecSize execSize) {
  // we need to preserve accRegSel if it's set
  if (isUnalignedRegion(filledRegion, execSize)) {
    unsigned segmentDisp = getEncAlignedSegmentDisp(filledRegion, execSize);
    unsigned regionDisp = getRegionDisp(filledRegion);
    vASSERT(regionDisp >= segmentDisp);
    unsigned short subRegOff =
        (regionDisp - segmentDisp) / filledRegion->getElemSize();
    vASSERT((regionDisp - segmentDisp) % filledRegion->getElemSize() == 0);

    return builder_->createSrcRegRegion(
        filledRegion->getModifier(), Direct, fillRangeRegVar, REG_ORIGIN,
        subRegOff, filledRegion->getRegion(), filledRegion->getType(),
        filledRegion->getAccRegSel());
  } else {
    // fill intrinsic's sub-reg offset is always 0 since it is GRF aligned.
    // but original filled range's offset may not be 0, so actual filled
    // src needs to use sub-reg offset from original region.
    return builder_->createSrcRegRegion(
        filledRegion->getModifier(), Direct, fillRangeRegVar, REG_ORIGIN,
        filledRegion->getSubRegOff(), filledRegion->getRegion(),
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
G4_SrcRegRegion *SpillManagerGRF::createBlockSpillRangeSrcRegion(
    G4_RegVar *spillRangeRegVar, unsigned regOff, unsigned subregOff) {
  vASSERT(getByteSize(spillRangeRegVar) % DWORD_BYTE_SIZE == 0);
  const RegionDesc *rDesc =
      builder_->rgnpool.createRegion(DWORD_BYTE_SIZE, DWORD_BYTE_SIZE, 1);
  return builder_->createSrc(spillRangeRegVar, (short)regOff, (short)subregOff,
                             rDesc, Type_UD);
}

std::optional<G4_Declare *>
SpillManagerGRF::getPreDefinedMRangeDeclare() const {
  if (useSplitSend() && useScratchMsg_)
    return builder_->getBuiltinR0();
  if (gra.useLscForSpillFill)
    // TODO: I think we can remove this, as if useLscForSpillFill is true
    // useScratchMsg_ should also be true.
    return nullptr;
  if (builder_->usesStack())
    return gra.kernel.fg.scratchRegDcl;
  return std::nullopt;
}

// Create a GRF regvar and a declare directive for it, to represent an
// implicit MFR live range that will be used as the send message payload
// header and write payload for spilling a regvar to memory.
G4_Declare *SpillManagerGRF::createMRangeDeclare(G4_RegVar *regVar) {

  auto maybeDcl = getPreDefinedMRangeDeclare();
  if (maybeDcl)
    return *maybeDcl;

  // For legacy platforms where we can neither use LSC nor the scratch message
  // for spill/fill.
  G4_RegVar *repRegVar =
      (regVar->isRegVarTransient()) ? regVar->getBaseRegVar() : regVar;
  const char *name = gra.builder.getNameString(
      64, "SP_MSG_%s_%d", repRegVar->getName(), getMsgSpillIndex());

  unsigned short height = 1;
  if (!useSplitSend()) {
    unsigned regVarByteSize = getByteSize(regVar);
    unsigned writePayloadHeight =
        cdiv(regVarByteSize, builder_->numEltPerGRF<Type_UB>());

    if (writePayloadHeight > SPILL_PAYLOAD_HEIGHT_LIMIT) {
      writePayloadHeight = SPILL_PAYLOAD_HEIGHT_LIMIT;
    }
    unsigned payloadHeaderHeight = (regVarByteSize != DWORD_BYTE_SIZE)
                                       ? OWORD_PAYLOAD_HEADER_MAX_HEIGHT
                                       : DWORD_PAYLOAD_HEADER_MAX_HEIGHT;
    height = payloadHeaderHeight + writePayloadHeight;
    // We should not find ourselves using dword scattered write
    if (useScratchMsg_) {
      vASSERT(payloadHeaderHeight != DWORD_PAYLOAD_HEADER_MAX_HEIGHT);
    }
  }
  unsigned short width = builder_->numEltPerGRF<Type_UD>();

  G4_Declare *msgRangeDeclare = createRangeDeclare(
      name, G4_GRF, width, height, Type_UD, DeclareType::Tmp,
      regVar->getNonTransientBaseRegVar(), NULL, G4_ExecSize(0));

  if (failSafeSpill_) {
    if (!builder_->getOption(vISA_NewFailSafeRA)) {
      msgRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(spillRegStart_), 0);
    } else {
      msgRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(context.getFreeGRF(height)), 0);
    }
  }

  return msgRangeDeclare;
}

// Create a GRF regvar and a declare directive for it, to represent an
// implicit live range that will be used as the send message payload
// header and write payload for spilling a regvar region to memory.
G4_Declare *SpillManagerGRF::createMRangeDeclare(G4_DstRegRegion *region,
                                                 G4_ExecSize execSize) {
  auto maybeDcl = getPreDefinedMRangeDeclare();
  if (maybeDcl)
    return *maybeDcl;

  // For legacy platforms where we can neither use LSC nor the scratch message
  // for spill/fill.
  const char *name = gra.builder.getNameString(
      64, "SP_MSG_%s_%d", getRegVar(region)->getName(), getMsgSpillIndex());
  unsigned short height = 1;
  if (!useSplitSend()) {
    unsigned regionByteSize = getSegmentByteSize(region, execSize);
    unsigned writePayloadHeight =
        cdiv(regionByteSize, builder_->numEltPerGRF<Type_UB>());
    unsigned msgType = getMsgType(region, execSize);
    unsigned payloadHeaderHeight =
        (msgType == owordMask() || msgType == hwordMask())
            ? OWORD_PAYLOAD_HEADER_MAX_HEIGHT
            : DWORD_PAYLOAD_HEADER_MAX_HEIGHT;
    // We should not find ourselves using dword scattered write
    if (useScratchMsg_) {
      vASSERT(payloadHeaderHeight != DWORD_PAYLOAD_HEADER_MAX_HEIGHT);
    }
    height = payloadHeaderHeight + writePayloadHeight;
  }
  unsigned short width = builder_->numEltPerGRF<Type_UD>();
  G4_Declare *msgRangeDeclare =
      createRangeDeclare(name, G4_GRF, width, height, Type_UD, DeclareType::Tmp,
                         region->getBase()->asRegVar(), NULL, G4_ExecSize(0));

  if (failSafeSpill_) {
    if (!builder_->getOption(vISA_NewFailSafeRA)) {
      msgRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(spillRegOffset_), 0);
      spillRegOffset_ += height;
    } else {
      msgRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(context.getFreeGRF(height)), 0);
    }
  }

  return msgRangeDeclare;
}

// Create a GRF regvar and a declare directive for it, that will be used as
// the send message payload header and write payload for filling a regvar
// from memory.

G4_Declare *SpillManagerGRF::createMRangeDeclare(G4_SrcRegRegion *region,
                                                 G4_ExecSize execSize) {
  auto maybeDcl = getPreDefinedMRangeDeclare();
  if (maybeDcl)
    return *maybeDcl;

  // For legacy platforms where we can neither use LSC nor the scratch message
  // for spill/fill.
  const char *name = gra.builder.getNameString(
      64, "FL_MSG_%s_%d", getRegVar(region)->getName(), getMsgFillIndex());
  getSegmentByteSize(region, execSize);
  unsigned payloadHeaderHeight = (getMsgType(region, execSize) == owordMask())
                                     ? OWORD_PAYLOAD_HEADER_MIN_HEIGHT
                                     : DWORD_PAYLOAD_HEADER_MIN_HEIGHT;

  // We should not find ourselves using dword scattered write
  if (useScratchMsg_) {
    vASSERT(payloadHeaderHeight != DWORD_PAYLOAD_HEADER_MAX_HEIGHT);
    // When using scratch msg descriptor we don't need to use a
    // separate GRF for payload. Source operand of send can directly
    // use r0.0.
    return builder_->getBuiltinR0();
  }

  unsigned height = payloadHeaderHeight;
  unsigned width = builder_->numEltPerGRF<Type_UD>();
  G4_Declare *msgRangeDeclare = createRangeDeclare(
      name, G4_GRF, (unsigned short)width, (unsigned short)height, Type_UD,
      DeclareType::Tmp, region->getBase()->asRegVar(), NULL, G4_ExecSize(0));

  if (failSafeSpill_) {
    if (!builder_->getOption(vISA_NewFailSafeRA)) {
      msgRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(spillRegOffset_), 0);
      spillRegOffset_ += height;
    } else {
      msgRangeDeclare->getRegVar()->setPhyReg(
          builder_->phyregpool.getGreg(context.getFreeGRF(height)), 0);
    }
  }

  return msgRangeDeclare;
}

// Create a destination region for the GRF regvar for the write payload
// portion of the oword block send message (used for spill). The exec size
// can be either 4 or 8 for a regular 2 cycle instruction detination spills or
// 16 for simd16 instruction destination spills.
G4_DstRegRegion *SpillManagerGRF::createMPayloadBlockWriteDstRegion(
    G4_RegVar *grfRange, unsigned regOff, unsigned subregOff) {
  regOff += OWORD_PAYLOAD_WRITE_REG_OFFSET;
  subregOff += OWORD_PAYLOAD_WRITE_SUBREG_OFFSET;
  return builder_->createDst(grfRange, (short)regOff, (short)subregOff,
                             DEF_HORIZ_STRIDE, Type_UD);
}

// Create a destination region for the GRF regvar for the input header
// payload portion of the send message to the data port. The exec size
// needs to be 8 for the mov instruction that uses this as a destination.
G4_DstRegRegion *
SpillManagerGRF::createMHeaderInputDstRegion(G4_RegVar *grfRange,
                                             unsigned subregOff) {
  return builder_->createDst(grfRange, PAYLOAD_INPUT_REG_OFFSET,
                             (short)subregOff, DEF_HORIZ_STRIDE, Type_UD);
}

// Create a destination region for the GRF regvar for the payload offset
// portion of the oword block send message. The exec size needs to be 1
// for the mov instruction that uses this as a destination.
G4_DstRegRegion *
SpillManagerGRF::createMHeaderBlockOffsetDstRegion(G4_RegVar *grfRange) {
  return builder_->createDst(grfRange, OWORD_PAYLOAD_SPOFFSET_REG_OFFSET,
                             OWORD_PAYLOAD_SPOFFSET_SUBREG_OFFSET,
                             DEF_HORIZ_STRIDE, Type_UD);
}

// Create a source region for the input payload (r0.0). The exec size
// needs to be 8 for the mov instruction that uses this as a source.
G4_SrcRegRegion *SpillManagerGRF::createInputPayloadSrcRegion() {
  G4_RegVar *inputPayloadDirectReg = builder_->getBuiltinR0()->getRegVar();
  const RegionDesc *rDesc = builder_->rgnpool.createRegion(
      builder_->numEltPerGRF<Type_UD>(), builder_->numEltPerGRF<Type_UD>(),
      DEF_HORIZ_STRIDE);
  return builder_->createSrc(inputPayloadDirectReg, PAYLOAD_INPUT_REG_OFFSET,
                             PAYLOAD_INPUT_SUBREG_OFFSET, rDesc, Type_UD);
}

// Create and initialize the message header for the send instruction for
// save/load of value to/from memory.
// The header includes the input payload and the offset (for spill disp).
template <class REGION_TYPE>
G4_Declare *SpillManagerGRF::createAndInitMHeader(REGION_TYPE *region,
                                                  G4_ExecSize execSize) {
  G4_Declare *mRangeDcl = createMRangeDeclare(region, execSize);
  return initMHeader(mRangeDcl, region, execSize);
}

// Initialize the message header for the send instruction for save/load
// of value to/from memory.
// The header includes the input payload and the offset (for spill disp).
template <class REGION_TYPE>
G4_Declare *SpillManagerGRF::initMHeader(G4_Declare *mRangeDcl,
                                         REGION_TYPE *region,
                                         G4_ExecSize execSize) {
  // Initialize the message header with the input payload.
  if ((useScratchMsg_ && mRangeDcl == builder_->getBuiltinR0()) ||
      !headerNeeded()) {
    // mRangeDcl is NULL for fills
    return mRangeDcl;
  }

  G4_DstRegRegion *mHeaderInputDstRegion =
      createMHeaderInputDstRegion(mRangeDcl->getRegVar());
  G4_SrcRegRegion *inputPayload = createInputPayloadSrcRegion();
  createMovInst(G4_ExecSize(builder_->numEltPerGRF<Type_UD>()),
                mHeaderInputDstRegion, inputPayload);

  if (useScratchMsg_) {
    // Initialize msg header when region is a spill
    // When using scratch msg description, we only need to copy
    // r0.0 in to msg header. Memory offset will be
    // specified in the msg descriptor.
  } else {
    // Initialize the message header with the spill disp for block
    // read/write.
    G4_DstRegRegion *mHeaderOffsetDstRegion =
        createMHeaderBlockOffsetDstRegion(mRangeDcl->getRegVar());
    int offset = getSegmentDisp(region, execSize);
    getSpillOffset(offset);
    unsigned segmentDisp = offset / OWORD_BYTE_SIZE;
    G4_Imm *segmentDispImm = builder_->createImm(segmentDisp, Type_UD);

    if (!region->isSrcRegRegion() && !region->isDstRegRegion()) {
      vISA_ASSERT(false, ERROR_GRAPHCOLOR);
    }

    if (builder_->getIsKernel() == false) {
      createAddFPInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
    } else {
      createMovInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
    }
  }

  // Initialize the message header with the spill disp for scatter
  // read/write.
  return mRangeDcl;
}

// Create and initialize the message header for the send instruction.
// The header includes the input payload (for spill disp).
G4_Declare *SpillManagerGRF::createAndInitMHeader(G4_RegVar *regVar) {
  G4_Declare *mRangeDcl = createMRangeDeclare(regVar);
  return initMHeader(mRangeDcl);
}

// Initialize the message header for the send instruction.
// The header includes the input payload (for spill disp).
G4_Declare *SpillManagerGRF::initMHeader(G4_Declare *mRangeDcl) {
  // Initialize the message header with the input payload.
  if ((useScratchMsg_ && mRangeDcl == builder_->getBuiltinR0()) ||
      !headerNeeded()) {
    // mRangeDcl is NULL for fills
    return mRangeDcl;
  }

  G4_DstRegRegion *mHeaderInputDstRegion =
      createMHeaderInputDstRegion(mRangeDcl->getRegVar());
  G4_SrcRegRegion *inputPayload = createInputPayloadSrcRegion();
  createMovInst(G4_ExecSize(builder_->numEltPerGRF<Type_UD>()),
                mHeaderInputDstRegion, inputPayload);

  return mRangeDcl;
}

// Initialize the the write payload part of the message for spilled regvars.
// Either of the following restrictions for spillRangeDcl are assumed:
//        - the regvar element type is dword and its 2 <= width <= 8 and
//        height - regOff == 1
//        - the regvar element type is dword and its width = 8 and
//        2 <= height - regOff <= 8
//      - the regvar element type is dword and its width and height are 1
void SpillManagerGRF::initMWritePayload(G4_Declare *spillRangeDcl,
                                        G4_Declare *mRangeDcl, unsigned regOff,
                                        unsigned height) {
  if (useSplitSend()) {
    // no need for payload moves if using sends
    return;
  }

  // We use an block write when the spilled regvars's segment is greater
  // than a dword. Generate a mov to copy the oword aligned segment into
  // the write payload part of the message.
  {
    unsigned nRows = height;

    for (unsigned i = 0; i < nRows; i++) {
      G4_SrcRegRegion *spillRangeSrcRegion = createBlockSpillRangeSrcRegion(
          spillRangeDcl->getRegVar(), i + regOff);
      G4_DstRegRegion *mPayloadWriteDstRegion =
          createMPayloadBlockWriteDstRegion(mRangeDcl->getRegVar(), i);
      G4_ExecSize movExecSize =
          G4_ExecSize((nRows > 1) ? builder_->numEltPerGRF<Type_UD>()
                                  : spillRangeDcl->getNumElems());
      createMovInst(movExecSize, mPayloadWriteDstRegion, spillRangeSrcRegion);
    }
  }
}

// Initialize the the write payload part of the message for spilled regions.
void SpillManagerGRF::initMWritePayload(G4_Declare *spillRangeDcl,
                                        G4_Declare *mRangeDcl,
                                        G4_DstRegRegion *spilledRangeRegion,
                                        G4_ExecSize execSize, unsigned regOff) {
  // We use an block write when the spilled region's segment is greater
  // than a dword. Generate a mov to copy the oword aligned segment into
  // the write payload part of the message.
  if (useSplitSend()) {
    // no need for payload moves
    return;
  } else {
    G4_SrcRegRegion *spillRangeSrcRegion =
        createBlockSpillRangeSrcRegion(spillRangeDcl->getRegVar(), regOff);
    G4_DstRegRegion *mPayloadWriteDstRegion =
        createMPayloadBlockWriteDstRegion(mRangeDcl->getRegVar());
    unsigned segmentByteSize = getSegmentByteSize(spilledRangeRegion, execSize);
    G4_ExecSize movExecSize{segmentByteSize / DWORD_BYTE_SIZE};

    // Write entire GRF when using scratch msg descriptor
    if (useScratchMsg_) {
      if (movExecSize <= 8)
        movExecSize = g4::SIMD8;
      else if (movExecSize < g4::SIMD16)
        movExecSize = g4::SIMD16;
    }

    vASSERT(segmentByteSize % DWORD_BYTE_SIZE == 0);
    vASSERT(movExecSize <= g4::SIMD16);
    createMovInst(movExecSize, mPayloadWriteDstRegion, spillRangeSrcRegion);
  }
}

// Return the block size encoding for oword block reads.
unsigned SpillManagerGRF::blockSendBlockSizeCode(unsigned size) {
  auto code = GlobalRA::sendBlockSizeCode(size);
  return code << getSendDescDataSizeBitOffset();
}

// Return the block size encoding for dword scatter reads.
unsigned SpillManagerGRF::scatterSendBlockSizeCode(unsigned size) const {
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
    vISA_ASSERT_UNREACHABLE("invalid size");
    code = 0;
  }

  return code << getSendDescDataSizeBitOffset();
}

static uint32_t getScratchBlocksizeEncoding(int numGRF,
                                            const IR_Builder &builder) {

  int size = (numGRF * builder.getGRFSize()) / 32; // in HWwords
  unsigned blocksize_encoding = 0;
  if (size == 1) {
    blocksize_encoding = 0x0;
  } else if (size == 2) {
    blocksize_encoding = 0x1;
  } else if (size == 4) {
    blocksize_encoding = 0x2;
  } else if (size == 8) {
    vASSERT(builder.getPlatform() >= GENX_SKL);
    blocksize_encoding = 0x3;
  } else
    vASSERT(false);
  return blocksize_encoding;
}

std::tuple<uint32_t, G4_ExecSize>
SpillManagerGRF::createSpillSendMsgDescOWord(const IR_Builder &builder,
                                             unsigned int height) {
  unsigned segmentByteSize = height * builder.numEltPerGRF<Type_UB>();
  unsigned writePayloadCount =
      cdiv(segmentByteSize, builder.numEltPerGRF<Type_UB>());
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
  auto execSize =
      G4_ExecSize(LIMIT_SEND_EXEC_SIZE(segmentOwordSize * DWORD_BYTE_SIZE));

  return std::make_tuple(message, execSize);
}

// Create the message descriptor for a spill send instruction for spilled
// post destinations of send instructions.
G4_Imm *SpillManagerGRF::createSpillSendMsgDesc(unsigned regOff,
                                                unsigned height,
                                                G4_ExecSize &execSize,
                                                G4_RegVar *base) {
  unsigned message = 0;

  if (useScratchMsg_) {
    unsigned headerPresent = 0x80000;
    message = headerPresent;
    unsigned msgLength = useSplitSend()
                             ? SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT
                             : SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT + height;
    message |= (msgLength << getSendMsgLengthBitOffset());
    message |= (1 << SCRATCH_MSG_DESC_CATEORY);
    message |= (1 << SCRATCH_MSG_DESC_CHANNEL_MODE);
    message |= (1 << SCRATCH_MSG_DESC_OPERATION_MODE);
    unsigned blocksize_encoding =
        getScratchBlocksizeEncoding(height, *builder_);
    message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
    int offset = getDisp(base);
    getSpillOffset(offset);
    // message expects offsets to be in HWord
    message |= (offset + regOff * builder_->getGRFSize()) >>
               SCRATCH_SPACE_ADDRESS_UNIT;
    execSize = g4::SIMD16;
  } else {
    auto [message, retSize] = createSpillSendMsgDescOWord(*builder_, height);
    execSize = retSize;
  }
  return builder_->createImm(message, Type_UD);
}

// Create the message descriptor for a spill send instruction for spilled
// destination regions.
std::tuple<G4_Imm *, G4_ExecSize>
SpillManagerGRF::createSpillSendMsgDesc(G4_DstRegRegion *spilledRangeRegion,
                                        G4_ExecSize execSize) {
  unsigned message = 0;

  if (useScratchMsg_) {
    /*
    bits    description
    18:0    function control
    19    Header present
    24:20    Response length
    28:25    Message length
    31:29    MBZ

    18:0
    11:0    Offset (12b hword offset)
    13:12    Block size (00 - 1 register, 01 - 2 regs, 10 - reserved, 11 - 4
    regs) 14    MBZ 15    Invalidate after read (0 - no invalidate, 1 -
    invalidate) 16    Channel mode (0 - oword, 1 - dword) 17    Operation type
    (0 - read, 1 - write) 18    Category (1 - scratch block read/write)
    */
    unsigned segmentByteSize = getSegmentByteSize(spilledRangeRegion, execSize);
    unsigned writePayloadCount =
        cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
    unsigned headerPresent = 0x80000;
    message |= headerPresent;

    unsigned payloadHeaderCount = SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT;
    // message length = 1 if we are using sends, 1 + payload otherwise
    unsigned messageLength = useSplitSend()
                                 ? payloadHeaderCount
                                 : writePayloadCount + payloadHeaderCount;
    message |= (messageLength << getSendMsgLengthBitOffset());
    message |= (1 << SCRATCH_MSG_DESC_CATEORY);        // category
    message |= (1 << SCRATCH_MSG_DESC_CHANNEL_MODE);   // channel mode
    message |= (1 << SCRATCH_MSG_DESC_OPERATION_MODE); // write operation
    unsigned numGRFs = cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());

    unsigned blocksize_encoding =
        getScratchBlocksizeEncoding(numGRFs, *builder_);

    message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
    int offset = getRegionDisp(spilledRangeRegion);
    getSpillOffset(offset);
    message |= offset >> SCRATCH_SPACE_ADDRESS_UNIT;
    if (numGRFs > 1) {
      execSize = g4::SIMD16;
    } else {
      if (execSize > g4::SIMD8) {
        execSize = g4::SIMD16;
      } else {
        execSize = g4::SIMD8;
      }
    }
  } else {
    unsigned segmentByteSize = getSegmentByteSize(spilledRangeRegion, execSize);
    unsigned writePayloadCount =
        cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
    unsigned statelessSurfaceIndex = 0xFF;
    message = statelessSurfaceIndex;

    unsigned headerPresent = 0x80000;
    message |= headerPresent;
    unsigned messageType = getSendOwordWriteType();
    message |= messageType << getSendWriteTypeBitOffset();
    unsigned payloadHeaderCount = OWORD_PAYLOAD_HEADER_MAX_HEIGHT;
    unsigned messageLength = useSplitSend()
                                 ? payloadHeaderCount
                                 : writePayloadCount + payloadHeaderCount;
    message |= messageLength << getSendMsgLengthBitOffset();
    unsigned segmentOwordSize = cdiv(segmentByteSize, OWORD_BYTE_SIZE);
    message |= blockSendBlockSizeCode(segmentOwordSize);
    execSize =
        G4_ExecSize(LIMIT_SEND_EXEC_SIZE(segmentOwordSize * DWORD_BYTE_SIZE));
  }
  return std::make_tuple(builder_->createImm(message, Type_UD), execSize);
}

// Create an add instruction to add the FP needed for generating spill/fill
// code. We always set the NoMask flag and use a null conditional modifier.
G4_INST *SpillManagerGRF::createAddFPInst(G4_ExecSize execSize,
                                          G4_DstRegRegion *dst,
                                          G4_Operand *src) {
  const RegionDesc *rDesc = builder_->getRegionScalar();
  G4_Operand *fp = builder_->createSrc(
      builder_->kernel.fg.framePtrDcl->getRegVar(), 0, 0, rDesc, Type_UD);
  auto newInst = builder_->createBinOp(G4_add, execSize, dst, fp, src,
                                       InstOpt_WriteEnable, true);
  newInst->inheritDIFrom(curInst);

  return newInst;
}

// Create a mov instruction needed for generating spill/fill code.
// We always set the NoMask flag and use a null conditional modifier.
G4_INST *SpillManagerGRF::createMovInst(G4_ExecSize execSize,
                                        G4_DstRegRegion *dst, G4_Operand *src,
                                        G4_Predicate *predicate,
                                        unsigned int options) {
  auto newInst = builder_->createMov(execSize, dst, src, options, true);

  if (predicate) {
    newInst->setPredicate(predicate);
  }

  return newInst;
}

// Create a send instruction needed for generating spill/fill code.
// We always set the NoMask flag and use a null predicate and conditional
// modifier.
G4_INST *SpillManagerGRF::createSendInst(G4_ExecSize execSize,
                                         G4_DstRegRegion *postDst,
                                         G4_SrcRegRegion *payload, G4_Imm *desc,
                                         SFID funcID, bool isWrite,
                                         unsigned option) {
  // ToDo: create exDesc in createSendMsgDesc()
  uint32_t exDesc = G4_SendDescRaw::createExtDesc(funcID);
  auto msgDesc = builder_->createSendMsgDesc(
      funcID, (uint32_t)desc->getInt(), exDesc, 0,
      isWrite ? SendAccess::WRITE_ONLY : SendAccess::READ_ONLY, nullptr);
  auto sendInst = builder_->createSendInst(
      NULL, G4_send, execSize, postDst, payload, desc, option, msgDesc, true);
  sendInst->inheritDIFrom(curInst);

  return sendInst;
}

// Create the send instructions to fill in the value of spillRangeDcl into
// fillRangeDcl in aligned portions.
static int getNextSize(int height, bool useHWordMsg, const IR_Builder &irb) {
  bool has8GRFMessage =
      useHWordMsg && irb.getPlatform() >= GENX_SKL && irb.getGRFSize() == 32;
  if (has8GRFMessage && height >= 8) {
    return 8;
  } else if (height >= 4) {
    return 4;
  } else if (height >= 2) {
    return 2;
  } else if (height >= 1) {
    return 1;
  }
  return 0;
}

void SpillManagerGRF::sendInSpilledRegVarPortions(G4_Declare *fillRangeDcl,
                                                  G4_Declare *mRangeDcl,
                                                  unsigned regOff,
                                                  unsigned height,
                                                  unsigned srcRegOff) {
  if ((useScratchMsg_ && mRangeDcl == builder_->getBuiltinR0()) ||
      !headerNeeded()) {
    // Skip initializing message header
  } else {
    // Initialize the message header with the spill disp for portion.
    int offset = getDisp(fillRangeDcl->getRegVar()) +
                 regOff * builder_->numEltPerGRF<Type_UB>();
    getSpillOffset(offset);

    unsigned segmentDisp = offset / OWORD_BYTE_SIZE;
    G4_Imm *segmentDispImm = builder_->createImm(segmentDisp, Type_UD);
    G4_DstRegRegion *mHeaderOffsetDstRegion =
        createMHeaderBlockOffsetDstRegion(mRangeDcl->getRegVar());

    if (builder_->getIsKernel() == false) {
      createAddFPInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
    } else {
      createMovInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
    }
  }

  // Read in the portions using a greedy approach.
  int currentStride = getNextSize(height, useScratchMsg_, *builder_);

  if (currentStride) {
    if (gra.useLscForSpillFill) {
      createLSCFill(fillRangeDcl, regOff, currentStride, srcRegOff);
    } else {
      createFillSendInstr(fillRangeDcl, mRangeDcl, regOff, currentStride,
                          srcRegOff);
    }

    if (height - currentStride > 0) {
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
bool SpillManagerGRF::shouldPreloadSpillRange(G4_INST *instContext,
                                              G4_BB *parentBB) {
  // Check for partial and unaligned regions and add pre-load code, if
  // necessary.
  auto spilledRangeRegion = instContext->getDst();
  G4_ExecSize execSize = instContext->getExecSize();

  if (isPartialRegion(spilledRangeRegion, execSize) ||
      isUnalignedRegion(spilledRangeRegion, execSize) ||
      instContext->isPartialWriteForSpill(!parentBB->isAllLaneActive(),
                                          usesOWOrLSC(spilledRangeRegion))) {
    // special check for scalar variables: no need for pre-fill if instruction
    // writes to whole variable and is not predicated
    auto spilledDcl = spilledRangeRegion->getTopDcl()->getRootDeclare();
    if (execSize == g4::SIMD1 &&
        spilledRangeRegion->getTypeSize() == spilledDcl->getByteSize() &&
        !instContext->getPredicate()) {
      // ToDo: investigate why we are spilling so many scalar variables
      return false;
    }
    return true;
  }
  // No pre-load for whole and aligned region writes
  else {
    return false;
  }
}

// Create the send instruction to perform the pre-load of the spilled region's
// segment into spill memory.
void SpillManagerGRF::preloadSpillRange(G4_Declare *spillRangeDcl,
                                        G4_Declare *mRangeDcl,
                                        G4_DstRegRegion *spilledRangeRegion,
                                        G4_ExecSize execSize) {
  // When execSize is 32, regions <32, 32, 1> or <64; 32, 2> are invalid.
  // Use a uniform region descriptor <stride; 1, 0>. Note that stride could
  // be 0 when execsize is 1.
  uint16_t hstride = spilledRangeRegion->getHorzStride();
  const RegionDesc *rDesc = builder_->createRegionDesc(execSize, hstride, 1, 0);

  G4_SrcRegRegion *preloadRegion = builder_->createSrc(
      spillRangeDcl->getRegVar(), REG_ORIGIN,
      spilledRangeRegion->getSubRegOff(), rDesc, spilledRangeRegion->getType());

  if (useScratchMsg_) {
    // src region's base refers to the filled region's base
    // The size of src region is equal to number of rows that
    // have to be filled, starting at the reg offset specified
    // in the original operand. For eg,
    // Let the spilled operand be V40(3,3)
    //
    // => mov (1) V40(3,3)<1>:ud    V30(0,0)<0;1,0>:ud
    // When this will be replaced with a preload fill,
    // => mov (1) TM_GRF_V40_0(0,0)<1>:ud   V30(0,0)<0;1,0>:ud
    // => send (16) SP_V40_0(0,0)<1>:ud ...                            <--- load
    // V40's 3rd row in SP_V40_0
    // => mov (1) SP_V40_0(0,3)<1>:ud   TM_GRF_V40_0(0,0)<8;8,1>:ud <--- overlay
    // => send (16) null ...                                        <--- store
    // V40's updated 3rd row to memory
    //
    // Since the filled register's register offset is 0,0 in first
    // send instruction, this change is made when creating the operand
    // itself.

    // Attach preloadRegion to dummy mov so getLeftBound/getRightBound won't
    // crash when called from crossGRF in createFillSendMsgDesc
    builder_->createMov(execSize, builder_->createNullDst(Type_UD),
                        preloadRegion, InstOpt_NoOpt, false);
  }

  if (gra.useLscForSpillFill) {
    createLSCFill(spillRangeDcl, preloadRegion, execSize);
  } else {
    createFillSendInstr(spillRangeDcl, mRangeDcl, preloadRegion, execSize);
  }
}

G4_SrcRegRegion *vISA::getSpillFillHeader(IR_Builder &builder,
                                          G4_Declare *decl) {
  if (builder.supportsLSC()) {
    // LSC in its current incarnation needs a header to store the address
    return builder.createSrcRegRegion(builder.getSpillFillHeader(),
                                      builder.getRegionStride1());
  }
  return builder.createSrcRegRegion(decl, builder.getRegionStride1());
}

// Create the send instruction to perform the spill of the spilled regvars's
// segment into spill memory.
// regOff - Offset of sub-spill. If one spill is split into more than one spill,
// this is the offset of them, unit in register size
//  spillOff - Offset of the original variable being spilled, unit in register
//  size.
G4_INST *SpillManagerGRF::createSpillSendInstr(G4_Declare *spillRangeDcl,
                                               G4_Declare *mRangeDcl,
                                               unsigned regOff, unsigned height,
                                               unsigned spillOff) {
  G4_ExecSize execSize(0);

  G4_Imm *messageDescImm = NULL;

  if (useScratchMsg_) {
    G4_RegVar *r = spillRangeDcl->getRegVar();
    G4_RegVarTmp *rvar = static_cast<G4_RegVarTmp *>(r);
    messageDescImm = createSpillSendMsgDesc(spillOff, height, execSize,
                                            rvar->getBaseRegVar());
#ifdef _DEBUG
    int offset =
        (messageDescImm->getInt() & 0xFFF) * builder_->numEltPerGRF<Type_UB>();
    vISA_ASSERT(offset >= globalScratchOffset, "incorrect offset");
#endif
  } else {
    messageDescImm = createSpillSendMsgDesc(regOff, height, execSize);
  }

  G4_DstRegRegion *postDst =
      builder_->createNullDst(execSize > g4::SIMD8 ? Type_UW : Type_UD);

  G4_INST *sendInst = NULL;
  if (useSplitSend()) {
    auto headerOpnd = getSpillFillHeader(*builder_, mRangeDcl);
    G4_SrcRegRegion *srcOpnd =
        createBlockSpillRangeSrcRegion(spillRangeDcl->getRegVar(), regOff);

    auto off = G4_SpillIntrinsic::InvalidOffset;
    G4_Declare *fp = nullptr;
    if (useScratchMsg_)
      off = (messageDescImm->getInt() & 0xfff);
    else {
      if (builder_->usesStack()) {
        G4_RegVar *r = spillRangeDcl->getRegVar();
        G4_RegVarTmp *rvar = static_cast<G4_RegVarTmp *>(r);
        int offset = getDisp(rvar->getBaseRegVar());
        getSpillOffset(offset);
        // message expects offsets to be in HWord
        off = (offset + spillOff * builder_->getGRFSize()) >>
              SCRATCH_SPACE_ADDRESS_UNIT;
        if (builder_->usesStack())
          fp = builder_->kernel.fg.getFramePtrDcl();

        if (!fp && offset < SCRATCH_MSG_LIMIT)
          headerOpnd = builder_->createSrcRegRegion(
              builder_->getBuiltinR0(), builder_->getRegionStride1());
      }
    }
    sendInst =
        builder_->createSpill(postDst, headerOpnd, srcOpnd, execSize, height,
                              off, fp, InstOpt_WriteEnable, true);
    sendInst->inheritDIFrom(curInst);
  } else {
    G4_SrcRegRegion *payload = builder_->createSrc(
        mRangeDcl->getRegVar(), 0, 0, builder_->getRegionStride1(), Type_UD);
    sendInst = createSendInst(execSize, postDst, payload, messageDescImm,
                              SFID::DP_DC0, true, InstOpt_WriteEnable);
  }

  return sendInst;
}

// Create the send instruction to perform the spill of the spilled region's
// segment into spill memory.
G4_INST *
SpillManagerGRF::createSpillSendInstr(G4_Declare *spillRangeDcl,
                                      G4_Declare *mRangeDcl,
                                      G4_DstRegRegion *spilledRangeRegion,
                                      G4_ExecSize execSize, unsigned option) {

  G4_DstRegRegion *postDst =
      builder_->createNullDst(execSize > g4::SIMD8 ? Type_UW : Type_UD);

  G4_INST *sendInst = NULL;
  if (useSplitSend()) {
    unsigned extMsgLength = spillRangeDcl->getNumRows();
    const RegionDesc *region = builder_->getRegionStride1();
    auto headerOpnd = getSpillFillHeader(*builder_, mRangeDcl);
    G4_SrcRegRegion *srcOpnd =
        builder_->createSrcRegRegion(spillRangeDcl, region);

    auto off = G4_SpillIntrinsic::InvalidOffset;
    G4_Declare *fp = nullptr;
    auto spillExecSize = execSize;
    if (useScratchMsg_) {
      auto [messageDescImm, retSize] =
          createSpillSendMsgDesc(spilledRangeRegion, execSize);
      spillExecSize = retSize;
      off = (messageDescImm->getInt() & 0xfff);
    } else {
      if (builder_->usesStack()) {
        G4_RegVar *r = spillRangeDcl->getRegVar();
        G4_RegVarTmp *rvar = static_cast<G4_RegVarTmp *>(r);
        int offset = getDisp(rvar->getBaseRegVar());
        getSpillOffset(offset);
        // message expects offsets to be in HWord
        auto regOff = spilledRangeRegion->getRegOff();
        off = (offset + regOff * builder_->getGRFSize()) >>
              SCRATCH_SPACE_ADDRESS_UNIT;
        if (builder_->usesStack())
          fp = builder_->kernel.fg.getFramePtrDcl();

        if (!fp && offset < SCRATCH_MSG_LIMIT)
          headerOpnd = builder_->createSrcRegRegion(
              builder_->getBuiltinR0(), builder_->getRegionStride1());
      }
    }
    sendInst = builder_->createSpill(
        postDst, headerOpnd, srcOpnd, spillExecSize, (uint16_t)extMsgLength,
        off, fp, static_cast<G4_InstOption>(option), true);
    sendInst->inheritDIFrom(curInst);
  } else {
    auto [messageDescImm, spillExecSize] =
        createSpillSendMsgDesc(spilledRangeRegion, execSize);
    G4_SrcRegRegion *payload = builder_->createSrc(
        mRangeDcl->getRegVar(), 0, 0, builder_->getRegionStride1(), Type_UD);
    sendInst =
        createSendInst(spillExecSize, postDst, payload, messageDescImm,
                       SFID::DP_DC0, true, static_cast<G4_InstOption>(option));
  }

  return sendInst;
}

// Create the message description for a fill send instruction for filled
// regvars.
G4_Imm *SpillManagerGRF::createFillSendMsgDesc(unsigned regOff, unsigned height,
                                               G4_ExecSize &execSize,
                                               G4_RegVar *base) {
  unsigned message = 0;

  if (useScratchMsg_) {
    unsigned segmentByteSize = height * builder_->numEltPerGRF<Type_UB>();
    unsigned responseLength =
        cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
    message = responseLength << getSendRspLengthBitOffset();
    unsigned headerPresent = 0x80000;
    message |= SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT << getSendMsgLengthBitOffset();
    message |= headerPresent;

    message |= (1 << SCRATCH_MSG_DESC_CATEORY);
    message |= (0 << SCRATCH_MSG_INVALIDATE_AFTER_READ);
    unsigned blocksize_encoding =
        getScratchBlocksizeEncoding(height, *builder_);

    message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);

    int offset = getDisp(base);
    getSpillOffset(offset);
    // message expects offsets to be in HWord
    message |= (offset + regOff * builder_->getGRFSize()) >>
               SCRATCH_SPACE_ADDRESS_UNIT;

    execSize = g4::SIMD16;
  } else {
    unsigned segmentByteSize = height * builder_->numEltPerGRF<Type_UB>();
    unsigned statelessSurfaceIndex = 0xFF;
    unsigned responseLength =
        cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
    responseLength = responseLength << getSendRspLengthBitOffset();
    message = statelessSurfaceIndex | responseLength;

    unsigned headerPresent = 0x80000;
    message |= headerPresent;
    unsigned messageType = getSendOwordReadType();
    message |= messageType << getSendReadTypeBitOffset();
    unsigned messageLength = OWORD_PAYLOAD_HEADER_MIN_HEIGHT;
    message |= messageLength << getSendMsgLengthBitOffset();
    unsigned segmentOwordSize = cdiv(segmentByteSize, OWORD_BYTE_SIZE);
    message |= blockSendBlockSizeCode(segmentOwordSize);
    execSize =
        G4_ExecSize(LIMIT_SEND_EXEC_SIZE(segmentOwordSize * DWORD_BYTE_SIZE));
  }
  return builder_->createImm(message, Type_UD);
}

// Create the message description for a fill send instruction for filled
// source regions.
template <class REGION_TYPE>
G4_Imm *SpillManagerGRF::createFillSendMsgDesc(REGION_TYPE *filledRangeRegion,
                                               G4_ExecSize execSize) {
  unsigned message = 0;

  if (useScratchMsg_) {
    unsigned segmentByteSize = getSegmentByteSize(filledRangeRegion, execSize);
    if (filledRangeRegion->crossGRF(*builder_)) {
      segmentByteSize = 2 * builder_->numEltPerGRF<Type_UB>();
    }

    unsigned responseLength =
        cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
    message = responseLength << getSendRspLengthBitOffset();

    unsigned headerPresent = 0x80000;
    message |= headerPresent;

    message |=
        (SCRATCH_PAYLOAD_HEADER_MAX_HEIGHT << getSendMsgLengthBitOffset());
    message |= (1 << SCRATCH_MSG_DESC_CATEORY);
    message |= (0 << SCRATCH_MSG_INVALIDATE_AFTER_READ);
    unsigned blocksize_encoding =
        getScratchBlocksizeEncoding(responseLength, *builder_);

    message |= (blocksize_encoding << SCRATCH_MSG_DESC_BLOCK_SIZE);
    int offset = getRegionDisp(filledRangeRegion);
    getSpillOffset(offset);
    message |= offset >> SCRATCH_SPACE_ADDRESS_UNIT;
  } else {
    unsigned segmentByteSize = getSegmentByteSize(filledRangeRegion, execSize);
    unsigned statelessSurfaceIndex = 0xFF;
    unsigned responseLength =
        cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
    responseLength = responseLength << getSendRspLengthBitOffset();
    message = statelessSurfaceIndex | responseLength;

    unsigned headerPresent = 0x80000;
    message |= headerPresent;
    unsigned messageType = getSendOwordReadType();
    message |= messageType << getSendReadTypeBitOffset();
    unsigned messageLength = OWORD_PAYLOAD_HEADER_MIN_HEIGHT;
    message |= messageLength << getSendMsgLengthBitOffset();
    unsigned segmentOwordSize = cdiv(segmentByteSize, OWORD_BYTE_SIZE);
    message |= blockSendBlockSizeCode(segmentOwordSize);
  }
  return builder_->createImm(message, Type_UD);
}

// Create the send instruction to perform the fill of the spilled regvars's
// segment from spill memory.
// spillOff - spill offset to the fillRangeDcl, in unit of grf size
G4_INST *SpillManagerGRF::createFillSendInstr(G4_Declare *fillRangeDcl,
                                              G4_Declare *mRangeDcl,
                                              unsigned regOff, unsigned height,
                                              unsigned spillOff) {
  G4_ExecSize execSize{0};

  G4_Imm *messageDescImm = NULL;

  if (useScratchMsg_) {
    G4_RegVar *r = fillRangeDcl->getRegVar();
    G4_RegVarTmp *rvar = static_cast<G4_RegVarTmp *>(r);
    messageDescImm = createFillSendMsgDesc(spillOff, height, execSize,
                                           rvar->getBaseRegVar());
#ifdef _DEBUG
    int offset =
        (messageDescImm->getInt() & 0xFFF) * builder_->numEltPerGRF<Type_UB>();
    vISA_ASSERT(offset >= globalScratchOffset, "incorrect offset");
#endif
  } else {
    messageDescImm = createFillSendMsgDesc(regOff, height, execSize);
  }

  G4_DstRegRegion *postDst = builder_->createDst(
      fillRangeDcl->getRegVar(), (short)regOff, SUBREG_ORIGIN, DEF_HORIZ_STRIDE,
      (execSize > 8) ? Type_UW : Type_UD);

  auto payload = getSpillFillHeader(*builder_, mRangeDcl);

  unsigned int off = G4_FillIntrinsic::InvalidOffset;
  G4_Declare *fp = nullptr;
  if (useScratchMsg_)
    off = (messageDescImm->getInt() & 0xfff);
  else {
    if (builder_->usesStack()) {
      // compute hword offset to emit later when expanding spill/fill intrinsic
      G4_RegVar *r = fillRangeDcl->getRegVar();
      G4_RegVarTmp *rvar = static_cast<G4_RegVarTmp *>(r);
      int offset = getDisp(rvar->getBaseRegVar());
      getSpillOffset(offset);
      // message expects offsets to be in HWord
      off = (offset + spillOff * builder_->getGRFSize()) >>
            SCRATCH_SPACE_ADDRESS_UNIT;
      if (builder_->usesStack())
        fp = builder_->kernel.fg.getFramePtrDcl();

      if (!fp && offset < SCRATCH_MSG_LIMIT)
        payload = builder_->createSrcRegRegion(builder_->getBuiltinR0(),
                                               builder_->getRegionStride1());
    }
  }
  auto fillInst = builder_->createFill(payload, postDst, execSize, height, off,
                                       fp, InstOpt_WriteEnable, true);
  fillInst->inheritDIFrom(curInst);
  return fillInst;
}

// Create the send instruction to perform the fill of the filled region's
// segment into fill memory.
G4_INST *SpillManagerGRF::createFillSendInstr(
    G4_Declare *fillRangeDcl, G4_Declare *mRangeDcl,
    G4_SrcRegRegion *filledRangeRegion, G4_ExecSize execSize) {
  auto oldExecSize = execSize;

  if (useScratchMsg_) {
    execSize = g4::SIMD16;
  }

  G4_DstRegRegion *postDst =
      builder_->createDst(fillRangeDcl->getRegVar(), 0, SUBREG_ORIGIN,
                          DEF_HORIZ_STRIDE, (execSize > 8) ? Type_UW : Type_UD);

  auto payload = getSpillFillHeader(*builder_, mRangeDcl);

  unsigned int off = G4_FillIntrinsic::InvalidOffset;
  unsigned segmentByteSize = getSegmentByteSize(filledRangeRegion, oldExecSize);
  G4_Declare *fp = nullptr;
  if (useScratchMsg_) {
    G4_Imm *messageDescImm =
        createFillSendMsgDesc(filledRangeRegion, oldExecSize);

    off = (messageDescImm->getInt() & 0xfff);
    if (filledRangeRegion->crossGRF(*builder_)) {
      segmentByteSize = 2 * builder_->numEltPerGRF<Type_UB>();
    }
  } else {
    if (builder_->usesStack()) {
      // compute hword offset to emit later when expanding spill/fill intrinsic
      int offset = getRegionDisp(filledRangeRegion);
      getSpillOffset(offset);
      off = offset >> SCRATCH_SPACE_ADDRESS_UNIT;
      if (builder_->usesStack())
        fp = builder_->kernel.fg.getFramePtrDcl();

      if (!fp && offset < SCRATCH_MSG_LIMIT)
        payload = builder_->createSrcRegRegion(builder_->getBuiltinR0(),
                                               builder_->getRegionStride1());
    }
  }

  unsigned responseLength =
      cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
  auto fillInst =
      builder_->createFill(payload, postDst, execSize, responseLength, off, fp,
                           InstOpt_WriteEnable, true);
  fillInst->inheritDIFrom(curInst);
  return fillInst;
}

// LSC versions of spill/fill, gra.useLscForSpillFill must be true.
G4_SrcRegRegion *SpillManagerGRF::getLSCSpillFillHeader(const G4_Declare *fp,
                                                        int offset) {
  if (!fp && offset < SCRATCH_MSG_LIMIT &&
      !gra.useLscForNonStackCallSpillFill) {
    // Use the legacy non-LSC scratch message with R0 as its header.
    return builder_->createSrcRegRegion(builder_->getBuiltinR0(),
                                        builder_->getRegionStride1());
  }
  // Use LSC for spill/fill; a temp GRF is needed for the address payload.
  return builder_->createSrcRegRegion(builder_->getSpillFillHeader(),
                                      builder_->getRegionStride1());
}

// Create the send instruction to perform the spill of the spilled regvars's
// segment into spill memory.
//
// regOff - Offset of sub-spill. If one spill is splitted into more than one
// spill, this is the offset of them, unit in register size spillOff - Offset of
// the original variable being spilled, unit in register size.
G4_INST *SpillManagerGRF::createLSCSpill(G4_Declare *spillRangeDcl,
                                         unsigned regOff, unsigned height,
                                         unsigned spillOff) {
  G4_ExecSize execSize(16);

  G4_DstRegRegion *postDst = builder_->createNullDst(Type_UD);

  G4_SrcRegRegion *srcOpnd =
      createBlockSpillRangeSrcRegion(spillRangeDcl->getRegVar(), regOff);
  G4_Declare *fp =
      builder_->usesStack() ? builder_->kernel.fg.getFramePtrDcl() : nullptr;

  G4_RegVarTmp *rvar = static_cast<G4_RegVarTmp *>(spillRangeDcl->getRegVar());
  int offset = getDisp(rvar->getBaseRegVar());
  getSpillOffset(offset);
  uint32_t totalByteOffset = (offset + spillOff * builder_->getGRFSize());
  // message expects offsets to be in HWord
  uint32_t offsetHwords = totalByteOffset >> SCRATCH_SPACE_ADDRESS_UNIT;

  G4_SrcRegRegion *header = getLSCSpillFillHeader(fp, totalByteOffset);
  auto sendInst =
      builder_->createSpill(postDst, header, srcOpnd, execSize, height,
                            offsetHwords, fp, InstOpt_WriteEnable, true);
  sendInst->inheritDIFrom(curInst);

  return sendInst;
}

// Create the send instruction to perform the spill of the spilled region's
// segment into spill memory.
G4_INST *SpillManagerGRF::createLSCSpill(G4_Declare *spillRangeDcl,
                                         G4_DstRegRegion *spilledRangeRegion,
                                         G4_ExecSize execSize, unsigned option,
                                         bool isScatter) {
  G4_DstRegRegion *postDst = builder_->createNullDst(
      isScatter ? spilledRangeRegion->getType() : Type_UD);

  unsigned extMsgLength = spillRangeDcl->getNumRows();
  const RegionDesc *region = builder_->getRegionStride1();
  G4_SrcRegRegion *srcOpnd =
      builder_->createSrcRegRegion(spillRangeDcl, region);

  G4_Declare *fp =
      builder_->usesStack() ? builder_->kernel.fg.getFramePtrDcl() : nullptr;

  G4_RegVarTmp *rvar = static_cast<G4_RegVarTmp *>(spillRangeDcl->getRegVar());
  int offset = getDisp(rvar->getBaseRegVar());
  getSpillOffset(offset);
  // message expects offsets to be in HWord
  auto regOff = spilledRangeRegion->getRegOff();
  uint32_t totalByteOffset = (offset + regOff * builder_->getGRFSize());
  uint32_t offsetHwords = totalByteOffset >> SCRATCH_SPACE_ADDRESS_UNIT;

  G4_SrcRegRegion *header = getLSCSpillFillHeader(fp, totalByteOffset);
  auto sendInst = builder_->createSpill(
      postDst, header, srcOpnd, execSize, (uint16_t)extMsgLength, offsetHwords,
      fp, static_cast<G4_InstOption>(option), true, isScatter);
  sendInst->inheritDIFrom(curInst);

  return sendInst;
}

// Create the send instruction to perform the fill of the spilled regvars's
// segment from spill memory.
// spillOff - spill offset to the fillRangeDcl, in unit of grf size
G4_INST *SpillManagerGRF::createLSCFill(G4_Declare *fillRangeDcl,
                                        unsigned regOff, unsigned height,
                                        unsigned spillOff) {
  G4_DstRegRegion *postDst =
      builder_->createDst(fillRangeDcl->getRegVar(), (short)regOff,
                          SUBREG_ORIGIN, DEF_HORIZ_STRIDE, Type_UD);

  G4_Declare *fp =
      builder_->usesStack() ? builder_->kernel.fg.getFramePtrDcl() : nullptr;

  // compute hword offset to emit later when expanding spill/fill intrinsic
  G4_RegVar *r = fillRangeDcl->getRegVar();
  G4_RegVarTmp *rvar = static_cast<G4_RegVarTmp *>(r);
  int offset = getDisp(rvar->getBaseRegVar());
  getSpillOffset(offset);
  uint32_t totalByteOffset = (offset + spillOff * builder_->getGRFSize());
  // fill intrinsic expects offsets to be in HWord
  uint32_t offsetHwords = totalByteOffset >> SCRATCH_SPACE_ADDRESS_UNIT;

  G4_SrcRegRegion *header = getLSCSpillFillHeader(fp, totalByteOffset);
  auto fillInst =
      builder_->createFill(header, postDst, g4::SIMD16, height, offsetHwords,
                           fp, InstOpt_WriteEnable, true);
  fillInst->inheritDIFrom(curInst);
  return fillInst;
}

// Create the send instruction to perform the fill of the filled region's
// segment into fill memory.
G4_INST *SpillManagerGRF::createLSCFill(G4_Declare *fillRangeDcl,
                                        G4_SrcRegRegion *filledRangeRegion,
                                        G4_ExecSize execSize) {
  auto oldExecSize = execSize;

  G4_DstRegRegion *postDst = builder_->createDst(
      fillRangeDcl->getRegVar(), 0, SUBREG_ORIGIN, DEF_HORIZ_STRIDE, Type_UD);

  unsigned segmentByteSize = getSegmentByteSize(filledRangeRegion, oldExecSize);
  G4_Declare *fp =
      builder_->usesStack() ? builder_->kernel.fg.getFramePtrDcl() : nullptr;

  // compute hword offset to emit later when expanding spill/fill intrinsic
  int offset = getRegionDisp(filledRangeRegion);
  getSpillOffset(offset);
  uint32_t offsetHwords = offset >> SCRATCH_SPACE_ADDRESS_UNIT;

  unsigned responseLength =
      cdiv(segmentByteSize, builder_->numEltPerGRF<Type_UB>());
  G4_SrcRegRegion *header = getLSCSpillFillHeader(fp, offset);
  auto fillInst =
      builder_->createFill(header, postDst, execSize, responseLength,
                           offsetHwords, fp, InstOpt_WriteEnable, true);
  fillInst->inheritDIFrom(curInst);
  return fillInst;
}

// Replace the reference to the spilled region with a reference to an
// equivalent reference to the spill range region.
void SpillManagerGRF::replaceSpilledRange(G4_Declare *spillRangeDcl,
                                          G4_DstRegRegion *spilledRegion,
                                          G4_INST *spilledInst,
                                          uint32_t subRegOff) {
  // we need to preserve accRegSel if it's set
  G4_DstRegRegion *tmpRangeDstRegion = builder_->createDst(
      spillRangeDcl->getRegVar(), REG_ORIGIN, subRegOff,
      spilledRegion->getHorzStride(), spilledRegion->getType(),
      spilledRegion->getAccRegSel());
  spilledInst->setDest(tmpRangeDstRegion);
}

// Replace the reference to the filled region with a reference to an
// equivalent reference to the fill range region.
void SpillManagerGRF::replaceFilledRange(G4_Declare *fillRangeDcl,
                                         G4_SrcRegRegion *filledRegion,
                                         G4_INST *filledInst) {
  G4_ExecSize execSize = isMultiRegComprSource(filledRegion, filledInst)
                             ? G4_ExecSize(filledInst->getExecSize() / 2)
                             : filledInst->getExecSize();

  for (int i = 0, numSrc = filledInst->getNumSrc(); i < numSrc; i++) {
    G4_Operand *src = filledInst->getSrc(i);

    if (src && src->isSrcRegRegion()) {
      G4_SrcRegRegion *srcRgn = src->asSrcRegRegion();
      if (*srcRgn == *filledRegion) {
        G4_SrcRegRegion *fillRangeSrcRegion = createFillRangeSrcRegion(
            fillRangeDcl->getRegVar(), filledRegion, execSize);
        filledInst->setSrc(fillRangeSrcRegion, i);
      }
    }
  }
}

// Create the send instructions to write out the spillRangeDcl in aligned
// portions.
void SpillManagerGRF::sendOutSpilledRegVarPortions(G4_Declare *spillRangeDcl,
                                                   G4_Declare *mRangeDcl,
                                                   unsigned regOff,
                                                   unsigned height,
                                                   unsigned srcRegOff) {
  if (!headerNeeded()) {
    // No need to make a copy of offset because when using
    // scratch msg descriptor, the offset is part of send
    // msg descriptor and not the header.
  } else {
    // Initialize the message header with the spill disp for portion.
    int offset = getDisp(spillRangeDcl->getRegVar()) +
                 regOff * builder_->numEltPerGRF<Type_UB>();
    getSpillOffset(offset);
    unsigned segmentDisp = offset / OWORD_BYTE_SIZE;

    G4_Imm *segmentDispImm = builder_->createImm(segmentDisp, Type_UD);
    G4_DstRegRegion *mHeaderOffsetDstRegion =
        createMHeaderBlockOffsetDstRegion(mRangeDcl->getRegVar());

    if (builder_->getIsKernel() == false) {
      createAddFPInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
    } else {
      createMovInst(g4::SIMD1, mHeaderOffsetDstRegion, segmentDispImm);
    }
  }

  // Write out the portions using a greedy approach.
  int currentStride = getNextSize(height, useScratchMsg_, *builder_);

  if (currentStride) {
    initMWritePayload(spillRangeDcl, mRangeDcl, regOff, currentStride);

    if (gra.useLscForSpillFill) {
      createLSCSpill(spillRangeDcl, regOff, currentStride, srcRegOff);
    } else {
      createSpillSendInstr(spillRangeDcl, mRangeDcl, regOff, currentStride,
                           srcRegOff);
    }

    if (height - currentStride > 0) {
      sendOutSpilledRegVarPortions(
          spillRangeDcl, mRangeDcl, regOff + currentStride,
          height - currentStride, srcRegOff + currentStride);
    }
  }
}

bool SpillManagerGRF::checkDefUseDomRel(G4_DstRegRegion *dst, G4_BB *defBB) {
  if (!refs.isUniqueDef(dst))
    return false;

  auto dcl = dst->getTopDcl();

  // check whether this def dominates all its uses
  auto uses = refs.getUses(dcl);

  for (auto &use : *uses) {
    auto useBB = std::get<1>(use);

    // check if def dominates use
    if (!defBB->dominates(useBB))
      return false;

    if (defBB == useBB) {
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
  if (defLoop) {
    // since def is in loop, check whether uses are also in same loop.
    for (auto &use : *uses) {
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

bool SpillManagerGRF::checkUniqueDefAligned(G4_DstRegRegion *dst,
                                            G4_BB *defBB) {
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

  for (auto &def : *defs) {
    // check whether dst and def write same row
    auto otherDefInst = std::get<0>(def);

    if (otherDefInst == dst->getInst())
      continue;

    auto otherDefDstRgn = otherDefInst->getDst();
    unsigned int otherLb = otherDefDstRgn->getLeftBound();
    unsigned int otherRb = otherDefDstRgn->getRightBound();
    unsigned int otherTypeSize = otherDefDstRgn->getTypeSize();
    bool commonRow = false;
    for (unsigned int i = otherLb; i <= otherRb; i += otherTypeSize) {
      auto rowWritten = i / GRFSize;
      if (rowWritten >= startRow && rowWritten <= endRow) {
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

    if (defBB == otherDefBB) {
      if (dst->getInst()->getLexicalId() > otherDefInst->getLexicalId())
        return false;
    }
  }

  return true;
}

bool SpillManagerGRF::isFirstLexicalDef(G4_DstRegRegion *dst) {
  // Check whether dst is lexically first def
  auto dcl = dst->getTopDcl();
  auto *defs = refs.getDefs(dcl);

  // Check whether any def has lower lexical id than dst
  for (auto& def : *defs) {
    auto *defInst = std::get<0>(def);
    if (defInst->getLexicalId() < dst->getInst()->getLexicalId())
      return false;
  }

  return true;
}

// This function checks whether each spill dst region requires a
// read-modify-write operation when inserting spill code. Dominator/unique defs
// don't require redundant read operation. Dst regions that do not need RMW are
// added to a set. This functionality isnt needed for functional correctness.
// This function is executed before inserting spill code because we need all dst
// regions of dcl available to decide whether read is redundant. If this is
// executed when inserting spill then dst regions of dcl appearing earlier than
// current one would be translated to spill code already. Spill/fill code
// insertion replaces dst region of spills with new temp region. This makes it
// difficult to check whether current dst and an earlier spilled dst write to
// same GRF row.
void SpillManagerGRF::updateRMWNeeded() {
  if (!gra.kernel.getOption(vISA_SkipRedundantFillInRMW))
    return;

  auto isRMWNeededForSpilledDst = [&](G4_BB *bb,
                                      G4_DstRegRegion *spilledRegion) {
    auto isUniqueDef = checkUniqueDefAligned(spilledRegion, bb);

    // Check0 : Def is NoMask, -- checked in isPartialWriteForSpill()
    // Check1 : Def is unique def,
    // Check2 : Def is in loop L and all use(s) of dcl are in loop L or it's
    //          inner loop nest,
    // Check3 : Flowgraph is reducible
    // Check4 : Dcl is not a split around loop temp
    // Check5 : bb is entryBB and spilledRegion is lexically first definition
    // RMW_Not_Needed = Check5 || ((Check0 || (Check1 && Check2 && Check3)) &&
    // Check4)
    bool RMW_Needed = true;

    // Reason for Check4:
    // ********************
    // Before:
    // Loop_Header:
    // V10:uq = ...
    // ...
    // jmpi Loop_Header
    // ...
    // = V10
    // ********************
    // After split around loop:
    //
    // (W) LOOP_TMP = V10
    // Loop_Header:
    // LOOP_TMP:uq = ...
    // ...
    // jmpi Loop_Header
    // (W) V10 = LOOP_TMP
    // ...
    // = V10
    // ********************
    // Since V10 is spilled already, spill/fill code is inserted as:
    // (Note that program no longer has direct references to V10)
    //
    // (W) FILL_TMP = Fill from V10 offset
    // (W) LOOP_TMP = FILL_TMP
    // Loop_Header:
    // LOOP_TMP:uq = ...
    // ...
    // jmpi Loop_Header
    // (W) SPILL_TMP = LOOP_TMP
    // (W) Spill SPILL_TMP to V10 offset
    // ...
    // (W) FILL_TMP1 = Fill from V10 offset
    // = FILL_TMP1
    // ********************
    //
    // If LOOP_TMP is spilled in later iteration, we need to check whether
    // RMW is needed for its def in the loop body. But by this iteration
    // all original references to V10 have already been transformed to
    // temporary ranges, so we cannot easily determine dominance relation
    // between LOOP_TMP and other V10 references. If LOOP_TMP doesn't
    // dominate all defs and uses then it would be illegal to skip RMW. Hence,
    // we conservatively assume RMW is required for LOOP_TMP.
    if (gra.splitResults.count(spilledRegion->getTopDcl()) == 0) {
      if (isUniqueDef && builder_->kernel.fg.isReducible() &&
          checkDefUseDomRel(spilledRegion, bb)) {
        RMW_Needed = false;
      }
    }

    // Check5
    if (RMW_Needed && bb == builder_->kernel.fg.getEntryBB()) {
      RMW_Needed = !isFirstLexicalDef(spilledRegion);
    }

    return RMW_Needed;
  };

  // First pass to setup lexical ids of instruction so dominator relation can be
  // computed correctly intra-BB.
  unsigned int lexId = 0;
  for (auto bb : gra.kernel.fg.getBBList()) {
    for (auto inst : bb->getInstList()) {
      inst->setLexicalId(lexId++);
    }
  }

  for (auto bb : gra.kernel.fg.getBBList()) {
    for (auto inst : bb->getInstList()) {
      if (inst->isPseudoKill())
        continue;

      auto dst = inst->getDst();
      if (!dst)
        continue;

      if (!dst->getBase()->isRegVar())
        continue;

      auto dstRegVar = dst->getBase()->asRegVar();
      if (dstRegVar && shouldSpillRegister(dstRegVar)) {
        if (getRFType(dstRegVar) != G4_GRF)
          continue;

        auto RMW_Needed = isRMWNeededForSpilledDst(bb, dst);
        if (!RMW_Needed) {
          // Any spilled dst region that doesnt need RMW
          // is added to noRMWNeeded set. This set is later
          // checked when inserting spill/fill code.
          noRMWNeeded.insert(dst);
        }
      }
    }
  }
}

// Return true if spill code for spilledRegion requires OW or LSC.
bool SpillManagerGRF::usesOWOrLSC(G4_DstRegRegion *spilledRegion) {
  auto inst = spilledRegion->getInst();

  // Use LSC always
  if (gra.useLscForNonStackCallSpillFill)
    return true;

  // * platform supports LSC and/or OW
  // * but we favor HW scratch, offset permitting
  //
  // If offset exceeds encoding limit, we emit OW/LSC instead.
  int spillOffset = (int)getSegmentDisp(spilledRegion, inst->getExecSize());
  getSpillOffset(spillOffset);
  return spillOffset >= SCRATCH_MSG_LIMIT;
}

// Create the code to create the spill range and save it to spill memory.
void SpillManagerGRF::insertSpillRangeCode(INST_LIST::iterator spilledInstIter,
                                           G4_BB *bb) {
  G4_ExecSize execSize = (*spilledInstIter)->getExecSize();
  G4_Declare *replacementRangeDcl;
  builder_->instList.clear();

  bool optimizeSplitLLR = false;
  G4_INST *inst = *spilledInstIter;
  G4_INST *spillSendInst = NULL;
  auto spilledRegion = inst->getDst();

  auto spillDcl = spilledRegion->getTopDcl()->getRootDeclare();
  if (!isScalarImmRespill(spillDcl) &&
      scalarImmSpill.find(spillDcl) != scalarImmSpill.end()) {
    // do not spill scalar immediate values
    bb->erase(spilledInstIter);
    return;
  }

  auto checkRMWNeeded = [this, spilledRegion]() {
    return noRMWNeeded.find(spilledRegion) == noRMWNeeded.end();
  };

  auto dstFullOverlapSrc = [this, inst]() {
    // For inst, check whether dst fully overlaps any source.
    // Return true if there's an overlap.
    // This is useful in deciding whether scatter should be used
    // or block spill should be used. When there's full overlap,
    // a fill is emitted for the source. So even if inst doesn't
    // use NoMask, it's preferable to use block spill.
    auto *dstDcl = inst->getDst()->getTopDcl();
    auto dstLb = inst->getDst()->getLeftBound();
    auto dstRb = inst->getDst()->getRightBound();
    for (unsigned int i = 0; i != inst->getNumSrc(); ++i) {
      auto *src = inst->getSrc(i);
      if (!src || src->getTopDcl() != dstDcl)
        continue;
      auto srcLb = src->getLeftBound();
      auto srcRb = src->getRightBound();
      if (dstLb <= srcLb && dstRb <= srcRb)
        return true;
    }
    return false;
  };

  auto isScatterSpillCandidate = [this, inst, dstFullOverlapSrc]() {
    // Conditions for scatter spill: inst can't be NoMask,
    // element size is W/DW/Q, isn't dst partial region, and
    // inst isn't predicated
    auto elemSz = inst->getDst()->getElemSize();
    auto fillAvailable = dstFullOverlapSrc();
    return gra.useLscForScatterSpill && !inst->isWriteEnableInst() &&
           (elemSz == 2 || elemSz == 4 || elemSz == 8) &&
           !isPartialRegion(inst->getDst(), inst->getExecSize()) &&
           !inst->getPredicate() && !fillAvailable;
  };


  // subreg offset for new dst that replaces the spilled dst
  auto newSubregOff = 0;

  if (inst->isSend() || inst->isDpas()) {
    // Handle send instructions (special treatment)
    // Create the spill range for the whole post destination, assign spill
    // offset to the spill range and create the instructions to load the
    // save the spill range to spill memory.
    INST_LIST::iterator sendOutIter = spilledInstIter;
    vASSERT(getRFType(spilledRegion) == G4_GRF);
    G4_Declare *spillRangeDcl = createPostDstSpillRangeDeclare(*sendOutIter);
    G4_Declare *mRangeDcl =
        createAndInitMHeader((G4_RegVarTransient *)spillRangeDcl->getRegVar());

    bool needRMW = inst->isPartialWriteForSpill(!bb->isAllLaneActive(),
                                                usesOWOrLSC(spilledRegion)) &&
                   checkRMWNeeded();
    if (needRMW) {
      sendInSpilledRegVarPortions(spillRangeDcl, mRangeDcl, 0,
                                  spillRangeDcl->getNumRows(),
                                  spilledRegion->getRegOff());

      INST_LIST::iterator insertPos = sendOutIter;
      splice(bb, insertPos, builder_->instList, curInst->getVISAId());
    }

    sendOutSpilledRegVarPortions(spillRangeDcl, mRangeDcl, 0,
                                 spillRangeDcl->getNumRows(),
                                 spilledRegion->getRegOff());

    replacementRangeDcl = spillRangeDcl;
  } else {
    // Handle other regular single/multi destination register instructions.
    // Create the spill range for the destination region, assign spill
    // offset to the spill range and create the instructions to load the
    // save the spill range to spill memory.

    // Create the segment aligned spill range
    G4_Declare *spillRangeDcl =
        createSpillRangeDeclare(spilledRegion, execSize, *spilledInstIter);

    // Create and initialize the message header
    G4_Declare *mRangeDcl = createAndInitMHeader(spilledRegion, execSize);

    // Unaligned region specific handling.
    unsigned int spillSendOption = InstOpt_WriteEnable;

    auto preloadNeeded = shouldPreloadSpillRange(*spilledInstIter, bb);
    auto scatterSpillCandidate = isScatterSpillCandidate();
    auto needsRMW = checkRMWNeeded();
    // Scatter spill is used only when the spill needs RMW.
    // If spill doesn't need RMW then we can use block spill as those
    // can be coalesced with nearby block spills.
    // If a fill was emitted then there's no need to use scatter as
    // we can use block spill that can be coalesced.
    auto useScatter = (scatterSpillCandidate && needsRMW);

    if (preloadNeeded && needsRMW && !scatterSpillCandidate) {

      // Preload the segment aligned spill range from memory to use
      // as an overlay

      preloadSpillRange(spillRangeDcl, mRangeDcl, spilledRegion, execSize);

      // Create the temporary range to use as a replacement range.

      G4_Declare *tmpRangeDcl =
          createTemporaryRangeDeclare(spilledRegion, execSize);

      // Copy out the value in the temporary range into its
      // location in the spill range.

      G4_DstRegRegion *spillRangeDstRegion = createSpillRangeDstRegion(
          spillRangeDcl->getRegVar(), spilledRegion, execSize);

      G4_SrcRegRegion *tmpRangeSrcRegion = createTemporaryRangeSrcRegion(
          tmpRangeDcl->getRegVar(), spilledRegion, execSize);

      // NOTE: Never use a predicate for the final mov if the spilled
      //       instruction was a sel (even in a SIMD CF context).

      G4_Predicate *predicate = ((*spilledInstIter)->opcode() != G4_sel)
                                    ? (*spilledInstIter)->getPredicate()
                                    : nullptr;

      if (tmpRangeSrcRegion->getType() == spillRangeDstRegion->getType() &&
          IS_TYPE_FLOAT_ALL(tmpRangeSrcRegion->getType())) {
        // use int copy when possible as floating-point copy moves may need
        // further legalization
        auto equivIntTy = floatToSameWidthIntType(tmpRangeSrcRegion->getType());
        tmpRangeSrcRegion->setType(*builder_, equivIntTy);
        spillRangeDstRegion->setType(*builder_, equivIntTy);
      }

      createMovInst(execSize, spillRangeDstRegion, tmpRangeSrcRegion,
                    builder_->duplicateOperand(predicate),
                    (*spilledInstIter)->getMaskOption());

      replacementRangeDcl = tmpRangeDcl;
      // maintain the spilled dst's subreg to not break the regioning
      // restriction
      newSubregOff = spilledRegion->getSubRegOff();
    } else {
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
      LocalLiveRange *spilledLLR =
          gra.getLocalLR(spilledRegion->getBase()->asRegVar()->getDeclare());
      if (spilledLLR && spilledLLR->getSplit()) {
        // if we are spilling the dest of a copy move introduced by local
        // live-range splitting, we can spill the source value instead and
        // delete the move ToDo: we should generalize this to cover all moves
        G4_SrcRegRegion *srcRegion = inst->getSrc(0)->asSrcRegRegion();
        G4_Declare *srcDcl = srcRegion->getBase()->asRegVar()->getDeclare();
        unsigned int lb = srcRegion->getLeftBound();
        unsigned int rb = srcRegion->getRightBound();

        G4_RegVar *regVar = NULL;
        if (srcRegion->getBase()->isRegVar()) {
          regVar = getRegVar(srcRegion);
        }

        if (gra.getSubRegAlign(srcDcl) == builder_->getGRFAlign() &&
            lb % builder_->numEltPerGRF<Type_UB>() == 0 &&
            (rb + 1) % builder_->numEltPerGRF<Type_UB>() == 0 &&
            (rb - lb + 1) == spillRangeDcl->getByteSize() && regVar &&
            !shouldSpillRegister(regVar)) {
          optimizeSplitLLR = true;
        }
      }

      replacementRangeDcl = spillRangeDcl;
      // maintain the spilled dst's subreg since the spill is done on a per-GRF
      // basis
      newSubregOff = spilledRegion->getSubRegOff();

      if (preloadNeeded && isUnalignedRegion(spilledRegion, execSize)) {
        // A dst region may be not need pre-fill, however, if it is unaligned,
        // we need to use non-zero sub-reg offset in newly created spill dcl.
        // This section of code computes sub-reg offset to use for such cases.
        // It is insufficient to simply use spilledRegion's sub-reg offset in
        // case the region dcl is an alias of another dcl. This typically
        // happens when 2 scalar dcls are merged by merge scalar pass, merged
        // dcl is spilled, and dominating def writes non-zeroth element.
        unsigned segmentDisp =
            getEncAlignedSegmentDisp(spilledRegion, execSize);
        unsigned regionDisp = getRegionDisp(spilledRegion);
        vASSERT(regionDisp >= segmentDisp);
        unsigned short subRegOff =
            (regionDisp - segmentDisp) / spilledRegion->getElemSize();
        vASSERT((regionDisp - segmentDisp) % spilledRegion->getElemSize() == 0);
        vASSERT(subRegOff * spilledRegion->getElemSize() +
                    getRegionByteSize(spilledRegion, execSize) <=
                2u * builder_->numEltPerGRF<Type_UB>());
        newSubregOff = subRegOff;
      }

      if ((!bb->isAllLaneActive() && !preloadNeeded) || useScatter) {
        spillSendOption = (*spilledInstIter)->getMaskOption();
      }
    }

    // Save the spill range to memory.

    initMWritePayload(spillRangeDcl, mRangeDcl, spilledRegion, execSize);

    if (gra.useLscForSpillFill) {
      spillSendInst = createLSCSpill(spillRangeDcl, spilledRegion, execSize,
                                     spillSendOption, useScatter);
    } else {
      spillSendInst = createSpillSendInstr(
          spillRangeDcl, mRangeDcl, spilledRegion, execSize, spillSendOption);
    }

    if (failSafeSpill_ && !builder_->avoidDstSrcOverlap()) {
      spillRegOffset_ = spillRegStart_;
    }
  }

  if (builder_->getOption(vISA_DoSplitOnSpill)) {
    if (inst->isRawMov()) {
      // check whether mov is copy in loop preheader or exit
      auto it = gra.splitResults.find(inst->getSrc(0)->getTopDcl());
      if (it != gra.splitResults.end()) {
        if ((*it).second.origDcl == spillDcl) {
          // srcRegion is a split var temp
          // this is a copy in either preheader or loop exit.
          // add it to list so we know it shouldnt be optimized
          // by spill cleanup.
          for (auto addedInst : builder_->instList) {
            (*it).second.insts[bb].insert(addedInst);
          }
        }
      }
    }
  }

  // Replace the spilled range with the spill range and insert spill
  // instructions.

  INST_LIST::iterator insertPos = std::next(spilledInstIter);
  replaceSpilledRange(replacementRangeDcl, spilledRegion, *spilledInstIter,
                      newSubregOff);

  splice(bb, insertPos, builder_->instList, curInst->getVISAId());

  if (optimizeSplitLLR && spillSendInst && spillSendInst->isSplitSend()) {
    // delete the move and spill the source instead. Note that we can't do this
    // if split send is not enabled, as payload contains header
    bb->erase(spilledInstIter);
    unsigned int pos = 1;
    spillSendInst->setSrc(inst->getSrc(0), pos);
  } else {
    splice(bb, spilledInstIter, builder_->instList, curInst->getVISAId());
  }
}

bool SpillManagerGRF::isScalarImmRespill(G4_Declare* spillDcl) const
{
  return gra.scalarSpills.find(spillDcl) != gra.scalarSpills.end();
}

bool SpillManagerGRF::immFill(G4_SrcRegRegion *filledRegion,
                              INST_LIST::iterator filledInstIter, G4_BB *bb,
                              G4_Declare *spillDcl) {
  if (isScalarImmRespill(spillDcl)) {
    // Don't rematerialize spillDcl if it's a spilled dcl from
    // earlier RA iteration.
    return false;
  }

  G4_INST *inst = *filledInstIter;
  auto sisIt = scalarImmSpill.find(spillDcl);
  if (sisIt != scalarImmSpill.end()) {
    G4_Declare *tempDcl = nullptr;
    auto &nearbyFill = scalarImmFillCache[spillDcl];
    if (!failSafeSpill_ && std::get<0>(nearbyFill) == bb &&
        (inst->getLexicalId() - std::get<2>(nearbyFill)) <=
            scalarImmReuseDistance) {
      tempDcl = std::get<1>(nearbyFill)->getDst()->getTopDcl();
    } else {
      // re-materialize the scalar immediate value
      auto dstType = sisIt->second.first;
      auto *imm = sisIt->second.second;
      tempDcl = builder_->createTempVar(1, dstType, spillDcl->getSubRegAlign());
      auto movInst = builder_->createMov(
          g4::SIMD1, builder_->createDstRegRegion(tempDcl, 1), imm,
          InstOpt_WriteEnable, false);
      bb->insertBefore(filledInstIter, movInst);
      nearbyFill = std::make_tuple(bb, movInst, inst->getLexicalId());
      // tempDcl is fill dcl that rematerializes scalar immediate.
      // If tempDcl spills in later RA iteration we shouldn't
      // try to rematerialize the value. Instead we should
      // insert regular spill/fill code for it.
      gra.scalarSpills.insert(tempDcl);
      vASSERT(!filledRegion->isIndirect());
    }
    auto newSrc = builder_->createSrc(
        tempDcl->getRegVar(), filledRegion->getRegOff(),
        filledRegion->getSubRegOff(), filledRegion->getRegion(),
        filledRegion->getType(), filledRegion->getAccRegSel());
    newSrc->setModifier(filledRegion->getModifier());
    int i = 0;
    for (; i < inst->getNumSrc(); ++i) {
      if (inst->getSrc(i) == filledRegion) {
        break;
      }
    }
    inst->setSrc(newSrc, i);

    if (failSafeSpill_) {
      if (!gra.kernel.getOption(vISA_NewFailSafeRA)) {
        tempDcl->getRegVar()->setPhyReg(
            builder_->phyregpool.getGreg(spillRegOffset_), 0);
        spillRegOffset_ += 1;
      } else {
        tempDcl->getRegVar()->setPhyReg(
            builder_->phyregpool.getGreg(context.getFreeGRF(1)), 0);
      }
    }
    return true;
  }
  return false;
}

// Create the code to create the GRF fill range and load it to spill memory.
void SpillManagerGRF::insertFillGRFRangeCode(G4_SrcRegRegion *filledRegion,
                                             INST_LIST::iterator filledInstIter,
                                             G4_BB *bb) {
  G4_ExecSize execSize = (*filledInstIter)->getExecSize();

  // Create the fill range, assign spill offset to the fill range and
  // create the instructions to load the fill range from spill memory.

  G4_Declare *fillRangeDcl = nullptr;

  bool optimizeSplitLLR = false;
  G4_INST *inst = *filledInstIter;
  vISA_ASSERT(!inst->isSpillIntrinsic(), "must take send path");
  auto dstRegion = inst->getDst();
  G4_INST *fillSendInst = nullptr;
  auto spillDcl = filledRegion->getTopDcl()->getRootDeclare();

  if (immFill(filledRegion, filledInstIter, bb, spillDcl)) {
    return;
  }

  {
    fillRangeDcl =
        createGRFFillRangeDeclare(filledRegion, execSize, *filledInstIter);
    G4_Declare *mRangeDcl = createAndInitMHeader(filledRegion, execSize);

    if (gra.useLscForSpillFill) {
      fillSendInst =
          createLSCFill(fillRangeDcl, filledRegion, execSize);
    } else {
      fillSendInst =
          createFillSendInstr(fillRangeDcl, mRangeDcl, filledRegion, execSize);
    }

    LocalLiveRange *filledLLR =
        gra.getLocalLR(filledRegion->getBase()->asRegVar()->getDeclare());
    if (filledLLR && filledLLR->getSplit()) {
      G4_Declare *dstDcl = dstRegion->getBase()->asRegVar()->getDeclare();
      unsigned int lb = dstRegion->getLeftBound();
      unsigned int rb = dstRegion->getRightBound();

      if (gra.getSubRegAlign(dstDcl) == builder_->getGRFAlign() &&
          lb % builder_->numEltPerGRF<Type_UB>() == 0 &&
          (rb + 1) % builder_->numEltPerGRF<Type_UB>() == 0 &&
          (rb - lb + 1) == fillRangeDcl->getByteSize()) {
        optimizeSplitLLR = true;
      }
    }
  }

  if (builder_->getOption(vISA_DoSplitOnSpill)) {
    if (inst->isRawMov()) {
      // check whether mov is copy in loop preheader or exit
      auto it = gra.splitResults.find(dstRegion->getTopDcl());
      if (it != gra.splitResults.end()) {
        if ((*it).second.origDcl == filledRegion->getTopDcl()) {
          // dstRegion is a split var temp
          // this is a copy in either preheader or loop exit.
          // add it to list so we know it shouldnt be optimized
          // by spill cleanup.
          for (auto addedInst : builder_->instList) {
            (*it).second.insts[bb].insert(addedInst);
          }
        }
      }
    }
  }

  // Replace the spilled range with the fill range and insert spill
  // instructions.
  replaceFilledRange(fillRangeDcl, filledRegion, *filledInstIter);
  INST_LIST::iterator insertPos = filledInstIter;

  splice(bb, insertPos, builder_->instList, curInst->getVISAId());
  if (optimizeSplitLLR) {
    INST_LIST::iterator nextIter = filledInstIter;
    INST_LIST::iterator prevIter = filledInstIter;
    nextIter++;
    prevIter--;
    prevIter--;
    bb->erase(filledInstIter);
    fillSendInst->setDest(dstRegion);
    G4_INST *prevInst = (*prevIter);
    if (prevInst->isPseudoKill() &&
        GetTopDclFromRegRegion(prevInst->getDst()) == fillRangeDcl) {
      prevInst->setDest(builder_->createDst(
          GetTopDclFromRegRegion(dstRegion)->getRegVar(), 0, 0, 1, Type_UD));
    }
  }
}

// Create the code to create the GRF fill range and load it to spill memory.
void SpillManagerGRF::insertSendFillRangeCode(
    G4_SrcRegRegion *filledRegion, INST_LIST::iterator filledInstIter,
    G4_BB *bb) {
  G4_INST *sendInst = *filledInstIter;
  auto spillDcl = filledRegion->getTopDcl()->getRootDeclare();

  if (immFill(filledRegion, filledInstIter, bb, spillDcl)) {
    return;
  }

  unsigned width =
      builder_->numEltPerGRF<Type_UB>() / filledRegion->getElemSize();

  // Create the fill range, assign spill offset to the fill range

  G4_Declare *fillGRFRangeDcl =
      createSendFillRangeDeclare(filledRegion, sendInst);

  // Create the instructions to load the fill range from spill memory.

  G4_Declare *mRangeDcl = createMRangeDeclare(filledRegion, G4_ExecSize(width));
  initMHeader(mRangeDcl);
  sendInSpilledRegVarPortions(fillGRFRangeDcl, mRangeDcl, 0,
                              fillGRFRangeDcl->getNumRows(),
                              filledRegion->getRegOff());

  // Replace the spilled range with the fill range and insert spill
  // instructions.

  replaceFilledRange(fillGRFRangeDcl, filledRegion, *filledInstIter);
  INST_LIST::iterator insertPos = filledInstIter;

  splice(bb, insertPos, builder_->instList, curInst->getVISAId());
}

G4_Declare *SpillManagerGRF::getOrCreateAddrSpillFillDcl(
    G4_RegVar *addrDcl, G4_Declare *spilledAddrTakenDcl, G4_Kernel *kernel) {
  auto pointsToSet = gra.pointsToAnalysis.getAllInPointsToAddrExps(addrDcl);
  G4_Declare *temp = nullptr;
  bool created = false;

  // Create a spill/fill declare for the AddrExps which are assigned to same
  // address varaible and with same addressed varaible. Note that we are trying
  // to capture senorious (A1, 1:&V1), (A2, 2::&V1), where "1:" and "2:" means
  // different AddrExp. IGC is trying to use different address registers.
  // Scenarios   (A1, 1:&V1), (A1, 2:&V1) may happen, by should be rare. In this
  // case, only one declare will be created.
  std::vector<G4_AddrExp *> newAddExpList;
  for (const auto &pt : *pointsToSet) {
    G4_AddrExp *addrExp = pt.exp;
    G4_Declare *dcl = addrExp->getRegVar()->getDeclare();
    while (dcl->getAliasDeclare()) {
      dcl = dcl->getAliasDeclare();
    }

    // The variable V is spilled
    if (dcl == spilledAddrTakenDcl) {
      G4_AddrExp *currentAddrExp = getAddrTakenSpillFill(addrExp);

      // Either all are created, or none.
      if (created) {
        // Either all addr expressoins are null, or all are not null.
        vASSERT(currentAddrExp == nullptr);
      }

      if (currentAddrExp != nullptr) {
        return currentAddrExp->getRegVar()->getDeclare();
        ;
      }

      // Create the spill fill variable
      if (!created) {
        // If spilledAddrTakenDcl already has a spill/fill range created, return
        // it. Else create new one and return it.
        const char *dclName = kernel->fg.builder->getNameString(
            32, "ADDR_SP_FL_V%d_%d", spilledAddrTakenDcl->getDeclId(),
            getAddrSpillFillIndex());

        // temp is created of sub-class G4_RegVarTmp so that is
        // assigned infinite spill cost when coloring.
        temp = kernel->fg.builder->createDeclare(
            dclName, G4_GRF, spilledAddrTakenDcl->getNumElems(),
            spilledAddrTakenDcl->getNumRows(),
            spilledAddrTakenDcl->getElemType(), DeclareType::Tmp,
            spilledAddrTakenDcl->getRegVar());
        temp->setAddrSpillFill();
        G4_AddrExp *newAddExp = builder_->createAddrExp(
            temp->getRegVar(), addrExp->getOffset(), addrExp->getType());
        setAddrTakenSpillFill(addrExp, newAddExp);
        newAddExpList.push_back(newAddExp);
        created = true;
      } else {
        G4_AddrExp *newAddExp = builder_->createAddrExp(
            temp->getRegVar(), addrExp->getOffset(), addrExp->getType());
        setAddrTakenSpillFill(addrExp, newAddExp);
        newAddExpList.push_back(newAddExp);
      }
    }
  }
  for (auto newAddExp : newAddExpList) {
    gra.pointsToAnalysis.patchPointsToSet(addrDcl, newAddExp,
                                          newAddExp->getOffset());
  }

  return temp;
}

// For each address taken register spill find an available physical register
// and assign it to the decl. This physical register will be used for inserting
// spill/fill code for indirect reference instructions that point to the
// spilled range.
// Return true if enough registers found, false if sufficient registers
// unavailable.
bool SpillManagerGRF::handleAddrTakenSpills(
    G4_Kernel *kernel, PointsToAnalysis &pointsToAnalysis) {
  bool success = true;
  unsigned int numAddrTakenSpills = 0;

  for (const LiveRange *lr : *spilledLRs_) {
    if (lvInfo_->isAddressSensitive(lr->getVar()->getId())) {
      numAddrTakenSpills++;
    }
  }

  if (numAddrTakenSpills > 0) {
    insertAddrTakenSpillFill(kernel, pointsToAnalysis);
    prunePointsTo(kernel, pointsToAnalysis);
  }

  return success;
}

unsigned int
SpillManagerGRF::handleAddrTakenLSSpills(G4_Kernel *kernel,
                                         PointsToAnalysis &pointsToAnalysis) {
  unsigned int numAddrTakenSpills = 0;

  for (LSLiveRange *lr : *spilledLSLRs_) {
    if (lvInfo_->isAddressSensitive(lr->getTopDcl()->getRegVar()->getId())) {
      numAddrTakenSpills++;
    }
  }

  if (numAddrTakenSpills > 0) {
    insertAddrTakenLSSpillFill(kernel, pointsToAnalysis);
    prunePointsToLS(kernel, pointsToAnalysis);
  }

  return numAddrTakenSpills;
}

// Insert spill and fill code for indirect GRF accesses
void SpillManagerGRF::insertAddrTakenSpillAndFillCode(
    G4_Kernel *kernel, G4_BB *bb, INST_LIST::iterator inst_it, G4_Operand *opnd,
    PointsToAnalysis &pointsToAnalysis, bool spill, unsigned int bbid) {
  curInst = (*inst_it);
  INST_LIST::iterator next_inst_it = ++inst_it;
  inst_it--;
  bool BBhasSpillCode = false;

  // Check whether spill operand points to any spilled range
  for (const LiveRange *lr : *spilledLRs_) {
    G4_RegVar *var = nullptr;

    if (opnd->isDstRegRegion() && opnd->asDstRegRegion()->getBase()->asRegVar())
      var = opnd->asDstRegRegion()->getBase()->asRegVar();

    if (opnd->isSrcRegRegion() && opnd->asSrcRegRegion()->getBase()->asRegVar())
      var = opnd->asSrcRegRegion()->getBase()->asRegVar();

    vISA_ASSERT(var != NULL, "Fill operand is neither a source nor dst region");

    if (var && pointsToAnalysis.isPresentInPointsTo(var, lr->getVar())) {
      BBhasSpillCode = true;
      unsigned int numrows = lr->getDcl()->getNumRows();
      G4_Declare *temp = getOrCreateAddrSpillFillDcl(var, lr->getDcl(), kernel);

      if (failSafeSpill_) {
        if (!kernel->getOption(vISA_NewFailSafeRA) &&
            !temp->getRegVar()->getPhyReg()) {
          temp->getRegVar()->setPhyReg(
              builder_->phyregpool.getGreg(spillRegOffset_), 0);
          spillRegOffset_ += numrows;
        } else {
          context.setInst(curInst, bb);
          context.prev = inst_it;
          if (curInst == bb->front())
            context.prev = bb->begin();
          else
            --context.prev;
          context.next = next_inst_it;
          context.setAddrDcl(var->getDeclare());
          if (!temp->getRegVar()->getPhyReg()) {
            // choose GRFs to assign to new range
            temp->getRegVar()->setPhyReg(
                builder_->phyregpool.getGreg(context.getFreeGRFIndir(numrows)),
                0);
          }
          context.markClobbered(
              temp->getRegVar()->getPhyReg()->asGreg()->getRegNum(), numrows);
          context.resetAddrDcl();
        }
      }

      if (numrows > 1 ||
          (lr->getDcl()->getNumElems() * lr->getDcl()->getElemSize() ==
           builder_->getGRFSize())) {
        if (useScratchMsg_ || useSplitSend()) {
          G4_Declare *fillGRFRangeDcl = temp;
          G4_Declare *mRangeDcl = createAndInitMHeader(
              (G4_RegVarTransient *)temp->getRegVar()->getBaseRegVar());

          kernel->fg.builder->createPseudoKill(temp, PseudoKillType::Other,
                                               true);

          sendInSpilledRegVarPortions(fillGRFRangeDcl, mRangeDcl, 0,
                                      temp->getNumRows(), 0);

          splice(bb, inst_it, builder_->instList, curInst->getVISAId());

          if (spill) {
            sendOutSpilledRegVarPortions(temp, mRangeDcl, 0, temp->getNumRows(),
                                         0);

            splice(bb, next_inst_it, builder_->instList, curInst->getVISAId());
          }
        } else {

          for (unsigned int i = 0; i < numrows; i++) {
            G4_INST *inst;
            const RegionDesc *rd = kernel->fg.builder->getRegionStride1();
            G4_ExecSize curExSize{builder_->numEltPerGRF<Type_UD>()};

            if ((i + 1) < numrows)
              curExSize = G4_ExecSize(builder_->numEltPerGRF<Type_UD>() * 2);

            G4_SrcRegRegion *srcRex = kernel->fg.builder->createSrc(
                lr->getVar(), (short)i, 0, rd, Type_F);

            G4_DstRegRegion *dstRex = kernel->fg.builder->createDst(
                temp->getRegVar(), (short)i, 0, 1, Type_F);

            inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex,
                                                 InstOpt_WriteEnable, false);

            bb->insertBefore(inst_it, inst);

            if (spill) {
              // Also insert spill code
              G4_SrcRegRegion *srcRex = kernel->fg.builder->createSrc(
                  temp->getRegVar(), (short)i, 0, rd, Type_F);

              G4_DstRegRegion *dstRex = kernel->fg.builder->createDst(
                  lr->getVar(), (short)i, 0, 1, Type_F);

              inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex,
                                                   InstOpt_WriteEnable, false);

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
        // pointsToAnalysis.addFillToPointsTo(bbid, var, temp->getRegVar());
      } else if (numrows == 1) {
        // Insert spill/fill when there decl uses a single row, that too not
        // completely
        G4_ExecSize curExSize = g4::SIMD16;
        unsigned short numbytes =
            lr->getDcl()->getNumElems() * lr->getDcl()->getElemSize();

        // temp->setAddressed();
        short off = 0;

        while (numbytes > 0) {
          G4_INST *inst;
          G4_Type type = Type_W;

          if (numbytes >= 16)
            curExSize = g4::SIMD8;
          else if (numbytes >= 8 && numbytes < 16)
            curExSize = g4::SIMD4;
          else if (numbytes >= 4 && numbytes < 8)
            curExSize = g4::SIMD2;
          else if (numbytes >= 2 && numbytes < 4)
            curExSize = g4::SIMD1;
          else if (numbytes == 1) {
            // If a region has odd number of bytes, copy last byte in final
            // iteration
            curExSize = g4::SIMD1;
            type = Type_UB;
          } else {
            vISA_ASSERT(false, "Cannot emit SIMD1 for byte");
            curExSize = G4_ExecSize(0);
          }

          const RegionDesc *rd = kernel->fg.builder->getRegionStride1();

          G4_SrcRegRegion *srcRex =
              kernel->fg.builder->createSrc(lr->getVar(), 0, off, rd, type);

          G4_DstRegRegion *dstRex =
              kernel->fg.builder->createDst(temp->getRegVar(), 0, off, 1, type);

          inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex,
                                               InstOpt_WriteEnable, false);

          bb->insertBefore(inst_it, inst);

          if (spill) {
            // Also insert spill code
            G4_SrcRegRegion *srcRex = kernel->fg.builder->createSrc(
                temp->getRegVar(), 0, off, rd, type);

            G4_DstRegRegion *dstRex =
                kernel->fg.builder->createDst(lr->getVar(), 0, off, 1, type);

            inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex,
                                                 InstOpt_WriteEnable, false);

            bb->insertBefore(next_inst_it, inst);
          }

          off += curExSize;
          numbytes -= curExSize * 2;
        }

        // Update points to
        // pointsToAnalysis.addFillToPointsTo(bbid, var, temp->getRegVar());
      }

      if (!spill) {
        // Insert pseudo_use node so that liveness keeps the
        // filled variable live through the indirect access.
        // Not required for spill because for spill we will
        // anyway insert a ues of the variable to emit store.
        const RegionDesc *rd = kernel->fg.builder->getRegionScalar();

        G4_SrcRegRegion *pseudoUseSrc =
            kernel->fg.builder->createSrc(temp->getRegVar(), 0, 0, rd, Type_F);

        G4_INST *pseudoUseInst =
            kernel->fg.builder->createInternalIntrinsicInst(
                nullptr, Intrinsic::Use, g4::SIMD1, nullptr, pseudoUseSrc,
                nullptr, nullptr, InstOpt_NoOpt);

        bb->insertBefore(next_inst_it, pseudoUseInst);
      }

      if (failSafeSpill_ && builder_->getOption(vISA_NewFailSafeRA)) {
        context.insertPushPop(gra.useLscForSpillFill);
      }
    }
  }

  if (BBhasSpillCode)
    gra.addSpillCodeInBB(bb);
}

// Insert spill and fill code for indirect GRF accesses
void SpillManagerGRF::insertAddrTakenLSSpillAndFillCode(
    G4_Kernel *kernel, G4_BB *bb, INST_LIST::iterator inst_it, G4_Operand *opnd,
    PointsToAnalysis &pointsToAnalysis, bool spill, unsigned int bbid) {
  curInst = (*inst_it);
  INST_LIST::iterator next_inst_it = ++inst_it;
  inst_it--;

  // Check whether spill operand points to any spilled range
  for (LSLiveRange *lr : *spilledLSLRs_) {
    G4_RegVar *var = nullptr;

    if (opnd->isDstRegRegion() && opnd->asDstRegRegion()->getBase()->asRegVar())
      var = opnd->asDstRegRegion()->getBase()->asRegVar();

    if (opnd->isSrcRegRegion() && opnd->asSrcRegRegion()->getBase()->asRegVar())
      var = opnd->asSrcRegRegion()->getBase()->asRegVar();

    vISA_ASSERT(var != NULL, "Fill operand is neither a source nor dst region");

    if (var && pointsToAnalysis.isPresentInPointsTo(
                   var, lr->getTopDcl()->getRegVar())) {
      unsigned int numrows = lr->getTopDcl()->getNumRows();
      G4_Declare *temp =
          getOrCreateAddrSpillFillDcl(var, lr->getTopDcl(), kernel);
      vISA_ASSERT(temp != nullptr,
                  "Failed to get the fill variable for indirect GRF access");
      if (failSafeSpill_ && temp->getRegVar()->getPhyReg() == NULL) {

        temp->getRegVar()->setPhyReg(
            builder_->phyregpool.getGreg(spillRegOffset_), 0);
        spillRegOffset_ += numrows;
      }

      if (numrows > 1 ||
          (lr->getTopDcl()->getNumElems() * lr->getTopDcl()->getElemSize() ==
           builder_->getGRFSize())) {
        if (useScratchMsg_ || useSplitSend()) {
          G4_Declare *fillGRFRangeDcl = temp;
          G4_Declare *mRangeDcl = createAndInitMHeader(
              (G4_RegVarTransient *)temp->getRegVar()->getBaseRegVar());

          sendInSpilledRegVarPortions(fillGRFRangeDcl, mRangeDcl, 0,
                                      temp->getNumRows(), 0);

          splice(bb, inst_it, builder_->instList, curInst->getVISAId());

          if (spill) {
            sendOutSpilledRegVarPortions(temp, mRangeDcl, 0, temp->getNumRows(),
                                         0);

            splice(bb, next_inst_it, builder_->instList, curInst->getVISAId());
          }
        } else {

          for (unsigned int i = 0; i < numrows; i++) {
            G4_INST *inst;
            const RegionDesc *rd = kernel->fg.builder->getRegionStride1();
            G4_ExecSize curExSize{builder_->numEltPerGRF<Type_UD>()};

            if ((i + 1) < numrows)
              curExSize = G4_ExecSize(builder_->numEltPerGRF<Type_UD>() * 2);

            G4_SrcRegRegion *srcRex = kernel->fg.builder->createSrc(
                lr->getTopDcl()->getRegVar(), (short)i, 0, rd, Type_F);

            G4_DstRegRegion *dstRex = kernel->fg.builder->createDst(
                temp->getRegVar(), (short)i, 0, 1, Type_F);

            inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex,
                                                 InstOpt_WriteEnable, false);

            bb->insertBefore(inst_it, inst);

            if (spill) {
              // Also insert spill code
              G4_SrcRegRegion *srcRex = kernel->fg.builder->createSrc(
                  temp->getRegVar(), (short)i, 0, rd, Type_F);

              G4_DstRegRegion *dstRex = kernel->fg.builder->createDst(
                  lr->getTopDcl()->getRegVar(), (short)i, 0, 1, Type_F);

              inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex,
                                                   InstOpt_WriteEnable, false);

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
        // pointsToAnalysis.addFillToPointsTo(bbid, var, temp->getRegVar());
      } else if (numrows == 1) {
        // Insert spill/fill when there decl uses a single row, that too not
        // completely
        G4_ExecSize curExSize = g4::SIMD16;
        unsigned short numbytes =
            lr->getTopDcl()->getNumElems() * lr->getTopDcl()->getElemSize();

        // temp->setAddressed();
        short off = 0;

        while (numbytes > 0) {
          G4_INST *inst;
          G4_Type type = Type_W;

          if (numbytes >= 16)
            curExSize = g4::SIMD8;
          else if (numbytes >= 8 && numbytes < 16)
            curExSize = g4::SIMD4;
          else if (numbytes >= 4 && numbytes < 8)
            curExSize = g4::SIMD2;
          else if (numbytes >= 2 && numbytes < 4)
            curExSize = g4::SIMD1;
          else if (numbytes == 1) {
            // If a region has odd number of bytes, copy last byte in final
            // iteration
            curExSize = g4::SIMD1;
            type = Type_UB;
          } else {
            vISA_ASSERT(false, "Cannot emit SIMD1 for byte");
            curExSize = G4_ExecSize(0);
          }

          const RegionDesc *rd = kernel->fg.builder->getRegionStride1();

          G4_SrcRegRegion *srcRex = kernel->fg.builder->createSrc(
              lr->getTopDcl()->getRegVar(), 0, off, rd, type);

          G4_DstRegRegion *dstRex =
              kernel->fg.builder->createDst(temp->getRegVar(), 0, off, 1, type);

          inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex,
                                               InstOpt_WriteEnable, false);

          bb->insertBefore(inst_it, inst);

          if (spill) {
            // Also insert spill code
            G4_SrcRegRegion *srcRex = kernel->fg.builder->createSrc(
                temp->getRegVar(), 0, off, rd, type);

            G4_DstRegRegion *dstRex = kernel->fg.builder->createDst(
                lr->getTopDcl()->getRegVar(), 0, off, 1, type);

            inst = kernel->fg.builder->createMov(curExSize, dstRex, srcRex,
                                                 InstOpt_WriteEnable, false);

            bb->insertBefore(next_inst_it, inst);
          }

          off += curExSize;
          numbytes -= curExSize * 2;
        }

        // Update points to
        // pointsToAnalysis.addFillToPointsTo(bbid, var, temp->getRegVar());
      }

      if (!spill) {
        // Insert pseudo_use node so that liveness keeps the
        // filled variable live through the indirect access.
        // Not required for spill because for spill we will
        // anyway insert a ues of the variable to emit store.
        const RegionDesc *rd = kernel->fg.builder->getRegionScalar();

        G4_SrcRegRegion *pseudoUseSrc =
            kernel->fg.builder->createSrc(temp->getRegVar(), 0, 0, rd, Type_F);

        G4_INST *pseudoUseInst =
            kernel->fg.builder->createInternalIntrinsicInst(
                nullptr, Intrinsic::Use, g4::SIMD1, nullptr, pseudoUseSrc,
                nullptr, nullptr, InstOpt_NoOpt);

        bb->insertBefore(next_inst_it, pseudoUseInst);
      }
    }
  }
}

// Insert any spill/fills for address taken
void SpillManagerGRF::insertAddrTakenSpillFill(
    G4_Kernel *kernel, PointsToAnalysis &pointsToAnalysis) {
  if (failSafeSpill_) {
    if (kernel->getOption(vISA_NewFailSafeRA)) {
      context.markIndirIntfs();
    }
  }

  for (auto bb : kernel->fg) {
    for (INST_LIST_ITER inst_it = bb->begin(); inst_it != bb->end();
         inst_it++) {
      G4_INST *curInst = (*inst_it);

      if (failSafeSpill_) {
        spillRegOffset_ = indrSpillRegStart_;
      }

      // Handle indirect destination
      G4_DstRegRegion *dst = curInst->getDst();

      if (dst && dst->getRegAccess() == IndirGRF) {
        insertAddrTakenSpillAndFillCode(kernel, bb, inst_it, dst,
                                        pointsToAnalysis, true, bb->getId());
      }

      for (int i = 0, numSrc = curInst->getNumSrc(); i < numSrc; i++) {
        G4_Operand *src = curInst->getSrc(i);

        if (src && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getRegAccess() == IndirGRF) {
          insertAddrTakenSpillAndFillCode(kernel, bb, inst_it, src,
                                          pointsToAnalysis, false, bb->getId());
        }
      }
    }
  }
}

void SpillManagerGRF::insertAddrTakenLSSpillFill(
    G4_Kernel *kernel, PointsToAnalysis &pointsToAnalysis) {
  for (auto bb : kernel->fg) {
    for (INST_LIST_ITER inst_it = bb->begin(); inst_it != bb->end();
         inst_it++) {
      G4_INST *curInst = (*inst_it);

      if (failSafeSpill_) {
        spillRegOffset_ = indrSpillRegStart_;
      }

      // Handle indirect destination
      G4_DstRegRegion *dst = curInst->getDst();

      if (dst && dst->getRegAccess() == IndirGRF) {
        insertAddrTakenLSSpillAndFillCode(kernel, bb, inst_it, dst,
                                          pointsToAnalysis, true, bb->getId());
      }

      for (int i = 0, numSrc = curInst->getNumSrc(); i < numSrc; i++) {
        G4_Operand *src = curInst->getSrc(i);

        if (src && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getRegAccess() == IndirGRF) {
          insertAddrTakenLSSpillAndFillCode(
              kernel, bb, inst_it, src, pointsToAnalysis, false, bb->getId());
        }
      }
    }
  }
}

// For address spill/fill code inserted remove from points of each indirect
// operand the original regvar that is spilled.
void SpillManagerGRF::prunePointsTo(G4_Kernel *kernel,
                                    PointsToAnalysis &pointsToAnalysis) {
  for (auto bb : kernel->fg) {
    for (INST_LIST_ITER inst_it = bb->begin(); inst_it != bb->end();
         inst_it++) {
      G4_INST *curInst = (*inst_it);
      std::stack<G4_Operand *> st;

      // Handle indirect destination
      G4_DstRegRegion *dst = curInst->getDst();

      if (dst && dst->getRegAccess() == IndirGRF) {
        st.push(dst);
      }

      for (int i = 0, numSrc = curInst->getNumSrc(); i < numSrc; i++) {
        G4_Operand *src = curInst->getSrc(i);

        if (src && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getRegAccess() == IndirGRF) {
          st.push(src);
        }

        // Replace the variable in the address expression with fill variable
        if (src && src->isAddrExp()) {
          G4_AddrExp *addExp = getAddrTakenSpillFill(src->asAddrExp());
          if (addExp != nullptr) {
            curInst->setSrc(addExp, i);
          }
        }
      }

      while (st.size() > 0) {
        G4_Operand *cur = st.top();
        st.pop();

        // Check whether spill operand points to any spilled range
        for (const LiveRange *lr : *spilledLRs_) {
          G4_RegVar *var = nullptr;

          if (cur->isDstRegRegion() &&
              cur->asDstRegRegion()->getBase()->asRegVar())
            var = cur->asDstRegRegion()->getBase()->asRegVar();

          if (cur->isSrcRegRegion() &&
              cur->asSrcRegRegion()->getBase()->asRegVar())
            var = cur->asSrcRegRegion()->getBase()->asRegVar();

          vISA_ASSERT(var != nullptr,
                      "Operand is neither a source nor dst region");

          if (var && pointsToAnalysis.isPresentInPointsTo(var, lr->getVar())) {
            // Remove this from points to
            pointsToAnalysis.removeFromPointsTo(var, lr->getVar());
          }
        }
      }
    }
  }
}

void SpillManagerGRF::prunePointsToLS(G4_Kernel *kernel,
                                      PointsToAnalysis &pointsToAnalysis) {
  for (auto bb : kernel->fg) {
    for (INST_LIST_ITER inst_it = bb->begin(); inst_it != bb->end();
         inst_it++) {
      G4_INST *curInst = (*inst_it);
      std::stack<G4_Operand *> st;

      // Handle indirect destination
      G4_DstRegRegion *dst = curInst->getDst();

      if (dst && dst->getRegAccess() == IndirGRF) {
        st.push(dst);
      }

      for (int i = 0, numSrc = curInst->getNumSrc(); i < numSrc; i++) {
        G4_Operand *src = curInst->getSrc(i);

        if (src && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getRegAccess() == IndirGRF) {
          st.push(src);
        }

        // Replace the variable in the address expression with fill variable
        if (src && src->isAddrExp()) {
          G4_AddrExp *addExp = getAddrTakenSpillFill(src->asAddrExp());
          if (addExp != nullptr) {
            curInst->setSrc(addExp, i);
          }
        }
      }

      while (st.size() > 0) {
        G4_Operand *cur = st.top();
        st.pop();

        // Check whether spill operand points to any spilled range
        for (LSLiveRange *lr : *spilledLSLRs_) {
          G4_RegVar *var = nullptr;

          if (cur->isDstRegRegion() &&
              cur->asDstRegRegion()->getBase()->asRegVar())
            var = cur->asDstRegRegion()->getBase()->asRegVar();

          if (cur->isSrcRegRegion() &&
              cur->asSrcRegRegion()->getBase()->asRegVar())
            var = cur->asSrcRegRegion()->getBase()->asRegVar();

          vISA_ASSERT(var != NULL,
                      "Operand is neither a source nor dst region");

          if (var && pointsToAnalysis.isPresentInPointsTo(
                         var, lr->getTopDcl()->getRegVar())) {
            // Remove this from points to
            pointsToAnalysis.removeFromPointsTo(var,
                                                lr->getTopDcl()->getRegVar());
          }
        }
      }
    }
  }
}

void SpillManagerGRF::immMovSpillAnalysis() {
  if (!builder_->getOption(vISA_FillConstOpt))
    return;

  std::unordered_set<G4_Declare *> spilledDcl;
  scalarImmSpill.clear();

  for (auto bb : gra.kernel.fg) {
    for (auto inst : *bb) {
      if (inst->isPseudoKill())
        continue;
      auto dst = inst->getDst();
      auto dcl = dst && dst->getTopDcl() ? dst->getTopDcl()->getRootDeclare()
                                         : nullptr;
      if (!dcl || dcl->getAddressed() || dcl->getNumElems() != 1 ||
          !shouldSpillRegister(dcl->getRegVar())) {
        // declare must be a scalar without address taken
        continue;
      }
      if (spilledDcl.count(dcl)) {
        // this spilled declare is defined more than once
        scalarImmSpill.erase(dcl);
        continue;
      }
      spilledDcl.insert(dcl);
      if (immFillCandidate(inst)) {
        scalarImmSpill[dcl] =
            std::make_pair(dst->getType(), inst->getSrc(0)->asImm());
      }
    }
  }
}

bool vISA::isEOTSpillWithFailSafeRA(const IR_Builder &builder,
                                    const LiveRange *lr, bool isFailSafeIter) {
  bool needsEOTGRF = lr->getEOTSrc() && builder.hasEOTGRFBinding();
  if (isFailSafeIter && needsEOTGRF &&
      (lr->getVar()->isRegVarTransient() || lr->getVar()->isRegVarTmp() ||
       lr->getIsInfiniteSpillCost()))
    return true;
  return false;
}

// Insert spill/fill code for all registers that have not been assigned
// physical registers in the current iteration of the graph coloring
// allocator.
// returns false if spill fails somehow
bool SpillManagerGRF::insertSpillFillCode(G4_Kernel *kernel,
                                          PointsToAnalysis &pointsToAnalysis) {
  immMovSpillAnalysis();

  //  Set the spill flag of all spilled regvars.
  for (const LiveRange *lr : *spilledLRs_) {

    // Ignore request to spill/fill the spill/fill ranges
    // as it does not help the allocator.
    if (shouldSpillRegister(lr->getVar()) == false) {
      if (isEOTSpillWithFailSafeRA(*builder_, lr, failSafeSpill_)) {
        if (!builder_->getOption(vISA_NewFailSafeRA)) {
          lr->getVar()->setPhyReg(
              builder_->phyregpool.getGreg(
                  spillRegStart_ > (kernel->getNumRegTotal() - 16)
                      ? spillRegStart_
                      : (kernel->getNumRegTotal() - 16)),
              0);
        }
        continue;
      }
      return false;
    } else {
      lr->getVar()->getDeclare()->setSpillFlag();
      gra.incRA.markForIntfUpdate(lr->getDcl());
    }
  }

  if (doSpillSpaceCompression) {
    // cache spilling intervals in sorted order so we can use these
    // for spill compression.
    for (auto& segment : spillIntf_->getAugmentation().getSortedLiveIntervals()) {
      auto *dcl = segment.dcl;
      if (dcl->isSpilled())
        spillingIntervals.push_back(dcl);
    }
  }

  if (failSafeSpill_ && builder_->getOption(vISA_NewFailSafeRA)) {
    context.computeAllBusy();
  }

  // Handle address taken spills
  bool success = handleAddrTakenSpills(kernel, pointsToAnalysis);

  if (!success) {
    // FIXME: this should be an assert?
    VISA_DEBUG(std::cout << "Enough physical register not available for "
                            "handling address taken spills\n");
    return false;
  }

  if (kernel->getOption(vISA_DoSplitOnSpill)) {
    // remove all spilled splits
    for (const LiveRange *lr : *spilledLRs_) {
      auto dcl = lr->getDcl();
      // check whether spilled variable is one of split vars
      if (gra.splitResults.find(dcl) == gra.splitResults.end())
        continue;

      gra.incRA.markForIntfUpdate(dcl);
      LoopVarSplit::removeAllSplitInsts(&gra, dcl);
    }
  }

  // Insert spill/fill code for all basic blocks.
  updateRMWNeeded();
  FlowGraph &fg = kernel->fg;

  unsigned int id = 0;
  for (BB_LIST_ITER it = fg.begin(); it != fg.end(); it++) {
    bbId_ = (*it)->getId();
    INST_LIST::iterator jt = (*it)->begin();
    bool BBhasSpillCode = false;

    while (jt != (*it)->end()) {
      INST_LIST::iterator kt = jt;
      ++kt;
      G4_INST *inst = *jt;

      curInst = inst;
      curInst->setLexicalId(id++);

      if (failSafeSpill_) {
        spillRegOffset_ = spillRegStart_;
        if (kernel->getOption(vISA_NewFailSafeRA)) {
          context.setInst(curInst, (*it));
          context.next = kt;
          if ((*it)->front() == inst)
            context.prev = (*it)->begin();
          else {
            auto prev = jt;
            context.prev = --prev;
          }
        }
      }

      // Insert spill code, when the target is a spilled register.

      if (inst->getDst()) {
        G4_RegVar *regVar = nullptr;
        if (inst->getDst()->getBase()->isRegVar()) {
          regVar = getRegVar(inst->getDst());
        }

        if (regVar && shouldSpillRegister(regVar)) {
          if (getRFType(regVar) == G4_GRF) {
            if (inst->isPseudoKill()) {
              (*it)->erase(jt);
              jt = kt;
              continue;
            }
            insertSpillRangeCode(jt, (*it));
            BBhasSpillCode = true;
          } else {
            vASSERT(false);
          }
        }
      }

      // Insert fill code, when the source is a spilled register.

      for (unsigned i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
        if (inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion()) {
          auto srcRR = inst->getSrc(i)->asSrcRegRegion();
          G4_RegVar *regVar = nullptr;
          if (srcRR->getBase()->isRegVar()) {
            regVar = getRegVar(srcRR);
          }

          if (regVar && shouldSpillRegister(regVar)) {
            if (inst->isLifeTimeEnd()) {
              (*it)->erase(jt);
              break;
            }
            bool useSendFill = (inst->isSend() && i == 0) || inst->isDpas() ||
                               (inst->isSplitSend() && i == 1) ||
                               (inst->isSpillIntrinsic());

            if (useSendFill) {
              // this path may be taken if current instruction is a spill
              // intrinsic. for eg, V81 below is not spilled in 1st RA
              // iteration:
              // clang-format off
              // mov (16)             SP_GRF_V167_0(0,0)<1>:w   V81(0,0)<1;1,0>:w
              // (W) intrinsic.spill.1 (16)  Scratch[0x32]  R0_Copy0(0,0)<1;1,0>:ud  SP_GRF_V167_0(0,0)<1;1,0>:w
              // mov (16)             SP_GRF_V167_1(0,0)<1>:w  V81(1,0)<1;1,0>:w
              // (W) intrinsic.spill.1 (16)  Scratch[1x32]  R0_Copy0(0,0)<1;1,0>:ud  SP_GRF_V167_1(0,0)<1;1,0>:w
              // mov (16)             SP_GRF_V167_2(0,0)<1>:w  V81(2,0)<1;1,0>:w
              // (W) intrinsic.spill.1 (16)  Scratch[2x32]  R0_Copy0(0,0)<1;1,0>:ud  SP_GRF_V167_2(0,0)<1;1,0>:w
              // mov (16)             SP_GRF_V167_3(0,0)<1>:w  V81(3,0)<1;1,0>:w
              // (W) intrinsic.spill.1 (16)  Scratch[3x32]  R0_Copy0(0,0)<1;1,0>:ud  SP_GRF_V167_3(0,0)<1;1,0>:w
              //
              // ==> spill cleanup emits (notice spill range V81 appears in
              // spill intrinsic but is not spilled itself):
              //
              // (W) intrinsic.spill.4 (16)  Scratch[0x32]  R0_Copy0(0,0)<1;1,0>:ud  V81(0,0)<1;1,0>:ud
              //
              // in next RA iteration assume V81 spills. so we should emit:
              //
              // (W) intrinsic.fill.4 (16)  FL_Send_V81_0(0,0)<1>:uw  R0_Copy0(0,0)<1;1,0>:ud  Scratch[88x32]
              // (W) intrinsic.spill.4 (16)  Scratch[0x32]  R0_Copy0(0,0)<1;1,0>:ud  FL_Send_V81_0(0,0)<1;1,0>:ud
              // clang-format on
              //
              // V81 appears in spill intrinsic instruction as an optimization
              // earlier even though it wasnt spilled in 1st RA iteration.
              // however, since it spills in later RA iteration, we should
              // handle it like send.
              //
              // If we don't handle it like send then the intrinsic.fill would
              // use incorrect size as non-send fill code only looks at filled
              // region properties like hstride, exec size, elem size. See:
              // getSegmentByteSize().
              insertSendFillRangeCode(srcRR, jt, *it);
              BBhasSpillCode = true;
            } else if (getRFType(regVar) == G4_GRF) {
              insertFillGRFRangeCode(srcRR, jt, *it);
              BBhasSpillCode = true;
            } else
              vASSERT(false);
          }
        }
      }

      if (failSafeSpill_ && builder_->getOption(vISA_NewFailSafeRA)) {
        context.insertPushPop(gra.useLscForSpillFill);
      }

      jt = kt;
    }

    if (BBhasSpillCode)
      gra.addSpillCodeInBB(*it);
  }

  bbId_ = UINT_MAX;

  // Calculate the spill memory used in this iteration

  for (auto spill : *spilledLRs_) {
    unsigned disp = spill->getVar()->getDisp();

    if (spill->getVar()->isSpilled()) {
      if (disp != UINT_MAX) {
        nextSpillOffset_ =
            std::max(nextSpillOffset_, disp + getByteSize(spill->getVar()));
      }
    }
  }

  return true;
}

void SpillManagerGRF::expireRanges(unsigned int idx,
                                   std::list<LSLiveRange *> *liveList) {
  // active list is sorted in ascending order of starting index

  while (liveList->size() > 0) {
    unsigned int endIdx;
    LSLiveRange *lr = liveList->front();

    lr->getLastRef(endIdx);

    if (endIdx <= idx) {
      VISA_DEBUG_VERBOSE(std::cout << "Expiring range "
                                   << lr->getTopDcl()->getName() << "\n");
      // Remove range from active list
      liveList->pop_front();
      lr->setActiveLR(false);
    } else {
      // As soon as we find first range that ends after ids break loop
      break;
    }
  }

  return;
}

void SpillManagerGRF::updateActiveList(LSLiveRange *lr,
                                       std::list<LSLiveRange *> *liveList) {
  bool done = false;
  unsigned int newlr_end;

  lr->getLastRef(newlr_end);

  for (auto active_it = liveList->begin(); active_it != liveList->end();
       active_it++) {
    unsigned int end_idx;
    LSLiveRange *active_lr = (*active_it);

    active_lr->getLastRef(end_idx);

    if (end_idx > newlr_end) {
      liveList->insert(active_it, lr);
      done = true;
      break;
    }
  }

  if (done == false)
    liveList->push_back(lr);
}

bool SpillManagerGRF::spillLiveRanges(G4_Kernel *kernel) {
  // Set the spill flag of all spilled regvars.
  for (LSLiveRange *lr : *spilledLSLRs_) {
    lr->getTopDcl()->setSpillFlag();
  }

  // Handle address taken spills
  unsigned addrSpillNum = handleAddrTakenLSSpills(kernel, gra.pointsToAnalysis);

  if (addrSpillNum) {
    for (auto spill : *spilledLSLRs_) {
      unsigned disp = spill->getTopDcl()->getRegVar()->getDisp();

      if (spill->getTopDcl()->getRegVar()->isSpilled()) {
        if (disp != UINT_MAX) {
          nextSpillOffset_ =
              std::max(nextSpillOffset_,
                       disp + getByteSize(spill->getTopDcl()->getRegVar()));
        }
      }
    }
  }

  // Insert spill/fill code for all basic blocks.
  FlowGraph &fg = kernel->fg;
  for (BB_LIST_ITER it = fg.begin(); it != fg.end(); it++) {
    bbId_ = (*it)->getId();
    INST_LIST::iterator jt = (*it)->begin();

    while (jt != (*it)->end()) {
      INST_LIST::iterator kt = jt;
      ++kt;
      G4_INST *inst = *jt;
      curInst = inst;

      if (failSafeSpill_) {
        spillRegOffset_ = spillRegStart_;
      }

      // Insert spill code, when the target is a spilled register.
      if (inst->getDst()) {
        G4_RegVar *regVar = nullptr;
        if (inst->getDst()->getBase()->isRegVar()) {
          regVar = getRegVar(inst->getDst());
        }

        if (regVar && regVar->getDeclare()->isSpilled()) {
          G4_Declare *dcl = regVar->getDeclare();
          while (dcl->getAliasDeclare()) {
            dcl = dcl->getAliasDeclare();
          }

          if (getRFType(regVar) == G4_GRF) {
            if (inst->isPseudoKill()) {
              (*it)->erase(jt);
              jt = kt;
              continue;
            }

            insertSpillRangeCode(jt, (*it));
          } else {
            vASSERT(false);
          }
        }
      }

      // Insert fill code, when the source is a spilled register.
      for (unsigned i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
        if (inst->getSrc(i) && inst->getSrc(i)->isSrcRegRegion()) {
          auto srcRR = inst->getSrc(i)->asSrcRegRegion();
          G4_RegVar *regVar = nullptr;
          if (srcRR->getBase()->isRegVar()) {
            regVar = getRegVar(srcRR);
          }

          if (regVar && regVar->getDeclare()->isSpilled()) {
            G4_Declare *dcl = regVar->getDeclare();
            while (dcl->getAliasDeclare()) {
              dcl = dcl->getAliasDeclare();
            }

            if (inst->isLifeTimeEnd()) {
              (*it)->erase(jt);
              break;
            }
            bool mayExceedTwoGRF = (inst->isSend() && i == 0) ||
                                   inst->isDpas() ||
                                   (inst->isSplitSend() && i == 1);

            if (mayExceedTwoGRF) {
              insertSendFillRangeCode(srcRR, jt, *it);
            } else if (getRFType(regVar) == G4_GRF)
              insertFillGRFRangeCode(srcRR, jt, *it);
            else
              vASSERT(false);
          }
        }
      }

      jt = kt;
    }
  }

  bbId_ = UINT_MAX;

  // Calculate the spill memory used in this iteration
  for (auto spill : (*spilledLSLRs_)) {
    unsigned disp = spill->getTopDcl()->getRegVar()->getDisp();

    if (spill->getTopDcl()->getRegVar()->isSpilled()) {
      if (disp != UINT_MAX) {
        nextSpillOffset_ =
            std::max(nextSpillOffset_,
                     disp + getByteSize(spill->getTopDcl()->getRegVar()));
      }
    }
  }

  return true;
}

//
// For XeHP_SDV and later, scratch surface is used for the vISA stack.  This
// means when the scratch message cannot be used for spill/fill (e.g., stack
// call), a0.2 will be used as the message descriptor for the spill/fill. As
// address RA is done before GRF, we don't know if a0.2 is live at the point of
// the spill/fill inst and thus may need to preserve its value. The good news is
// that all spill/fill may share the same A0, so we only need to save/restore A0
// when it's actually referenced in the BB.
//
void GlobalRA::saveRestoreA0(G4_BB *bb) {
  G4_Declare *tmpDcl = nullptr;
  unsigned int subReg = 0;
  if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc()) {
    // Use r126.6:ud for storing old a0.2 as it isn't caller/callee save
    tmpDcl = builder.kernel.fg.getScratchRegDcl();
    subReg = 6;
  } else {
    vISA_ASSERT(builder.hasValidOldA0Dot2(), "old a0.2 not saved");
    tmpDcl = builder.getOldA0Dot2Temp();
    subReg = 0;
  }

  auto usesAddr = [](G4_INST *inst) {
    // ToDo: handle send with A0 msg desc better.
    if (inst->isSpillIntrinsic() || inst->isFillIntrinsic()) {
      return false;
    }
    if (inst->getDst() &&
        (inst->getDst()->isIndirect() || inst->getDst()->isDirectAddress())) {
      return true;
    }
    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
      if (!inst->getSrc(i) || !inst->getSrc(i)->isSrcRegRegion())
        continue;
      auto src = inst->getSrc(i)->asSrcRegRegion();
      if (src->isDirectAddress() || src->isIndirect())
        return true;
    }
    return false;
  };

  // a0.2 is spilled to r126.6 (r126 is scratch reg reserved for stack call)
  auto a0SaveMov = [this, tmpDcl, subReg]() {
    auto dstSave =
        builder.createDst(tmpDcl->getRegVar(), 0, subReg, 1, Type_UD);
    auto srcSave = builder.createSrc(builder.getBuiltinA0Dot2()->getRegVar(), 0,
                                     0, builder.getRegionScalar(), Type_UD);
    auto saveInst = builder.createMov(g4::SIMD1, dstSave, srcSave,
                                      InstOpt_WriteEnable, false);
    return saveInst;
  };

  auto a0RestoreMov = [this, tmpDcl, subReg]() {
    auto dstRestore = builder.createDstRegRegion(builder.getBuiltinA0Dot2(), 1);
    auto srcRestore = builder.createSrc(tmpDcl->getRegVar(), 0, subReg,
                                        builder.getRegionScalar(), Type_UD);
    auto restoreInst = builder.createMov(g4::SIMD1, dstRestore, srcRestore,
                                         InstOpt_WriteEnable, false);
    return restoreInst;
  };

  auto a0SSOMove = [this]() {
    // SSO is stored in r126.7
    auto dst = builder.createDstRegRegion(builder.getBuiltinA0Dot2(), 1);
    auto SSOsrc =
        builder.createSrc(builder.getSpillSurfaceOffset()->getRegVar(), 0, 0,
                          builder.getRegionScalar(), Type_UD);
    {
      // shr (1) a0.2   SSO   0x4 {NM}
      auto imm4 = builder.createImm(4, Type_UD);
      return builder.createBinOp(G4_shr, g4::SIMD1, dst, SSOsrc, imm4,
                                 InstOpt_WriteEnable, false);
    }
  };

  auto isPrologOrEpilog = [this](G4_INST *inst) {
    // a0 is a caller save register. Don't save/restore it if it is used in
    // callee save/restore sequence or for frame descriptor spill instruction.
    if (inst == kernel.fg.builder->getFDSpillInst())
      return false;

    if (calleeSaveInsts.find(inst) != calleeSaveInsts.end() ||
        calleeRestoreInsts.find(inst) != calleeRestoreInsts.end())
      return false;

    return true;
  };

  bool hasActiveSpillFill = false;

  for (auto instIt = bb->begin(); instIt != bb->end(); ++instIt) {
    auto inst = (*instIt);

    if (inst->isSpillIntrinsic() || inst->isFillIntrinsic()) {
      if (!hasActiveSpillFill) {
        // save a0.2 to addrSpillLoc, then overwrite it with the scratch surface
        // offset
        if (isPrologOrEpilog(inst)) {
          auto addrSpill = a0SaveMov();
          bb->insertBefore(instIt, addrSpill);
        }
        auto a0SSO = a0SSOMove();
        bb->insertBefore(instIt, a0SSO);
        // Need Call WA.
        // NoMask WA: save/restore code does not need NoMask WA.
        if (EUFusionCallWANeeded()) {
          addEUFusionCallWAInst(a0SSO);
        }
        hasActiveSpillFill = true;
      }
    } else if (hasActiveSpillFill && usesAddr(inst)) {
      // restore A0
      auto addrFill = a0RestoreMov();
      bb->insertBefore(instIt, addrFill);
      hasActiveSpillFill = false;
    }
  }

  if (hasActiveSpillFill && !bb->isLastInstEOT() && !bb->isEndWithFRet()) {
    // restore A0 before BB exit. BB is guaranteed to be non-empty as there's at
    // least one spill/fill If last inst is branch, insert restore before it.
    // Otherwise insert it as last inst
    auto endIt = bb->back()->isCFInst() ? std::prev(bb->end()) : bb->end();
    bb->insertBefore(endIt, a0RestoreMov());
  }
}

static uint32_t computeSpillMsgDesc(unsigned int payloadSize,
                                    unsigned int offsetInGrfUnits,
                                    const IR_Builder &irb) {
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

static uint32_t computeFillMsgDesc(unsigned int payloadSize,
                                   unsigned int offsetInGrfUnits,
                                   const IR_Builder &irb) {
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
static unsigned int getPayloadSizeGRF(unsigned int numRows) {
  if (numRows >= 8)
    return 8u;

  if (numRows >= 4)
    return 4u;

  if (numRows >= 2)
    return 2u;

  return 1u;
}

static unsigned int getPayloadSizeOword(unsigned int numOwords) {
  if (numOwords >= 8)
    return 8u;

  if (numOwords >= 4)
    return 4u;

  if (numOwords >= 2)
    return 2u;

  return 1u;
}

unsigned int GlobalRA::owordToGRFSize(unsigned int numOwords,
                                      const IR_Builder &builder) {
  unsigned int GRFSize =
      numOwords / (2 * (builder.numEltPerGRF<Type_UB>() / HWORD_BYTE_SIZE));

  return GRFSize;
}

unsigned int GlobalRA::hwordToGRFSize(unsigned int numHwords,
                                      const IR_Builder &builder) {
  return owordToGRFSize(numHwords * 2, builder);
}

unsigned int GlobalRA::GRFToHwordSize(unsigned int numGRFs,
                                      const IR_Builder &builder) {
  return GRFSizeToOwords(numGRFs, builder) / 2;
}

unsigned int GlobalRA::GRFSizeToOwords(unsigned int numGRFs,
                                       const IR_Builder &builder) {
  return numGRFs * (builder.numEltPerGRF<Type_UB>() / OWORD_BYTE_SIZE);
}

unsigned int GlobalRA::getHWordByteSize() { return HWORD_BYTE_SIZE; }

bool Interval::intervalsOverlap(const Interval &second) const {
  return !(start->getLexicalId() > second.end->getLexicalId() ||
           second.start->getLexicalId() > end->getLexicalId());
}

static G4_INST *createSpillFillAddr(IR_Builder &builder, G4_Declare *addr,
                                    G4_Declare *fp, int offset) {
  auto imm = builder.createImm(offset, Type_UD);
  auto dst = builder.createDstRegRegion(addr, 1);
  if (fp) {
    auto src0 = builder.createSrcRegRegion(fp, builder.getRegionScalar());
    return builder.createBinOp(G4_add, g4::SIMD1, dst, src0, imm,
                               InstOpt_WriteEnable, true);
  } else {
    return builder.createMov(g4::SIMD1, dst, imm, InstOpt_WriteEnable, true);
  }
}

static std::string makeSpillFillComment(const char *spillFill,
                                        const char *toFrom, const char *base,
                                        uint32_t spillOffset, const char *of,
                                        unsigned grfSize) {
  std::stringstream comment;
  comment << spillFill << " " << toFrom << " ";
  comment << base << "[" << spillOffset / grfSize << "*" << (int)grfSize << "]";
  if (!of || *of == 0) // some have "" as name
    of = "?";
  comment << " of " << of;
  return comment.str();
}

void GlobalRA::expandSpillLSC(G4_BB *bb, INST_LIST_ITER &instIt) {
  auto &builder = kernel.fg.builder;
  auto inst = (*instIt)->asSpillIntrinsic();
  // offset into scratch surface in bytes
  auto spillOffset = inst->getOffsetInBytes();
  uint32_t numRows = inst->getNumRows();
  auto payload = inst->getSrc(1)->asSrcRegRegion();
  auto rowOffset = payload->getRegOff();

  LSC_OP op = LSC_STORE;
  LSC_SFID lscSfid = LSC_UGM;
  LSC_CACHE_OPTS cacheOpts{LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
  LSC_L1_L3_CC store_cc =
      (LSC_L1_L3_CC)builder->getuint32Option(vISA_lscSpillStoreCCOverride);
  if (store_cc != LSC_CACHING_DEFAULT) {
    cacheOpts = convertLSCLoadStoreCacheControlEnum(store_cc, false);
  }

  LSC_ADDR addrInfo;
  addrInfo.type = LSC_ADDR_TYPE_SS; // Scratch memory
  addrInfo.immScale = 1;
  addrInfo.immOffset = 0;
  addrInfo.size = LSC_ADDR_SIZE_32b;
  if (canUseLscImmediateOffsetSpillFill) {
    // spillOffset must be Dword aligned
    // spillOffset must be in range [0, SPILL_FILL_IMMOFF_MAX * 2)
    vISA_ASSERT(spillOffset % 4 == 0 && spillOffset < SPILL_FILL_IMMOFF_MAX * 2,
                "invalid immediate offset");

    // immOffset range for SS: [-2^16, 2^16-1]
    addrInfo.immOffset = spillOffset - SPILL_FILL_IMMOFF_MAX;
  }

  builder->instList.clear();
  while (numRows > 0) {
    auto numGRFToWrite = getPayloadSizeGRF(numRows);

    G4_Declare *spillAddr = inst->getFP() ? kernel.fg.scratchRegDcl
                                          : inst->getHeader()->getTopDcl();
    if (!canUseLscImmediateOffsetSpillFill)
    {
      // need to calculate spill address
      createSpillFillAddr(*builder, spillAddr, inst->getFP(), spillOffset);
    }

    unsigned int elemSize = 4;
    LSC_DATA_SHAPE dataShape;
    dataShape.size = LSC_DATA_SIZE_32b;
    if (builder->getGRFSize() > 32 && numGRFToWrite > 4) {
      if (inst->isWriteEnableInst()) {
        elemSize = 8;
        dataShape.size = LSC_DATA_SIZE_64b;
      } else {
        // don't change type if instruction isn't WriteEnable
        numGRFToWrite = 4;
      }
    }
    dataShape.order = LSC_DATA_ORDER_TRANSPOSE;
    dataShape.elems = builder->lscGetElementNum(
        numGRFToWrite * builder->getGRFSize() / elemSize);

    auto src0Addr =
        builder->createSrcRegRegion(spillAddr, builder->getRegionStride1());
    auto payloadToUse = builder->createSrcWithNewRegOff(payload, rowOffset);

    auto surface = builder->createSrcRegRegion(builder->getSpillSurfaceOffset(),
                                               builder->getRegionScalar());

    G4_DstRegRegion *postDst = builder->createNullDst(Type_UD);
    G4_SendDescRaw *desc = builder->createLscMsgDesc(
        op, lscSfid, EXEC_SIZE_1, cacheOpts, addrInfo, dataShape, surface, 0, 1,
        LdStAttrs::SCRATCH_SURFACE);

    auto sendInst = builder->createLscSendInst(
        nullptr, postDst, src0Addr, payloadToUse, g4::SIMD1, desc,
        inst->getOption(), LSC_ADDR_TYPE_SS, 0x0, false);

    sendInst->addComment(makeSpillFillComment(
        "spill", "to", inst->getFP() ? "FP" : "offset", spillOffset,
        payload->getTopDcl()->getName(), builder->getGRFSize()));

    numRows -= numGRFToWrite;
    rowOffset += numGRFToWrite;
    spillOffset += numGRFToWrite * builder->getGRFSize();
    if (canUseLscImmediateOffsetSpillFill) {
      addrInfo.immOffset = spillOffset - SPILL_FILL_IMMOFF_MAX;
    }
  }

  if (inst->getFP() && kernel.getOption(vISA_GenerateDebugInfo)) {
    kernel.getKernelDebugInfo()->updateExpandedIntrinsic(
        inst->asSpillIntrinsic(), builder->instList);
  }

  // Call WA and NoMask WA are mutual exclusive.
  if (getEUFusionCallWAInsts().count(inst) > 0) {
    removeEUFusionCallWAInst(inst);
    for (auto inst : builder->instList)
      addEUFusionCallWAInst(inst);
  } else if (getEUFusionNoMaskWAInsts().count(inst) > 0) {
    // no NoMask WA needed for stack call spill/fill
    removeEUFusionNoMaskWAInst(inst);
    if (!inst->getFP() && inst->isWriteEnableInst()) {
      for (auto inst : builder->instList)
        addEUFusionNoMaskWAInst(bb, inst);
    }
  }

  splice(bb, instIt, builder->instList, inst->getVISAId());
} // expandSpillLSC

void GlobalRA::expandScatterSpillLSC(G4_BB *bb, INST_LIST_ITER &instIt) {
  auto &builder = kernel.fg.builder;
  auto inst = (*instIt)->asSpillIntrinsic();
  // offset into scratch surface in bytes
  auto spillOffset = inst->getOffsetInBytes();
  auto payload = inst->getSrc(1)->asSrcRegRegion();
  auto rowOffset = payload->getRegOff();

  // Max elements to write in LSC scatter store is 32 (SIMD32)
  // Scatter spill intrinsics are expanded to either SIMD8, SIMD16 or SIMD32
  G4_ExecSize execSize(inst->getExecSize());
  vISA_ASSERT(execSize == 8 || execSize == 16 || execSize == 32,
              "Execution size not supported for scatter spill");

  LSC_OP op = LSC_STORE;
  LSC_SFID lscSfid = LSC_UGM;
  LSC_CACHE_OPTS cacheOpts{LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
  LSC_L1_L3_CC store_cc =
      (LSC_L1_L3_CC)builder->getuint32Option(vISA_lscSpillStoreCCOverride);
  if (store_cc != LSC_CACHING_DEFAULT) {
    cacheOpts = convertLSCLoadStoreCacheControlEnum(store_cc, false);
  }

  // Set the LSC address info
  LSC_ADDR addrInfo;
  addrInfo.type = LSC_ADDR_TYPE_SS; // Scratch memory
  addrInfo.immScale = 1;
  addrInfo.immOffset = 0;
  addrInfo.size = LSC_ADDR_SIZE_32b;
  unsigned int addrSizeInBytes = 4;

  // Set the LSC data shape
  LSC_DATA_SHAPE dataShape;
  dataShape.order = LSC_DATA_ORDER_NONTRANSPOSE;
  dataShape.elems = LSC_DATA_ELEMS_1;

  auto elemSz = inst->getDst()->getTypeSize();
  switch (elemSz) {
  case 2:
    dataShape.size = LSC_DATA_SIZE_16b;
    break;
  case 4:
    dataShape.size = LSC_DATA_SIZE_32b;
    break;
  case 8:
    dataShape.size = LSC_DATA_SIZE_64b;
    break;
  default:
    vISA_ASSERT(false, "Data size not supported");
  }

  unsigned numGRFAddressToWrite =
      execSize * addrSizeInBytes / builder->getGRFSize();
  builder->instList.clear();

  // Add spill offset to vector of addresses
  G4_Declare *baseAddresses = builder->getScatterSpillBaseAddress();
  G4_Declare *spillAddresses = builder->getScatterSpillAddress();
  G4_INST *addA0 = builder->createBinOp(
      G4_add, execSize,
      builder->createDst(spillAddresses->getRegVar(), 0, 0, 1, Type_D),
      builder->createSrcRegRegion(baseAddresses, builder->getRegionStride1()),
      builder->createImm(spillOffset, Type_W), inst->getOption(), false);
  bb->insertBefore(instIt, addA0);

  auto payloadToUse = builder->createSrcWithNewRegOff(payload, rowOffset);
  auto surface = builder->createSrcRegRegion(builder->getSpillSurfaceOffset(),
                                             builder->getRegionScalar());

  G4_DstRegRegion *postDst =
      builder->createNullDst(inst->getDst()->getType());
  G4_SendDescRaw *desc = builder->createLscMsgDesc(
      op, lscSfid, Get_VISA_Exec_Size_From_Raw_Size(execSize), cacheOpts,
      addrInfo, dataShape, surface, 0, numGRFAddressToWrite,
      LdStAttrs::SCRATCH_SURFACE);
  auto src0Addr =
      builder->createSrcRegRegion(spillAddresses, builder->getRegionStride1());

  auto sendInst = builder->createLscSendInst(
      nullptr, postDst, src0Addr, payloadToUse, execSize, desc,
      inst->getOption(), LSC_ADDR_TYPE_SS, 0x0, false);

  sendInst->addComment(makeSpillFillComment(
      "scatter spill", "to", inst->getFP() ? "FP" : "offset", spillOffset,
      payload->getTopDcl()->getName(), builder->getGRFSize()));

  if (inst->getFP() && kernel.getOption(vISA_GenerateDebugInfo)) {
    kernel.getKernelDebugInfo()->updateExpandedIntrinsic(
        inst->asSpillIntrinsic(), builder->instList);
  }

  splice(bb, instIt, builder->instList, inst->getVISAId());
}

void GlobalRA::expandFillLSC(G4_BB *bb, INST_LIST_ITER &instIt) {
  auto &builder = kernel.fg.builder;
  auto inst = (*instIt)->asFillIntrinsic();
  // offset into scratch surface in bytes
  auto fillOffset = inst->getOffsetInBytes();
  uint32_t numRows = inst->getNumRows();
  auto rowOffset = inst->getDst()->getRegOff();

  LSC_OP op = LSC_LOAD;
  LSC_SFID lscSfid = LSC_UGM;
  LSC_CACHE_OPTS cacheOpts{LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
  LSC_L1_L3_CC ld_cc =
      (LSC_L1_L3_CC)builder->getuint32Option(vISA_lscSpillLoadCCOverride);
  if (ld_cc != LSC_CACHING_DEFAULT) {
    cacheOpts = convertLSCLoadStoreCacheControlEnum(ld_cc, true);
  }

  LSC_ADDR addrInfo;
  addrInfo.type = LSC_ADDR_TYPE_SS; // Scratch memory
  addrInfo.immScale = 1;
  addrInfo.immOffset = 0;
  addrInfo.size = LSC_ADDR_SIZE_32b;
  if (canUseLscImmediateOffsetSpillFill) {
    // fillOffset must be Dword aligned
    // fillOffset must be in range [0, SPILL_FILL_IMMOFF_MAX * 2)
    vISA_ASSERT(fillOffset % 4 == 0 && fillOffset < SPILL_FILL_IMMOFF_MAX * 2,
                "invalid immediate offset");

    // immOffset range for SS: [-2^16, 2^16-1]
    addrInfo.immOffset = fillOffset - SPILL_FILL_IMMOFF_MAX;
  }

  builder->instList.clear();

  while (numRows > 0) {
    unsigned int elemSize = 4;
    unsigned responseLength = getPayloadSizeGRF(numRows);
    LSC_DATA_SHAPE dataShape;
    dataShape.size = LSC_DATA_SIZE_32b;
    if (builder->getGRFSize() > 32 && responseLength > 4) {
      if (inst->isWriteEnableInst()) {
        elemSize = 8;
        dataShape.size = LSC_DATA_SIZE_64b;
      } else {
        // don't change type if instruction isn't WriteEnable
        responseLength = 4;
      }
    }
    dataShape.order = LSC_DATA_ORDER_TRANSPOSE;
    dataShape.elems = builder->lscGetElementNum(
        responseLength * builder->getGRFSize() / elemSize);
    G4_Declare *fillAddr = inst->getFP() ? kernel.fg.scratchRegDcl
                                         : inst->getHeader()->getTopDcl();
    if (!canUseLscImmediateOffsetSpillFill)
    {
      // need to calculate fill address
      createSpillFillAddr(*builder, fillAddr, inst->getFP(), fillOffset);
    }
    auto dstRead = builder->createDst(inst->getDst()->getTopDcl()->getRegVar(),
                                      (short)rowOffset, 0, 1, Type_UD);

    auto surface = builder->createSrcRegRegion(builder->getSpillSurfaceOffset(),
                                               builder->getRegionScalar());

    G4_SendDescRaw *desc = builder->createLscMsgDesc(
        op, lscSfid, EXEC_SIZE_1, cacheOpts, addrInfo, dataShape, surface,
        responseLength, 1, LdStAttrs::SCRATCH_SURFACE);

    auto sendInst = builder->createLscSendInst(
        nullptr, dstRead,
        builder->createSrcRegRegion(fillAddr, builder->getRegionScalar()),
        nullptr, g4::SIMD1, desc, inst->getOption(), LSC_ADDR_TYPE_SS, 0x0,
        false);

    sendInst->addComment(makeSpillFillComment(
        "fill", "from", inst->getFP() ? "FP" : "offset", fillOffset,
        dstRead->getTopDcl()->getName(), builder->getGRFSize()));

    numRows -= responseLength;
    rowOffset += responseLength;
    fillOffset += responseLength * builder->getGRFSize();
    if (canUseLscImmediateOffsetSpillFill) {
      addrInfo.immOffset = fillOffset - SPILL_FILL_IMMOFF_MAX;
    }
  }

  if (inst->getFP() && kernel.getOption(vISA_GenerateDebugInfo)) {
    kernel.getKernelDebugInfo()->updateExpandedIntrinsic(
        inst->asFillIntrinsic(), builder->instList);
  }

  // Call WA and NoMask WA are mutual exclusive.
  if (getEUFusionCallWAInsts().count(inst) > 0) {
    removeEUFusionCallWAInst(inst);
    for (auto inst : builder->instList)
      addEUFusionCallWAInst(inst);
  } else if (getEUFusionNoMaskWAInsts().count(inst) > 0) {
    removeEUFusionNoMaskWAInst(inst);
    // no WA needed for stack call spill/fill
    if (!inst->getFP() && inst->isWriteEnableInst()) {
      for (auto inst : builder->instList)
        addEUFusionNoMaskWAInst(bb, inst);
    }
  }

  splice(bb, instIt, builder->instList, inst->getVISAId());
}

void GlobalRA::insertSlot1HwordR0Set(G4_BB *bb, INST_LIST_ITER &instIt) {
  auto &builder = kernel.fg.builder;
  G4_INST *inst = (*instIt);

  if (slot1SetR0.count(inst)) {
    // Insert
    //   (W)    add (1|M0)    r0.5:ud    r0.5:ud    0x400:ud
    // to make r0.5 point at slot#1 scratch surface.
    G4_DstRegRegion* r0_5Dst = builder->createDst(
        builder->getBuiltinR0()->getRegVar(), 0, 5, 1, Type_UD);
    G4_Imm* addImm = builder->createImm(0x400, Type_UD);
    G4_SrcRegRegion *r0_5Src = builder->createSrc(
        builder->getBuiltinR0()->getRegVar(), 0, 5,
        builder->getRegionScalar(), Type_UD);
    G4_InstOption options =
        bb->isAllLaneActive() ? InstOpt_NoOpt : InstOpt_WriteEnable;
    G4_INST *addInst = builder->createBinOp(
        G4_opcode::G4_add, g4::SIMD1, r0_5Dst, r0_5Src, addImm,
        options, false);

    bb->insertBefore(instIt, addInst);

    if (EUFusionNoMaskWANeeded() && options & InstOpt_WriteEnable) {
      addEUFusionNoMaskWAInst(bb, addInst);
    }
  }
}

void GlobalRA::insertSlot1HwordR0Reset(G4_BB *bb, INST_LIST_ITER &instIt) {
  auto &builder = kernel.fg.builder;
  G4_INST *inst = (*instIt);

  if (slot1ResetR0.count(inst)) {
    // Insert
    //   (W)    add (1|M0)    r0.5:ud    r0.5:ud    -1024:d
    // to reset modification done by `insertSlot1HwordR0Set` and make r0.5
    // point to slot#1 scratch surface for private memory access.
    G4_DstRegRegion* r0_5Dst = builder->createDst(
        builder->getBuiltinR0()->getRegVar(), 0, 5, 1, Type_UD);
    G4_Imm *addImm = builder->createImm(-0x400, Type_D);
    G4_SrcRegRegion *r0_5Src = builder->createSrc(
        builder->getBuiltinR0()->getRegVar(), 0, 5,
        builder->getRegionScalar(), Type_UD);
    G4_InstOption options =
        bb->isAllLaneActive() ? InstOpt_NoOpt : InstOpt_WriteEnable;
    G4_INST *addInst = builder->createBinOp(
        G4_opcode::G4_add, g4::SIMD1, r0_5Dst, r0_5Src, addImm,
        options, false);

    bb->insertBefore(instIt, addInst);

    if (EUFusionNoMaskWANeeded() && options & InstOpt_WriteEnable) {
      addEUFusionNoMaskWAInst(bb, addInst);
    }
  }
}

void GlobalRA::expandSpillNonStackcall(uint32_t numRows, uint32_t offset,
                                       short rowOffset, G4_SrcRegRegion *header,
                                       G4_SrcRegRegion *payload, G4_BB *bb,
                                       INST_LIST_ITER &instIt) {
  auto &builder = kernel.fg.builder;
  auto inst = (*instIt);

  if (offset == G4_SpillIntrinsic::InvalidOffset) {
    // oword msg
    auto payloadToUse = builder->createSrcRegRegion(*payload);
    auto [spillMsgDesc, execSize] =
        SpillManagerGRF::createSpillSendMsgDescOWord(*builder, numRows);
    G4_INST *sendInst = nullptr;
    // Use bindless for XeHP_SDV and later
    if (builder->hasScratchSurface()) {
      G4_Imm *descImm =
          createMsgDesc(GRFSizeToOwords(numRows, *builder), true, true);
      // Update BTI to 251
      auto spillMsgDesc = descImm->getInt();
      spillMsgDesc = spillMsgDesc & 0xffffff00;
      spillMsgDesc |= 251;

      auto msgDesc = builder->createWriteMsgDesc(
          SFID::DP_DC0, (uint32_t)spillMsgDesc, numRows);
      G4_Imm *msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);

      // a0 is set by saveRestoreA0()
      auto a0Src = builder->createSrcRegRegion(builder->getBuiltinA0Dot2(),
                                               builder->getRegionScalar());
      sendInst = builder->createInternalSplitSendInst(
          execSize, inst->getDst(), header, payloadToUse, msgDescImm,
          inst->getOption(), msgDesc, a0Src);
    } else {
      G4_SendDescRaw *msgDesc = kernel.fg.builder->createSendMsgDesc(
          spillMsgDesc & 0x000FFFFFu, 0, 1, SFID::DP_DC0, numRows, 0,
          SendAccess::WRITE_ONLY);
      G4_Imm *msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);
      G4_Imm *extDesc = builder->createImm(msgDesc->getExtendedDesc(), Type_UD);
      sendInst = builder->createInternalSplitSendInst(
          execSize, inst->getDst(), header, payloadToUse, msgDescImm,
          inst->getOption(), msgDesc, extDesc);
    }
    instIt = bb->insertBefore(instIt, sendInst);
    if (EUFusionNoMaskWANeeded() && sendInst->isWriteEnableInst()) {
      addEUFusionNoMaskWAInst(bb, sendInst);
    }
  } else {
    INST_LIST_ITER origInstIt = instIt;
    insertSlot1HwordR0Set(bb, instIt);

    while (numRows >= 1) {
      auto payloadToUse = builder->createSrcWithNewRegOff(payload, rowOffset);

      auto region = builder->getRegionStride1();

      uint32_t spillMsgDesc =
          computeSpillMsgDesc(getPayloadSizeGRF(numRows), offset, *builder);
      auto msgDesc = builder->createWriteMsgDesc(SFID::DP_DC0, spillMsgDesc,
                                                 getPayloadSizeGRF(numRows));
      G4_Imm *msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);

      G4_SrcRegRegion *headerOpnd =
          builder->createSrcRegRegion(builder->getBuiltinR0(), region);
      G4_Imm *extDesc = builder->createImm(msgDesc->getExtendedDesc(), Type_UD);
      G4_ExecSize execSize = numRows > 1 ? g4::SIMD16 : g4::SIMD8;

      auto sendInst = builder->createInternalSplitSendInst(
          execSize, inst->getDst(), headerOpnd, payloadToUse, msgDescImm,
          inst->getOption(), msgDesc, extDesc);

      std::stringstream comments;
      comments << "scratch space spill: "
               << payloadToUse->getTopDcl()->getName() << " from offset["
               << offset << "x32]";
      sendInst->addComment(comments.str());

      instIt = bb->insertBefore(instIt, sendInst);

      if (EUFusionNoMaskWANeeded() && sendInst->isWriteEnableInst()) {
        addEUFusionNoMaskWAInst(bb, sendInst);
      }

      numRows -= getPayloadSizeGRF(numRows);
      offset += getPayloadSizeGRF(numRows);
      rowOffset += getPayloadSizeGRF(numRows);
    }

    insertSlot1HwordR0Reset(bb, origInstIt);
  }
}

void GlobalRA::expandSpillStackcall(uint32_t numRows, uint32_t offset,
                                    short rowOffset, G4_SrcRegRegion *payload,
                                    G4_BB *bb, INST_LIST_ITER &instIt) {
  auto &builder = kernel.fg.builder;
  auto inst = (*instIt);

  auto spillIt = instIt;

  // Use oword ld for stackcall. Lower intrinsic to:
  // (W)      add(1 | M0)         r126.2 < 1 > :ud  r125.7 < 0; 1, 0 > : ud  0x0
  // : ud (W)      sends(8 | M0)         null : ud       r126 payload - src2
  // 0x4A      0x20A02FF
  G4_Operand *src0 = nullptr;
  G4_Imm *src1 = nullptr;
  G4_Declare *scratchRegDcl = builder->kernel.fg.scratchRegDcl;
  G4_Declare *framePtr = inst->asSpillIntrinsic()->getFP();

  // convert hword to oword offset
  auto numRowsOword = numRows * 2;
  auto offsetOword = offset * 2;
  auto rowOffsetOword = rowOffset * 2;

  while (numRowsOword >= 1) {
    auto createOwordSpill = [&](unsigned int owordSize,
                                G4_SrcRegRegion *payloadToUse) {
      G4_ExecSize execSize = (owordSize > 2) ? g4::SIMD16 : g4::SIMD8;
      G4_DstRegRegion *dst =
          builder->createNullDst((execSize > g4::SIMD8) ? Type_UW : Type_UD);
      auto sendSrc0 =
          builder->createSrc(scratchRegDcl->getRegVar(), 0, 0,
                             builder->rgnpool.createRegion(8, 8, 1), Type_UD);
      unsigned messageLength = owordToGRFSize(owordSize, *builder);
      G4_Imm *descImm = createMsgDesc(owordSize, true, true);
      G4_INST *sendInst = nullptr;
      // Use bindless for XeHP_SDV and later
      if (builder->getPlatform() >= Xe_XeHPSDV) {
        // Update BTI to 251
        auto spillMsgDesc = descImm->getInt();
        spillMsgDesc = spillMsgDesc & 0xffffff00;
        spillMsgDesc |= 251;

        auto msgDesc = builder->createWriteMsgDesc(
            SFID::DP_DC0, (uint32_t)spillMsgDesc, messageLength);
        G4_Imm *msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);

        // a0 is set by saveRestoreA0()
        auto a0Src = builder->createSrcRegRegion(builder->getBuiltinA0Dot2(),
                                                 builder->getRegionScalar());
        sendInst = builder->createInternalSplitSendInst(
            execSize, inst->getDst(), sendSrc0, payloadToUse, msgDescImm,
            inst->getOption(), msgDesc, a0Src);
      } else {
        auto msgDesc = builder->createWriteMsgDesc(
            SFID::DP_DC0, (uint32_t)descImm->getInt(), messageLength);
        G4_Imm *msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);
        G4_Imm *extDesc =
            builder->createImm(msgDesc->getExtendedDesc(), Type_UD);
        sendInst = builder->createInternalSplitSendInst(
            execSize, dst, sendSrc0, payloadToUse, msgDescImm,
            inst->getOption() | InstOpt_WriteEnable, msgDesc, extDesc);
      }
      return sendInst;
    };

    auto payloadSizeInOwords = getPayloadSizeOword(numRowsOword);

    auto payloadToUse =
        builder->createSrcWithNewRegOff(payload, rowOffsetOword / 2);

    G4_DstRegRegion *dst =
        builder->createDst(scratchRegDcl->getRegVar(), 0, 2, 1, Type_UD);

    G4_INST *hdrSetInst = nullptr;
    if (inst->asSpillIntrinsic()->isOffsetValid()) {
      // Skip header if spill module emits its own header
      if (framePtr) {
        src0 = builder->createSrc(framePtr->getRegVar(), 0, 0,
                                  builder->getRegionScalar(), Type_UD);
        src1 = builder->createImm(offsetOword, Type_UD);
        hdrSetInst = builder->createBinOp(G4_add, g4::SIMD1, dst, src0, src1,
                                          InstOpt_WriteEnable, false);
      } else {
        src0 = builder->createImm(offsetOword, Type_UD);
        hdrSetInst = builder->createMov(g4::SIMD1, dst, src0,
                                        InstOpt_WriteEnable, false);
      }

      bb->insertBefore(spillIt, hdrSetInst);
    }

    auto spillSends = createOwordSpill(payloadSizeInOwords, payloadToUse);
    std::stringstream comments;
    comments << "stack spill: " << payload->getTopDcl()->getName() << " to FP["
             << inst->asSpillIntrinsic()->getOffset() << "x32]";
    spillSends->addComment(comments.str());

    bb->insertBefore(spillIt, spillSends);

    if (getEUFusionCallWAInsts().count(inst) > 0) {
      removeEUFusionCallWAInst(inst);
      addEUFusionCallWAInst(spillSends);
      if (hdrSetInst)
        addEUFusionCallWAInst(hdrSetInst);
    }

    if (kernel.getOption(vISA_GenerateDebugInfo)) {
      INST_LIST expandedInsts = {hdrSetInst, spillSends};
      kernel.getKernelDebugInfo()->updateExpandedIntrinsic(
          inst->asSpillIntrinsic(), expandedInsts);
    }

    numRowsOword -= payloadSizeInOwords;
    offsetOword += payloadSizeInOwords;
    rowOffsetOword += payloadSizeInOwords;
  }
}

// Non-stack call:
//  sends <-- scratch - default, supported
//  send  <-- scratch - disable split send using compiler option, not supported
//  by intrinsic send  <-- non-scratch - used when scratch space usage is very
//  high, supported

//  Stack call :
//  sends <-- non-scratch - default spill, supported
//  send  <-- non-scratch - default fill, supported
void GlobalRA::expandSpillIntrinsic(G4_BB *bb) {
  // spill (1) null:ud   bitmask:ud   r0:ud   payload:ud
  for (auto instIt = bb->begin(); instIt != bb->end();) {
    auto inst = (*instIt);
    if (inst->isSpillIntrinsic()) {
      bool isOffBP = inst->asSpillIntrinsic()->isOffBP();
      uint32_t numRows = inst->asSpillIntrinsic()->getNumRows();
      uint32_t offset = inst->asSpillIntrinsic()->getOffset() *
                        (builder.numEltPerGRF<Type_UB>() / HWORD_BYTE_SIZE);
      auto header = inst->getSrc(0)->asSrcRegRegion();
      auto payload = inst->getSrc(1)->asSrcRegRegion();
      auto spillIt = instIt;

      auto rowOffset = payload->getRegOff();
      if (useLscForNonStackCallSpillFill || spillFillIntrinUsesLSC(inst) ||
          useLscForScatterSpill) {
        if (inst->asSpillIntrinsic()->isScatterSpill())
          expandScatterSpillLSC(bb, instIt);
        else
          expandSpillLSC(bb, instIt);
      } else {
        if (!isOffBP) {
          expandSpillNonStackcall(numRows, offset, rowOffset, header, payload,
                                  bb, instIt);
        } else {
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

void GlobalRA::expandFillNonStackcall(uint32_t numRows, uint32_t offset,
                                      short rowOffset, G4_SrcRegRegion *header,
                                      G4_DstRegRegion *resultRgn, G4_BB *bb,
                                      INST_LIST_ITER &instIt) {
  auto &builder = kernel.fg.builder;
  auto inst = (*instIt);

  if (offset == G4_FillIntrinsic::InvalidOffset) {
    // oword msg
    G4_ExecSize execSize = g4::SIMD16;
    auto numRowsOword = GRFSizeToOwords(numRows, *builder);
    auto fillDst =
        builder->createDst(resultRgn->getBase(), rowOffset, 0,
                           resultRgn->getHorzStride(), resultRgn->getType());
    auto sendSrc0 =
        builder->createSrc(header->getBase(), 0, 0,
                           builder->rgnpool.createRegion(8, 8, 1), Type_UD);
    G4_Imm *desc = createMsgDesc(numRowsOword, false, false);
    G4_INST *sendInst = nullptr;
    auto sfId = SFID::DP_DC0;

    // Use bindless for XeHP_SDV and later
    if (builder->hasScratchSurface()) {
      // Update BTI to 251
      auto newDesc = desc->getInt() & 0xffffff00;
      newDesc |= 251;
      desc = builder->createImm(newDesc, Type_UD);

      auto msgDesc = builder->createReadMsgDesc(sfId, (uint32_t)desc->getInt());
      G4_Operand *msgDescOpnd = builder->createImm(msgDesc->getDesc(), Type_UD);

      // a0 is set by saveRestoreA0()
      auto src1 = builder->createSrc(builder->getBuiltinA0Dot2()->getRegVar(),
                                     0, 0, builder->getRegionScalar(), Type_UD);

      sendInst = builder->createInternalSplitSendInst(
          execSize, fillDst, sendSrc0, nullptr, msgDescOpnd,
          InstOpt_WriteEnable, msgDesc, src1);
    } else {
      auto msgDesc = builder->createReadMsgDesc(sfId, (uint32_t)desc->getInt());
      G4_Operand *msgDescOpnd = builder->createImm(msgDesc->getDesc(), Type_UD);
      sendInst = builder->createInternalSendInst(nullptr, G4_send, execSize,
                                                 fillDst, sendSrc0, msgDescOpnd,
                                                 InstOpt_WriteEnable, msgDesc);
    }
    instIt = bb->insertBefore(instIt, sendInst);

    if (EUFusionNoMaskWANeeded() && sendInst->isWriteEnableInst()) {
      addEUFusionNoMaskWAInst(bb, sendInst);
    }
  } else {
    INST_LIST_ITER origInstIt = instIt;
    insertSlot1HwordR0Set(bb, instIt);

    while (numRows >= 1) {
      auto fillDst =
          builder->createDst(resultRgn->getBase(), rowOffset, 0,
                             resultRgn->getHorzStride(), resultRgn->getType());

      auto region = builder->getRegionStride1();
      G4_SrcRegRegion *headerOpnd =
          builder->createSrcRegRegion(builder->getBuiltinR0(), region);

      uint32_t fillMsgDesc =
          computeFillMsgDesc(getPayloadSizeGRF(numRows), offset, *builder);

      G4_SendDescRaw *msgDesc = kernel.fg.builder->createSendMsgDesc(
          fillMsgDesc, getPayloadSizeGRF(numRows), 1, SFID::DP_DC0, 0, 0,
          SendAccess::READ_ONLY);

      G4_Imm *msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);

      auto sendInst = builder->createInternalSendInst(
          nullptr, G4_send, g4::SIMD16, fillDst, headerOpnd, msgDescImm,
          inst->getOption(), msgDesc);

      std::stringstream comments;
      comments << "scratch space fill: "
               << inst->getDst()->getTopDcl()->getName() << " from offset["
               << offset << "x32]";
      sendInst->addComment(comments.str());

      instIt = bb->insertBefore(instIt, sendInst);

      if (EUFusionNoMaskWANeeded() && sendInst->isWriteEnableInst()) {
        addEUFusionNoMaskWAInst(bb, sendInst);
      }

      numRows -= getPayloadSizeGRF(numRows);
      offset += getPayloadSizeGRF(numRows);
      rowOffset += getPayloadSizeGRF(numRows);
    }

    insertSlot1HwordR0Reset(bb, origInstIt);
  }
}

void GlobalRA::expandFillStackcall(uint32_t numRows, uint32_t offset,
                                   short rowOffset, G4_SrcRegRegion *header,
                                   G4_DstRegRegion *resultRgn, G4_BB *bb,
                                   INST_LIST_ITER &instIt) {
  auto &builder = kernel.fg.builder;
  auto inst = (*instIt);
  auto fillIt = instIt;

  // Use oword ld for stackcall. Lower intrinsic to:
  // add (1) r126.2<1>:d FP<0;1,0>:d offset
  //  send (16) r[startReg]<1>:uw r126 0xa desc:ud
  G4_Operand *src0 = nullptr;
  G4_Imm *src1 = nullptr;
  G4_Declare *scratchRegDcl = builder->kernel.fg.scratchRegDcl;
  G4_Declare *framePtr = inst->asFillIntrinsic()->getFP();

  // convert hword to oword offset
  auto numRowsOword = numRows * 2;
  auto offsetOword = offset * 2;
  auto rowOffsetOword = rowOffset * 2;

  while (numRowsOword >= 1) {
    auto createOwordFill = [&](unsigned int owordSize,
                               G4_DstRegRegion *fillVar) {
      G4_ExecSize execSize = (owordSize > 2) ? g4::SIMD16 : g4::SIMD8;
      auto sendSrc0 =
          builder->createSrc(scratchRegDcl->getRegVar(), 0, 0,
                             builder->rgnpool.createRegion(8, 8, 1), Type_UD);
      G4_Imm *desc = createMsgDesc(owordSize, false, false);
      G4_INST *sendInst = nullptr;
      auto sfId = SFID::DP_DC0;

      // Use bindless for XeHP_SDV and later
      if (builder->getPlatform() >= Xe_XeHPSDV) {
        // Update BTI to 251
        auto newDesc = desc->getInt() & 0xffffff00;
        newDesc |= 251;
        desc = builder->createImm(newDesc, Type_UD);

        auto msgDesc =
            builder->createReadMsgDesc(sfId, (uint32_t)desc->getInt());
        G4_Operand *msgDescOpnd =
            builder->createImm(msgDesc->getDesc(), Type_UD);

        // a0 is set by saveRestoreA0()
        auto src1 =
            builder->createSrc(builder->getBuiltinA0Dot2()->getRegVar(), 0, 0,
                               builder->getRegionScalar(), Type_UD);

        sendInst = builder->createInternalSplitSendInst(
            execSize, fillVar, sendSrc0, nullptr, msgDescOpnd,
            InstOpt_WriteEnable, msgDesc, src1);
      } else {
        auto msgDesc =
            builder->createReadMsgDesc(SFID::DP_DC0, (uint32_t)desc->getInt());
        auto msgDescImm = builder->createImm(msgDesc->getDesc(), Type_UD);
        sendInst = builder->createInternalSendInst(
            nullptr, G4_send, execSize, fillVar, sendSrc0, msgDescImm,
            InstOpt_WriteEnable, msgDesc);
      }
      return sendInst;
    };

    auto respSizeInOwords = getPayloadSizeOword(numRowsOword);
    auto fillDst =
        builder->createDst(resultRgn->getBase(), rowOffsetOword / 2, 0,
                           resultRgn->getHorzStride(), resultRgn->getType());

    G4_DstRegRegion *dst =
        builder->createDst(scratchRegDcl->getRegVar(), 0, 2, 1, Type_UD);

    G4_INST *hdrSetInst = nullptr;
    if (inst->asFillIntrinsic()->isOffsetValid()) {
      // Skip header if spill module emits its own header
      if (framePtr) {
        src0 = builder->createSrc(framePtr->getRegVar(), 0, 0,
                                  builder->getRegionScalar(), Type_UD);
        src1 = builder->createImm(offsetOword, Type_UD);
        hdrSetInst = builder->createBinOp(G4_add, g4::SIMD1, dst, src0, src1,
                                          InstOpt_WriteEnable, false);
      } else {
        src0 = builder->createImm(offsetOword, Type_UD);
        hdrSetInst = builder->createMov(g4::SIMD1, dst, src0,
                                        InstOpt_WriteEnable, false);
      }

      bb->insertBefore(fillIt, hdrSetInst);
    }

    auto fillSends = createOwordFill(respSizeInOwords, fillDst);

    if (getEUFusionCallWAInsts().count(inst) > 0) {
      removeEUFusionCallWAInst(inst);
      addEUFusionCallWAInst(fillSends);
      if (hdrSetInst)
        addEUFusionCallWAInst(hdrSetInst);
    }

    std::stringstream comments;
    comments << "stack fill: " << resultRgn->getTopDcl()->getName()
             << " from FP[" << inst->asFillIntrinsic()->getOffset() << "x32]";
    fillSends->addComment(comments.str());

    bb->insertBefore(fillIt, fillSends);

    if (kernel.getOption(vISA_GenerateDebugInfo)) {
      INST_LIST expandedInsts = {hdrSetInst, fillSends};
      kernel.getKernelDebugInfo()->updateExpandedIntrinsic(
          inst->asFillIntrinsic(), expandedInsts);
    }

    numRowsOword -= respSizeInOwords;
    offsetOword += respSizeInOwords;
    rowOffsetOword += respSizeInOwords;
  }
}

bool GlobalRA::spillFillIntrinUsesLSC(G4_INST *spillFillIntrin) {
  G4_Declare *headerDcl = nullptr;
  if (!spillFillIntrin)
    return false;

  if (spillFillIntrin->isFillIntrinsic())
    headerDcl = spillFillIntrin->asFillIntrinsic()->getHeader()->getTopDcl();
  else if (spillFillIntrin->isSpillIntrinsic())
    headerDcl = spillFillIntrin->asSpillIntrinsic()->getHeader()->getTopDcl();

  if (useLscForSpillFill &&
      headerDcl != builder.getBuiltinR0()->getRootDeclare()) {
    return true;
  }
  return false;
}

void GlobalRA::markSlot1HwordSpillFill(G4_BB *bb) {
  if (useLscForNonStackCallSpillFill ||
      !kernel.getBoolKernelAttr(Attributes::ATTR_SepSpillPvtSS))
    return;

  bool isSet = false;
  G4_INST *prevSetInst = nullptr;
  G4_Declare *builtinR0 = builder.getBuiltinR0()->getRootDeclare();
  unsigned builtinR0RegNum =
      builtinR0->getRegVar()->getPhyReg()->asGreg()->getRegNum();

  for (auto instIt = bb->begin(); instIt != bb->end(); ++instIt) {
    G4_INST *inst = (*instIt);
    if (inst->isFillIntrinsic()) {
      G4_FillIntrinsic *fillInst = inst->asFillIntrinsic();

      if (!spillFillIntrinUsesLSC(inst) && !fillInst->isOffBP() &&
          fillInst->isOffsetValid()) {
        if (!isSet) {
          slot1SetR0.insert(inst);
          isSet = true;
        }
        prevSetInst = inst;

        // Special case of builtin r0 being overwritten by the fill instruction.
        // In this case the register shouldn't be reset from slot1 after the
        // fill because the physical register got reused and builtin r0 is no
        // longer alive.
        // Example IR illustrating the problem:
        //   //.declare FL (r0.0)
        //   (W) intrinsic.fill.1 (8)  FL(0,0)<1>:ud r0.0<1;1,0>:ud
        //                             Scratch[0x32]
        //       mov (8)               M2(0,0)<1>:f FL(0,0)<1;1,0>:f
        //       mov (8)               M2(1,0)<1>:f FL(0,0)<1;1,0>:f
        //       mov (8)               M2(2,0)<1>:f FL(0,0)<1;1,0>:f
        //       mov (8)               M2(3,0)<1>:f FL(0,0)<1;1,0>:f
        //       sendsc (8)            null:ud M2(0,0) null 0x25:ud
        //                             0x8031400:ud {EOT} // render target write
        G4_VarBase *dstVarBase = fillInst->getDst()->getBase();

        vASSERT(dstVarBase);
        G4_VarBase *dstPhyReg = dstVarBase->asRegVar()->getPhyReg();

        vASSERT(dstPhyReg);
        unsigned dstRegNum = dstPhyReg->asGreg()->getRegNum();

        if (dstRegNum <= builtinR0RegNum &&
            builtinR0RegNum < dstRegNum + fillInst->getNumRows()) {
          isSet = false;
          prevSetInst = nullptr;
        }
      }
    } else if (inst->isSpillIntrinsic()) {
      G4_SpillIntrinsic *spillInst = inst->asSpillIntrinsic();

      if (!spillFillIntrinUsesLSC(inst) && !spillInst->isOffBP() &&
          spillInst->isOffsetValid()) {
        if (!isSet) {
          slot1SetR0.insert(inst);
          isSet = true;
        }
        prevSetInst = inst;
      }
    } else if (isSet) {
      // Check instruction for r0 usage.
      // If r0 is used, add previous spill/fill instruction to reset list.
      if (G4_Operand *dstOpnd = inst->getDst()) {
        G4_Declare *dstDecl = dstOpnd->getTopDcl();
        if (dstDecl && dstDecl->getRootDeclare() == builtinR0) {
          isSet = false;
          break;
        }
      }

      unsigned srcNum = static_cast<unsigned>(inst->getNumSrc());
      for (unsigned srcIdx = 0; srcIdx < srcNum; ++srcIdx) {
        G4_Operand *srcOpnd = inst->getSrc(srcIdx);
        G4_Declare *srcDecl = srcOpnd->getTopDcl();
        if (srcDecl && srcDecl->getRootDeclare() == builtinR0) {
          isSet = false;
          break;
        }
      }

      if (!isSet) {
        slot1ResetR0.insert(prevSetInst);
      }
    }
  }

  // End of the block with r0.5 pointing to slot1,
  // mark last spill/fill to reset it.
  if (isSet) {
    slot1ResetR0.insert(prevSetInst);
  }
}


void GlobalRA::expandFillIntrinsic(G4_BB *bb) {
  // fill (1) fill_var:ud     bitmask:ud     offset:ud
  for (auto instIt = bb->begin(); instIt != bb->end();) {
    auto inst = (*instIt);
    if (inst->isFillIntrinsic()) {
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
      } else {
        if (!isOffBP) {
          expandFillNonStackcall(numRows, offset, rowOffset, header, resultRgn,
                                 bb, instIt);
        } else {
          expandFillStackcall(numRows, offset, rowOffset, header, resultRgn, bb,
                              instIt);
        }
      }
      numGRFFill++;
      instIt = bb->erase(fillIt);
      continue;
    }
    instIt++;
  }
}


// Initialize address for immediate offset usage in LSC spill/fill messages
void GlobalRA::initAddrRegForImmOffUseNonStackCall() {
  // create a tmp register and store value 0x10000 for immediate offset usage
  // in non-stackcall spill/fill
  //    mov spillHeader 0x10000
  G4_BB *entryBB = builder.kernel.fg.getEntryBB();
  auto iter = std::find_if(entryBB->begin(), entryBB->end(),
                           [](G4_INST *inst) { return !inst->isLabel(); });

  auto movInst = builder.createMov(
      g4::SIMD1, builder.createDstRegRegion(builder.getSpillFillHeader(), 1),
      builder.createImm(0x10000, Type_UD), InstOpt_WriteEnable, false);
  entryBB->insertBefore(iter, movInst);
}

void GlobalRA::expandSpillFillIntrinsics(unsigned int spillSizeInBytes) {

  auto globalScratchOffset =
      kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
  bool hasStackCall =
      kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc();

  // turn off immediate offset if the spill size is larger than 128k for non
  // stack call. Such test should be rare and don't think it needs to be fast.
  if (!hasStackCall &&
      ((spillSizeInBytes + globalScratchOffset) > SPILL_FILL_IMMOFF_MAX * 2))
    canUseLscImmediateOffsetSpillFill = false;

  // No need to init address reg for immediate offset usage if there is no
  // scratch message
  if (canUseLscImmediateOffsetSpillFill &&
      ((!hasStackCall && spillSizeInBytes > 0)))
    initAddrRegForImmOffUseNonStackCall();


  for (auto bb : kernel.fg) {
    if (builder.hasScratchSurface() &&
        (hasStackCall || kernel.fg.builder->hasValidOldA0Dot2() ||
         (useLscForSpillFill &&
          (spillSizeInBytes + globalScratchOffset) >= SCRATCH_MSG_LIMIT &&
          spillSizeInBytes > 0) ||
         (useLscForNonStackCallSpillFill && spillSizeInBytes > 0)
    // Following cases exist:
    // a. XeHP_SDV without stackcall => use hword scratch msg
    // b. XeHP_SDV without stackcall => using oword block msg
    // c. XeHP_SDV with stackcall
    // d. DG2+ without stackcall => hword scratch msg (illegal in Xe2+)
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
             )) {
      saveRestoreA0(bb);
    }
    markSlot1HwordSpillFill(bb);
    expandSpillIntrinsic(bb);
    expandFillIntrinsic(bb);
  }
}

BoundedRA::BoundedRA(GlobalRA &ra, const LiveRangeVec *l)
    : gra(ra), kernel(ra.kernel), lrs(l) {}

void vISA::BoundedRA::markBusyGRFs() {
  vASSERT(lrs);
  auto dst = curInst->getDst();
  if (dst && dst->isDstRegRegion() && dst->getTopDcl() &&
      dst->getTopDcl()->useGRF()) {
    if (!dst->isIndirect()) {
      auto id = dst->getTopDcl()->getRegVar()->getId();
      bool isPartaker = dst->getTopDcl()->getRegVar()->isRegAllocPartaker();
      auto phyReg = dst->getTopDcl()->getRegVar()->getPhyReg();
      if (!phyReg && isPartaker)
        phyReg = (*lrs)[id]->getPhyReg();
      if (phyReg && phyReg->isGreg()) {
        auto regLB =
            phyReg->asGreg()->getRegNum() * kernel.numEltPerGRF<Type_UB>();
        regLB += dst->getTopDcl()->getRegVar()->getPhyRegOff() *
                 TypeSize(dst->getType());
        regLB += dst->getLeftBound();
        auto regRB = regLB + dst->getRightBound() - dst->getLeftBound();
        auto startGRF = regLB / kernel.numEltPerGRF<Type_UB>();
        auto endGRF = regRB / kernel.numEltPerGRF<Type_UB>();
        for (unsigned int reg = startGRF; reg != (endGRF + 1); ++reg)
          markGRF(reg);
      } else if (isPartaker)
        markForbidden((*lrs)[id]);
    } else if (dst->isIndirect()) {
      auto &p2a = gra.pointsToAnalysis;
      auto pointees = p2a.getAllInPointsTo(dst->getTopDcl()->getRegVar());
      for (auto &pointee : *pointees) {
        if (!pointee.var->isRegAllocPartaker())
          continue;
        auto id = pointee.var->getId();
        auto phyReg = pointee.var->getPhyReg();
        if (!phyReg)
          phyReg = (*lrs)[id]->getPhyReg();
        if (phyReg) {
          auto regLB =
              phyReg->asGreg()->getRegNum() * kernel.numEltPerGRF<Type_UB>();
          auto regRB =
              regLB +
              pointee.var->getDeclare()->getRootDeclare()->getByteSize();
          auto startGRF = regLB / kernel.numEltPerGRF<Type_UB>();
          auto endGRF = regRB / kernel.numEltPerGRF<Type_UB>();
          for (unsigned int reg = startGRF; reg != (endGRF + 1); ++reg)
            markGRF(reg);
        } else
          markForbidden((*lrs)[id]);
      }
    }
  }

  for (unsigned int i = 0; i != curInst->getNumSrc(); ++i) {
    auto src = curInst->getSrc(i);
    if (src && src->isSrcRegRegion()) {
      if (!src->asSrcRegRegion()->isIndirect() &&
          src->asSrcRegRegion()->getTopDcl() &&
          src->asSrcRegRegion()->getTopDcl()->useGRF()) {
        auto id = src->getTopDcl()->getRegVar()->getId();
        bool isPartaker = src->getTopDcl()->getRegVar()->isRegAllocPartaker();
        auto phyReg = src->getTopDcl()->getRegVar()->getPhyReg();
        if (!phyReg && isPartaker)
          phyReg = (*lrs)[id]->getPhyReg();
        if (phyReg && phyReg->isGreg()) {
          auto regLB =
              phyReg->asGreg()->getRegNum() * kernel.numEltPerGRF<Type_UB>();
          regLB += src->getTopDcl()->getRegVar()->getPhyRegOff() *
                   TypeSize(src->getType());
          regLB += src->getLeftBound();
          auto regRB = regLB + src->getRightBound() - src->getLeftBound();
          auto startGRF = regLB / kernel.numEltPerGRF<Type_UB>();
          auto endGRF = regRB / kernel.numEltPerGRF<Type_UB>();
          for (unsigned int reg = startGRF; reg != (endGRF + 1); ++reg)
            markGRF(reg);
        } else if (isPartaker)
          markForbidden((*lrs)[id]);
      } else if (src->asSrcRegRegion()->isIndirect()) {
        auto &p2a = gra.pointsToAnalysis;
        auto pointees = p2a.getAllInPointsTo(src->getTopDcl()->getRegVar());
        for (auto &pointee : *pointees) {
          if (!pointee.var->isRegAllocPartaker())
            continue;
          auto id = pointee.var->getId();
          auto phyReg = pointee.var->getPhyReg();
          if (!phyReg)
            phyReg = (*lrs)[id]->getPhyReg();
          if (phyReg) {
            auto regLB =
                phyReg->asGreg()->getRegNum() * kernel.numEltPerGRF<Type_UB>();
            auto regRB =
                regLB +
                pointee.var->getDeclare()->getRootDeclare()->getByteSize();
            auto startGRF = regLB / kernel.numEltPerGRF<Type_UB>();
            auto endGRF = regRB / kernel.numEltPerGRF<Type_UB>();
            for (unsigned int reg = startGRF; reg != (endGRF + 1); ++reg)
              markGRF(reg);
          } else
            markForbidden((*lrs)[id]);
        }
      }
    }
  }
}

void BoundedRA::markUniversalForbidden() {
  vASSERT(lrs);
  auto markPhyReg = [&](G4_Declare *dcl) {
    if (dcl->getRegVar() && dcl->getRegVar()->getPhyReg())
      markGRF(dcl->getRegVar()->getPhyReg()->asGreg()->getRegNum());
    else if (dcl->getRegVar()->isRegAllocPartaker())
      if (auto reg = (*lrs)[dcl->getRegVar()->getId()]->getPhyReg())
        markGRF(reg->asGreg()->getRegNum());
  };
  // r0 is special and shouldn't be allocated to temps
  markGRF(0);

  // BuiltInR0 may be needed by spill/fill instruction
  markPhyReg(kernel.fg.builder->getBuiltinR0());

  // spill/fill header may be clobbered by spill/fill instruction
  if (gra.builder.hasValidSpillFillHeader()) {
    markPhyReg(kernel.fg.builder->getSpillFillHeader());
  }

  // a0.2 may be copied to special GRF to preserve it across
  // the spill/fill instruction. So forbid allocation to temp
  // GRF that holds old a0.2 value.
  if (gra.builder.hasValidOldA0Dot2()) {
    markPhyReg(kernel.fg.builder->getOldA0Dot2Temp());
  }
}

void BoundedRA::markForbidden(LiveRange *lr) {
  auto totalRegs = gra.kernel.getNumRegTotal();
  const BitSet *forbidden = lr->getForbidden();
  // We've 0 reserved GRFs if an RA iteration was converted
  // to fail safe. But we may have non-zero reserved GRFs
  // if fail safe was set before running RA iteration.
  auto numReserved =
      (reservedGRFStart == NOT_FOUND) ? 0 : gra.getNumReservedGRFs();
  auto isReservedGRF = [&](unsigned int reg) {
    return (numReserved > 0 && reg >= reservedGRFStart &&
            reg < (reservedGRFStart + numReserved));
  };
  for (unsigned int i = 0; i != totalRegs; ++i) {
    if (!isReservedGRF(i) && forbidden && forbidden->isSet(i))
      markGRF(i);
  }

  markUniversalForbidden();
}

void BoundedRA::markGRFs(unsigned int reg, unsigned int num) {
  for (unsigned int r = reg; r != (reg + num); ++r) {
    markGRF(r);
    // Reserved  GRFs shouldnt be marked as clobbered
    if (reservedGRFStart != NOT_FOUND && reg >= reservedGRFStart &&
        reg < (reservedGRFStart + gra.getNumReservedGRFs()))
      continue;
    clobberedGRFs[curInst].insert(r);
  }
}

void BoundedRA::insertPushPop(bool useLSCMsg) {
  // List of clobbered GRFs is available.
  // prev and next iters are available.
  // Insert push (spill) and pop (fill) from clobbered physical GRFs here.
  auto cIt = clobberedGRFs.find(curInst);
  if (cIt == clobberedGRFs.end() || (*cIt).second.size() == 0)
    return;

  auto &clobbered = clobberedGRFs[curInst];
  // list of <leading GRF, # GRFs>
  std::list<std::pair<unsigned int, unsigned int>> segments;
  unsigned int maxGRFSpillFill = 4;
  // HWord message supports max size of 8 HWords.
  // For platforms with 32-byte GRF, 8GRF r/w is supported.
  // 8GRFs are supported for HWord and OWord messages.
  if (kernel.getGRFSize() == 32)
    maxGRFSpillFill = 8;
  // Stack call functions never use HWord message. So r/w of
  // 8 GRFs is supported when using stack call with LSC when
  // GRF size > 32 bytes.
  else if ((kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc()) &&
           useLSCMsg)
    maxGRFSpillFill = 8;
  // Non-stack call kernel may use either HWord scratch message or LSC.
  // If kernel uses LSC then max supported r/w size is 8 GRFs.
  else if (gra.useLscForNonStackCallSpillFill)
    maxGRFSpillFill = 8;

  auto segmentClobbered = [&]() {
    for (auto grf : clobbered) {
      if (segments.size() > 0 &&
          (segments.back().first + segments.back().second) == grf) {
        segments.back().second += 1;
        continue;
      }
      segments.push_back(std::make_pair(grf, 1));
    }

    // Ensure each segment size is power of 2
    for (auto it = segments.begin(); it != segments.end();) {
      auto &sz = (*it).second;
      if (sz == 1 || sz == 2 || sz == 4 || (sz == 8 && maxGRFSpillFill >= 8)) {
        ++it;
        continue;
      }
      // non-power of 2 found
      auto grf = (*it).first;
      if (sz > 8 && maxGRFSpillFill >= 8) {
        it = segments.insert(it, std::make_pair(grf + 8, sz - 8));
        sz = 8;
        continue;
      } else if (sz > 4) {
        it = segments.insert(it, std::make_pair(grf + 4, sz - 4));
        sz = 4;
        continue;
      } else if (sz > 2) {
        it = segments.insert(it, std::make_pair(grf + 2, sz - 2));
        sz = 2;
        continue;
      }
      ++it;
    }
  };

  // Aim to minimize # spill/fill instructions.
  segmentClobbered();

  G4_Declare *fp =
      kernel.fg.builder->usesStack() ? kernel.fg.getFramePtrDcl() : nullptr;
  auto *builder = kernel.fg.builder;
  const unsigned int DclSize = 16;

  for (auto &segment : segments) {
    G4_SrcRegRegion *headerOpnd = nullptr;
    G4_SrcRegRegion *headerOpnd1 = nullptr;
    G4_INST *movInst = nullptr;

    unsigned int off = spillOffset;
    off += segment.first * kernel.numEltPerGRF<Type_UB>();
    if (off < SCRATCH_MSG_LIMIT && !gra.useLscForNonStackCallSpillFill) {
      // Stack call, HWord cases can take BuiltInR0 as header
      headerOpnd = builder->createSrcRegRegion(builder->getBuiltinR0(),
                                               builder->getRegionStride1());
      headerOpnd1 = builder->createSrcRegRegion(builder->getBuiltinR0(),
                                                builder->getRegionStride1());
      off = off >> SCRATCH_SPACE_ADDRESS_UNIT;
    } else if (useLSCMsg) {
      // LSC needs its own spill/fill header
      headerOpnd = getSpillFillHeader(*builder, nullptr);
      headerOpnd1 = getSpillFillHeader(*builder, nullptr);
      off = off >> SCRATCH_SPACE_ADDRESS_UNIT;
    } else {
      // off is beyond HWord addressable range
      // LSC is not available on platform
      // Use OWord. Header needs to be initialized separately when using OWord.
      vISA_ASSERT(reservedGRFStart != NOT_FOUND,
                  "expecting valid reserved GRF");
      auto *immOpnd = builder->createImm(off / OWORD_BYTE_SIZE, Type_UD);
      auto *tmp = builder->createDeclare(
          "TMP", G4_GRF, kernel.numEltPerGRF<Type_UD>(), 1, Type_UD);
      tmp->getRegVar()->setPhyReg(builder->phyregpool.getGreg(reservedGRFStart),
                                  0);
      auto *dstMov = builder->createDstRegRegion(tmp, 1);
      movInst = builder->createMov(G4_ExecSize(1), dstMov, immOpnd,
                                   InstOpt_WriteEnable, false);
      movInst->inheritDIFrom(curInst);
      headerOpnd =
          builder->createSrcRegRegion(tmp, builder->getRegionStride1());
      headerOpnd1 =
          builder->createSrcRegRegion(tmp, builder->getRegionStride1());
      off = G4_SpillIntrinsic::InvalidOffset;
    }

    G4_ExecSize execSize(16);
    const char *dclName = builder->getNameString(DclSize, "PUSH%d_%d",
                                                 segment.first, segment.second);
    G4_Declare *tmp = builder->createDeclare(
        dclName, G4_GRF, kernel.numEltPerGRF<Type_D>(), segment.second, Type_D);
    tmp->getRegVar()->setPhyReg(builder->phyregpool.getGreg(segment.first), 0);

    // Push
    G4_DstRegRegion *postDst = builder->createNullDst(Type_UD);
    auto srcOpnd = builder->createSrc(tmp->getRegVar(), 0, 0,
                                      builder->getRegionStride1(), Type_D);

    auto spill = builder->createSpill(postDst, headerOpnd, srcOpnd, execSize,
                                      segment.second, off, fp,
                                      InstOpt_WriteEnable, false);

    curBB->insertAfter(prev, spill, false);
    spill->inheritDIFrom(curInst);
    if (movInst)
      curBB->insertAfter(prev, movInst, false);

    if (!(next == curBB->end() && curBB->back()->isEOT())) {
      // Pop
      if (movInst)
        curBB->insertBefore(next, movInst->cloneInst(), false);

      auto dst = builder->createDst(tmp->getRegVar(), 0, 0, 1, Type_D);
      auto fill =
          builder->createFill(headerOpnd1, dst, execSize, segment.second, off,
                              fp, InstOpt_WriteEnable, false);

      curBB->insertBefore(next, fill, false);
      fill->inheritDIFrom(curInst);
    }
  }

  // Reset clobbered after inserting push/pop
  clobbered.clear();
}

void BoundedRA::markIndirIntfs() {
  // In this function we create ref list of all addr
  // dcls and instructions where they appear.
  //
  // A0 -> {List of instructions where r[A0] occurs}
  // A1 -> {List of instructions where r[A1] occurs}

  for (auto bb : kernel.fg) {
    for (INST_LIST_ITER inst_it = bb->begin(); inst_it != bb->end();
         inst_it++) {
      G4_INST *curInst = (*inst_it);

      // Handle indirect destination
      G4_DstRegRegion *dst = curInst->getDst();

      if (dst && dst->getRegAccess() == IndirGRF) {
        setInst(curInst, bb);

        auto topdcl = dst->getTopDcl();
        busyIndir[topdcl].push_back(curInst);
      }

      for (int i = 0, numSrc = curInst->getNumSrc(); i < numSrc; i++) {
        G4_Operand *src = curInst->getSrc(i);

        if (src && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getRegAccess() == IndirGRF) {
          setInst(curInst, bb);

          auto topdcl = src->asSrcRegRegion()->getTopDcl();
          busyIndir[topdcl].push_back(curInst);
        }
      }
    }
  }
}

bool BoundedRA::isFreeIndir(unsigned int r) {
  // A0 = &V10
  // A1 = &V11
  // A2 = &V12
  //
  // Assume V10, V11, V12 are spilled
  //
  //                             Busy:
  // r1 = r[A1] + r2             r1, r2 -  (1)
  // r5 = r[A2] + r10            r5, r10 - (2)
  // r20 = r[A1] + r3            r20, r3 - (3)
  // r[A2] = r[A1] + r6          r6      - (4)
  // r[A3] = r[A2] + r12         r12     - (5)
  //
  // Busy for r[A1] -> (1) + (3) + (4)
  // Busy for r[A2] -> (2) + (4) + (5)
  // Busy for r[A3] -> (5)

  // When choosing a GRF to use for an indirect
  // operand, we need to lookup a data structure to
  // find free GRFs. It is not sufficient to find free
  // GRF from current instruction only because the
  // chosen GRF has to be free in all instructions
  // where the indirect appears. For eg, in above
  // sequence we cannot use r6 to load r[A1] in (1)
  // even though r6 is not busy in (1) - because it's
  // busy in (4).

  vISA_ASSERT(addrDcl, "expecting non-nullptr addrDcl");
  auto &refs = busyIndir[addrDcl];

  for (auto ref : refs) {
    if (!isFreeGRFOtherInst(r, ref))
      return false;
  }

  return true;
}
